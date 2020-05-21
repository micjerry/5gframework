#include <agc.h>
#include <yaml.h>
#include <hiredis.h>
#include <hircluster.h>

AGC_MODULE_LOAD_FUNCTION(mod_redis_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown);
AGC_MODULE_DEFINITION(mod_redis, mod_redis_load, mod_redis_shutdown, NULL);

#define REDIS_CONFIG_FILE "hiredis.yml"
#define MAX_ADDRS_LEN 512
#define MIN_POOL_CONNECTIONS 2
#define MAX_POOL_CONNECTIONS 32
#define MAX_POOL_RETRIES 5

static agc_memory_pool_t *module_pool = NULL;
static char *redis_addrs = NULL;
static uint32_t CONNECTION_POOL_SIZE = 0;

static volatile int SYSTEM_RUNNING = 0;

static agc_status_t agc_cache_redis_set(const char *key, const char *value, int size, int expires);

static agc_status_t agc_cache_redis_get(const char *key, char **result, int *len);

static agc_status_t agc_cache_redis_delete(const char *key);

static agc_status_t agc_cache_redis_set_pipeline(keyvalues_t *keyvalues);

static agc_status_t agc_cache_redis_get_pipeline(keys_t *keys, keyvalues_t **keyvalues);

static agc_status_t agc_cache_redis_delete_pipeline(keys_t *keys);

static agc_status_t agc_cache_redis_expire(const char *key, int expires);

static agc_status_t agc_cache_redis_incr(const char *key, int *value);

static agc_status_t agc_cache_redis_decr(const char *key, int *value);

static agc_status_t agc_cache_redis_hashset(const char *tablename, const char *key, const char *value);

static agc_status_t agc_cache_redis_hashget(const char *tablename, const char *key, char **value, int *len);

static agc_status_t agc_cache_redis_hashmset(const char *tablename, keyvalues_t *keyvalues);
	
static agc_status_t agc_cache_redis_hashmget(const char *tablename, keys_t *keys,  keyvalues_t **keyvalues);

static agc_status_t agc_cache_redis_hashdel(const char *tablename,  keys_t *keys);

static agc_cache_actions_t agc_cache_routine = {
    agc_cache_redis_set,
    agc_cache_redis_get,
    agc_cache_redis_delete,
    agc_cache_redis_set_pipeline,
    agc_cache_redis_get_pipeline,
    agc_cache_redis_delete_pipeline,
    agc_cache_redis_expire,
    agc_cache_redis_incr,
    agc_cache_redis_decr,
    agc_cache_redis_hashset,
    agc_cache_redis_hashget,
    agc_cache_redis_hashmset,
    agc_cache_redis_hashmget,
    agc_cache_redis_hashdel
};

typedef struct agc_redis_connection_s agc_redis_connection_t;

struct agc_redis_connection_s {
	uint8_t connected;
	redisClusterContext *cc;
	agc_time_t connect_time;
};

static agc_queue_t *REDIS_CONTEXT_QUEUE;

static agc_status_t load_configuration();

static agc_status_t initial_connection_pool();

static void destroy_connection_pool();

static agc_status_t create_connection();

static agc_redis_connection_t *get_connection();

static void free_connection(agc_redis_connection_t *connection);

static agc_bool_t check_connection(agc_redis_connection_t *connection);

static agc_time_t next_connect_time();

static agc_status_t create_context(agc_redis_connection_t *connection);
static void release_context(agc_redis_connection_t *connection);

static void get_strvalue(redisReply *reply, char **result,  int *len);
static void get_intvalue(redisReply *reply, int *result);
static void log_error(redisReply *reply);

static agc_status_t agc_redis_multireplies(agc_redis_connection_t *connection, int replies);
static redisReply *agc_redis_hashargv(agc_redis_connection_t *connection, const char *cmd, const char *tablename, keys_t *keys, keyvalues_t *keyvalues);
	
