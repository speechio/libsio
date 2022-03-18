#ifndef SIO_SEARCH_H
#define SIO_SEARCH_H

#include <limits>

#include "sio/common.h"
#include "sio/allocator.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_machine.h"
#include "sio/language_model.h"
#include "sio/dbg.h"

namespace sio {
class GreedySearch {
public:
    void Push(const torch::Tensor score) {
        std::tuple<torch::Tensor, torch::Tensor> best = score.topk(1);

        best_path_scores_.push_back(std::get<0>(best).item<f32>());
        best_path_tokens_.push_back(std::get<1>(best).item<TokenId>());
    }


    void PushEos() { }


    void Reset() {
        best_path_tokens_.clear();
        best_path_scores_.clear();
    }


    Vec<TokenId> BestPath() {
        // dup removal
        Vec<TokenId> res1;
        for (index_t i = 0; i < best_path_tokens_.size(); i++) {
            if (res1.size() == 0 || best_path_tokens_[i] != res1.back()) {
                res1.push_back( best_path_tokens_[i] );
            }
        }
        // blank removal
        Vec<TokenId> res2;
        for (index_t i = 0; i < res1.size(); i++) {
            if (res1[i] != 0) { // CAUTION: blank index is assumed to be 0 here(this may not be true)
                res2.push_back(res1[i]);
            }
        }
        return std::move(res2);
    }

private:
    Vec<TokenId> best_path_tokens_;
    Vec<f32> best_path_scores_;
}; // class GreedySearch


struct BeamSearchConfig {
    f32 beam = 16.0;
    i32 min_active = 8;
    i32 max_active = 12;

    f32 token_set_size = 1;
    f32 token_set_beam = 0.01;

    i32 token_allocator_slab_size = 5000;

    bool apply_score_offset = true;  // for numerical stability of long audio scores

    bool debug_mode = false;

    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".beam", &beam);
        loader->AddEntry(module + ".min_active", &min_active);
        loader->AddEntry(module + ".max_active", &max_active);

        loader->AddEntry(module + ".token_set_size", &token_set_size);
        loader->AddEntry(module + ".token_set_beam", &token_set_beam);

        loader->AddEntry(module + ".token_allocator_slab_size", &token_allocator_slab_size);

        loader->AddEntry(module + ".apply_score_offset", &apply_score_offset);

        loader->AddEntry(module + ".debug_mode", &debug_mode);

        return Error::OK;
    }
};


enum class SearchStatus : int {
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
    LmScore lm_scores[SIO_MAX_LM] = {};
};


struct Token {
    TokenSet* master = nullptr;
    Nullable<Token*> next = nullptr; // nullptr -> last token in a TokenSet

    f32 total_score = 0.0;

    u64 prefix_hash = 0;
    LmStateId lm_states[SIO_MAX_LM] = {};

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
    Vec<Unique<LanguageModel*>> lms_;

    Str session_key_ = "default_session";
    SearchStatus status_ = SearchStatus::kIdle;

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

public:

    Error Load(const BeamSearchConfig& config, const Fsm& graph) {
        SIO_CHECK(graph_ == nullptr);

        config_ = config;
        graph_ = &graph;
        SIO_CHECK(status_ == SearchStatus::kIdle);

        return Error::OK;
    }


