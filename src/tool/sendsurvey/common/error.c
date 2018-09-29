// error.c 
//
//   
// 
// MODIFIED by
// CREATED by Guo Le, 09/16/2010

#include "error.h"

int error_flags = 0;

void setabort()
{
    error_flags |= ERROR_FLAG_ABORT;
}
void unsetabort()
{
    error_flags &= ~ERROR_FLAG_ABORT;
}
int isabort()
{
    return error_flags & ERROR_FLAG_ABORT ? 1 : 0;
}


