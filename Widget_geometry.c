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

#include "apricot.h"
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef  my
#define inherited CDrawable
#define my  ((( PWidget) self)-> self)
#define var (( PWidget) self)
#define his (( PWidget) child)

void Widget_pack_children( Handle self); 
void Widget_place_children( Handle self); 
Bool Widget_size_notify( Handle self, Handle child, const Rect* metrix);
Bool Widget_move_notify( Handle self, Handle child, Point * moveTo);
static void Widget_pack_enter( Handle self); 
static void Widget_pack_leave( Handle self); 

/*
   geometry managers.

   growMode - native Prima model, borrowed from TurboVision. Does not live with
              geomSize request size, uses virtualSize instead.

   pack and place - stolen from Perl-Tk.  
   Warning - all Tk positioning code is in Tk coordinates, meaning that Y axis descends
   
  */


/* action taken when geomSize changes - adjust own geometry, and children's .
 */
static void
geometry_reset( Handle self)
{
   /* own geometry */
   switch ( var-> geometry) {
   case gtGrowMode:
      if ( var-> growMode & gmCenter) 
         my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
      break;
   }

   /* children geometry */
   Widget_place_children( self);
   Widget_pack_children( self);
}

int
Widget_geometry( Handle self, Bool set, int geometry)
{
   if ( !set)
      return var-> geometry;
   if ( geometry == var-> geometry) return geometry;

   if ( geometry < gtDefault || geometry > gtMax)
      croak("Prima::Widget::geometry: invalid value passed");
   
   switch ( var-> geometry) {
   case gtPack:
      Widget_pack_leave( self);
      break;
   }
   var-> geometry = geometry;
   switch ( var-> geometry) {
   case gtPack:
      Widget_pack_enter( self);
      break;
   }
   if ( var-> owner) geometry_reset( var-> owner);
   return var-> geometry;
}

Point
Widget_geomSize( Handle self, Bool set, Point geomSize)
{
   if ( !set)
      return ( var-> geometry == gtDefault) ? my-> get_size(self) : var-> geomSize;
   var-> geomSize = geomSize;
   if ( var-> geometry == gtDefault) 
      my-> set_size( self, var-> geomSize);
   else if ( var-> owner) 
      geometry_reset( var-> owner);
   return var-> geomSize;
}

int
Widget_geomHeight( Handle self, Bool set, int geomHeight)
{
   if ( !set) 
      return ( var-> geometry == gtDefault) ? my-> get_height( self) : var-> geomSize. y;
   var-> geomSize. y = geomHeight;
   if ( var-> geometry == gtDefault) 
      my-> set_height( self, var-> geomSize. y);
   else if ( var-> owner) 
      geometry_reset( var-> owner);
   return var-> geomSize. y;
}

int
Widget_geomWidth( Handle self, Bool set, int geomWidth)
{
   if ( !set) 
      return ( var-> geometry == gtDefault) ? my-> get_width( self) : var-> geomSize. x;
   var-> geomSize. x = geomWidth;
   if ( var-> geometry == gtDefault) 
      my-> set_width( self, var-> geomSize. x);
   else if ( var-> owner) 
      geometry_reset( var-> owner);
   return var-> geomSize. x;
}

Bool
Widget_propagateGeometry( Handle self, Bool set, Bool propagate)
{
   Bool repack;
   if ( !set) return is_opt( optPropagateGeometry);
   repack = !(is_opt( optPropagateGeometry)) && propagate;
   opt_assign( optPropagateGeometry, propagate);
   if ( repack && var-> owner) geometry_reset( var-> owner);
   return is_opt( optPropagateGeometry);
}

void
Widget_reset_children_geometry( Handle self)
{
   Widget_pack_children( self);
   Widget_place_children( self);
}

Point
Widget_sizeMin( Handle self, Bool set, Point min)
{
   if ( !set)
      return var-> sizeMin;
   var-> sizeMin = min;
   if ( var-> stage <= csFrozen) {
      Point sizeActual  = my-> get_size( self);
      Point newSize     = sizeActual;
      if ( sizeActual. x < min. x) newSize. x = min. x;
      if ( sizeActual. y < min. y) newSize. y = min. y;
      if (( newSize. x != sizeActual. x) || ( newSize. y != sizeActual. y))
         my-> set_size( self, newSize);
      if ( var-> geometry != gtDefault && var-> owner) 
         geometry_reset( var-> owner);
   }
   apc_widget_set_size_bounds( self, var-> sizeMin, var-> sizeMax);
   return min;
}

Point
Widget_sizeMax( Handle self, Bool set, Point max)
{
   if ( !set)
      return var-> sizeMax;
   var-> sizeMax = max;
   if ( var-> stage <= csFrozen) {
      Point sizeActual  = my-> get_size( self);
      Point newSize     = sizeActual;
      if ( sizeActual. x > max. x) newSize. x = max. x;
      if ( sizeActual. y > max. y) newSize. y = max. y;
      if (( newSize. x != sizeActual. x) || ( newSize. y !=  sizeActual. y))
          my-> set_size( self, newSize);
      if ( var-> geometry != gtDefault && var-> owner) 
         geometry_reset( var-> owner);
   }
   apc_widget_set_size_bounds( self, var-> sizeMin, var-> sizeMax);
   return max;
}


