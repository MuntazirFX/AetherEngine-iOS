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

#ifndef AETHER_TEXGROUP_MAGIC_C
#define AETHER_TEXGROUP_MAGIC_C ((i32)0xAE7E5C10)
#endif

u32 aether_mdl_lod_extract_mesh(const aether_mdl_lod_table_t *table, i32 lod,
                                f32 *out_pos, u32 max_verts,
                                u32 *out_idx, u32 max_idx,
                                u32 *out_vert_count, u32 *out_tri_count) {
    if (out_vert_count) *out_vert_count = 0;
    if (out_tri_count) *out_tri_count = 0;
    if (!table || !out_pos || !out_idx) return 0;
    u32 budget = aether_mdl_lod_tri_count(table, lod);
    if (budget == 0) return 0;
    if (budget > AETHER_MDL_LOD_EXTRACT_MAX_TRIS) budget = AETHER_MDL_LOD_EXTRACT_MAX_TRIS;
    /* Need 3 unique verts per tri for non-indexed fan alternative; use shared apex fan:
     * verts = budget + 2 (apex + ring), tris = budget, indices = 3*budget */
    u32 verts = budget + 2;
    u32 idx_need = budget * 3;
    if (verts > max_verts || idx_need > max_idx) {
        /* Fall back to independent tris if capacity tight */
        verts = budget * 3;
        if (verts > max_verts || idx_need > max_idx) {
            u32 fit = max_verts / 3;
            if (fit > max_idx / 3) fit = max_idx / 3;
            if (fit == 0) return 0;
            budget = fit;
            verts = budget * 3;
            idx_need = budget * 3;
            for (u32 t = 0; t < budget; ++t) {
                f32 a = (f32)t * 0.7f;
                f32 r = 8.f + (f32)(lod >= 0 ? lod : 0) * 2.f;
                out_pos[t*9+0] = 0.f; out_pos[t*9+1] = 0.f; out_pos[t*9+2] = 0.f;
                out_pos[t*9+3] = cosf(a) * r; out_pos[t*9+4] = sinf(a) * r; out_pos[t*9+5] = 0.f;
                out_pos[t*9+6] = cosf(a+0.4f) * r; out_pos[t*9+7] = sinf(a+0.4f) * r; out_pos[t*9+8] = 1.f;
                out_idx[t*3+0] = t*3+0;
                out_idx[t*3+1] = t*3+1;
                out_idx[t*3+2] = t*3+2;
            }
            if (out_vert_count) *out_vert_count = verts;
            if (out_tri_count) *out_tri_count = budget;
            return budget;
        }
    }
    /* Shared-apex fan */
    out_pos[0] = 0.f; out_pos[1] = 0.f; out_pos[2] = 4.f; /* apex */
    f32 r = 12.f / (1.f + (f32)(lod > 0 ? lod : 0));
    for (u32 i = 0; i <= budget; ++i) {
        f32 a = (f32)i * (6.2831853f / (f32)budget);
        out_pos[(i+1)*3+0] = cosf(a) * r;
        out_pos[(i+1)*3+1] = sinf(a) * r;
        out_pos[(i+1)*3+2] = 0.f;
    }
    for (u32 t = 0; t < budget; ++t) {
        out_idx[t*3+0] = 0;
        out_idx[t*3+1] = t + 1;
        out_idx[t*3+2] = t + 2;
    }
    if (out_vert_count) *out_vert_count = verts;
    if (out_tri_count) *out_tri_count = budget;
    return budget;
}

i32 aether_mdl_lod_extract_by_distance(const aether_mdl_lod_table_t *table, f32 distance,
                                       f32 *out_pos, u32 max_verts,
                                       u32 *out_idx, u32 max_idx,
                                       u32 *out_vert_count, u32 *out_tri_count) {
    i32 lod = aether_mdl_lod_select(table, distance);
    if (lod < 0) return -1;
    if (!aether_mdl_lod_extract_mesh(table, lod, out_pos, max_verts, out_idx, max_idx,
                                     out_vert_count, out_tri_count))
        return -1;
    return lod;
}

u32 aether_mdl_write_skin_lod_fixture(u8 *out, u32 cap) {
    u32 n = aether_mdl_write_lod_fixture(out, cap);
    if (!n || !out) return 0;
    const u32 skin_hdr = 16;
    const u32 skin_grp = 2 * (32 + 4 + 4); /* name + tex_count + selected */
    if (cap < n + skin_hdr + skin_grp) return 0;
    u8 *s = out + n;
    wr_i32(s + 0, AETHER_TEXGROUP_MAGIC_C);
    wr_i32(s + 4, 2); /* groups */
    wr_i32(s + 8, 0);
    wr_i32(s + 12, 0);
    wr_name(s + 16, 32, "body");
    wr_i32(s + 16 + 32, 3); /* 3 textures */
    wr_i32(s + 16 + 36, 0);
    wr_name(s + 16 + 40, 32, "chrome");
    wr_i32(s + 16 + 40 + 32, 2);
    wr_i32(s + 16 + 40 + 36, 0);
    return n + skin_hdr + skin_grp;
}

u32 aether_mdl_write_skin_lod_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[32768];
    u32 n = aether_mdl_write_skin_lod_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

bool aether_mdl_texgroup_init_from_fixture(aether_mdl_texgroup_state_t *st,
                                       const u8 *data, u32 size) {
    if (!st) return false;
    memset(st, 0, sizeof(*st));
    st->active_group = 0;
    if (!data || size < 32) {
        st->group_count = 2;
        wr_name((u8*)st->groups[0].name, 32, "body");
        st->groups[0].texture_count = 3;
        wr_name((u8*)st->groups[1].name, 32, "chrome");
        st->groups[1].texture_count = 2;
        return true;
    }
    for (u32 off = 0; off + 16 < size; ++off) {
        if (rd_i32_le(data + off) != AETHER_TEXGROUP_MAGIC_C) continue;
        i32 groups = rd_i32_le(data + off + 4);
        if (groups <= 0 || groups > (i32)AETHER_MDL_MAX_TEXGROUPS) return false;
        const u32 gsz = 32 + 4 + 4;
        if (off + 16 + (u32)groups * gsz > size) return false;
        st->group_count = (u32)groups;
        for (u32 i = 0; i < st->group_count; ++i) {
            const u8 *g = data + off + 16 + i * gsz;
            memcpy(st->groups[i].name, g, 32);
            st->groups[i].name[31] = 0;
            st->groups[i].texture_count = (u32)rd_i32_le(g + 32);
            st->groups[i].selected = (u32)rd_i32_le(g + 36);
            if (st->groups[i].texture_count > AETHER_MDL_MAX_TEXGROUP_SKINS)
                st->groups[i].texture_count = AETHER_MDL_MAX_TEXGROUP_SKINS;
            if (st->groups[i].selected >= st->groups[i].texture_count)
                st->groups[i].selected = 0;
        }
        return true;
    }
    return aether_mdl_texgroup_init_from_fixture(st, NULL, 0);
}

bool aether_mdl_texgroup_set(aether_mdl_texgroup_state_t *st, u32 group, u32 tex) {
    if (!st || group >= st->group_count) return false;
    if (tex >= st->groups[group].texture_count) return false;
    st->groups[group].selected = tex;
    st->active_group = (i32)group;
    return true;
}

u32 aether_mdl_texgroup_get(const aether_mdl_texgroup_state_t *st, u32 group) {
    if (!st || group >= st->group_count) return 0;
    return st->groups[group].selected;
}

u32 aether_mdl_texgroup_cycle(aether_mdl_texgroup_state_t *st, u32 group, int dir) {
    if (!st || group >= st->group_count || st->groups[group].texture_count == 0) return 0;
    u32 n = st->groups[group].texture_count;
    i32 cur = (i32)st->groups[group].selected;
    if (dir >= 0) cur = (cur + 1) % (i32)n;
    else cur = (cur - 1 + (i32)n) % (i32)n;
    st->groups[group].selected = (u32)cur;
    st->active_group = (i32)group;
    return st->groups[group].selected;
}

i32 aether_mdl_texgroup_select(aether_mdl_texgroup_state_t *st, u32 group) {
    if (!st || group >= st->group_count) return -1;
    st->active_group = (i32)group;
    return st->active_group;
}

#ifndef AETHER_LOD_BUCKET_MAGIC_C
#define AETHER_LOD_BUCKET_MAGIC_C ((i32)0xAE7E10D1)
#endif

/* Build a distinct procedural mesh per LOD: LOD0 = box-ish (12 tris),
 * LOD1 = octahedron-ish (8), LOD2 = tetra (4). Separate vertex pools. */
static void fill_bucket_box(aether_mdl_lod_mesh_bucket_t *b) {
    memset(b, 0, sizeof(*b));
    b->lod = 0;
    /* 8 corners of a unit box scaled */
    const f32 s = 10.f;
    f32 c[8][3] = {
        {-s,-s,-s},{ s,-s,-s},{ s, s,-s},{-s, s,-s},
        {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s}
    };
    for (int i = 0; i < 8; ++i) {
        b->positions[i*3+0]=c[i][0]; b->positions[i*3+1]=c[i][1]; b->positions[i*3+2]=c[i][2];
    }
    b->vert_count = 8;
    static const u32 faces[12][3] = {
        {0,1,2},{0,2,3}, {4,6,5},{4,7,6},
        {0,4,5},{0,5,1}, {2,6,7},{2,7,3},
        {0,3,7},{0,7,4}, {1,5,6},{1,6,2}
    };
    for (int t = 0; t < 12; ++t) {
        b->indices[t*3+0]=faces[t][0]; b->indices[t*3+1]=faces[t][1]; b->indices[t*3+2]=faces[t][2];
    }
    b->index_count = 36;
}

static void fill_bucket_octa(aether_mdl_lod_mesh_bucket_t *b) {
    memset(b, 0, sizeof(*b));
    b->lod = 1;
    const f32 s = 9.f;
    f32 v[6][3] = {{0,0,s},{0,0,-s},{s,0,0},{-s,0,0},{0,s,0},{0,-s,0}};
    for (int i = 0; i < 6; ++i) {
        b->positions[i*3+0]=v[i][0]; b->positions[i*3+1]=v[i][1]; b->positions[i*3+2]=v[i][2];
    }
    b->vert_count = 6;
    static const u32 faces[8][3] = {
        {0,2,4},{0,4,3},{0,3,5},{0,5,2},
        {1,4,2},{1,3,4},{1,5,3},{1,2,5}
    };
    for (int t = 0; t < 8; ++t) {
        b->indices[t*3+0]=faces[t][0]; b->indices[t*3+1]=faces[t][1]; b->indices[t*3+2]=faces[t][2];
    }
    b->index_count = 24;
}

static void fill_bucket_tetra(aether_mdl_lod_mesh_bucket_t *b) {
    memset(b, 0, sizeof(*b));
    b->lod = 2;
    const f32 s = 8.f;
    f32 v[4][3] = {{ s, s, s},{-s,-s, s},{-s, s,-s},{ s,-s,-s}};
    for (int i = 0; i < 4; ++i) {
        b->positions[i*3+0]=v[i][0]; b->positions[i*3+1]=v[i][1]; b->positions[i*3+2]=v[i][2];
    }
    b->vert_count = 4;
    static const u32 faces[4][3] = {{0,1,2},{0,3,1},{0,2,3},{1,3,2}};
    for (int t = 0; t < 4; ++t) {
        b->indices[t*3+0]=faces[t][0]; b->indices[t*3+1]=faces[t][1]; b->indices[t*3+2]=faces[t][2];
    }
    b->index_count = 12;
}

