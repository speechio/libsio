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
        loader->AddEntry(module + ".chunk_size", &chunk_size);
        loader->AddEntry(module + ".num_left_chunks", &num_left_chunks);
        loader->AddEntry(module + ".num_threads", &num_threads);
        return Error::OK;
    }
};


class Scorer {
    ScorerConfig config_;
    torch::jit::script::Module* nnet_ = nullptr;
    int nnet_idim_ = 0;
    int nnet_odim_ = 0;

    int subsampling_factor_ = 0;
    int right_context_ = 0;

    // nnet input cache
    std::deque<Vec<f32>> feat_cache_;
    index_t cur_feat_frame_ = 0; // feats[0, cur_feat_frame_) pushed

    // nnet internal cache
    torch::jit::IValue subsampling_cache_;
    torch::jit::IValue elayers_output_cache_;
    torch::jit::IValue conformer_cnn_cache_;
    Vec<torch::Tensor> acoustic_encoding_cache_;

    // nnet output cache
    std::deque<torch::Tensor> scores_cache_;
    index_t cur_score_frame_ = 0; // scores[0, cur_score_frame_) ready, notice: output frame counts is subsampled

public:

    Error Load(const ScorerConfig& config, torch::jit::script::Module& nnet, int nnet_idim, int nnet_odim) { 
        SIO_CHECK(nnet_ == nullptr) << "NNet already initialized";
        config_ = config;
        nnet_ = &nnet;
        nnet_idim_ = nnet_idim;
        nnet_odim_ = nnet_odim;

        torch::set_num_threads(config_.num_threads);
        //at::set_num_threads(config_.num_threads);

        torch::NoGradGuard no_grad;
        nnet_->eval();

        cur_feat_frame_ = 0;
        cur_score_frame_ = 0;

        subsampling_factor_ = nnet_->run_method("subsampling_rate").toInt();
        SIO_DEBUG << "subsampling_factor: " << subsampling_factor_;

        right_context_ = nnet_->run_method("right_context").toInt(); 
        SIO_DEBUG << "right context: " << right_context_;

        return Error::OK;
    }


    void Push(const Vec<f32>& feat_frame) {
        feat_cache_.emplace_back(feat_frame);
        ++cur_feat_frame_;

        if (feat_cache_.size() == config_.chunk_size * subsampling_factor_ + right_context_) {
            Advance();

            while (feat_cache_.size() > right_context_) {
                feat_cache_.pop_front();
            }
        }
    }


    void PushEnd() {
        if (feat_cache_.size() > right_context_) {
            Advance();
        }
        feat_cache_.clear();
    }


    torch::Tensor Pop() {
        torch::Tensor score_frame = scores_cache_.front();
        scores_cache_.pop_front();
        return score_frame;
    }


    Error Reset() {
        feat_cache_.clear();
        cur_feat_frame_ = 0;

        subsampling_cache_ = std::move(torch::jit::IValue());
        elayers_output_cache_ = std::move(torch::jit::IValue());
        conformer_cnn_cache_ = std::move(torch::jit::IValue());
        acoustic_encoding_cache_.clear();

        scores_cache_.clear();
        cur_score_frame_ = 0;

        return Error::OK;
    }


    size_t Len() const {
        return scores_cache_.size();
    }


    size_t Dim() const {
        return 0; // TODO: this should be the dim of nnet output
    }

private:
    Error Advance() {
        torch::NoGradGuard no_grad;

        // Prepare feature chunk tensor: [batch_size = 1, num_cached_frames, feature_dim]
        torch::Tensor chunk_feat = torch::zeros(
            {1, static_cast<long>(feat_cache_.size()), nnet_idim_}, 
            torch::kFloat
        );
        for (index_t f = 0; f != feat_cache_.size(); f++) {
            torch::Tensor frame_tensor = torch::from_blob(
                feat_cache_[f].data(),
                { nnet_idim_ },
                torch::kFloat
            ).clone();
            chunk_feat[0][f] = std::move(frame_tensor);
        }

        // FIX THIS: extremely confusing units due to subsampling factor
        // here offset refers to sub-sampled frames
        // assemble feature and caches as input
        int requried_cache_size = config_.chunk_size * config_.num_left_chunks;
        Vec<torch::jit::IValue> chunk_input = {
            chunk_feat,
            cur_score_frame_,
            requried_cache_size,
            subsampling_cache_,
            elayers_output_cache_,
            conformer_cnn_cache_
        };

        // Encoder forward
        auto r = nnet_->get_method("forward_encoder_chunk")(chunk_input).toTuple()->elements();

        // Cache encoder buffers & results
        SIO_CHECK_EQ(r.size(), 4);
        torch::Tensor acoustic_encoding = r[0].toTensor();
        // what is "assign" semantics for torch::jit:IValue ?
        subsampling_cache_ = r[1];
        elayers_output_cache_ = r[2];
        conformer_cnn_cache_ = r[3];
        acoustic_encoding_cache_.push_back(acoustic_encoding);

        // Compute chunk scores: [frames, nnet_odim]
        torch::Tensor scores = nnet_->run_method("ctc_activation", acoustic_encoding).toTensor()[0];

        // Add chunk score to caches
        for (index_t s = 0; s != scores.size(0); s++) {
            scores_cache_.push_back(scores[s]);
            ++cur_score_frame_;
        }
        //dbg(scores_cache_.size(0), scores_cache_.size(1));

        return Error::OK;
    }

}; // class scorer.h
}  // namespace sio
#endif
