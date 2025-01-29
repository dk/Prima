#include "apricot.h"
#include "guts.h"
#include "Widget.h"
#include "Widget_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * tkGrid.c --
 *
 *        Grid based geometry manager.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: tkGrid.c,v 1.25 2002/10/10 21:07:51 pspjuth Exp $
 */

/*
 * Convenience Macros
 */

#ifdef MAX
#   undef MAX
#endif
#define MAX(x,y)        ((x) > (y) ? (x) : (y))
#ifdef MIN
#   undef MIN
#endif
#define MIN(x,y)        ((x) > (y) ? (y) : (x))

#define COLUMN       (1)        /* working on column offsets */
#define ROW          (2)        /* working on row offsets */

#define CHECK_ONLY   (1)        /* check max slot constraint */
#define CHECK_SPACE  (2)        /* alloc more space, don't change max */

/*
 * Pre-allocate enough row and column slots for "typical" sized tables
 * this value should be chosen so by the time the extra malloc's are
 * required, the layout calculations overwehlm them. [A "slot" contains
 * information for either a row or column, depending upon the context.]
 */

#define TYPICAL_SIZE        25  /* (arbitrary guess) */
#define PREALLOC            10  /* extra slots to allocate */

/*
 * Pre-allocate room for uniform groups during layout.
 */

#define UNIFORM_PREALLOC 10

/*
 * Data structures are allocated dynamically to support arbitrary sized tables.
 * However, the space is proportional to the highest numbered slot with
 * some non-default property.  This limit is used to head off mistakes and
 * denial of service attacks by limiting the amount of storage required.
 */

#define MAX_ELEMENT        10000

/*
 * Special characters to support relative layouts.
 */

#define REL_SKIP        'x'        /* Skip this column. */
#define REL_HORIZ        '-'        /* Extend previous widget horizontally. */
#define REL_VERT        '^'        /* Extend widget from row above. */

/*
 *  Structure to hold information for grid masters.  A slot is either
 *  a row or column.
 */

typedef struct SlotInfo {
	int minSize;            /* The minimum size of this slot (in pixels).
				 * It is set via the rowconfigure or
				 * columnconfigure commands. */
	int weight;             /* The resize weight of this slot. (0) means
				 * this slot doesn't resize. Extra space in
				 * the layout is given distributed among slots
				 * inproportion to their weights. */
	int pad;                /* Extra padding, in pixels, required for
				 * this slot.  This amount is "added" to the
				 * largest slave in the slot. */
	int uniform;            /* Value of -uniform option. It is used to
				 * group slots that should have the same
				 * size. */
	int offset;             /* This is a cached value used for
				 * introspection.  It is the pixel
				 * offset of the right or bottom edge
				 * of this slot from the beginning of the
				 * layout. */
	 int temp;              /* This is a temporary value used for
				 * calculating adjusted weights when
				 * shrinking the layout below its
				 * nominal size. */
} SlotInfo;

/*
 * Structure to hold information during layout calculations.  There
 * is one of these for each slot, an array for each of the rows or columns.
 */

typedef struct GridLayout {
	struct Gridder *binNextPtr; /* The next slave window in this bin.
				 * Each bin contains a list of all
				 * slaves whose spans are >1 and whose
				 * right edges fall in this slot. */
	int minSize;            /* Minimum size needed for this slot,
				 * in pixels.  This is the space required
				 * to hold any slaves contained entirely
				 * in this slot, adjusted for any slot
				 * constrants, such as size or padding. */
	int pad;                /* Padding needed for this slot */
	int weight;             /* Slot weight, controls resizing. */
	unsigned int uniform;   /* Value of -uniform option. It is used to
				 * group slots that should have the same
				 * size. */
	int minOffset;          /* The minimum offset, in pixels, from
				 * the beginning of the layout to the
				 * right/bottom edge of the slot calculated
				 * from top/left to bottom/right. */
	int maxOffset;          /* The maximum offset, in pixels, from
				 * the beginning of the layout to the
				 * right-or-bottom edge of the slot calculated
				 * from bottom-or-right to top-or-left. */
} GridLayout;

/*
 * Keep one of these for each geometry master.
 */

typedef struct {
	SlotInfo *columnPtr;        /* Pointer to array of column constraints. */
	SlotInfo *rowPtr;           /* Pointer to array of row constraints.    */
	int columnEnd;              /* The last column occupied by any slave.  */
	int columnMax;              /* The number of columns with constraints. */
	int columnSpace;            /* The number of slots currently allocated for
	                             * column constraints.                     */
	int rowEnd;                 /* The last row occupied by any slave.     */
	int rowMax;                 /* The number of rows with constraints.    */
	int rowSpace;               /* The number of slots currently allocated
	                             * for row constraints.                    */
	int startX;                 /* Pixel offset of this layout within its
	                             * parent.                                 */
	int startY;                 /* Pixel offset of this layout within its
	                             * parent.                                 */
} GridMaster;

/*
 * For each window that the grid cares about (either because
 * the window is managed by the grid or because the window
 * has slaves that are managed by the grid), there is a
 * structure of the following type:
 */

typedef struct Gridder {
	Handle window;                /* Tk token for window.  NULL means that
	                               * the window has been deleted, but the
	                               * gridder hasn't had a chance to clean up
	                               * yet because the structure is still in
	                               * use. */
	struct Gridder *masterPtr;    /* Master window within which this window
	                               * is managed (NULL means this window
	                               * isn't managed by the gridder). */
	struct Gridder *nextPtr;      /* Next window managed within same
	                               * parent.  List order doesn't matter. */
	struct Gridder *slavePtr;     /* First in list of slaves managed
	                               * inside this window (NULL means
	                               * no grid slaves). */
	GridMaster *masterDataPtr;    /* Additional data for geometry master. */
	int column, row;              /* Location in the grid (starting
	                               * from zero). */
	int numCols, numRows;         /* Number of columns or rows this slave spans.
	                               * Should be at least 1. */
	int padX, padY;               /* Total additional pixels to leave around the
	                               * window.  Some is of this space is on each
	                               * side.  This is space *outside* the window:
	                               * we'll allocate extra space in frame but
	                               * won't enlarge window). */
	int padLeft, padBottom;          /* The part of padX or padY to use on the
	                               * left or top of the widget, respectively.
	                               * By default, this is half of padX or padY. */
	int iPadX, iPadY;             /* Total extra pixels to allocate inside the
	                               * window (half this amount will appear on
	                               * each side). */
	int sticky;                   /* which sides of its cavity this window
	                               * sticks to. See below for definitions */
	int *abortPtr;                /* If non-NULL, it means that there is a nested
	                               * call to ArrangeGrid already working on
	                               * this window.  *abortPtr may be set to 1 to
	                               * abort that nested call.  This happens, for
	                               * example, if window or any of its slaves
	                               * is deleted. */
	int flags;                    /* Miscellaneous flags;  see below
                                       * for definitions. */

	Handle saved_in;              /* remember eventual in, if not empty, otherwise owner */

	/*
	 * These fields are used temporarily for layout calculations only.
	 */

	struct Gridder *binNextPtr;   /* Link to next span>1 slave in this bin. */
	int size;                     /* Nominal size (width or height) in pixels
                                       * of the slave.  This includes the padding. */
} Gridder;

/* Flag values for "sticky"ness  The 16 combinations subsume the packer's
 * notion of anchor and fill.
 *
 * STICK_NORTH          This window sticks to the top of its cavity.
 * STICK_EAST           This window sticks to the right edge of its cavity.
 * STICK_SOUTH          This window sticks to the bottom of its cavity.
 * STICK_WEST           This window sticks to the left edge of its cavity.
 */

#define STICK_NORTH               1
#define STICK_EAST                2
#define STICK_SOUTH               4
#define STICK_WEST                8


/*
 * Structure to gather information about uniform groups during layout.
 */

typedef struct UniformGroup {
	unsigned int group;
	int minSize;
} UniformGroup;

/*
 * Flag values for Grid structures:
 *
 * DONT_PROPAGATE:                1 means don't set this window's requested
 *                                size.  0 means if this window is a master
 *                                then Tk will set its requested size to fit
 *                                the needs of its slaves.
 */

#define DONT_PROPAGATE            2

/*
 * Prototypes for procedures used only in this file:
 */

static void     AdjustForSticky(Gridder *slavePtr, int *xPtr, int *yPtr, int *widthPtr, int *heightPtr);
static int      AdjustOffsets(int width, int elements, register SlotInfo *slotPtr);
static void     ArrangeGrid(Gridder *masterPtr);
static Bool     CheckSlotData(Gridder *masterPtr, int slot, int slotType, int checkOnly);
static void     ConfigureSlaves(Handle self, SV * window, HV * profile);
static void     DestroyGrid(Handle self);
static Gridder* GetGrid(Handle self);
static Bool     GridBboxCommand(Handle self, PList in, PList out);
static Bool     GridInfoCommand(Handle self, PList in, PList out);
static Bool     GridForgetRemoveCommand(Handle self, Bool forget, PList in, PList out);
static Bool     GridLocationCommand(Handle self, PList in, PList out);
static Bool     GridPropagateCommand(Handle self, PList in, PList out);
static Bool     GridRowColumnConfigureCommand(Handle self, int slotType, PList in, PList out);
static Bool     GridSizeCommand(Handle self, PList in, PList out);
static void     InitMasterData(Gridder *masterPtr);
static int      ResolveConstraints(Gridder *gridPtr, int rowOrColumn, int maxOffset);
static void     SetGridSize(Gridder *gridPtr);
static void     StickyToString(int flags, char *result);
static int      StringToSticky(char *string);
static void     Unlink(Gridder *gridPtr);

#define ARGsv(x) ((SV*)(in->items[x]))
#define ARGi(x)  SvIV(ARGsv(x))
#define ARGs(x)  SvPV_nolen(ARGsv(x))
#define RETsv(x) list_add(out, (Handle)(x))
#define RETi(x)  list_add(out, (Handle) newSViv(x))
#define RETs(x)  list_add(out, (Handle) newSVpv(x,0))

