#ifndef AGC_UTILS_H
#define AGC_UTILS_H

#include <agc.h>

AGC_BEGIN_EXTERN_C

#include <math.h>

#define agc_yield(ms) agc_sleep(ms);

#define agc_str_nil(s) (s ? s : "")

/* https://code.google.com/p/stringencoders/wiki/PerformanceAscii 
   http://www.azillionmonkeys.com/qed/asmexample.html
*/
static inline uint32_t agc_toupper(uint32_t eax)
{
    uint32_t ebx = (0x7f7f7f7ful & eax) + 0x05050505ul;
    ebx = (0x7f7f7f7ful & ebx) + 0x1a1a1a1aul;
    ebx = ((ebx & ~eax) >> 2 ) & 0x20202020ul;
    return eax - ebx;
}

/* https://code.google.com/p/stringencoders/wiki/PerformanceAscii 
   http://www.azillionmonkeys.com/qed/asmexample.html
*/
static inline uint32_t agc_tolower(uint32_t eax)
{
	uint32_t ebx = (0x7f7f7f7ful & eax) + 0x25252525ul;
	ebx = (0x7f7f7f7ful & ebx) + 0x1a1a1a1aul;
	ebx = ((ebx & ~eax) >> 2)  & 0x20202020ul;
	return eax + ebx;
}

#ifdef AGC_64BIT
/* https://code.google.com/p/stringencoders/wiki/PerformanceAscii 
   http://www.azillionmonkeys.com/qed/asmexample.html
*/
static inline uint64_t agc_toupper64(uint64_t eax)
{
    uint64_t ebx = (0x7f7f7f7f7f7f7f7full & eax) + 0x0505050505050505ull;
    ebx = (0x7f7f7f7f7f7f7f7full & ebx) + 0x1a1a1a1a1a1a1a1aull;
    ebx = ((ebx & ~eax) >> 2 ) & 0x2020202020202020ull;
    return eax - ebx;
}

/* https://code.google.com/p/stringencoders/wiki/PerformanceAscii 
   http://www.azillionmonkeys.com/qed/asmexample.html
*/
static inline uint64_t agc_tolower64(uint64_t eax)
{
	uint64_t ebx = (0x7f7f7f7f7f7f7f7full & eax) + 0x2525252525252525ull;
	ebx = (0x7f7f7f7f7f7f7f7full & ebx) + 0x1a1a1a1a1a1a1a1aull;
	ebx = ((ebx & ~eax) >> 2)  & 0x2020202020202020ull;
	return eax + ebx;
}

static inline int agc_random(int max)
{
    return rand() % max;
}

static inline void agc_toupper_max(char *s)
{
	uint64_t *b,*p;
	char *c;
	size_t l;

	l = strlen(s);

	p = (uint64_t *) s;

	while (l > 8) {
		b = p;
		*b = (uint64_t) agc_toupper64(*b);
		b++;
		p++;
		l -= 8;
	}

	c = (char *)p;

	while(l > 0) {
		*c = (char) agc_toupper(*c);
		c++;
		l--;
	}

}

static inline void agc_tolower_max(char *s)
{
	uint64_t *b,*p;
	char *c;
	size_t l;

	l = strlen(s);

	p = (uint64_t *) s;

	while (l > 8) {
		b = p;
		*b = (uint64_t) agc_tolower64(*b);
		b++;
		p++;
		l -= 8;
	}

	c = (char *)p;

	while(l > 0) {
		*c = (char) agc_tolower(*c);
		c++;
		l--;
	}

}

#else
static inline void agc_toupper_max(char *s)
{
	uint32_t *b,*p;
	char *c;
	size_t l;

	l = strlen(s);

	p = (uint32_t *) s;

	while (l > 4) {
		b = p;
		*b = (uint32_t) agc_toupper(*b);
		b++;
		p++;
		l -= 4;
	}

	c = (char *)p;

	while(l > 0) {
		*c = (char) agc_toupper(*c);
		c++;
		l--;
	}
	
}

static inline void agc_tolower_max(char *s)
{
	uint32_t *b,*p;
	char *c;
	size_t l;

	l = strlen(s);

	p = (uint32_t *) s;

	while (l > 4) {
		b = p;
		*b = (uint32_t) agc_tolower(*b);
		b++;
		p++;
		l -= 4;
	}

	c = (char *)p;

	while(l > 0) {
		*c = (char) agc_tolower(*c);
		c++;
		l--;
	}
}
#endif

static inline int _zstr(const char *s)
{
	return !s || *s == '\0';
}

#define zstr(x) _zstr(x)

#define agc_strlen_zero(x) zstr(x)
#define agc_safe_free(it) if (it) {free(it);it=NULL;}
#define agc_set_string(_dst, _src) agc_copy_string(_dst, _src, sizeof(_dst))

#define AGC_STATUS_IS_BREAK(x) (x == AGC_STATUS_BREAK || x == 730035 || x == 35 || x == AGC_STATUS_INTR)


#define agc_malloc(ptr, len) (void)(assert(((ptr) = malloc((len)))),ptr)
#define agc_zmalloc(ptr, len) (void)(assert((ptr = calloc(1, (len)))),ptr)
#define agc_strdup(ptr, s) (void)(assert(((ptr) = strdup((s)))),ptr)

#define end_of_p(_s) (*_s == '\0' ? _s : _s + strlen(_s) - 1)

AGC_DECLARE(const char *) agc_cut_path(const char *in);
AGC_DECLARE(agc_bool_t) agc_is_number(const char *str);
AGC_DECLARE(unsigned int) agc_atoui(const char *nptr);
AGC_DECLARE(unsigned int) agc_separate_string(char *buf, char delim, char **array, unsigned int arraylen);

#define agc_arraylen(_a) (sizeof(_a) / sizeof(_a[0]))
#define agc_split(_data, _delim, _array) agc_separate_string(_data, _delim, _array, agc_arraylen(_array))

static inline int agc_true(const char *expr)
{
	return ((expr && ( !strcasecmp(expr, "yes") ||
					   !strcasecmp(expr, "on") ||
					   !strcasecmp(expr, "true") ||
					   !strcasecmp(expr, "t") ||
					   !strcasecmp(expr, "enabled") ||
					   !strcasecmp(expr, "active") ||
					   !strcasecmp(expr, "allow") ||
					   (agc_is_number(expr) && atoi(expr)))) ? AGC_TRUE : AGC_FALSE);
}


AGC_DECLARE(char *) get_addr(char *buf, agc_size_t len, struct sockaddr *sa, socklen_t salen);
AGC_DECLARE(char *) get_addr6(char *buf, agc_size_t len, struct sockaddr_in6 *sa, socklen_t salen);

AGC_END_EXTERN_C

#endif
