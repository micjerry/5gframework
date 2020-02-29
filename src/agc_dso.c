#include <agc.h>
#include "agc_dso.h"
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>

void agc_dso_destroy(agc_dso_lib_t *lib)
{
	if (lib && *lib) {
		dlclose(*lib);
		*lib = NULL;
	}
}

agc_dso_lib_t agc_dso_open(const char *path, int global, char **err)
{
	void *lib;

	if (global) {
		lib = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	} else {
		lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	}

	if (lib == NULL) {
		const char *dlerr = dlerror();
		if (dlerr) {
			*err = strdup(dlerr);
		} else {
			*err = strdup("Unknown error");
		}
	}
	return lib;
}

agc_dso_func_t agc_dso_func_sym(agc_dso_lib_t lib, const char *sym, char **err)
{
	void *func = dlsym(lib, sym);
	if (!func) {
		*err = strdup(dlerror());
	}
	return (agc_dso_func_t) (intptr_t) func;
}

void *agc_dso_data_sym(agc_dso_lib_t lib, const char *sym, char **err)
{
	void *addr = dlsym(lib, sym);
	if (!addr) {
		char *err_str = NULL;
		dlerror();

		if (!(addr = dlsym(lib, sym))) {
			err_str = (char *)dlerror();
		}

		if (err_str) {
			*err = strdup(err_str);
		}
	}
	return addr;
}

