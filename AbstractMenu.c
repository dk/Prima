/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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
 */

#include "apricot.h"
#include "AbstractMenu.h"
#include "Image.h"
#include "AbstractMenu.inc"

#undef  my
#define inherited CComponent->
#define my  ((( PAbstractMenu) self)-> self)->
#define var (( PAbstractMenu) self)->

typedef Bool MenuProc ( Handle self, PMenuItemReg m, void * params);
typedef MenuProc *PMenuProc;

static int
key_normalize( const char * key)
{
   /*
    *   Valid keys:
    *      keycode as a string representing decimal number;
    *      any combination of ^, @, #  (Control, Alt, Shift) plus
    *         exactly one character - lowercase and get ascii code of this
    *         fN - function key, N is a number from 1 to 16 inclusive
    *   All other combinations will result in kbNoKey returned
    */
   int r = 0, r1;

   for (;;) {
      if (*key == '^')
	 r |= kmCtrl;
      else if (*key == '@')
	 r |= kmAlt;
      else if (*key == '#')
	 r |= kmShift;
      else
	 break;
      key++;
   }
   if (!*key) return kbNoKey;  /* #, ^, @ alone are not allowed */
   if (!key[1]) {
      return r | tolower(*key);
   } else {
      char *e;
      if (isdigit(*key)) {
	 if (r) return kbNoKey;
	 r = strtol( key, &e, 10);
	 if (*e) return kbNoKey;
	 return r | '0';
      } else if (tolower(*key) != 'f')
	 return kbNoKey;
      key++;
      r1 = strtol( key, &e, 10);
      if (*e || r1 < 1 || r1 > 16) return kbNoKey;
      return r | (kbF1 + ((r1-1) << 8));
   }
}

void
AbstractMenu_dispose_menu( Handle self, void * menu)
{
   PMenuItemReg m = menu;
   if  ( m == nil) return;
   free( m-> text);
   free( m-> accel);
   free( m-> variable);
   free( m-> perlSub);
   if ( m-> code) sv_free( m-> code);
   if ( m-> bitmap) my detach( self, m-> bitmap, false);
   my dispose_menu( self, m-> next);
   my dispose_menu( self, m-> down);
   free( m);
}


// #define log_write debug_write

