/* ---------- Entity diagnostics (runtime-based, no BSP struct) ---------- */
int engine_entity_dump_current_map(void) {
    if (!g_entity_mgr) {
        aether_log(AETHER_LOG_WARN, "bridge", "no entity manager");
        return 0;
    }
    aether_entity_mgr_dump(g_entity_mgr);
    return 1;
}

int engine_entity_summary_text(char *out_buf, int out_cap) {
    if (!out_buf || out_cap <= 0) return -1;

    int w = 0;
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "STEP 17A — Entity diagnostics\n\n");

    if (!g_entity_mgr) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w),
                      "❌ No entities loaded.\n\nFirst load a map.");
        return 0;
    }

    u32 total = aether_entity_mgr_count(g_entity_mgr);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "Total entities: %u\n\n", total);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "Categories:\n");

    /* Count by class prefix */
    u32 monsters = 0, weapons = 0, items = 0, playerstart = 0, doors = 0, triggers = 0, other = 0;
    for (u32 i = 0; i < total; ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(g_entity_mgr, i);
        if (!e) continue;
        if (strncmp(e->classname, "monster_", 8) == 0) monsters++;
        else if (strncmp(e->classname, "weapon_", 7) == 0) weapons++;
        else if (strncmp(e->classname, "item_", 5) == 0 || strncmp(e->classname, "ammo_", 5) == 0) items++;
        else if (strncmp(e->classname, "info_player", 11) == 0) playerstart++;
        else if (strncmp(e->classname, "func_door", 9) == 0) doors++;
        else if (strncmp(e->classname, "trigger_", 8) == 0) triggers++;
        else other++;
    }
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  MONSTER       : %u\n", monsters);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  WEAPON        : %u\n", weapons);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  ITEM/AMMO     : %u\n", items);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  PLAYER_START  : %u\n", playerstart);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  DOOR          : %u\n", doors);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  TRIGGER       : %u\n", triggers);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  OTHER         : %u\n", other);

    /* Player start */
    aether_vec3_t ps_pos = {0,0,0};
    aether_vec3_t ps_ang = {0,0,0};
    if (aether_entity_get_player_start(g_entity_mgr, &ps_pos, &ps_ang) == AETHER_OK) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w),
                      "\nPlayer start:\n  info_player_start @ (%.0f, %.0f, %.0f)\n",
                      ps_pos.x, ps_pos.y, ps_pos.z);
    } else {
        w += snprintf(out_buf + w, (size_t)(out_cap - w), "\n⚠️ No player start found\n");
    }

    /* First monsters */
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "\nFirst few monsters:\n");
    u32 shown = 0;
    for (u32 i = 0; i < total && shown < 5; ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(g_entity_mgr, i);
        if (!e) continue;
        if (strncmp(e->classname, "monster_", 8) == 0) {
            w += snprintf(out_buf + w, (size_t)(out_cap - w),
                          "  %-24s @ (%.0f, %.0f, %.0f)\n",
                          e->classname, e->origin.x, e->origin.y, e->origin.z);
            shown++;
        }
    }
    if (shown == 0) w += snprintf(out_buf + w, (size_t)(out_cap - w), "  (none)\n");

    /* Monster registry summary */
    if (g_monsters_init) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w),
                      "\nMonster registry: %u spawned, %u alive\n",
                      aether_monster_registry_count(&g_monsters),
                      aether_monster_registry_alive(&g_monsters));
    }
    return 1;
}

/* ---------- Entity spawning ---------- */
int engine_entity_spawn_current_map(void) {
    if (!g_entity_mgr) return 0;
    aether_entity_spawn_dump(g_entity_mgr);
    return (int)aether_entity_mgr_count(g_entity_mgr);
}
int engine_entity_count(void) {
    return g_entity_mgr ? (int)aether_entity_mgr_count(g_entity_mgr) : 0;
}
int engine_entity_monster_count(void) {
    if (!g_monsters_init) return 0;
    return (int)aether_monster_registry_count(&g_monsters);
}
int engine_entity_alive_monster_count(void) {
    if (!g_monsters_init) return 0;
    return (int)aether_monster_registry_alive(&g_monsters);
}

int engine_monster_positions_copy(float *out_xyz_flat, int max_monsters) {
    if (!g_monsters_init || !out_xyz_flat || max_monsters <= 0) return 0;
    int n = 0;
    for (u32 i = 0; i < g_monsters.count && n < max_monsters; ++i) {
        const aether_monster_t *m = &g_monsters.monsters[i];
        if (!m->entity) continue;
        out_xyz_flat[n*3 + 0] = m->entity->origin.x;
        out_xyz_flat[n*3 + 1] = m->entity->origin.y;
        out_xyz_flat[n*3 + 2] = m->entity->origin.z;
        n++;
    }
    return n;
}

int engine_monster_healths_copy(int *out_health, int max_monsters) {
    if (!g_monsters_init || !out_health || max_monsters <= 0) return 0;
    int n = 0;
    for (u32 i = 0; i < g_monsters.count && n < max_monsters; ++i) {
        if (!g_monsters.monsters[i].entity) continue;
        out_health[n] = (int)g_monsters.monsters[i].entity->health;
        n++;
    }
    return n;
}

/* ---------- Utility ---------- */
const char *engine_base_path(void) { return g_base_path; }
const char *engine_version(void)   { return AETHER_VERSION_STRING; }
