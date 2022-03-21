#include <stdio.h>
#include <stdlib.h>

#include "memstk.h"

void dump(char* p, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        printf("%x ", p[i]);
    }
    printf("\n");
}

int main() {
    if (memstk_init() == -1) {
        perror("memstk_init");
        exit(EXIT_FAILURE);
    }

    char* p = memstk_map(4);
    if (!p) {
        perror("memstk_map");
        exit(EXIT_FAILURE);
    }

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

    memstk_free(p);
}
