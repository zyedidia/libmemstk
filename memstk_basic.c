#include <stdlib.h>
#include <stdint.h>

#include "memstk.h"

typedef struct {
    void* data;
} mement_t;

typedef struct {
    void* data;
    size_t datasz;

    mement_t* entries;
    size_t sz;
    size_t cap;
} memstk_t;

typedef struct metadata {
    memstk_t stk;
    struct metadata* next;
    struct metadata* prev;
} metadata_t;

static metadata_t* head;

static void mdat_insert_head(metadata_t* n) {
    n->next = head;
    n->prev = NULL;
    if (head)
        head->prev = n;
    head = n;
}

static void mdat_remove(metadata_t* n) {
    if (n->next)
        n->next->prev = n->prev;
    if (n->prev)
        n->prev->next = n->next;
    else
        head = n->next;
}

static int memstk_init() {
    return 0;
}

static void* memstk_map(size_t sz) {
    metadata_t* mdat = (metadata_t*) malloc(sz + sizeof(metadata_t));
    char* alloc = (char*) mdat + sizeof(metadata_t);

    mdat_insert_head(mdat);

    mdat->stk = (memstk_t){
        .data = alloc,
        .datasz = sz,
        .entries = NULL,
        .sz = 0,
        .cap = 0,
    };

    return alloc;
}

static metadata_t* get_mdat(void* p) {
    return (metadata_t*) ((char*) p - sizeof(metadata_t));
}

static void memstk_push(void* p) {
    metadata_t* mdat = get_mdat(p);

    if (mdat->stk.sz >= mdat->stk.cap) {
        mdat->stk.cap = mdat->stk.cap == 0 ? 1 : mdat->stk.cap * 2;
        mdat->stk.entries =
            realloc(mdat->stk.entries, mdat->stk.cap * sizeof(mement_t));
    }

    void* data = malloc(mdat->stk.datasz);
    memcpy(data, mdat->stk.data, mdat->stk.datasz);

    mdat->stk.entries[mdat->stk.sz] = (mement_t){
        .data = data,
    };
    mdat->stk.sz++;
}

static void memstk_pop(void* p) {
    memstk_t* stk = &get_mdat(p)->stk;
    if (stk->sz == 0) {
        return;
    }
    size_t top = stk->sz - 1;
    memcpy(stk->data, stk->entries[top].data, stk->datasz);
    free(stk->entries[top].data);
    stk->sz--;
}

static void memstk_free(void* p) {
    memstk_t* stk = &get_mdat(p)->stk;
    for (size_t i = 0; i < stk->sz; i++) {
        free(stk->entries[i].data);
    }
    free(stk->entries);

    metadata_t* mdat = get_mdat(p);
    mdat_remove(mdat);
    free(mdat);
}

memstk_mapper_t memstk_basic() {
    return (memstk_mapper_t){
        .init = memstk_init,
        .map = memstk_map,
        .push = memstk_push,
        .pop = memstk_pop,
        .free = memstk_free,
    };
}
