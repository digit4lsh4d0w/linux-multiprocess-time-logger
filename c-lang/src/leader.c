#include "time-logger/leader.h"
#include "time-logger/shm.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

bool leader_is_me() {
    SharedState* state = shm_get_state();
    if (state == nullptr) {
        return false;
    }

    return atomic_load(&state->leader_pid) == getpid();
}

bool leader_try_elect() {
    SharedState* state = shm_get_state();
    if (state == nullptr) {
        return false;
    }

    pid_t my_pid = getpid();
    pid_t current_leader_pid = atomic_load(&state->leader_pid);

    // Если текущий процесс и так лидер - выход.
    if (current_leader_pid == my_pid) {
        return true;
    }

    // Если лидер еще не был избран - попытаться занять позицию.
    if (current_leader_pid == 0) {
        // Ожидается, что переключения контекста еще не произошло и никто не
        // занял позицию лидера.
        pid_t expected = 0;
        if (atomic_compare_exchange_strong(
                &state->leader_pid,
                &expected,
                my_pid
            )) {
            printf("[PID: %d] Я стал новым лидером\n", my_pid);
            return true;
        }

        current_leader_pid = expected;
    }

    // URL: https://man7.org/linux/man-pages/man3/kill.3p.html
    //
    // Проверка существования процесса с заданным PID.
    if (kill(current_leader_pid, 0) == -1 && errno == ESRCH) {
        if (atomic_compare_exchange_strong(
                &state->leader_pid,
                &current_leader_pid,
                my_pid
            )) {
            printf(
                "[PID: %d] Предыдущий лидер (%d) мертв, теперь лидер я\n",
                my_pid,
                current_leader_pid
            );
            return true;
        }
    }

    return false;
}

void leader_resign() {
    SharedState* state = shm_get_state();
    if (state == nullptr) {
        return;
    }

    pid_t my_pid = getpid();
    atomic_compare_exchange_strong(&state->leader_pid, &my_pid, 0);
}