AGC_MODULE_LOAD_FUNCTION(mod_redis_load)
{
	assert(pool);
	module_pool = pool;
	
	 *module_interface = agc_loadable_module_create_interface(module_pool, modname);
 
	if (load_configuration() !=  AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Redis load configuration failed.\n");
		return AGC_STATUS_GENERR;
	}
	
	SYSTEM_RUNNING = 1;
	
	initial_connection_pool();
	agc_cache_register_impl(&agc_cache_routine);
	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Redis init success.\n");
	return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown)
{
	agc_redis_connection_t *connection = NULL;
	
	SYSTEM_RUNNING = 0;

	destroy_connection_pool();
	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Redis shutdown success.\n");
	return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_cache_redis_set(const char *key, const char *value, int size, int expires)
{
	agc_redis_connection_t *connection = NULL;
	redisReply *reply;
	agc_status_t status =  AGC_STATUS_GENERR;
	int cmds = 1;

	if (!(connection = get_connection())) {
		return status;
	}

	if (redisClusterAppendCommand(connection->cc, "SET %s %s",  key, value) != REDIS_OK)  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "redis append command failed %s.\n", connection->cc->errstr);
		goto end;
	}

	if (expires > 0) {
		cmds++;
		if (redisClusterAppendCommand(connection->cc, "EXPIRE %s %d", key, expires) != REDIS_OK) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "redis append command failed %s.\n", connection->cc->errstr);
		}
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "redis refresh.\n");

end:
	status = agc_redis_multireplies(connection, cmds);
	
	free_connection(connection);
	
	return status;
}

static agc_status_t agc_cache_redis_get(const char *key, char **result, int *len)
{
	agc_redis_connection_t *connection = NULL;
	redisReply *reply;
	char *result_ptr = NULL;
	agc_status_t status =  AGC_STATUS_GENERR;

	if (!result || !len) {
		return status;
	}

	if (!(connection = get_connection())) {
		return status;
	}

	reply = redisClusterCommand(connection->cc, "GET %s", key);
	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
		get_strvalue(reply, result, len);
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);
	
	return status;

}


static agc_status_t agc_cache_redis_delete(const char *key)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status =  AGC_STATUS_GENERR;

	if (!(connection = get_connection())) {
		return status;
	}

	reply = redisClusterCommand(connection->cc, "DEL %s", key);
	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);
	
	return status;
}

static agc_status_t agc_cache_redis_set_pipeline(keyvalues_t *keyvalues)
{
	agc_redis_connection_t *connection  = NULL;
	agc_status_t status =  AGC_STATUS_GENERR;
	redisReply *reply;
	keyvalues_t *iter;
	int cmds = 0;

	if (!(connection = get_connection())) {
		return status;
	}

	for (iter = keyvalues; iter; iter = iter->next) {
		cmds++;
		redisClusterAppendCommand(connection->cc, "SET %s %s", iter->key, iter->value);
	}

	status = agc_redis_multireplies(connection, cmds);

	free_connection(connection);

	return status;
	
}

static agc_status_t agc_cache_redis_get_pipeline(keys_t *keys, keyvalues_t **keyvalues)
{	
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keys_t *iter;
	keyvalues_t *header = NULL;
	keyvalues_t *new_keyvalue = NULL;
	keyvalues_t *tail= NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;

	if (!keys || !keyvalues) 
		return AGC_STATUS_GENERR;

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	for (iter = keys; iter; iter = iter->next)  {
		redisClusterAppendCommand(connection->cc, "GET %s", iter->key);
	}

	for (iter = keys; iter; iter = iter->next) {
		redisClusterGetReply(connection->cc, (void **)&reply);
		if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
			status = AGC_STATUS_GENERR;
			log_error(reply);
			freeReplyObject(reply);
			break;
		}
		
		new_keyvalue = malloc(sizeof(keyvalues_t));
		memset(new_keyvalue, 0, sizeof(keyvalues_t));
		if (!header) {
			header = new_keyvalue;
		} 

		if (tail) {
			tail->next = new_keyvalue;
		}

		tail = new_keyvalue;

		new_keyvalue->key = strdup(iter->key);
		get_strvalue(reply, &new_keyvalue->value, &new_keyvalue->value_len);
		
		freeReplyObject(reply);
			
	}

	if (connection->cc)
		redisClusterReset(connection->cc);

	free_connection(connection);

	*keyvalues = header;

	return status;
	
}

