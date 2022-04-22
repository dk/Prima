#include "unix/guts.h"
#include "Application.h"
#include "Clipboard.h"
#include "Icon.h"

#define WIN PComponent(prima_guts.application)-> handle

#define CF_NAME(x)   (guts. clipboard_formats[(x)*3])
#define CF_TYPE(x)   (guts. clipboard_formats[(x)*3+1])
#define CF_FORMAT(x) (guts. clipboard_formats[(x)*3+2])
#define CF_ASSIGN(i,a,b,c) CF_NAME(i)=(a);CF_TYPE(i)=(b);CF_FORMAT(i)=((Atom)c)

Bool
prima_init_clipboard_subsystem(char * error_buf)
{
	guts. clipboards = hash_create();

	if ( !(guts. clipboard_formats = malloc( cfCOUNT * 3 * sizeof(Atom)))) {
		sprintf( error_buf, "No memory");
		return false;
	}
	guts. clipboard_formats_count = cfCOUNT;
#if (cfText != 0) || (cfBitmap != 1) || (cfUTF8 != 2)
#error broken clipboard type formats
#endif

	CF_ASSIGN(cfText, XA_STRING, XA_STRING, 8);
	CF_ASSIGN(cfUTF8, UTF8_STRING, UTF8_STRING, 8);
	CF_ASSIGN(cfBitmap, XA_PIXMAP, XA_PIXMAP, CF_32);
	CF_ASSIGN(cfTargets, CF_TARGETS, XA_ATOM, CF_32);

	/* XXX - bitmaps and indexed pixmaps may have the associated colormap or pixel values
	CF_ASSIGN(cfPalette, XA_COLORMAP, XA_ATOM, CF_32);
	CF_ASSIGN(cfForeground, CF_FOREGROUND, CF_PIXEL, CF_32);
	CF_ASSIGN(cfBackground, CF_BACKGROUND, CF_PIXEL, CF_32);
	*/

	guts. clipboard_event_timeout = 2000;
	return true;
}

PList
apc_get_standard_clipboards( void)
{
	PList l = plist_create( 4, 1);
	if (!l) return NULL;
	list_add( l, (Handle)duplicate_string( "Primary"));
	list_add( l, (Handle)duplicate_string( "Secondary"));
	list_add( l, (Handle)duplicate_string( "Clipboard"));
	list_add( l, (Handle)duplicate_string( "XdndSelection"));
	return l;
}

Bool
apc_clipboard_create( Handle self)
{
	PClipboard c = (PClipboard)self;
	int i;
	DEFCC;

	if ( strcmp(c->name, "XdndSelection") != 0 ) {
		char *name, *x;
		name = x = duplicate_string( c-> name);
		while (*x) {
			*x = toupper(*x);
			x++;
		}
		XX-> selection = XInternAtom( DISP, name, false);
		free( name);
	} else {
		XX-> selection = XdndSelection;
	}

	if ( hash_fetch( guts.clipboards, &XX->selection, sizeof(XX->selection))) {
		warn("This clipboard is already present");
		return false;
	}

	if ( !( XX-> internal = malloc( sizeof( ClipboardDataItem) * cfCOUNT))) {
		warn("Not enough memory");
		return false;
	}
	if ( !( XX-> external = malloc( sizeof( ClipboardDataItem) * cfCOUNT))) {
		free( XX-> internal);
		warn("Not enough memory");
		return false;
	}
	bzero( XX-> internal, sizeof( ClipboardDataItem) * cfCOUNT);
	bzero( XX-> external, sizeof( ClipboardDataItem) * cfCOUNT);

	XX->internal[cfTargets].name = CF_NAME(cfTargets);

	for ( i = 0; i < cfCOUNT; i++) 
		XX->internal[i].immediate = XX->external[i].immediate = true;

	hash_store( guts.clipboards, &XX->selection, sizeof(XX->selection), (void*)self);

	if ( XX-> selection == XdndSelection )
		guts. xdnd_clipboard = self;

	return true;
}

static void
clipboard_free_data( void * data, int size, Handle id)
{
	if ( size <= 0) {
		if ( size == 0 && data != NULL) free( data);
		return;
	}
	if ( id == cfBitmap) {
		int i;
		Pixmap * p = (Pixmap*) data;
		for ( i = 0; i < size/sizeof(Pixmap); i++, p++)
			if ( *p)
				XFreePixmap( DISP, *p);
	}
	free( data);
}

/*
	each clipboard type can be represented by a set of
	X properties pairs, where each is X name and X type.
	get_typename() returns such pairs by the index.
*/
static Atom
get_typename( Handle id, int index, Atom * type)
{
	if ( type) *type = None;
	switch ( id) {
	case cfText:
		if ( index > 1) return None;
		if ( index == 1) {
			if ( type) *type = PLAINTEXT_MIME;
			return PLAINTEXT_MIME;
		}
	case cfUTF8:
		if ( index > 1) return None;
		if ( index == 1) {
			if ( type) *type = UTF8_MIME;
			return UTF8_MIME;
		}
	case cfBitmap:
		if ( index > 1) return None;
		if ( index == 1) {
			if ( type) *type = XA_BITMAP;
			return XA_BITMAP;
		}
	case cfTargets:
		if ( index > 1) return None;
		if ( index == 1) {
			if ( type) *type = CF_TARGETS;
			return CF_NAME(id);
		}
	}
	if ( index > 0) return None;
	if ( type) *type = CF_TYPE(id);
	return CF_NAME(id);
}

