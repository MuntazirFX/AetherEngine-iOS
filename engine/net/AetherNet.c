/* AetherNet.c — Socket abstraction implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNet.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

struct aether_socket {
    int fd;
    bool is_bound;
};

aether_socket_t *aether_socket_create_udp(void) {
    aether_socket_t *s = (aether_socket_t*)calloc(1, sizeof *s);
    if (!s) return NULL;
    s->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->fd < 0) { free(s); return NULL; }
    s->is_bound = false;
    return s;
}

aether_socket_t *aether_socket_create_udp_bound(u16 port) {
    aether_socket_t *s = aether_socket_create_udp();
    if (!s) return NULL;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(s->fd, (struct sockaddr*)&addr, sizeof addr) < 0) {
        aether_log(AETHER_LOG_ERROR, "net", "bind failed on port %u", port);
        close(s->fd);
        free(s);
        return NULL;
    }
    s->is_bound = true;
    aether_log(AETHER_LOG_INFO, "net", "UDP socket bound to port %u", port);
    return s;
}

void aether_socket_destroy(aether_socket_t *s) {
    if (!s) return;
    if (s->fd >= 0) close(s->fd);
    free(s);
}

bool aether_socket_set_nonblocking(aether_socket_t *s, bool nonblocking) {
    if (!s || s->fd < 0) return false;
    int flags = fcntl(s->fd, F_GETFL, 0);
    if (flags < 0) return false;
    if (nonblocking) flags |= O_NONBLOCK;
    else             flags &= ~O_NONBLOCK;
    return fcntl(s->fd, F_SETFL, flags) == 0;
}

i32 aether_socket_send(aether_socket_t *s,
                        const aether_net_addr_t *to,
                        const void *data, u32 size) {
    if (!s || s->fd < 0 || !to || !data) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(to->port);
    memcpy(&addr.sin_addr.s_addr, to->ip, 4);

    ssize_t n = sendto(s->fd, data, size, 0,
                       (struct sockaddr*)&addr, sizeof addr);
    return (i32)n;
}

i32 aether_socket_recv(aether_socket_t *s,
                        aether_net_addr_t *out_from,
                        void *buf, u32 cap) {
    if (!s || s->fd < 0 || !buf) return -1;

    struct sockaddr_in from;
    socklen_t from_len = sizeof from;
    memset(&from, 0, sizeof from);

    ssize_t n = recvfrom(s->fd, buf, cap, 0,
                         (struct sockaddr*)&from, &from_len);

    if (n < 0) return -1;

    if (out_from) {
        memset(out_from, 0, sizeof *out_from);
        memcpy(out_from->ip, &from.sin_addr.s_addr, 4);
        out_from->port = ntohs(from.sin_port);
        out_from->is_v6 = false;
    }
    return (i32)n;
}

bool aether_net_addr_from_string(const char *str, u16 port, aether_net_addr_t *out) {
    if (!str || !out) return false;
    struct in_addr a4;
    if (inet_pton(AF_INET, str, &a4) == 1) {
        memset(out, 0, sizeof *out);
        memcpy(out->ip, &a4.s_addr, 4);
        out->port = port;
        out->is_v6 = false;
        return true;
    }
    struct in6_addr a6;
    if (inet_pton(AF_INET6, str, &a6) == 1) {
        memset(out, 0, sizeof *out);
        memcpy(out->ip, &a6, 16);
        out->port = port;
        out->is_v6 = true;
        return true;
    }
    return false;
}

void aether_net_addr_to_string(const aether_net_addr_t *a, char *out, u32 cap) {
    if (!a || !out || cap == 0) return;
    if (a->is_v6) {
        struct in6_addr a6;
        memcpy(&a6, a->ip, 16);
        inet_ntop(AF_INET6, &a6, out, cap);
    } else {
        struct in_addr a4;
        memcpy(&a4.s_addr, a->ip, 4);
        inet_ntop(AF_INET, &a4, out, cap);
    }
}

bool aether_net_addr_equal(const aether_net_addr_t *a, const aether_net_addr_t *b) {
    if (!a || !b) return false;
    if (a->port != b->port) return false;
    if (a->is_v6 != b->is_v6) return false;
    u32 len = a->is_v6 ? 16 : 4;
    return memcmp(a->ip, b->ip, len) == 0;
}

bool aether_net_resolve(const char *hostname, u16 port, aether_net_addr_t *out) {
    if (!hostname || !out) return false;

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) return false;
    if (!res) return false;

    struct sockaddr_in *sin = (struct sockaddr_in*)res->ai_addr;
    memset(out, 0, sizeof *out);
    memcpy(out->ip, &sin->sin_addr.s_addr, 4);
    out->port = port;
    out->is_v6 = false;

    freeaddrinfo(res);
    return true;
}

f64 aether_net_time(void) {
    struct timespec ts;
#if defined(CLOCK_MONOTONIC)
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1e9;
#endif
    timespec_get(&ts, TIME_UTC);
    return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1e9;
}
