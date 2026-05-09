#include "time-logger/shm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    if (!shm_init()) {
        perror("shared memory initialization failed");
        return EXIT_FAILURE;
    }

    printf("Starting...\n");
}
