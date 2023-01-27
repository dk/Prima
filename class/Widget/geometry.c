#include "apricot.h"
#include "guts.h"
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef  my
#define inherited CDrawable
#define my  ((( PWidget) self)-> self)
#define var (( PWidget) self)
#define his (( PWidget) child)

void Widget_pack_slaves( Handle self);
void Widget_place_slaves( Handle self);
Bool Widget_size_notify( Handle self, Handle child, const Rect* metrix);
Bool Widget_move_notify( Handle self, Handle child, Point * moveTo);
static void Widget_pack_enter( Handle self);
static void Widget_pack_leave( Handle self);
static void Widget_place_enter( Handle self);
static void Widget_place_leave( Handle self);

/*
	geometry managers.

	growMode - native Prima model, borrowed from TurboVision. Does not live with
				geomSize request size, uses virtualSize instead.

	pack and place - copy-pasted from Perl-Tk.

*/

/*
pack Handle fields:

	next       - available only when geometry == gtPack
	order, in  - available always, but is always valid when geometry == gtPack only

	in and order cause croaks when submitted via packInfo(), but are silently
	converted to NULL when geometry changes and the references are not valid anymore

*/


#define MASTER      ((var->geometry != gtGrowMode && var->geomInfo.in)?var->geomInfo.in:var->owner)
#define geometry_reset_all() geometry_reset(MASTER,-1)


/* resets particular ( or all, if geometry < 0 ) geometry widgets */

static void
geometry_reset( Handle self, int geometry)
{
	if ( !self) return;

	if (
		(var-> geometry == gtGrowMode) &&
		(var-> growMode & gmCenter) &&
		( geometry == gtGrowMode || geometry < 0)
	) {
		my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
	}

	if ( geometry == gtPack || geometry < 0)
		Widget_pack_slaves( self);

	if ( geometry == gtPlace || geometry < 0)
		Widget_place_slaves( self);
}

int
Widget_geometry( Handle self, Bool set, int geometry)
{
	if ( !set)
		return var-> geometry;
	if ( geometry == var-> geometry) {
		/* because called within set_owner() */
		if ((var-> geometry == gtGrowMode) && (var-> growMode & gmCenter))
			my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
		return geometry;
	}

	if ( geometry < gtDefault || geometry > gtMax)
		croak("Prima::Widget::geometry: invalid value passed");

	switch ( var-> geometry) {
	case gtPlace:
		Widget_place_leave( self);
		break;
	case gtPack:
		Widget_pack_leave( self);
		break;
	}
	var-> geometry = geometry;
	switch ( var-> geometry) {
	case gtGrowMode:
		if ( var-> growMode & gmCenter)
			my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
		break;
	case gtPlace:
		Widget_place_enter( self);
		break;
	case gtPack:
		Widget_pack_enter( self);
		break;
	}
	geometry_reset_all();
	return var-> geometry;
}

Point
Widget_geomSize( Handle self, Bool set, Point geomSize)
{
	if ( !set)
		return var-> geomSize;
		/* return ( var-> geometry == gtDefault) ? my-> get_size(self) : var-> geomSize; */
	var-> geomSize = geomSize;
	if ( var-> geometry == gtDefault)
		my-> set_size( self, var-> geomSize);
	else
		geometry_reset_all();
	return var-> geomSize;
}

int
Widget_geomHeight( Handle self, Bool set, int geomHeight)
{
	if ( set) {
		Point p = { var-> geomSize. x, geomHeight};
		my-> set_geomSize( self, p);
	}
	return var-> geomSize. y;
}

int
Widget_geomWidth( Handle self, Bool set, int geomWidth)
{
	if ( set) {
		Point p = { geomWidth, var-> geomSize. y};
		my-> set_geomSize( self, p);
	}
	return var-> geomSize. x;
}

Bool
Widget_packPropagate( Handle self, Bool set, Bool propagate)
{
	Bool repack;
	if ( !set) return is_opt( optPackPropagate);
	repack = !(is_opt( optPackPropagate)) && propagate;
	opt_assign( optPackPropagate, propagate);
	if ( repack) geometry_reset(self,-1);
	return is_opt( optPackPropagate);
}

void
Widget_reset_children_geometry( Handle self)
{
	Widget_pack_slaves( self);
	Widget_place_slaves( self);
}

/* checks if Handle in is allowed to be a master for self -
	used for gt::Pack */