/* geometry managers */

/* gtGrowMode */

Bool
Widget_size_notify( Handle self, Handle child, const Rect* metrix)
{
   if ( his-> growMode) {
      Point size  =  his-> self-> get_virtual_size( child);
      Point pos   =  his-> self-> get_origin( child);
      Point osize = size, opos = pos;
      int   dx    = ((Rect *) metrix)-> right - ((Rect *) metrix)-> left;
      int   dy    = ((Rect *) metrix)-> top   - ((Rect *) metrix)-> bottom;

      if ( his-> growMode & gmGrowLoX) pos.  x += dx;
      if ( his-> growMode & gmGrowHiX) size. x += dx;
      if ( his-> growMode & gmGrowLoY) pos.  y += dy;
      if ( his-> growMode & gmGrowHiY) size. y += dy;
      if ( his-> growMode & gmXCenter) pos. x = (((Rect *) metrix)-> right - size. x) / 2;
      if ( his-> growMode & gmYCenter) pos. y = (((Rect *) metrix)-> top   - size. y) / 2;

      if ( pos.x != opos.x || pos.y != opos.y || size.x != osize.x || size.y != osize.y) {
         if ( pos.x == opos.x && pos.y == opos.y) {
            his-> self-> set_size( child, size);
         } else if ( size.x == osize.x && size.y == osize.y) {
            his-> self-> set_origin( child, pos);
         } else {
            Rect r;
            r. left   = pos. x;
            r. bottom = pos. y;
            r. right  = pos. x + size. x;
            r. top    = pos. y + size. y;
            his-> self-> set_rect( child, r);
         }
      }
   }
   return false;
}

Bool
Widget_move_notify( Handle self, Handle child, Point * moveTo)
{
   Bool clp = his-> self-> get_clipOwner( child);
   int  dx  = moveTo-> x - var-> pos. x;
   int  dy  = moveTo-> y - var-> pos. y;
   Point p;

   if ( his-> growMode & gmDontCare) {
      if ( !clp) return false;
      p = his-> self-> get_origin( child);
      p. x -= dx;
      p. y -= dy;
      his-> self-> set_origin( child, p);
   } else {
      if ( clp) return false;
      p = his-> self-> get_origin( child);
      p. x += dx;
      p. y += dy;
      his-> self-> set_origin( child, p);
   }

   return false;
}

/*
    PACK
*/    

#define LEFT    0
#define BOTTOM  1
#define RIGHT   2
#define TOP     3

#define SOUTH   0
#define NORTH   2
#define WEST    0
#define EAST    2
#define CENTER  1

/* pack() internal mechanism - stolen from Tk v800.24, tkPack.c 
   Note that the original algorithm is taught to respect sizeMin
   and sizeMax, not present in Tk */

/*
 *----------------------------------------------------------------------
 *
 * XExpansion --
 *
 *	Given a list of packed slaves, the first of which is packed
 *	on the left or right and is expandable, compute how much to
 *	expand the child.
 *
 * Results:
 *	The return value is the number of additional pixels to give to
 *	the child.
 *
 *----------------------------------------------------------------------
 */

static int
slave_width( register PWidget slavePtr, register int plus)
{
   register int width = slavePtr-> geomSize. x + slavePtr-> packInfo. pad.x + 
                        slavePtr-> packInfo. ipad.x + plus;
   if ( width < slavePtr-> sizeMin.x) width = slavePtr-> sizeMin.x;
   if ( width > slavePtr-> sizeMax.x) width = slavePtr-> sizeMax.x;
   return width;
}

static int
slave_height( register PWidget slavePtr, register int plus)
{
   register int height = slavePtr-> geomSize.y + slavePtr-> packInfo. pad.y + 
                        slavePtr-> packInfo. ipad.y + plus;
   if ( height < slavePtr-> sizeMin.y) height = slavePtr-> sizeMin.y;
   if ( height > slavePtr-> sizeMax.y) height = slavePtr-> sizeMax.y;
   return height;
}

