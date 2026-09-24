/* AetherMapLoad.c — FS BSP load with synthetic fallback + minimal fixture writer.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMapLoad.h"
#include "../bsp/AetherBSPSynthetic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

aether_result_t aether_map_load(aether_fs_t *fs, const char *vpath,
                                aether_map_load_result_t *out) {
    if (!out) return AETHER_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    if (vpath) aether_str_copy(out->path_tried, sizeof out->path_tried, vpath);

    if (fs && vpath && vpath[0]) {
        if (aether_fs_exists(fs, vpath)) {
            const char *resolved = aether_fs_resolve(fs, vpath);
            if (resolved) {
                aether_str_copy(out->resolved, sizeof out->resolved, resolved);
                aether_bsp_t *bsp = aether_bsp_load(resolved);
                if (bsp && aether_bsp_is_valid(bsp)) {
                    out->bsp = bsp;
                    out->source = AETHER_MAP_SOURCE_FILE;
                    aether_log(AETHER_LOG_INFO, "map",
                               "loaded BSP from FS '%s' → %s", vpath, resolved);
                    return AETHER_OK;
                }
                if (bsp) aether_bsp_free(bsp);
            }
            u8 *buf = (u8 *)malloc(8u * 1024u * 1024u);
            if (buf) {
                u32 n = aether_fs_read_file(fs, vpath, buf, 8u * 1024u * 1024u);
                if (n > 0) {
                    aether_bsp_t *bsp = aether_bsp_load_from_memory(buf, n, vpath);
                    free(buf);
                    if (bsp && aether_bsp_is_valid(bsp)) {
                        out->bsp = bsp;
                        out->source = AETHER_MAP_SOURCE_FILE;
                        aether_str_copy(out->resolved, sizeof out->resolved, vpath);
                        aether_log(AETHER_LOG_INFO, "map",
                                   "loaded BSP from FS memory '%s' (%u bytes)", vpath, n);
                        return AETHER_OK;
                    }
                    if (bsp) aether_bsp_free(bsp);
                } else {
                    free(buf);
                }
            }
            aether_log(AETHER_LOG_WARN, "map",
                       "FS had '%s' but BSP parse failed — synthetic", vpath);
        } else {
            aether_log(AETHER_LOG_INFO, "map",
                       "FS miss '%s' — synthetic demo room", vpath);
        }
    }

    aether_bsp_t *synth = aether_bsp_create_synthetic_room();
    if (!synth) return AETHER_ERR_OUT_OF_MEM;
    out->bsp = synth;
    out->source = AETHER_MAP_SOURCE_SYNTHETIC;
    aether_str_copy(out->resolved, sizeof out->resolved, "synthetic:demo_room");
    return AETHER_OK;
}

aether_result_t aether_map_load_named(aether_fs_t *fs, const char *map_name,
                                      aether_map_load_result_t *out) {
    char vpath[256];
    if (!map_name || !map_name[0]) return aether_map_load(fs, NULL, out);
    if (strncmp(map_name, "maps/", 5) == 0 || strstr(map_name, ".bsp")) {
        aether_str_copy(vpath, sizeof vpath, map_name);
        if (!strstr(vpath, ".bsp")) {
            size_t n = strlen(vpath);
            if (n + 4 < sizeof vpath) { vpath[n] = '.'; vpath[n+1]='b'; vpath[n+2]='s'; vpath[n+3]='p'; vpath[n+4]=0; }
        }
    } else {
        snprintf(vpath, sizeof vpath, "maps/%s.bsp", map_name);
    }
    return aether_map_load(fs, vpath, out);
}

static void wr_u32(u8 *p, u32 v) {
    p[0]=(u8)v; p[1]=(u8)(v>>8); p[2]=(u8)(v>>16); p[3]=(u8)(v>>24);
}
static void wr_i32(u8 *p, i32 v) { wr_u32(p, (u32)v); }
static void wr_u16(u8 *p, u16 v) { p[0]=(u8)v; p[1]=(u8)(v>>8); }
static void wr_f32(u8 *p, f32 f) { u32 u; memcpy(&u, &f, 4); wr_u32(p, u); }

u32 aether_map_write_minimal_fixture(const char *filepath) {
    if (!filepath) return 0;
    static const char k_ents[] =
        "{\n\"classname\" \"worldspawn\"\n\"message\" \"Aether minimal fixture\"\n}\n"
        "{\n\"classname\" \"info_player_start\"\n\"origin\" \"0 0 32\"\n\"angles\" \"0 90 0\"\n}\n"
        "{\n\"classname\" \"monster_headcrab\"\n\"origin\" \"64 0 0\"\n}\n"
        "{\n\"classname\" \"light\"\n\"origin\" \"0 0 96\"\n\"_light\" \"255 255 180 200\"\n}\n";
    const u32 ents_sz = (u32)strlen(k_ents) + 1u;
    const u32 lighting_samples = 16;
    const u32 lighting_sz = lighting_samples * 3u;

    u32 sizes[15];
    memset(sizes, 0, sizeof sizes);
    sizes[0] = ents_sz;
    sizes[1] = 20u;           /* 1 plane */
    sizes[2] = 4u + 4u + 40u; /* miptex */
    sizes[3] = 4u * 12u;      /* 4 verts */
    sizes[5] = 24u;           /* 1 node */
    sizes[6] = 40u;           /* 1 texinfo */
    sizes[7] = 20u;           /* 1 face */
    sizes[8] = lighting_sz;
    sizes[9] = 8u;            /* 1 clipnode */
    sizes[10] = 2u * 28u;     /* 2 leaves */
    sizes[11] = 2u;           /* 1 marksurface */
    sizes[12] = 4u * 4u;      /* 4 edges */
    sizes[13] = 4u * 4u;      /* 4 surfedges */
    sizes[14] = 64u;          /* 1 model */

    u32 header_sz = 4u + 15u * 8u;
    u32 offsets[15];
    u32 cursor = header_sz;
    for (int i = 0; i < 15; ++i) {
        offsets[i] = cursor;
        cursor += sizes[i];
        cursor = (cursor + 3u) & ~3u;
    }
    u32 total = cursor;
    u8 *buf = (u8 *)calloc(1, total);
    if (!buf) return 0;

    wr_u32(buf, 30);
    for (int i = 0; i < 15; ++i) {
        wr_u32(buf + 4 + i * 8 + 0, offsets[i]);
        wr_u32(buf + 4 + i * 8 + 4, sizes[i]);
    }
    memcpy(buf + offsets[0], k_ents, ents_sz);

    { u8 *p = buf + offsets[1];
      wr_f32(p+0,0.f); wr_f32(p+4,0.f); wr_f32(p+8,1.f); wr_f32(p+12,0.f); wr_i32(p+16,2); }

    { u8 *t = buf + offsets[2];
      wr_u32(t, 1); wr_i32(t+4, 8); memset(t+8, 0, 40);
      memcpy(t+8, "fixture", 7); wr_u32(t+8+16, 16); wr_u32(t+8+20, 16); }

    { u8 *v = buf + offsets[3];
      f32 pts[4][3] = {{-64,-64,0},{64,-64,0},{64,64,0},{-64,64,0}};
      for (u32 i = 0; i < 4; ++i) {
        wr_f32(v+i*12+0, pts[i][0]); wr_f32(v+i*12+4, pts[i][1]); wr_f32(v+i*12+8, pts[i][2]);
      }
    }

    { u8 *n = buf + offsets[5];
      wr_i32(n+0, 0);
      wr_u16(n+4, (u16)(i16)-1); wr_u16(n+6, (u16)(i16)-2);
      for (int i=0;i<3;i++) wr_u16(n+8+i*2, (u16)(i16)-128);
      for (int i=0;i<3;i++) wr_u16(n+14+i*2, (u16)(i16)128);
      wr_u16(n+20, 0); wr_u16(n+22, 1); }

    { u8 *t = buf + offsets[6]; memset(t, 0, 40); wr_f32(t+0,1.f); wr_f32(t+16,1.f); }

    { u8 *f = buf + offsets[7];
      wr_u16(f+0,0); wr_u16(f+2,0); wr_i32(f+4,0); wr_u16(f+8,4); wr_u16(f+10,0);
      f[12]=0; f[13]=f[14]=f[15]=255; wr_i32(f+16, 0); }

    { u8 *L = buf + offsets[8];
      for (u32 i = 0; i < lighting_samples; ++i) {
        u8 g = (u8)(40 + (i * 12) % 200);
        L[i*3+0]=g; L[i*3+1]=g; L[i*3+2]=(u8)(g/2+20);
      }
    }

    { u8 *c = buf + offsets[9];
      wr_i32(c+0,0); wr_u16(c+4,(u16)(i16)-1); wr_u16(c+6,(u16)(i16)-2); }

    { u8 *l = buf + offsets[10];
      /* leaf 0 solid */
      wr_i32(l+0, -2); wr_i32(l+4, -1);
      /* leaf 1 empty with marksurface */
      wr_i32(l+28, -1); wr_i32(l+32, -1);
      wr_u16(l+28+20, 0); wr_u16(l+28+22, 1); }

    wr_u16(buf + offsets[11], 0);

    { u8 *e = buf + offsets[12];
      wr_u16(e+0,0); wr_u16(e+2,1); wr_u16(e+4,1); wr_u16(e+6,2);
      wr_u16(e+8,2); wr_u16(e+10,3); wr_u16(e+12,3); wr_u16(e+14,0);
      u8 *s = buf + offsets[13];
      for (int i=0;i<4;i++) wr_i32(s+i*4, i); }

    { u8 *m = buf + offsets[14];
      wr_f32(m+0,-64); wr_f32(m+4,-64); wr_f32(m+8,-16);
      wr_f32(m+12,64); wr_f32(m+16,64); wr_f32(m+20,128);
      wr_f32(m+24,0); wr_f32(m+28,0); wr_f32(m+32,0);
      wr_i32(m+36,0); wr_i32(m+40,0); wr_i32(m+44,0); wr_i32(m+48,0);
      wr_i32(m+52,2); wr_i32(m+56,0); wr_i32(m+60,1); }

    FILE *fp = fopen(filepath, "wb");
    if (!fp) { free(buf); return 0; }
    size_t wrote = fwrite(buf, 1, total, fp);
    fclose(fp);
    free(buf);
    aether_log(AETHER_LOG_INFO, "map", "wrote minimal fixture '%s' (%u bytes)",
               filepath, (u32)wrote);
    return (u32)wrote;
}
