#ifndef SIO_H
#define SIO_H

#ifdef __cplusplus
extern "C" {
#endif

void* sio_init(const char* path);
int sio_deinit(void* sio);

void* sio_stt_init(void* sio);
int sio_stt_deinit(void* stt);

int sio_stt_speech(void* stt, const float* samples, int n, float sample_rate);
int sio_stt_to(void* stt);
const char* sio_stt_text(void* stt);
int sio_stt_clear(void* stt);

#ifdef __cplusplus
}
#endif

#endif