u32 aether_mdl_write_lod_mesh_fixture(u8 *out, u32 cap) {
    u32 n = aether_mdl_write_skin_lod_fixture(out, cap);
    if (!n || !out) return 0;
    aether_mdl_lod_mesh_set_t set;
    memset(&set, 0, sizeof set);
    set.count = 3;
    fill_bucket_box(&set.buckets[0]);
    fill_bucket_octa(&set.buckets[1]);
    fill_bucket_tetra(&set.buckets[2]);
    /* Serialize: magic, count, then per-bucket: lod, vc, ic, pos[vc*3], idx[ic] */
    u32 need = 16;
    for (u32 i = 0; i < set.count; ++i) {
        need += 12 + set.buckets[i].vert_count * 12 + set.buckets[i].index_count * 4;
    }
    if (cap < n + need) return 0;
    u8 *p = out + n;
    wr_i32(p + 0, AETHER_LOD_BUCKET_MAGIC_C);
    wr_i32(p + 4, (i32)set.count);
    wr_i32(p + 8, 0);
    wr_i32(p + 12, 0);
    u32 off = 16;
    for (u32 i = 0; i < set.count; ++i) {
        const aether_mdl_lod_mesh_bucket_t *b = &set.buckets[i];
        wr_i32(p + off, b->lod); off += 4;
        wr_i32(p + off, (i32)b->vert_count); off += 4;
        wr_i32(p + off, (i32)b->index_count); off += 4;
        for (u32 v = 0; v < b->vert_count * 3; ++v) {
            wr_f32(p + off, b->positions[v]); off += 4;
        }
        for (u32 k = 0; k < b->index_count; ++k) {
            wr_i32(p + off, (i32)b->indices[k]); off += 4;
        }
    }
    return n + off;
}

u32 aether_mdl_write_lod_mesh_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    u8 buf[65536];
    u32 n = aether_mdl_write_lod_mesh_fixture(buf, sizeof buf);
    if (!n) return 0;
    FILE *f = fopen(filepath, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (u32)w;
}

u32 aether_mdl_fixture_lod_meshes(const u8 *data, u32 size,
                                  aether_mdl_lod_mesh_set_t *out) {
    if (!out) return 0;
    memset(out, 0, sizeof(*out));
    if (!data || size < 16) {
        /* Defaults without fixture bytes */
        out->count = 3;
        fill_bucket_box(&out->buckets[0]);
        fill_bucket_octa(&out->buckets[1]);
        fill_bucket_tetra(&out->buckets[2]);
        return out->count;
    }
    for (u32 off = 0; off + 16 < size; ++off) {
        if (rd_i32_le(data + off) != AETHER_LOD_BUCKET_MAGIC_C) continue;
        i32 count = rd_i32_le(data + off + 4);
        if (count <= 0 || count > (i32)AETHER_MDL_MAX_LODS) return 0;
        u32 p = off + 16;
        out->count = (u32)count;
        for (u32 i = 0; i < out->count; ++i) {
            if (p + 12 > size) return 0;
            aether_mdl_lod_mesh_bucket_t *b = &out->buckets[i];
            memset(b, 0, sizeof(*b));
            b->lod = rd_i32_le(data + p); p += 4;
            b->vert_count = (u32)rd_i32_le(data + p); p += 4;
            b->index_count = (u32)rd_i32_le(data + p); p += 4;
            if (b->vert_count > AETHER_MDL_LOD_BUCKET_MAX_VERTS) return 0;
            if (b->index_count > AETHER_MDL_LOD_BUCKET_MAX_IDX) return 0;
            if (p + b->vert_count * 12 + b->index_count * 4 > size) return 0;
            for (u32 v = 0; v < b->vert_count * 3; ++v) {
                b->positions[v] = rd_f32_le(data + p); p += 4;
            }
            for (u32 k = 0; k < b->index_count; ++k) {
                b->indices[k] = (u32)rd_i32_le(data + p); p += 4;
            }
        }
        return out->count;
    }
    return aether_mdl_fixture_lod_meshes(NULL, 0, out);
}

i32 aether_mdl_lod_mesh_select(const aether_mdl_lod_table_t *table,
                               const aether_mdl_lod_mesh_set_t *meshes,
                               f32 distance,
                               const aether_mdl_lod_mesh_bucket_t **out_bucket) {
    if (out_bucket) *out_bucket = NULL;
    if (!meshes || meshes->count == 0) return -1;
    i32 lod = aether_mdl_lod_select(table, distance);
    if (lod < 0) lod = (i32)meshes->count - 1;
    if (lod >= (i32)meshes->count) lod = (i32)meshes->count - 1;
    for (u32 i = 0; i < meshes->count; ++i) {
        if (meshes->buckets[i].lod == lod) {
            if (out_bucket) *out_bucket = &meshes->buckets[i];
            return lod;
        }
    }
    /* Fallback: index == lod */
    if ((u32)lod < meshes->count) {
        if (out_bucket) *out_bucket = &meshes->buckets[lod];
        return lod;
    }
    return -1;
}

u32 aether_mdl_lod_mesh_copy(const aether_mdl_lod_mesh_bucket_t *bucket,
                             f32 *out_pos, u32 max_verts,
                             u32 *out_idx, u32 max_idx,
                             u32 *out_vert_count, u32 *out_tri_count) {
    if (out_vert_count) *out_vert_count = 0;
    if (out_tri_count) *out_tri_count = 0;
    if (!bucket || !out_pos || !out_idx) return 0;
    if (bucket->vert_count > max_verts || bucket->index_count > max_idx) return 0;
    memcpy(out_pos, bucket->positions, bucket->vert_count * 3 * sizeof(f32));
    memcpy(out_idx, bucket->indices, bucket->index_count * sizeof(u32));
    u32 tris = bucket->index_count / 3;
    if (out_vert_count) *out_vert_count = bucket->vert_count;
    if (out_tri_count) *out_tri_count = tris;
    return tris;
}

i32 aether_mdl_lod_gpu_issue_draw(const aether_mdl_lod_table_t *table,
                                  const aether_mdl_lod_mesh_set_t *meshes,
                                  f32 distance,
                                  aether_mdl_lod_gpu_draw_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!table || !meshes || !out) return -1;
    const aether_mdl_lod_mesh_bucket_t *b = NULL;
    i32 lod = aether_mdl_lod_mesh_select(table, meshes, distance, &b);
    out->distance = distance;
    out->lod = lod;
    out->cpu_select = true;
    if (lod < 0 || !b || b->vert_count == 0 || b->index_count < 3) {
        out->issue = false;
        return lod;
    }
    out->vert_count = b->vert_count;
    out->index_count = b->index_count;
    out->tri_count = b->index_count / 3;
    out->first_vertex = 0;
    out->first_index = 0;
    out->issue = true;
    return lod;
}

i32 aether_mdl_lod_gpu_issue_draw_copy(const aether_mdl_lod_table_t *table,
                                       const aether_mdl_lod_mesh_set_t *meshes,
                                       f32 distance,
                                       aether_mdl_lod_gpu_draw_t *out,
                                       f32 *out_pos, u32 max_verts,
                                       u32 *out_idx, u32 max_idx) {
    i32 lod = aether_mdl_lod_gpu_issue_draw(table, meshes, distance, out);
    if (lod < 0 || !out || !out->issue) return lod;
    const aether_mdl_lod_mesh_bucket_t *b = NULL;
    aether_mdl_lod_mesh_select(table, meshes, distance, &b);
    if (!b) { out->issue = false; return -1; }
    u32 vc = 0, tc = 0;
    aether_mdl_lod_mesh_copy(b, out_pos, max_verts, out_idx, max_idx, &vc, &tc);
    out->vert_count = vc;
    out->tri_count = tc;
    out->index_count = tc * 3;
    return lod;
}

void aether_mdl_hiz_init(aether_mdl_hiz_t *hiz) {
    if (!hiz) return;
    memset(hiz, 0, sizeof(*hiz));
    hiz->near_z = 1.f;
    hiz->far_z = 4096.f;
    hiz->enabled = true;
}
void aether_mdl_hiz_clear(aether_mdl_hiz_t *hiz) {
    if (!hiz) return;
    hiz->count = 0;
}
void aether_mdl_hiz_set_range(aether_mdl_hiz_t *hiz, f32 near_z, f32 far_z) {
    if (!hiz) return;
    hiz->near_z = near_z > 0.f ? near_z : 1.f;
    hiz->far_z = far_z > hiz->near_z ? far_z : hiz->near_z + 1.f;
}
int aether_mdl_hiz_push(aether_mdl_hiz_t *hiz, f32 depth, f32 sx, f32 sy) {
    if (!hiz || hiz->count >= AETHER_MDL_HIZ_MAX_SAMPLES) return 0;
    aether_mdl_hiz_sample_t *s = &hiz->samples[hiz->count++];
    s->depth = depth < 0.f ? 0.f : (depth > 1.f ? 1.f : depth);
    s->screen_x = sx;
    s->screen_y = sy;
    return 1;
}
f32 aether_mdl_hiz_sample(const aether_mdl_hiz_t *hiz, f32 sx, f32 sy) {
    if (!hiz || hiz->count == 0) return 1.f;
    f32 best_d = 1e9f;
    f32 best_z = 1.f;
    for (u32 i = 0; i < hiz->count; ++i) {
        f32 dx = hiz->samples[i].screen_x - sx;
        f32 dy = hiz->samples[i].screen_y - sy;
        f32 d2 = dx * dx + dy * dy;
        if (d2 < best_d) { best_d = d2; best_z = hiz->samples[i].depth; }
    }
    return best_z;
}

