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
        int count = 0;
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
					printf("YAML_STREAM_START_TOKEN: encoding=%d\n", token.data.stream_start.encoding);
				break;
				case YAML_STREAM_END_TOKEN:
					printf("YAML_STREAM_END_TOKEN\n");
				break;
				case YAML_VERSION_DIRECTIVE_TOKEN:
					printf("YAML_VERSION_DIRECTIVE_TOKEN: major=%d, minor=%d\n", token.data.version_directive.major, token.data.version_directive.minor);
				break;
				case YAML_TAG_DIRECTIVE_TOKEN:
					printf("YAML_TAG_DIRECTIVE_TOKEN: handle=%s, prefix=%s\n", token.data.tag_directive.handle, token.data.tag_directive.prefix);
				break;
				case YAML_DOCUMENT_START_TOKEN:
					printf("YAML_DOCUMENT_START_TOKEN\n");
				break;
				case YAML_DOCUMENT_END_TOKEN:
					printf("YAML_DOCUMENT_END_TOKEN\n");
				break;
				case YAML_BLOCK_SEQUENCE_START_TOKEN:
					printf("YAML_BLOCK_SEQUENCE_START_TOKEN\n");
				break;
				case YAML_BLOCK_MAPPING_START_TOKEN:
                    if (strlen(last_key) > 0) {
                        headers[pos] = malloc(strlen(last_key) + 1);
                        strcpy(headers[pos], last_key);
                        pos++;
                    }
					printf("YAML_BLOCK_MAPPING_START_TOKEN\n");
				break;
				case YAML_BLOCK_END_TOKEN:
                    pos--;
                    free(headers[pos]);
                    headers[pos] = NULL;
					printf("YAML_BLOCK_END_TOKEN\n");
				break;
				case YAML_FLOW_SEQUENCE_START_TOKEN:
					printf("YAML_FLOW_SEQUENCE_START_TOKEN\n");
				break;
				case YAML_FLOW_SEQUENCE_END_TOKEN:
					printf("YAML_FLOW_SEQUENCE_END_TOKEN\n");
				break;
				case YAML_FLOW_MAPPING_START_TOKEN:
					printf("YAML_FLOW_MAPPING_START_TOKEN\n");
				break;
				case YAML_FLOW_MAPPING_END_TOKEN:
					printf("YAML_FLOW_MAPPING_END_TOKEN\n");
				break;
				case YAML_BLOCK_ENTRY_TOKEN:
					printf("YAML_BLOCK_ENTRY_TOKEN\n");
				break;
				case YAML_FLOW_ENTRY_TOKEN:
					printf("YAML_FLOW_ENTRY_TOKEN\n");
				break;
				case YAML_KEY_TOKEN:
                    iskey = 1;
					printf("YAML_KEY_TOKEN\n");
				break;
				case YAML_VALUE_TOKEN:
                    iskey = 0;
					printf("YAML_VALUE_TOKEN\n");
				break;
				case YAML_ALIAS_TOKEN:
					printf("YAML_ALIAS_TOKEN: value=%s\n", token.data.alias.value);
				break;
				case YAML_ANCHOR_TOKEN:
					printf("YAML_ANCHOR_TOKEN: value=%s\n", token.data.anchor.value);
				break;
				case YAML_TAG_TOKEN:
					printf("YAML_TAG_TOKEN: handle=%s, suffix=%s\n", token.data.tag.handle, token.data.tag.suffix);
				break;
				case YAML_SCALAR_TOKEN:
				{
					printf("YAML_SCALAR_TOKEN: value=%s, length=%lu, style=%d\n", token.data.scalar.value, token.data.scalar.length, token.data.scalar.style);
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
 
            count ++;
        }
 
        yaml_parser_delete(&parser);
 
        assert(!fclose(file));
 
		printf("[%d] Scanning '%s': ", number, argv[number]);
        printf("%s (%d tokens)\n", (error ? "FAILURE" : "SUCCESS"), count);
    }
 
    return 0;
}
