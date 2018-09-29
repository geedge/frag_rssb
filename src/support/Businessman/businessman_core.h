#include "smooth_send.h"
#include "reliable_send.h"
#include "reliable_recv.h"
#include "stream_recover.h"
#include "network_layer.h"
#include "businessman_time.h"
typedef struct _businessman_send_space_t{
	smooth_space_t *thread_smooth_space[MAX_THREAD_SUPPORT];
	reliable_send_space_t *core_reliable_space;
	time_tick_t send_smooth_timeout_tick;
	time_tick_t stream_frozen_stream_time_tick;
	unsigned long long cached_smooth_pkts_limit;
}bizman_send_space_t;
typedef struct _businessman_recv_space_t{
	recover_space_t *stream_recover_space;
	reliable_recv_space_t* reliable_recv_space;
	smooth_space_t*ack_send_smooth_space;
	unsigned long long stat_packet_recv_num;
	unsigned long long stat_ack_send_num;
	unsigned long long stat_invalid_packet_recv_num;
}bizman_recv_space_t;
typedef struct _businessman_work_space_t{
	unsigned int should_sender_active;
	unsigned int should_receiver_active;
	unsigned int flags;
	bizman_recv_space_t *recv_space;
	bizman_send_space_t *send_space;
	network_space_t* network_layer;
	unsigned long long sender_cached_pkts_limit;
}bizman_work_space_t;

typedef bizman_work_space_t bizman_handle_t;

