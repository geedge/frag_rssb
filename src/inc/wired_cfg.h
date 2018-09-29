#ifndef H_WIRED_CFG_H_INCLUDE
#define H_WIRED_CFG_H_INCLUDE
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define	WCFG_RET_ERR		-1
#define	WCFG_RET_NOT_EXIST	0
#define	WCFG_RET_OK			1

enum WCFG_OPTION
{
	LOCAL_ONLY=0,
	REMOTE_TIMEOUT,
	DCFG_ERR
};

void * wired_cfg_create(const char* app_name, const char* cfg_path);

// return DCFG_RET_xx
int wired_cfg_set_opt(void*handle, enum WCFG_OPTION option, const char* val, size_t size);

// return DCFG_RET_xx
int wired_cfg_init(void* handle);

// convert the value as your own wish with sscanf
// handle		[IN] which aquired by wired_cfg_create
// section	[IN] section name in initialization file
// key		[IN] keyword name in initialization file
// value		[OUT] returned string
// size		[IN] buffer size(bytes)
//default_value[IN] default string
int wired_cfg_read(void*handle, const char* section, const char* key,char* value, size_t size,const char* default_value);

void wired_cfg_destroy(void* handle);
#ifdef __cplusplus
}
#endif
#endif
