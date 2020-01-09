#include <yaml.h>
 
#include <stdlib.h>
#include <stdio.h>
 
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>


int
main(int argc, char *argv[])
{
    int number;
    char *headers[16];
 
    if (argc < 2) {
        printf("Usage: %s file1.yaml ...\n", argv[0]);
        return 0;
    }
 
    for (number = 1; number < argc; number ++)
    {
        FILE *file;
        yaml_parser_t parser;
        yaml_token_t token;
        int done = 0;
        int error = 0;
        int pos = 0;
        int iskey = 0;
        int i = 0;
        char last_key[128] = {0};
        char full_key[512] = {0};
 
        fflush(stdout);
 
        file = fopen(argv[number], "rb");
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
				case YAML_STREAM_START_TOKEN:
				case YAML_STREAM_END_TOKEN:
				case YAML_VERSION_DIRECTIVE_TOKEN:
				case YAML_TAG_DIRECTIVE_TOKEN:
				case YAML_DOCUMENT_START_TOKEN:
				case YAML_DOCUMENT_END_TOKEN:
				case YAML_BLOCK_SEQUENCE_START_TOKEN:
				    break;
				case YAML_BLOCK_MAPPING_START_TOKEN:
                    if (strlen(last_key) > 0) {
                        headers[pos] = malloc(strlen(last_key) + 1);
                        strcpy(headers[pos], last_key);
                        pos++;
                    }
				    break;
				case YAML_BLOCK_END_TOKEN:
                    if (pos > 0) {
                        pos--;
                        free(headers[pos]);
                        headers[pos] = NULL;
                    }
				    break;
				case YAML_FLOW_SEQUENCE_START_TOKEN:
				case YAML_FLOW_SEQUENCE_END_TOKEN:
				case YAML_FLOW_MAPPING_START_TOKEN:
				case YAML_FLOW_MAPPING_END_TOKEN:
				case YAML_BLOCK_ENTRY_TOKEN:
				case YAML_FLOW_ENTRY_TOKEN:
				    break;
				case YAML_KEY_TOKEN:
                    iskey = 1;
				    break;
				case YAML_VALUE_TOKEN:
                    iskey = 0;
				    break;
				case YAML_ALIAS_TOKEN: 
				case YAML_ANCHOR_TOKEN:
				case YAML_TAG_TOKEN:
				    break;
				case YAML_SCALAR_TOKEN:
				{
                    if (iskey) {
                        strcpy(last_key, token.data.scalar.value);
                    }
                    else {
                        memset(full_key, 0, sizeof(full_key));
                        for (i = 0; i < pos; i++) {
                            strcat(full_key, headers[i]);
                            strcat(full_key, ".");
                        }
                        
                        strcat(full_key, last_key);
                        printf("YAML_SCALAR_TOKEN: key=%s, value=%s, style=%d\n", full_key, token.data.scalar.value);
                    }
				}
				break;
			}
            done = (token.type == YAML_STREAM_END_TOKEN);
 
            yaml_token_delete(&token);
 
        }
 
        yaml_parser_delete(&parser);
 
        assert(!fclose(file));
    }
 
    return 0;
}
