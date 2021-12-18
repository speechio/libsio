#include <iostream>

#include "sio_stt.h"

int main() {
    sio::SpeechToTextConfig config;
    config.feature_config.feature_type = "fbank";

    json::JSON json_config = json::Load("testdata/config.json");

    int chunk_size = std::numeric_limits<int>::max();
    if (config.online == true) {
        chunk_size = 1000;
    }

    sio::SpeechToText<float> stt(config);

    //std::ifstream wav_scp("testdata/MINI/wav.scp");
    std::string scp_file = json_config["wav"].ToString();
    std::ifstream wav_scp(scp_file);
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

        sio::SttStreamHandle stream;
        sio::Error err = stt.CreateStream(&stream, audio_id.c_str());
        assert(!err);

        int samples_done = 0;
        while (samples_done < audio.Dim()) {
            int actual_chunk_size = std::min(chunk_size, audio.Dim() - samples_done);
            stt.SpeechIn(
                stream,
                audio.Data() + samples_done,
                actual_chunk_size,
                wave_data.SampFreq()
            );
            samples_done += actual_chunk_size;
        }
        std::string result;
        stt.TextOut(stream, &result);
        stt.DestroyStream(stream);
        SIO_INFO << audio_id << " -> " << samples_done << " samples decoded: " << result;
    }

    return 0;
}