static int
XExpansion(slavePtr, cavityWidth)
    register PWidget slavePtr;		        /* First in list of remaining slaves. */
    int cavityWidth;			/* Horizontal space left for all
					 * remaining slaves. */
{
    int numExpand, minExpand, curExpand;
    int childWidth;

    /*
     * This procedure is tricky because windows packed top or bottom can
     * be interspersed among expandable windows packed left or right.
     * Scan through the list, keeping a running sum of the widths of
     * all left and right windows (actually, count the cavity space not
     * allocated) and a running count of all expandable left and right
     * windows.  At each top or bottom window, and at the end of the
     * list, compute the expansion factor that seems reasonable at that
     * point.  Return the smallest factor seen at any of these points.
     */

    minExpand = cavityWidth;
    numExpand = 0;
    for (; slavePtr != NULL;
        slavePtr = ( PWidget) slavePtr-> packInfo. next) {
	childWidth = slave_width(slavePtr, 0);
	if ((slavePtr-> packInfo. side == TOP) || (slavePtr-> packInfo. side == BOTTOM)) {
	    curExpand = (cavityWidth - childWidth)/numExpand;
	    if (curExpand < minExpand) {
		minExpand = curExpand;
	    }
	} else {
	    cavityWidth -= childWidth;
	    if (slavePtr->packInfo. expand) {
		numExpand++;
	    }
	}
    }
    curExpand = cavityWidth/numExpand;
    if (curExpand < minExpand) {
	minExpand = curExpand;
    }
    return (minExpand < 0) ? 0 : minExpand;
}
/*
 *----------------------------------------------------------------------
 *
 * YExpansion --
 *
 *	Given a list of packed slaves, the first of which is packed
 *	on the top or bottom and is expandable, compute how much to
 *	expand the child.
 *
 * Results:
 *	The return value is the number of additional pixels to give to
 *	the child.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
YExpansion(slavePtr, cavityHeight)
    register PWidget slavePtr;	        /* First in list of remaining
					 * slaves. */
    int cavityHeight;			/* Vertical space left for all
					 * remaining slaves. */
{
    int numExpand, minExpand, curExpand;
    int childHeight;

    /*
     * See comments for XExpansion.
     */

    minExpand = cavityHeight;
    numExpand = 0;
    for (; slavePtr != NULL; slavePtr = (PWidget) slavePtr->packInfo. next) {
	childHeight = slave_height(slavePtr, 0);
	if ((slavePtr-> packInfo. side == LEFT) || (slavePtr-> packInfo. side == RIGHT)) {
	    curExpand = (cavityHeight - childHeight)/numExpand;
	    if (curExpand < minExpand) {
		minExpand = curExpand;
	    }
	} else {
	    cavityHeight -= childHeight;
	    if (slavePtr-> packInfo. expand) {
		numExpand++;
	    }
	}
    }
    curExpand = cavityHeight/numExpand;
    if (curExpand < minExpand) {
	minExpand = curExpand;
    }
    return (minExpand < 0) ? 0 : minExpand;
}

