#ifndef SIO_AUDIO_H
#define SIO_AUDIO_H

#include <iostream>
#include <string>

#include "feat/wave-reader.h"

#include "sio/base.h"

namespace sio {

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

// Kaldi online feature supports internal resampler:
//   https://github.com/kaldi-asr/kaldi/blob/d366a93aad98127683b010fd01e145093c1e9e08/src/feat/online-feature.cc#L143
// 
// Implementation:
//   https://github.com/kaldi-asr/kaldi/blob/master/src/feat/resample.h#L147

}  // namespace sio
#endif
