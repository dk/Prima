#include "apricot.h"
#include "Drawable.h"
#include "private/Drawable.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GS  var->current_state

static Bool
read_line_end(SV *lineEnd, DrawablePaintState *state, int index)
{
	int j, l;
	AV* av;
	SV *rv;
	List tmp_list;

	bzero(&tmp_list, sizeof(tmp_list));

	bzero( &state->line_end[index], sizeof(state->line_end[index]));
	state->line_end[index].type = (index == 0) ? leRound : leDefault;

	if ( !SvROK(lineEnd)) {
		if ( !SvOK(lineEnd)) {
			if ( index == 0 ) {
				warn("cannot set lineEnd[0] to undef");
				return false;
			}
			state->line_end[index].type = leDefault;
		} else {
			int le = SvIV( lineEnd );
			if ( le < 0 || le > leMax ) le = 0;
			state->line_end[index].type = le;
		}
		return true;
	}

	rv = SvRV(lineEnd);
	if ( SvTYPE(rv) != SVt_PVAV) {
		warn("lineEnd: not an array passed");
		return false;
	}
	av = (AV*) rv;

	list_create(&tmp_list, 16, 16);
	l = av_len(av);
	for ( j = 0; j <= l; j += 2) {
		SV *param, **holder;
		char *cmd_src;
		int div, min, max, cmd_dst, n_points;
		double *params;
		PPathCommand pathcmd;

		holder = av_fetch(av, j, 0);
		if ( !( holder && *holder && SvOK(*holder) )) {
			warn("bad array item %d in lineEnd descriptor %d\n", j, index);
			goto FAIL;
		}
		cmd_src = SvPV(*holder, PL_na);

		holder = av_fetch(av, j+1, 0);
		if ( !( holder && *holder && SvOK(*holder) )) {
			warn("bad array item %d in lineEnd descriptor %d\n", j + 1, index);
			goto FAIL;
		}
		param = *holder;

		if ( strcmp(cmd_src, "line") == 0) {
			div = 2;
			min = 1;
			max = -1;
			cmd_dst = leCmdLine;
		} else if ( strcmp(cmd_src, "conic") == 0) {
			div = 2;
			min = 2;
			max = -1;
			cmd_dst = leCmdConic;
		} else if ( strcmp(cmd_src, "cubic") == 0) {
			div = 2;
			min = 3;
			max = -1;
			cmd_dst = leCmdCubic;
		} else if (
			( strcmp(cmd_src, "open") == 0 ) ||
			( strcmp(cmd_src, "arc") == 0 )
		) {
			warn("command '%s' is not allowed in lineEnd descriptors", cmd_src);
			goto FAIL;
		} else {
			warn("Unknown command #%d in lineEnd descriptor %d", index, j);
			goto FAIL;
		}

		if ( !( params = prima_read_array( param, "lineEnd", 'd', div, min, max, &n_points, NULL)))
			goto FAIL;
		if ( !( pathcmd = malloc(sizeof(PathCommand)))) {
			free(params);
			goto FAIL;
		}
		pathcmd-> command = cmd_dst;
		pathcmd-> n_args  = n_points * div;
		list_add( &tmp_list, (Handle) pathcmd);
		list_add( &tmp_list, (Handle) params);
	}

	state->line_end[index].type = (tmp_list.count == 0) ? leFlat : leCustom;

	/* compact into a single-block structure */
	if ( state->line_end[index].type == leCustom ) {
		int j, k, sz, n;
		PPath p;
		n = tmp_list.count / 2;
		sz = sizeof(Path) + sizeof(PPathCommand) * (n - 1);
		for ( j = 0; j < tmp_list.count; j += 2 ) {
			PPathCommand pc = (PPathCommand) list_at(&tmp_list, j );
			sz += sizeof(PathCommand) + (pc->n_args - 1) * sizeof(double);
		}

		if ( !( p = malloc(sz))) goto FAIL;
		p->refcnt     = 0;
		p->n_commands = n;
		p->commands   = p->commands_buf;

		sz = sizeof(Path) + sizeof(PPathCommand) * (n - 1);
		for ( j = k = 0; j < tmp_list.count; j += 2, k++ ) {
			PPathCommand pc_dst;
			PPathCommand pc_src = (PPathCommand) list_at(&tmp_list, j );
			double *data        = (double*)      list_at(&tmp_list, j + 1 );

			p->commands[k] = pc_dst = (PPathCommand)(((Byte*) p) + sz);
			*pc_dst      = *pc_src;
			pc_dst->args = pc_dst-> args_buf;
			memcpy( pc_dst->args, data, pc_src->n_args * sizeof(double));
			sz += sizeof(PathCommand) + (pc_src->n_args - 1) * sizeof(double);
		}

		state->line_end[index].path = p;
	}
	list_delete_all(&tmp_list, true);

	return true;

FAIL:
	if ( state->line_end[index].type == leCustom ) {
		state->line_end[index].type = (index == 0) ? leRound : leDefault;
		free(state->line_end[index].path);
		state->line_end[index].path = NULL;
	}
	list_delete_all(&tmp_list, true);

	return false;
}

