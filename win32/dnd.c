#include "win32\win32guts.h"
#include <ole2.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Image.h"
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define DHANDLE(x) dsys(x) handle

#define DEFCC IDataObject* data = (IDataObject*) ( guts.dndInsideEvent ? guts.dndDataReceiver : guts.dndDataSender );

static PList formats = NULL;

/*
	Classes
*/

/* IUnknown */
typedef struct {
	IUnknownVtbl* lpVtbl;
	const GUID *guid;
	ULONG refcnt;
} OLEUnknown, *POLEUnknown;

static void*
create_ole_object(int size, void * vmt, const GUID * guid)
{
	void * object;
	if ( !( object = malloc(size)))
		return false;
	bzero(object,size);
	((POLEUnknown)object)->refcnt = 1;
	((POLEUnknown)object)->guid   = guid;
	((POLEUnknown)object)->lpVtbl = (void*)vmt;
	return object;
}

#define CreateObject(type) create_ole_object(sizeof(type),&type ## VMT, &IID_I ## type)

static ULONG __stdcall
OLEUnknown__AddRef(POLEUnknown self)
{
	return self->refcnt++;
}

static ULONG __stdcall
OLEUnknown__Release(POLEUnknown self)
{
	if ( --(self->refcnt) == 0 ) {
		free(self);
		return 0;
	} else
		return self->refcnt;
}

static HRESULT __stdcall
OLEUnknown__QueryInterface(POLEUnknown self, REFIID riid, void **object)
{
	if (
		IsEqualGUID(riid, &IID_IUnknown) ||
		IsEqualGUID(riid, self->guid)
	) {
		self->lpVtbl->AddRef((IUnknown*) self);
		*object = self;
		return S_OK;
	} else {
		*object = NULL;
		return E_NOINTERFACE;
	}
}

/* IDropTarget */
typedef struct {
	IDropTargetVtbl* lpVtbl;
	GUID * guid;
	ULONG  refcnt;
	Handle widget;
	Box    pad;
	int    first_action;
	int    last_action;
} DropTarget, *PDropTarget;

int
convert_modmap(int modmap)
{
	return 0
		| ((modmap & MK_CONTROL  ) ? kmCtrl  : 0)
		| ((modmap & MK_SHIFT    ) ? kmShift : 0)
		| ((modmap & MK_ALT      ) ? kmAlt   : 0)
		| ((modmap & MK_LBUTTON  ) ? mb1     : 0)
		| ((modmap & MK_RBUTTON  ) ? mb2     : 0)
		| ((modmap & MK_MBUTTON  ) ? mb3     : 0)
		| ((modmap & MK_XBUTTON1 ) ? mb4     : 0)
		| ((modmap & MK_XBUTTON2 ) ? mb5     : 0)
	;
}

static HRESULT __stdcall
DropTarget__DragEnter(PDropTarget self, IDataObject *data, DWORD modmap, POINTL pt, DWORD *effect)
{
	int stage;
	Handle w;
	Event ev = { cmDragBegin };

	if ((ev.dnd.clipboard = guts.clipboards[CLIPBOARD_DND]) == nilHandle) {
		*effect = DROPEFFECT_NONE;
		return S_OK;
	}

	guts.dndDataReceiver = data;
	bzero(&(self->pad), sizeof(self->pad));
	self->last_action  = DROPEFFECT_NONE;
	self->first_action = *effect & dndMask;

	w = self->widget;
	protect_object(w);
	guts.dndInsideEvent = true;
	CWidget(w)->message(w, &ev);
	guts.dndInsideEvent = false;
	stage = PObject(w)-> stage;
	unprotect_object(w);
	if ( stage == csDead ) {
		self->widget = nilHandle;
		guts. dndDataReceiver = NULL;
		*effect = DROPEFFECT_NONE;
		return S_OK;
	}

	self->lpVtbl->DragOver((IDropTarget*)self, modmap, pt, effect);

	return S_OK;
}