void *
AbstractMenu_new_menu( Handle self, SV * sv, int level)
{
   AV * av;
   int i, count;
   int n;
   PMenuItemReg m    = nil;
   PMenuItemReg curr = nil;
   Bool rightAdjust = false;
   static int subCount;
   static int autoEnum;

  //  char buf [ 200];
  //  memset( buf, ' ', 200);
  //  buf[ level * 3] = '\0';

   if ( level == 0)
   {
      if ( SvTYPE( sv) == SVt_NULL) return nil; // null menu
      subCount = autoEnum = 0;
   }

   if ( !SvROK( sv) || ( SvTYPE( SvRV( sv)) != SVt_PVAV)) {
      warn("RTC0034: menu build error: menu is not an array");
      return nil;
   }
   av = (AV *) SvRV( sv);
   n = av_len( av);
   // log_write("%s(%d){", buf, n+1);

   // cycling the list of items
   for ( i = 0; i <= n; i++)
   {
      SV **itemHolder = av_fetch( av, i, 0);
      AV *item;
      SV *subItem;
      PMenuItemReg r;
      SV **holder;

      int l_var   = -1;
      int l_text  = -1;
      int l_sub   = -1;
      int l_accel = -1;
      int l_key   = -1;
      Bool addToSubs = true;

      if ( itemHolder == nil)
      {
         warn("RTC0035: menu build error: array panic");
         my dispose_menu( self, m);
         return nil;
      }
      if ( !SvROK( *itemHolder) || ( SvTYPE( SvRV( *itemHolder)) != SVt_PVAV)) {
         warn("RTC0036: menu build error: submenu is not an array");
         my dispose_menu( self, m);
         return nil;
      }
      // entering item description
      item = ( AV *) SvRV( *itemHolder);
      count = av_len( item) + 1;
      if ( count > 5) {
         warn("RTC0032: menu build error: extra declaration");
         count = 5;
      }
      r = malloc( sizeof( MenuItemReg));
      memset( r, 0, sizeof( MenuItemReg));
      r-> key = kbNoKey;
      // log_write("%sNo: %d, count: %d", buf, i, count);

      if ( count < 2) {          // empty of 1 means line divisor, no matter of text
         r-> divider = true;
         rightAdjust = (( level == 0) && ( var anchored));
         addToSubs = false;
      } else if ( count == 2) {
         l_text = 0;
         l_sub  = 1;
      } else if ( count == 3) {
         l_var  = 0;
         l_text = 1;
         l_sub  = 2;
      } else if ( count == 4) {
         l_text  = 0;
         l_accel = 1;
         l_key   = 2;
         l_sub   = 3;
      } else {
         l_var   = 0;
         l_text  = 1;
         l_accel = 2;
         l_key   = 3;
         l_sub   = 4;
      }

      if ( m) curr = curr-> next = r; else curr = m = r; // adding to list

      r-> rightAdjust = rightAdjust;
      r-> id = -1;

#define a_get( l_, num) if ( num >= 0 ) {                                    \
                           holder = av_fetch( item, num, 0);                 \
                           if ( holder) {                                    \
                              char * stk = SvPV( *holder, na);               \
                              l_ = malloc( strlen( stk) + 1);                \
                              strcpy( l_, stk);                              \
                           } else {                                          \
                              warn("RTC003A: menu build error: array panic");\
                              my dispose_menu( self, m);                     \
                              return nil;                                    \
                           }                                                 \
                        }
      a_get( r-> accel   , l_accel);
      a_get( r-> variable, l_var);
      if ( l_key >= 0) {
         holder = av_fetch( item, l_key, 0);
         if ( !holder) {
            warn("RTC003B: menu build error: array panic");
            my dispose_menu( self, m);
            return nil;
         }
         r-> key = key_normalize( SvPV( *holder, na));
      }

      if ( r-> variable)
      {
         #define s r-> variable
         int i, decr = 0;
         for ( i = 0; i < 2; i++)
         {
           if ( s[i] == '-') r-> disabled = ( ++decr > 0); else
           if ( s[i] == '*') r-> checked  = ( ++decr > 0); // e.g. true
         }
         if ( decr) memmove( s, s + decr, strlen( s) + 1 - decr);
         #undef s
      } else {
         char b[256];    // auto enumeration
         r-> variable = malloc( snprintf( b, 256, "MenuItem%d", ++autoEnum) + 1);
         strcpy( r-> variable, b);
      }
      // log_write( r-> variable);

      // parsing text
      if ( l_text >= 0)
      {
         holder = av_fetch( item, l_text, 0);
         if ( !holder) {
            warn("RTC003C: menu build error: array panic");
            my dispose_menu( self, m);
            return nil;
         }
         subItem = *holder;

         if ( SvROK( subItem)) {
            Handle c_object = gimme_the_mate( subItem);
            if (( c_object == nilHandle) || !( kind_of( c_object, CImage)))
            {
               warn("RTC0033: menu build error: not an image passed");
               goto TEXT;
            }
            // log_write("%sbmp: %s %d", buf, ((PComponent)c_object)->name, kind_of( c_object, CImage));
            if (((( PImage) c_object)-> status != 0) || ((( PImage) c_object)-> w == 0)
               || ((( PImage) c_object)-> h == 0))
            {
               warn("RTC0037: menu build error: invalid image passed");
               goto TEXT;
            }
            r-> bitmap =  gimme_the_mate( subItem);              // storing PImage as SV*
            my attach( self, r-> bitmap);
         } else {
            char * stk;
         TEXT:
            stk = SvPV( subItem, na);
            r-> text = malloc( strlen( stk) + 1);
            strcpy( r-> text, stk);
            // log_write( "%stext:%s", buf, r->text);
         }
      }

      // parsing sub
      if ( l_sub >= 0)
      {
         holder = av_fetch( item, l_sub, 0);
         if ( !holder) {
            warn("RTC003D: menu build error: array panic");
            my dispose_menu( self, m);
            return nil;
         }
         subItem = *holder;

         if ( SvROK( subItem))
         {
            if ( SvTYPE( SvRV( subItem)) == SVt_PVCV)
            {
               r-> code = newSVsv( subItem);
            } else {
               r-> down = my new_menu( self, subItem, level + 1);
               addToSubs = false;
               if ( r-> down == nil)
               {
                  // seems error was occured inside this call
                  my dispose_menu( self, m);
                  return nil;
               }
            }
         } else {
            if ( SvPOK( subItem)) {
               char * line = ( char *) SvPV( subItem, na);
               // log_write( "%s sub:%s", buf, line);
               r-> perlSub = malloc( strlen( line) + 1);
               strcpy( r-> perlSub, line);
            } else {
               warn("RTC0038: menu build error: invalid sub name passed");
               addToSubs = false;
            }
         }
         if ( addToSubs) r-> id = ++subCount;
      }
   }
   // log_write("%s}", buf);
