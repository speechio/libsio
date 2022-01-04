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
    torch::jit::script::Module& model, 
    int num_threads = 1
  ) : 
    tokenizer_(tokenizer),
    config_(config),
    model_(model),
    offset_(0)
  { 
    //torch::set_num_threads(1); ?
    at::set_num_threads(num_threads);

    torch::NoGradGuard no_grad;
    model_.eval();

    subsampling_factor_ = model_.run_method("subsampling_rate").toInt();
    SIO_DEBUG << "subsampling_factor: " << subsampling_factor_;

    right_context_ = model_.run_method("right_context").toInt(); 
    SIO_DEBUG << "right context: " << right_context_;
  }

  void PushFeat(const Vec<float>& frame) {
    feat_cache_.emplace_back(frame);
    // this may go to private member
    //const int chunk_size_frames = config_.chunk_size * subsampling_factor_;
    //if (config_.chunk_size > 0 /* streaming */ && 
    //    feat_cache_.size() == chunk_size_frames + right_context_ /* accumulate to a chunk */) {
    //  // a chunk ready
    //  const int feature_dim = frame.size();
    //  torch::Tensor chunk = torch::zeros({1, static_cast<long>(feat_cache_.size()), feature_dim}, torch::kFloat);
    //  for (index_t f = 0; f != feat_cache_.size(); f++) {
    //    torch::Tensor frame_tensor = torch::from_blob(
    //      feat_cache_[f].data(),
    //      {feature_dim},
    //      torch::kFloat
    //    ).clone();
    //    chunk[0][f] = std::move(frame_tensor);
    //  }

    //  // FIX THIS: extremely confusing of count units due to subsampling factor
    //  int requried_cache_size = config_.chunk_size * config_.num_left_chunks;
    //  std::vector<torch::jit::IValue> inputs = {
    //    chunk,
    //    offset_,
    //    requried_cache_size,
    //    subsampling_cache_,
    //    elayers_output_cache_,
    //    conformer_cnn_cache_
    //  };

    //  auto outputs = model_.get_method("forward_encoder_chunk")(inputs).toTuple()->elements();

    //  CHECK_EQ(outputs.size(), 4);
    //  torch::Tensor chunk_out = outputs[0].toTensor();
    //  subsampling_cache_ = outputs[1];
    //  elayers_output_cache_ = outputs[2];
    //  conformer_cnn_cache_ = outputs[3];

    //  //dbg(chunk_out.size(0), chunk_out.size(1), chunk_out.size(2));

    //  for (index_t f = 0; f != chunk_size_frames; f++) {
    //    feat_cache_.pop_front();
    //    feat_begin_ += 1;
    //  }

    //  offset_ +=  chunk_out.size(1);

    //  torch::Tensor ctc_log_probscoress = model_.run_method("ctc_activation", chunk_out).toTensor()[0];
    //  //dbg(ctc_log_probscoress.size(0), ctc_log_probscoress.size(1));

    //  for (index_t f = 0; f != ctc_log_probscoress.size(0); f++) {
    //    std::tuple<torch::Tensor, torch::Tensor> best = ctc_log_probscoress[f].topk(1);
    //    float score = std::get<0>(best).item<float>();
    //    int idx = std::get<1>(best).item<int>();
    //    if (idx != 0) {
    //      SIO_DEBUG << "frame:" << f << " score:" << score <<  " idx:" << idx << " token:" << tokens_[idx];
    //    }
    //  }
    //}
  }

  void EndOfFeats() { 
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

    // encoder forward
    auto r = model_.get_method("forward_encoder_chunk")(inputs).toTuple()->elements();
    SIO_CHECK_EQ(r.size(), 4);

    // what is "assign" semantics for torch::Tensor? ref-counted shared ownership?
    torch::Tensor encoder_out = r[0].toTensor();

    // what is "assign" semantics for torch::jit:IValue ?
    subsampling_cache_ = r[1];
    elayers_output_cache_ = r[2];
    conformer_cnn_cache_ = r[3];

    offset_ += encoder_out.size(1);

    torch::Tensor scores = model_.run_method("ctc_activation", encoder_out).toTensor()[0];
    encoder_outs_.push_back(std::move(encoder_out));
    //dbg(ctc_log_probscoress.size(0), ctc_log_probscoress.size(1));

    // greedy search
    for (index_t f = 0; f != scores.size(0); f++) {
      std::tuple<torch::Tensor, torch::Tensor> top1 = scores[f].topk(1);
      f32 score = std::get<0>(top1).item<f32>();
      i32 index = std::get<1>(top1).item<i32>();
      if (index != 0) {
        result_ += tokenizer_.index_to_token.at(dbg(index));
        SIO_DEBUG << " frame:" << f 
                  << " score:" << score 
                  << " index:" << index 
                  << " token:" << tokenizer_.index_to_token.at(index);
      }
    }
  }

  void PopScore(Vec<float>* score) { }

  void Reset() {
    feat_cache_.clear();

    subsampling_cache_ = std::move(torch::jit::IValue());
    elayers_output_cache_ = std::move(torch::jit::IValue());
    conformer_cnn_cache_ = std::move(torch::jit::IValue());
    encoder_outs_.clear();
    offset_; // this is offset of subsampled caches
    result_ = "";
  }

  Str Result() { return result_; }

 private:
  ScorerConfig config_;
  const Tokenizer& tokenizer_;
  torch::jit::script::Module& model_;

  int subsampling_factor_;
  int right_context_;

  std::deque<Vec<float>> feat_cache_;

  torch::jit::IValue subsampling_cache_;
  torch::jit::IValue elayers_output_cache_;
  torch::jit::IValue conformer_cnn_cache_;
  Vec<torch::Tensor> encoder_outs_;
  i64 offset_;

  Str result_;
};

} // namespace sio
#endif
