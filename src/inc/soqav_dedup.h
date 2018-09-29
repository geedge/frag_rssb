#ifndef __SOQ_AV_DEDUPLICATION_H__
#define __SOQ_AV_DEDUPLICATION_H__

#include "stream.h"
#include "interval_index.h"

#define BIT_TD_KNOWN 1
#define BIT_TD_MULTI_SOURCE 2

typedef struct __htable_opt
{
	unsigned int htable_num;
	int htable_exp_time;
}htable_opt_t;

typedef struct __addr_node{
	unsigned int ip_nr;  //network order
	unsigned short port_nr; //network order
}addr_node_t ;

typedef struct __query_detail
{
	unsigned long long mid;
	unsigned long total_len;   //节目总长度
	const char *url;
	const char *other;		//其他用于多源TD的串，可为空
	unsigned int urllen;
	unsigned int otherlen;
	union
	{
		struct stream_tuple4_v4 *tuple4_v4;
		struct stream_tuple4_v6 *tuple4_v6;
	};
	unsigned char addrtype; //ADDR_TYPE_IPV4/* 1, 基于IPv4地址的四元组信息 */ADDR_TYPE_IPV6,/* 2, 基于IPv6地址的四元组信息 */
}query_detail_t;

//TODO
typedef struct __scan_detail
{
	long long config_id;
}scan_detail_t;

typedef struct __report_detail
{
	unsigned long long mid;
	scan_detail_t *scan_detail; //命中信息
	unsigned long current_len; //当前上报的有效长度
	unsigned long total_len;   //节目总长度
	union
	{
		struct stream_tuple4_v4 *tuple4_v4;
		struct stream_tuple4_v6 *tuple4_v6;
	};
	IVI_t *ivi_handle;
	const char *file_sfh;
	const char *url;
	const char *other;		//其他用于多源TD的串，可为空
	unsigned int sfh_len;
	unsigned int urllen;
	unsigned int otherlen;
	unsigned char addrtype; //ADDR_TYPE_IPV4/* 1, 基于IPv4地址的四元组信息 */ADDR_TYPE_IPV6,/* 2, 基于IPv6地址的四元组信息 */
}report_detail_t;

typedef enum
{
	DEDUP_TYPE_BALANCE_NUM,	//unsgined int类型，若有多个后级，进行负载均衡，默认一个查询2000次；
	DEDUP_TYPE_QUEUE_NUM,	//unsgined int类型，待查询或上报的队列最大元素个数，默认100000个
	DEDUP_TYPE_STAT_INTERVAL, //unsigned int类型，默认统计周期是300秒，若设置为0，则关闭
	DEDUP_TYPE_THRIFT_TIME, //unsigned int类型，连接THRIFT超时时间，单位是秒。默认5s
	DEDUP_TYPE_HTABLE_ARG,	//htable_opt_t类型，查询本地缓存的哈希表最大元素个数，默认1048576个，超时时间3600s
}DEDUP_OPT_TYPE_t;

typedef void * soqav_dedup_handle_t;

typedef struct __query_result
{
	int status;			//status & BIT_TD_KNOWN：节目重复；status & BIT_TD_MULTI_SOURCE：多源，
	int cpz_mg_ip;		//需要注意多源时主粗拼装IP地址是本机的情况，返回16777343(127.0.0.1)
	int cpz_payload_ip;	//需要注意多源时主粗拼装IP地址是本机的情况，返回16777343(127.0.0.1)
	int reserve; 		//预留
	long long mid;		//mid: 多源时，节目首次出现的mid
}query_result_t;

//user_arg: 用户自定义
typedef long soqav_query_callback_t(const char *td, const query_result_t *result, void *user_arg);

extern "C" int soqav_dedup_set_opt(soqav_dedup_handle_t handle, DEDUP_OPT_TYPE_t type, const void *value, int size);
//local_ifname：本地通信网卡名称，用于回传本地IP地址到服务端。
extern "C" soqav_dedup_handle_t soqav_dedup_plug_init(addr_node_t *addr_array, unsigned int addr_num, int mg_ip, int payload_ip, unsigned int thread_num, void *handle_log);
extern "C" void soqav_dedup_plug_destroy(soqav_dedup_handle_t handle);

//如果TD为NULL，表示只查询多源；
//多源节目不需要单独调用soqav_dedup_report进行上报，其查询即上报，所以疑似多源的节目一定要查询；
extern "C" int soqav_dedup_query(soqav_dedup_handle_t handle, const char *td, const query_detail_t *detail, soqav_query_callback_t *callback_fun, void *user_arg);

//soqav_dedup_report只针对去重，重复的节目不要上报
extern "C" int soqav_dedup_report(soqav_dedup_handle_t handle, const char *td, const report_detail_t *detail);

#endif

