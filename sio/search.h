#ifndef SIO_SEARCH_H
#define SIO_SEARCH_H

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


    void PushEnd() { }


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

    f32 node_beam = 0.01;
    f32 node_topk = 1;

    i32 token_slab_size = 5000;

    bool apply_score_offset = true; // used to improve numeric stability

    bool debug_mode = false;

    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".beam", &beam);
        loader->AddEntry(module + ".min_active", &min_active);
        loader->AddEntry(module + ".max_active", &max_active);

        loader->AddEntry(module + ".node_beam", &node_beam);
        loader->AddEntry(module + ".node_topk", &node_topk);

        loader->AddEntry(module + ".token_slab_size", &token_slab_size);

        loader->AddEntry(module + ".apply_score_offset", &apply_score_offset);

        loader->AddEntry(module + ".debug_mode", &debug_mode);

        return Error::OK;
    }
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


// SearchStateId:
//   SearchStateId represents a state in *beam search space*.
//   It should not be limited to a specific graph state, because later
//   we may support dynamic multi-graph decoding (such as sub-grammar, class-based LM, ...)
//
// For single-graph decoding: 
//   SearchStateId = FsmStateId. All search states come from the same fsm.
//
// For multi-graph decoding:
//   Say, SearchStateId = 64-bits(32 + 32) integer type:
//     1st 32 bits represent sub-graph index
//     2nd 32 bits represent state index inside that sub-graph
//   More sophisticated bit-packing can be designed to represent SearchStateId.
//
// Entire beam search space = (search state axis * search time axis) 
using SearchStateId = FsmStateId;


struct Token;
struct LatticeNode;


struct TraceBack {
    Token* token = nullptr;
    FsmArc arc;
    f32 score = 0.0;
    LmScore rescores[SIO_MAX_LM] = {};
};


struct Token {
    //LatticeNode* master = nullptr;
    Nullable<Token*> next = nullptr; // nullptr -> last token in a lattice node

    f32 score = 0.0;

    u64 prefix = 0;
    LmStateId lm_states[SIO_MAX_LM] = {};

    TraceBack trace_back;
};


// LatticeNode represents a point in beam search space, i.e. a (time, state) pair.
// Each node holds a list of tokens that store hypothesis scores, rescores, tracebacks etc
struct LatticeNode {
    int t = 0;
    SearchStateId s = 0;
    Nullable<Token*> head = nullptr; // nullptr -> lattice node pruned or inactive
};


enum class SessionStatus : int {
    kIdle,
    kBusy,
    kDone,
};


class BeamSearch {
    Str session_key_ = "default_session";
    SessionStatus status_ = SessionStatus::kIdle;

    BeamSearchConfig config_;
    const Fsm* graph_ = nullptr;
    Vec<Unique<LanguageModel*>> lms_;

    SlabAllocator<Token> token_arena_;
    Vec<Vec<LatticeNode>> lattice_; // [time, node_index]

    // beam search frontier 
    Vec<LatticeNode> frontier_nodes_;
    Map<SearchStateId, int> frontier_; // search state -> frontier lattice node index
    f32 score_max_ = 0.0;
    f32 score_cutoff_ = 0.0;

    Vec<f32> score_offset_;

public:

    Error Load(const BeamSearchConfig& config, const Fsm& graph) {
        SIO_CHECK(graph_ == nullptr);

        config_ = config;
        graph_ = &graph;
        SIO_CHECK(status_ == SessionStatus::kIdle);

        return Error::OK;
    }


    Error Push(const torch::Tensor score) {
        SIO_CHECK(status_ == SessionStatus::kIdle || status_ == SessionStatus::kBusy);
        if (status_ == SessionStatus::kIdle) {
            InitSession();
        }
        SIO_CHECK(status_ == SessionStatus::kBusy);

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
        
        ProcessEmitting(score_data);
        ProcessNonemitting();

        return Error::OK;
    }


    Error PushEnd() {
        SIO_CHECK(status_ == SessionStatus::kBusy);

        status_ = SessionStatus::kDone;
        return Error::OK;
    }


    Error Reset() {
        SIO_CHECK(status_ == SessionStatus::kDone);
        DeinitSession();
        SIO_CHECK(status_ == SessionStatus::kIdle);

        return Error::OK; 
    }

private:

    inline Token* NewToken() {
        Token* p = token_arena_.Alloc();
        new (p) Token(); // placement new
        return p;
    }


    inline void DeleteToken(Token *p) {
        //p->~Token();
        token_arena_.Free(p);
    }


    LatticeNode* FindOrExpandFrontier(int t, SearchStateId s) {
        SIO_CHECK_EQ(lattice_.size(), t) << "frontier time & lattice size mismatch.";

        int ni; // node index
        auto it = frontier_.find(s);
        if (it == frontier_.end()) {
            LatticeNode node;
            node.t = t;
            node.s = s;

            ni = frontier_nodes_.size();

            frontier_nodes_.push_back(node);
            frontier_.insert({s, ni});
        } else {
            ni = it->second;
        }

        return &frontier_nodes_[ni];
    }


    void PinFrontierToLattice() {
        lattice_.push_back(frontier_nodes_); // capacity is not copy-assigned

        frontier_.clear();
        frontier_nodes_.clear();
    }


    Error InitSession() {
        // Precondition checks
        SIO_CHECK_EQ(token_arena_.NumUsed(), 0);
        token_arena_.SetSlabSize(config_.token_slab_size);

        SIO_CHECK(lattice_.empty());
        lattice_.reserve(25 * 30); // 25 frame_rates(subsample = 4) * 30 seconds

        SIO_CHECK(frontier_nodes_.empty());
        frontier_nodes_.reserve(config_.max_active * 3);

        SIO_CHECK(frontier_.empty());
        frontier_.reserve(frontier_nodes_.capacity() / 0.5); // presumably 50% load factoer

        if (config_.apply_score_offset) {
            SIO_CHECK(score_offset_.empty());
        }

        // Initialize search session
        status_ = SessionStatus::kBusy;

        Token* token = NewToken();
        for (int i = 0; i != lms_.size(); i++) {
            LanguageModel* lm = lms_[i].get();

            f32 score = 0.0;
            bool r = lm->GetScore(lm->NullState(), lm->Bos(), &score, &token->lm_states[i]);
            SIO_CHECK(r == true);

            token->score += score;
        }

        LatticeNode* node = FindOrExpandFrontier(0, graph_->start_state);
        node->head = token; // TODO: replace with AddTokenToNode()?

        score_max_ = token->score;
        score_cutoff_ = score_max_ - config_.beam;

        ProcessNonemitting();

        PinFrontierToLattice();

        return Error::OK;
    }


    Error DeinitSession() {
        frontier_.clear();
        frontier_nodes_.clear();

        lattice_.clear();
        token_arena_.Reset();

        if (config_.apply_score_offset) {
            score_offset_.clear();
        }

        status_ = SessionStatus::kIdle;

        return Error::OK;
    }


    Error ProcessEmitting(const float* score) {
        return Error::OK;
    }


    Error ProcessNonemitting() {
        return Error::OK;
    }


}; // class BeamSearch
}  // namespace sio
#endif
