
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
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

#include "hard_keepalive.h"

int udp_socket_recv(int sockfd, uint32_t *src_ip, uint16_t *src_port, uint8_t *buf, uint32_t buf_size)
{
    if (NULL == buf)     return -1;

    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
	
    if ((numbytes = recvfrom(sockfd, buf, buf_size , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
	{
        perror("recvfrom");
        return -1;
    }
    *src_ip = ((struct sockaddr_in *)&their_addr)->sin_addr.s_addr;	
    *src_port = ((struct sockaddr_in *)&their_addr)->sin_port;
    return numbytes;
}

// send udp packet
int udp_socket_send(int sockfd, uint32_t addr, uint16_t port, char *data, int datalen)
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
				return -1;
			}
		}
		to_send_len-=already_sended_len;						
	}
    return already_sended_len;
}

int create_recv_udp_socket(uint16_t port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd) 
	{
        perror("listener: socket");
        return -1;
    }
    struct sockaddr_in my_addr; 			/* my address information */
    my_addr.sin_family = AF_INET; 			/* host byte order */
    my_addr.sin_port = port; 				/* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY; 	/* auto-fill with my IP */
    bzero(&(my_addr.sin_zero), 8); 			/* zero the rest of the struct */

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) 
	{
        perror("listener: bind");
        close(sockfd);
        return -1;
    }   
    return sockfd;
}

void* thread_hard_keepalive(void *param)
{
	char		buf[1500] = {0};
	char		discriminator_temp[ID_SIZE] = {0};
	bfd_port_t*	bfd_port = (bfd_port_t*)param;
	int			size = 0;
	uint32_t	src_ip = 0;	
	uint16_t	src_port = 0;	
	int			send_rec = 0;
	fd_set 		rset;
	int			recv_pkt_sd = create_recv_udp_socket(htons(bfd_port->recv_port));
	
	if(-1==recv_pkt_sd)
	{
		printf("hard_keepalive create socket error.\n");		
		return NULL;
	}

	while(1)
	{		
		FD_ZERO(&rset);
		FD_SET(recv_pkt_sd,&rset);
		if(-1==select(recv_pkt_sd+1,&rset,NULL,NULL,NULL))
		{			
			continue;
		}
		if(FD_ISSET(recv_pkt_sd, &rset))
		{
			size = udp_socket_recv(recv_pkt_sd, &src_ip, &src_port, (unsigned char*)buf, sizeof(buf));			
			if(size>0)
			{
				/*½»»»discriminator*/
				memcpy(discriminator_temp, buf+MY_ID_OFFSET, ID_SIZE);
				memcpy(buf+MY_ID_OFFSET, buf+YOUR_ID_OFFSET, ID_SIZE);
				memcpy(buf+YOUR_ID_OFFSET, discriminator_temp, ID_SIZE);
				send_rec = udp_socket_send(recv_pkt_sd,
												src_ip,
												src_port,
												(char*)buf,size);
				if(-1==send_rec)
				{
					printf("hard_keepalive send pkt error.\n");	
				}
			}
		}
		else
		{
			continue;
		}
	}
	
	return NULL;
}

int hard_keepalive_run(void* bfd_port)
{
	pthread_t	 	thread_desc;
	pthread_attr_t 	attr;

	memset(&thread_desc, 0, sizeof(thread_desc));
	memset(&attr, 0, sizeof(attr));	
	
	if(0 != pthread_attr_init(&(attr))) 
    {
		return -1;
	}
	if(0 != pthread_attr_setdetachstate(&(attr), PTHREAD_CREATE_DETACHED)) 
    {		
		return -1;
	}
	if(0 != pthread_create(&(thread_desc), &(attr), thread_hard_keepalive, (void*)bfd_port)) 
    {
		pthread_attr_destroy(&(attr));
		return -1;
	}
	pthread_attr_destroy(&(attr));
	return 0;
}

