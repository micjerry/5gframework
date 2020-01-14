#include <agc.h>
#include "private/agc_core_pvt.h"

AGC_DECLARE(const char *) agc_cut_path(const char *in)
{
	const char *p, *ret = in;
	const char delims[] = "/\\";
	const char *i;

	if (in) {
		for (i = delims; *i; i++) {
			p = in;
			while ((p = strchr(p, *i)) != 0) {
				ret = ++p;
			}
		}
		return ret;
	} else {
		return NULL;
	}
}

AGC_DECLARE(agc_bool_t) agc_is_number(const char *str)
{
    const char *p;
	agc_bool_t r = AGC_TRUE;

	if (*str == '-' || *str == '+') {
		str++;
	}

	for (p = str; p && *p; p++) {
		if (!(*p == '.' || (*p > 47 && *p < 58))) {
			r = AGC_FALSE;
			break;
		}
	}

	return r;
}

