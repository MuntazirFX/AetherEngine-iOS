/* AetherWav.c — WAV header parse + procedural tone writer.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWav.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static u16 rd_u16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }
static u32 rd_u32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
static void wr_u16(u8 *p, u16 v) { p[0]=(u8)v; p[1]=(u8)(v>>8); }
static void wr_u32(u8 *p, u32 v) {
    p[0]=(u8)v; p[1]=(u8)(v>>8); p[2]=(u8)(v>>16); p[3]=(u8)(v>>24);
}

aether_result_t aether_wav_parse_header(const u8 *data, u32 size, aether_wav_info_t *out) {
    if (!data || !out || size < 44) return AETHER_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0)
        return AETHER_ERR_UNSUPPORTED;

    u32 pos = 12;
    u32 data_off = 0, data_sz = 0;
    u16 fmt = 0, ch = 0, bps = 0;
    u32 rate = 0;
    bool got_fmt = false, got_data = false;

    while (pos + 8 <= size) {
        const u8 *chk = data + pos;
        u32 csz = rd_u32(chk + 4);
        if (memcmp(chk, "fmt ", 4) == 0) {
            if (pos + 8 + csz > size || csz < 16) return AETHER_ERR_UNSUPPORTED;
            const u8 *f = data + pos + 8;
            fmt = rd_u16(f + 0);
            ch  = rd_u16(f + 2);
            rate = rd_u32(f + 4);
            bps = rd_u16(f + 14);
            got_fmt = true;
        } else if (memcmp(chk, "data", 4) == 0) {
            data_off = pos + 8;
            data_sz = csz;
            got_data = true;
            break;
        }
        pos += 8 + csz;
        if (csz & 1u) pos++; /* word align */
    }
    if (!got_fmt || !got_data) return AETHER_ERR_UNSUPPORTED;
    out->sample_rate = rate;
    out->channels = ch;
    out->bits_per_sample = bps;
    out->audio_format = fmt;
    out->data_offset = data_off;
    out->data_size = data_sz;
    out->valid = true;
    return AETHER_OK;
}

aether_result_t aether_wav_parse_file(const char *filepath, aether_wav_info_t *out) {
    if (!filepath || !out) return AETHER_ERR_INVALID_ARG;
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return AETHER_ERR_NOT_FOUND;
    u8 hdr[256];
    size_t n = fread(hdr, 1, sizeof hdr, fp);
    fclose(fp);
    return aether_wav_parse_header(hdr, (u32)n, out);
}

u32 aether_wav_write_tone_pcm(u8 *out, u32 cap, u32 sample_rate, u16 channels,
                              f32 freq_hz, f32 duration_sec, f32 amplitude) {
    if (!out || channels == 0 || sample_rate == 0) return 0;
    if (amplitude < 0.f) amplitude = 0.f;
    if (amplitude > 1.f) amplitude = 1.f;
    u32 frames = (u32)(sample_rate * duration_sec + 0.5f);
    if (frames < 1) frames = 1;
    u32 data_bytes = frames * channels * 2u; /* 16-bit */
    u32 total = 44u + data_bytes;
    if (cap < total) return 0;

    memset(out, 0, total);
    memcpy(out + 0, "RIFF", 4);
    wr_u32(out + 4, 36u + data_bytes);
    memcpy(out + 8, "WAVE", 4);
    memcpy(out + 12, "fmt ", 4);
    wr_u32(out + 16, 16);
    wr_u16(out + 20, 1); /* PCM */
    wr_u16(out + 22, channels);
    wr_u32(out + 24, sample_rate);
    wr_u32(out + 28, sample_rate * channels * 2u);
    wr_u16(out + 32, (u16)(channels * 2));
    wr_u16(out + 34, 16);
    memcpy(out + 36, "data", 4);
    wr_u32(out + 40, data_bytes);

    i16 *pcm = (i16 *)(out + 44);
    for (u32 i = 0; i < frames; ++i) {
        f32 t = (f32)i / (f32)sample_rate;
        f32 s = sinf(6.2831853f * freq_hz * t) * amplitude;
        i16 sample = (i16)(s * 32767.f);
        for (u16 c = 0; c < channels; ++c) pcm[i * channels + c] = sample;
    }
    return total;
}
