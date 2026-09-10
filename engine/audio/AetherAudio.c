#include "AetherAudio.h"
#include <stdlib.h>
#include <string.h>

typedef struct aether_voice {
    bool   active;
    bool   is_music;
    u32    sample_rate;
    u32    channels;
    u32    byte_size;
    u8    *pcm;             /* owned copy (host stub); on iOS we hand to AVAudioEngine */
    char   vpath[256];      /* for music */
} aether_voice_t;

struct aether_audio {
    f32  volumes[AETHER_CHANNEL_COUNT];
    bool muted;
    u32  next_id;
    aether_voice_t voices[AETHER_AUDIO_MAX_VOICES];
};

aether_audio_t *aether_audio_create(void) {
    aether_audio_t *a = (aether_audio_t*)calloc(1, sizeof *a);
    if (!a) return NULL;
    a->volumes[AETHER_CHANNEL_MASTER]  = 1.0f;
    a->volumes[AETHER_CHANNEL_MUSIC]   = 0.7f;
    a->volumes[AETHER_CHANNEL_EFFECTS] = 1.0f;
    a->next_id = 1;
    aether_log(AETHER_LOG_INFO, "audio", "audio system created");
    return a;
}

void aether_audio_destroy(aether_audio_t *a) {
    if (!a) return;
    aether_audio_stop_all(a);
    free(a);
}

void aether_audio_set_volume(aether_audio_t *a, aether_audio_channel_t ch, f32 v) {
    if (!a || ch < 0 || ch >= AETHER_CHANNEL_COUNT) return;
    if (v < 0) v = 0; if (v > 1) v = 1;
    a->volumes[ch] = v;
    aether_log(AETHER_LOG_DEBUG, "audio", "volume[%d] = %.2f", (int)ch, v);
}

f32 aether_audio_get_volume(const aether_audio_t *a, aether_audio_channel_t ch) {
    if (!a || ch < 0 || ch >= AETHER_CHANNEL_COUNT) return 0.0f;
    return a->volumes[ch];
}

void aether_audio_set_master_mute(aether_audio_t *a, bool mute) {
    if (!a) return;
    a->muted = mute;
    aether_log(AETHER_LOG_INFO, "audio", "master mute = %s", mute ? "on" : "off");
}

static aether_voice_t *find_free_voice(aether_audio_t *a) {
    for (u32 i = 0; i < AETHER_AUDIO_MAX_VOICES; ++i) {
        if (!a->voices[i].active) return &a->voices[i];
    }
    return NULL;
}

aether_audio_handle_t aether_audio_play_sfx(aether_audio_t *a,
                                            const u8 *pcm16, u32 byte_size,
                                            u32 sample_rate, u32 channels) {
    aether_audio_handle_t h = {0};
    if (!a || !pcm16 || byte_size == 0) return h;

    aether_voice_t *v = find_free_voice(a);
    if (!v) {
        aether_log(AETHER_LOG_WARN, "audio", "no free voice for SFX");
        return h;
    }
    v->active      = true;
    v->is_music    = false;
    v->sample_rate = sample_rate;
    v->channels    = channels;
    v->byte_size   = byte_size;
    v->pcm         = (u8*)malloc(byte_size);
    if (!v->pcm) { v->active = false; return h; }
    memcpy(v->pcm, pcm16, byte_size);
    h.id = a->next_id++;
    return h;
}

aether_audio_handle_t aether_audio_play_music(aether_audio_t *a, const char *vpath) {
    aether_audio_handle_t h = {0};
    if (!a || !vpath) return h;

    /* Stop existing music. */
    for (u32 i = 0; i < AETHER_AUDIO_MAX_VOICES; ++i) {
        if (a->voices[i].active && a->voices[i].is_music) {
            free(a->voices[i].pcm); a->voices[i].pcm = NULL;
            a->voices[i].active = false;
        }
    }

    aether_voice_t *v = find_free_voice(a);
    if (!v) return h;
    v->active   = true;
    v->is_music = true;
    aether_str_copy(v->vpath, sizeof v->vpath, vpath);
    h.id = a->next_id++;
    aether_log(AETHER_LOG_INFO, "audio", "music queued: %s", vpath);
    return h;
}

void aether_audio_stop_music(aether_audio_t *a) {
    if (!a) return;
    for (u32 i = 0; i < AETHER_AUDIO_MAX_VOICES; ++i) {
        if (a->voices[i].active && a->voices[i].is_music) {
            free(a->voices[i].pcm); a->voices[i].pcm = NULL;
            a->voices[i].active = false;
        }
    }
}

void aether_audio_stop(aether_audio_t *a, aether_audio_handle_t h) {
    if (!a || h.id == 0) return;
    /* Handle id is opaque; we clear first voice with matching tag.
       For simplicity, use id modulo 32 as slot. */
    u32 slot = (h.id - 1) % AETHER_AUDIO_MAX_VOICES;
    aether_voice_t *v = &a->voices[slot];
    if (v->active) {
        free(v->pcm); v->pcm = NULL;
        v->active = false;
    }
}

void aether_audio_stop_all(aether_audio_t *a) {
    if (!a) return;
    for (u32 i = 0; i < AETHER_AUDIO_MAX_VOICES; ++i) {
        if (a->voices[i].active) {
            free(a->voices[i].pcm);
            a->voices[i].pcm = NULL;
            a->voices[i].active = false;
        }
    }
}

void aether_audio_tick(aether_audio_t *a, f32 dt) {
    if (!a) return;
    (void)dt;
    /* Future: fade-outs, music transitions, DSP. */
}
