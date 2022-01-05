#include <iostream>

#include "stt.h"

int main() {
	sio::SpeechToText stt;
	stt.Load("testdata/stt.json");

	int samples_per_chunk = std::numeric_limits<int>::max();
	if (stt.config.online == true) {
		samples_per_chunk = 1000;
	}

	sio::Recognizer* rec = stt.CreateRecognizer(); assert(rec != nullptr);

	//std::ifstream wav_scp("testdata/MINI/wav.scp");
	std::ifstream wav_scp("testdata/aishell1-test.scp");
	std::string line;
	int k = 0;
	while (std::getline(wav_scp, line)) {
		std::vector<std::string> fields = absl::StrSplit(line, absl::ByAnyChar(" \t,:;"));
		if (fields.size() != 2) continue;

		std::string& audio_id   = fields[0];
		std::string& audio_path = fields[1];
		SIO_DEBUG << audio_id << " " << audio_path;

		std::vector<float> audio;
		float sample_rate;
		sio::ReadAudio(audio_path, &audio, &sample_rate);


		int N = 0;
		while (N < audio.size()) {
			int n = std::min(samples_per_chunk , (int)audio.size() - N);
			rec->Speech(
				audio.data() + N,
				n,
				sample_rate
			);
			N += n;
		}
		rec->To();

		std::string text;
		rec->Text(&text);
		rec->Reset();
		SIO_DEBUG << audio_id << " -> " << N << " samples decoded: " << text;
		SIO_INFO << k++ << "\t" << audio_id << "\t" << text;
	}

	stt.DestroyRecognizer(rec);

	return 0;
}
