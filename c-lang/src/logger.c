#include "time-logger/logger.h"
#include "time-logger/shm.h"
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

const char* const LOG_FILE_PATH = "time-logger.log";

void logger_log(const char* format, ...) {
    SharedState* state = shm_get_state();
    if (state == nullptr) {
        return;
    }

    // Блокировка глобального состояния.
    int lock_status = pthread_mutex_lock(&state->global_lock);

    // Если владелец мьютекса умер, восстанавливаем состояние мьютекса.
    if (lock_status == EOWNERDEAD) {
        fprintf(
            stderr,
            "[PID: %d] Обнаружен мертвый владелец мьютекса. "
            "Восстановление...\n",
            getpid()
        );
        pthread_mutex_consistent(&state->global_lock);
    } else if (lock_status != 0 && lock_status != ENOTRECOVERABLE) {
        perror("pthread_mutex_lock failed");
        return;
    }

    FILE* file = fopen(LOG_FILE_PATH, "a");
    if (file != nullptr) {
        struct timespec time_spec;
        clock_gettime(CLOCK_REALTIME, &time_spec);

        struct tm time_info;
        localtime_r(&time_spec.tv_sec, &time_info);

        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &time_info);
        int64_t time_ms = time_spec.tv_nsec / 1'000'000;

        // Сначала вывод обязательной информации.
        fprintf(file, "[%s.%03ld] [PID: %d] ", time_buf, time_ms, getpid());

        va_list args;
        va_start(args, format);
        // Затем вывод информации из опциональных аргументов функции.
        vfprintf(file, format, args);
        va_end(args);

        // В конце выводим символ новой строки.
        fprintf(file, "\n");

        // Очищаем буффер и закрываем файловый дескриптор.
        fflush(file);
        fclose(file);
    } else {
        perror("fopen failed");
    }

    // Разблокировка глобального состояния.
    pthread_mutex_unlock(&state->global_lock);
}
