#ifndef SIO_H
#define SIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sio_handle {
	uintptr_t stt_module;
	uintptr_t tts_module;
};

struct sio_handle sio_create(const char* path);
int sio_destroy(struct sio_handle sio);


struct sio_stt_handle {
	uintptr_t runtime;
};

struct sio_stt_handle sio_stt_create(struct sio_handle sio);
int sio_stt_destroy(struct sio_stt_handle stt);

int sio_stt_speech(struct sio_stt_handle stt, const float* samples, int n, float sample_rate);
int sio_stt_to(struct sio_stt_handle stt);
const char* sio_stt_text(struct sio_stt_handle stt);
int sio_stt_reset(struct sio_stt_handle stt);

#ifdef __cplusplus
}
#endif

#endif

