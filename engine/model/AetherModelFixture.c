/* AetherModelFixture.c — Procedural MDL/SPR fixtures (no HL assets).
 * AetherEngine-iOS · Clean-room.
 * MDL fixture embeds 1 studio mesh = 1 triangle (3 verts) for Metal draw.
 */
#include "AetherModelFixture.h"
#include "AetherMDL.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static void wr_i32(u8 *p, i32 v) {
    u32 u = (u32)v;
    p[0]=(u8)u; p[1]=(u8)(u>>8); p[2]=(u8)(u>>16); p[3]=(u8)(u>>24);
}
static void wr_u16(u8 *p, u16 v) {
    p[0]=(u8)v; p[1]=(u8)(v>>8);
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

/* Layout matching AetherMDLGeometry's on-disk structs:
 *   hdr(244) + bone(112) + bodypart(76) + model(104) + mesh(20)
 *   + verts(3*12) + norms(3*12) + tri_idx(3*u16)
 * Geometry extractor reads model.vert_info_index as float3 positions.
 */
u32 aether_mdl_write_fixture(u8 *out, u32 cap) {
    if (!out) return 0;
    const u32 HDR = 244, BONE = 112, BODY = 76, MODEL = 104, MESH = 20;
    const u32 NVERTS = 3;
    const u32 VERT_BYTES = NVERTS * 12;
    const u32 NORM_BYTES = NVERTS * 12;
    const u32 TRI_BYTES = 3 * 2; /* 1 tri × 3 × u16 */

    const u32 bone_off  = HDR;
    const u32 body_off  = bone_off + BONE;
    const u32 model_off = body_off + BODY;
    const u32 mesh_off  = model_off + MODEL;
    const u32 vert_off  = mesh_off + MESH;
    const u32 norm_off  = vert_off + VERT_BYTES;
    const u32 tri_off   = norm_off + NORM_BYTES;
    const u32 total     = tri_off + TRI_BYTES + 16; /* pad */
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
    wr_i32(b + 32, -1);

    /* Bodypart → 1 model */
    u8 *bp = out + body_off;
    wr_name(bp, 64, "body");
    wr_i32(bp + 64, 1); /* nummodels */
    wr_i32(bp + 68, 0); /* base */
    wr_i32(bp + 72, (i32)model_off);

    /* Model — matches mdl_model_disk_t (104 bytes) used by geometry extract */
    u8 *md = out + model_off;
    wr_name(md, 64, "fixture_tri");
    wr_i32(md + 64, 0);                 /* type */
    wr_f32(md + 68, 16.f);              /* boundingradius */
    wr_i32(md + 72, 1);                 /* num_meshes */
    wr_i32(md + 76, (i32)mesh_off);     /* mesh_index */
    wr_i32(md + 80, (i32)NVERTS);       /* num_verts */
    wr_i32(md + 84, (i32)vert_off);     /* vert_info_index → float3 positions */
    wr_i32(md + 88, (i32)NVERTS);       /* num_norms */
    wr_i32(md + 92, (i32)norm_off);     /* norm_info_index → float3 normals */
    wr_i32(md + 96, 0);                 /* num_groups */
    wr_i32(md + 100, 0);                /* group_index */

    /* Mesh (20-byte stride used by extractor) */
    u8 *ms = out + mesh_off;
    wr_i32(ms + 0, 1);                  /* num_tris */
    wr_i32(ms + 4, (i32)tri_off);       /* tri_indexes */
    wr_i32(ms + 8, 0);                  /* skin_ref */
    wr_i32(ms + 12, (i32)NVERTS);       /* num_verts */
    wr_i32(ms + 16, 0);                 /* vert_info_index (unused) */

    /* Clean-room triangle in XY plane, Z-up normal */
    f32 verts[9] = {
        -16.f, -16.f, 0.f,
         16.f, -16.f, 0.f,
          0.f,  16.f, 0.f
    };
    f32 norms[9] = {
        0.f, 0.f, 1.f,
        0.f, 0.f, 1.f,
        0.f, 0.f, 1.f
    };
    for (u32 i = 0; i < 9; ++i) {
        wr_f32(out + vert_off + i * 4, verts[i]);
        wr_f32(out + norm_off + i * 4, norms[i]);
    }
    wr_u16(out + tri_off + 0, 0);
    wr_u16(out + tri_off + 2, 1);
    wr_u16(out + tri_off + 4, 2);

    return total;
}

u32 aether_mdl_write_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[2048];
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
    const u32 hdr = 40;
    const u32 frame_hdr = 16;
    const u32 pix = 16 * 16;
    const u32 pal = 2 + 256 * 3;
    const u32 total = hdr + frame_hdr + pix + pal;
    if (cap < total) return 0;
    memset(out, 0, total);
    wr_i32(out + 0, AETHER_SPR_ID);
    wr_i32(out + 4, 2);
    wr_i32(out + 8, 0);
    wr_i32(out + 12, 0);
    wr_f32(out + 16, 8.f);
    wr_i32(out + 20, 16);
    wr_i32(out + 24, 16);
    wr_i32(out + 28, 1);
    wr_f32(out + 32, 0.f);
    wr_i32(out + 36, 0);

    u8 *fr = out + hdr;
    wr_i32(fr + 0, -8); wr_i32(fr + 4, -8);
    wr_i32(fr + 8, 16); wr_i32(fr + 12, 16);
    u8 *px = fr + frame_hdr;
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            int dx = x - 8, dy = y - 8;
            px[y*16+x] = (dx*dx + dy*dy <= 36) ? 1 : 0;
        }
    }
    u8 *palp = px + pix;
    {
        u16 n = 256;
        palp[0] = (u8)n; palp[1] = (u8)(n >> 8);
        u8 *rgb = palp + 2;
        memset(rgb, 0, 256 * 3);
        rgb[0]=0; rgb[1]=0; rgb[2]=0;
        rgb[3]=40; rgb[4]=220; rgb[5]=80;
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


/* Textured + 2-bone fixture with proper layout (bones before bodypart). */
u32 aether_mdl_write_textured_fixture(u8 *out, u32 cap) {
    if (!out) return 0;
    const u32 HDR = 244, BONE = 112, BODY = 76, MODEL = 104, MESH = 20;
    const u32 NVERTS = 3;
    const u32 VERT_BYTES = NVERTS * 12;
    const u32 NORM_BYTES = NVERTS * 12;
    const u32 TRI_BYTES = 3 * 2;
    const u32 TEX_W = 8, TEX_H = 8;
    const u32 TEX_BYTES = TEX_W * TEX_H * 4u;
    const u32 NBONES = 2;

    const u32 bone_off  = HDR;
    const u32 body_off  = bone_off + BONE * NBONES;
    const u32 model_off = body_off + BODY;
    const u32 mesh_off  = model_off + MODEL;
    const u32 vert_off  = mesh_off + MESH;
    const u32 norm_off  = vert_off + VERT_BYTES;
    const u32 tri_off   = norm_off + NORM_BYTES;
    const u32 tex_off   = tri_off + TRI_BYTES + 16;
    const u32 total     = tex_off + TEX_BYTES + 16;
    if (cap < total) return 0;
    memset(out, 0, total);

    wr_i32(out + 0, AETHER_MDL_ID);
    wr_i32(out + 4, AETHER_MDL_VERSION);
    wr_name(out + 8, 64, "aether_tex_fixture");
    wr_i32(out + 72, (i32)total);
    wr_f32(out + 76, 0.f); wr_f32(out + 80, 0.f); wr_f32(out + 84, 0.f);
    wr_f32(out + 88, -8.f); wr_f32(out + 92, -8.f); wr_f32(out + 96, 0.f);
    wr_f32(out + 100, 8.f); wr_f32(out + 104, 8.f); wr_f32(out + 108, 16.f);
    wr_f32(out + 112, -8.f); wr_f32(out + 116, -8.f); wr_f32(out + 120, 0.f);
    wr_f32(out + 124, 8.f); wr_f32(out + 128, 8.f); wr_f32(out + 132, 16.f);
    wr_i32(out + 136, 0);
    wr_i32(out + 140, (i32)NBONES);
    wr_i32(out + 144, (i32)bone_off);
    wr_i32(out + 148, 0); wr_i32(out + 152, 0);
    wr_i32(out + 156, 0); wr_i32(out + 160, 0);
    wr_i32(out + 164, 0); wr_i32(out + 168, 0);
    wr_i32(out + 172, 0); wr_i32(out + 176, 0);
    wr_i32(out + 180, 1); wr_i32(out + 184, 0); wr_i32(out + 188, 0); /* textures=1 */
    wr_i32(out + 192, 0); wr_i32(out + 196, 0); wr_i32(out + 200, 0);
    wr_i32(out + 204, 1);
    wr_i32(out + 208, (i32)body_off);
    wr_i32(out + 212, 0); wr_i32(out + 216, 0);

    u8 *b0 = out + bone_off;
    wr_name(b0, 32, "root");
    wr_i32(b0 + 32, -1);
    u8 *b1 = out + bone_off + BONE;
    wr_name(b1, 32, "child");
    wr_i32(b1 + 32, 0);

    u8 *bp = out + body_off;
    wr_name(bp, 64, "body");
    wr_i32(bp + 64, 1);
    wr_i32(bp + 68, 0);
    wr_i32(bp + 72, (i32)model_off);

    u8 *md = out + model_off;
    wr_name(md, 64, "fixture_tex_tri");
    wr_i32(md + 64, 0);
    wr_f32(md + 68, 16.f);
    wr_i32(md + 72, 1);
    wr_i32(md + 76, (i32)mesh_off);
    wr_i32(md + 80, (i32)NVERTS);
    wr_i32(md + 84, (i32)vert_off);
    wr_i32(md + 88, (i32)NVERTS);
    wr_i32(md + 92, (i32)norm_off);
    wr_i32(md + 96, 0);
    wr_i32(md + 100, 0);

    u8 *ms = out + mesh_off;
    wr_i32(ms + 0, 1);
    wr_i32(ms + 4, (i32)tri_off);
    wr_i32(ms + 8, 0);
    wr_i32(ms + 12, (i32)NVERTS);
    wr_i32(ms + 16, 0);

    f32 verts[9] = { -16.f,-16.f,0.f, 16.f,-16.f,0.f, 0.f,16.f,0.f };
    f32 norms[9] = { 0,0,1, 0,0,1, 0,0,1 };
    for (u32 i = 0; i < 9; ++i) {
        wr_f32(out + vert_off + i * 4, verts[i]);
        wr_f32(out + norm_off + i * 4, norms[i]);
    }
    wr_u16(out + tri_off + 0, 0);
    wr_u16(out + tri_off + 2, 1);
    wr_u16(out + tri_off + 4, 2);

    u8 *tex = out + tex_off;
    for (u32 y = 0; y < TEX_H; ++y) {
        for (u32 x = 0; x < TEX_W; ++x) {
            u32 i = (y * TEX_W + x) * 4u;
            int on = ((x / 2) ^ (y / 2)) & 1;
            tex[i+0] = on ? 220 : 40;
            tex[i+1] = on ? 180 : 60;
            tex[i+2] = on ? 40 : 160;
            tex[i+3] = 255;
        }
    }
    u8 *meta = tex + TEX_BYTES;
    wr_i32(meta + 0, (i32)0xAE7E0001);
    wr_i32(meta + 4, (i32)TEX_W);
    wr_i32(meta + 8, (i32)TEX_H);
    wr_i32(meta + 12, (i32)tex_off);
    return total;
}

u32 aether_mdl_write_textured_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[4096];
    u32 n = aether_mdl_write_textured_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

u32 aether_mdl_fixture_texture_rgba(u8 *out, u32 cap, u32 *out_w, u32 *out_h) {
    const u32 TEX_W = 8, TEX_H = 8;
    const u32 TEX_BYTES = TEX_W * TEX_H * 4u;
    if (out_w) *out_w = TEX_W;
    if (out_h) *out_h = TEX_H;
    if (!out || cap < TEX_BYTES) return 0;
    for (u32 y = 0; y < TEX_H; ++y) {
        for (u32 x = 0; x < TEX_W; ++x) {
            u32 i = (y * TEX_W + x) * 4u;
            int on = ((x / 2) ^ (y / 2)) & 1;
            out[i+0] = on ? 220 : 40;
            out[i+1] = on ? 180 : 60;
            out[i+2] = on ? 40 : 160;
            out[i+3] = 255;
        }
    }
    return TEX_BYTES;
}

/* Sequence fixture = textured fixture + trailing seq meta (frame count / bone count). */
u32 aether_mdl_write_seq_fixture(u8 *out, u32 cap) {
    u32 n = aether_mdl_write_textured_fixture(out, cap);
    if (!n || cap < n + 32) return 0;
    /* Mark sequences=1 in header (offset 164/168) even if no on-disk anim block —
     * skinning uses aether_mdl_sequence_init_sway for clean-room frames. */
    wr_i32(out + 164, 1); /* numseq */
    wr_i32(out + 168, 0); /* seqindex (none — runtime sway stub) */
    u8 *meta = out + n;
    wr_i32(meta + 0, (i32)0xAE7E5E01); /* seq magic */
    wr_i32(meta + 4, 4);               /* frame_count */
    wr_i32(meta + 8, 2);               /* bone_count */
    wr_f32(meta + 12, 10.f);           /* fps */
    return n + 32;
}

u32 aether_mdl_write_seq_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[8192];
    u32 n = aether_mdl_write_seq_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

u32 aether_mdl_fixture_seq_frame_count(const u8 *data, u32 size) {
    if (!data || size < 32) return 0;
    /* Scan last 32 bytes for seq magic */
    if (size >= 32) {
        const u8 *meta = data + size - 32;
        i32 mag = (i32)((u32)meta[0] | ((u32)meta[1]<<8) | ((u32)meta[2]<<16) | ((u32)meta[3]<<24));
        if (mag == (i32)0xAE7E5E01) {
            return (u32)((u32)meta[4] | ((u32)meta[5]<<8) | ((u32)meta[6]<<16) | ((u32)meta[7]<<24));
        }
    }
    return 0;
}

/* Studio fixture magic: sequence descriptor + packed bone keys + hitboxes. */
#define AETHER_STUDIO_MAGIC  ((i32)0xAE7E5E02)
#define AETHER_HITBOX_MAGIC  ((i32)0xAE7E4842)

u32 aether_mdl_write_studio_fixture(u8 *out, u32 cap) {
    u32 n = aether_mdl_write_textured_fixture(out, cap);
    if (!n || !out) return 0;
    const u32 frames = 4, bones = 2, hitboxes = 2;
    const u32 key_bytes = frames * bones * 24u; /* 6 f32 */
    const u32 seq_hdr = 32;
    const u32 hb_hdr = 16;
    const u32 hb_bytes = hitboxes * 32u;
    const u32 need = n + seq_hdr + key_bytes + hb_hdr + hb_bytes + 16;
    if (cap < need) return 0;

    wr_i32(out + 164, 1); /* numseq */
    wr_i32(out + 168, (i32)n); /* seqindex → our trailer */
    wr_i32(out + 156, (i32)hitboxes);
    wr_i32(out + 160, (i32)(n + seq_hdr + key_bytes)); /* hitboxindex */

    u8 *seq = out + n;
    wr_i32(seq + 0, AETHER_STUDIO_MAGIC);
    wr_i32(seq + 4, (i32)frames);
    wr_i32(seq + 8, (i32)bones);
    wr_f32(seq + 12, 12.f); /* fps */
    wr_i32(seq + 16, 1);    /* loop */
    wr_i32(seq + 20, (i32)key_bytes);
    wr_i32(seq + 24, 0);
    wr_i32(seq + 28, 0);

    u8 *keys = seq + seq_hdr;
    for (u32 f = 0; f < frames; ++f) {
        f32 t = (f32)f / (f32)(frames - 1);
        f32 swing = sinf(t * 3.14159265f * 2.f) * 30.f;
        for (u32 b = 0; b < bones; ++b) {
            u8 *k = keys + (f * bones + b) * 24u;
            wr_f32(k + 0, 0.f);
            wr_f32(k + 4, 0.f);
            wr_f32(k + 8, (b == 0) ? 0.f : (10.f + 3.f * sinf(t * 6.2831853f)));
            wr_f32(k + 12, 0.f);
            wr_f32(k + 16, (b == 0) ? swing : (-swing * 0.6f));
            wr_f32(k + 20, 0.f);
        }
    }

    u8 *hb = keys + key_bytes;
    wr_i32(hb + 0, AETHER_HITBOX_MAGIC);
    wr_i32(hb + 4, (i32)hitboxes);
    wr_i32(hb + 8, 0);
    wr_i32(hb + 12, 0);
    /* Hitbox 0: torso on bone 0 */
    wr_i32(hb + 16 + 0, 0);  /* bone */
    wr_i32(hb + 16 + 4, 1);  /* group generic */
    wr_f32(hb + 16 + 8, -12.f); wr_f32(hb + 16 + 12, -8.f); wr_f32(hb + 16 + 16, 0.f);
    wr_f32(hb + 16 + 20, 12.f); wr_f32(hb + 16 + 24, 8.f); wr_f32(hb + 16 + 28, 24.f);
    /* Hitbox 1: head-ish on bone 1 */
    wr_i32(hb + 16 + 32 + 0, 1);
    wr_i32(hb + 16 + 32 + 4, 2); /* head group */
    wr_f32(hb + 16 + 32 + 8, -6.f); wr_f32(hb + 16 + 32 + 12, -6.f); wr_f32(hb + 16 + 32 + 16, 20.f);
    wr_f32(hb + 16 + 32 + 20, 6.f); wr_f32(hb + 16 + 32 + 24, 6.f); wr_f32(hb + 16 + 32 + 28, 32.f);

    return need;
}

u32 aether_mdl_write_studio_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[16384];
    u32 n = aether_mdl_write_studio_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

static i32 rd_i32_le(const u8 *p) {
    return (i32)((u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24));
}
static f32 rd_f32_le(const u8 *p) {
    i32 i = rd_i32_le(p);
    f32 f; memcpy(&f, &i, 4);
    return f;
}

u32 aether_mdl_fixture_hitboxes(const u8 *data, u32 size,
                                aether_mdl_hitbox_t *out, u32 max_out) {
    if (!data || size < 48 || !out || max_out == 0) return 0;
    /* Scan for hitbox magic */
    for (u32 off = 0; off + 16 < size; ++off) {
        if (rd_i32_le(data + off) != AETHER_HITBOX_MAGIC) continue;
        i32 count = rd_i32_le(data + off + 4);
        if (count <= 0 || count > (i32)AETHER_MDL_FIXTURE_MAX_HITBOXES) return 0;
        u32 need = (u32)count * 32u;
        if (off + 16 + need > size) return 0;
        u32 n = (u32)count;
        if (n > max_out) n = max_out;
        for (u32 i = 0; i < n; ++i) {
            const u8 *h = data + off + 16 + i * 32u;
            out[i].bone = rd_i32_le(h + 0);
            out[i].group = rd_i32_le(h + 4);
            out[i].mins[0] = rd_f32_le(h + 8);
            out[i].mins[1] = rd_f32_le(h + 12);
            out[i].mins[2] = rd_f32_le(h + 16);
            out[i].maxs[0] = rd_f32_le(h + 20);
            out[i].maxs[1] = rd_f32_le(h + 24);
            out[i].maxs[2] = rd_f32_le(h + 28);
        }
        return n;
    }
    return 0;
}

bool aether_mdl_hitbox_trace(const aether_mdl_hitbox_t *boxes, u32 count,
                             const f32 origin[3], const f32 dir[3], f32 max_dist,
                             i32 *out_index, f32 *out_t, f32 out_point[3]) {
    if (!boxes || !origin || !dir || count == 0 || max_dist <= 0.f) return false;
    f32 best_t = max_dist + 1.f;
    i32 best = -1;
    f32 best_pt[3] = {0,0,0};
    for (u32 i = 0; i < count; ++i) {
        /* Slab test */
        f32 tmin = 0.f, tmax = max_dist;
        int ok = 1;
        for (int a = 0; a < 3; ++a) {
            f32 o = origin[a], d = dir[a];
            f32 mn = boxes[i].mins[a], mx = boxes[i].maxs[a];
            if (fabsf(d) < 1e-8f) {
                if (o < mn || o > mx) { ok = 0; break; }
                continue;
            }
            f32 inv = 1.f / d;
            f32 t0 = (mn - o) * inv;
            f32 t1 = (mx - o) * inv;
            if (t0 > t1) { f32 tmp = t0; t0 = t1; t1 = tmp; }
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmin > tmax) { ok = 0; break; }
        }
        if (!ok) continue;
        if (tmin < 0.f) tmin = 0.f;
        if (tmin < best_t) {
            best_t = tmin;
            best = (i32)i;
            best_pt[0] = origin[0] + dir[0] * tmin;
            best_pt[1] = origin[1] + dir[1] * tmin;
            best_pt[2] = origin[2] + dir[2] * tmin;
        }
    }
    if (best < 0) return false;
    if (out_index) *out_index = best;
    if (out_t) *out_t = best_t;
    if (out_point) { out_point[0]=best_pt[0]; out_point[1]=best_pt[1]; out_point[2]=best_pt[2]; }
    return true;
}

