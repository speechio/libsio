#include "sio.h"

#include "sio/stt.h"

struct sio_stt_model* sio_stt_model_create(const char* path) { return nullptr;}
int sio_stt_model_destroy(struct sio_stt_model* model) { return 0; }

struct sio_stt* sio_stt_create(struct sio_stt_model* model) { return nullptr; }
int sio_stt_destroy(struct sio_stt* stt) { return 0; }

int sio_stt_speech(struct sio_stt* stt, float* samples, int n, float sample_rate) { return 0; }
int sio_stt_to(struct sio_stt* stt) { return 0; }
const char* sio_stt_text(struct sio_stt* stt) { return nullptr; }
int sio_stt_reset(struct sio_stt* stt) { return 0; }

