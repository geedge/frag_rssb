//gcc -g -Wall un_server.c -o un_server
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/un.h>
/*
		memset(&(g_pproc->un_backend_addr),0,sizeof(g_pproc->un_backend_addr));
		strcpy(g_pproc->un_backend_addr[i].sun_path,back_end_file_name);
		g_pproc->un_backend_addr[i].sun_family=AF_UNIX;
		
*/
int init_named_unix_domain_socket(const char* file_name)
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

			printf("Unix-domain send data: loss data %s\n",strerror(errno));
			return -1;

		}
		to_send_len-=already_sended_len;						
	}
	return already_sended_len;
}
int main()
{
	int sd=-1;
	char buf[2048];
	int recv_len=0;
	struct sockaddr_storage sa;
	socklen_t sa_len;
	char* recv_file_name="/home/zhengchao/recv_preproc";
	unsigned long long stat_recv_bytes=0,stat_recv_pkts=0;
	  unsigned long long mult_tmp=0;
	  int i=0;
	sd=init_named_unix_domain_socket(recv_file_name);
	while(1)
	{
		recv_len=recvfrom(sd,buf,sizeof(buf),0,(struct sockaddr*)&sa,&sa_len);
		stat_recv_bytes+=recv_len;
		stat_recv_pkts++;
	    for(i=1;i<stat_recv_pkts%10;i++)
	    {
	      mult_tmp*=i;
	      mult_tmp%=7827368423;
	    }

//		printf("%d %llu\n",recv_len,stat_recv_pkts);
	}
}