#define AETHER_ATTACH_MAGIC  ((i32)0xAE7E4154) /* ATT */
#define AETHER_EVENT_MAGIC   ((i32)0xAE7E4556) /* EV */
#define AETHER_RLE_MAGIC     ((i32)0xAE7E524C) /* RLE */

static void wr_name32(u8 *p, const char *s) {
    memset(p, 0, 32);
    if (!s) return;
    size_t n = strlen(s);
    if (n > 31) n = 31;
    memcpy(p, s, n);
}

u32 aether_mdl_write_studio_fixture_ex(u8 *out, u32 cap) {
    /* Base studio (packed keys + hitboxes) then append RLE + attachments + events. */
    u32 n = aether_mdl_write_studio_fixture(out, cap);
    if (!n || !out) return 0;
    const u32 frames = 4, bones = 2;
    /* RLE block: for each bone×channel(6), store runs of (count:u8, value:f32).
     * Encode each channel's 4 frames as RLE. */
    u32 rle_est = 16 + bones * 6 * (1 + 4 * 5); /* generous */
    u32 att_bytes = 16 + 2 * (4 + 12 + 12 + 32); /* 2 attachments */
    u32 evt_bytes = 16 + 2 * (4 + 4 + 64);
    if (cap < n + rle_est + att_bytes + evt_bytes + 64) return 0;

    u8 *rle = out + n;
    wr_i32(rle + 0, AETHER_RLE_MAGIC);
    wr_i32(rle + 4, (i32)frames);
    wr_i32(rle + 8, (i32)bones);
    wr_i32(rle + 12, 0); /* payload size filled later */
    u8 *payload = rle + 16;
    u32 poff = 0;
    for (u32 b = 0; b < bones; ++b) {
        for (u32 ch = 0; ch < 6; ++ch) {
            /* Build 4 values matching studio packed keys */
            f32 vals[4];
            for (u32 f = 0; f < frames; ++f) {
                f32 t = (f32)f / (f32)(frames - 1);
                f32 swing = sinf(t * 3.14159265f * 2.f) * 30.f;
                if (ch < 3) {
                    /* pos */
                    if (ch == 2) vals[f] = (b == 0) ? 0.f : (10.f + 3.f * sinf(t * 6.2831853f));
                    else vals[f] = 0.f;
                } else {
                    /* angles: yaw on channel 4 (index 1 of angles) */
                    int ai = (int)ch - 3;
                    if (ai == 1) vals[f] = (b == 0) ? swing : (-swing * 0.6f);
                    else vals[f] = 0.f;
                }
            }
            /* Simple RLE */
            u32 i = 0;
            while (i < frames) {
                u32 j = i + 1;
                while (j < frames && fabsf(vals[j] - vals[i]) < 1e-4f) j++;
                u8 run = (u8)(j - i);
                payload[poff++] = run;
                wr_f32(payload + poff, vals[i]);
                poff += 4;
                i = j;
            }
        }
    }
    wr_i32(rle + 12, (i32)poff);
    u32 rle_total = 16 + poff;
    /* Align */
    while ((rle_total & 3u) && (n + rle_total) < cap) {
        out[n + rle_total] = 0;
        rle_total++;
    }

    u8 *att = out + n + rle_total;
    wr_i32(att + 0, AETHER_ATTACH_MAGIC);
    wr_i32(att + 4, 2); /* count */
    wr_i32(att + 8, 0);
    wr_i32(att + 12, 0);
    /* attachment 0: muzzle on bone 1 */
    wr_i32(att + 16 + 0, 1); /* bone */
    wr_f32(att + 16 + 4, 0.f);
    wr_f32(att + 16 + 8, 18.f);
    wr_f32(att + 16 + 12, 4.f);
    wr_f32(att + 16 + 16, 0.f);
    wr_f32(att + 16 + 20, 0.f);
    wr_f32(att + 16 + 24, 0.f);
    wr_name32(att + 16 + 28, "muzzle");
    /* attachment 1: shell on bone 0 */
    wr_i32(att + 16 + 60 + 0, 0);
    wr_f32(att + 16 + 60 + 4, 4.f);
    wr_f32(att + 16 + 60 + 8, 2.f);
    wr_f32(att + 16 + 60 + 12, 8.f);
    wr_f32(att + 16 + 60 + 16, 0.f);
    wr_f32(att + 16 + 60 + 20, 90.f);
    wr_f32(att + 16 + 60 + 24, 0.f);
    wr_name32(att + 16 + 60 + 28, "shell");
    u32 att_total = 16 + 2 * 60;

    u8 *ev = att + att_total;
    wr_i32(ev + 0, AETHER_EVENT_MAGIC);
    wr_i32(ev + 4, 2);
    wr_i32(ev + 8, 0);
    wr_i32(ev + 12, 0);
    /* event 0: muzzle flash at frame 0.5 */
    wr_f32(ev + 16 + 0, 0.5f);
    wr_i32(ev + 16 + 4, 5001);
    memset(ev + 16 + 8, 0, 64);
    memcpy(ev + 16 + 8, "muzzle", 6);
    /* event 1: sound cue at frame 1.0 */
    wr_f32(ev + 16 + 72 + 0, 1.0f);
    wr_i32(ev + 16 + 72 + 4, 5004);
    memset(ev + 16 + 72 + 8, 0, 64);
    memcpy(ev + 16 + 72 + 8, "weapons/generic_shot.wav", 24);
    u32 ev_total = 16 + 2 * 72;

    /* Also stamp attachment count into MDL header slots (212/216). */
    wr_i32(out + 212, 2);
    wr_i32(out + 216, (i32)(n + rle_total));

    return n + rle_total + att_total + ev_total;
}