void
Widget_pack_children( Handle self)
{
    PWidget masterPtr, slavePtr;
    int cavityX, cavityY, cavityWidth, cavityHeight;
				/* These variables keep track of the
				 * as-yet-unallocated space remaining in
				 * the middle of the parent window. */
    int frameX, frameY, frameWidth, frameHeight;
				/* These variables keep track of the frame
				 * allocated to the current window. */
    int x, y, width, height;	/* These variables are used to hold the
				 * actual geometry of the current window. */
    int maxWidth, maxHeight, tmp;
    int borderX, borderY;
    Point size;

    if ( var-> stage > csNormal) return;

    /*
     * If the parent has no slaves anymore, then don't do anything
     * at all:  just leave the parent's size as-is.
     */

    if (!( masterPtr = ( PWidget) var-> packSlaves)) return;

    /*
     * Pass #1: scan all the slaves to figure out the total amount
     * of space needed.  Two separate width and height values are
     * computed:
     *
     * width -		Holds the sum of the widths (plus padding) of
     *			all the slaves seen so far that were packed LEFT
     *			or RIGHT.
     * height -		Holds the sum of the heights (plus padding) of
     *			all the slaves seen so far that were packed TOP
     *			or BOTTOM.
     *
     * maxWidth -	Gradually builds up the width needed by the master
     *			to just barely satisfy all the slave's needs.  For
     *			each slave, the code computes the width needed for
     *			all the slaves so far and updates maxWidth if the
     *			new value is greater.
     * maxHeight -	Same as maxWidth, except keeps height info.
     */

    width = height = maxWidth = maxHeight = 0;
    for (slavePtr=masterPtr; slavePtr != NULL; 
         slavePtr = ( PWidget) slavePtr-> packInfo. next) {
	if ((slavePtr-> packInfo. side == TOP) || (slavePtr-> packInfo. side == BOTTOM)) {
	    tmp = slave_width( slavePtr, width);
	    if (tmp > maxWidth) maxWidth = tmp;
	    height += slave_height(slavePtr,0);
	} else {
	    tmp = slave_height(slavePtr, height);
	    if (tmp > maxHeight) maxHeight = tmp;
	    width += slave_width(slavePtr,0);
	}
    }
    if (width > maxWidth) {
	maxWidth = width;
    }
    if (height > maxHeight) {
	maxHeight = height;
    }

    /*
     * If the total amount of space needed in the parent window has
     * changed, and if we're propagating geometry information, then
     * notify the next geometry manager up and requeue ourselves to
     * start again after the parent has had a chance to
     * resize us.
     */

    if ((((maxWidth != my-> get_geomWidth(self)))
	    || (maxHeight != my-> get_geomHeight(self)))
	    && is_opt( optPropagateGeometry)) {
        Point p;
        p. x = maxWidth;
        p. y = maxHeight;
        my-> set_geomSize( self, p);
	return;
    }

    /*
     * Pass #2: scan the slaves a second time assigning
     * new sizes.  The "cavity" variables keep track of the
     * unclaimed space in the cavity of the window;  this
     * shrinks inward as we allocate windows around the
     * edges.  The "frame" variables keep track of the space
     * allocated to the current window and its frame.  The
     * current window is then placed somewhere inside the
     * frame, depending on anchor.
     */

    cavityX = cavityY = x = y = 0;
    size = CWidget(self)-> get_size( self);
    cavityWidth = size. x;
    cavityHeight = size. y;
    for ( slavePtr=masterPtr; slavePtr != NULL; slavePtr = ( PWidget) slavePtr-> packInfo. next) {
	if ((slavePtr-> packInfo. side == TOP) || (slavePtr-> packInfo. side == BOTTOM)) {
	    frameWidth = cavityWidth;
	    frameHeight = slave_height(slavePtr,0);
	    if (slavePtr-> packInfo. expand)
		frameHeight += YExpansion(slavePtr, cavityHeight);
	    cavityHeight -= frameHeight;
	    if (cavityHeight < 0) {
		frameHeight += cavityHeight;
		cavityHeight = 0;
	    }
	    frameX = cavityX;
	    if (slavePtr-> packInfo. side == TOP) {
		frameY = cavityY;
		cavityY += frameHeight;
	    } else {
		frameY = cavityY + cavityHeight;
	    }
	} else {
	    frameHeight = cavityHeight;
	    frameWidth = slave_width(slavePtr,0);
	    if (slavePtr->  packInfo. expand)
		frameWidth += XExpansion(slavePtr, cavityWidth);
	    cavityWidth -= frameWidth;
	    if (cavityWidth < 0) {
		frameWidth += cavityWidth;
		cavityWidth = 0;
	    }
	    frameY = cavityY;
	    if (slavePtr-> packInfo. side == LEFT) {
		frameX = cavityX;
		cavityX += frameWidth;
	    } else {
		frameX = cavityX + cavityWidth;
	    }
	}

	/*
	 * Now that we've got the size of the frame for the window,
	 * compute the window's actual size and location using the
	 * fill, padding, and frame factors.  
	 */

	borderX = slavePtr-> packInfo. pad.x;
	borderY = slavePtr-> packInfo. pad.y;
	width = slavePtr->  geomSize. x + slavePtr-> packInfo. ipad.x;
	if (slavePtr->  packInfo. fillx || (width > (frameWidth - borderX))) 
	    width = frameWidth - borderX;
	height = slavePtr->  geomSize. y + slavePtr-> packInfo. ipad.y;
	if (slavePtr->  packInfo. filly || (height > (frameHeight - borderY))) 
	    height = frameHeight - borderY;
	borderX /= 2;
	borderY /= 2;
        if ( width < slavePtr-> sizeMin.x) width = slavePtr-> sizeMin.x;
        if ( height < slavePtr-> sizeMin.y) height = slavePtr-> sizeMin.y;
        if ( width > slavePtr-> sizeMax.x) width = slavePtr-> sizeMax.x;
        if ( height > slavePtr-> sizeMax.y) height = slavePtr-> sizeMax.y;
	switch (slavePtr-> packInfo. anchorx) {
        case WEST:
           x = frameX + borderX;
           break;
        case CENTER:
           x = frameX + (frameWidth - width)/2;
           break;
        case EAST:
           x = frameX + frameWidth - width - borderX;
           break;
        }
	switch (slavePtr-> packInfo. anchory) {
        case NORTH:
           y = frameY + borderY;
           break;
        case CENTER:
           y = frameY + (frameHeight - height)/2;
           break;
        case SOUTH:
           y = frameY + frameHeight - height - borderY;
           break;
        }

        {
           Rect r;
           r. left   = x;
           r. bottom = size. y - y - height;
           r. right  = x + width;
           r. top    = size. y - y;

           /* printf("%s: %d %d %d %d\n", slavePtr-> name, x, r.bottom, width, r.top); */
           slavePtr-> self-> set_rect(( Handle) slavePtr, r);
        }
    }
}

/* applies pack parameters and enters pack slaves chain */

