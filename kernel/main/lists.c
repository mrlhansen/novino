#include <kernel/lists.h>
#include <kernel/errno.h>

// GCC allows pointer arithmetic on void pointers as if they were uint8_t
// The following functions use this non-standard property

void list_init(list_t *list, int offset)
{
    list->length = 0;
    list->offset = offset;
    list->head = 0;
    list->tail = 0;
    list->lock = 0;
}

void list_insert(list_t *list, void *item)
{
    link_t *link = item + list->offset;
    link_t *temp;

    acquire_lock(&list->lock);

    link->next = list->head;
    link->prev = 0;
    list->head = item;
    list->length++;

    if(list->tail == 0)
    {
        list->tail = item;
    }

    if(link->next)
    {
        temp = link->next + list->offset;
        temp->prev = item;
    }

    release_lock(&list->lock);
}

void list_insert_after(list_t *list, void *head, void *item)
{
    link_t *link = item + list->offset;
    link_t *prev = head + list->offset;
    link_t *temp;

    acquire_lock(&list->lock);

    link->prev = head;
    link->next = prev->next;
    prev->next = item;
    list->length++;

    if(list->tail == head)
    {
        list->tail = item;
    }

    if(link->next)
    {
        temp = link->next + list->offset;
        temp->prev = item;
    }

    release_lock(&list->lock);
}

void list_insert_before(list_t *list, void *head, void *item)
{
    link_t *link = item + list->offset;
    link_t *next = head + list->offset;
    link_t *temp;

    acquire_lock(&list->lock);

    link->prev = next->prev;
    link->next = head;
    next->prev = item;
    list->length++;

    if(list->head == head)
    {
        list->head = item;
    }

    if(link->prev)
    {
        temp = link->prev + list->offset;
        temp->next = item;
    }

    release_lock(&list->lock);
}

void list_append(list_t *list, void *item)
{
    link_t *link = item + list->offset;
    link_t *temp;

    acquire_lock(&list->lock);

    link->prev = list->tail;
    link->next = 0;
    list->tail = item;
    list->length++;

    if(list->head == 0)
    {
        list->head = item;
    }

    if(link->prev)
    {
        temp = link->prev + list->offset;
        temp->next = item;
    }

    release_lock(&list->lock);
}

int list_remove(list_t *list, void *item)
{
    link_t *link = item + list->offset;
    link_t *temp;
    int status;

    status = -EFAIL;
    acquire_lock(&list->lock);

    if(list->head == item)
    {
        list->head = link->next;
        status = 0;
    }

    if(list->tail == item)
    {
        list->tail = link->prev;
        status = 0;
    }

    if(link->prev)
    {
        temp = link->prev + list->offset;
        temp->next = link->next;
        status = 0;
    }

    if(link->next)
    {
        temp = link->next + list->offset;
        temp->prev = link->prev;
        status = 0;
    }

    if(status == 0)
    {
        link->prev = 0;
        link->next = 0;
        list->length--;
    }

    release_lock(&list->lock);
    return status;
}

void *list_pop(list_t *list)
{
    link_t *link;
    link_t *temp;
    void *item;

    acquire_lock(&list->lock);
    item = list->head;

    if(item)
    {
        link = item + list->offset;
        list->head = link->next;

        if(link->next)
        {
            temp = link->next + list->offset;
            temp->prev = 0;
        }
        else
        {
            list->tail = 0;
        }

        link->prev = 0;
        link->next = 0;
        list->length--;
    }

    release_lock(&list->lock);
    return item;
}

void *list_head(list_t *list)
{
    return list->head;
}
