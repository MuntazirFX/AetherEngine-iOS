/* AetherSave.c — High-level save/load implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherSave.h"
#include "AetherSavePlayer.h"
#include "AetherSaveEntity.h"
#include "AetherSaveWorld.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* ---------- Write helpers ---------- */
static bool write_bytes(FILE *f, const void *data, size_t n) {
    return fwrite(data, 1, n, f) == n;
}

static bool write_section(FILE *f, aether_save_section_t id,
                          const void *data, u32 size) {
    aether_save_sec_header_t h = { .id = (u8)id, .size = size };
    if (!write_bytes(f, &h, sizeof h)) return false;
    if (size > 0 && !write_bytes(f, data, size)) return false;
    return true;
}

/* ---------- Write save file ---------- */
aether_result_t aether_save_write(const char *filepath, const aether_save_ctx_t *ctx) {
    if (!filepath || !ctx) return AETHER_ERR_INVALID_ARG;

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        aether_log(AETHER_LOG_ERROR, "save", "cannot open for write: %s", filepath);
        return AETHER_ERR_IO;
    }

    /* ---------- Header ---------- */
    aether_save_header_t hdr;
    memset(&hdr, 0, sizeof hdr);
    hdr.magic      = AETHER_SAVE_MAGIC;
    hdr.version    = AETHER_SAVE_VERSION;
    hdr.game_id    = (u32)ctx->game_id;
    hdr.timestamp  = (u32)time(NULL);
    hdr.play_time_seconds = ctx->play_time_seconds;

    if (ctx->map_name)  aether_str_copy(hdr.map_name,  AETHER_SAVE_MAP_NAME_MAX,  ctx->map_name);
    if (ctx->game_name) aether_str_copy(hdr.game_name, AETHER_SAVE_GAME_NAME_MAX, ctx->game_name);

    if (!write_bytes(f, &hdr, sizeof hdr)) {
        fclose(f);
        return AETHER_ERR_IO;
    }

    /* ---------- Player section ---------- */
    aether_save_player_t ply;
    aether_vec3_t zero = { 0, 0, 0 };
    aether_save_player_pack(&ply, ctx->player_health, ctx->player_inventory,
                            ctx->player_origin, ctx->player_angles, zero);
    if (!write_section(f, AETHER_SAVE_SEC_PLAYER, &ply, sizeof ply)) {
        fclose(f);
        return AETHER_ERR_IO;
    }

    /* ---------- Entity section (using a large buffer) ---------- */
    u32 ent_bytes = 0;
    u32 ent_count = 0;
    if (ctx->entities) {
        u32 cap = sizeof(aether_save_entity_t) * 4096;
        u8 *buf = (u8*)malloc(cap);
        if (buf) {
            ent_bytes = aether_save_entities_write(ctx->entities, buf, cap, &ent_count);
            if (ent_bytes > 0) {
                if (!write_section(f, AETHER_SAVE_SEC_ENTITIES, buf, ent_bytes)) {
                    free(buf); fclose(f); return AETHER_ERR_IO;
                }
            }
            free(buf);
        }
    }
    hdr.entity_count = ent_count;

    /* ---------- World section ---------- */
    aether_save_world_t wld;
    aether_save_world_pack(&wld, ctx->world_time, ctx->world_flags, 0, 0, 1.0f);
    if (!write_section(f, AETHER_SAVE_SEC_WORLD, &wld, sizeof wld)) {
        fclose(f);
        return AETHER_ERR_IO;
    }

    /* ---------- End marker ---------- */
    write_section(f, AETHER_SAVE_SEC_END, NULL, 0);

    /* ---------- Rewrite header with final checksum ---------- */
    hdr.checksum = aether_save_checksum((const u8*)&hdr, sizeof hdr - sizeof hdr.checksum);
    fseek(f, 0, SEEK_SET);
    write_bytes(f, &hdr, sizeof hdr);

    fclose(f);

    aether_log(AETHER_LOG_INFO, "save",
               "saved to %s (map=%s, entities=%u, play=%us)",
               filepath, hdr.map_name, ent_count, hdr.play_time_seconds);
    return AETHER_OK;
}