static HRESULT __stdcall
DropTarget__DragOver(PDropTarget self, DWORD modmap, POINTL pt, DWORD *effect)
{
	int stage;
	Handle w;
	RECT r;
	Event ev = { cmDragOver };

	if ( self->widget == nilHandle || guts.clipboards[CLIPBOARD_DND] == nilHandle) {
		*effect = DROPEFFECT_NONE;
		return S_OK;
	}

	w = self->widget;
	ev.dnd.clipboard = guts.clipboards[CLIPBOARD_DND];

	GetWindowRect( DHANDLE(w), &r);
	ev.dnd.where.x   = pt.x;
	ev.dnd.where.y   = r.bottom - r.top - pt.y - 1;
	if (
		self->pad.width > 0 && self->pad.height > 0 &&
		(
			ev.dnd.where.x < self->pad.x ||
			ev.dnd.where.x > self->pad.width  + self->pad.x ||
			ev.dnd.where.y < self->pad.y ||
			ev.dnd.where.y > self->pad.height + self->pad.y
		)
	) {
		*effect = self-> last_action;
		return S_OK;
	}

	/* DROPEFFECT_XXX and dndXXX constants are directly mapped to each other, except DROPEFFECT_SCROLL */
	ev.dnd.action = *effect & dndMask;
	ev.dnd.modmap = convert_modmap(modmap);

	protect_object(w);
	guts.dndInsideEvent = true;
	CWidget(w)->message(w, &ev);
	guts.dndInsideEvent = false;
	stage = PObject(w)-> stage;
	unprotect_object(w);
	if ( stage == csDead ) {
		self->widget = nilHandle;
		guts. dndDataReceiver = NULL;
		*effect = DROPEFFECT_NONE;
		return S_OK;
	}

	ev.dnd.action &= self->first_action;
	self->last_action = *effect = ev.dnd.allow ? ev.dnd.action : DROPEFFECT_NONE;
	self->pad = ev.dnd.pad;

	return S_OK;
}

static HRESULT __stdcall
DropTarget__DragLeave(PDropTarget self)
{
	Handle w;
	Event ev = { cmDragEnd };

	guts. dndDataReceiver = NULL;
	if ( self->widget == nilHandle || guts.clipboards[CLIPBOARD_DND] == nilHandle)
		return S_OK;

	w = self->widget;
	ev.dnd.allow = 0;
	ev.dnd.clipboard = nilHandle;
	guts.dndInsideEvent = true;
	CWidget(w)->message(w, &ev);
	guts.dndInsideEvent = false;
	return S_OK;
}

static HRESULT __stdcall
DropTarget__Drop(PDropTarget self, IDataObject *data, DWORD modmap, POINTL pt, DWORD *effect)
{
	Handle w;
	DWORD dummy_effect;
	Event ev = { cmDragEnd };

	dummy_effect = *effect;
	self->lpVtbl->DragOver((IDropTarget*)self, modmap, pt, &dummy_effect);
	if ( self->widget == nilHandle || guts.clipboards[CLIPBOARD_DND] == nilHandle) {
		*effect = DROPEFFECT_NONE;
		guts. dndDataReceiver = NULL;
		return S_OK;
	}

	w = self->widget;
	ev.dnd.allow     = 1;
	ev.dnd.clipboard = guts.clipboards[CLIPBOARD_DND];
	ev.dnd.action    = *effect & dndMask;
	guts.dndInsideEvent = true;
	CWidget(w)->message(w, &ev);

	guts.dndInsideEvent = false;
	*effect          = ev.dnd.allow ? (ev.dnd.action & self->first_action) : DROPEFFECT_NONE;
	if ( *effect & dndCopy ) *effect = dndCopy;
	else if ( *effect & dndMove ) *effect = dndMove;
	else if ( *effect & dndLink ) *effect = dndLink;

	return S_OK;
}

static IDropTargetVtbl DropTargetVMT = {
	(void*) OLEUnknown__QueryInterface,
	(void*) OLEUnknown__AddRef,
	(void*) OLEUnknown__Release,
	(void*) DropTarget__DragEnter,
	(void*) DropTarget__DragOver,
	(void*) DropTarget__DragLeave,
	(void*) DropTarget__Drop
};

/* IDropSource */
typedef struct {
	IDropSourceVtbl* lpVtbl;
	GUID * guid;
	ULONG  refcnt;
	Handle widget;
	int    first_modmap;
	int    last_action;
} DropSource, *PDropSource;

