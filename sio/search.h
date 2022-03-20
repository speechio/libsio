#ifndef SIO_SEARCH_H
#define SIO_SEARCH_H

#include "string.h"
#include <limits>

#include "sio/common.h"
#include "sio/allocator.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_machine.h"
#include "sio/language_model.h"
#include "sio/dbg.h"

namespace sio {
class GreedySearch {
    Vec<TokenId> best_tokens_;
    Vec<f32> best_scores_;
    Vec<TokenId> best_path_;

public:
    void Push(const torch::Tensor score) {
        std::tuple<torch::Tensor, torch::Tensor> best = score.topk(1);

        best_scores_.push_back(std::get<0>(best).item<f32>());
        best_tokens_.push_back(std::get<1>(best).item<TokenId>());
    }


    void PushEos() {
        // deduplication
        Vec<TokenId> res1;
        for (index_t i = 0; i < best_tokens_.size(); i++) {
            if (res1.size() == 0 || best_tokens_[i] != res1.back()) {
                res1.push_back( best_tokens_[i] );
            }
        }
        // blank removal
        Vec<TokenId> res2;
        for (index_t i = 0; i < res1.size(); i++) {
            if (res1[i] != 0) { // CAUTION: blank index is assumed to be 0 here(this may not be true)
                res2.push_back(res1[i]);
            }
        }
        best_path_ = std::move(res2);
    }


    void Reset() {
        best_tokens_.clear();
        best_scores_.clear();
        best_path_.clear();
    }


    Vec<TokenId> BestPath() {
        return best_path_;
    }

}; // class GreedySearch


struct BeamSearchConfig {
    f32 beam = 16.0;
    i32 min_active = 1;
    i32 max_active = 12;
    f32 token_set_size = 1;

    i32 token_allocator_slab_size = 5000;
    bool apply_score_offset = true;  // for numerical stability of long audio scores
    bool debug = false;

    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".beam", &beam);
        loader->AddEntry(module + ".min_active", &min_active);
        loader->AddEntry(module + ".max_active", &max_active);
        loader->AddEntry(module + ".token_set_size", &token_set_size);

        loader->AddEntry(module + ".token_allocator_slab_size", &token_allocator_slab_size);
        loader->AddEntry(module + ".apply_score_offset", &apply_score_offset);
        loader->AddEntry(module + ".debug", &debug);

        return Error::OK;
    }
};


enum class SearchStatus : int {
    kUnconstructed,
    kIdle,
    kBusy,
    kDone,
};


// Typical rescoring language models are:
// 1. Lookahead-LM or Internal-LM subtractor
// 2. Big-LM or External-LM
// 3. Specific Domain-LM
// 4. Hotfix-LM (sometimes also called hint, hot-word/hot-phrase)
//
// These LMs are normally represented as *Deterministic Fsa*, 
// so that shallow-fusion based contextual biasing can be applied 
// via on-the-fly rescoring.
#define SIO_MAX_LM 4


// SearchStateHandle: 
//   SearchStateHandle represents a unique state in the decoding graph.
//
// For single-graph decoding:
//   SearchStateHandle = FsmStateId. i.e. the search graph *is* a Fsm:
//     * T for vanilla CTC
//     * TLG for CTC with lexicon & external LM
//     * HCLG for WFST
//
// For multi-graph decoding:
//   Say, SearchStateHandle = 64-bits(32 + 32) integer type:
//     1st 32 bits represent a Fsm
//     2nd 32 bits represent a state inside that Fsm
//   More sophisticated bit-packing can be designed.
using SearchStateHandle = FsmStateId;


struct Token;
struct TokenSet;

struct TraceBack {
    Token* token = nullptr;
    FsmArc arc;
    f32 score = 0.0;
    LmScore lm_scores[SIO_MAX_LM] = {}; // zero initialized to 0.0
};


struct Token {
    Nullable<Token*> next = nullptr; // nullptr -> last token in a TokenSet
    TokenSet* master = nullptr;