static agc_status_t agc_cache_redis_delete_pipeline(keys_t *keys)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keys_t *iter;
	int cmds = 0;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!keys)
		return  status;

	if (!(connection = get_connection())) {
		return status;
	}

	for (iter = keys; iter; iter = iter->next)  {
		cmds++;
		redisClusterAppendCommand(connection->cc, "DEL %s", iter->key);
	}

	status = agc_redis_multireplies(connection, cmds);

	free_connection(connection);

	return status;
}

static agc_status_t agc_cache_redis_expire(const char *key, int expires)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!key || (expires <=0 )) {
		return status;
	}

	if (!(connection = get_connection())) {
		return status;
	}

	reply = redisClusterCommand(connection->cc, "EXPIRE %s %d", key, expires);
	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);
	
	return status;
	
}

static agc_status_t agc_cache_redis_incr(const char *key, int *value)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!key) {
		return status;
	}
		
	if (!(connection = get_connection())) {
		return status;
	}

	reply = redisClusterCommand(connection->cc, "INCR %s", key);

	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
		get_intvalue(reply, value);
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);
	
	return status;
}

static agc_status_t agc_cache_redis_decr(const char *key, int *value)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!key) {
		return status;
	}
		
	if (!(connection = get_connection())) {
		return status;
	}

	reply = redisClusterCommand(connection->cc, "DECR %s", key);

	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
		get_intvalue(reply, value);
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);
	
	return status;
}

static agc_status_t agc_cache_redis_hashset(const char *tablename, const char *key, const char *value)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!tablename || !key || !value) {
		return status;
	}

	if (!(connection = get_connection())) {
		return status;
	}

	reply = redisClusterCommand(connection->cc, "HSET %s %s %s", tablename, key, value);

	if (reply == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "hset return null %s.\n", connection->cc->errstr);
	}

	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);
	
}

static agc_status_t agc_cache_redis_hashget(const char *tablename, const char *key, char **value, int *len)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!tablename || !key || !value || !len) {
		return status;
	}

	if (!(connection = get_connection())) {
		return status;
	}

	reply = redisClusterCommand(connection->cc, "HGET %s %s", tablename, key);

	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		get_strvalue(reply, value, len);
		status = AGC_STATUS_SUCCESS;
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);

	return status;
}

static agc_status_t agc_cache_redis_hashmset(const char *tablename, keyvalues_t *keyvalues)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!tablename || !keyvalues) {
		return status;
	}

	if (!(connection = get_connection())) {
		return status;
	}

	reply = agc_redis_hashargv(connection, "HMSET", tablename, NULL, keyvalues); 

	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);
	
	return status;
	
}

static agc_status_t agc_cache_redis_hashmget(const char *tablename, keys_t *keys,  keyvalues_t **keyvalues)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	int i;
	keys_t *iter;
	keyvalues_t *header = NULL;
	keyvalues_t *new_keyvalue = NULL;
	keyvalues_t *tail= NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!tablename || !keys) {
		return status;
	}	

	if (!(connection = get_connection())) {
		return status;
	}

	reply = agc_redis_hashargv(connection, "HMGET", tablename, keys, NULL); 
	
	free_connection(connection);

	if (!reply || ( reply->type == REDIS_REPLY_ERROR) || (reply->type != REDIS_REPLY_ARRAY)) {
		freeReplyObject(reply);
		log_error(reply);
		return status;
	}

	for (i = 0, iter = keys; iter; iter = iter->next, i++) {
		new_keyvalue = malloc(sizeof(keyvalues_t));
		memset(new_keyvalue, 0, sizeof(keyvalues_t));

		if (!header) {
			header = new_keyvalue;
		} 

		if (tail) {
			tail->next = new_keyvalue;
		}

		tail = new_keyvalue;

		new_keyvalue->key = strdup(iter->key);
		new_keyvalue->value = strndup(reply->element[i]->str, reply->element[i]->len);
		new_keyvalue->value_len = reply->element[i]->len;	
	}

	*keyvalues = header;

	freeReplyObject(reply);
	status = AGC_STATUS_SUCCESS;
	
	return status;
}