void
prima_clipboard_kill_item( PClipboardDataItem item, Handle id)
{
	item += id;
	clipboard_free_data( item-> data, item-> size, id);
	if ( item-> image ) {
		if ( PObject(item->image)-> mate )
			SvREFCNT_dec( SvRV( PObject(item-> image)->mate ));
		unprotect_object( item-> image );
	}
	item-> image = NULL_HANDLE;
	item-> data = NULL;
	item-> size = 0;
	item-> name = get_typename( id, 0, NULL);
	item-> immediate = true;
}

/*
	Deletes a transfer record from pending xfer chain.
*/
static void
delete_xfer( PClipboardSysData cc, ClipboardXfer * xfer)
{
	ClipboardXferKey key;
	CLIPBOARD_XFER_KEY( key, xfer-> requestor, xfer-> property);
	if ( guts. clipboard_xfers) {
		IV refcnt;
		hash_delete( guts. clipboard_xfers, key, sizeof( key), false);
		refcnt = PTR2IV( hash_fetch( guts. clipboard_xfers, &xfer-> requestor, sizeof(XWindow)));
		if ( --refcnt == 0) {
			XSelectInput( DISP, xfer-> requestor, 0);
			hash_delete( guts. clipboard_xfers, &xfer-> requestor, sizeof(XWindow), false);
		} else {
			if ( refcnt < 0) refcnt = 0;
			hash_store( guts. clipboard_xfers, &xfer-> requestor, sizeof(XWindow), INT2PTR(void*, refcnt));
		}
	}
	if ( cc-> xfers)
		list_delete( cc-> xfers, ( Handle) xfer);
	if ( xfer-> data_detached && xfer-> data_master)
		clipboard_free_data( xfer-> data, xfer-> size, xfer-> id);
	free( xfer);
}

Bool
apc_clipboard_destroy( Handle self)
{
	DEFCC;
	int i;

	if ( guts. xdnd_clipboard == self )
		guts. xdnd_clipboard = NULL_HANDLE;

	if (XX-> selection == None) return true;

	if ( XX-> xfers) {
		for ( i = 0; i < XX-> xfers-> count; i++)
			delete_xfer( XX, ( ClipboardXfer*) XX-> xfers-> items[i]);
		plist_destroy( XX-> xfers);
	}

	for ( i = 0; i < guts. clipboard_formats_count; i++) {
		if ( XX-> external) prima_clipboard_kill_item( XX-> external, i);
		if ( XX-> internal) prima_clipboard_kill_item( XX-> internal, i);
	}

	free( XX-> external);
	free( XX-> internal);
	hash_delete( guts.clipboards, &XX->selection, sizeof(XX->selection), false);

	XX-> selection = None;
	return true;
}

Bool
apc_clipboard_open( Handle self)
{
	DEFCC;
	if ( XX-> xdnd_receiving ) return true;
	if ( XX-> opened) return false;
	XX-> opened = true;

	if ( !XX-> inside_event) XX-> need_write = false;

	return true;
}

Bool
apc_clipboard_close( Handle self)
{
	DEFCC;
	if ( XX-> xdnd_receiving ) return true; /* XXX */
	if ( !XX-> opened) return false;
	XX-> opened = false;

	/* check if UTF8 is present and Text is not, and downgrade */
	if ( XX-> need_write &&
		XX-> internal[cfUTF8]. size > 0 &&
		XX-> internal[cfText]. size == 0) {
		Byte * src = XX-> internal[cfUTF8]. data;
		int len    = utf8_length( src, src + XX-> internal[cfUTF8]. size);
		if (( XX-> internal[cfText]. data = malloc( len))) {
			STRLEN charlen;
			U8 *dst, *end = src + XX-> internal[cfUTF8]. size;
			dst = XX-> internal[cfText]. data;
			XX-> internal[cfText]. size = len;
			while ( len--) {
				register UV u = prima_utf8_uvchr_end(src, end, &charlen);
				*(dst++) = ( u < 0x7f) ? u : '?'; /* XXX employ $LANG and iconv() */
				src += charlen;
			}
		}
	}


	if ( !XX-> inside_event) {
		int i;
		for ( i = 0; i < guts. clipboard_formats_count; i++)
			prima_clipboard_kill_item( XX-> external, i);
		if ( XX-> need_write && (!XX->xdnd_receiving || XX->xdnd_sending))
			if ( XGetSelectionOwner( DISP, XX-> selection) != WIN)
				XSetSelectionOwner( DISP, XX-> selection, WIN, CurrentTime);
	}

	return true;
}

/*
	Detaches data for pending transfers from XX, so eventual changes
	to XX->internal would not affect them. detach_xfers() should be
	called before clipboard_kill_item(XX-> internal), otherwise
	there's a chance of coredump.
*/
void
prima_detach_xfers( PClipboardSysData XX, Handle id, Bool clear_original_data)
{
	int i, got_master = 0, got_anything = 0;
	if ( !XX-> xfers) return;
	for ( i = 0; i < XX-> xfers-> count; i++) {
		ClipboardXfer * x = ( ClipboardXfer *) XX-> xfers-> items[i];
		if ( x-> data_detached || x-> id != id) continue;
		got_anything = 1;
		if ( !got_master) {
			x-> data_master = true;
			got_master = 1;
		}
		x-> data_detached = true;
	}
	if ( got_anything && clear_original_data) {
		XX-> internal[id]. data = NULL;
		XX-> internal[id]. size = 0;
		XX-> internal[id]. name = get_typename( id, 0, NULL);
	}
}

