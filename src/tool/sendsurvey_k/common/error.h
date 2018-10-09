#ifndef _ERROR_H_
#define _ERROR_H_
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdarg.h>

#define ERROR_FLAG_ABORT 0x1

#define ERROR_MESSAGE(msg) \
    fprintf(stderr, "[Error]: " msg "\n")

void setabort();
void unsetabort();
int isabort();


#endif // #define _DEBUG_H_
