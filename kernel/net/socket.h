#pragma once

#include <kernel/sched/threads.h>
#include <kernel/net/ipv4.h>
#include <kernel/lists.h>

#define AF_LOCAL    1
#define AF_INET4    4
#define AF_INET6    6

#define SOCK_TCP    1
#define SOCK_UDP    2
#define SOCK_RAW    3

#define PROTO_ICMP  1
#define PROTO_TCP   6
#define PROTO_UDP   17
#define PROTO_RAW   256

typedef struct {
    int domain;
    int type;
    int proto;
    link_t link;
    list_t list;
    spinlock_t lock;
    thread_t *thread;
} socket_t;

typedef struct { // TODO: once we support other things, this should be made into a sort of union
    uint32_t saddr;
    uint32_t daddr;
    uint16_t sport;
    uint16_t dport;
} socket_addr_t;

typedef struct {
    int length;         // Total size
    int offset;         // Offset (when data has been partially read)
    link_t link;        // Link in chain
    socket_addr_t addr; // Address information
    uint8_t data[];     // Data
} socket_data_t;

void socket_inet4_recv(int type, int proto, void *buf, int size, ipv4_sdp_t *sdp);

int socket_open(int domain, int type, int proto);
int socket_read(int id, void *data, size_t size, int flags, socket_addr_t *addr);
int socket_write(int id, void *data, size_t size, int flags, socket_addr_t *addr);
