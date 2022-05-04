#include <assert.h>
#include <limits>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "sio/stt.h"

int main() {
    sio::SpeechToTextModule stt_module;
    stt_module.Load("stt_module/stt.json");
    size_t samples_per_chunk = stt_module.config.online ? 1000 : std::numeric_limits<size_t>::max();

    sio::SpeechToTextRuntime stt;
    stt.Load(stt_module);

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
            stt.Speech(&samples[offset], k, sample_rate);
            offset += k;
        }
        stt.To();
        text = stt.Text();
        std::cout << ++ndone << "\t" << audio << "\t" << offset/sample_rate << "\t" << text << "\n";

        stt.Reset();
    }

    return 0;
}

