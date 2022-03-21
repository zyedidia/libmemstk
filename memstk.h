#pragma once

#include <string.h>

typedef struct {
    int (*init)();
    void* (*map)(size_t sz);
    void (*push)(void* p);
    void (*pop)(void* p);
    void (*free)(void* p);
} memstk_mapper_t;

memstk_mapper_t memstk_cow();
memstk_mapper_t memstk_basic();