Bool
apc_clipboard_clear( Handle self)
{
	DEFCC;
	int i;

	for ( i = 0; i < guts. clipboard_formats_count; i++) {
		prima_detach_xfers( XX, i, true);
		prima_clipboard_kill_item( XX-> internal, i);
		prima_clipboard_kill_item( XX-> external, i);
	}

	if ( XX-> inside_event) {
		XX-> need_write = true;
	} else if ( !XX->xdnd_receiving || XX->xdnd_sending) {
		XWindow owner = XGetSelectionOwner( DISP, XX-> selection);
		XX-> need_write = false;
		if ( owner != None && owner != WIN)
			XSetSelectionOwner( DISP, XX-> selection, None, CurrentTime);
	}

	return true;
}

typedef struct {
	Atom selection;
	long mask;
} SelectionProcData;

#define SELECTION_NOTIFY_MASK 1
#define PROPERTY_NOTIFY_MASK  2

static int
selection_filter( Display * disp, XEvent * ev, SelectionProcData * data)
{
	switch ( ev-> type) {
	case PropertyNotify:
		return (data-> mask & PROPERTY_NOTIFY_MASK) && (data-> selection == ev-> xproperty. atom);
	case SelectionRequest:
	case SelectionClear:
	case MappingNotify:
		return true;
	case SelectionNotify:
		return (data-> mask & SELECTION_NOTIFY_MASK) && (data-> selection == ev-> xselection. selection);
	case ClientMessage:
		if ( ev-> xclient. window == WIN ||
			ev-> xclient. window == guts. root ||
			ev-> xclient. window == None) return true;
		if ( hash_fetch( guts.windows, (void*)&ev-> xclient. window,
			sizeof(ev-> xclient. window))) return false;
		return true;
	}
	return false;
}

#define CFDATA_NONE            0
#define CFDATA_NOT_ACQUIRED  (-1)
#define CFDATA_ERROR         (-2)

int
prima_read_property( XWindow window, Atom property, Atom * type, int * format,
	unsigned long * size, unsigned char ** data, Bool delete_property)
{
	int ret = ( *size > 0) ? RPS_PARTIAL : RPS_ERROR;
	unsigned char * prop, *a1;
	unsigned long n, left, offs = 0, new_size, big_offs = *size;

	XCHECKPOINT;
	Cdebug("clipboard: read_property: %s\n", XGetAtomName(DISP, property));
	while ( 1) {
		if ( XGetWindowProperty( DISP, window, property,
			offs, guts. limits. request_length - 4, false,
			AnyPropertyType,
			type, format, &n, &left, &prop) != Success) {
			if ( delete_property )
				XDeleteProperty( DISP, window, property);
			Cdebug("clipboard:fail\n");
			return ret;
		}
		XCHECKPOINT;
		Cdebug("clipboard: type=0x%x(%s) fmt=%d n=%d left=%d\n",
				*type, XGetAtomName(DISP,*type), *format, n, left);

		if ( *format == 32) *format = CF_32;

		if ( *type == 0 ) return RPS_NODATA;

		new_size = n * *format / 8;

		if ( new_size > 0) {
			if ( !( a1 = realloc( *data, big_offs + offs * 4 + new_size))) {
				warn("Not enough memory: %ld bytes\n", offs * 4 + new_size);
				if ( delete_property )
					XDeleteProperty( DISP, window, property);
				XFree( prop);
				return ret;
			}
			*data = a1;
			memcpy( *data + big_offs + offs * 4, prop, new_size);
			*size = big_offs + (offs * 4) + new_size;
			if ( *size > INT_MAX) *size = INT_MAX;
			offs += new_size / 4;
			ret = RPS_PARTIAL;
		}
		XFree( prop);
		if ( left <= 0 || *size == INT_MAX || n * *format == 0) break;
	}

	if ( delete_property )
		XDeleteProperty( DISP, window, property);
	XCHECKPOINT;

	return RPS_OK;
}

