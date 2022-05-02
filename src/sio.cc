#include "sio.h"

#include "sio/stt.h"

struct sio_module* sio_create(const char* path) {
    sio::SpeechToTextModel* model = new sio::SpeechToTextModel;
    model->Load(path);
    return (sio_module*)model;
}

int sio_destroy(struct sio_module* m) {
    delete (sio::SpeechToTextModel*)m;
    return 0;
}

struct sio_stt* sio_stt_create(struct sio_module* m) {
    sio::SpeechToText *stt = new sio::SpeechToText;
    stt->Load(*(sio::SpeechToTextModel*)m);
    return (struct sio_stt*)stt;
}

int sio_stt_destroy(struct sio_stt* stt) {
    delete (sio::SpeechToText*)stt;
    return 0;
}

int sio_stt_speech(struct sio_stt* stt, float* samples, int n, float sample_rate) {
    return ((sio::SpeechToText*)stt)->Speech(samples, n, sample_rate);
}

int sio_stt_to(struct sio_stt* stt) {
    return ((sio::SpeechToText*)stt)->To();
}

const char* sio_stt_text(struct sio_stt* stt) {
    return ((sio::SpeechToText*)stt)->Text();
}

int sio_stt_reset(struct sio_stt* stt) {
    return ((sio::SpeechToText*)stt)->Reset();
}

