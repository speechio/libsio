#ifndef SIO_BEAM_SEARCH_H
#define SIO_BEAM_SEARCH_H

#include <string.h>
#include <limits>

#include "sio/base.h"
#include "sio/allocator.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_transducer.h"
#include "sio/language_model.h"

namespace sio {

enum class SearchStatus : int {
    kUnconstructed,
    kIdle,
    kBusy,
    kDone,
};


struct BeamSearchConfig {
    bool debug = false;

    f32 beam = 16.0;
    i32 max_active = 12;
    f32 token_set_size = 1;

    i32 nbest = 1;

    f32 insertion_penalty = 0.0;
    bool apply_score_offsets = true;  // for numerical stability of long audio scores

    i32 token_allocator_slab_size = 4096;


    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".debug", &debug);

        loader->AddEntry(module + ".beam", &beam);
        loader->AddEntry(module + ".max_active", &max_active);
        loader->AddEntry(module + ".token_set_size", &token_set_size);

        loader->AddEntry(module + ".nbest", &nbest);

        loader->AddEntry(module + ".insertion_penalty", &insertion_penalty);
        loader->AddEntry(module + ".apply_score_offsets", &apply_score_offsets);

        loader->AddEntry(module + ".token_allocator_slab_size", &token_allocator_slab_size);

        return Error::OK;
    }
};


/* 
 * Typical rescoring language models are:
 *   1. Lookahead-LM or Internal-LM subtractor
 *   2. Big-LM or External-LM
 *   3. Specific Domain-LM
 *   4. Hotfix-LM (sometimes also called hint, hot-word/hot-phrase)
 * These LMs are normally represented as *Deterministic Fsa*, 
 * so that shallow-fusion based contextual biasing can be applied 
 * via on-the-fly rescoring.
 */
#define SIO_MAX_LM 5


/*
 * StateHandle represents a unique state in decoding graph during beam search.
 *
 *   For single-graph decoding: StateHandle = FstStateId. e.g.:
 *       * T (vanilla CTC)
 *       * TLG (CTC with lexicon & external LM)
 *       * HCLG (WFST)
 *
 *   For multi-graph decoding: say StateHandle = 64-bits(32 + 32) integer:
 *       1st 32 bits represent a Fst
 *       2nd 32 bits represent a state inside that Fst
 */
using StateHandle = FstStateId;
//using StateHandle = u64;

static inline StateHandle ComposeStateHandle(int graph, FstStateId state) {
    return state;
    //return (static_cast<StateHandle>(graph) << 32) + static_cast<StateHandle>(state);
}
static inline int HandleToGraph(StateHandle h) {
    return 0;
    //return static_cast<int>(static_cast<u32>(h >> 32));
}
static inline FstStateId HandleToState(StateHandle h) {
    return h;
    //return static_cast<FstStateId>(static_cast<u32>(h))
}


struct Token;
struct TokenSet;

struct TraceBack {
    Token* token = nullptr;
    FstArc arc;
    f32 score = 0.0;
    LmScore lm_scores[SIO_MAX_LM] = {}; // zero initialized to 0.0
};


struct Token {
    Nullable<Token*> next = nullptr; // nullptr -> last token in a TokenSet
    //TokenSet* master = nullptr;

    f32 total_score = 0.0;
    LmStateId lm_states[SIO_MAX_LM] = {}; // zero initialized to 0 
    TraceBack trace_back;
};


// TokenSet represents a location(time, state handle) in beam search space (sometimes called trellis space),
// Each TokenSet holds a list of tokens representing search hypotheses
struct TokenSet {
    Nullable<Token*> head = nullptr; // nullptr -> TokenSet pruned or inactive

    f32 best_score = std::numeric_limits<f32>::lowest();
    int time = 0;
    StateHandle handle = 0;
};


class BeamSearch {
    BeamSearchConfig config_;
    const Fst* graph_ = nullptr;
    const Tokenizer* tokenizer_ = nullptr;
    vec<LanguageModel> lms_;

    str session_key_ = "default_session";
    SearchStatus status_ = SearchStatus::kUnconstructed;

    // lattice indexes: 
    //   [time, token_set_index]
    // invariant of time & frame indexing:
    //   {time=k} ---[frame=k]---> {time=k+1}
    // where: k ~ [0, total_frames)
    vec<vec<TokenSet>> lattice_;
    SlabAllocator<Token> token_arena_;

    // search frontier
    int cur_time_ = 0;  // frontier location on time axis
    vec<TokenSet> frontier_;
    hashtab<StateHandle, int> frontier_map_;  // search state handle -> token set index in frontier
    vec<int> eps_queue_;

