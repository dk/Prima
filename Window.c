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
#include "Window.h"
#include "Menu.h"
#include "Icon.h"
#include "Application.h"
#include <Window.inc>

#undef  my
#define inherited CWidget->
#define my  ((( PWindow) self)-> self)
#define var (( PWindow) self)


void
Window_init( Handle self, HV * profile)
{
   SV * sv;
   inherited init( self, profile);
   opt_set( optSystemSelectable);
   opt_assign( optOwnerIcon, pget_B( ownerIcon));
   my-> set_icon( self, pget_H( icon));
   my-> set_menu_color ( self, pget_i( menuColor),             ciFore);
   my-> set_menu_color ( self, pget_i( menuBackColor),         ciBack);
   my-> set_menu_color ( self, pget_i( menuHiliteColor),       ciHiliteText);
   my-> set_menu_color ( self, pget_i( menuHiliteBackColor),   ciHilite);
   my-> set_menu_color ( self, pget_i( menuDisabledColor),     ciDisabledText);
   my-> set_menu_color ( self, pget_i( menuDisabledBackColor), ciDisabled);
   my-> set_menu_color ( self, pget_i( menuLight3DColor),      ciLight3DColor);
   my-> set_menu_color ( self, pget_i( menuDark3DColor),       ciDark3DColor);
   SvHV_Font( pget_sv( menuFont), &Font_buffer, "Window::init");
   my-> set_menu_font  ( self, Font_buffer);
   if ( SvTYPE( sv = pget_sv( menuItems)) != SVt_NULL)
      my-> set_menu_items( self, sv);
   my-> set_modal_result( self, pget_i( modalResult));
   my-> set_modal_horizon( self, pget_B( modalHorizon));
}

void
Window_cancel( Handle self)
{
   my-> set_modal_result ( self, cmCancel);
   my-> end_modal( self);
}

void
Window_ok( Handle self)
{
   my-> set_modal_result ( self, cmOK);
   my-> end_modal( self);
}


void
Window_done( Handle self)
{
   if ( var-> modal) my-> cancel( self);
   my-> detach( self, var-> menu, true);
   var-> menu = nilHandle;
   inherited done( self);
}

void Window_update_sys_handle( Handle self, HV * profile)
{
   var-> widgetClass = wcWindow;
   if (!(
       pexist( owner) ||
       pexist( syncPaint) ||
       pexist( taskListed) ||
       pexist( clipOwner) ||
       pexist( borderIcons) ||
       pexist( borderStyle)
    )) return;
   if ( pexist( owner)) my-> cancel_children( self);
   if ( pexist( clipOwner) && pget_B( clipOwner)) my-> set_modal_horizon( self, false);
   if ( !apc_window_create( self,
      pexist( owner )      ? pget_H( owner )      : var-> owner ,
      pexist( syncPaint)   ? pget_B( syncPaint)   : my-> get_sync_paint( self),
      pexist( clipOwner)   ? pget_B( clipOwner)   : my-> get_clip_owner( self),
      pexist( borderIcons) ? pget_i( borderIcons) : my-> get_border_icons( self),
      pexist( borderStyle) ? pget_i( borderStyle) : my-> get_border_style( self),
      pexist( taskListed)  ? pget_B( taskListed)  : my-> get_task_listed( self),
      pexist( windowState) ? pget_i( windowState) : my-> get_window_state( self),
      !( pexist( originDontCare) && pget_B( originDontCare)),
      !( pexist( sizeDontCare)   && pget_B( sizeDontCare))
   ))
      croak("RTC0090: Cannot create window");
   pdelete( borderStyle);
   pdelete( borderIcons);
   pdelete( syncPaint);
   pdelete( taskListed);
   pdelete( clipOwner);
   pdelete( windowState);
   pdelete( clipOwner);
   pdelete( owner);
}

