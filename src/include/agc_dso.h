#ifndef AGC_DSO_H
#define AGC_DSO_H

typedef int (*agc_dso_func_t) (void);
typedef void *agc_dso_lib_t;
typedef void *agc_dso_data_t;

AGC_DECLARE(void) agc_dso_destroy(agc_dso_lib_t *lib);
AGC_DECLARE(agc_dso_lib_t) agc_dso_open(const char *path, int global, char **err);
AGC_DECLARE(agc_dso_func_t) agc_dso_func_sym(agc_dso_lib_t lib, const char *sym, char **err);
AGC_DECLARE(void *) agc_dso_data_sym(agc_dso_lib_t lib, const char *sym, char **err);

#endif