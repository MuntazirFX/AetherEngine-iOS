/* AetherManifest.c — Minimal JSON-like parser for game manifests.
 * This is NOT a full JSON parser. It scans for specific keys we need.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherManifest.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Helper: extract a string value from a line like: "key": "value" */
static bool extract_string(const char *line, const char *key, char *out, size_t cap) {
    const char *pos = strstr(line, key);
    if (!pos) return false;
    
    pos = strchr(pos, ':');
    if (!pos) return false;
    pos++; // skip ':'
    
    while (*pos == ' ' || *pos == '\"') pos++;
    
    const char *end = strchr(pos, '\"');
    if (!end) return false;
    
    size_t len = (size_t)(end - pos);
    if (len >= cap) len = cap - 1;
    memcpy(out, pos, len);
    out[len] = '\0';
    return true;
}

/* NOTE: extract_int was removed because it was unused (caused -Werror on iOS build).
 * We can re-add it when we actually parse integer fields from manifests. */

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
    u32 features_raw = 0;
    
    while (fgets(line, sizeof line, f)) {
        /* Skip comments and empty lines */
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
    
    /* Fill output struct. We copy strings into static storage, so caller must
       not free them. (Future: use an arena for long-lived strings.) */
    static char s_display[128], s_dir[64], s_short[32], s_version[32], s_map[64];
    aether_str_copy(s_display, sizeof s_display, display_name);
    aether_str_copy(s_dir,     sizeof s_dir,     dir_name);
    aether_str_copy(s_short,   sizeof s_short,   short_code);
    aether_str_copy(s_version, sizeof s_version, game_version);
    aether_str_copy(s_map,     sizeof s_map,     start_map);
    
    out_info->display_name   = s_display;
    out_info->dir_name       = s_dir;
    out_info->short_code     = s_short;
    out_info->game_version   = s_version;
    out_info->start_map      = s_map;
    out_info->supported_features = features_raw;
    
    aether_log(AETHER_LOG_INFO, "manifest", "loaded '%s' (%s) from %s",
               out_info->display_name, out_info->dir_name, filepath);
    return AETHER_OK;
}

i32 aether_manifest_load_all(const char *dir_path) {
    if (!dir_path) return -1;
    /* For now, this is a stub. In future, we will use opendir/readdir to scan
       all .json files in the directory. For iOS, we will bundle them. */
    aether_log(AETHER_LOG_INFO, "manifest", "load_all from %s (stub)", dir_path);
    return 0;
}