static Bool
query_datum( Handle self, Handle id, Atom query_target, Atom query_type)
{
	DEFCC;
	XEvent ev;
	Atom type;
	int format, rps;
	SelectionProcData spd;
	unsigned long size = 0, incr = 0, old_size, delay;
	long timestamp;
	unsigned char * data;
	struct timeval start_time, timeout;
	XWindow window;

	/* init */
	if ( query_target == None) return false;
	window = XX-> xdnd_receiving ? PWidget(guts.xdndr_receiver)->handle : WIN;
	if ( window == None ) return false; /* don't operate on XDND clipboard outside the drag */
	timestamp = XX-> xdnd_receiving ? guts.xdndr_timestamp : guts.last_time;

	data = malloc(0);
	XX-> external[id]. size = CFDATA_ERROR;
	gettimeofday( &start_time, NULL);
	XCHECKPOINT;

	Cdebug("clipboard:convert %s from %08x on %s\n", XGetAtomName( DISP, query_target), window, XGetAtomName(DISP, XX->selection));
	XDeleteProperty( DISP, WIN, XX-> selection);
	XConvertSelection( DISP, XX-> selection, query_target, XX-> selection, window, timestamp);
	XFlush( DISP);
	XCHECKPOINT;

	/* wait for SelectionNotify */
	spd. selection = XX-> selection;
	spd. mask = SELECTION_NOTIFY_MASK;
	while ( 1) {
		Bool ok;

		ok = XCheckIfEvent( DISP, &ev, (XIfEventProcType)selection_filter, (char*)&spd);
		if ( !ok ) {
			gettimeofday( &timeout, NULL);
			delay = (( timeout. tv_sec - start_time. tv_sec) * 1000 +
				( timeout. tv_usec - start_time. tv_usec) / 1000);
			if ( delay > guts. clipboard_event_timeout) {
				Cdebug("clipboard:selection timeout\n");
				goto FAIL;
			}
			XFlush( DISP);
			continue;
		}
		gettimeofday( &timeout, NULL);
		delay = 2 * (( timeout. tv_sec - start_time. tv_sec) * 1000 +
			( timeout. tv_usec - start_time. tv_usec) / 1000);
		if ( ev. type != SelectionNotify) {
			prima_handle_event( &ev, NULL);
			continue;
		}
		if ( ev. xselection. property == None) goto FAIL;
		Cdebug("clipboard:read SelectionNotify  %s %s\n",
				XGetAtomName(DISP, ev. xselection. property),
				XGetAtomName(DISP, ev. xselection. target));
		if ( prima_read_property( window, ev. xselection. property, &type, &format, &size, &data, 1) > RPS_PARTIAL)
			goto FAIL;
		XFlush( DISP);
		break;
	}
	XCHECKPOINT;

	if ( type != XA_INCR) { /* ordinary, single-property selection */
		if ( format != CF_FORMAT(id) || type != query_type) {
			if ( format != CF_FORMAT(id))
				Cdebug("clipboard: id=%d: formats mismatch: got %d, want %d\n", id, format, CF_FORMAT(id));
			if ( type != query_type)
				Cdebug("clipboard: id=%d: types mismatch: got %s, want %s\n", id,
						XGetAtomName(DISP,type), XGetAtomName(DISP,query_type));
			return false;
		}
		XX-> external[id]. size = size;
		XX-> external[id]. data = data;
		XX-> external[id]. name = query_target;
		return true;
	}

	/* setup INCR */
	if ( format != CF_32 || size < 4) goto FAIL;
	incr = (unsigned long) *(( Atom*) data);
	if ( incr == 0) goto FAIL;
	size = 0;
	spd. mask = PROPERTY_NOTIFY_MASK;

	while ( 1) {
		/* wait for PropertyNotify */
		while ( XCheckIfEvent( DISP, &ev, (XIfEventProcType)selection_filter, (char*)&spd) == False) {
			gettimeofday( &timeout, NULL);
			if ((( timeout. tv_sec - start_time. tv_sec) * 1000 +
				( timeout. tv_usec - start_time. tv_usec) / 1000) > delay)
				goto END_LOOP;
		}
		if ( ev. type != PropertyNotify) {
			prima_handle_event( &ev, NULL);
			continue;
		}
		if ( ev. xproperty. state != PropertyNewValue) continue;
		start_time = timeout;
		old_size = size;

		rps = prima_read_property( window, ev. xproperty. atom, &type, &format, &size, &data, 1);
		XFlush( DISP);
		if ( rps == RPS_NODATA) continue;
		if ( rps == RPS_ERROR) goto FAIL;
		if ( format != CF_FORMAT(id) || type != CF_TYPE(id)) return false;
		if ( size > incr ||                       /* read all in INCR */
			rps == RPS_PARTIAL ||                /* failed somewhere */
			( size == incr && old_size == size)  /* wait for empty PropertyNotify otherwise */
			) break;
	}
END_LOOP:
	XCHECKPOINT;

	XX-> external[id]. size   = size;
	XX-> external[id]. data   = data;
	XX-> external[id]. name   = query_target;
	return true;

FAIL:
	XCHECKPOINT;
	free( data);
	return false;
}

static Bool
query_data( Handle self, Handle id)
{
	DEFCC;
	Atom name, type;
	int index = 0;
	Bool filter_by_targets = id != cfTargets && XX-> external[cfTargets]. size > 0;

	/* query all types */
	while (( name = get_typename( id, index++, &type)) != None) {
		if ( filter_by_targets ) {
			Bool found = false;
			int i, length = XX->external[cfTargets].size;
			Atom * data   = (Atom*) XX->external[cfTargets].data;
			for ( i = 0; i < length / sizeof(Atom); i++) {
				if ( data[i] != name) continue;
				found = true;
				break;
			}
			if ( !found ) continue;
		}

		if ( query_datum( self, id, name, type)) return true;
	}
	return false;
}

static Atom
find_atoms( Atom * data, int length, int id)
{
	int i, index = 0;
	Atom name;

	while (( name = get_typename( id, index++, NULL)) != None) {
		for ( i = 0; i < length / sizeof(Atom); i++) {
			if ( data[i] == name)
				return name;
		}
	}
	return None;
}

void
prima_clipboard_query_targets( Handle self)
{
	DEFCC;

	if ( !XX->xdnd_receiving ) {
		if ( XX-> external[cfTargets]. size != 0)
			return;
		query_data( self, cfTargets);
	}

	/* read TARGETS, which is an array of ATOMs */
	if ( XX-> external[cfTargets].size > 0) {
		int i, size = XX-> external[cfTargets].size;
		Atom * data = ( Atom*)(XX-> external[cfTargets]. data);
		Atom ret;

		Cdebug("clipboard targets:");
		for ( i = 0; i < size/sizeof(Atom); i++)
			Cdebug("%s\n", XGetAtomName( DISP, data[i]));

		/* find our index for TARGETS[i], assign CFDATA_NOT_ACQUIRED to it */
		for ( i = 0; i < guts. clipboard_formats_count; i++) {
			if ( i == cfTargets) continue;
			ret = find_atoms( data, size, i);
			if (
				XX-> external[i]. size == 0 ||
				XX-> external[i]. size == CFDATA_ERROR
			) {
				XX-> external[i]. size = CFDATA_NOT_ACQUIRED;
				XX-> external[i]. name = ret;
			}
		}
	}
}

