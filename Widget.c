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
#include "Application.h"
#include "Icon.h"
#include "Popup.h"
#include "Widget.h"
#include "Window.h"
#include <Widget.inc>

#undef  my
#define inherited CDrawable
#define enter_method PWidget_vmt selfvmt = ((( PWidget) self)-> self)
#define my  selfvmt
#define var (( PWidget) self)

typedef Bool ActionProc ( Handle self, Handle item, void * params);
typedef ActionProc *PActionProc;
#define his (( PWidget) child)

/* local defines */
typedef struct _SingleColor
{
   Color color;
   int   index;
} SingleColor, *PSingleColor;

static Bool find_dup_msg( PEvent event, int cmd);
static Bool pquery ( Handle window, Handle self, void * v);
static Bool get_top_current( Handle self);
static Bool sptr( Handle window, Handle self, void * v);
static Bool size_notify( Handle self, Handle child, const Rect* metrix);
static Bool move_notify( Handle self, Handle child, Point * moveTo);
static Handle find_tabfoc( Handle self);
static Bool showhint_notify ( Handle self, Handle child, void * data);
static Bool hint_notify ( Handle self, Handle child, char * hint);
       Bool read_point( AV * av, int * pt, int number, char * error);
       Bool accel_notify ( Handle group, Handle self, PEvent event);
       Bool font_notify ( Handle self, Handle child, void * font);
       Bool find_accel( Handle self, Handle item, int * key);
       Bool single_color_notify ( Handle self, Handle child, void * color);
       Bool kill_all( Handle self, Handle child, void * dummy);
extern PRGBColor read_palette( int * palSize, SV * palette);

/* init, done & update_sys_handle */
void
Widget_init( Handle self, HV * profile)
{
   enter_method;
   PComponent attachTo;
   SV * sv;

   inherited-> init( self, profile);

   /* var init */
   list_create( &var-> widgets, 0, 8);
   var-> tabOrder = -1;

   if ( !kind_of( var-> owner, CWidget)) {
      croak("Illegal object reference passed to Widget::init");
      return;
   }

   attachTo = ( PComponent) var-> owner;
   attachTo-> self-> attach ( var-> owner, self);

   my-> update_sys_handle( self, profile);
   /* props init */
   /* font and colors */
   SvHV_Font( pget_sv( font), &Font_buffer, "Widget::init");
   my-> set_widget_class       ( self, pget_i( widgetClass  ));
   my-> set_color              ( self, pget_i( color        ));
   my-> set_back_color         ( self, pget_i( backColor    ));
   my-> set_font               ( self, Font_buffer);
   opt_assign( optOwnerBackColor, pget_B( ownerBackColor));
   opt_assign( optOwnerColor    , pget_B( ownerColor));
   opt_assign( optOwnerFont     , pget_B( ownerFont));
   opt_assign( optOwnerHint     , pget_B( ownerHint));
   opt_assign( optOwnerShowHint , pget_B( ownerShowHint));
   opt_assign( optOwnerPalette  , pget_B( ownerPalette));
   my-> set_color_index( self, pget_i( hiliteColor),       ciHiliteText);
   my-> set_color_index( self, pget_i( hiliteBackColor),   ciHilite);
   my-> set_color_index( self, pget_i( disabledColor),     ciDisabledText);
   my-> set_color_index( self, pget_i( disabledBackColor), ciDisabled);
   my-> set_color_index( self, pget_i( light3DColor),      ciLight3DColor);
   my-> set_color_index( self, pget_i( dark3DColor),       ciDark3DColor);
   my-> set_palette( self, pget_sv( palette));

   /* light props */
   my-> set_brief_keys         ( self, pget_B(  briefKeys));
   my-> set_buffered           ( self, pget_B(  buffered));
   my-> set_cursor_visible     ( self, pget_B(  cursorVisible));
   my-> set_grow_mode          ( self, pget_i(  growMode));
   my-> set_hint               ( self, pget_c(  hint));
   my-> set_help_context       ( self, pget_i(  helpContext));
   my-> set_first_click        ( self, pget_B(  firstClick));
   {
      Point hotSpot;
      Handle icon = pget_H( pointerIcon);
      read_point(( AV *) SvRV( pget_sv( pointerHotSpot)), (int*)&hotSpot, 2, "RTC0087: Array panic on 'pointerHotSpot'");
      if ( icon != nilHandle && !kind_of( icon, CIcon)) {
         warn("RTC083: Illegal object reference passed to Widget.set_pointer_icon");
         icon = nilHandle;
      }
      apc_pointer_set_user( self, icon, hotSpot);
   }
   my-> set_pointer_type       ( self, pget_i(  pointerType));
   my-> set_selecting_buttons  ( self, pget_i(  selectingButtons));
   my-> set_selectable         ( self, pget_B(  selectable));
   my-> set_show_hint          ( self, pget_B(  showHint));
   my-> set_tab_order          ( self, pget_i(  tabOrder));
   my-> set_tab_stop           ( self, pget_i(  tabStop));
   my-> set_text               ( self, pget_c(  text));

   opt_assign( optScaleChildren, pget_B( scaleChildren));

   /* subcomponents props */
   my-> set_popup_color ( self, pget_i( popupColor),             ciFore);
   my-> set_popup_color ( self, pget_i( popupBackColor),         ciBack);
   my-> set_popup_color ( self, pget_i( popupHiliteColor),       ciHiliteText);
   my-> set_popup_color ( self, pget_i( popupHiliteBackColor),   ciHilite);
   my-> set_popup_color ( self, pget_i( popupDisabledColor),     ciDisabledText);
   my-> set_popup_color ( self, pget_i( popupDisabledBackColor), ciDisabled);
   my-> set_popup_color ( self, pget_i( popupLight3DColor),      ciLight3DColor);
   my-> set_popup_color ( self, pget_i( popupDark3DColor),       ciDark3DColor);
   SvHV_Font( pget_sv( popupFont), &Font_buffer, "Widget::init");
   my-> set_popup_font  ( self, Font_buffer);
   if ( SvTYPE( sv = pget_sv( popupItems)) != SVt_NULL)
      my-> set_popup_items( self, sv);
   if ( SvTYPE( sv = pget_sv( accelItems)) != SVt_NULL)
      my-> set_accel_items( self, sv);

   /* size, position, enabling, visibliity etc. runtime */
   {
      Point set;
      int i[2];
      AV * av;
      SV ** holder;
      NPoint ds = {1,1};

      read_point(( AV *) SvRV( pget_sv( sizeMin)), (int*)&set, 2, "RTC0082: Array panic on 'sizeMin'");
      my-> set_size_min( self, set);
      read_point(( AV *) SvRV( pget_sv( sizeMax)), (int*)&set, 2, "RTC0083: Array panic on 'sizeMax'");
      my-> set_size_max( self, set);
      read_point(( AV *) SvRV( pget_sv( cursorSize)), i, 2, "RTC0084: Array panic on 'cursorSize'");
      my-> set_cursor_size( self, i[0], i[1]);
      read_point(( AV *) SvRV( pget_sv( cursorPos)), i, 2, "RTC0085: Array panic on 'cursorPos'");
      my-> set_cursor_pos( self, i[0], i[1]);

      av = ( AV *) SvRV( pget_sv( designScale));
      holder = av_fetch( av, 0, 0);
      ds. x = holder ? SvNV( *holder) : 1;
      if ( !holder) warn("RTC0086: Array panic on 'designScale'");
      holder = av_fetch( av, 1, 0);
      ds. y = holder ? SvNV( *holder) : 1;
      if ( !holder) warn("RTC0086: Array panic on 'designScale'");
      my-> set_design_scale( self, ds.x, ds.y);
   }
   my-> set_enabled     ( self, pget_B( enabled));
   if ( !pexist( originDontCare) || !pget_B( originDontCare))
      my-> set_pos( self, pget_i( left), pget_i( bottom));
   if ( !pexist( sizeDontCare  ) || !pget_B( sizeDontCare  ))
      my-> set_size( self, pget_i( width), pget_i( height));
   {
      Bool x = 0, y = 0;
      if ( pget_B( centered)) { x = 1; y = 1; };
      if ( pget_B( x_centered) || ( var-> growMode & gmXCenter)) x = 1;
      if ( pget_B( y_centered) || ( var-> growMode & gmYCenter)) y = 1;
      if ( x || y) my-> set_centered( self, x, y);
   }
   my-> set_shape       ( self, pget_H(  shape));
   my-> set_visible     ( self, pget_B( visible));
   if ( pget_B( capture)) my-> set_capture( self, 1, nilHandle);
   if ( pget_B( current)) my-> set_current( self, 1);
}


void
Widget_update_sys_handle( Handle self, HV * profile)
{
   enter_method;
   if (!(
       pexist( owner) ||
       pexist( syncPaint) ||
       pexist( clipOwner) ||
       pexist( transparent)
    )) return;
   if ( !apc_widget_create( self,
      pexist( owner)      ? pget_H( owner)      : var-> owner,
      pexist( syncPaint)  ? pget_B( syncPaint)  : my-> get_sync_paint( self),
      pexist( clipOwner)  ? pget_B( clipOwner)  : my-> get_clip_owner( self),
      pexist( transparent)? pget_B( transparent): my-> get_transparent( self)
   ))
     croak( "RTC0080: Cannot create widget");
   pdelete( transparent);
   pdelete( syncPaint);
   pdelete( clipOwner);
   pdelete( owner);
}


void
Widget_done( Handle self)
{
   enter_method;
   PComponent detachFrom = ( PComponent) var-> owner;

   if ((( PApplication) application)-> hintUnder == self)
      my-> set_hint_visible( self, 0);

   my-> first_that( self, kill_all, nil);
   if ( var-> accelTable)
      my-> detach( self, var-> accelTable, true);
   var-> accelTable = nilHandle;

   detachFrom-> self-> detach( var-> owner, self, false);
   my-> detach( self, var-> popupMenu, true);
   var-> popupMenu = nilHandle;

   free( var-> text);
   apc_widget_destroy( self);
   free( var-> hint);
   var-> text = nil;
   var-> hint = nil;

   list_destroy( &var-> widgets);
   inherited-> done( self);
}

