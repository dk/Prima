#include <stdlib.h>
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define _DEBUG
#define DEBUG if(1)printf
#else
#define DEBUG if(0)printf
#endif

/*

The code below is based on the libX11 region implementation

Differences:
  - extents are not calculated
  - recognizes fmOverlay
  - 4-point rectangular polyline is recognized as a RECT but contrary to the
	X11 implementation is NOT a special case, and returns inclusive-exclusive
	coordinates when fmOverlay is unset.

*/

/************************************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

						All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

#define LARGE_COORDINATE 1000000
#define SMALL_COORDINATE -LARGE_COORDINATE

#undef MAXSHORT
#undef MINSHORT
#define MAXSHORT 32767
#define MINSHORT -MAXSHORT

/*
 *     This file contains a few macros to help track
 *     the edge of a filled object.  The object is assumed
 *     to be filled in scanline order, and thus the
 *     algorithm used is an extension of Bresenham's line
 *     drawing algorithm which assumes that y is always the
 *     major axis.
 *     Since these pieces of code are the same for any filled shape,
 *     it is more convenient to gather the library in one
 *     place, but since these pieces of code are also in
 *     the inner loops of output primitives, procedure call
 *     overhead is out of the question.
 *     See the author for a derivation if needed.
 */


/*
 *  In scan converting polygons, we want to choose those pixels
 *  which are inside the polygon.  Thus, we add .5 to the starting
 *  x coordinate for both left and right edges.  Now we choose the
 *  first pixel which is inside the pgon for the left edge and the
 *  first pixel which is outside the pgon for the right edge.
 *  Draw the left pixel, but not the right.
 *
 *  How to add .5 to the starting x coordinate:
 *      If the edge is moving to the right, then subtract dy from the
 *  error term from the general form of the algorithm.
 *      If the edge is moving to the left, then add dy to the error term.
 *
 *  The reason for the difference between edges moving to the left
 *  and edges moving to the right is simple:  If an edge is moving
 *  to the right, then we want the algorithm to flip immediately.
 *  If it is moving to the left, then we don't want it to flip until
 *  we traverse an entire pixel.
 */
#define BRESINITPGON(dy, x1, x2, xStart, d, m, m1, incr1, incr2) {      \
        int dx;      /* local storage */                                \
                                                                        \
        /*                                                              \
         *  if the edge is horizontal, then it is ignored               \
         *  and assumed not to be processed.  Otherwise, do this stuff. \
         */                                                             \
        if ((dy) != 0) {                                                \
                xStart = (x1);                                          \
                dx = (x2) - xStart;                                     \
                if (dx < 0) {                                           \
                        m = dx / (dy);                                  \
                        m1 = m - 1;                                     \
                        incr1 = -2 * dx + 2 * (dy) * m1;                \
                        incr2 = -2 * dx + 2 * (dy) * m;                 \
                        d = 2 * m * (dy) - 2 * dx - 2 * (dy);           \
                } else {                                                \
                        m = dx / (dy);                                  \
                        m1 = m + 1;                                     \
                        incr1 = 2 * dx - 2 * (dy) * m1;                 \
                        incr2 = 2 * dx - 2 * (dy) * m;                  \
                        d = -2 * m * (dy) + 2 * dx;                     \
                }                                                       \
                DEBUG("x start = %d incr1=%d incr2=%d m=%d m1=%d\n", x1, incr1, incr2, m, m1);\
        }                                                               \
}

#define BRESINCRPGON(d, minval, m, m1, incr1, incr2) { \
        if (m1 > 0) {                 \
                if (d > 0) {          \
                        minval += m1; \
                        d += incr1;   \
                }                     \
                else {                \
                        minval += m;  \
                        d += incr2;   \
                }                     \
        } else {                      \
                if (d >= 0) {         \
                        minval += m1; \
                        d += incr1;   \
                }                     \
                else {                \
                        minval += m;  \
                        d += incr2;   \
                }                     \
        }                             \
}


/*
 *     This structure contains all of the information needed
 *     to run the bresenham algorithm.
 *     The variables may be hardcoded into the declarations
 *     instead of using this structure to make use of
 *     register declarations.
 */
typedef struct {
	int minor_axis;	/* minor axis        */
	int d;		/* decision variable */
	int m, m1;		/* slope and slope+1 */
	int incr1, incr2;	/* error increments */
} BRESINFO;


