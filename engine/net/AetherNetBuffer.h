/* AetherNetBuffer.h — Byte-packed serialization buffer for network messages.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_BUFFER_H
#define AETHER_NET_BUFFER_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_NETBUF_SIZE  2048

typedef struct aether_netbuf {
    u8   data[AETHER_NETBUF_SIZE];
    u32  write_pos;
    u32  read_pos;
    bool overflow;
} aether_netbuf_t;

/* Lifecycle */
void aether_netbuf_init_write(aether_netbuf_t *b);
void aether_netbuf_init_read (aether_netbuf_t *b, const u8 *data, u32 size);

/* Write (little-endian) */
void aether_netbuf_write_u8 (aether_netbuf_t *b, u8  v);
void aether_netbuf_write_u16(aether_netbuf_t *b, u16 v);
void aether_netbuf_write_u32(aether_netbuf_t *b, u32 v);
void aether_netbuf_write_i32(aether_netbuf_t *b, i32 v);
void aether_netbuf_write_f32(aether_netbuf_t *b, f32 v);
void aether_netbuf_write_vec3(aether_netbuf_t *b, aether_vec3_t v);
void aether_netbuf_write_bytes(aether_netbuf_t *b, const void *data, u32 n);
void aether_netbuf_write_string(aether_netbuf_t *b, const char *s, u32 max_len);

/* Read (little-endian) */
u8   aether_netbuf_read_u8 (aether_netbuf_t *b);
u16  aether_netbuf_read_u16(aether_netbuf_t *b);
u32  aether_netbuf_read_u32(aether_netbuf_t *b);
i32  aether_netbuf_read_i32(aether_netbuf_t *b);
f32  aether_netbuf_read_f32(aether_netbuf_t *b);
aether_vec3_t aether_netbuf_read_vec3(aether_netbuf_t *b);
bool aether_netbuf_read_bytes(aether_netbuf_t *b, void *out, u32 n);
bool aether_netbuf_read_string(aether_netbuf_t *b, char *out, u32 cap);

/* Utility */
u32  aether_netbuf_size  (const aether_netbuf_t *b);
bool aether_netbuf_error (const aether_netbuf_t *b);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_BUFFER_H */