i32 aether_mdl_lod_hiz_gate(const aether_mdl_lod_table_t *table,
                            const aether_mdl_lod_mesh_set_t *meshes,
                            const aether_mdl_hiz_t *hiz,
                            f32 distance, f32 aabb_radius, f32 fov_y_deg,
                            f32 min_pixels, f32 max_distance,
                            f32 screen_x, f32 screen_y, f32 depth_ndc,
                            aether_mdl_hiz_gate_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!table || !meshes) return -1;
    if (min_pixels <= 0.f) min_pixels = 4.f;
    if (fov_y_deg <= 0.f) fov_y_deg = 75.f;
    if (aabb_radius <= 0.f) aabb_radius = 16.f;
    f32 dist = distance < 1.f ? 1.f : distance;
    /* Projected screen pixels ≈ radius / (dist * tan(fov/2)) * 1080 */
    f32 half = fov_y_deg * 0.5f * 0.01745329252f;
    f32 tan_h = tanf(half);
    if (tan_h < 1e-4f) tan_h = 1e-4f;
    f32 screen_px = (aabb_radius / (dist * tan_h)) * 1080.f;
    i32 lod = aether_mdl_lod_select(table, dist);
    bool culled = false;
    bool occ = false;
    if (max_distance > 0.f && dist > max_distance) culled = true;
    if (screen_px < min_pixels) culled = true;
    /* Hi-Z: if stored depth is significantly nearer than object, occluded. */
    if (hiz && hiz->enabled && hiz->count > 0) {
        f32 hz = aether_mdl_hiz_sample(hiz, screen_x, screen_y);
        f32 obj_z = depth_ndc;
        if (obj_z <= 0.f) {
            /* Approximate NDC depth from distance */
            f32 nz = hiz->near_z, fz = hiz->far_z;
            obj_z = ((dist - nz) / (fz - nz));
            if (obj_z < 0.f) obj_z = 0.f;
            if (obj_z > 1.f) obj_z = 1.f;
        }
        if (hz + 0.02f < obj_z) occ = true; /* something nearer covers us */
    }
    /* If barely passing pixel gate, bump LOD toward lower detail. */
    if (!culled && !occ && screen_px < min_pixels * 3.f && lod >= 0) {
        i32 bump = lod + 1;
        if (table && bump < (i32)table->count) lod = bump;
        else if (meshes && bump < (i32)meshes->count) lod = bump;
    }
    if (out) {
        out->occluded = occ;
        out->distance_culled = culled;
        out->issue = !culled && !occ && lod >= 0;
        out->lod = lod;
        out->screen_pixels = screen_px;
        out->min_pixels = min_pixels;
        out->distance = dist;
    }
    if (culled || occ) return -1;
    return lod;
}

i32 aether_mdl_lod_gpu_issue_draw_hiz(const aether_mdl_lod_table_t *table,
                                      const aether_mdl_lod_mesh_set_t *meshes,
                                      const aether_mdl_hiz_t *hiz,
                                      f32 distance, f32 aabb_radius,
                                      aether_mdl_lod_gpu_draw_t *out,
                                      aether_mdl_hiz_gate_t *gate) {
    aether_mdl_hiz_gate_t g;
    i32 lod = aether_mdl_lod_hiz_gate(table, meshes, hiz, distance, aabb_radius,
                                      75.f, 4.f, 0.f, 0.5f, 0.5f, 0.f, &g);
    if (gate) *gate = g;
    if (out) memset(out, 0, sizeof(*out));
    if (lod < 0 || !g.issue) {
        if (out) { out->issue = false; out->distance = distance; out->lod = -1; out->cpu_select = true; }
        return -1;
    }
    /* Force distance that selects the gated LOD (use table distances mid). */
    f32 use_dist = distance;
    if (table && lod < (i32)table->count) {
        /* Prefer a distance inside the selected LOD band. */
        use_dist = table->levels[lod].max_distance * 0.5f;
        if (use_dist < 1.f) use_dist = distance;
    }
    i32 issued = aether_mdl_lod_gpu_issue_draw(table, meshes, use_dist, out);
    if (out && issued >= 0) {
        out->lod = lod;
        out->distance = distance;
    }
    return issued >= 0 ? lod : -1;
}

/* ---------- Hierarchical Hi-Z mip pyramid ---------- */
void aether_mdl_hiz_pyramid_init(aether_mdl_hiz_pyramid_t *pyr) {
    if (!pyr) return;
    memset(pyr, 0, sizeof(*pyr));
    aether_mdl_hiz_pyramid_reset(pyr, AETHER_MDL_HIZ_MIP0_W, AETHER_MDL_HIZ_MIP0_H);
}

void aether_mdl_hiz_pyramid_reset(aether_mdl_hiz_pyramid_t *pyr, u32 mip0_w, u32 mip0_h) {
    if (!pyr) return;
    if (mip0_w < 2) mip0_w = 2;
    if (mip0_h < 2) mip0_h = 2;
    if (mip0_w > AETHER_MDL_HIZ_MIP0_W) mip0_w = AETHER_MDL_HIZ_MIP0_W;
    if (mip0_h > AETHER_MDL_HIZ_MIP0_H) mip0_h = AETHER_MDL_HIZ_MIP0_H;
    memset(pyr->depth, 0, sizeof pyr->depth);
    /* Init to far (1.0) so empty = fully visible (nothing nearer). */
    for (u32 i = 0; i < AETHER_MDL_HIZ_PYRAMID_TEXELS; ++i) pyr->depth[i] = 1.f;
    pyr->mip0_w = mip0_w;
    pyr->mip0_h = mip0_h;
    pyr->levels = 0;
    pyr->built = false;
    pyr->vis_queries = 0;
    pyr->vis_occluded = 0;
    u32 off = 0, w = mip0_w, h = mip0_h;
    for (u32 l = 0; l < AETHER_MDL_HIZ_MIP_LEVELS; ++l) {
        pyr->level_offset[l] = off;
        pyr->level_w[l] = w;
        pyr->level_h[l] = h;
        off += w * h;
        if (off > AETHER_MDL_HIZ_PYRAMID_TEXELS) {
            /* Clamp levels to fit buffer. */
            pyr->levels = l;
            return;
        }
        if (w <= 1 && h <= 1) { pyr->levels = l + 1; return; }
        w = w > 1 ? (w + 1) / 2 : 1;
        h = h > 1 ? (h + 1) / 2 : 1;
    }
    pyr->levels = AETHER_MDL_HIZ_MIP_LEVELS;
}

int aether_mdl_hiz_pyramid_write(aether_mdl_hiz_pyramid_t *pyr, u32 x, u32 y, f32 depth) {
    if (!pyr || x >= pyr->mip0_w || y >= pyr->mip0_h) return 0;
    f32 d = depth < 0.f ? 0.f : (depth > 1.f ? 1.f : depth);
    pyr->depth[pyr->level_offset[0] + y * pyr->mip0_w + x] = d;
    pyr->built = false;
    return 1;
}

u32 aether_mdl_hiz_pyramid_fill_mip0(aether_mdl_hiz_pyramid_t *pyr,
                                     const f32 *depth_mip0, u32 count) {
    if (!pyr || !depth_mip0) return 0;
    u32 n = pyr->mip0_w * pyr->mip0_h;
    if (count < n) n = count;
    for (u32 i = 0; i < n; ++i) {
        f32 d = depth_mip0[i];
        pyr->depth[pyr->level_offset[0] + i] = d < 0.f ? 0.f : (d > 1.f ? 1.f : d);
    }
    pyr->built = false;
    return n;
}

u32 aether_mdl_hiz_build_pyramid(aether_mdl_hiz_pyramid_t *pyr) {
    if (!pyr || pyr->levels < 1) return 0;
    /* Ensure level layout if reset was partial. */
    if (pyr->level_w[0] == 0) aether_mdl_hiz_pyramid_reset(pyr, pyr->mip0_w, pyr->mip0_h);
    for (u32 l = 1; l < pyr->levels; ++l) {
        u32 pw = pyr->level_w[l - 1], ph = pyr->level_h[l - 1];
        u32 cw = pyr->level_w[l], ch = pyr->level_h[l];
        const f32 *src = &pyr->depth[pyr->level_offset[l - 1]];
        f32 *dst = &pyr->depth[pyr->level_offset[l]];
        for (u32 y = 0; y < ch; ++y) {
            for (u32 x = 0; x < cw; ++x) {
                /* Max of 2x2 (conservative: farthest occlusion plane). */
                u32 x0 = x * 2, y0 = y * 2;
                f32 m = src[y0 * pw + x0];
                if (x0 + 1 < pw) { f32 v = src[y0 * pw + x0 + 1]; if (v < m) m = v; }
                if (y0 + 1 < ph) {
                    f32 v = src[(y0 + 1) * pw + x0]; if (v < m) m = v;
                    if (x0 + 1 < pw) { f32 v2 = src[(y0 + 1) * pw + x0 + 1]; if (v2 < m) m = v2; }
                }
                /* Use min-Z for nearer-covers: occlusion when hiz < object. */
                dst[y * cw + x] = m;
            }
        }
    }
    pyr->built = true;
    return pyr->levels;
}

f32 aether_mdl_hiz_pyramid_sample_rect(const aether_mdl_hiz_pyramid_t *pyr,
                                       f32 x0, f32 y0, f32 x1, f32 y1, i32 *out_mip) {
    if (out_mip) *out_mip = -1;
    if (!pyr || !pyr->built || pyr->levels == 0) return 1.f;
    if (x0 > x1) { f32 t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { f32 t = y0; y0 = y1; y1 = t; }
    if (x0 < 0.f) x0 = 0.f; if (y0 < 0.f) y0 = 0.f;
    if (x1 > 1.f) x1 = 1.f; if (y1 > 1.f) y1 = 1.f;
    f32 rw = x1 - x0, rh = y1 - y0;
    if (rw < 1e-4f) rw = 1e-4f;
    if (rh < 1e-4f) rh = 1e-4f;
    /* Pick mip where one texel ≈ rect size. */
    f32 px = rw * (f32)pyr->mip0_w;
    f32 py = rh * (f32)pyr->mip0_h;
    f32 pmax = px > py ? px : py;
    i32 mip = 0;
    while (mip + 1 < (i32)pyr->levels && pmax > 2.f) {
        pmax *= 0.5f;
        mip++;
    }
    u32 lw = pyr->level_w[mip], lh = pyr->level_h[mip];
    if (lw == 0 || lh == 0) return 1.f;
    i32 ix0 = (i32)(x0 * (f32)lw); if (ix0 < 0) ix0 = 0;
    i32 iy0 = (i32)(y0 * (f32)lh); if (iy0 < 0) iy0 = 0;
    i32 ix1 = (i32)(x1 * (f32)lw); if (ix1 >= (i32)lw) ix1 = (i32)lw - 1;
    i32 iy1 = (i32)(y1 * (f32)lh); if (iy1 >= (i32)lh) iy1 = (i32)lh - 1;
    if (ix1 < ix0) ix1 = ix0;
    if (iy1 < iy0) iy1 = iy0;
    const f32 *src = &pyr->depth[pyr->level_offset[mip]];
    f32 nearest = 1.f;
    for (i32 y = iy0; y <= iy1; ++y) {
        for (i32 x = ix0; x <= ix1; ++x) {
            f32 v = src[(u32)y * lw + (u32)x];
            if (v < nearest) nearest = v;
        }
    }
    if (out_mip) *out_mip = mip;
    return nearest;
}

int aether_mdl_hiz_vis_query(const aether_mdl_hiz_pyramid_t *pyr,
                             f32 x0, f32 y0, f32 x1, f32 y1, f32 object_depth,
                             aether_mdl_hiz_vis_query_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!pyr || !pyr->built) {
        if (out) { out->visible = true; out->valid = false; }
        return 0;
    }
    i32 mip = -1;
    f32 hz = aether_mdl_hiz_pyramid_sample_rect(pyr, x0, y0, x1, y1, &mip);
    f32 od = object_depth < 0.f ? 0.f : (object_depth > 1.f ? 1.f : object_depth);
    bool occ = (hz + 0.01f < od);
    if (out) {
        out->screen_x0 = x0; out->screen_y0 = y0;
        out->screen_x1 = x1; out->screen_y1 = y1;
        out->object_depth = od;
        out->nearest_hiz = hz;
        out->mip_used = mip;
        out->occluded = occ;
        out->visible = !occ;
        out->valid = true;
    }
    /* Mutable stats via cast — queries counted on non-const wrapper in bridge. */
    return occ ? 0 : 1;
}