// log_write("adda bunch:");
// {
//   PMenuItemReg x = m;
//   while ( x)
//   {
//      log_write( x-> variable);
//      x = x-> next;
//   }
// }
// log_write("end.");
   return m;
}

void
AbstractMenu_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   ((( PComponent) var owner)-> self)-> attach ( var owner, self);
   my set_items( self, pget_sv( items));
   my update_sys_handle( self, profile);
   if ( pget_B( selected)) my set_selected( self, true);
}

void
AbstractMenu_done( Handle self)
{
   if ( my get_selected( self)) my set_selected( self, false);
   ((( PComponent) var owner)-> self)-> detach( var owner, self, false);
   apc_menu_destroy( self);
   my dispose_menu( self, var tree);
   var tree = nil;
   inherited done( self);
}


void
AbstractMenu_set( Handle self, HV * profile)
{
   Handle postOwner = var owner;
   Bool select = false;
   if ( pexist( owner))
   {
      postOwner = pget_H( owner);
      my migrate( self, postOwner);
   }
   if ( pexist( selected))
   {
       if ( pget_B( selected)) select = true;
       pdelete( selected);
   } else
      if ( my get_selected( self)) select = true;
   inherited set( self, profile);

   var owner = postOwner;
   if ( select) my set_selected( self, true);
}

static SV *
new_av(  PMenuItemReg m, int level)
{
   AV * glo;
   if ( m == nil) return nilSV;
   glo = newAV();
   while ( m)
   {
      AV * loc = newAV();
      if ( !m-> divider)
      {
         if ( m-> variable)
         {
            int shift = ( m-> checked ? 1 : 0) + ( m-> disabled ? 1 : 0);
            char * varName = malloc( strlen( m-> variable) + 1 + shift);
            strcpy( &varName[ shift], m-> variable);
            if ( m-> checked)  varName[ --shift] = '*';
            if ( m-> disabled) varName[ --shift] = '-';
            av_push( loc, newSVpv( varName, 0));
            free( varName);
         }

         if ( m-> bitmap) av_push( loc, newRV( SvRV((( PObject)( m-> bitmap))-> mate)));
         else av_push( loc, newSVpv( m-> text, 0));

         if ( m-> accel)
         {
            av_push( loc, newSVpv( m-> accel, 0));
            av_push( loc, newSViv( m-> key));
         }

         if ( m-> down) av_push( loc, new_av( m-> down, level + 1));
         else {
            if ( m-> code) av_push( loc, newSVsv( m-> code)); else
            if ( m-> perlSub) av_push( loc, newSVpv( m-> perlSub, 0));
         }
      }
      av_push( glo, newRV_noinc(( SV *) loc));
      m = m-> next;
   }
   return newRV_noinc(( SV *) glo);
}

static Bool
var_match ( Handle self, PMenuItemReg m, void * params)
{
   return ( strcmp( m-> variable, ( char *) params) == 0);
}

