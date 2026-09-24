/* AetherLightmap.h — Lightmap/style state + procedural atlas stub.
 * Clean-room; no WAD/HL lightmap assets required.
 * AetherEngine-iOS.
 */
#ifndef AETHER_LIGHTMAP_H
#define AETHER_LIGHTMAP_H
#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward-declare mesh (defined in AetherBSPGeometry.h) to avoid a hard cycle. */
struct aether_mesh;

typedef struct aether_lightmap {
    u32  width, height;
    u32  style_count;
    bool enabled;
    u8  *rgba;      /* optional RGBA8 atlas (grayscale stub or baked); may be NULL */
    bool stub;      /* true when rgba was generated procedurally */
    u32  face_tiles;/* how many face tiles the stub was laid out for (0 = world-space) */
} aether_lightmap_t;

aether_result_t aether_lightmap_init(aether_lightmap_t *lm, u32 width, u32 height, u32 styles);
void aether_lightmap_shutdown(aether_lightmap_t *lm);
void aether_lightmap_enable(aether_lightmap_t *lm, bool enabled);
bool aether_lightmap_is_enabled(const aether_lightmap_t *lm);

/* Allocate/replace atlas with a procedural grayscale stub (no game assets).
 * face_tiles: number of face slots to paint (0 → single full-atlas radial). */
aether_result_t aether_lightmap_generate_stub(aether_lightmap_t *lm, u32 width, u32 height, u32 face_tiles);

/* Write lightmap UVs (lu,lv) onto mesh vertices for face-tile layout matching the stub.
 * Groups vertices by contiguous fan runs of matching normals (BSP mesh_from_bsp order). */
aether_result_t aether_lightmap_assign_mesh_uvs(aether_lightmap_t *lm, struct aether_mesh *mesh);

/* Convenience: generate stub sized for mesh face estimate + assign UVs. */
aether_result_t aether_lightmap_bake_mesh_stub(aether_lightmap_t *lm, struct aether_mesh *mesh);

u32  aether_lightmap_width(const aether_lightmap_t *lm);
u32  aether_lightmap_height(const aether_lightmap_t *lm);
bool aether_lightmap_is_stub(const aether_lightmap_t *lm);
/* Copy RGBA8 atlas bytes. Returns bytes copied, or 0 on failure. */
u32  aether_lightmap_copy_rgba(const aether_lightmap_t *lm, u8 *out, u32 max_bytes);

/* If BSP has a non-empty LIGHTING lump, bake face samples into atlas (stub=false).
 * Otherwise fall back to procedural stub and leave stub=true. */
struct aether_bsp;
aether_result_t aether_lightmap_bake_from_bsp(aether_lightmap_t *lm,
                                              const struct aether_bsp *bsp,
                                              struct aether_mesh *mesh);

/* Unpack lightmap UVs from BSP face texinfo vecs into mesh face_ranges.
 * Requires mesh built via aether_mesh_from_bsp (face_ranges populated).
 * Falls back to face-run tiling when texinfo/face_ranges missing. */
aether_result_t aether_lightmap_unpack_uvs_from_bsp(aether_lightmap_t *lm,
                                                    const struct aether_bsp *bsp,
                                                    struct aether_mesh *mesh);

/* GoldSrc-style lightstyles: up to 64 style strings ('a'..'z' mapped to 0..1).
 * style 0 is normally fullbright ("m"); animated styles cycle over time. */
#define AETHER_MAX_LIGHTSTYLES 64
#define AETHER_LIGHTSTYLE_LEN  64

typedef struct aether_lightstyles {
    char  strings[AETHER_MAX_LIGHTSTYLES][AETHER_LIGHTSTYLE_LEN];
    f32   values[AETHER_MAX_LIGHTSTYLES]; /* current 0..1 scaled value */
    f32   time;
    u32   count;
} aether_lightstyles_t;

void aether_lightstyles_init(aether_lightstyles_t *ls);
void aether_lightstyles_set(aether_lightstyles_t *ls, u32 index, const char *pattern);
void aether_lightstyles_update(aether_lightstyles_t *ls, f32 time);
f32  aether_lightstyles_value(const aether_lightstyles_t *ls, u32 index);

/* Modulate atlas RGB by style 0 (or style_index) current value — host/Metal stub. */
aether_result_t aether_lightmap_apply_style(aether_lightmap_t *lm,
                                            const aether_lightstyles_t *ls,
                                            u32 style_index);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_LIGHTMAP_H */