void aether_mdl_hiz_pyramid_set_gpu_hooks(aether_mdl_hiz_pyramid_t *pyr, bool armed) {
    if (!pyr) return;
    pyr->gpu_hooks = armed;
}
bool aether_mdl_hiz_pyramid_gpu_hooks(const aether_mdl_hiz_pyramid_t *pyr) {
    return pyr && pyr->gpu_hooks;
}

i32 aether_mdl_lod_hiz_pyramid_gate(const aether_mdl_lod_table_t *table,
                                    const aether_mdl_lod_mesh_set_t *meshes,
                                    const aether_mdl_hiz_pyramid_t *pyr,
                                    f32 distance, f32 aabb_radius, f32 fov_y_deg,
                                    f32 min_pixels, f32 max_distance,
                                    f32 sx, f32 sy, f32 depth_ndc,
                                    aether_mdl_hiz_gate_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!table || !meshes) return -1;
    if (min_pixels <= 0.f) min_pixels = 4.f;
    if (fov_y_deg <= 0.f) fov_y_deg = 75.f;
    if (aabb_radius <= 0.f) aabb_radius = 16.f;
    f32 dist = distance < 1.f ? 1.f : distance;
    f32 half = fov_y_deg * 0.5f * 0.01745329252f;
    f32 tan_h = tanf(half); if (tan_h < 1e-4f) tan_h = 1e-4f;
    f32 screen_px = (aabb_radius / (dist * tan_h)) * 1080.f;
    i32 lod = aether_mdl_lod_select(table, dist);
    bool culled = false, occ = false;
    if (max_distance > 0.f && dist > max_distance) culled = true;
    if (screen_px < min_pixels) culled = true;
    f32 half_uv = (aabb_radius / (dist * tan_h)) * 0.5f;
    if (half_uv < 0.01f) half_uv = 0.01f;
    if (half_uv > 0.4f) half_uv = 0.4f;
    f32 od = depth_ndc;
    if (od <= 0.f && pyr) {
        /* Approximate from distance if caller omitted. */
        od = dist / 4096.f; if (od > 1.f) od = 1.f;
    }
    if (pyr && pyr->built && !culled) {
        aether_mdl_hiz_vis_query_t q;
        aether_mdl_hiz_vis_query(pyr, sx - half_uv, sy - half_uv,
                                 sx + half_uv, sy + half_uv, od, &q);
        if (q.valid && q.occluded) occ = true;
    }
    if (!culled && !occ && screen_px < min_pixels * 3.f && lod >= 0) {
        i32 bump = lod + 1;
        if (bump < (i32)table->count) lod = bump;
    }
    if (out) {
        out->occluded = occ;
        out->distance_culled = culled;
        out->issue = !culled && !occ && lod >= 0;
        out->lod = lod;
        out->screen_pixels = screen_px;
        out->min_pixels = min_pixels;
        out->distance = dist;
    }
    if (culled || occ) return -1;
    return lod;
}


/* ---------- Depth → Hi-Z bind + mip-explicit / multi-mip vis ---------- */
u32 aether_mdl_hiz_bind_from_depth(aether_mdl_hiz_pyramid_t *pyr,
                                   const f32 *depth_samples, u32 count,
                                   u32 mip0_w, u32 mip0_h,
                                   aether_mdl_hiz_bind_result_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!pyr) return 0;
    aether_mdl_hiz_pyramid_reset(pyr, mip0_w, mip0_h);
    u32 filled = 0;
    if (depth_samples && count > 0)
        filled = aether_mdl_hiz_pyramid_fill_mip0(pyr, depth_samples, count);
    else {
        /* Empty depth → far plane (fully visible). */
        filled = pyr->mip0_w * pyr->mip0_h;
    }
    u32 levels = aether_mdl_hiz_build_pyramid(pyr);
    aether_mdl_hiz_pyramid_set_gpu_hooks(pyr, true);
    if (out) {
        out->filled = (filled > 0);
        out->built = pyr->built;
        out->mip0_filled = filled;
        out->levels = levels;
        out->view_count = levels;
        out->views_ready = (levels > 0);
    }
    return levels;
}

u32 aether_mdl_hiz_pyramid_texture_views(const aether_mdl_hiz_pyramid_t *pyr,
                                         u32 *out_w, u32 *out_h, u32 *out_off,
                                         u32 max_levels) {
    if (!pyr || !pyr->built || max_levels == 0) return 0;
    u32 n = pyr->levels < max_levels ? pyr->levels : max_levels;
    for (u32 i = 0; i < n; ++i) {
        if (out_w) out_w[i] = pyr->level_w[i];
        if (out_h) out_h[i] = pyr->level_h[i];
        if (out_off) out_off[i] = pyr->level_offset[i];
    }
    return n;
}

int aether_mdl_hiz_vis_query_at_mip(const aether_mdl_hiz_pyramid_t *pyr,
                                    f32 x0, f32 y0, f32 x1, f32 y1,
                                    f32 object_depth, i32 mip,
                                    aether_mdl_hiz_vis_query_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!pyr || !pyr->built || mip < 0 || (u32)mip >= pyr->levels) {
        if (out) { out->visible = true; out->valid = false; }
        return 0;
    }
    if (x0 > x1) { f32 t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { f32 t = y0; y0 = y1; y1 = t; }
    if (x0 < 0.f) x0 = 0.f; if (y0 < 0.f) y0 = 0.f;
    if (x1 > 1.f) x1 = 1.f; if (y1 > 1.f) y1 = 1.f;
    u32 lw = pyr->level_w[mip], lh = pyr->level_h[mip];
    if (lw == 0 || lh == 0) return 0;
    i32 ix0 = (i32)(x0 * (f32)lw); if (ix0 < 0) ix0 = 0;
    i32 iy0 = (i32)(y0 * (f32)lh); if (iy0 < 0) iy0 = 0;
    i32 ix1 = (i32)(x1 * (f32)lw); if (ix1 >= (i32)lw) ix1 = (i32)lw - 1;
    i32 iy1 = (i32)(y1 * (f32)lh); if (iy1 >= (i32)lh) iy1 = (i32)lh - 1;
    if (ix1 < ix0) ix1 = ix0;
    if (iy1 < iy0) iy1 = iy0;
    const f32 *src = &pyr->depth[pyr->level_offset[mip]];
    f32 nearest = 1.f;
    for (i32 y = iy0; y <= iy1; ++y)
        for (i32 x = ix0; x <= ix1; ++x) {
            f32 v = src[(u32)y * lw + (u32)x];
            if (v < nearest) nearest = v;
        }
    f32 od = object_depth < 0.f ? 0.f : (object_depth > 1.f ? 1.f : object_depth);
    bool occ = (nearest + 0.01f < od);
    if (out) {
        out->screen_x0 = x0; out->screen_y0 = y0;
        out->screen_x1 = x1; out->screen_y1 = y1;
        out->object_depth = od;
        out->nearest_hiz = nearest;
        out->mip_used = mip;
        out->occluded = occ;
        out->visible = !occ;
        out->valid = true;
    }
    return occ ? 0 : 1;
}

int aether_mdl_hiz_vis_query_multi_mip(const aether_mdl_hiz_pyramid_t *pyr,
                                       f32 x0, f32 y0, f32 x1, f32 y1,
                                       f32 object_depth,
                                       aether_mdl_hiz_vis_query_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!pyr || !pyr->built) {
        if (out) { out->visible = true; out->valid = false; }
        return 0;
    }
    bool any_occ = false;
    f32 nearest = 1.f;
    i32 used = 0;
    u32 max_check = pyr->levels < 4 ? pyr->levels : 4;
    for (u32 m = 0; m < max_check; ++m) {
        aether_mdl_hiz_vis_query_t q;
        aether_mdl_hiz_vis_query_at_mip(pyr, x0, y0, x1, y1, object_depth, (i32)m, &q);
        if (!q.valid) continue;
        if (q.nearest_hiz < nearest) { nearest = q.nearest_hiz; used = (i32)m; }
        if (q.occluded) any_occ = true;
    }
    f32 od = object_depth < 0.f ? 0.f : (object_depth > 1.f ? 1.f : object_depth);
    if (out) {
        out->screen_x0 = x0; out->screen_y0 = y0;
        out->screen_x1 = x1; out->screen_y1 = y1;
        out->object_depth = od;
        out->nearest_hiz = nearest;
        out->mip_used = used;
        out->occluded = any_occ;
        out->visible = !any_occ;
        out->valid = true;
    }
    return any_occ ? 0 : 1;
}

/* ---------- Fixture skin pages ---------- */
void aether_mdl_skin_pages_init(aether_mdl_skin_page_set_t *set) {
    if (!set) return;
    memset(set, 0, sizeof(*set));
}

static void skin_page_fill(aether_mdl_skin_page_t *page, u8 group, u8 tex) {
    memset(page, 0, sizeof(*page));
    page->width = AETHER_MDL_SKIN_PAGE_W;
    page->height = AETHER_MDL_SKIN_PAGE_H;
    page->group = group;
    page->tex = tex;
    page->valid = true;
    for (u32 y = 0; y < page->height; ++y) {
        for (u32 x = 0; x < page->width; ++x) {
            u32 i = (y * page->width + x) * 4u;
            int on = ((x / 2) ^ (y / 2)) & 1;
            /* Distinct tint per group/tex (clean-room, not HL skins). */
            u8 r = (u8)(40 + group * 50 + (on ? 80 : 0) + tex * 10);
            u8 g = (u8)(50 + tex * 45 + (on ? 60 : 20) + group * 8);
            u8 b = (u8)(70 + ((group + tex) & 3) * 40 + (on ? 30 : 90));
            page->rgba[i+0] = r;
            page->rgba[i+1] = g;
            page->rgba[i+2] = b;
            page->rgba[i+3] = 255;
        }
    }
}

u32 aether_mdl_skin_pages_build_fixture(aether_mdl_skin_page_set_t *set, u32 page_count) {
    if (!set) return 0;
    aether_mdl_skin_pages_init(set);
    if (page_count == 0) page_count = 2;
    if (page_count > AETHER_MDL_SKIN_PAGE_MAX) page_count = AETHER_MDL_SKIN_PAGE_MAX;
    for (u32 i = 0; i < page_count; ++i) {
        u8 g = (u8)(i / 2);
        u8 t = (u8)(i % 2);
        skin_page_fill(&set->pages[i], g, t);
    }
    set->count = page_count;
    return page_count;
}

i32 aether_mdl_skin_pages_find(const aether_mdl_skin_page_set_t *set, u8 group, u8 tex) {
    if (!set) return -1;
    for (u32 i = 0; i < set->count; ++i)
        if (set->pages[i].valid && set->pages[i].group == group && set->pages[i].tex == tex)
            return (i32)i;
    return -1;
}

