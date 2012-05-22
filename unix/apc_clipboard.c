/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "unix/guts.h"
#include "Application.h"
#include "Clipboard.h"
#include "Icon.h"

#define WIN PComponent(application)-> handle

#define CF_NAME(x)   (guts. clipboard_formats[(x)*3])
#define CF_TYPE(x)   (guts. clipboard_formats[(x)*3+1])
#define CF_FORMAT(x) (guts. clipboard_formats[(x)*3+2])
#define CF_ASSIGN(i,a,b,c) CF_NAME(i)=(a);CF_TYPE(i)=(b);CF_FORMAT(i)=((Atom)c)
#define CF_32        (sizeof(long)*8)        /* 32-bit properties are hacky */

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
   PList l = plist_create( 3, 1);
   if (!l) return nil;
   list_add( l, (Handle)duplicate_string( "Primary"));
   list_add( l, (Handle)duplicate_string( "Secondary"));
   list_add( l, (Handle)duplicate_string( "Clipboard"));
   return l;
}

Bool
apc_clipboard_create( Handle self)
{
   PClipboard c = (PClipboard)self;
   char *name, *x;
   DEFCC;

   XX-> selection = None;
   
   name = x = duplicate_string( c-> name);
   while (*x) {
      *x = toupper(*x);
      x++;
   }
   XX-> selection = XInternAtom( DISP, name, false);
   free( name);

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

   hash_store( guts.clipboards, &XX->selection, sizeof(XX->selection), (void*)self);

   return true;
}

static void
clipboard_free_data( void * data, int size, Handle id)
{
   if ( size <= 0) {
      if ( size == 0 && data != nil) free( data);
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
   case cfUTF8:
      if ( index > 1) return None;
      if ( index == 0) {
         if ( type) *type = CF_TYPE(id);
         return CF_NAME(id);
      } else {
         if ( type) *type = UTF8_MIME;
         return UTF8_MIME;
      }
   case cfBitmap:
      if ( index > 1) return None;
      if ( index == 0) {
         if ( type) *type = CF_TYPE(id);
	 return CF_NAME(id);
      } else {
         if ( type) *type = XA_BITMAP;
	 return XA_BITMAP;
      }
   case cfTargets:
      if ( index > 1) return None;
      if ( index == 0) {
         if ( type) *type = CF_TYPE(id);
         return CF_NAME(id);
      } else {
         if ( type) *type = CF_TARGETS;
         return CF_NAME(id);
      }
   }
   if ( index > 0) return None;
   if ( type) *type = CF_TYPE(id);
   return CF_NAME(id);
}

static void
clipboard_kill_item( PClipboardDataItem item, Handle id)
{
   item += id;
   clipboard_free_data( item-> data, item-> size, id);
   item-> data = nil;
   item-> size = 0;
   item-> name = get_typename( id, 0, nil);
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

   if (XX-> selection == None) return true;

   if ( XX-> xfers) {
      for ( i = 0; i < XX-> xfers-> count; i++) 
         delete_xfer( XX, ( ClipboardXfer*) XX-> xfers-> items[i]);
      plist_destroy( XX-> xfers);
   }

   for ( i = 0; i < guts. clipboard_formats_count; i++) {
      if ( XX-> external) clipboard_kill_item( XX-> external, i);
      if ( XX-> internal) clipboard_kill_item( XX-> internal, i);
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
   if ( XX-> opened) return false;
   XX-> opened = true;
   
   if ( !XX-> inside_event) XX-> need_write = false;

   return true;
}

Bool
apc_clipboard_close( Handle self)
{
   DEFCC;
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
	  U8 *dst;
	  dst = XX-> internal[cfText]. data;
          XX-> internal[cfText]. size = len;
	  while ( len--) {
             register UV u = 
#if PERL_PATCHLEVEL >= 16
	     utf8_to_uvchr_buf( src, src + XX-> internal[cfUTF8]. size, &charlen);
#else
	     utf8_to_uvchr( src, &charlen)
#endif
	     ;
	     *(dst++) = ( u < 0x7f) ? u : '?'; /* XXX employ $LANG and iconv() */
	     src += charlen;
	  }
      } 
   }
      

   if ( !XX-> inside_event) {
      int i;         
      for ( i = 0; i < guts. clipboard_formats_count; i++) 
         clipboard_kill_item( XX-> external, i);
      if ( XX-> need_write) 
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
static void
detach_xfers( PClipboardSysData XX, Handle id, Bool clear_original_data)
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
      XX-> internal[id]. data = nil;
      XX-> internal[id]. size = 0;
      XX-> internal[id]. name = get_typename( id, 0, nil);
   }
}

