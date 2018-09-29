// ioctl_op.c 
//
//   
// 
// MODIFIED by
// CREATED by Guo Le, 11/29/2010

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "debug.h"

unsigned int getipbyifname(const char *ifname)
{
    int sockfd;
    struct ifreq ifr;
    unsigned int ip;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd) {
        perror("socket");
        goto error;
    }

    strcpy(ifr.ifr_name,ifname); 
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl");
        goto error;
    }

    ip = ((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr.s_addr;
    close(sockfd);
    return ip;

error:
    close(sockfd);
    return INADDR_NONE;
}
