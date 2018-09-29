/* Businessman Data Exchange System Timer implement
 * if the user app initialed multi bizman_handler ,they will share a same timer.
*/
#ifndef _BIZMAN_TIME_INCLUDE_H_
#define _BIZMAN_TIME_INCLUDE_H_
const int TIME_TICK_STEP_USEC=100;//microsecond for time accuracy
typedef unsigned long long time_tick_t;
//typedef unsigned int time_tick_t;
time_tick_t get_time_tick();
void bizman_timer_start();
unsigned int generate_stream_id();
#endif //_BIZMAN_TIME_INCLUDE_H_
