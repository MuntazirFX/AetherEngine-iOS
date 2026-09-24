/* AetherNetProtocol.h — Wire protocol constants for multiplayer.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_PROTOCOL_H
#define AETHER_NET_PROTOCOL_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol identity */
#define AETHER_NET_PROTOCOL_ID     0x4E564541u  /* "AEVN" LE */
#define AETHER_NET_PROTOCOL_VER    1

/* Standard ports */
#define AETHER_NET_DEFAULT_PORT    27015
#define AETHER_NET_MASTER_PORT     27010
#define AETHER_NET_QUERY_PORT      27016

/* Limits */
#define AETHER_NET_MAX_PLAYERS     32
#define AETHER_NET_MAX_PACKET      1400   /* UDP MTU safety */
#define AETHER_NET_MAX_NAME        32
#define AETHER_NET_MAX_CHAT        256
#define AETHER_NET_MAX_SERVER_NAME 64
#define AETHER_NET_MAX_MAP_NAME    64
#define AETHER_NET_CHALLENGE_LEN   8
#define AETHER_NET_TICKRATE        60
#define AETHER_NET_SNAPSHOT_RATE   20

/* Message IDs */
typedef enum aether_net_msg {
    AETHER_MSG_NONE              = 0x00,
    /* Handshake */
    AETHER_MSG_CONNECT_REQUEST   = 0x01,   /* client → server */
    AETHER_MSG_CONNECT_CHALLENGE = 0x02,   /* server → client */
    AETHER_MSG_CONNECT_RESPONSE  = 0x03,   /* client → server */
    AETHER_MSG_CONNECT_ACCEPT    = 0x04,   /* server → client */
    AETHER_MSG_CONNECT_REJECT    = 0x05,   /* server → client */
    AETHER_MSG_DISCONNECT        = 0x06,
    /* In-game */
    AETHER_MSG_CLIENT_CMD        = 0x10,   /* input snapshot */
    AETHER_MSG_SERVER_SNAPSHOT   = 0x11,   /* world snapshot */
    AETHER_MSG_CHAT              = 0x12,
    AETHER_MSG_SCOREBOARD        = 0x13,
    AETHER_MSG_PLAYER_JOIN       = 0x14,
    AETHER_MSG_PLAYER_LEAVE      = 0x15,
    AETHER_MSG_SERVER_INFO       = 0x16,
    AETHER_MSG_SERVER_DELTA      = 0x17,   /* delta snapshot vs baseline */
    AETHER_MSG_KILL              = 0x18,   /* kill feed stub */
    /* Query */
    AETHER_MSG_QUERY             = 0x20,
    AETHER_MSG_QUERY_RESPONSE    = 0x21,
    /* Keepalive */
    AETHER_MSG_PING              = 0x30,
    AETHER_MSG_PONG              = 0x31,
} aether_net_msg_t;

/* Disconnect reason codes */
typedef enum aether_net_reject_reason {
    AETHER_REJECT_NONE          = 0,
    AETHER_REJECT_SERVER_FULL,
    AETHER_REJECT_BAD_PROTOCOL,
    AETHER_REJECT_BAD_PASSWORD,
    AETHER_REJECT_CHALLENGE_FAIL,
    AETHER_REJECT_TIMEOUT,
    AETHER_REJECT_KICKED,
    AETHER_REJECT_BANNED,
} aether_net_reject_reason_t;

/* Client connection state machine */
typedef enum aether_net_state {
    AETHER_NET_STATE_DISCONNECTED = 0,
    AETHER_NET_STATE_CONNECTING,
    AETHER_NET_STATE_CHALLENGING,
    AETHER_NET_STATE_CONNECTED,
    AETHER_NET_STATE_ACTIVE,
} aether_net_state_t;

/* Net address (IPv4 + IPv6 ready) */
typedef struct aether_net_addr {
    u8      ip[16];       /* 4 for v4, 16 for v6 */
    u16     port;
    bool    is_v6;
} aether_net_addr_t;

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_PROTOCOL_H */
