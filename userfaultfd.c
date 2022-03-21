#define _GNU_SOURCE

#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "protect.h"

static long uffd;

typedef struct {
    long uffd;
    void (*fault_handler)(uintptr_t);
} thread_args_t;

extern int page_size;

static void* fault_handler_thread(void* arg) {
    thread_args_t* args = (thread_args_t*) arg;

    while (1) {
        struct pollfd pollfd = (struct pollfd){
            .fd = args->uffd,
            .events = POLLIN,
        };
        printf("Entering poll\n");
        int nready = poll(&pollfd, 1, -1);
        if (nready == -1) {
            exit(EXIT_FAILURE);
        }

        printf("fault_handler_thread\n");
        exit(EXIT_FAILURE);
        /* args->fault_handler(); */
    }
}

int mprot_init(void (*fault_handler)(uintptr_t)) {
    uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd == -1) {
        return -1;
    }

    struct uffdio_api uffdio_api = (struct uffdio_api){
        .api = UFFD_API,
        .features = UFFD_FEATURE_PAGEFAULT_FLAG_WP,
    };
    if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1) {
        return -1;
    }

    thread_args_t* targs = malloc(sizeof(thread_args_t));

    targs->uffd = uffd;
    targs->fault_handler = fault_handler;

    pthread_t thr;
    int s = pthread_create(&thr, NULL, fault_handler_thread, (void*) targs);
    if (s != 0) {
        return -1;
    }

    return 0;
}

int mprot_register_mem(void* p, size_t sz) {
    struct uffdio_register reg = (struct uffdio_register){
        .range =
            {
                .start = (unsigned long) p,
                .len = sz,
            },
        .mode = UFFDIO_REGISTER_MODE_WP,
    };
    int r = ioctl(uffd, UFFDIO_REGISTER, &reg);
    if (r == -1) {
        return -1;
    }

    // touch every page in the memory to register it (undocumented "feature").
    char* b = (char*) p;
    for (size_t i = 0; i < sz; i += page_size) {
        b[i] = 0;
    }

    return 0;
}

int mprot_protect_mem(void* p, size_t sz, prot_t prot) {
    struct uffdio_writeprotect wp = (struct uffdio_writeprotect){
        .range =
            {
                .start = (unsigned long) p,
                .len = sz,
            },
        .mode = prot == PROT_R ? UFFDIO_WRITEPROTECT_MODE_WP : 0,
    };
    return ioctl(uffd, UFFDIO_WRITEPROTECT, &wp);
}
