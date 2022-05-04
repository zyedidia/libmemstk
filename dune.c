#define _GNU_SOURCE

#include <stdio.h>

#include "libdune/dune.h"
#include "mprot.h"

static void (*handler)(uintptr_t);

static void dune_pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf* tf) {
    (void) fec;
    (void) tf;
    handler(addr);
}

int mprot_init(void (*fault_handler)(uintptr_t)) {
    if (dune_init_and_enter()) {
        printf("failed to initialize dune\n");
        return -1;
    }

    handler = fault_handler;
    dune_register_pgflt_handler(dune_pgflt_handler);
    return 0;
}

int mprot_register_mem(void* p, size_t sz) {
    dune_vm_map_pages(pgroot, p, sz, PERM_R | PERM_W);
    return 0;
}

static int prot2perm(prot_t prot) {
    switch (prot) {
    case PROT_R:
        return PERM_R;
    default:
        return PERM_R | PERM_W;
    }
}
int mprot_protect_mem(void* p, size_t sz, prot_t prot) {
    dune_vm_mprotect(pgroot, (void*) p, sz, prot2perm(prot));
    return 0;
}
