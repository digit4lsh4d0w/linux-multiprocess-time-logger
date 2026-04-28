#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct SharedData {
    _Atomic int counter;
};

int main() {
    const char* shm_name = "/my_app_shared_mem";

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("shm_open error");
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_fd, sizeof(struct SharedData)) == -1) {
        perror("ftruncate error");
        return EXIT_FAILURE;
    }

    void* map_ptr = mmap(
        nullptr,
        sizeof(struct SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    printf("Starting...");
}
