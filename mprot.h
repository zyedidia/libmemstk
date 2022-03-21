#pragma once

#include <sys/mman.h>
#include <stdint.h>

typedef enum {
    PROT_R = PROT_READ,
    PROT_RW = PROT_READ | PROT_WRITE,
} prot_t;

int mprot_init(void (*fault_handler)(uintptr_t));
int mprot_register_mem(void* p, size_t sz);
int mprot_protect_mem(void* p, size_t sz, prot_t prot);
