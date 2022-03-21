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
    memstk_mapper_t memstk = memstk_cow();

    if (memstk.init() == -1) {
        perror("memstk.init");
        exit(EXIT_FAILURE);
    }

    char* p = memstk.map(4);
    if (!p) {
        perror("memstk.map");
        exit(EXIT_FAILURE);
    }

    printf("Allocated: %p\n", p);
    memset(p, 0, 4);

    p[0] = 42;
    dump(p, 4);

    memstk.push(p);

    p[1] = 42;
    dump(p, 4);

    memstk.push(p);
    p[2] = 42;
    dump(p, 4);
    memstk.pop(p);

    dump(p, 4);

    memstk.pop(p);
    dump(p, 4);

    memstk.free(p);
}
