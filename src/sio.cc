#include "sio.h"
#include "sio/stt.h"

void* sio_init(const char* path) {
    SIO_CHECK(path != nullptr);

    sio::SpeechToTextModule* p = new sio::SpeechToTextModule;
    int err = p->Load(path);
    if (err) {
        delete p;
        return nullptr;
    } else {
        return p;
    }
}

int sio_deinit(void* sio) {
    SIO_CHECK(sio != nullptr);
    delete (sio::SpeechToTextModule*)sio;

    return 0;
}

void* sio_stt_init(void* sio) {
    SIO_CHECK(sio != nullptr);

    sio::SpeechToText* p = new sio::SpeechToText;
    int err = p->Load(*(sio::SpeechToTextModule*)sio);
    if (err) {
        delete p;
        return nullptr;
    } else {
        return p;
    }
}

int sio_stt_deinit(void* stt) {
    SIO_CHECK(stt != nullptr);
    delete (sio::SpeechToText*)stt;

    return 0;
}

int sio_stt_speech(void* stt, const float* samples, int n, float sample_rate) {
    return ((sio::SpeechToText*)stt)->Speech(samples, n, sample_rate);
}

int sio_stt_to(void* stt) {
    return ((sio::SpeechToText*)stt)->To();
}

const char* sio_stt_text(void* stt) {
    return ((sio::SpeechToText*)stt)->Text();
}

int sio_stt_clear(void* stt) {
    return ((sio::SpeechToText*)stt)->Clear();
}

