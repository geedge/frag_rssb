#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "MESA_trace.h"
#include "MESA_handle_logger.h"

int MESA_TRACE_VERSION_V1_20150804 = 0;
void MESA_trace_history()
{
	//2014.11.14 V1 create the project by lishu
	//2014.12.16 V1 add MESA_trace	
	//2015.02.04 V1 MESA_trace.h trace_layer_addr		
	//2015.08.04 V1 trace_match_addr6 v6 addr 	
}

#define TRACE_MODULE_NAME			"MESA_TRACE"

/*if more than MAX_NUM, will ignore*/
#define MAX_NUMERICAL_TRACE_RULE		(1024) 
#define MAX_ADDR_TRACE_RULE				(256) 
#define MAX_STR_TRACE_RULE				(128) 
#define MAX_TRACE_LEN					(128) 

typedef struct 
{
	uint64_t 			numerical[MAX_NUMERICAL_TRACE_RULE];
	int					num;		
}trace_numerical_handle_nel_t;

typedef struct 
{
	char 				str[MAX_STR_TRACE_RULE][MAX_TRACE_LEN];
	int					str_num;		
}trace_substr_handle_nel_t;

typedef struct 
{
	char 				str[MAX_STR_TRACE_RULE][MAX_TRACE_LEN];
	int					str_num;		
}trace_fullstr_handle_nel_t;

typedef struct 
{
	struct trace_tuple4_v4 	addr4[MAX_ADDR_TRACE_RULE];
	struct trace_tuple4_v6 	addr6[MAX_ADDR_TRACE_RULE];
	int					addr4_num;
	int					addr6_num;		
}trace_addr_handle_nel_t;


/* É¾³ýÄ©Î²µÄ»»ÐÐ·û"\r\n" */
void trace_del_last_rn(char *data)
{
	int len = strlen(data)-1;
    int i;
    for(i = len; i >=0 ; i--)
	{
        if(('\r' != data[i]) && ('\n' != data[i]))
		{
            data[i+1] = '\0';
            return;
        }
    }
    return;
}

/* this function is copy from other system. */
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


MESA_trace_numerical_handle_t MESA_trace_create_numerical_handle(void* logger, const char *filename)
{
	FILE *fp;
	char conf_buf[MAX_TRACE_LEN];
	trace_numerical_handle_nel_t* handle = NULL;	
	
	fp = fopen(filename, "r");
	if(NULL == fp)
	{
		MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"open %s error.", filename);
	    return NULL;
	}
	handle = (trace_numerical_handle_nel_t*)calloc(1, sizeof(trace_numerical_handle_nel_t));
	while(fgets(conf_buf, MAX_TRACE_LEN, fp) != NULL)
	{
		if('#' == conf_buf[0] || '\r' == conf_buf[0] || '\n' == conf_buf[0])
		{
			continue;
		}		
		trace_del_last_rn(conf_buf);
		handle->numerical[handle->num] = strtoul(conf_buf, NULL, 10);
		handle->num++;
	}

	fclose(fp);
	return (MESA_trace_numerical_handle_t)handle;
}

int MESA_trace_match_numerial(MESA_trace_numerical_handle_t handle, uint64_t value)
{
	trace_numerical_handle_nel_t* handler = (trace_numerical_handle_nel_t*)handle;
    int i;
	for(i = 0; i<handler->num; i++)
	{
		if(handler->numerical[i] == value)
		{
			return 1;
		}		
	}
	return 0;
}

void MESA_trace_destory_numerial_handle(MESA_trace_numerical_handle_t handle)
{
	if(handle!=NULL)
	{
		free(handle);
	}
}


void MESA_trace_destory_substr_handle(MESA_trace_addr_handle_t handle)
{
	if(handle!=NULL)
	{
		free(handle);
	}
}