void Window_handle_event( Handle self, PEvent event)
{
#define evOK ( var-> evStack[ var-> evPtr - 1])
#define objCheck if ( var-> stage > csNormal) return
   switch (event-> cmd)
   {
   case cmColorChanged:
      if ( event-> gen. source == var-> menu) {
         var-> menuColor[ event-> gen. i] = apc_menu_get_color( var-> menu, event-> gen. i);
         return;
      }
      break;
   case cmFontChanged:
      if ( event-> gen. source == var-> menu)
      {
         apc_menu_get_font( var-> menu, &var-> menuFont);
         return;
      }
      break;
   case cmExecute:
      my-> notify( self, "<s", "Execute");
      break;
   case cmEndModal:
      my-> notify( self, "<s", "EndModal");
      break;
   case cmActivate:
      if ( var-> owner)
         PWidget( var-> owner)-> currentWidget = self;
      my-> notify( self, "<s", "Activate");
      break;
   case cmDeactivate:
      my-> notify( self, "<s", "Deactivate");
      break;
   case cmWindowState:
      my-> notify( self, "<si", "WindowState", event-> gen. i);
      break;
   case cmDelegateKey:
      #define leave { my-> clear_event( self); return; }
      if ( var-> modal && event-> key. subcmd == 0)
      {
          Event ev = *event;
          ev. key. cmd   = cmTranslateAccel;
          if ( !my-> message( self, &ev)) leave;
          if ( my-> first_that( self, accel_notify, &ev)) leave;
          ev. key. cmd    = cmDelegateKey;
          ev. key. subcmd = 1;
          if ( my-> first_that( self, accel_notify, &ev)) leave;
      }
      objCheck;
      break;
   case cmTranslateAccel:
      if ( event-> key. key == kbEsc && var-> modal)
      {
         my-> cancel( self);
         my-> clear_event( self);
      }
      objCheck;
      if ( event-> key. key == kbTab || event-> key. key == kbBackTab)
      {
         int i         = 0, count = var-> widgets. count;
         int dir       = ( event-> key. key == kbTab) ? 1 : -1;
         Handle foc    = apc_widget_get_focused();
         if ( count > 1)
         {
            int order, min = INT_MAX, max = 0, j = 0;
            Handle minH = nilHandle, maxH = nilHandle, match;
            /* searching for 1-st level view that responds for focus */
            while ( foc && PWidget( foc)-> owner != self)
               foc = PWidget( foc)-> owner;

            /* determining widgets with minimum and maiximum tabOrder values */
            for ( i = 0; i < count; i++) {
                PWidget v = ( PWidget) var-> widgets. items[ i];
                if ( v-> tabOrder < 0) continue;
                if ( max <= v-> tabOrder) {
                   max  = v-> tabOrder;
                   maxH = ( Handle) v;
                }
                if ( min >= v-> tabOrder) {
                   min  = v-> tabOrder;
                   minH = ( Handle) v;
                }
                j++;
            }
            if ( j == 0) return; // protecting the unlikely case when all tabOrders < 0
            if ( foc == nilHandle) foc = minH;
            order = PWidget( foc)-> tabOrder;

            do {
               // determining next possible selection
               if ( dir > 0 && order == max) { // cycling to start
                  match = minH;
               } else if ( dir < 0 && order == min) { // cycling to end
                  match = maxH;
               } else {
                  int left = INT_MAX, right = INT_MIN;
                  Handle leftH = nilHandle, rightH = nilHandle;
                  for ( i = 0; i < count; i++) {
                      PWidget v = ( PWidget) var-> widgets. items[ i];
                      int x;
                      if ( v-> tabOrder < 0) continue;
                      x = order - v-> tabOrder;
                      if ( x > 0 && x < left) {
                         left = x;
                         leftH = ( Handle) v;
                      }
                      if ( x < 0 && x > right) {
                         right = x;
                         rightH = ( Handle) v;
                      }
                  }
                  match = ( dir < 0) ? leftH : rightH;
               }

               // checking, whether selectable ...
               if ( CWidget( match)-> get_enabled( match)  &&
                    CWidget( match)-> get_tab_stop( match) &&
                    CWidget( match)-> get_selectee( match))
               {
                    CWidget( match)-> set_selected( match, 1);
                    break;
               }
               order = PWidget( match)-> tabOrder;
            } while ( match != foc);
         }
         my-> clear_event( self);
      }
      break;
   }
   inherited handle_event ( self, event);
}

void
Window_end_modal( Handle self)
{
   Event ev = { cmEndModal};
   if ( !var-> modal) return;

   apc_window_end_modal( self);
   ev. gen. source = self;
   my-> message( self, &ev);
}

int
Window_get_modal( Handle self)
{
   return var-> modal;
}

Handle
Window_get_horizon( Handle self)
{
   /* self trick is appropriate here;
      don't bump into it accidentally */
   self = var-> owner;
   while ( self != application && !my-> get_modal_horizon( self))
      self = var-> owner;
   return self;
}