static Handle
Widget_check_in( Handle self, Handle in, Bool barf)
{
	Handle h = in;

	/* check overall validity */
	if ( !in || !kind_of( in, CWidget)) {
		if ( barf)
			croak("%s: invalid 'in': not a widget", "Prima::Widget::pack");
		else
			return NULL_HANDLE;
	}

	/* check direct inheritance */
	while ( h) {
		if ( h == self) {
			if ( barf)
				croak("%s: invalid 'in': is already a child", "Prima::Widget::pack");
			else
				return NULL_HANDLE;
		}
		h = PWidget( h)-> owner;
	}

	/* check slaves chain */
	h = PWidget( in)-> packSlaves;
	while ( h) {
		if ( h == in) {
			if ( barf)
				croak("%s: invalid 'in': already a pack slave", "Prima::Widget::pack");
			else
				return NULL_HANDLE;
		}
		h = PWidget( h)-> geomInfo. next;
	}

	h = PWidget( in)-> placeSlaves;
	while ( h) {
		if ( h == in) {
			if ( barf)
				croak("%s: invalid 'in': already a place slave", "Prima::Widget::pack");
			else
				return NULL_HANDLE;
		}
		h = PWidget( h)-> geomInfo. next;
	}

	/* place to check other chains if needed */

	return in;
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
		if ( var-> geometry != gtDefault)
			geometry_reset_all();
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
		if ( var-> geometry != gtDefault)
			geometry_reset_all();
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

	if ( his-> geometry != gtGrowMode ) return false;

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
	register int width =
		slavePtr-> geomSize. x +
		slavePtr-> geomInfo. pad.x +
		slavePtr-> geomInfo. ipad.x +
		plus;
	if ( width < slavePtr-> sizeMin.x) width = slavePtr-> sizeMin.x;
	if ( width > slavePtr-> sizeMax.x) width = slavePtr-> sizeMax.x;
	return width;
}

static int
slave_height( register PWidget slavePtr, register int plus)
{
	register int height =
		slavePtr-> geomSize.y +
		slavePtr-> geomInfo. pad.y +
		slavePtr-> geomInfo. ipad.y +
		plus;
	if ( height < slavePtr-> sizeMin.y) height = slavePtr-> sizeMin.y;
	if ( height > slavePtr-> sizeMax.y) height = slavePtr-> sizeMax.y;
	return height;
}

static int
XExpansion(slavePtr, cavityWidth)
	register PWidget slavePtr; /* First in list of remaining slaves. */
	int cavityWidth;           /* Horizontal space left for all remaining slaves. */
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
	for (
		;
		slavePtr != NULL;
		slavePtr = ( PWidget) slavePtr-> geomInfo. next
	) {
		childWidth = slave_width(slavePtr, 0);
		if ((slavePtr-> geomInfo. side == TOP) || (slavePtr-> geomInfo. side == BOTTOM)) {
			curExpand = (cavityWidth - childWidth)/numExpand;
			if (curExpand < minExpand)
				minExpand = curExpand;
		} else {
			cavityWidth -= childWidth;
			if (slavePtr->geomInfo. expand)
				numExpand++;
		}
	}

	curExpand = cavityWidth / numExpand;
	if (curExpand < minExpand) 
		minExpand = curExpand;
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
	for (
		;
		slavePtr != NULL;
		slavePtr = (PWidget) slavePtr->geomInfo. next
	) {
		childHeight = slave_height(slavePtr, 0);
		if ((slavePtr-> geomInfo. side == LEFT) || (slavePtr-> geomInfo. side == RIGHT)) {
			curExpand = (cavityHeight - childHeight)/numExpand;
			if (curExpand < minExpand)
				minExpand = curExpand;
		} else {
			cavityHeight -= childHeight;
			if (slavePtr-> geomInfo. expand)
				numExpand++;
		}
	}

	curExpand = cavityHeight/numExpand;
	if (curExpand < minExpand) 
		minExpand = curExpand;
	return (minExpand < 0) ? 0 : minExpand;
}