/* ---------- Read header ---------- */
aether_result_t aether_save_read_header(const char *filepath, aether_save_header_t *out) {
    if (!filepath || !out) return AETHER_ERR_INVALID_ARG;

    FILE *f = fopen(filepath, "rb");
    if (!f) return AETHER_ERR_NOT_FOUND;

    size_t got = fread(out, 1, sizeof *out, f);
    fclose(f);

    if (got != sizeof *out) return AETHER_ERR_IO;
    if (out->magic != AETHER_SAVE_MAGIC) {
        aether_log(AETHER_LOG_ERROR, "save", "bad magic in %s", filepath);
        return AETHER_ERR_IO;
    }
    if (out->version != AETHER_SAVE_VERSION) {
        aether_log(AETHER_LOG_WARN, "save", "version mismatch (file=%u engine=%u)",
                   out->version, AETHER_SAVE_VERSION);
    }
    return AETHER_OK;
}

/* ---------- Read save file ---------- */
aether_result_t aether_save_read(const char *filepath, aether_save_ctx_t *ctx) {
    if (!filepath || !ctx) return AETHER_ERR_INVALID_ARG;

    FILE *f = fopen(filepath, "rb");
    if (!f) return AETHER_ERR_NOT_FOUND;

    aether_save_header_t hdr;
    if (fread(&hdr, 1, sizeof hdr, f) != sizeof hdr) {
        fclose(f); return AETHER_ERR_IO;
    }
    if (hdr.magic != AETHER_SAVE_MAGIC) {
        aether_log(AETHER_LOG_ERROR, "save", "bad magic");
        fclose(f); return AETHER_ERR_IO;
    }

    aether_log(AETHER_LOG_INFO, "save",
               "loading %s (map=%s, game=%s)",
               filepath, hdr.map_name, hdr.game_name);

    /* Update ctx identity */
    ctx->game_id = (int)hdr.game_id;
    ctx->play_time_seconds = hdr.play_time_seconds;

    /* Read sections */
    aether_save_sec_header_t sec;
    while (fread(&sec, 1, sizeof sec, f) == sizeof sec) {
        if (sec.id == AETHER_SAVE_SEC_END) break;

        if (sec.id == AETHER_SAVE_SEC_PLAYER) {
            aether_save_player_t ply;
            if (fread(&ply, 1, sizeof ply, f) != sizeof ply) break;
            aether_save_player_unpack(&ply, ctx->player_health, ctx->player_inventory,
                                       &ctx->player_origin, &ctx->player_angles, NULL);
            aether_save_player_dump(&ply);
        }
        else if (sec.id == AETHER_SAVE_SEC_ENTITIES) {
            u8 *buf = (u8*)malloc(sec.size);
            if (!buf) break;
            if (fread(buf, 1, sec.size, f) != sec.size) { free(buf); break; }
            if (ctx->entities) {
                u32 per = sizeof(aether_save_entity_t);
                u32 count = sec.size / per;
                aether_save_entities_read(ctx->entities, buf, sec.size, count);
            }
            free(buf);
        }
        else if (sec.id == AETHER_SAVE_SEC_WORLD) {
            aether_save_world_t wld;
            if (fread(&wld, 1, sizeof wld, f) != sizeof wld) break;
            aether_save_world_unpack(&wld, &ctx->world_time, &ctx->world_flags,
                                      NULL, NULL, NULL);
            aether_save_world_dump(&wld);
        }
        else {
            /* Skip unknown section */
            if (fseek(f, sec.size, SEEK_CUR) != 0) break;
        }
    }

    fclose(f);
    aether_log(AETHER_LOG_INFO, "save", "load complete: %s", filepath);
    return AETHER_OK;
}

/* ---------- Slots ---------- */
u32 aether_save_list_slots(const char *dir, aether_save_slot_t *out_slots, u32 max_slots) {
    /* In real implementation, would iterate directory.
     * Here we stub the API — actually filling slots requires opendir/readdir. */
    (void)dir; (void)out_slots; (void)max_slots;
    return 0;
}

aether_result_t aether_save_delete(const char *filepath) {
    if (!filepath) return AETHER_ERR_INVALID_ARG;
    return (remove(filepath) == 0) ? AETHER_OK : AETHER_ERR_IO;
}

void aether_save_dump_header(const aether_save_header_t *h) {
    if (!h) return;
    aether_log(AETHER_LOG_INFO, "save-hdr",
               "magic=0x%X version=%u game=%u map=%s play=%us entities=%u",
               h->magic, h->version, h->game_id, h->map_name,
               h->play_time_seconds, h->entity_count);
}
