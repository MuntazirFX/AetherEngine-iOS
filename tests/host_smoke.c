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
#include "AetherRender.h"
#include "AetherRenderFeatures.h"
#include "AetherParticle.h"
#include "AetherMath.h"

/* Host stubs for Metal backend entry points (Swift provides these on iOS). */
aether_result_t aether_metal_init(void *user, u32 w, u32 h) {
    (void)user; (void)w; (void)h; return AETHER_OK;
}
aether_result_t aether_metal_resize(void *user, u32 w, u32 h) {
    (void)user; (void)w; (void)h; return AETHER_OK;
}
aether_result_t aether_metal_submit(void *user, const aether_render_cmd_t *cmd) {
    (void)user; (void)cmd; return AETHER_OK;
}
aether_result_t aether_metal_shutdown(void *user) {
    (void)user; return AETHER_OK;
}

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


    /* Particle pool: spawn → tick → copy_render (feeds Metal). */
    {
        aether_particles_t parts;
        expect(aether_particles_init(&parts) == AETHER_OK, "particles_init");
        f32 origin[3] = { 10.f, 20.f, 30.f };
        u32 spawned = aether_particles_spawn_burst(&parts, origin, 32);
        expect(spawned == 32, "particles_spawn_burst");
        expect(aether_particles_active_count(&parts) == 32, "particles_active_after_spawn");
        aether_particles_update(&parts, 0.5f);
        expect(aether_particles_active_count(&parts) > 0, "particles_active_after_tick");
        aether_particle_vertex_t verts[64];
        u32 copied = aether_particles_copy_render(&parts, verts, 64);
        expect(copied > 0 && copied <= 32, "particles_copy_render");
        expect(verts[0].size > 0.f, "particles_vertex_size");
        aether_particles_clear(&parts);
        expect(aether_particles_active_count(&parts) == 0, "particles_clear");
    }

    /* Renderer feature particles via begin_frame_dt path. */
    /* Renderer camera + feature tick plumbing (NULL backend — no GPU). */
    aether_renderer_t *rend = aether_renderer_create(AETHER_RENDER_NULL, NULL);
    expect(rend != NULL, "renderer_create_null");
    expect(aether_renderer_init(rend, 640, 360) == AETHER_OK, "renderer_init");
    aether_mat4_t view = aether_mat4_look_at(
        (aether_vec3_t){0, 0, 0},
        (aether_vec3_t){1, 0, 0},
        (aether_vec3_t){0, 0, 1});
    aether_mat4_t proj = aether_mat4_perspective(1.2f, 16.f / 9.f, 1.f, 1000.f);
    expect(aether_renderer_set_camera(rend, view, proj) == AETHER_OK, "renderer_set_camera");
    aether_mat4_t got_view, got_proj;
    aether_renderer_get_view(rend, &got_view);
    aether_renderer_get_proj(rend, &got_proj);
    expect(memcmp(got_view.m, view.m, sizeof view.m) == 0, "renderer_get_view");
    expect(memcmp(got_proj.m, proj.m, sizeof proj.m) == 0, "renderer_get_proj");
    expect(aether_renderer_begin_frame_dt(rend, 0.1f, 0.1f, 0.1f, 1.f, 1.f / 30.f) == AETHER_OK,
           "renderer_begin_frame_dt");
    aether_renderer_tick_features(rend, 1.f / 30.f);
    expect(aether_renderer_features(rend) != NULL, "renderer_features");
    {
        aether_render_features_t *feat = aether_renderer_features(rend);
        f32 origin[3] = { 0.f, 0.f, 40.f };
        u32 n = aether_particles_spawn_burst(&feat->particles, origin, 16);
        expect(n == 16, "renderer_particles_burst");
        aether_renderer_tick_features(rend, 1.f / 60.f);
        expect(aether_particles_active_count(&feat->particles) > 0,
               "renderer_particles_still_active");
        expect(aether_renderer_draw_feature(rend, AETHER_CMD_DRAW_PARTICLES) == AETHER_OK,
               "renderer_draw_particles");
    }
    expect(aether_renderer_draw_world(rend) == AETHER_OK, "renderer_draw_world");
    expect(aether_renderer_draw_hud(rend) == AETHER_OK, "renderer_draw_hud");
    expect(aether_renderer_end_frame(rend) == AETHER_OK, "renderer_end_frame");
    expect(aether_renderer_width(rend) == 640 && aether_renderer_height(rend) == 360,
           "renderer_size");
    aether_renderer_destroy(rend);

    if (g_failures) {
        fprintf(stderr, "\n%d smoke check(s) failed\n", g_failures);
        return 1;
    }
    printf("\nAll host smoke checks passed.\n");
    return 0;
}