#define BRESINITPGONSTRUCT(dmaj, min1, min2, bres) \
        BRESINITPGON(dmaj, min1, min2, bres.minor_axis, bres.d, \
                 bres.m, bres.m1, bres.incr1, bres.incr2)

#define BRESINCRPGONSTRUCT(bres) \
        BRESINCRPGON(bres.d, bres.minor_axis, bres.m, bres.m1, bres.incr1, bres.incr2)



/*
 *     These are the data structures needed to scan
 *     convert regions.  Two different scan conversion
 *     methods are available -- the even-odd method, and
 *     the winding number method.
 *     The even-odd rule states that a point is inside
 *     the polygon if a ray drawn from that point in any
 *     direction will pass through an odd number of
 *     path segments.
 *     By the winding number rule, a point is decided
 *     to be inside the polygon if a ray drawn from that
 *     point in any direction passes through a different
 *     number of clockwise and counter-clockwise path
 *     segments.
 *
 *     These data structures are adapted somewhat from
 *     the algorithm in (Foley/Van Dam) for scan converting
 *     polygons.
 *     The basic algorithm is to start at the top (smallest y)
 *     of the polygon, stepping down to the bottom of
 *     the polygon by incrementing the y coordinate.  We
 *     keep a list of edges which the current scanline crosses,
 *     sorted by x.  This list is called the Active Edge Table (AET)
 *     As we change the y-coordinate, we update each entry in
 *     in the active edge table to reflect the edges new xcoord.
 *     This list must be sorted at each scanline in case
 *     two edges intersect.
 *     We also keep a data structure known as the Edge Table (ET),
 *     which keeps track of all the edges which the current
 *     scanline has not yet reached.  The ET is basically a
 *     list of ScanLineList structures containing a list of
 *     edges which are entered at a given scanline.  There is one
 *     ScanLineList per scanline at which an edge is entered.
 *     When we enter a new edge, we move it from the ET to the AET.
 *
 *     From the AET, we can implement the even-odd rule as in
 *     (Foley/Van Dam).
 *     The winding number rule is a little trickier.  We also
 *     keep the EdgeTableEntries in the AET linked by the
 *     nextWETE (winding EdgeTableEntry) link.  This allows
 *     the edges to be linked just as before for updating
 *     purposes, but only uses the edges linked by the nextWETE
 *     link as edges representing spans of the polygon to
 *     drawn (as with the even-odd rule).
 */

/*
 * for the winding number rule
 */
#define CLOCKWISE          1
#define COUNTERCLOCKWISE  -1

typedef struct _EdgeTableEntry {
	 int ymax;             /* ycoord at which we exit this edge. */
	 BRESINFO bres;        /* Bresenham info to run the edge     */
	 struct _EdgeTableEntry *next;       /* next in the list     */
	 struct _EdgeTableEntry *back;       /* for insertion sort   */
	 struct _EdgeTableEntry *nextWETE;   /* for winding num rule */
	 int ClockWise;        /* flag for winding number rule       */
#ifdef _DEBUG
	 Point p1, p2;
#endif
} EdgeTableEntry;


typedef struct _ScanLineList{
	 int scanline;              /* the scanline represented */
	 EdgeTableEntry *edgelist;  /* header node              */
	 struct _ScanLineList *next;  /* next in the list       */
} ScanLineList;


typedef struct {
	 int ymax;                 /* ymax for the polygon     */
	 int ymin;                 /* ymin for the polygon     */
	 ScanLineList scanlines;   /* header node              */
} EdgeTable;


/*
 * Here is a struct to help with storage allocation
 * so we can allocate a big chunk at a time, and then take
 * pieces from this heap when we need to.
 */
#define SLLSPERBLOCK 25

typedef struct _ScanLineListBlock {
	 ScanLineList SLLs[SLLSPERBLOCK];
	 struct _ScanLineListBlock *next;
} ScanLineListBlock;

#ifdef _DEBUG
#define EDGE_DEBUG(p) p->p1.x,p->p1.y,p->p2.x,p->p2.y
#else
#define EDGE_DEBUG(p) 0,0,0,0
#endif



/*
 *
 *     a few macros for the inner loops of the fill code where
 *     performance considerations don't allow a procedure call.
 *
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The winding number rule is in effect, so we must notify
 *     the caller when the edge has been removed so he
 *     can reorder the Winding Active Edge Table.
 */
