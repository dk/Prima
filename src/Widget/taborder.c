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

static void
fill_tab_candidates( PList list, Handle level)
{
	int i;
	PList w = &(PWidget( level)-> widgets);
	for ( i = 0; i < w-> count; i++) {
		Handle x = w-> items[i];
		if ( CWidget( x)-> get_visible( x) && CWidget( x)-> get_enabled( x)) {
			if ( CWidget( x)-> get_selectable( x) && CWidget( x)-> get_tabStop( x))
				list_add( list, x);
			fill_tab_candidates( list, x);
		}
	}
}

Handle
Widget_next_positional( Handle self, int dx, int dy)
{
	Handle horizon = self;

	int i, maxDiff = INT_MAX;
	Handle max = NULL_HANDLE;
	List candidates;
	Point p[2];

	int minor[2], major[2], axis, extraDiff, ir[4];

	/*
		In order to compute positional difference, using four penalties.
		To simplify algorithm, Rect will be translated to int[4] and
		minor, major and extraDiff assigned to array indices for those
		steps - minor for first and third, major for second and extraDiff for last one.
	*/

	axis = ( dx == 0) ? dy : dx;
	minor[0] = ( dx == 0) ? 0 : 1;
	minor[1] = minor[0] + 2;
	extraDiff = major[(axis < 0) ? 0 : 1] = ( dx == 0) ? 1 : 0;
	major[(axis < 0) ? 1 : 0] = extraDiff + 2;
	extraDiff = ( dx == 0) ? (( axis < 0) ? 0 : 2) : (( axis < 0) ? 1 : 3);

	while ( PWidget( horizon)-> owner) {
		if (
			( PWidget( horizon)-> options. optSystemSelectable) || /* fast check for CWindow */
			( PWidget( horizon)-> options. optModalHorizon)
			) break;
		horizon = PWidget( horizon)-> owner;
	}

	if ( !CWidget( horizon)-> get_visible( horizon) ||
		!CWidget( horizon)-> get_enabled( horizon)) return NULL_HANDLE;

	list_create( &candidates, 64, 64);
	fill_tab_candidates( &candidates, horizon);

	p[0].x = p[0].y = 0;
	p[1] = CWidget( self)-> get_size( self);
	apc_widget_map_points( self, true, 2, p);
	apc_widget_map_points( horizon, false, 2, p);
	ir[0] = p[0].x; ir[1] = p[0].y; ir[2] = p[1].x; ir[3] = p[1].y;

	for ( i = 0; i < candidates. count; i++) {
		int    diff, ix[4];
		Handle x = candidates. items[i];

		if ( x == self) continue;

		p[0].x = p[0].y = 0;
		p[1] = CWidget( x)-> get_size( x);
		apc_widget_map_points( x, true, 2, p);
		apc_widget_map_points( horizon, false, 2, p);
		ix[0] = p[0].x; ix[1] = p[0].y; ix[2] = p[1].x; ix[3] = p[1].y;

		/* First step - checking if the widget is subject to comparison. It is not,
			if it's minor axis is not contiguous with self's */

		if ( ix[ minor[0]] > ir[ minor[1]] || ix[ minor[1]] < ir[ minor[0]])
			continue;

		/* Using x100 penalty for distance in major axis - and discarding those that
			of different sign */
		diff = ( ix[ major[ 1]] - ir[ major[0]]) * 100 * axis;
		if ( diff < 0)
			continue;

		/* Adding x10 penalty for incomplete minor axis congruence. Addition goes in tenths,
			in a way to not allow congruence overweight major axis distance */
		if ( ix[ minor[0]] > ir[ minor[0]])
			diff += ( ix[ minor[0]] - ir[ minor[0]]) * 100 / ( ir[ minor[1]] - ir[ minor[0]]);
		if ( ix[ minor[1]] < ir[ minor[1]])
			diff += ( ir[ minor[1]] - ix[ minor[1]]) * 100 / ( ir[ minor[1]] - ir[ minor[0]]);

		/* Adding 'distance from level' x1 penalty */
		if (( ix[ extraDiff] - ir[ extraDiff]) * axis < 0)
			diff += abs( ix[ extraDiff] - ir[ extraDiff]);

		if ( diff < maxDiff) {
			max = x;
			maxDiff = diff;
		}
	}

	list_destroy( &candidates);

	return max;
}

