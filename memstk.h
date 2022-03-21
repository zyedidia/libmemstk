#pragma once

#include <string.h>

typedef struct {
    int (*init)();
    void* (*map)(size_t sz);
    void (*push)(void* p);
    void (*pop)(void* p);
    void (*free)(void* p);
} memstk_mapper_t;

int memstk_init();
void* memstk_map(size_t sz);
void memstk_push(void* p);
void memstk_pop(void* p);
void memstk_free(void* p);
