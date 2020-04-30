#include <agc.h>
#include <hiredis.h>

AGC_MODULE_LOAD_FUNCTION(mod_redis_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown);
AGC_MODULE_DEFINITION(mod_redis, mod_redis_load, mod_redis_shutdown, NULL);

#define REDIS_CONFIG_FILE "hiredis.yml"
#define MAX_ADDRS_LEN 512
#define MIN_POOL_CONNECTIONS 2
#define MAX_POOL_CONNECTIONS 32

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
	
static agc_status_t agc_cache_redis_hashmget(const char *tablename, keys_t *keys,  keyvalues_t *keyvalues);

static agc_status_t agc_cache_redis_hashdel(const char *tablename,  keys_t *keys);

static agc_status_t agc_cache_redis_hashkeys(const char *tablename, keys_t **keys);

static agc_status_t agc_cache_redis_hashgetall(const char *tablename, keyvalues_t **keyvalues);

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
    agc_cache_redis_hashdel,
    agc_cache_redis_hashkeys,
    agc_cache_redis_hashgetall
};

typedef struct agc_redis_connection_s agc_redis_connection_t;

struct agc_redis_connection_s {
	uint8_t connected;
	redisClusterContext *cc;
	agc_time_t connect_time;
}

static agc_queue_t *REDIS_CONTEXT_QUEUE;

static agc_status_t load_configuration();

static agc_status_t initial_connection_pool();

static void destroy_connection_pool();

static agc_status_t add_connection(agc_bool_t check);

static agc_redis_connection_t *get_connection();

static void free_connection(agc_redis_connection_t *connection, redisReply *reply);

static agc_status_t decodeResult(redisReply *reply);

static agc_bool_t check_connection(agc_redis_connection_t *connection);

static agc_time_t next_connect_time();

static agc_bool_t is_connection_break(redisReply *reply);

static void create_context(agc_redis_connection_t *connection);
static void release_context(agc_redis_connection_t *connection);

static void get_strvalue(redisReply *reply, char **result);
static void get_intvalue(redisReply *reply, int *result);

AGC_MODULE_LOAD_FUNCTION(mod_redis_load)
{
	module_pool = pool;
	load_configuration();
	
	SYSTEM_RUNNING = 1;
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Redis init success.\n");
	return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown)
{
	SYSTEM_RUNNING = 0;

	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Redis shutdown success.\n");
	return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_cache_redis_set(const char *key, const char *value, int size, int expires)
{
	agc_redis_connection_t *connection = NULL;
	redisReply *reply;
	agc_status_t status =  AGC_STATUS_GENERR;
	int failed = 0;
	int i;
	int cmds = 1;

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	redisClusterAppendCommand(connection->cc, "SET %s %s",  key, value);

	if (expires > 0) {
		cmds++;
		redisClusterAppendCommand(connection->cc, "EXPIRE %s %d", key, expires);
	}

	for (i =0; i < cmds; i++) {
		redisClusterGetReply(connection->cc, &reply);

		if (is_connection_break(reply)) { // connection error, skip
			failed++;
			release_context(connection);
			break;
		}

		if (decodeResult(reply) != AGC_STATUS_SUCCESS)
			failed++;
	
	}
	
	free_connection(connection, NULL);
	
	return failed ? AGC_STATUS_GENERR : AGC_STATUS_SUCCESS;
}

static agc_status_t agc_cache_redis_get(const char *key, char **result, int *len)
{
	agc_redis_connection_t *connection = NULL;
	redisReply *reply;
	char *result_ptr = NULL;

	if (!result || !len) {
		return AGC_STATUS_GENERR;
	}

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	reply = redisClusterCommand(connection->cc, "GET %s", key);
	free_connection(connection, reply);

	if (!reply)
		return AGC_STATUS_GENERR;

	get_strvalue(reply, result, len);
	
	return decodeResult(reply);

}


static agc_status_t agc_cache_redis_delete(const char *key)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	reply = redisClusterCommand(connection->cc, "DEL %s", key);
	free_connection(connection, reply);

	
	return decodeResult(reply);
}

