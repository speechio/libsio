#include <assert.h>
#include <limits>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "sio/audio.h"
#include "sio.h"

int main() {
    struct sio_module* siom = sio_create("model/stt.json");
    struct sio_stt* stt = sio_stt_create(siom);

    size_t samples_per_chunk = 1000;
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
        text = sio_stt_text(stt);
        std::cout << ++ndone << "\t" << audio << "\t" << offset/sample_rate << "\t" << text << "\n";

        sio_stt_reset(stt);
    }

    sio_stt_destroy(stt);
    sio_destroy(siom);
    return 0;
}