void
Widget_pack_enter( Handle self)
{

   /* see if leftover object reference is alive */
   if ( var-> packInfo. order && 
        !hash_fetch( primaObjects, &var-> packInfo. order, sizeof(Handle))) {
      var-> packInfo. order = nilHandle;
      var-> packInfo. after = 0;
   }


   /* store into slaves list */
   if ( var-> owner) {
      Handle master = var-> owner, ptr;

      if ( PWidget( master)-> packSlaves) {
         /* insert into list using 'order' marker */
         ptr = PWidget( master)-> packSlaves;
         if ( ptr != var-> packInfo. order) {
            Handle optr = ptr;
            Bool inserted = false;
            while ( ptr) {
               if ( ptr == var-> packInfo. order) {
                  if ( var-> packInfo. after) {
                     var-> packInfo. next = PWidget( ptr)-> packInfo. next;
                     PWidget( ptr)-> packInfo. next = self;
                  } else {
                     var-> packInfo. next = ptr;
                     PWidget( optr)-> packInfo. next = self;
                  }
                  inserted = true;
                  break;
               }
               optr = ptr;
               ptr = PWidget( ptr)-> packInfo. next;
            }
            if ( !inserted) PWidget( optr)-> packInfo. next = self;
         } else {
            /* order is first in list */
            if ( var-> packInfo. after) {
               var-> packInfo. next = PWidget( ptr)-> packInfo. next;
               PWidget( ptr)-> packInfo. next = self;
            } else {
               var-> packInfo. next = ptr;
               PWidget( master)-> packSlaves = self;
            }
         }
      } else { /* owner has no slaves, we're first */
         PWidget( master)-> packSlaves = self;
      }
   }
}

/* removes widget from list of pack slaves */
void
Widget_pack_leave( Handle self)
{
   Handle ptr;
   
   if ( var-> owner) {
      if (( ptr = PWidget( var-> owner)-> packSlaves) != self) {
         while ( PWidget(ptr)-> packInfo. next) {
            if ( PWidget(ptr)-> packInfo. next == self) {
               PWidget(ptr)-> packInfo. next = var-> packInfo. next;
               break;
            }
            ptr = PWidget(ptr)-> packInfo. next;
         }
      } else {
         PWidget( var-> owner)-> packSlaves = var-> packInfo. next;
      }
   }

   var-> packInfo. next = nilHandle;
}