Bool
apc_clipboard_clear( Handle self)
{
   DEFCC;
   int i;

   for ( i = 0; i < guts. clipboard_formats_count; i++) {
      detach_xfers( XX, i, true);
      clipboard_kill_item( XX-> internal, i);
      clipboard_kill_item( XX-> external, i);
   }
   
   if ( XX-> inside_event) { 
     XX-> need_write = true; 
   } else {
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

#define RPS_OK       0
#define RPS_PARTIAL  1
#define RPS_NODATA   2
#define RPS_ERROR    3

static int
read_property( Atom property, Atom * type, int * format, 
               unsigned long * size, unsigned char ** data)
{
   int ret = ( *size > 0) ? RPS_PARTIAL : RPS_ERROR;
   unsigned char * prop, *a1;
   unsigned long n, left, offs = 0, new_size, big_offs = *size;

   XCHECKPOINT;
   Cdebug("clipboard: read_property: %s\n", XGetAtomName(DISP, property));
   while ( 1) {
      if ( XGetWindowProperty( DISP, WIN, property,
          offs, guts. limits. request_length - 4, false, 
          AnyPropertyType, 
          type, format, &n, &left, &prop) != Success) {
         XDeleteProperty( DISP, WIN, property);
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
            XDeleteProperty( DISP, WIN, property);
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

   XDeleteProperty( DISP, WIN, property);
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
   unsigned char * data;
   struct timeval start_time, timeout;
 
   /* init */
   if ( query_target == None) return false;
   data = malloc(0);
   XX-> external[id]. size = CFDATA_ERROR;
   gettimeofday( &start_time, nil);
   XCHECKPOINT;
   Cdebug("clipboard:convert %s from %08x\n", XGetAtomName( DISP, query_target), WIN);
   XDeleteProperty( DISP, WIN, XX-> selection);
   XConvertSelection( DISP, XX-> selection, query_target, XX-> selection, WIN, guts. last_time);
   XFlush( DISP);
   XCHECKPOINT;

   /* wait for SelectionNotify */
   spd. selection = XX-> selection;
   spd. mask = SELECTION_NOTIFY_MASK;
   while ( 1) {
      XIfEvent( DISP, &ev, (XIfEventProcType)selection_filter, (char*)&spd);
      if ( ev. type != SelectionNotify) {
         prima_handle_event( &ev, nil);
         continue;
      }
      if ( ev. xselection. property == None) goto FAIL;
      Cdebug("clipboard:read SelectionNotify  %s %s\n",
             XGetAtomName(DISP, ev. xselection. property),
             XGetAtomName(DISP, ev. xselection. target));
      gettimeofday( &timeout, nil);
      delay = 2 * (( timeout. tv_sec - start_time. tv_sec) * 1000 + 
                   ( timeout. tv_usec - start_time. tv_usec) / 1000) + guts. clipboard_event_timeout;
      start_time = timeout;
      if ( read_property( ev. xselection. property, &type, &format, &size, &data) > RPS_PARTIAL) 
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
         gettimeofday( &timeout, nil);
         if ((( timeout. tv_sec - start_time. tv_sec) * 1000 + 
              ( timeout. tv_usec - start_time. tv_usec) / 1000) > delay) 
              goto END_LOOP;
      }
      if ( ev. type != PropertyNotify) {
         prima_handle_event( &ev, nil);
         continue;
      }
      if ( ev. xproperty. state != PropertyNewValue) continue;
      start_time = timeout;
      old_size = size;

      rps = read_property( ev. xproperty. atom, &type, &format, &size, &data);
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
   Atom name, type;
   int index = 0;
   while (( name = get_typename( id, index++, &type)) != None) {
      if ( query_datum( self, id, name, type)) return true;
   }
   return false;
}

static Atom
find_atoms( Atom * data, int length, int id)
{
   int i, index = 0;
   Atom name;
   
   while (( name = get_typename( id, index++, nil)) != None) {
      for ( i = 0; i < length / sizeof(Atom); i++) {
         if ( data[i] == name) 
            return name;
      }
   }
   return None;
}


Bool
apc_clipboard_has_format( Handle self, Handle id)
{
   DEFCC;
   if ( id < 0 || id >= guts. clipboard_formats_count) return false;

   if ( XX-> inside_event) {
      return XX-> internal[id]. size > 0 || XX-> external[id]. size > 0;
   } else {
      if ( XX-> internal[id]. size > 0) return true;

      if ( XX-> external[cfTargets]. size == 0) {
         /* read TARGETS, which as array of ATOMs */
         query_data( self, cfTargets);

         if ( XX-> external[cfTargets].size > 0) {
            int i, size = XX-> external[cfTargets].size;
            Atom * data = ( Atom*)(XX-> external[cfTargets]. data);
            Atom ret;

            
            Cdebug("clipboard targets:");
            for ( i = 0; i < size/4; i++) 
               Cdebug("%s\n", XGetAtomName( DISP, data[i]));

            /* find our index for TARGETS[i], assign CFDATA_NOT_ACQUIRED to it */
            for ( i = 0; i < guts. clipboard_formats_count; i++) {
               if ( i == cfTargets) continue;
               ret = find_atoms( data, size, i);
               if ( ret != None && (
                      XX-> external[i]. size == 0 ||
                      XX-> external[i]. size == CFDATA_ERROR
                    )
                  ) { 
                   XX-> external[i]. size = CFDATA_NOT_ACQUIRED;
                   XX-> external[i]. name = ret;
                }
            }

            if ( XX-> external[id]. size == 0 || 
                 XX-> external[id]. size == CFDATA_ERROR)
               return false;
         }
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

Bool
apc_clipboard_get_data( Handle self, Handle id, PClipboardDataRec c)
{
   DEFCC;
   STRLEN size;
   unsigned char * data;
   Atom name;

   if ( id < 0 || id >= guts. clipboard_formats_count) return false;

   if ( !XX-> inside_event) {
      if ( XX-> internal[id]. size == 0) {
         if ( XX-> external[id]. size == CFDATA_NOT_ACQUIRED) {
            if ( !query_data( self, id)) return false;
         }
         if ( XX-> external[id]. size == CFDATA_ERROR) return false;
      }
   }
   if ( XX-> internal[id]. size == CFDATA_ERROR) return false;

   if ( XX-> internal[id]. size > 0) {
      size = XX-> internal[id]. size;
      data = XX-> internal[id]. data;
      name = XX-> internal[id]. name;
   } else {
      size = XX-> external[id]. size;
      data = XX-> external[id]. data;
      name = XX-> external[id]. name;
   }
   if ( size == 0 || data == nil) return false;

   switch ( id) {
   case cfBitmap: {
      Handle img = c-> image; 
      XWindow foo;
      Pixmap px = *(( Pixmap*)( data));
      unsigned int dummy, x, y, d;
      int bar;
     
      if ( !XGetGeometry( DISP, px, &foo, &bar, &bar, &x, &y, &dummy, &d))
         return false;
      CImage( img)-> create_empty( img, x, y, ( d == 1) ? imBW : guts. qdepth);
      if ( !prima_std_query_image( img, px)) return false;
      break;}
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
   if ( id < 0 || id >= guts. clipboard_formats_count) return false;

   if ( id >= cfTargets && id < cfCOUNT ) return false;
   detach_xfers( XX, id, true);
   clipboard_kill_item( XX-> internal, id);

   switch ( id) {
   case cfBitmap: {  
      Pixmap px = prima_std_pixmap( c-> image, CACHE_LOW_RES);
      if ( px) {
         if ( !( XX-> internal[cfBitmap]. data = malloc( sizeof( px)))) {
            XFreePixmap( DISP, px);
            return false;
         }
         XX-> internal[cfBitmap]. size = sizeof(px);
         memcpy( XX-> internal[cfBitmap]. data, &px, sizeof(px));
      } else
         return false;
      break;}
   default:
      if ( !( XX-> internal[id]. data = malloc( c-> length))) 
         return false;
      XX-> internal[id]. size = c-> length;
      memcpy( XX-> internal[id]. data, c-> data, c-> length);
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
   f[ guts. clipboard_formats_count-1].size = 0;
   f[ guts. clipboard_formats_count-1].data = nil;
   f[ guts. clipboard_formats_count-1].name = CF_NAME(guts. clipboard_formats_count-1);
   XX-> internal = f;
   if ( !( f = realloc( XX-> external, 
      sizeof( ClipboardDataItem) * guts. clipboard_formats_count))) {
      guts. clipboard_formats_count--;
      return true;
   }
   f[ guts. clipboard_formats_count-1].size = 0;
   f[ guts. clipboard_formats_count-1].data = nil;
   f[ guts. clipboard_formats_count-1].name = CF_NAME(guts. clipboard_formats_count-1);
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

   if ( hash_first_that( guts. clipboards, (void*)expand_clipboards, nil, nil, nil))
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

void
prima_handle_selection_event( XEvent *ev, XWindow win, Handle self)
{
   XCHECKPOINT;
   switch ( ev-> type) {
   case SelectionRequest: {
      XEvent xe;
      int i, id = -1;
      Atom prop   = ev-> xselectionrequest. property,
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
      
      Cdebug("from %08x %s at %s\n", ev-> xselectionrequest. requestor, 
             XGetAtomName( DISP, ev-> xselectionrequest. target),
             XGetAtomName( DISP, ev-> xselectionrequest. property)
             );

      if ( self) { 
         PClipboardSysData CC = C(self);
         Bool event = CC-> inside_event;
         int format, utf8_mime = 0;

         for ( i = 0; i < guts. clipboard_formats_count; i++) {
            if ( xe. xselection. target == CC-> internal[i]. name) {
               id = i;
               break;
            } else if ( i == cfUTF8 && xe. xselection. target == UTF8_MIME) {
               id = i;
               utf8_mime = 1;
               break;
            }
         }
         if ( id < 0) goto SEND_EMPTY;
         for ( i = 0; i < guts. clipboard_formats_count; i++)
            clipboard_kill_item( CC-> external, i);
         
         CC-> target = xe. xselection. target;
         CC-> need_write = false;
         
         CC-> inside_event = true;
         /* XXX cmSelection */
         CC-> inside_event = event;

         format = CF_FORMAT(id);
         target = CF_TYPE( id);
         if ( utf8_mime) target = UTF8_MIME;

         if ( id == cfTargets) { 
            int count = 0, have_utf8 = 0;
            Atom * ci;
            for ( i = 0; i < guts. clipboard_formats_count; i++) {
               if ( i != cfTargets && CC-> internal[i]. size > 0) {
                  count++;
		  if ( i == cfUTF8) {
		     count++;
		     have_utf8 = 1;
		  }
	       }
	    }
            detach_xfers( CC, cfTargets, true);
            clipboard_kill_item( CC-> internal, cfTargets);
            if (( CC-> internal[cfTargets]. data = malloc( count * sizeof( Atom)))) {
               CC-> internal[cfTargets]. size = count * sizeof( Atom);
               ci = (Atom*)CC-> internal[cfTargets]. data;
               for ( i = 0; i < guts. clipboard_formats_count; i++) 
                  if ( i != cfTargets && CC-> internal[i]. size > 0) 
                     *(ci++) = CF_NAME(i);
               if ( have_utf8) 
		  *(ci++) = UTF8_MIME;
            }
         }
        
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
                     gettimeofday( &x-> time, nil);

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
                     Cdebug("clpboard: init INCR for %08x %d\n", x-> requestor, x-> property);
                  }
               }
               if ( !ok) size = reqlen;
            }

            if ( format == CF_32) format = 32;
            XChangeProperty( 
               xe. xselection. display,
               xe. xselection. requestor,
               prop, target, format, mode, data, size);
            Cdebug("clipboard: store prop %s\n", XGetAtomName( DISP, prop));
            xe. xselection. property = prop;
         }

         /* content of PIXMAP or BITMAP is seemingly gets invalidated
            after the selection transfer, unlike the string data format */
         if ( id == cfBitmap) {
            bzero( CC-> internal[id].data, CC-> internal[id].size);
            bzero( CC-> external[id].data, CC-> external[id].size);
            clipboard_kill_item( CC-> internal, id);
            clipboard_kill_item( CC-> external, id);
         }
      }
SEND_EMPTY:
      XSendEvent( xe.xselection.display, xe.xselection.requestor, false, 0, &xe);
      XFlush( DISP);
      Cdebug("clipboard:id %d, SelectionNotify to %08x , %s %s\n", id, xe.xselection.requestor, 
         XGetAtomName( DISP, xe. xselection. property),
         XGetAtomName( DISP, xe. xselection. target)); 
   } break;
   case SelectionClear: 
      guts. last_time = ev-> xselectionclear. time;
      if ( XGetSelectionOwner( DISP, ev-> xselectionclear. selection) != WIN) {
         Handle c = ( Handle) hash_fetch( guts. clipboards, 
                                          &ev-> xselectionclear. selection, sizeof( Atom)); 
         guts. last_time = ev-> xselectionclear. time;
         if (c) {
            int i;
            C(c)-> selection_owner = nilHandle;  
            for ( i = 0; i < guts. clipboard_formats_count; i++) {
               detach_xfers( C(c), i, true);
               clipboard_kill_item( C(c)-> external, i);
               clipboard_kill_item( C(c)-> internal, i);
            }
         }
      }   
      break;
   case PropertyNotify:
      if ( ev-> xproperty. state == PropertyDelete) {
         unsigned long offs, size, reqlen = guts. limits. request_length - 4, format;
         ClipboardXfer * x = ( ClipboardXfer *) self;
         PClipboardSysData CC = C(x-> self);
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
      break;
   case DestroyNotify:
      Cdebug("clipboard: destroy xfers at %08x\n", ev-> xdestroywindow. window);
      hash_first_that( guts. clipboards, (void*)delete_xfers, (void*) &ev-> xdestroywindow. window, nil, nil);
      XFlush( DISP);
      break;
   }
   XCHECKPOINT;
}