static HRESULT __stdcall
DropSource__QueryContinueDrag(PDropSource self, BOOL escape, DWORD modmap)
{
	Event ev = { cmDragQuery };
	ev.dnd.modmap =
		convert_modmap(modmap) |
		(escape ? kmEscape : 0)
	;
	if ( escape )
		ev.dnd.allow = -1;
	else if ( self->first_modmap != (ev.dnd.modmap & self->first_modmap ))
		ev.dnd.allow = 1;
	else
		ev.dnd.allow = 0;
	CWidget(self->widget)->message(self->widget, &ev);

	if ( ev.dnd.allow < 0 )
		return DRAGDROP_S_CANCEL;
	else if ( ev.dnd.allow > 0)
		return DRAGDROP_S_DROP;
	else
		return S_OK;
}

static HRESULT __stdcall
DropSource__GiveFeedback(PDropSource self, DWORD effect)
{
	Event ev = { cmDragResponse };
	ev.dnd.action = effect & dndMask;
	ev.dnd.allow  = ev.dnd.action != dndNone;
	CWidget(self->widget)->message(self->widget, &ev);
	if ( self->last_action != ev.dnd.action ) {
		self->last_action = ev.dnd.action;
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}
	return S_OK;
}

static IDropSourceVtbl DropSourceVMT = {
	(void*) OLEUnknown__QueryInterface,
	(void*) OLEUnknown__AddRef,
	(void*) OLEUnknown__Release,
	(void*) DropSource__QueryContinueDrag,
	(void*) DropSource__GiveFeedback
};

/* IDataObject */
typedef struct {
	IDataObjectVtbl* lpVtbl;
	GUID * guid;
	ULONG  refcnt;
	List   list;
} DataObject, *PDataObject;

typedef struct {
	int formats[3], n_formats, size;
	void * data;
	Handle image;
	char payload[1]; /* data points here */
} DataObjectEntry, *PDataObjectEntry;

static TYMED
cf2tymed(int cf)
{
	return (cf == CF_BITMAP || cf == CF_PALETTE) ? TYMED_GDI : TYMED_HGLOBAL;
}

static PDataObjectEntry
dataobject_alloc_buffer(int format, int size, void * data)
{
	PDataObjectEntry entry;
	if ( !( entry = malloc( size + sizeof(DataObjectEntry)))) {
		warn("Not enough memory");
		return NULL;
	}
	bzero( entry, sizeof(DataObjectEntry));
	entry->formats[0] = format;
	entry->formats[1] = -1;
	entry->formats[2] = -1;
	entry->n_formats = 1;
	entry->size    = size;
	entry->data    = &(entry->payload);
	memcpy( entry->data, data, size );
	return entry;
}

static PDataObjectEntry
dataobject_alloc_image(Handle image)
{
	PDataObjectEntry entry;
	if ( !( entry = malloc( sizeof(DataObjectEntry)))) {
		warn("Not enough memory");
		return NULL;
	}
	bzero( entry, sizeof(DataObjectEntry));
	entry->image   = image;
	entry->formats[0] = CF_BITMAP;
	entry->formats[1] = CF_PALETTE;
	entry->formats[2] = CF_DIB;
	entry->n_formats = 3;
	protect_object(image);
	CComponent(guts.clipboards[CLIPBOARD_DND])->attach(guts.clipboards[CLIPBOARD_DND], image);
	return entry;
}

static void
dataobject_free_entry(PDataObjectEntry entry)
{
	if ( entry->formats[0] == CF_BITMAP ) {
		CComponent(guts.clipboards[CLIPBOARD_DND])->detach(guts.clipboards[CLIPBOARD_DND], entry->image, false);
		unprotect_object(entry->image);
	}
	free( entry );
}

static int
dataobject_find_entry(PDataObject data, int format, PDataObjectEntry * entry_ref)
{
	int i, j;

	if ( entry_ref ) *entry_ref = NULL;
	for ( i = 0; i < data->list.count; i++) {
		PDataObjectEntry entry = (void*) data->list.items[i];
		for ( j = 0; j < entry->n_formats; j++) {
			if ( entry->formats[j] == format ) {
				if ( entry_ref ) *entry_ref = entry;
				return i;
			}
		}
	}
	return -1;
}

