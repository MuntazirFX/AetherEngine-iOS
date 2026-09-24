/* AetherWav.h — WAV RIFF header parse scaffold (body decode optional).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_WAV_H
#define AETHER_WAV_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_wav_info {
    u32 sample_rate;
    u16 channels;
    u16 bits_per_sample;
    u16 audio_format;   /* 1 = PCM */
    u32 data_offset;    /* byte offset of PCM payload in file */
    u32 data_size;      /* PCM payload bytes */
    bool valid;
} aether_wav_info_t;

/* Parse RIFF/WAVE header. Does not decode sample body. */
aether_result_t aether_wav_parse_header(const u8 *data, u32 size, aether_wav_info_t *out);

/* Convenience: parse from file path. */
aether_result_t aether_wav_parse_file(const char *filepath, aether_wav_info_t *out);

/* Build a tiny in-memory PCM WAV (header + sine body) for smoke tests.
 * Returns bytes written into out (0 on failure). */
u32 aether_wav_write_tone_pcm(u8 *out, u32 cap, u32 sample_rate, u16 channels,
                              f32 freq_hz, f32 duration_sec, f32 amplitude);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_WAV_H */
