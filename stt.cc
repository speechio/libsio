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

        kaldi::WaveData wave_data;
        std::ifstream is(audio_path, std::ifstream::binary);
        wave_data.Read(is);
        kaldi::SubVector<float> audio(wave_data.Data(), 0);

        sio::Recognizer* rec = stt.CreateRecognizer();
        assert(rec != nullptr);

        int samples_done = 0;
        while (samples_done < audio.Dim()) {
            int actual_chunk_size = std::min(chunk_size, audio.Dim() - samples_done);
            rec->Speech(
                audio.Data() + samples_done,
                actual_chunk_size,
                wave_data.SampFreq()
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