Bool
Drawable_read_line_ends(SV *lineEnd, DrawablePaintState *state)
{
	int i;
	Bool four_individual_constants;
	SV **holder;
	AV* av;
	SV *rv;

	if ( !SvROK(lineEnd)) {
		int i, le = SvIV( lineEnd );
		if ( le < 0 || le > leMax ) le = 0;
		state->line_end[0].type = le;
		for ( i = 1; i <= leiMax; i++)
			state->line_end[i].type = leDefault;
		return true;
	}

	rv = SvRV(lineEnd);
	if ( SvTYPE(rv) != SVt_PVAV) {
		warn("lineEnd: not an array passed");
		return false;
	}
	av = (AV*) rv;

	/* is it a leCustom for all 4 or 4 separate entries? */
	holder  = av_fetch(av, 0, 0);
	four_individual_constants = (holder && *holder && SvOK(*holder)) ?
		(SvNOK(*holder) || SvIOK(*holder) || SvROK(*holder)) :
		false;

	if ( four_individual_constants ) {
		if ( av_len(av) > leiMax ) warn("lineEnd: only %d items are understood, the rest is ignored", leiMax + 1);

		/* parse SVs */
		for ( i = 0; i <= leiMax; i++) {
			holder = av_fetch( av, i, 0);
			if ( !( holder && *holder && SvOK(*holder) ) ) {
				if ( i == 0 ) {
					warn("lineEnd: first item in array cannot be undef");
					goto FAIL;
				}
				state->line_end[i].type = leDefault;
				continue;
			}

			if ( !read_line_end(*holder, state, i))
				goto FAIL;
		}
	} else {
		int i;
		if ( !read_line_end(lineEnd, state, 0))
			goto FAIL;
		for ( i = 1; i <= leiMax; i++)
			state->line_end[i].type = leDefault;
	}

	return true;

FAIL:
	for ( i = 0; i <= leiMax; i++) {
		if ( state->line_end[i].type == leCustom ) {
			state->line_end[i].type = (i == 0) ? leRound : leDefault;
			free(state->line_end[i].path);
			state->line_end[i].path = NULL;
		}
	}

	return false;
}

static void
line_end_refcnt( DrawablePaintState *gs, int index, int delta)
{
	if ( gs->line_end[index].type == leCustom ) {
		PPath p = gs->line_end[index].path;
		if ( delta < 0 ) {
			if ( p-> refcnt-- <= 0 ) {
				free(p);
				gs->line_end[index].path = NULL;
				gs->line_end[index].type = (index == 0) ? leRound : leDefault;
			}
		} else
			p-> refcnt++;
	}
}

void
Drawable_line_end_refcnt( DrawablePaintState *gs, int delta)
{
	int i;
	for ( i = 0; i <= leiMax; i++)
		line_end_refcnt( gs, i, delta);
}

static SV*
produce_line_end(Handle self, int index)
{
	int j, k;
	PPath p;
	PPathCommand *pc;
	AV *av, *av2;

	switch ( GS.line_end[index].type ) {
	case leDefault:
		return NULL_SV;
	case leCustom:
		if ( !( p = GS.line_end[index].path)) { 
			warn("panic: bad line_end #%d structure", index);
			return NULL_SV;
		}
		break;
	default:
		return newSViv(GS.line_end[index].type);
	}

	av = newAV();

	for ( j = 0, pc = p->commands; j < p->n_commands; j++, pc++) {
		switch ( (*pc)->command ) {
		case leCmdArc:
			av_push(av, newSVpv("arc", 0));
			break;
		case leCmdLine:
			av_push(av, newSVpv("line", 0));
			break;
		case leCmdConic:
			av_push(av, newSVpv("conic", 0));
			break;
		case leCmdCubic:
			av_push(av, newSVpv("cubic", 0));
			break;
		default:
			warn("panic: bad line_end #%d structure", index);
			return false;
		}

		av2 = newAV();
		av_push(av, newRV_noinc((SV*) av2));
		for ( k = 0; k < (*pc)->n_args; k++)
			av_push(av2, newSVnv((*pc)->args[k]));
	}

	return newRV_noinc((SV*) av);
}

