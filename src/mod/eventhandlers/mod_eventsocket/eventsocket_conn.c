#include <mod_eventsocket.h>

static void *api_exec(agc_thread_t *thread, void *obj);

agc_status_t read_packet(event_connect_t *conn, agc_event_t **event)
{
	agc_size_t mlen, bytes = 0;
	agc_size_t len, hlen;
	char *mbuf = NULL;
	char *ptr;
	void *pop;
	agc_status_t status = AGC_STATUS_SUCCESS;
	uint32_t buf_len = 0;
	int count = 0;
	int clen = 0;
	uint8_t crcount = 0;
    
	*event = NULL;
    
	if (profile.done) {
		return AGC_STATUS_FALSE;
	}
    
	agc_zmalloc(mbuf, EVTSKT_BLOCK_LEN);
	assert(mbuf);
	buf_len = EVTSKT_BLOCK_LEN;
	ptr = mbuf;
    
	while (conn->sock && !profile.done) {
		uint8_t do_sleep = 1;
		mlen = 1;
        
		if (bytes == buf_len - 1) {
			char *tmp;
			int pos;
            
			pos = (int)(ptr - mbuf);
			buf_len += EVTSKT_BLOCK_LEN;
			tmp = realloc(mbuf, buf_len);
			assert(tmp);
			mbuf = tmp;
			memset(mbuf + bytes, 0, buf_len - bytes);
			ptr = (mbuf + pos);
		}
        
		status = agc_socket_recv(conn->sock, ptr, &mlen);
        
		if (profile.done || (!AGC_STATUS_IS_BREAK(status) && status != AGC_STATUS_SUCCESS)) {
			agc_safe_free(mbuf);
			return AGC_STATUS_FALSE;
		}
        
		if (mlen) {
			bytes += mlen;
			do_sleep = 0;
            
			if (*mbuf == '\r' || *mbuf == '\n') {	
				ptr = mbuf;
				mbuf[0] = '\0';
				bytes = 0;
				continue;
			}

			if (*ptr == '\n') {
				crcount++;
			} else if (*ptr != '\r') {
				crcount = 0;
			}
			ptr++;

			if (bytes >= EVTSKT_MAX_LEN) {
				crcount = 2;
			}

			if (crcount == 2) {
				char *next;
				char *cur = mbuf;
				bytes = 0;
				while (cur) {
					if ((next = strchr(cur, '\r')) || (next = strchr(cur, '\n'))) {
						while (*next == '\r' || *next == '\n') {
							next++;
						}
					}
					count++;
					if (count == 1) {
						agc_event_create(event, EVENT_ID_EVENTSOCKET, EVENT_NULL_SOURCEID);
						agc_event_add_header_string(*event, "command", mbuf);
					} else if (cur) {
						char *var, *val;
						var = cur;
						strip_cr(var);
						if (!zstr(var)) {
							if ((val = strchr(var, ':'))) {
								*val++ = '\0';
								while (*val == ' ') {
									val++;
								}
							}
							if (var && val) {
								agc_event_add_header_string(*event, var, val);
								if (!strcasecmp(var, "content-length")) {
									clen = atoi(val);

									if (clen > 0) {
										char *body;
										char *p;
                                        
										agc_zmalloc(body, clen + 1);
                                        
										p = body;
                                        
										while (clen > 0) {
											mlen = clen;
											status = agc_socket_recv(conn->sock, p, &mlen);

											if (profile.done || (!AGC_STATUS_IS_BREAK(status) && status != AGC_STATUS_SUCCESS)){
 												free(body);
												agc_safe_free(mbuf);  
												return AGC_STATUS_FALSE;
											}
                                            
											clen -= (int) mlen;
											p += mlen;
										}

										agc_event_add_body(*event, "%s", body);
										free(body);
									}
								}
							}
						}
					}

					cur = next;
				}
                
				break;
			}
		}
        
        if (!*mbuf) {
            if (conn->has_event) {
                while (agc_queue_trypop(conn->event_queue, &pop) == AGC_STATUS_SUCCESS) {
                    char hbuf[512];
		            agc_event_t *pevent = (agc_event_t *) pop;
            
                    do_sleep = 0;
            
                    agc_event_serialize_json(pevent, &conn->ebuf);
            
                    assert(conn->ebuf);
                    len = strlen(conn->ebuf);
                    
                    agc_snprintf(hbuf, sizeof(hbuf), "Content-Length: %d\n" "Content-Type: text/event-json\n" "\n", len);
                    hlen = strlen(hbuf);
                    
                    agc_socket_send(conn->sock, hbuf, &hlen);
                    agc_socket_send(conn->sock, conn->ebuf, &len);
                    
                    agc_safe_free(conn->ebuf);
                    agc_event_destroy(&pevent);
                }
            }
        }
        
        if (do_sleep) {
			int fdr = 0;
			agc_poll(conn->pollfd, 1, &fdr, 20000);
		} else {
			agc_os_yield();
		}
    }
    
    agc_safe_free(mbuf);
    return status;
}