static void
dataobject_clear()
{
	int i;
	PDataObject data = (PDataObject) guts.dndDataSender;
	if ( !data ) return;
	for ( i = 0; i < data->list.count; i++)
		dataobject_free_entry((PDataObjectEntry) data->list.items[i]);
	data->list.count = 0;
}

static HRESULT
dataobject_convert( PDataObjectEntry entry, int format, LPSTGMEDIUM medium)
{
	switch (format) {
	case CF_BITMAP: {
		HPALETTE p;
		if ( PImage(entry->image)->stage != csNormal)
			return E_UNEXPECTED;
		p = ( HGLOBAL) palette_create( entry-> image);
		medium->hBitmap = ( HBITMAP) image_create_bitmap( entry-> image, p, NULL, BM_AUTO);
		DeleteObject(p);
		break;
	}
	case CF_PALETTE:
		if ( PImage(entry->image)->stage != csNormal)
			return E_UNEXPECTED;
		medium->hGlobal = ( HGLOBAL) palette_create( entry-> image);
		break;
	case CF_DIB: {
		if ( PImage(entry->image)->stage != csNormal)
			return E_UNEXPECTED;
		if ((medium->hGlobal = image_create_dib(entry->image, true)) == NULL)
			return E_OUTOFMEMORY;
		break;
	}
	case CF_UNICODETEXT: {
		int ulen;
		void *ptr = NULL;

		ulen = MultiByteToWideChar(CP_UTF8, 0, (char*) entry->data, entry->size, NULL, 0) + 1;
		if (( medium->hGlobal = GlobalAlloc( GMEM_DDESHARE, ( ulen + 0) * sizeof( WCHAR))) == NULL)
			return E_OUTOFMEMORY;

		if (( ptr = GlobalLock( medium->hGlobal)) == NULL) {
			GlobalFree( medium->hGlobal);
			medium->hGlobal = NULL;
			return E_UNEXPECTED;
		}

		MultiByteToWideChar(CP_UTF8, 0, (LPSTR)entry-> data, entry->size, ptr, ulen);
		GlobalUnlock( medium->hGlobal);
		break;
	}
	case CF_TEXT: {
		int i, cr = 0;
		void *ptr = nil;
		char *src = (char*)entry->data, *dst;

		for ( i = 0; i < entry-> size; i++)
			if (src[i] == '\n' && ( i == 0 || src[i-1] != '\r'))
				cr++;

		if (( medium->hGlobal = GlobalAlloc( GMEM_DDESHARE, entry->size + cr + 1)) == NULL)
			return E_OUTOFMEMORY;
		if (( ptr = GlobalLock( medium->hGlobal)) == NULL ) {
			GlobalFree( medium->hGlobal);
			medium->hGlobal = NULL;
			return E_UNEXPECTED;
		}

		dst = ( char *) ptr;
		for ( i = 0; i < entry->size; i++) {
			if ( src[i] == '\n' && ( i == 0 || src[i-1] != '\r'))
				*(dst++) = '\r';
			*(dst++) = src[i];
		}
		*dst = 0;
		GlobalUnlock( ptr);
		break;
	}
	default: {
		char* ptr;
		if (!( medium->hGlobal = GlobalAlloc( GMEM_DDESHARE, entry-> size)))
			return E_OUTOFMEMORY;
		if ( !( ptr = ( char *) GlobalLock( medium->hGlobal))) {
			GlobalFree( medium->hGlobal );
			medium->hGlobal = NULL;
			return E_UNEXPECTED;
		}
		memcpy( ptr, entry-> data, entry-> size);
		GlobalUnlock( medium->hGlobal );
		break;
	}}

	return S_OK;
}

static HRESULT __stdcall
DataObject__GetData(PDataObject self, LPFORMATETC format, LPSTGMEDIUM medium)
{
	PDataObjectEntry entry;
	int found = -1;

	if ((found = dataobject_find_entry(self, format->cfFormat, &entry)) < 0)
		return DV_E_FORMATETC;
	if ( format->tymed != cf2tymed(format->cfFormat))
		return DV_E_TYMED;

	medium->tymed = format->tymed;
	medium->pUnkForRelease  = 0;
	return dataobject_convert(entry, format->cfFormat, medium);
}