int aether_mdl_skin_page_sample(const aether_mdl_skin_page_t *page,
                                f32 u, f32 v, f32 out_rgba[4]) {
    if (out_rgba) { out_rgba[0]=out_rgba[1]=out_rgba[2]=0.f; out_rgba[3]=1.f; }
    if (!page || !page->valid || !out_rgba) return 0;
    f32 uu = u - floorf(u);
    f32 vv = v - floorf(v);
    if (uu < 0.f) uu += 1.f;
    if (vv < 0.f) vv += 1.f;
    u32 x = (u32)(uu * (f32)page->width); if (x >= page->width) x = page->width - 1;
    u32 y = (u32)(vv * (f32)page->height); if (y >= page->height) y = page->height - 1;
    const u8 *p = &page->rgba[(y * page->width + x) * 4u];
    out_rgba[0] = p[0] / 255.f;
    out_rgba[1] = p[1] / 255.f;
    out_rgba[2] = p[2] / 255.f;
    out_rgba[3] = p[3] / 255.f;
    return 1;
}

int aether_mdl_skin_pages_sample(const aether_mdl_skin_page_set_t *set,
                                 u8 group, u8 tex, f32 u, f32 v, f32 out_rgba[4]) {
    i32 idx = aether_mdl_skin_pages_find(set, group, tex);
    if (idx < 0) {
        /* Fallback: first page or procedural tint. */
        if (set && set->count > 0)
            return aether_mdl_skin_page_sample(&set->pages[0], u, v, out_rgba);
        if (out_rgba) {
            out_rgba[0] = 0.4f + group * 0.1f;
            out_rgba[1] = 0.3f + tex * 0.15f;
            out_rgba[2] = 0.5f;
            out_rgba[3] = 1.f;
        }
        return 0;
    }
    return aether_mdl_skin_page_sample(&set->pages[idx], u, v, out_rgba);
}


/* ===== texture2d_array Hi-Z + packed skin lumps (batch hiz-array/portal-graph/skin-ipa) ===== */

void aether_mdl_hiz_array_init(aether_mdl_hiz_array_t *arr) {
    if (!arr) return;
    memset(arr, 0, sizeof(*arr));
}

u32 aether_mdl_hiz_bind_texture2d_array(const aether_mdl_hiz_pyramid_t *pyr,
                                        aether_mdl_hiz_array_t *out) {
    if (!out) return 0;
    aether_mdl_hiz_array_init(out);
    if (!pyr || !pyr->built || pyr->levels == 0) return 0;
    u32 n = pyr->levels;
    if (n > AETHER_MDL_HIZ_ARRAY_MAX_SLICES) n = AETHER_MDL_HIZ_ARRAY_MAX_SLICES;
    out->mip0_w = pyr->mip0_w;
    out->mip0_h = pyr->mip0_h;
    for (u32 i = 0; i < n; ++i) {
        out->slices[i].slice = i;
        out->slices[i].width = pyr->level_w[i];
        out->slices[i].height = pyr->level_h[i];
        out->slices[i].texel_offset = pyr->level_offset[i];
        out->slices[i].valid = (pyr->level_w[i] > 0 && pyr->level_h[i] > 0);
    }
    out->slice_count = n;
    out->gpu_array = true;
    return n;
}

void aether_mdl_hiz_array_mark_bound(aether_mdl_hiz_array_t *arr) {
    if (arr) arr->bound = (arr->slice_count > 0);
}

bool aether_mdl_hiz_array_was_bound(const aether_mdl_hiz_array_t *arr) {
    return arr && arr->bound && arr->slice_count > 0;
}

void aether_mdl_hiz_array_set_gpu(aether_mdl_hiz_array_t *arr, bool armed) {
    if (arr) arr->gpu_array = armed;
}

bool aether_mdl_hiz_array_gpu(const aether_mdl_hiz_array_t *arr) {
    return arr && arr->gpu_array;
}

int aether_mdl_hiz_vis_query_array_mip(const aether_mdl_hiz_pyramid_t *pyr,
                                       const aether_mdl_hiz_array_t *arr,
                                       f32 x0, f32 y0, f32 x1, f32 y1,
                                       f32 object_depth, i32 array_mip,
                                       aether_mdl_hiz_vis_query_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!pyr || !pyr->built) {
        if (out) { out->visible = true; out->valid = false; }
        return 0;
    }
    i32 mip = array_mip;
    if (arr && arr->slice_count > 0) {
        if (mip < 0) mip = 0;
        if ((u32)mip >= arr->slice_count) mip = (i32)arr->slice_count - 1;
        if (!arr->slices[mip].valid) {
            /* fall back to auto mip query */
            return aether_mdl_hiz_vis_query_at_mip(pyr, x0, y0, x1, y1, object_depth, mip, out);
        }
    }
    return aether_mdl_hiz_vis_query_at_mip(pyr, x0, y0, x1, y1, object_depth, mip, out);
}

void aether_mdl_skin_lumps_init(aether_mdl_skin_lump_set_t *set) {
    if (!set) return;
    memset(set, 0, sizeof(*set));
}

static int skin_lump_rd_i32_le(const u8 *p) {
    return (int)((u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24));
}

u32 aether_mdl_skin_lumps_load(aether_mdl_skin_lump_set_t *set,
                               const u8 *mdl_bytes, u32 size) {
    if (!set) return 0;
    aether_mdl_skin_lumps_init(set);
    if (!mdl_bytes || size < 32) return 0;

    /* Scan for clean-room texture trailer magic 0xAE7E0001 (textured fixture). */
    for (u32 i = 0; i + 16 <= size; ++i) {
        if (skin_lump_rd_i32_le(mdl_bytes + i) != (int)AETHER_MDL_SKIN_LUMP_MAGIC) continue;
        if (i + 16 > size) break;
        u32 tw = (u32)skin_lump_rd_i32_le(mdl_bytes + i + 4);
        u32 th = (u32)skin_lump_rd_i32_le(mdl_bytes + i + 8);
        u32 tex_off = (u32)skin_lump_rd_i32_le(mdl_bytes + i + 12);
        if (tw == 0 || th == 0 || tw > AETHER_MDL_SKIN_LUMP_MAX_W || th > AETHER_MDL_SKIN_LUMP_MAX_H)
            continue;
        u32 need = tw * th * 4u;
        if (tex_off >= size || tex_off + need > size) continue;
        aether_mdl_skin_lump_t *L = &set->lumps[0];
        memset(L, 0, sizeof(*L));
        snprintf(L->name, sizeof L->name, "packed_skin0");
        L->width = tw;
        L->height = th;
        L->rgba_bytes = need;
        memcpy(L->rgba, mdl_bytes + tex_off, need);
        L->from_asset = true;
        L->valid = true;
        set->count = 1;
        return 1;
    }
    return 0;
}

u32 aether_mdl_skin_lumps_load_file(aether_mdl_skin_lump_set_t *set, const char *path) {
    if (!set || !path) return 0;
    aether_mdl_skin_lumps_init(set);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    if (sz <= 0 || sz > 8 * 1024 * 1024) { fclose(f); return 0; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    u8 *buf = (u8 *)malloc((size_t)sz);
    if (!buf) { fclose(f); return 0; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    u32 c = 0;
    if (n == (size_t)sz) c = aether_mdl_skin_lumps_load(set, buf, (u32)n);
    free(buf);
    return c;
}

u32 aether_mdl_skin_lumps_load_or_fixture(aether_mdl_skin_lump_set_t *set,
                                          const u8 *mdl_bytes, u32 size,
                                          u32 fixture_pages) {
    if (!set) return 0;
    u32 n = aether_mdl_skin_lumps_load(set, mdl_bytes, size);
    if (n > 0) return n;

    /* Fixture fallback: promote skin pages → lumps. */
    aether_mdl_skin_page_set_t pages;
    u32 pc = aether_mdl_skin_pages_build_fixture(&pages, fixture_pages ? fixture_pages : 2);
    aether_mdl_skin_lumps_init(set);
    set->used_fixture_fallback = true;
    u32 out = 0;
    for (u32 i = 0; i < pc && out < AETHER_MDL_SKIN_LUMP_MAX; ++i) {
        const aether_mdl_skin_page_t *pg = &pages.pages[i];
        if (!pg->valid) continue;
        aether_mdl_skin_lump_t *L = &set->lumps[out];
        memset(L, 0, sizeof(*L));
        snprintf(L->name, sizeof L->name, "fixture_g%u_t%u", (unsigned)pg->group, (unsigned)pg->tex);
        L->width = pg->width;
        L->height = pg->height;
        L->rgba_bytes = pg->width * pg->height * 4u;
        if (L->rgba_bytes > AETHER_MDL_SKIN_LUMP_MAX_RGBA)
            L->rgba_bytes = AETHER_MDL_SKIN_LUMP_MAX_RGBA;
        memcpy(L->rgba, pg->rgba, L->rgba_bytes);
        L->from_asset = false;
        L->valid = true;
        ++out;
    }
    set->count = out;
    return out;
}

int aether_mdl_skin_lump_sample(const aether_mdl_skin_lump_t *lump,
                                f32 u, f32 v, f32 out_rgba[4]) {
    if (out_rgba) { out_rgba[0]=out_rgba[1]=out_rgba[2]=0.f; out_rgba[3]=1.f; }
    if (!lump || !lump->valid || !out_rgba || lump->width == 0 || lump->height == 0) return 0;
    f32 uu = u - floorf(u); if (uu < 0.f) uu += 1.f;
    f32 vv = v - floorf(v); if (vv < 0.f) vv += 1.f;
    u32 x = (u32)(uu * (f32)lump->width); if (x >= lump->width) x = lump->width - 1;
    u32 y = (u32)(vv * (f32)lump->height); if (y >= lump->height) y = lump->height - 1;
    const u8 *p = &lump->rgba[(y * lump->width + x) * 4u];
    out_rgba[0] = p[0] / 255.f;
    out_rgba[1] = p[1] / 255.f;
    out_rgba[2] = p[2] / 255.f;
    out_rgba[3] = p[3] / 255.f;
    return 1;
}

int aether_mdl_skin_lumps_sample(const aether_mdl_skin_lump_set_t *set,
                                 u32 index, f32 u, f32 v, f32 out_rgba[4]) {
    if (!set || index >= set->count) {
        if (out_rgba) { out_rgba[0]=0.5f; out_rgba[1]=0.5f; out_rgba[2]=0.5f; out_rgba[3]=1.f; }
        return 0;
    }
    return aether_mdl_skin_lump_sample(&set->lumps[index], u, v, out_rgba);
}

int aether_mdl_skin_lump_to_page(const aether_mdl_skin_lump_t *lump,
                                 aether_mdl_skin_page_t *out_page) {
    if (!out_page) return 0;
    memset(out_page, 0, sizeof(*out_page));
    if (!lump || !lump->valid) return 0;
    out_page->width = AETHER_MDL_SKIN_PAGE_W;
    out_page->height = AETHER_MDL_SKIN_PAGE_H;
    out_page->group = 0;
    out_page->tex = 0;
    out_page->valid = true;
    for (u32 y = 0; y < out_page->height; ++y) {
        for (u32 x = 0; x < out_page->width; ++x) {
            u32 sx = (x * lump->width) / out_page->width;
            u32 sy = (y * lump->height) / out_page->height;
            if (sx >= lump->width) sx = lump->width - 1;
            if (sy >= lump->height) sy = lump->height - 1;
            const u8 *s = &lump->rgba[(sy * lump->width + sx) * 4u];
            u8 *d = &out_page->rgba[(y * out_page->width + x) * 4u];
            d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=s[3];
        }
    }
    return 1;
}


/* ===== GPU Hi-Z array downsample + MDL skinref family select (batch17) ===== */

void aether_mdl_hiz_array_downsample_init(aether_mdl_hiz_array_downsample_t *ds) {
    if (!ds) return;
    memset(ds, 0, sizeof(*ds));
}

u32 aether_mdl_hiz_array_downsample_chain(aether_mdl_hiz_pyramid_t *pyr,
                                          aether_mdl_hiz_array_t *arr,
                                          aether_mdl_hiz_array_downsample_t *out) {
    if (out) aether_mdl_hiz_array_downsample_init(out);
    if (!pyr || !arr) return 0;

    /* Ensure mip0 exists; build remaining mips via 2x2 min (GPU downsample mirror). */
    if (!pyr->built || pyr->levels == 0) {
        if (pyr->mip0_w == 0 || pyr->mip0_h == 0) return 0;
        aether_mdl_hiz_build_pyramid(pyr);
    } else if (pyr->levels < 2) {
        aether_mdl_hiz_build_pyramid(pyr);
    }

    u32 slices = aether_mdl_hiz_bind_texture2d_array(pyr, arr);
    if (slices == 0) return 0;

    /* Simulate Metal compute/fragment passes: one pass per slice after mip0. */
    u32 passes = (slices > 0) ? (slices - 1) : 0;
    if (passes == 0 && slices == 1) passes = 1; /* still mark one encode */

    aether_mdl_hiz_array_set_gpu(arr, true);
    aether_mdl_hiz_array_mark_bound(arr);

    if (out) {
        out->slices_written = slices;
        out->mip0_w = arr->mip0_w;
        out->mip0_h = arr->mip0_h;
        out->compute_passes = passes ? passes : 1;
        out->from_mip0 = true;
        out->gpu_chain = true;
        out->ready = (slices > 0);
    }
    return slices;
}

void aether_mdl_hiz_array_downsample_set_gpu(aether_mdl_hiz_array_downsample_t *ds, bool armed) {
    if (ds) ds->gpu_chain = armed;
}

bool aether_mdl_hiz_array_downsample_gpu(const aether_mdl_hiz_array_downsample_t *ds) {
    return ds && ds->gpu_chain;
}

bool aether_mdl_hiz_array_downsample_ready(const aether_mdl_hiz_array_downsample_t *ds) {
    return ds && ds->ready && ds->slices_written > 0;
}

int aether_mdl_hiz_vis_query_downsampled(const aether_mdl_hiz_pyramid_t *pyr,
                                         const aether_mdl_hiz_array_t *arr,
                                         const aether_mdl_hiz_array_downsample_t *ds,
                                         f32 x0, f32 y0, f32 x1, f32 y1,
                                         f32 object_depth, i32 preferred_mip,
                                         aether_mdl_hiz_vis_query_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!ds || !aether_mdl_hiz_array_downsample_ready(ds)) {
        if (out) { out->visible = true; out->valid = false; }
        return 0;
    }
    if (!arr || !aether_mdl_hiz_array_was_bound(arr)) {
        if (out) { out->visible = true; out->valid = false; }
        return 0;
    }
    i32 mip = preferred_mip;
    if (mip < 0) {
        /* Auto: prefer mid slice when downsample filled several. */
        mip = (i32)(ds->slices_written > 1 ? ds->slices_written / 2 : 0);
    }
    return aether_mdl_hiz_vis_query_array_mip(pyr, arr, x0, y0, x1, y1,
                                              object_depth, mip, out);
}

void aether_mdl_skinref_init(aether_mdl_skinref_table_t *t) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
}

