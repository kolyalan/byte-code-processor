#pragma once
#include <cstdio>
#include <cstdarg>

static FILE * log = NULL;
char default_log_file[] = "log.txt";

bool log_init(const char *filename = NULL) {
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (filename == NULL) {
        log = fopen(default_log_file, "w");
    } else {
        log = fopen(filename, "w");
    }
    if (log == NULL) {
        return 0;
    }
#endif
    return 1;
}

void log_close() {
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (log != NULL) {
        fclose(log);
    }
#endif
}

int log_printf(const char *format, ...) {
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (log == NULL) {
        return 0;
    }
    va_list vl;
    va_start(vl, format);
    int n = vfprintf(log, format, vl);
    fflush(log);
    va_end(vl);
    return n;
#endif
    return 0;
}
