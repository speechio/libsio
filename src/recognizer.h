#ifndef SIO_RECOGNIZER_H
#define SIO_RECOGNIZER_H

#include <stddef.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "sio/base.h"
#include "sio/feature_extractor.h"
#include "sio/tokenizer.h"
#include "sio/scorer.h"
#include "sio/beam_search.h"

//#include "sio/dbg.h"

namespace sio {
class Recognizer {
public:
	Recognizer(
		const FeatureExtractorConfig& feature_extractor_config, const MeanVarNorm* mean_var_norm,
		const ScorerConfig& scorer_config, torch::jit::script::Module& nnet,
		const Tokenizer& tokenizer
	) :
		feature_extractor_(feature_extractor_config, mean_var_norm),
		scorer_(scorer_config, nnet),
		search_(),
		tokenizer_(tokenizer)
	{ }

	Error Speech(const float* samples, size_t num_samples, float sample_rate) {
		SIO_CHECK(samples != nullptr && num_samples != 0);
		return Advance(samples, num_samples, sample_rate, /*eos*/false);
	}

	Error To() { 
		Error err = Advance(nullptr, 0, /*dont care sample rate*/123.456, /*eos*/true);
		return err;
	}

	Error Text(std::string* result) { 
		auto best_path = search_.BestPath();
		for (index_t i = 0; i < best_path.size(); i++) {
			*result += tokenizer_.index_to_token.at(best_path[i]);
		}
		return Error::OK;
	}

	Error Reset() { 
		feature_extractor_.Reset();
		scorer_.Reset();
		search_.Reset();
		return Error::OK; 
	}

private:
	Error Advance(const float* samples, size_t num_samples, float sample_rate, bool eos) {
		if (samples != nullptr && num_samples != 0) {
			feature_extractor_.PushAudio(samples, num_samples, sample_rate);
		}

		if (eos) {
			feature_extractor_.EOS();
		}

		while (feature_extractor_.NumFrames() > 0) {
			Vec<float> frame;
			feature_extractor_.PopFeat(&frame);
			scorer_.PushFeat(frame);
		}

		if (eos) {
			scorer_.EOS();
		}

		while (scorer_.NumChunks() > 0) {
			torch::Tensor chunk_scores = scorer_.PopScore();
			search_.PushScore(chunk_scores);
		}
		
		if (eos) {
			search_.EOS();
		}

		return Error::OK;
	}

	FeatureExtractor feature_extractor_;
	Scorer scorer_;
	BeamSearch search_;
	const Tokenizer& tokenizer_;

}; // class Recognizer
}  // namespace sio
#endif