/* ::a */
void
Widget_attach( Handle self, Handle objectHandle)
{
   if ( objectHandle == nilHandle) return;
   if ( var-> stage > csNormal) return;
   if ( kind_of( objectHandle, CWidget)) list_add( &var-> widgets, objectHandle);
   inherited-> attach( self, objectHandle);
}

/*::b */
Bool
Widget_begin_paint( Handle self)
{
   Bool ok;
   if ( is_opt( optInDraw)) return false;
   inherited-> begin_paint( self);
   if ( !( ok = apc_widget_begin_paint( self, false)))
      inherited-> end_paint( self);
   return ok;
}

Bool
Widget_begin_paint_info( Handle self)
{
   Bool ok;
   if ( is_opt( optInDraw))     return true;
   if ( is_opt( optInDrawInfo)) return false;
   inherited-> begin_paint_info( self);
   if ( !( ok = apc_widget_begin_paint_info( self)))
      inherited-> end_paint_info( self);
   return ok;
}


void
Widget_bring_to_front( Handle self)
{
   if ( opt_InPaint) return;
   apc_widget_set_z_order( self, nilHandle, true);
}

/*::c */
Bool
Widget_can_close( Handle self)
{
   enter_method;
   Event ev = { cmClose};
   return ( var-> stage <= csNormal) ? my-> message( self, &ev) : true;
}

void
Widget_click( Handle self)
{
   enter_method;
   Event ev = { cmClick};
   my-> message( self, &ev);
}

Bool
Widget_close( Handle self)
{
   Bool canClose;
   enter_method;
   if ( var-> stage > csNormal) return true;
   if (( canClose = my-> can_close( self)))
      Object_destroy( self);
   return canClose;
}

Bool
Widget_custom_paint( Handle self)
{
   PList  list;
   void * ret;
   enter_method;
   if ( my-> on_paint != Widget_on_paint) return true;
   if ( var-> eventIDs == nil) return false;
   ret = hash_fetch( var-> eventIDs, "onPaint", 7);
   if ( ret == nil) return false;
   list = var-> events + ( int) ret - 1;
   return list-> count > 0;
}

/*::d */
void
Widget_detach( Handle self, Handle objectHandle, Bool kill)
{
   enter_method;
   if ( kind_of( objectHandle, CWidget)) {
      list_delete( &var-> widgets, objectHandle);
      if ( var-> currentWidget == objectHandle && objectHandle != nilHandle)
          my-> set_current_widget( self, nilHandle);
   }
   inherited-> detach( self, objectHandle, kill);
}

/*::e */
void
Widget_end_paint( Handle self)
{
  if ( !is_opt( optInDraw)) return;
  apc_widget_end_paint( self);
  inherited-> end_paint( self);
}

void
Widget_end_paint_info( Handle self)
{
  if ( !is_opt( optInDrawInfo)) return;
  apc_widget_end_paint_info( self);
  inherited-> end_paint_info( self);
}


/*::f */
Handle
Widget_first( Handle self)
{
   return apc_widget_get_z_order( self, zoFirst);
}

Handle
Widget_first_that( Handle self, void * actionProc, void * params)
{
   Handle child  = nilHandle;
   int i, count  = var-> widgets. count;
   Handle * list;
   if ( actionProc == nil || count == 0) return nilHandle;
   list = malloc( sizeof( Handle) * count);
   memcpy( list, var-> widgets. items, sizeof( Handle) * count);

   for ( i = 0; i < count; i++)
   {
      if ((( PActionProc) actionProc)( self, list[ i], params))
      {
         child = list[ i];
         break;
      }
   }
   free( list);
   return child;
}

/*::g */
/*::h */

