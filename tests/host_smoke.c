/* host_smoke.c — Linux/macOS host smoke test for AetherEngine C core.
 * AetherEngine-iOS · Clean-room. No game assets required.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AetherCore.h"
#include "AetherEngine.h"
#include "AetherMath.h"
#include "AetherGameManager.h"
#include "AetherManifest.h"
#include "AetherEntityClassRegistry.h"
#include "AetherPlayerInventory.h"
#include "AetherWeaponDefs.h"
#include "AetherMonsterTypes.h"
#include "AetherMonsterDefs.h"
#include "AetherNetScoreboard.h"
#include "AetherNetChat.h"
#include "AetherVGUIRuntime.h"

static int g_failures = 0;

static void expect(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        g_failures++;
    } else {
        printf("OK:   %s\n", msg);
    }
}

int main(void) {
    printf("AetherEngine host smoke (%s)\n", AETHER_VERSION_STRING);

    aether_arena_t arena;
    expect(aether_arena_init(&arena, 4096) == AETHER_OK, "arena_init");
    void *p = aether_arena_alloc(&arena, 64, 16);
    expect(p != NULL, "arena_alloc");
    aether_arena_destroy(&arena);

    aether_vec3_t a = {1.f, 0.f, 0.f};
    aether_vec3_t b = {0.f, 1.f, 0.f};
    aether_vec3_t c = aether_vec3_cross(a, b);
    expect(fabsf(c.z - 1.f) < 1e-5f, "vec3_cross");

    aether_engine_desc_t desc = {
        .base_path = ".",
        .asset_path = ".",
        .flags = 0,
    };
    aether_engine_t *eng = aether_engine_create(&desc);
    expect(eng != NULL, "engine_create");
    expect(aether_engine_start(eng) == AETHER_OK, "engine_start");
    expect(aether_engine_step(eng, 1.f / 60.f) == AETHER_OK, "engine_step");
    expect(aether_engine_frame_count(eng) >= 1, "engine_frame_count");
    expect(aether_engine_stop(eng) == AETHER_OK, "engine_stop");
    expect(aether_engine_destroy(eng) == AETHER_OK, "engine_destroy");

    expect(aether_game_count() == 5, "game_count==5");
    expect(aether_game_info_by_dir("valve") != NULL, "game valve");
    expect(aether_game_info_by_dir("bshift") != NULL, "game bshift");
    expect(aether_game_info_by_dir("gearbox") != NULL, "game gearbox");
    expect(aether_game_info_by_dir("cstrike") != NULL, "game cstrike");
    expect(aether_game_info_by_dir("czero") != NULL, "game czero");

    aether_game_info_t info;
    memset(&info, 0, sizeof info);
    expect(aether_manifest_load("engine/game/manifests/valve.json", &info) == AETHER_OK,
           "manifest_load valve.json");
    expect(info.dir_name && strcmp(info.dir_name, "valve") == 0, "manifest dir_name valve");

    aether_entity_class_registry_t classes;
    aether_entity_class_registry_init(&classes);
    u32 nclass = aether_entity_class_register_builtin(&classes);
    expect(nclass == 113, "entity_builtin_count==113");
    expect(aether_entity_class_find(&classes, "info_player_start") != NULL,
           "find info_player_start");
    expect(aether_entity_class_find(&classes, "monster_zombie") != NULL,
           "find monster_zombie");

    expect(aether_weapon_defs_count() == (u32)AETHER_WPN_COUNT, "weapon_defs_count");
    expect(aether_weapon_defs_lookup(AETHER_WPN_CROWBAR) != NULL, "weapon crowbar");
    expect(aether_monster_defs_count() == (u32)AETHER_MON_COUNT, "monster_defs_count");
    expect(aether_monster_defs_lookup(AETHER_MON_HEADCRAB) != NULL, "monster headcrab");

    aether_scoreboard_t board;
    aether_scoreboard_init(&board);
    aether_scoreboard_set_visible(&board, true);
    expect(board.visible, "scoreboard_visible");

    aether_chat_log_t chat;
    aether_chat_init(&chat);
    aether_chat_add_system(&chat, "smoke", 0.f);
    expect(chat.count >= 1, "chat_add_system");

    expect(aether_vgui_runtime_init() == AETHER_OK, "vgui_runtime_init");
    aether_vgui_runtime_show_main();
    aether_vgui_runtime_toggle_console();
    expect(aether_vgui_runtime_console_visible(), "vgui_console_visible");
    aether_vgui_runtime_shutdown();

    if (g_failures) {
        fprintf(stderr, "\n%d smoke check(s) failed\n", g_failures);
        return 1;
    }
    printf("\nAll host smoke checks passed.\n");
    return 0;
}
