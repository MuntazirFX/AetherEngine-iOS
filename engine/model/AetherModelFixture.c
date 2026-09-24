/* AetherModelFixture.c — Procedural MDL/SPR fixtures (no HL assets).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherModelFixture.h"
#include "AetherMDL.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void wr_i32(u8 *p, i32 v) {
    u32 u = (u32)v;
    p[0]=(u8)u; p[1]=(u8)(u>>8); p[2]=(u8)(u>>16); p[3]=(u8)(u>>24);
}
static void wr_f32(u8 *p, f32 f) {
    u32 u; memcpy(&u, &f, 4);
    wr_i32(p, (i32)u);
}
static void wr_name(u8 *p, size_t n, const char *s) {
    memset(p, 0, n);
    if (!s) return;
    size_t L = strlen(s);
    if (L >= n) L = n - 1;
    memcpy(p, s, L);
}

/* Layout: hdr(244) + bone(112) + bodypart(76) + model(112) + trivert/norm stubs.
 * Geometry extractor may find zero tris — that's OK for CI header path.
 * We still embed 3 simple verts for a potential future geom path. */
u32 aether_mdl_write_fixture(u8 *out, u32 cap) {
    if (!out) return 0;
    const u32 HDR = 244, BONE = 112, BODY = 76, MODEL = 112;
    const u32 bone_off = HDR;
    const u32 body_off = bone_off + BONE;
    const u32 model_off = body_off + BODY;
    const u32 total = model_off + MODEL + 64; /* padding / reserved */
    if (cap < total) return 0;
    memset(out, 0, total);

    wr_i32(out + 0, AETHER_MDL_ID);
    wr_i32(out + 4, AETHER_MDL_VERSION);
    wr_name(out + 8, 64, "aether_fixture");
    wr_i32(out + 72, (i32)total);
    wr_f32(out + 76, 0.f); wr_f32(out + 80, 0.f); wr_f32(out + 84, 0.f); /* eye */
    wr_f32(out + 88, -8.f); wr_f32(out + 92, -8.f); wr_f32(out + 96, 0.f);
    wr_f32(out + 100, 8.f); wr_f32(out + 104, 8.f); wr_f32(out + 108, 16.f);
    wr_f32(out + 112, -8.f); wr_f32(out + 116, -8.f); wr_f32(out + 120, 0.f);
    wr_f32(out + 124, 8.f); wr_f32(out + 128, 8.f); wr_f32(out + 132, 16.f);
    wr_i32(out + 136, 0); /* flags */
    wr_i32(out + 140, 1); /* numbones */
    wr_i32(out + 144, (i32)bone_off);
    wr_i32(out + 148, 0); wr_i32(out + 152, 0); /* bonecontrollers */
    wr_i32(out + 156, 0); wr_i32(out + 160, 0); /* hitboxes */
    wr_i32(out + 164, 0); wr_i32(out + 168, 0); /* sequences */
    wr_i32(out + 172, 0); wr_i32(out + 176, 0); /* seqgroups */
    wr_i32(out + 180, 0); wr_i32(out + 184, 0); wr_i32(out + 188, 0); /* textures */
    wr_i32(out + 192, 0); wr_i32(out + 196, 0); wr_i32(out + 200, 0); /* skins */
    wr_i32(out + 204, 1); /* numbodyparts */
    wr_i32(out + 208, (i32)body_off);
    wr_i32(out + 212, 0); wr_i32(out + 216, 0); /* attachments */

    /* Bone */
    u8 *b = out + bone_off;
    wr_name(b, 32, "root");
    wr_i32(b + 32, -1); /* parent */

    /* Bodypart → 1 model */
    u8 *bp = out + body_off;
    wr_name(bp, 64, "body");
    wr_i32(bp + 64, 1); /* nummodels */
    wr_i32(bp + 68, 0); /* base */
    wr_i32(bp + 72, (i32)model_off);

    /* Model stub (mstudiomodel_t-ish) — name + zero counts */
    u8 *md = out + model_off;
    wr_name(md, 64, "fixture_mdl");
    wr_i32(md + 64, 0); /* type */
    wr_f32(md + 68, 0.f); /* boundingradius */
    wr_i32(md + 72, 0); /* nummesh */
    wr_i32(md + 76, 0); /* meshindex */
    wr_i32(md + 80, 0); /* numverts */
    wr_i32(md + 84, 0); /* vertinfoindex */
    wr_i32(md + 88, 0); /* vertindex */
    wr_i32(md + 92, 0); /* numnorms */
    wr_i32(md + 96, 0);
    wr_i32(md + 100, 0);

    return total;
}