u32 aether_mdl_fixture_attachments(const u8 *data, u32 size,
                                   aether_mdl_attachment_t *out, u32 max_out) {
    if (!data || !out || max_out == 0 || size < 32) return 0;
    for (u32 off = 0; off + 16 < size; ++off) {
        if (rd_i32_le(data + off) != AETHER_ATTACH_MAGIC) continue;
        i32 count = rd_i32_le(data + off + 4);
        if (count <= 0 || count > (i32)AETHER_MDL_FIXTURE_MAX_ATTACHMENTS) return 0;
        u32 need = (u32)count * 60u;
        if (off + 16 + need > size) return 0;
        u32 n = (u32)count;
        if (n > max_out) n = max_out;
        for (u32 i = 0; i < n; ++i) {
            const u8 *a = data + off + 16 + i * 60u;
            out[i].bone = rd_i32_le(a + 0);
            out[i].origin[0] = rd_f32_le(a + 4);
            out[i].origin[1] = rd_f32_le(a + 8);
            out[i].origin[2] = rd_f32_le(a + 12);
            out[i].angles_deg[0] = rd_f32_le(a + 16);
            out[i].angles_deg[1] = rd_f32_le(a + 20);
            out[i].angles_deg[2] = rd_f32_le(a + 24);
            memset(out[i].name, 0, sizeof out[i].name);
            memcpy(out[i].name, a + 28, 31);
        }
        return n;
    }
    return 0;
}