#define EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET) {      \
   if (pAET->ymax == y) {          /* leaving this edge */     \
          DEBUG("v edge exit\n");                              \
          pPrevAET->next = pAET->next;                         \
          pAET = pPrevAET->next;                               \
          fixWAET = 1;                                         \
          if (pAET)                                            \
                 pAET->back = pPrevAET;                        \
   }                                                           \
   else {                                                      \
          BRESINCRPGONSTRUCT(pAET->bres);                      \
          DEBUG("| edge update x=%d\n", pAET->bres.minor_axis);\
          pPrevAET = pAET;                                     \
          pAET = pAET->next;                                   \
   }                                                           \
}


/*
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The even-odd rule is in effect.
 */
#define EVALUATEEDGEEVENODD(pAET, pPrevAET, y) {               \
   if (pAET->ymax == y) {          /* leaving this edge */     \
          DEBUG("v edge exit\n");                              \
          pPrevAET->next = pAET->next;                         \
          pAET = pPrevAET->next;                               \
          if (pAET)                                            \
                 pAET->back = pPrevAET;                        \
   }                                                           \
   else {                                                      \
          BRESINCRPGONSTRUCT(pAET->bres);                      \
          DEBUG("| edge update x=%d\n", pAET->bres.minor_axis);\
          pPrevAET = pAET;                                     \
          pAET = pAET->next;                                   \
   }                                                           \
}

/*
 *     InsertEdgeInET
 *
 *     Insert the given edge into the edge table.
 *     First we must find the correct bucket in the
 *     Edge table, then find the right slot in the
 *     bucket.  Finally, we can insert it.
 *
 */
static void
InsertEdgeInET(
	EdgeTable *ET,
	EdgeTableEntry *ETE,
	int scanline,
	ScanLineListBlock **SLLBlock,
	int *iSLLBlock)
{
	register EdgeTableEntry *start, *prev;
	register ScanLineList *pSLL, *pPrevSLL;
	ScanLineListBlock *tmpSLLBlock;

	/*
	 * find the right bucket to put the edge into
	 */
	pPrevSLL = &ET->scanlines;
	pSLL = pPrevSLL->next;
	while (pSLL && (pSLL->scanline < scanline))
	{
		pPrevSLL = pSLL;
		pSLL = pSLL->next;
	}

	/*
	 * reassign pSLL (pointer to ScanLineList) if necessary
	 */
	if ((!pSLL) || (pSLL->scanline > scanline))
	{
		if (*iSLLBlock > SLLSPERBLOCK-1)
		{
			tmpSLLBlock = malloc(sizeof(ScanLineListBlock));
			(*SLLBlock)->next = tmpSLLBlock;
			tmpSLLBlock->next = (ScanLineListBlock *)NULL;
			*SLLBlock = tmpSLLBlock;
			*iSLLBlock = 0;
		}
		pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

		pSLL->next = pPrevSLL->next;
		pSLL->edgelist = (EdgeTableEntry *)NULL;
		pPrevSLL->next = pSLL;
	}
	pSLL->scanline = scanline;

	/*
	 * now insert the edge in the right bucket
	 */
	prev = (EdgeTableEntry *)NULL;
	start = pSLL->edgelist;
	while (start && (start->bres.minor_axis < ETE->bres.minor_axis))
	{
		prev = start;
		start = start->next;
	}
	ETE->next = start;

	if (prev) {
		prev->next = ETE;
	}
	else {
		pSLL->edgelist = ETE;
	}
}

static void
init_edge_table(
	EdgeTable *ET,
	EdgeTableEntry *AET,
	ScanLineListBlock   *pSLLBlock)
{
	/*
	 *  initialize the Active Edge Table
	 */
	AET->next = (EdgeTableEntry *)NULL;
	AET->back = (EdgeTableEntry *)NULL;
	AET->nextWETE = (EdgeTableEntry *)NULL;
	AET->bres.minor_axis = SMALL_COORDINATE;

	/*
	 *  initialize the Edge Table.
	 */
	ET->scanlines.next = (ScanLineList *)NULL;
	ET->ymax = SMALL_COORDINATE;
	ET->ymin = LARGE_COORDINATE;
	pSLLBlock->next = (ScanLineListBlock *)NULL;
}