SV * 
Widget_packInfo( Handle self, Bool set, SV * packInfo)
{
   if ( !set) {
      HV * profile = newHV();
      PackInfo *p = &var-> packInfo;

      switch ( p-> side) {
      case LEFT   : pset_c( side, "top");    break;
      case BOTTOM : pset_c( side, "bottom"); break;
      case RIGHT  : pset_c( side, "right");  break;
      case TOP    : pset_c( side, "top");    break;
      }

      if ( p-> fillx) {
         pset_c( fill, p-> filly ? "both" : "x");
      } else {
         pset_c( fill, p-> filly ? "y" : "none");
      }

      pset_i( expand, p-> expand);

      switch ( p-> anchorx) {
      case WEST:
         pset_c( anchor, 
                 (( p-> anchory == NORTH) ? "nw" : (( p-> anchory == CENTER) ? "w" : "sw"))
               );
         break;
      case CENTER:
         pset_c( anchor, 
                 (( p-> anchory == NORTH) ? "n" : (( p-> anchory == CENTER) ? "center" : "s"))
               );
         break;
      case EAST:
         pset_c( anchor, 
                 (( p-> anchory == NORTH) ? "ne" : (( p-> anchory == CENTER) ? "e" : "se"))
               );
         break;
      }

      pset_H( after,  ( p-> order && p-> after)  ? p-> order : nilHandle);
      pset_H( before, ( p-> order && !p-> after) ? p-> order : nilHandle);
      pset_H( in, var-> owner);
      
      pset_i( ipadx, p-> ipad. x);
      pset_i( ipady, p-> ipad. y);
      pset_i( padx, p-> pad. x);
      pset_i( pady, p-> pad. y);
      
      return newRV_noinc(( SV *) profile);
   } else {
      HV * profile;
      Bool reset_zorder = false;

      if ( SvTYPE(packInfo) == SVt_NULL) return nilSV;
      
      if ( !SvOK(packInfo) || !SvROK(packInfo) || SvTYPE(SvRV(packInfo)) != SVt_PVHV)
         croak("Widget::packInfo: parameter is not a hash");

      profile = ( HV*) SvRV( packInfo);
      if ( pexist( side)) {
         char * c = pget_c( side);
         if ( *c == 'l' && (strcmp( c, "left")==0))   var-> packInfo. side = LEFT; else
         if ( *c == 'b' && (strcmp( c, "bottom")==0)) var-> packInfo. side = BOTTOM; else
         if ( *c == 'r' && (strcmp( c, "right")==0))  var-> packInfo. side = RIGHT; else
         if ( *c == 't' && (strcmp( c, "top")==0))    var-> packInfo. side = TOP; else
            croak("%s: invalid 'side'", "RTC008F: Prima::Widget::pack");
      }

      if ( pexist( fill)) {
         char * c = pget_c( fill);
         if (( strcmp( c, "x") == 0)) {
            var-> packInfo. fillx = 1; 
            var-> packInfo. filly = 0; 
         } else if (( strcmp( c, "y") == 0)) {
            var-> packInfo. fillx = 0; 
            var-> packInfo. filly = 1; 
         } else if ( *c == 'n' && ( strcmp( c, "none") == 0)) {
            var-> packInfo. fillx = 
            var-> packInfo. filly = 0; 
         } else if ( *c == 'b' && ( strcmp( c, "both") == 0)) {
            var-> packInfo. fillx = 
            var-> packInfo. filly = 1; 
         } else
            croak("%s: invalid 'fill'", "RTC008F: Prima::Widget::pack");
      }
      
      if ( pexist( expand)) {
         var-> packInfo. expand = pget_B( expand);
      }

      if ( pexist( anchor)) {
         char * c = pget_c( anchor);
         if (( strcmp( c, "n") == 0)) {
            var-> packInfo. anchorx = CENTER;
            var-> packInfo. anchory = NORTH;
         } else if (( strcmp( c, "ne") == 0)) {
            var-> packInfo. anchorx = EAST;
            var-> packInfo. anchory = NORTH;
         } else if (( strcmp( c, "e") == 0)) {
            var-> packInfo. anchorx = EAST;
            var-> packInfo. anchory = CENTER;
         } else if (( strcmp( c, "se") == 0)) {
            var-> packInfo. anchorx = EAST;
            var-> packInfo. anchory = SOUTH;
         } else if (( strcmp( c, "s") == 0)) {
            var-> packInfo. anchorx = CENTER;
            var-> packInfo. anchory = SOUTH;
         } else if (( strcmp( c, "sw") == 0)) {
            var-> packInfo. anchorx = WEST;
            var-> packInfo. anchory = SOUTH;
         } else if (( strcmp( c, "w") == 0)) {
            var-> packInfo. anchorx = WEST;
            var-> packInfo. anchory = CENTER;
         } else if (( strcmp( c, "nw") == 0)) {
            var-> packInfo. anchorx = WEST;
            var-> packInfo. anchory = NORTH;
         } else if ( *c == 'c' && ( strcmp( c, "center") == 0)) {
            var-> packInfo. anchorx = CENTER;
            var-> packInfo. anchory = CENTER;
         } else
            croak("%s: invalid 'anchor'", "RTC008F: Prima::Widget::pack");
      }

      if ( pexist( ipadx)) var-> packInfo. ipad. x = pget_i( ipadx);
      if ( pexist( ipady)) var-> packInfo. ipad. y = pget_i( ipady);
      if ( pexist( padx))  var-> packInfo. pad. x  = pget_i( padx);
      if ( pexist( pady))  var-> packInfo. pad. y  = pget_i( pady);

      if ( pexist( after)) {
         SV * sv = pget_sv( after);
         if ( SvTYPE(sv) != SVt_NULL) {
            if ( !( var-> packInfo. order = gimme_the_mate( sv)))
               croak("%s: invalid 'after'", "RTC008F: Prima::Widget::pack");
            var-> packInfo. after = 1;
            if ( pexist( before)) {
               sv = pget_sv( before);
               if ( SvTYPE(sv) != SVt_NULL)
                  croak("%s: 'after' and 'before' cannot be present simultaneously", "RTC008F: Prima::Widget::pack");
            }
         } else {
            var-> packInfo. order = nilHandle;
            var-> packInfo. after = 0;
         }
         reset_zorder = true;
      } else if ( pexist( before)) {
         SV * sv = pget_sv( before);
         if ( SvTYPE(sv) != SVt_NULL) {
            if ( !( var-> packInfo. order = gimme_the_mate( sv)))
               croak("%s: invalid 'before'", "RTC008F: Prima::Widget::pack");
         } else
            var-> packInfo. order = nilHandle;
         var-> packInfo. after = 0;
         reset_zorder = true;
      }

      if ( pexist( in)) {
         if ( pget_H( in) != var-> owner)
            croak("%s: invalid 'in'", "RTC008F: Prima::Widget::pack");
      }

      if ( reset_zorder) {
         Widget_pack_leave( self);
         Widget_pack_enter( self);
      }

      if ( var-> owner) geometry_reset( var-> owner);
   }
   return nilSV;
}

XS( Widget_get_pack_slaves_FROMPERL)
{
   dXSARGS;
   Handle self;

   if ( items != 1)
      croak ("Invalid usage of Widget.get_pack_slaves");
   SP -= items;
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Widget.get_pack_slaves");
   self = var-> packSlaves;
   while ( self) {
      XPUSHs( sv_2mortal( newSVsv((( PAnyObject) self)-> mate)));
      self = var-> packInfo. next;
   }
   PUTBACK;
   return;
}


void Widget_get_pack_slaves          ( Handle self) { warn("Invalid call of Widget::get_pack_slaves"); }
void Widget_get_pack_slaves_REDEFINED( Handle self) { warn("Invalid call of Widget::get_pack_slaves"); }

/* 
  PLACE   
 */
/* place internal mechanism - stolen from Tk v800.24, tkPlace.c */

