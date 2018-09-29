#ifndef _HARD_KEEPALIVE_H
#define _HARD_KEEPALIVE_H

typedef struct bfd_port_s
{		
	uint32_t	recv_port;
	uint32_t	send_port;
}bfd_port_t;

#define	MY_ID_OFFSET	4
#define	YOUR_ID_OFFSET	8
#define	ID_SIZE			4
typedef struct bfd_header_s
{
    unsigned char version_diag;
    unsigned char flags;
    unsigned char detect_time_multiplier;
    unsigned char length;
    unsigned char my_discriminator[4]; /* ҵ?º? ²²â*/
    unsigned char your_discriminator[4];
    unsigned char desired_min_tx_interval[4];
    unsigned char required_min_rx_interval[4];
    unsigned char required_min_echo_interval[4];
}bfd_header_t; 

#ifdef __cplusplus
extern "C" {
#endif


/*return 0:succ -1:fail*/
int hard_keepalive_run(void* bfd_port);

#ifdef __cplusplus
}
#endif


#endif
