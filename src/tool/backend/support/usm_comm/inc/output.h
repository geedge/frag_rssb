#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <stdint.h>
#include <sys/un.h>
// send udp packet
int init_output_un_socket(const char* file_name);
int named_unix_domain_socket_send(int un_sd,struct sockaddr_un *dest_un,const char*data,int datalen);
#endif /* _OUTPUT_H_ */
