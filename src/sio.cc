#include "sio.h"

#include "sio/stt.h"

struct sio_module* sio_create(const char* path) {
	sio::SpeechToTextModel* model = new sio::SpeechToTextModel;
	model->Load(path);
	return (sio_module*)model;
}

int sio_destroy(struct sio_module* m) {
	sio::SpeechToTextModel* model = (sio::SpeechToTextModel*)m;
	delete model;
	return 0;
}

struct sio_stt* sio_stt_create(struct sio_module* m) {
	sio::SpeechToTextModel* model = (sio::SpeechToTextModel*)m;
	sio::SpeechToText *stt = new sio::SpeechToText;
	stt->Load(*model);
	return (struct sio_stt*)stt;
}

int sio_stt_destroy(struct sio_stt* stt) {
	delete (sio::SpeechToText*)stt;
	return 0;
}

int sio_stt_speech_f32(struct sio_stt* stt, float* samples, int n, float sample_rate) {
	sio::SpeechToText* p = (sio::SpeechToText*)stt;
	p->Speech(samples, n, sample_rate);
	return 0;
}

int sio_stt_to(struct sio_stt* stt) {
	sio::SpeechToText* p = (sio::SpeechToText*)stt;
	p->To();
	return 0;
}

const char* sio_stt_text(struct sio_stt* stt) { return nullptr; }

int sio_stt_reset(struct sio_stt* stt) {
	sio::SpeechToText* p = (sio::SpeechToText*)stt;
	p->Reset();
	return 0;
}

