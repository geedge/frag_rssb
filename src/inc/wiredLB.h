
/*
*****************Wired Load Balancer********
*	Load balance form producer to the consumer.
*	Cooperate with consul, which is a service discovery infrastructure.
*	See document for detail instructions.
*	Author: zhengchao@iie.ac.cn, MESA
*	All right reserved by www.mesalab.cn  2018~2021
*********************************************************
*/

#ifndef H_WIRED_LOAD_BALANCER_H_INCLUDE
#define H_WIRED_LOAD_BALANCER_H_INCLUDE
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define	WLB_CONSUMER		0
#define	WLB_PRODUCER		1
#define	WLB_MAX_TAG_SIZE	1024
#define ADDRSRLEN_MAX		46

enum WLB_OPTION
{
	WLB_OPT_ENABLE_OVERRIDE=0,				// VALUE is an int, 1 for enable, 0 for disable. DEFAULT: 0.
	WLB_OPT_HEALTH_CHECK_PORT, 				// VALUE is a short int, SIZE=sizeof(short int). DEFAULT:52100.
	WLB_OPT_HEALTH_CHECK_INTERVAL, 			// VALUE is a short int, SIZE=sizeof(short int). DEFAULT:10 seconds.
	WLB_CONS_OPT_DATA_PORT,					// VALUE is an unsigned short, SIZE=sizeof(unsigned short). DEFAULT: 0. 
	WLB_CONS_OPT_PRIMARY_ADDR,				// VALUE is a const char*, MUST end with '\0', SIZE= strlen(string+'\0')+1. DEFAULT: use consul agent listen address.
	WLB_CONS_OPT_SECONDARY_ADDR,			// VALUE is a const char*, MUST end with '\0', SIZE= strlen(string+'\0')+1. DEFAULT: no default.
	WLB_CONS_OPT_CAPACITY,					// VALUE is an int that range from 1 to 100, SIZE=sizeof(int). DEFAULT: 32.
	WLB_CONS_OPT_COST,						// VALUE is an int that range from 1 to 100, SIZE=sizeof(int). DEFAULT: 32.
	WLB_CONS_OPT_USER_TAG,					// VALUE is a const char*, MUST end with '\0', SIZE= strlen(string+'\0')+1. DEFAULT: "null". Size must Not exceed WLB_MAX_TAG_SIZE.
	WLB_PROD_OPT_OVERRIDE_PRIMARY_IP,		// VALUE is a const char*, MUST end with '\0', SIZE= strlen(string+'\0')+1. DEFAULT: "null", format: "10.2.0.1-250;123.57.35.100-250;"
	WLB_PROD_OPT_OVERRIDE_SECONDARY_IP,		// VALUE is a const char*, MUST end with '\0', SIZE= strlen(string+'\0')+1. DEFAULT: "null", same format as WLB_PROD_OPT_OVERRIDE_PRIMARY_IP.
	WLB_PROD_OPT_OVERRIDE_DATAPORT,			// VALUE is an unsigned short, SIZE=sizeof(unsigned short). DEFAULT: 0. 
	WLB_PROD_OPT_OVERRIDE_USER_TAG			// Same requirement as WLB_CONS_OPT_USER_TAG.
};
typedef void* WLB_handle_t;

struct WLB_consumer_t
{
	char ip_addr[ADDRSRLEN_MAX];
	unsigned short data_port;
	char user_tag[WLB_MAX_TAG_SIZE];
};
// Lookup is THREAD SAFE.
int wiredLB_lookup(WLB_handle_t handle, const void* key, int len, struct WLB_consumer_t* consumer);

int wiredLB_list(WLB_handle_t handle,size_t n_cons, struct WLB_consumer_t* cons_array);


//Report is THREAD SAFE, NULL is allowed for runtime_info.
void wiredLB_report(WLB_handle_t handle,long proc_bytes, long proc_count, const char* runtime_info);

//[IN] topic, MADATORY, use utf-8 for non English character.
//[IN] group_name, OPTIONALl, could be NULL, use utf-8 for non English character.
//[IN] role, WLB_COSUMER or WLB_PRODUCER
WLB_handle_t wiredLB_create(const char* topic, const char* group_name, int role);
int wiredLB_set_opt(WLB_handle_t handle, enum WLB_OPTION opt, const void* value, size_t size);
int wiredLB_init(WLB_handle_t handle);
void wiredLB_destroy(WLB_handle_t handle);


#ifdef __cplusplus
}
#endif
#endif

