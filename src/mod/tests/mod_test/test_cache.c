#include "mod_test.h"

#define AGC_TEST_HASHNAME "agc_hasher"

#define AGC_TEST_KEY "agc_test_key"
#define AGC_TEST_VALUE "test_value"
#define AGC_TEST_INTVALUE "10"

#define AGC_TEST_KEY2 "agc_test_key2"
#define AGC_TEST_VALUE2 "test_value2"

static void make_keyvalues(keyvalues_t **result);
static void make_keys(keys_t **result);

static void test_set(agc_stream_handle_t *stream, int argc, char **argv);
static void test_get(agc_stream_handle_t *stream, int argc, char **argv);
static void test_all(agc_stream_handle_t *stream, int argc, char **argv);

void test_cache_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	if (argc && argv[0]) {
		if (strcasecmp(argv[0], "set") == 0) {
			return test_set(stream, argc, argv);
		} else if (strcasecmp(argv[0], "get") == 0) {
			return test_get(stream, argc, argv);
		} else {
			stream->write_function(stream, "unkown command.\n");
			return;
		}
	} else {
		test_all(stream, argc, argv);
	}
}

static void test_set(agc_stream_handle_t *stream, int argc, char **argv)
{
	if (argc == 3) {
		if (agc_cache_set(argv[1], argv[2], strlen(argv[2]), 0) == AGC_STATUS_SUCCESS) {
			stream->write_function(stream, "set success.\n");
		} else {
			stream->write_function(stream, "set failed.\n");
		}

		return;
	}

	if (argc == 4) {
		if (agc_cache_set(argv[1], argv[2], strlen(argv[2]), atoi(argv[3])) == AGC_STATUS_SUCCESS) {
			stream->write_function(stream, "set success.\n");
		} else {
			stream->write_function(stream, "set failed.\n");
		}

		return;
	}

	stream->write_function(stream, "invalid set command.\n");		
}

static void test_get(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *result = NULL;
	int len = 0;
	
	if (argc == 2) {
		if (agc_cache_get(argv[1], &result, &len) ==AGC_STATUS_SUCCESS) {
			stream->write_function(stream, "get success %*.s.\n", len, result);
		} else {
			stream->write_function(stream, "get failed.\n");
		}
		
		agc_safe_free(result);
		return;
	}

	stream->write_function(stream, "invalid set command.\n");		
}