    f32 total_score = 0.0;
    LmStateId lm_states[SIO_MAX_LM] = {}; // zero initialized to 0 
    TraceBack trace_back;
};


// TokenSet represents a location(time, state) in beam search space (sometimes called trellis space),
// Each TokenSet holds a list of tokens representing search hypotheses
struct TokenSet {
    f32 best_score = -std::numeric_limits<f32>::infinity();
    Nullable<Token*> head = nullptr; // nullptr -> TokenSet pruned or inactive

    int time = 0;
    SearchStateHandle state = 0;
};


class BeamSearch {
    BeamSearchConfig config_;
    const Fsm* graph_ = nullptr;
    const Tokenizer* tokenizer_ = nullptr;
    Vec<Unique<LanguageModel*>> lms_;

    Str session_key_ = "default_session";
    SearchStatus status_ = SearchStatus::kUnconstructed;

    // lattice indexes: 
    //   [time, token_set_index]
    // invariant of time & frame indexing:
    //   {time=k} ---[frame=k]---> {time=k+1}
    // where: k ~ [0, total_frames)
    Vec<Vec<TokenSet>> lattice_;
    SlabAllocator<Token> token_allocator_;

    // search frontier
    int cur_time_ = 0;  // frontier location on time axis
    Vec<TokenSet> frontier_;
    Map<SearchStateHandle, int> frontier_map_;  // beam search state -> token set index in frontier
    Vec<int> eps_queue_;

    // beam
    f32 score_max_ = 0.0;
    f32 score_cutoff_ = 0.0;

    Vec<f32> score_offset_;  // for numerical stability of long audio scores

    Vec<TokenId> best_path_;

public:

    Error Load(const BeamSearchConfig& config, const Fsm& graph, const Tokenizer& tokenizer) {
        SIO_CHECK(status_ == SearchStatus::kUnconstructed);

        config_ = config; // make a copy to block outside changes 

        SIO_CHECK(graph_ == nullptr);
        graph_ = &graph;

        SIO_CHECK(tokenizer_ == nullptr);
        tokenizer_ = &tokenizer;

        SIO_CHECK(lms_.empty());
        lms_.push_back(make_unique<PrefixTreeLM>());

        status_ = SearchStatus::kIdle;

        return Error::OK;
    }


    Error Push(const torch::Tensor score) {
        SIO_CHECK(status_ == SearchStatus::kIdle || status_ == SearchStatus::kBusy);
        if (status_ == SearchStatus::kIdle) {
            InitSession();
            OnSessionBegin();
        }
        SIO_CHECK(status_ == SearchStatus::kBusy);

        SIO_CHECK_EQ(score.dim(), 1) << "Can't push multiple frames.";
        const float* score_data = score.data_ptr<float>();
        //{ // Check whether LibTorch tensor elements can be accessed via raw data pointer
        //    dbg(score.size(0));
        //    for (int i = 0; i != score.size(0); i++) {
        //        dbg(score[i].item<float>(), score_data[i]);
        //        if (score_data[i] != score[i].item<float>()) {
        //            SIO_FATAL << i;
        //        }
        //    }
        //}
        
        OnFrameBegin();
        {
            FrontierExpandEmitting(score_data);
            FrontierExpandEpsilon();
            FrontierPrune();

            PinFrontierToLattice();
        }
        OnFrameEnd();

        return Error::OK;
    }


    Error PushEos() {
        SIO_CHECK(status_ == SearchStatus::kBusy);
        ExpandFrontierEos();
        TraceBestPath();
        SIO_CHECK(status_ == SearchStatus::kDone);

        OnSessionEnd();

        return Error::OK;
    }


    Vec<TokenId> BestPath() {
        return best_path_;
    }


    Error Reset() {
        SIO_CHECK(status_ == SearchStatus::kDone);
        DeinitSession();
        SIO_CHECK(status_ == SearchStatus::kIdle);

        return Error::OK; 
    }

private:

