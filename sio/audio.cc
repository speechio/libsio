#include "sio/audio.h"

#include <iostream>

#include "feat/wave-reader.h"

namespace sio {

Error ReadAudio(const Str& filepath, Vec<f32>* samples, f32* sample_rate) {
    std::ifstream is(filepath, std::ifstream::binary);
	if (!is.good()) {
        return Error::OpenFileFailure;
    }

    kaldi::WaveData kaldi_wave;
    kaldi_wave.Read(is);

    *sample_rate = kaldi_wave.SampFreq();
    kaldi::SubVector<f32> ch0(kaldi_wave.Data(), 0);

    SIO_CHECK(samples != nullptr);
    if (!samples->empty()) {
        samples->clear();
    }
    samples->resize(ch0.Dim(), 0.0f);

    for (int i = 0; i < ch0.Dim(); i++) {
        (*samples)[i] = ch0(i);
    }

    return Error::OK;
}

} // namespace sio
