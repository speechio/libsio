#ifndef SIO_SCORER_H
#define SIO_SCORER_H

#include "torch/script.h"
#include "torch/torch.h"

#include "sio/base.h"
#include "sio/tokenizer.h"

#include "sio/dbg.h"

namespace sio {

struct ScorerConfig {
  int chunk_size = 8;
  int num_left_chunks = -1;

  Error Register(StructLoader* loader, const std::string module = "") {
    loader->AddEntry(module, ".chunk_size", &chunk_size);
    loader->AddEntry(module, ".num_left_chunks", &num_left_chunks);
    return Error::OK;
  }
};

class Scorer {
 public:
  Scorer(
    const Tokenizer& tokenizer,
    const ScorerConfig & config,
    torch::jit::script::Module& nnet, 
    int num_threads = 1
  ) : 
    tokenizer_(tokenizer),
    config_(config),
    nnet_(nnet),
    offset_(0)
  { 
    //torch::set_num_threads(1); ?
    //at::set_num_threads(num_threads);

    torch::NoGradGuard no_grad;
    nnet_.eval();

    subsampling_factor_ = nnet_.run_method("subsampling_rate").toInt();
    SIO_DEBUG << "subsampling_factor: " << subsampling_factor_;

    right_context_ = nnet_.run_method("right_context").toInt(); 
    SIO_DEBUG << "right context: " << right_context_;
  }

  void PushFeat(const Vec<float>& frame) {
    feat_cache_.emplace_back(frame);
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
      offset_,
      requried_cache_size,
      subsampling_cache_,
      elayers_output_cache_,
      conformer_cnn_cache_
    };

    // Forward encoder layers
    auto r = nnet_.get_method("forward_encoder_chunk")(inputs).toTuple()->elements();
    SIO_CHECK_EQ(r.size(), 4);

    // what is "assign" semantics for torch::Tensor? ref-counted shared ownership?
    torch::Tensor encoder_out = r[0].toTensor();

    // what is "assign" semantics for torch::jit:IValue ?
    subsampling_cache_ = r[1];
    elayers_output_cache_ = r[2];
    conformer_cnn_cache_ = r[3];

    offset_ += encoder_out.size(1);

    // how this move semantic actually acts for a slice of tensor, i.e. toTensor()[0]
    scores_.push_back(
      std::move(
        nnet_.run_method("ctc_activation", encoder_out).toTensor()[0]
      )
    );
    encoder_outs_.push_back(std::move(encoder_out));
    //dbg(scores_.size(0), scores_.size(1));
  }

  size_t Empty() const {
    return scores_.empty();
  }

  Vec<torch::Tensor> PopScore() { return std::move(scores_); }

  void Reset() {
    feat_cache_.clear();

    subsampling_cache_ = std::move(torch::jit::IValue());
    elayers_output_cache_ = std::move(torch::jit::IValue());
    conformer_cnn_cache_ = std::move(torch::jit::IValue());
    encoder_outs_.clear();
    scores_.clear();
    offset_; // this is offset of subsampled caches
  }

 private:
  ScorerConfig config_;
  const Tokenizer& tokenizer_;
  torch::jit::script::Module& nnet_;

  int subsampling_factor_;
  int right_context_;

  std::deque<Vec<float>> feat_cache_;

  torch::jit::IValue subsampling_cache_;
  torch::jit::IValue elayers_output_cache_;
  torch::jit::IValue conformer_cnn_cache_;
  Vec<torch::Tensor> encoder_outs_;
  i64 offset_;

  Vec<torch::Tensor> scores_;
};

} // namespace sio
#endif