MESA_trace_substr_handle_t MESA_trace_create_substr_handle(void* logger, const char *filename)
{
	FILE *fp;
	char conf_buf[MAX_TRACE_LEN];
	trace_substr_handle_nel_t* handle = NULL;

	fp = fopen(filename, "r");
	if(NULL == fp)
	{
		MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"open %s error.", filename);
	    return NULL;
	}
	handle = (trace_substr_handle_nel_t*)calloc(1, sizeof(trace_substr_handle_nel_t));
	while(fgets(conf_buf, MAX_TRACE_LEN, fp) != NULL)
	{
		if('#' == conf_buf[0] || '\r' == conf_buf[0] || '\n' == conf_buf[0])
		{
			continue;
		}		
		trace_del_last_rn(conf_buf);
		memcpy(handle->str[handle->str_num], conf_buf, strlen(conf_buf));
		handle->str_num++;
	}

	fclose(fp);
	return (MESA_trace_substr_handle_t)handle;
}

int MESA_trace_match_substr(MESA_trace_substr_handle_t handle, const char *str, int str_len)
{
	trace_substr_handle_nel_t* handler = (trace_substr_handle_nel_t*)handle;
	int i;
	for(i = 0; i<handler->str_num; i++)
	{
		if(NULL!=(memcasemem(str, str_len, handler->str[i], strlen(handler->str[i]))))
		{
			return 1;	
		} 
	}
	return 0;
}

void MESA_trace_destory_fullstr_handle(MESA_trace_addr_handle_t handle)
{
	if(handle!=NULL)
	{
		free(handle);
	}
}


MESA_trace_fullstr_handle_t MESA_trace_create_fullstr_handle(void* logger, const char *filename)
{
	FILE *fp;
	char conf_buf[MAX_TRACE_LEN];
	trace_fullstr_handle_nel_t* handle = NULL;

	fp = fopen(filename, "r");
	if(NULL == fp)
	{
		MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"open %s error.", filename);
	    return NULL;
	}
	handle = (trace_fullstr_handle_nel_t*)calloc(1, sizeof(trace_fullstr_handle_nel_t));
	while(fgets(conf_buf, MAX_TRACE_LEN, fp) != NULL)
	{
		if('#' == conf_buf[0] || '\r' == conf_buf[0] || '\n' == conf_buf[0])
		{
			continue;
		}		
		trace_del_last_rn(conf_buf);
		memcpy(handle->str[handle->str_num], conf_buf, strlen(conf_buf));
		handle->str_num++;
	}

	fclose(fp);
	return (MESA_trace_fullstr_handle_t)handle;
}

int MESA_trace_match_fullstr(MESA_trace_fullstr_handle_t handle, const char *str, int str_len)
{
	trace_fullstr_handle_nel_t* handler = (trace_fullstr_handle_nel_t*)handle;
    	int i;
	int len=0;
	for(i = 0; i<handler->str_num; i++)
	{
		len = strlen(handler->str[i]);
		if(len==str_len && (strncasecmp(str, handler->str[i], len) ==0))
		{
			return 1;		
		}
	}
	return 0;
}

void MESA_trace_destory_addr_handle(MESA_trace_addr_handle_t handle)
{
	if(handle!=NULL)
	{
		free(handle);
	}
}