static int compare_taborders_forward( const void *a, const void *b)
{
	if ((*(PWidget*) a)-> tabOrder < (*(PWidget*) b)-> tabOrder)
		return -1; else
	if ((*(PWidget*) a)-> tabOrder > (*(PWidget*) b)-> tabOrder)
		return 1;
	else
		return 0;
}

static int compare_taborders_backward( const void *a, const void *b)
{
	if ((*(PWidget*) a)-> tabOrder < (*(PWidget*) b)-> tabOrder)
		return 1; else
	if ((*(PWidget*) a)-> tabOrder > (*(PWidget*) b)-> tabOrder)
		return -1;
	else
		return 0;
}

static int
do_taborder_candidates( Handle level, Handle who,
	int (*compareProc)(const void *, const void *),
	int * stage, Handle * result)
{
	int i, fsel = -1;
	PList w = &(PWidget( level)-> widgets);
	Handle * ordered;

	if ( w-> count == 0) return true;

	ordered = ( Handle *) malloc( w-> count * sizeof( Handle));
	if ( !ordered) return true;

	memcpy( ordered, w-> items, w-> count * sizeof( Handle));
	qsort( ordered, w-> count, sizeof( Handle), compareProc);

	/* finding current widget in the group */
	for ( i = 0; i < w-> count; i++) {
		Handle x = ordered[i];
		if ( CWidget( x)-> get_current( x)) {
			fsel = i;
			break;
		}
	}
	if ( fsel < 0) fsel = 0;

	for ( i = 0; i < w-> count; i++) {
		int j;
		Handle x;

		j = i + fsel;
		if ( j >= w-> count) j -= w-> count;

		x = ordered[j];
		if ( CWidget( x)-> get_visible( x) && CWidget( x)-> get_enabled( x)) {
			if ( CWidget( x)-> get_selectable( x) && CWidget( x)-> get_tabStop( x)) {
				if ( *result == NULL_HANDLE) *result = x;
				switch( *stage) {
				case 0: /* nothing found yet */
					if ( x == who) *stage = 1;
					break;
				default:
					/* next widget after 'who' is ours */
					*result = x;
					free( ordered);
					return false;
				}
			}
			if ( !do_taborder_candidates( x, who, compareProc, stage, result)) {
				free( ordered);
				return false; /* fall through */
			}
		}
	}
	free( ordered);
	return true;
}

Handle
Widget_next_tab( Handle self, Bool forward)
{
	Handle horizon = self, result = NULL_HANDLE;
	int stage = 0;

	while ( PWidget( horizon)-> owner) {
		if (
			( PWidget( horizon)-> options. optSystemSelectable) || /* fast check for CWindow */
			( PWidget( horizon)-> options. optModalHorizon)
			) break;
		horizon = PWidget( horizon)-> owner;
	}

	if ( !CWidget( horizon)-> get_visible( horizon) ||
		!CWidget( horizon)-> get_enabled( horizon)) return NULL_HANDLE;

	do_taborder_candidates( horizon, self,
		forward ? compare_taborders_forward : compare_taborders_backward,
		&stage, &result);
	if ( result == self) result = NULL_HANDLE;
	return result;
}

int
Widget_tabOrder( Handle self, Bool set, int tabOrder)
{
	int count;
	PWidget owner;

	if ( var-> stage > csFrozen) return 0;
	if ( !set)
		return var-> tabOrder;

	owner = ( PWidget) var-> owner;
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
			return 0;
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
			/* incrementing all tabOrders that greater than ours */
			for ( i = 0; i < count; i++) {
				PWidget ctrl = ( PWidget) owner-> widgets. items[ i];
				if ( self == ( Handle) ctrl) continue;
				if ( ctrl-> tabOrder >= tabOrder) ctrl-> tabOrder++;
			}
		var-> tabOrder = tabOrder;
	}
	return 0;
}

Bool
Widget_tabStop( Handle self, Bool set, Bool stop)
{
	if ( !set)
		return is_opt( optTabStop);
	opt_assign( optTabStop, stop);
	return false;
}


#ifdef __cplusplus
}
#endif
