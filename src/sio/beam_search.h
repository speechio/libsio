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
static inline int HandleToGraph(StateHandle s) {
    return 0;
    //return static_cast<int>(static_cast<u32>(s >> 32));
}
static inline FstStateId HandleToState(StateHandle s) {
    return s;
    //return static_cast<FstStateId>(static_cast<u32>(s))
}


struct Token;
struct TokenSet;

struct Traceback {
    Nullable<const Token*> token = nullptr; // nullptr: alpha token
    Nullable<const FstArc*> arc = nullptr;  // nullptr: alpha token
    f32 am_score = 0.0;
    f32 lm_scores[SIO_MAX_LM] = {};
};


struct Token {
    Nullable<Token*> next = nullptr; // nullptr: last token in a TokenSet
    //TokenSet* master = nullptr;

    f32 score = 0.0;
    LmStateId lm_states[SIO_MAX_LM] = {};

    Traceback traceback;
};


// TokenSet represents a location(time, state handle) in beam search space (sometimes called trellis space),
// Each TokenSet holds a list of tokens representing search hypotheses
struct TokenSet {
    Nullable<Token*> head = nullptr; // nullptr: empty TokenSet (pruned or inactive)

    f32 best_score = std::numeric_limits<f32>::lowest();
    int time = 0;
    StateHandle state_handle = 0;
};


class BeamSearch {
    BeamSearchConfig config_;
    const Fst* graph_ = nullptr;
    const Tokenizer* tokenizer_ = nullptr;
    vec<LanguageModel> lms_;

    str session_key_;

    // lattice indexes: [time, token_set_index]
    // invariant of time & frame indexing:
    //   {time=k} --[frame=k]--> {time=k+1}
    //   where: k ~ [0, total_frames)
    vec<vec<TokenSet>> lattice_;
    SlabAllocator<Token> token_arena_;

    // search frontier
    int cur_time_ = 0;  // frontier location on time axis
    vec<TokenSet> frontier_;
    hashtab<StateHandle, int> frontier_map_;  // search state handle -> token set index in frontier
    vec<int> eps_queue_;

    // beam range
    f32 score_max_ = 0.0;
    f32 score_min_ = 0.0;

    vec<f32> score_offsets_;  // keep hypotheses scores in a good dynamic range

    vec<vec<TokenId>> nbest_;

public:

    Error Load(const BeamSearchConfig& config, const Fst& graph, const Tokenizer& tokenizer) {
        config_ = config; // make a copy to block outside changes 

        SIO_CHECK(graph_ == nullptr);
        graph_ = &graph;

        SIO_CHECK(tokenizer_ == nullptr);
        tokenizer_ = &tokenizer;

        SIO_CHECK(lms_.empty());

        return Error::OK;
    }


    Error SetContext(const Context& c) {
        SIO_CHECK_LT(lms_.size(), SIO_MAX_LM);

        // Behaviour tags parsing
        bool major = absl::StartsWith(c.tags, "*") ? true : false;

        // construct contextual LM
        lms_.emplace_back();
        LanguageModel& lm = lms_.back();

        switch (c.type) {
            case LmType::PrefixTreeLm:
                SIO_CHECK(major == true);
                lm.LoadPrefixTreeLm(major);

                SIO_INFO << "    context enabled: " << c.tags << c.name;
                break;

            case LmType::KenLm:
                lm.LoadCachedNgramLm(*c.kenlm.get(), c.scale, c.cache, major);

                SIO_INFO << "    context enabled: " << c.tags << c.name <<" "<< c.scale <<" "<< c.cache;
                break;

            case LmType::FstLm:
                break;

            default:
                SIO_PANIC(Error::UnsupportedLanguageModel);
        }

        return Error::OK;
    }