/*
 *     CreateEdgeTable
 *
 *     This routine creates the edge table for
 *     scan converting polygons.
 *     The Edge Table (ET) looks like:
 *
 *    EdgeTable
 *     --------
 *    |  ymax  |        ScanLineLists
 *    |scanline|-->------------>-------------->...
 *     --------   |scanline|   |scanline|
 *                |edgelist|   |edgelist|
 *                ---------    ---------
 *                    |             |
 *                    |             |
 *                    V             V
 *              list of ETEs   list of ETEs
 *
 *     where ETE is an EdgeTableEntry data structure,
 *     and there is one ScanLineList per scanline at
 *     which an edge is initially entered.
 *
 */

static void
CreateETandAET(
	register int count,
	register Point *pts,
	EdgeTable *ET,
	EdgeTableEntry *AET,
	register EdgeTableEntry *pETEs,
	ScanLineListBlock   *pSLLBlock)
{
	register Point *top, *bottom;
	register Point *PrevPt, *CurrPt;
	int iSLLBlock = 0;
	int dy;

	init_edge_table(ET, AET, pSLLBlock);
	PrevPt = &pts[count-1];

	/*
	 *  for each vertex in the array of points.
	 *  In this loop we are dealing with two vertices at
	 *  a time -- these make up one edge of the polygon.
	 */
	while (count--)
	{
		CurrPt = pts++;

		/*
		 *  find out which point is above and which is below.
		 */
		DEBUG("new edge %d.%d-%d.%d\n", PrevPt->x, PrevPt->y, CurrPt->x, CurrPt->y);
#ifdef _DEBUG
		pETEs->p1 = *PrevPt;
		pETEs->p2 = *CurrPt;
#endif
		if (PrevPt->y > CurrPt->y)
		{
			top = PrevPt, bottom = CurrPt;
			pETEs->ClockWise = 0;
		}
		else
		{
			top = CurrPt, bottom = PrevPt;
			pETEs->ClockWise = 1;
		}

		/*
		 * don't add horizontal edges to the Edge table.
		 */
		if (bottom->y != top->y)
		{
			pETEs->ymax = top->y - 1;  /* -1 so we don't get last scanline */

			/*
			 *  initialize integer edge algorithm
			 */
			dy = top->y - bottom->y;
			BRESINITPGONSTRUCT(dy, bottom->x, top->x, pETEs->bres);

			InsertEdgeInET(ET, pETEs, bottom->y, &pSLLBlock, &iSLLBlock);

			if (PrevPt->y > ET->ymax)
				ET->ymax = PrevPt->y;
			if (PrevPt->y < ET->ymin)
				ET->ymin = PrevPt->y;
			pETEs++;
		}

		PrevPt = CurrPt;
	}
}

static void
CreateETandAET_Boxes(
	register int count,
	register Box *boxes,
	EdgeTable *ET,
	EdgeTableEntry *AET,
	register EdgeTableEntry *pETEs,
	ScanLineListBlock   *pSLLBlock)
{
	int iSLLBlock = 0;
	init_edge_table(ET, AET, pSLLBlock);

	while ( count-- ) {
		int x, y;

#define INSERT_EDGE                                                          \
 		pETEs->ymax = boxes->y + boxes->height - 1;  /* we actually do get last scanline for a box */ \
 		BRESINITPGONSTRUCT(boxes->height, x, x, pETEs->bres);        \
 		InsertEdgeInET(ET, pETEs, boxes->y, &pSLLBlock, &iSLLBlock); \
 		if (y > ET->ymax) ET->ymax = y;                              \
 		if (y < ET->ymin) ET->ymin = y;                              \
 		pETEs++;

		pETEs->ClockWise = 1;
		x = boxes->x;
		y = boxes->y;
#ifdef _DEBUG
		pETEs->p1.x = pETEs->p2.x = x;
		pETEs->p1.y = boxes->y;
		pETEs->p2.y = boxes->y + boxes->height - 1;
#endif
		INSERT_EDGE;

		pETEs->ClockWise = 0;
		x = boxes->x + boxes->width  - 1;
		y = boxes->y + boxes->height;
#ifdef _DEBUG
		pETEs->p1.x = pETEs->p2.x = x;
		pETEs->p1.y = boxes->y;
		pETEs->p2.y = boxes->y + boxes->height - 1;
#endif
		INSERT_EDGE;

			boxes++;
	}
}

