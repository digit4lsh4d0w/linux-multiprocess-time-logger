#include "time-logger/shm.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

const char* const SHM_NAME = "/time-logger";

static SharedState* g_shared_state = nullptr;

SharedState* shm_get_state() {
    return g_shared_state;
}

[[nodiscard]]
bool shm_init() {
    // URL: https://man7.org/linux/man-pages/man3/shm_open.3p.html
    //
    // `shm_open` возвращает либо номер файлового дескриптора, либо -1 при
    // ошибке.
    //
    // Если объект разделяемой памяти не существует - он будет создан,
    // в противном случае будет открыт существующий объект. Этот объект
    // используется для хранения состояния разделяемой памяти.
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, SHM_MODE);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return false;
    }

    // URL: https://man7.org/linux/man-pages/man3/ftruncate.3p.html
    //
    // `ftruncate` возвращает 0 при успешном выполнении, и -1 при ошибке.
    //
    // `ftruncate` используется для изменения размера объекта разделяемой
    // памяти. Только что созданный объект разделяемой памяти имеет размер 0,
    // поэтому `ftruncate` используется для увеличения его до размера структуры
    // `SharedState`.
    if (ftruncate(shm_fd, sizeof(SharedState)) == -1) {
        perror("ftruncate failed");
        close(shm_fd);
        return false;
    }

    // URL: https://man7.org/linux/man-pages/man3/mmap.3p.html
    //
    // `mmap` возвращает либо указатель на отображенный регион памяти, либо
    // `MAP_FAILED` при ошибке.
    //
    // `mmap` используется для отображения региона памяти из объекта разделяемой
    // памяти в адресное пространство процесса.
    //
    // **Для себя**
    //
    // - Передача нулевого указателя первым аргументом означает смаппить
    //   разделяемую область памяти в адресное пространство программы
    //   автоматически, на выбор ядра. То есть мне не нужно указывать какой-то
    //   магический адрес памяти, который может перекрываться стеком, кучей или
    //   еще чем-то.
    // - Размер выделяемых данных, который будет округляться в большую сторону
    //   по отношению к размеру страницы.
    // - Настройка разрешений, которые регулируют что можно производить с этой
    //   памятью. В моем случае мне нужно в нее лишь писать и читать из нее.
    // - Область видимости изменений. В режиме `MAP_SHARED` память честно
    //   разделена между несколькими потребителями - все изменения видны всем.
    //   Есть вариант `MAP_PRIVATE`, работающий по принципу CoW (Copy-on-Write),
    //   когда данные будут скопированы, если потребитель попытается их
    //   изменить. Глобальные значения не изменятся, а конкретный процесс
    //   получит себе отдельную копию, которую он изменил. В Linux (но не в
    //   POSIX) еще есть `MAP_ANONYMOUS`, реализующий разделение памяти (с
    //   помощью MAP_SHARED) без файла, и другие опции.
    // - Файловый дескриптор, в моем случае `shm_fd` - файловый дескриптор
    //   разделяемой памяти, который был открыт ранее с помощью `shm_open`.
    // - Смещение внутри файла, доступного по дескриптору, в моем случае `0` -
    //   начало файла.
    void* mapped_mem = mmap(
        nullptr,
        sizeof(SharedState),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    if (mapped_mem == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        return false;
    }

    g_shared_state = (SharedState*) mapped_mem;
    close(shm_fd);

    bool expected = false;
    if (atomic_compare_exchange_strong(
            &g_shared_state->is_initialized,
            &expected,
            true
        )) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);

        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

        pthread_mutex_init(&g_shared_state->global_lock, &attr);
        pthread_mutexattr_destroy(&attr);

        atomic_init(&g_shared_state->counter, 0);
        atomic_init(&g_shared_state->leader_pid, 0);
        atomic_init(&g_shared_state->copy_1_pid, 0);
        atomic_init(&g_shared_state->copy_2_pid, 0);
    }

    return true;
}

void shm_deinit() {
    if (g_shared_state == nullptr) {
        return;
    }

    // Если произошла ошибка во время удаления проекции общей памяти в
    // адресное пространство процесса я просто логгирую это сообщение, т.к.
    // другого вариант просто нет, а ОС в любом случае при завершении
    // программы все подчистит.
    if (munmap(g_shared_state, sizeof(SharedState)) == -1) {
        perror("munmap failed");
    }
    g_shared_state = nullptr;
}

void shm_delete() {
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink failed");
    }
}