static agc_status_t agc_cache_redis_hashdel(const char *tablename, keys_t *keys)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!tablename || !keys) {
		return status;
	}

	if (!(connection = get_connection())) {
		return status;
	}

	reply = agc_redis_hashargv(connection, "HDEL", tablename, keys, NULL); 

	free_connection(connection);

	if (reply != NULL && reply->type != REDIS_REPLY_ERROR) {
		status = AGC_STATUS_SUCCESS;
	} else {
		log_error(reply);
	}

	freeReplyObject(reply);

	return status;
}

static agc_status_t load_configuration()
{
	char *filename;
	char *pos;
	FILE *file;
	yaml_parser_t parser;
	yaml_token_t token;
	int done = 0;
	int error = 0;
	int iskey = 0;
	enum {
		KEYTYPE_POOLSIZE,
		KEYTYPE_ADDR,
		KEYTYPE_UNKOWN
	} keytype = KEYTYPE_UNKOWN;
	
	const char *err;


	redis_addrs = agc_memory_alloc(module_pool, MAX_ADDRS_LEN);
	pos = redis_addrs;
	filename = agc_mprintf("%s%s%s", AGC_GLOBAL_dirs.conf_dir, AGC_PATH_SEPARATOR, REDIS_CONFIG_FILE);
	
	file = fopen(filename, "rb");
	assert(file);
	assert(yaml_parser_initialize(&parser));
    
	yaml_parser_set_input_file(&parser, file);
    
	while (!done)
	{
		if (!yaml_parser_scan(&parser, &token)) {
			error = 1;
			break;
		}
        
		switch(token.type)
		{
			case YAML_KEY_TOKEN:
				iskey = 1;
				break;
			case YAML_VALUE_TOKEN:
				iskey = 0;
				break;
			case YAML_SCALAR_TOKEN:
				{
					if (iskey)
					{
						if (strcmp(token.data.scalar.value, "poolsize") == 0)
						{
							keytype = KEYTYPE_POOLSIZE;
						} else if (strcmp(token.data.scalar.value, "addr") == 0)
						{
							keytype = KEYTYPE_ADDR;
						} else {
							keytype = KEYTYPE_UNKOWN;
						}
					} else {
						if (keytype == KEYTYPE_ADDR) {
							strcpy(pos, token.data.scalar.value);
							pos += strlen(token.data.scalar.value);
							*pos = ',';
							pos++;
						} else if (keytype == KEYTYPE_POOLSIZE) {
							CONNECTION_POOL_SIZE = agc_atoui(token.data.scalar.value);
							if (CONNECTION_POOL_SIZE > MAX_POOL_CONNECTIONS)
								CONNECTION_POOL_SIZE = MAX_POOL_CONNECTIONS;
							if (CONNECTION_POOL_SIZE < MIN_POOL_CONNECTIONS)
								CONNECTION_POOL_SIZE = MIN_POOL_CONNECTIONS;
						} else {
						}
					}
				}
				break;
			default:
				break;
		}
	    
		done = (token.type == YAML_STREAM_END_TOKEN);
		yaml_token_delete(&token);
	}

	yaml_parser_delete(&parser);
	assert(!fclose(file));	

	agc_safe_free(filename);

	if (*(pos -1) == ',') {
		*(pos -1) = '\0';
	}

	return AGC_STATUS_SUCCESS;
}


static agc_status_t initial_connection_pool()
{
	int i;
	redisClusterContext *cc;
	
	agc_queue_create(&REDIS_CONTEXT_QUEUE, MAX_POOL_CONNECTIONS * 2,  module_pool);

	for (i = 0; i < CONNECTION_POOL_SIZE; i++) {
		if (create_connection() != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create redis connection failed %s.\n", redis_addrs);
			return AGC_STATUS_GENERR;
		}
	}

	return AGC_STATUS_SUCCESS;
}

