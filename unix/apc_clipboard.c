/*-
 * Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
 */

#include "unix/guts.h"
#include "Application.h"
#include "Clipboard.h"
#include "Icon.h"

#define WIN PComponent(application)-> handle
   
Bool
prima_init_clipboard_subsystem(void)
{
   guts. clipboards = hash_create();
   
   if ( !(guts. clipboard_formats = malloc(
      (guts. clipboard_formats_count = 4) * sizeof(Atom)
   ))) return false;
#if (cfText != 0) || (cfBitmap != 1)
#error broken clipboard type formats
#endif   
   guts. clipboard_formats[cfText]    = XA_STRING;
   guts. clipboard_formats[cfBitmap]  = XA_BITMAP;
   guts. clipboard_formats[cfPixmap]  = XA_PIXMAP;
   guts. clipboard_formats[cfTargets] = XInternAtom( DISP, "TARGETS", false);
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
   while (*x) *(x++) = toupper(*x);
   XX-> selection = XInternAtom( DISP, name, false);
   free( name);

   if ( hash_fetch( guts.clipboards, &XX->selection, sizeof(XX->selection))) {
      warn("This clipboard is already present");
      return false;
   }

   if ( !( XX-> internal = malloc( sizeof( ClipboardDataItem) * 4))) {
      warn("Not enough memory");
      return false;
   }
   if ( !( XX-> external = malloc( sizeof( ClipboardDataItem) * 4))) {
      free( XX-> internal);
      warn("Not enough memory");
      return false;
   }
   bzero( XX-> internal, sizeof( ClipboardDataItem) * 4);
   bzero( XX-> external, sizeof( ClipboardDataItem) * 4);

   hash_store( guts.clipboards, &XX->selection, sizeof(XX->selection), (void*)self);

   return true;
}

static void
clipboard_kill_item( PClipboardDataItem item, long id)
{
   item += id;
   if ( item-> size > 0) {
      if ( id == cfBitmap || id == cfPixmap) {
         int i;
         Pixmap * p = (Pixmap*) item-> data;
         for ( i = 0; i < item-> size/sizeof(Pixmap); i++, p++)
            XFreePixmap( DISP, *p);
      }
      free( item-> data);
   }
   item-> data = nil;
   item-> size = 0;
}

