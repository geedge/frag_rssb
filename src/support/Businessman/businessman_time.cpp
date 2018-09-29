#include <signal.h>
#include <unistd.h>
//#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>//gettimeofday
#include "businessman_time.h"
#include "businessman_limit.h"

static volatile unsigned int g_stream_id;//iatomic.h do not support atomic_inc_return operation in X86_64 platform

static struct timeval start_tv;
int initial_ok=FALSE;
time_tick_t get_time_tick(){
	struct timeval current_tv;
	time_tick_t ticks=0;
	gettimeofday(&current_tv,NULL);
	ticks=(1000000*(current_tv.tv_sec-start_tv.tv_sec)+(current_tv.tv_usec-start_tv.tv_usec))/TIME_TICK_STEP_USEC;
	return ticks;
}
unsigned int generate_stream_id()
{
	return g_stream_id++;
}
void bizman_timer_start(){
	if(initial_ok==FALSE)
	{
		gettimeofday(&start_tv,NULL);
		initial_ok=TRUE;
		g_stream_id=1;
	}
}

