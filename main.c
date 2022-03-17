#include <stdio.h>

#include "memstk.h"

void dump(char* p, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        printf("%x ", p[i]);
    }
    printf("\n");
}

int main() {
    memstk_init();

    char* p = memstk_alloc(4);
    printf("Allocated: %p\n", p);
    memset(p, 0, 4);

    p[0] = 42;
    dump(p, 4);

    memstk_push(p);

    p[1] = 42;
    dump(p, 4);

    memstk_push(p);
    p[2] = 42;
    dump(p, 4);
    memstk_pop(p);

    dump(p, 4);

    memstk_pop(p);
    dump(p, 4);
}