/*
 *----------------------------------------------------------------------
 *
 * GridBboxCommand --
 *
 *	Implementation of the [grid bbox] subcommand.
 *
 *
 *----------------------------------------------------------------------
 */
static Bool
GridBboxCommand(Handle self, PList in, PList out)
{
	Gridder *masterPtr;  /* master grid record */
	GridMaster *gridPtr; /* pointer to grid data */
	int row, column;     /* origin for bounding box */
	int row2, column2;   /* end of bounding box */
	int endX, endY;      /* last column/row in the layout */

	Box r = {0,0,0,0};

	switch (in->count) {
	case 0:
		row = column = row2 = column2 = 0;
		break;
	case 2:
		column = column2 = ARGi(0);
		row    = row2    = ARGi(1);
		break;
	case 4:
		column  = ARGi(0);
		row     = ARGi(1);
		column2 = ARGi(2);
		row2    = ARGi(3);
		break;
	default:
		return false;
	}

	masterPtr = GetGrid(self);
	if ( !( gridPtr = masterPtr->masterDataPtr))
		return false;

	SetGridSize(masterPtr);
	endX = MAX(gridPtr->columnEnd, gridPtr->columnMax);
	endY = MAX(gridPtr->rowEnd, gridPtr->rowMax);

	if ((endX == 0) || (endY == 0))
		goto EXIT;

	if ( in-> count == 0 ) {
		row = column = 0;
		row2 = endY;
		column2 = endX;
	}

	if (column > column2) {
		int temp = column;
		column = column2, column2 = temp;
	}
	if (row > row2) {
		int temp = row;
		row = row2, row2 = temp;
	}

	if (column > 0 && column < endX) {
		r.x = gridPtr->columnPtr[column-1].offset;
	} else if  (column > 0) {
		r.x = gridPtr->columnPtr[endX-1].offset;
	}

	if (row > 0 && row < endY) {
		r.y = gridPtr->rowPtr[row-1].offset;
	} else if (row > 0) {
		r.y = gridPtr->rowPtr[endY-1].offset;
	}

	if (column2 < 0) {
		r.width = 0;
	} else if (column2 >= endX) {
		r.width = gridPtr->columnPtr[endX-1].offset - r.x;
	} else {
		r.width = gridPtr->columnPtr[column2].offset - r.x;
	}

	if (row2 < 0) {
		r.height = 0;
	} else if (row2 >= endY) {
		r.height = gridPtr->rowPtr[endY-1].offset - r.y;
	} else {
		r.height = gridPtr->rowPtr[row2].offset - r.y;
	}

	r.x += gridPtr->startX;
	r.y += gridPtr->startY;

EXIT:
	RETi(r.x);
	RETi(r.y);
	RETi(r.width);
	RETi(r.height);
	return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GridForgetRemoveCommand --
 *
 *	Implementation of the [grid forget]/[grid remove] subcommands.
 *	See the user documentation for details on what these do.
 *
 * Side effects:
 *	Removes a window from a grid layout.
 *
 *----------------------------------------------------------------------
 */


static Handle
get_slave( Handle self, SV * window)
{
	if ( !SvOK(window))
		return false;
	return SvROK(window) ?
		gimme_the_mate(window) :
		my->bring(self, SvPV_nolen(window), is_opt(optDeepLookup) ? 1000 : 0);
}

static Bool
GridForgetRemoveCommand(Handle self, Bool forget, PList in, PList out)
{
	int i;

	for (i = 0; i < in->count; i++) {
		Handle slave = get_slave(self, ARGsv(i));
		Gridder *slavePtr = GetGrid(slave), *masterPtr = slavePtr->masterPtr;
		if (!masterPtr)
			continue;

		/*
		* For "forget", reset all the settings to their defaults
		*/
		if (forget) {
			slavePtr->column = slavePtr->row = -1;
			slavePtr->numCols = 1;
			slavePtr->numRows = 1;
			slavePtr->padX = slavePtr->padY = 0;
			slavePtr->padLeft = slavePtr->padBottom = 0;
			slavePtr->iPadX = slavePtr->iPadY = 0;
			slavePtr->flags = 0;
			slavePtr->sticky = 0;
		}
		Unlink(slavePtr);
		CWidget(slavePtr->window)->hide(slavePtr->window);
		CWidget(slave)->set_geometry(self, gtGrowMode);
		SetGridSize(masterPtr);
		ArrangeGrid(masterPtr);
	}

	return true;
}


/*
 *----------------------------------------------------------------------
 *
 * GridInfoCommand --
 *
 *        Implementation of the [grid info] subcommand.  See the user
 *        documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

static SV*
two_ints(int a, int b)
{
	AV * av = newAV();
	av_push(av, newSViv(a));
	av_push(av, newSViv(b));
	return newRV_noinc((SV*) av);
}

static Bool
GridInfoCommand(Handle self, PList in, PList out)
{
	HV * profile = newHV();
	register Gridder *p;
	char buffer[4];

	if ( in->count != 0 )
		return false;

	p = GetGrid(self);

	pset_H( in      , p-> saved_in );
	pset_i( column  , p-> column   );
	pset_i( row     , p-> row      );
	pset_i( colspan , p-> numCols  );
	pset_i( rowspan , p-> numRows  );
	pset_i( ipadx   , p-> iPadX    );
	pset_i( ipady   , p-> iPadY    );
	if ( p->padX != p->padLeft )
		pset_sv( padx, two_ints(p->padLeft,p->padX) );
	else
		pset_i( padx, p-> padX );
	if ( p->padY != p->padBottom )
		pset_sv( pady, two_ints(p->padBottom,p->padY) );
	else
		pset_i( pady, p-> padY );

	StickyToString(p->sticky,buffer);
	pset_c( sticky  , buffer);

	RETsv(newRV_noinc(( SV *) profile));
	return true;
}


/*
 *----------------------------------------------------------------------
 *
 * GridLocationCommand --
 *
 *        Implementation of the [grid location] subcommand.  See the user
 *        documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

static Bool
GridLocationCommand(Handle self, PList in, PList out)
{
	Gridder *masterPtr;         /* master grid record */
	GridMaster *gridPtr;        /* pointer to grid data */
	register SlotInfo *slotPtr;
	int x, y;                   /* Offset in pixels, from edge of parent. */
	int endX, endY;             /* end of grid */
	Point r = {-1,-1};          /* Corresponding column and row indeces. */ 

	if (in->count != 2)
		return false;

	masterPtr = GetGrid(self);
	if (masterPtr->masterDataPtr == NULL)
		return false;
	x = ARGi(0);
	y = ARGi(1);

	gridPtr = masterPtr->masterDataPtr;

	SetGridSize(masterPtr);
	endX = MAX(gridPtr->columnEnd, gridPtr->columnMax);
	endY = MAX(gridPtr->rowEnd, gridPtr->rowMax);

	slotPtr  = masterPtr->masterDataPtr->columnPtr;
	if (x >= masterPtr->masterDataPtr->startX) {
		x -= masterPtr->masterDataPtr->startX;
		for (r.x = 0; slotPtr[r.x].offset < x && r.x < endX; r.x++) {
			/* null body */
		}
	}

	slotPtr  = masterPtr->masterDataPtr->rowPtr;
	if (y >= masterPtr->masterDataPtr->startY) {
		y -= masterPtr->masterDataPtr->startY;
		for (r.y = 0; slotPtr[r.y].offset < y && r.y < endY; r.y++) {
			/* null body */
		}
	}

	RETi(r.x);
	RETi(r.y);
	return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GridPropagateCommand --
 *
 *        Implementation of the [grid propagate] subcommand.  See the user
 *        documentation for details on what it does.
 *
 * Side effects:
 *        May alter geometry propagation for a widget.
 *
 *----------------------------------------------------------------------
 */

static Bool
GridPropagateCommand(Handle self, PList in, PList out)
{
	Gridder *masterPtr;
	int propagate, old;

	if ( in->count > 1 )
		return false;

	masterPtr = GetGrid(self);
	if ( in->count == 0 ) {
		RETi( !(masterPtr->flags & DONT_PROPAGATE)) ;
		return true;
	}
	propagate = SvBOOL(ARGsv(1));

	/* Only request a relayout if the propagation bit changes */
	old = !(masterPtr->flags & DONT_PROPAGATE);
	if (propagate != old) {
		if (propagate) {
			masterPtr->flags &= ~DONT_PROPAGATE;
		} else {
			masterPtr->flags |= DONT_PROPAGATE;
		}

		/*
		 * Re-arrange the master to allow new geometry information to
		 * propagate upwards to the master's master.
		 */

		if (masterPtr->abortPtr != NULL) {
			*masterPtr->abortPtr = 1;
		}
		ArrangeGrid (masterPtr);
	}

	return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GridRowColumnConfigureCommand --
 *
 *        Implementation of the [grid rowconfigure] and [grid columnconfigure]
 *        subcommands.  See the user documentation for details on what these
 *        do.
 *
 * Side effects:
 *        Depends on arguments; see user documentation.
 *
 *----------------------------------------------------------------------
 */

static Bool
GridRowColumnConfigureCommand(Handle self, int slotType, PList in, PList out)
{
	Gridder *masterPtr;
	SlotInfo *slotPtr = NULL;
	int slot;                    /* the column or row number */
	//int slotType;                /* COLUMN or ROW */
	int checkOnly;               /* check the size only */
	int j, *indexes, n_indexes;
	SV *index;

#define METHOD (slotType == COLUMN) ? "column" : "row"

	if ((((in->count % 2) == 0) && (in->count > 3)) || (in->count < 1))
		return false;

	index = ARGsv(0);
	if (!SvOK(index) || (SvROK(index) && SvTYPE(SvRV(index)) != SVt_PVAV))
		croak("Widget::grid(%sconfigure): index is neither a number nor an array of numbers", METHOD);

	if (SvROK(index)) {
		if ( !( indexes = (int*) prima_read_array(index, "Widget::gridInfo", 'i', 1, 0, 0, &n_indexes, NULL)))
			croak("Widget::grid(%sconfigure): cannot read array of indexes, or the array is empty", METHOD);
	} else {
		indexes   = malloc(sizeof(int));
		*indexes  = SvIV(index);
		n_indexes = 1;
	}

	masterPtr = GetGrid(self);
	checkOnly = ((in->count == 1) || (in->count == 2));
	if (checkOnly && n_indexes > 1)
		croak("Widget::grid(%sconfigure): row must be a single element", METHOD);

	for ( slot = 0; slot < n_indexes; slot++) {
		Bool ok = CheckSlotData(masterPtr, slot, slotType, checkOnly);
		if (!ok && !checkOnly)
			croak("Widget::grid(%sconfigure %d): is out of range", METHOD, slot);
		slotPtr = (slotType == COLUMN) ?
			masterPtr->masterDataPtr->columnPtr :
			masterPtr->masterDataPtr->rowPtr;

		/*
		 * Return all of the options for this row or column.  If the
		 * request is out of range, return all 0's.
		 */

		if (in->count == 1) {
			int minsize = 0, pad = 0, weight = 0, uniform = 0;

			if (ok) {
				minsize = slotPtr[slot].minSize;
				pad     = slotPtr[slot].pad;
				weight  = slotPtr[slot].weight;
				uniform = slotPtr[slot].uniform;
			}
			RETs("minsize");
			RETi(minsize);
			RETs("pad");
			RETi(pad);
			RETs("uniform");
			RETi(uniform);
			RETs("weight");
			RETi(weight);
			return true;
		}

		/*
		 * Loop through each option value pair, setting the values as
		 * required.  If only one option is given, with no value, the
		 * current value is returned.
		 */

		for ( j = 1; j < in->count; j += 2) {
			int   i, *p;
			char *f = ARGs(j);
			     if ( strcmp(f, "minsize") == 0) p = &slotPtr[slot].minSize;
			else if ( strcmp(f, "weight" ) == 0) p = &slotPtr[slot].weight ;
			else if ( strcmp(f, "uniform") == 0) p = &slotPtr[slot].uniform;
			else if ( strcmp(f, "pad"    ) == 0) p = &slotPtr[slot].pad    ;
			else croak("Widget::grid(%sconfigure): bad option `%s'", METHOD, f);

			if ( in->count == 2 ) {
				i = ok ? *p : 0;
				RETi(i);
			} else {
				i = ARGi(j+1);
				if ( i < 0 )
					croak("Widget::grid(%sconfigure): bad option `%s': should be non-negative", METHOD, f);
				*p = i;
			}
		}
	}

	/*
	 * If we changed a property, re-arrange the table,
	 * and check for constraint shrinkage.
	 */

	if ( in->count != 2 ) {
		if (slotType == ROW) {
			int last = masterPtr->masterDataPtr->rowMax - 1;
			while ((last >= 0) && (slotPtr[last].weight == 0)
					&& (slotPtr[last].pad == 0)
					&& (slotPtr[last].minSize == 0)
					&& (slotPtr[last].uniform == 0)) {
				last--;
			}
			masterPtr->masterDataPtr->rowMax = last+1;
		} else {
			int last = masterPtr->masterDataPtr->columnMax - 1;
			while ((last >= 0) && (slotPtr[last].weight == 0)
					&& (slotPtr[last].pad == 0)
					&& (slotPtr[last].minSize == 0)
					&& (slotPtr[last].uniform == 0)) {
				last--;
			}
			masterPtr->masterDataPtr->columnMax = last + 1;
		}
	}

	if (masterPtr->abortPtr != NULL) {
		*masterPtr->abortPtr = 1;
	}
	ArrangeGrid(masterPtr);
#undef METHOD
	return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GridSizeCommand --
 *
 *        Implementation of the [grid size] subcommand.  See the user
 *        documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

static Bool
GridSizeCommand(Handle self, PList in, PList out)
{
	Gridder *masterPtr;
	GridMaster *gridPtr;        /* pointer to grid data */
	Point r = {0,0};

	if ( in->count != 0 )
		return false;

	masterPtr = GetGrid(self);

	if (masterPtr->masterDataPtr != NULL) {
		SetGridSize(masterPtr);
		gridPtr = masterPtr->masterDataPtr;
		r.x = MAX(gridPtr->columnEnd, gridPtr->columnMax);
		r.y = MAX(gridPtr->rowEnd, gridPtr->rowEnd);
	}

	RETi(r.x);
	RETi(r.y);

	return true;
}

/*
 *----------------------------------------------------------------------
 *
 * GridSlavesCommand --
 *
 *        Implementation of the [grid slaves] subcommand.  See the user
 *        documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

Bool
GridSlavesCommand( Handle self, PList in, PList out)
{
	int slot, slotType;
	char *cmd;
	Gridder *masterPtr;
	Gridder *slavePtr;

	if (in->count != 2)
		return false;

	cmd = ARGs(0);
	if ( strcmp(cmd, "row") == 0)
		slotType = ROW;
	else if ( strcmp(cmd, "column") == 0)
		slotType = COLUMN;
	else
		return false;

	slot = ARGi(1);
	if ( slot < 0 )
		croak("Widget::grid(slaves): is an invalid value: should NOT be < 0");

	masterPtr = GetGrid(self);
	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL; slavePtr = slavePtr->nextPtr) {
		if (slotType == COLUMN && (slavePtr->column > slot
				|| slavePtr->column+slavePtr->numCols-1 < slot)) {
			continue;
		}
		if (slotType == ROW && (slavePtr->row > slot ||
				slavePtr->row+slavePtr->numRows-1 < slot)) {
			continue;
		}
		if (slavePtr->window == NULL_HANDLE)
			continue;
		RETsv( newSVsv((( PAnyObject) slavePtr->window)-> mate));
	}

	return true;
}

/*
 *--------------------------------------------------------------
 *
 * AdjustOffsets --
 *
 *        This procedure adjusts the size of the layout to fit in the
 *        space provided.  If it needs more space, the extra is added
 *        according to the weights.  If it needs less, the space is removed
 *        according to the weights, but at no time does the size drop below
 *        the minsize specified for that slot.
 *
 * Results:
 *        The initial offset of the layout,
 *        if all the weights are zero, else 0.
 *
 * Side effects:
 *        The slot offsets are modified to shrink the layout.
 *
 *--------------------------------------------------------------
 */

static int
AdjustOffsets(
	int size,                      /* The total layout size (in pixels).        */
	int slots,                     /* Number of slots.                          */
	register SlotInfo *slotPtr     /* Pointer to slot array.                    */
) {
	register int slot;             /* Current slot.                             */
	int diff;                      /* Extra pixels needed to add to the layout. */
	int totalWeight = 0;           /* Sum of the weights for all the slots.     */
	int weight = 0;                /* Sum of the weights so far.                */
	int minSize = 0;               /* Minimum possible layout size.             */
	int newDiff;                   /* The most pixels that can be added on * the current pass. */

	diff = size - slotPtr[slots-1].offset;

	/*
	 * The layout is already the correct size; all done.
	 */

	if (diff == 0) {
		return(0);
	}

	/*
	 * If all the weights are zero, center the layout in its parent if
	 * there is extra space, else clip on the bottom/right.
	 */

	for (slot=0; slot < slots; slot++) {
		totalWeight += slotPtr[slot].weight;
	}

	if (totalWeight == 0 ) {
		return(diff > 0 ? diff/2 : 0);
	}

	/*
	 * Add extra space according to the slot weights.  This is done
	 * cumulatively to prevent round-off error accumulation.
	 */

	if (diff > 0) {
		for (weight=slot=0; slot < slots; slot++) {
			weight += slotPtr[slot].weight;
			slotPtr[slot].offset += diff * weight / totalWeight;
		}
		return(0);
	}

	/*
	 * The layout must shrink below its requested size.  Compute the
	 * minimum possible size by looking at the slot minSizes.
	 */

	for (slot=0; slot < slots; slot++) {
		if (slotPtr[slot].weight > 0) {
			minSize += slotPtr[slot].minSize;
		} else if (slot > 0) {
			minSize += slotPtr[slot].offset - slotPtr[slot-1].offset;
		} else {
			minSize += slotPtr[slot].offset;
		}
	}

	/*
	 * If the requested size is less than the minimum required size,
	 * set the slot sizes to their minimum values, then clip on the
	 * bottom/right.
	 */

	if (size <= minSize) {
		int offset = 0;
		for (slot=0; slot < slots; slot++) {
			if (slotPtr[slot].weight > 0) {
				offset += slotPtr[slot].minSize;
			} else if (slot > 0) {
				offset += slotPtr[slot].offset - slotPtr[slot-1].offset;
			} else {
				offset += slotPtr[slot].offset;
			}
			slotPtr[slot].offset = offset;
		}
		return(0);
	}

	/*
	 * Remove space from slots according to their weights.  The weights
	 * get renormalized anytime a slot shrinks to its minimum size.
	 */

	while (diff < 0) {

		/*
		 * Find the total weight for the shrinkable slots.
		 */

		for (totalWeight=slot=0; slot < slots; slot++) {
			int current = (slot == 0) ? slotPtr[slot].offset :
					slotPtr[slot].offset - slotPtr[slot-1].offset;
			if (current > slotPtr[slot].minSize) {
				totalWeight += slotPtr[slot].weight;
				slotPtr[slot].temp = slotPtr[slot].weight;
			} else {
				slotPtr[slot].temp = 0;
			}
		}
		if (totalWeight == 0) {
			break;
		}

		/*
		 * Find the maximum amount of space we can distribute this pass.
		 */

		newDiff = diff;
		for (slot = 0; slot < slots; slot++) {
			int current;  /* current size of this slot */
			int maxDiff;  /* max diff that would cause this slot to equal its minsize */
			if (slotPtr[slot].temp == 0) {
				continue;
			}
			current = (slot == 0) ? slotPtr[slot].offset :
					slotPtr[slot].offset - slotPtr[slot-1].offset;
			maxDiff = totalWeight * (slotPtr[slot].minSize - current)
					/ slotPtr[slot].temp;
			if (maxDiff > newDiff) {
				newDiff = maxDiff;
			}
		}

		/*
		 * Now distribute the space.
		 */

		for (weight=slot=0; slot < slots; slot++) {
			weight += slotPtr[slot].temp;
			slotPtr[slot].offset += newDiff * weight / totalWeight;
		}
		diff -= newDiff;
	}
	return(0);
}

/*
 *--------------------------------------------------------------
 *
 * AdjustForSticky --
 *
 *        This procedure adjusts the size of a slave in its cavity based
 *        on its "sticky" flags.
 *
 * Results:
 *        The input x, y, width, and height are changed to represent the
 *        desired coordinates of the slave.
 *
 * Side effects:
 *        None.
 *
 *--------------------------------------------------------------
 */

static void
AdjustForSticky(slavePtr, xPtr, yPtr, widthPtr, heightPtr)
	Gridder *slavePtr;  /* Slave window to arrange in its cavity. */
	int *xPtr;          /* Pixel location of the left edge of the cavity. */
	int *yPtr;          /* Pixel location of the top edge of the cavity. */
	int *widthPtr;      /* Width of the cavity (in pixels). */
	int *heightPtr;     /* Height of the cavity (in pixels). */
{
	int diffx=0;        /* Cavity width - slave width. */
	int diffy=0;        /* Cavity hight - slave height. */
	int sticky = slavePtr->sticky;

	Point sz;

	*xPtr += slavePtr->padLeft;
	*widthPtr -= slavePtr->padX;
	*yPtr += slavePtr->padBottom;
	*heightPtr -= slavePtr->padY;

	sz = PWidget(slavePtr->window)->geomSize;
	if (*widthPtr > sz.x + slavePtr->iPadX) {
		diffx = *widthPtr - (sz.x + slavePtr->iPadX);
		*widthPtr = sz.x + slavePtr->iPadX;
	}

	if (*heightPtr > (sz.y + slavePtr->iPadY)) {
		diffy = *heightPtr - (sz.y + slavePtr->iPadY);
		*heightPtr = sz.y + slavePtr->iPadY;
	}

	if (sticky&STICK_EAST && sticky&STICK_WEST) {
		*widthPtr += diffx;
	}
	if (sticky&STICK_NORTH && sticky&STICK_SOUTH) {
		*heightPtr += diffy;
	}
	if (!(sticky&STICK_WEST)) {
		*xPtr += (sticky&STICK_EAST) ? diffx : diffx/2;
	}
	if (!(sticky&STICK_NORTH)) {
		*yPtr += (sticky&STICK_SOUTH) ? diffy : diffy/2;
	}
}

/*
 *--------------------------------------------------------------
 *
 * ArrangeGrid --
 *
 *        This procedure is invoked to re-layout a set of windows managed by
 *        the grid. Contrary to the Tk version, it is NOT invoked 
 *        at idle time
 *
 * Side effects:
 *        The slaves of masterPtr may get resized or moved.
 *
 *--------------------------------------------------------------
 */

static void
ArrangeGrid(Gridder *masterPtr)
{
	register Gridder *slavePtr;
	GridMaster *slotPtr = masterPtr->masterDataPtr;
	int abort;
	int width, height;                /* requested size of layout, in pixels */
	int realWidth, realHeight;        /* actual size layout should take-up */
	Point sz;

	/*
	 * If the parent has no slaves anymore, then don't do anything
	 * at all:  just leave the parent's size as-is.  Otherwise there is
	 * no way to "relinquish" control over the parent so another geometry
	 * manager can take over.
	 */

	if (masterPtr->slavePtr == NULL) {
		return;
	}

	if (masterPtr->masterDataPtr == NULL) {
		return;
	}

	/*
	 * Abort any nested call to ArrangeGrid for this window, since
	 * we'll do everything necessary here, and set up so this call
	 * can be aborted if necessary.
	 */

	if (masterPtr->abortPtr != NULL) {
		*masterPtr->abortPtr = 1;
	}
	masterPtr->abortPtr = &abort;
	abort = 0;

	/*
	 * Call the constraint engine to fill in the row and column offsets.
	 */

	SetGridSize(masterPtr);
	width =  ResolveConstraints(masterPtr, COLUMN, 0);
	height = ResolveConstraints(masterPtr, ROW, 0);

	if (width < PWidget(masterPtr->window)->sizeMin.x) {
		width = PWidget(masterPtr->window)->sizeMin.x;
	}
	if (height < PWidget(masterPtr->window)->sizeMin.y) {
		height = PWidget(masterPtr->window)->sizeMin.y;
	}

	sz = PWidget(masterPtr->window)->geomSize;
	if (
		(width != sz.x || height != sz.y ) &&
		!(masterPtr->flags & DONT_PROPAGATE)
	) {
		Point p = {width,height};
		CWidget(masterPtr->window)->set_geomSize(masterPtr->window,p);
		masterPtr->abortPtr = NULL;
		return;
	}

	/*
	 * If the currently requested layout size doesn't match the parent's
	 * window size, then adjust the slot offsets according to the
	 * weights.  If all of the weights are zero, center the layout in
	 * its parent.  I haven't decided what to do if the parent is smaller
	 * than the requested size.
	 */

	sz = CWidget(masterPtr->window)->get_size(masterPtr->window);
	realWidth  = sz.x;
	realHeight = sz.y;
	slotPtr->startX = AdjustOffsets(realWidth,
			MAX(slotPtr->columnEnd,slotPtr->columnMax), slotPtr->columnPtr);
	slotPtr->startY = AdjustOffsets(realHeight,
			MAX(slotPtr->rowEnd,slotPtr->rowMax), slotPtr->rowPtr);

	/*
	 * Now adjust the actual size of the slave to its cavity by
	 * computing the cavity size, and adjusting the widget according
	 * to its stickyness.
	 */

	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL && !abort;
			slavePtr = slavePtr->nextPtr) {
		int x, y;                        /* top left coordinate */
		int width, height;                /* slot or slave size */
		int col = slavePtr->column;
		int row = slavePtr->row;

		x = (col>0) ? slotPtr->columnPtr[col-1].offset : 0;
		y = (row>0) ? slotPtr->rowPtr[row-1].offset : 0;

		width = slotPtr->columnPtr[slavePtr->numCols+col-1].offset - x;
		height = slotPtr->rowPtr[slavePtr->numRows+row-1].offset - y;

		x += slotPtr->startX;
		y += slotPtr->startY;

		AdjustForSticky(slavePtr, &x, &y, &width, &height);

		if (masterPtr->window == PWidget(slavePtr-> window)-> owner) {
			Rect r;
			r. left   = x;
			r. bottom = y;
			r. right  = x + width;
			r. top    = y + height;
			CWidget(slavePtr-> window)-> set_rect(slavePtr->window, r);
		}
	}

	masterPtr->abortPtr = NULL;
}

/*
 *--------------------------------------------------------------
 *
 * ResolveConstraints --
 *
 *        Resolve all of the column and row boundaries.  Most of
 *        the calculations are identical for rows and columns, so this procedure
 *        is called twice, once for rows, and again for columns.
 *
 * Results:
 *        The offset (in pixels) from the left/top edge of this layout is
 *        returned.
 *
 * Side effects:
 *        The slot offsets are copied into the SlotInfo structure for the
 *        geometry master.
 *
 *--------------------------------------------------------------
 */

static int
ResolveConstraints(
	Gridder *masterPtr,           /* The geometry master for this grid. */
	int slotType,                 /* Either ROW or COLUMN. */
	int maxOffset                 /* The actual maximum size of this layout in pixels,  or 0 (not currently used). */
) {
	register SlotInfo *slotPtr;   /* Pointer to row/col constraints. */
	register Gridder *slavePtr;   /* List of slave windows in this grid. */
	int constraintCount;          /* Count of rows or columns that have constraints. */
	int slotCount;                /* Last occupied row or column. */
	int gridCount;                /* The larger of slotCount and constraintCount  */
	GridLayout *layoutPtr;        /* Temporary layout structure. */
	int requiredSize;             /* The natural size of the grid (pixels).
	                               * This is the minimum size needed to
	                               * accomodate all of the slaves at their
	                               * requested sizes. */
	int offset;                   /* The pixel offset of the right edge of the
	                               * current slot from the beginning of the
	                               * layout. */
	int slot;                     /* The current slot. */
	int start;                    /* The first slot of a contiguous set whose
	                               * constraints are not yet fully resolved. */
	int end;                      /* The Last slot of a contiguous set whose constraints are not yet fully resolved. */
	UniformGroup uniformPre[UNIFORM_PREALLOC]; /* Pre-allocated space for uniform groups. */
	UniformGroup *uniformGroupPtr;
	                              /* Uniform groups data. */
	int uniformGroups;            /* Number of currently used uniform groups. */
	int uniformGroupsAlloced;     /* Size of allocated space for uniform groups.  */
	int weight, minSize;

	/*
	 * For typical sized tables, we'll use stack space for the layout data
	 * to avoid the overhead of a malloc and free for every layout.
	 */

	GridLayout layoutData[TYPICAL_SIZE + 2];

	if (slotType == COLUMN) {
		constraintCount = masterPtr->masterDataPtr->columnMax;
		slotCount = masterPtr->masterDataPtr->columnEnd;
		slotPtr  = masterPtr->masterDataPtr->columnPtr;
	} else {
		constraintCount = masterPtr->masterDataPtr->rowMax;
		slotCount = masterPtr->masterDataPtr->rowEnd;
		slotPtr  = masterPtr->masterDataPtr->rowPtr;
	}

	/*
	 * Make sure there is enough memory for the layout.
	 */

	gridCount = MAX(constraintCount,slotCount);
	if (gridCount >= TYPICAL_SIZE) {
		if ( !( layoutPtr = (GridLayout *) malloc(sizeof(GridLayout) * (1+gridCount)))) {
			warn("Not enough memory");
			return 1;
		}
	} else {
		layoutPtr = layoutData;
	}

	/*
	 * Allocate an extra layout slot to represent the left/top edge of
	 * the 0th slot to make it easier to calculate slot widths from
	 * offsets without special case code.
	 * Initialize the "dummy" slot to the left/top of the table.
	 * This slot avoids special casing the first slot.
	 */

	layoutPtr->minOffset = 0;
	layoutPtr->maxOffset = 0;
	layoutPtr++;

	/*
	 * Step 1.
	 * Copy the slot constraints into the layout structure,
	 * and initialize the rest of the fields.
	 */

	for (slot=0; slot < constraintCount; slot++) {
		layoutPtr[slot].minSize = slotPtr[slot].minSize;
		layoutPtr[slot].weight  = slotPtr[slot].weight;
		layoutPtr[slot].uniform = slotPtr[slot].uniform;
		layoutPtr[slot].pad =  slotPtr[slot].pad;
		layoutPtr[slot].binNextPtr = NULL;
	}
	for(;slot<gridCount;slot++) {
		layoutPtr[slot].minSize = 0;
		layoutPtr[slot].weight = 0;
		layoutPtr[slot].uniform = 0;
		layoutPtr[slot].pad = 0;
		layoutPtr[slot].binNextPtr = NULL;
	}

	/*
	 * Step 2.
	 * Slaves with a span of 1 are used to determine the minimum size of
	 * each slot.  Slaves whose span is two or more slots don't
	 * contribute to the minimum size of each slot directly, but can cause
	 * slots to grow if their size exceeds the the sizes of the slots they
	 * span.
	 *
	 * Bin all slaves whose spans are > 1 by their right edges.  This
	 * allows the computation on minimum and maximum possible layout
	 * sizes at each slot boundary, without the need to re-sort the slaves.
	 */

	switch (slotType) {
	case COLUMN:
		for (slavePtr = masterPtr->slavePtr; slavePtr != NULL; slavePtr = slavePtr->nextPtr) {
			int rightEdge = slavePtr->column + slavePtr->numCols - 1;
			slavePtr->size = PWidget(slavePtr->window)->geomSize.x + slavePtr->padX + slavePtr->iPadX;
			if (slavePtr->numCols > 1) {
				slavePtr->binNextPtr = layoutPtr[rightEdge].binNextPtr;
				layoutPtr[rightEdge].binNextPtr = slavePtr;
			} else {
				int size = slavePtr->size + layoutPtr[rightEdge].pad;
				if (size > layoutPtr[rightEdge].minSize) {
					layoutPtr[rightEdge].minSize = size;
				}
			}
		}
		break;
	case ROW:
		for (slavePtr = masterPtr->slavePtr; slavePtr != NULL; slavePtr = slavePtr->nextPtr) {
			int rightEdge = slavePtr->row + slavePtr->numRows - 1;
			slavePtr->size = PWidget(slavePtr->window)->geomSize.y + slavePtr->padY + slavePtr->iPadY;
			if (slavePtr->numRows > 1) {
				slavePtr->binNextPtr = layoutPtr[rightEdge].binNextPtr;
				layoutPtr[rightEdge].binNextPtr = slavePtr;
			} else {
				int size = slavePtr->size + layoutPtr[rightEdge].pad;
				if (size > layoutPtr[rightEdge].minSize) {
					layoutPtr[rightEdge].minSize = size;
				}
			}
		}
		break;
	}

	/*
	 * Step 2b.
	 * Consider demands on uniform sizes.
	 */

	uniformGroupPtr = uniformPre;
	uniformGroupsAlloced = UNIFORM_PREALLOC;
	uniformGroups = 0;

	for (slot = 0; slot < gridCount; slot++) {
		if (layoutPtr[slot].uniform != 0) {
			for (start = 0; start < uniformGroups; start++) {
				if (uniformGroupPtr[start].group == layoutPtr[slot].uniform) {
					break;
				}
			}
			if (start >= uniformGroups) {
				/*
				 * Have not seen that group before, set up data for it.
				 */

				if (uniformGroups >= uniformGroupsAlloced) {
					/*
					 * We need to allocate more space.
					 */

					size_t newSize = (uniformGroupsAlloced + UNIFORM_PREALLOC)
							* sizeof(UniformGroup);
					UniformGroup *n = (UniformGroup *) realloc( uniformGroupPtr, newSize);
					if (!n) {
						warn("Not enough memory");
						return 1;
					}
					uniformGroupPtr = n;
					uniformGroupsAlloced += UNIFORM_PREALLOC;
				}
				uniformGroups++;
				uniformGroupPtr[start].group = layoutPtr[slot].uniform;
				uniformGroupPtr[start].minSize = 0;
			}
			weight = layoutPtr[slot].weight;
			weight = weight > 0 ? weight : 1;
			minSize = (layoutPtr[slot].minSize + weight - 1) / weight;
			if (minSize > uniformGroupPtr[start].minSize) {
				uniformGroupPtr[start].minSize = minSize;
			}
		}
	}

	/*
	 * Data has been gathered about uniform groups. Now relayout accordingly.
	 */

	if (uniformGroups > 0) {
		for (slot = 0; slot < gridCount; slot++) {
			if (layoutPtr[slot].uniform != 0) {
				for (start = 0; start < uniformGroups; start++) {
					if (uniformGroupPtr[start].group ==
							layoutPtr[slot].uniform) {
						weight = layoutPtr[slot].weight;
						weight = weight > 0 ? weight : 1;
						layoutPtr[slot].minSize =
							uniformGroupPtr[start].minSize * weight;
						break;
					}
				}
			}
		}
	}

	if (uniformGroupPtr != uniformPre) {
		free(uniformGroupPtr);
	}

	/*
	 * Step 3.
	 * Determine the minimum slot offsets going from left to right
	 * that would fit all of the slaves.  This determines the minimum
	 */

	for (offset=slot=0; slot < gridCount; slot++) {
		layoutPtr[slot].minOffset = layoutPtr[slot].minSize + offset;
		for (slavePtr = layoutPtr[slot].binNextPtr; slavePtr != NULL;
					slavePtr = slavePtr->binNextPtr) {
			int span = (slotType == COLUMN) ? slavePtr->numCols : slavePtr->numRows;
			int required = slavePtr->size + layoutPtr[slot - span].minOffset;
			if (required > layoutPtr[slot].minOffset) {
				layoutPtr[slot].minOffset = required;
			}
		}
		offset = layoutPtr[slot].minOffset;
	}

	/*
	 * At this point, we know the minimum required size of the entire layout.
	 * It might be prudent to stop here if our "master" will resize itself
	 * to this size.
	 */

	requiredSize = offset;
	if (maxOffset > offset) {
		offset=maxOffset;
	}

	/*
	 * Step 4.
	 * Determine the minimum slot offsets going from right to left,
	 * bounding the pixel range of each slot boundary.
	 * Pre-fill all of the right offsets with the actual size of the table;
	 * they will be reduced as required.
	 */

	for (slot=0; slot < gridCount; slot++) {
		layoutPtr[slot].maxOffset = offset;
	}
	for (slot=gridCount-1; slot > 0;) {
		for (slavePtr = layoutPtr[slot].binNextPtr; slavePtr != NULL;
					slavePtr = slavePtr->binNextPtr) {
			int span = (slotType == COLUMN) ? slavePtr->numCols : slavePtr->numRows;
			int require = offset - slavePtr->size;
			int startSlot  = slot - span;
			if (startSlot >=0 && require < layoutPtr[startSlot].maxOffset) {
				layoutPtr[startSlot].maxOffset = require;
			}
		}
		offset -= layoutPtr[slot].minSize;
		slot--;
		if (layoutPtr[slot].maxOffset < offset) {
			offset = layoutPtr[slot].maxOffset;
		} else {
			layoutPtr[slot].maxOffset = offset;
		}
	}

	/*
	 * Step 5.
	 * At this point, each slot boundary has a range of values that
	 * will satisfy the overall layout size.
	 * Make repeated passes over the layout structure looking for
	 * spans of slot boundaries where the minOffsets are less than
	 * the maxOffsets, and adjust the offsets according to the slot
	 * weights.  At each pass, at least one slot boundary will have
	 * its range of possible values fixed at a single value.
	 */

	for (start=0; start < gridCount;) {
		int totalWeight = 0;        /* Sum of the weights for all of the
								 * slots in this span. */
		int need = 0;                /* The minimum space needed to layout
								 * this span. */
		int have;                /* The actual amount of space that will
								 * be taken up by this span. */
		int weight;                /* Cumulative weights of the columns in
								 * this span. */
		int noWeights = 0;        /* True if the span has no weights. */

		/*
		 * Find a span by identifying ranges of slots whose edges are
		 * already constrained at fixed offsets, but whose internal
		 * slot boundaries have a range of possible positions.
		 */

		if (layoutPtr[start].minOffset == layoutPtr[start].maxOffset) {
			start++;
			continue;
		}

		for (end=start+1; end<gridCount; end++) {
			if (layoutPtr[end].minOffset == layoutPtr[end].maxOffset) {
				break;
			}
		}

		/*
		 * We found a span.  Compute the total weight, minumum space required,
		 * for this span, and the actual amount of space the span should
		 * use.
		 */

		for (slot=start; slot<=end; slot++) {
			totalWeight += layoutPtr[slot].weight;
			need += layoutPtr[slot].minSize;
		}
		have = layoutPtr[end].maxOffset - layoutPtr[start-1].minOffset;

		/*
		 * If all the weights in the span are zero, then distribute the
		 * extra space evenly.
		 */

		if (totalWeight == 0) {
			noWeights++;
			totalWeight = end - start + 1;
		}

		/*
		 * It might not be possible to give the span all of the space
		 * available on this pass without violating the size constraints
		 * of one or more of the internal slot boundaries.
		 * Determine the maximum amount of space that when added to the
		 * entire span, would cause a slot boundary to have its possible
		 * range reduced to one value, and reduce the amount of extra
		 * space allocated on this pass accordingly.
		 *
		 * The calculation is done cumulatively to avoid accumulating
		 * roundoff errors.
		 */

		for (weight=0,slot=start; slot<end; slot++) {
			int diff = layoutPtr[slot].maxOffset - layoutPtr[slot].minOffset;
			weight += noWeights ? 1 : layoutPtr[slot].weight;
			if ((noWeights || layoutPtr[slot].weight>0) &&
					(diff*totalWeight/weight) < (have-need)) {
				have = diff * totalWeight / weight + need;
			}
		}

		/*
		 * Now distribute the extra space among the slots by
		 * adjusting the minSizes and minOffsets.
		 */

		for (weight=0,slot=start; slot<end; slot++) {
			weight += noWeights ? 1 : layoutPtr[slot].weight;
			layoutPtr[slot].minOffset +=
				(int)((double) (have-need) * weight/totalWeight + 0.5);
			layoutPtr[slot].minSize = layoutPtr[slot].minOffset
					- layoutPtr[slot-1].minOffset;
		}
		layoutPtr[slot].minSize = layoutPtr[slot].minOffset
				- layoutPtr[slot-1].minOffset;

		/*
		 * Having pushed the top/left boundaries of the slots to
		 * take up extra space, the bottom/right space is recalculated
		 * to propagate the new space allocation.
		 */

		for (slot=end; slot > start; slot--) {
			layoutPtr[slot-1].maxOffset =
				layoutPtr[slot].maxOffset-layoutPtr[slot].minSize;
		}
	}


	/*
	 * Step 6.
	 * All of the space has been apportioned; copy the
	 * layout information back into the master.
	 */

	for (slot=0; slot < gridCount; slot++) {
		slotPtr[slot].offset = layoutPtr[slot].minOffset;
	}

	--layoutPtr;
	if (layoutPtr != layoutData) {
		free(layoutPtr);
	}
	return requiredSize;
}

/*
 *--------------------------------------------------------------
 *
 * GetGrid --
 *
 *        This internal procedure is used to locate a Grid
 *        structure for a given window, creating one if one
 *        doesn't exist already.
 *
 * Results:
 *        The return value is a pointer to the Grid structure
 *        corresponding to window.
 *
 * Side effects:
 *        A new grid structure may be created.
 *
 *--------------------------------------------------------------
 */

static Gridder *
GetGrid(
	Handle self  /* Token for window for which grid structure is desired. */
) {
	register Gridder *gridPtr;
	if ( var-> gridder )
		return (Gridder*) var->gridder;

	/*
	 * See if there's already grid for this window.  If not,
	 * then create a new one.
	 */

	if ( !( gridPtr = (Gridder *) malloc(sizeof(Gridder))))
		croak("Not enough memory");
	gridPtr->window        = self;
	gridPtr->masterPtr     = NULL;
	gridPtr->masterDataPtr = NULL;
	gridPtr->nextPtr       = NULL;
	gridPtr->slavePtr      = NULL;
	gridPtr->binNextPtr    = NULL;

	gridPtr->column = gridPtr->row = -1;
	gridPtr->numCols = 1;
	gridPtr->numRows = 1;

	gridPtr->padX     = gridPtr->padY   = 0;
	gridPtr->padLeft  = gridPtr->padBottom = 0;
	gridPtr->iPadX    = gridPtr->iPadY  = 0;
	gridPtr->abortPtr = NULL;
	gridPtr->flags    = 0;
	gridPtr->sticky   = 0;
	gridPtr->size     = 0;

	gridPtr->saved_in = NULL_HANDLE;

	var->gridder = gridPtr;

	return gridPtr;
}

/*
 *--------------------------------------------------------------
 *
 * SetGridSize --
 *
 *        This internal procedure sets the size of the grid occupied
 *        by slaves.
 *
 * Results:
 *        none
 *
 * Side effects:
 *        The width and height arguments are filled in the master data structure.
 *        Additional space is allocated for the constraints to accomodate
 *        the offsets.
 *
 *--------------------------------------------------------------
 */

static void
SetGridSize(
	Gridder *masterPtr                        /* The geometry master for this grid. */
) {
	register Gridder *slavePtr;               /* Current slave window. */
	int maxX = 0, maxY = 0;

	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
				slavePtr = slavePtr->nextPtr) {
		maxX = MAX(maxX,slavePtr->numCols + slavePtr->column);
		maxY = MAX(maxY,slavePtr->numRows + slavePtr->row);
	}
	masterPtr->masterDataPtr->columnEnd = maxX;
	masterPtr->masterDataPtr->rowEnd = maxY;
	CheckSlotData(masterPtr, maxX, COLUMN, CHECK_SPACE);
	CheckSlotData(masterPtr, maxY, ROW, CHECK_SPACE);
}

/*
 *--------------------------------------------------------------
 *
 * CheckSlotData --
 *
 *        This internal procedure is used to manage the storage for
 *        row and column (slot) constraints.
 *
 * Results:
 *        TRUE if the index is OK, False otherwise.
 *
 * Side effects:
 *        A new master grid structure may be created.  If so, then
 *        it is initialized.  In addition, additional storage for
 *        a row or column constraints may be allocated, and the constraint
 *        maximums are adjusted.
 *
 *--------------------------------------------------------------
 */

static Bool
CheckSlotData(
	Gridder *masterPtr,   /* the geometry master for this grid */
	int slot,             /* which slot to look at */
	int slotType,         /* ROW or COLUMN */
	int checkOnly         /* don't allocate new space if true */
) {
	int numSlot;          /* number of slots already allocated (Space) */
	int end;              /* last used constraint */

	/*
	 * If slot is out of bounds, return immediately.
	 */

	if (slot < 0 || slot >= MAX_ELEMENT) {
		warn("grid row or column out of range");
		return false;
	}

	if ((checkOnly == CHECK_ONLY) && (masterPtr->masterDataPtr == NULL)) {
		return false;
	}

	/*
	 * If we need to allocate more space, allocate a little extra to avoid
	 * repeated re-alloc's for large tables.  We need enough space to
	 * hold all of the offsets as well.
	 */

	InitMasterData(masterPtr);
	end = (slotType == ROW) ? masterPtr->masterDataPtr->rowMax :
			masterPtr->masterDataPtr->columnMax;
	if (checkOnly == CHECK_ONLY)
		return end >= slot;

	numSlot = (slotType == ROW) ?
		masterPtr->masterDataPtr->rowSpace :
		masterPtr->masterDataPtr->columnSpace;

	if (slot >= numSlot) {
		int      newNumSlot = slot + PREALLOC ;
		size_t   oldSize = numSlot    * sizeof(SlotInfo) ;
		size_t   newSize = newNumSlot * sizeof(SlotInfo) ;
		SlotInfo *old    = (slotType == ROW) ?
			masterPtr->masterDataPtr->rowPtr :
			masterPtr->masterDataPtr->columnPtr;
		SlotInfo *newx   = (SlotInfo *) realloc(old, newSize);
		if ( !newx ) {
			warn("not enough memory");
			return false;
		}
		bzero(newx + numSlot, newSize - oldSize );
		if (slotType == ROW) {
			masterPtr->masterDataPtr->rowPtr      = newx;
			masterPtr->masterDataPtr->rowSpace    = newNumSlot ;
		} else {
			masterPtr->masterDataPtr->columnPtr   = newx;
			masterPtr->masterDataPtr->columnSpace = newNumSlot ;
		}
	}

	if (slot >= end && checkOnly != CHECK_SPACE) {
		if (slotType == ROW) {
			masterPtr->masterDataPtr->rowMax = slot+1;
		} else {
			masterPtr->masterDataPtr->columnMax = slot+1;
		}
	}

	return true;
}

/*
 *--------------------------------------------------------------
 *
 * InitMasterData --
 *
 *        This internal procedure is used to allocate and initialize
 *        the data for a geometry master, if the data
 *        doesn't exist already.
 *
 * Results:
 *        none
 *
 * Side effects:
 *        A new master grid structure may be created.  If so, then
 *        it is initialized.
 *
 *--------------------------------------------------------------
 */

static void
InitMasterData( Gridder *masterPtr)
{
	size_t size;
	if (masterPtr->masterDataPtr == NULL) {
		GridMaster *gridPtr = masterPtr->masterDataPtr =
				(GridMaster *) malloc(sizeof(GridMaster));
		size = sizeof(SlotInfo) * TYPICAL_SIZE;

		gridPtr->columnEnd = 0;
		gridPtr->columnMax = 0;
		gridPtr->columnPtr = (SlotInfo *) malloc(size);
		gridPtr->columnSpace = TYPICAL_SIZE;
		gridPtr->rowEnd = 0;
		gridPtr->rowMax = 0;
		gridPtr->rowPtr = (SlotInfo *) malloc(size);
		gridPtr->rowSpace = TYPICAL_SIZE;
		gridPtr->startX = 0;
		gridPtr->startY = 0;

		bzero(gridPtr->columnPtr, size);
		bzero(gridPtr->rowPtr, size);
	}
}

/*
 *----------------------------------------------------------------------
 *
 * Unlink --
 *
 *        Remove a grid from its parent's list of slaves.
 *
 *----------------------------------------------------------------------
 */

static void
Unlink( Gridder *slavePtr)                /* Window to unlink. */
{
	register Gridder *masterPtr, *slavePtr2;

	masterPtr = slavePtr->masterPtr;
	if (masterPtr == NULL) {
		return;
	}

	if (masterPtr->slavePtr == slavePtr) {
		masterPtr->slavePtr = slavePtr->nextPtr;
	} else {
		for (slavePtr2 = masterPtr->slavePtr; ; slavePtr2 = slavePtr2->nextPtr) {
			if (slavePtr2 == NULL) {
				croak("Widget.gridUnlink: couldn't find previous window");
			}
			if (slavePtr2->nextPtr == slavePtr) {
				slavePtr2->nextPtr = slavePtr->nextPtr;
				break;
			}
		}
	}
	if (masterPtr->abortPtr != NULL) {
		*masterPtr->abortPtr = 1;
	}

	SetGridSize(slavePtr->masterPtr);
	slavePtr->masterPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyGrid --
 *
 *        This procedure is invoked
 *        to clean up the internal structure of a grid at a safe time
 *        (when no-one is using it anymore).   Cleaning up the grid involves
 *        freeing the main structure for all windows. and the master structure
 *        for geometry managers.
 *
 * Side effects:
 *        Everything associated with the grid is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyGrid(Handle self)
{
	Gridder *gridPtr = (Gridder*) var-> gridder, *gridPtr2, *nextPtr;
	if ( !gridPtr )
		return;

	if (gridPtr->masterPtr != NULL) {
		Unlink(gridPtr);
	}
	for (gridPtr2 = gridPtr->slavePtr; gridPtr2 != NULL; gridPtr2 = nextPtr) {
		gridPtr2->masterPtr = NULL;
		nextPtr = gridPtr2->nextPtr;
		gridPtr2->nextPtr = NULL;
	}

	if (gridPtr->masterDataPtr != NULL) {
		if (gridPtr->masterDataPtr->rowPtr != NULL) {
			free(gridPtr->masterDataPtr -> rowPtr);
		}
		if (gridPtr->masterDataPtr->columnPtr != NULL) {
			free(gridPtr->masterDataPtr -> columnPtr);
		}
		free(gridPtr->masterDataPtr);
	}

	free(gridPtr);
	var-> gridder = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureSlaves --
 *
 *        This implements the guts of the "grid configure" command.  Given
 *        a list of slaves and configuration options, it arranges for the
 *        grid to manage the slaves and sets the specified options.
 *        arguments consist of a window or an arrayref of windows followed by
 *        a profile hash
 *
 *	1.Contrary to the Tk syntax, window names such as 'xe' and 'xy' will be treated as true window names,
 *	not REL_SKIP commands. Only string 'x' will be.
 *
 *	2. s/columnspan/colspan/, hello html
 *
 * Side effects:
 *        Slave windows get taken over by the grid.
 *
 *----------------------------------------------------------------------
 */

#define METHOD "Widget::grid_configure_slaves"

static void
parse_pad_amount(SV *src, int *i1, int *i2)
{
	if (SvOK(src) && SvROK(src) && SvTYPE(SvRV(src)) == SVt_PVAV) {
		int *p = prima_read_array(src, METHOD, 'i', 1, 2, 2, NULL, NULL);
		*i1 = p[0];
		*i2 = p[1];
		free(p);
	} else {
		*i1 = *i2 = SvIV(src);
	}
}

static Gridder*
parse_info( Gridder *slavePtr, HV *profile)
{
	dPROFILE;
	Gridder *masterPtr = NULL;

	if ( pexist(column)) {
		int c = pget_i(column);
		if ( c < 0 ) croak("%s: column value must be a non-negative integer", METHOD);
		slavePtr->column = c;
	}

	if ( pexist(colspan)) {
		int c = pget_i(colspan);
		if ( c < 0 ) croak("%s: colspan value must be a non-negative integer", METHOD);
		slavePtr->numCols = c;
	}

	if ( pexist(in)) {
		Handle in, h;
		if ( ( h = in = pget_H(in)) != NULL_HANDLE ) {
			if (in == slavePtr->window)
				croak("%s: widget can't be managed in itself", METHOD);
			while ( h) {
				h = PWidget(h)-> owner;
				if (in == slavePtr->window)
					croak("%s: widget can't be managed by its child", METHOD);
			}
			masterPtr = GetGrid(in);
			InitMasterData(masterPtr);
		}
		slavePtr->saved_in = in;
	}

	if (pexist(ipadx)) slavePtr->iPadX = 2 * pget_i(ipadx);
	if (pexist(ipady)) slavePtr->iPadY = 2 * pget_i(ipady);

	if (pexist(padx)) parse_pad_amount( pget_sv(padx), &slavePtr->padLeft, &slavePtr->padX);
	if (pexist(pady)) parse_pad_amount( pget_sv(pady), &slavePtr->padBottom, &slavePtr->padY);

	if ( pexist(row)) {
		int c = pget_i(row);
		if ( c < 0 ) croak("%s: row value must be a non-negative integer", METHOD);
		slavePtr->row = c;
	}

	if ( pexist(rowspan)) {
		int c = pget_i(rowspan);
		if ( c < 0 ) croak("%s: rowspan value must be a non-negative integer", METHOD);
		slavePtr->numRows = c;
	}

	if (pexist(sticky)) {
		char *s = pget_c(sticky);
		int sticky = StringToSticky(s);
		if (sticky == -1)
			croak("%s: bad stickyness value \"%s\": must be a string containing n, e, s, and/or w",
				METHOD, s);
		slavePtr->sticky = sticky;
	}

	return masterPtr;
}

static void
ConfigureSlaves(Handle self, SV* window, HV *profile)
{
	SV **slaves;
	int n_slaves;

	Gridder *masterPtr;
	Gridder *slavePtr;
	Handle slave, parent;
	int i;
	int width;
	int defaultColumn = 0;        /* default column number */
	int defaultColumnSpan = 1;    /* default number of columns */
	Handle lastWindow;            /* use this window to base current row/col on */
	int numSkip;                  /* number of 'x' found */
	STRLEN len;
	char *firstStr, *prevStr;

	if (!(
		SvOK(window) && (
			!SvROK(window) || (
				(SvTYPE(SvRV(window)) == SVt_PVAV) ||
				(SvTYPE(SvRV(window)) == SVt_PVHV)
			)
		)
	))
		croak("%s: window is neither an object nor an array of objects", METHOD);

	if (SvROK(window) && (SvTYPE(SvRV(window)) == SVt_PVAV)) {
		if ( !( slaves = (SV**) prima_read_array(window, METHOD, 'H', 1, 0, 0, &n_slaves, NULL)))
			croak("%s: cannot read array of windows, or the array is empty", METHOD);
	} else {
		slaves   = malloc(sizeof(SV*));
		n_slaves = 1;
		*slaves  = window;
	}

	firstStr = "";
	for (i = 0; i < n_slaves; i++) {
		if ( SvROK( slaves[i]))
			continue;
		prevStr = firstStr;
		firstStr = SvPV( slaves[i], len);
		if (*firstStr == REL_HORIZ) {
			if (
				i==0 ||
				(prevStr[0] == REL_SKIP && prevStr[1] == 0) ||
				(strcmp(prevStr, "^") == 0)
			)
			croak("%s: Must specify window before shortcut '-'.", METHOD);
			continue;
		}
	}

	/*
	 * Iterate over all of the slave windows and short-cuts, parsing
	 * options for each slave.  It's a bit wasteful to re-parse the
	 * options for each slave, but things get too messy if we try to
	 * parse the arguments just once at the beginning.  For example,
	 * if a slave already is managed we want to just change a few
	 * existing values without resetting everything.  If there are
	 * multiple windows, the -in option only gets processed for the
	 * first window.
	 */

	masterPtr = NULL;
	for (i = 0; i < n_slaves; i++) {
		if ( !SvROK(slaves[i])) {
			firstStr = SvPV( slaves[i], len);

			/*
			 * '^' and 'x' cause us to skip a column.  '-' is processed
			 * as part of its preceeding slave.
			 */
			if (len == 1 && (*firstStr == REL_VERT || *firstStr == REL_SKIP)) {
				defaultColumn++;
				continue;
			}
			if (*firstStr == REL_HORIZ) {
				continue;
			}

			for (defaultColumnSpan = 1; i + defaultColumnSpan < n_slaves; defaultColumnSpan++) {
				STRLEN l = 1;
				char *string = SvROK( slaves[i+defaultColumnSpan] ) ? "" : SvPV(slaves[i + defaultColumnSpan], l);
				if ( l != 1 || *string != REL_HORIZ) {
					break;
				}
			}
		}

		if (( slave = get_slave(self, slaves[i])) == NULL_HANDLE )
			croak("%s: cannot locate child widget %s", METHOD, firstStr);
		Widget_check_in(slave, self, true);
		slavePtr = GetGrid(slave);

		/*
		* The following statement is taken from tkPack.c:
		*
		* "If the slave isn't currently managed, reset all of its
		* configuration information to default values (there could
		* be old values_HANDLE left from a previous packer)."
		*
		* I [D.S.] disagree with this statement.  If a slave is disabled (using
		* "forget") and then re-enabled, I submit that 90% of the time the
		* programmer will want it to retain its old configuration information.
		* If the programmer doesn't want this behavior, then the
		* defaults can be reestablished by hand, without having to worry
		* about keeping track of the old state.
		*/
		masterPtr = parse_info( slavePtr, profile );

		/*
		* Make sure we have a geometry master.  We look at:
		*  1)   the -in flag
		*  2)   the geometry master of the first slave (if specified)
		*  3)   the parent of the first slave.
		*/

		if (masterPtr == NULL) {
			masterPtr = slavePtr->masterPtr;
		}
		parent = PWidget(slave)->owner;
		if (masterPtr == NULL) {
			masterPtr = GetGrid(parent);
			InitMasterData(masterPtr);
		}

		if (slavePtr->masterPtr != NULL && slavePtr->masterPtr != masterPtr) {
			Unlink(slavePtr);
			slavePtr->masterPtr = NULL;
		}

		if (slavePtr->masterPtr == NULL) {
			Gridder *tempPtr = masterPtr->slavePtr;
			slavePtr->masterPtr = masterPtr;
			masterPtr->slavePtr = slavePtr;
			slavePtr->nextPtr = tempPtr;
		}

		/*
		 * Try to make sure our master isn't managed by us.
		 */

		if (masterPtr->masterPtr == slavePtr) {
			Unlink(slavePtr);
			croak("%s: can't put that slave here", METHOD);
		 }

		/*
		 * Assign default position information.
		 */

		if (slavePtr->column == -1) {
			slavePtr->column = defaultColumn;
		}
		slavePtr->numCols += defaultColumnSpan - 1;
		if (slavePtr->row == -1) {
			if (masterPtr->masterDataPtr == NULL) {
				slavePtr->row = 0;
			} else {
				slavePtr->row = masterPtr->masterDataPtr->rowEnd;
			}
		}
		defaultColumn += slavePtr->numCols;
		defaultColumnSpan = 1;

		/*
		 * Arrange for the parent to be re-arranged at the first
		 * idle moment.
		 */

		if (masterPtr->abortPtr != NULL) {
			*masterPtr->abortPtr = 1;
		}

		if ( PWidget(slave)->geometry == gtGrid) {
			SetGridSize(masterPtr);
			ArrangeGrid(masterPtr);
		} else {
			CWidget(slave)->set_geometry(slave, gtGrid);
		}
	}

	/* Now look for all the "^"'s. */

	lastWindow = -1;
	numSkip = 0;
	for (i = 0; i < n_slaves; i++) {
		struct Gridder *otherPtr;
		int match;                        /* found a match for the ^ */
		int lastRow, lastColumn;        /* implied end of table */

		len = 0;
		firstStr = SvROK(slaves[i]) ? "" : SvPV( slaves[i], len);

		if ( len == 1 && *firstStr == REL_SKIP ) {
			numSkip++;
		} else if ( len == 1 && *firstStr == REL_VERT) {
			/* do nothing */
		} else {
			numSkip = 0;
			lastWindow = get_slave(self, slaves[i]);
			continue;
		}

		if (masterPtr == NULL) {
			croak("%s: can't use '^', can't find master", METHOD);
		}

		/* Count the number of consecutive ^'s starting from this position */
		for (width = 1; width + i < n_slaves; width++) {
			len = 0;
			firstStr = SvROK(slaves[i+width]) ? "" : SvPV( slaves[i+width], len);
			if (len != 1 || *firstStr != REL_VERT )
				break;
		}

		/*
		 * Find the implied grid location of the ^
		 */

		if (lastWindow < 0) {
			if (masterPtr->masterDataPtr != NULL) {
				SetGridSize(masterPtr);
				lastRow = masterPtr->masterDataPtr->rowEnd - 2;
			} else {
				lastRow = 0;
			}
			lastColumn = 0;
		} else {
			otherPtr = GetGrid(lastWindow);
			lastRow = otherPtr->row + otherPtr->numRows - 2;
			lastColumn = otherPtr->column + otherPtr->numCols;
		}

		lastColumn += numSkip;

		for (match=0, slavePtr = masterPtr->slavePtr; slavePtr != NULL;
			slavePtr = slavePtr->nextPtr) {

			if (slavePtr->column == lastColumn
				&& slavePtr->row + slavePtr->numRows - 1 == lastRow) {
				if (slavePtr->numCols <= width) {
					slavePtr->numRows++;
					match++;
					i += slavePtr->numCols - 1;
					lastWindow = slavePtr->window;
					numSkip = 0;
					break;
				}
			}
		}
		if (!match) {
			croak("%s: can't find slave to extend with \"^\".", METHOD);
		}
	}

	if (masterPtr == NULL) {
		croak("%s: can't determine master window", METHOD);
	}
	SetGridSize(masterPtr);
}

#undef METHOD

/*
 *----------------------------------------------------------------------
 *
 * StickyToString
 *
 *        Converts the internal boolean combination of "sticky" bits onto
 *        a TCL list element containing zero or mor of n, s, e, or w.
 *
 * Results:
 *        A string is placed into the "result" pointer.
 *
 * Side effects:
 *        none.
 *
 *----------------------------------------------------------------------
 */

static void
StickyToString(
	int flags,           /* the sticky flags */
	char *result        /* where to put the result */
) {
	int count = 0;
	if (flags&STICK_NORTH) {
		result[count++] = 'n';
	}
	if (flags&STICK_EAST) {
		result[count++] = 'e';
	}
	if (flags&STICK_SOUTH) {
		result[count++] = 's';
	}
	if (flags&STICK_WEST) {
		result[count++] = 'w';
	}
	if (count) {
		result[count] = '\0';
	} else {
		sprintf(result,"{}");
	}
}

/*
 *----------------------------------------------------------------------
 *
 * StringToSticky --
 *
 *        Converts an ascii string representing a widgets stickyness
 *        into the boolean result.
 *
 * Results:
 *        The boolean combination of the "sticky" bits is retuned.  If an
 *        error occurs, such as an invalid character, -1 is returned instead.
 *
 * Side effects:
 *        none
 *
 *----------------------------------------------------------------------
 */

static int
StringToSticky(string)
	char *string;
{
	int sticky = 0;
	char c;

	while ((c = *string++) != '\0') {
		switch (c) {
			case 'n': case 'N': sticky |= STICK_NORTH; break;
			case 'e': case 'E': sticky |= STICK_EAST;  break;
			case 's': case 'S': sticky |= STICK_SOUTH; break;
			case 'w': case 'W': sticky |= STICK_WEST;  break;
			case ' ': case ',': case '\t': case '\r': case '\n': break;
			default: return -1;
		}
	}
	return sticky;
}

/* ------- end of Tk ---------- */

void
Widget_grid_slaves( Handle self)
{
	if ( !var-> gridder )
		return;
	ArrangeGrid((Gridder*) var->gridder);
}

/* removes widget from list of grid slaves */
void
Widget_grid_leave( Handle self)
{
	Gridder *g = (Gridder*) var-> gridder;
	if ( !g)
		return;
	Unlink(g);
	g->masterPtr = NULL;
}

/* become a slave to masterPtr */
static void
link_slave( Gridder *slavePtr, Gridder* masterPtr)
{
	slavePtr->masterPtr = masterPtr;
	InitMasterData(slavePtr->masterPtr);

	slavePtr->nextPtr = masterPtr->slavePtr;
	masterPtr->slavePtr = slavePtr;

	if (masterPtr->abortPtr != NULL) {
		*masterPtr->abortPtr = 1;
	}

	SetGridSize(slavePtr->masterPtr);
}

/* applies grid parameters and enters grid slaves chain */
void
Widget_grid_enter( Handle self)
{
	Gridder *slavePtr = (Gridder*) var-> gridder;

	if (slavePtr->masterPtr != NULL) {
		Unlink(slavePtr);
		slavePtr->masterPtr = NULL;
	}

	if (
		slavePtr->saved_in &&
		!hash_fetch( prima_guts.objects, &slavePtr->saved_in, sizeof(Handle))
	)
		slavePtr->saved_in = NULL_HANDLE;

	link_slave(slavePtr, GetGrid(slavePtr->saved_in ? slavePtr->saved_in : var->owner));
}

void
Widget_grid_destroy( Handle self)
{
	DestroyGrid(self);
}

static HV*
opt2hv( PList in, int start )
{
	HV *profile = prima_hash_create();
	for ( ; start < in->count; start += 2 ) {
		STRLEN l;
		char *key = SvPV((SV*) in->items[start], l);
		SV   *val = (SV*) in->items[start+1];
		(void) hv_store( profile, key, (I32) l, val, 0);
	}
	return profile;
}

XS( Widget_grid_action_FROMPERL)
{
	dXSARGS;
	Handle self;
	char *selector = "";
	int i, ok = false;
	List in, out;

	list_create(&in,  items, 1);
	list_create(&out, 32,   32);

	if (items < 2)
		goto FAIL;

	if ( !( self = gimme_the_mate( ST( 0))))
		goto FAIL;
	selector = SvPV_nolen(ST(1));

	for ( i = 2; i < items; i++)
		list_add( &in, (Handle)(ST(i)));

	SPAGAIN;
	SP -= items;

	if ( strcmp(selector, "bbox") == 0) {
		ok = GridBboxCommand(self, &in, &out);
	} else if (
		(strcmp(selector, "colconfigure") == 0) ||
		(strcmp(selector, "rowconfigure") == 0)
	) {
		ok = GridRowColumnConfigureCommand(self, (*selector == 'c') ? COLUMN : ROW, &in, &out);
	} else if ( strcmp(selector, "configure") == 0) {
		if (( ok = ( in.count > 0 ))) {
			HV * profile = opt2hv(&in, 1);
			ConfigureSlaves(self, (SV*)in.items[0], profile);
			hash_destroy(profile, false);
		}
	} else if (
		(strcmp(selector, "forget") == 0) ||
		(strcmp(selector, "remove") == 0)
	) {
		ok = GridForgetRemoveCommand(self, *selector == 'f', &in, &out);
	} else if ( strcmp(selector, "info") == 0) {
		ok = GridInfoCommand(self, &in, &out);
	} else if ( strcmp(selector, "location") == 0) {
		ok = GridLocationCommand(self, &in, &out);
	} else if ( strcmp(selector, "propagate") == 0) {
		ok = GridPropagateCommand(self, &in, &out);
	} else if ( strcmp(selector, "size") == 0) {
		ok = GridSizeCommand(self, &in, &out);
	} else if ( strcmp(selector, "slaves") == 0) {
		ok = GridSlavesCommand(self, &in, &out);
	} else {
		croak("Widget.grid_action: not such subcommand");
	}

	if ( out.count > 0 ) {
		EXTEND(sp, out.count);
		for ( i = 0; i < out.count; i++)
			PUSHs(sv_2mortal((SV*) out.items[i]));
	}
	list_destroy(&in);
	list_destroy(&out);
	PUTBACK;

FAIL:
	if ( !ok ) croak ("Invalid usage of Widget.grid_action(%s)", selector);
	return;
}

void Widget_grid_action          ( Handle self) { warn("Invalid call of Widget::grid_action"); }
void Widget_grid_action_REDEFINED( Handle self) { warn("Invalid call of Widget::grid_action"); }


#ifdef __cplusplus
}
#endif