agc_status_t parse_command(event_connect_t *conn, agc_event_t **event, char *reply, uint32_t reply_len)
{
	agc_status_t status = AGC_STATUS_SUCCESS;
	char *cmd = NULL;
	agc_event_t *pevent = *event;
    
	*reply = '\0';
    
	if (!event || !pevent || !(cmd = agc_event_get_header(pevent, "command"))) {
		conn->is_running = 0;
		agc_snprintf(reply, reply_len, "-ERR command parse error.");
		return status;
	}
    
	if (!strncasecmp(cmd, "exit", 4) || !strncasecmp(cmd, "...", 3)) {
		conn->is_running = 0;
		agc_snprintf(reply, reply_len, "+OK bye");
		return status;
	}
    
	if (!strncasecmp(cmd, "api ", 4)) {
		api_command_t acs = {0};
		char *console_execute = agc_event_get_header(pevent, "console_execute");
        
		char *api_cmd = cmd + 4;
		char *arg = NULL;
		strip_cr(api_cmd);
        
		if (!(acs.console_execute = agc_true(console_execute))) {
			if ((arg = strchr(api_cmd, ' '))) {
				*arg++ = '\0';
			}
		}
        
		acs.conn = conn;
		acs.api_cmd = api_cmd;
		acs.arg = arg;
		acs.bg = 0;
        
		api_exec(NULL, (void *)&acs);
        
		status = AGC_STATUS_SUCCESS;
        
		if (event) {
			agc_event_destroy(event);
		}   
	} else if (!strncasecmp(cmd, "event", 5)) {
		char *next, *cur;
		uint32_t count = 0, key_count = 0;
		uint8_t custom = 0;
		uint32_t x = 0;
        
		//re subscribed clear current
		for (x = 0; x < EVENT_ID_LIMIT; x++) {
			conn->event_list[x] = 0;
		}
        
		strip_cr(cmd);
		cur = cmd + 5;
        
		if (cur && (cur = strchr(cur, ' '))) {
			for (cur++; cur; count++) {
				int event_id;
                
				if ((next = strchr(cur, ' '))) {
					*next++ = '\0';
				}
                
				if (agc_event_get_id(cur, &event_id) == AGC_STATUS_SUCCESS) {
					key_count++;
					if (event_id == EVENT_ID_ALL) {
						for (x = 0; x < EVENT_ID_LIMIT; x++) {
							conn->event_list[x] = 1;
						}
					} else {
						conn->event_list[event_id] = 1;
					}
				}
			}
            
			cur = next;
		}
        
		if (!key_count) {
			conn->has_event = 0;
			agc_snprintf(reply, reply_len, "-ERR no keywords supplied");
			return status;
		}
        
		conn->has_event = 1;
	}
    
	return status; 
}

void strip_cr(char *s)
{
    char *p;
	if ((p = strchr(s, '\r')) || (p = strchr(s, '\n'))) {
		*p = '\0';
	}
}

static void *api_exec(agc_thread_t *thread, void *obj)
{
	api_command_t *acs = (api_command_t *)obj;
	agc_stream_handle_t stream = { 0 };
	char *reply, *freply = NULL;
	agc_status_t status;
    
	if (thread) {
		agc_mutex_lock(profile.mutex);
		profile.threads++;
		agc_mutex_unlock(profile.mutex);
	}
    
	if (!acs) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Internal error.\n");
		return NULL;
	}
    
	if (!acs->conn || !acs->conn->is_running ||
		!acs->conn->rwlock || agc_thread_rwlock_tryrdlock(acs->conn->rwlock) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Error! cannot get read lock.\n");
		acs->ack = -1;
		return NULL;
	}
    
	acs->ack = 1;
	agc_api_stand_stream(&stream);
    
	/*if (acs->console_execute) {
        //TODO
	} else {
		status = agc_api_execute(acs->api_cmd, acs->arg, &stream);
	}*/

	status = agc_api_execute(acs->api_cmd, acs->arg, &stream);
    
	if (status == AGC_STATUS_SUCCESS) {
		reply = stream.data;
	} else {
		freply = agc_mprintf("-ERR %s Command not found!\n", acs->api_cmd);
		reply = freply;
	}

	if (!reply) {
		reply = "Command returned no output!";
	}
    
	if (acs->bg) {
		//TODO
	} else {
		agc_size_t rlen, blen;
		char buf[1024] = "";

		if (!(rlen = strlen(reply))) {
			reply = "-ERR no reply\n";
			rlen = strlen(reply);
		}

		agc_snprintf(buf, sizeof(buf), "Content-Type: api/response\nContent-Length: %d" "\n\n", rlen);
		blen = strlen(buf);
		agc_socket_send(acs->conn->sock, buf, &blen);
		agc_socket_send(acs->conn->sock, reply, &rlen);
	}
    
	agc_safe_free(stream.data);
	agc_safe_free(freply);
    
	if (acs->conn->rwlock) {
		agc_thread_rwlock_unlock(acs->conn->rwlock);
	}
    
	if (thread) {
		agc_mutex_lock(profile.mutex);
		profile.threads--;
		agc_mutex_unlock(profile.mutex);
	}
    
    return NULL;
}
