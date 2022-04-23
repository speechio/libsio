#ifndef SIO_AUDIO_H
#define SIO_AUDIO_H

#include "sio/base.h"

namespace sio {

Error ReadAudio(const Str& filepath, Vec<f32>* samples, f32* sample_rate);

}  // namespace sio
#endif