static Bool
id_match ( Handle self, PMenuItemReg m, void * params)
{
   return m-> id == ( long) params;
}

static Bool
key_match ( Handle self, PMenuItemReg m, void * params)
{
   return (( m-> key == ( long) params) && ( m-> key != kbNoKey) && !( m->disabled));
}

SV *
AbstractMenu_get_items ( Handle self, char * varName)
{
   if ( var stage > csNormal) return nilSV;
   if ( strlen( varName))
   {
      PMenuItemReg m = my first_that( self, var_match, varName, true);
      return ( m && m-> down) ? new_av( m-> down, 1) : nilSV;
   } else return var tree ? new_av( var tree, 0) : nilSV;
}

void
AbstractMenu_set_items ( Handle self, SV * items)
{
   PMenuItemReg oldBranch = var tree;
   if ( var stage > csNormal) return;
   var tree = my new_menu( self, items, 0);
   apc_menu_update( self, oldBranch, var tree);
   my dispose_menu( self, oldBranch);
}


static PMenuItemReg
do_link( Handle self, PMenuItemReg m, PMenuProc p, void * params, Bool useDisabled)
{
   while( m)
   {
      if ( !m-> disabled || useDisabled)
      {
         if ( m-> down)
         {
            PMenuItemReg i = do_link( self, m-> down, p, params, useDisabled);
            if ( i) return i;
         }
         if ( p( self, m, params)) return m;
      }
      m = m-> next;
   }
   return nil;
}


void *
AbstractMenu_first_that( Handle self, void * actionProc, void * params, Bool useDisabled)
{
   return actionProc ? do_link( self, var tree, ( PMenuProc) actionProc, params, useDisabled) : nil;
}

Bool
AbstractMenu_has_item( Handle self, char * varName)
{
   return my first_that( self, var_match, varName, true) != nil;
}

void
AbstractMenu_set_check( Handle self, char * varName, Bool check)
{
   PMenuItemReg m;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   if ( m-> divider || m-> down) return;
   if ( m-> id) apc_menu_item_set_check( self, m, check);
   m-> checked = check;
}

Bool
AbstractMenu_get_check   ( Handle self, char * varName)
{
   PMenuItemReg m = my first_that( self, var_match, varName, true);
   return ( m) ? m-> checked : false;
}

Bool
AbstractMenu_get_enabled ( Handle self, char * varName)
{
   PMenuItemReg m = my first_that( self, var_match, varName, true);
   return ( m) ? !m-> disabled : false;
}

Handle
AbstractMenu_get_image( Handle self, char * varName)
{
   PMenuItemReg m = my first_that( self, var_match, varName, true);
   if ( m == nil) return false;
   return ( m) ? m-> bitmap : nilHandle;
}

char *
AbstractMenu_get_text ( Handle self, char * varName)
{
   PMenuItemReg m = my first_that( self, var_match, varName, true);
   if ( m == nil) return false;
   return ( m && m-> text) ? m-> text : "";
}

char *
AbstractMenu_get_accel ( Handle self, char * varName)
{
   PMenuItemReg m = my first_that( self, var_match, varName, true);
   return ( m && m-> accel) ? m-> accel : "";
}

SV *
AbstractMenu_get_action ( Handle self, char * varName)
{
   PMenuItemReg m;
   if ( var stage > csNormal) return nilSV;
   m = my first_that( self, var_match, varName, true);
   if ( m)
   {
      if ( m-> code)    return newSVsv( m-> code);
      if ( m-> perlSub) return newSVpv( m-> perlSub, 0);
   }
   return nilSV;
}

int
AbstractMenu_get_key ( Handle self, char * varName)
{
   PMenuItemReg m = my first_that( self, var_match, varName, true);
   return ( m && !(m-> divider) && !(m-> down)) ? m-> key : kbNoKey;
}

void
AbstractMenu_set_enabled ( Handle self, char * varName, Bool enabled)
{
   PMenuItemReg m;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   if ( m-> divider || m-> down) return;
   if ( m-> id) apc_menu_item_set_enabled( self, m, enabled);
   m-> disabled = !enabled;
}

