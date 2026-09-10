/* AetherAudio.h — Platform-agnostic audio mixer abstraction.
 * Actual playback is delegated to platform backends (iOS: AVAudioEngine).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_AUDIO_H
#define AETHER_AUDIO_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MAX_AUDIO_VOICES 32
#define AETHER_AUDIO_NAME_MAX   128

typedef enum aether_audio_channel {
    AETHER_AUDIO_CHANNEL_MASTER = 0,
    AETHER_AUDIO_CHANNEL_MUSIC,
    AETHER_AUDIO_CHANNEL_EFFECTS,
    AETHER_AUDIO_CHANNEL_VOICE,
    AETHER_AUDIO_CHANNEL_COUNT
} aether_audio_channel_t;

typedef enum aether_audio_state {
    AETHER_AUDIO_STATE_UNINIT = 0,
    AETHER_AUDIO_STATE_READY,
    AETHER_AUDIO_STATE_SUSPENDED,
} aether_audio_state_t;

typedef struct aether_audio_voice {
    u32         id;                        /* voice slot */
    char        asset[AETHER_AUDIO_NAME_MAX]; /* logical path, e.g. "sound/ambience.wav" */
    aether_audio_channel_t channel;
    f32         volume;                    /* 0..1 (voice-local) */
    bool        loop;
    bool        playing;
} aether_audio_voice_t;

typedef struct aether_audio aether_audio_t;

/* Lifecycle */
aether_audio_t *aether_audio_create(void);
void            aether_audio_destroy(aether_audio_t *a);
aether_result_t aether_audio_init(aether_audio_t *a);
aether_result_t aether_audio_shutdown(aether_audio_t *a);

/* Volume controls */
void aether_audio_set_master_volume(aether_audio_t *a, f32 vol);   /* 0..1 */
void aether_audio_set_channel_volume(aether_audio_t *a, aether_audio_channel_t ch, f32 vol);
f32  aether_audio_get_master_volume(const aether_audio_t *a);
f32  aether_audio_get_channel_volume(const aether_audio_t *a, aether_audio_channel_t ch);
void aether_audio_set_mute(aether_audio_t *a, bool muted);
bool aether_audio_is_muted(const aether_audio_t *a);

/* Playback requests (recorded into a queue for the platform to consume) */
i32             aether_audio_play_effect(aether_audio_t *a, const char *asset_path,
                                         f32 volume, bool loop);
aether_result_t aether_audio_stop_voice(aether_audio_t *a, i32 voice_id);
void            aether_audio_stop_channel(aether_audio_t *a, aether_audio_channel_t ch);
void            aether_audio_stop_all(aether_audio_t *a);

/* Voice inspection (read-only) */
u32                            aether_audio_voice_count(const aether_audio_t *a);
const aether_audio_voice_t    *aether_audio_voice_at(const aether_audio_t *a, u32 idx);

/* The platform (Swift) layer pulls pending commands via this callback. */
typedef void (*aether_audio_platform_fn)(const aether_audio_voice_t *voice,
                                         const char *command,
                                         void *user);
void aether_audio_set_platform_callback(aether_audio_t *a,
                                        aether_audio_platform_fn fn,
                                        void *user);

/* Internal: called by the engine to flush queued commands. */
void aether_audio_flush(aether_audio_t *a);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_AUDIO_H */
