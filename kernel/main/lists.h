#pragma once

#include <kernel/sched/spinlock.h>
#include <kernel/types.h>
#include <stddef.h>

typedef struct {
    spinlock_t lock;
    uint32_t length;
    uint32_t offset;
    void *head;
    void *tail;
} list_t;

typedef struct {
    void *prev;
    void *next;
} link_t;

#define LIST_INIT(name, st, m) \
    list_t name = { .offset = offsetof(st, m), .length = 0, .head = 0, .tail = 0, .lock = 0 }

void list_init(list_t *list, int offset);
void list_insert(list_t *list, void *item);
void list_insert_after(list_t *list, void *head, void *item);
void list_insert_before(list_t *list, void *head, void *item);
void list_append(list_t *list, void *item);
int list_remove(list_t *list, void *item);
void *list_pop(list_t *list);
void *list_head(list_t *list);
