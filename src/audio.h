#ifndef SIO_AUDIO_H
#define SIO_AUDIO_H

#include <iostream>
#include <string>

#include "feat/wave-reader.h"
#include "feat/resample.h"

#include "sio/check.h"

namespace sio {

/*
enum class AudioFormat: int {
  kUnknown,
  kMono8k,
  kMono16k,
};
*/

/* AudioSegment has no ownership */
template <typename SampleType>
struct AudioSegment {
  SampleType* data = nullptr;
  size_t len = 0;
  float sample_rate = 0;

  AudioSegment(SampleType* data, size_t len, float sample_rate) :
    data(data), len(len), sample_rate(sample_rate)
  { }
};

class Resampler {
 public:
  Resampler(float source_sample_rate, float target_sample_rate) :
    // https://github.com/kaldi-asr/kaldi/blob/master/src/feat/resample.cc#L368
    resampler_(
      source_sample_rate, target_sample_rate,
      0.99 * 0.5 * std::min(source_sample_rate, target_sample_rate), // use lower Nyquist as cutoff filtering
      6 // Sinc interpolation truncation
    )
  { }

  void Forward(
    const float* data, size_t len, float sample_rate, 
    kaldi::Vector<float> *output,
    bool flush
  ) {
    SIO_CHECK_EQ(sample_rate, resampler_.GetInputSamplingRate());

    kaldi::SubVector<float> input(data, len);
    resampler_.Resample(input, flush, output);
  }

  float SourceSampleRate() { return resampler_.GetInputSamplingRate(); }
  float TargetSampleRate() { return resampler_.GetOutputSamplingRate(); }

 private:
  // https://github.com/kaldi-asr/kaldi/blob/master/src/feat/resample.h#L147
  kaldi::LinearResample resampler_;

}; // class Resampler

}  // namespace sio
#endif
