#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_MODE
#define DEBUG_NOMEMLOG

#undef DEBUG_MODE

#ifdef DEBUG_MODE

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

extern FILE *u_log;

void u_init_log();
void u_fina_log();

#define log(...) fprintf(u_log, ##__VA_ARGS__)

#define debug_log_any(data, size)                                              \
    do {                                                                       \
        for (size_t i = 0; i < size; i += 8)                                   \
            log("%zd ", ((long long *)data)[i / 8]);                           \
        log("\n");                                                             \
    } while (0)

#ifndef DEBUG_NOMEMLOG

#define malloc(size)                                                           \
    ({                                                                         \
        void *_ptr = malloc(size);                                             \
        log("malloc %zu %p\n", size, _ptr);                                    \
        _ptr;                                                                  \
    })

#define calloc(num, size)                                                      \
    ({                                                                         \
        void *_ptr = calloc(num, size);                                        \
        log("calloc %zu %zu %p\n", num, size, _ptr);                           \
        _ptr;                                                                  \
    })

#define realloc(ptr, size)                                                     \
    ({                                                                         \
        void *_ptr = realloc(ptr, size);                                       \
        log("realloc %p %zu %p\n", ptr, size, _ptr);                           \
        _ptr;                                                                  \
    })

#define free(_ptr)                                                             \
    ({                                                                         \
        log("free %p\n", _ptr);                                                \
        free(_ptr);                                                            \
    })

#define memcpy(dst, src, n)                                                    \
    ({                                                                         \
        void *_dst = dst;                                                      \
        void *_src = src;                                                      \
        size_t _n = n;                                                         \
        log("memcpy %p %p %zu\n", _dst, _src, _n);                             \
        memcpy(_dst, _src, _n);                                                \
    })

#endif // DEBUG_NOMEMLOG

#else // DEBUG_MODE

#define log(...)

#endif // DEBUG_MODE

#endif // DEBUG_H