static void destroy_connection_pool()
{
	agc_redis_connection_t *connection = NULL;

	while ((connection = get_connection())) {
		release_context(connection);
	}
}

static agc_status_t create_connection()
{
	agc_redis_connection_t *connection = NULL;
	
	if ( !SYSTEM_RUNNING)
		return AGC_STATUS_SUCCESS;

	connection = (agc_redis_connection_t *)agc_memory_alloc(module_pool,  sizeof(agc_redis_connection_t));

	assert(connection);
	
	create_context(connection) ;

	agc_queue_push(REDIS_CONTEXT_QUEUE, connection);
	return AGC_STATUS_SUCCESS;
}

static agc_redis_connection_t *get_connection()
{
	void *pop = NULL;
	agc_redis_connection_t *connection;
	int retries = 0;
	
	if (!SYSTEM_RUNNING)
		return NULL;

	while ((agc_queue_trypop(REDIS_CONTEXT_QUEUE, &pop) != AGC_STATUS_SUCCESS) && (retries < MAX_POOL_RETRIES)) {
		retries++;
		agc_yield(10000);
	}

	if (!pop) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Can not get connection.\n");
		return NULL;
	}

	connection = (agc_redis_connection_t *)pop;
	if (!check_connection(connection)) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "The connection can not create.\n");
		agc_queue_push(REDIS_CONTEXT_QUEUE, connection);
		return NULL;
	}

	return connection;
}

static void free_connection(agc_redis_connection_t *connection)
{
	if (!connection)
		return;
	
	if (!SYSTEM_RUNNING) {
		release_context(connection);
		return;
	}

	if (connection->cc && connection->cc->err) {
		release_context(connection);
	}

	agc_queue_push(REDIS_CONTEXT_QUEUE, connection);
}

static agc_bool_t check_connection(agc_redis_connection_t *connection)
{		
	if (!connection)
		return AGC_FALSE;

	if (connection->connected)
		return AGC_TRUE;

	//try to reconnect redis
	if (connection->connect_time > (agc_timer_curtime() / 1000)) {	
		if (create_context(connection) == AGC_STATUS_SUCCESS) {
			return AGC_TRUE;
		}
	}

	return AGC_FALSE;
}

static agc_status_t create_context(agc_redis_connection_t *connection)
{
	redisClusterContext *cc = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!connection)
		return status;

	if (connection->cc) {
		redisClusterFree(connection->cc);
		connection->cc = NULL;
	}

	cc = redisClusterContextInit();
	if (cc) {
		redisClusterSetOptionAddNodes(cc, redis_addrs);
		redisClusterSetOptionRouteUseSlots(cc);
		redisClusterConnect2(cc);
		if (cc->err) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create redis context failed %s.\n", cc->errstr);
			connection->connected = 0;
			connection->cc = NULL;
			connection->connect_time = next_connect_time();
			redisClusterFree(cc);
		} else {
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Create redis context success %s.\n", redis_addrs);
			connection->connected = 1;
			connection->cc = cc;
			connection->connect_time = 0;
			status = AGC_STATUS_SUCCESS;
		}		
	}

	return status;
}

static void release_context(agc_redis_connection_t *connection)
{
	if (!connection)
		return;
	
	if (connection->cc)
		redisClusterFree(connection->cc);

	connection->cc = NULL;
	connection->connected = 0;
	connection->connect_time = next_connect_time();
}

static agc_time_t next_connect_time()
{
	return (agc_time_now() / 1000) + 10000;
}

static void get_strvalue(redisReply *reply, char **result, int *len)
{
	char *result_ptr = NULL;
	
	if (!reply || !result || !len)
		return;
	
	if (reply->type == REDIS_REPLY_STRING) {
		*result = strndup(reply->str, reply->len);
		*len = reply->len;
	}  else {
		*result = NULL;
		*len = 0;
	}
}

