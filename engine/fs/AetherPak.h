/* AetherPak.h — Reader for GoldSrc-style PAK archives (pak0.pak, pak1.pak).
 * This is a clean-room re-implementation. Format is publicly documented.
 */
#ifndef AETHER_PAK_H
#define AETHER_PAK_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_PAK_MAGIC 0x4B434150u  /* "PACK" little-endian */
#define AETHER_PAK_MAX_NAME 56

typedef struct aether_pak_entry {
    char name[AETHER_PAK_MAX_NAME];   /* NUL-terminated path inside archive */
    u32  offset;                       /* byte offset in file */
    u32  size;                         /* uncompressed size */
} aether_pak_entry_t;

typedef struct aether_pak aether_pak_t;

aether_pak_t   *aether_pak_open(const char *filepath);
void            aether_pak_close(aether_pak_t *p);

u32             aether_pak_entry_count(const aether_pak_t *p);
const aether_pak_entry_t *aether_pak_entry_at(const aether_pak_t *p, u32 index);
const aether_pak_entry_t *aether_pak_find(const aether_pak_t *p, const char *name);

/* Reads a file's bytes into a caller-allocated buffer. Returns bytes read,
 * or 0 on failure. out_size must equal entry->size (pre-checked by caller). */
u32             aether_pak_read_file(const aether_pak_t *p,
                                     const aether_pak_entry_t *entry,
                                     u8 *out_buffer, u32 out_size);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_PAK_H */
