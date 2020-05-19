#ifndef MOD_EVENTSOCKET_H
#define MOD_EVENTSOCKET_H

#include <agc.h>


#define EVTSKT_BLOCK_LEN 2048
#define EVTSKT_MAX_LEN 10485760

typedef struct event_connect_s event_connect_t;

struct event_connect_s {
    agc_socket_t *sock;
    agc_queue_t *event_queue;
    agc_memory_pool_t *pool;
    agc_pollfd_t *pollfd;
    agc_thread_rwlock_t *rwlock;
    agc_sockaddr_t *sa;
    char remote_ip[64];
    agc_port_t remote_port;
    uint8_t has_event;
    uint8_t is_running;
    char *ebuf;
    uint8_t event_list[EVENT_ID_LIMIT];
    event_connect_t *next;
};

typedef struct event_listener_s event_listener_t;

struct event_listener_s {
    agc_socket_t *sock;
    agc_mutex_t *sock_mutex;
    uint8_t ready;
    event_connect_t *connections;
};

typedef struct eventsocket_profile_s eventsocket_profile_t;

struct eventsocket_profile_s {
    char ip[64];
    uint16_t port;
    int done;
    int threads;
    agc_mutex_t *mutex;
};

typedef struct api_command_s api_command_t;

struct api_command_s {
	char *api_cmd;
	char *arg;
	event_connect_t *conn;
	char uuid_str[AGC_UUID_FORMATTED_LENGTH + 1];
	int bg;
	int ack;
	int console_execute;
	agc_memory_pool_t *pool;
};

extern eventsocket_profile_t profile;

agc_status_t read_packet(event_connect_t *conn, agc_event_t **event);
agc_status_t parse_command(event_connect_t *conn, agc_event_t **event, char *reply, uint32_t reply_len);
void strip_cr(char *s);

#endif