i32 aether_mdl_attachment_find(const aether_mdl_attachment_t *atts, u32 count,
                               const char *name) {
    if (!atts || !name) return -1;
    for (u32 i = 0; i < count; ++i) {
        if (strncmp(atts[i].name, name, 31) == 0) return (i32)i;
    }
    return -1;
}

bool aether_mdl_attachment_transform(const aether_mdl_attachment_t *att,
                                     const f32 *bone_mats, u32 bone_count,
                                     f32 out_pos[3], f32 out_forward[3]) {
    if (!att || !out_pos) return false;
    f32 lx = att->origin[0], ly = att->origin[1], lz = att->origin[2];
    if (bone_mats && bone_count > 0 && att->bone >= 0 && (u32)att->bone < bone_count) {
        const f32 *m = bone_mats + (u32)att->bone * 16u;
        /* column-major 4x4 */
        out_pos[0] = m[0]*lx + m[4]*ly + m[8]*lz  + m[12];
        out_pos[1] = m[1]*lx + m[5]*ly + m[9]*lz  + m[13];
        out_pos[2] = m[2]*lx + m[6]*ly + m[10]*lz + m[14];
        if (out_forward) {
            out_forward[0] = m[0]; out_forward[1] = m[1]; out_forward[2] = m[2];
        }
    } else {
        out_pos[0]=lx; out_pos[1]=ly; out_pos[2]=lz;
        if (out_forward) { out_forward[0]=1; out_forward[1]=0; out_forward[2]=0; }
    }
    return true;
}

