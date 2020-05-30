#include "mod_test.h"

/*
epoll_client.c to test driver
*/

#define TEST_DRIVER_LOCALADDR "127.0.0.1"
#define TEST_DRIVER_LOCALPORT 9000
#define TEST_MAX_BUFFER 2048

typedef struct {
	int intvalue;
	char buf[TEST_MAX_BUFFER];
} test_driver_context_t;

static agc_std_socket_t socket_bind(struct sockaddr* addr);
static void handle_newconnection(agc_connection_t *c);

static void handle_read(void *data);
static void handle_write(void *data);
static void handle_error(void *data);

void test_driver_listen(agc_stream_handle_t *stream);

void test_driver_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	test_driver_listen(stream);
}

void test_driver_listen(agc_stream_handle_t *stream)
{
	agc_std_socket_t listenfd;
	agc_memory_pool_t *pool = NULL;
	struct sockaddr_in servaddr;
	agc_listening_t *listening = NULL;
	agc_connection_t *connection = NULL;
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, TEST_DRIVER_LOCALADDR, &servaddr.sin_addr);
	servaddr.sin_port = htons(TEST_DRIVER_LOCALPORT);

	listenfd = socket_bind((struct sockaddr*)&servaddr);
	if (listenfd == -1) {
		stream->write_function(stream, "socket bind failed.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket bind failed .\n");
		return;
	}

	if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "alloc memory failed.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "alloc memory failed .\n");
		return;
	}

	listening = agc_conn_create_listening((agc_std_sockaddr_t *)&servaddr, 
							sizeof(servaddr), 
							pool,
							handle_newconnection);

	if (!listening) {
		stream->write_function(stream, "create listening failed.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "create listening failed .\n");
		agc_memory_destroy_pool(&pool);
		return;
	}

	listening->fd = listenfd;
	listen(listening->fd, 10);

	connection = agc_conn_get_connection(listening);

	if (!connection) {
		stream->write_function(stream, "create connection failed.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "create connection failed .\n");
		agc_memory_destroy_pool(&pool);
		return;
	}

	if (agc_diver_add_connection(connection) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "add connection failed.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "add connection failed .\n");
		agc_memory_destroy_pool(&pool);
		return;
	}

	stream->write_function(stream, "add connection ok.\n");
}

static agc_std_socket_t socket_bind(struct sockaddr* addr)
{
	agc_std_socket_t listenfd = -1;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1)	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket create failed .\n");
		return listenfd;
	}
	
	if (bind(listenfd, addr, sizeof(*addr)) == -1) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket bind failed .\n");
		return -1;
	}

	return listenfd;
}

static void handle_newconnection(agc_connection_t *c)
{
	agc_std_socket_t new_socket;
	struct sockaddr_in new_addr;
	socklen_t new_addr_len;
	agc_connection_t *new_connection = NULL;
	agc_memory_pool_t *pool = NULL;
	test_driver_context_t *context;
	
	if (!c) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid new connection .\n");
		return;
	}

	new_socket = accept(c->fd, (struct sockaddr*)&new_addr, &new_addr_len);

	if (new_socket == -1) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "accept socket failed .\n");
		return;
	}

	if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "alloc memory failed .\n");
		return;
	}

	context = (test_driver_context_t *)agc_memory_alloc(pool, sizeof(*context));

	assert(context);

	context->intvalue = 99;

	new_connection = agc_conn_create_connection(new_socket, 
                                                      (agc_std_sockaddr_t *)&new_addr, 
                                                      new_addr_len, 
                                                      pool,
                                                      context, 
                                                      handle_read,
                                                      handle_write,
                                                      handle_error);

	if (!new_connection) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "alloc memory failed .\n");
		agc_memory_destroy_pool(&pool);
		return;
	}

	if (agc_diver_add_connection(new_connection) != AGC_STATUS_SUCCESS ) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "add connection failed .\n");
		return;
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "new connection %d add .\n", new_socket);
}


static void handle_read(void *data)
{
	agc_connection_t *connection = (agc_connection_t *)data;
	int reads = 0;
	int writes = 0;
	test_driver_context_t *context = NULL;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "handle_read enter.\n");

	if (!connection) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid connection read failed .\n");
		return;
	}

	context = connection->context;

	if (!context) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid connection no context .\n");
		return;
	}
	
	memset(context->buf, 0, TEST_MAX_BUFFER);
	
	reads = read(connection->fd, context->buf, TEST_MAX_BUFFER);

	if (reads == -1 || reads == 0) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "read socket exception .\n");
		agc_diver_del_connection(connection);
		close(connection->fd);
		agc_memory_destroy_pool(&connection->pool);
		return;
	}

	agc_diver_add_event(connection, AGC_WRITE_EVENT);

	if (reads < TEST_MAX_BUFFER) {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "read message %s .\n", context->buf);
	} else {
		//clear left data
		while (reads > 0) {
			reads = read(connection->fd, context->buf, TEST_MAX_BUFFER);
			if (reads == -1) {
				agc_diver_del_connection(connection);
				close(connection->fd);
				agc_memory_destroy_pool(&connection->pool);
				break;
			}
		}
	}
}

static void handle_write(void *data)
{
	int writes = 0;
	agc_connection_t *connection = (agc_connection_t *)data;
	test_driver_context_t *context = NULL;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "handle_write enter .\n");

	if (!connection) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "read message %s .\n", context->buf);
		return;
	}

	context = connection->context;

	if (!context) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid connection no context .\n");
		return;
	}
	
	if (strlen(context->buf) > 0) {
		if (write(connection->fd, context->buf, strlen(context->buf)) == -1) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "write failed %d.\n",  connection->fd);
			agc_diver_del_connection(connection);
			close(connection->fd);
			agc_memory_destroy_pool(&connection->pool);
			return;
		} 
	}

	agc_diver_del_event(connection, AGC_WRITE_EVENT);
	
}

static void handle_error(void *data)
{
	agc_connection_t *connection = (agc_connection_t *)data;

	if (!connection) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid connection handle_error failed .\n");
		return;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "connection %d deleted .\n", connection->fd);
	agc_diver_del_connection(connection);
	close(connection->fd);
	agc_memory_destroy_pool(&connection->pool);
}

