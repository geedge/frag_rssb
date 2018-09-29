// output.c
//
//
//
// MODIFIED by
// CREATED BY Guo Le, 08/24/2010
// use sendnto instead of sendto, zhengchao@iie.ac.cn 2012-05-30
// add Unix-domain sokcet,zhengchao@iie.ac.cn 2012-06-04
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <stddef.h>//offsetof
#include "debug.h"

#include "output.h"

int output_udp_init()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (-1 == sockfd) {
        DEBUG_PRINTF("udpout_init: socket %s\n", strerror(errno));
        return -1;
    }
    return sockfd;
}


void output_udp_clear(int sockfd)
{
    close(sockfd);
}

// send udp packet
int output_udp_send(int sockfd, uint32_t addr, uint16_t port, uint8_t *data, int datalen)
{
    struct sockaddr_in dst_addr; /* connector's address information */
    dst_addr.sin_family = AF_INET; /* host byte order */
    dst_addr.sin_port = port; /* short, network byte order */
    dst_addr.sin_addr.s_addr = addr;
    bzero(&(dst_addr.sin_zero), 8); /* zero the rest of the struct */
	int to_send_len=datalen;
	int already_sended_len=0;
	while(to_send_len>0)
	{
		already_sended_len=sendto(sockfd,data,
								to_send_len-already_sended_len,
								0,
								(struct sockaddr *)&(dst_addr),
								sizeof(dst_addr));
		if(already_sended_len==-1)
		{
			if((EAGAIN == errno)||( EINTR == errno )|| (EWOULDBLOCK==errno))
			{
				continue;
			}
			else
			{
				DEBUG_MESSAGE("udpout_send: loss data");
				return -1;
			}
		}
		to_send_len-=already_sended_len;						
	}
    return already_sended_len;
}


int output_udp_init_connect(uint32_t addr, uint16_t port, char *dev)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (-1 == sockfd) {
        DEBUG_PRINTF("udpout_init_connect: socket %s\n", strerror(errno));
        return -1;
    }
	struct sockaddr_in dst_addr; /* connector's address information */
	dst_addr.sin_family = AF_INET; /* host byte order */
	dst_addr.sin_port = port; /* short, network byte order */
	dst_addr.sin_addr.s_addr = addr;
    bzero(&(dst_addr.sin_zero), 8); /* zero the rest of the struct */

	if (-1 == connect(sockfd, (struct sockaddr*)&dst_addr, sizeof(dst_addr)))
	{
        DEBUG_PRINTF("udpout_init_connect: error in connect %s\n", strerror(errno));
		return -1;
	}

    if (dev) {
        struct ifreq interface;
        strncpy(interface.ifr_ifrn.ifrn_name, dev, IFNAMSIZ);
        if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface, sizeof(interface))  < 0) {
            DEBUG_PRINTF("SO_BINDTODEVICE failed %s\n", strerror(errno));
            return -1;
        }
    }

    return sockfd;
}

void output_udp_clear_connect(int sockfd)
{
    close(sockfd);
}

// send udp packet
int output_udp_send_connect(int sockfd, uint8_t *data, int datalen)
{
	int to_send_len=datalen;
	int already_sended_len=0;
	while(to_send_len>0)
	{
		already_sended_len= send(sockfd, data, datalen, 0);
		if(already_sended_len==-1)
		{
			if((EAGAIN == errno)||( EINTR == errno )|| (EWOULDBLOCK==errno))
			{
				continue;
			}
			else
			{
				printf("udpout_send_connect: loss data %s\n",strerror(errno));
				DEBUG_MESSAGE("udpout_send_connect: loss data");
				return -1;
			}
		}
		to_send_len-=already_sended_len;						
	}
	return already_sended_len;
}
int init_output_un_socket(const char* file_name)
{
	int sd=-1;
	int len;
	struct sockaddr_un un;
	struct timeval send_timeout;
	int send_buf_size=1024*1024*4;
	if(strlen(file_name)+8>sizeof(un.sun_path))
	{
		printf("Unix-domain socket file name too long.\n");
		return -1;
	}
	sd=socket(AF_UNIX,SOCK_DGRAM,0);
	if(sd<0)
	{
		printf("Get Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}/*
	flags=fcntl(sd,F_GETFL);
	flags|=O_NONBLOCK;
	if(fcntl(sd,F_SETFL,flags)==-1)
	{
		printf("Set Unix-domain socket error:%s",strerror(errno));
		return -1;
	}*/
	send_timeout.tv_sec=0;
	send_timeout.tv_usec=100000;
	memset(&un,0,sizeof(un));
	un.sun_family=AF_UNIX;
	if(-1==setsockopt(sd,SOL_SOCKET,SO_SNDTIMEO,&send_timeout,sizeof(send_timeout)))
	{
		printf("Set Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	if(-1==setsockopt(sd,SOL_SOCKET,SO_SNDBUF,&send_buf_size,sizeof(send_buf_size)))
	{
                printf("Set Unix-domain socket error:%s\n",strerror(errno));
                return -1;
	}
	snprintf(un.sun_path,sizeof(un.sun_path),"%s",file_name);
	len=offsetof(struct sockaddr_un,sun_path)+strlen(un.sun_path);
	unlink(un.sun_path);
	if(0>bind(sd,(struct sockaddr*)&un,len))
	{
		printf("Bind Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	return sd;
}
int named_unix_domain_socket_send(int un_sd,struct sockaddr_un *dest_un,const char*data,int datalen)
{
	int to_send_len=datalen;
	int already_sended_len=0;
	int sock_addr_len=offsetof(struct sockaddr_un,sun_path)+strlen(dest_un->sun_path);
	while(to_send_len>0)
	{
		already_sended_len= sendto(un_sd, data, datalen,0,(struct sockaddr*)dest_un, sock_addr_len);
		if(already_sended_len==-1)
		{

			//DEBUG_PRINTF("Unix-domain send data: loss data %s\n",strerror(errno));
			return -1;

		}
		to_send_len-=already_sended_len;						
	}
	return already_sended_len;
}
