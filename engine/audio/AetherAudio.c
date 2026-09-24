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

    aether_audio_buffer_fn    buffer_fn;
    void                     *buffer_user;

    f32                       listener_pos[3];
    f32                       listener_fwd[3];
};

/* ---------- Lifecycle ---------- */
aether_audio_t *aether_audio_create(void) {
    aether_audio_t *a = (aether_audio_t*)calloc(1, sizeof *a);
    if (!a) return NULL;

    a->state = AETHER_AUDIO_STATE_UNINIT;
    a->muted = false;
    for (int i = 0; i < AETHER_AUDIO_CHANNEL_COUNT; ++i) a->volume[i] = 1.0f;
    a->volume[AETHER_AUDIO_CHANNEL_MUSIC] = 0.7f;
    a->listener_fwd[0] = 1.f; /* +X forward */

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

bool aether_audio_is_ready(const aether_audio_t *a) {
    return a && a->state == AETHER_AUDIO_STATE_READY;
}

aether_audio_state_t aether_audio_get_state(const aether_audio_t *a) {
    return a ? a->state : AETHER_AUDIO_STATE_UNINIT;
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


#include <math.h>
#include "AetherWav.h"

void aether_audio_set_buffer_callback(aether_audio_t *a,
                                      aether_audio_buffer_fn fn,
                                      void *user) {
    if (!a) return;
    a->buffer_fn = fn;
    a->buffer_user = user;
}

aether_result_t aether_audio_submit_buffer(aether_audio_t *a,
                                           const aether_audio_buffer_t *buf) {
    if (!a || !buf || !buf->samples || buf->frame_count == 0)
        return AETHER_ERR_INVALID_ARG;
    if (a->state != AETHER_AUDIO_STATE_READY) return AETHER_ERR_NOT_READY;
    if (a->muted) return AETHER_OK;
    if (a->buffer_fn) {
        aether_audio_buffer_t local = *buf;
        f32 master = a->volume[AETHER_AUDIO_CHANNEL_MASTER];
        local.volume = buf->volume * master;
        a->buffer_fn(&local, a->buffer_user);
    }
    aether_log(AETHER_LOG_DEBUG, "audio",
               "submit_buffer frames=%u rate=%u ch=%u vol=%.2f",
               buf->frame_count, buf->sample_rate, buf->channels, buf->volume);
    return AETHER_OK;
}

aether_result_t aether_audio_play_beep(aether_audio_t *a, f32 freq_hz,
                                       f32 duration_sec, f32 volume) {
    if (!a) return AETHER_ERR_INVALID_ARG;
    if (a->state != AETHER_AUDIO_STATE_READY) return AETHER_ERR_NOT_READY;
    if (freq_hz < 20.f) freq_hz = 20.f;
    if (freq_hz > 8000.f) freq_hz = 8000.f;
    if (duration_sec <= 0.f) duration_sec = 0.05f;
    if (duration_sec > 2.f) duration_sec = 2.f;
    if (volume < 0.f) volume = 0.f;
    if (volume > 1.f) volume = 1.f;

    const u32 rate = 22050;
    const u16 ch = 1;
    u32 frames = (u32)(rate * duration_sec + 0.5f);
    if (frames < 1) frames = 1;
    i16 *pcm = (i16 *)malloc(sizeof(i16) * frames);
    if (!pcm) return AETHER_ERR_OUT_OF_MEM;
    for (u32 i = 0; i < frames; ++i) {
        f32 t = (f32)i / (f32)rate;
        f32 env = 1.f;
        if (i < 64) env = (f32)i / 64.f;
        if (i + 64 > frames) env = (f32)(frames - i) / 64.f;
        f32 s = sinf(6.2831853f * freq_hz * t) * volume * env;
        pcm[i] = (i16)(s * 32767.f);
    }
    aether_audio_buffer_t buf = {
        .samples = pcm,
        .frame_count = frames,
        .sample_rate = rate,
        .channels = ch,
        .volume = 1.f,
    };
    aether_result_t r = aether_audio_submit_buffer(a, &buf);
    /* Also enqueue a logical voice so platform voice callback sees a "beep". */
    (void)aether_audio_play_effect(a, "procedural:beep", volume, false);
    free(pcm);
    return r;
}

aether_result_t aether_audio_play_wav_data(aether_audio_t *a,
                                           const u8 *wav_data, u32 wav_size,
                                           f32 volume) {
    if (!a || !wav_data || wav_size < 44) return AETHER_ERR_INVALID_ARG;
    if (a->state != AETHER_AUDIO_STATE_READY) return AETHER_ERR_NOT_READY;
    if (volume < 0.f) volume = 0.f;
    if (volume > 1.f) volume = 1.f;

    aether_wav_info_t info;
    aether_result_t pr = aether_wav_parse_header(wav_data, wav_size, &info);
    if (pr != AETHER_OK || !info.valid) return pr != AETHER_OK ? pr : AETHER_ERR_UNSUPPORTED;
    if (info.audio_format != 1 || info.bits_per_sample != 16 || info.channels == 0)
        return AETHER_ERR_UNSUPPORTED;

    u32 max_samp = info.data_size / 2u;
    if (max_samp == 0) return AETHER_ERR_UNSUPPORTED;
    /* Cap stream body for host safety (~2s @ 44.1k stereo). */
    if (max_samp > 176400u) max_samp = 176400u;
    i16 *pcm = (i16 *)malloc(sizeof(i16) * max_samp);
    if (!pcm) return AETHER_ERR_OUT_OF_MEM;
    u32 got = aether_wav_extract_pcm16(wav_data, wav_size, &info, pcm, max_samp);
    if (got == 0) { free(pcm); return AETHER_ERR_UNSUPPORTED; }

    u32 frames = got / info.channels;
    if (frames == 0) { free(pcm); return AETHER_ERR_UNSUPPORTED; }

    aether_audio_buffer_t buf = {
        .samples = pcm,
        .frame_count = frames,
        .sample_rate = info.sample_rate,
        .channels = info.channels,
        .volume = volume,
    };
    aether_result_t r = aether_audio_submit_buffer(a, &buf);
    (void)aether_audio_play_effect(a, "stream:wav", volume, false);
    free(pcm);
    aether_log(AETHER_LOG_INFO, "audio",
               "play_wav_data frames=%u rate=%u ch=%u", frames, info.sample_rate, info.channels);
    return r;
}


void aether_audio_set_listener(aether_audio_t *a,
                               f32 x, f32 y, f32 z,
                               f32 forward_x, f32 forward_y, f32 forward_z) {
    if (!a) return;
    a->listener_pos[0] = x; a->listener_pos[1] = y; a->listener_pos[2] = z;
    f32 len = sqrtf(forward_x*forward_x + forward_y*forward_y + forward_z*forward_z);
    if (len < 1e-5f) { a->listener_fwd[0]=1.f; a->listener_fwd[1]=0.f; a->listener_fwd[2]=0.f; return; }
    a->listener_fwd[0] = forward_x / len;
    a->listener_fwd[1] = forward_y / len;
    a->listener_fwd[2] = forward_z / len;
}

void aether_audio_get_listener(const aether_audio_t *a, f32 out_pos[3], f32 out_fwd[3]) {
    if (out_pos) {
        if (!a) { out_pos[0]=out_pos[1]=out_pos[2]=0; }
        else { out_pos[0]=a->listener_pos[0]; out_pos[1]=a->listener_pos[1]; out_pos[2]=a->listener_pos[2]; }
    }
    if (out_fwd) {
        if (!a) { out_fwd[0]=1; out_fwd[1]=0; out_fwd[2]=0; }
        else { out_fwd[0]=a->listener_fwd[0]; out_fwd[1]=a->listener_fwd[1]; out_fwd[2]=a->listener_fwd[2]; }
    }
}

void aether_audio_spatial_atten(const aether_audio_t *a,
                                f32 src_x, f32 src_y, f32 src_z,
                                f32 ref_dist, f32 max_dist,
                                aether_audio_spatial_t *out) {
    if (!out) return;
    out->gain = 0.f; out->pan = 0.f; out->dist = 0.f;
    if (!a) return;
    if (ref_dist < 1.f) ref_dist = 1.f;
    if (max_dist < ref_dist) max_dist = ref_dist * 16.f;
    f32 dx = src_x - a->listener_pos[0];
    f32 dy = src_y - a->listener_pos[1];
    f32 dz = src_z - a->listener_pos[2];
    f32 dist = sqrtf(dx*dx + dy*dy + dz*dz);
    out->dist = dist;
    if (dist >= max_dist) { out->gain = 0.f; return; }
    if (dist <= ref_dist) out->gain = 1.f;
    else {
        f32 t = (dist - ref_dist) / (max_dist - ref_dist);
        out->gain = 1.f - t;
        if (out->gain < 0.f) out->gain = 0.f;
    }
    /* Pan: project onto right = cross(fwd, up_z). */
    f32 fx = a->listener_fwd[0], fy = a->listener_fwd[1];
    f32 rx = -fy, ry = fx;
    f32 rlen = sqrtf(rx*rx + ry*ry);
    if (rlen > 1e-5f) { rx /= rlen; ry /= rlen; }
    f32 side = 0.f;
    if (dist > 1e-4f) side = (dx * rx + dy * ry) / dist;
    if (side < -1.f) side = -1.f;
    if (side > 1.f) side = 1.f;
    out->pan = side;
}

aether_result_t aether_audio_play_beep_at(aether_audio_t *a, f32 freq_hz,
                                          f32 duration_sec, f32 volume,
                                          f32 src_x, f32 src_y, f32 src_z) {
    if (!a) return AETHER_ERR_INVALID_ARG;
    aether_audio_spatial_t sp;
    aether_audio_spatial_atten(a, src_x, src_y, src_z, 64.f, 1024.f, &sp);
    f32 v = volume * sp.gain;
    if (v < 0.001f) return AETHER_OK; /* culled */
    /* Encode pan into volume slightly for mono beep (host hears gain only). */
    (void)sp.pan;
    return aether_audio_play_beep(a, freq_hz, duration_sec, v);
}

aether_result_t aether_audio_play_wav_at(aether_audio_t *a,
                                         const u8 *wav_data, u32 wav_size,
                                         f32 volume,
                                         f32 src_x, f32 src_y, f32 src_z) {
    if (!a) return AETHER_ERR_INVALID_ARG;
    aether_audio_spatial_t sp;
    aether_audio_spatial_atten(a, src_x, src_y, src_z, 64.f, 1024.f, &sp);
    f32 v = volume * sp.gain;
    if (v < 0.001f) return AETHER_OK;
    (void)sp.pan;
    return aether_audio_play_wav_data(a, wav_data, wav_size, v);
}
