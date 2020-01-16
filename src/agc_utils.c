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

AGC_DECLARE(char *) get_addr(char *buf, agc_size_t len, struct sockaddr *sa, socklen_t salen)
{
    agc_assert(buf);
    *buf = '\0';

    if (sa) {
        getnameinfo(sa, salen, buf, (socklen_t) len, NULL, 0, NI_NUMERICHOST);
    }
    return buf;
}

AGC_DECLARE(char *) get_addr6(char *buf, agc_size_t len, struct sockaddr_in6 *sa, socklen_t salen)
{
    agc_assert(buf);
    *buf = '\0';

    if (sa) {
        inet_ntop(AF_INET6, &(sa->sin6_addr), buf, len);
    }
    
    return buf;
}