void Widget_handle_event( Handle self, PEvent event)
{
   enter_method;
#define evOK ( var-> evStack[ var-> evPtr - 1])
#define objCheck if ( var-> stage > csNormal) return
   inherited-> handle_event ( self, event);
   objCheck;
   switch ( event-> cmd)
   {
      case cmCalcBounds:
        {
           Point min, max;
           min = my-> get_size_min( self);
           max = my-> get_size_max( self);
           if (( min. x > 0) && ( min. x > event-> gen. R. right  )) event-> gen. R. right  = min. x;
           if (( min. y > 0) && ( min. y > event-> gen. R. top    )) event-> gen. R. top    = min. y;
           if (( max. x > 0) && ( max. x < event-> gen. R. right  )) event-> gen. R. right  = max. x;
           if (( max. y > 0) && ( max. y < event-> gen. R. top    )) event-> gen. R. top    = max. y;
        }
        break;
      case cmSetup:
        if ( !is_opt( optSetupComplete)) {
           opt_set( optSetupComplete);
           my-> notify( self, "<s", "Setup");
        }
        break;
      case cmRepaint:
        my-> repaint( self);
        break;
      case cmPaint        :
        if ( !opt_InPaint && !my-> get_locked( self))
          if ( inherited-> begin_paint( self)) {
             if ( apc_widget_begin_paint( self, true)) {
                my-> notify( self, "<sH", "Paint", self);
                objCheck;
                apc_widget_end_paint( self);
                inherited-> end_paint( self);
             } else
                inherited-> end_paint( self);
          }
        break;
      case cmHelp:
        my-> notify( self, "<s", "Help");
        if ( evOK) my-> help( self);
        break;
      case cmEnable       :
        my-> notify( self, "<s", "Enable");
        break;
      case cmDisable      :
        my-> notify( self, "<s", "Disable");
        break;
      case cmReceiveFocus :
        my-> notify( self, "<s", "Enter");
        break;
      case cmReleaseFocus :
        my-> notify( self, "<s", "Leave");
        break;
      case cmShow         :
        my-> notify( self, "<s", "Show");
        break;
      case cmHide         :
        my-> notify( self, "<s", "Hide");
        break;
      case cmHint:
        my-> notify( self, "<si", "Hint", event-> gen. B);
        break;
      case cmClose        :
        if ( my-> first_that( self, pquery, nil)) {
           my-> clear_event( self);
           return;
        }
        objCheck;
        my-> notify( self, "<s", "Close");
        break;
      case cmZOrderChanged:
        my-> notify( self, "<s", "ZOrderChanged");
        break;
      case cmOK:
      case cmCancel:
        my-> clear_event( self);
        break;
      case cmClick:
        my-> notify( self, "<s", "Click");
        break;
      case cmColorChanged:
        if ( !kind_of( event-> gen. source, CPopup))
            my-> notify( self, "<si", "ColorChanged", event-> gen. i);
        else
            var-> popupColor[ event-> gen. i] =
               apc_menu_get_color( event-> gen. source, event-> gen. i);
        break;
      case cmFontChanged:
        if ( !kind_of( event-> gen. source, CPopup))
           my-> notify( self, "<s", "FontChanged");
        else
           apc_menu_get_font( event-> gen. source, &var-> popupFont);
        break;
      case cmMenu:
         my-> notify( self, "<sHs", "Menu", event-> gen. H, event-> gen. p);
         break;
      case cmMouseClick:
         my-> notify( self, "<siiPi", "MouseClick",
            event-> pos. button, event-> pos. mod, event -> pos. where, event-> pos. dblclk);
         break;
      case cmMouseDown:
         if ((( PApplication) application)-> hintUnder == self)
            my-> set_hint_visible( self, 0);
         objCheck;
         if (((event-> pos. button & var-> selectingButtons) != 0) && my-> get_selectable( self))
            my-> set_selected( self, true);
         objCheck;
         my-> notify( self, "<siiP", "MouseDown",
            event-> pos. button, event-> pos. mod, event -> pos. where);
         break;
      case cmMouseUp:
        my-> notify( self, "<siiP", "MouseUp",
            event-> pos. button, event-> pos. mod, event -> pos. where);
        break;
      case cmMouseMove:
        if ((( PApplication) application)-> hintUnder == self)
           my-> set_hint_visible( self, 1);
        objCheck;
        my-> notify( self, "<siP", "MouseMove", event-> pos. mod, event -> pos. where);
        break;
      case cmMouseWheel:
        my-> notify( self, "<siPi", "MouseWheel",
           event-> pos. mod, event -> pos. where,
           event-> pos. button); /* +n*delta == up, -n*delta == down */
        break;
      case cmMouseEnter:
        my-> notify( self, "<siP", "MouseEnter", event-> pos. mod, event -> pos. where);
        objCheck;
        if ( application && is_opt( optShowHint) && ((( PApplication) application)-> options. optShowHint) && var-> hint[0])
        {
           PApplication app = ( PApplication) application;
           app-> self-> set_hint_action( application, self, true, true);
        }
        break;
      case cmMouseLeave:
        if ( application && is_opt( optShowHint)) {
           PApplication app = ( PApplication) application;
           app-> self-> set_hint_action( application, self, false, true);
        }
        my-> notify( self, "<s", "MouseLeave");
        break;
      case cmKeyDown:
        {
           int i;
           int rep = event-> key. repeat;
           if ( is_opt( optBriefKeys))
              rep = 1;
           else
              event-> key. repeat = 1;
           for ( i = 0; i < rep; i++) {
              my-> notify( self, "<siiii", "KeyDown",
                  event-> key.code, event-> key. key, event-> key. mod, event-> key. repeat);
              objCheck;
              if ( evOK) {
                 int key = CAbstractMenu-> translate_key( nilHandle, event-> key. code, event-> key. key, event-> key. mod);
                 if ( my-> process_accel( self, key)) my-> clear_event( self);
              }
              objCheck;
              if ( evOK && var-> owner) {
                 Event ev = *event;
                 ev. key. source = self;
                 ev. cmd         = cmDelegateKey;
                 ev. key. subcmd = 0;
                 if ( !my-> message( self, &ev)) my-> clear_event( self);
                 objCheck;
              }
              if ( !evOK) break;
           }
        }
        break;
      case cmDelegateKey:
        switch ( event-> key. subcmd)
        {
           case 0: {
              Event ev = *event;
              ev. cmd         = cmTranslateAccel;
              if ( !my-> message( self, &ev)) {
                 my-> clear_event( self);
                 return;
              }
              objCheck;

              if ( my-> first_that( self, accel_notify, &ev)) {
                 my-> clear_event( self);
                 return;
              }
              objCheck;
              ev. cmd         = cmDelegateKey;
              ev. key. subcmd = 1;
              if ( my-> first_that( self, accel_notify, &ev)) {
                 my-> clear_event( self);
                 return;
              }
              if ( var-> owner)
              {
                 if ( var-> owner == application)
                    ev. cmd = cmTranslateAccel;
                 else
                    ev. key. subcmd = 0;
                 ev. key. source = self;
                 if (!(((( PWidget) var-> owner)-> self)-> message( var-> owner, &ev))) {
                    objCheck;
                    my-> clear_event( self);
                 }
              }
           }
           break;
           case 1: {
              Event ev = *event;
              ev. cmd  = cmTranslateAccel;
              if ( my-> first_that( self, accel_notify, &ev)) {
                 my-> clear_event( self);
                 return;
              }
              objCheck;
              ev = *event;
              if ( my-> first_that( self, accel_notify, &ev)) {
                 my-> clear_event( self);
                 return;
              }
           }
           break;
        }
        break;
      case cmTranslateAccel:
        my-> notify( self, "<siii", "TranslateAccel",
            event-> key.code, event-> key. key, event-> key. mod);
        objCheck;
        if ( evOK) {
           if ( event-> key. key == kbLeft      || event-> key. key == kbRight     ||
                event-> key. key == kbUp        || event-> key. key == kbDown
              )
           {
              int dir = ( event-> key. key == kbRight
                       || event-> key. key == kbDown) ? 1 : -1;
              Handle foc = apc_widget_get_focused();
              Handle * list;
              PWidget  next;
              int i, no = -1, count = var-> widgets. count;

              list = var-> widgets. items;
              if ( count)
              {
                 if ( kind_of( foc, CWidget))
                 {
                    for ( i = 0; i < count; i++) if ( list[ i] == foc) { no = i; break; }
                    if ( no < 0) foc = list[ no = 0];
                 } else
                    foc = list[ no = 0];
                 next = ( PWidget) foc;
                 count--;
                 while ( 1)
                 {
                    if ( dir < 0 && no == 0) no = count; else
                    if ( dir > 0 && no == count) no = 0; else
                    no += dir;
                    next = ( PWidget) list[ no];
                    if ( next == ( PWidget) foc) goto out;
                    if ( next-> self-> get_enabled( list[ no]) &&
                         next-> self-> get_tab_stop( list[ no]) &&
                         next-> self-> get_selectable( list[ no])) break;
                 }
                 next-> self-> set_selected(( Handle) next, true);
                 objCheck;
                 my-> clear_event( self);
              }
           out:;
           }
        }
        break;
      case cmKeyUp:
        my-> notify( self, "<siii", "KeyUp",
           event-> key.code, event-> key. key, event-> key. mod);
        break;
      case cmMenuCmd:
        if ( event-> gen. source)
           ((( PAbstractMenu) event-> gen. source)-> self)-> sub_call_id( event-> gen. source, event-> gen. i);
        break;
      case cmMove:
         {
            Bool doNotify = false;
            if ( var-> stage == csNormal) {
               doNotify = true;
            } else if ( var-> stage > csNormal) {
               break;
            } else if ( var-> evQueue != nil) {
              int    i = list_first_that( var-> evQueue, find_dup_msg, (void*) event-> cmd);
              PEvent n;
              if ( i < 0) {
                 n = malloc( sizeof( Event));
                 memcpy( n, event, sizeof( Event));
                 n-> gen. B = 1;
                 n-> gen. R. left = n-> gen. R. bottom = 0;
                 list_add( var-> evQueue, ( Handle) n);
              } else
                 n = ( PEvent) list_at( var-> evQueue, i);
              n-> gen. P = event-> gen. P;
            }
            if ( !event-> gen. B)
               my-> first_that( self, move_notify, &event-> gen. P);
            var-> pos = event-> gen. P;
            if ( doNotify) {
               my-> notify( self, "<sPP", "Move", var-> pos, event-> gen. P);
               objCheck;
               if ( var-> growMode & gmCenter) my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
            }
         }
        break;
      case cmPopup:
        {
           PPopup p;
           my-> notify( self, "<siP", "Popup", event-> gen. B, event-> gen. P. x, event-> gen. P. y);
           objCheck;
           p = ( PPopup) my-> get_popup( self);
           if ( evOK && p && p-> self-> get_auto(( Handle) p))
              p-> self-> popup(( Handle) p, event-> gen. P. x, event-> gen. P. y ,0,0,0,0);
        }
        break;
      case cmSize:
        /* expecting new size in P, old & new size in R. */
        {
           Bool doNotify = false;
           if ( var-> stage == csNormal) {
              doNotify = true;
           } else if ( var-> stage > csNormal) {
              break;
           } else if ( var-> evQueue != nil) {
              int    i = list_first_that( var-> evQueue, find_dup_msg, (void*) event-> cmd);
              PEvent n;
              if ( i < 0) {
                 n = malloc( sizeof( Event));
                 memcpy( n, event, sizeof( Event));
                 n-> gen. B = 1;
                 n-> gen. R. left = n-> gen. R. bottom = 0;
                 list_add( var-> evQueue, ( Handle) n);
              } else
                 n = ( PEvent) list_at( var-> evQueue, i);
              n-> gen. P. x = n-> gen. R. right  = event-> gen. P. x;
              n-> gen. P. y = n-> gen. R. top    = event-> gen. P. y;
           }
           if ( var-> growMode & gmCenter) my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);

           if ( !event-> gen. B) my-> first_that( self, size_notify, &event-> gen. R);
           if ( doNotify) {
              Point oldSize;
	      oldSize. x = event-> gen. R. left;
	      oldSize. y = event-> gen. R. bottom;
              my-> notify( self, "<sPP", "Size", oldSize, event-> gen. P);
           }
        }
        break;
   }
}

Bool
Widget_help( Handle self)
{
   long ctx = var-> helpContext;
   if ( ctx == hmpOwner) {
      PWidget next = ( PWidget) self;
      while ( next && next-> helpContext == hmpOwner) next = ( PWidget) next-> owner;
      ctx = next-> helpContext;
   }
   if ( ctx == hmpNone) return true;
   return apc_help_open_topic( application, ctx);
}

void
Widget_hide( Handle self)
{
   enter_method;
   my-> set_visible( self, false);
}

void
Widget_hide_cursor( Handle self)
{
   enter_method;
   if ( my-> get_cursor_visible( self))
      my-> set_cursor_visible( self, false);
   else
      var-> cursorLock++;
}

/*::i */
void
Widget_insert_behind ( Handle self, Handle widget)
{
   apc_widget_set_z_order( self, widget, 0);
}

void
Widget_invalidate_rect( Handle self, Rect rect)
{
   enter_method;
   if ( !opt_InPaint && ( var-> stage == csNormal) && !my-> get_locked( self))
      apc_widget_invalidate_rect( self, &rect);
}


Bool
Widget_is_child( Handle self, Handle owner)
{
   if ( !owner)
      return false;
   while ( self) {
      if ( self == owner)
         return true;
      self = var-> owner;
   }
   return false;
}

/*::j */
/*::k */
void
Widget_key_event( Handle self, int command, int code, int key, int mod, int repeat, Bool post)
{
   Event ev;
   if ( command != cmKeyDown && command != cmKeyUp) return;
   memset( &ev, 0, sizeof( ev));
   if ( repeat <= 0) repeat = 1;
   ev. cmd = command;
   ev. key. code   = code;
   ev. key. key    = key;
   ev. key. mod    = mod;
   ev. key. repeat = repeat;
   apc_message( self, &ev, post);
}

/*::l */
Handle
Widget_last( Handle self)
{
   return apc_widget_get_z_order( self, zoLast);
}

void
Widget_locate( Handle self, Rect r )
{
   enter_method;
   my-> set_size( self, r. right - r. left, r. top - r. bottom);
   my-> set_pos( self, r. left, r. bottom);
}

Bool
Widget_lock( Handle self)
{
   var-> lockCount++;
   return true;
}

/*::m */
void
Widget_mouse_event( Handle self, int command, int button, int mod, int x, int y, Bool dbl, Bool post)
{
   Event ev;
   if ( command != cmMouseDown
     && command != cmMouseUp
     && command != cmMouseClick
     && command != cmMouseMove
     && command != cmMouseWheel
     && command != cmMouseEnter
     && command != cmMouseLeave
     ) return;
   memset( &ev, 0, sizeof( ev));
   ev. cmd = command;
   ev. pos. where. x = x;
   ev. pos. where. y = y;
   ev. pos. mod    = mod;
   ev. pos. button = button;
   if ( command == cmMouseClick) ev. pos. dblclk = dbl;
   apc_message( self, &ev, post);
}

/*::n */
Handle
Widget_next( Handle self)
{
   return apc_widget_get_z_order( self, zoNext);
}

/*::o */
/*::p */
void
Widget_post_message( Handle self, SV * info1, SV * info2)
{
   PPostMsg p;
   Event ev = { cmPost};
   if ( var-> stage > csNormal) return;
   p = malloc( sizeof( PostMsg));
   p-> info1  = newSVsv( info1);
   p-> info2  = newSVsv( info2);
   p-> h = self;
   if ( var-> postList == nil) list_create( var-> postList = malloc( sizeof( List)), 8, 8);
   list_add( var-> postList, ( Handle) p);
   ev. gen. p = p;
   ev. gen. source = ev. gen. H = self;
   apc_message( self, &ev, true);
}