    Error Push(const torch::Tensor score) {
        SIO_CHECK(status_ == SearchStatus::kIdle || status_ == SearchStatus::kBusy);
        if (status_ == SearchStatus::kIdle) {
            InitSession();
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
        
        ExpandFrontierEmitting(score_data);
        ExpandFrontierEpsilon();
        PruneFrontier();

        PinFrontierToLattice();

        return Error::OK;
    }


    Error PushEos() {
        SIO_CHECK(status_ == SearchStatus::kBusy);
        ExpandFrontierEos();
        SIO_CHECK(status_ == SearchStatus::kDone);

        return Error::OK;
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


    inline Token* NewToken() {
        Token* p = token_allocator_.Alloc();
        new (p) Token(); // placement new
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


    inline int FindOrAddTokenSet(int time, SearchStateHandle state) {
        SIO_CHECK_EQ(cur_time_, time) << "Cannot find or add non-frontier TokenSet.";

        int k;
        auto it = frontier_map_.find(state);
        if (it == frontier_map_.end()) {
            TokenSet ts;
            ts.time = time;
            ts.state = state;

            k = frontier_.size();
            frontier_.push_back(ts);
            frontier_map_.insert({state, k});
        } else {
            k = it->second;
        }

        return k;
    }


    inline int TokenContextEqual(const Token& x, const Token& y) {
        if (lms_.empty()) {
            return x.prefix_hash == y.prefix_hash;
        }

        for (int i = 0; i != lms_.size(); i++) {
            if (x.lm_states[i] != y.lm_states[i]) {
                return false;
            }
        }

        return true;
    }


    bool TokenPassing(const TokenSet& src, const FsmArc& arc, f32 score, TokenSet* dst) {
        bool changed = false; // dst token set is changed

        for (Token* t = src.head; t != nullptr; t = t->next) {
            Token* nt = NewToken();

            nt->total_score = t->total_score + arc.score + score;

            nt->trace_back.token = t;
            nt->trace_back.arc = arc;
            nt->trace_back.score = score;

            // lm scoring
            if (arc.olabel != kFsmEpsilon) {
                // need prefix_hash as unique hypothesis identifier if there is no external LM
                if (lms_.empty()) {
                    // prime picked from Kaldi's VectorHasher: 
                    //   https://github.com/kaldi-asr/kaldi/blob/master/src/util/stl-utils.h#L230
                    constexpr u64 prime = 7853;
                    nt->prefix_hash = t->prefix_hash * prime + (u64)arc.olabel;
                }

                for (int i = 0; i != lms_.size(); i++) {
                    LanguageModel* lm = lms_[i].get();

                    f32 lm_score = 0.0;
                    bool found = lm->GetScore(t->lm_states[i], arc.olabel, &lm_score, &nt->lm_states[i]);
                    SIO_CHECK(found == true);

                    nt->total_score += lm_score;
                    nt->trace_back.lm_scores[i] = lm_score;
                }
            }

            if (nt->total_score < score_cutoff_ ||
                nt->total_score < dst->best_score - config_.token_set_beam) 
            {
                DeleteToken(nt);
                continue;
            }

            // eliminate token context collision
            {
                int k = 0;
                Token** p = &dst->head;
                for ( ; *p != nullptr && k != config_.token_set_size; p = &(*p)->next, k++) {
                    if (TokenContextEqual(**p, *nt)) { // found collision
                        if ((*p)->total_score <= nt->total_score) {
                            // existing token is worse, remove it from token set
                            // new token will be inserted later
                            Token *next = (*p)->next;
                            DeleteToken(*p);
                            *p = next;
                        } else {
                            // existing token is better, just delete new token
                            DeleteToken(nt);
                            nt = nullptr;
                        }

                        changed = true;
                        break;
                    }
                }
            }

            // insert new token
            if (nt != nullptr) {
                int k = 0;
                Token** p = &dst->head;

                // find position
                for ( ; *p != NULL && k != config_.token_set_size; p = &(*p)->next, k++) {
                    if ((*p)->total_score <= nt->total_score) {
                        break;
                    }
                }

                if (k != config_.token_set_size) {
                    nt->next = *p;
                    *p = nt;
                    changed = true;
                }
            }

        } // for each token in src token set

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
        for (int i = 0; i != lms_.size(); i++) {
            LanguageModel* lm = lms_[i].get();

            f32 bos_score = 0.0;
            bool found = lm->GetScore(lm->NullState(), lm->Bos(), &bos_score, &t->lm_states[i]);
            SIO_CHECK(found == true);

            t->total_score += bos_score;
        }

        SIO_CHECK_EQ(cur_time_, 0);
        int k = FindOrAddTokenSet(cur_time_, graph_->start_state);
        SIO_CHECK_EQ(k, 0);
        TokenSet& ts = frontier_[0];

        SIO_CHECK(ts.head == nullptr);
        ts.head = t; // TODO: replace with AddTokenToSet()?
        ts.best_score = t->total_score;

        score_max_ = ts.best_score;
        score_cutoff_ = score_max_ - config_.beam;
        ExpandFrontierEpsilon();
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

        status_ = SearchStatus::kIdle;

        return Error::OK;
    }


    Error ExpandFrontierEmitting(const float* score) {
        cur_time_++;
        return Error::OK;
    }


    Error ExpandFrontierEpsilon() {
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

                    bool changed = TokenPassing(src, arc, 0.0, &dst);

                    if (changed && graph_->ContainEpsilonArc(arc.dst)) {
                        eps_queue_.push_back(dst_k);
                    }
                }
            }
        }

        return Error::OK;
    }


    Error ExpandFrontierEos() {
        status_ = SearchStatus::kDone;
        return Error::OK;
    }


    Error PruneFrontier() {
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

}; // class BeamSearch
}  // namespace sio
#endif