/*
 *     loadAET
 *
 *     This routine moves EdgeTableEntries from the
 *     EdgeTable into the Active Edge Table,
 *     leaving them sorted by smaller x coordinate.
 *
 */

static void
loadAET(
	register EdgeTableEntry *AET,
	register EdgeTableEntry *ETEs)
{
	register EdgeTableEntry *pPrevAET;
	register EdgeTableEntry *tmp;
#ifdef _DEBUG
	EdgeTableEntry *saveAET = AET;
#endif

	pPrevAET = AET;
	AET = AET->next;
	while (ETEs)
	{
		while (AET && (AET->bres.minor_axis < ETEs->bres.minor_axis))
		{
			pPrevAET = AET;
			AET = AET->next;
		}
		tmp = ETEs->next;
		ETEs->next = AET;
		if (AET)
			AET->back = ETEs;
		ETEs->back = pPrevAET;
		pPrevAET->next = ETEs;
		pPrevAET = ETEs;

		DEBUG("^ edge %d.%d-%d.%d enter\n", EDGE_DEBUG(ETEs));
		ETEs = tmp;
	}
#ifdef _DEBUG
	DEBUG("active edges: ");
	AET = saveAET->next;
	while ( AET ) {
		DEBUG("x=%d(%d.%d-%d.%d) ", AET->bres.minor_axis, EDGE_DEBUG(AET));
		AET = AET->next;
	}
	DEBUG("\n");
#endif
}

/*
 *     computeWAET
 *
 *     This routine links the AET by the
 *     nextWETE (winding EdgeTableEntry) link for
 *     use by the winding number rule.  The final
 *     Active Edge Table (AET) might look something
 *     like:
 *
 *     AET
 *     ----------  ---------   ---------
 *     |ymax    |  |ymax    |  |ymax    |
 *     | ...    |  |...     |  |...     |
 *     |next    |->|next    |->|next    |->...
 *     |nextWETE|  |nextWETE|  |nextWETE|
 *     ---------   ---------   ^--------
 *         |                   |       |
 *         V------------------->       V---> ...
 *
 */
static void
computeWAET(
	register EdgeTableEntry *AET)
{
	register EdgeTableEntry *pWETE;
	register int inside = 1;
	register int isInside = 0;

	AET->nextWETE = (EdgeTableEntry *)NULL;
	pWETE = AET;
	AET = AET->next;
	while (AET)
	{
		if (AET->ClockWise)
			isInside++;
		else
			isInside--;

		if ((!inside && !isInside) ||
			( inside &&  isInside))
		{
			pWETE->nextWETE = AET;
			pWETE = AET;
			inside = !inside;
		}
		AET = AET->next;
	}
	pWETE->nextWETE = (EdgeTableEntry *)NULL;
}

/*
 *     InsertionSort
 *
 *     Just a simple insertion sort using
 *     pointers and back pointers to sort the Active
 *     Edge Table.
 *
 */

static int
InsertionSort(
	register EdgeTableEntry *AET)
{
	register EdgeTableEntry *pETEchase;
	register EdgeTableEntry *pETEinsert;
	register EdgeTableEntry *pETEchaseBackTMP;
	register int changed = 0;

	AET = AET->next;
	while (AET)
	{
		pETEinsert = AET;
		pETEchase = AET;
		while (pETEchase->back->bres.minor_axis > AET->bres.minor_axis)
			pETEchase = pETEchase->back;

		AET = AET->next;
		if (pETEchase != pETEinsert)
		{
			pETEchaseBackTMP = pETEchase->back;
			pETEinsert->back->next = AET;
			if (AET)
				AET->back = pETEinsert->back;
			pETEinsert->next = pETEchase;
			pETEchase->back->next = pETEinsert;
			pETEchase->back = pETEinsert;
			pETEinsert->back = pETEchaseBackTMP;
			changed = 1;
		}
	}
	return(changed);
}

/*
 *     Clean up our act.
 */
static void
FreeStorage(
	register ScanLineListBlock   *pSLLBlock)
{
	register ScanLineListBlock   *tmpSLLBlock;

	while (pSLLBlock)
	{
		tmpSLLBlock = pSLLBlock->next;
		free(pSLLBlock);
		pSLLBlock = tmpSLLBlock;
	}
}
 
 /* number of points to buffer before sending them off
 * to scanlines() :  Must be an even number
 */
#define NUMPTSTOBUFFER 200