static HRESULT __stdcall
DataObject__GetDataHere(PDataObject self, LPFORMATETC format, LPSTGMEDIUM medium)
{
	return DATA_E_FORMATETC;
}

static HRESULT __stdcall
DataObject__QueryGetData(PDataObject self, LPFORMATETC format)
{
	PDataObjectEntry entry;
	int found = -1;

	if ((found = dataobject_find_entry(self, format->cfFormat, &entry)) < 0)
		return S_FALSE;
	if ( format->tymed != cf2tymed(format->cfFormat))
		return DV_E_TYMED;

	return S_OK;
}

static HRESULT __stdcall
DataObject__GetCanonicalFormatEtc(PDataObject self, LPFORMATETC format_in, LPFORMATETC format_out)
{
	format_out->ptd = NULL;
	return E_NOTIMPL;
}

static HRESULT __stdcall
DataObject__SetData(PDataObject self, LPFORMATETC format, LPSTGMEDIUM medium, BOOL release)
{
	return E_UNEXPECTED;
}

STDAPI __stdcall SHCreateStdEnumFmtEtc(UINT cfmt, const FORMATETC* afmt, LPENUMFORMATETC *ppenumFormatEtc);

static HRESULT __stdcall
DataObject__EnumFormatEtc(PDataObject self, DWORD direction, LPENUMFORMATETC *enum_formats)
{
	int i, j, n_formats = 0;
	FORMATETC *formats, *f;

	if (direction != DATADIR_GET)
		return E_NOTIMPL;

	for ( i = 0; i < self->list.count; i++)
		n_formats += ((PDataObjectEntry)self->list.items[i])->n_formats;

	if ((formats = malloc(sizeof(FORMATETC) * n_formats)) == NULL)
		return E_OUTOFMEMORY;

	for ( i = 0, f = formats; i < self->list.count; i++) {
		PDataObjectEntry entry = (PDataObjectEntry)self->list.items[i];
		for ( j = 0; j < entry->n_formats; j++, f++) {
			f->cfFormat = entry->formats[j];
			f->ptd      = NULL;
			f->dwAspect = DVASPECT_CONTENT;
			f->lindex   = -1;
			f->tymed    = cf2tymed(entry->formats[j]);
		}
	}

	SHCreateStdEnumFmtEtc(n_formats, formats, enum_formats);
	free(formats);

	return S_OK;
}

static HRESULT __stdcall
DataObject__DAdvise(PDataObject self, LPFORMATETC format, DWORD advf, LPADVISESINK sink, DWORD *connection)
{
	return E_FAIL;
}

static HRESULT __stdcall
DataObject__Dunadvise(PDataObject self, DWORD connection)
{
	return E_FAIL;
}

static HRESULT __stdcall
DataObject__EnumDAdvise(PDataObject self, LPENUMSTATDATA *enum_advise)
{
	return E_FAIL;
}

static IDataObjectVtbl DataObjectVMT = {
	(void*) OLEUnknown__QueryInterface,
	(void*) OLEUnknown__AddRef,
	(void*) OLEUnknown__Release,
	(void*) DataObject__GetData,
	(void*) DataObject__GetDataHere,
	(void*) DataObject__QueryGetData,
	(void*) DataObject__GetCanonicalFormatEtc,
	(void*) DataObject__SetData,
	(void*) DataObject__EnumFormatEtc,
	(void*) DataObject__DAdvise,
	(void*) DataObject__Dunadvise,
	(void*) DataObject__EnumDAdvise
};

/* clipboard */

static PList
get_formats(void)
{
	DEFCC;
	FORMATETC etc;
	IEnumFORMATETC *e = NULL;

	if (data == NULL) return NULL;
	if (formats != NULL) return formats;

	if (data->lpVtbl->EnumFormatEtc(data, DATADIR_GET, &e) != S_OK) {
		apiErr;
		return NULL;
	}
	if (e == NULL)
		return NULL;
	formats = plist_create(8, 8);
	while ( e->lpVtbl->Next(e, 1, &etc, NULL) == S_OK ) {
		list_add( formats, (Handle)etc.cfFormat);
		list_add( formats, (Handle)etc.tymed);
	}
	e->lpVtbl->Release(e);
	return formats;
}

