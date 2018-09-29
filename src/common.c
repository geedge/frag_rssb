#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stddef.h>

#include "common.h"
#include "stream.h"
#include "MESA_handle_logger.h"


int pag_send_av_udp(int Datasize, const char *pData, u_int src_ip, u_int dst_ip, u_short src_prt, u_short dst_prt)
{
	printf("empty function,should not be called by preprocesosor!\n");
	exit(-1);
	return 0;
}

int create_pthread(void *(*worker)(void *), void * params, void* logger)
{
    pthread_t	 	thread_desc;
	pthread_attr_t 	attr;

	memset(&thread_desc, 0, sizeof(thread_desc));
	memset(&attr, 0, sizeof(attr));	
	
	if(0 != pthread_attr_init(&(attr))) 
    {
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, "pthread_attr_init(): %d %s", errno, strerror(errno));
		return -1;
	}
	if(0 != pthread_attr_setdetachstate(&(attr), PTHREAD_CREATE_DETACHED)) 
    {		
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, "pthread_attr_setdetachstate(): %d %s", errno, strerror(errno));
		return -1;
	}
	if(0 != pthread_create(&(thread_desc), &(attr), (void *(*)(void *))(worker), (void *)(params))) 
    {
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, "pthread_create(): %d %s", errno, strerror(errno));
		pthread_attr_destroy(&(attr));
		return -1;
	}
	pthread_attr_destroy(&(attr));
	return 0;
}

int string_split (const char* src, char dest[][MAX_PATH_LEN], int dest_maxnum, char sep)
{
	if(NULL==src) return 0;
	int			dest_cnt = 0;
	const char*	start_token = src;
	const char*	end_token = src;
	const char*	end_pos = src+strlen(src);
	
	while(end_token<end_pos&&dest_cnt<dest_maxnum)
	{
		end_token = (char*)memchr(start_token, sep, end_pos-start_token);
		if(end_token!=NULL)
		{
			memcpy(dest[dest_cnt], start_token, end_token-start_token);
			start_token = end_token+1;
			end_token += 1;
			dest_cnt++;
		}
		else 
		{
			memcpy(dest[dest_cnt], start_token, end_pos-start_token);			
			end_token = end_pos;			
			dest_cnt++;
		}
	}
	return dest_cnt;
}

int mkdir_r(const char *path)
{
	int i=0, len=0;
	int ret=0;
	len = strlen(path);
	char dir_path[len+1];
	dir_path[len] = '\0';

	strncpy(dir_path, path, len);

	for (i=0; i<len; i++)
	{
		if (dir_path[i] == '/' && i > 0)
		{
			dir_path[i]='\0';
			if (access(dir_path, F_OK) < 0)
			{
				ret=mkdir(dir_path,0755);
				if (ret < 0)
				{
					printf("mkdir=%s:msg=%s\n", dir_path, strerror(errno));					
					return -1;
				}
			}
			dir_path[i]='/';
		}
	}
	return 0;
}

//read ip from string like "127.0.1-32;"
int MESA_split_read_IP(char *ipstr, int ipnum, unsigned int *output_ip)
{
    char          buf[1024] = { 0 };
    char          *begin, *end;
    int    i;
    int              rc = 0;
    begin        = ipstr;
    end  = NULL;
    for(i = 0; i < ipnum; )
    {
        if(NULL != (end = strchr(begin, ';') ) )
        {
            memset(buf, 0, 524);
            strncpy(buf, begin, end - begin);
            begin = end + 1;
            char *pchr        = strchr(buf, '-');
            int    tmp  = 0, tmp2 = 0;
            if(pchr == NULL)
            {
                tmp = 1;
            }
            else
            {
                char *ptmp2 = strrchr(buf, '.');
                tmp2         = atoi(ptmp2 + 1);
                tmp  = atoi(pchr + 1);
                tmp  = tmp - tmp2 + 1;
                *pchr        = '\0';
                if(tmp <= 0)
                {
                    printf("MESA_split_read_IP Bad bsendip format %s\n", ipstr);
                }
            }
            rc = inet_addr(buf);
            if(rc == -1)
            {
                printf("MESA_split_read_IP Bad bsendip format %s\n", ipstr);
            }
            unsigned int tmpid = ntohl(rc);
            for(int j = 0; j < tmp; j++)
            {
                if(i >= ipnum)
                {
                    printf("Bad bsendip format,bsendipnum  %d not right\n", ipnum);
                }
                output_ip[i] = (unsigned int)htonl( (tmpid + j) );
                i++;
            }
        }
    }
    return(0);
}