Handle
Widget_prev( Handle self)
{
   return apc_widget_get_z_order( self, zoPrev);
}

Bool
Widget_process_accel( Handle self, int key)
{
   enter_method;
   if ( my-> first_that_component( self, find_accel, &key)) return true;
   return kind_of( var-> owner, CWidget) ?
          ((( PWidget) var-> owner)-> self)->process_accel( var-> owner, key) : false;
}

/*::q */
/*::r */
void
Widget_repaint( Handle self)
{
   enter_method;
   if ( !opt_InPaint && ( var-> stage == csNormal) && !my-> get_locked( self))
      apc_widget_invalidate_rect( self, nil);
}

/*::s */
void
Widget_scroll( Handle self, int horiz, int vert, Bool scrollChildren)
{
   enter_method;
   if ( !opt_InPaint && ( var-> stage == csNormal) && !my-> get_locked( self))
      apc_widget_scroll( self, horiz, vert, nil, scrollChildren);
}

void
Widget_scroll_rect( Handle self, int horiz, int vert, Rect rect, Bool scrollChildren)
{
   enter_method;
   if ( !opt_InPaint && ( var-> stage == csNormal) && !my-> get_locked( self))
      apc_widget_scroll( self, horiz, vert, &rect, scrollChildren);
}

void
Widget_send_to_back( Handle self)
{
   apc_widget_set_z_order( self, nilHandle, false);
}

void
Widget_set( Handle self, HV * profile)
{
   enter_method;
   Handle postOwner = var-> owner;
   AV *order = nil;

   if ( pexist(__ORDER__)) order = (AV*)SvRV(pget_sv( __ORDER__));

   if ( pexist( owner))
   {
      postOwner = pget_H( owner);
      if ( !kind_of( postOwner, CWidget))
         croak("RTC0081: Illegal object reference passed to Widget::set_owner");
      if ( my-> migrate( self, postOwner))
      {
         if ( postOwner)
         {
            if ( is_opt( optOwnerColor))
            {
               my-> set_color( self, ((( PWidget) postOwner)-> self)-> get_color( postOwner));
               opt_set( optOwnerColor);
            }
            if ( is_opt( optOwnerBackColor))
            {
               my-> set_back_color( self, ((( PWidget) postOwner)-> self)-> get_back_color( postOwner));
               opt_set( optOwnerBackColor);
            }
            if ( is_opt( optOwnerShowHint))
            {
               Bool newSH = ( postOwner == application) ? 1 : ((( PWidget) postOwner)-> self)-> get_show_hint( postOwner);
               my-> set_show_hint( self, newSH);
               opt_set( optOwnerShowHint);
            }
            if ( is_opt( optOwnerHint))
            {
               my-> set_hint( self, ((( PWidget) postOwner)-> self)-> get_hint( postOwner));
               opt_set( optOwnerHint);
            }
            if ( is_opt( optOwnerFont))
            {
               my-> set_font ( self, ((( PWidget) postOwner)-> self)-> get_font( postOwner));
               opt_set( optOwnerFont);
            }
         }
      }
   }
   if ( pexist( origin))
   {
      AV * av = ( AV *) SvRV( pget_sv( origin));
      int set[2];
      if (order && !pexist(left))   av_push( order, newSVpv("left",0));
      if (order && !pexist(bottom)) av_push( order, newSVpv("bottom",0));
      read_point( av, set, 2, "RTC0087: Array panic on 'origin'");
      pset_sv( left,   newSViv(set[0]));
      pset_sv( bottom, newSViv(set[1]));
      pdelete( origin);
   }
   if ( pexist( rect))
   {
      AV * av = ( AV *) SvRV( pget_sv( rect));
      int rect[4];
      if (order && !pexist(left)) av_push( order, newSVpv("left",0));
      if (order && !pexist(bottom)) av_push( order, newSVpv("bottom",0));
      if (order && !pexist(width)) av_push( order, newSVpv("width",0));
      if (order && !pexist(height)) av_push( order, newSVpv("height",0));
      read_point( av, rect, 4, "RTC0088: Array panic on 'rect'");
      pset_sv( left,   newSViv( rect[0]));
      pset_sv( bottom, newSViv( rect[1]));
      pset_sv( width,  newSViv( rect[2] - rect[0]));
      pset_sv( height, newSViv( rect[3]   - rect[1]));
      pdelete( rect);
   }
   if ( pexist( size))
   {
      AV * av = ( AV *) SvRV( pget_sv( size));
      int set[2];
      if (order && !pexist(width)) av_push( order, newSVpv("width",0));
      if (order && !pexist(height)) av_push( order, newSVpv("height",0));
      read_point( av, set, 2, "RTC0089: Array panic on 'size'");
      pset_sv( width,  newSViv(set[0]));
      pset_sv( height, newSViv(set[1]));
      pdelete( size);
   }
   if ( pexist( width) && pexist( right) && pexist( left))  pdelete( right);
   if ( pexist( height) && pexist( top) && pexist( bottom)) pdelete( top);
   if ( pexist( right) && pexist( left))
   {
      int right = pget_i( right);
      if (order && !pexist(width)) av_push( order, newSVpv("width",0));
      pset_i( width, right - pget_i( left));
      pdelete( right);
   }
   if ( pexist( top) && pexist( bottom)) {
      int top = pget_i( top);
      if (order && !pexist(height)) av_push( order, newSVpv("height",0));
      pset_i( height, top - pget_i( bottom));
      pdelete( top);
   }
   if ( pexist( left) && pexist( bottom)) {
      my-> set_pos( self, pget_i( left), pget_i( bottom));
      pdelete( left);
      pdelete( bottom);
   }
   if ( pexist( width) && pexist( height)) {
      my-> set_size( self, pget_i( width), pget_i( height));
      pdelete( width);
      pdelete( height);
   }
   if ( pexist( popupFont))
   {
      SvHV_Font( pget_sv( popupFont), &Font_buffer, "Widget::set");
      my-> set_popup_font( self, Font_buffer);
      pdelete( popupFont);
   }
   if ( pexist( pointerIcon) && pexist( pointerHotSpot))
   {
      Point hotSpot;
      Handle icon = pget_H( pointerIcon);
      read_point(( AV *) SvRV( pget_sv( pointerHotSpot)), (int*)&hotSpot, 2, "RTC0087: Array panic on 'pointerHotSpot'");
      if ( icon != nilHandle && !kind_of( icon, CIcon)) {
         warn("RTC083: Illegal object reference passed to Widget.set_pointer_icon");
         icon = nilHandle;
      }
      apc_pointer_set_user( self, icon, hotSpot);
      if ( var-> pointerType == crUser) my-> first_that( self, sptr, nil);
      pdelete( pointerIcon);
      pdelete( pointerHotSpot);
   }
   if ( pexist( designScale))
   {
      AV * av = ( AV *) SvRV( pget_sv( designScale));
      SV ** holder = av_fetch( av, 0, 0);
      NPoint ds = {1,1};
      ds. x = holder ? SvNV( *holder) : 1;
      if ( !holder) warn("RTC0086: Array panic on 'designScale'");
      holder = av_fetch( av, 1, 0);
      ds. y = holder ? SvNV( *holder) : 1;
      if ( !holder) warn("RTC0086: Array panic on 'designScale'");
      my-> set_design_scale( self, ds.x, ds.y);
      pdelete( designScale);
   }

   inherited-> set( self, profile);
   if ( var-> owner != postOwner)
   {
       var-> owner = postOwner;
       my-> set_tab_order( self, var-> tabOrder);
   }
   if ( var-> growMode & gmCenter) my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
}

void
Widget_setup( Handle self)
{
   enter_method;
   inherited-> setup( self);
   if ( get_top_current( self) &&
        my-> get_enabled( self) &&
        my-> get_visible( self))
      my-> set_selected( self, true);
}

void
Widget_show( Handle self)
{
   enter_method;
   my-> set_visible( self, true);
}

void
Widget_show_cursor( Handle self)
{
   enter_method;
   if ( var-> cursorLock-- <= 0) {
      my-> set_cursor_visible( self, true);
      var-> cursorLock = 0;
   }
}

void
Widget_show_hint( Handle self)
{
   ((( PApplication) application)-> self)-> set_hint_action( application, self, true, true);
}

/*::t */
/*::u */

static Bool
repaint_all( Handle owner, Handle self, void * dummy)
{
   enter_method;
   my-> repaint( self);
   my-> first_that( self, repaint_all, nil);
   return false;
}

Bool
Widget_unlock( Handle self)
{
   if ( --var-> lockCount <= 0) {
      var-> lockCount = 0;
      repaint_all( var-> owner, self, nil);
   }
   return true;
}

void
Widget_update_view( Handle self)
{
   if ( !opt_InPaint) apc_widget_update( self);
}
/*::v */
/*::w */
/*::x */
/*::y */
/*::z */

/* get_props() */

SV *
Widget_get_accel_items( Handle self)
{
   if ( var-> stage > csNormal) return nilSV;
   return var-> accelTable ? ((( PAbstractMenu) var-> accelTable)-> self)-> get_items( var-> accelTable, "") : nilSV;
}

Handle
Widget_get_accel_table( Handle self)
{
   return var-> accelTable;
}

Color
Widget_get_back_color( Handle self)
{
   enter_method;
   return my-> get_color_index( self, ciBack);
}

int
Widget_get_bottom( Handle self)
{
   enter_method;
   return my-> get_pos ( self). y;
}

Bool
Widget_get_brief_keys( Handle self)
{
   return is_opt( optBriefKeys);
}

Bool
Widget_get_buffered( Handle self)
{
   return is_opt( optBuffered);
}

