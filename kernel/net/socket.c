#include <kernel/net/socket.h>
#include <kernel/mem/heap.h>
#include <kernel/vfs/fd.h>
#include <kernel/errno.h>
#include <kernel/debug.h>
#include <string.h>

static LIST_INIT(inet4, socket_t, link);

static int socket_close(file_t *file)
{
    socket_t *sk;
    socket_data_t *ck;

    sk = file->data;
    while(ck = list_pop(&sk->list), ck)
    {
        kfree(ck);
    }
    kfree(sk);

    return 0;
}

static vfs_ops_t ops = {
    .close = socket_close,
};

static inode_t inode = {
    .flags = I_SOCKET,
    .ops = &ops,
};

int socket_open(int domain, int type, int proto)
{
    socket_t *sk;
    fd_t *fd;

    if(domain != AF_INET4)
    {
        return -EINVAL; // we do not support anything else atm
    }

    // proto is only really meaningful for SOCK_RAW

    if(type == SOCK_TCP)
    {
        if(!proto)
        {
            proto = PROTO_TCP;
        }
        else if(proto != PROTO_TCP)
        {
            return -EINVAL;
        }
    }

    if(type == SOCK_UDP)
    {
        if(!proto)
        {
            proto = PROTO_UDP;
        }
        else if(proto != PROTO_UDP)
        {
            return -EINVAL;
        }
    }

    fd = fd_create();
    if(!fd)
    {
        return -ENOMEM;
    }

    sk = kzalloc(sizeof(*sk));
    if(!sk)
    {
        return -ENOMEM;
    }

    fd->file->flags = I_SOCKET;
    fd->file->inode = &inode;
    fd->file->data = sk;

    sk->domain = domain;
    sk->type = type;
    sk->proto = proto;
    list_init(&sk->list, offsetof(socket_data_t, link));

    if(domain == AF_INET4)
    {
        list_append(&inet4, sk);
    }

    return fd->id;
}

void socket_inet4_recv(int type, int proto, void *data, int size, ipv4_sdp_t *sdp)
{
    socket_t *item = 0;
    socket_data_t *chunk;

    while(item = list_iterate(&inet4, item), item)
    {
        if(item->type != type)
        {
            continue;
        }

        if(item->proto != proto)
        {
            continue;
        }

        chunk = kzalloc(sizeof(*chunk) + size);
        if(!chunk)
        {
            break;
        }

        chunk->addr.daddr = sdp->daddr;
        chunk->addr.saddr = sdp->saddr;

        chunk->length = size;
        chunk->offset = size;
        memcpy(chunk->data, data, size);
        list_append(&item->list, chunk);

        // we could accumulate size of the socket data and have an upper limit

        acquire_lock(&item->lock);
        if(item->thread)
        {
            thread_unblock(item->thread);
            item->thread = 0;
        }
        release_lock(&item->lock);
    }
}

int socket_read(int id, void *data, size_t size, int flags, socket_addr_t *addr)
{
    socket_data_t *chunk;
    socket_t *sk;
    fd_t *fd;

    fd = fd_find(id);
    if(!fd)
    {
        return -EBADF;
    }

    if((fd->file->flags & I_SOCKET) == 0)
    {
        return -EBADF;
    }

    // flags decide blocking vs non-blocking

    sk = fd->file->data;
    chunk = list_head(&sk->list);

    if(!chunk)
    {
        acquire_lock(&sk->lock);
        sk->thread = thread_handle();
        thread_block(&sk->lock);
        chunk = list_head(&sk->list);
    }

    // raw sockets do not support partial reads, the remaining is discarded
    // assume only raw for now

    chunk = list_pop(&sk->list);

    if(chunk->length < size)
    {
        size = chunk->length;
    }

    if(addr)
    {
        addr[0] = chunk->addr;
    }

    memcpy(data, chunk->data, size);
    kfree(chunk);

    return size;
}

int socket_write(int id, void *data, size_t size, int flags, socket_addr_t *addr)
{
    socket_t *sk;
    fd_t *fd;

    fd = fd_find(id);
    if(!fd)
    {
        return -EBADF;
    }

    if((fd->file->flags & I_SOCKET) == 0)
    {
        return -EBADF;
    }

    sk = fd->file->data;

    // we assume raw socket for now
    // we assume inet4 socket for now

    ipv4_sdp_t sdp = {
        .daddr = addr->daddr,
        .saddr = addr->saddr, // should generally be zero, unless bind() has been called
        .proto = sk->proto,
    };

    ipv4_send(data, size, &sdp); // should return an error

    return size;
}