u32 aether_mdl_skinref_build_fixture(aether_mdl_skinref_table_t *t) {
    if (!t) return 0;
    aether_mdl_skinref_init(t);

    /* Family 0: default — refs body/face */
    aether_mdl_skinref_family_t *f0 = &t->families[0];
    snprintf(f0->name, sizeof f0->name, "default");
    f0->family_id = 0;
    f0->ref_first = 0;
    f0->ref_count = 2;
    f0->valid = true;

    aether_mdl_skinref_entry_t *e0 = &t->entries[0];
    e0->family = 0; e0->skin_index = 0; e0->group = 0; e0->tex = 0;
    snprintf(e0->name, sizeof e0->name, "body"); e0->valid = true;
    aether_mdl_skinref_entry_t *e1 = &t->entries[1];
    e1->family = 0; e1->skin_index = 1; e1->group = 0; e1->tex = 1;
    snprintf(e1->name, sizeof e1->name, "face"); e1->valid = true;

    /* Family 1: camo — refs body_camo / gear */
    aether_mdl_skinref_family_t *f1 = &t->families[1];
    snprintf(f1->name, sizeof f1->name, "camo");
    f1->family_id = 1;
    f1->ref_first = 2;
    f1->ref_count = 2;
    f1->valid = true;

    aether_mdl_skinref_entry_t *e2 = &t->entries[2];
    e2->family = 1; e2->skin_index = 2; e2->group = 1; e2->tex = 0;
    snprintf(e2->name, sizeof e2->name, "body_camo"); e2->valid = true;
    aether_mdl_skinref_entry_t *e3 = &t->entries[3];
    e3->family = 1; e3->skin_index = 3; e3->group = 1; e3->tex = 1;
    snprintf(e3->name, sizeof e3->name, "gear"); e3->valid = true;

    t->family_count = 2;
    t->entry_count = 4;
    t->selected_family = 0;
    t->selected_ref = 0;
    t->from_fixture = true;
    return t->family_count;
}

int aether_mdl_skinref_select_family(aether_mdl_skinref_table_t *t, u32 family_id) {
    if (!t || family_id >= t->family_count) return 0;
    if (!t->families[family_id].valid) return 0;
    t->selected_family = family_id;
    t->selected_ref = 0;
    return 1;
}

int aether_mdl_skinref_select_family_name(aether_mdl_skinref_table_t *t, const char *name) {
    if (!t || !name) return 0;
    for (u32 i = 0; i < t->family_count; ++i) {
        if (!t->families[i].valid) continue;
        if (strncmp(t->families[i].name, name, AETHER_MDL_SKINREF_NAME_LEN) == 0)
            return aether_mdl_skinref_select_family(t, i);
    }
    return 0;
}

int aether_mdl_skinref_select_ref(aether_mdl_skinref_table_t *t, u32 ref_in_family) {
    if (!t || t->selected_family >= t->family_count) return 0;
    aether_mdl_skinref_family_t *f = &t->families[t->selected_family];
    if (!f->valid || ref_in_family >= f->ref_count) return 0;
    t->selected_ref = ref_in_family;
    return 1;
}

i32 aether_mdl_skinref_cycle_family(aether_mdl_skinref_table_t *t, int dir) {
    if (!t || t->family_count == 0) return -1;
    i32 next = (i32)t->selected_family + (dir >= 0 ? 1 : -1);
    if (next < 0) next = (i32)t->family_count - 1;
    if (next >= (i32)t->family_count) next = 0;
    if (!aether_mdl_skinref_select_family(t, (u32)next)) return -1;
    return (i32)t->selected_family;
}

int aether_mdl_skinref_resolve(const aether_mdl_skinref_table_t *t,
                               u32 *out_family, u32 *out_ref,
                               u8 *out_group, u8 *out_tex, u16 *out_skin_index) {
    if (!t || t->family_count == 0 || t->selected_family >= t->family_count) return 0;
    const aether_mdl_skinref_family_t *f = &t->families[t->selected_family];
    if (!f->valid || t->selected_ref >= f->ref_count) return 0;
    u32 idx = (u32)f->ref_first + t->selected_ref;
    if (idx >= t->entry_count || !t->entries[idx].valid) return 0;
    const aether_mdl_skinref_entry_t *e = &t->entries[idx];
    if (out_family) *out_family = t->selected_family;
    if (out_ref) *out_ref = t->selected_ref;
    if (out_group) *out_group = e->group;
    if (out_tex) *out_tex = e->tex;
    if (out_skin_index) *out_skin_index = e->skin_index;
    return 1;
}

int aether_mdl_skinref_sample(const aether_mdl_skinref_table_t *t,
                              const aether_mdl_skin_page_set_t *pages,
                              f32 u, f32 v, f32 out_rgba[4]) {
    u8 g = 0, tx = 0;
    if (!aether_mdl_skinref_resolve(t, NULL, NULL, &g, &tx, NULL)) return 0;
    if (!pages || pages->count == 0) {
        /* Procedural tint from family/ref when pages absent. */
        if (!out_rgba) return 0;
        f32 fam = t ? (f32)t->selected_family * 0.35f : 0.f;
        f32 rf  = t ? (f32)t->selected_ref * 0.2f : 0.f;
        out_rgba[0] = 0.45f + fam; out_rgba[1] = 0.55f + rf;
        out_rgba[2] = 0.40f + 0.1f * (f32)g; out_rgba[3] = 1.f;
        return 1;
    }
    return aether_mdl_skin_pages_sample(pages, g, tx, u, v, out_rgba);
}

u32 aether_mdl_write_skinref_fixture(u8 *out, u32 cap) {
    /* Compact trailer: magic + family_count + entry_count (LE). */
    const u32 need = 16;
    if (!out || cap < need) return 0;
    i32 magic = AETHER_MDL_SKINREF_MAGIC;
    out[0] = (u8)(magic & 0xff);
    out[1] = (u8)((magic >> 8) & 0xff);
    out[2] = (u8)((magic >> 16) & 0xff);
    out[3] = (u8)((magic >> 24) & 0xff);
    out[4] = 2; out[5] = 0; out[6] = 0; out[7] = 0; /* family_count */
    out[8] = 4; out[9] = 0; out[10] = 0; out[11] = 0; /* entry_count */
    out[12] = 0; out[13] = 0; out[14] = 0; out[15] = 0; /* reserved */
    return need;
}



/* ===== Live Hi-Z encode from depth + skinref remap (batch18) ===== */

void aether_mdl_hiz_live_encode_plan_init(aether_mdl_hiz_live_encode_plan_t *plan) {
    if (!plan) return;
    memset(plan, 0, sizeof(*plan));
}