void
AbstractMenu_set_image ( Handle self, char * varName, Handle image)
{
   PMenuItemReg m;
   PImage i = ( PImage) image;

   m = my first_that( self, var_match, varName, true);
   if ( var stage > csNormal) return;
   if ( m == nil) return;
   if ( !m-> bitmap) return;
   if (( image == nilHandle) || !( kind_of( image, CImage))) {
      warn("RTC0039: set_image error: invalid image passed");
      return;
   }
   if ( i-> status != 0 || i-> w == 0 || i-> h == 0) {
      warn("RTC0039: set_image error: invalid image passed");
      return;
   }

   my attach( self, image);
   my detach( self, m-> bitmap, false);
   if ( m-> id) apc_menu_item_set_image( self, m, image);
   m-> bitmap = image;
}


void
AbstractMenu_set_text ( Handle self, char * varName, char * text)
{
   PMenuItemReg m;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   if ( m-> text == nil) return;
   if ( m-> id) apc_menu_item_set_text( self, m, text);
   free( m-> text);
   m-> text = malloc( strlen( text) + 1);
   strcpy( m-> text, text);
}

void
AbstractMenu_set_accel ( Handle self, char * varName, char * accel)
{
   PMenuItemReg m;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   if ( m-> text == nil) return;
   if ( m-> id) apc_menu_item_set_accel( self, m, accel);
   free( m-> accel);
   m-> accel = malloc( strlen( accel) + 1);
   strcpy( m-> accel, accel);
}

void
AbstractMenu_set_action ( Handle self, char * varName, SV * action)
{
   PMenuItemReg m;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   if ( m-> divider || m-> down) return;
   if ( SvROK( action))
   {
      if ( m-> code) sv_free( m-> code);
      m-> code = nil;
      if ( SvTYPE( SvRV( action)) == SVt_PVCV)
      {
         m-> code = newSVsv( action);
         free( m-> perlSub);
         m-> perlSub = nil;
      }
   }
   else
   {
      char * line = ( char *) SvPV( action, na);
      free( m-> perlSub);
      if ( m-> code) sv_free( m-> code);
      m-> code = nil;
      m-> perlSub = malloc( strlen( line) + 1);
      strcpy( m-> perlSub, line);
   }
}

void
AbstractMenu_set_key( Handle self, char * varName, char * key)
{
   PMenuItemReg m;
   int _key;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   _key = key_normalize( key);
   if ( m-> divider || m-> down) return;
   if ( m-> id) apc_menu_item_set_key( self, m, _key);
   m-> key = _key;
}

void
AbstractMenu_set_variable( Handle self, char * varName, char * newName)
{
   PMenuItemReg m;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   free( m-> variable);
   m-> variable = malloc( strlen( newName) + 1);
   strcpy( m-> variable, newName);

}

static Bool
sub_call( Handle self, PMenuItemReg m)
{
   if ( m == nil) return false;
   if ( m-> code) cv_call_perl((( PComponent) var owner)-> mate, SvRV( m-> code), "s", m-> variable);
   else if ( m-> perlSub) call_perl( var owner, m-> perlSub, "s", m-> variable);
   return true;
}

Bool
AbstractMenu_sub_call_id ( Handle self, int sysId)
{
   return sub_call( self, my first_that( self, id_match, (void *) sysId, false));
}

#define keyRealize( key)     if ((( key & 0xFF) >= 'A') && (( key & 0xFF) <= 'z')) \
                             key = tolower( key & 0xFF) |                          \
                                (( key & ( kmCtrl | kmAlt)) ?                      \
                                ( key & ( kmCtrl | kmAlt | kmShift))               \
                             : 0)

Bool
AbstractMenu_sub_call_key ( Handle self, int key)
{
   keyRealize( key);
   return sub_call( self, my first_that( self, key_match, (void *) key, false));
}

typedef struct _Kmcc
{
   int  key;
   Bool enabled;
} Kmcc, *PKmcc;

