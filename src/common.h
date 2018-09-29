#ifndef _COMMON_H
#define _COMMON_H

#define 		FRAG_REASSEMBLY_MODULE_NAME 		"frag_rssb"
#define			MAX_PATH_LEN						256
#define			SPLIT_BUF_MAXNUM					8   //config split buf cnt

#define			MIN(a,b)								((a)<(b)?(a):(b))
#define			MAX(a,b)								((a)>(b)?(a):(b))

#ifdef __cplusplus
extern "C" {
#endif

int create_pthread(void *(*worker)(void *), void * params, void* logger);
int pag_send_av_udp(int Datasize, const char *pData, u_int src_ip, u_int dst_ip, u_short src_prt, u_short dst_prt);
int string_split (const char* src, char dest[][MAX_PATH_LEN], int dest_maxnum, char sep);
int mkdir_r(const char *path);
int MESA_split_read_IP(char *ipstr, int ipnum, unsigned int *output_ip);
unsigned int get_ip_by_ifname(const char *ifname);
char *memcasemem(const char *strsrc,int len1,const char *substr,int len2);

#ifdef __cplusplus
}
#endif

#endif