Bool
apc_clipboard_has_format( Handle self, Handle id)
{
	DEFCC;
	if ( id >= guts. clipboard_formats_count) return false;

	if ( XX-> inside_event) {
		return XX-> internal[id]. size > 0 || !XX->internal[id].immediate || XX-> external[id]. size > 0;
	} else {
		if ( XX-> internal[id]. size > 0 || !XX->internal[id].immediate) return true;
		prima_clipboard_query_targets(self);
		if ( XX-> external[cfTargets]. size > 0) {
			return find_atoms(
				(Atom*) XX-> external[cfTargets]. data,
				XX-> external[cfTargets]. size, id
			) != None;
		}

		if ( XX-> external[id]. size > 0 ||
			XX-> external[id]. size == CFDATA_NOT_ACQUIRED)
			return true;

		if ( XX-> external[id]. size == CFDATA_ERROR)
			return false;

		/* selection owner does not support TARGETS, so peek */
		if ( XX-> external[cfTargets]. size == 0 && XX-> internal[id]. size == 0)
			return query_data( self, id);
	}
	return false;
}

PList
apc_clipboard_get_formats( Handle self)
{
	DEFCC;
	int id;
	PList list = plist_create(guts.clipboard_formats_count, 8);
	Byte visited[1024]; /* 8K formats */

	bzero( visited, sizeof(visited));

	if ( !XX-> inside_event ) {
		int i, j, size;
		Atom * data;

		prima_clipboard_query_targets(self);
		size = XX-> external[cfTargets].size;
		data = ( Atom*)(XX-> external[cfTargets]. data);
		if ( size > 0 && data != NULL ) {
			/* TARGETS supported */
			for ( i = 0; i < size/sizeof(Atom); i++, data++) {
				Atom atom = None;
				char *name = NULL;
				/* try to map back f.ex. text/plain to Text */
				for ( j = 0; j < guts.clipboard_formats_count; j++) {
					if (*data == XX->external[j].name) {
						atom = CF_NAME(j);
						if (atom == XA_STRING )
							name = "Text";
						else if ( atom == XA_BITMAP)
							name = "Image";
						else if ( atom == UTF8_STRING )
							name = "UTF8";
					}
					if ( atom != None || name != NULL ) {
						int ofs = j >> 3;
						if ( ofs < 1024 ) visited[ofs] |= 1 << (j & 7);
					}
				}
				if ( atom == None ) atom = *data;
				if ( name == NULL ) name = XGetAtomName(DISP, atom);
				list_add( list, (Handle) duplicate_string(name));
			}
		}
	}

	for ( id = 0; id < guts. clipboard_formats_count; id++) {
		int ofs = id >> 3;
		int was_visited = ( ofs < 1024 ) ? visited[ofs] & (1 << (id & 7)) : 0;
		if (XX-> internal[id]. size > 0 || !XX->internal[id]. immediate || XX-> external[id]. size > 0) {
			char * name;
			if ( was_visited ) continue;
			switch ( id ) {
			case cfText: 
				name = "Text";
				break;
			case cfUTF8: 
				name = "UTF8";
				break;
			case cfBitmap: 
				name = "Image";
				break;
			default:
				name = XGetAtomName(DISP, XX->internal[id].name);
			}
			list_add( list, (Handle) duplicate_string(name));
		}
	}

	return list;
}

static Bool
fill_target( Handle self, Atom target );

static Bool
fill_bitmap( Handle self );

Bool
apc_clipboard_get_data( Handle self, Handle id, PClipboardDataRec c)
{
	DEFCC;
	STRLEN size;
	unsigned char * data;
	Bool imm;

	if ( id >= guts. clipboard_formats_count) return false;

	if ( !XX-> inside_event) {
		if ( XX-> internal[id]. size == 0) {
			if ( XX-> external[id]. size == CFDATA_NOT_ACQUIRED) {
				if ( !query_data( self, id)) return false;
			}
			if ( XX-> external[id]. size == CFDATA_ERROR) return false;
		}
	}
	if ( XX-> internal[id]. size == CFDATA_ERROR) return false;

	if ( XX-> internal[id]. size > 0 || !XX-> internal[id]. immediate) {
		size = XX-> internal[id]. size;
		data = XX-> internal[id]. data;
		imm  = XX-> internal[id]. immediate;
	} else {
		size = XX-> external[id]. size;
		data = XX-> external[id]. data;
		imm  = true;
	}
	if ( size == 0 || data == NULL) {
		Bool ret;
		if ( imm ) return false;
		if ( id == cfBitmap ) {
			ret = fill_bitmap(self);
		} else {
			Atom name, type;
			int index = 0;
			while (( name = get_typename( id, index++, &type)) != None) {
				ret = fill_target(self, name);
				if ( ret ) break;
			}
		}

		if ( !ret ) return false;
		size = XX-> internal[id]. size;
		data = XX-> internal[id]. data;
	}

	switch ( id) {
	case cfBitmap:
		if ( XX-> internal[cfBitmap].image) {
			PObject o = (PObject) XX->internal[cfBitmap].image;
			if (o-> stage == csNormal)
				c->image = (Handle)o;
		} else if ( XX->external[cfBitmap].size > 0 ) {
			XWindow foo;
			Pixmap px = *(( Pixmap*)( XX-> external[id]. data ));
			unsigned int dummy, x, y, d;
			int bar;
			if ( !XGetGeometry( DISP, px, &foo, &bar, &bar, &x, &y, &dummy, &d))
				return false;
			c->image = (Handle) create_object("Prima::Image", "iii",
				"width", x,
				"height", y,
				"type", (d == 1) ? imBW : guts.qdepth
			);
			if ( !prima_std_query_image( c->image, px)) {
				Object_destroy(c->image);
				return false;
			}
		}
		break;
	case cfText:
	case cfUTF8: {
		void * ret = malloc( size);
		if ( !ret) {
			warn("Not enough memory: %d bytes\n", (int)size);
			return false;
		}
		memcpy( ret, data, size);
		c-> data   = ret;
		c-> length = size;
		break;}
	default: {
		void * ret = malloc( size);
		if ( !ret) {
			warn("Not enough memory: %d bytes\n", (int)size);
			return false;
		}
		memcpy( ret, data, size);
		c-> data = ( Byte * ) ret;
		c-> length = size;
		break;}
	}
	return true;
}

