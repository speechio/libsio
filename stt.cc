#include <assert.h>
#include <limits>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "sio/stt.h"

int main() {
    sio::SpeechToTextModel model;
    model.Load("stt.json");

    sio::SpeechToText stt;
    stt.Load(model);

    size_t samples_per_chunk = model.config.online ? 1000 : std::numeric_limits<size_t>::max();

    std::ifstream audio_list("wav.list");
    std::string audio;
    int num_utts = 0;

    while (std::getline(audio_list, audio)) {
        std::vector<float> samples;
        float sample_rate;
        sio::ReadAudio(audio, &samples, &sample_rate);
        assert(sample_rate == 16000.0);

        size_t offset = 0;
        while (offset < samples.size()) {
            size_t n = std::min(samples_per_chunk, samples.size() - offset);
            stt.Speech(&samples[offset], n, sample_rate);
            offset += n;
        }

        stt.To();

        std::string text;
        stt.Text(&text);

        std::cout << ++num_utts << "\t" << audio << "\t" << offset/sample_rate << "\t" << text << "\n";

        stt.Reset();
    }

    return 0;
}