Rect
Widget_get_clip_rect( Handle self)
{
   return opt_InPaint ?
      inherited-> get_clip_rect( self) :
      apc_widget_get_clip_rect( self);

}

Color
Widget_get_color( Handle self)
{
   enter_method;
   return my-> get_color_index( self, ciFore);
}

Color
Widget_get_color_index( Handle self, int index)
{
   if ( index < 0 || index > ciMaxId) return clInvalid;
   switch ( index)
   {
     case ciFore:
        return opt_InPaint ? inherited-> get_color ( self) : apc_widget_get_color( self, ciFore);
     case ciBack:
        return opt_InPaint ? inherited-> get_back_color ( self) : apc_widget_get_color( self, ciBack);
     default:
        return apc_widget_get_color( self, index);
   }
}

Bool
Widget_get_current( Handle self)
{
   return ( var-> owner && ((( PWidget) var-> owner)-> currentWidget == self));
}

Handle
Widget_get_current_widget( Handle self)
{
   return var-> currentWidget;
}

Font
Widget_get_default_font( char * dummy)
{
   Font font;
   apc_widget_default_font( &font);
   return font;
}

Font
Widget_get_default_popup_font( char * dummy)
{
   Font f;
   apc_popup_default_font( &f);
   return f;
}


NPoint
Widget_get_design_scale( Handle self)
{
   return var-> designScale;
}

int
Widget_get_grow_mode( Handle self)
{
   return var-> growMode;
}

SV *
Widget_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_widget_get_handle( self));
   return newSVpv( buf, 0);
}

int
Widget_get_height( Handle self)
{
   enter_method;
   return my-> get_size( self). y;
}

long int
Widget_get_help_context( Handle self)
{
  return var-> helpContext;
}

char *
Widget_get_hint( Handle self)
{
   return var-> hint ? var-> hint : "";
}

Bool
Widget_get_hint_visible( Handle self)
{
   PApplication app = ( PApplication) application;
   return app-> hintVisible;
}

int
Widget_get_left( Handle self)
{
   enter_method;
   return my-> get_pos( self). x;
}

Bool
Widget_get_locked( Handle self)
{
   while ( self) {
      if ( var-> lockCount != 0) return true;
      self = var-> owner;
   }
   return false;
}


Bool Widget_get_owner_color( Handle self)      { return is_opt( optOwnerColor); }
Bool Widget_get_owner_back_color( Handle self) { return is_opt( optOwnerBackColor); }
Bool Widget_get_owner_font( Handle self)       { return is_opt( optOwnerFont); }
Bool Widget_get_owner_hint( Handle self)       { return is_opt( optOwnerHint); }
Bool Widget_get_owner_show_hint( Handle self)  { return is_opt( optOwnerShowHint); }
Bool Widget_get_owner_palette( Handle self)  { return is_opt( optOwnerPalette); }


Handle
Widget_get_parent( Handle self)
{
   enter_method;
   return my-> get_clip_owner( self) ? var-> owner : application;
}

Handle
Widget_get_pointer_icon( Handle self)
{
   if ( var-> stage > csNormal) return nilHandle;
   {
      HV * profile = newHV();
      Handle icon = Object_create( "Prima::Icon", profile);
      sv_free(( SV *) profile);
      apc_pointer_get_bitmap( self, icon);
      --SvREFCNT( SvRV((( PAnyObject) icon)-> mate));
      return icon;
   }
}

Point
Widget_get_pointer_pos( Handle self)
{
   enter_method;
   return my-> screen_to_client( self, apc_pointer_get_pos( self));
}

Point
Widget_get_pointer_size( char*dummy)
{
   return apc_pointer_get_size( nilHandle);
}

int
Widget_get_pointer_type( Handle self)
{
   return var-> pointerType;
}

Handle
Widget_get_popup( Handle self)
{
   return var-> popupMenu;
}

Color
Widget_get_popup_color( Handle self, int index)
{
   index = (( index < 0) || ( index > ciMaxId)) ? 0 : index;
   return  var-> popupColor[ index];
}

Font
Widget_get_popup_font( Handle self)
{
   return var-> popupFont;
}

SV *
Widget_get_popup_items( Handle self)
{
   return var-> popupMenu ? ((( PAbstractMenu) var-> popupMenu)-> self)-> get_items( var-> popupMenu, "") : nilSV;
}

Rect
Widget_get_rect( Handle self)
{
   enter_method;
   Point p   = my-> get_pos( self);
   Point s   = my-> get_size( self);
   Rect r;
   r. left = p. x;
   r. bottom = p. y;
   r. right = p. x + s. x;
   r. top = p. y + s. y;
   return r;
}

int
Widget_get_right( Handle self)
{
   enter_method;
   Rect r = my-> get_rect( self);
   return r. right;
}

Bool
Widget_get_scale_children( Handle self)
{
   return is_opt( optScaleChildren);
}


Bool
Widget_get_selectable( Handle self)
{
   return is_opt( optSelectable);
}

Handle
Widget_get_selectee( Handle self)
{
   if ( var-> stage > csNormal) return nilHandle;
   if ( var-> currentWidget) {
      PWidget w = ( PWidget) var-> currentWidget;
      if ( w-> options. optSystemSelectable && !w-> self-> get_clip_owner(( Handle) w))
         return ( Handle) w;
      else
         return w-> self-> get_selectee(( Handle) w);
   } else
   if ( is_opt( optSelectable) || is_opt( optSystemSelectable))
      return self;
   else
      return find_tabfoc( self);
}

Bool
Widget_get_selected( Handle self)
{
   enter_method;
   return my-> get_selected_widget( self) != nilHandle;
}

Handle
Widget_get_selected_widget( Handle self)
{
   if ( var-> stage <= csNormal) {
      Handle foc = apc_widget_get_focused();
      PWidget  f = ( PWidget) foc;
      while( f) {
         if (( Handle) f == self) return foc;
         f = ( PWidget) f-> owner;
      }
   }
   return nilHandle;

   /* classic solution should be recursive and inheritant call */
   /* of get_selected() here, when Widget would return state of */
   /* child-group selected state until Widget::get_selected() called; */
   /* thus, each of them would call apc_widget_get_focused - that's expensive, */
   /* so that's the reason not to use classic object model here. */
}

int
Widget_get_selecting_buttons( Handle self)
{
   return var-> selectingButtons;
}


Handle
Widget_get_shape( Handle self)
{
   if ( var-> stage > csNormal) return nilHandle;
   if ( apc_widget_get_shape( self, nilHandle)) {
      HV * profile = newHV();
      Handle i = Object_create( "Prima::Image", profile);
      sv_free(( SV *) profile);
      apc_widget_get_shape( self, i);
      --SvREFCNT( SvRV((( PAnyObject) i)-> mate));
      return i;
   } else
      return nilHandle;
}


Bool
Widget_get_show_hint( Handle self)
{
   return is_opt( optShowHint);
}

Point
Widget_get_size_max( Handle self)
{
   return var-> sizeMax;
}

Point
Widget_get_size_min( Handle self)
{
   return var-> sizeMin;
}

int
Widget_get_tab_order( Handle self)
{
   return var-> tabOrder;
}

Bool
Widget_get_tab_stop( Handle self)
{
   return is_opt( optTabStop);
}

char *
Widget_get_text( Handle self)
{
   return var-> text ? var-> text : "";
}

int
Widget_get_top( Handle self)
{
   enter_method;
   Rect r = my-> get_rect( self);
   return r. top;
}

Point
Widget_get_virtual_size( Handle self)
{
   return var-> virtualSize;
}

int
Widget_get_widget_class( Handle self)
{
   return var-> widgetClass;
}

int
Widget_get_width( Handle self)
{
   enter_method;
   return my-> get_size ( self). x;
}

/* set_props() */

void
Widget_set_accel_items( Handle self, SV * accelItems)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( var-> accelTable == nilHandle)
   {
      HV * profile = newHV();
      if ( SvTYPE( accelItems)) pset_sv( items, accelItems);
      pset_H ( owner, self);
      my-> set_accel_table( self, create_instance( "Prima::AccelTable"));
      sv_free(( SV *) profile);
   }
   else
      ((( PAbstractMenu) var-> accelTable)-> self)-> set_items( var-> accelTable, accelItems);
}

void
Widget_set_accel_table( Handle self, Handle accelTable)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( accelTable && !kind_of( accelTable, CAbstractMenu)) return;
   if ( accelTable && (( PAbstractMenu) accelTable)-> owner != self)
     my-> set_accel_items( self, ((( PAbstractMenu) accelTable)-> self)-> get_items( accelTable, ""));
   else var-> accelTable = accelTable;
}

Bool
Widget_set_back_color( Handle self, Color color)
{
   enter_method;
   my-> set_color_index( self, color, ciBack);
   return true;
}

void
Widget_set_bottom( Handle self, int _bottom )
{
   enter_method;
   int _l = my-> get_left ( self);
   my-> set_pos ( self, _l, _bottom);
}

void
Widget_set_brief_keys( Handle self, Bool briefKeys)
{
   opt_assign( optBriefKeys, briefKeys);
}

void
Widget_set_buffered( Handle self, Bool buffered)
{
   if ( opt_InPaint) return;
   opt_assign( optBuffered, buffered);
}

void
Widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
   if ( opt_InPaint) return;
   apc_widget_set_capture( self, capture, confineTo);
}

void
Widget_set_centered( Handle self, Bool x, Bool y)
{
   enter_method;
   Handle parent = my-> get_parent( self);
   Point size    = CWidget( parent)-> get_size( parent);
   Point mysize  = my-> get_size ( self);
   Point mypos   = my-> get_pos( self);
   if ( x) mypos. x = ( size. x - mysize. x) / 2;
   if ( y) mypos. y = ( size. y - mysize. y) / 2;
   my-> set_pos( self, mypos. x, mypos. y);
}