static Bool
kmcc ( Handle self, PMenuItemReg m, void * params)
{
   if ((( PKmcc) params)-> key == m-> key)
   {
      if ( m-> id) apc_menu_item_set_enabled( self, m, !m-> disabled);
      m-> disabled = !(( PKmcc) params)-> enabled;
   }
   return false;
}

void
AbstractMenu_set_command( Handle self, char * key, Bool enabled)
{
   Kmcc mcc = { key_normalize( key), enabled};
   if ( var stage > csNormal) return;
   my first_that( self, kmcc, &mcc, true);
}

void AbstractMenu_set_selected( Handle self, Bool selected) {}
Bool AbstractMenu_get_selected( Handle self) { return false; }

SV *
AbstractMenu_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_menu_get_handle( self));
   return newSVpv( buf, 0);
}

int
AbstractMenu_translate_accel( Handle self, char * accel)
{
   if ( !accel) return false;
   accel = strchr( accel, '~');
   return ( !accel || !isalnum( accel[1])) ? kbNoKey : tolower( accel[1]);
}

int
AbstractMenu_translate_key( Handle self, int code, int key, int mod)
{
   key = ( key != kbNoKey ? key : code) | mod;
   keyRealize( key);
   return key;
}

static Bool up_match   ( Handle self, PMenuItemReg m, void * params) { return m-> down == params; }
static Bool prev_match ( Handle self, PMenuItemReg m, void * params) { return m-> next == params; }

void
AbstractMenu_delete( Handle self, char * varName)
{
   PMenuItemReg up, prev, m;
   if ( var stage > csNormal) return;
   m = my first_that( self, var_match, varName, true);
   if ( m == nil) return;
   apc_menu_item_delete( self, m);
   up   = my first_that( self, up_match, m, true);
   prev = my first_that( self, prev_match, m, true);
   if ( up)   up  -> down = m-> next;
   if ( prev) prev-> next = m-> next;
   if ( m == var tree) var tree = m-> next;
   m-> next = nil;
   my dispose_menu( self, m);
}

static Bool collect_id( Handle self, PMenuItemReg m, int * params)
{
   if ( m-> id > 0) (*params)++;
   return false;
}

static Bool increase_id( Handle self, PMenuItemReg m, int * params)
{
   if ( m-> id > 0) m-> id += *params;
   return false;
}

void
AbstractMenu_insert( Handle self, SV * menuItems, char * rootName, int index)
{
   PMenuItemReg *up, m, addFirst, addLast, branch;

   if ( var stage > csNormal) return;

   if ( SvTYPE( menuItems) == SVt_NULL) return;

   if ( strlen( rootName) == 0)
   {
      if ( var tree == nil)
      {
         var tree = my new_menu( self, menuItems, 0);
         apc_menu_update( self, nil, var tree);
         return;
      }
      m = var tree;
      up = &var tree;
   } else {
      branch = m = my first_that( self, var_match, rootName, true);
      if ( m == nil || m-> down == nil) return;
      up = &m-> down;
      m = m-> down;
   }
   {
      int maxId = 0;
      PMenuItemReg save = var tree;
      my first_that( self, collect_id, &maxId, true);
      addFirst = my new_menu( self, menuItems, 0);
      if ( !addFirst) return; // error in menuItems
      var tree = addFirst;
      my first_that( self, increase_id, &maxId, true);
      var tree = save;
   }

   addLast = addFirst;
   while ( addLast-> next) addLast = addLast-> next;

   if ( index == 0)
   {
      addLast-> next = *up;
      *up = addFirst;
   } else
   {
      int i = 1;
      while ( m-> next)
      {
         if ( i++ == index) break;
         m = m-> next;
      }
      addLast-> next = m-> next;
      m-> next = addFirst;
   }

   if ( m-> rightAdjust) while ( addFirst != addLast-> next)
   {
      addFirst-> rightAdjust = true;
      addFirst = addFirst-> next;
   }

   apc_menu_update( self, branch, branch);
}