static agc_status_t agc_cache_redis_set_pipeline(keyvalues_t *keyvalues)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keyvalues_t *l_keyvalue;
	int failed = 0;

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	for (l_keyvalue = keyvalues; l_keyvalue; l_keyvalue = l_keyvalue->next) {
		redisClusterAppendCommand(connection->cc, "SET %s %s", l_keyvalue->key, l_keyvalue->value);
	}

	for (l_keyvalue = keyvalues; l_keyvalue; l_keyvalue = l_keyvalue->next) {
		redisClusterGetReply(connection->cc, &reply);
		if (is_connection_break(reply)) { // connection error, skip
			failed++;
			release_context(connection);
			break;
		}
			
		if (decodeResult(reply) != AGC_STATUS_SUCCESS)
			failed++;
	}

	if (connection->cc)
		redisClusterReset(connection->cc);

	free_connection(connection, NULL);

	return failed ? AGC_STATUS_GENERR : AGC_STATUS_SUCCESS;
	
}

static agc_status_t agc_cache_redis_get_pipeline(keys_t *keys, keyvalues_t **keyvalues)
{	
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keys_t *l_key;
	keyvalues_t *header = NULL;
	keyvalues_t *l_keyvalue = NULL;
	keyvalues_t *tail= NULL;
	int failed = 0;

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	for (l_key = keys; l_key; l_key = l_key->next)  {
		redisClusterAppendCommand(connection->cc, "GET %s", l_key->key);
	}

	for (l_key = keys; l_key; l_key = l_key->next) {
		redisClusterGetReply(connection->cc, &reply);
		if (is_connection_break(reply)) { // connection error, skip
			failed++;
			release_context(connection);
			break;
		}
		
		l_keyvalue = malloc(sizeof(keyvalues_t));
		memset(l_keyvalue, 0, sizeof(keyvalues_t));
		if (!header) {
			header = l_keyvalue;
		} 

		if (tail) {
			tail->next = l_keyvalue;
		}

		tail = l_keyvalue;

		l_keyvalue->key = strdup(l_key->key);
		get_strvalue(reply, &l_keyvalue->value, l_keyvalue->value_len);
			
	}

	if (connection->cc)
		redisClusterReset(connection->cc);

	free_connection(connection, NULL);

	*keyvalues = header;

	return failed ? AGC_STATUS_GENERR : AGC_STATUS_SUCCESS;
	
}

static agc_status_t agc_cache_redis_delete_pipeline(keys_t *keys)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keys_t *l_key;
	int failed = 0;

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	for (l_key = keys; l_key; l_key = l_key->next)  {
		redisClusterAppendCommand(connection->cc, "DEL %s", l_key->key);
	}

	for (l_key = keys; l_key; l_key = l_key->next) {
		redisClusterGetReply(connection->cc, &reply);
		if (is_connection_break(reply)) { // connection error, skip
			failed++;
			release_context(connection);
			break;
		}

		if (decodeResult(reply) != AGC_STATUS_SUCCESS)
			failed++;
	}

	if (connection->cc)
		redisClusterReset(connection->cc);

	free_connection(connection, NULL);

	return failed ? AGC_STATUS_GENERR : AGC_STATUS_SUCCESS;
}

static agc_status_t agc_cache_redis_expire(const char *key, int expires)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;

	if (!key || (expires <=0 )) {
		return AGC_STATUS_GENERR;
	}

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	reply = redisClusterCommand(connection->cc, "EXPIRE %s %d", key, expires);
	free_connection(connection, reply);
	
	return decodeResult(reply);
	
}

static agc_status_t agc_cache_redis_incr(const char *key, int *value)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;

	if (!key) {
		return AGC_STATUS_GENERR;
	}
		
	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	reply = redisClusterCommand(connection->cc, "INCR %s", key);
	get_intvalue(reply, value);

	return decodeResult(reply);
}

static agc_status_t agc_cache_redis_decr(const char *key, int *value)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;

	if (!key) {
		return AGC_STATUS_GENERR;
	}
		
	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	reply = redisClusterCommand(connection->cc, "DECR %s", key);
	get_intvalue(reply, value);

	return decodeResult(reply);
}

static agc_status_t agc_cache_redis_hashset(const char *tablename, const char *key, const char *value)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	int failed = 0;

	if (!tablename || !key || !value) {
		return AGC_STATUS_GENERR;
	}

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	reply = redisClusterCommand(connection->cc, "HSET %s %s %s", tablename, key, value);

	free_connection(connection, reply);

	return decodeResult(reply);
	
}