    static inline SearchStateHandle ComposeStateHandle(int graph, FsmStateId state) {
        //return (static_cast<SearchStateHandle>(graph) << 32) + static_cast<SearchStateHandle>(state);
        return state;
    }
    static inline int HandleToGraph(SearchStateHandle h) {
        //return static_cast<int>(static_cast<u32>(h >> 32));
        return 0;
    }
    static inline FsmStateId HandleToState(SearchStateHandle h) {
        //return static_cast<FsmStateId>(static_cast<u32>(h))
        return h;
    }


    inline Token* NewToken(const Token* copy_from = nullptr) {
        Token* p = token_allocator_.Alloc();
        if (copy_from == nullptr) {
            new (p) Token(); // placement new via default constructor
        } else {
            *p = *copy_from; // POD copy
        }
        return p;
    }


    inline void DeleteToken(Token *p) {
        //p->~Token();
        token_allocator_.Free(p);
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


    inline int FindOrAddTokenSet(int t, SearchStateHandle s) {
        SIO_CHECK_EQ(cur_time_, t) << "Cannot find or add non-frontier TokenSet.";

        int k;
        auto it = frontier_map_.find(s);
        if (it == frontier_map_.end()) {
            TokenSet ts;
            ts.time = t;
            ts.state = s;

            k = frontier_.size();
            frontier_.push_back(ts);
            frontier_map_.insert({s, k});
        } else {
            k = it->second;
        }

        return k;
    }


    inline bool ContextEqual(const Token& x, const Token& y) {
        // option A: the first LM state as unique context id.
        return x.lm_states[0] == y.lm_states[0];

        /*
        // option B: All LMs states combined as unique context id,
        // these LMs can be specified via config for even more flexibility.
        for (int i = 0; i != lms_.size(); i++) {
            if (x.lm_states[i] != y.lm_states[i]) {
                return false;
            }
        }
        return true;
        */
    }


    bool TokenPassing(const TokenSet& src, const FsmArc& arc, f32 score, TokenSet* dst) {
        bool changed = false; // dst token set is changed

        for (const Token* t = src.head; t != nullptr; t = t->next) {
            // most tokens won't survive pruning and context recombination,
            // here we use a "new token" on stack for probing, 
            // and a heap-based copy is created only after its actual survival.
            Token nt;

            // 1. graph & AM score
            nt.total_score = t->total_score + arc.score + score;

            // 2. LM
            if (arc.olabel == kFsmEpsilon) {
                memcpy(nt.lm_states, t->lm_states, sizeof(LmStateId) * lms_.size());
            } else {  /* word-end arc */
                for (int i = 0; i != lms_.size(); i++) {
                    LanguageModel* lm = lms_[i].get();
                    LmScore& lm_score = nt.trace_back.lm_scores[i];

                    bool found = lm->GetScore(t->lm_states[i], arc.olabel, &lm_score, &nt.lm_states[i]);
                    SIO_CHECK(found == true);

                    nt.total_score += lm_score;
                }
            }

            // 3. trace back 
            // this can be moved to back for optimization, keep it here for simplicity
            nt.trace_back.token = const_cast<Token*>(t);
            nt.trace_back.arc = arc;
            nt.trace_back.score = score;

            // beam pruning
            if (nt.total_score < score_cutoff_) {
                continue;
            } else if (nt.total_score > score_max_) {
                // score is high enough to lift current beam range
                score_cutoff_ += (nt.total_score - score_max_);
                score_max_ = nt.total_score;
            }

            // context recombination
            bool survived = true;
            {
                int k = 0;
                Token** p = &dst->head;
                for ( ; *p != nullptr && k != config_.token_set_size; p = &(*p)->next, k++) {
                    if (ContextEqual(**p, nt)) {
                        if ((*p)->total_score < nt.total_score) {
                            // existing token is worse, remove it from dst token set
                            // new token will be inserted later
                            Token *next = (*p)->next;
                            DeleteToken(*p);
                            *p = next;

                            changed = true;
                        } else {
                            // existing token is better, no need to process new token any further
                            survived = false;
                        }

                        break;
                    }
                }
            }

            if (survived) {
                // find position to insert the new token
                int k = 0;
                Token** p = &dst->head;
                for ( ; *p != NULL && k != config_.token_set_size; p = &(*p)->next, k++) {
                    if ((*p)->total_score <= nt.total_score) {
                        break;
                    }
                }

                if (k != config_.token_set_size) {
                    // need a heap-based copy of stack-based probing token for actual insertion
                    Token* q = NewToken(&nt);

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
        SIO_CHECK_EQ(token_allocator_.NumUsed(), 0);
        token_allocator_.SetSlabSize(config_.token_allocator_slab_size);

        SIO_CHECK(lattice_.empty());
        lattice_.reserve(25 * 30); // 25 frame_rates(subsample = 4) * 30 seconds

        SIO_CHECK(frontier_.empty());
        frontier_.reserve(config_.max_active * 3);

        SIO_CHECK(frontier_map_.empty());
        frontier_map_.reserve(frontier_.capacity() * 2); // presumably 50% load factoer

        if (config_.apply_score_offset) {
            SIO_CHECK(score_offset_.empty());
        }

        // Initialize search session
        status_ = SearchStatus::kBusy;

        Token* t = NewToken();
        t->trace_back.arc.ilabel = kFsmEpsilon;
        t->trace_back.arc.olabel = tokenizer_->bos;

        for (int i = 0; i != lms_.size(); i++) {
            LanguageModel* lm = lms_[i].get();

            LmScore bos_score = 0.0;
            bool found = lm->GetScore(lm->NullState(), tokenizer_->bos, &bos_score, &t->lm_states[i]);
            SIO_CHECK(found == true);

            t->total_score += bos_score;
        }

        SIO_CHECK_EQ(cur_time_, 0);
        int k = FindOrAddTokenSet(cur_time_, graph_->start_state);
        SIO_CHECK_EQ(k, 0);
        TokenSet& ts = frontier_[0];

        SIO_CHECK(ts.head == nullptr);
        ts.head = t;
        ts.best_score = t->total_score;

        score_max_ = ts.best_score;
        score_cutoff_ = score_max_ - config_.beam;

        FrontierExpandEpsilon();
        PinFrontierToLattice();

        return Error::OK;
    }


    Error DeinitSession() {
        cur_time_ = 0;
        frontier_.clear();
        frontier_map_.clear();

        lattice_.clear();
        token_allocator_.Reset();

        if (config_.apply_score_offset) {
            score_offset_.clear();
        }

        best_path_.clear();

        status_ = SearchStatus::kIdle;

        return Error::OK;
    }


    Error FrontierExpandEmitting(const float* frame_score) {
        SIO_CHECK(frontier_.empty());
        score_max_ -= 1000.0;
        score_cutoff_ -= 1000.0;

        cur_time_++;
        for (const TokenSet& src : lattice_.back()) {
            for (auto aiter = graph_->GetArcIterator(HandleToState(src.state)); !aiter.Done(); aiter.Next()) {
                const FsmArc& arc = aiter.Value();
                if (arc.ilabel != kFsmEpsilon && arc.ilabel != kFsmInputEnd) {
                    f32 score = frame_score[arc.ilabel];
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


    Error FrontierExpandEpsilon() {
        SIO_CHECK(eps_queue_.empty());

        for (int k = 0; k != frontier_.size(); k++) {
            if (graph_->ContainEpsilonArc(HandleToState(frontier_[k].state))) {
                eps_queue_.push_back(k);
            }
        }

        while (!eps_queue_.empty()) {
            int src_k = eps_queue_.back(); eps_queue_.pop_back();
            const TokenSet& src = frontier_[src_k];

            if (src.best_score < score_cutoff_) continue;

            for (auto aiter = graph_->GetArcIterator(HandleToState(src.state)); !aiter.Done(); aiter.Next()) {
                const FsmArc& arc = aiter.Value();
                if (arc.ilabel == kFsmEpsilon) {
                    if (src.best_score + arc.score < score_cutoff_) continue;

                    int dst_k = FindOrAddTokenSet(cur_time_, ComposeStateHandle(0, arc.dst));
                    TokenSet& dst = frontier_[dst_k];

                    bool changed = TokenPassing(src, arc, -1e-6, &dst);

                    if (changed && graph_->ContainEpsilonArc(arc.dst)) {
                        eps_queue_.push_back(dst_k);
                    }
                }
            }
        }

        return Error::OK;
    }


    Error ExpandFrontierEos() {
        SIO_CHECK(frontier_.empty());

        for (const TokenSet& src : lattice_.back()) {
            for (auto aiter = graph_->GetArcIterator(HandleToState(src.state)); !aiter.Done(); aiter.Next()) {
                const FsmArc& arc = aiter.Value();
                if (arc.ilabel == kFsmInputEnd) {
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
        //for (int k = 0; k != frontier_.size(); k++) {
        //    frontier_[k].best_score = frontier_[k].head->total_score;
        //}

        auto compare = [](const TokenSet& x, const TokenSet& y) -> bool {
            return (x.best_score != y.best_score) ? (x.best_score > y.best_score) : (x.state < y.state);
        };

        if (config_.max_active > 0 && frontier_.size() > config_.max_active) {
            std::nth_element(
                frontier_.begin(),
                frontier_.begin() + config_.max_active - 1,
                frontier_.end(),
                compare
            );
            frontier_.resize(config_.max_active);

            score_cutoff_ = frontier_.back().best_score;
        }

        //if (config_.min_active > 1 && frontier_.size() > config_.min_active) {
        //    std::nth_element(
        //        frontier_.begin(),
        //        frontier_.begin() + config_.min_active - 1,
        //        frontier_.end(),
        //        compare
        //    );
        //    score_cutoff_ = std::min(score_cutoff_, frontier_[config_.min_active - 1].best_score);
        //}

        std::nth_element(
            frontier_.begin(),
            frontier_.begin(),
            frontier_.end(),
            compare
        );
        SIO_CHECK_EQ(frontier_[0].best_score, score_max_);
        
        return Error::OK;
    }


    Error PinFrontierToLattice() {
        // intentially push_back() via "copy" instead of "move"
        lattice_.push_back(frontier_);

        // frontier's capacity() is reserved after clear(),
        // avoiding unnecessary reallocations across frames.
        frontier_.clear();
        frontier_map_.clear();

        Vec<TokenSet>& v = lattice_.back();
        for (int k = 0; k != v.size() ; k++) {
            for (Token* t = v[k].head; t != nullptr; t = t->next) {
                t->master = &v[k];
            }
        }

        return Error::OK;
    }

    void TraceBestPath() {
        SIO_CHECK(best_path_.empty());

        auto it = frontier_map_.find(ComposeStateHandle(0, graph_->final_state));
        if (it == frontier_map_.end()) {
            SIO_WARNING << "No surviving hypothesis reaches to the end, key: " << session_key_;
            return;
        }

        int k = it->second;
        for(Token* t = frontier_[k].head; t != nullptr; t = t->trace_back.token) {
            if (t->trace_back.arc.olabel != kFsmEpsilon) {
                best_path_.push_back(t->trace_back.arc.olabel);
            }
        }

        std::reverse(best_path_.begin(), best_path_.end());
    }


    void OnSessionBegin() {
    }

    void OnSessionEnd() {
        //for (int i = 0; i != lattice_.size(); i++) {
        //    dbg(i, lattice_[i].size());
        //}
    }

    inline void OnFrameBegin() {
    }

    inline void OnFrameEnd() {
        //dbg(cur_time_, score_max_, score_cutoff_, lattice_.back().size());
        dbg(lattice_.back().size());
    }

}; // class BeamSearch
}  // namespace sio
#endif
