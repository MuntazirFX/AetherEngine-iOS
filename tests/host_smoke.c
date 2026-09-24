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
#include "AetherSky.h"
#include "AetherWater.h"
#include "AetherFog.h"
#include "AetherLightmap.h"
#include "AetherBSP.h"
#include "AetherBSPGeometry.h"
#include "AetherBSPSynthetic.h"
#include "AetherBSPVis.h"
#include "AetherEntityBase.h"
#include "AetherEntitySpawn.h"
#include "AetherWorld.h"
#include "AetherCollision.h"
#include "AetherPlayer.h"
#include "AetherPlayerHealth.h"
#include "AetherPlayerDamage.h"
#include "AetherInput.h"
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





    /* Fog state: init → params → copy_render (feeds Metal fullscreen tint). */
    {
        aether_fog_t fog;
        expect(aether_fog_init(&fog) == AETHER_OK, "fog_init");
        expect(fog.enabled, "fog_enabled_default");
        expect(fog.density > 0.f, "fog_density_default");
        expect(fog.factor > 0.f, "fog_factor_default");
        expect(fog.end > fog.start, "fog_range_default");
        f32 fcol[4] = {0.5f, 0.6f, 0.7f, 1.f};
        expect(aether_fog_set_color(&fog, fcol) == AETHER_OK, "fog_set_color");
        expect(aether_fog_set_factor(&fog, 0.4f) == AETHER_OK, "fog_set_factor");
        aether_fog_set_density(&fog, 0.5f);
        aether_fog_set_range(&fog, 64.f, 2048.f);
        expect(fabsf(fog.start - 64.f) < 1e-5f, "fog_start");
        expect(fabsf(fog.end - 2048.f) < 1e-5f, "fog_end");
        u32 need = aether_fog_render_vertex_count();
        expect(need == 6u, "fog_render_vertex_count");
        aether_fog_vertex_t verts[6];
        u32 copied = aether_fog_copy_render(&fog, verts, need);
        expect(copied == need, "fog_copy_render");
        expect(verts[0].a > 0.f, "fog_vertex_alpha");
        aether_fog_set_enabled(&fog, false);
        expect(aether_fog_copy_render(&fog, verts, need) == 0, "fog_copy_disabled");
        aether_fog_shutdown(&fog);
        expect(!fog.enabled, "fog_shutdown");
    }

    /* Water state: init → update → copy_render (feeds Metal plane). */
    {
        aether_water_t water;
        expect(aether_water_init(&water) == AETHER_OK, "water_init");
        expect(water.enabled, "water_enabled_default");
        expect(water.size > 0.f, "water_size");
        f32 wcol[4] = {0.1f, 0.4f, 0.6f, 0.7f};
        expect(aether_water_set_color(&water, wcol) == AETHER_OK, "water_set_color");
        expect(aether_water_set_size(&water, 256.f) == AETHER_OK, "water_set_size");
        expect(aether_water_set_height(&water, -32.f) == AETHER_OK, "water_set_height");
        expect(aether_water_set_origin(&water, 10.f, 20.f) == AETHER_OK, "water_set_origin");
        expect(aether_water_set_wave(&water, 1.5f, 4.f, 0.05f) == AETHER_OK, "water_set_wave");
        aether_water_update(&water, 0.5f);
        expect(water.wave_time > 0.f, "water_wave_time");
        u32 need = aether_water_render_vertex_count();
        expect(need > 0 && (need % 3u) == 0u, "water_render_vertex_count");
        aether_water_vertex_t *verts = (aether_water_vertex_t *)malloc(sizeof(*verts) * need);
        expect(verts != NULL, "water_verts_alloc");
        u32 copied = aether_water_copy_render(&water, verts, need);
        expect(copied == need, "water_copy_render");
        expect(verts[0].a > 0.f, "water_vertex_alpha");
        free(verts);
        aether_water_set_enabled(&water, false);
        expect(aether_water_copy_render(&water, NULL, need) == 0, "water_copy_disabled");
        aether_water_shutdown(&water);
        expect(!water.enabled, "water_shutdown");
    }

    /* Sky state: init → gradient → copy_render (feeds Metal dome). */
    {
        aether_sky_t sky;
        expect(aether_sky_init(&sky) == AETHER_OK, "sky_init");
        expect(sky.enabled, "sky_enabled_default");
        expect(sky.face_count == (u32)AETHER_SKY_FACE_COUNT, "sky_face_count");
        expect(sky.radius > 0.f, "sky_radius");
        expect(aether_sky_set_name(&sky, "desert") == AETHER_OK, "sky_set_name");
        f32 up[4] = {0.2f, 0.4f, 0.9f, 1.f};
        expect(aether_sky_set_face_color(&sky, AETHER_SKY_FACE_UP, up) == AETHER_OK,
               "sky_set_face_up");
        aether_sky_rebuild_gradient(&sky);
        expect(fabsf(sky.top_color[2] - 0.9f) < 1e-5f, "sky_top_from_up");
        u32 need = aether_sky_render_vertex_count();
        expect(need > 0 && (need % 3u) == 0u, "sky_render_vertex_count");
        aether_sky_vertex_t *verts = (aether_sky_vertex_t *)malloc(sizeof(*verts) * need);
        expect(verts != NULL, "sky_verts_alloc");
        u32 copied = aether_sky_copy_render(&sky, verts, need);
        expect(copied == need, "sky_copy_render");
        expect(verts[0].a > 0.f, "sky_vertex_alpha");
        free(verts);
        aether_sky_shutdown(&sky);
        expect(!sky.enabled && sky.face_count == 0, "sky_shutdown");
    }

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


    /* Synthetic BSP demo room → mesh + entities (feeds Metal world path). */
    {
        aether_bsp_t *bsp = aether_bsp_create_synthetic_room();
        expect(bsp != NULL, "bsp_create_synthetic_room");
        expect(aether_bsp_is_valid(bsp), "bsp_synthetic_valid");
        expect(aether_bsp_is_synthetic(bsp), "bsp_is_synthetic");
        expect(aether_bsp_version(bsp) == 30u, "bsp_synthetic_version");
        expect(aether_bsp_vertex_count(bsp) == 8u, "bsp_synthetic_verts");
        expect(aether_bsp_face_count(bsp) == 6u, "bsp_synthetic_faces");
        expect(aether_bsp_edge_count(bsp) == 12u, "bsp_synthetic_edges");
        expect(aether_bsp_plane_count(bsp) == 38u, "bsp_synthetic_planes"); /* render 7 + clip + ledge + alcove + water */
        expect(aether_bsp_node_count(bsp) == 1u, "bsp_synthetic_nodes");
        expect(aether_bsp_leaf_count(bsp) == 3u, "bsp_synthetic_leaves"); /* solid + west + east */

        aether_mesh_t *mesh = NULL;
        expect(aether_mesh_from_bsp(bsp, NULL, &mesh) == AETHER_OK && mesh != NULL,
               "mesh_from_synthetic_bsp");
        expect(mesh->vertex_count >= 24u, "mesh_synth_vertex_count");
        expect(mesh->index_count >= 36u && (mesh->index_count % 3u) == 0u,
               "mesh_synth_index_count");
        expect(mesh->bounds_max[0] > mesh->bounds_min[0], "mesh_synth_bounds");

        aether_entity_mgr_t *mgr = aether_entity_mgr_create();
        expect(mgr != NULL, "entity_mgr_create");
        u32 spawned = aether_entity_spawn_from_bsp(mgr, bsp);
        expect(spawned >= 4u, "entity_spawn_from_synthetic"); /* worldspawn+start+3 monsters; light skipped */
        aether_vec3_t pos, ang;
        expect(aether_entity_get_player_start(mgr, &pos, &ang) == AETHER_OK,
               "synthetic_player_start");
        expect(fabsf(pos.z - 40.f) < 1e-3f, "synthetic_player_start_z");

        aether_world_render_t world;
        expect(aether_world_render_init(&world) == AETHER_OK, "world_render_init");
        aether_world_render_set_surface_count(&world, mesh->index_count / 3u);
        expect(world.surface_count == mesh->index_count / 3u, "world_surface_count");
        aether_world_render_shutdown(&world);

        /* VIS / leaf culling (synthetic stub: X=0 split, vis_offset=-1 → all empty). */
        expect(mesh->face_count == 6u && mesh->face_ranges != NULL, "mesh_face_ranges");
        {
            i32 west = aether_bsp_find_leaf(bsp, -50.f, 0.f, 40.f);
            i32 east = aether_bsp_find_leaf(bsp,  50.f, 0.f, 40.f);
            expect(west == 1, "bsp_find_leaf_west");
            expect(east == 2, "bsp_find_leaf_east");
            expect(aether_bsp_leaf_is_drawable(bsp, west), "leaf_west_drawable");
            expect(aether_bsp_leaf_is_drawable(bsp, east), "leaf_east_drawable");
            expect(!aether_bsp_leaf_is_drawable(bsp, 0), "leaf_solid_not_drawable");

            u8 bits[8];
            u32 *idx = (u32 *)malloc(mesh->index_count * sizeof(u32));
            expect(idx != NULL, "vis_idx_alloc");

            /* Default USE_PVS with vis_offset=-1 → all empty leaves → 6 unique faces. */
            aether_bsp_vis_stats_t st_pvs;
            u32 n_pvs = aether_bsp_vis_cull_mesh(bsp, mesh, -50.f, 0.f, 40.f,
                                                 AETHER_BSP_VIS_USE_PVS,
                                                 idx, mesh->index_count, &st_pvs);
            expect(st_pvs.view_leaf == 1, "vis_pvs_view_leaf");
            expect(st_pvs.visible_faces == 6u, "vis_pvs_all_faces");
            expect(n_pvs == mesh->index_count, "vis_pvs_all_indices");

            /* CURRENT_LEAF_ONLY west → 4 faces (skip +X/+Y). */
            aether_bsp_vis_stats_t st_w;
            u32 n_w = aether_bsp_vis_cull_mesh(bsp, mesh, -50.f, 0.f, 40.f,
                                               AETHER_BSP_VIS_CURRENT_LEAF_ONLY,
                                               idx, mesh->index_count, &st_w);
            expect(st_w.visible_faces == 4u, "vis_leaf_only_west_faces");
            expect(n_w == 24u, "vis_leaf_only_west_indices"); /* 4 quads × 6 idx */
            expect(n_w < mesh->index_count, "vis_leaf_only_skips_faces");

            u32 marked = aether_bsp_vis_mark_faces(bsp, east, AETHER_BSP_VIS_CURRENT_LEAF_ONLY,
                                                   bits, 8);
            expect(marked == 4u, "vis_leaf_only_east_faces");
            expect(bits[5] == 1 && bits[4] == 0, "vis_east_has_plusx_not_minusx");

            aether_bsp_vis_stats_t st_full;
            u32 n_full = aether_bsp_vis_cull_mesh(bsp, mesh, 0.f, 0.f, 40.f,
                                                  AETHER_BSP_VIS_FORCE_FULL,
                                                  idx, mesh->index_count, &st_full);
            expect(st_full.visible_faces == 6u && n_full == mesh->index_count,
                   "vis_force_full");

            aether_world_render_t w2;
            expect(aether_world_render_init(&w2) == AETHER_OK, "world_vis_init");
            aether_world_render_set_surface_count(&w2, mesh->index_count / 3u);
            aether_world_render_set_visible_surface_count(&w2, n_w / 3u);
            expect(w2.surface_count == 12u && w2.visible_surface_count == 8u,
                   "world_visible_surface_count");
            aether_world_render_shutdown(&w2);

            free(idx);
            printf("  VIS cull: full=%u faces, leaf-only west=%u faces/%u idx (before=%u indices)\n",
                   st_pvs.visible_faces, st_w.visible_faces, n_w, mesh->index_count);
        }

        /* Clipnodes / collision: floor, wall, 16u ledge + step-up (feeds player move). */
        {
            u32 clip_sz = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_CLIPNODES);
            expect(clip_sz >= 72u * 8u, "bsp_synthetic_clipnodes_lump"); /* 72 × sizeof(clipnode)=8 */
            aether_collision_t *col = aether_collision_build(bsp);
            expect(col != NULL, "collision_build");
            expect(aether_collision_clipnode_count(col) == 72u, "collision_clipnode_count");
            expect(aether_collision_hull_root(col, 1) == 6, "collision_hull1_root");
            expect(aether_collision_hull_root(col, 2) == 12, "collision_hull2_root");

            /* Interior empty, below floor solid, outside wall solid. */
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){0, 0, 40}, 1),
                   "collision_interior_empty");
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){0, 0, -1}, 1),
                   "collision_below_floor_solid");
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){250, 0, 40}, 1),
                   "collision_past_wall_solid"); /* standing inset → |x|>240 solid */
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){200, 0, 40}, 1),
                   "collision_inside_wall_margin_empty");

            /* 16u ledge: solid inside (100,0,8), empty on top (100,0,16), empty before face. */
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){100, 0, 8}, 1),
                   "collision_ledge_interior_solid");
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){100, 0, 16}, 1),
                   "collision_ledge_top_empty");
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){40, 0, 0}, 1),
                   "collision_before_ledge_empty");

            /* Trace down onto floor → on_ground, z settles near 0. */
            bool on_ground = false;
            aether_vec3_t landed = aether_collision_move(
                col, (aether_vec3_t){0, 0, 40}, (aether_vec3_t){0, 0, -10}, 1, 0.f, &on_ground);
            expect(on_ground, "collision_trace_hits_floor");
            expect(landed.z >= -0.05f && landed.z <= 0.05f, "collision_land_z_near_0");

            /* Walk into +X wall → X clamped, Y free (step disabled). */
            on_ground = false;
            aether_vec3_t slid = aether_collision_move(
                col, (aether_vec3_t){200, 0, 40}, (aether_vec3_t){300, 50, 40}, 1, 0.f, &on_ground);
            expect(slid.x < 241.f && slid.x > 200.f - 1.f, "collision_wall_blocks_x");
            expect(fabsf(slid.y - 50.f) < 1e-2f, "collision_wall_allows_slide_y");

            /* Step-up smoke: without step, ledge face blocks; with step 18, climbs to z≈16. */
            on_ground = false;
            aether_vec3_t blocked = aether_collision_move(
                col, (aether_vec3_t){40, 0, 0}, (aether_vec3_t){100, 0, 0}, 1, 0.f, &on_ground);
            expect(blocked.x < 65.f, "stepup_disabled_blocks_at_ledge");
            expect(fabsf(blocked.z) < 0.1f, "stepup_disabled_stays_on_floor");

            on_ground = false;
            aether_vec3_t climbed = aether_collision_move(
                col, (aether_vec3_t){40, 0, 0}, (aether_vec3_t){100, 0, 0}, 1,
                AETHER_DEFAULT_STEP_HEIGHT, &on_ground);
            expect(climbed.x > 90.f, "stepup_enabled_crosses_ledge");
            expect(climbed.z >= 15.5f && climbed.z <= 16.5f, "stepup_enabled_lands_on_ledge");
            expect(on_ground, "stepup_enabled_on_ground");

            /* Tall wall still blocks even with step-up (ceiling room wall at 240). */
            on_ground = false;
            aether_vec3_t wall = aether_collision_move(
                col, (aether_vec3_t){220, 0, 40}, (aether_vec3_t){300, 0, 40}, 1,
                AETHER_DEFAULT_STEP_HEIGHT, &on_ground);
            expect(wall.x < 241.f, "stepup_does_not_bypass_wall");

            /* Player gravity soak: fall from spawn height onto floor. */
            aether_player_t ply;
            aether_player_init(&ply);
            expect(fabsf(ply.step_height - AETHER_DEFAULT_STEP_HEIGHT) < 1e-3f,
                   "player_default_step_height");
            aether_player_set_position(&ply, (aether_vec3_t){0, 0, 40});
            aether_input_t *in = aether_input_create();
            expect(in != NULL, "collision_player_input");
            for (int i = 0; i < 180; ++i) {
                aether_input_begin_frame(in);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(ply.on_ground, "player_lands_on_ground");
            expect(ply.position.z >= -0.1f && ply.position.z <= 2.0f, "player_feet_on_floor");

            /* Holding forward into +X wall does not tunnel (yaw=0 → forward=+X). */
            aether_player_set_position(&ply, (aether_vec3_t){220, 0, 1});
            ply.on_ground = true;
            ply.yaw = 0.f;
            for (int i = 0; i < 60; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_move(in, 0.f, 1.f); /* move_y = forward */
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(ply.position.x < 241.f, "player_cannot_walk_through_wall");

            /* Player step-up onto +X ledge with default step_height. */
            aether_player_set_position(&ply, (aether_vec3_t){40, 0, 0});
            ply.on_ground = true;
            ply.yaw = 0.f;
            ply.step_height = AETHER_DEFAULT_STEP_HEIGHT;
            for (int i = 0; i < 90; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_move(in, 0.f, 1.f);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(ply.position.x > 90.f, "player_stepup_crosses_ledge");
            expect(ply.position.z >= 15.0f && ply.position.z <= 18.0f, "player_stepup_on_ledge_z");
            expect(ply.on_ground, "player_stepup_grounded");

            /* With step_height=0, same walk stays blocked before ledge. */
            aether_player_set_position(&ply, (aether_vec3_t){40, 0, 0});
            ply.on_ground = true;
            ply.step_height = 0.f;
            ply.yaw = 0.f;
            for (int i = 0; i < 90; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_move(in, 0.f, 1.f);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(ply.position.x < 65.f, "player_nostep_blocked_at_ledge");
            expect(ply.position.z < 2.0f, "player_nostep_stays_low");

            /* --- Crouch hull (distinct Z) + jump ceiling + low alcove --- */
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){0, 0, 40}, 1),
                   "stand_open_room_z40_empty");
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){0, 0, 60}, 1),
                   "stand_above_headroom_solid"); /* feet max 56 */
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){0, 0, 60}, 2),
                   "crouch_higher_headroom_empty"); /* crouch feet max 92 */
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){0, 0, 100}, 2),
                   "crouch_above_headroom_solid");

            /* Low alcove at x≈-180: standing blocked on floor, crouch fits. */
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){-180, 0, 0}, 1),
                   "alcove_stand_blocked");
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){-180, 0, 0}, 2),
                   "alcove_crouch_fits");
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){-180, 0, 20}, 2),
                   "alcove_crouch_too_tall_solid");

            /* Jump / upward move clamps against standing ceiling (~56). */
            bool ceil_ground = false;
            aether_vec3_t jumped = aether_collision_move(
                col, (aether_vec3_t){0, 0, 0}, (aether_vec3_t){0, 0, 200}, 1, 0.f, &ceil_ground);
            expect(jumped.z < 57.f && jumped.z > 50.f, "jump_clamped_by_stand_ceiling");
            aether_vec3_t crouch_up = aether_collision_move(
                col, (aether_vec3_t){0, 0, 0}, (aether_vec3_t){0, 0, 200}, 2, 0.f, &ceil_ground);
            expect(crouch_up.z > jumped.z + 10.f, "crouch_jump_higher_than_stand");
            expect(crouch_up.z < 93.f && crouch_up.z > 85.f, "jump_clamped_by_crouch_ceiling");

            /* Player duck toggle + stand-up blocked under alcove. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 0, 0});
            ply.on_ground = true;
            ply.crouching = false;
            ply.hull_index = 1;
            for (int i = 0; i < 10; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_action(in, AETHER_ACTION_DUCK, true);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(ply.crouching && ply.hull_index == 2, "player_duck_sets_hull2");

            /* Walk crouched into alcove, then release duck — stay crouched. */
            aether_player_set_position(&ply, (aether_vec3_t){-180, 0, 0});
            ply.on_ground = true;
            ply.crouching = true;
            ply.hull_index = 2;
            ply.yaw = 3.14159265f; /* face -X */
            for (int i = 0; i < 30; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_action(in, AETHER_ACTION_DUCK, false);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(ply.crouching && ply.hull_index == 2, "player_standup_blocked_in_alcove");

            /* Player jump hits standing ceiling and zeros upward vel. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 0, 0});
            ply.on_ground = true;
            ply.crouching = false;
            ply.hull_index = 1;
            ply.velocity = (aether_vec3_t){0, 0, 0};
            float peak_z = 0.f;
            for (int i = 0; i < 120; ++i) {
                aether_input_begin_frame(in);
                if (i == 0) aether_input_set_action(in, AETHER_ACTION_JUMP, true);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
                if (ply.position.z > peak_z) peak_z = ply.position.z;
            }
            expect(peak_z < 57.f, "player_jump_peak_under_stand_ceiling");
            expect(peak_z > 20.f, "player_jump_got_airborne");

            /* --- Water contents / swim / buoyancy (+Y pool) --- */
            expect(aether_collision_point_contents(col, (aether_vec3_t){0, 170, 20}, 1)
                       == AETHER_CONTENTS_WATER,
                   "water_pool_center_contents");
            /* z=50: above water surface (48) but under stand ceiling (56). */
            expect(aether_collision_point_contents(col, (aether_vec3_t){0, 170, 50}, 1)
                       == AETHER_CONTENTS_EMPTY,
                   "water_above_surface_empty");
            expect(aether_collision_point_contents(col, (aether_vec3_t){0, 0, 40}, 1)
                       == AETHER_CONTENTS_EMPTY,
                   "dry_room_center_empty");
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){0, 170, 20}, 1),
                   "water_is_not_solid");
            /* Walls/floor still solid after water clip insert. */
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){0, 0, -1}, 1),
                   "water_floor_still_solid");
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){250, 0, 40}, 1),
                   "water_wall_still_solid");

            /* Mid-pool buoyancy: float up instead of falling like dry air. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 170, 20});
            ply.on_ground = false;
            ply.crouching = false;
            ply.hull_index = 1;
            ply.velocity = (aether_vec3_t){0, 0, 0};
            ply.in_water = false;
            float z0 = ply.position.z;
            int saw_water = 0;
            for (int i = 0; i < 90; ++i) {
                aether_input_begin_frame(in);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
                if (ply.in_water) saw_water = 1;
            }
            expect(saw_water, "player_detects_water");
            expect(ply.position.z > z0 + 2.f, "player_buoyancy_rises");
            expect(ply.position.z < 56.f, "player_buoyancy_under_surface");

            /* Swim-up with jump held. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 170, 10});
            ply.on_ground = false;
            ply.velocity = (aether_vec3_t){0, 0, 0};
            float swim_peak = ply.position.z;
            for (int i = 0; i < 90; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_action(in, AETHER_ACTION_JUMP, true);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
                if (ply.position.z > swim_peak) swim_peak = ply.position.z;
            }
            expect(ply.in_water || swim_peak > 30.f, "player_swim_up_progress");
            expect(swim_peak > 25.f, "player_swim_up_peak");

            /* Exit water toward -Y, then dry walk on floor outside pool. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 170, 0});
            ply.on_ground = true;
            ply.velocity = (aether_vec3_t){0, 0, 0};
            ply.yaw = -1.5707963f; /* face -Y */
            ply.step_height = AETHER_DEFAULT_STEP_HEIGHT;
            for (int i = 0; i < 120; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_move(in, 0.f, 1.f);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(!ply.in_water, "player_exits_water");
            expect(ply.position.y < 130.f, "player_walked_out_of_pool");

            /* Dry walk: spawn outside pool (y=80), confirm no water + floor. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 80, 5});
            ply.on_ground = false;
            ply.velocity = (aether_vec3_t){0, 0, 0};
            ply.crouching = false;
            ply.hull_index = 1;
            ply.in_water = true; /* force clear via update */
            for (int i = 0; i < 90; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_action(in, AETHER_ACTION_JUMP, false);
                aether_input_set_action(in, AETHER_ACTION_DUCK, false);
                aether_input_set_move(in, 0.f, 0.f);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(!ply.in_water, "player_dry_outside_pool");
            expect(ply.on_ground, "player_dry_lands");
            expect(ply.position.z < 3.f, "player_dry_walk_on_floor");

            /* Regression: step-up / crouch still work after water changes. */
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){100, 0, 16}, 1),
                   "water_ledge_top_still_empty");
            expect(aether_collision_point_in_solid(col, (aether_vec3_t){-180, 0, 0}, 1),
                   "water_alcove_stand_still_blocked");
            expect(!aether_collision_point_in_solid(col, (aether_vec3_t){-180, 0, 0}, 2),
                   "water_alcove_crouch_still_fits");

            /* --- Waterlevel tiers (feet/waist/eye) + splash + air stub --- */
            {
                i32 dry = aether_player_sample_waterlevel(
                    col, (aether_vec3_t){0, 80, 0}, 28.f, 1);
                expect(dry == AETHER_WATERLEVEL_DRY, "waterlevel_dry_outside");

                /* Deep floor: feet+waist+eye all under surface z=48. */
                i32 under = aether_player_sample_waterlevel(
                    col, (aether_vec3_t){0, 170, 0}, 28.f, 1);
                expect(under == AETHER_WATERLEVEL_EYE, "waterlevel_eye_deep");

                /* Mid depth: feet+waist wet, eye at ~48 → empty → WAIST/swim. */
                i32 swim = aether_player_sample_waterlevel(
                    col, (aether_vec3_t){0, 170, 20}, 28.f, 1);
                expect(swim == AETHER_WATERLEVEL_WAIST, "waterlevel_waist_mid");

                /* Near surface: only feet wet → FEET/wade. */
                i32 wade = aether_player_sample_waterlevel(
                    col, (aether_vec3_t){0, 170, 40}, 28.f, 1);
                expect(wade == AETHER_WATERLEVEL_FEET, "waterlevel_feet_wade");

                /* Above surface samples → dry. */
                i32 above = aether_player_sample_waterlevel(
                    col, (aether_vec3_t){0, 170, 50}, 28.f, 1);
                expect(above == AETHER_WATERLEVEL_DRY, "waterlevel_dry_above");
            }

            /* Enter splash: dry → wet transition. */
            aether_player_init(&ply);
            aether_player_set_position(&ply, (aether_vec3_t){0, 80, 0});
            ply.on_ground = true;
            ply.waterlevel = AETHER_WATERLEVEL_DRY;
            ply.in_water = false;
            /* One update still dry (outside pool). */
            aether_input_begin_frame(in);
            aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
            aether_input_end_frame(in);
            expect(aether_player_take_splash_event(&ply) == AETHER_SPLASH_NONE,
                   "splash_none_while_dry");

            /* Teleport into deep water; next update should ENTER. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 170, 5});
            ply.waterlevel = AETHER_WATERLEVEL_DRY;
            ply.in_water = false;
            ply.splash_event = AETHER_SPLASH_NONE;
            aether_input_begin_frame(in);
            aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
            aether_input_end_frame(in);
            expect(ply.waterlevel >= AETHER_WATERLEVEL_FEET, "enter_sets_wet_tier");
            expect(aether_player_take_splash_event(&ply) == AETHER_SPLASH_ENTER,
                   "splash_enter_on_wet");
            expect(aether_player_take_splash_event(&ply) == AETHER_SPLASH_NONE,
                   "splash_enter_consumed");

            /* Particle burst path used by splash FX. */
            {
                aether_particles_t parts;
                expect(aether_particles_init(&parts) == AETHER_OK, "splash_parts_init");
                f32 origin[3] = { ply.position.x, ply.position.y,
                                  ply.position.z + ply.eye_height * 0.5f };
                u32 n = aether_particles_spawn_burst(&parts, origin, 24);
                expect(n == 24, "splash_particle_burst");
                expect(aether_particles_active_count(&parts) == 24,
                       "splash_particles_active");
                aether_particles_clear(&parts);
            }

            /* Exit splash: walk out toward -Y. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 170, 0});
            ply.on_ground = true;
            ply.velocity = (aether_vec3_t){0, 0, 0};
            ply.yaw = -1.5707963f;
            ply.waterlevel = AETHER_WATERLEVEL_EYE;
            ply.in_water = true;
            ply.splash_event = AETHER_SPLASH_NONE;
            ply.step_height = AETHER_DEFAULT_STEP_HEIGHT;
            int saw_exit = 0;
            for (int i = 0; i < 150; ++i) {
                aether_input_begin_frame(in);
                aether_input_set_move(in, 0.f, 1.f);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
                if (aether_player_take_splash_event(&ply) == AETHER_SPLASH_EXIT)
                    saw_exit = 1;
            }
            expect(saw_exit, "splash_exit_on_leave");
            expect(ply.waterlevel == AETHER_WATERLEVEL_DRY, "exit_waterlevel_dry");
            expect(!ply.in_water, "exit_in_water_clear");

            /* Air / drown stub: eye underwater drains air; dry recovers. */
            aether_player_init(&ply);
            aether_player_set_position(&ply, (aether_vec3_t){0, 170, 0});
            ply.waterlevel = AETHER_WATERLEVEL_DRY;
            expect(ply.air == AETHER_PLAYER_AIR_MAX, "air_full_on_init");
            expect(!aether_player_is_drowning(&ply), "air_not_drowning_init");
            for (int i = 0; i < 60; ++i) { /* 1s underwater */
                aether_input_begin_frame(in);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(aether_player_eye_underwater(&ply), "air_eye_under_deep");
            expect(ply.air < AETHER_PLAYER_AIR_MAX - 0.5f, "air_drains_under");
            expect(!ply.drowning, "air_not_yet_drowned_1s");

            /* Force drown stub: pin deep (buoyancy can surface) with air already empty. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 170, 0});
            ply.velocity = (aether_vec3_t){0, 0, 0};
            ply.air = 0.0f;
            ply.drowning = false;
            ply.waterlevel = AETHER_WATERLEVEL_DRY;
            aether_input_begin_frame(in);
            aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
            aether_input_end_frame(in);
            expect(aether_player_eye_underwater(&ply), "air_still_under_for_drown");
            expect(ply.air <= 0.f && aether_player_is_drowning(&ply),
                   "air_drown_stub_trips");

            /* Recover on dry land. */
            aether_player_set_position(&ply, (aether_vec3_t){0, 80, 0});
            ply.waterlevel = AETHER_WATERLEVEL_EYE; /* cleared on update */
            ply.air = 1.0f;
            ply.drowning = true;
            for (int i = 0; i < 90; ++i) {
                aether_input_begin_frame(in);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
            }
            expect(ply.waterlevel == AETHER_WATERLEVEL_DRY, "air_recover_dry");
            expect(ply.air > 1.0f, "air_recovers_on_surface");
            expect(!ply.drowning, "air_drown_clears_on_surface");

            /* Drown → health damage tick (STEP 2o). Surface stops damage + air recovers. */
            {
                aether_player_health_t hp;
                aether_player_health_init(&hp);
                expect(aether_player_health_get(&hp) == AETHER_PLAYER_START_HEALTH,
                       "drown_hp_full_init");

                /* Deep + air empty → drowning flag → tick_drown drains HP. */
                aether_player_init(&ply);
                aether_player_set_position(&ply, (aether_vec3_t){0, 170, 0});
                ply.air = 0.0f;
                ply.drowning = false;
                aether_input_begin_frame(in);
                aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                aether_input_end_frame(in);
                expect(aether_player_is_drowning(&ply), "drown_flag_for_damage");

                f32 hp0 = aether_player_health_get(&hp);
                /* Simulate ~1s of drown damage at 10 hp/s (bridge calls this each tick). */
                for (int i = 0; i < 60; ++i)
                    aether_player_tick_drown(&hp, 1.f / 60.f,
                                             aether_player_is_drowning(&ply));
                f32 hp1 = aether_player_health_get(&hp);
                expect(hp1 < hp0 - 8.0f && hp1 > hp0 - 12.0f,
                       "drown_health_drops_approx_10ps");

                /* No damage while flag false (even if tick_drown called). */
                aether_player_tick_drown(&hp, 1.0f, false);
                expect(aether_player_health_get(&hp) == hp1,
                       "drown_no_damage_when_not_drowning");

                /* Surface: drowning clears, air recovers, further ticks do not hurt. */
                aether_player_set_position(&ply, (aether_vec3_t){0, 80, 0});
                ply.air = 0.5f;
                ply.drowning = true;
                for (int i = 0; i < 90; ++i) {
                    aether_input_begin_frame(in);
                    aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                    aether_input_end_frame(in);
                    aether_player_tick_drown(&hp, 1.f / 60.f,
                                             aether_player_is_drowning(&ply));
                }
                expect(!ply.drowning, "drown_clears_on_surface_for_hp");
                expect(ply.air > 0.5f, "air_recovers_with_hp_wire");
                expect(aether_player_health_get(&hp) == hp1,
                       "drown_damage_stops_on_surface");

                /* HUD bridge field contract (ClassicHUD reads these via EngineBridge). */
                expect(aether_player_air(&ply) == ply.air, "hud_air_getter_matches");
                expect(ply.air_max == AETHER_PLAYER_AIR_MAX, "hud_air_max_contract");
                expect(!aether_player_is_drowning(&ply), "hud_drowning_false_surface");
            }

            /* Fall damage on land impact (STEP 2p). Room is short so inject impact speed. */
            {
                aether_player_health_t hp;
                aether_player_health_init(&hp);

                /* Formula contract: below 580 u/s → 0; high speed → HP loss. */
                expect(aether_player_calc_fall_damage(-200.0f) == 0.0f,
                       "fall_calc_short_drop_zero");
                expect(aether_player_calc_fall_damage(-580.0f) == 0.0f,
                       "fall_calc_threshold_zero");
                f32 big = aether_player_calc_fall_damage(-900.0f);
                expect(big > 20.0f && big < 50.0f, "fall_calc_high_speed_damage");

                /* High fall → land dry → pending damage → HP drop + punch.
                 * Synthetic room is short; inject impact speed and soak until ground. */
                aether_player_init(&ply);
                aether_player_set_position(&ply, (aether_vec3_t){0, 0, 40});
                ply.on_ground = false;
                ply.velocity = (aether_vec3_t){0, 0, -900.0f};
                ply.fall_velocity_z = -900.0f;
                for (int i = 0; i < 30 && !ply.on_ground; ++i) {
                    ply.velocity.z = -900.0f;
                    ply.fall_velocity_z = -900.0f;
                    aether_input_begin_frame(in);
                    aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                    aether_input_end_frame(in);
                }
                expect(ply.on_ground, "fall_high_lands_ground");
                f32 pending = aether_player_take_fall_damage(&ply);
                expect(pending > 20.0f, "fall_high_pending_damage");
                expect(aether_player_view_punch_pitch(&ply) < 0.0f,
                       "fall_high_view_punch");
                {
                    aether_damage_event_t ev = {0};
                    ev.amount = pending;
                    ev.type = AETHER_DMG_FALL;
                    f32 hp0 = aether_player_health_get(&hp);
                    aether_player_apply_damage(&hp, &ev);
                    expect(aether_player_health_get(&hp) < hp0 - 20.0f,
                           "fall_high_hp_loss");
                }
                expect(aether_player_take_fall_damage(&ply) == 0.0f,
                       "fall_pending_cleared");

                /* Small step / short drop from z=8 → impact << 580 → no damage. */
                aether_player_init(&ply);
                aether_player_set_position(&ply, (aether_vec3_t){0, 0, 8});
                for (int i = 0; i < 60 && !ply.on_ground; ++i) {
                    aether_input_begin_frame(in);
                    aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                    aether_input_end_frame(in);
                }
                expect(ply.on_ground, "fall_short_lands");
                expect(aether_player_take_fall_damage(&ply) == 0.0f,
                       "fall_short_no_damage");

                /* Water soft: dive from just above surface (stand headroom ~56) into pool. */
                aether_player_init(&ply);
                aether_player_set_position(&ply, (aether_vec3_t){0, 170, 52});
                ply.on_ground = false;
                f32 water_fall_dmg = 0.0f;
                for (int i = 0; i < 60; ++i) {
                    ply.velocity.z = -900.0f;
                    ply.fall_velocity_z = -900.0f;
                    aether_input_begin_frame(in);
                    aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                    aether_input_end_frame(in);
                    water_fall_dmg += aether_player_take_fall_damage(&ply);
                }
                expect(water_fall_dmg == 0.0f, "fall_water_soft_or_wet");

                /* Wet land impact: airborne→ground while feet in water. */
                aether_player_init(&ply);
                aether_player_set_position(&ply, (aether_vec3_t){0, 170, 24});
                ply.on_ground = false;
                f32 wet_land_dmg = 0.0f;
                for (int i = 0; i < 45; ++i) {
                    ply.velocity.z = -900.0f;
                    ply.fall_velocity_z = -900.0f;
                    aether_input_begin_frame(in);
                    aether_player_update(&ply, aether_input_state(in), col, 1.f / 60.f);
                    aether_input_end_frame(in);
                    wet_land_dmg += aether_player_take_fall_damage(&ply);
                    if (ply.on_ground) break;
                }
                expect(wet_land_dmg == 0.0f, "fall_water_land_soft");
            }

            /* Manual splash trigger API. */
            aether_player_trigger_splash(&ply, AETHER_SPLASH_ENTER);
            expect(aether_player_take_splash_event(&ply) == AETHER_SPLASH_ENTER,
                   "splash_manual_trigger");

            aether_input_destroy(in);
            aether_collision_free(col);
            printf("  collision: floor z=%.3f, ledge climb x=%.1f z=%.1f, jump ceil~%.1f, "
                   "water swim peak~%.1f, waterlevel/splash/air/drown-hp/fall ok\n",
                   landed.z, climbed.x, climbed.z, jumped.z, swim_peak);
        }

        /* Lightmap stub: procedural atlas + mesh LUV (feeds Metal sample). */
        aether_lightmap_t lm;
        expect(aether_lightmap_init(&lm, 64, 64, 1) == AETHER_OK, "lightmap_init");
        expect(aether_lightmap_is_enabled(&lm), "lightmap_enabled_default");
        expect(aether_lightmap_bake_mesh_stub(&lm, mesh) == AETHER_OK, "lightmap_bake_mesh_stub");
        expect(aether_lightmap_is_stub(&lm), "lightmap_is_stub");
        expect(aether_lightmap_width(&lm) == 256u && aether_lightmap_height(&lm) == 256u,
               "lightmap_stub_size");
        expect(lm.rgba != NULL, "lightmap_rgba");
        expect(mesh->vertices[0].lu >= 0.f && mesh->vertices[0].lu <= 1.f, "lightmap_uv_lu");
        expect(mesh->vertices[0].lv >= 0.f && mesh->vertices[0].lv <= 1.f, "lightmap_uv_lv");
        {
            u8 *rgba = (u8 *)malloc(256u * 256u * 4u);
            expect(rgba != NULL, "lightmap_copy_alloc");
            u32 got = aether_lightmap_copy_rgba(&lm, rgba, 256u * 256u * 4u);
            expect(got == 256u * 256u * 4u, "lightmap_copy_rgba");
            /* Stub should vary (not a flat fill). */
            u8 lo = 255, hi = 0;
            for (u32 i = 0; i < 256u * 256u; ++i) {
                u8 g = rgba[i * 4u];
                if (g < lo) lo = g;
                if (g > hi) hi = g;
            }
            expect(hi > lo + 20, "lightmap_stub_contrast");
            free(rgba);
        }
        aether_lightmap_shutdown(&lm);
        expect(lm.rgba == NULL && !lm.enabled, "lightmap_shutdown");

        aether_entity_mgr_destroy(mgr);
        aether_mesh_free(mesh);
        aether_bsp_free(bsp);
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
        expect(aether_lightmap_is_enabled(&feat->lightmap), "renderer_lightmap_enabled");
        f32 origin[3] = { 0.f, 0.f, 40.f };
        u32 n = aether_particles_spawn_burst(&feat->particles, origin, 16);
        expect(n == 16, "renderer_particles_burst");
        aether_renderer_tick_features(rend, 1.f / 60.f);
        expect(aether_particles_active_count(&feat->particles) > 0,
               "renderer_particles_still_active");
        expect(aether_renderer_draw_feature(rend, AETHER_CMD_DRAW_PARTICLES) == AETHER_OK,
               "renderer_draw_particles");
        expect(feat->sky.enabled, "renderer_sky_enabled");
        expect(feat->sky.face_count == 6, "renderer_sky_faces");
        expect(aether_renderer_draw_feature(rend, AETHER_CMD_DRAW_SKY) == AETHER_OK,
               "renderer_draw_sky");
        expect(feat->water.enabled, "renderer_water_enabled");
        expect(aether_renderer_draw_feature(rend, AETHER_CMD_DRAW_WATER) == AETHER_OK,
               "renderer_draw_water");
        expect(feat->fog.enabled, "renderer_fog_enabled");
        expect(aether_renderer_draw_feature(rend, AETHER_CMD_DRAW_FOG) == AETHER_OK,
               "renderer_draw_fog");
        aether_renderer_tick_features(rend, 1.f / 30.f);
        expect(feat->water.wave_time > 0.f, "renderer_water_wave_ticks");
    }
    {
        aether_render_features_t *feat = aether_renderer_features(rend);
        if (feat) aether_world_render_set_surface_count(&feat->world, 12u);
    }
    expect(aether_renderer_draw_world(rend) == AETHER_OK, "renderer_draw_world");
    expect(aether_renderer_features(rend)->world.surface_count == 12u,
           "renderer_world_surfaces");
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