u32 aether_mdl_fixture_events(const u8 *data, u32 size,
                              aether_mdl_studio_event_t *out, u32 max_out) {
    if (!data || !out || max_out == 0 || size < 32) return 0;
    for (u32 off = 0; off + 16 < size; ++off) {
        if (rd_i32_le(data + off) != AETHER_EVENT_MAGIC) continue;
        i32 count = rd_i32_le(data + off + 4);
        if (count <= 0 || count > (i32)AETHER_MDL_FIXTURE_MAX_EVENTS) return 0;
        u32 need = (u32)count * 72u;
        if (off + 16 + need > size) return 0;
        u32 n = (u32)count;
        if (n > max_out) n = max_out;
        for (u32 i = 0; i < n; ++i) {
            const u8 *e = data + off + 16 + i * 72u;
            out[i].frame = rd_f32_le(e + 0);
            out[i].event = rd_i32_le(e + 4);
            memset(out[i].options, 0, sizeof out[i].options);
            memcpy(out[i].options, e + 8, 63);
        }
        return n;
    }
    return 0;
}

u32 aether_mdl_studio_events_fire(const aether_mdl_studio_event_t *evts, u32 count,
                                  f32 prev_frame, f32 frame,
                                  aether_mdl_studio_event_t *out_fired, u32 max_out) {
    if (!evts || count == 0) return 0;
    u32 n = 0;
    for (u32 i = 0; i < count; ++i) {
        f32 ef = evts[i].frame;
        int crossed = 0;
        if (frame >= prev_frame) {
            crossed = (ef > prev_frame && ef <= frame);
        } else {
            /* loop wrap */
            crossed = (ef > prev_frame || ef <= frame);
        }
        if (!crossed) continue;
        if (out_fired && n < max_out) out_fired[n] = evts[i];
        n++;
    }
    return n;
}