Bool
dnd_clipboard_create( void)
{
	if (guts.dndDataSender) return false;
	if (!(guts.dndDataSender = CreateObject(DataObject)))
		return false;
	list_create(&((PDataObject) guts.dndDataSender)->list, 8, 8);
	return true;
}

void
dnd_clipboard_destroy( void)
{
	if (!guts.dndDataSender) return;
	dataobject_clear();
	list_destroy(&((PDataObject) guts.dndDataSender)->list);
	free(guts.dndDataSender);
	guts.dndDataSender = NULL;
}

Bool
dnd_clipboard_open( void)
{
	DEFCC;
	if (data == NULL) return false;
	return true;
}

Bool
dnd_clipboard_close( void)
{
	DEFCC;
	if ( formats ) {
		plist_destroy(formats);
		formats = NULL;
	}
	if (data == NULL) return false;
	return true;
}

Bool
dnd_clipboard_clear( void)
{
	DEFCC;
	if ( guts.dndInsideEvent ) return false;
	if (data == NULL) return false;
	dataobject_clear();
	return true;
}

PList
dnd_clipboard_get_formats( void)
{
	int i;
	PList cf_formats, ret;

	if (( cf_formats = get_formats()) == NULL )
		return NULL;
	ret = plist_create(8, 8);
	for ( i = 0; i < cf_formats->count; i+=2) {
		char buf[256];
		char * name = cf2name((UINT) cf_formats->items[i]);
		if ( name )
			snprintf(buf, 256, "%s", name);
		else
			snprintf(buf, 256, "0x%x", (unsigned int)cf_formats->items[i]);
		switch (cf_formats->items[i+1]) {
		case TYMED_HGLOBAL  : break;
		case TYMED_FILE	    : strncat(buf, ".FILE"    ,255); break;
		case TYMED_ISTREAM  : strncat(buf, ".ISTREAM" ,255); break;
		case TYMED_ISTORAGE : strncat(buf, ".ISTORAGE",255); break;
		case TYMED_GDI	    : strncat(buf, ".GDI"     ,255); break;
		case TYMED_MFPICT   : strncat(buf, ".MFPICT"  ,255); break;
		case TYMED_ENHMF    : strncat(buf, ".ENHMF"   ,255); break;
		default             : {
			char num[16];
			snprintf(num, 16, ".0x%x", (unsigned int) cf_formats->items[i+1]);
			strncat(buf, num, 255);
		}}
		buf[255] = 0;
		list_add( ret, (Handle) duplicate_string(buf));
	}

	return ret;
}

