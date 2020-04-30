#include <agc.h>

AGC_DECLARE(void)  agc_cache_free_keyvalues(keyvalues_t *keyvalues)
{
	keyvalues_t *iter = keyvalues;
	keyvalues_t *current = NULL;

	while (iter) {
		current = iter;
		iter = iter->next;

		agc_safe_free(current->key);
		agc_safe_free(current->value);
		agc_safe_free(current);
	}
}

AGC_DECLARE(void) agc_cache_free_keys(keys_t *keys)
{
	keys_t *iter = keys;
	keys_t *current = NULL;	

	while (iter) {
		current = iter;
		iter = iter->next;

		agc_safe_free(current->key);
		agc_safe_free(current);
	}	
}