Bool
Widget_set_clip_rect( Handle self, Rect clipRect)
{
   if ( opt_InPaint)
      inherited-> set_clip_rect( self, clipRect);
   else
      apc_widget_set_clip_rect( self, clipRect);
   return true;
}

Bool
Widget_set_color( Handle self, Color color)
{
   enter_method;
   my-> set_color_index( self, color, ciFore);
   return true;
}

void
Widget_set_color_index( Handle self, Color color, int index)
{
   enter_method;
   SingleColor s;
   s. color = color;
   s. index = index;
   if (( index < 0) || ( index > ciMaxId)) return;
   if ( !opt_InPaint) my-> first_that( self, single_color_notify, &s);

   if ( var-> handle == nilHandle) return; /* aware of call from Drawable::init */
   if (( color < 0) && (( color & wcMask) == 0)) color |= var-> widgetClass;
   if ( opt_InPaint)
   {
      switch ( index)
      {
         case ciFore:
            inherited-> set_color ( self, color);
            break;
         case ciBack:
            inherited-> set_back_color ( self, color);
            break;
         default:
            apc_widget_set_color ( self, color, index);
      }
   } else {
      switch ( index)
      {
         case ciFore:
            opt_clear( optOwnerColor);
            break;
         case ciBack:
            opt_clear( optOwnerBackColor);
            break;
      }
      apc_widget_set_color( self, color, index);
      my-> repaint( self);
   }
}

void
Widget_set_current( Handle self, Bool current)
{
   PWidget o;
   o = ( PWidget) var-> owner;
   if ( o == nil) return;
   if ( var-> stage > csNormal) return;
   if ( current)
      o-> self-> set_current_widget( var-> owner, self);
   else
      if ( o-> currentWidget == self)
         o-> self-> set_current_widget( var-> owner, nilHandle);
}

void
Widget_set_current_widget( Handle self, Handle widget)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( widget) {
      if ( !widget || (( PWidget) widget)-> stage > csNormal ||
         ((( PWidget) widget)-> owner != self)) return;
      var-> currentWidget = widget;
   } else {
      var-> currentWidget = nilHandle;
   }

   /* adjust selection if we're in currently selected chain */
   if ( my-> get_selected( self))
      my-> set_selected_widget( self, widget);
}

void
Widget_set_design_scale( Handle self, double x, double y)
{
   if ( x < 0) x = 0;
   if ( y < 0) y = 0;
   var-> designScale. x = x;
   var-> designScale. y = y;
}

void
Widget_set_focused( Handle self, Bool focused)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( focused) {
      PWidget x = ( PWidget)( var-> owner);
      Handle current = self;
      while ( x) {
         x-> currentWidget = current;
         current = ( Handle) x;
         x = ( PWidget) x-> owner;
      }
      var-> currentWidget = nilHandle;
      if ( var-> stage == csNormal)
         apc_widget_set_focused( self);
   } else
      if ( var-> stage == csNormal && my-> get_selected( self))
         apc_widget_set_focused( nilHandle);
}

void
Widget_set_font( Handle self, Font font)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( !opt_InPaint) my-> first_that( self, font_notify, &font);
   if ( var-> handle == nilHandle) return; /* aware of call from Drawable::init */
   apc_font_pick( self, &font, & var-> font);
   if ( opt_InPaint) apc_gp_set_font ( self, & var-> font);
   else {
      opt_clear( optOwnerFont);
      apc_widget_set_font( self, & var-> font);
      my-> repaint( self);
   }
}

void
Widget_set_grow_mode( Handle self, int flags )
{
   enter_method;
   Bool x = 0, y = 0;
   var-> growMode = flags;
   if ( var-> growMode & gmXCenter) x = 1;
   if ( var-> growMode & gmYCenter) y = 1;
   if ( x || y) my-> set_centered( self, x, y);
}

void
Widget_set_height( Handle self, int _height )
{
   enter_method;
   int _w = my-> get_width ( self);
   my-> set_size ( self, _w, _height);
}

void
Widget_set_help_context( Handle self, long int helpContext )
{
   var-> helpContext = helpContext;
}

void
Widget_set_hint( Handle self, char * hint)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   my-> first_that( self, hint_notify, (void*)hint);

   free( var-> hint);
   if ( hint)
   {
      var-> hint = malloc( strlen( hint) + 1);
      strcpy( var-> hint, hint);
   } else {
      var-> hint = malloc(1);
      var-> hint[0] = 0;
   };
   if ( application && (( PApplication) application)-> hintVisible &&
        (( PApplication) application)-> hintUnder == self)
   {
      if ( strlen( var-> hint) == 0) my-> set_hint_visible( self, 0);
      {
         char * hintText = var-> hint;
         Handle self = (( PApplication) application)-> hintWidget;
         enter_method;
         if ( self)
            my-> set_text( self, hintText);
      }
   }
   opt_clear( optOwnerHint);
}

void
Widget_set_hint_visible( Handle self, Bool visible)
{
   PApplication app = ( PApplication) application;
   if ( var-> stage >= csDead) return;
   if ( visible == app-> hintVisible) return;
   if ( visible && strlen( var-> hint) == 0) return;
   app-> self-> set_hint_action( application, self, visible, false);
}

void
Widget_set_left( Handle self, int _left )
{
   enter_method;
   int _t = my-> get_bottom ( self);
   my-> set_pos( self, _left, _t);
}

void
Widget_set_owner_back_color( Handle self, Bool ownerBackColor )
{
   enter_method;
   opt_assign( optOwnerBackColor, ownerBackColor);
   if ( is_opt( optOwnerBackColor) && var-> owner)
   {
      my-> set_back_color( self, ((( PWidget) var-> owner)-> self)-> get_back_color( var-> owner));
      opt_set( optOwnerBackColor);
      my-> repaint ( self);
   }
}

void
Widget_set_owner_color( Handle self, Bool ownerColor )
{
   enter_method;
   opt_assign( optOwnerColor, ownerColor);
   if ( is_opt( optOwnerColor) && var-> owner)
   {
      my-> set_color( self, ((( PWidget) var-> owner)-> self)-> get_color( var-> owner));
      opt_set( optOwnerColor);
      my-> repaint( self);
   }
}

void
Widget_set_owner_font( Handle self, Bool ownerFont )
{
   enter_method;
   opt_assign( optOwnerFont, ownerFont);
   if ( is_opt( optOwnerFont) && var-> owner)
   {
      my-> set_font ( self, ((( PWidget) var-> owner)-> self)-> get_font ( var-> owner));
      opt_set( optOwnerFont);
      my-> repaint ( self);
   }
}

void
Widget_set_owner_hint( Handle self, Bool ownerHint )
{
   enter_method;
   opt_assign( optOwnerHint, ownerHint);
   if ( is_opt( optOwnerHint) && var-> owner)
   {
      my-> set_hint( self, ((( PWidget) var-> owner)-> self)-> get_hint ( var-> owner));
      opt_set( optOwnerHint);
   }
}

void
Widget_set_owner_palette( Handle self, Bool ownerPalette)
{
   enter_method;
   if ( ownerPalette) my-> set_palette( self, nilSV);
   opt_assign( optOwnerPalette, ownerPalette);
}

void
Widget_set_owner_show_hint( Handle self, Bool ownerShowHint )
{
   enter_method;
   opt_assign( optOwnerShowHint, ownerShowHint);
   if ( is_opt( optOwnerShowHint) && var-> owner)
   {
      my-> set_show_hint( self, ((( PWidget) var-> owner)-> self)-> get_show_hint ( var-> owner));
      opt_set( optOwnerShowHint);
   }
}

void
Widget_set_palette( Handle self, SV * palette)
{
   int oclrs = var-> palSize;
   if ( var-> stage > csNormal) return;
   if ( var-> handle == nilHandle) return; /* aware of call from Drawable::init */
   free( var-> palette);
   var-> palette = read_palette( &var-> palSize, palette);
   opt_clear( optOwnerPalette);
   if ( oclrs == 0 && var-> palSize == 0)
      return; /* do not bother apc */
   if ( opt_InPaint)
      apc_gp_set_palette( self);
   else
      apc_widget_set_palette( self);
}

void
Widget_set_pointer_icon( Handle self, Handle icon)
{
   enter_method;
   Point hotSpot;
   if ( var-> stage > csNormal) return;
   if ( icon != nilHandle && !kind_of( icon, CIcon)) {
      warn("RTC083: Illegal object reference passed to Widget.set_pointer_icon");
      return;
   }
   hotSpot = my-> get_pointer_hot_spot( self);
   apc_pointer_set_user( self, icon, hotSpot);
   if ( var-> pointerType == crUser) my-> first_that( self, sptr, nil);
}

void
Widget_set_pointer_hot_spot( Handle self, int x, int y)
{
   enter_method;
   Handle icon;
   Point hotSpot;
   hotSpot. x = x;
   hotSpot. y =  y;
   if ( var-> stage > csNormal) return;
   icon = my-> get_pointer_icon( self);
   apc_pointer_set_user( self, icon, hotSpot);
   if ( var-> pointerType == crUser) my-> first_that( self, sptr, nil);
}

void
Widget_set_pointer_type( Handle self, int type)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   var-> pointerType = type;
   apc_pointer_set_shape( self, type);
   my-> first_that( self, sptr, nil);
}

void
Widget_set_pointer_pos( Handle self, int x, int y)
{
   enter_method;
   Point p;
   p. x = x;
   p. y = y;
   p = my-> client_to_screen( self, p);
   apc_pointer_set_pos( self, p. x, p. y);
}

void
Widget_set_popup( Handle self, Handle popup)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( popup && !kind_of( popup, CPopup)) return;
   if ( popup && (( PAbstractMenu) popup)-> owner != self)
      my-> set_popup_items( self, ((( PAbstractMenu) popup)-> self)-> get_items( popup, ""));
   else var-> popupMenu = popup;
}