/*

 1. Add the intersection point
 3. Send out the buffer if filled

 */

#define ADD_POINT                                               \
        pts->x = pAET->bres.minor_axis,  pts->y = y;            \
        if (clip != NULL && (curPtBlock->size % 2) == 1) {      \
                if (                                            \
                        pts[-1].x > clip-> right  ||            \
                        pts->x    < clip-> left   ||            \
                        pts->y    < clip-> bottom ||            \
                        pts->y    > clip-> top                  \
                ) {                                             \
                        DEBUG("SKIP.%d %d-%d/%d-%d %d/%d-%d\n", curPtBlock->size, pts[-1].x, pts->x, clip->left, clip->right, pts->y, clip->bottom, clip->top);\
                        pts              -= 2;                  \
                        curPtBlock->size -= 2;                  \
                } else {                                        \
                        if (pts[-1].x < clip->left  )           \
                                pts[-1].x = clip->left;         \
                        if (pts->x    > clip->right )           \
                                pts->x    = clip->right;        \
                        DEBUG("ADD.%d %d-%d %d\n", curPtBlock->size, pts[-1].x, pts->x, pts->y);\
                }                                               \
        }                                                       \
        DEBUG("+p %d %d\n", pts->x, pts->y);                    \
        pts++, curPtBlock->size++;                              \
        if (curPtBlock->size == curr_max_size) {                \
                curr_max_size *= 2;                             \
                tmpPtBlock = malloc(sizeof(PolyPointBlock) + (curr_max_size - 1) * sizeof(Point)); \
                tmpPtBlock->next = NULL;                        \
                tmpPtBlock->size = 0   ;                        \
                curPtBlock->next = tmpPtBlock;                  \
                curPtBlock = tmpPtBlock;                        \
                pts = curPtBlock->pts;                          \
        }

PolyPointBlock*
poly_poly2points(
	Point     *Pts,                  /* the pts                 */
	int       Count,                 /* number of pts           */
	int       rule,                  /* winding and outline     */
	PRect     clip
) {
	register EdgeTableEntry *pAET;   /* Active Edge Table       */
	register int y;                  /* current scanline        */
	register EdgeTableEntry *pWETE;  /* Winding Edge Table Entry*/
	register ScanLineList *pSLL;     /* current scanLineList    */
	register Point *pts;             /* output buffer           */
	EdgeTableEntry *pPrevAET;        /* ptr to previous AET     */
	EdgeTable ET;                    /* header node for ET      */
	EdgeTableEntry AET;              /* header node for AET     */
	EdgeTableEntry *pETEs;           /* EdgeTableEntries pool   */
	ScanLineListBlock SLLBlock;      /* header for scanlinelist */
	int fixWAET = 0;
	PolyPointBlock *FirstPtBlock, *curPtBlock; /* PtBlock buffers    */
	PolyPointBlock *tmpPtBlock;
	Bool outline, winding;
	unsigned long curr_max_size = NUMPTSTOBUFFER;

	outline = (rule & fmOverlay) ? 1 : 0;
	winding = (rule & fmWinding) ? 1 : 0;
	DEBUG("init: %s %s\n", winding ? "wind" : "alt", outline ? "outl" : "raw");

	if ( Count < 2 )
		return NULL;

	if (! (pETEs = malloc(Count * sizeof(EdgeTableEntry))))
		return NULL;

	if ( !( FirstPtBlock = malloc(sizeof(PolyPointBlock) + ( curr_max_size - 1 ) * sizeof(Point)))) {
		free(pETEs);
		return NULL;
	}
	FirstPtBlock->next = NULL;
	FirstPtBlock->size = 0;

	pts = FirstPtBlock->pts;
	CreateETandAET(Count, Pts, &ET, &AET, pETEs, &SLLBlock);
	pSLL = ET.scanlines.next;
	curPtBlock = FirstPtBlock;

	if (winding) {
		/*
		 *  for each scanline
		 */
		DEBUG("for %d <=> %d\n", ET.ymin, ET.ymax);
		for (y = ET.ymin; y < ET.ymax; y++) {
			DEBUG("== %d ==\n", y);
			/*
			 *  Add a new edge to the active edge table when we
			 *  get to the next edge.
			 */
			if (pSLL != NULL && y == pSLL->scanline) {
				loadAET(&AET, pSLL->edgelist);
				computeWAET(&AET);
				pSLL = pSLL->next;
			}
			pPrevAET = &AET;
			pAET = AET.next;
			pWETE = pAET;

			/*
			 *  for each active edge
			 */
			while (pAET) {
				/*
				 *  add to the buffer only those edges that
				 *  are in the Winding active edge table.
				 */
				if (pWETE == pAET) {
					ADD_POINT;
					pWETE = pWETE->nextWETE;
				}
				EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
			}

			/*
			 *  recompute the winding active edge table if
			 *  we just resorted or have exited an edge.
			 */
			if (InsertionSort(&AET) || fixWAET) {
				computeWAET(&AET);
				fixWAET = 0;
			}
		}
	}
	else {
		/*
		 *  for each scanline
		 */
		DEBUG("for %d <=> %d\n", ET.ymin, ET.ymax);
		for (y = ET.ymin; y < ET.ymax; y++) {
			DEBUG("== %d ==\n", y);
			/*
			 *  Add a new edge to the active edge table when we
			 *  get to the next edge.
			 */
			if (pSLL != NULL && y == pSLL->scanline) {
				loadAET(&AET, pSLL->edgelist);
				pSLL = pSLL->next;
			}
			pPrevAET = &AET;
			pAET = AET.next;

			/*
			 *  for each active edge
			 */
			while (pAET) {
				ADD_POINT;
				EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
			}
			(void) InsertionSort(&AET);
		}
	}
	FreeStorage(SLLBlock.next);
	free(pETEs);

	return FirstPtBlock;
}

