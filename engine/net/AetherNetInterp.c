#include "AetherNetInterp.h"
#include <string.h>

void aether_net_interp_init(aether_net_interp_t *it) {
    if (!it) return;
    memset(it, 0, sizeof(*it));
}

void aether_net_interp_push(aether_net_interp_t *it, const aether_net_snapshot_t *snap) {
    if (!it || !snap) return;
    if (it->has_curr) {
        it->prev = it->curr;
        it->has_prev = true;
    }
    it->curr = *snap;
    it->has_curr = true;
    it->fraction = 0.f;
}

void aether_net_interp_set_fraction(aether_net_interp_t *it, f32 frac) {
    if (!it) return;
    if (frac < 0.f) frac = 0.f;
    if (frac > 1.f) frac = 1.f;
    it->fraction = frac;
}

static const aether_net_snapshot_player_t *find_p(const aether_net_snapshot_t *s, u32 id) {
    if (!s) return NULL;
    for (u32 i = 0; i < s->player_count && i < AETHER_NET_MAX_PLAYERS; ++i)
        if (s->players[i].player_id == id) return &s->players[i];
    return NULL;
}

int aether_net_interp_origin(const aether_net_interp_t *it, u32 player_id, f32 out[3]) {
    if (!it || !out || !it->has_curr) return 0;
    const aether_net_snapshot_player_t *c = find_p(&it->curr, player_id);
    if (!c) return 0;
    const aether_net_snapshot_player_t *p = it->has_prev ? find_p(&it->prev, player_id) : NULL;
    if (!p) {
        out[0]=c->origin[0]; out[1]=c->origin[1]; out[2]=c->origin[2];
        return 1;
    }
    f32 t = it->fraction;
    out[0] = p->origin[0] + (c->origin[0] - p->origin[0]) * t;
    out[1] = p->origin[1] + (c->origin[1] - p->origin[1]) * t;
    out[2] = p->origin[2] + (c->origin[2] - p->origin[2]) * t;
    return 1;
}

void aether_net_interp_sample(const aether_net_interp_t *it, aether_net_snapshot_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!it || !it->has_curr) return;
    *out = it->curr;
    for (u32 i = 0; i < out->player_count && i < AETHER_NET_MAX_PLAYERS; ++i) {
        f32 o[3];
        if (aether_net_interp_origin(it, out->players[i].player_id, o)) {
            out->players[i].origin[0] = o[0];
            out->players[i].origin[1] = o[1];
            out->players[i].origin[2] = o[2];
        }
    }
}