static agc_status_t agc_cache_redis_hashget(const char *tablename, const char *key, char **value, int *len)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	int failed = 0;

	if (!tablename || !key || !value || !len) {
		return AGC_STATUS_GENERR;
	}

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	reply = redisClusterCommand(connection->cc, "HGET %s %s", tablename, key);

	free_connection(connection, reply);

	get_strvalue(reply, value, len);
	
	return decodeResult(reply);
}

static agc_status_t agc_cache_redis_hashmset(const char *tablename, keyvalues_t *keyvalues)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	int keyvalue_count = 0;
	keyvalues_t *iter;
	int argcount = 0;
	char **args = NULL;
	agc_size_t *argvlen = NULL;
	int i;

	if (!tablename || !keyvalues) {
		return AGC_STATUS_GENERR;
	}

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	for (iter = keyvalues; iter; iter = iter->next) {
		keyvalue_count++;
	}

	 // command + key + 2 * (number of field/value pairs)
	argcount = 2 + keyvalue_count*2;
	args = malloc(argcount * sizeof(char *));
	argvlen = malloc(argcount * sizeof(agc_size_t));

	args[0] = malloc(strlen("HMSET"));
    	argvlen[0] = strlen("HMSET");
    	memcpy(args[0], "HMSET", strlen("HMSET"));

	args[1] = malloc(strlen(tablename));
	argvlen[1] = strlen(tablename);
	memcpy(args[1], tablename, strlen(tablename));

	for (i=0, iter = keyvalues; i < keyvalue_count; i++, iter = iter->next) {
		int argsidx = 2 + (i*2);
		int key_len = strlen(iter->key);
		int value_len = strlen(iter->value);
		
		args[argsidx] = malloc(key_len * sizeof(char));
		args[argsidx+1] = malloc(value_len * sizeof(char));
		memcpy(args[argsidx], iter->key, key_len);
		memcpy(args[argsidx+1], iter->value, value_len);
		argvlen[argsidx] = key_len;
		argvlen[argsidx+1] = value_len;
		
	}

	reply = redisClusterCommandArgv(connection->cc, argcount, (const char**)args, argvlen);

	free_connection(connection, reply);

	//free parameters memory
	for (i=0; i < argcount; i++) {
		agc_safe_free(args[i]);
	}

	agc_safe_free(args);
	agc_safe_free(argvlen);
	
	return decodeResult(reply);
	
}

