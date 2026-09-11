/* AetherMDL.h — GoldSrc MDL v10 parser (STEP 16A, diagnostics only).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MDL_H
#define AETHER_MDL_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MDL_VERSION      10
#define AETHER_MDL_ID           (('I') | ('D'<<8) | ('S'<<16) | ('T'<<24))
#define AETHER_MDL_NAME_MAX     64
#define AETHER_MDL_BONE_NAME    32

/* Parsed header (native layout, populated by loader). */
typedef struct aether_mdl_info {
    char name[AETHER_MDL_NAME_MAX];
    i32  length;               /* total file bytes */
    f32  eye_position[3];
    f32  min[3];
    f32  max[3];
    f32  bbmin[3];
    f32  bbmax[3];

    i32  bone_count;
    i32  bodypart_count;
    i32  texture_count;
    i32  sequence_count;
    i32  skinref_count;
    i32  skinfamily_count;
    i32  hitbox_count;
    i32  attachment_count;
} aether_mdl_info_t;

/* A single bone (name + parent index). */
typedef struct aether_mdl_bone {
    char name[AETHER_MDL_BONE_NAME];
    i32  parent;
} aether_mdl_bone_t;

/* A single texture reference (name + size, no pixel data). */
typedef struct aether_mdl_skin {
    char name[AETHER_MDL_NAME_MAX];
    i32  width;
    i32  height;
    i32  flags;
} aether_mdl_skin_t;

/* A bodypart summary. */
typedef struct aether_mdl_bodypart {
    char name[AETHER_MDL_NAME_MAX];
    i32  num_models;
} aether_mdl_bodypart_t;

typedef struct aether_mdl aether_mdl_t;

/* Load from disk / memory. Parses header + bones + textures + bodyparts. */
aether_mdl_t *aether_mdl_load(const char *filepath);
aether_mdl_t *aether_mdl_load_from_memory(const u8 *data, u32 size, const char *name);
void          aether_mdl_free(aether_mdl_t *m);

bool                        aether_mdl_is_valid(const aether_mdl_t *m);
const aether_mdl_info_t    *aether_mdl_info(const aether_mdl_t *m);
i32                         aether_mdl_bone_count(const aether_mdl_t *m);
const aether_mdl_bone_t    *aether_mdl_bone_at(const aether_mdl_t *m, i32 idx);
i32                         aether_mdl_skin_count(const aether_mdl_t *m);
const aether_mdl_skin_t    *aether_mdl_skin_at(const aether_mdl_t *m, i32 idx);
i32                         aether_mdl_bodypart_count(const aether_mdl_t *m);
const aether_mdl_bodypart_t *aether_mdl_bodypart_at(const aether_mdl_t *m, i32 idx);

void aether_mdl_dump(const aether_mdl_t *m);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MDL_H */
