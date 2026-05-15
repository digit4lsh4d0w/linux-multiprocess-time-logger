#include "time-logger/leader.h"
#include "time-logger/logger.h"
#include "time-logger/shm.h"
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

constexpr uint64_t MAX_EVENTS = 10;

static sig_atomic_t g_running = 1;

static void signal_handler(int signum) {
    (void) signum;
    g_running = 0;
}

// Функция инициализации обработчика сигналов.
static void setup_signals() {
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Устанавливаем обработчики сигналов для SIGINT и SIGTERM.
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

[[nodiscard]]
static int32_t create_timer(int64_t interval_ms) {
    int32_t timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd == -1) {
        return -1;
    };

    struct itimerspec timer_spec = {0};
    timer_spec.it_value.tv_sec = interval_ms / 1000;
    timer_spec.it_value.tv_nsec = (interval_ms % 1000) * 1'000'000;
    timer_spec.it_interval = timer_spec.it_value;

    if (timerfd_settime(timer_fd, 0, &timer_spec, nullptr) == -1) {
        close(timer_fd);
        return -1;
    }

    return timer_fd;
}

static bool epoll_add(int32_t epoll_fd, int32_t fd, uint32_t events) {
    struct epoll_event event = {
        .events = events,
        .data.fd = fd,
    };

    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) != -1;
}

int main(int argc, char* argv[]) {
    // Обработка аргументов.
    for (int i = 1; i < argc; i++) {
        // Если есть флаг `--cleanup`, то программа удаляет объект разделяемой
        // памяти и завершается.
        if (strcmp(argv[i], "--cleanup") == 0) {
            printf("Удаление объекта разделяемой памяти...\n");
            shm_delete();
            return EXIT_SUCCESS;
        }
    }

    setup_signals();

    if (!shm_init()) {
        perror("Ошибка инициализации разделяемой памяти.\n");
        return EXIT_FAILURE;
    }

    logger_log("Программа запущена!");

    int32_t epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        return EXIT_FAILURE;
    }

    int32_t timer_300ms = create_timer(300);
    int32_t timer_1s = create_timer(1000);
    int32_t timer_3s = create_timer(3000);

    epoll_add(epoll_fd, timer_300ms, EPOLLIN);
    epoll_add(epoll_fd, timer_1s, EPOLLIN);
    epoll_add(epoll_fd, timer_3s, EPOLLIN);
    epoll_add(epoll_fd, STDIN_FILENO, EPOLLIN);

    struct epoll_event events[MAX_EVENTS];

    while (g_running) {
        leader_try_elect();

        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (num_events == -1) {
            // Проверка, не был ли epoll прерван системным сигналом.
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait failed");
            break;
        }

        for (int32_t i = 0; i < num_events; i++) {
            int32_t fd = events[i].data.fd;

            if (fd == timer_300ms) {
                uint64_t expirations;
                read(fd, &expirations, sizeof(expirations));

                // TODO: Counter
            } else if (fd == timer_1s) {
                uint64_t expirations;
                read(fd, &expirations, sizeof(expirations));

                if (leader_is_me()) {
                    // TODO: Write state to log
                    SharedState* state = shm_get_state();
                    int64_t current_counter = atomic_load(&state->counter);

                    logger_log("Текущее значение счетчика: %lu", current_counter);
                }
            } else if (fd == timer_3s) {
                uint64_t expirations;
                read(fd, &expirations, sizeof(expirations));

                if (leader_is_me()) {
                    // TODO: Write state to log
                }
            } else if (fd == STDIN_FILENO) {
                char buf[256];
                ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1);
                if (bytes > 0) {
                    buf[bytes] = '\0';
                    // TODO: Parse num and update counter
                    printf("Вы ввели: %s", buf);
                }
            }
        }
    }

    printf("[PID: %d] Завершение работы...\n", getpid());

    // Закрытие всех дескрипторов.
    close(timer_300ms);
    close(timer_1s);
    close(timer_3s);
    close(epoll_fd);

    leader_resign();
    shm_deinit();

    return EXIT_SUCCESS;
}