static void get_intvalue(redisReply *reply, int *result)
{
	if (!reply || !result)
		return;

	if (reply->type == REDIS_REPLY_INTEGER) {
		*result = (int)reply->integer;
	}
}

static agc_status_t agc_redis_multireplies(agc_redis_connection_t *connection, int replies)
{
	agc_status_t status = AGC_STATUS_SUCCESS;
	redisReply *reply;
	int i;

	assert(connection);
	for (i = 0; i < replies; i++) {
		if (redisClusterGetReply(connection->cc,  (void **)&reply) != REDIS_OK) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "redis get reply failed %s.\n", connection->cc->err);
			status = AGC_STATUS_GENERR;
			break;
		}

		if (reply == NULL) {
			status = AGC_STATUS_GENERR;
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "redis no reply.\n");
			break;
		}

		if (reply->type == REDIS_REPLY_ERROR) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "redis get reply failed %s.\n", reply->str);
			status = AGC_STATUS_GENERR;
			freeReplyObject(reply);
			break;
		}

		freeReplyObject(reply);
	}

	if (connection->cc)
		redisClusterReset(connection->cc);

	return status;
}

static redisReply *agc_redis_hashargv(agc_redis_connection_t *connection, const char *cmd, const char *tablename, keys_t *keys, keyvalues_t *keyvalues)
{
	int keys_count = 0;
	int argcount, len, i;
	keys_t *iter;
	keyvalues_t *iter_kv;
	char **args = NULL;
	agc_size_t *argvlen = NULL;
	redisReply *reply;
	
	if (!connection || !cmd || !tablename)
		return NULL;

	if (!keys && !keyvalues)
		return NULL;

	if (keys && keyvalues)
		return NULL;

	if (keys) {
		for (iter = keys; iter; iter = iter->next) {
			keys_count++;
		}	
		 // command + tablename + (number of keys)
		argcount = 2 + keys_count;
	} else {
		for (iter_kv = keyvalues; iter_kv; iter_kv = iter_kv->next) {
			keys_count++;
		}	
		 // command + tablename + (number of key and value)
		argcount = 2 + keys_count  * 2;
	}
	
	args = malloc(argcount * sizeof(char *));
	argvlen = malloc(argcount * sizeof(agc_size_t));

	len = strlen(cmd);
	args[0] = malloc(len);
    	argvlen[0] = len;
    	memcpy(args[0], cmd, len);

	len = strlen(tablename);
	args[1] = malloc(len);
	argvlen[1] = len ;
	memcpy(args[1], tablename, len);

	if (keys) { 
		for (i=0, iter = keys; i < keys_count; i++, iter = iter->next) {
			int argsidx = 2 + i;
			int key_len = strlen(iter->key);

			args[argsidx] = malloc(key_len * sizeof(char));
			memcpy(args[argsidx], iter->key, key_len);
			argvlen[argsidx] = key_len;
		}	
	} else {
		for (i=0, iter_kv = keyvalues; i < keys_count; i++, iter_kv = iter_kv->next) {
			int argsidx = 2 + (i*2);
			int key_len = strlen(iter_kv->key);
			int value_len = strlen(iter_kv->value);
			
			args[argsidx] = malloc(key_len * sizeof(char));
			args[argsidx+1] = malloc(value_len * sizeof(char));
			memcpy(args[argsidx], iter_kv->key, key_len);
			memcpy(args[argsidx+1], iter_kv->value, value_len);
			argvlen[argsidx] = key_len;
			argvlen[argsidx+1] = value_len;
		}
	}

	
	reply =  redisClusterCommandArgv(connection->cc, argcount, (const char**)args, argvlen);

	//release argvs
	for (i=0; i < argcount; i++) {
		agc_safe_free(args[i]);
	}

	agc_safe_free(args);
	agc_safe_free(argvlen);	

	return reply;
}

static void log_error(redisReply *reply)
{
	if (!reply) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "reply is null.\n");
		return;
	}

	if (reply->type == REDIS_REPLY_ERROR) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "reply error %s.\n", reply->str);
	}
}