u32 aether_mdl_hiz_encode_from_depth(aether_mdl_hiz_pyramid_t *pyr,
                                     aether_mdl_hiz_array_t *arr,
                                     aether_mdl_hiz_array_downsample_t *ds,
                                     const f32 *depth_lin, u32 depth_count,
                                     u32 w, u32 h,
                                     aether_mdl_hiz_live_encode_plan_t *plan) {
    if (plan) aether_mdl_hiz_live_encode_plan_init(plan);
    if (!pyr || !arr || !ds || !depth_lin || w == 0 || h == 0) return 0;
    if (depth_count < w * h) return 0;

    aether_mdl_hiz_pyramid_reset(pyr, w, h);
    u32 written = 0;
    for (u32 y = 0; y < h; ++y) {
        for (u32 x = 0; x < w; ++x) {
            f32 z = depth_lin[y * w + x];
            if (z < 0.f) z = 0.f;
            if (z > 1.f) z = 1.f;
            aether_mdl_hiz_pyramid_write(pyr, x, y, z);
            written++;
        }
    }
    /* Build remaining mips via host 2x2 min (mirrors Metal downsample). */
    aether_mdl_hiz_build_pyramid(pyr);

    u32 slices = aether_mdl_hiz_array_downsample_chain(pyr, arr, ds);
    if (slices == 0) return 0;

    if (plan) {
        plan->fill_mip0_from_depth = true;
        plan->downsample_chain = (ds->compute_passes > 0);
        plan->from_depth_texture = true;
        plan->needed = true;
        plan->mip0_w = w;
        plan->mip0_h = h;
        plan->slices = slices;
        plan->encode_passes = 1u + (ds->compute_passes > 0 ? ds->compute_passes : 0);
        if (plan->encode_passes < 1) plan->encode_passes = 1;
        plan->depth_samples = written;
        plan->encoded = false;
    }
    return slices;
}

void aether_mdl_hiz_live_encode_mark(aether_mdl_hiz_live_encode_plan_t *plan) {
    if (!plan) return;
    plan->encoded = plan->needed && plan->fill_mip0_from_depth && plan->slices > 0;
}

bool aether_mdl_hiz_live_encode_was_encoded(const aether_mdl_hiz_live_encode_plan_t *plan) {
    return plan && plan->encoded;
}

void aether_mdl_skinref_remap_init(aether_mdl_skinref_remap_t *r) {
    if (!r) return;
    memset(r, 0, sizeof(*r));
    r->uv_scale[0] = 1.f; r->uv_scale[1] = 1.f;
    r->atlas[2] = 1.f; r->atlas[3] = 1.f;
}

int aether_mdl_skinref_remap_draw(const aether_mdl_skinref_table_t *t,
                                  u32 draw_slot,
                                  aether_mdl_skinref_remap_t *out) {
    if (!out) return 0;
    aether_mdl_skinref_remap_init(out);
    u32 fam = 0, ref = 0; u8 g = 0, tx = 0; u16 skin = 0;
    if (!aether_mdl_skinref_resolve(t, &fam, &ref, &g, &tx, &skin)) return 0;
    out->family = fam;
    out->ref = ref;
    out->group = g;
    out->tex = tx;
    out->skin_index = skin;
    out->draw_slot = draw_slot;
    /* Atlas cell from family/ref grid (2x2 families×refs for fixture). */
    f32 cell_u = 0.5f;
    f32 cell_v = 0.5f;
    out->atlas[0] = (f32)(ref & 1u) * cell_u;
    out->atlas[1] = (f32)(fam & 1u) * cell_v;
    out->atlas[2] = out->atlas[0] + cell_u;
    out->atlas[3] = out->atlas[1] + cell_v;
    out->uv_scale[0] = cell_u;
    out->uv_scale[1] = cell_v;
    out->uv_offset[0] = out->atlas[0];
    out->uv_offset[1] = out->atlas[1];
    out->valid = true;
    return 1;
}

void aether_mdl_skinref_remap_uv(const aether_mdl_skinref_remap_t *r,
                                 f32 u, f32 v, f32 out_uv[2]) {
    if (!out_uv) return;
    if (!r || !r->valid) {
        out_uv[0] = u; out_uv[1] = v;
        return;
    }
    f32 uu = u - floorf(u); if (uu < 0.f) uu += 1.f;
    f32 vv = v - floorf(v); if (vv < 0.f) vv += 1.f;
    out_uv[0] = r->uv_offset[0] + uu * r->uv_scale[0];
    out_uv[1] = r->uv_offset[1] + vv * r->uv_scale[1];
}

int aether_mdl_skinref_remap_sample(const aether_mdl_skinref_remap_t *r,
                                    const aether_mdl_skin_page_set_t *pages,
                                    f32 u, f32 v, f32 out_rgba[4]) {
    if (!r || !r->valid || !out_rgba) return 0;
    f32 uv[2];
    aether_mdl_skinref_remap_uv(r, u, v, uv);
    if (pages && pages->count > 0)
        return aether_mdl_skin_pages_sample(pages, r->group, r->tex, uv[0], uv[1], out_rgba);
    /* Procedural tint when pages absent. */
    f32 fam = (f32)r->family * 0.3f;
    f32 rf  = (f32)r->ref * 0.15f;
    out_rgba[0] = 0.40f + fam + uv[0] * 0.1f;
    out_rgba[1] = 0.50f + rf + uv[1] * 0.1f;
    out_rgba[2] = 0.45f + 0.08f * (f32)r->tex;
    out_rgba[3] = 1.f;
    return 1;
}


/* ===== MTK depth attach encode + skinref Metal bind (batch19) ===== */

u32 aether_mdl_hiz_encode_from_mtk_attach(aether_mdl_hiz_pyramid_t *pyr,
                                          aether_mdl_hiz_array_t *arr,
                                          aether_mdl_hiz_array_downsample_t *ds,
                                          const f32 *depth_lin, u32 depth_count,
                                          u32 w, u32 h,
                                          int mtk_attached, int shader_read,
                                          aether_mdl_hiz_live_encode_plan_t *plan) {
    if (!mtk_attached || !shader_read) {
        if (plan) aether_mdl_hiz_live_encode_plan_init(plan);
        return 0;
    }
    u32 slices = aether_mdl_hiz_encode_from_depth(pyr, arr, ds, depth_lin, depth_count,
                                                   w, h, plan);
    if (plan && slices > 0) {
        plan->from_depth_texture = true;
        plan->fill_mip0_from_depth = true;
        plan->needed = true;
    }
    return slices;
}

void aether_mdl_skinref_metal_bind_init(aether_mdl_skinref_metal_bind_t *b) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
}

u32 aether_mdl_skinref_metal_atlas_rgba(const aether_mdl_skin_page_set_t *pages,
                                        u8 *out_rgba, u32 cap,
                                        u32 *out_w, u32 *out_h) {
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (!pages || pages->count == 0 || !out_rgba) return 0;
    u32 pc = pages->count;
    if (pc > AETHER_MDL_SKIN_PAGE_MAX) pc = AETHER_MDL_SKIN_PAGE_MAX;
    /* Horizontal strip of pages: W = PAGE_W * pc, H = PAGE_H */
    u32 aw = AETHER_MDL_SKIN_PAGE_W * pc;
    u32 ah = AETHER_MDL_SKIN_PAGE_H;
    u32 need = aw * ah * 4u;
    if (cap < need) return 0;
    memset(out_rgba, 0, need);
    for (u32 p = 0; p < pc; ++p) {
        const aether_mdl_skin_page_t *pg = &pages->pages[p];
        for (u32 y = 0; y < AETHER_MDL_SKIN_PAGE_H; ++y) {
            for (u32 x = 0; x < AETHER_MDL_SKIN_PAGE_W; ++x) {
                u32 src = (y * AETHER_MDL_SKIN_PAGE_W + x) * 4u;
                u32 dst_x = p * AETHER_MDL_SKIN_PAGE_W + x;
                u32 dst = (y * aw + dst_x) * 4u;
                out_rgba[dst + 0] = pg->rgba[src + 0];
                out_rgba[dst + 1] = pg->rgba[src + 1];
                out_rgba[dst + 2] = pg->rgba[src + 2];
                out_rgba[dst + 3] = pg->rgba[src + 3];
            }
        }
    }
    if (out_w) *out_w = aw;
    if (out_h) *out_h = ah;
    return need;
}

int aether_mdl_skinref_metal_bind_draw(const aether_mdl_skinref_table_t *t,
                                       const aether_mdl_skin_page_set_t *pages,
                                       u32 draw_slot,
                                       aether_mdl_skinref_remap_t *remap,
                                       aether_mdl_skinref_metal_bind_t *bind) {
    if (!bind) return 0;
    aether_mdl_skinref_metal_bind_init(bind);
    aether_mdl_skinref_remap_t local;
    aether_mdl_skinref_remap_t *rm = remap ? remap : &local;
    if (!aether_mdl_skinref_remap_draw(t, draw_slot, rm) || !rm->valid) return 0;

    bind->family = rm->family;
    bind->ref = rm->ref;
    bind->group = rm->group;
    bind->tex = rm->tex;
    bind->skin_index = rm->skin_index;
    bind->draw_slot = draw_slot;
    bind->page_count = pages ? pages->count : 0;
    if (pages && pages->count > 0) {
        bind->tex_width = AETHER_MDL_SKIN_PAGE_W * pages->count;
        bind->tex_height = AETHER_MDL_SKIN_PAGE_H;
        bind->bytes_per_row = bind->tex_width * 4u;
        bind->rgba_bytes = bind->bytes_per_row * bind->tex_height;
        bind->texture_ready = true;
    } else {
        bind->tex_width = AETHER_MDL_SKIN_PAGE_W;
        bind->tex_height = AETHER_MDL_SKIN_PAGE_H;
        bind->bytes_per_row = bind->tex_width * 4u;
        bind->rgba_bytes = bind->bytes_per_row * bind->tex_height;
        bind->texture_ready = false;
    }
    bind->valid = true;
    bind->bound = false;
    return 1;
}

void aether_mdl_skinref_metal_bind_mark(aether_mdl_skinref_metal_bind_t *b) {
    if (!b) return;
    b->bound = b->valid && b->texture_ready && b->draw_slot > 0;
}

bool aether_mdl_skinref_metal_bind_was_bound(const aether_mdl_skinref_metal_bind_t *b) {
    return b && b->bound;
}

/* ===== Full GPU Hi-Z mipchain after MTK + skin-lump Metal families (batch20) ===== */

void aether_mdl_hiz_gpu_mipchain_init(aether_mdl_hiz_gpu_mipchain_t *m) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
}

u32 aether_mdl_hiz_gpu_mipchain_after_mtk(aether_mdl_hiz_pyramid_t *pyr,
                                          aether_mdl_hiz_array_t *arr,
                                          aether_mdl_hiz_array_downsample_t *ds,
                                          const f32 *depth_lin, u32 depth_count,
                                          u32 w, u32 h,
                                          int mtk_attached, int shader_read,
                                          aether_mdl_hiz_live_encode_plan_t *encode_plan,
                                          aether_mdl_hiz_gpu_mipchain_t *out) {
    if (out) aether_mdl_hiz_gpu_mipchain_init(out);
    if (!mtk_attached || !shader_read) return 0;

    aether_mdl_hiz_live_encode_plan_t local_plan;
    aether_mdl_hiz_live_encode_plan_t *plan = encode_plan ? encode_plan : &local_plan;
    u32 slices = aether_mdl_hiz_encode_from_mtk_attach(pyr, arr, ds, depth_lin, depth_count,
                                                       w, h, mtk_attached, shader_read, plan);
    if (slices == 0 || !out) return slices;

    /* Full mipchain: ensure downsample chain ran for every slice after mip0. */
    if (ds && !aether_mdl_hiz_array_downsample_ready(ds) && pyr && arr) {
        slices = aether_mdl_hiz_array_downsample_chain(pyr, arr, ds);
    }
    if (ds) aether_mdl_hiz_array_downsample_set_gpu(ds, true);

    out->from_mtk_attach = true;
    out->gpu_armed = true;
    out->mip0_w = w ? w : (plan->mip0_w ? plan->mip0_w : 64);
    out->mip0_h = h ? h : (plan->mip0_h ? plan->mip0_h : 64);
    out->slices_filled = slices;
    out->mip_levels = slices;
    out->gpu_passes = (slices > 1) ? (slices - 1) : 0;
    out->chain_complete = (slices >= 2) && ds && aether_mdl_hiz_array_downsample_ready(ds);
    out->ready = out->chain_complete && out->from_mtk_attach && out->gpu_armed;
    out->valid = out->ready;
    if (plan && slices > 0) {
        plan->downsample_chain = true;
        plan->needed = true;
        plan->from_depth_texture = true;
        plan->slices = slices;
        plan->encode_passes = 1u + out->gpu_passes;
    }
    return slices;
}

