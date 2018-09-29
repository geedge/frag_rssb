#ifndef _APPDSENDER_H
#define _APPDSENDER_H

#include "opt.h"
#include "MESA_tcp_interface.h"

typedef void* appdsender_t;

#ifdef __cplusplus
extern "C" {
#endif

/*gloable init*/
int 			init_appdsender(int iovalue, int thread_num, uint32 local_ip_nr);
void			release_appdsender();

appdsender_t 	creat_appdsender(uint32 sendip, uint16 port, char is_trace, const char* fileinfo,int thread_seq);
int				appdsender(appdsender_t  handle, opt_t opt, int thread_seq);
void 			destory_appdsender(appdsender_t  handle, int thread_seq);

#ifdef __cplusplus
}
#endif

#endif