void Widget_set_popup_color( Handle self, Color color, int index)
{
   if (( index < 0) || ( index > ciMaxId)) return;
   if (( color < 0) && (( color & wcMask) == 0)) color |= wcPopup;
   var-> popupColor[ index] = color;
}

void
Widget_set_popup_font( Handle self, Font font)
{
   apc_font_pick( self, &font, &var-> popupFont);
}

void
Widget_set_popup_items( Handle self, SV * popupItems)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( var-> popupMenu == nilHandle)
   {
     if ( SvTYPE( popupItems))
     {
         HV * profile = newHV();
         pset_sv( items, popupItems);
         pset_H ( owner, self);
         my-> set_popup( self, create_instance( "Prima::Popup"));
         sv_free(( SV *) profile);
      }
   }
   else
      ((( PAbstractMenu) var-> popupMenu)-> self)-> set_items( var-> popupMenu, popupItems);
}

void
Widget_set_rect( Handle self, Rect r)
{
   enter_method;
   my-> set_size( self, r. right - r. left, r. top - r. bottom);
   my-> set_pos ( self, r. left, r. bottom);
}

void
Widget_set_right( Handle self, int _right )
{
   enter_method;
   Rect r = my-> get_rect ( self);
   my-> set_pos ( self, r. left - r. right + _right, r. bottom);
}

void
Widget_set_scale_children( Handle self, Bool scaleChildren)
{
   opt_assign( optScaleChildren, scaleChildren);
}

void
Widget_set_selectable( Handle self, Bool selectable)
{
   opt_assign( optSelectable, selectable);
}

void
Widget_set_selected( Handle self, Bool selected)
{
   enter_method;
   if ( var-> stage > csNormal) return;
   if ( selected) {
      if ( var-> currentWidget) {
         PWidget w = ( PWidget) var-> currentWidget;
         if ( w-> options. optSystemSelectable && !w-> self-> get_clip_owner(( Handle) w))
            w-> self-> bring_to_front(( Handle) w); /* <- very uncertain !!!! */
         else
            w-> self-> set_selected(( Handle) w, true);
      } else
      if ( is_opt( optSelectable)) {
         my-> set_focused( self, true);
      } else
      if ( is_opt( optSystemSelectable)) {
         /* nothing to do with Widget, reserved for Window */
      }
      else {
         PWidget toFocus = ( PWidget) find_tabfoc( self);
         if ( toFocus)
            toFocus-> self-> set_selected(( Handle) toFocus, 1);
         else {
         /* if group has no selectable widgets and cannot be selected by itself, */
         /* process chain of bring_to_front(), followed by set_focused(1) call, if available */
            PWidget x = ( PWidget) var-> owner;
            List  lst;
            int i;

            list_create( &lst, 8, 8);
            while ( x) {
               if ( !toFocus && x-> options. optSelectable) {
                  toFocus = x;  /* choose closest owner to focus */
                  break;
               }
               if (( Handle) x != application && !kind_of(( Handle) x, CWindow))
                  list_insert_at( &lst, ( Handle) x, 0);
               x = ( PWidget) x-> owner;
            }

            if ( toFocus)
               toFocus-> self-> set_focused(( Handle) toFocus, 1);

            for ( i = 0; i < lst. count; i++) {
               PWidget v = ( PWidget) list_at( &lst, i);
               v-> self-> bring_to_front(( Handle) v);
            }
            list_destroy( &lst);
         }
      } /* end set_selected( true); */
   } else
      my-> set_focused( self, false);
}

void
Widget_set_selected_widget( Handle self, Handle widget)
{
   if ( var-> stage > csNormal) return;
   if ( widget) {
      if ((( PWidget) widget)-> owner == self)
         ((( PWidget) widget)-> self)-> set_selected( widget, true);
   } else {
      /* give selection up to hierarchy chain */
      Handle s = self;
      while ( s) {
         if ( CWidget( s)-> get_selectable( s)) {
            CWidget( s)-> set_selected( s, true);
            break;
         }
         s = PWidget( s)-> owner;
      }
   }
}

void
Widget_set_selecting_buttons( Handle self, int sb)
{
   var-> selectingButtons = sb;
}

void
Widget_set_show_hint( Handle self, Bool showHint )
{
   enter_method;
   Bool oldShowHint = is_opt( optShowHint);
   my-> first_that( self, showhint_notify, &showHint);
   opt_clear( optOwnerShowHint);
   opt_assign( optShowHint, showHint);
   if ( application && !is_opt( optShowHint) && oldShowHint) my-> set_hint_visible( self, 0);
}

void
Widget_set_shape( Handle self, Handle mask)
{
   if ( mask && !kind_of( mask, CImage)) {
      warn("RTC008A: Illegal object reference passed to Widget.set_shape");
      return;
   }

   if ( mask && (( PImage( mask)-> type & imBPP) != imbpp1)) {
      Handle i = CImage( mask)-> dup( mask);
      ++SvREFCNT( SvRV( PImage( i)-> mate));
      CImage( i)-> set_conversion( i, ictNone);
      CImage( i)-> set_type( i, imBW);
      apc_widget_set_shape( self, i);
      --SvREFCNT( SvRV( PImage( i)-> mate));
      Object_destroy( i);
   } else
      apc_widget_set_shape( self, mask);
}


void
Widget_set_size_min( Handle self, Point min)
{
   enter_method;
   var-> sizeMin = min;
   if ( var-> stage == csNormal)
   {
      Point sizeActual  = my-> get_size( self);
      Point newSize     = sizeActual;
      if ( sizeActual. x < min. x) newSize. x = min. x;
      if ( sizeActual. y < min. y) newSize. y = min. y;
      if (( newSize. x != sizeActual. x) || ( newSize. y != sizeActual. y))
         my-> set_size( self, newSize. x, newSize. y);
   }
}

void
Widget_set_size_max( Handle self, Point max)
{
   enter_method;
   var-> sizeMax = max;
   if ( var-> stage == csNormal)
   {
      Point sizeActual  = my-> get_size( self);
      Point newSize     = sizeActual;
      if ( sizeActual. x > max. x) newSize. x = max. x;
      if ( sizeActual. y > max. y) newSize. y = max. y;
      if (( newSize. x != sizeActual. x) || ( newSize. y !=  sizeActual. y))
          my-> set_size( self, newSize. x, newSize. y);
   }
}

void
Widget_set_tab_order( Handle self, int tabOrder)
{
    int count;
    PWidget owner = ( PWidget) var-> owner;
    if ( var-> stage > csNormal) return;
    count = owner-> widgets. count;
    if ( tabOrder < 0) {
       int i, maxOrder = -1;
       /* finding maximal tabOrder value among the siblings */
       for ( i = 0; i < count; i++) {
          PWidget ctrl = ( PWidget) owner-> widgets. items[ i];
          if ( self == ( Handle) ctrl) continue;
          if ( maxOrder < ctrl-> tabOrder) maxOrder = ctrl-> tabOrder;
       }
       if ( maxOrder < INT_MAX) {
          var-> tabOrder = maxOrder + 1;
          return;
       }
       /* maximal value found, but has no use; finding gaps */
       {
          int j = 0;
          Bool match = 1;
          while ( !match) {
             for ( i = 0; i < count; i++) {
                PWidget ctrl = ( PWidget) owner-> widgets. items[ i];
                if ( self == ( Handle) ctrl) continue;
                if ( ctrl-> tabOrder == j) {
                   match = 1;
                   break;
                }
             }
             j++;
          }
          var-> tabOrder = j - 1;
       }
    } else {
       int i;
       Bool match = 0;
       /* finding exact match among the siblings */
       for ( i = 0; i < count; i++) {
          PWidget ctrl = ( PWidget) owner-> widgets. items[ i];
          if ( self == ( Handle) ctrl) continue;
          if ( ctrl-> tabOrder == tabOrder) {
             match = 1;
             break;
          }
       }
       if ( match)
          /* incrementing all tabOrders that greater than out */
          for ( i = 0; i < count; i++) {
             PWidget ctrl = ( PWidget) owner-> widgets. items[ i];
             if ( self == ( Handle) ctrl) continue;
             if ( ctrl-> tabOrder >= tabOrder) ctrl-> tabOrder++;
          }
       var-> tabOrder = tabOrder;
    }
}

void
Widget_set_tab_stop( Handle self, Bool stop)
{
   opt_assign( optTabStop, stop);
}

void
Widget_set_text( Handle self, char * text)
{
   if ( var-> stage > csNormal) return;
   free( var-> text);
   if ( text)
   {
      var-> text = malloc( strlen( text) + 1);
      strcpy( var-> text, text);
   } else {
      var-> text = malloc(1);
      var-> text[0] = 0;
   }
}

void
Widget_set_top( Handle self, int _top )
{
   enter_method;
   Rect r = my-> get_rect ( self);
   my-> set_pos( self, r. left, r. bottom - r. top + _top);
}

void
Widget_set_widget_class( Handle self, int widgetClass)
{
   enter_method;
   var-> widgetClass = widgetClass;
   my-> repaint( self);
}

void
Widget_set_width( Handle self, int _width )
{
   enter_method;
   int _h = my-> get_height( self);
   my-> set_size (  self, _width, _h);
}

/* event handlers */

void
Widget_on_paint( Handle self, Handle canvas)
{
   enter_method;
   PDrawable c = ( PDrawable) canvas;
   Point size;
   size. x = c-> self-> get_width( canvas);
   size. y = c-> self-> get_height( canvas);
   c-> self-> set_color( canvas, my-> get_back_color( self));
   c-> self-> bar( canvas, 0, 0, size. x - 1, size. y - 1);
}

