#include <iostream>

#include "stt.h"

int main() {
    sio::SpeechToText stt;
    stt.Load("testdata/stt.json");

    int chunk_size = std::numeric_limits<int>::max();
    if (stt.config.online == true) {
        chunk_size = 1000;
    }

    std::ifstream wav_scp("testdata/MINI/wav.scp");
    std::string line;
    while (std::getline(wav_scp, line)) {
        std::vector<std::string> fields = absl::StrSplit(line, absl::ByAnyChar(" \t,:;"));
        if (fields.size() != 2) continue;

        std::string& audio_id   = fields[0];
        std::string& audio_path = fields[1];
        SIO_DEBUG << audio_id << " " << audio_path;

        std::vector<float> audio;
        float sample_rate;
        sio::ReadAudio(audio_path, &audio, &sample_rate);

        sio::Recognizer* rec = stt.CreateRecognizer();
        assert(rec != nullptr);

        int samples_done = 0;
        while (samples_done < audio.size()) {
            int actual_chunk_size = std::min(chunk_size, (int)audio.size() - samples_done);
            rec->Speech(
                audio.data() + samples_done,
                actual_chunk_size,
                sample_rate
            );
            samples_done += actual_chunk_size;
        }
        rec->To();

        std::string text;
        rec->Text(&text);
        stt.DestroyRecognizer(rec);
        SIO_INFO << audio_id << " -> " << samples_done << " samples decoded: " << text;
    }

    return 0;
}
