#define _GNU_SOURCE

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int page_size;

typedef struct {
    // if the memory has been written, the original memory is saved here
    void* original;
    // the protections when this stack entry was created
    int prot;
} mement_t;

typedef struct {
    // size of the memory block
    size_t datasz;
    // current protections of the memory block
    int prot;

    // stack entries, each with protections at the time of push and the
    // original data (if modified)
    mement_t* entries;
    size_t sz;
    size_t cap;
} memstk_t;

// magic is currently not used, but could be used for verifying metadata
#define MAGIC 0xdeadbeef

typedef struct {
    memstk_t* stk;
    uint32_t magic;
} metadata_t;

// receive write protection faults here
static void handler(int signo, siginfo_t* info, void* context) {
    (void) signo;
    (void) context;

    // calculate the base of the allocation by rounding down to the nearest
    // page-aligned address
    uintptr_t alloc = (uintptr_t) info->si_addr & ~(page_size - 1);
    metadata_t* mdat = (metadata_t*) alloc;

    size_t top = mdat->stk->sz - 1;
    // make a backup of the data before marking the memory as writable
    void* original = malloc(mdat->stk->datasz);
    memcpy(original, (void*) (alloc + sizeof(metadata_t)), mdat->stk->datasz);
    mdat->stk->entries[top].original = original;
    mdat->stk->prot = PROT_READ | PROT_WRITE;

    mprotect((void*) alloc, mdat->stk->datasz + sizeof(metadata_t),
             PROT_READ | PROT_WRITE);
}

// Must be called before using memstk. Installs a SIGSEGV handler to handle
// attempts to write to mprotected memory.
void memstk_init() {
    page_size = sysconf(_SC_PAGE_SIZE);

    struct sigaction act = {0};

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &handler;
    if (sigaction(SIGSEGV, &act, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

// Allocate a block of sz bytes.
// Requirement: sz <= page_size - sizeof(metadata_t)
void* memstk_alloc(size_t sz) {
    if (sz > page_size - sizeof(metadata_t)) {
        return NULL;
    }

    char* alloc =
        (char*) mmap(NULL, sz + sizeof(metadata_t), PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memstk_t* stk = (memstk_t*) malloc(sizeof(memstk_t));
    *stk = (memstk_t){
        .datasz = sz,
        .prot = PROT_READ | PROT_WRITE,
        .entries = NULL,
        .sz = 0,
        .cap = 0,
    };
    *((metadata_t*) alloc) = (metadata_t){
        .stk = stk,
        .magic = MAGIC,
    };

    return (void*) (alloc + sizeof(metadata_t));
}

static void* get_alloc(void* p) {
    return (char*) p - sizeof(metadata_t);
}

static memstk_t* get_stk(void* p) {
    return ((metadata_t*) get_alloc(p))->stk;
}

static void memstk_double(memstk_t* stk) {
    if (stk->cap == 0) {
        stk->cap = 1;
    } else {
        stk->cap *= 2;
    }
    stk->entries =
        (mement_t*) realloc(stk->entries, sizeof(mement_t) * stk->cap);
}

// Save the current state of allocation 'p'.
void memstk_push(void* p) {
    memstk_t* stk = get_stk(p);

    if (stk->sz >= stk->cap) {
        memstk_double(stk);
    }

    stk->entries[stk->sz] = (mement_t){
        .original = NULL,
        .prot = stk->prot,
    };
    mprotect(get_alloc(p), stk->datasz + sizeof(metadata_t), PROT_READ);
    stk->prot = PROT_READ;
    stk->sz++;
}

// Return allocation 'p' its state at the time of the most recent push.
void memstk_pop(void* p) {
    memstk_t* stk = get_stk(p);
    size_t top = stk->sz - 1;
    if (stk->entries[top].original) {
        memcpy(p, stk->entries[top].original, stk->datasz);
        free(stk->entries[top].original);
    }
    mprotect(get_alloc(p), stk->datasz + sizeof(metadata_t),
             stk->entries[top].prot);
    stk->prot = stk->entries[top].prot;
    stk->sz--;
}

// Free allocation 'p'.
void memstk_free(void* p) {
    memstk_t* stk = get_stk(p);
    for (size_t i = 0; i < stk->sz; i++) {
        if (stk->entries[i].original) {
            free(stk->entries[i].original);
        }
    }
    free(stk->entries);
    free(get_alloc(p));
}
