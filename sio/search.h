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
// 1. "lookahead-cancel" LM
// 2. big LM
// 3. domain LM as contextual biasing
// 4. hotfix Lm as contextual biasing (sometimes also called hint, hot-word/hot-phrase)
//
// These LMs are normally abstracted as *Deterministic Fsa*, 
// so they can be used in an on-the-fly rescoring fasion.
// Nowadays E2E systems tend to call this "shallow fusion".
#define SIO_MAX_CONTEXT_LM 4


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

struct Token {

    struct Context {
        size_t prefix_hash = 0;
        LmStateId states[SIO_MAX_CONTEXT_LM] = {};
    };


    struct TraceBack {
        Token* token = nullptr;
        FsmArc arc;
        f32 score = 0.0;
        LmScore rescores[SIO_MAX_CONTEXT_LM] = {};
    };


    Optional<Token*> next = nullptr; // nullptr -> last token in a lattice node
    f32 score = 0.0;
    Context context;
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

