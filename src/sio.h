#ifndef SIO_H
#define SIO_H

#ifdef __cplusplus
extern "C" {
#endif

struct sio_package {
    void* stt_module;
    //void* tts_module;
};

struct sio_stt {
    void* handle;
};

int sio_init(const char* path, struct sio_package*);
int sio_deinit(struct sio_package*);

int sio_stt_init(struct sio_package, struct sio_stt*);
int sio_stt_deinit(struct sio_stt*);

int sio_stt_speech(struct sio_stt, const float* samples, int n, float sample_rate);
int sio_stt_to(struct sio_stt);
const char* sio_stt_text(struct sio_stt);
int sio_stt_clear(struct sio_stt);

#ifdef __cplusplus
}
#endif

#endif

