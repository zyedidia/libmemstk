#pragma once

#include <string.h>

int memstk_init();
void* memstk_alloc(size_t sz);
void memstk_push(void* p);
void memstk_pop(void* p);
void memstk_free(void* p);
