#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

int
prima_utf8_length( const char * utf8, int maxlen)
{
	int ulen = 0;
	if ( maxlen < 0 ) maxlen = INT16_MAX;
	while ( maxlen > 0 && *utf8 ) {
		const char *u = (char*) utf8_hop(( U8*) utf8, 1);
		ulen++;
		maxlen -= u - utf8;
		utf8 = u;
	}
	return ulen;
}

Bool
prima_is_utf8_sv( SV * sv)
{
	/* from Encode.xs */
	if (SvGMAGICAL(sv)) {
		SV * sv2 = newSVsv(sv); /* GMAGIG will be done */
		Bool ret = SvUTF8(sv2) ? 1 : 0;
		SvREFCNT_dec(sv2); /* it was a temp copy */
		return ret;
	} else {
		return SvUTF8(sv) ? 1 : 0;
	}
}

SV*
prima_svpv_utf8( const char *text, int is_utf8)
{
	SV *sv = newSVpv(text, 0);
	if ( is_utf8 ) SvUTF8_on(sv);
	return sv;
}

UV
prima_utf8_uvchr_end(const char * text, const char * end, unsigned int *charlen)
{
	STRLEN l;
	UV uv =
#if PERL_PATCHLEVEL >= 16
		utf8_to_uvchr_buf(( U8*)(text), (U8*)(end), &l);
#else
		utf8_to_uvchr(( U8*)(text), &l);
#endif
	*charlen = ((int)l < 0) ? 0 : l;
	return uv;
}

#ifdef __cplusplus
}
#endif
