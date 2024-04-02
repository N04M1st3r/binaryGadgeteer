#include <stdio.h>
#include "costumErrors.h"

int err(char const *fmt, ...){
    va_list args;

    fprintf(stderr, "Error!");
    if(fmt){
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }else{
        fprintf(stderr, "%s", strerror(errno));
    }

    fputc('\n', stderr);
    return 0;
}