Bool
apc_clipboard_set_data( Handle self, Handle id, PClipboardDataRec c)
{
	DEFCC;
	if ( id >= guts. clipboard_formats_count) return false;

	if ( id >= cfTargets && id < cfCOUNT ) return false;
	prima_detach_xfers( XX, id, true);
	prima_clipboard_kill_item( XX-> internal, id);

	switch ( id) {
	case cfBitmap:
		if (( XX-> internal[id]. image = c-> image) != NULL_HANDLE) {
			protect_object( XX-> internal[id]. image );
			SvREFCNT_inc( SvRV( PObject(XX-> internal[id]. image)-> mate ));
			XX-> internal[id]. immediate = false;
		}
		break;
	default:
		if ( c-> length < 0 ) {
			XX-> internal[id]. immediate = false;
		} else {
			if ( !( XX-> internal[id]. data = malloc( c-> length)))
				return false;
			XX-> internal[id]. size = c-> length;
			memcpy( XX-> internal[id]. data, c-> data, c-> length);
		}
		break;
	}
	XX-> need_write = true;
	return true;
}

static Bool
expand_clipboards( Handle self, int keyLen, void * key, void * dummy)
{
	DEFCC;
	PClipboardDataItem f;

	if ( !( f = realloc( XX-> internal,
		sizeof( ClipboardDataItem) * guts. clipboard_formats_count))) {
		guts. clipboard_formats_count--;
		return true;
	}
	f[ guts. clipboard_formats_count-1].size      = 0;
	f[ guts. clipboard_formats_count-1].data      = NULL;
	f[ guts. clipboard_formats_count-1].name      = CF_NAME(guts. clipboard_formats_count-1);
	f[ guts. clipboard_formats_count-1].immediate = true;
	f[ guts. clipboard_formats_count-1].image     = NULL_HANDLE;
	XX-> internal = f;
	if ( !( f = realloc( XX-> external,
		sizeof( ClipboardDataItem) * guts. clipboard_formats_count))) {
		guts. clipboard_formats_count--;
		return true;
	}
	f[ guts. clipboard_formats_count-1].size      = 0;
	f[ guts. clipboard_formats_count-1].data      = NULL;
	f[ guts. clipboard_formats_count-1].name      = CF_NAME(guts. clipboard_formats_count-1);
	f[ guts. clipboard_formats_count-1].immediate = true;       /* unused */
	f[ guts. clipboard_formats_count-1].image     = NULL_HANDLE;  /* unused */
	XX-> external = f;
	return false;
}

Handle
apc_clipboard_register_format( Handle self, const char* format)
{
	int i;
	Atom x = XInternAtom( DISP, format, false);
	Atom *f;

	for ( i = 0; i < guts. clipboard_formats_count; i++) {
		if ( x == CF_NAME(i))
			return i;
	}

	if ( !( f = realloc( guts. clipboard_formats,
		sizeof( Atom) * 3 * ( guts. clipboard_formats_count + 1))))
		return false;

	guts. clipboard_formats = f;
	CF_ASSIGN( guts. clipboard_formats_count, x, x, 8);
	guts. clipboard_formats_count++;

	if ( hash_first_that( guts. clipboards, (void*)expand_clipboards, NULL, NULL, NULL))
		return -1;

	return guts. clipboard_formats_count - 1;
}

Bool
apc_clipboard_deregister_format( Handle self, Handle id)
{
	return true;
}

ApiHandle
apc_clipboard_get_handle( Handle self)
{
	return C(self)-> selection;
}

Bool
apc_clipboard_is_dnd( Handle self)
{
	return guts. xdnd_clipboard == self;
}

static Bool
delete_xfers( Handle self, int keyLen, void * key, XWindow * window)
{
	DEFCC;
	if ( XX-> xfers) {
		int i;
		for ( i = 0; i < XX-> xfers-> count; i++)
			delete_xfer( XX, ( ClipboardXfer*) XX-> xfers-> items[i]);
	}
	hash_delete( guts. clipboard_xfers, window, sizeof( XWindow), false);
	return false;
}

