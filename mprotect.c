#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "mprot.h"

int mprot_register_mem(void* p, size_t sz) {
    (void) p;
    (void) sz;

    return 0;
}

int mprot_protect_mem(void* p, size_t sz, prot_t prot) {
    return mprotect(p, sz, (int) prot);
}

static void (*handler)(uintptr_t);

static void sighandler(int signo, siginfo_t* info, void* context) {
    (void) signo;
    (void) context;

    handler((uintptr_t) info->si_addr);
}

int mprot_init(void (*fault_handler)(uintptr_t)) {
    struct sigaction act = {0};

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &sighandler;
    if (sigaction(SIGSEGV, &act, NULL) == -1) {
        return -1;
    }
    handler = fault_handler;

    return 0;
}