    // beam
    f32 score_max_ = 0.0;
    f32 score_cutoff_ = 0.0;

    vec<f32> score_offsets_;  // keep hypotheses scores in a good dynamic range

    vec<vec<TokenId>> nbest_;

public:

    Error Load(const BeamSearchConfig& config, const Fst& graph, const Tokenizer& tokenizer) {
        SIO_CHECK(status_ == SearchStatus::kUnconstructed);

        config_ = config; // make a copy to block outside changes 

        SIO_CHECK(graph_ == nullptr);
        graph_ = &graph;

        SIO_CHECK(tokenizer_ == nullptr);
        tokenizer_ = &tokenizer;

        SIO_CHECK(lms_.empty());
        lms_.resize(1);
        lms_[0].LoadPrefixTreeLm();

        status_ = SearchStatus::kIdle;

        return Error::OK;
    }


    Error Push(const torch::Tensor score) {
        SIO_CHECK_EQ(score.dim(), 1); // should be one frame per each Push() call site

        SIO_CHECK(status_ == SearchStatus::kIdle || status_ == SearchStatus::kBusy);
        if (status_ == SearchStatus::kIdle) {
            InitSession();
            OnSessionBegin();
        }
        SIO_CHECK(status_ == SearchStatus::kBusy);

        OnFrameBegin();
        {
            FrontierExpandEmitting(score.data_ptr<float>());
            FrontierExpandEps();
            FrontierPrune();
            FrontierPinDown();
        }
        OnFrameEnd();

        return Error::OK;
    }


    Error PushEos() {
        SIO_CHECK(status_ == SearchStatus::kBusy);
        FrontierExpandEos();
        TraceBestPath();
        SIO_CHECK(status_ == SearchStatus::kDone);

        OnSessionEnd();

        return Error::OK;
    }


    const vec<vec<TokenId>>& NBest() {
        return nbest_;
    }


    Error Clear() {
        SIO_CHECK(status_ == SearchStatus::kDone);
        DeinitSession();
        SIO_CHECK(status_ == SearchStatus::kIdle);

        return Error::OK; 
    }

private:

    inline Token* NewToken(const Token* copy_from = nullptr) {
        Token* p = token_arena_.Alloc();
        if (copy_from == nullptr) {
            new (p) Token(); // placement new via default constructor
        } else {
            *p = *copy_from; // POD copy
        }
        return p;
    }


    inline void DeleteToken(Token *p) {
        //p->~Token();
        token_arena_.Free(p);
    }


    inline void ClearTokenSet(TokenSet *ts) {
        Token* t = ts->head;
        while (t != nullptr) {
            Token* next = t->next;
            DeleteToken(t);
            t = next;
        }
        ts->head = nullptr;
    }


    inline int FindOrAddTokenSet(int t, StateHandle h) {
        SIO_CHECK_EQ(cur_time_, t);

        int k;
        auto it = frontier_map_.find(h);
        if (it == frontier_map_.end()) {
            TokenSet ts;
            ts.time = t;
            ts.handle = h;

            k = frontier_.size();
            frontier_.push_back(ts);
            frontier_map_.insert({h, k});
        } else {
            k = it->second;
        }

        return k;
    }


    inline bool ContextEqual(const Token& x, const Token& y) {
        for (int i = 0; i != lms_.size(); i++) {
            if (x.lm_states[i] != y.lm_states[i]) {
                return false;
            }
        }
        return true;
    }