unsigned int get_ip_by_ifname(const char *ifname)
 {
	 int sockfd;
	 struct ifreq ifr;
	 unsigned int ip;
 
	 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	 if (-1 == sockfd) 
	 {
		 goto error;
	 }
 
	 strcpy(ifr.ifr_name,ifname); 
	 if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) 
	 {
		 goto error;
	 }
 
	 ip = ((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr.s_addr;
	 close(sockfd);
	 return ip;
 
 error:
	 close(sockfd);
	 return INADDR_NONE;
 }

/* this function is copy from other system. */
/*
char *memcasemem(const char *strsrc,int len1,const char *substr,int len2)
{
		
	char *p1,*p2,*pend;
	char *p;
	char *substrS;
	int i,lenth;

	if((strsrc==NULL)||substr==NULL)
		return NULL;
	if(len1<len2) return NULL;

	lenth=len2;
	substrS=(char *)malloc(sizeof(char)*(lenth+1));
	if(substrS==NULL)
		return NULL;
	for(i=0;i<lenth;i++)
	{
		substrS[i]=tolower(*(substr+i));
	}
	substrS[lenth]=0;

	lenth=len1;
	pend =(char *)strsrc + lenth;
	for(p1 =(char *) strsrc; p1 != pend;p1++)
	{
		for(p2 = p1,p = substrS;*p != '\0'&& p2 != pend;p++,p2++)
		{
			if(tolower(*p2) != *p)
				break;
		}
		if(*p == '\0')
		{
			free(substrS);
			return p1;
		}
	}

	free(substrS);
	return NULL;
}	
*/
/*from sapp*/
extern "C" const char *printaddr (const struct layer_addr *paddrinfo, int threadindex)																		  
{
	static char maxbuf[64+1][128];
	char *buf=maxbuf[threadindex];
	char ip_str[64];
	struct stream_tuple4_v4 *paddr;
	struct stream_tuple4_v6 *paddr6;
 
	if(NULL == paddrinfo){
	 return NULL;
	}
 
	if(paddrinfo->addrtype==ADDR_TYPE_IPV4)
	{
		paddr=(struct stream_tuple4_v4 *)paddrinfo->paddr;
		memset(buf,0,64);
		//strcpy (buf, int_ntoa (paddr->saddr));
		inet_ntop(AF_INET, &paddr->saddr, ip_str, 64);
		strcpy (buf, ip_str);
		sprintf (buf + strlen (buf), ".%u>", ntohs(paddr->source));
		//strcat (buf, int_ntoa (paddr->daddr));
		inet_ntop(AF_INET, &paddr->daddr, ip_str, 64);
		strcat (buf, ip_str);
		sprintf (buf + strlen (buf), ".%u", ntohs(paddr->dest));
	}
	//to addjust
	else	 if(paddrinfo->addrtype==ADDR_TYPE_IPV6)
	{
		paddr6=(struct stream_tuple4_v6 *)(paddrinfo->paddr);
		memset(buf,0,128);
		inet_ntop(AF_INET6, paddr6->saddr, ip_str, 64);
		strcpy (buf, ip_str);
		sprintf (buf + strlen (buf), ".%u>", ntohs(paddr6->source));
		inet_ntop(AF_INET6, paddr6->daddr, ip_str, 64);
		strcat (buf, ip_str);
		sprintf (buf + strlen (buf), ".%u", ntohs(paddr6->dest));
	}
	else
	{
		return (const char *)"Not support layer type";
	}
	return buf;
}
	 


