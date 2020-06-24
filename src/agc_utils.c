#include <agc.h>
#include "private/agc_core_pvt.h"

#define ESCAPE_META '\\'

static char unescape_char(char escaped);
static unsigned int separate_string_blank_delim(char *buf, char **array, unsigned int arraylen);
static unsigned int separate_string_char_delim(char *buf, char delim, char **array, unsigned int arraylen);

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

AGC_DECLARE(unsigned int) agc_atoui(const char *nptr)
{
	long tmp = atol(nptr);
	if (tmp < 0) 
		return 0;
    
	return (unsigned long) tmp;
}

static char *cleanup_separated_string(char *str, char delim)
{
	char *ptr;
	char *dest;
	char *start;
	char *end = NULL;
	int inside_quotes = 0;

	/* Skip initial whitespace */
	for (ptr = str; *ptr == ' '; ++ptr) {
	}

	for (start = dest = ptr; *ptr; ++ptr) {
		char e;
		int esc = 0;

		if (*ptr == ESCAPE_META) {
			e = *(ptr + 1);
			if (e == '\'' || e == '"' || (delim && e == delim) || e == ESCAPE_META || (e = unescape_char(*(ptr + 1))) != *(ptr + 1)) {
				++ptr;
				*dest++ = e;
				end = dest;
				esc++;
			}
		}
		
		if (!esc) {
			if (*ptr == '\'' && (inside_quotes || ((ptr+1) && strchr(ptr+1, '\'')))) {
				if ((inside_quotes = (1 - inside_quotes))) {
					end = dest;
				}
			} else {
				*dest++ = *ptr;
				if (*ptr != ' ' || inside_quotes) {
					end = dest;
				}
			}
		}
	}
	
	if (end) {
		*end = '\0';
	}

    return start;
}

static char unescape_char(char escaped)
{
	char unescaped;

	switch (escaped) {
	case 'n':
		unescaped = '\n';
		break;
	case 'r':
		unescaped = '\r';
		break;
	case 't':
		unescaped = '\t';
		break;
	case 's':
		unescaped = ' ';
		break;
	default:
		unescaped = escaped;
	}
	return unescaped;
}

static unsigned int separate_string_blank_delim(char *buf, char **array, unsigned int arraylen)
{
	enum tokenizer_state {
		START,
		SKIP_INITIAL_SPACE,
		FIND_DELIM,
		SKIP_ENDING_SPACE
	} state = START;
    
	unsigned int count = 0;
	char *ptr = buf;
	int inside_quotes = 0;
	unsigned int i;

	while (*ptr && count < arraylen) {
		switch (state) {
			case START:
				array[count++] = ptr;
				state = SKIP_INITIAL_SPACE;
				break;

			case SKIP_INITIAL_SPACE:
				if (*ptr == ' ') {
					++ptr;
				} else {
					state = FIND_DELIM;
				}
				break;

			case FIND_DELIM:
				if (*ptr == ESCAPE_META) {
					++ptr;
				} else if (*ptr == '\'') {
					inside_quotes = (1 - inside_quotes);
				} else if (*ptr == ' ' && !inside_quotes) {
					*ptr = '\0';
					state = SKIP_ENDING_SPACE;
				}
				++ptr;
				break;

			case SKIP_ENDING_SPACE:
				if (*ptr == ' ') {
					++ptr;
				} else {
					state = START;
				}
				break;
		}
	}
	/* strip quotes, escaped chars and leading / trailing spaces */

	for (i = 0; i < count; ++i) {
		array[i] = cleanup_separated_string(array[i], 0);
	}

	return count;
}

static unsigned int separate_string_char_delim(char *buf, char delim, char **array, unsigned int arraylen)
{
	enum tokenizer_state {
		START,
		FIND_DELIM
	} state = START;

	unsigned int count = 0;
	char *ptr = buf;
	int inside_quotes = 0;
	unsigned int i;

	while (*ptr && count < arraylen) {
		switch (state) {
			case START:
				array[count++] = ptr;
				state = FIND_DELIM;
				break;

			case FIND_DELIM:
				/* escaped characters are copied verbatim to the destination string */
				if (*ptr == ESCAPE_META) {
					++ptr;
				} else if (*ptr == '\'' && (inside_quotes || ((ptr+1) && strchr(ptr+1, '\'')))) {
					inside_quotes = (1 - inside_quotes);
				} else if (*ptr == delim && !inside_quotes) {
					*ptr = '\0';
					state = START;
				}
				++ptr;
				break;
		}
	}
	/* strip quotes, escaped chars and leading / trailing spaces */

	for (i = 0; i < count; ++i) {
		array[i] = cleanup_separated_string(array[i], delim);
	}

	return count;
}

AGC_DECLARE(unsigned int) agc_separate_string(char *buf, char delim, char **array, unsigned int arraylen)
{
	if (!buf || !array || !arraylen) {
		return 0;
	}

	if (*buf == '^' && *(buf+1) == '^') {
		char *p = buf + 2;
	    
		if (p && *p && *(p+1)) {
			buf = p;
			delim = *buf++;
		}
	}

	memset(array, 0, arraylen * sizeof(*array));

	return (delim == ' ' ? separate_string_blank_delim(buf, array, arraylen) : separate_string_char_delim(buf, delim, array, arraylen));
}