int
prima_clipboard_fill_targets( Handle self)
{
	DEFCC;
	int i, count = 0, have_utf8 = 0, have_plaintext = 0;
	Atom * ci;
	prima_detach_xfers( XX, cfTargets, true);
	prima_clipboard_kill_item( XX-> internal, cfTargets);

	for ( i = 0; i < guts. clipboard_formats_count; i++) {
		if ( i != cfTargets && (XX-> internal[i]. size > 0 || !XX-> internal[i]. immediate)) {
			count++;
			if ( i == cfUTF8) {
				count++;
				have_utf8 = 1;
			}
			if ( i == cfText) {
				count++;
				have_plaintext = 1;
			}
		}
	}
	if ( count == 0 ) return 0;

	if (( XX-> internal[cfTargets]. data = malloc( count * sizeof( Atom)))) {
		Cdebug("clipboard: fill targets: ", guts. clipboard_formats_count);
		XX-> internal[cfTargets]. size = count * sizeof( Atom);
		ci = (Atom*)XX-> internal[cfTargets]. data;
		for ( i = 0; i < guts. clipboard_formats_count; i++) {
			if ( i != cfTargets && ( XX-> internal[i]. size > 0 || !XX-> internal[i]. immediate)) {
				*(ci++) = CF_NAME(i);
				Cdebug("%s ", XGetAtomName(DISP, CF_NAME(i)));
			}
		}
		if ( have_utf8) {
			*(ci++) = UTF8_MIME;
			Cdebug("UTF8_MIME ");
		}
		if ( have_plaintext) {
			*(ci++) = PLAINTEXT_MIME;
			Cdebug("PLAINTEXT_MIME ");
		}
		Cdebug("\n");
	}
	return count;
}

static Bool
fill_bitmap( Handle self )
{
	Pixmap px;
	PClipboardDataItem c;

	c = &C(self)->internal[cfBitmap];
	if ( !c-> image || PObject(c->image)->stage != csNormal) return false;

	px = prima_std_pixmap( c-> image, CACHE_LOW_RES);
	if ( !px) return false;

	if ( !( c-> data = malloc( sizeof( px)))) {
		XFreePixmap( DISP, px);
		return false;
	}

	c->immediate = true;
	c->size = sizeof(px);
	memcpy( c->data, &px, sizeof(px));

	return true;
}

static Bool
fill_target( Handle self, Atom target )
{
	PClipboardSysData CC = C(self);

	Bool event  = CC-> inside_event;
	Bool flag   = exception_block(true);
	Event ev    = { cmClipboard };
	int h_stage = PObject(self)->stage;

	CC-> inside_event = true;
	CC-> opened       = true;
	protect_object(self);

	ev.gen.p = XGetAtomName(DISP, target);
	CComponent(self)->message(self, &ev);

	unprotect_object(self);
	exception_block(flag);
	if ( h_stage == csDead ) return false;

	CC-> opened       = false;
	CC-> inside_event = event;
	if ( exception_charged()) return false;

	return true;
}

static void
handle_selection_request( XEvent *ev, XWindow win, Handle self)
{
	XEvent xe;
	int i, id = -1;
	Atom
		prop   = ev-> xselectionrequest. property,
		target = ev-> xselectionrequest. target;
	self = ( Handle) hash_fetch( guts. clipboards, &ev-> xselectionrequest. selection, sizeof( Atom));

	guts. last_time = ev-> xselectionrequest. time;
	xe. type      = SelectionNotify;
	xe. xselection. send_event = true;
	xe. xselection. serial    = ev-> xselectionrequest. serial;
	xe. xselection. display   = ev-> xselectionrequest. display;
	xe. xselection. requestor = ev-> xselectionrequest. requestor;
	xe. xselection. selection = ev-> xselectionrequest. selection;
	xe. xselection. target    = target;
	xe. xselection. property  = None;
	xe. xselection. time      = ev-> xselectionrequest. time;

	Cdebug("clipboard: from %08x %s at %s\n", ev-> xselectionrequest. requestor,
		XGetAtomName( DISP, ev-> xselectionrequest. target),
		XGetAtomName( DISP, ev-> xselectionrequest. property)
	);

	if ( self) {
		PClipboardSysData CC = C(self);
		int format, utf8_mime = 0, plaintext_mime = 0;

		for ( i = 0; i < guts. clipboard_formats_count; i++) {
			if ( xe. xselection. target == CC-> internal[i]. name) {
				id = i;
				break;
			} else if ( i == cfUTF8 && xe. xselection. target == UTF8_MIME) {
				id = i;
				utf8_mime = 1;
				break;
			} else if ( i == cfText && xe. xselection. target == PLAINTEXT_MIME) {
				id = i;
				plaintext_mime = 1;
				break;
			}
		}
		if ( id < 0) goto SEND_EMPTY;
		for ( i = 0; i < guts. clipboard_formats_count; i++)
			prima_clipboard_kill_item( CC-> external, i);

		CC-> target = xe. xselection. target;
		CC-> need_write = false;

		if ( !CC-> internal[id]. immediate && id != cfTargets ) {
			Bool ret;
			ret = ( id == cfBitmap ) ? fill_bitmap(self) : fill_target(self, target);
			if ( !ret ) goto SEND_EMPTY;
		}

		format = CF_FORMAT(id);
		target = CF_TYPE( id);
		if ( utf8_mime) target = UTF8_MIME;
		if ( plaintext_mime) target = PLAINTEXT_MIME;

		if ( id == cfTargets)
			prima_clipboard_fill_targets(self);

		if ( CC-> internal[id]. size > 0) {
			Atom incr;
			int mode = PropModeReplace;
			unsigned char * data = CC-> internal[id]. data;
			unsigned long size = CC-> internal[id]. size * 8 / format;
			if ( CC-> internal[id]. size > guts. limits. request_length - 4) {
				int ok = 0;
				int reqlen = guts. limits. request_length - 4;
				/* INCR */
				if ( !guts. clipboard_xfers)
					guts. clipboard_xfers = hash_create();
				if ( !CC-> xfers)
					CC-> xfers = plist_create( 1, 1);
				if ( CC-> xfers && guts. clipboard_xfers) {
					ClipboardXfer * x = malloc( sizeof( ClipboardXfer));
					if ( x) {
						IV refcnt;
						ClipboardXferKey key;

						bzero( x, sizeof( ClipboardXfer));
						list_add( CC-> xfers, ( Handle) x);
						x-> size = CC-> internal[id]. size;
						x-> data = CC-> internal[id]. data;
						x-> blocks = ( x-> size / reqlen ) + ( x-> size % reqlen) ? 1 : 0;
						x-> requestor = xe. xselection. requestor;
						x-> property  = prop;
						x-> target    = xe. xselection. target;
						x-> self      = self;
						x-> format    = format;
						x-> id        = id;
						gettimeofday( &x-> time, NULL);

						CLIPBOARD_XFER_KEY( key, x-> requestor, x-> property);
						hash_store( guts. clipboard_xfers, key, sizeof(key), (void*) x);
						refcnt = PTR2IV( hash_fetch( guts. clipboard_xfers, &x-> requestor, sizeof( XWindow)));
						if ( refcnt++ == 0)
							XSelectInput( DISP, x-> requestor, PropertyChangeMask|StructureNotifyMask);
						hash_store( guts. clipboard_xfers, &x-> requestor, sizeof(XWindow), INT2PTR( void*, refcnt));

						format = CF_32;
						size = 1;
						incr = ( Atom) CC-> internal[id]. size;
						data = ( unsigned char*) &incr;
						ok = 1;
						target = XA_INCR;
						Cdebug("clipboard: init INCR for %08x %d\n", x-> requestor, x-> property);
					}
				}
				if ( !ok) size = reqlen;
			}

			if ( format == CF_32) format = 32;
			XChangeProperty(
				xe. xselection. display,
				xe. xselection. requestor,
				prop, target, format, mode, data, size);
			Cdebug("clipboard: store prop %s f=%d s=%d\n", XGetAtomName( DISP, prop), format, size);
			xe. xselection. property = prop;
		}

		/* content of PIXMAP or BITMAP is seemingly gets invalidated
			after the selection transfer, unlike the string data format */
		if ( id == cfBitmap) {
			bzero( CC-> internal[id].data, CC-> internal[id].size);
			bzero( CC-> external[id].data, CC-> external[id].size);
			prima_clipboard_kill_item( CC-> internal, id);
			prima_clipboard_kill_item( CC-> external, id);
		}
	}
SEND_EMPTY:

	XSendEvent( xe.xselection.display, xe.xselection.requestor, false, 0, &xe);
	XFlush( DISP);
	Cdebug("clipboard:id %d, SelectionNotify to %08x , %s %s\n", id, xe.xselection.requestor,
		xe. xselection. property ? XGetAtomName( DISP, xe. xselection. property) : "None",
		XGetAtomName( DISP, xe. xselection. target));
	exception_check_raise();
}

