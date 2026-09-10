/* AetherAudio.h — Audio abstraction layer. Backend-agnostic.
 * iOS backend: AVAudioEngine (in Swift). Host backend: stub.
 */
#ifndef AETHER_AUDIO_H
#define AETHER_AUDIO_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_AUDIO_MAX_VOICES 32

typedef enum aether_audio_channel {
    AETHER_CHANNEL_MASTER = 0,
    AETHER_CHANNEL_MUSIC,
    AETHER_CHANNEL_EFFECTS,
    AETHER_CHANNEL_COUNT
} aether_audio_channel_t;

typedef struct aether_audio_handle { u32 id; } aether_audio_handle_t;

typedef struct aether_audio aether_audio_t;

aether_audio_t *aether_audio_create(void);
void            aether_audio_destroy(aether_audio_t *a);

/* Volume (0..1) per channel */
void            aether_audio_set_volume(aether_audio_t *a, aether_audio_channel_t ch, f32 v);
f32             aether_audio_get_volume(const aether_audio_t *a, aether_audio_channel_t ch);
void            aether_audio_set_master_mute(aether_audio_t *a, bool mute);

/* One-shot SFX (fire-and-forget). data must be PCM16 interleaved. */
aether_audio_handle_t aether_audio_play_sfx(aether_audio_t *a,
                                            const u8 *pcm16, u32 byte_size,
                                            u32 sample_rate, u32 channels);

/* Streaming music (looped) — backend may queue, this just registers desire. */
aether_audio_handle_t aether_audio_play_music(aether_audio_t *a, const char *vpath);
void                  aether_audio_stop_music(aether_audio_t *a);

/* Stop a specific handle. */
void            aether_audio_stop(aether_audio_t *a, aether_audio_handle_t h);
void            aether_audio_stop_all(aether_audio_t *a);

/* Called by engine each frame (for fade/cleanup). */
void            aether_audio_tick(aether_audio_t *a, f32 dt);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_AUDIO_H */
