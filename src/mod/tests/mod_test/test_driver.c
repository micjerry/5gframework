#include "mod_test.h"
//#include <net/if.h>

/*
epoll_client.c as the client to test driver
*/

#define TEST_DRIVER_LOCALADDR "127.0.0.1"
#define TEST_DRIVER_LOCALPORT 9000
#define TEST_MAX_BUFFER 2048

#define TEST_DRIVER_LOCALADDRIP6 "::1"

typedef struct {
	int intvalue;
	char buf[TEST_MAX_BUFFER];
} test_driver_context_t;

static agc_std_socket_t socket_bind(struct sockaddr* addr, socklen_t len);
static agc_std_socket_t socket_bindip6(struct sockaddr* addr, socklen_t );
static void handle_newconnection(agc_connection_t *c);

static void handle_read(void *data);
static void handle_write(void *data);
static void handle_error(void *data);

void test_driver_listen(agc_stream_handle_t *stream);
void test_driver_listen6(agc_stream_handle_t *stream);

void test_driver_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	test_driver_listen(stream);
	test_driver_listen6(stream);
}

void test_driver_listen(agc_stream_handle_t *stream)
{
	agc_std_socket_t listenfd;
	agc_memory_pool_t *pool = NULL;
	struct sockaddr_in servaddr;
	agc_listening_t *listening = NULL;
	agc_connection_t *connection = NULL;
	socklen_t addrSize;
	//struct sockaddr_in6 servaddr;

	addrSize = sizeof(struct sockaddr_in);
	//addrSize = sizeof(struct sockaddr_in6);
	
	memset(&servaddr, 0, addrSize);
	
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, TEST_DRIVER_LOCALADDR, &servaddr.sin_addr);
	servaddr.sin_port = htons(TEST_DRIVER_LOCALPORT);

	/* 
	servaddr.sin6_family = AF_INET6;
	inet_pton(AF_INET6, TEST_DRIVER_LOCALADDRIP6, &servaddr.sin6_addr);
	servaddr.sin6_port = htons(TEST_DRIVER_LOCALPORT);
	*/

	listenfd = socket_bind((struct sockaddr*)&servaddr, addrSize);

	//listenfd = socket_bind6((struct sockaddr*)&servaddr, addrSize);
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

	listening = agc_conn_create_listening(listenfd, (agc_std_sockaddr_t *)&servaddr, 
							addrSize, 
							pool,
							handle_newconnection);

	if (!listening) {
		stream->write_function(stream, "create listening failed.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "create listening failed .\n");
		agc_memory_destroy_pool(&pool);
		return;
	}

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

void test_driver_listen6(agc_stream_handle_t *stream)
{
	agc_std_socket_t listenfd;
	agc_memory_pool_t *pool = NULL;
	agc_listening_t *listening = NULL;
	agc_connection_t *connection = NULL;
	socklen_t addrSize;
	struct sockaddr_in6 servaddr;

	addrSize = sizeof(struct sockaddr_in6);
	
	memset(&servaddr, 0, addrSize);
	
	servaddr.sin6_family = AF_INET6;
	//servaddr.sin6_addr = in6addr_any;
	inet_pton(AF_INET6, TEST_DRIVER_LOCALADDRIP6, (void *)&servaddr.sin6_addr);
	//servaddr.sin6_scope_id = if_nametoindex("lo");
	servaddr.sin6_port = htons(TEST_DRIVER_LOCALPORT);
	
	listenfd = socket_bindip6((struct sockaddr*)&servaddr, addrSize);
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

	listening = agc_conn_create_listening(listenfd, (agc_std_sockaddr_t *)&servaddr, 
							addrSize, 
							pool,
							handle_newconnection);

	if (!listening) {
		stream->write_function(stream, "create listening failed.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "create listening failed .\n");
		agc_memory_destroy_pool(&pool);
		return;
	}

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


static agc_std_socket_t socket_bind(struct sockaddr* addr, socklen_t len)
{
	agc_std_socket_t listenfd = -1;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1)	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket create failed .\n");
		return listenfd;
	}
	
	if (bind(listenfd, addr, len) == -1) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket bind failed .\n");
		return -1;
	}

	return listenfd;
}

static agc_std_socket_t socket_bindip6(struct sockaddr* addr, socklen_t len)
{
	agc_std_socket_t listenfd = -1;

	listenfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (listenfd == -1)	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket create failed .\n");
		return listenfd;
	}
	
	if (bind(listenfd, addr, len) == -1) {
		perror("bind");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "socket bind failed .\n");
		return -1;
	}

	return listenfd;
}

static void handle_newconnection(agc_connection_t *c)
{
	agc_std_socket_t new_socket;
	//struct sockaddr_in new_addr;
	agc_std_sockaddr_t new_addr;
	socklen_t new_addr_len = sizeof(struct sockaddr_in);
	agc_connection_t *new_connection = NULL;
	agc_memory_pool_t *pool = NULL;
	test_driver_context_t *context;
	agc_std_sockaddr_t *listen_addr;
	agc_std_sockaddr_t *conn_addr;
	
	if (!c) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid new connection .\n");
		return;
	}

	//debug listen addr
	listen_addr = c->sockaddr;

	if (listen_addr) {
		if (listen_addr->ss_family == AF_INET) {
			struct sockaddr_in *listen_addr4 = (struct sockaddr_in *)listen_addr;
			char str_addr[64];
			inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "debug listen addr4 %s .\n", str_addr);

			new_addr_len = sizeof(struct sockaddr_in);
		} else {
			struct sockaddr_in6 *listen_addr6 = (struct sockaddr_in6 *)listen_addr;
			char str_addr[64];
			inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr, 64);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "debug listen addr6 %s .\n", str_addr);

			new_addr_len = sizeof(struct sockaddr_in6);
		}
	}

	//(struct sockaddr*)
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

	//debug conn addr
	conn_addr = new_connection->sockaddr;

	if (conn_addr) {
		if (conn_addr->ss_family == AF_INET) {
			struct sockaddr_in *clientaddr4 = (struct sockaddr_in *)conn_addr;
			char str_addr[64];
			inet_ntop(AF_INET, &(clientaddr4->sin_addr), str_addr, 64);
			
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "debug connection addr4 %s .\n", str_addr);
		} else {
			struct sockaddr_in6 *clientaddr6 = (struct sockaddr_in6 *)conn_addr;
			char str_addr[64];
			inet_ntop(AF_INET6, &(clientaddr6 ->sin6_addr), str_addr, 64);
			
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "debug connection addr6 %s .\n", str_addr);
		}		
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

