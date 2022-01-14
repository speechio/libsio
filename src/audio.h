#ifndef SIO_AUDIO_H
#define SIO_AUDIO_H

#include <iostream>
#include <string>

#include "feat/wave-reader.h"
//#include "feat/resample.h"

#include "sio/common.h"

namespace sio {

/*
enum class AudioFormat: int {
  kUnknown,
  kMono8k,
  kMono16k,
};
*/

inline Error ReadAudio(const std::string& filepath, Vec<f32>* audio, f32* sample_rate) {
    kaldi::WaveData kaldi_wave;
    std::ifstream is(filepath, std::ifstream::binary);
    kaldi_wave.Read(is);
    *sample_rate = kaldi_wave.SampFreq();
    kaldi::SubVector<f32> ch0(kaldi_wave.Data(), 0);

    SIO_CHECK(audio != nullptr);
    if (!audio->empty()) {
        audio->clear();
    }
    audio->resize(ch0.Dim(), 0.0f);

    for (int i = 0; i < ch0.Dim(); i++) {
        (*audio)[i] = ch0(i);
    }
    return Error::OK;
}

template <typename SampleT>
struct AudioSegment {
    SampleT* data = nullptr; // no ownership
    size_t len = 0;
    f32 sample_rate = 0;

    void Set(SampleT* samples, size_t n, f32 sample_rate) { 
        data = samples;
        len = n;
        sample_rate = sample_rate;
    }
};

/* 
//Kaldi online feature supports internal resampler:
//  https://github.com/kaldi-asr/kaldi/blob/d366a93aad98127683b010fd01e145093c1e9e08/src/feat/online-feature.cc#L143
//so the following class is probably not necessary

class Resampler {
public:
    Resampler(f32 source_sample_rate, f32 target_sample_rate) :
        // https://github.com/kaldi-asr/kaldi/blob/master/src/feat/resample.cc#L368
        resampler_(
            source_sample_rate, target_sample_rate,
            0.99 * 0.5 * std::min(source_sample_rate, target_sample_rate), // use lower Nyquist as cutoff filtering
            6 // Sinc interpolation truncation    
        )
    { }

    void Forward(
        const f32* data, size_t len, f32 sample_rate, 
        kaldi::Vector<f32> *output,
        bool flush
    ) {
        SIO_CHECK_EQ(sample_rate, resampler_.GetInputSamplingRate());

        kaldi::SubVector<f32> input(data, len);
        resampler_.Resample(input, flush, output);
    }

    f32 SourceSampleRate() { return resampler_.GetInputSamplingRate(); }
    f32 TargetSampleRate() { return resampler_.GetOutputSamplingRate(); }

private:
    // https://github.com/kaldi-asr/kaldi/blob/master/src/feat/resample.h#L147
    kaldi::LinearResample resampler_;

}; // class Resampler
*/

}  // namespace sio
#endif
