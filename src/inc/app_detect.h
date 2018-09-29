#ifndef __APP_DETECT_H__
#define __APP_DETECT_H__

#define TUPLE4_TYPE_V4 1
#define TUPLE4_TYPE_V6 2

typedef enum __APP_TYPE
{
	APP_TYPE_UNKNOWN=1,
	APP_TYPE_ANDROID,
	APP_TYPE_IOS,
}APP_TYPE_t;

typedef struct __appdtc_detail
{
	char *tuple4;
	char *user_agent;
	char *url;
	void *app_data;
	unsigned int tuple4_len;
	unsigned short app_type; //APP_TYPE_t
	unsigned int ua_len;
	unsigned int url_len;
	unsigned long long data_len;
	unsigned long long appid;
}appdtc_detail_t;

typedef void * appdtc_handle_t;

extern "C" appdtc_handle_t APPDETECT_PLUG_INIT(int thread_num, const char *conf_file_name);
//·µ»ØÖµ: 0-succ
extern "C" int APPDTC_PLUG_ENTRY(appdtc_handle_t handle, appdtc_detail_t *detail, unsigned int thread_id);

#endif

