#ifndef SIO_BEAM_SEARCH_H
#define SIO_BEAM_SEARCH_H

#include "sio/base.h"
#include "sio/dbg.h"

namespace sio {
class BeamSearch {
 public:
  Error PushScore(const Vec<torch::Tensor>& scores) {
    for (index_t b = 0; b != scores.size(); b++) {
      for (index_t f = 0; f < scores[b].size(0); f++) {
        std::tuple<torch::Tensor, torch::Tensor> top1 = scores[b][f].topk(1);
        f32 score = std::get<0>(top1).item<f32>();
        i32 index = std::get<1>(top1).item<i32>();
        if (index != 0) {
          best_path_.push_back(index);
          best_score_.push_back(score);
        }
      }
    }

    return Error::OK;
  }

  Error EOS() { return Error::OK; }

  const Vec<index_t>& BestPath() {
    return best_path_;
  }

  void Reset() {
    best_path_.clear();
    best_score_.clear();
  }

 private:
  Vec<index_t> best_path_;
  Vec<float> best_score_;

}; // class BeamSearch
}  // namespace sio
#endif
