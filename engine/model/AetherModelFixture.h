/* AetherModelFixture.h — Procedural clean-room MDL / SPR fixtures for CI.
 * No Half-Life IP; tiny synthetic files the host parser can load.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MODEL_FIXTURE_H
#define AETHER_MODEL_FIXTURE_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Write a minimal valid MDL v10 (IDST) with 1 bone, 1 bodypart, 1 studio
 * triangle mesh (3 verts) extractable for Metal draw. Returns bytes written. */
u32 aether_mdl_write_fixture(u8 *out, u32 cap);

/* Write fixture to filepath. Returns bytes written. */
u32 aether_mdl_write_fixture_file(const char *filepath);

/* Minimal GoldSrc-ish SPR (IDSP) v2 single-frame stub. Returns bytes written. */
u32 aether_sprite_write_fixture(u8 *out, u32 cap);
u32 aether_sprite_write_fixture_file(const char *filepath);

/* Load SPR fixture header fields (type/width/height/frames). Returns AETHER_OK. */
typedef struct aether_sprite_file_info {
    i32 version;
    i32 type;
    i32 width;
    i32 height;
    i32 numframes;
} aether_sprite_file_info_t;

aether_result_t aether_sprite_parse_header(const u8 *data, u32 size,
                                           aether_sprite_file_info_t *out);


/* Textured + 2-bone fixture (clean-room checker texture + dual-bone hierarchy).
 * Returns bytes written. Geometry extract still yields the triangle mesh. */
u32 aether_mdl_write_textured_fixture(u8 *out, u32 cap);
u32 aether_mdl_write_textured_fixture_file(const char *filepath);

/* Extract embedded texture RGBA8 stub from textured fixture (8x8 checker).
 * Returns bytes written (w*h*4) or 0. */
u32 aether_mdl_fixture_texture_rgba(u8 *out, u32 cap, u32 *out_w, u32 *out_h);

#ifdef __cplusplus
}
#endif
#endif
