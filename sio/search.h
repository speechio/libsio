#ifndef SIO_SEARCH_H
#define SIO_SEARCH_H

#include "sio/common.h"
#include "sio/allocator.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_machine.h"
#include "sio/language_model.h"
//#include "sio/dbg.h"

namespace sio {
class GreedySearch {
public:
    void Push(const torch::Tensor frame_score) {
        std::tuple<torch::Tensor, torch::Tensor> best = frame_score.topk(1);
        auto score = std::get<0>(best).item<f32>();
        auto token = std::get<1>(best).item<TokenId>();
        best_path_tokens_.push_back(token);
        best_path_scores_.push_back(score);
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



// This is the max number of rescoring language models, typical rescoring LM types are:
// 1. Lookahead-LM or Internal-LM subtractor
// 2. Big LM or External-LM
// 3. Domain specific LM
// 4. Hotfix LM (sometimes also called hint, hot-word/hot-phrase)
//
// These LMs are normally represented as *Deterministic Fsa*, 
// so contextual biasing can be applied in an on-the-fly rescoring fashion.
// Nowadays E2E systems tend to call this "shallow fusion".
#define SIO_MAX_CONTEXT_LM 3


// BeamSearchState concept: 
//   Designed for future extension to multi-graph decoding such as sub-grammar, class-based LM, ...
//
// For single graph decoding: BeamSearchState = FsmStateId
//   It is enough to represent beam search space.
//
// For multi-graph decoding: BeamSearchState = 64-bits(32 + 32) integer type:
//   1st 32 bits represent sub-graph index
//   2nd 32 bits represent state index inside that sub-graph
// More sophisticated bit-packing can be designed & implemented to represent beam search space.
using BeamSearchState = FsmStateId;


struct TokenContext {
    size_t prefix_state = 0;
    LmStateId lm_states[SIO_MAX_CONTEXT_LM] = {};
};


struct Token;
struct TraceBack {
    Token* token = nullptr;
    FsmArc arc;
    f32 score = 0.0;
    LmScore rescores[SIO_MAX_CONTEXT_LM] = {};
};


struct Token {
    Optional<Token*> next = nullptr; // nullptr -> last token in a lattice node
    f32 score = 0.0;
    TokenContext context;
    TraceBack trace_back;
};


struct LatticeNode {
    BeamSearchState state = 0;
    Optional<Token*> head = nullptr; // nullptr -> lattice node pruned or inactive
};


class BeamSearch {
    using LatticeNodeHandle = size_t;

    Unique<SlabAllocator<Token>*> token_arena_;

    Map<BeamSearchState, LatticeNodeHandle> frontier_;

    Vec<Vec<LatticeNode>> lattice_; // [time, node_handle]

public:

}; // class BeamSearch

}  // namespace sio
#endif