Bool
apc_clipboard_destroy( Handle self)
{
   DEFCC;
   int i;

   if (XX-> selection == None) return true;

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

Bool
apc_clipboard_clear( Handle self)
{
   DEFCC;
   int i;
   
   for ( i = 0; i < guts. clipboard_formats_count; i++) {
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


static Bool
selection_filter( Display * disp, XEvent * ev, PClipboardSysData XX)
{
   switch ( ev-> type) {
   case SelectionRequest:
   case SelectionClear:
   case MappingNotify:
      return true;
   case SelectionNotify:
      return XX-> selection == ev-> xselection. selection;
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

static Bool
query_data( Handle self, long id, Atom * ret_type) 
{
   DEFCC;
   XEvent ev;
   XConvertSelection( DISP, XX-> selection, guts. clipboard_formats[id], 
      XX-> selection, WIN, CurrentTime);
   while ( 1) {
      XIfEvent( DISP, &ev, (void*)selection_filter, (char*)XX);

      if ( ev. type == SelectionNotify) {
         Atom type;
         int format;
         unsigned long n, left, offs = 0, size = 0;
         unsigned char * data, * prop, *a1;
         XX-> external[id]. size = CFDATA_ERROR;
         if ( ev. xselection. property == None) return false;
         data = malloc(0);
         
         while ( 1) {
            if ( XGetWindowProperty( DISP, WIN, ev. xselection. property,
                offs, guts. limits. request_length - 4, false, 
                AnyPropertyType, 
                &type, &format, &n, &left, &prop) == Success) {
               format /= 8;
               if ( ret_type) *ret_type = type;
               
               switch ( id) {
               case cfText:
                  /* XXX check for big transfers */
                  if ( type != XA_STRING) goto FAIL;
                  break;
               case cfBitmap:
               case cfPixmap:
                  if ( type != XA_BITMAP && type != XA_PIXMAP) goto FAIL;
                  break;
               case cfTargets:
                  if ( type != XA_ATOM) goto FAIL;
                  break;
               }
               if ( !( a1 = realloc( data, offs * 4 + n * format))) {
                  warn("Not enough memory: %d bytes\n", offs * 4 + n * format);
               FAIL:
                  XDeleteProperty( DISP, WIN, ev. xselection. property);
                  XFree( prop);
                  free( data);
                  return false;
               }
               data = a1;
               memcpy( data + offs * 4, prop, n * format);
               size = (offs * 4) + n * format;
               if ( size > INT_MAX) size = INT_MAX;
               offs += (n * format) / 4;
               XFree( prop);
               if ( left <= 0 || size == INT_MAX || n * format == 0) break;
            } else {
               XDeleteProperty( DISP, WIN, ev. xselection. property);
               free( data);
               return false;
            }
         }
         XDeleteProperty( DISP, WIN, ev. xselection. property);
         XX-> external[id]. size = size;
         XX-> external[id]. data = data;
         break;
      } else 
         prima_handle_event( &ev, nil);
   }
   return true;
}

Bool
apc_clipboard_has_format( Handle self, long id)
{
   DEFCC;
   if ( id < 0 || id >= guts. clipboard_formats_count) return false;

   if ( id == cfBitmap && apc_clipboard_has_format( self, cfPixmap))
      return true;
   
   if ( XX-> inside_event) {
      return XX-> internal[id]. size > 0 || XX-> external[id]. size > 0;
   } else {
      if ( XX-> internal[id]. size > 0) return true;

      if ( XX-> external[cfTargets]. size == 0) {
         Atom ret;
         // read TARGETS, which as array of ATOMs
         query_data( self, cfTargets, &ret);

         if ( XX-> external[cfTargets].size > 0) {
            int i;
            Atom * x = (Atom*)(XX-> external[cfTargets].data);

            // find our index for TARGETS[i], assign CFDATA_NOT_ACQUIRED to it
            for ( i = 0; i < XX-> external[cfTargets].size / sizeof(Atom); i++,x++) { 
               int j, k = -1;
               for ( j = 0; j < guts. clipboard_formats_count; j++) {
                  if ( guts. clipboard_formats[ j] == *x) {
                     k = j;
                     break;
                  }
               }
               if ( k >= 0 && 
                 (XX-> external[k]. size == 0 ||
                  XX-> external[k]. size == CFDATA_ERROR))
                  XX-> external[k]. size = CFDATA_NOT_ACQUIRED;
            }
         }
      }
      
      if ( XX-> external[id]. size > 0 || 
           XX-> external[id]. size == CFDATA_NOT_ACQUIRED)
         return true;

      if ( XX-> external[id]. size == CFDATA_ERROR) 
         return false;
      if ( XX-> external[id]. size == 0 && XX-> internal[id]. size == 0) 
         return query_data( self, id, nil);
   }
   return false;
}

void *
apc_clipboard_get_data( Handle self, long id, int *length)
{
   DEFCC;
   unsigned long size;
   unsigned char * data;
   void * ret;

   if ( id < 0 || id >= guts. clipboard_formats_count) return nil;

   if ( id == cfBitmap) {
      ret = apc_clipboard_get_data( self, cfPixmap, length);
      if ( ret) return ret;
   }

   if ( !XX-> inside_event) {
      if ( XX-> internal[id]. size == 0) {
         if ( XX-> external[id]. size == CFDATA_NOT_ACQUIRED) {
            if ( !query_data( self, id, nil)) return nil;
         }
         if ( XX-> external[id]. size == CFDATA_ERROR) return nil;
      }
   }
   if ( XX-> internal[id]. size == CFDATA_ERROR) return nil;

   if ( XX-> internal[id]. size > 0) {
      size = XX-> internal[id]. size;
      data = XX-> internal[id]. data;
   } else {
      size = XX-> external[id]. size;
      data = XX-> external[id]. data;
   }
   if ( size == 0 || data == nil) return nil;
   ret = nil;

   if ( id == cfBitmap || id == cfPixmap) {
      Handle img = *(( Handle*) length);
      XWindow foo;
      Pixmap px = *(( Pixmap*)( data));
      int bar, x, y, d;
      
      if ( !XGetGeometry( DISP, px, &foo, &bar, &bar, &x, &y, &bar, &d))
         return nil;
      CImage( img)-> create_empty( img, x, y, ( d == 1) ? 1 : guts. qdepth);
      if ( !prima_std_query_image( img, px)) return nil;
      ret = (void*)1;
   } else {
      if (( ret = malloc( size))) {
         memcpy( ret, data, size);
         *length = size;
      } else
         warn("Not enough memory: %d bytes\n", size);
   }
   return ret;
}

Bool
apc_clipboard_set_data( Handle self, long id, void * data, int length)
{
   DEFCC;
   if ( id < 0 || id >= guts. clipboard_formats_count) return false;
   if ( id == cfPixmap || id == cfTargets) return false;

   if ( data == nil) {
      apc_clipboard_clear( self);
      return true;
   }

   if ( id == cfBitmap) clipboard_kill_item( XX-> internal, cfPixmap);
   clipboard_kill_item( XX-> internal, id);

   if ( id == cfBitmap) {
      Pixmap px = prima_std_pixmap(( Handle) data, CACHE_LOW_RES);
      if ( px) {
         if ( !( XX-> internal[cfPixmap]. data = malloc( sizeof( px)))) {
            XFreePixmap( DISP, px);
            return false;
         }
         XX-> internal[cfPixmap]. size = sizeof(px);
         memcpy( XX-> internal[cfPixmap]. data, &px, sizeof(px));
      } else
         return false;
   } else {
      if ( !( XX-> internal[id]. data = malloc( length))) 
         return false;
      XX-> internal[id]. size = length;
      memcpy( XX-> internal[id]. data, data, length);
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
   XX-> internal = f;
   if ( !( f = realloc( XX-> external, 
      sizeof( ClipboardDataItem) * guts. clipboard_formats_count))) {
      guts. clipboard_formats_count--;
      return true;
   }
   f[ guts. clipboard_formats_count-1].size = 0;
   f[ guts. clipboard_formats_count-1].data = nil;
   XX-> external = f;
   return false;
}

long
apc_clipboard_register_format( Handle self, const char* format)
{
   int i;
   Atom x = XInternAtom( DISP, format, false);
   Atom *f;

   for ( i = 0; i < guts. clipboard_formats_count; i++) {
      if ( x == guts. clipboard_formats[i]) 
         return i;
   }

   if ( !( f = realloc( guts. clipboard_formats, 
      sizeof( Atom) * ( guts. clipboard_formats_count + 1)))) 
      return false;
   
   guts. clipboard_formats = f;
   guts. clipboard_formats_count++;

   if ( hash_first_that( guts. clipboards, expand_clipboards, nil, nil, nil))
      return -1;

   return guts. clipboard_formats_count - 1;
}

Bool
apc_clipboard_deregister_format( Handle self, long id)
{
   return true;
}

ApiHandle
apc_clipboard_get_handle( Handle self)
{
  return C(self)-> selection;
}

void
prima_handle_selection_event( XEvent *ev, XWindow win, Handle self)
{
   switch ( ev-> type) {
   case SelectionRequest: 
   {
      XEvent xe;
      int i, id = -1;
      Atom prop = ev-> xselectionrequest. property;
      Handle c = ( Handle) hash_fetch( guts. clipboards, &ev-> xselectionrequest. selection, sizeof( Atom)); 

      guts. last_time = ev-> xselectionrequest. time;
      xe. type      = SelectionNotify;
      xe. xselection. serial    = ev-> xselectionrequest. serial;
      xe. xselection. display   = ev-> xselectionrequest. display;
      xe. xselection. requestor = ev-> xselectionrequest. requestor;
      xe. xselection. selection = ev-> xselectionrequest. selection;
      xe. xselection. target    = ev-> xselectionrequest. target;
      xe. xselection. property  = None;
      xe. xselection. time      = ev-> xselectionrequest. time;

      for ( i = 0; i < guts. clipboard_formats_count; i++) {
         if ( i == cfBitmap) {
            if ( xe. xselection. target == XA_PIXMAP ||
                 xe. xselection. target == XA_BITMAP) {
               id = i;
               break;
            }
         } else {
            if ( xe. xselection. target == guts. clipboard_formats[i]) {
               id = i;
               break;
            }
         }
      }
      
      if ( c && id >= 0) {
         PClipboardSysData CC = C(c);
         Bool event = CC-> inside_event;
         int format;

         for ( i = 0; i < guts. clipboard_formats_count; i++)
            clipboard_kill_item( CC-> external, i);
         
         CC-> target = xe. xselection. target;
         CC-> need_write = false;
         
         CC-> inside_event = true;
         /* XXX cmSelection */
         CC-> inside_event = event;

         switch ( id) {
         case cfText:
            format = 8;
            break;
         case cfBitmap: 
            format = 32;
            break;
         case cfTargets:
            {
               int count = 0;
               Atom * ci;
               for ( i = 0; i < guts. clipboard_formats_count; i++) 
                  if ( i != cfTargets && CC-> internal[i]. size > 0)
                     count++;
               clipboard_kill_item( CC-> internal, cfTargets);
               if (( CC-> internal[cfTargets]. data = malloc( count * sizeof( Atom)))) {
                  CC-> internal[cfTargets]. size = count * sizeof( Atom);
                  ci = (Atom*)CC-> internal[cfTargets]. data;
                  for ( i = 0; i < guts. clipboard_formats_count; i++) 
                     if ( i != cfTargets && CC-> internal[i]. size > 0) 
                        *(ci++) = guts. clipboard_formats[i];
               }
            }
            format = 32;
            break;
         default:
            format = 8;
         }
        
         if ( CC-> internal[id]. size > 0) {
            XChangeProperty( 
               xe. xselection. display,
               xe. xselection. requestor,
               prop, 
               xe. xselection. target, 
               format, PropModeReplace,
               CC-> internal[id]. data,
               CC-> internal[id]. size * 8 / format);
            xe. xselection. property = prop;
         }
      }
      XSendEvent( xe.xselection.display, xe.xselection.requestor, false, 0, &xe);
   }
   break;
   case SelectionClear: 
   if ( XGetSelectionOwner( DISP, ev-> xselectionclear. selection) != WIN) {
      Handle c = ( Handle) hash_fetch( guts. clipboards, &ev-> xselectionclear. selection, sizeof( Atom)); 
      guts. last_time = ev-> xselectionclear. time;
      if (c) {
         int i;
         C(c)-> selection_owner = nilHandle;  
         for ( i = 0; i < guts. clipboard_formats_count; i++) {
            clipboard_kill_item( C(c)-> external, i);
            clipboard_kill_item( C(c)-> internal, i);
         }
      }
   }   
   break;
   }
}

