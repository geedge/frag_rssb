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


int input_udp_recv(int sockfd, uint32_t *src_ip, uint8_t *buf, uint32_t *size)
{
    if (NULL == buf)
        return -1;

    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    //printf("listener: waiting to recvfrom...\n");
    if ((numbytes = recvfrom(sockfd, buf, 1500 , 0,
                    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        return -1;
    }
    //*src_ip = get_in_addr((struct sockaddr *)&their_addr);
    *src_ip = ((struct sockaddr_in *)&their_addr)->sin_addr.s_addr;
    *size = numbytes;
    return numbytes;
}

