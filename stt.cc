#include <stdlib.h>
#include <assert.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "sio/audio.h" // import sio::ReadAudio()
#include "sio.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage:\n  %s <audio_list>\n", argv[0]);
        return 0;
    }
    const char* audio_list = argv[1];
    const char* config = std::getenv("SIO") ? std::getenv("SIO") : "sio.json";

    struct sio_package sio = {}; // Zerolization Is Initialization(ZII) is required here
    sio_init(config, &sio);

    struct sio_stt stt = {};
    sio_stt_init(sio, &stt);

    std::ifstream audios(audio_list);
    std::string audio;

    int ndone = 0;
    while (std::getline(audios, audio)) {
        std::vector<float> samples;
        float sample_rate;

        sio::ReadAudio(audio, &samples, &sample_rate);
        assert(samples.size() != 0 && sample_rate == 16000.0);

        size_t offset = 0;
        size_t samples_per_chunk = sample_rate/20; // 50ms
        while (offset < samples.size()) { // streaming via successive Speech() calls
            size_t k = std::min(samples_per_chunk, samples.size() - offset);
            sio_stt_speech(stt, &samples[offset], k, sample_rate);
            offset += k;
        }
        sio_stt_to(stt);
        std::cout << ++ndone
                  << "\t" << audio
                  << "\t" << offset/sample_rate
                  << "\t" << sio_stt_text(stt)
                  << "\n";

        sio_stt_clear(stt);
    }

    sio_stt_deinit(&stt);
    sio_deinit(&sio);

    return 0;
}

