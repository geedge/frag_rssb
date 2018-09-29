#ifndef _MESA_TRACE_H_
#define _MESA_TRACE_H_

#include <netinet/in.h>            
#include <netinet/ip.h>            
#include <netinet/ip6.h> 

#define H_TRACE_VERSION_1_20150204  0

typedef	void* MESA_trace_addr_handle_t;
typedef	void* MESA_trace_fullstr_handle_t;
typedef	void* MESA_trace_substr_handle_t;
typedef	void* MESA_trace_numerical_handle_t;

struct trace_tuple4_v4
{
	unsigned int 			saddr;
	unsigned int 			daddr;
	unsigned short 		source;
	unsigned short 		dest;
};

#ifndef IPV6_ADDR_LEN
#define IPV6_ADDR_LEN	(sizeof(struct in6_addr))
#endif

struct trace_tuple4_v6
{
	unsigned char 		saddr[IPV6_ADDR_LEN] ;
	unsigned char 		daddr[IPV6_ADDR_LEN] ;
	unsigned short 		source;	
	unsigned short 		dest;
};

enum trace_addr_type_t
{
	_TRACE_ADDR_TYPE_INIT = 0,
	TRACE_ADDR_TYPE_IPV4 = 1,				
	TRACE_ADDR_TYPE_IPV6 = 2,
	TRACE__ADDR_TYPE_IP_PAIR_V4 = 12,      
    TRACE__ADDR_TYPE_IP_PAIR_V6 = 13,
};

/*stream.h*/
typedef struct 
{
	unsigned char addrtype;		
	unsigned char pkttype;
	unsigned char addrlen;
	unsigned char __pad[5];
	union
	{		
		struct trace_tuple4_v4 *tuple4_v4;
		struct trace_tuple4_v6 *tuple4_v6;		
	};
}trace_layer_addr;

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------Part1-------------------------------------------*/
/*
*Paras: 			logger: trace runtime_log; 		 filename: configure file
*Return: 			addr_trace_handle_t, NULL means error.
*configure file form: 	addr_type\tsip\tsource\tdip\tdest  (separator : '\t', ' ', ';')
*such as:
*4	192.168.10.147		125		10.10.6.203		80
*6	10::25:156::5			20662	10::25:156::45	80
*4	0				0		0			0				0 (means all IPV4 tuple4)
*6	0				0		0			0				0 (means all IPV6 tuple4)
*/
MESA_trace_addr_handle_t MESA_trace_create_addr_handle(void* logger, const char *filename);
/*
*Paras:   	handle: addr_trace_handle_t  		paddr:addr
*Return:  	1 when match success, return 0 when match failed
*/
int MESA_trace_match_addr(MESA_trace_addr_handle_t handle, trace_layer_addr *paddr);
void MESA_trace_destory_addr_handle(MESA_trace_addr_handle_t handle);
/*--------------------------------Part2------------------------------------------*/
/*
*Paras: 			logger: trace runtime_log; 		 filename: configure file
*Return: 			trace_substr_handle_t
*configure file form: 	string
*such as:
*192.168.10.123:26662
*10:25::68:58
*then 192.168.10.123:2545-55.25.65.55:123 will match
*/
MESA_trace_substr_handle_t MESA_trace_create_substr_handle(void* logger, const char *filename);
/*
*Paras:   	handle: trace_substr_handle_t ;	str:      str_len:
*Return:  	1 when match success, return 0 when match failed
*func:		sub match
*/
int  MESA_trace_match_substr(MESA_trace_substr_handle_t handle, const char *str, int str_len);
void MESA_trace_destory_substr_handle(MESA_trace_substr_handle_t handle);

/*-------------------------------Part3-------------------------------------------*/
/*
*Paras: 				logger: trace runtime_log;  filename: configure file
*Return: 			trace_fullstr_handle_t
*configure file form:	string
*such as:
*HTTP
*MAIL
*then HTTP will match , HTTP_UP will not match
*/
MESA_trace_fullstr_handle_t MESA_trace_create_fullstr_handle(void* logger, const char *filename);
/*
*Paras:   	handle: addr_trace_handle_t ;  	str:      str_len:
*Return:  	1 when match success, return 0 when match failed
*func:		complete match
*/
int  MESA_trace_match_fullstr(MESA_trace_fullstr_handle_t handle, const char *str, int str_len);
void MESA_trace_destory_fullstr_handle(MESA_trace_fullstr_handle_t handle);


/*-------------------------------Part4-------------------------------------------*/
/*
*Paras: 				logger: trace runtime_log;  filename: configure file
*Return: 			trace_fullstr_handle_t
*configure file form:	string
*such as:
*HTTP
*MAIL
*then HTTP will match , HTTP_UP will not match
*/
MESA_trace_numerical_handle_t MESA_trace_create_numerical_handle(void* logger, const char *filename);

/*
*Paras:   	handle: addr_trace_handle_t ;  	str:      str_len:
*Return:  	1 when match success, return 0 when match failed
*func:		complete match
*/
int  MESA_trace_match_numerial(MESA_trace_numerical_handle_t handle, uint64_t value);
void MESA_trace_destory_numberial_handle(MESA_trace_numerical_handle_t handle);



#ifdef __cplusplus
}
#endif

#endif