void
Window_exec_enter_proc( Handle self, Bool sharedExec, Handle insertBefore)
{
   if ( var-> modal)
      return;

   if ( sharedExec) {
      Handle mh = my-> get_horizon( self);
      var-> modal = mtShared;

      /* adding new modal horizon in global mh-list */
      if ( mh != application && !PWindow(mh)-> nextSharedModal)
         list_add( &PApplication( application)-> modalHorizons, mh);

      if (( var-> nextSharedModal = insertBefore)) {
         /* inserting window in between of modal list */
         Handle *iBottom = ( mh == application) ?
            &PApplication(mh)-> sharedModal : &PWindow(mh)-> nextSharedModal;
         var-> prevSharedModal = PWindow( insertBefore)-> prevSharedModal;
         if ( *iBottom == insertBefore)
            *iBottom = self;
      } else {
         /* inserting window on top of modal list */
         Handle *iTop = ( mh == application) ?
            &PApplication(mh)-> topSharedModal : &PWindow(mh)-> topSharedModal;
         if ( *iTop)
            PWindow( *iTop)-> nextSharedModal = self;
         else {
            if ( mh == application)
               PApplication(mh)-> sharedModal = self;
            else
               PWindow(mh)-> nextSharedModal = self;
         }
         var-> prevSharedModal = *iTop;
         *iTop = self;
      }
   }
   /* end of shared exec */
   else
   /* start of exclusive exec */
   {
      PApplication app = ( PApplication) application;
      var-> modal = mtExclusive;
      if (( var-> nextExclModal = insertBefore)) {
         var-> prevExclModal = PWindow( insertBefore)-> prevExclModal;
         if ( app-> exclModal == insertBefore)
            app-> exclModal = self;
      } else {
         var-> prevExclModal = app-> topExclModal;
         if ( app-> exclModal)
            PWindow( app-> topExclModal)-> nextExclModal = self;
         else
            app-> exclModal = self;
         app-> topExclModal = self;
      }
   }
}

void
Window_exec_leave_proc( Handle self)
{
   if ( !var-> modal)
      return;

   if ( var-> modal == mtShared) {
      Handle mh = my-> get_horizon( self);

      if ( var-> prevSharedModal) {
         PWindow prev = ( PWindow) var-> prevSharedModal;
         if ( prev-> nextSharedModal == self)
            prev-> nextSharedModal = var-> nextSharedModal;
      }
      if ( var-> nextSharedModal) {
         PWindow next = ( PWindow) var-> nextSharedModal;
         if ( next-> prevSharedModal == self)
            next-> prevSharedModal = var-> prevSharedModal;
      }
      if ( mh == application) {
         PApplication app = ( PApplication) application;
         if ( app) {
            if ( app-> sharedModal == self)
               app-> sharedModal = var-> nextSharedModal;
            if ( app-> topSharedModal == self)
               app-> topSharedModal = var-> prevSharedModal;
         }
      } else {
         PWindow win = ( PWindow) mh;
         if ( win-> nextSharedModal == self)
            win-> nextSharedModal = var-> nextSharedModal;
         if ( win-> topSharedModal == self)
            win-> topSharedModal = var-> prevSharedModal;
         /* removing horizon from global mh-list */
         if ( !win-> nextSharedModal)
             list_delete( &PApplication(application)-> modalHorizons, mh);
      }

      var-> prevSharedModal = var-> nextSharedModal = nilHandle;
   } /* end of shared exec */
   else
   /* start of exclusive exec */
   {
      PApplication app = ( PApplication) application;
      if ( var-> prevExclModal) {
         PWindow prev = ( PWindow) var-> prevExclModal;
         if ( prev-> nextExclModal == self)
            prev-> nextExclModal = var-> nextExclModal;
      }
      if ( var-> nextExclModal) {
         PWindow next = ( PWindow) var-> nextExclModal;
         if ( next-> prevExclModal == self)
            next-> prevExclModal = var-> prevExclModal;
      }
      if ( app) {
         if ( app-> exclModal == self)
            app-> exclModal = var-> nextExclModal;
         if ( app-> topExclModal == self)
            app-> topExclModal = var-> prevExclModal;
      }
      var-> prevExclModal = var-> nextExclModal = nilHandle;
   }
   var-> modal = mtNone;
}