static void test_all(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *result = NULL;
	keyvalues_t *keyvalues =NULL;
	keys_t *keys = NULL;
	int len;
	int value;

	//test set
	if (agc_cache_set(AGC_TEST_KEY, AGC_TEST_VALUE, strlen(AGC_TEST_VALUE), 0) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "set [ok]\n");
	} else {
		stream->write_function(stream, "set [fail]\n");
		goto clean;
	}

	//test get
	if (agc_cache_get(AGC_TEST_KEY, &result, &len) ==AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "get [ok]\n");
		agc_safe_free(result);
	} else {
		stream->write_function(stream, "get [fail]\n");
		goto clean;
	}

	//test del
	if (agc_cache_delete(AGC_TEST_KEY) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "del [ok]\n");
	} else {
		stream->write_function(stream, "del [fail]\n");
		goto clean;
	}

	//test set pipeline
	make_keyvalues(&keyvalues);

	if (agc_cache_set_pipeline(keyvalues) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "setpipeline [ok]\n");
		agc_cache_free_keyvalues(&keyvalues);
	} else {
		stream->write_function(stream, "setpipeline [fail]\n");
		goto clean;
	}

	//test get pipeline
	make_keys(&keys);

	if (agc_cache_get_pipeline(keys, &keyvalues) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "getpipeline [ok]\n");
		agc_cache_free_keys(&keys);
		agc_cache_free_keyvalues(&keyvalues);
	} else {
		stream->write_function(stream, "getpipeline [fail]\n");
		goto clean;
	}

	//test del pipeline
	make_keys(&keys);
	if (agc_cache_delete_pipeline(keys) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "delpipeline [ok]\n");
		agc_cache_free_keys(&keys);
	} else {
		stream->write_function(stream, "delpipeline [fail]\n");
		goto clean;
	}

	//test expire
	if (agc_cache_set(AGC_TEST_KEY, AGC_TEST_INTVALUE, strlen(AGC_TEST_INTVALUE), 0) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "prepare expire [fail]\n");
		goto clean;
	}

	if (agc_cache_expire(AGC_TEST_KEY, 100) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "expire [ok]\n");
	} else {
		stream->write_function(stream, "expire [fail]\n");
		goto clean;
	}

	//test incr
	if (agc_cache_incr(AGC_TEST_KEY, &value) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "incr [ok]\n");
	} else {
		stream->write_function(stream, "incr [fail]\n");
		goto clean;
	}

	//test decr
	if (agc_cache_decr(AGC_TEST_KEY, &value) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "decr [ok]\n");
	} else {
		stream->write_function(stream, "decr [fail]\n");
		goto clean;
	}

	//test hashset
	if (agc_cache_hashset(AGC_TEST_HASHNAME, AGC_TEST_KEY, AGC_TEST_VALUE) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "hashset [ok]\n");
	} else {
		stream->write_function(stream, "hashset [fail]\n");
		goto clean;
	}

	//test hashget
	if (agc_cache_hashget(AGC_TEST_HASHNAME, AGC_TEST_KEY, &result, &len) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "hashget [ok]\n");
		agc_safe_free(result);
	} else {
		stream->write_function(stream, "hashget [fail]\n");
		goto clean;
	}

	//test hashmset
	make_keyvalues(&keyvalues);
	if (agc_cache_hashmset(AGC_TEST_HASHNAME, keyvalues) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "hashmset [ok]\n");
		agc_cache_free_keyvalues(&keyvalues);
	} else {
		stream->write_function(stream, "hashmset [fail]\n");
		goto clean;
	}

	//test hashmget
	make_keys(&keys);
	if (agc_cache_hashmget(AGC_TEST_HASHNAME, keys,  &keyvalues) == AGC_STATUS_SUCCESS) {
		agc_cache_free_keys(&keys);
		agc_cache_free_keyvalues(&keyvalues);
		stream->write_function(stream, "hashmget [ok]\n");
	} else {
		stream->write_function(stream, "hashmget [fail]\n");
		goto clean;
	}

	//test hashdel
	make_keys(&keys);
	if (agc_cache_hashdel(AGC_TEST_HASHNAME, keys) == AGC_STATUS_SUCCESS) {
		agc_cache_free_keys(&keys);
		stream->write_function(stream, "hashdel [ok]\n");
	} else {
		stream->write_function(stream, "hashdel [fail]\n");
		goto clean;
	}

clean:
	agc_cache_free_keys(&keys);
	agc_cache_free_keyvalues(&keyvalues);
	agc_safe_free(result);
	agc_cache_delete(AGC_TEST_KEY);
	agc_cache_delete(AGC_TEST_KEY2);
	agc_cache_delete(AGC_TEST_HASHNAME);
	
}

static void make_keyvalues(keyvalues_t **result)
{
	keyvalues_t *keyvalues =NULL;
	keyvalues_t *keyvalue_next = NULL;

	keyvalues = malloc(sizeof(*keyvalues));
	assert(keyvalues);
	keyvalues->key = strdup(AGC_TEST_KEY);
	keyvalues->value = strdup(AGC_TEST_VALUE);

	keyvalue_next = malloc(sizeof(*keyvalue_next));
	assert(keyvalue_next);
	keyvalues->next = keyvalue_next;
	keyvalue_next->key = strdup(AGC_TEST_KEY2);
	keyvalue_next->value = strdup(AGC_TEST_VALUE2);
	keyvalue_next->next = NULL;

	*result = keyvalues;
}

static void make_keys(keys_t **result)
{
	keys_t *keys = NULL;
	keys_t *key_next = NULL;

	keys = malloc(sizeof(*keys));
	assert(keys);
	keys->key = strdup(AGC_TEST_KEY);

	key_next = malloc(sizeof(*key_next));
	assert(key_next);
	keys->next= key_next;	
	key_next->next = NULL;
	key_next->key = strdup(AGC_TEST_KEY2);

	*result = keys;
	
}

