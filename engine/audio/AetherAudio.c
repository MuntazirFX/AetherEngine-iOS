#include "AetherAudio.h"
#include <stdlib.h>
#include <string.h>

struct aether_audio {
    aether_audio_state_t      state;
    f32                       volume[AETHER_AUDIO_CHANNEL_COUNT];
    bool                      muted;

    aether_audio_voice_t      voices[AETHER_MAX_AUDIO_VOICES];
    u32                       voice_count;

    aether_audio_platform_fn  platform_fn;
    void                     *platform_user;
};

/* ---------- Lifecycle ---------- */
aether_audio_t *aether_audio_create(void) {
    aether_audio_t *a = (aether_audio_t*)calloc(1, sizeof *a);
    if (!a) return NULL;

    a->state = AETHER_AUDIO_STATE_UNINIT;
    a->muted = false;
    for (int i = 0; i < AETHER_AUDIO_CHANNEL_COUNT; ++i) a->volume[i] = 1.0f;
    a->volume[AETHER_AUDIO_CHANNEL_MUSIC] = 0.7f;

    aether_log(AETHER_LOG_INFO, "audio", "audio system created");
    return a;
}

void aether_audio_destroy(aether_audio_t *a) {
    if (!a) return;
    free(a);
    aether_log(AETHER_LOG_INFO, "audio", "audio system destroyed");
}

aether_result_t aether_audio_init(aether_audio_t *a) {
    if (!a) return AETHER_ERR_INVALID_ARG;
    a->state = AETHER_AUDIO_STATE_READY;
    aether_log(AETHER_LOG_INFO, "audio", "audio initialized (ready)");
    return AETHER_OK;
}

aether_result_t aether_audio_shutdown(aether_audio_t *a) {
    if (!a) return AETHER_ERR_INVALID_ARG;
    aether_audio_stop_all(a);
    a->state = AETHER_AUDIO_STATE_UNINIT;
    aether_log(AETHER_LOG_INFO, "audio", "audio shutdown");
    return AETHER_OK;
}

/* ---------- Volume ---------- */
static f32 clamp01(f32 v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void aether_audio_set_master_volume(aether_audio_t *a, f32 vol) {
    if (!a) return;
    a->volume[AETHER_AUDIO_CHANNEL_MASTER] = clamp01(vol);
    aether_log(AETHER_LOG_DEBUG, "audio", "master volume = %.2f", a->volume[AETHER_AUDIO_CHANNEL_MASTER]);
}

void aether_audio_set_channel_volume(aether_audio_t *a, aether_audio_channel_t ch, f32 vol) {
    if (!a || ch < 0 || ch >= AETHER_AUDIO_CHANNEL_COUNT) return;
    a->volume[ch] = clamp01(vol);
}

f32 aether_audio_get_master_volume(const aether_audio_t *a) {
    return a ? a->volume[AETHER_AUDIO_CHANNEL_MASTER] : 0.0f;
}

f32 aether_audio_get_channel_volume(const aether_audio_t *a, aether_audio_channel_t ch) {
    if (!a || ch < 0 || ch >= AETHER_AUDIO_CHANNEL_COUNT) return 0.0f;
    return a->volume[ch];
}

void aether_audio_set_mute(aether_audio_t *a, bool muted) {
    if (a) a->muted = muted;
}

bool aether_audio_is_muted(const aether_audio_t *a) {
    return a ? a->muted : true;
}

/* ---------- Voice management ---------- */
static void dispatch_voice(aether_audio_t *a, const aether_audio_voice_t *v, const char *cmd) {
    if (a->platform_fn) {
        a->platform_fn(v, cmd, a->platform_user);
    }
}

i32 aether_audio_play_effect(aether_audio_t *a, const char *asset_path,
                             f32 volume, bool loop) {
    if (!a || !asset_path) return -1;
    if (a->voice_count >= AETHER_MAX_AUDIO_VOICES) {
        aether_log(AETHER_LOG_WARN, "audio", "voice limit reached");
        return -1;
    }

    aether_audio_voice_t *v = &a->voices[a->voice_count];
    v->id      = a->voice_count;
    aether_str_copy(v->asset, sizeof v->asset, asset_path);
    v->channel = AETHER_AUDIO_CHANNEL_EFFECTS;
    v->volume  = clamp01(volume);
    v->loop    = loop;
    v->playing = true;

    a->voice_count++;

    /* Notify platform to start playback. */
    dispatch_voice(a, v, "play");

    aether_log(AETHER_LOG_DEBUG, "audio", "play voice %d: %s (vol=%.2f, loop=%d)",
               (int)v->id, v->asset, v->volume, v->loop ? 1 : 0);
    return (i32)v->id;
}

aether_result_t aether_audio_stop_voice(aether_audio_t *a, i32 voice_id) {
    if (!a || voice_id < 0 || voice_id >= (i32)a->voice_count) return AETHER_ERR_INVALID_ARG;
    aether_audio_voice_t *v = &a->voices[voice_id];
    if (!v->playing) return AETHER_ERR_NOT_READY;

    v->playing = false;
    dispatch_voice(a, v, "stop");
    return AETHER_OK;
}

void aether_audio_stop_channel(aether_audio_t *a, aether_audio_channel_t ch) {
    if (!a || ch < 0 || ch >= AETHER_AUDIO_CHANNEL_COUNT) return;
    for (u32 i = 0; i < a->voice_count; ++i) {
        if (a->voices[i].playing && a->voices[i].channel == ch) {
            a->voices[i].playing = false;
            dispatch_voice(a, &a->voices[i], "stop");
        }
    }
}

void aether_audio_stop_all(aether_audio_t *a) {
    if (!a) return;
    for (u32 i = 0; i < a->voice_count; ++i) {
        if (a->voices[i].playing) {
            a->voices[i].playing = false;
            dispatch_voice(a, &a->voices[i], "stop");
        }
    }
    a->voice_count = 0;
}

u32 aether_audio_voice_count(const aether_audio_t *a) {
    return a ? a->voice_count : 0;
}

const aether_audio_voice_t *aether_audio_voice_at(const aether_audio_t *a, u32 idx) {
    if (!a || idx >= a->voice_count) return NULL;
    return &a->voices[idx];
}

void aether_audio_set_platform_callback(aether_audio_t *a,
                                        aether_audio_platform_fn fn,
                                        void *user) {
    if (!a) return;
    a->platform_fn   = fn;
    a->platform_user = user;
}

void aether_audio_flush(aether_audio_t *a) {
    if (!a) return;
    /* Compact dead voices from the front. */
    u32 write = 0;
    for (u32 i = 0; i < a->voice_count; ++i) {
        if (a->voices[i].playing) {
            if (write != i) a->voices[write] = a->voices[i];
            a->voices[write].id = write;
            write++;
        }
    }
    a->voice_count = write;
}