    Error InitSession(const char* session_key = "default_session") {
        session_key_ = session_key;

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

        Token* t = NewToken();
        for (int i = 0; i != lms_.size(); i++) {
            t->score += lms_[i].GetScore(lms_[i].NullState(), tokenizer_->bos, &t->lm_states[i]);
        }

        SIO_CHECK_EQ(cur_time_, 0);
        int k = FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, graph_->start_state));
        SIO_CHECK_EQ(k, 0);
        TokenSet& ts = frontier_[0];

        SIO_CHECK(ts.head == nullptr);
        ts.head = t;
        ts.best_score = t->score;

        score_max_ = ts.best_score;
        score_min_ = score_max_ - config_.beam;

        FrontierExpandEps();
        FrontierPinDown();

        OnSessionBegin();

        return Error::OK;
    }


    Error Push(const torch::Tensor scores_tensor) {
        SIO_CHECK_EQ(scores_tensor.dim(), 1); // frame by frame

        OnFrameBegin();
        {
            FrontierExpandEmitting(scores_tensor.data_ptr<float>());
            FrontierExpandEps();
            FrontierPrune();
            FrontierPinDown();
        }
        OnFrameEnd();

        return Error::OK;
    }


    Error PushEos() {
        FrontierExpandEos();
        TraceBestPath();

        return Error::OK;
    }


    const vec<vec<TokenId>>& NBest() {
        return nbest_;
    }


    Error DeinitSession() {
        OnSessionEnd();

        cur_time_ = 0;
        frontier_.clear();
        frontier_map_.clear();

        lattice_.clear();
        token_arena_.Clear();

        if (config_.apply_score_offsets) {
            score_offsets_.clear();
        }

        nbest_.clear();

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


    inline int FindOrAddTokenSet(int t, StateHandle s) {
        SIO_CHECK_EQ(cur_time_, t);

        int k;
        auto it = frontier_map_.find(s);
        if (it == frontier_map_.end()) {
            TokenSet ts;
            ts.time = t;
            ts.state_handle = s;

            k = frontier_.size();
            frontier_.push_back(ts);
            frontier_map_.insert({s, k});
        } else {
            k = it->second;
        }

        return k;
    }


    inline bool ContextEqual(const Token& x, const Token& y) {
        for (int i = 0; i != lms_.size(); i++) {
            if (lms_[i].Major()) {
                if (x.lm_states[i] != y.lm_states[i]) {
                    return false;
                }
            }
        }
        return true;
    }


    bool TokenPassing(const TokenSet& src, const FstArc& arc, TokenSet* dst, f32 am_score = 0.0f) {
        bool changed = false; // dst token set is changed

        for (const Token* x = src.head; x != nullptr; x = x->next) {
            // most tokens won't survive pruning and context recombination,
            // here we use a "new token" on stack for probing, 
            // and a heap-based copy is created only after its actual survival.
            Token y;

            // 1. graph & AM score
            y.score = x->score + arc.score + am_score;

            // 2. LM
            if (arc.olabel == kFstEps) {
                memcpy(y.lm_states, x->lm_states, sizeof(LmStateId) * lms_.size());
            } else {  /* word-end arc */
                for (int i = 0; i != lms_.size(); i++) {
                    LanguageModel& lm = lms_[i];

                    f32& lm_score = y.traceback.lm_scores[i];
                    lm_score = lm.GetScore(x->lm_states[i], arc.olabel, &y.lm_states[i]);
                    y.score += lm_score;
                }
                y.score -= config_.insertion_penalty;
            }

            // 3. trace back 
            // this can be moved to back for optimization, keep it here for simplicity
            y.traceback.token = x;
            y.traceback.arc = &arc;
            y.traceback.am_score = am_score;

            // beam pruning
            if (y.score < score_min_) {
                continue;
            } else if (y.score > score_max_) {  // high enough to lift current beam range
                score_min_ += (y.score - score_max_);
                score_max_ = y.score;
            }

            // context recombination
            bool survived = true;
            {
                int k;
                Token** p;
                for (k = 0, p = &dst->head; k < config_.token_set_size && *p != nullptr; k++, p = &(*p)->next) {
                    if (ContextEqual(**p, y)) {
                        if ((*p)->score < y.score) {  // existing token is worse, remove it
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
                    if ((*p)->score <= y.score) {
                        break;
                    }
                }

                if (k != config_.token_set_size) {
                    Token* q = NewToken(&y); // actual heap copy to insert

                    q->next = *p;
                    *p = q;

                    changed = true;
                }
            }

        } // for each token in src token set

        if (changed) {
            dst->best_score = dst->head->score;
        }

        return changed;
    }


    Error FrontierExpandEmitting(const float* scores) {
        SIO_CHECK(frontier_.empty());

        score_max_ -= 1000.0;
        score_min_ -= 1000.0;
        cur_time_++; // consumes a time frame

        f32 score_offset = config_.apply_score_offsets ? score_offsets_.back() : 0.0;

        for (const TokenSet& src : lattice_.back()) {
            for (auto aiter = graph_->GetArcIterator(HandleToState(src.state_handle)); !aiter.Done(); aiter.Next()) {
                const FstArc& arc = aiter.Value();
                if (arc.ilabel != kFstEps && arc.ilabel != kFstInputEnd) {
                    f32 am_score = scores[arc.ilabel] + score_offset;
                    if (src.best_score + arc.score + am_score < score_min_) {
                        continue;
                    }

                    TokenSet& dst = frontier_[FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, arc.dst))];
                    TokenPassing(src, arc, &dst, am_score);
                }
            }
        }
        return Error::OK;
    }


    Error FrontierExpandEps() {
        SIO_CHECK(eps_queue_.empty());

        for (int i = 0; i != frontier_.size(); i++) {
            if (graph_->ContainEpsilonArc(HandleToState(frontier_[i].state_handle))) {
                eps_queue_.push_back(i);
            }
        }

        while (!eps_queue_.empty()) {
            int i = eps_queue_.back(); eps_queue_.pop_back();
            const TokenSet& src = frontier_[i];

            if (src.best_score < score_min_) {
                continue;
            }

            for (auto aiter = graph_->GetArcIterator(HandleToState(src.state_handle)); !aiter.Done(); aiter.Next()) {
                const FstArc& arc = aiter.Value();
                if (arc.ilabel == kFstEps) {
                    if (src.best_score + arc.score < score_min_) {
                        continue;
                    }

                    int j = FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, arc.dst));
                    TokenSet& dst = frontier_[j];

                    bool changed = TokenPassing(src, arc, &dst);
                    if (changed && graph_->ContainEpsilonArc(arc.dst)) {
                        eps_queue_.push_back(j);
                    }
                }
            }
        }

        return Error::OK;
    }


    Error FrontierExpandEos() {
        SIO_CHECK(frontier_.empty());

        for (const TokenSet& src : lattice_.back()) {
            for (auto aiter = graph_->GetArcIterator(HandleToState(src.state_handle)); !aiter.Done(); aiter.Next()) {
                const FstArc& arc = aiter.Value();
                if (arc.ilabel == kFstInputEnd) {
                    TokenSet& dst = frontier_[FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, arc.dst))];
                    TokenPassing(src, arc, &dst);
                }
            }
        }

        return Error::OK;
    }


    Error FrontierPrune() {
        auto token_set_better_than = [](const TokenSet& x, const TokenSet& y) -> bool {
            return (x.best_score != y.best_score) ? (x.best_score > y.best_score) : (x.state_handle < y.state_handle);
        };

        score_min_ = score_max_ - config_.beam;

        // adapt beam regarding to max_active constraint
        if (config_.max_active > 0 && frontier_.size() > config_.max_active) {
            std::nth_element(
                frontier_.begin(),
                frontier_.begin() + config_.max_active - 1,
                frontier_.end(),
                token_set_better_than
            );
            frontier_.resize(config_.max_active);

            score_min_ = std::max(score_min_, frontier_.back().best_score);
        }

        // put best TokenSet first so that beam of next frame will be established quickly.
        std::nth_element(frontier_.begin(), frontier_.begin(), frontier_.end(), token_set_better_than);
        SIO_CHECK_EQ(frontier_[0].best_score, score_max_);
        
        return Error::OK;
    }


    Error FrontierPinDown() {
        // use "copy" instead of "move", so frontier's capacity() is reserved across frames
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
            for(const Token* t = p; t != nullptr; t = t->traceback.token) {
                if (t->traceback.arc != nullptr) {
                    FstLabel olabel = t->traceback.arc->olabel;
                    if (olabel != kFstEps) {
                        path.push_back(t->traceback.arc->olabel);
                    }
                }
            }
            path.push_back(tokenizer_->bos);
            std::reverse(path.begin(), path.end());

            nbest_.push_back(std::move(path));
        }

        return Error::OK;
    }


    void OnSessionBegin() { }
    void OnSessionEnd() { }
    void OnFrameBegin() { }
    void OnFrameEnd() {
        if (config_.debug) {
            printf("%d\t%f\t%f\t%lu\n",
                cur_time_,
                score_max_,
                score_max_ - score_min_,
                lattice_.back().size()
            );
        }
    }

}; // class BeamSearch
}  // namespace sio
#endif
