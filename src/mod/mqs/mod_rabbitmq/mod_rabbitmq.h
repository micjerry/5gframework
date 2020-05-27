#ifndef MOD_RABBITMQ_H
#define MOD_RABBITMQ_H

#include <agc.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>

#define MAX_MQ_ROUTING_KEY_LENGTH 255
#define MQ_DEFAULT_CONTENT_TYPE "text/json"

typedef struct {
    char routing_key[MAX_MQ_ROUTING_KEY_LENGTH];
    char *pjson;
} agcmq_message_t;

typedef struct agcmq_connection_info_s agcmq_connection_info_t;

struct agcmq_connection_info_s {
	char *name;
	char *hostname;
	char *virtualhost;
	char *username;
	char *password;
	unsigned int port;
	unsigned int heartbeat;
	agcmq_connection_info_t *next;
};

typedef struct agcmq_connection_s agcmq_connection_t;

struct agcmq_connection_s {
	agc_bool_t active;
	amqp_connection_state_t state;
};

typedef struct agcmq_conn_parameter_s agcmq_conn_parameter_t;
struct agcmq_conn_parameter_s {
	char *ex_name;
	char *ex_type;
	char *bind_key;
	char *queuename;
	unsigned int reconnect_interval_ms;
	unsigned int circuit_breaker_ms;
	unsigned int queue_size;
	agc_bool_t enable_fallback_format_fields;
	char *event_filter;
	unsigned int delivery_mode;
	unsigned int delivery_timestamp;
};

typedef struct agcmq_producer_profile_s {
	char *name;
	agcmq_conn_parameter_t *conn_parameter;
	agcmq_connection_info_t *conn_infos;
	agcmq_connection_t *mq_conn;
	
	agc_thread_t *producer_thread;
	agc_bool_t running;
	agc_memory_pool_t *pool;

	agc_time_t reset_time;

	agc_queue_t *send_queue;
	uint8_t event_list[EVENT_ID_LIMIT];
} agcmq_producer_profile_t;

typedef struct agcmq_consumer_profile_s {
	char *name;
	agcmq_conn_parameter_t *conn_parameter;
	agcmq_connection_info_t *conn_infos;
	agcmq_connection_t *mq_conn;	

	agc_thread_t *consumer_thread;
	agc_bool_t running;
	agc_memory_pool_t *pool;
} agcmq_consumer_profile_t;

struct {
	agc_memory_pool_t *pool;

	agc_hash_t *producer_hash;
	agc_hash_t *consumer_hash;
} agcmq_global;

void agcmq_producer_msg_destroy(agcmq_message_t **msg);

agc_status_t agcmq_producer_create(char *name, agcmq_connection_info_t *conn_infos, agcmq_conn_parameter_t *parameters, agc_memory_pool_t *pool);
agc_status_t agcmq_producer_destroy(agcmq_producer_profile_t **profile);
agc_status_t agcmq_producer_send(agcmq_producer_profile_t *producer, agcmq_message_t *msg);
void *agcmq_producer_thread(agc_thread_t *thread, void *data);

agc_status_t agcmq_consumer_create(char *name, agcmq_connection_info_t *conn_infos, agcmq_conn_parameter_t *parameters, agc_memory_pool_t *pool);
agc_status_t agcmq_consumer_destroy(agcmq_consumer_profile_t **profile);
void *agcmq_consumer_thread(agc_thread_t *thread, void *data);

int agcmq_parse_amqp_reply(amqp_rpc_reply_t x, char const *context);
agc_status_t agcmq_connection_open(agcmq_connection_info_t *conn_infos,  agcmq_connection_t *conn, char *profile_name);
void agcmq_connection_close(agcmq_connection_t *conn);

static inline agc_bool_t agcmq_is_conn_active(agcmq_connection_t *conn)
{
	return conn->active;
}

agc_status_t agcmq_load_config();

#endif

