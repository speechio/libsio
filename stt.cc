
#include <iostream>
#include "feat/wave-reader.h"
#include "util/common-utils.h"
#include "base/kaldi-error.h"
#include "itf/online-feature-itf.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "hmm/transition-model.h"
#include "nnet3/nnet-utils.h"
#include "nnet3/decodable-online-looped.h"
#include "lat/lattice-functions.h"
#include "lat/determinize-lattice-pruned.h"
#include "online2/online-timing.h"

#include "sio/sio.h"

int main() {
    using namespace kaldi;
    using namespace sio;

    float chunk_secs = 0.2;

    i32 num_err = 0;
    std::ifstream wav_scp("testdata/MINI/wav.scp");
    std::string line;
    while (getline(wav_scp, line)) {
        std::vector<std::string> cols = absl::StrSplit(line, absl::ByAnyChar(" \t,:;"));
        if (cols.size() != 2) continue;

        std::string audio_key  = cols[0];
        std::string audio_path = cols[1];
        SIO_DEBUG << audio_key << " " << audio_path;

        WaveData wave_data;
        std::ifstream stream(audio_path, std::ifstream::binary);
        wave_data.Read(stream);
        float sample_rate = wave_data.SampFreq();
        SIO_CHECK(sample_rate == 16000, "sample rate is not 16k.");
        SubVector<BaseFloat> audio(wave_data.Data(), 0); // only use channel 0

        int chunk_samples;
        if (chunk_secs > 0) {
            chunk_samples = std::max(1, int(sample_rate * chunk_secs));
        } else {
            chunk_samples = std::numeric_limits<int>::max();
        }

        OnlineTimer decoding_timer(audio_key);
        //recognizer->StartSession(audio_key.c_str());
        int samples_done = 0;
        while (samples_done < audio.Dim()) {
            int samples_remaining = audio.Dim() - samples_done;
            int n = chunk_samples < samples_remaining ? chunk_samples : samples_remaining;
    
            SubVector<BaseFloat> audio_chunk(audio, samples_done, n);
            //recognizer->AcceptAudioChunk(audio_chunk.Data(), audio_chunk.SizeInBytes(), audio_format);
    
            samples_done += n;
            decoding_timer.WaitUntil(samples_done / sample_rate);
    
            //if (opts.do_endpointing && recognizer->EndOfSentenceDetected()) {
            //    break;
            //}
            SIO_DEBUG << audio_key << ": " << samples_done << " samples decoded.";
        }
        SIO_INFO << "Decoded " << samples_done << " samples for " << audio_key;
        //recognizer->StopSession();
    }

    return 0;
}
