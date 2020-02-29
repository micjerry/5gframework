#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

void agc_loadable_module_init()
{
    FILE *file;
    yaml_parser_t parser;
    yaml_token_t token;
    int done = 0;
    int error = 0;
    int iskey = 0;
    int block = 0;
    int BLOCK_MODULES = 1;
    int new_module = 0;
    char module_name[256] = {0};
    char module_path[256] = {0};
    char *datap = NULL;
    const char *err;
    
    file = fopen("modules.yml", "rb");
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
                        if (strcmp(token.data.scalar.value, "modules") == 0)
                        {
                            block = BLOCK_MODULES;
                            datap = NULL;
                        } else if (strcmp(token.data.scalar.value, "name") == 0)
                        {
                            datap = module_name;
                        } else if (strcmp(token.data.scalar.value, "path") == 0)
                        {
                            datap = module_path;
                        } else {
                            block = 0;
                            datap = NULL;
                        }
                    } else {
                        if (datap)
                            strcpy(datap, token.data.scalar.value);
                    }
                }
                break;
            case YAML_BLOCK_ENTRY_TOKEN:
                if (block == BLOCK_MODULES) {
                    new_module = 1;
                }
                break;
            case YAML_BLOCK_END_TOKEN:
                if ((block == BLOCK_MODULES) && new_module) {
                    printf("module: path=%s, name=%s\n", module_path, module_name);
                    new_module = 0;
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
}

int
main(int argc, char *argv[])
{
    agc_loadable_module_init();
    return 0;
}


