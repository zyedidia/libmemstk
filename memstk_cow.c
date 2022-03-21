#define _GNU_SOURCE

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "memstk.h"
#include "mprot.h"

typedef struct {
    void* data;
    uintptr_t addr;
} backup_t;

typedef struct {
    backup_t* backups;
    size_t sz;
    size_t cap;

    // original protections for each page at time of push
    prot_t* prots;
} mement_t;

void mement_append(mement_t* ent, backup_t backup) {
    if (ent->sz >= ent->cap) {
        ent->cap = ent->cap == 0 ? 1 : ent->cap * 2;
        ent->backups = realloc(ent->backups, sizeof(backup_t) * ent->cap);
    }

    ent->backups[ent->sz] = backup;
    ent->sz++;
}

typedef struct {
    uintptr_t addr;
    size_t npages;
    // list of protections for each page
    prot_t* prots;

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

int page_size;

// receive write protection faults here
static void handler(uintptr_t addr) {
    metadata_t* mdat = head;
    while (mdat) {
        memstk_t* stk = &mdat->stk;

        if (addr >= stk->addr && addr < (stk->addr + stk->npages * page_size)) {
            // get the page corresponding to this write
            uintptr_t page_addr = addr & ~(page_size - 1);
            // page number within the matching allocation
            size_t page = (page_addr - stk->addr) / page_size;
            // make a backup page
            void* backup = malloc(page_size);
            memcpy(backup, (void*) page_addr, page_size);
            mement_append(&stk->entries[stk->sz - 1], (backup_t){
                                                          .data = backup,
                                                          .addr = page_addr,
                                                      });
            // now that we have a backup, make the page R/W
            stk->prots[page] = PROT_RW;
            mprot_protect_mem((void*) page_addr, page_size, PROT_RW);
            break;
        }

        mdat = mdat->next;
    }
}

static int memstk_init() {
    page_size = sysconf(_SC_PAGE_SIZE);
    return mprot_init(handler);
}

static void* memstk_map_pages(size_t npages) {
    metadata_t* mdat = (metadata_t*) mmap(NULL, (npages + 1) * page_size,
                                          PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!mdat) {
        return NULL;
    }
    char* alloc = (char*) mdat + page_size;
    if (mprot_register_mem(alloc, npages * page_size) == -1) {
        return NULL;
    }

    mdat_insert_head(mdat);

    mdat->stk = (memstk_t){
        .addr = (uintptr_t) alloc,
        .npages = npages,
        .prots = (prot_t*) malloc(sizeof(prot_t) * npages),
        .entries = NULL,
        .sz = 0,
        .cap = 0,
    };
    for (size_t i = 0; i < npages; i++) {
        mdat->stk.prots[i] = PROT_RW;
    }

    return alloc;
}

static void* memstk_map(size_t sz) {
    return memstk_map_pages((sz + page_size - 1) / page_size);
}

static metadata_t* get_mdat(void* p) {
    return (metadata_t*) ((char*) p - page_size);
}

static void memstk_push(void* p) {
    metadata_t* mdat = get_mdat(p);

    if (mdat->stk.sz >= mdat->stk.cap) {
        mdat->stk.cap = mdat->stk.cap == 0 ? 1 : mdat->stk.cap * 2;
        mdat->stk.entries =
            realloc(mdat->stk.entries, mdat->stk.cap * sizeof(mement_t));
    }

    prot_t* prots = malloc(sizeof(prot_t) * mdat->stk.npages);
    memcpy(prots, mdat->stk.prots, sizeof(prot_t) * mdat->stk.npages);

    mdat->stk.entries[mdat->stk.sz] = (mement_t){
        .backups = NULL,
        .sz = 0,
        .cap = 0,
        .prots = prots,
    };
    for (size_t i = 0; i < mdat->stk.npages; i++) {
        mdat->stk.prots[i] = PROT_R;
    }
    mprot_protect_mem(p, mdat->stk.npages * page_size, PROT_R);
    mdat->stk.sz++;
}

static void memstk_pop(void* p) {
    memstk_t* stk = &get_mdat(p)->stk;
    if (stk->sz == 0) {
        return;
    }
    size_t top = stk->sz - 1;
    if (stk->entries[top].sz > 0) {
        for (size_t i = 0; i < stk->entries[top].sz; i++) {
            backup_t* backup = &stk->entries[top].backups[i];
            mprot_protect_mem((void*) backup->addr, page_size, PROT_RW);
            memcpy((void*) backup->addr, backup->data, page_size);
            free(backup->data);
        }
    }
    for (size_t i = 0; i < stk->npages; i++) {
        prot_t prot = stk->entries[top].prots[i];
        // TODO: optimization: only re-protect memory if necessary
        mprot_protect_mem(p + i * page_size, page_size, prot);
        stk->prots[i] = prot;
    }
    free(stk->entries[top].prots);
    free(stk->entries[top].backups);
    stk->sz--;
}

static void memstk_free(void* p) {
    memstk_t* stk = &get_mdat(p)->stk;
    for (size_t i = 0; i < stk->sz; i++) {
        if (stk->entries[i].sz > 0) {
            for (size_t j = 0; j < stk->entries[i].sz; j++) {
                free(stk->entries[i].backups[j].data);
            }
            free(stk->entries[i].backups);
        }
        free(stk->entries[i].prots);
    }
    free(stk->entries);
    free(stk->prots);

    size_t npages = stk->npages;
    metadata_t* mdat = get_mdat(p);
    mdat_remove(mdat);
    munmap((void*) mdat, (npages + 1) * page_size);
}

memstk_mapper_t memstk_cow() {
    return (memstk_mapper_t){
        .init = memstk_init,
        .map = memstk_map,
        .push = memstk_push,
        .pop = memstk_pop,
        .free = memstk_free,
    };
}
