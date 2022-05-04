#include "sio.h"

#include "sio/stt.h"

struct sio_module sio_create(const char* path) {
    sio::SpeechToTextModule* p = new sio::SpeechToTextModule;
    p->Load(path);
    return { (void*)p };
}

int sio_destroy(struct sio_module m) {
    delete (sio::SpeechToTextModule*)m.stt_module;
    return 0;
}

struct sio_stt sio_stt_create(struct sio_module m) {
    sio::SpeechToTextRuntime* p = new sio::SpeechToTextRuntime;
    p->Load(*(sio::SpeechToTextModule*)m.stt_module);
    return { (void*)p };
}

int sio_stt_destroy(struct sio_stt stt) {
    delete (sio::SpeechToTextRuntime*)stt.runtime;
    return 0;
}

int sio_stt_speech(struct sio_stt stt, const float* samples, int n, float sample_rate) {
    return ((sio::SpeechToTextRuntime*)stt.runtime)->Speech(samples, n, sample_rate);
}

int sio_stt_to(struct sio_stt stt) {
    return ((sio::SpeechToTextRuntime*)stt.runtime)->To();
}

const char* sio_stt_text(struct sio_stt stt) {
    return ((sio::SpeechToTextRuntime*)stt.runtime)->Text();
}

int sio_stt_reset(struct sio_stt stt) {
    return ((sio::SpeechToTextRuntime*)stt.runtime)->Reset();
}