    bool TokenPassing(const TokenSet& src, const FstArc& arc, f32 score, TokenSet* dst) {
        bool changed = false; // dst token set is changed

        for (const Token* t = src.head; t != nullptr; t = t->next) {
            // most tokens won't survive pruning and context recombination,
            // here we use a "new token" on stack for probing, 
            // and a heap-based copy is created only after its actual survival.
            Token nt;

            // 1. graph & AM score
            nt.total_score = t->total_score + arc.score + score;

            // 2. LM
            if (arc.olabel == kFstEps) {
                memcpy(nt.lm_states, t->lm_states, sizeof(LmStateId) * lms_.size());
            } else {  /* word-end arc */
                for (int i = 0; i != lms_.size(); i++) {
                    LanguageModel& lm = lms_[i];

                    LmScore& lm_score = nt.trace_back.lm_scores[i];
                    lm_score = lm.GetScore(t->lm_states[i], arc.olabel, &nt.lm_states[i]);
                    nt.total_score += lm_score;
                }
                nt.total_score -= config_.insertion_penalty;
            }

            // 3. trace back 
            // this can be moved to back for optimization, keep it here for simplicity
            nt.trace_back.token = const_cast<Token*>(t);
            nt.trace_back.arc = arc;
            nt.trace_back.score = score;

            // beam pruning
            if (nt.total_score < score_cutoff_) {
                continue;
            } else if (nt.total_score > score_max_) {  // high enough to lift current beam range
                score_cutoff_ += (nt.total_score - score_max_);
                score_max_ = nt.total_score;
            }

            // context recombination
            bool survived = true;
            {
                int k;
                Token** p;
                for (k = 0, p = &dst->head; k < config_.token_set_size && *p != nullptr; k++, p = &(*p)->next) {
                    if (ContextEqual(**p, nt)) {
                        if ((*p)->total_score < nt.total_score) {  // existing token is worse, remove it
                            Token *next = (*p)->next;
                            DeleteToken(*p);
                            *p = next;

                            changed = true;
                        } else {  // existing token is better, kill new token
                            survived = false;
                        }

                        break;
                    }
                }
            }

            if (survived) {
                int k;
                Token** p;
                for (k = 0, p = &dst->head; k < config_.token_set_size && *p != nullptr; k++, p = &(*p)->next) {
                    if ((*p)->total_score <= nt.total_score) {
                        break;
                    }
                }

                if (k != config_.token_set_size) {
                    Token* q = NewToken(&nt); // actual heap copy to insert

                    q->next = *p;
                    *p = q;

                    changed = true;
                }
            }

        } // for each token in src token set

        if (changed) {
            dst->best_score = dst->head->total_score;
        }

        return changed;
    }


