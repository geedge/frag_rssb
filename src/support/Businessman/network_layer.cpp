#include <sys/socket.h>//socket
#include <sys/types.h>//socket
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h> //strerror
#include <errno.h>//strerror
#include <fcntl.h>//fcntl
#include <unistd.h>//fcntl
#include <net/if.h>//fcntl
#include <stdlib.h>//free
#include <sys/ioctl.h>//ioctl
#include "businessman_error.h"
#include "businessman_memory.h"
#include "businessman_limit.h"
#include "network_layer.h"

#ifdef DEPLOY_YSP_SYSTEM
//this function defined in df_B feedback.c
extern "C" int pag_send_av_udp(int Datasize, const char *pData, u_int src_ip, u_int dst_ip, u_short src_prt, u_short dst_prt);
#endif
void set_network_interface(network_space_t* network_layer, int interface_type,const char* interface_name){
	switch(interface_type)
	{	
		case BIZMAN_INTERFACE_SOCKET:
			network_layer->socket_interface_name=(char*)b_malloc(strlen(interface_name)+1);
			memcpy(network_layer->socket_interface_name,interface_name,strlen(interface_name));
			break;
#ifdef DEPLOY_YSP_SYSTEM
		case BIZMAN_INTERFACE_PAG:
			network_layer->using_pag_send=TRUE;
			break;
#endif
		default:
			set_user_error("invalid interface type %d",interface_type);
			break;
	}
	return ;
}
int set_network_listen_port(network_space_t* network_layer, unsigned short port)
{
	network_layer->listen_port=port;
	return 0;
}
int set_network_send_port(network_space_t* network_layer, unsigned short port)
{
	network_layer->send_port=port;
	return 0;
}
int startup_udp(network_space_t* network_layer)
{
	unsigned short bind_port=network_layer->listen_port;
	int sd_udp=-1;
	int flags;
	struct ifreq ifr;



 	if(-1==(sd_udp = socket(AF_INET, SOCK_DGRAM, 0)))
	{
		set_user_error( "socket error: %d %s, restart socket.", errno, strerror(errno));
		sd_udp=-1;
	}
	if(NULL!=network_layer->socket_interface_name)
	{
		strcpy(ifr.ifr_name,network_layer->socket_interface_name); 
		if (ioctl(sd_udp, SIOCGIFADDR, &ifr) < 0) 
		{
			set_user_error( "get interface %s addr error",network_layer->socket_interface_name);
			return -1;
		}
		network_layer->src_addr = ((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr.s_addr;
	}
	flags=fcntl(sd_udp,F_GETFL);
	flags|=O_NONBLOCK;
	if(fcntl(sd_udp,F_SETFL,flags)==-1)
	{
		set_user_error( "socket error: %d %s, restart socket.", errno, strerror(errno));
		sd_udp=-1;
	}
	int opt=1;
	struct sockaddr_in socket;
	memset(&socket, 0,sizeof(socket));
	socket.sin_family = AF_INET;
	socket.sin_port = htons(bind_port);
	socket.sin_addr.s_addr = htonl(INADDR_ANY);

	if (setsockopt (sd_udp, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt) ) < 0) {
		set_user_error("setsockopt error: %d %s, restart socket.", errno, strerror(errno));
		close (sd_udp);
		sd_udp=-1;
		return sd_udp;
	}
	if(-1 == bind(sd_udp, (struct sockaddr *)(&socket), sizeof(socket))) {
		set_user_error("bind error: %d %s, restart socket.", errno, strerror(errno));
		close (sd_udp);
		sd_udp=-1;
		return sd_udp;
	}
	return sd_udp;
}
int startup_pag_send(){
	return 1;
}
int  init_network_layer(network_space_t* network_space)
{
	if(TRUE==network_space->using_pag_send)
	{
		if(network_space->listen_port==0)
		{
			set_user_error("listen port is necessary in pag send");
			return -1;
		}
		if(NULL==network_space->socket_interface_name)
		{
			set_user_error("socket interface name is necessary in pag send");
		}
	}
	network_space->socket_handle=startup_udp(network_space);
	return 1;
}
network_space_t* get_network_handle(void)
{
	return (network_space_t*)b_malloc(sizeof(network_space_t));
}
void destroy_network_handle(network_space_t* network_layer)
{
	if(network_layer->socket_handle>0)
	{
		close(network_layer->socket_handle);
	}
	if(NULL!=network_layer->socket_interface_name)
	{
		free(network_layer->socket_interface_name);
	}
	free(network_layer);
}
int do_actual_send(network_space_t * network_handle, unsigned int dest_ip, unsigned short dest_port, const char * data, int len)
{
	int to_send_len=len;
	int already_sended_len=0,this_sended_len=0;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr =dest_ip;
	addr.sin_port = htons(dest_port);

	if(TRUE==network_handle->using_pag_send)
	{
#ifdef DEPLOY_YSP_SYSTEM
		if(0>pag_send_av_udp(len,data, network_handle->src_addr, dest_ip,network_handle->listen_port,dest_port))
		{
			set_user_error("pag send error.");
		}
#endif
	}
	else
	{
		while(to_send_len>already_sended_len)
		{
			this_sended_len=sendto(network_handle->socket_handle,(void*)(data+already_sended_len),
									to_send_len-already_sended_len,
									0,
									(struct sockaddr *)&(addr),
									sizeof(addr));
			if(this_sended_len==-1)
			{
				if((EAGAIN == errno)||( EINTR == errno )|| (EWOULDBLOCK==errno))
				{
					continue;
				}
				else
				{
					set_user_error("at send,socket error: %d %s", errno, strerror(errno));
					return -1;
				}
			}
			already_sended_len=+this_sended_len;			
		}
	}
	return 0;
}
int do_actual_recv(network_space_t *network_handle,unsigned int *src_ip,unsigned short* src_port,char*buf,int buf_len)
{
	int received_length=-1;
	struct sockaddr_in src_socket;

	int socket_length=sizeof(src_socket);
	received_length = recvfrom(network_handle->socket_handle, buf, buf_len, 0, (struct sockaddr *)&src_socket, (socklen_t*)(&socket_length));
	if(-1==received_length)
	{
//		set_user_error( "UDP rcv error %d, %s", errno, strerror(errno));
	}
	else
	{
		*src_ip=src_socket.sin_addr.s_addr;
		*src_port=ntohs(src_socket.sin_port);
	}
	return received_length;
}