static SV*
produce_line_ends(Handle self)
{
	int i, items, all_are_default = true;
	AV *av;

	for ( i = 1; i <= leiMax; i++)
		if ( GS.line_end[i].type != leDefault ) {
			all_are_default = false;
			break;
		}

	if ( all_are_default ) {
		if ( GS.line_end[0].type != leCustom )
			return newSViv(GS.line_end[0].type);
		else
			return produce_line_end(self, 0);
	}

	av = newAV();
	items = leiMax + 1;

	/* skip undefs in tail */
	while (items > 0 && GS.line_end[items-1].type == leDefault)
		items--;

	for ( i = 0; i < items; i++)
		av_push( av, produce_line_end(self, i));

	return newRV_noinc((SV*) av);
}

SV*
Drawable_lineEnd( Handle self, Bool set, SV *lineEnd)
{
	if (!set)
		return produce_line_ends(self);

	Drawable_line_end_refcnt( &GS, -1);
	if ( !Drawable_read_line_ends( lineEnd, &GS ))
		return NULL_SV;
	Drawable_line_end_refcnt( &GS, +1);

	return NULL_SV;
}

SV*
Drawable_lineEndIndex( Handle self, Bool set, int index, SV *lineEnd)
{
	Bool only;

	only = (index & leiOnly) ? 1 : 0;
	index &= ~leiOnly;
	if ( index > leiMax )
		return NULL_SV;
	/*
		leDefault is special, depending on the index:
		0 (line tail) cannot be leDefault, because others may reference it
		1 (line head) if leDefault, same as 0
		2 (arrow tail) if leDefault, same as 0
		3 (arrow head) if leDefault, same as 1, which if is also leDefault, then same as 0
	*/
	if (!set) {
		if ( only ) {
			while ( index != leiHeadsAndTails && GS.line_end[index].type == leDefault)
				index = (index == leiArrowHead) ? leiHeads : leiHeadsAndTails;
		}
		return produce_line_end(self, index);
	} else if ( only && index == leiHeadsAndTails  ) {
		int i;
		for ( i = 1; i <= leiMax; i++) {
			if (GS.line_end[i].type == leDefault) {
				GS.line_end[i] = GS.line_end[leiHeadsAndTails];
				line_end_refcnt( &GS, i, +1);
			}
		}
	} else if ( only && index == leiHeads && GS.line_end[leiArrowHead].type == leDefault) {
		GS.line_end[leiArrowHead] = GS.line_end[leiHeads];
		line_end_refcnt( &GS, leiArrowHead, +1);
	}

	line_end_refcnt( &GS, index, -1);
	if ( !read_line_end( lineEnd, &GS, index ))
		return NULL_SV;
	line_end_refcnt( &GS, index, +1);

	return NULL_SV;
}

int
Drawable_resolve_line_end_index( DrawablePaintState *gs, int index)
{
	int le = gs->line_end[index].type;
	while ( le == leDefault ) {
		switch ( index ) {
		case leiLineTail:
			le = leRound;
			break;
		case leiLineHead:
		case leiArrowTail:
			index = leiLineTail;
			le = gs->line_end[index].type;
			break;
		case leiArrowHead:
			index = leiLineHead;
			le = gs->line_end[index].type;
			break;
		default:;
		}
	}
	return index;
}

NRect
Drawable_line_end_box( DrawablePaintState *gs, int index)
{
	NRect r = {0,0,0,0};
	LineEnd *le = gs->line_end + index;

	switch ( le->type ) {
	case leRound:
	case leSquare:
		r.left = r.bottom = r.right = r.top = 0.5;
		break;
	case leCustom: {
		int k;
		PPath p = le->path;
		for ( k = 0; k < p->n_commands; k++) {
			int l;
			PPathCommand pc = p->commands[k];
			double *pts = pc->args;
			for ( l = 0; l < pc->n_args; l++) {
				if ( *pts < r.left )
					r.left = *pts;
				if ( *pts > r.right )
					r.right = *pts;
				pts++;
				if ( *pts < r.bottom )
					r.bottom = *pts;
				if ( *pts > r.top )
					r.top = *pts;
				pts++;
			}
		}
		break;
	}
	default:;
	}

	return r;
}

#ifdef __cplusplus
}
#endif
