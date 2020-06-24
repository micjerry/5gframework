
#include "agc_sctp.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>

static agc_status_t agc_sctp_set_rtoinfo(agc_sctp_sock_t sock, agc_sctp_config_t *cfg)
{
    struct sctp_rtoinfo rtoinfo;

    memset(&rtoinfo, 0, sizeof(rtoinfo));
    rtoinfo.srto_initial = cfg->rto_initial;
    rtoinfo.srto_min = cfg->rto_min;
    rtoinfo.srto_max = cfg->rto_max;

    if (setsockopt(sock, IPPROTO_SCTP, SCTP_RTOINFO,
                            &rtoinfo, sizeof(rtoinfo)) != 0 ) 
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "setsockopt for SCTP_RTOINFO failed(%d:%s)",
                errno, strerror( errno ));
        return AGC_STATUS_FALSE;
    }

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sctp setsockopt success rtoinfo(%d:%d:%d).\n",
		cfg->rto_initial,
		cfg->rto_min,
		cfg->rto_max);
    return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_sctp_set_init_msg(agc_sctp_sock_t sock, agc_sctp_config_t *cfg)
{
    struct sctp_initmsg initmsg;

	memset(&initmsg,0,sizeof(initmsg));
    initmsg.sinit_num_ostreams = cfg->outbound_stream_num;
    initmsg.sinit_max_attempts = cfg->init_max_retrans;
	
    if (setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG,
                            &initmsg, sizeof(initmsg)) != 0 ) 
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "setsockopt for sctp_initmsg failed(%d:%s)",
                errno, strerror( errno ));
        return AGC_STATUS_FALSE;
    }

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sctp setsockopt success sctp_initmsg(%d:%d).\n",
		cfg->outbound_stream_num,
		cfg->init_max_retrans);
    return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_sctp_set_paddrparams(agc_sctp_sock_t sock, agc_sctp_config_t *cfg)
{
    struct sctp_paddrparams heartbeat;
    socklen_t socklen;

    memset(&heartbeat, 0, sizeof(heartbeat));
    socklen = sizeof(heartbeat);
    heartbeat.spp_hbinterval = cfg->hbinvertal;
	heartbeat.spp_pathmaxrxt = cfg->path_max_retrans;

    if (setsockopt(sock, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
                            &heartbeat, sizeof( heartbeat)) != 0 ) 
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "setsockopt for SCTP_PEER_ADDR_PARAMS failed(%d:%s)",
                errno, strerror(errno));
        return AGC_STATUS_FALSE;
    }

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sctp setsockopt success sctp_paddrparams(%d:%d).\n",
		cfg->hbinvertal,
		cfg->path_max_retrans);
    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_sctp_server(agc_sctp_sock_t *sock, agc_std_sockaddr_t *local_addr, agc_sctp_config_t *cfg)
{
	agc_sctp_sock_t fd = socket(local_addr->ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd <= 0)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create Sctp socket failed.\n");
		return AGC_STATUS_FALSE;
	}

	agc_sctp_set_init_msg(fd, cfg);
	agc_sctp_set_rtoinfo(fd, cfg);
	agc_sctp_set_paddrparams(fd, cfg);

    if (bind(fd, (struct sockaddr *)local_addr, sizeof(local_addr)) != 0)
    {
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_sctp_server bind(%d) failed(%d:%s)",
                local_addr->ss_family, errno, strerror(errno));
        return AGC_STATUS_FALSE;
    }

    *sock = fd;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Create sctp server socket success.\n");
    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_sctp_client(agc_sctp_sock_t *sock, agc_std_sockaddr_t *local_addr, agc_sctp_config_t *cfg)
{
	agc_sctp_sock_t fd = socket(local_addr->ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd <= 0)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create Sctp socket failed.\n");
		return AGC_STATUS_FALSE;
	}

	agc_sctp_set_init_msg(fd, cfg);
	agc_sctp_set_rtoinfo(fd, cfg);
	agc_sctp_set_paddrparams(fd, cfg);

    *sock = fd;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Create sctp client socket success.\n");
    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_sctp_connect(agc_sctp_sock_t sock, agc_std_sockaddr_t *remote_addr)
{
    if (connect(sock, (struct sockaddr *)remote_addr, sizeof(remote_addr)) != 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket connect failed(%d:%s)", errno, strerror(errno));
        return AGC_STATUS_FALSE;
    }

    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_sctp_send(agc_sctp_sock_t sock, agc_sctp_stream_t *stream, const char *buf, agc_size_t *len)
{
    agc_size_t size = sctp_sendmsg(sock, buf, *len,
            (struct sockaddr *)&stream->remote_addr, sizeof(stream->remote_addr),
            htonl(stream->ppid),
            0,  /* flags */
            stream->stream_no,
            0,  /* timetolive */
            0); /* context */
    if (size < 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_sendmsg(len:%ld) failed(%d:%s)",
                *len, errno, strerror(errno));
    }

    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_sctp_recv(agc_sctp_sock_t sock, agc_sctp_stream_t *stream, char *buf, agc_size_t *len)
{
    struct sctp_sndrcvinfo sndrcvinfo;
    socklen_t addrlen = sizeof(struct sockaddr_storage);

    *len = sctp_recvmsg(sock, buf, AGC_SCTP_MAX_BUFFER,
                (struct sockaddr *)&stream->remote_addr,  &addrlen,
                &sndrcvinfo, &stream->msg_flags);
    if (*len < 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_recvmsg(%d) failed(%d:%s)",
                *len, errno, strerror(errno));
        return AGC_STATUS_FALSE;
    }
    stream->associate = sndrcvinfo.sinfo_assoc_id;
    stream->ppid = ntohl(sndrcvinfo.sinfo_ppid);
    stream->stream_no = sndrcvinfo.sinfo_stream;

    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_sctp_close(agc_sctp_sock_t sock)
{
    shutdown(sock,SHUT_WR);
    close(sock);
    
    return AGC_STATUS_SUCCESS;
}