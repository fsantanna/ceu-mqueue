#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>

#include "ceu_types.h"

#define MSGSIZE 1024

enum {
    QU_LINK   = -1,
    QU_UNLINK = -2,
    QU_WCLOCK = -10,
    QU_ASYNC  = -11,
};

#define ASR(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \


