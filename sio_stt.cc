#include <assert.h>
#include <limits>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "sio/stt.h" // libsioxx
#include "sio.h" // libsio

int main() {
    struct sio_module sio = {}; // Zerolization Is Initialization(ZII) required here
    sio_init("model/stt.json", &sio);

    struct sio_stt stt = {}; // ZII required here
    sio_stt_init(sio, &stt);

    size_t samples_per_chunk = 1600;

    std::ifstream audio_list("wav.list");
    std::string audio;
    int ndone = 0;

    while (std::getline(audio_list, audio)) {
        std::vector<float> samples;
        float sample_rate;

        sio::ReadAudio(audio, &samples, &sample_rate);
        assert(samples.size() != 0 && sample_rate == 16000.0);

        std::string text;
        size_t offset = 0;
        while (offset < samples.size()) { // streaming via successive Speech() calls
            size_t k = std::min(samples_per_chunk, samples.size() - offset);
            sio_stt_speech(stt, &samples[offset], k, sample_rate);
            offset += k;
        }
        sio_stt_to(stt);
        text = sio_stt_text(stt); // const char* -> str::string copy
        sio_stt_reset(stt);

        std::cout << ++ndone << "\t" << audio << "\t" << offset/sample_rate << "\t" << text << "\n";
    }

    sio_stt_deinit(&stt);
    sio_deinit(&sio);

    return 0;
}

