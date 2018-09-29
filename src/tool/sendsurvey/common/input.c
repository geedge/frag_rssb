// input.c 
//
//   
// 
// MODIFIED by
// CREATED by Guo Le, 09/11/2010

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/un.h>
#include <stddef.h>//offsetof
#include "input.h"


int input_udp_init(uint16_t port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd) {
        //DEBUG_PRINTF("listener: socket %s", strerror());
        perror("listener: socket");
        return -1;
    }
    struct sockaddr_in my_addr; /* my address information */
    my_addr.sin_family = AF_INET; /* host byte order */
    my_addr.sin_port = port; /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
    bzero(&(my_addr.sin_zero), 8); /* zero the rest of the struct */

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        //DEBUG_PRINTF("listener: bind %s", strerror());
        perror("listener: bind");
        close(sockfd);
        return -1;
    }
    /*
    if(fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        close(sockfd);
        perror("fcntl");
        return -1;
    }
    */
    return sockfd;
}

void input_udp_clear(int sockfd)
{
    close(sockfd);
}


int input_udp_recv(int sockfd, uint32_t *src_ip, uint8_t *buf, uint32_t buf_size)
{
    if (NULL == buf)
        return -1;

    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    //printf("listener: waiting to recvfrom...\n");
    if ((numbytes = recvfrom(sockfd, buf, buf_size , 0,
                    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        return -1;
    }
    *src_ip = ((struct sockaddr_in *)&their_addr)->sin_addr.s_addr;
    return numbytes;
}
int init_input_un_socket(const char* file_name)
{
	int sd=-1;
	int len;
	struct sockaddr_un un;
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
	}
	memset(&un,0,sizeof(un));
	un.sun_family=AF_UNIX;

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
int named_unix_domain_socket_recv(int un_sd,char*buf,int buf_len)
{
	int recv_len=0;
	struct sockaddr_storage sa;
	socklen_t sa_len;
	recv_len=recvfrom(un_sd,buf,buf_len,0,(struct sockaddr*)&sa,&sa_len);
	return recv_len;
}
