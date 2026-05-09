#pragma once

#include <stdatomic.h>
#include <sys/types.h>

// Название "файла" разделяемой памяти.
extern const char* const SHM_NAME;
// Разрешения "файла" разделяемого файла, если он будет создан в процессе
// открытия.
constexpr mode_t SHM_MODE = 0660;

// Структура, которая будет располагаться в разделяемой памяти.
typedef struct {
    pthread_mutex_t global_lock;
    atomic_int_fast64_t counter;
    _Atomic pid_t leader_pid;
    _Atomic pid_t copy_1_pid;
    _Atomic pid_t copy_2_pid;
    atomic_bool is_initialized;
} SharedState;

// Геттер для взятия глобального состояния всех программ.
[[nodiscard]]
SharedState* shm_get_state();

// Инициализация разделяемой памяти.
//
// При необходимости она будет создана.
//
// Возвращает:
//
// - `true`, если инициализация прошла успешно.
// - `false`, если произошла ошибка.
[[nodiscard]]
bool shm_init();

// Деинициализация разделяемой памяти.
//
// В результате разделяемая память НЕ будет удалена, будет удалена только
// проекция фрагмента памяти в данный экземпляр программы.
void shm_deinit();

// Удаление разделяемой памяти.
//
// В результате разделяемая память будет удалена.
void shm_delete();
