#ifndef _DEBUG_H_
#define _DEBUG_H_

#include    <stdio.h>
#include    <stdlib.h>
#include    <stdarg.h>

#ifdef _DEBUG_
#define DEBUG_MESSAGE(msg) \
    fprintf(stderr, "[Debug]: %s(%d) " msg "\n", __FILE__, __LINE__)
#define DEBUG_PRINTF(fmt,...) \
    fprintf(stderr, "[Debug]: %s(%d) " fmt, __FILE__, __LINE__, __VA_ARGS__)
#else
#define DEBUG_MESSAGE(msg)
#define DEBUG_PRINTF(fmt,...)
#endif /* _DEBUG_ */

#ifdef _DEBUG_VERBOSE_
#define DEBUG_WRAP(msg) msg 
#else
#define DEBUG_WRAP(msg)
#endif /* _DEBUG_VERBOSE_ */


#endif // #define _DEBUG_H_