Bool
dnd_clipboard_get_data( Handle id, PClipboardDataRec c)
{
	DEFCC;
	Bool ret;
	HPALETTE palette = NULL;
	STGMEDIUM medium, medium2;
	FORMATETC etc = { id, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	if (data == NULL)
		return false;

	/* copy cached data directly */
	if ( !guts. dndInsideEvent ) {
		PDataObjectEntry entry;
		if (( dataobject_find_entry((PDataObject) data, id, &entry)) < 0)
			return false;

		if ( id == CF_BITMAP ) {
			c->image = CImage(entry->image)->dup((Handle)(entry->image));
		} else {
			c->length = 0;
			if (( c->data = malloc(entry->size )) == NULL)
				return false;
			memcpy( c->data, entry->data, c->length = entry->size );
		}
		return true;
	}

	/* read from OLE */
	if ( id == CF_BITMAP ) {
		/* HBITMAP needs palette, which doesn't seem to be served through DND - check DIB first */
		etc.cfFormat = CF_DIB;
		if (
			(data->lpVtbl->QueryGetData(data, &etc) == S_OK) &&
			(data->lpVtbl->GetData(data, &etc, &medium) == S_OK)
		) {
			id = CF_DIB;
		} else {
			etc.cfFormat = CF_BITMAP;
			etc.tymed    = TYMED_GDI;
			if (
				(data->lpVtbl->QueryGetData(data, &etc) == S_OK) &&
				(data->lpVtbl->GetData(data, &etc, &medium) == S_OK)
			) {
				id = CF_BITMAP;
				etc.cfFormat = CF_PALETTE;
				if (
					(data->lpVtbl->QueryGetData(data, &etc) == S_OK) &&
					(data->lpVtbl->GetData(data, &etc, &medium2) == S_OK)
				) {
					palette = medium2.hGlobal;
				}
			} else
				return false;
		}
	} else {
		if (data->lpVtbl->QueryGetData(data, &etc) != S_OK)
			return false;
		if (( rc = data->lpVtbl->GetData(data, &etc, &medium)) != S_OK) {
			apcWarn;
			return false;
		}
	}

	ret = clipboard_get_data(id, c, (void*) medium.hGlobal, (void*)palette);

	if ( palette )
		ReleaseStgMedium(&medium2);
	ReleaseStgMedium(&medium);

	return ret;
}

Bool
dnd_clipboard_has_format( Handle id)
{
	DEFCC;
	int i;
	PList cf_formats;

	if ( !guts. dndInsideEvent ) {
		if ( !data ) 
			return false;
		if (( dataobject_find_entry((PDataObject)data, id, NULL)) < 0)
			return false;
		return true;
	}

	if (( cf_formats = get_formats()) == NULL )
		return false;
	for ( i = 0; i < cf_formats->count; i++) {
		if ( id == cf_formats->items[i])
			return true;
	}

	return false;
}

Bool
dnd_clipboard_set_data( Handle id, PClipboardDataRec c)
{
	int index;
	PDataObject data = (PDataObject) guts.dndDataSender;
	PDataObjectEntry entry;

	if ( guts. dndInsideEvent )
		return false;
	if (data == NULL)
		return false;

	if ((index = dataobject_find_entry((PDataObject)data, id, &entry)) >= 0) {
		dataobject_free_entry(entry);
		data->list.items[index] = nilHandle;
	}

	entry = ( id == CF_BITMAP ) ?
		dataobject_alloc_image(c->image) :
		dataobject_alloc_buffer(id, c->length, c->data);
	if ( entry == NULL ) {
		warn("Not enough memory");
		return false;
	}

	if ( index >= 0 )
		data->list.items[index] = (Handle) entry;
	else
		list_add(&data->list, (Handle) entry);

	return true;
}

/* Widgets */

Bool
apc_dnd_get_aware( Handle self )
{
	return sys dropTarget != NULL;
}

Bool
apc_dnd_set_aware( Handle self, Bool is_target )
{
	if ( is_target == (sys dropTarget != NULL )) return true;

	if ( is_target ) {
		PDropTarget target;
		if (!(target = CreateObject(DropTarget)))
			return false;
		target->widget = self;
		if ( RegisterDragDrop( sys handle, (IDropTarget*) target) != S_OK) {
			apiErr;
			free(target);
			return false;
		}
		sys dropTarget = target;
	} else {
		RevokeDragDrop(sys handle);
		free(sys dropTarget);
		sys dropTarget = NULL;
	}

	return true;
}

Handle
apc_dnd_get_clipboard( Handle self )
{
	return guts.clipboards[CLIPBOARD_DND];
}

int
apc_dnd_start( Handle self, int actions)
{
	int ret = dndNone;
	DWORD effect;
	actions &= dndMask;

	if ( actions == 0 )
		return -1;
	if ( guts.dragSource )
		return -1;
	if (!(guts.dragSource = CreateObject(DropSource)))
		return -1;
	((PDropSource)guts.dragSource)->widget       = self;
	((PDropSource)guts.dragSource)->first_modmap =
		apc_kbd_get_state(self) | apc_pointer_get_state(self);
	((PDropSource)guts.dragSource)->last_action = -1;

	rc = DoDragDrop(
		(LPDATAOBJECT) guts.dndDataSender,
		(LPDROPSOURCE) guts.dragSource,
		actions,
		&effect
	);

	switch (rc) {
	case DRAGDROP_S_DROP:
		ret = dndNone;
		break;
	case DRAGDROP_S_CANCEL:
		ret = effect & dndMask;
		break;
	default:
		ret = -1;
		apcWarn;
	}

	free(guts.dragSource);
	guts.dragSource    = NULL;

	return ret;
}

#ifdef __cplusplus
}
#endif
