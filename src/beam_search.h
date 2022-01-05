#ifndef SIO_BEAM_SEARCH_H
#define SIO_BEAM_SEARCH_H

#include "sio/base.h"
//#include "sio/dbg.h"

namespace sio {
class BeamSearch {
public:
	Error PushScore(const torch::Tensor scores) {
		for (index_t f = 0; f < scores.size(0); f++) {
			std::tuple<torch::Tensor, torch::Tensor> top1 = scores[f].topk(1);
			f32 score = std::get<0>(top1).item<float>();
			i32 index = std::get<1>(top1).item<index_t>();
			if (index != 0) {
				best_path_tokens_.push_back(index);
				best_path_scores_.push_back(score);
			}
		}

		return Error::OK;
	}

	Error EOS() { return Error::OK; }

	const Vec<index_t>& BestPath() {
		return best_path_tokens_;
	}

	void Reset() {
		best_path_tokens_.clear();
		best_path_scores_.clear();
	}

private:
	Vec<index_t> best_path_tokens_;
	Vec<float> best_path_scores_;
}; // class BeamSearch
}  // namespace sio
#endif