static agc_status_t agc_cache_redis_hashmget(const char *tablename, keys_t *keys,  keyvalues_t *keyvalues)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keys_t *iter;
	int keys_count;
	char **args = NULL;
	agc_size_t *argvlen = NULL;
	int argcount;
	keyvalues_t *header = NULL;
	keyvalues_t *l_keyvalue = NULL;
	keyvalues_t *tail= NULL;

	if (!tablename || !keys) {
		return AGC_STATUS_GENERR;
	}	

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}
	
	for (iter = keys; iter; iter = iter->next) {
		keys_count++;
	}

	 // command + key + (number of keys)
	argcount = 2 + keys_count;
	args = malloc(argcount * sizeof(char *));
	argvlen = malloc(argcount * sizeof(agc_size_t));

	args[0] = malloc(strlen("HMGET"));
    	argvlen[0] = strlen("HMGET");
    	memcpy(args[0], "HMGET", strlen("HMGET"));

	args[1] = malloc(strlen(tablename));
	argvlen[1] = strlen(tablename);
	memcpy(args[1], tablename, strlen(tablename));

	for (i=0, iter = keys; i < keys_count; i++, iter = iter->next) {
		int argsidx = 2 + i;
		int key_len = strlen(iter->key);

		args[argsidx] = malloc(key_len * sizeof(char));
		memcpy(args[argsidx], iter->key, key_len);
		argvlen[argsidx] = key_len;
	}

	reply = redisClusterCommandArgv(connection->cc, argcount, (const char**)args, argvlen);

	free_connection(connection, reply);

	//free parameters memory
	for (i=0; i < argcount; i++) {
		agc_safe_free(args[i]);
	}

	agc_safe_free(args);
	agc_safe_free(argvlen);


	if ( reply->type == REDIS_REPLY_ERROR || reply->type != REDIS_REPLY_ARRAY) {
		freeReplyObject(reply);
		return AGC_STATUS_GENERR;
	}

	for (i = 0; iter = keys; iter; iter = iter->next, i++) {
		char *value = NULL;
		int value_len = 0;
		l_keyvalue = malloc(sizeof(keyvalues_t));
		memset(l_keyvalue, 0, sizeof(keyvalues_t));

		if (!header) {
			header = l_keyvalue;
		} 

		if (tail) {
			tail->next = l_keyvalue;
		}

		tail = l_keyvalue;

		l_keyvalue->key = strdup(iter->key);

		value_len = strlen(reply->element[i]->str)
		value = malloc(value_len));
		assert(value);
		strncpy(value, reply->element[i]->str, value_len);

		l_keyvalue->value = value;
		l_keyvalue->value_len = value_len;	
	}

	*keyvalues = header;

	if (connection->cc)
		redisClusterReset(connection->cc);

	freeReplyObject(reply);
	return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_cache_redis_hashdel(const char *tablename, keys_t *keys)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keys_t *iter;
	int keys_count;
	char **args = NULL;
	agc_size_t *argvlen = NULL;
	int argcount;

	if (!tablename || !keys) {
		return AGC_STATUS_GENERR;
	}

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	for (iter = keys; iter; iter = iter->next) {
		keys_count++;
	}	

	 // command + tablename + (number of keys)
	argcount = 2 + keys_count;
	args = malloc(argcount * sizeof(char *));
	argvlen = malloc(argcount * sizeof(agc_size_t));


	args[0] = malloc(strlen("HDEL"));
    	argvlen[0] = strlen("HDEL");
    	memcpy(args[0], "HDEL", strlen("HDEL"));

	args[1] = malloc(strlen(tablename));
	argvlen[1] = strlen(tablename);
	memcpy(args[1], tablename, strlen(tablename));

	for (i=0, iter = keys; i < keys_count; i++, iter = iter->next) {
		int argsidx = 2 + i;
		int key_len = strlen(iter->key);

		args[argsidx] = malloc(key_len * sizeof(char));
		memcpy(args[argsidx], iter->key, key_len);
		argvlen[argsidx] = key_len;
	}	

	reply = redisClusterCommandArgv(connection->cc, argcount, (const char**)args, argvlen);

	free_connection(connection, reply);

	//free parameters memory
	for (i=0; i < argcount; i++) {
		agc_safe_free(args[i]);
	}

	agc_safe_free(args);
	agc_safe_free(argvlen);

	return decodeResult(reply);
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
		if (add_connection() != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create redis connection failed %s.\n", redis_addrs);
			return AGC_STATUS_GENERR;
		}
	}

	return AGC_STATUS_SUCCESS;
}

static void destroy_connection_pool()
{
	redisClusterContext *cc;

	while (1) {
		void *pop;
		
		if (agc_queue_pop(queue, &pop) != AGC_STATUS_SUCCESS) {
			break;
		}

		cc = (redisClusterContext *)pop;

		redisClusterFree(cc);
	}
}

static agc_status_t add_connection()
{
	redisClusterContext *cc;
	
	if ( !SYSTEM_RUNNING)
		return AGC_STATUS_SUCCESS;

	agc_redis_connection_t *connection = (agc_redis_connection_t *)agc_memory_alloc(module_pool, sizeof(agc_redis_connection_t));

	create_context(connection) ;

	agc_queue_push(REDIS_CONTEXT_QUEUE, connection);
	return AGC_STATUS_SUCCESS;
}

static agc_redis_connection_t *get_connection()
{
	void *pop;
	agc_redis_connection_t *connection;
	
	if (!SYSTEM_RUNNING)
		return NULL;

	if (agc_queue_pop(queue, &pop) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Can not get connection.\n");
		return NULL;
	}

	connection = (agc_redis_connection_t *)pop;
	if (!check_connection(connection)) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "The connection can not create.\n");
		agc_queue_push(connection);
		return NULL;
	}

	return connection;
}

static void free_connection(agc_redis_connection_t *connection, redisReply *reply)
{
	if (!connection)
		return;
	
	if (!SYSTEM_RUNNING) {
		release_context(connection);
		return;
	}

	if (is_connection_break(reply)) {
		release_context(connection);
	}

	agc_queue_push(REDIS_CONTEXT_QUEUE, connection);
}

