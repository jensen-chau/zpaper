#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define LOG(fmt, ...) do{\
    fprintf(stdout, "[LOG]" fmt "\n", ##__VA_ARGS__);\
}while(0)

#define ERR(fmt, ...) do{\
    fprintf(stderr, "[ERR]" fmt "\n", ##__VA_ARGS__);\
}while(0)

#define UNUSE(x) do{\
    (void)(x);\
}while(0)

#endif
