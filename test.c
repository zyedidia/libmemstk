#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "memstk.h"

#define DEBUG 0

typedef struct {
    memstk_mapper_t* mappers;
    char** ptrs;
    size_t n;
    size_t sz;
} xcheck_t;

static void xcheck_init(xcheck_t* x) {
    for (size_t i = 0; i < x->n; i++) {
        if (x->mappers[i].init() == -1) {
            perror("init");
            exit(EXIT_FAILURE);
        }
    }
}

static void xcheck_map(xcheck_t* x, size_t sz) {
    assert(x->sz == 0);

    for (size_t i = 0; i < x->n; i++) {
        void* p = x->mappers[i].map(sz);
        if (!p) {
            perror("map");
            exit(EXIT_FAILURE);
        }
        x->ptrs[i] = p;
        memset(p, 0, sz);
    }
    x->sz = sz;
}

static void xcheck_check(xcheck_t* x) {
    for (size_t i = 1; i < x->n; i++) {
        if (memcmp(x->ptrs[i], x->ptrs[0], x->sz) != 0) {
            for (size_t j = 0; j < x->sz; j++) {
                if (x->ptrs[0][j] != x->ptrs[i][j]) {
                    printf("%ld: %x %x\n", j, x->ptrs[0][j], x->ptrs[i][j]);
                }
            }
            assert(memcmp(x->ptrs[i], x->ptrs[0], x->sz) == 0);
        }
    }
}

static void xcheck_modify(xcheck_t* x) {
    if (DEBUG) printf("modify\n");
    const int nchanges = 100;
    for (int j = 0; j < nchanges; j++) {
        unsigned idx = rand() % x->sz;
        unsigned val = rand();
        for (size_t i = 0; i < x->n; i++) {
            x->ptrs[i][idx] = val;
        }
    }
}

static void xcheck_push(xcheck_t* x) {
    if (DEBUG) printf("push\n");
    for (size_t i = 0; i < x->n; i++) {
        x->mappers[i].push(x->ptrs[i]);
    }
}

static void xcheck_pop(xcheck_t* x) {
    if (DEBUG) printf("pop\n");
    for (size_t i = 0; i < x->n; i++) {
        x->mappers[i].pop(x->ptrs[i]);
    }
}

static void xcheck_free(xcheck_t* x) {
    for (size_t i = 0; i < x->n; i++) {
        x->mappers[i].free(x->ptrs[i]);
    }
}

int main() {
    memstk_mapper_t mappers[2] = {memstk_basic(), memstk_cow()};
    char* ptrs[2];
    xcheck_t x = (xcheck_t){
        .mappers = mappers,
        .ptrs = ptrs,
        .n = sizeof(mappers) / sizeof(memstk_mapper_t),
        .sz = 0,
    };
    xcheck_init(&x);
    int page_size = sysconf(_SC_PAGE_SIZE);
    const int npages = 10;
    xcheck_map(&x, page_size * npages);

    const int nops = 10000;

    for (int i = 0; i < nops; i++) {
        int op = rand() % 3;

        switch (op) {
            case 0:
                xcheck_modify(&x);
                break;
            case 1:
                xcheck_push(&x);
                break;
            case 2:
                xcheck_pop(&x);
                break;
        }
        xcheck_check(&x);
    }

    xcheck_free(&x);

    printf("PASS\n");
}