void
Widget_pack_slaves( Handle self)
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
	for (
		slavePtr = masterPtr;
		slavePtr != NULL;
		slavePtr = ( PWidget) slavePtr-> geomInfo. next
	) {
		if (
			(slavePtr-> geomInfo. side == TOP   ) ||
			(slavePtr-> geomInfo. side == BOTTOM)
		) {
			tmp = slave_width( slavePtr, width);
			if (tmp > maxWidth) maxWidth = tmp;
			height += slave_height(slavePtr,0);
		} else {
			tmp = slave_height(slavePtr, height);
			if (tmp > maxHeight) maxHeight = tmp;
			width += slave_width(slavePtr,0);
		}
	}
	if (width > maxWidth)
		maxWidth = width;
	if (height > maxHeight)
		maxHeight = height;

	/*
	* If the total amount of space needed in the parent window has
	* changed, and if we're propagating geometry information, then
	* notify the next geometry manager up and requeue ourselves to
	* start again after the parent has had a chance to
	* resize us.
	*/

	if (
		is_opt( optPackPropagate) && (
			maxWidth  != my-> get_geomWidth(self) ||
			maxHeight != my-> get_geomHeight(self)
		)
	) {
		Point p, oldsize;
		p. x = maxWidth;
		p. y = maxHeight;
		oldsize = my-> get_size( self);
		my-> set_geomSize( self, p);
		size = my-> get_size( self);
		/* if size didn't change, that means, that no cmSize came, and thus
		the actual repacking of slaves never took place */
		if ( oldsize. x != size. x || oldsize. y != size. y)
			return;
	} else {
		size = my-> get_size( self);
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
	cavityWidth = size. x;
	cavityHeight = size. y;
	for (
		slavePtr = masterPtr;
		slavePtr != NULL;
		slavePtr = ( PWidget) slavePtr-> geomInfo. next
	) {
		if (
			(slavePtr-> geomInfo. side == TOP   ) ||
			(slavePtr-> geomInfo. side == BOTTOM)
		) {
			frameWidth = cavityWidth;
			frameHeight = slave_height(slavePtr,0);
			if (slavePtr-> geomInfo. expand)
				frameHeight += YExpansion(slavePtr, cavityHeight);
			cavityHeight -= frameHeight;
			if (cavityHeight < 0) {
				frameHeight += cavityHeight;
				cavityHeight = 0;
			}
			frameX = cavityX;
			if (slavePtr-> geomInfo. side == BOTTOM) {
				frameY = cavityY;
				cavityY += frameHeight;
			} else {
				frameY = cavityY + cavityHeight;
			}
		} else {
			frameHeight = cavityHeight;
			frameWidth = slave_width(slavePtr,0);
			if (slavePtr->  geomInfo. expand)
				frameWidth += XExpansion(slavePtr, cavityWidth);
			cavityWidth -= frameWidth;
			if (cavityWidth < 0) {
				frameWidth += cavityWidth;
				cavityWidth = 0;
			}
			frameY = cavityY;
			if (slavePtr-> geomInfo. side == LEFT) {
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

		borderX = slavePtr-> geomInfo. pad.x;
		borderY = slavePtr-> geomInfo. pad.y;
		width = slavePtr->  geomSize. x + slavePtr-> geomInfo. ipad.x;
		if (slavePtr->  geomInfo. fillx || (width > (frameWidth - borderX)))
			width = frameWidth - borderX;
		height = slavePtr->  geomSize. y + slavePtr-> geomInfo. ipad.y;
		if (slavePtr->  geomInfo. filly || (height > (frameHeight - borderY)))
			height = frameHeight - borderY;
		borderX /= 2;
		borderY /= 2;
		if ( width  < slavePtr-> sizeMin.x) width  = slavePtr-> sizeMin.x;
		if ( height < slavePtr-> sizeMin.y) height = slavePtr-> sizeMin.y;
		if ( width  > slavePtr-> sizeMax.x) width  = slavePtr-> sizeMax.x;
		if ( height > slavePtr-> sizeMax.y) height = slavePtr-> sizeMax.y;
		switch (slavePtr-> geomInfo. anchorx) {
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
		switch (slavePtr-> geomInfo. anchory) {
		case SOUTH:
			y = frameY + borderY;
			break;
		case CENTER:
			y = frameY + (frameHeight - height)/2;
			break;
		case NORTH:
			y = frameY + frameHeight - height - borderY;
			break;
		}

		{
			Rect r;
			r. left   = x;
			r. bottom = y;
			r. right  = x + width;
			r. top    = y + height;

			/* printf("%s: %d %d %d %d\n", slavePtr-> name, x, r.bottom, width, r.top); */
			slavePtr-> self-> set_rect(( Handle) slavePtr, r);
		}
	}
}

/* applies pack parameters and enters pack slaves chain */

void
Widget_pack_enter( Handle self)
{
	Handle master, ptr;

	/* see if leftover object references are alive */
	if ( var-> geomInfo. order &&
		!hash_fetch( prima_guts.objects, &var-> geomInfo. order, sizeof(Handle))) {
		var-> geomInfo. order = NULL_HANDLE;
		var-> geomInfo. after = 0;
	}
	if ( var-> geomInfo. in) {
		if ( hash_fetch( prima_guts.objects, &var-> geomInfo. in, sizeof(Handle)))
			var-> geomInfo. in = Widget_check_in( self, var-> geomInfo. in, false);
		else
			var-> geomInfo. in = NULL_HANDLE;
	}

	/* store into slaves list */
	master = (( var-> geomInfo. in) ? var-> geomInfo. in : var-> owner);

	if ( PWidget( master)-> packSlaves) {
		/* insert into list using 'order' marker */
		ptr = PWidget( master)-> packSlaves;
		if ( ptr != var-> geomInfo. order) {
			Handle optr = ptr;
			Bool inserted = false;
			while ( ptr) {
				if ( ptr == var-> geomInfo. order) {
					if ( var-> geomInfo. after) {
						var-> geomInfo. next = PWidget( ptr)-> geomInfo. next;
						PWidget( ptr)-> geomInfo. next = self;
					} else {
						var-> geomInfo. next = ptr;
						PWidget( optr)-> geomInfo. next = self;
					}
					inserted = true;
					break;
				}
				optr = ptr;
				ptr = PWidget( ptr)-> geomInfo. next;
			}
			if ( !inserted) PWidget( optr)-> geomInfo. next = self;
		} else {
			/* order is first in list */
			if ( var-> geomInfo. after) {
				var-> geomInfo. next = PWidget( ptr)-> geomInfo. next;
				PWidget( ptr)-> geomInfo. next = self;
			} else {
				var-> geomInfo. next = ptr;
				PWidget( master)-> packSlaves = self;
			}
		}
	} else { /* master has no slaves, we're first */
		PWidget( master)-> packSlaves = self;
	}
}

/* removes widget from list of pack slaves */
void
Widget_pack_leave( Handle self)
{
	Handle ptr, master;

	master = (( var-> geomInfo. in) ? var-> geomInfo. in : var-> owner);

	if ( master) {
		if (( ptr = PWidget( master)-> packSlaves) != self) {
			if ( ptr) {
				while ( PWidget(ptr)-> geomInfo. next) {
					if ( PWidget(ptr)-> geomInfo. next == self) {
						PWidget(ptr)-> geomInfo. next = var-> geomInfo. next;
						break;
					}
					ptr = PWidget(ptr)-> geomInfo. next;
				}
			}
		} else {
			PWidget( master)-> packSlaves = var-> geomInfo. next;
		}
	}

	var-> geomInfo. next = NULL_HANDLE;
}

SV *
Widget_packInfo( Handle self, Bool set, SV * packInfo)
{
	if ( !set) {
		HV * profile = newHV();
		GeomInfo *p = &var-> geomInfo;

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

		pset_H( after,  ( p-> order && p-> after)  ? p-> order : NULL_HANDLE);
		pset_H( before, ( p-> order && !p-> after) ? p-> order : NULL_HANDLE);
		pset_H( in, var-> geomInfo. in);

		pset_i( ipadx, p-> ipad. x);
		pset_i( ipady, p-> ipad. y);
		pset_i( padx, p-> pad. x);
		pset_i( pady, p-> pad. y);

		return newRV_noinc(( SV *) profile);
	} else {
		dPROFILE;
		HV * profile;
		Bool reset_zorder = false, set_in = false;
		Handle in = NULL_HANDLE;

		if ( !SvOK(packInfo)) return NULL_SV;

		if ( !SvOK(packInfo) || !SvROK(packInfo) || SvTYPE(SvRV(packInfo)) != SVt_PVHV)
			croak("Widget::packInfo: parameter is not a hash");

		profile = ( HV*) SvRV( packInfo);
		if ( pexist( side)) {
			char * c = pget_c( side);
			if ( *c == 'l' && (strcmp( c, "left")==0))   var-> geomInfo. side = LEFT; else
			if ( *c == 'b' && (strcmp( c, "bottom")==0)) var-> geomInfo. side = BOTTOM; else
			if ( *c == 'r' && (strcmp( c, "right")==0))  var-> geomInfo. side = RIGHT; else
			if ( *c == 't' && (strcmp( c, "top")==0))    var-> geomInfo. side = TOP; else
				croak("%s: invalid 'side'", "Prima::Widget::pack");
		}

		if ( pexist( fill)) {
			char * c = pget_c( fill);
			if (( strcmp( c, "x") == 0)) {
				var-> geomInfo. fillx = 1;
				var-> geomInfo. filly = 0;
			} else if (( strcmp( c, "y") == 0)) {
				var-> geomInfo. fillx = 0;
				var-> geomInfo. filly = 1;
			} else if ( *c == 'n' && ( strcmp( c, "none") == 0)) {
				var-> geomInfo. fillx =
				var-> geomInfo. filly = 0;
			} else if ( *c == 'b' && ( strcmp( c, "both") == 0)) {
				var-> geomInfo. fillx =
				var-> geomInfo. filly = 1;
			} else
				croak("%s: invalid 'fill'", "Prima::Widget::pack");
		}

		if ( pexist( expand)) {
			var-> geomInfo. expand = pget_B( expand);
		}

		if ( pexist( anchor)) {
			char * c = pget_c( anchor);
			if (( strcmp( c, "n") == 0)) {
				var-> geomInfo. anchorx = CENTER;
				var-> geomInfo. anchory = NORTH;
			} else if (( strcmp( c, "ne") == 0)) {
				var-> geomInfo. anchorx = EAST;
				var-> geomInfo. anchory = NORTH;
			} else if (( strcmp( c, "e") == 0)) {
				var-> geomInfo. anchorx = EAST;
				var-> geomInfo. anchory = CENTER;
			} else if (( strcmp( c, "se") == 0)) {
				var-> geomInfo. anchorx = EAST;
				var-> geomInfo. anchory = SOUTH;
			} else if (( strcmp( c, "s") == 0)) {
				var-> geomInfo. anchorx = CENTER;
				var-> geomInfo. anchory = SOUTH;
			} else if (( strcmp( c, "sw") == 0)) {
				var-> geomInfo. anchorx = WEST;
				var-> geomInfo. anchory = SOUTH;
			} else if (( strcmp( c, "w") == 0)) {
				var-> geomInfo. anchorx = WEST;
				var-> geomInfo. anchory = CENTER;
			} else if (( strcmp( c, "nw") == 0)) {
				var-> geomInfo. anchorx = WEST;
				var-> geomInfo. anchory = NORTH;
			} else if ( *c == 'c' && ( strcmp( c, "center") == 0)) {
				var-> geomInfo. anchorx = CENTER;
				var-> geomInfo. anchory = CENTER;
			} else
				croak("%s: invalid 'anchor'", "Prima::Widget::pack");
		}

		if ( pexist( ipad))  var-> geomInfo. ipad. x = var-> geomInfo. ipad. y = pget_i( ipad);
		if ( pexist( ipadx)) var-> geomInfo. ipad. x = pget_i( ipadx);
		if ( pexist( ipady)) var-> geomInfo. ipad. y = pget_i( ipady);
		if ( pexist( pad))   var-> geomInfo. pad. x  = var-> geomInfo. pad. y = pget_i( pad);
		if ( pexist( padx))  var-> geomInfo. pad. x  = pget_i( padx);
		if ( pexist( pady))  var-> geomInfo. pad. y  = pget_i( pady);

		if ( pexist( after)) {
			SV * sv = pget_sv( after);
			if ( SvOK(sv)) {
				if ( !( var-> geomInfo. order = gimme_the_mate( sv)))
					croak("%s: invalid 'after'", "Prima::Widget::pack");
				var-> geomInfo. after = 1;
				if ( pexist( before)) {
					sv = pget_sv( before);
					if ( SvOK(sv))
						croak("%s: 'after' and 'before' cannot be present simultaneously", "Prima::Widget::pack");
				}
			} else {
				var-> geomInfo. order = NULL_HANDLE;
				var-> geomInfo. after = 0;
			}
			reset_zorder = true;
		} else if ( pexist( before)) {
			SV * sv = pget_sv( before);
			if ( SvOK(sv)) {
				if ( !( var-> geomInfo. order = gimme_the_mate( sv)))
					croak("%s: invalid 'before'", "Prima::Widget::pack");
			} else
				var-> geomInfo. order = NULL_HANDLE;
			var-> geomInfo. after = 0;
			reset_zorder = true;
		}

		if ( pexist( in)) {
			SV * sv = pget_sv( in);
			in = NULL_HANDLE;
			if ( SvOK( sv))
				in = Widget_check_in( self, gimme_the_mate( sv), true);
			set_in = reset_zorder = true;
		}

		if ( var-> geometry == gtPack) {
			if ( reset_zorder)
				Widget_pack_leave( self);
		}
		if ( set_in) var-> geomInfo. in = in;
		if ( var-> geometry == gtPack) {
			if ( reset_zorder)
				Widget_pack_enter( self);
			geometry_reset( MASTER, gtPack);
		}
	}
	return NULL_SV;
}

XS( Widget_get_pack_slaves_FROMPERL)
{
	dXSARGS;
	Handle self;

	if ( items != 1)
		croak ("Invalid usage of Widget.get_pack_slaves");
	SP -= items;
	self = gimme_the_mate( ST( 0));
	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to Widget.get_pack_slaves");
	self = var-> packSlaves;
	while ( self) {
		XPUSHs( sv_2mortal( newSVsv((( PAnyObject) self)-> mate)));
		self = var-> geomInfo. next;
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
Widget_place_enter( Handle self)
{
	Handle master, ptr;

	/* see if leftover object references are alive */
	if ( var-> geomInfo. in) {
		if ( hash_fetch( prima_guts.objects, &var-> geomInfo. in, sizeof(Handle)))
			var-> geomInfo. in = Widget_check_in( self, var-> geomInfo. in, false);
		else
			var-> geomInfo. in = NULL_HANDLE;
	}

	/* store into slaves list */
	master = (( var-> geomInfo. in) ? var-> geomInfo. in : var-> owner);

	if ( PWidget( master)-> placeSlaves) {
		/* append to the end of list  */
		if (( ptr = PWidget( master)-> placeSlaves)) {
			while ( PWidget( ptr)-> geomInfo. next)
				ptr = PWidget( ptr)-> geomInfo. next;
			PWidget( ptr)-> geomInfo. next = self;
		} else {
			/* first in list */
			var-> geomInfo. next = ptr;
			PWidget( master)-> placeSlaves = self;
		}
	} else { /* master has no slaves, we're first */
		PWidget( master)-> placeSlaves = self;
	}
}

/* removes widget from list of place slaves */
void
Widget_place_leave( Handle self)
{
	Handle ptr, master;

	master = (( var-> geomInfo. in) ? var-> geomInfo. in : var-> owner);

	if ( master) {
		if (( ptr = PWidget( master)-> placeSlaves) != self) {
			if ( ptr) {
				while ( PWidget(ptr)-> geomInfo. next) {
					if ( PWidget(ptr)-> geomInfo. next == self) {
						PWidget(ptr)-> geomInfo. next = var-> geomInfo. next;
						break;
					}
					ptr = PWidget(ptr)-> geomInfo. next;
				}
			}
		} else {
			PWidget( master)-> placeSlaves = var-> geomInfo. next;
		}
	}

	var-> geomInfo. next = NULL_HANDLE;
}

void
Widget_place_slaves( Handle self)
{
	PWidget slave, master;
	int x, y, width, height, tmp;
	int masterWidth, masterHeight;
	double x1, y1, x2, y2;
	Point size;


	/*
	* Iterate over all the slaves for the master.  Each slave's
	* geometry can be computed independently of the other slaves.
	*/

	if (!( master = ( PWidget) var-> placeSlaves)) return;
	size = my-> get_size( self);
	masterWidth  = size. x;
	masterHeight = size. y;

	for (
		slave = master;
		slave != NULL;
		slave = ( PWidget) slave-> geomInfo. next
	) {
		Point sz;
		register GeomInfo* slavePtr = &slave-> geomInfo;

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
			y -= height;
			break;
		case CENTER:
			y -= height/2;
			break;
		case SOUTH:
			break;
		}

		{
			Rect r;
			r. left   = x;
			r. bottom = y;
			r. right  = x + width;
			r. top    = y + height;

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
		GeomInfo *p = &var-> geomInfo;

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

		pset_H( in, var-> geomInfo. in);

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
		dPROFILE;
		HV * profile;
		Handle in = NULL_HANDLE;
		Bool set_in = false;

		if ( !SvOK(placeInfo)) return NULL_SV;

		if ( !SvOK(placeInfo) || !SvROK(placeInfo) || SvTYPE(SvRV(placeInfo)) != SVt_PVHV)
			croak("Widget::placeInfo: parameter is not a hash");

		profile = ( HV*) SvRV( placeInfo);

		if ( pexist( anchor)) {
			char * c = pget_c( anchor);
			if (( strcmp( c, "n") == 0)) {
				var-> geomInfo. anchorx = CENTER;
				var-> geomInfo. anchory = NORTH;
			} else if (( strcmp( c, "ne") == 0)) {
				var-> geomInfo. anchorx = EAST;
				var-> geomInfo. anchory = NORTH;
			} else if (( strcmp( c, "e") == 0)) {
				var-> geomInfo. anchorx = EAST;
				var-> geomInfo. anchory = CENTER;
			} else if (( strcmp( c, "se") == 0)) {
				var-> geomInfo. anchorx = EAST;
				var-> geomInfo. anchory = SOUTH;
			} else if (( strcmp( c, "s") == 0)) {
				var-> geomInfo. anchorx = CENTER;
				var-> geomInfo. anchory = SOUTH;
			} else if (( strcmp( c, "sw") == 0)) {
				var-> geomInfo. anchorx = WEST;
				var-> geomInfo. anchory = SOUTH;
			} else if (( strcmp( c, "w") == 0)) {
				var-> geomInfo. anchorx = WEST;
				var-> geomInfo. anchory = CENTER;
			} else if (( strcmp( c, "nw") == 0)) {
				var-> geomInfo. anchorx = WEST;
				var-> geomInfo. anchory = NORTH;
			} else if ( *c == 'c' && ( strcmp( c, "center") == 0)) {
				var-> geomInfo. anchorx = CENTER;
				var-> geomInfo. anchory = CENTER;
			} else
				croak("%s: invalid 'anchor'", "Prima::Widget::place");
		}

		if ( pexist( x)) {
			SV * sv = pget_sv( x);
			if (( var-> geomInfo. use_x = ( SvOK(sv) ? 1 : 0 )))
				var-> geomInfo. x = SvIV( sv);
		}
		if ( pexist( y)) {
			SV * sv = pget_sv( y);
			if (( var-> geomInfo. use_y = ( SvOK(sv) ? 1 : 0 )))
				var-> geomInfo. y = SvIV( sv);
		}
		if ( pexist( width)) {
			SV * sv = pget_sv( width);
			if (( var-> geomInfo. use_w = ( SvOK(sv) ? 1 : 0 )))
				var-> geomSize. x = SvIV( sv);
		}
		if ( pexist( height)) {
			SV * sv = pget_sv( height);
			if (( var-> geomInfo. use_h = ( SvOK(sv) ? 1 : 0 )))
				var-> geomSize. y = SvIV( sv);
		}
		if ( pexist( relx)) {
			SV * sv = pget_sv( relx);
			if (( var-> geomInfo. use_rx = ( SvOK(sv) ? 1 : 0 )))
				var-> geomInfo. relX = SvNV( sv);
		}
		if ( pexist( rely)) {
			SV * sv = pget_sv( rely);
			if (( var-> geomInfo. use_ry = ( SvOK(sv) ? 1 : 0 )))
				var-> geomInfo. relY = SvNV( sv);
		}
		if ( pexist( relwidth)) {
			SV * sv = pget_sv( relwidth);
			if (( var-> geomInfo. use_rw = ( SvOK(sv) ? 1 : 0 )))
				var-> geomInfo. relWidth = SvNV( sv);
		}
		if ( pexist( relheight)) {
			SV * sv = pget_sv( relheight);
			if (( var-> geomInfo. use_rh = ( SvOK(sv) ? 1 : 0 )))
				var-> geomInfo. relHeight = SvNV( sv);
		}

		if ( pexist( in)) {
			SV * sv = pget_sv( in);
			in = NULL_HANDLE;
			if ( SvOK(sv))
				in = Widget_check_in( self, gimme_the_mate( sv), true);
			set_in = true;
		}

		if ( var-> geometry == gtPlace) {
			if ( set_in)
				Widget_place_leave( self);
		}
		if ( set_in) var-> geomInfo. in = in;
		if ( var-> geometry == gtPlace) {
			if ( set_in)
				Widget_place_enter( self);
			geometry_reset( MASTER, gtPlace);
		}
	}

	return NULL_SV;
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
	if ( self == NULL_HANDLE)
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

