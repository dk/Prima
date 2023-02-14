/*******************************************************************************/
/*                                                                             */
/*  XIM support                                                                */
/*                                                                             */
/*  Code courtesy Yin Maofan                                                   */
/*  https://tedyin.com/posts/a-brief-intro-to-linux-input-method-framework/    */
/*                                                                             */
/*******************************************************************************/

#include "unix/guts.h"
#include "Application.h"

void
prima_xim_init(void)
{
	if (
		( guts.xic_buffer = malloc( guts.xic_bufsize = 256 )) != NULL
	) {
		char *loc = setlocale(LC_CTYPE, NULL);
		setlocale(LC_CTYPE, "");
		XSetLocaleModifiers("");
		guts.xim = XOpenIM(DISP, NULL, NULL, NULL);
		if ( guts.xim) {
			guts.xic = XCreateIC(guts.xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, NULL);
			guts.use_xim = true;
		}
		setlocale(LC_CTYPE, loc);
	}
}

void
prima_xim_done(void)
{
	XDestroyIC(guts.xic);
	XCloseIM(guts.xim);
	free(guts.xic_buffer);
	guts.use_xim = false;
}

void
prima_xim_update_cursor( Handle self)
{
	DEFXX;
	XPoint spot;
	XVaNestedList preedit_attr;
	char *error;

	spot.x = XX->cursor_pos.x;
	spot.y = XX->size. y - XX->cursor_pos. y;
	preedit_attr = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
	error = XSetICValues(guts.xic, XNPreeditAttributes, preedit_attr, NULL);
	if ( error != NULL )
		Mdebug("XSetICValues(XNSpotLocation) error: %s\n", error);
	XFree(preedit_attr);
}

void
prima_xim_focus_in(Handle self)
{
	char *error;
	Handle toplevel = prima_find_root_parent(self);

    	error = XSetICValues(guts.xic, XNClientWindow, PWidget(toplevel)->handle, NULL);
	if ( error != NULL )
		Mdebug("XSetICValues(XNClientWindow) error: %s\n", error);

    	error = XSetICValues(guts.xic, XNFocusWindow, PWidget(self)->handle, NULL);
	if ( error != NULL )
		Mdebug("XSetICValues(XNFocusWindow) error: %s\n", error);

    	XSetICFocus(guts.xic);
#ifdef X_HAVE_UTF8_STRING
	XFree(Xutf8ResetIC(guts.xic));
#endif
	XCHECKPOINT;
}

void
prima_xim_focus_out(void)
{
	XUnsetICFocus(guts.xic);
	XSetICValues( guts.xic, XNClientWindow, 0, NULL);
	XSetICValues( guts.xic, XNFocusWindow, 0, NULL);
}

Bool
prima_xim_handle_key_press( Handle self, XKeyEvent *xev, Event *e, KeySym *sym)
{
#ifdef X_HAVE_UTF8_STRING
	Status status;
	int c, n;
	Bool ok = true;
	semistatic_t local_buf_ptr;
	char local_buf[256], *buf;

	while ( 1 ) {
		char *b;

		c = Xutf8LookupString(guts.xic, xev, guts.xic_buffer, guts.xic_bufsize - 1, sym, &status);
		Mdebug("Xutf8LookupString: nc=%d status=%d\n", c, status);
		switch ( status ) {
		case XLookupNone:
		case XLookupKeySym:
			return false;
		}

		if (status != XBufferOverflow) 
			break;

		if ( ( b = realloc( guts.xic_buffer, guts.xic_bufsize * 2)) == NULL ) {
			Mdebug("cannot realloc %d bytes for XIC buffer", guts.xic_bufsize * 2);
			return false;
		}
		guts.xic_bufsize *= 2;
		guts.xic_buffer = b;
	}

	if ( c == 0 ) return true;

	semistatic_init( &local_buf_ptr, &local_buf, sizeof(char), sizeof(local_buf));
	if ( !semistatic_expand( &local_buf_ptr, c + 1 ))
		return false;
	buf = (char*) local_buf_ptr.heap;
	memcpy( buf, guts.xic_buffer, c);
	guts.xic_buffer[0] = 0;
	buf[c] = 0;
	Mdebug("Xutf8LookupString: string=%s\n", buf);

	/* send events */
	protect_object(self);
	n = 0;
	while ( *buf ) {
		UV uv;
		STRLEN charlen;
		Event ev;

		uv = prima_utf8_uvchr(buf, c, &charlen);
		buf  += charlen;
		c    -= charlen;
		if ( charlen == 0 ) break;
		n++;
		if ( uv > 0x10FFFF ) continue;

		ev.cmd        = cmKeyDown;
		ev.key.code   = uv;
		ev.key.mod    = kmUnicode;
		ev.key.repeat = 1;
		ev.key.key    = kbNoKey;
		if ( n == 1 && status == XLookupBoth ) {
			U32 key = prima_keysym_to_keycode(*sym, xev, 0) & kbCodeMask;
			if ( key != 0 ) ev.key.key = key;
		}
		Mdebug("Xutf8LookupString: char(%d)=%x key=%x\n", n, uv, ev.key.key);
		CComponent(self)-> message(self,&ev);
		if (PWidget(self)-> stage != csNormal)
			break;
	}
	unprotect_object(self);

	semistatic_done( &local_buf_ptr);

	XCHECKPOINT;
	return ok;
#else
	return false;
#endif
}

