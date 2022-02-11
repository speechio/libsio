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


#define SIO_MAX_CONTEXT 4

struct Token;

struct TokenContext {
    size_t prefix = 0;
    LmStateId states[SIO_MAX_CONTEXT] = {};
};


struct TraceBack {
    Token* token = nullptr;

    FsmLabel ilabel = 0;
    FsmLabel olabel = 0;
    FsmScore arc_score = 0.0;

    f32 model_score = 0.0;

    LmScore rescores[SIO_MAX_CONTEXT] = {};
};


struct Token {
    Optional<Token*> next = nullptr; // nullptr -> last token in a lattice node
    f32 score = 0.0;
    TokenContext context;
    TraceBack trace_back;
};


struct LatticeNode {
    FsmStateId state = 0;
    Optional<Token*> head = nullptr; // nullptr -> lattice node pruned or inactive
};


class BeamSearch {
    Unique<SlabAllocator<Token>*> token_arena_;

    Map<FsmStateId, i32> frontier_;

    Vec<Vec<LatticeNode>> lattice_; // [time, node_index]

public:

}; // class BeamSearch

}  // namespace sio
#endif

