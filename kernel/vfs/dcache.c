#include <kernel/vfs/types.h>
#include <kernel/mem/heap.h>
#include <string.h>

static dentry_t *dentry_alloc()
{
    dentry_t *dentry;
    int memsz;

    memsz = sizeof(dentry_t) + sizeof(inode_t);
    dentry = kzalloc(memsz);
    if(dentry == 0)
    {
        return 0;
    }

    dentry->inode = (inode_t*)(dentry + 1);
    return dentry;
}

static uint64_t dentry_hash(const char *p) // case sensitive
{
    uint32_t h = 0; // why is this 32 when we return 64??

    while(*p)
    {
        h = (37 * h) + *p++;
    }

    return h;
}

dentry_t *dcache_lookup(dentry_t *parent, const char *name)
{
    dentry_t *curr;
    uint64_t hash;

    hash = dentry_hash(name);
    curr = parent->child;

    while(curr)
    {
        if(curr->hash == hash)
        {
            if(strcmp(curr->name, name) == 0)
            {
                return curr;
            }
        }
        curr = curr->next;
    }

    return 0;
}

dentry_t *dcache_append(dentry_t *parent, const char *name, inode_t *inode, int invalid)
{
    dentry_t *dentry;

    dentry = dentry_alloc();
    if(dentry == 0)
    {
        return 0;
    }

    strcpy(dentry->name, name);
    dentry->hash = dentry_hash(dentry->name);

    dentry->inode[0] = inode[0];
    dentry->invalid = invalid;
    dentry->cached = 0;
    dentry->count = 0;

    dentry->parent = parent;
    dentry->child = 0;
    dentry->next = parent->child;

    parent->child = dentry;
    parent->count++;

    return dentry;
}
