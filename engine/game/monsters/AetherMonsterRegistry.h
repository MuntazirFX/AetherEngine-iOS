/* AetherSaveFormat.h — Save file format constants + header struct.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_SAVE_FORMAT_H
#define AETHER_SAVE_FORMAT_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Magic: "AESV" (Aether Engine SaVe) */
#define AETHER_SAVE_MAGIC        0x56534541u  /* "AESV" little-endian */
#define AETHER_SAVE_VERSION      1
#define AETHER_SAVE_HEADER_SIZE  256
#define AETHER_SAVE_MAP_NAME_MAX 64
#define AETHER_SAVE_GAME_NAME_MAX 32
#define AETHER_SAVE_SLOT_NAME_MAX 64
#define AETHER_SAVE_MAX_SLOTS    16
#define AETHER_SAVE_RESERVED_SIZE 128

/* Save file header (written at the start of every save) */
typedef struct aether_save_header {
    u32   magic;                            /* AETHER_SAVE_MAGIC */
    u32   version;                          /* AETHER_SAVE_VERSION */
    u32   game_id;                          /* aether_game_id_t */
    char  map_name[AETHER_SAVE_MAP_NAME_MAX];
    char  game_name[AETHER_SAVE_GAME_NAME_MAX];
    u32   timestamp;
    u32   play_time_seconds;
    u32   flags;
    u32   entity_count;
    u32   checksum;
    u8    reserved[AETHER_SAVE_RESERVED_SIZE];
} aether_save_header_t;

/* Sections inside the file */
typedef enum aether_save_section {
    AETHER_SAVE_SEC_PLAYER  = 0x01,
    AETHER_SAVE_SEC_ENTITIES = 0x02,
    AETHER_SAVE_SEC_WEAPONS = 0x03,
    AETHER_SAVE_SEC_WORLD   = 0x04,
    AETHER_SAVE_SEC_END     = 0xFF,
} aether_save_section_t;

/* Section header (before each section's data) */
typedef struct aether_save_sec_header {
    u8    id;
    u8    pad[3];
    u32   size;
} aether_save_sec_header_t;

/* Simple checksum */
u32 aether_save_checksum(const u8 *data, u32 size);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_SAVE_FORMAT_H */