void aether_mdl_mat4_identity(f32 out[16]) {
    if (!out) return;
    memset(out, 0, 16 * sizeof(f32));
    out[0] = out[5] = out[10] = out[15] = 1.f;
}

void aether_mdl_mat4_translate(const f32 origin[3], f32 out[16]) {
    aether_mdl_mat4_identity(out);
    if (!origin || !out) return;
    out[12] = origin[0];
    out[13] = origin[1];
    out[14] = origin[2];
}

void aether_mdl_mat4_mul(const f32 A[16], const f32 B[16], f32 out[16]) {
    if (!A || !B || !out) return;
    f32 tmp[16];
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            tmp[c * 4 + r] =
                A[0 * 4 + r] * B[c * 4 + 0] +
                A[1 * 4 + r] * B[c * 4 + 1] +
                A[2 * 4 + r] * B[c * 4 + 2] +
                A[3 * 4 + r] * B[c * 4 + 3];
        }
    }
    memcpy(out, tmp, sizeof tmp);
}

bool aether_mdl_attachment_chain_world(const aether_mdl_attachment_t *hand_att,
                                       const f32 *player_bone_mats, u32 player_bones,
                                       const aether_mdl_attachment_t *weapon_att,
                                       const f32 *weapon_bone_mats, u32 weapon_bones,
                                       const f32 player_origin[3],
                                       f32 out_pos[3], f32 out_forward[3]) {
    if (!hand_att || !weapon_att || !out_pos) return false;
    /* 1) Weapon attach in weapon bone space */
    f32 muz_pos[3], muz_fwd[3];
    if (!aether_mdl_attachment_transform(weapon_att, weapon_bone_mats, weapon_bones,
                                         muz_pos, muz_fwd))
        return false;
    /* 2) Hand attach on player → world-relative bone space */
    f32 hand_pos[3], hand_fwd[3];
    if (!aether_mdl_attachment_transform(hand_att, player_bone_mats, player_bones,
                                         hand_pos, hand_fwd))
        return false;
    /* 3) Build hand basis (forward = hand_fwd, up = Z, right = cross) and place muzzle */
    f32 fx = hand_fwd[0], fy = hand_fwd[1], fz = hand_fwd[2];
    f32 fl = sqrtf(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-5f) { fx = 1.f; fy = 0.f; fz = 0.f; fl = 1.f; }
    fx /= fl; fy /= fl; fz /= fl;
    /* right = forward × up(0,0,1) approx; fallback if parallel */
    f32 rx = fy * 1.f - fz * 0.f;
    f32 ry = fz * 0.f - fx * 1.f;
    f32 rz = fx * 0.f - fy * 0.f;
    f32 rl = sqrtf(rx*rx + ry*ry + rz*rz);
    if (rl < 1e-5f) { rx = 0.f; ry = 1.f; rz = 0.f; rl = 1.f; }
    rx /= rl; ry /= rl; rz /= rl;
    /* up = right × forward */
    f32 ux = ry * fz - rz * fy;
    f32 uy = rz * fx - rx * fz;
    f32 uz = rx * fy - ry * fx;
    /* weapon local muzzle offset relative to weapon origin, treated as hand-local offset */
    f32 lx = muz_pos[0], ly = muz_pos[1], lz = muz_pos[2];
    f32 wx = hand_pos[0] + rx * lx + ux * ly + fx * lz;
    f32 wy = hand_pos[1] + ry * lx + uy * ly + fy * lz;
    f32 wz = hand_pos[2] + rz * lx + uz * ly + fz * lz;
    if (player_origin) {
        wx += player_origin[0];
        wy += player_origin[1];
        wz += player_origin[2];
    }
    out_pos[0] = wx; out_pos[1] = wy; out_pos[2] = wz;
    if (out_forward) {
        /* compose hand forward with weapon muzzle forward (weapon fwd in hand basis) */
        f32 mfx = muz_fwd[0], mfy = muz_fwd[1], mfz = muz_fwd[2];
        out_forward[0] = rx * mfx + ux * mfy + fx * mfz;
        out_forward[1] = ry * mfx + uy * mfy + fy * mfz;
        out_forward[2] = rz * mfx + uz * mfy + fz * mfz;
        f32 ml = sqrtf(out_forward[0]*out_forward[0] + out_forward[1]*out_forward[1] +
                       out_forward[2]*out_forward[2]);
        if (ml > 1e-5f) {
            out_forward[0] /= ml; out_forward[1] /= ml; out_forward[2] /= ml;
        } else {
            out_forward[0] = fx; out_forward[1] = fy; out_forward[2] = fz;
        }
    }
    return true;
}


