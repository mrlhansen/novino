#include <kernel/vfs/types.h>
#include <kernel/mem/heap.h>
#include <string.h>

static dentry_t *dentry_alloc()
{
    dentry_t *item;
    int memsz;

    memsz = sizeof(dentry_t) + sizeof(inode_t);
    item = kzalloc(memsz);
    if(item == 0)
    {
        return 0;
    }

    item->inode = (inode_t*)(item + 1);
    return item;
}

static size_t dentry_hash(const char *p) // case sensitive
{
    size_t h = 0;
    while(*p)
    {
        h = (37 * h) + *p++;
    }
    return h;
}

dentry_t *dcache_lookup(dentry_t *parent, const char *name)
{
    dentry_t *item;
    size_t hash;

    hash = dentry_hash(name);
    item = parent->child;

    while(item)
    {
        if(item->hash == hash)
        {
            if(strcmp(item->name, name) == 0)
            {
                return item;
            }
        }
        item = item->next;
    }

    return 0;
}

void dcache_delete(dentry_t *item)
{
    dentry_t *parent = item->parent;
    dentry_t *curr = parent->child;
    dentry_t *prev = 0;

    while(curr)
    {
        if(curr == item)
        {
            if(prev)
            {
                prev->next = curr->next;
            }
            else
            {
                parent->child = curr->next;
            }

            kfree(item);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

dentry_t *dcache_append(dentry_t *parent, const char *name, inode_t *inode)
{
    dentry_t *item;

    item = dentry_alloc();
    if(item == 0)
    {
        return 0;
    }

    strcpy(item->name, name);
    item->hash = dentry_hash(name);

    if(inode)
    {
        item->inode[0] = inode[0];
        item->inode->fs = parent->inode->fs;
        item->inode->mp = parent->inode->mp;
        item->inode->data = parent->inode->data;
        parent->positive++;
    }
    else
    {
        item->inode = 0;
        parent->negative++;
    }

    item->parent = parent;
    item->next = parent->child;
    parent->child = item;

    return item;
}

void dcache_mark_positive(dentry_t *item)
{
    dentry_t *parent;
    inode_t *inode;

    if(item->inode)
    {
        return;
    }

    parent = item->parent;
    parent->negative--;
    parent->positive++;

    inode = (void*)(item + 1);
    item->inode = inode;
    memset(inode, 0, sizeof(inode_t));

    inode->fs = parent->inode->fs;
    inode->mp = parent->inode->mp;
    inode->data = parent->inode->data;
}

void dcache_mark_negative(dentry_t *item)
{
    dentry_t *parent;

    if(!item->inode)
    {
        return;
    }

    parent = item->parent;
    parent->negative++;
    parent->positive--;

    item->inode = 0;
}

void dcache_purge(dentry_t *root)
{
    dentry_t *item;

    item = root->child;
    while(item)
    {
        dcache_purge(item);
        item = item->next;
    }

    item = root->child;
    while(item)
    {
        root = item->next;
        kfree(item);
        item = root;
    }
}
