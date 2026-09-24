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
bool            aether_audio_is_ready(const aether_audio_t *a);
aether_audio_state_t aether_audio_get_state(const aether_audio_t *a);

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

/* PCM buffer submit for platform backends (iOS AVAudioEngine, host stub). */
typedef struct aether_audio_buffer {
    const i16 *samples;   /* interleaved PCM16 */
    u32        frame_count;
    u32        sample_rate;
    u16        channels;
    f32        volume;    /* 0..1 */
} aether_audio_buffer_t;

typedef void (*aether_audio_buffer_fn)(const aether_audio_buffer_t *buf, void *user);
void aether_audio_set_buffer_callback(aether_audio_t *a,
                                      aether_audio_buffer_fn fn,
                                      void *user);
aether_result_t aether_audio_submit_buffer(aether_audio_t *a,
                                           const aether_audio_buffer_t *buf);

/* Procedural beep/tone (no WAV asset required). Generates PCM and submits. */
aether_result_t aether_audio_play_beep(aether_audio_t *a, f32 freq_hz,
                                       f32 duration_sec, f32 volume);

/* Stream a WAV (header+PCM16 body) through the buffer callback.
 * Parses header, extracts PCM, submits. Tone writer WAV works for host smoke. */
aether_result_t aether_audio_play_wav_data(aether_audio_t *a,
                                           const u8 *wav_data, u32 wav_size,
                                           f32 volume);

/* Internal: called by the engine to flush queued commands. */
void aether_audio_flush(aether_audio_t *a);

/* ---------- Spatial audio stub (distance + stereo pan) ---------- */
typedef struct aether_audio_spatial {
    f32 gain;      /* 0..1 distance attenuation */
    f32 pan;       /* -1..1 left/right */
    f32 dist;      /* world units from listener */
} aether_audio_spatial_t;

void aether_audio_set_listener(aether_audio_t *a,
                               f32 x, f32 y, f32 z,
                               f32 forward_x, f32 forward_y, f32 forward_z);
void aether_audio_get_listener(const aether_audio_t *a, f32 out_pos[3], f32 out_fwd[3]);

/* Compute gain/pan for a world source. ref_dist/max_dist control falloff. */
void aether_audio_spatial_atten(const aether_audio_t *a,
                                f32 src_x, f32 src_y, f32 src_z,
                                f32 ref_dist, f32 max_dist,
                                aether_audio_spatial_t *out);

/* Beep/WAV at world position (volume *= gain; stereo pan encoded if possible). */
aether_result_t aether_audio_play_beep_at(aether_audio_t *a, f32 freq_hz,
                                          f32 duration_sec, f32 volume,
                                          f32 src_x, f32 src_y, f32 src_z);
aether_result_t aether_audio_play_wav_at(aether_audio_t *a,
                                         const u8 *wav_data, u32 wav_size,
                                         f32 volume,
                                         f32 src_x, f32 src_y, f32 src_z);

#ifdef __cplusplus
}
#endif

/* Stereo pan → L/R gains (constant-power-ish). pan -1..1. */
void aether_audio_spatial_stereo_gains(f32 pan, f32 *out_left, f32 *out_right);

/* Submit stereo beep with explicit L/R gains (channels=2 interleaved). */
aether_result_t aether_audio_play_beep_stereo(aether_audio_t *a, f32 freq_hz,
                                              f32 duration_sec, f32 volume,
                                              f32 gain_l, f32 gain_r);

/* Spatial beep that mixes stereo L/R from pan (host + iOS PCM path). */
aether_result_t aether_audio_play_beep_stereo_at(aether_audio_t *a, f32 freq_hz,
                                                 f32 duration_sec, f32 volume,
                                                 f32 src_x, f32 src_y, f32 src_z);

/* Studio sound-cue stub: play a short beep tagged with cue name (host/smoke). */
aether_result_t aether_audio_play_studio_cue(aether_audio_t *a, const char *cue,
                                             f32 volume);

#endif /* AETHER_AUDIO_H */