void aether_mdl_hiz_gpu_mipchain_set_gpu(aether_mdl_hiz_gpu_mipchain_t *m, bool armed) {
    if (!m) return;
    m->gpu_armed = armed;
    m->ready = m->chain_complete && m->from_mtk_attach && m->gpu_armed;
    m->valid = m->ready;
}

bool aether_mdl_hiz_gpu_mipchain_ready(const aether_mdl_hiz_gpu_mipchain_t *m) {
    return m && m->ready && m->valid;
}

int aether_mdl_hiz_vis_query_mipchain(const aether_mdl_hiz_pyramid_t *pyr,
                                      const aether_mdl_hiz_array_t *arr,
                                      const aether_mdl_hiz_gpu_mipchain_t *chain,
                                      f32 x0, f32 y0, f32 x1, f32 y1,
                                      f32 object_depth, i32 preferred_mip,
                                      aether_mdl_hiz_vis_query_t *out) {
    if (!chain || !aether_mdl_hiz_gpu_mipchain_ready(chain)) {
        if (out) memset(out, 0, sizeof(*out));
        return 0;
    }
    aether_mdl_hiz_array_downsample_t ds;
    aether_mdl_hiz_array_downsample_init(&ds);
    ds.slices_written = chain->slices_filled;
    ds.mip0_w = chain->mip0_w;
    ds.mip0_h = chain->mip0_h;
    ds.compute_passes = chain->gpu_passes;
    ds.from_mip0 = true;
    ds.gpu_chain = chain->gpu_armed;
    ds.ready = true;
    return aether_mdl_hiz_vis_query_downsampled(pyr, arr, &ds, x0, y0, x1, y1,
                                                object_depth, preferred_mip, out);
}

void aether_mdl_skin_lump_metal_bind_init(aether_mdl_skin_lump_metal_bind_t *b) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
}

static void skin_lump_fam_name(char *dst, u32 fam) {
    static const char *names[] = { "default", "camo", "urban", "desert" };
    const char *n = (fam < 4) ? names[fam] : "skin";
    size_t i = 0;
    for (; n[i] && i + 1 < AETHER_MDL_SKINREF_NAME_LEN; ++i) dst[i] = n[i];
    dst[i] = '\0';
}

u32 aether_mdl_skin_lump_metal_families_from_lumps(const aether_mdl_skin_lump_set_t *lumps,
                                                   u32 draw_slot,
                                                   aether_mdl_skin_lump_metal_bind_t *out) {
    if (!out) return 0;
    aether_mdl_skin_lump_metal_bind_init(out);
    if (!lumps || lumps->count == 0) return 0;
    u32 n = lumps->count;
    if (n > AETHER_MDL_SKIN_LUMP_METAL_MAX_FAM) n = AETHER_MDL_SKIN_LUMP_METAL_MAX_FAM;
    u32 atlas_w = 0, atlas_h = 0, atlas_bytes = 0;
    for (u32 i = 0; i < n; ++i) {
        const aether_mdl_skin_lump_t *L = &lumps->lumps[i];
        if (!L->valid) continue;
        aether_mdl_skin_lump_metal_family_t *f = &out->families[out->family_count];
        memset(f, 0, sizeof(*f));
        f->family_id = out->family_count;
        if (L->name[0]) {
            size_t j = 0;
            for (; L->name[j] && j + 1 < AETHER_MDL_SKINREF_NAME_LEN; ++j)
                f->name[j] = L->name[j];
            f->name[j] = '\0';
        } else {
            skin_lump_fam_name(f->name, f->family_id);
        }
        f->lump_index = i;
        f->tex_width = L->width ? L->width : 16;
        f->tex_height = L->height ? L->height : 16;
        f->rgba_bytes = L->rgba_bytes ? L->rgba_bytes : (f->tex_width * f->tex_height * 4u);
        f->draw_slot = draw_slot ? draw_slot : 3;
        f->from_fixture = lumps->used_fixture_fallback || !L->from_asset;
        f->texture_ready = (f->rgba_bytes > 0);
        f->valid = true;
        atlas_w += f->tex_width;
        if (f->tex_height > atlas_h) atlas_h = f->tex_height;
        atlas_bytes += f->rgba_bytes;
        out->family_count++;
    }
    if (out->family_count == 0) return 0;
    out->selected_family = 0;
    out->draw_slot = draw_slot ? draw_slot : 3;
    out->atlas_w = atlas_w;
    out->atlas_h = atlas_h ? atlas_h : 16;
    out->atlas_bytes = atlas_bytes ? atlas_bytes : (out->atlas_w * out->atlas_h * 4u);
    out->atlas_ready = true;
    out->used_fixture = lumps->used_fixture_fallback;
    out->valid = true;
    return out->family_count;
}

u32 aether_mdl_skin_lump_metal_families_fixture(u32 fixture_pages, u32 draw_slot,
                                                aether_mdl_skin_lump_set_t *out_lumps,
                                                aether_mdl_skin_lump_metal_bind_t *out) {
    aether_mdl_skin_lump_set_t local;
    aether_mdl_skin_lump_set_t *lumps = out_lumps ? out_lumps : &local;
    if (fixture_pages == 0) fixture_pages = 4;
    u32 lc = aether_mdl_skin_lumps_load_or_fixture(lumps, NULL, 0, fixture_pages);
    if (lc == 0) return 0;
    /* Name lumps as families for Metal bind path. */
    for (u32 i = 0; i < lumps->count && i < AETHER_MDL_SKIN_LUMP_METAL_MAX_FAM; ++i) {
        skin_lump_fam_name(lumps->lumps[i].name, i);
        lumps->lumps[i].valid = true;
    }
    return aether_mdl_skin_lump_metal_families_from_lumps(lumps, draw_slot, out);
}

int aether_mdl_skin_lump_metal_select_family(aether_mdl_skin_lump_metal_bind_t *b,
                                             u32 family_id) {
    if (!b || !b->valid || family_id >= b->family_count) return 0;
    if (!b->families[family_id].valid) return 0;
    b->selected_family = family_id;
    return 1;
}

int aether_mdl_skin_lump_metal_select_family_name(aether_mdl_skin_lump_metal_bind_t *b,
                                                  const char *name) {
    if (!b || !b->valid || !name) return 0;
    for (u32 i = 0; i < b->family_count; ++i) {
        if (!b->families[i].valid) continue;
        const char *a = b->families[i].name;
        const char *c = name;
        int eq = 1;
        while (*a || *c) {
            if (*a != *c) { eq = 0; break; }
            if (*a) ++a;
            if (*c) ++c;
        }
        if (eq) {
            b->selected_family = i;
            return 1;
        }
    }
    return 0;
}

u32 aether_mdl_skin_lump_metal_atlas_rgba(const aether_mdl_skin_lump_set_t *lumps,
                                          const aether_mdl_skin_lump_metal_bind_t *b,
                                          u8 *out_rgba, u32 cap,
                                          u32 *out_w, u32 *out_h) {
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (!lumps || !b || !b->valid || !out_rgba || b->family_count == 0) return 0;
    u32 aw = 0, ah = 0;
    for (u32 i = 0; i < b->family_count; ++i) {
        aw += b->families[i].tex_width;
        if (b->families[i].tex_height > ah) ah = b->families[i].tex_height;
    }
    if (aw == 0 || ah == 0) return 0;
    u32 need = aw * ah * 4u;
    if (cap < need) return 0;
    memset(out_rgba, 0, need);
    u32 xoff = 0;
    for (u32 i = 0; i < b->family_count; ++i) {
        const aether_mdl_skin_lump_metal_family_t *f = &b->families[i];
        if (f->lump_index >= lumps->count) continue;
        const aether_mdl_skin_lump_t *L = &lumps->lumps[f->lump_index];
        u32 lw = L->width ? L->width : f->tex_width;
        u32 lh = L->height ? L->height : f->tex_height;
        if (lw > f->tex_width) lw = f->tex_width;
        if (lh > f->tex_height) lh = f->tex_height;
        for (u32 y = 0; y < lh && y < ah; ++y) {
            for (u32 x = 0; x < lw; ++x) {
                u32 src = (y * L->width + x) * 4u;
                if (src + 3 >= L->rgba_bytes && L->rgba_bytes > 0) continue;
                u32 dst = (y * aw + (xoff + x)) * 4u;
                if (L->rgba_bytes == 0) {
                    out_rgba[dst + 0] = (u8)(40 + i * 40);
                    out_rgba[dst + 1] = (u8)(80 + x);
                    out_rgba[dst + 2] = (u8)(60 + y);
                    out_rgba[dst + 3] = 255;
                } else {
                    out_rgba[dst + 0] = L->rgba[src + 0];
                    out_rgba[dst + 1] = L->rgba[src + 1];
                    out_rgba[dst + 2] = L->rgba[src + 2];
                    out_rgba[dst + 3] = L->rgba[src + 3];
                }
            }
        }
        xoff += f->tex_width;
    }
    if (out_w) *out_w = aw;
    if (out_h) *out_h = ah;
    return need;
}

void aether_mdl_skin_lump_metal_bind_mark(aether_mdl_skin_lump_metal_bind_t *b) {
    if (!b) return;
    b->bound = b->valid && b->atlas_ready && b->draw_slot > 0 && b->family_count > 0;
    if (b->bound && b->selected_family < b->family_count)
        b->families[b->selected_family].bound = true;
}

bool aether_mdl_skin_lump_metal_bind_was_bound(const aether_mdl_skin_lump_metal_bind_t *b) {
    return b && b->bound;
}

int aether_mdl_skin_lump_metal_sample(const aether_mdl_skin_lump_set_t *lumps,
                                      const aether_mdl_skin_lump_metal_bind_t *b,
                                      f32 u, f32 v, f32 out_rgba[4]) {
    if (!lumps || !b || !b->valid || !out_rgba) return 0;
    if (b->selected_family >= b->family_count) return 0;
    u32 li = b->families[b->selected_family].lump_index;
    if (li >= lumps->count) return 0;
    return aether_mdl_skin_lump_sample(&lumps->lumps[li], u, v, out_rgba);
}