PolyPointBlock*
poly_region2points(
	PRegionRec rgn,
	PRect     clip
) {
	register EdgeTableEntry *pAET;   /* Active Edge Table       */
	register int y;                  /* current scanline        */
	register ScanLineList *pSLL;     /* current scanLineList    */
	register Point *pts;             /* output buffer           */
	EdgeTableEntry *pPrevAET;        /* ptr to previous AET     */
	EdgeTable ET;                    /* header node for ET      */
	EdgeTableEntry AET;              /* header node for AET     */
	EdgeTableEntry *pETEs;           /* EdgeTableEntries pool   */
	ScanLineListBlock SLLBlock;      /* header for scanlinelist */
	PolyPointBlock *FirstPtBlock, *curPtBlock; /* PtBlock buffers    */
	PolyPointBlock *tmpPtBlock;
	unsigned long curr_max_size = NUMPTSTOBUFFER;
	unsigned int Count = rgn->n_boxes * 2;

	if ( Count < 2 )
		return NULL;

	if (! (pETEs = malloc(Count * sizeof(EdgeTableEntry))))
		return NULL;

	if ( !( FirstPtBlock = malloc(sizeof(PolyPointBlock) + ( curr_max_size - 1 ) * sizeof(Point)))) {
		free(pETEs);
		return NULL;
	}
	FirstPtBlock->next = NULL;
	FirstPtBlock->size = 0;

	pts = FirstPtBlock->pts;
	CreateETandAET_Boxes(rgn->n_boxes, rgn->boxes, &ET, &AET, pETEs, &SLLBlock);
	pSLL = ET.scanlines.next;
	curPtBlock = FirstPtBlock;

	DEBUG("for %d <=> %d\n", ET.ymin, ET.ymax);
	for (y = ET.ymin; y < ET.ymax; y++) {
		DEBUG("== %d ==\n", y);
		/*
		 *  Add a new edge to the active edge table when we
		 *  get to the next edge.
		 */
		if (pSLL != NULL && y == pSLL->scanline) {
			loadAET(&AET, pSLL->edgelist);
			pSLL = pSLL->next;
		}
		pPrevAET = &AET;
		pAET = AET.next;

		/*
		 *  for each active edge
		 */
		while (pAET) {
			ADD_POINT;
			EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
		}
		(void) InsertionSort(&AET);
	}
	FreeStorage(SLLBlock.next);
	free(pETEs);

	return FirstPtBlock;
}

void
poly_free_blocks( PolyPointBlock * first )
{
	PolyPointBlock * curPtBlock;
	for (curPtBlock = first; curPtBlock != NULL; ) {
		PolyPointBlock *tmpPtBlock = curPtBlock->next;
		free(curPtBlock);
		curPtBlock = tmpPtBlock;
	}
}

#ifdef __cplusplus
}
#endif
