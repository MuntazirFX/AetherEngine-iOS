/* AetherNetBuffer.c — Serialization buffer implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetBuffer.h"
#include <string.h>

void aether_netbuf_init_write(aether_netbuf_t *b) {
    if (!b) return;
    memset(b, 0, sizeof *b);
    b->write_pos = 0;
    b->read_pos = 0;
    b->overflow = false;
}

void aether_netbuf_init_read(aether_netbuf_t *b, const u8 *data, u32 size) {
    if (!b) return;
    memset(b, 0, sizeof *b);
    if (size > AETHER_NETBUF_SIZE) size = AETHER_NETBUF_SIZE;
    if (data && size > 0) memcpy(b->data, data, size);
    b->read_pos = 0;
    b->write_pos = size;
    b->overflow = false;
}

/* ---------- Write ---------- */
static bool ensure_space(aether_netbuf_t *b, u32 n) {
    if (!b || b->write_pos + n > AETHER_NETBUF_SIZE) {
        if (b) b->overflow = true;
        return false;
    }
    return true;
}

void aether_netbuf_write_u8(aether_netbuf_t *b, u8 v) {
    if (!ensure_space(b, 1)) return;
    b->data[b->write_pos++] = v;
}

void aether_netbuf_write_u16(aether_netbuf_t *b, u16 v) {
    if (!ensure_space(b, 2)) return;
    b->data[b->write_pos++] = (u8)(v & 0xFF);
    b->data[b->write_pos++] = (u8)((v >> 8) & 0xFF);
}

void aether_netbuf_write_u32(aether_netbuf_t *b, u32 v) {
    if (!ensure_space(b, 4)) return;
    b->data[b->write_pos++] = (u8)(v & 0xFF);
    b->data[b->write_pos++] = (u8)((v >> 8) & 0xFF);
    b->data[b->write_pos++] = (u8)((v >> 16) & 0xFF);
    b->data[b->write_pos++] = (u8)((v >> 24) & 0xFF);
}

void aether_netbuf_write_i32(aether_netbuf_t *b, i32 v) {
    aether_netbuf_write_u32(b, (u32)v);
}

void aether_netbuf_write_f32(aether_netbuf_t *b, f32 v) {
    u32 u;
    memcpy(&u, &v, 4);
    aether_netbuf_write_u32(b, u);
}

void aether_netbuf_write_vec3(aether_netbuf_t *b, aether_vec3_t v) {
    aether_netbuf_write_f32(b, v.x);
    aether_netbuf_write_f32(b, v.y);
    aether_netbuf_write_f32(b, v.z);
}

void aether_netbuf_write_bytes(aether_netbuf_t *b, const void *data, u32 n) {
    if (!data || !ensure_space(b, n)) return;
    memcpy(b->data + b->write_pos, data, n);
    b->write_pos += n;
}

void aether_netbuf_write_string(aether_netbuf_t *b, const char *s, u32 max_len) {
    if (!s) { aether_netbuf_write_u8(b, 0); return; }
    u32 n = (u32)strlen(s);
    if (n >= max_len) n = max_len - 1;
    aether_netbuf_write_u8(b, (u8)n);
    aether_netbuf_write_bytes(b, s, n);
}

/* ---------- Read ---------- */
static bool has_data(aether_netbuf_t *b, u32 n) {
    if (!b) return false;
    if (b->read_pos + n > b->write_pos) { b->overflow = true; return false; }
    return true;
}

u8 aether_netbuf_read_u8(aether_netbuf_t *b) {
    if (!has_data(b, 1)) return 0;
    return b->data[b->read_pos++];
}

u16 aether_netbuf_read_u16(aether_netbuf_t *b) {
    if (!has_data(b, 2)) return 0;
    u16 v = (u16)b->data[b->read_pos] | ((u16)b->data[b->read_pos + 1] << 8);
    b->read_pos += 2;
    return v;
}

u32 aether_netbuf_read_u32(aether_netbuf_t *b) {
    if (!has_data(b, 4)) return 0;
    u32 v = (u32)b->data[b->read_pos]
          | ((u32)b->data[b->read_pos + 1] << 8)
          | ((u32)b->data[b->read_pos + 2] << 16)
          | ((u32)b->data[b->read_pos + 3] << 24);
    b->read_pos += 4;
    return v;
}

i32 aether_netbuf_read_i32(aether_netbuf_t *b) { return (i32)aether_netbuf_read_u32(b); }

f32 aether_netbuf_read_f32(aether_netbuf_t *b) {
    u32 u = aether_netbuf_read_u32(b);
    f32 v; memcpy(&v, &u, 4); return v;
}

aether_vec3_t aether_netbuf_read_vec3(aether_netbuf_t *b) {
    aether_vec3_t v;
    v.x = aether_netbuf_read_f32(b);
    v.y = aether_netbuf_read_f32(b);
    v.z = aether_netbuf_read_f32(b);
    return v;
}

bool aether_netbuf_read_bytes(aether_netbuf_t *b, void *out, u32 n) {
    if (!out || !has_data(b, n)) return false;
    memcpy(out, b->data + b->read_pos, n);
    b->read_pos += n;
    return true;
}

bool aether_netbuf_read_string(aether_netbuf_t *b, char *out, u32 cap) {
    if (!out || cap == 0) return false;
    u8 len = aether_netbuf_read_u8(b);
    if (len >= cap) len = (u8)(cap - 1);
    if (!aether_netbuf_read_bytes(b, out, len)) { out[0] = 0; return false; }
    out[len] = 0;
    return true;
}

u32  aether_netbuf_size(const aether_netbuf_t *b) { return b ? b->write_pos : 0; }
bool aether_netbuf_error(const aether_netbuf_t *b) { return !b || b->overflow; }
