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

uint32_t*
prima_string2uint32( register const char * src, unsigned int dlen, Bool is_utf8, unsigned int * size)
{
	uint32_t *ret;

	*size = is_utf8 ? prima_utf8_length(src, dlen) : dlen;
	if (!(ret = ( uint32_t*) malloc(sizeof(uint32_t) * (*size)))) {
		warn("Not enough memory: %ld bytes", (unsigned long int)(sizeof(uint32_t) * (*size)));
		return NULL;
	}

	if (is_utf8) {
		uint32_t *dst = ret;
		while ( dlen > 0 && dst - ret < *size) {
			UV uv;
			unsigned int charlen;
			uv = prima_utf8_uvchr(src, dlen, &charlen);
			if ( uv > 0x10FFFF ) uv = 0x10FFFF;
			*(dst++) = uv;
			if ( charlen == 0 ) break;
			src  += charlen;
			dlen -= charlen;
		}
		*size = dst - ret;
	} else {
		register int i = *size;
		register uint32_t *dst = ret;
		while (i-- > 0) *(dst++) = *((unsigned char*) src++);
	}

	return ret;
}

uint32_t*
prima_sv2uint32( SV * text, unsigned int * size, unsigned int * flags)
{
	STRLEN dlen;
	const char * src;

	src = (const char*) SvPV(text, dlen);
	if (prima_is_utf8_sv(text))
		*flags |= toUTF8;
	return prima_string2uint32( src, dlen, *flags & toUTF8, size);
}

#ifdef __cplusplus
}
#endif