void Widget_on_click( Handle self) {}
void Widget_on_change( Handle self) {}
void Widget_on_close( Handle self) {}
void Widget_on_colorchanged( Handle self, int colorIndex){}
void Widget_on_disable( Handle self) {}
void Widget_on_dragdrop( Handle self, Handle source , int x , int y ) {}
void Widget_on_dragover( Handle self, Handle source , int x , int y , int state ) {}
void Widget_on_enable( Handle self) {}
void Widget_on_enddrag( Handle self, Handle target , int x , int y ) {}
void Widget_on_fontchanged( Handle self) {}
void Widget_on_enter( Handle self) {}
void Widget_on_help( Handle self) {}
void Widget_on_keydown( Handle self, int code , int key , int shiftState, int repeat ) {}
void Widget_on_keyup( Handle self, int code , int key , int shiftState ) {}
void Widget_on_menu( Handle self, Handle menu, char * variable) {}
void Widget_on_setup( Handle self) {}
void Widget_on_size( Handle self, Point oldSize, Point newSize) {}
void Widget_on_move( Handle self, Point oldPos, Point newPos) {}
void Widget_on_show( Handle self) {}
void Widget_on_hide( Handle self) {}
void Widget_on_hint( Handle self, Bool show) {}
void Widget_on_translateaccel( Handle self, int code , int key , int shiftState ) {}
void Widget_on_zorderchanged( Handle self) {}
void Widget_on_popup( Handle self, Bool mouseDriven, int x, int y) {}
void Widget_on_mouseclick( Handle self, int button , int shiftState , int x , int y , Bool dbl ) {}
void Widget_on_mousedown( Handle self, int button , int shiftState , int x , int y ) {}
void Widget_on_mouseup( Handle self, int button , int shiftState , int x , int y ) {}
void Widget_on_mousemove( Handle self, int shiftState , int x , int y ) {}
void Widget_on_mousewheel( Handle self, int shiftState , int x , int y, int z ) {}
void Widget_on_mouseenter( Handle self, int shiftState , int x , int y ) {}
void Widget_on_mouseleave( Handle self ) {}
void Widget_on_leave( Handle self) {}


/* static iterators */
Bool kill_all( Handle self, Handle child, void * dummy)
{
   Object_destroy( child); return 0;
}

static Bool find_dup_msg( PEvent event, int cmd) { return event-> cmd == cmd; }

Bool
accel_notify ( Handle group, Handle self, PEvent event)
{
   enter_method;
   if (( self != event-> key. source) && my-> get_enabled( self))
      return ( var-> stage <= csNormal) ? !my-> message( self, event) : false;
   else
      return false;
}

static Bool
pquery ( Handle window, Handle self, void * v)
{
   enter_method;
   Event ev = {cmClose};
   return ( var-> stage <= csNormal) ? !my-> message( self, &ev) : false;
}

Bool
find_accel( Handle self, Handle item, int * key)
{
   return ( kind_of( item, CAbstractMenu)
            && CAbstractMenu(item)-> sub_call_key( item, *key));
}

static Handle
find_tabfoc( Handle self)
{
   int i;
   Handle toRet;
   for ( i = 0; i < var-> widgets. count; i++) {
      PWidget w = ( PWidget)( var-> widgets. items[ i]);
      if (
           w-> self-> get_selectable(( Handle) w) &&
           w-> self-> get_enabled(( Handle) w)
         )
         return ( Handle) w;
   }
   for ( i = 0; i < var-> widgets. count; i++)
      if (( toRet = find_tabfoc( var-> widgets. items[ i])))
         return toRet;
   return nilHandle;
}


static Bool
get_top_current( Handle self)
{
   PWidget o  = ( PWidget) var-> owner;
   Handle  me = self;
   while ( o) {
      if ( o-> currentWidget != me)
         return false;
      me = ( Handle) o;
      o  = ( PWidget) o-> owner;
   }
   return true;
}

static Bool
sptr( Handle window, Handle self, void * v)
{
   enter_method;
   /* does nothing but refreshes system pointer */
   if ( var-> pointerType == crDefault)
      my-> set_pointer_type( self, crDefault);
   return false;
}

/* static iterators for ownership notifications */

static Bool
size_notify( Handle self, Handle child, const Rect* metrix)
{
   if ( his-> growMode)
   {
#if 0
      Point size  =  his-> virtualSize;
      Point reportedSize  =  his-> self-> get_size( child);
      Point pos   =  his-> self-> get_pos( child);
      int   dx    = metrix-> right - metrix-> left;
      int   dy    = metrix-> top   - metrix-> bottom;

      size = reportedSize;

      if ( !metrix-> left)   dx = 0;
      if ( !metrix-> bottom) dy = 0;

      printf( ">size_notify: %s, pos: %dx%d, size: %dx%d, delta: %dx%d\n",
	      his-> name, pos. x, pos.y, size. x, size. y, dx, dy);

      if ( his-> growMode & gmGrowLoX) pos.  x += dx;
      if ( his-> growMode & gmGrowHiX && dx) size. x += dx; else size. x = reportedSize. x;
      if ( his-> growMode & gmGrowLoY) pos.  y += dy;
      if ( his-> growMode & gmGrowHiY && dy) size. y += dy; else size. y = reportedSize. y;
      if ( his-> growMode & gmXCenter) pos. x = (((Rect *) metrix)-> right - size. x) / 2;
      if ( his-> growMode & gmYCenter) pos. y = (((Rect *) metrix)-> top   - size. y) / 2;

      printf( "size_notify>: %s, pos: %dx%d, size: %dx%d\n",
	      his-> name, pos. x, pos.y, size. x, size. y);

      his-> self-> set_pos ( child, pos.x, pos. y);
      his-> self-> set_size( child, size.x, size. y);
      his-> virtualSize = size;
#else
      Point size  =  his-> self-> get_virtual_size( child);
      Point pos   =  his-> self-> get_pos( child);
      int   dx    = ((Rect *) metrix)-> right - ((Rect *) metrix)-> left;
      int   dy    = ((Rect *) metrix)-> top   - ((Rect *) metrix)-> bottom;

      if ( his-> growMode & gmGrowLoX) pos.  x += dx;
      if ( his-> growMode & gmGrowHiX) size. x += dx;
      if ( his-> growMode & gmGrowLoY) pos.  y += dy;
      if ( his-> growMode & gmGrowHiY) size. y += dy;
      if ( his-> growMode & gmXCenter) pos. x = (((Rect *) metrix)-> right - size. x) / 2;
      if ( his-> growMode & gmYCenter) pos. y = (((Rect *) metrix)-> top   - size. y) / 2;

      if ( dx != 0 || dy != 0 || ( his-> growMode & gmCenter) != 0)
         his-> self-> set_pos  ( child, pos.x, pos. y);
      if ( dx != 0 || dy != 0)
         his-> self-> set_size ( child, size.x, size. y);
#endif
   }
   return false;
}

static Bool
move_notify( Handle self, Handle child, Point * moveTo)
{
   Bool clp = his-> self-> get_clip_owner( child);
   int  dx  = moveTo-> x - var-> pos. x;
   int  dy  = moveTo-> y - var-> pos. y;
   Point p;

   if ( his-> growMode & gmDontCare) {
      if ( !clp) return false;
      p = his-> self-> get_pos( child);
      his-> self-> set_pos( child, p. x - dx, p. y - dy);
   } else {
      if ( clp) return false;
      p = his-> self-> get_pos( child);
      his-> self-> set_pos( child, p. x + dx, p. y + dy);
   }

   return false;
}


Bool
font_notify ( Handle self, Handle child, void * font)
{
   if ( his-> options. optOwnerFont) {
      his-> self-> set_font ( child, *(( PFont) font));
      his-> options. optOwnerFont = 1;
   }
   return false;
}

static Bool
showhint_notify ( Handle self, Handle child, void * data)
{
    if ( his-> options. optOwnerShowHint) {
       his-> self-> set_show_hint ( child, *(( Bool *) data));
       his-> options. optOwnerShowHint = 1;
    }
    return false;
}


static Bool
hint_notify ( Handle self, Handle child, char * hint)
{
    if ( his-> options. optOwnerHint) {
       his-> self-> set_hint( child, hint);
       his-> options. optOwnerHint = 1;
    }
    return false;
}

Bool
single_color_notify ( Handle self, Handle child, void * color)
{
   PSingleColor s = ( PSingleColor) color;
   if ( his-> options. optOwnerColor && ( s-> index == ciFore))
   {
      his-> self-> set_color_index ( child, s-> color, s-> index);
      his-> options. optOwnerColor = 1;
   } else if (( his-> options. optOwnerBackColor) && ( s-> index == ciBack))
   {
      his-> self-> set_color_index ( child, s-> color, s-> index);
      his-> options. optOwnerBackColor = 1;
   } else if ( s-> index > ciBack)
      his-> self-> set_color_index ( child, s-> color, s-> index);
   return false;
}

Bool
read_point( AV * av, int * pt, int number, char * error)
{
   SV ** holder;
   int i;
   Bool result = true;
   for ( i = 0; i < number; i++) {
      holder = av_fetch( av, i, 0);
      if ( holder)
         pt[i] = SvIV( *holder);
      else {
         pt[i] = 0;
         result = false;
         if ( error) warn( error);
      }
   }
   return result;
}

/* XS section */
XS( Widget_get_widgets_FROMPERL)
{
   dXSARGS;
   Handle self;
   Handle * list;
   int i, count;

   if ( items != 1)
      croak ("Invalid usage of Widget.get_widgets");
   SP -= items;
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Widget.get_widgets");
   count = var-> widgets. count;
   list  = var-> widgets. items;
   EXTEND( sp, count);
   for ( i = 0; i < count; i++)
      PUSHs( sv_2mortal( newSVsv((( PAnyObject) list[ i])-> mate)));
   PUTBACK;
   return;
}

void Widget_get_widgets          ( Handle self) { warn("Invalid call of Widget::get_widgets"); }
void Widget_get_widgets_REDEFINED( Handle self) { warn("Invalid call of Widget::get_widgets"); }

