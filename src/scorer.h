#ifndef SIO_SCORER_H
#define SIO_SCORER_H

#include "torch/script.h"
#include "torch/torch.h"

#include "sio/common.h"
#include "sio/tokenizer.h"

//#include "sio/dbg.h"

namespace sio {

struct ScorerConfig {
    int chunk_size = 8;
    int num_left_chunks = -1;
    int num_threads = 1;

    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module, ".chunk_size", &chunk_size);
        loader->AddEntry(module, ".num_left_chunks", &num_left_chunks);
        loader->AddEntry(module, ".num_threads", &num_threads);
        return Error::OK;
    }
};

class Scorer {
 public:
    Scorer(const ScorerConfig & config, torch::jit::script::Module& nnet) : 
        config_(config),
        nnet_(nnet)
    { 
        torch::set_num_threads(config_.num_threads);
        //at::set_num_threads(config_.num_threads);

        torch::NoGradGuard no_grad;
        nnet_.eval();

        subsampling_factor_ = nnet_.run_method("subsampling_rate").toInt();
        SIO_DEBUG << "subsampling_factor: " << subsampling_factor_;

        right_context_ = nnet_.run_method("right_context").toInt(); 
        SIO_DEBUG << "right context: " << right_context_;

        cur_iframe_ = 0;
        cur_oframe_ = 0;
    }

    void PushFeat(const Vec<float>& frame) {
        feat_cache_.emplace_back(frame);
        ++cur_iframe_;
    }

    void EOS() {
        torch::NoGradGuard no_grad;

        // init empty feature tensor
        const int feature_dim = 80;
        torch::Tensor feats = torch::zeros(
            {
                1, 
                static_cast<long>(feat_cache_.size()), 
                feature_dim
            }, 
            torch::kFloat
        );

        // fill feature tensor with cached feature frames
        for (index_t f = 0; f != feat_cache_.size(); f++) {
            torch::Tensor frame_tensor = torch::from_blob(
                feat_cache_[f].data(),
                { feature_dim },
                torch::kFloat
            ).clone();
            feats[0][f] = std::move(frame_tensor);
        }

        // FIX THIS: extremely confusing units due to subsampling factor
        // here offset refers to sub-sampled frames
        // assemble feature and caches as input
        int requried_cache_size = config_.chunk_size * config_.num_left_chunks;
        std::vector<torch::jit::IValue> inputs = {
            feats,
            cur_oframe_,
            requried_cache_size,
            subsampling_cache_,
            elayers_output_cache_,
            conformer_cnn_cache_
        };

        // Forward encoder layers
        auto r = nnet_.get_method("forward_encoder_chunk")(inputs).toTuple()->elements();

        SIO_CHECK_EQ(r.size(), 4);
        torch::Tensor acoustic_encoding = r[0].toTensor();
        // what is "assign" semantics for torch::jit:IValue ?
        subsampling_cache_ = r[1];
        elayers_output_cache_ = r[2];
        conformer_cnn_cache_ = r[3];
        acoustic_encoding_cache_.push_back(acoustic_encoding);

        torch::Tensor scores = 
        nnet_.run_method("ctc_activation", acoustic_encoding).toTensor()[0]; // batch_size(i.e. dim0) = 1
        // scores: [frames, nnet_odim]

        scores_cache_.push_back(scores);
        cur_oframe_ += scores.size(1);
        //dbg(scores_cache_.size(0), scores_cache_.size(1));
    }

    size_t NumChunks() const {
        return scores_cache_.size();
    }

    torch::Tensor PopScore() { 
        torch::Tensor one_chunk_scores = scores_cache_.front();
        scores_cache_.pop_front();
        return one_chunk_scores;
    }

    void Reset() {
        feat_cache_.clear();
        cur_iframe_ = 0;

        subsampling_cache_ = std::move(torch::jit::IValue());
        elayers_output_cache_ = std::move(torch::jit::IValue());
        conformer_cnn_cache_ = std::move(torch::jit::IValue());
        acoustic_encoding_cache_.clear();
        scores_cache_.clear();
        cur_oframe_ = 0;
    }

private:
    ScorerConfig config_;
    torch::jit::script::Module& nnet_;

    int subsampling_factor_;
    int right_context_;

    // nnet input cache
    std::deque<Vec<float>> feat_cache_;
    index_t cur_iframe_; // feats[0, cur_iframe_) pushed

    // nnet internal cache
    torch::jit::IValue subsampling_cache_;
    torch::jit::IValue elayers_output_cache_;
    torch::jit::IValue conformer_cnn_cache_;
    Vec<torch::Tensor> acoustic_encoding_cache_;

    // nnet output cache
    std::deque<torch::Tensor> scores_cache_;
    index_t cur_oframe_; // scores[0, cur_oframe_) ready, notice: output frame counts is subsampled
};

} // namespace sio
#endif