void
Widget_place_children( Handle self)
{
    register PWidget slave;
    register PlaceInfo *slavePtr;
    int x, y, width, height, tmp;
    int i, masterWidth, masterHeight;
    double x1, y1, x2, y2;
    Point size;


    /*
     * Iterate over all the slaves for the master.  Each slave's
     * geometry can be computed independently of the other slaves.
     */

    size = my-> get_size( self);
    masterWidth  = size. x; 
    masterHeight = size. y; 

    for ( i = 0; i < var-> widgets. count; i++) {
       Point sz;
       slave = (PWidget) (var-> widgets. items[i]);
       if ( slave-> geometry != gtPlace) continue;
       slavePtr = &slave-> placeInfo;
       sz = slave-> self-> get_size(( Handle) slave);
        
	/*
	 * Step 2:  compute size of slave (outside dimensions including
	 * border) and location of anchor point within master.
	 */

	x1 = slavePtr->x + (slavePtr->relX*masterWidth);
	x = (int) (x1 + ((x1 > 0) ? 0.5 : -0.5));
	y1 = slavePtr->y + (slavePtr->relY*masterHeight);
	y = (int) (y1 + ((y1 > 0) ? 0.5 : -0.5));
	if (slavePtr-> use_w || slavePtr-> use_rw) {
	    width = 0;
	    if (slavePtr-> use_w) {
		width += slave->geomSize.x;
	    }
	    if (slavePtr-> use_rw) {
		/*
		 * The code below is a bit tricky.  In order to round
		 * correctly when both relX and relWidth are specified,
		 * compute the location of the right edge and round that,
		 * then compute width.  If we compute the width and round
		 * it, rounding errors in relX and relWidth accumulate.
		 */

		x2 = x1 + (slavePtr->relWidth*masterWidth);
		tmp = (int) (x2 + ((x2 > 0) ? 0.5 : -0.5));
		width += tmp - x;
	    }
	} else {
	    width = sz. x;
	}
	if (slavePtr-> use_h || slavePtr-> use_rh) {
	    height = 0;
	    if (slavePtr->use_h) {
		height += slave->geomSize. y;
	    }
	    if (slavePtr->use_rh) {
		/*
		 * See note above for rounding errors in width computation.
		 */

		y2 = y1 + (slavePtr->relHeight*masterHeight);
		tmp = (int) (y2 + ((y2 > 0) ? 0.5 : -0.5));
		height += tmp - y;
	    }
	} else {
	    height = sz. y;
	}

	/*
	 * Step 3: adjust the x and y positions so that the desired
	 * anchor point on the slave appears at that position.  Also
	 * adjust for the border mode and master's border.
	 */
	switch (slavePtr-> anchorx) {
        case WEST:
           break;
        case CENTER:
           x -= width/2;
           break;
        case EAST:
	   x -= width;
           break;
        }
	switch (slavePtr-> anchory) {
        case NORTH:
           break;
        case CENTER:
	   y -= height/2;
           break;
        case SOUTH:
           y -= height;
           break;
        }

        {
           Rect r;
           r. left   = x;
           r. bottom = size. y - y - height;
           r. right  = x + width;
           r. top    = size. y - y;
            
           /* printf("%s: %d %d %d %d\n", slave-> name, x, y, width, height); */
           slave-> self-> set_rect(( Handle) slave, r);
        }
    }
}


