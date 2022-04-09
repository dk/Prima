#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

char *
duplicate_string( const char *s)
{
	int l;
	char *d;

	if (!s) return NULL;
	l = strlen( s) + 1;
	d = ( char*)malloc( l);
	if ( d) memcpy( d, s, l);
	return d;
}

void *
prima_mallocz( size_t sz)
{
	void *p = malloc( sz);
	if (p)
		bzero( p, sz);
	return p;
}

char *
prima_normalize_resource_string( char *name, Bool isClass)
{
	static Bool initialize = true;
	static char table[256];
	int i;
	unsigned char *s;

	if ( initialize) {
		for ( i = 0; i < 256; i++) {
			table[i] = isalnum(i) ? i : '_';
		}
		table[0] = 0;
		initialize = false;
	}

	s = (unsigned char*)name;
	while (*s) {
		*s = table[*s];
		s++;
	}
	name[0] = isClass ? toupper(name[0]) : tolower(name[0]);
	return name;
}

#ifndef HAVE_BZERO
void
bzero( void * data, size_t size)
{
	memset( data, 0, size);
}
#endif

#ifdef PRIMA_NEED_OWN_STRICMP
int
stricmp(const char *s1, const char *s2)
{
	/* Code was taken from FreeBSD 4.0 /usr/src/lib/libc/string/strcasecmp.c */
	const unsigned char *u1 = (const unsigned char *)s1;
	const unsigned char *u2 = (const unsigned char *)s2;
	while (tolower(*u1) == tolower(*u2++))
		if (*u1++ == '\0')
			return 0;
	return (tolower(*u1) - tolower(*--u2));
}
#endif

#ifdef PRIMA_NEED_OWN_STRNICMP
int
strnicmp(const char *s1, const char *s2, size_t count)
{
	const unsigned char *u1 = (const unsigned char *)s1;
	const unsigned char *u2 = (const unsigned char *)s2;
	if ( count == 0) return 0;
	while (tolower(*u1) == tolower(*u2++))
		if (--count == 0 || *u1++ == '\0')
			return 0;
	return (tolower(*u1) - tolower(*--u2));
}
#endif

#ifndef HAVE_STRCASESTR
/* Code was taken from FreeBSD 4.8 /usr/src/lib/libc/string/strcasestr.c */
char *
strcasestr( register const char * s,  register const char * find)
{
		register char c, sc;
		register size_t len;

		if ((c = *find++) != 0) {
			c = tolower((unsigned char)c);
			len = strlen(find);
			do {
				do {
					if ((sc = *s++) == 0)
						return (NULL);
				} while ((char)tolower((unsigned char)sc) != c);
			} while (strnicmp(s, find, len) != 0);
			s--;
		}
		return ((char *)s);
}
#endif


#ifndef HAVE_REALLOCF
/*
	This code was taken from FreeBSD 4.0 /usr/src/lib/libc/stdlib/reallocf.c
	Thanks, Poul Henning!  :-)
*/
void *
reallocf(void *ptr, size_t size)
{
	void *nptr;

	nptr = realloc(ptr, size);
	if (!nptr && ptr)
		free(ptr);
	return (nptr);
}
#endif

#if ! ( defined( HAVE_SNPRINTF) || defined( HAVE__SNPRINTF))
int
snprintf( char *buf, size_t len, const char *format, ...)
{
	int rc;
	va_list args;
	va_start( args, format);
	rc = vsnprintf( buf, len, format, args);
	va_end( args);
	return rc;
}
#endif

#ifndef HAVE_MEMMEM
/* copied from https://github.com/trevd/android_external_bootimage_utils/blob/master/windows/memmem.c */
void *
memmem(const void *l, size_t l_len, const void *s, size_t s_len)
{
	register char *cur, *last;
	const char *cl = (const char *)l;
	const char *cs = (const char *)s;

	/* we need something to compare */
	if (l_len == 0 || s_len == 0)
		return NULL;

	/* "s" must be smaller or equal to "l" */
	if (l_len < s_len)
		return NULL;

	/* special case where s_len == 1 */
	if (s_len == 1)
		return memchr(l, (int)*cs, l_len);

	/* the last position where its possible to find "s" in "l" */
	last = (char *)cl + l_len - s_len;

	for (cur = (char *)cl; cur <= last; cur++)
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
			return cur;

	return NULL;
}

#endif

#ifdef __cplusplus
}
#endif