static agc_status_t decodeResult(redisReply *reply)
{
	agc_status_t status = AGC_STATUS_GENERR;
	if (reply == NULL) {
		return status;
	}

	if (reply->type == REDIS_REPLY_ERROR) {
		status = AGC_STATUS_GENERR;
	}

	status = AGC_STATUS_SUCCESS;
	freeReplyObject(reply);
	return status;
}

static agc_bool_t check_connection(agc_redis_connection_t *connection)
{
	agc_time_t current_time;
		
	if (!connection)
		return AGC_FALSE;

	if (connection->connected)
		return AGC_TRUE;

	//try to reconnect redis
	if (connection->connect_time > (agc_time_now() / 1000) {
		redisClusterContext *cc;
		
		if (create_context(connection) == 	AGC_STATUS_SUCCESS)
			return AGC_TRUE;
	}

	return AGC_FALSE;
}

static agc_status_t create_context(agc_redis_connection_t *connection)
{
	redisClusterContext *cc;

	if (!connection)
		return AGC_STATUS_GENERR;

	if (connection->cc)
		redisClusterFree(connection->cc);

	cc = redisClusterContextInit();
	redisClusterSetOptionAddNodes(cc, redis_addrs);
	redisClusterConnect2(cc);
	if (cc != NULL && cc->err) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create redis context failed %s.\n", redis_addrs);
		connection->connected = 0;
		connection->cc = NULL;
		connection->connect_time = next_connect_time();
		redisClusterFree(cc);
		return AGC_STATUS_GENERR;
	} else {
		connection->connected = 1;
		connection->cc = cc;
		connection->connect_time = 0;
		return AGC_STATUS_SUCCESS:
	}

	return AGC_STATUS_SUCCESS;
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

static agc_bool_t is_connection_break(redisReply *reply)
{
	agc_bool_t connect_down = AGC_FALSE;
	
	if (reply) {
		switch (cluster_reply_error_type(reply)) {
			case CLUSTER_ERR_CLUSTERDOWN:
				connect_down = AGC_TRUE;
				break;

			default:
				break;
		}
	}	

	return connect_down;
}

static void get_strvalue(redisReply *reply, char **result, int *len)
{
	char *result_ptr = NULL;
	
	if (!reply || !result || !len)
		return;
	
	if (reply->type == REDIS_REPLY_STRING) {
		result_ptr = malloc(reply->len);
		strncpy(result_ptr, reply->str, reply->len);
		*result = result_ptr;
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

static agc_status_t redis_get_pipeline(const char *tablename, keys_t *keys, keyvalues_t **keyvalues)
{
	agc_redis_connection_t *connection  = NULL;
	redisReply *reply;
	keys_t *l_key;
	char tmp[512];
	keyvalues_t *header = NULL;
	keyvalues_t *l_keyvalue = NULL;
	keyvalues_t *tail= NULL;
	int failed = 0;

	if (!(connection = get_connection())) {
		return AGC_STATUS_GENERR;
	}

	for (l_key = keys; l_key; l_key = l_key->next)  {
		if (tablename) {
			agc_snprintfv(tmp, sizeof(tmp), "GET %s", l_key->key);
		} else {
			agc_snprintfv(tmp, sizeof(tmp), "GET %s", l_key->key);
		}
		redisClusterAppendCommand(connection->cc, tmp);
	}

	for (l_key = keys; l_key; l_key = l_key->next) {
		redisClusterGetReply(connection->cc, &reply);
		if (is_connection_break(reply)) { // connection error, skip
			failed++;
			release_context(connection);
			break;
		}
		
		l_keyvalue = malloc(sizeof(l_keyvalue));
		memset(l_keyvalue, 0, sizeof(l_keyvalue));
		if (!header) {
			header = l_keyvalue;
		} 

		if (tail) {
			tail->next = l_keyvalue;
		}

		tail = l_keyvalue;

		l_keyvalue->key = strdup(l_key->key);
		get_strvalue(reply, &l_keyvalue->value, l_keyvalue->value_len);
			
	}

	if (connection->cc)
		redisClusterReset(connection->cc);

	free_connection(connection, NULL);

	return failed ? AGC_STATUS_GENERR : AGC_STATUS_SUCCESS;
}