MESA_trace_addr_handle_t MESA_trace_create_addr_handle(void* logger, const char *filename)
{
	FILE	*fp;
	char	buf[MAX_TRACE_LEN];
	char	seps[] = " ;\t";
	char	*toke;
	int    linenum = 0;
	unsigned int addr_type=0, sip=0, dip=0, source=0, dest=0;
	int 	inet_pton_rec = 0;
	trace_addr_handle_nel_t* handle = NULL;
	
	if((fp = fopen(filename, "r")) == NULL)
	{
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, TRACE_MODULE_NAME, "open %s error.", filename);
		return NULL;
	}
	handle = (trace_addr_handle_nel_t*)calloc(1, sizeof(trace_addr_handle_nel_t));
	while(fgets(buf, MAX_TRACE_LEN, fp))
	{
	    linenum++;		
		if(buf[0] == '\0' || buf[0] == '#' || isdigit(buf[0]) == 0|| '\r' == buf[0] || '\n' == buf[0]) /* empty or comment line */
		{ 
			continue;
		}
		trace_del_last_rn(buf);
		sip = dip = source = dest = 0;

		/*addr_type */
		toke = strtok(buf, seps);
		if(toke != NULL)
		{
			addr_type = atoi(toke);
			if( addr_type != 4 && addr_type != 6)
			{
				MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: Invalid ip address:%s.", filename, linenum, toke);
				return NULL;
			}
		}
		else
		{
			MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: pattern is: sip dip source dest.", filename, linenum);
			return NULL;
		}

		/* sip */
		toke = strtok(NULL, seps);
		if(toke != NULL)
		{
			switch(addr_type)
			{
				case 4:
					inet_pton_rec = inet_pton(AF_INET, toke, &handle->addr4[handle->addr4_num].saddr);
					break;
				case 6:					
					inet_pton_rec = inet_pton(AF_INET6, toke, &handle->addr6[handle->addr6_num].saddr);
					break;
			}
			if((atoi(toke) != 0) && (inet_pton_rec <=0))
			{
				MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: Invalid ip address:%s.", filename, linenum, toke);
				return NULL;
			}
		}
		else
		{
			MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: pattern is: sip dip source dest.", filename, linenum);
			return NULL;
		}
		
		/* dip */
		toke = strtok(NULL, seps);
		if(toke != NULL)
		{
			switch(addr_type)
			{
				case 4:
					inet_pton_rec = inet_pton(AF_INET, toke, &handle->addr4[handle->addr4_num].daddr);
					break;
				case 6:					
					inet_pton_rec = inet_pton(AF_INET6, toke, &handle->addr6[handle->addr6_num].daddr);
					break;
			}
			if((atoi(toke) != 0) && (inet_pton_rec <=0))
			{
			    MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: Invalid ip address:%s.", filename, linenum, toke);
				return NULL;
			}
		}
		else
		{
		    MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: pattern is: sip dip source dest.", filename, linenum);
			return NULL;
		}
		
		/* source */
		toke = strtok(NULL, seps);
		if(toke != NULL)
		{
			source = atoi(toke);
			if(source <0 || source > 65535)
			{
				MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: Invalid port:%s.", filename, linenum, toke);			  
				return NULL;
			}
			source = htons(source);
		}
		else
		{
		   MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: pattern is: sip dip source dest.", filename, linenum);
		   return NULL;
		}

		/* dest */
		toke = strtok(NULL, seps);
		if(toke != NULL)
		{
			dest = atoi(toke);
			if(dest <0 || dest > 65535)
			{
				MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: Invalid port:%s.", filename, linenum, toke);	
				return NULL;
			}
			dest = htons(dest);
		}
		else
		{
		    MESA_handle_runtime_log(logger,RLOG_LV_FATAL, TRACE_MODULE_NAME,"%s-%d: pattern is: sip dip source dest.", filename, linenum);
			return NULL;
		}

		while(strtok(NULL, seps)); /* clear strtok buf */

		switch(addr_type)
		{
			case 4:
				handle->addr4[handle->addr4_num].source  	= source;
				handle->addr4[handle->addr4_num].dest	    = dest;
				handle->addr4_num++;
				break;
			case 6:	
				handle->addr6[handle->addr6_num].source  	= source;
				handle->addr6[handle->addr6_num].dest	    = dest;
				handle->addr6_num++;				
				break;
		}		
		if(handle->addr4_num == MAX_ADDR_TRACE_RULE - 1||handle->addr6_num == MAX_ADDR_TRACE_RULE - 1)
		{			 
			break;
		}
	}
	fclose(fp);
	
	return (MESA_trace_addr_handle_t)handle;
}

int trace_match_addr4(MESA_trace_addr_handle_t handle, struct trace_tuple4_v4* paddr)
{		
	if(handle==NULL) return 0;
	trace_addr_handle_nel_t* handler = (trace_addr_handle_nel_t*)handle;
	int i;
	struct trace_tuple4_v4* addr4 = NULL;
	
	for(i = 0; i < handler->addr4_num; i++)
	{
		addr4 = &(handler->addr4[i]);
		if((addr4->saddr == 0 || addr4->saddr == paddr->saddr) && (addr4->daddr == 0 || addr4->daddr == paddr->daddr)  
		    && (addr4->source == 0 || addr4->source == paddr->source) && (addr4->dest == 0 || addr4->dest == paddr->dest))	 
		{   
			return 1;
		}
		if((addr4->saddr == 0 || addr4->saddr == paddr->daddr)  && (addr4->daddr == 0 || addr4->daddr == paddr->saddr)
		    && (addr4->source == 0 || addr4->source == paddr->dest) && (addr4->dest == 0 || addr4->dest == paddr->source)) 
		{
			return 1;
		}
	}	
	return 0;
}

