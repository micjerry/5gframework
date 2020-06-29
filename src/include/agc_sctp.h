

#ifndef __AGC_SCTP_H__
#define __AGC_SCTP_H__

#include <agc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>

#define AGC_SCTP_MAX_BUFFER 4096

AGC_BEGIN_EXTERN_C

typedef agc_std_socket_t agc_sctp_sock_t;

typedef struct 
{         
    int32_t 	outbound_stream_num;        
	int32_t		hbinvertal;					
	int32_t		path_max_retrans;
	int32_t		init_max_retrans;				
	int32_t		rto_initial;					
	int32_t		rto_min;						
	int32_t		rto_max;					
}agc_sctp_config_t;

typedef struct
{
	uint32_t associate;
	uint32_t stream_no;
    uint32_t ppid;
    int32_t msg_flags;
	socklen_t addrlen;
	agc_std_sockaddr_t remote_addr;
}agc_sctp_stream_t;

AGC_DECLARE(agc_status_t) agc_sctp_server(agc_sctp_sock_t *sock, agc_std_sockaddr_t *local_addr, socklen_t addrlen, agc_sctp_config_t *cfg);
AGC_DECLARE(agc_status_t) agc_sctp_client(agc_sctp_sock_t *sock, agc_std_sockaddr_t *local_addr, socklen_t addrlen, agc_sctp_config_t *cfg);
AGC_DECLARE(agc_status_t) agc_sctp_connect(agc_sctp_sock_t sock, agc_std_sockaddr_t *remote_addr, socklen_t addrlen);
AGC_DECLARE(agc_status_t) agc_sctp_accept(agc_sctp_sock_t sock, agc_sctp_sock_t *client_sock, agc_std_sockaddr_t *remote_addr, socklen_t *addrlen);
AGC_DECLARE(agc_status_t) agc_sctp_send(agc_sctp_sock_t sock, agc_sctp_stream_t *stream, const char *buf, int32_t *len);
AGC_DECLARE(agc_status_t) agc_sctp_recv(agc_sctp_sock_t sock, agc_sctp_stream_t *stream, char *buf, int32_t *len);
AGC_DECLARE(agc_status_t) agc_sctp_close(agc_sctp_sock_t sock);

AGC_END_EXTERN_C

#endif