#ifndef SIO_H
#define SIO_H

#ifdef __cplusplus
extern "C" {
#endif

struct sio_module {
    void* stt_module;
    void* tts_module;
};

struct sio_stt {
    void* runtime;
};

struct sio_module sio_create(const char* path);
int sio_destroy(struct sio_module);

struct sio_stt sio_stt_create(struct sio_module);
int sio_stt_destroy(struct sio_stt);

int sio_stt_speech(struct sio_stt, const float* samples, int n, float sample_rate);
int sio_stt_to(struct sio_stt);
const char* sio_stt_text(struct sio_stt);
int sio_stt_reset(struct sio_stt);

#ifdef __cplusplus
}
#endif

#endif