int is_addr6_same(unsigned char *src, unsigned char *dest)
{
	/*
	for(unsigned int i=0;i<IPV6_ADDR_LEN;i++)
	{
		if(src[i]!=dest[i])
		{
			return 0;
		}
	}
	*/
	if(0==memcmp(src, dest, sizeof(struct trace_tuple4_v6)))
	{
		return 1;
	}
	return 0;
}

int is_addr6_any(unsigned char *src)
{	
	for(unsigned int i=0;i<IPV6_ADDR_LEN;i++)
	{
		if(src[i]!='\0')
		{
			return 0;
		}
	}
	return 1;	
}


int trace_match_addr6(MESA_trace_addr_handle_t handle,  struct trace_tuple4_v6* paddr) 
{	
	if(handle==NULL) return 0;
	trace_addr_handle_nel_t* handler = (trace_addr_handle_nel_t*)handle;
	int i;	
	struct trace_tuple4_v6* addr6 = NULL;
	
	for(i = 0; i < handler->addr6_num; i++)
	{
		addr6 = &(handler->addr6[i]);
		if((is_addr6_any(addr6->saddr) || is_addr6_same(addr6->saddr, paddr->saddr)) && (is_addr6_any(addr6->daddr)||is_addr6_same(addr6->daddr, paddr->daddr))  
		    && (addr6->source == 0 || addr6->source == paddr->source) && (addr6->dest == 0 || addr6->dest == paddr->dest))	 
		{   
			return 1;
		}
		if((addr6->saddr == 0 || is_addr6_same(addr6->saddr, paddr->daddr))&& (addr6->daddr == 0 ||  is_addr6_same(addr6->daddr, paddr->saddr))
		    && (addr6->source == 0 || addr6->source == paddr->dest) && (addr6->dest == 0 || addr6->dest == paddr->source)) 
		{
			return 1;
		}
	}	
	return 0;
}

int MESA_trace_match_addr(MESA_trace_addr_handle_t handle, trace_layer_addr *paddr)
{		
	if(handle==NULL) return 0;
	int rec=0;
	
	switch(paddr->addrtype)
	{
		case TRACE_ADDR_TYPE_IPV4:
		case TRACE__ADDR_TYPE_IP_PAIR_V4:
				rec = trace_match_addr4(handle,paddr->tuple4_v4);
				break;
		case TRACE_ADDR_TYPE_IPV6:
		case TRACE__ADDR_TYPE_IP_PAIR_V6:
				rec = trace_match_addr6(handle,paddr->tuple4_v6);		
				break;
	}
	return rec;
}


//for test
/*
int main(int argc, char *argv[])
{	
	void* logger = MESA_create_runtime_log_handle("./log/run_log",10);
	//trace_addr_handle_t addr_handle = trace_create_addr_handle(logger, "./conf/addr_trace.conf");	
	trace_fullstr_handle_t protocol_handle = trace_create_fullstr_handle(logger, "./conf/protocol_trace.conf");
	trace_substr_handle_t biz_handle = trace_create_substr_handle(logger, "./conf/biz_trace.conf");

	printf("%d;%d;%d;%d\n", trace_match_fullstr(protocol_handle, "SERVICE_HTTP", strlen("SERVICE_HTTP")),
					    trace_match_fullstr(protocol_handle, "HTTP", strlen("HTTP")),
					     trace_match_substr(biz_handle, "SERVICE_HTTP", strlen("SERVICE_HTTP")),
					      trace_match_substr(biz_handle, "SERVICE_HTTP_1", strlen("SERVICE_HTTP_1")));
	return 0;

}
*/
