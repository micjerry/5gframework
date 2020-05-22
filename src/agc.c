#include <agc.h>
#include "private/agc_core_pvt.h"

int main(int argc, char *argv[])
{
	char *local_argv[1024] = { 0 };
	const char *err = NULL;
	agc_bool_t nc = AGC_FALSE;
	int local_argc = argc;
	int x;
    
	for (x = 0; x < argc; x++) {
		local_argv[x] = argv[x];
	}
    
	if (local_argv[0] && strstr(local_argv[0], "agcd")) {
		nc = AGC_TRUE;
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
        
		else if (!strcmp(local_argv[x], "-log")) {
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
        
		else if (!strcmp(local_argv[x], "-run")){
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
        
		else if (!strcmp(local_argv[x], "-nc")) {
			nc = AGC_TRUE;
		}
        
		else if (!strcmp(local_argv[x], "-ncwait")) {
			nc = AGC_TRUE;
		}

		else if (!strcmp(local_argv[x], "-c")) {
			nc = AGC_FALSE;
		}
        
	}
    
	agc_core_set_globals();
	if (agc_core_init(nc ? AGC_FALSE : AGC_TRUE, &err) != AGC_STATUS_SUCCESS) {
		fprintf(stderr, "Cannot Initialize [%s]\n", err);
		return 1;
	}
    
	if (agc_core_modload(&err) != AGC_STATUS_SUCCESS) {
		fprintf(stderr, "Cannot load modules [%s]\n", err);
		return 1;
	}

	agc_core_runtime_loop();

	agc_core_destroy();
    
	return 0;
}
