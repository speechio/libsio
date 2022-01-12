#ifndef SIO_BEAM_SEARCH_H
#define SIO_BEAM_SEARCH_H

#include "sio/common.h"
//#include "sio/dbg.h"

namespace sio {
class Search {
public:
    void Push(const torch::Tensor frame_score) {
        std::tuple<torch::Tensor, torch::Tensor> best = frame_score.topk(1);
        auto score = std::get<0>(best).item<f32>();
        auto index = std::get<1>(best).item<i32>();
        if (index != 0) {
            best_path_tokens_.push_back(index);
            best_path_scores_.push_back(score);
        }
    }


    void PushEnd() { }


    void Reset() {
        best_path_tokens_.clear();
        best_path_scores_.clear();
    }


    const Vec<index_t>& BestPath() {
        return best_path_tokens_;
    }

private:
    Vec<index_t> best_path_tokens_;
    Vec<float> best_path_scores_;
}; // class Search
}  // namespace sio
#endif