    Error InitSession() {
        SIO_CHECK_EQ(token_arena_.NumUsed(), 0);
        token_arena_.SetSize(config_.token_allocator_slab_size);

        SIO_CHECK(lattice_.empty());
        lattice_.reserve(25 * 30); // 25 frame_rates(subsample = 4) * 30 seconds

        SIO_CHECK(frontier_.empty());
        frontier_.reserve(config_.max_active * 3);

        SIO_CHECK(frontier_map_.empty());
        frontier_map_.reserve(frontier_.capacity() * 2); // presumably 50% load factoer

        if (config_.apply_score_offsets) {
            SIO_CHECK(score_offsets_.empty());
            score_offsets_.push_back(0.0);
        }

        // Initialize search session
        status_ = SearchStatus::kBusy;

        Token* t = NewToken();
        t->trace_back.arc.ilabel = kFstEps;
        t->trace_back.arc.olabel = tokenizer_->bos;

        for (int i = 0; i != lms_.size(); i++) {
            LanguageModel& lm = lms_[i];

            LmScore bos_score = lm.GetScore(lm.NullState(), tokenizer_->bos, &t->lm_states[i]);
            t->total_score += bos_score;
        }

        SIO_CHECK_EQ(cur_time_, 0);
        int k = FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, graph_->start_state));
        SIO_CHECK_EQ(k, 0);
        TokenSet& ts = frontier_[0];

        SIO_CHECK(ts.head == nullptr);
        ts.head = t;
        ts.best_score = t->total_score;

        score_max_ = ts.best_score;
        score_cutoff_ = score_max_ - config_.beam;

        FrontierExpandEps();
        FrontierPinDown();

        return Error::OK;
    }


    Error DeinitSession() {
        cur_time_ = 0;
        frontier_.clear();
        frontier_map_.clear();

        lattice_.clear();
        token_arena_.Clear();

        if (config_.apply_score_offsets) {
            score_offsets_.clear();
        }

        nbest_.clear();

        status_ = SearchStatus::kIdle;

        return Error::OK;
    }


    Error FrontierExpandEmitting(const float* frame_score) {
        SIO_CHECK(frontier_.empty());

        cur_time_++; // consumes a time frame
        score_max_ -= 1000.0;
        score_cutoff_ -= 1000.0;

        f32 score_offset = 0.0;
        if (config_.apply_score_offsets) {
            score_offset = score_offsets_.back();
        }

        for (const TokenSet& src : lattice_.back()) {
            for (auto aiter = graph_->GetArcIterator(HandleToState(src.handle)); !aiter.Done(); aiter.Next()) {
                const FstArc& arc = aiter.Value();
                if (arc.ilabel != kFstEps && arc.ilabel != kFstInputEnd) {
                    f32 score = frame_score[arc.ilabel] + score_offset;
                    if (src.best_score + arc.score + score < score_cutoff_) continue;

                    TokenSet& dst = frontier_[
                        FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, arc.dst))
                    ];

                    TokenPassing(src, arc, score, &dst);
                }
            }
        }
        return Error::OK;
    }


    Error FrontierExpandEps() {
        SIO_CHECK(eps_queue_.empty());

        for (int k = 0; k != frontier_.size(); k++) {
            if (graph_->ContainEpsilonArc(HandleToState(frontier_[k].handle))) {
                eps_queue_.push_back(k);
            }
        }

        while (!eps_queue_.empty()) {
            int src_k = eps_queue_.back(); eps_queue_.pop_back();
            const TokenSet& src = frontier_[src_k];

            if (src.best_score < score_cutoff_) continue;

            for (auto aiter = graph_->GetArcIterator(HandleToState(src.handle)); !aiter.Done(); aiter.Next()) {
                const FstArc& arc = aiter.Value();
                if (arc.ilabel == kFstEps) {
                    if (src.best_score + arc.score < score_cutoff_) continue;

                    int dst_k = FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, arc.dst));
                    TokenSet& dst = frontier_[dst_k];

                    bool changed = TokenPassing(src, arc, 0.0, &dst);

                    if (changed && graph_->ContainEpsilonArc(arc.dst)) {
                        eps_queue_.push_back(dst_k);
                    }
                }
            }
        }

        return Error::OK;
    }


    Error FrontierExpandEos() {
        SIO_CHECK(frontier_.empty());

        for (const TokenSet& src : lattice_.back()) {
            for (auto aiter = graph_->GetArcIterator(HandleToState(src.handle)); !aiter.Done(); aiter.Next()) {
                const FstArc& arc = aiter.Value();
                if (arc.ilabel == kFstInputEnd) {
                    TokenSet& dst = frontier_[
                        FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, arc.dst))
                    ];
                    TokenPassing(src, arc, 0.0, &dst);
                }
            }
        }
        status_ = SearchStatus::kDone;

        return Error::OK;
    }


    Error FrontierPrune() {
        auto token_set_better_than = [](const TokenSet& x, const TokenSet& y) -> bool {
            return (x.best_score != y.best_score) ? (x.best_score > y.best_score) : (x.handle < y.handle);
        };

        score_cutoff_ = score_max_ - config_.beam;

        // adapt beam regarding to max_active constraint
        if (config_.max_active > 0 && frontier_.size() > config_.max_active) {
            std::nth_element(
                frontier_.begin(),
                frontier_.begin() + config_.max_active - 1,
                frontier_.end(),
                token_set_better_than
            );
            frontier_.resize(config_.max_active);

            score_cutoff_ = std::max(score_cutoff_, frontier_.back().best_score);
        }

        // put best TokenSet first so that beam of next frame will be established quickly.
        std::nth_element(frontier_.begin(), frontier_.begin(), frontier_.end(), token_set_better_than);
        SIO_CHECK_EQ(frontier_[0].best_score, score_max_);
        
        return Error::OK;
    }


    Error FrontierPinDown() {
        // use "copy" instead of "move" in push_back(),
        // so frontier's capacity() is reserved after clear(),
        // to avoid unnecessary reallocations across frames.
        lattice_.push_back(frontier_);

        frontier_.clear();
        frontier_map_.clear();

        if (config_.apply_score_offsets) {
            score_offsets_.push_back(-score_max_);
        }

        //for (TokenSet& ts : lattice_.back()) {
        //    for (Token* t = ts.head; t != nullptr; t = t->next) {
        //        t->master = &ts;
        //    }
        //}

        return Error::OK;
    }


    Error TraceBestPath() {
        SIO_CHECK(nbest_.empty());
        SIO_CHECK_EQ(frontier_.size(), 1); // There shoudl be only one final state(K2 convention)

        auto it = frontier_map_.find(ComposeStateHandle(0, graph_->final_state));
        if (it == frontier_map_.end()) {
            SIO_WARNING << "No surviving hypothesis reaches to the end, key: " << session_key_;
            return Error::NoRecognitionResult;
        }

        int k;
        Token* p;
        for (k = 0, p = frontier_[it->second].head; k < config_.nbest && p != nullptr; k++, p = p->next) {
            vec<TokenId> path;
            for(Token* t = p; t != nullptr; t = t->trace_back.token) {
                if (t->trace_back.arc.olabel != kFstEps) {
                    path.push_back(t->trace_back.arc.olabel);
                }
            }
            std::reverse(path.begin(), path.end());

            nbest_.push_back(std::move(path));
        }

        return Error::OK;
    }


    void OnSessionBegin() {

    }

    void OnSessionEnd() {

    }

    void OnFrameBegin() {

    }

    void OnFrameEnd() {
        if (config_.debug) {
            printf("%d\t%f\t%f\t%lu\n",
                cur_time_,
                score_max_,
                score_max_ - score_cutoff_,
                lattice_.back().size()
            );
        }
    }

}; // class BeamSearch
}  // namespace sio
#endif
