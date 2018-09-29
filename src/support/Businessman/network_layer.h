#ifndef _BIZMAN_NETWORK_H_INCLUDE_
#define _BIZMAN_NETWORK_H_INCLUDE_
#include "businessman_error.h"
const int	BIZMAN_INTERFACE_SOCKET=0;
const int	BIZMAN_INTERFACE_PAG=0xf;
const int	MAX_INTERFACE_NUM=2;
typedef struct _businessman_netwrok_space{

	char* socket_interface_name;
	unsigned int src_addr;
	unsigned char src_port;
	int using_pag_send;
	int socket_handle;
	unsigned short listen_port;
	unsigned short send_port;
}network_space_t;

network_space_t* get_network_handle(void);
void destroy_network_handle(network_space_t* network_layer);
void set_network_interface(network_space_t* network_layer, int interface_type,const char* interface_name);
int set_network_listen_port(network_space_t* network_layer, unsigned short port);
int set_network_send_port(network_space_t* network_layer, unsigned short port);

/*return 0 when success,return -1 when failed*/
int  init_network_layer(network_space_t* network_space);

int do_actual_send(network_space_t *network_handle,unsigned int dest_ip,unsigned short dest_port,const char* data,int len);
/*return received length(>=0) when success,return -1 when failed*/
int do_actual_recv(network_space_t *network_handle,unsigned int *src_ip,unsigned short* src_port,char*buf,int buf_len);
#endif //_BIZMAN_NETWORK_H_INCLUDE_
