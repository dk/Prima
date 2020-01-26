#include "win32\win32guts.h"
#include <ole2.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

#define DEFCC IDataObject* data = (IDataObject*) ( guts.dndInsideEvent ? guts.dndDataReceiver : guts.dndDataSender );

static PList formats = NULL;

/*
	Classes
*/

/* IUnknown */
typedef struct {
	IUnknownVtbl* lpVtbl;
	GUID *guid;
	ULONG refcnt;
} OLEUnknown, *POLEUnknown;

static void*
create_ole_object(int size, void * vmt)
{
	void * object;
	if ( !( object = malloc(size)))
		return false;
	bzero(object,size);
	((POLEUnknown)object)->refcnt = 1;
	((POLEUnknown)object)->lpVtbl = (void*)vmt;
	return object;
}

#define CreateObject(type) create_ole_object(sizeof(type),&type ## VMT)

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
	return S_OK;
}

static HRESULT __stdcall
DropSource__GiveFeedback(PDropSource self, DWORD effect)
{
	Event ev = { cmDragResponse };
	ev.dnd.action = effect & dndMask;
	ev.dnd.allow  = ev.dnd.action != dndNone;
	CWidget(self->widget)->message(self->widget, &ev);
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
	List   data;
} DataObject, *PDataObject;

static HRESULT __stdcall
DataObject__GetData(PDataObject self, LPFORMATETC format, LPSTGMEDIUM medium)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__GetDataHere(PDataObject self, LPFORMATETC format, LPSTGMEDIUM medium)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__QueryGetData(PDataObject self, LPFORMATETC format)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__GetCanonicalFormatEtc(PDataObject self, LPFORMATETC format_in, LPFORMATETC format_out)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__SetData(PDataObject self, LPFORMATETC format, LPSTGMEDIUM medium, BOOL release)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__EnumFormatEtc(PDataObject self, DWORD direction, LPENUMFORMATETC *enum_formats)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__DAdvise(PDataObject self, LPFORMATETC format, DWORD advf, LPADVISESINK sink, DWORD *connection)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__Dunadvise(PDataObject self, DWORD connection)
{
	return S_OK;
}

static HRESULT __stdcall
DataObject__EnumDAdvise(PDataObject self, LPENUMSTATDATA *enum_advise)
{
	return S_OK;
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

Bool
dnd_clipboard_open( Handle self)
{
	DEFCC;
	if (data == NULL) return false;
	return true;
}

Bool
dnd_clipboard_close( Handle self)
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
dnd_clipboard_clear( Handle self)
{
	DEFCC;
	if ( guts.dndInsideEvent ) return false;
	if (data == NULL) return false;
	/* XXX */
	return true;
}

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
	}
	e->lpVtbl->Release(e);
	return formats;
}

PList
dnd_clipboard_get_formats( Handle self)
{
	int i;
	PList cf_formats, ret;

	if (( cf_formats = get_formats()) == NULL )
		return NULL;
	ret = plist_create(8, 8);
	for ( i = 0; i < cf_formats->count; i++) {
		char * name = cf2name((UINT) cf_formats->items[i]);
		if ( name )
			list_add(ret, (Handle) duplicate_string(name));
	}

	return ret;
}

Bool
dnd_clipboard_get_data( Handle self, Handle id, PClipboardDataRec c)
{
	DEFCC;
	Bool ret;
	HPALETTE palette = NULL;
	STGMEDIUM medium, medium2;
	FORMATETC etc = { id, NULL, DVASPECT_CONTENT, -1, ( id == CF_BITMAP ) ? TYMED_GDI : TYMED_HGLOBAL };

	if (data == NULL)
		return false;
	if (data->lpVtbl->QueryGetData(data, &etc) != S_OK)
		return false;
	if (( rc = data->lpVtbl->GetData(data, &etc, &medium)) != S_OK) {
		apcWarn;
		return false;
	}

	if ( id == CF_BITMAP ) {
		etc.cfFormat = CF_PALETTE;
		if (
			(data->lpVtbl->QueryGetData(data, &etc) == S_OK) &&
			(data->lpVtbl->GetData(data, &etc, &medium2) == S_OK)
		) {
			palette = medium2.hGlobal;
		}
	}

	ret = clipboard_get_data(id, c, (void*) medium.hGlobal, (void*)palette);

	if ( palette )
		ReleaseStgMedium(&medium2);
	ReleaseStgMedium(&medium);

	return ret;
}

Bool
dnd_clipboard_has_format( Handle self, Handle id)
{
	int i;
	PList cf_formats;

	if (( cf_formats = get_formats()) == NULL )
		return false;
	for ( i = 0; i < cf_formats->count; i++) {
		if ( id == cf_formats->items[i])
			return true;
	}

	return false;
}

Bool
dnd_clipboard_set_data( Handle self, Handle id, PClipboardDataRec c)
{
	return false;
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
	if (!(guts.dndDataSender = CreateObject(DataObject)))
		return -1;
	if (!(guts.dragSource = CreateObject(DropSource))) {
		free(guts.dndDataSender);
		guts.dndDataSender = NULL;
		return -1;
	}
	((PDropSource)guts.dragSource)->widget       = self;
	((PDropSource)guts.dragSource)->first_modmap = 
		apc_kbd_get_state() | apc_pointer_get_state();

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

	free(guts.dndDataSender);
	free(guts.dragSource);
	guts.dndDataSender = NULL;
	guts.dragSource    = NULL;

	return ret;
}

#ifdef __cplusplus
}
#endif
