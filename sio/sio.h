#ifndef SIO_H
#define SIO_H

#ifdef __cplusplus
extern "C" {
#endif

struct sio_stt_model;
struct sio_stt;

struct sio_stt_model* sio_stt_model_create(const char* path);
int sio_stt_model_destroy(struct sio_stt_model* model);

struct sio_stt* sio_stt_create(struct sio_stt_model* model);
int sio_stt_destroy(struct sio_stt* stt);

int sio_stt_speech(struct sio_stt* stt, float* samples, int n, float sample_rate);
int sio_stt_to(struct sio_stt* stt);
const char* sio_stt_text(struct sio_stt* stt);
int sio_stt_reset(struct sio_stt* stt);

#ifdef __cplusplus
}
#endif

#endif