/* ---- LOD / bodygroup trailer (clean-room) ---- */
#define AETHER_LOD_MAGIC_C       ((i32)0xAE7E10D0)
#define AETHER_BODYGROUP_MAGIC_C ((i32)0xAE7E8006)

u32 aether_mdl_write_lod_fixture(u8 *out, u32 cap) {
    u32 n = aether_mdl_write_studio_fixture_ex(out, cap);
    if (!n || !out) return 0;
    /* LOD table: 3 levels — 12 / 6 / 3 tris at 256 / 512 / 1e9 dist */
    const u32 lod_hdr = 16;
    const u32 lod_lvl = 3 * 16; /* level, tri_count, max_distance(+pad) as i32,i32,f32,i32 */
    const u32 bg_hdr = 16;
    const u32 bg_part = 2 * (32 + 4 + 4 + 4 * 4); /* name + sub_count + selected + 4 tris */
    u32 need = n + lod_hdr + lod_lvl + bg_hdr + bg_part + 16;
    if (cap < need) return 0;

    u8 *lod = out + n;
    wr_i32(lod + 0, AETHER_LOD_MAGIC_C);
    wr_i32(lod + 4, 3); /* count */
    wr_i32(lod + 8, 0);
    wr_i32(lod + 12, 0);
    /* level 0 */
    wr_i32(lod + 16 + 0, 0);
    wr_i32(lod + 16 + 4, 12);
    wr_f32(lod + 16 + 8, 256.f);
    wr_i32(lod + 16 + 12, 0);
    /* level 1 */
    wr_i32(lod + 32 + 0, 1);
    wr_i32(lod + 32 + 4, 6);
    wr_f32(lod + 32 + 8, 512.f);
    wr_i32(lod + 32 + 12, 0);
    /* level 2 */
    wr_i32(lod + 48 + 0, 2);
    wr_i32(lod + 48 + 4, 3);
    wr_f32(lod + 48 + 8, 1.0e9f);
    wr_i32(lod + 48 + 12, 0);
    u32 lod_total = lod_hdr + lod_lvl;

    u8 *bg = out + n + lod_total;
    wr_i32(bg + 0, AETHER_BODYGROUP_MAGIC_C);
    wr_i32(bg + 4, 2); /* parts */
    wr_i32(bg + 8, 0);
    wr_i32(bg + 12, 0);
    /* part 0: body — subs 8 / 4 tris */
    wr_name(bg + 16, 32, "body");
    wr_i32(bg + 16 + 32, 2); /* submodel_count */
    wr_i32(bg + 16 + 36, 0); /* selected */
    wr_i32(bg + 16 + 40, 8);
    wr_i32(bg + 16 + 44, 4);
    wr_i32(bg + 16 + 48, 0);
    wr_i32(bg + 16 + 52, 0);
    /* part 1: head — subs 4 / 2 tris */
    wr_name(bg + 16 + 56, 32, "head");
    wr_i32(bg + 16 + 56 + 32, 2);
    wr_i32(bg + 16 + 56 + 36, 0);
    wr_i32(bg + 16 + 56 + 40, 4);
    wr_i32(bg + 16 + 56 + 44, 2);
    wr_i32(bg + 16 + 56 + 48, 0);
    wr_i32(bg + 16 + 56 + 52, 0);

    return n + lod_total + bg_hdr + bg_part;
}

u32 aether_mdl_write_lod_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[32768];
    u32 n = aether_mdl_write_lod_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

