#include "sio.h"

#include "sio/stt.h"

int sio_init(const char* path, struct sio_package* pkg) {
    SIO_CHECK(pkg != nullptr);
    SIO_CHECK(pkg->stt_module == nullptr);

    sio::SpeechToTextModule* p = new sio::SpeechToTextModule;
    int err = p->Load(path);

    if (!err) {
        pkg->stt_module = p;
    } else {
        delete p;
    }

    return err;
}

int sio_deinit(struct sio_package* pkg) {
    SIO_CHECK(pkg != nullptr);

    delete (sio::SpeechToTextModule*)pkg->stt_module;
    pkg->stt_module = nullptr;

    return 0;
}

int sio_stt_init(struct sio_package pkg, struct sio_stt* stt) {
    SIO_CHECK(pkg.stt_module != nullptr);

    SIO_CHECK(stt != nullptr);
    SIO_CHECK(stt->runtime == nullptr);

    sio::SpeechToTextRuntime* p = new sio::SpeechToTextRuntime;
    int err = p->Load(*(sio::SpeechToTextModule*)pkg.stt_module);

    if (!err) {
        stt->runtime = p;
    } else {
        delete p;
    }

    return err;
}

int sio_stt_deinit(struct sio_stt* stt) {
    SIO_CHECK(stt != nullptr);

    delete (sio::SpeechToTextRuntime*)stt->runtime;
    stt->runtime = nullptr;

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

