#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

FILE *log_init(const char *fname);
void log_clear(FILE *fp);
void write_local_log(FILE * fp,const char *fmt,...);

#endif /*_LOG_H_ */