u32 aether_mdl_fixture_lods(const u8 *data, u32 size, aether_mdl_lod_table_t *out) {
    if (!data || !out || size < 32) return 0;
    memset(out, 0, sizeof(*out));
    for (u32 off = 0; off + 16 < size; ++off) {
        if (rd_i32_le(data + off) != AETHER_LOD_MAGIC_C) continue;
        i32 count = rd_i32_le(data + off + 4);
        if (count <= 0 || count > (i32)AETHER_MDL_MAX_LODS) return 0;
        if (off + 16 + (u32)count * 16u > size) return 0;
        out->count = (u32)count;
        for (u32 i = 0; i < out->count; ++i) {
            const u8 *L = data + off + 16 + i * 16u;
            out->levels[i].level = rd_i32_le(L + 0);
            out->levels[i].tri_count = (u32)rd_i32_le(L + 4);
            out->levels[i].max_distance = rd_f32_le(L + 8);
        }
        return out->count;
    }
    return 0;
}

i32 aether_mdl_lod_select(const aether_mdl_lod_table_t *table, f32 distance) {
    if (!table || table->count == 0) return -1;
    if (distance < 0.f) distance = 0.f;
    for (u32 i = 0; i < table->count; ++i) {
        if (distance <= table->levels[i].max_distance)
            return (i32)i;
    }
    return (i32)(table->count - 1);
}

u32 aether_mdl_lod_tri_count(const aether_mdl_lod_table_t *table, i32 lod) {
    if (!table || lod < 0 || (u32)lod >= table->count) return 0;
    return table->levels[lod].tri_count;
}

bool aether_mdl_bodygroup_init_from_fixture(aether_mdl_bodygroup_state_t *st,
                                            const u8 *data, u32 size) {
    if (!st) return false;
    memset(st, 0, sizeof(*st));
    st->active_lod = 0;
    if (!data || size < 32) {
        /* defaults */
        st->part_count = 2;
        wr_name((u8*)st->parts[0].name, 32, "body");
        st->parts[0].submodel_count = 2;
        st->parts[0].tri_per_sub[0] = 8;
        st->parts[0].tri_per_sub[1] = 4;
        wr_name((u8*)st->parts[1].name, 32, "head");
        st->parts[1].submodel_count = 2;
        st->parts[1].tri_per_sub[0] = 4;
        st->parts[1].tri_per_sub[1] = 2;
        return true;
    }
    for (u32 off = 0; off + 16 < size; ++off) {
        if (rd_i32_le(data + off) != AETHER_BODYGROUP_MAGIC_C) continue;
        i32 parts = rd_i32_le(data + off + 4);
        if (parts <= 0 || parts > (i32)AETHER_MDL_MAX_BODYPARTS) return false;
        const u32 psz = 32 + 4 + 4 + 16;
        if (off + 16 + (u32)parts * psz > size) return false;
        st->part_count = (u32)parts;
        for (u32 i = 0; i < st->part_count; ++i) {
            const u8 *p = data + off + 16 + i * psz;
            memcpy(st->parts[i].name, p, 32);
            st->parts[i].name[31] = 0;
            st->parts[i].submodel_count = (u32)rd_i32_le(p + 32);
            st->parts[i].selected = (u32)rd_i32_le(p + 36);
            if (st->parts[i].submodel_count > AETHER_MDL_MAX_SUBMODELS)
                st->parts[i].submodel_count = AETHER_MDL_MAX_SUBMODELS;
            for (u32 s = 0; s < AETHER_MDL_MAX_SUBMODELS; ++s)
                st->parts[i].tri_per_sub[s] = (u32)rd_i32_le(p + 40 + s * 4);
            if (st->parts[i].selected >= st->parts[i].submodel_count)
                st->parts[i].selected = 0;
        }
        return true;
    }
    /* fallback defaults */
    return aether_mdl_bodygroup_init_from_fixture(st, NULL, 0);
}

bool aether_mdl_bodygroup_set(aether_mdl_bodygroup_state_t *st, u32 part, u32 sub) {
    if (!st || part >= st->part_count) return false;
    if (sub >= st->parts[part].submodel_count) return false;
    st->parts[part].selected = sub;
    return true;
}

u32 aether_mdl_bodygroup_get(const aether_mdl_bodygroup_state_t *st, u32 part) {
    if (!st || part >= st->part_count) return 0;
    return st->parts[part].selected;
}

u32 aether_mdl_bodygroup_cycle(aether_mdl_bodygroup_state_t *st, u32 part, int dir) {
    if (!st || part >= st->part_count || st->parts[part].submodel_count == 0) return 0;
    u32 n = st->parts[part].submodel_count;
    i32 cur = (i32)st->parts[part].selected;
    if (dir >= 0) cur = (cur + 1) % (i32)n;
    else cur = (cur - 1 + (i32)n) % (i32)n;
    st->parts[part].selected = (u32)cur;
    return st->parts[part].selected;
}

u32 aether_mdl_bodygroup_tri_total(const aether_mdl_bodygroup_state_t *st,
                                   const aether_mdl_lod_table_t *lods, i32 lod) {
    if (!st) return 0;
    u32 sum = 0;
    for (u32 i = 0; i < st->part_count; ++i) {
        u32 s = st->parts[i].selected;
        if (s >= st->parts[i].submodel_count) s = 0;
        sum += st->parts[i].tri_per_sub[s];
    }
    u32 budget = aether_mdl_lod_tri_count(lods, lod);
    if (budget > 0 && sum > budget) sum = budget;
    return sum;
}

i32 aether_mdl_bodygroup_apply_lod(aether_mdl_bodygroup_state_t *st,
                                   const aether_mdl_lod_table_t *lods, f32 distance) {
    if (!st) return -1;
    i32 lod = aether_mdl_lod_select(lods, distance);
    st->active_lod = lod;
    return lod;
}
