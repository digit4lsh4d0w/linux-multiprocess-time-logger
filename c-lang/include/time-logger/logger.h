#pragma once

extern const char* const LOG_FILE_PATH;

// Универсальная потоко- и процессо-безопасная функция записи в лог.
// Поддерживает форматирование как в `printf`.
void logger_log(const char* format, ...);