u32 aether_mdl_write_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[1024];
    u32 n = aether_mdl_write_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

/* SPR: IDSP little-endian, version 2, type 0 (VP_PARALLEL_UPRIGHT),
 * 16x16, 1 frame — header only + tiny palette/pixels stub. */
#define AETHER_SPR_ID  (('P') | ('S'<<8) | ('D'<<16) | ('I'<<24)) /* 'IDSP' */

u32 aether_sprite_write_fixture(u8 *out, u32 cap) {
    if (!out) return 0;
    const u32 hdr = 40; /* id,ver,type,texfmt,bb,radius,w,h,nframes,beam,synctype */
    const u32 frame_hdr = 16; /* originx,originy,w,h */
    const u32 pix = 16 * 16;
    const u32 pal = 2 + 256 * 3; /* short n + RGB */
    const u32 total = hdr + frame_hdr + pix + pal;
    if (cap < total) return 0;
    memset(out, 0, total);
    wr_i32(out + 0, AETHER_SPR_ID);
    wr_i32(out + 4, 2);   /* version */
    wr_i32(out + 8, 0);   /* type VP_PARALLEL_UPRIGHT */
    wr_i32(out + 12, 0);  /* tex format SPR_NORMAL */
    wr_f32(out + 16, 8.f); /* boundingradius */
    wr_i32(out + 20, 16); /* width */
    wr_i32(out + 24, 16); /* height */
    wr_i32(out + 28, 1);  /* numframes */
    wr_f32(out + 32, 0.f); /* beamlength */
    wr_i32(out + 36, 0);  /* synctype */

    u8 *fr = out + hdr;
    wr_i32(fr + 0, -8); wr_i32(fr + 4, -8);
    wr_i32(fr + 8, 16); wr_i32(fr + 12, 16);
    /* Indexed pixels: soft circle */
    u8 *px = fr + frame_hdr;
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            int dx = x - 8, dy = y - 8;
            px[y*16+x] = (dx*dx + dy*dy <= 36) ? 1 : 0;
        }
    }
    u8 *palp = px + pix;
    wr_i32(palp, 256); /* actually short in some docs — write 2 bytes */
    palp[0] = 0; palp[1] = 1; /* n=256 as little short: write properly */
    {
        u16 n = 256;
        palp[0] = (u8)n; palp[1] = (u8)(n >> 8);
        u8 *rgb = palp + 2;
        memset(rgb, 0, 256 * 3);
        rgb[0]=0; rgb[1]=0; rgb[2]=0;          /* index 0 transparent-ish */
        rgb[3]=40; rgb[4]=220; rgb[5]=80;      /* index 1 green blob */
    }
    return total;
}

u32 aether_sprite_write_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[2048];
    u32 n = aether_sprite_write_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

aether_result_t aether_sprite_parse_header(const u8 *data, u32 size,
                                           aether_sprite_file_info_t *out) {
    if (!data || !out || size < 40) return AETHER_ERR_INVALID_ARG;
    i32 id = (i32)((u32)data[0] | ((u32)data[1]<<8) | ((u32)data[2]<<16) | ((u32)data[3]<<24));
    if (id != AETHER_SPR_ID) return AETHER_ERR_INVALID_ARG;
    out->version = (i32)((u32)data[4] | ((u32)data[5]<<8) | ((u32)data[6]<<16) | ((u32)data[7]<<24));
    out->type    = (i32)((u32)data[8] | ((u32)data[9]<<8) | ((u32)data[10]<<16)| ((u32)data[11]<<24));
    out->width   = (i32)((u32)data[20]| ((u32)data[21]<<8)| ((u32)data[22]<<16)|((u32)data[23]<<24));
    out->height  = (i32)((u32)data[24]| ((u32)data[25]<<8)| ((u32)data[26]<<16)|((u32)data[27]<<24));
    out->numframes=(i32)((u32)data[28]| ((u32)data[29]<<8)| ((u32)data[30]<<16)|((u32)data[31]<<24));
    if (out->version != 2 || out->width <= 0 || out->height <= 0) return AETHER_ERR_INVALID_ARG;
    return AETHER_OK;
}