SV * 
Widget_placeInfo( Handle self, Bool set, SV * placeInfo)
{
   if ( !set) {
      HV * profile = newHV();
      PlaceInfo *p = &var-> placeInfo;

      switch ( p-> anchorx) {
      case WEST:
         pset_c( anchor, 
                 (( p-> anchory == NORTH) ? "nw" : (( p-> anchory == CENTER) ? "w" : "sw"))
               );
         break;
      case CENTER:
         pset_c( anchor, 
                 (( p-> anchory == NORTH) ? "n" : (( p-> anchory == CENTER) ? "center" : "s"))
               );
         break;
      case EAST:
         pset_c( anchor, 
                 (( p-> anchory == NORTH) ? "ne" : (( p-> anchory == CENTER) ? "e" : "se"))
               );
         break;
      }

      pset_H( in, var-> owner);
      
      if ( p-> use_x)   pset_i( x, p-> x);
      if ( p-> use_y)   pset_i( y, p-> y);
      if ( p-> use_w)   pset_i( width,  var-> geomSize. x);
      if ( p-> use_h)   pset_i( height, var-> geomSize. y);
      if ( p-> use_rx)  pset_f( relx, p-> relX);
      if ( p-> use_ry)  pset_f( rely, p-> relY);
      if ( p-> use_rw)  pset_f( relwidth, p-> relWidth);
      if ( p-> use_rh)  pset_f( relheight, p-> relHeight);
      
      return newRV_noinc(( SV *) profile);
   } else {
      HV * profile;

      if ( SvTYPE(placeInfo) == SVt_NULL) return nilSV;
      
      if ( !SvOK(placeInfo) || !SvROK(placeInfo) || SvTYPE(SvRV(placeInfo)) != SVt_PVHV)
         croak("Widget::placeInfo: parameter is not a hash");

      profile = ( HV*) SvRV( placeInfo);

      if ( pexist( anchor)) {
         char * c = pget_c( anchor);
         if (( strcmp( c, "n") == 0)) {
            var-> placeInfo. anchorx = CENTER;
            var-> placeInfo. anchory = NORTH;
         } else if (( strcmp( c, "ne") == 0)) {
            var-> placeInfo. anchorx = EAST;
            var-> placeInfo. anchory = NORTH;
         } else if (( strcmp( c, "e") == 0)) {
            var-> placeInfo. anchorx = EAST;
            var-> placeInfo. anchory = CENTER;
         } else if (( strcmp( c, "se") == 0)) {
            var-> placeInfo. anchorx = EAST;
            var-> placeInfo. anchory = SOUTH;
         } else if (( strcmp( c, "s") == 0)) {
            var-> placeInfo. anchorx = CENTER;
            var-> placeInfo. anchory = SOUTH;
         } else if (( strcmp( c, "sw") == 0)) {
            var-> placeInfo. anchorx = WEST;
            var-> placeInfo. anchory = SOUTH;
         } else if (( strcmp( c, "w") == 0)) {
            var-> placeInfo. anchorx = WEST;
            var-> placeInfo. anchory = CENTER;
         } else if (( strcmp( c, "nw") == 0)) {
            var-> placeInfo. anchorx = WEST;
            var-> placeInfo. anchory = NORTH;
         } else if ( *c == 'c' && ( strcmp( c, "center") == 0)) {
            var-> placeInfo. anchorx = CENTER;
            var-> placeInfo. anchory = CENTER;
         } else
            croak("%s: invalid 'anchor'", "RTC008F: Prima::Widget::place");
      }

      if ( pexist( x)) {
         SV * sv = pget_sv( x);
         if (( var-> placeInfo. use_x = (SvTYPE( sv) != SVt_NULL))) 
            var-> placeInfo. x = SvIV( sv);
      }
      if ( pexist( y)) {
         SV * sv = pget_sv( y);
         if (( var-> placeInfo. use_y = (SvTYPE( sv) != SVt_NULL))) 
            var-> placeInfo. y = SvIV( sv);
      }
      if ( pexist( width)) {
         SV * sv = pget_sv( width);
         if (( var-> placeInfo. use_w = (SvTYPE( sv) != SVt_NULL))) 
            var-> geomSize. x = SvIV( sv);
      }
      if ( pexist( height)) {
         SV * sv = pget_sv( height);
         if (( var-> placeInfo. use_h = (SvTYPE( sv) != SVt_NULL))) 
            var-> geomSize. y = SvIV( sv);
      }
      if ( pexist( relx)) {
         SV * sv = pget_sv( relx);
         if (( var-> placeInfo. use_rx = (SvTYPE( sv) != SVt_NULL))) 
            var-> placeInfo. relX = SvNV( sv);
      }
      if ( pexist( rely)) {
         SV * sv = pget_sv( rely);
         if (( var-> placeInfo. use_ry = (SvTYPE( sv) != SVt_NULL))) 
            var-> placeInfo. relY = SvNV( sv);
      }
      if ( pexist( relwidth)) {
         SV * sv = pget_sv( relwidth);
         if (( var-> placeInfo. use_rw = (SvTYPE( sv) != SVt_NULL))) 
            var-> placeInfo. relWidth = SvNV( sv);
      }
      if ( pexist( relheight)) {
         SV * sv = pget_sv( relheight);
         if (( var-> placeInfo. use_rh = (SvTYPE( sv) != SVt_NULL))) 
            var-> placeInfo. relHeight = SvNV( sv);
      }

      if ( pexist( in)) {
         if ( pget_H( in) != var-> owner)
            croak("%s: invalid 'in'", "RTC008F: Prima::Widget::place");
      }
      
      if ( var-> owner) geometry_reset( var-> owner);
   }

   return nilSV;
}

XS( Widget_get_place_slaves_FROMPERL)
{
   dXSARGS;
   int i;
   Handle self;

   if ( items != 1)
      croak ("Invalid usage of Widget.get_pack_slaves");
   SP -= items;
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Widget.get_pack_slaves");
   for ( i = 0; i < var-> widgets. count; i++) {
      if ( PWidget( var-> widgets. items[i])-> geometry == gtPlace)
         XPUSHs( sv_2mortal( newSVsv((( PAnyObject)(var-> widgets. items[i]))-> mate)));
   }
   PUTBACK;
   return;
}

void Widget_get_place_slaves          ( Handle self) { warn("Invalid call of Widget::get_place_slaves"); }
void Widget_get_place_slaves_REDEFINED( Handle self) { warn("Invalid call of Widget::get_place_slaves"); }

/*  */

#ifdef __cplusplus
}
#endif

