#include <iostream>

#include "sio/stt.h"
//#include "sio/dbg.h"

int main() {
    sio::SpeechToTextModel model;
    model.Load("stt.json");

    sio::SpeechToText stt;
    stt.Load(model);

    size_t samples_per_chunk = std::numeric_limits<size_t>::max();
    if (model.config.online == true) {
        samples_per_chunk = 1000;
    }

    //std::ifstream wav_scp("testdata/MINI/wav.scp");
    std::ifstream wav_scp("wav.scp");
    std::string line;
    int num_utts = 0;
    while (std::getline(wav_scp, line)) {
        std::vector<std::string> cols = absl::StrSplit(
            line, absl::ByAnyChar(" \t"), absl::SkipWhitespace()
        );
        if (cols.size() != 2) continue;

        std::string& audio_id   = cols[0];
        std::string& audio_path = cols[1];
        SIO_DEBUG << audio_id << " " << audio_path;

        std::vector<float> samples;
        float sample_rate;
        sio::ReadAudio(audio_path, &samples, &sample_rate);

        size_t offset = 0;
        while (offset < samples.size()) {
            size_t n = std::min(samples_per_chunk, samples.size() - offset);
            stt.Speech(&samples[offset], n, sample_rate);
            offset += n;
        }

        stt.To();

        std::string text;
        stt.Text(&text);

        SIO_DEBUG << audio_id << " -> " << offset << " samples decoded: " << text;
        SIO_INFO << num_utts << "\t" << audio_id << "\t" << text;
        num_utts++;

        stt.Reset();
    }

    return 0;
}
