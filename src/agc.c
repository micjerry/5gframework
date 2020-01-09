#include <agc.h>

int main(int argc, char *argv[])
{
    char *local_argv[1024] = { 0 };
    int local_argc = argc;
    int x;
    
    for (x = 0; x < argc; x++) {
		local_argv[x] = argv[x];
	}
    
    for (x = 1; x < local_argc; x++) {
        if (agc_strlen_zero(local_argv[x]))
            continue;
        
        if (!strcmp(local_argv[x], "-base")) {
            x++;
            if (agc_strlen_zero(local_argv[x])) {
                fprintf(stderr, "When using -base you must specify a base directory\n");
                return 255;
            }
            
            AGC_GLOBAL_dirs.base_dir = (char *) malloc(strlen(local_argv[x]) + 1);
            if (!AGC_GLOBAL_dirs.base_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
            
            strcpy(AGC_GLOBAL_dirs.base_dir, local_argv[x]);
        }
        
        else if  (!strcmp(local_argv[x], "-log")) {
            x++;
            if (agc_strlen_zero(local_argv[x])) {
                fprintf(stderr, "When using -log you must specify a log directory\n");
                return 255;
            }
            
            AGC_GLOBAL_dirs.log_dir = (char *) malloc(strlen(local_argv[x]) + 1);
            if (!AGC_GLOBAL_dirs.log_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
            
            strcpy(AGC_GLOBAL_dirs.log_dir, local_argv[x]);
        }
        
        else if if  (!strcmp(local_argv[x], "-run")){
            x++;
            if (agc_strlen_zero(local_argv[x])) {
                fprintf(stderr, "When using -run you must specify a run directory\n");
                return 255;
            }
            
            AGC_GLOBAL_dirs.run_dir = (char *) malloc(strlen(local_argv[x]) + 1);
            if (!AGC_GLOBAL_dirs.run_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
            
            strcpy(AGC_GLOBAL_dirs.run_dir, local_argv[x]);
        }
        
    }
    
    agc_core_set_globals();
}