#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "log.h"

FILE *log_init(const char *fname)
{
	FILE *fp = fopen(fname,"ab+");
	if (!fp) {
		fprintf(stderr, "log init error: %s\n", strerror(errno));
		exit(0);
	}
	return fp;
}

void log_clear(FILE *fp)
{
	if (fp)
		fclose(fp);
}

void write_local_log(FILE * fp,const char *fmt,...)
{
	if(!fp) return;
	struct tm * now;
	time_t currTime;
	time(&currTime);
	char strTime[32];
	now = localtime(&currTime);

	if(!fp) return ;
	memset(strTime,0,sizeof(strTime));
	strftime(strTime,sizeof(strTime),"%Y-%m-%d %H:%M:%S",now);
	fprintf(fp,"%s\t",strTime);

	va_list ap;

	va_start(ap,fmt);
	vfprintf(fp,fmt,ap);
	va_end(ap);

	fputc('\n',fp);
	fflush(fp);

}// end of WriteLocalLog

