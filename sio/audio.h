#ifndef SIO_AUDIO_H
#define SIO_AUDIO_H

#include <iostream>
#include <string>

#include "sio/base.h"

namespace sio {

Error ReadAudio(const std::string& filepath, Vec<f32>* samples, f32* sample_rate);

}  // namespace sio
#endif
