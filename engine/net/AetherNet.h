/* AetherNet.h — Socket abstraction (BSD sockets, iOS compatible).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_H
#define AETHER_NET_H

#include "AetherNetProtocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_socket aether_socket_t;

/* Create a UDP socket (bound or unbound). */
aether_socket_t *aether_socket_create_udp(void);
aether_socket_t *aether_socket_create_udp_bound(u16 port);

void aether_socket_destroy(aether_socket_t *s);

/* Non-blocking mode toggle */
bool aether_socket_set_nonblocking(aether_socket_t *s, bool nonblocking);

/* Send bytes to address. Returns bytes sent, or -1. */
i32  aether_socket_send(aether_socket_t *s,
                        const aether_net_addr_t *to,
                        const void *data, u32 size);

/* Receive bytes. Returns bytes received, or -1.
 * Fills out_from if non-NULL. */
i32  aether_socket_recv(aether_socket_t *s,
                        aether_net_addr_t *out_from,
                        void *buf, u32 cap);

/* Address helpers */
bool aether_net_addr_from_string(const char *str, u16 port, aether_net_addr_t *out);
void aether_net_addr_to_string(const aether_net_addr_t *a, char *out, u32 cap);
bool aether_net_addr_equal(const aether_net_addr_t *a, const aether_net_addr_t *b);

/* Host name resolution */
bool aether_net_resolve(const char *hostname, u16 port, aether_net_addr_t *out);

/* Time helper (monotonic seconds) */
f64  aether_net_time(void);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_H */
