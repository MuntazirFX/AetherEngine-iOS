/* AetherManifest.c — Minimal JSON-like parser for game manifests.
 * This is NOT a full JSON parser. It scans for specific keys we need.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherManifest.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

typedef struct aether_manifest_slot {
    bool used;
    aether_game_id_t id;
    char display_name[128];
    char dir_name[64];
    char short_code[32];
    char game_version[32];
    char start_map[64];
    u32  features;
    aether_game_info_t info;
} aether_manifest_slot_t;

static aether_manifest_slot_t g_slots[AETHER_MANIFEST_MAX_SLOTS];
static u32 g_slot_count = 0;

static bool extract_string(const char *line, const char *key, char *out, size_t cap) {
    const char *pos = strstr(line, key);
    if (!pos) return false;
    pos = strchr(pos, ':');
    if (!pos) return false;
    pos++;
    while (*pos == ' ' || *pos == '\"') pos++;
    const char *end = strchr(pos, '\"');
    if (!end) return false;
    size_t len = (size_t)(end - pos);
    if (len >= cap) len = cap - 1;
    memcpy(out, pos, len);
    out[len] = '\0';
    return true;
}

static aether_game_id_t id_from_dir(const char *dir) {
    const aether_game_info_t *g = aether_game_info_by_dir(dir);
    return g ? g->id : AETHER_GAME_NONE;
}

static aether_manifest_slot_t *alloc_slot(void) {
    if (g_slot_count >= AETHER_MANIFEST_MAX_SLOTS) return NULL;
    aether_manifest_slot_t *s = &g_slots[g_slot_count++];
    memset(s, 0, sizeof *s);
    s->used = true;
    return s;
}

void aether_manifest_clear(void) {
    memset(g_slots, 0, sizeof g_slots);
    g_slot_count = 0;
}

u32 aether_manifest_count(void) { return g_slot_count; }

const aether_game_info_t *aether_manifest_at(u32 index) {
    if (index >= g_slot_count || !g_slots[index].used) return NULL;
    return &g_slots[index].info;
}

aether_result_t aether_manifest_load(const char *filepath, aether_game_info_t *out_info) {
    if (!filepath || !out_info) return AETHER_ERR_INVALID_ARG;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        aether_log(AETHER_LOG_ERROR, "manifest", "cannot open %s", filepath);
        return AETHER_ERR_IO;
    }

    char line[512];
    char display_name[128] = {0};
    char dir_name[64] = {0};
    char short_code[32] = {0};
    char game_version[32] = {0};
    char start_map[64] = {0};

    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '/' || *p == '{' || *p == '}' || *p == '[' || *p == ']') continue;
        extract_string(p, "display_name", display_name, sizeof display_name);
        extract_string(p, "dir_name",     dir_name,     sizeof dir_name);
        extract_string(p, "short_code",   short_code,   sizeof short_code);
        extract_string(p, "game_version", game_version, sizeof game_version);
        extract_string(p, "start_map",    start_map,    sizeof start_map);
    }
    fclose(f);

    aether_manifest_slot_t *slot = alloc_slot();
    if (!slot) return AETHER_ERR_OUT_OF_MEM;

    aether_str_copy(slot->display_name, sizeof slot->display_name, display_name);
    aether_str_copy(slot->dir_name,     sizeof slot->dir_name,     dir_name);
    aether_str_copy(slot->short_code,   sizeof slot->short_code,   short_code);
    aether_str_copy(slot->game_version, sizeof slot->game_version, game_version);
    aether_str_copy(slot->start_map,    sizeof slot->start_map,    start_map);
    slot->id = id_from_dir(slot->dir_name);
    slot->features = 0;

    slot->info.id = slot->id;
    slot->info.display_name = slot->display_name;
    slot->info.dir_name = slot->dir_name;
    slot->info.short_code = slot->short_code;
    slot->info.game_version = slot->game_version;
    slot->info.start_map = slot->start_map;
    slot->info.supported_features = slot->features;

    *out_info = slot->info;

    aether_log(AETHER_LOG_INFO, "manifest", "loaded '%s' (%s) from %s",
               slot->info.display_name, slot->info.dir_name, filepath);
    return AETHER_OK;
}

i32 aether_manifest_load_all(const char *dir_path) {
    if (!dir_path) return -1;
    static const char *k_names[] = {
        "valve.json", "bshift.json", "gearbox.json", "cstrike.json", "czero.json"
    };
    i32 loaded = 0;
    for (u32 i = 0; i < sizeof(k_names)/sizeof(k_names[0]); ++i) {
        char path[600];
        snprintf(path, sizeof path, "%s/%s", dir_path, k_names[i]);
        aether_game_info_t info;
        memset(&info, 0, sizeof info);
        if (aether_manifest_load(path, &info) == AETHER_OK)
            loaded++;
        else
            aether_log(AETHER_LOG_WARN, "manifest", "missing or bad: %s", path);
    }
    aether_log(AETHER_LOG_INFO, "manifest", "load_all from %s → %d manifests", dir_path, loaded);
    return loaded;
}
