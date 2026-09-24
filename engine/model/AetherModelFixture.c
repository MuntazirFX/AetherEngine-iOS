/* AetherModelFixture.c — Procedural MDL/SPR fixtures (no HL assets).
 * AetherEngine-iOS · Clean-room.
 * MDL fixture embeds 1 studio mesh = 1 triangle (3 verts) for Metal draw.
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