void
Window_cancel_children( Handle self)
{
   protect_object( self);
   if ( my-> get_modal_horizon( self)) {
      Handle next = var-> nextSharedModal;
      while ( next) {
         CWindow( next)-> cancel( next);
         next = var-> nextSharedModal;
      }
   } else {
      Handle mh   = my-> get_horizon( self);
      Handle next = ( mh == application) ?
                  PApplication(mh)-> sharedModal :
                  PWindow(mh)-> nextSharedModal;
      while ( next) {
         if ( Widget_is_child( next, self)) {
            CWindow( next)-> cancel( next);
            next = PWindow(mh)-> nextSharedModal;
         } else
            next = PWindow(next)-> nextSharedModal;
      }
   }
   unprotect_object( self);
}


int
Window_execute( Handle self, Handle insertBefore)
{
   if ( var-> modal || my-> get_clip_owner( self))
      return cmCancel;

   protect_object( self);
   if ( insertBefore
	&& ( insertBefore == self
	     || !kind_of( insertBefore, CWindow)
	     || PWindow( insertBefore)-> modal != mtExclusive))
      insertBefore = nilHandle;
   if ( !apc_window_execute( self, insertBefore))
      var-> modalResult = cmCancel;

   unprotect_object( self);
   return var-> modalResult;
}

Bool
Window_execute_shared( Handle self, Handle insertBefore)
{
   if ( var-> modal || my-> get_clip_owner( self) || var-> nextSharedModal) return false;
   if ( insertBefore &&
         (( insertBefore == self) ||
         ( !kind_of( insertBefore, CWindow)) ||
         ( PWindow( insertBefore)-> modal != mtShared) ||
         ( CWindow( insertBefore)-> get_horizon( insertBefore) != my-> get_horizon( self))))
             insertBefore = nilHandle;
   return apc_window_execute_shared( self, insertBefore);
}


Bool Window_get_modal_horizon( Handle self)
{
   return is_opt( optModalHorizon);
}


void Window_set_modal_horizon( Handle self, Bool modalHorizon)
{
   if ( is_opt( optModalHorizon) == modalHorizon) return;
   if ( modalHorizon && my-> get_clip_owner( self)) return;
   my-> cancel_children( self);
   opt_assign( optModalHorizon, modalHorizon);
}


int Window_get_modal_result( Handle self)
{
  return var-> modalResult;
}

void
Window_set_text( Handle self, char * text)
{
   inherited set_text( self, text);
   apc_window_set_caption( self, var-> text);
}

void
Window_set_modal_result ( Handle self, int _modalResult)
{
   var-> modalResult = _modalResult;
}


static void
activate( Handle self, Bool ok)
{
   if ( var-> stage == csNormal) {
      if ( ok)
         apc_window_activate( self);
      else
         if ( apc_window_is_active( self))
            apc_window_activate( nilHandle);
   }
}

void
Window_set_focused( Handle self, Bool focused)
{
   activate( self, focused);
   inherited set_focused( self, focused);
}


void
Window_set_selected( Handle self, Bool selected)
{
   activate( self, selected);
   inherited set_selected( self, selected);
}


Handle
Window_get_menu( Handle self)
{
   return var-> menu;
}

SV *
Window_get_menu_items( Handle self)
{
   return var-> menu ? ((( PMenu) var-> menu)-> self)-> get_items( var-> menu, "") : nilSV;
}

void Window_set( Handle self, HV * profile)
{
   Handle postOwner = nilHandle;
   if ( pexist( menuFont)) {
      SvHV_Font( pget_sv( menuFont), &Font_buffer, "Window::set");
      my-> set_menu_font( self, Font_buffer);
      pdelete( menuFont);
   }
   if ( pexist( owner)) postOwner = pget_H( owner);
   inherited set( self, profile);
   if ( postOwner && is_opt( optOwnerIcon)) {
      my-> set_owner_icon( self, 1);
      opt_set( optOwnerColor);
   }
}

Handle
Window_get_icon( Handle self)
{
   if ( var-> stage > csNormal) return nilHandle;
   if ( apc_window_get_icon( self, nilHandle)) {
      HV * profile = newHV();
      Handle i = Object_create( "Prima::Icon", profile);
      sv_free(( SV *) profile);
      apc_window_get_icon( self, i);
      --SvREFCNT( SvRV((( PAnyObject) i)-> mate));
      return i;
   } else
      return nilHandle;
}