static void
handle_selection_clear( XEvent *ev, XWindow win, Handle self)
{
	guts. last_time = ev-> xselectionclear. time;
	if ( XGetSelectionOwner( DISP, ev-> xselectionclear. selection) != WIN) {
		Handle c = ( Handle) hash_fetch( guts. clipboards,
			&ev-> xselectionclear. selection, sizeof( Atom));
		guts. last_time = ev-> xselectionclear. time;
		if (c) {
			int i;
			C(c)-> selection_owner = NULL_HANDLE;
			for ( i = 0; i < guts. clipboard_formats_count; i++) {
				prima_detach_xfers( C(c), i, true);
				prima_clipboard_kill_item( C(c)-> external, i);
				prima_clipboard_kill_item( C(c)-> internal, i);
			}
		}
	}
}

static void
handle_property_notify( XEvent *ev, XWindow win, Handle self)
{
	unsigned long offs, size, reqlen = guts. limits. request_length - 4, format;
	ClipboardXfer * x = ( ClipboardXfer *) self;
	PClipboardSysData CC = C(x-> self);

	if ( ev-> xproperty. state != PropertyDelete)
		return;

	offs = x-> offset * reqlen;
	if ( offs >= x-> size) { /* clear termination */
		size = 0;
		offs = 0;
	} else {
		size = x-> size - offs;
		if ( size > reqlen) size = reqlen;
	}
	Cdebug("clipboard: put %d %d in %08x %d\n", x-> offset, size, x-> requestor, x-> property);
	if ( x-> format > 8)  size /= 2;
	if ( x-> format > 16) size /= 2;
	format = ( x-> format == CF_32) ? 32 : x-> format;
	XChangeProperty( DISP, x-> requestor, x-> property, x-> target,
		format, PropModeReplace,
		x-> data + offs, size);
	XFlush( DISP);
	x-> offset++;
	if ( size == 0) delete_xfer( CC, x);
}

static void
handle_destroy_notify( XEvent *ev, XWindow win, Handle self)
{
	Cdebug("clipboard: destroy xfers at %08x\n", ev-> xdestroywindow. window);
	hash_first_that( guts. clipboards, (void*)delete_xfers, (void*) &ev-> xdestroywindow. window, NULL, NULL);
	XFlush( DISP);
}

void
prima_handle_selection_event( XEvent *ev, XWindow win, Handle self)
{
	XCHECKPOINT;
	switch ( ev-> type) {
	case SelectionRequest:
		handle_selection_request(ev, win, self);
		break;
	case SelectionClear:
		handle_selection_clear(ev, win, self);
		break;
	case PropertyNotify:
		handle_property_notify(ev, win, self);
		break;
	case DestroyNotify:
		handle_destroy_notify(ev, win, self);
		break;
	}
	XCHECKPOINT;
}