static Bool
icon_notify ( Handle self, Handle child, Handle icon)
{
    if ( kind_of( child, CWindow) && (( PWindow) child)-> options. optOwnerIcon) {
       (( PWindow) child)-> self-> set_icon( child, icon);
       (( PWindow) child)-> options. optOwnerIcon = 1;
    }
    return false;
}

void
Window_set_icon( Handle self, Handle icon)
{
   if ( var-> stage > csNormal) return;
   if ( icon && !kind_of( icon, CImage)) {
       warn("RTC0091: Illegal object reference passed to Window.set_icon");
       return;
   }
   my-> first_that( self, icon_notify, (void*)icon);
   apc_window_set_icon( self, icon);
   opt_clear( optOwnerIcon);
}

void
Window_set_menu( Handle self, Handle menu)
{
   if ( var-> stage > csNormal) return;
   if ( menu && !kind_of( menu, CMenu)) return;
   if ( menu && (( PMenu) menu)-> owner != self)
      my-> set_menu_items( self, ((( PMenu) menu)-> self)-> get_items( menu, ""));
   else
   {
      apc_window_set_menu( self, menu);
      var-> menu = menu;
      if ( menu)
      {
         int i;
         ColorSet menuColor;
         memcpy( menuColor, var-> menuColor, sizeof( ColorSet));
         for ( i = 0; i < ciMaxId + 1; i++)
           apc_menu_set_color( menu, menuColor[ i], i);
         memcpy( var-> menuColor, menuColor, sizeof( ColorSet));
         apc_menu_set_font( menu, &var-> menuFont);
      }
   }
}

void
Window_set_menu_items( Handle self, SV * menuItems)
{
   if ( var-> stage > csNormal) return;
   if ( var-> menu == nilHandle)
   {
     if ( SvTYPE( menuItems))
     {
         HV * profile = newHV();
         pset_sv( items, menuItems);
         pset_H ( owner, self);
         pset_i ( selected, false);
         my-> set_menu( self, create_instance( "Prima::Menu"));
         sv_free(( SV *) profile);
      }
   }
   else
      ((( PMenu) var-> menu)-> self)-> set_items( var-> menu, menuItems);
}


void Window_set_menu_color( Handle self, Color color, int index)
{
   if (( index < 0) || ( index > ciMaxId)) return;
   if (( color < 0) && (( color & wcMask) == 0)) color |= wcMenu;
   var-> menuColor[ index] = color;
   if ( var-> menu) apc_menu_set_color( var-> menu, color, index);
}

void
Window_set_menu_font( Handle self, Font font)
{
   apc_font_pick( self, &font, &var-> menuFont);
   if ( var-> menu) apc_menu_set_font( var-> menu, &var-> menuFont);
}


Color Window_get_menu_color( Handle self, int index)
{
   index = (( index < 0) || ( index > ciMaxId)) ? 0 : index;
   return  var-> menuColor[ index];
}

Font
Window_get_menu_font( Handle self)
{
   return var-> menuFont;
}

Font
Window_get_default_menu_font( char * dummy)
{
   Font f;
   apc_menu_default_font( &f);
   return f;
}

Bool
Window_get_owner_icon( Handle self)
{
   return is_opt( optOwnerIcon);
}

void
Window_set_owner_icon( Handle self, Bool ownerIcon)
{
   opt_assign( optOwnerIcon, ownerIcon);
   if ( is_opt( optOwnerIcon) && var-> owner) {
      Handle icon = ( var-> owner == application) ?
         ((( PApplication) application)-> self)-> get_icon( application) :
         ((( PWindow)      var-> owner)-> self)-> get_icon( var-> owner);
      my-> set_icon( self, icon);
      opt_set( optOwnerIcon);
   }
}


Bool
Window_process_accel( Handle self, int key)
{
   return var-> modal ? my-> first_that_component( self, find_accel, &key)
     : inherited process_accel( self, key);
}

void  Window_on_execute( Handle self) {}
void  Window_on_endmodal( Handle self) {}
void  Window_on_activate( Handle self) {}
void  Window_on_deactivate( Handle self) {}
void  Window_on_windowstate( Handle self, int windowState) {}
void  Window_set_transparent( Handle self, Bool transparent) {}
