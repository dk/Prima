#include "apricot.h"
#include "AbstractMenu.h"
#include "Image.h"
#include "Menu.h"
#include "Widget.h"
#include <AbstractMenu.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CComponent->
#define my  ((( PAbstractMenu) self)-> self)
#define var (( PAbstractMenu) self)

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
		return (r&kmCtrl) && isalpha((int)*key) ? r | (toupper((int)*key)-'@') : r | tolower((int)*key);
	} else {
		char *e;
		if (isdigit((int)*key)) {
			if (r) return kbNoKey;
			r = strtol( key, &e, 10);
			if (*e) return kbNoKey;
			if ( !( r & kmCtrl)) return r;
			return ( isalpha( r & kbCharMask)) ?
				( r & kbModMask) | ( toupper( r & kbCharMask)-'@') :
				r;
		} else if (tolower(*key) != 'f')
			return kbNoKey;
		key++;
		r1 = strtol( key, &e, 10);
		if (*e || r1 < 1 || r1 > 16) return kbNoKey;
		return r | (kbF1 + ((r1-1) << 8));
	}
}

static int
is_var_id_name( char * name)
{
	int ret;
	char * e;
	if ( !name || *(name++) != '#') return 0;
	ret = strtol( name, &e, 10);
	if ( *e || ret < 0) return 0;
	return ret;
}

void
AbstractMenu_dispose_menu( Handle self, void * menu)
{
	PMenuItemReg m = ( PMenuItemReg) menu;
	if  ( m == nil) return;
	free( m-> text);
	free( m-> accel);
	free( m-> variable);
	free( m-> perlSub);
	if ( m-> code) sv_free( m-> code);
	if ( m-> options) sv_free( m-> options);
	if ( m-> bitmap) {
		if ( PObject( m-> bitmap)-> stage < csDead)
			SvREFCNT_dec( SvRV(( PObject( m-> bitmap))-> mate));
		unprotect_object( m-> bitmap);
	}
	if ( m-> icon) {
		if ( PObject( m-> icon)-> stage < csDead)
			SvREFCNT_dec( SvRV(( PObject( m-> icon))-> mate));
		unprotect_object( m-> icon);
	}
	my-> dispose_menu( self, m-> next);
	my-> dispose_menu( self, m-> down);
	free( m);
}


static Bool
register_image(Handle c_object)
{
	if (
		( c_object == nilHandle) || !( kind_of( c_object, CImage)) ||
		((( PImage) c_object)-> w == 0) || ((( PImage) c_object)-> h == 0)
	) {
		warn("menu build error: invalid image passed");
		return false;
	}
	protect_object( c_object);
	SvREFCNT_inc( SvRV(( PObject( c_object))-> mate));
	return true;
}

typedef struct {
	int max_group, curr_group;
	int max_group_initialized;
} AVTraverse;

static Bool
avt_init( Handle self, PMenuItemReg m, AVTraverse * avt)
{
	if ( m-> group > avt-> max_group ) {
		if ( m-> group < INT_MAX)
			avt-> max_group = m-> group;
		else {
			avt-> max_group = -1;
			warn("Cannot deduce next group id - don't use large numbers for it");
			return true;
		}
	}
	return false;
}

void *
AbstractMenu_new_menu( Handle self, SV * sv, int level, void * _avt)
{
	AV * av;
	int i, count;
	int n;
	PMenuItemReg m    = nil;
	PMenuItemReg curr = nil;
	Bool rightAdjust = false;
	AVTraverse avt_dummy, *avt;

	if ( level == 0)
	{
		if ( SvTYPE( sv) == SVt_NULL) return nil; /* null menu */
	}

	if ( !SvROK( sv) || ( SvTYPE( SvRV( sv)) != SVt_PVAV)) {
		warn("menu build error: menu is not an array");
		return nil;
	}
	av = (AV *) SvRV( sv);
	n = av_len( av);

	if ( n == -1) {
		if ( level == 0) return nil; /* null menu */
		warn("menu build error: empty array passed");
		return nil;
	}

	if ( _avt == NULL ) {
		avt = &avt_dummy;
		bzero(avt, sizeof(AVTraverse));
	} else
		avt = _avt;

	/* cycling the list of items */
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
		int l_options  = -1;
		Bool last_item_is_a_hash;
		Bool stop_group = false;

		if ( itemHolder == nil)
		{
			warn("menu build error: array panic");
			my-> dispose_menu( self, m);
			return nil;
		}
		if ( !SvROK( *itemHolder) || ( SvTYPE( SvRV( *itemHolder)) != SVt_PVAV)) {
			warn("menu build error: submenu is not an array");
			my-> dispose_menu( self, m);
			return nil;
		}
		/* entering item description */
		item = ( AV *) SvRV( *itemHolder);
		count = av_len( item) + 1;
		if ( count > 6) {
			warn("menu build error: extra declaration");
			count = 5;
		}
		if ( !( r = alloc1z( MenuItemReg))) {
			warn( "Not enough memory");
			my-> dispose_menu( self, m);
			return nil;
		}
		r-> key = kbNoKey;

		holder = av_fetch( item, count - 1, 0);
		last_item_is_a_hash = holder && SvOK(*holder) && SvROK(*holder) && SvTYPE(SvRV(*holder)) == SVt_PVHV;

		if ( count < 2) {          /* empty or 1 means line divisor, no matter of text */
			r-> flags. divider = true;
			rightAdjust = (( level == 0) && ( var-> anchored));
			if ( count == 1)
				l_var = 0;
			else /* auto-grouping ends with a divider */
				stop_group = true;
		} else if ( count == 2) {
			if ( last_item_is_a_hash ) {
				l_var  = 0;
				l_options = 1;
			} else {
				l_text = 0;
				l_sub  = 1;
			}
		} else if ( count == 3) {
			if ( last_item_is_a_hash ) {
				l_text = 0;
				l_sub  = 1;
				l_options = 2;
			} else {
				l_var  = 0;
				l_text = 1;
				l_sub  = 2;
			}
		} else if ( count == 4) {
			if ( last_item_is_a_hash ) {
				l_var  = 0;
				l_text = 1;
				l_sub  = 2;
				l_options = 3;
			} else {
				l_text  = 0;
				l_accel = 1;
				l_key   = 2;
				l_sub   = 3;
			}
		} else if ( count == 5) {
			l_var   = 0;
			l_text  = 1;
			l_accel = 2;
			l_key   = 3;
			l_sub   = 4;
		} else {
			l_var   = 0;
			l_text  = 1;
			l_accel = 2;
			l_key   = 3;
			l_sub   = 4;
			l_options  = 5;
		}

		if ( m) curr = curr-> next = r; else curr = m = r; /* adding to list */

		r-> flags. rightAdjust = rightAdjust ? 1 : 0;
		r-> id = ++(var-> autoEnum);

#define a_get( l_, fl_, num) \
	if ( num >= 0 ) {                                                     \
		holder = av_fetch( item, num, 0);                             \
		if ( holder) {                                                \
			if ( SvTYPE(*holder) != SVt_NULL) {                   \
				l_ = duplicate_string( SvPV_nolen( *holder)); \
				fl_ = prima_is_utf8_sv(*holder);              \
			}                                                     \
		} else {                                                      \
			warn("menu build error: array panic");                \
			my-> dispose_menu( self, m);                          \
			return nil;                                           \
		}                                                             \
	}
		a_get( r-> accel   , r-> flags. utf8_accel,    l_accel);
		a_get( r-> variable, r-> flags. utf8_variable, l_var);
		if ( l_key >= 0) {
			holder = av_fetch( item, l_key, 0);
			if ( !holder) {
				warn("menu build error: array panic");
				my-> dispose_menu( self, m);
				return nil;
			}
			r-> key = key_normalize( SvPV_nolen( *holder));
		}

		if ( r-> variable)
		{
			#define s r-> variable
			#define PARSE(name)                  \
				if ( r-> flags. divider )    \
					warn("warning: " #name " flag ignored on a divider menu");\
				else                         \
					r-> flags. name = 1; \
				decr++;                      \
				break;
			int i, decr = 0;
			for ( i = 0; ; i++) {
				switch ( s[i]) {
				case '-': PARSE(disabled);
				case '*': PARSE(checked);
				case '@': PARSE(autotoggle);
				case '?': PARSE(custom_draw);
				case '(':
					if ( !avt-> max_group_initialized ) {
						my-> first_that( self, (void*) &avt_init, avt, true);
						avt-> max_group_initialized = true;
					}
					if ( r-> flags. divider )
						/* variable name hinted , continue */
						stop_group = false;
					else if ( avt-> curr_group != 0) {
						/* that's okay, reuse the last group */
					} else if ( avt-> max_group >= 0 )
						avt-> curr_group = ++avt-> max_group;
					decr++;
					break;
				case ')':
					if ( avt-> curr_group == 0 ) {
						warn("group closing outside parantheses, ignoring");
					} else {
						r-> group = avt->curr_group;
						avt-> curr_group = 0;
					}
					decr++;
					break;
				default : goto STOP;
				}
			}
			STOP:
			if ( decr) memmove( s, s + decr, strlen( s) + 1 - decr);
			if ( strlen( s) == 0 || is_var_id_name( s) != 0) {
				free( s);
				s = nil;
			}
			#undef s
		}

		if ( stop_group )
			avt-> curr_group = 0;
		if ( avt-> curr_group != 0 && !r-> flags. divider)
			r-> group = avt-> curr_group;

		/* parsing text */
		if ( l_text >= 0)
		{
			holder = av_fetch( item, l_text, 0);
			if ( !holder) {
				warn("menu build error: array panic");
				my-> dispose_menu( self, m);
				return nil;
			}
			subItem = *holder;

			if ( SvROK( subItem)) {
				Handle c_object = gimme_the_mate( subItem);
				if ( !register_image(c_object))
					goto TEXT;
				r-> bitmap = c_object;
			} else {
			TEXT:
				r-> text = duplicate_string( SvPV_nolen( subItem));
				r-> flags. utf8_text = prima_is_utf8_sv( subItem);
			}
		}

		/* parsing sub */
		if ( l_sub >= 0)
		{
			holder = av_fetch( item, l_sub, 0);
			if ( !holder) {
				warn("menu build error: array panic");
				my-> dispose_menu( self, m);
				return nil;
			}
			subItem = *holder;

			if ( SvROK( subItem)) {
				if ( SvTYPE( SvRV( subItem)) == SVt_PVCV) {
					r-> code = newSVsv( subItem);
				} else {
					int save;

					save = avt-> curr_group;
					avt-> curr_group = 0;
					r-> down = ( PMenuItemReg) my-> new_menu( self, subItem, level + 1, avt);
					avt-> curr_group = save;

					if ( r-> down == nil) {
						/* seems error was occured inside this call */
						my-> dispose_menu( self, m);
						return nil;
					}
				}
			} else {
				if ( SvPOK( subItem)) {
					r-> perlSub = duplicate_string( SvPV_nolen( subItem));
					r-> flags. utf8_perlSub = prima_is_utf8_sv( subItem);
				} else {
					warn("menu build error: invalid sub name passed");
				}
			}
		}

		/* parsing options */
		if ( l_options >= 0)
		{
			holder = av_fetch( item, l_options, 0);
			if ( !holder) {
				warn("menu build error: array panic");
				my-> dispose_menu( self, m);
				return nil;
			}
			if (!( SvOK( *holder) && SvROK( *holder) && SvTYPE( SvRV( *holder)) == SVt_PVHV)) {
				warn("menu build error: options is not a hashref");
			} else {
				dPROFILE;
				HV * profile = (HV*) SvRV(*holder);
				if ( pexist( icon )) {
					Handle c_object = pget_H(icon);
					if ( register_image(c_object))
						r-> icon = c_object;
					pdelete(icon);
				}
				if ( pexist( group )) {
					r-> group = pget_i(group);
					pdelete(group);
				}
			}
			r-> options = newSVsv( *holder);
		}
	}
	return m;
}

void
AbstractMenu_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited init( self, profile);
	var-> anchored = kind_of( self, CMenu);
	my-> update_sys_handle( self, profile);
	my-> set_items( self, pget_sv( items));
	if ( var-> system) apc_menu_update( self, nil, var-> tree);
	if ( pget_B( selected)) my-> set_selected( self, true);
	CORE_INIT_TRANSIENT(AbstractMenu);
}

void
AbstractMenu_done( Handle self)
{
	if ( var-> system) apc_menu_destroy( self);
	my-> dispose_menu( self, var-> tree);
	var-> tree = nil;
	inherited done( self);
}

Bool
AbstractMenu_validate_owner( Handle self, Handle * owner, HV * profile)
{
	dPROFILE;
	*owner = pget_H( owner);
	if ( !*owner ) return !var-> system;
	if ( !kind_of( *owner, CWidget)) return false;
	return inherited validate_owner( self, owner, profile);
}

void
AbstractMenu_cleanup( Handle self)
{
	if ( my-> get_selected( self)) my-> set_selected( self, false);
	inherited cleanup( self);
}

void
AbstractMenu_set( Handle self, HV * profile)
{
	dPROFILE;
	Bool select = false;
	if ( pexist( owner)) {
		select = pexist( selected) ? pget_B( selected) : my-> get_selected( self);
		pdelete( selected);
	}
	inherited set( self, profile);
	if ( select) my-> set_selected( self, true);
}

static SV *
new_av(  PMenuItemReg m, int level, Bool fullTree);

static SV *
new_av_entry(  PMenuItemReg m, int level, Bool fullTree)
{
	AV * loc = newAV();
	if ( !m-> flags. divider) {
		if ( m-> variable) { /* has name */
			SV * sv;
			int shift =
				( m-> flags. checked     ? 1 : 0) +
				( m-> flags. disabled    ? 1 : 0) +
				( m-> flags. autotoggle  ? 1 : 0) +
				( m-> flags. custom_draw ? 1 : 0)
				;
			if ( shift > 0) { /* has flags */
				int len = (int) strlen( m-> variable);
				char * name = allocs( len + shift);
				if ( name) {
					int slen = len + shift;
					memcpy( name + shift, m-> variable, len);
					if ( m-> flags. disabled)   name[ --shift] = '-'; 
					if ( m-> flags. checked)    name[ --shift] = '*';
					if ( m-> flags. autotoggle) name[ --shift] = '@';
					if ( m-> flags. custom_draw)name[ --shift] = '?';
					sv = newSVpv( name, slen);
					free(name);
				} else
					sv = newSVpv( m-> variable, len);
			} else /* has name but no flags */
				sv = newSVpv( m-> variable, 0);

			if ( m-> flags. utf8_variable)
				SvUTF8_on( sv);
			av_push( loc, sv);
		} else { /* has flags but no name - autogenerate */
			int len;
			char buffer[20];
			len = sprintf( buffer, "%s%s%s%s#%d",
				m-> flags. disabled   ? "-" : "",
				m-> flags. checked    ? "*" : "",
				m-> flags. autotoggle ? "@" : "",
				m-> flags. custom_draw ? "?" : "",
				m-> id);
			av_push( loc, newSVpv( buffer, ( STRLEN) len));
		}

		if ( m-> bitmap) {
			if ( PObject( m-> bitmap)-> stage < csDead)
				av_push( loc, newRV( SvRV((( PObject)( m-> bitmap))-> mate)));
			else
				av_push( loc, newSVpv( "", 0));
		} else {
			SV * sv = newSVpv( m-> text, 0);
			if ( m-> flags. utf8_text) SvUTF8_on( sv);
			av_push( loc, sv);
		}

		if ( m-> accel) {
			SV * sv = newSVpv( m-> accel, 0);
			av_push( loc, sv);
			if ( m-> flags. utf8_accel) SvUTF8_on( sv);
		} else {
			av_push( loc, newSVpv( "", 0));
		}
		av_push( loc, newSViv( m-> key));

		if ( m-> down) {
			av_push( loc, fullTree ? 
				new_av( m-> down, level + 1, true) :
				newRV_noinc(( SV *) newAV())
			);
		} else if ( m-> code) {
			av_push( loc, newSVsv( m-> code));
		} else if ( m-> perlSub) {
			SV * sv = newSVpv( m-> perlSub, 0);
			if ( m-> flags. utf8_perlSub) SvUTF8_on( sv);
			av_push( loc, sv);
		} else {
			av_push( loc, newSVpv( "", 0));
		}

		if ( m-> options)
			av_push( loc, newSVsv(m->options));
	} else {
		/* divider */
		if ( m-> variable) {
			SV * sv = newSVpv( m-> variable, 0);
			if ( m-> flags. utf8_perlSub) SvUTF8_on( sv);
			av_push( loc, sv);
		} else {
			int len;
			char buffer[20];
			len = sprintf( buffer, "#%d", m-> id);
			av_push( loc, newSVpv( buffer, ( STRLEN) len));
		}
	}

	return newRV_noinc(( SV *) loc);
}

static SV *
new_av(  PMenuItemReg m, int level, Bool fullTree)
{
	AV * glo;
	if ( m == nil) return nilSV;
	glo = newAV();
	while ( m)
	{
		av_push( glo, new_av_entry(m, level, fullTree));
		m = m-> next;
	}
	return newRV_noinc(( SV *) glo);
}

static Bool
var_match( Handle self, PMenuItemReg m, void * params)
{
	if ( m-> variable == nil) return false;
	return ( strcmp( m-> variable, ( char *) params) == 0);
}

static Bool
id_match( Handle self, PMenuItemReg m, void * params)
{
	return m-> id == *(( int*) params);
}

static Bool
key_match( Handle self, PMenuItemReg m, void * params)
{
	return (( m-> key == *(( int*) params)) && ( m-> key != kbNoKey) && !m-> flags. disabled);
}

static PMenuItemReg
find_menuitem( Handle self, char * var_name, Bool match_disabled)
{
	int num;
	if ( !var_name) return nil;
	/* match special case /^#\d+$/ */
	if (( num = is_var_id_name( var_name)) != 0)
		return ( PMenuItemReg) my-> first_that( self, (void*)id_match, &num, match_disabled);
	else
		return ( PMenuItemReg) my-> first_that( self, (void*)var_match, var_name, match_disabled);
}

char *
AbstractMenu_make_var_context( Handle self, PMenuItemReg m, char * buffer)
{
	if ( !m) return "";
	if ( m-> variable)
		return m-> variable;
	sprintf( buffer, "#%d", m-> id);
	return buffer;
}

char *
AbstractMenu_make_id_context( Handle self, int id, char * buffer)
{
	return my-> make_var_context( self, my-> first_that( self, (void*)id_match, &id, true), buffer);
}

SV *
AbstractMenu_get_item( Handle self, char * varName, Bool fullTree)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilSV;
	m = find_menuitem( self, varName, true);
	if ( !m ) return nilSV;
	return new_av_entry( m, 1, fullTree);
}

SV *
AbstractMenu_get_items( Handle self, char * varName, Bool fullTree)
{
	if ( var-> stage > csFrozen) return nilSV;
	if ( strlen( varName))
	{
		PMenuItemReg m = find_menuitem( self, varName, true);
		if ( m && m-> down) {
			return fullTree ? 
				new_av( m-> down, 1, true) :
				newRV_noinc(( SV *) newAV());
		} else if ( m) {
			return newRV_noinc(( SV *) newAV());
		} else {
			return nilSV;
		}
	} else {
		return var-> tree ?
			new_av( var-> tree, 0, fullTree) :
			newRV_noinc(( SV *) newAV());
	}
}

SV *
AbstractMenu_get_children( Handle self, char * varName)
{
	PMenuItemReg m;
	AV * av;

	if ( var-> stage > csFrozen) return nilSV;
	if ( strlen( varName)) {
		if ( !( m = find_menuitem( self, varName, true)))
			return nilSV;
		m = m->down;
	} else
		m = var-> tree;
	if ( !m ) return nilSV;

	av = newAV();
	while ( m != NULL ) {
		if ( m-> variable) {
			SV * sv = newSVpv( m-> variable, 0);
			if ( m-> flags. utf8_perlSub) SvUTF8_on( sv);
			av_push( av, sv);
		} else {
			int len;
			char buffer[20];
			len = sprintf( buffer, "#%d", m-> id);
			av_push( av, newSVpv( buffer, ( STRLEN) len));
		}
		m = m-> next;
	}

	return newRV_noinc(( SV *) av);
}

static void
notify( Handle self, char * format, ...)
{
	va_list args;

	if ( var-> stage != csNormal) return;
	va_start( args, format);
	protect_object( self);
	my-> push_event( self);
	ENTER;
	SAVETMPS;
	call_perl_indirect( self, "notify", format, true, false, args);
	va_end( args);
	FREETMPS;
	LEAVE;
	my-> pop_event( self);
	unprotect_object( self);
}

void
AbstractMenu_set_items( Handle self, SV * items)
{
	PMenuItemReg oldBranch = var-> tree;
	if ( var-> stage > csFrozen) return;
	var-> tree = ( PMenuItemReg) my-> new_menu( self, items, 0, NULL);
	if ( var-> stage <= csNormal && var-> system)
		apc_menu_update( self, oldBranch, var-> tree);
	my-> dispose_menu( self, oldBranch);
	notify( self, "<sss", "Change", "items", "");
}


static PMenuItemReg
do_link( Handle self, PMenuItemReg m, PMenuProc p, void * params, Bool useDisabled)
{
	while( m)
	{
		if ( !m-> flags. disabled || useDisabled)
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
	return actionProc ? do_link( self, var-> tree, ( PMenuProc) actionProc, params, useDisabled) : nil;
}

Bool
AbstractMenu_has_item( Handle self, char * varName)
{
	return find_menuitem( self, varName, true) != nil;
}

SV *
AbstractMenu_accel( Handle self, Bool set, char * varName, SV * accel)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilSV;
	m = find_menuitem( self, varName, true);
	if ( !m) return nilSV;
	if ( !set) {
		SV * sv = newSVpv( m-> accel ? m-> accel : "", 0);
		if ( m-> flags. utf8_accel) SvUTF8_on( sv);
		return sv;
	}
	if ( m-> text == nil) return nilSV;
	free( m-> accel);
	m-> accel = nil;
	m-> accel = duplicate_string( SvPV_nolen( accel));
	m-> flags. utf8_accel = prima_is_utf8_sv( accel);

	if ( m-> id > 0) {
		if ( var-> stage <= csNormal && var-> system)
			apc_menu_item_set_accel( self, m);
		notify( self, "<ssUS", "Change", "accel", 
			m->variable ? m-> variable      : varName, 
			m->variable ? m-> flags.utf8_variable : 0, 
			accel);
	}
	return nilSV;
}

SV *
AbstractMenu_action( Handle self, Bool set, char * varName, SV * action)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilSV;
	m = find_menuitem( self, varName, true);
	if ( !m) return nilSV;
	if ( !set) {
		if ( m-> code)    return newSVsv( m-> code);
		if ( m-> perlSub) {
			SV * sv = newSVpv( m-> perlSub, 0);
			if ( m-> flags. utf8_perlSub) SvUTF8_on( sv);
			return sv;
		}
		return nilSV;
	}

	if ( m-> flags. divider || m-> down) return nilSV;
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
		m-> flags. utf8_perlSub = 0;
	} else {
		char * line = ( char *) SvPV_nolen( action);
		free( m-> perlSub);
		if ( m-> code) sv_free( m-> code);
		m-> code = nil;
		m-> perlSub = duplicate_string( line);
		m-> flags. utf8_perlSub = prima_is_utf8_sv( action);
	}
	return nilSV;
}

Bool
AbstractMenu_autoToggle( Handle self, Bool set, char * varName, Bool autotoggle)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return false;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return false;
	if ( !set)
		return m ? m-> flags. autotoggle : false;
	if ( m-> flags. divider || m-> down) return false;
	m-> flags. autotoggle = autotoggle ? 1 : 0;
	if ( m-> id > 0) {
		if ( var-> stage <= csNormal && var-> system)
			apc_menu_item_set_autotoggle( self, m);
		notify( self, "<ssUi", "Change", "autoToggle", 
			m->variable ? m-> variable      : varName, 
			m->variable ? m-> flags.utf8_variable : 0,
			autotoggle);
	}
	return autotoggle;
}

static void
set_check( Handle self, char * varName, PMenuItemReg m, Bool checked)
{
	char buffer[16];

	m-> flags. checked = checked ? 1 : 0;
	if ( var-> stage <= csNormal && var-> system)
		apc_menu_item_set_check( self, m);
	if ( varName == NULL )
		varName = AbstractMenu_make_var_context( self, m, buffer);
	notify( self, "<ssUi", "Change", "checked",
		varName,
		m->variable ? m-> flags.utf8_variable : 0,
		checked);
}

static Bool
update_group( Handle self, PMenuItemReg m, PMenuItemReg src)
{
	if ( m-> group == src->group && m != src && m->flags.checked )
		set_check(self, NULL, m, 0);
	return false;
}

Bool
AbstractMenu_checked( Handle self, Bool set, char * varName, Bool checked)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return false;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return false;
	if ( !set)
		return m ? m-> flags. checked : false;
	if ( m-> flags. divider || m-> down) return false;
	if ( m-> id > 0 ) {
		set_check(self, varName, m, checked);
		if ( checked && m-> group != 0 )
			my-> first_that( self, (void*)update_group, m, true);
	}
	return checked;
}

int
AbstractMenu_group( Handle self, Bool set, char * varName, int group)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return 0;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return 0;
	if ( !set) return m-> group;
	if ( m-> group == group ) return group;
	m-> group = group;
	notify( self, "<ssUS", "Change", "group",
		m->variable ? m-> variable      : varName,
		m->variable ? m-> flags.utf8_variable : 0,
		group);
	return group;
}


Bool
AbstractMenu_enabled( Handle self, Bool set, char * varName, Bool enabled)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return false;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return false;
	if ( !set)
		return m ? !m-> flags. disabled : false;
	if (m-> flags. divider) return false;

	m-> flags. disabled = !enabled;
	if ( m-> id > 0) {
		if ( var-> stage <= csNormal && var-> system)
			apc_menu_item_set_enabled( self, m);
		notify( self, "<ssUi", "Change", "enabled", 
			m->variable ? m-> variable      : varName, 
			m->variable ? m-> flags.utf8_variable : 0,
			enabled);
	}
	return enabled;
}

Handle
AbstractMenu_icon( Handle self, Bool set, char * varName, Handle icon)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilHandle;

	m = find_menuitem( self, varName, true);
	if ( m == nil) return nilHandle;
	if ( !m-> icon) return nilHandle;
	if ( !set) {
		if ( PObject( m-> icon)-> stage == csDead) return nilHandle;
		return m-> icon;
	}
	if ( !register_image(icon))
		return nilHandle;
	if ( m-> icon ) {
		if ( PObject( m-> icon)-> stage < csDead)
			SvREFCNT_dec( SvRV(( PObject( m-> icon))-> mate));
		unprotect_object( m-> icon);
	}
	m-> icon = icon;
	if ( m-> id > 0) {
		if ( var-> stage <= csNormal && var-> system)
			apc_menu_item_set_icon( self, m);
		notify( self, "<ssUH", "Change", "icon",
			m->variable ? m-> variable      : varName,
			m->variable ? m-> flags.utf8_variable : 0,
			icon);
	}
	return nilHandle;
}

Handle
AbstractMenu_image( Handle self, Bool set, char * varName, Handle image)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilHandle;

	m = find_menuitem( self, varName, true);
	if ( m == nil) return nilHandle;
	if ( !m-> bitmap) return nilHandle;
	if ( !set) {
		if ( PObject( m-> bitmap)-> stage == csDead) return nilHandle;
		return m-> bitmap;
	}
	if ( !register_image(image))
		return nilHandle;
	if ( PObject( m-> bitmap)-> stage < csDead)
		SvREFCNT_dec( SvRV(( PObject( m-> bitmap))-> mate));
	unprotect_object( m-> bitmap);
	m-> bitmap = image;
	if ( m-> id > 0) {
		if ( var-> stage <= csNormal && var-> system)
			apc_menu_item_set_image( self, m);
		notify( self, "<ssUH", "Change", "image", 
			m->variable ? m-> variable      : varName, 
			m->variable ? m-> flags.utf8_variable : 0,
			image);
	}
	return nilHandle;
}

SV *
AbstractMenu_options( Handle self, Bool set, char * varName, SV * options)
{
	HV * profile;
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilSV;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return nilSV;
	if ( !set) {
		HV * profile;
		if ( m->options )
			return newSVsv( m-> options);
		profile = newHV();
		if ( m-> icon )       pset_H(icon,  m->icon);
		if ( m-> group != 0 ) pset_i(group, m->group);
		return newRV_noinc((SV*)profile);
	}

	if (SvTYPE( SvRV( options)) == SVt_NULL) {
		sv_free( m-> options);
		m-> options = nilSV;
	} else {
		dPROFILE;
		if (!(SvROK( options) && SvTYPE( SvRV( options)) == SVt_PVHV)) {
			warn("options is not a hashref");
			return nilSV;
		}
		sv_free( m-> options);
		m-> options = newSVsv( options);
		profile = (HV*)SvRV(options);
		if ( pexist(icon)) {
			if ( m-> flags. divider )
				warn("Cannot set icon on a divider item");
			else
				my->icon(self, true, varName, pget_H(icon));
			pdelete(icon);
		}
		if ( pexist(group)) {
			if ( m-> flags. divider )
				warn("Cannot set group on a divider item");
			else
				my->group(self, true, varName, pget_i(group));
			pdelete(icon);
		}
	}
	notify( self, "<ssUS", "Change", "options",
		m->variable ? m-> variable      : varName,
		m->variable ? m-> flags.utf8_variable : 0,
		options);
	return nilSV;
}


SV *
AbstractMenu_submenu( Handle self, Bool set, char * varName, SV * submenu)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilSV;
	if ( !set)
		return my-> get_items( self, varName, true);

	m = find_menuitem( self, varName, true);
	if ( !m || !m-> down) return nilSV;

	if ( var-> stage <= csNormal && var-> system)
		apc_menu_item_delete( self, m-> down);
	my-> dispose_menu( self, m-> down);

	m-> down = ( PMenuItemReg) my-> new_menu( self, submenu, 1, NULL);
	if ( var-> stage <= csNormal && var-> system)
		apc_menu_update( self, m-> down, m-> down);
	notify( self, "<ssU", "Change", "submenu", 
		m->variable ? m-> variable      : varName, 
		m->variable ? m-> flags.utf8_variable : 0);

	return nilSV;
}

SV *
AbstractMenu_text( Handle self, Bool set, char * varName, SV * text)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilSV;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return nilSV;
	if ( m-> text == nil) return nilSV;
	if ( !set) {
		SV * sv = newSVpv( m-> text ? m-> text : "", 0);
		if ( m-> flags. utf8_text) SvUTF8_on( sv);
		return sv;
	}
	free( m-> text);
	m-> text = nil;
	m-> text = duplicate_string( SvPV_nolen( text));
	m-> flags. utf8_accel = prima_is_utf8_sv( text);
	if ( m-> id > 0) {
		if ( var-> stage <= csNormal && var-> system)
			apc_menu_item_set_text( self, m);
		notify( self, "<ssUS", "Change", "text", 
			m->variable ? m-> variable      : varName, 
			m->variable ? m-> flags.utf8_variable : 0,
			text);
	}
	return nilSV;
}

SV *
AbstractMenu_key( Handle self, Bool set, char * varName, SV * key)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return nilSV;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return nilSV;
	if ( m-> flags. divider || m-> down) return nilSV;
	if ( !set)
		return newSViv( m-> key);

	m-> key = key_normalize( SvPV_nolen( key));
	if ( m-> id > 0) {
		if ( var-> stage <= csNormal && var-> system)
			apc_menu_item_set_key( self, m);
		notify( self, "<ssUi", "Change", "key", 
			m->variable ? m-> variable      : varName, 
			m->variable ? m-> flags.utf8_variable : 0,
			m->key);
	}
	return nilSV;
}

void
AbstractMenu_set_variable( Handle self, char * varName, SV * newName)
{
	PMenuItemReg m;
	if ( var-> stage > csFrozen) return;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return;

	notify( self, "<ssUS", "Change", "rename", 
		m->variable ? m-> variable      : varName, 
		m->variable ? m-> flags.utf8_variable : 0,
		newName);

	free( m-> variable);
	if ( SvTYPE(newName) != SVt_NULL) {
		STRLEN len;
		char * v;
		v = SvPV( newName, len);
		if ( len > 0) {
			m-> variable = duplicate_string( v);
			m-> flags. utf8_variable = prima_is_utf8_sv( newName);
			return;
		}
	}
	m-> variable = nil;
	m-> flags. utf8_variable = 0;
}

Bool
AbstractMenu_sub_call( Handle self, PMenuItemReg m)
{
	Handle owner;
	char buffer[16], *context;
	if ( m == nil) return false;

	context = AbstractMenu_make_var_context( self, m, buffer);
	if ( m-> group != 0 ) {
		if ( !m->flags.checked )
			set_check(self, context, m, 1);
		else if ( m-> flags. autotoggle )
			set_check(self, context, m, 0);
		my-> first_that( self, (void*)update_group, m, true);
	} else if ( m-> flags. autotoggle )
		set_check(self, context, m, m-> flags. checked ? 0 : 1);
	owner = var-> owner;
	if ( owner == nilHandle ) return false;
	if ( m-> code) {
		if ( m-> flags. utf8_variable) {
			SV * sv = newSVpv( context, 0);
			SvUTF8_on( sv);
			cv_call_perl((( PComponent) owner)-> mate, SvRV( m-> code), "Si", sv, m-> flags. checked);
			sv_free( sv);
		} else
			cv_call_perl((( PComponent) owner)-> mate, SvRV( m-> code), "si", context, m-> flags. checked);
		return true;
	} else if ( m-> perlSub) {
		if ( m-> flags. utf8_variable) {
			SV * sv = newSVpv( context, 0);
			SvUTF8_on( sv);
			call_perl( owner, m-> perlSub, "Si", sv, m-> flags. checked);
			sv_free( sv);
		} else
			call_perl( owner, m-> perlSub, "si", context, m-> flags. checked);
		return true;
	}
	return false;
}

Bool
AbstractMenu_sub_call_id( Handle self, int sysId)
{
	return my-> sub_call( self, ( PMenuItemReg) my-> first_that( self, (void*)id_match, &sysId, false));
}

#define keyRealize( key) \
	if ((( key & 0xFF) >= 'A') && (( key & 0xFF) <= 'z'))  \
		key = tolower( key & 0xFF) |                   \
			(( key & ( kmCtrl | kmAlt)) ?          \
			( key & ( kmCtrl | kmAlt | kmShift))   \
		: 0)

Bool
AbstractMenu_sub_call_key ( Handle self, int key)
{
	keyRealize( key);
	return my-> sub_call( self, ( PMenuItemReg) my-> first_that( self, (void*)key_match, &key, false));
}

SV *
AbstractMenu_find_item_by_key ( Handle self, int key)
{
	PMenuItemReg m;
	SV * sv;
	char buffer[16], *context;
	keyRealize( key);
	m = ( PMenuItemReg) my-> first_that( self, (void*)key_match, &key, false);
	if ( m == NULL ) return nilSV;

	context = AbstractMenu_make_var_context( self, m, buffer);
	sv = newSVpv( context, 0 );
	if ( m-> flags. utf8_variable)
		SvUTF8_on( sv);
	return sv;
}

Bool AbstractMenu_selected( Handle self, Bool set, Bool selected)
{
	return false;
}

SV *
AbstractMenu_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, var-> system ? apc_menu_get_handle( self) : self);
	return newSVpv( buf, 0);
}

int
AbstractMenu_translate_accel( Handle self, char * accel)
{
	if ( !accel) return 0;
	while ( *accel) {
		if ( *(accel++) == '~') {
			switch ( *accel) {
			case '~' :
				accel++;
				break;
			case 0:
				return 0;
			default:
				return isalnum((int)*accel) ? *accel : tolower((int)*accel);
			}
		}
	}
	return 0;
}

int
AbstractMenu_translate_key( Handle self, int code, int key, int mod)
{
	mod &= kmAlt | kmShift | kmCtrl;
	key = ( key != kbNoKey ? key : code) | mod;
	keyRealize( key);
	return key;
}

int
AbstractMenu_translate_shortcut( Handle self, char * key)
{
	return key_normalize( key);
}

static Bool up_match   ( Handle self, PMenuItemReg m, void * params) { return m-> down == params; }
static Bool prev_match ( Handle self, PMenuItemReg m, void * params) { return m-> next == params; }

void
AbstractMenu_remove( Handle self, char * varName)
{
	PMenuItemReg up, prev, m;
	if ( var-> stage > csFrozen) return;
	m = find_menuitem( self, varName, true);
	if ( m == nil) return;
	if ( var-> stage <= csNormal && var-> system)
		apc_menu_item_delete( self, m);
	up   = ( PMenuItemReg) my-> first_that( self, (void*)up_match, m, true);
	prev = ( PMenuItemReg) my-> first_that( self, (void*)prev_match, m, true);
	if ( up)   up  -> down = m-> next;
	if ( prev) prev-> next = m-> next;
	if ( m == var-> tree) var-> tree = m-> next;
	m-> next = nil;
	notify( self, "<ssU", "Change", "remove", 
			m->variable ? m-> variable      : varName, 
			m->variable ? m-> flags.utf8_variable : 0);
	my-> dispose_menu( self, m);
}

void
AbstractMenu_insert( Handle self, SV * menuItems, char * rootName, int index)
{
	int level;
	PMenuItemReg *up, m, addFirst, addLast, branch;

	if ( var-> stage > csFrozen) return;

	if ( SvTYPE( menuItems) == SVt_NULL) return;

	if ( strlen( rootName) == 0)
	{
		if ( var-> tree == nil)
		{
			var-> tree = ( PMenuItemReg) my-> new_menu( self, menuItems, 0, NULL);
			if ( var-> stage <= csNormal && var-> system)
				apc_menu_update( self, nil, var-> tree);
			notify( self, "<sss", "Change", "insert", "");
			return;
		}
		branch = m = var-> tree;
		up = &var-> tree;
		level = 0;
	} else {
		branch = m = find_menuitem( self, rootName, true);
		if ( m == nil) return;
		if ( m-> down) index = 0;
		up = &m-> down;
		m = m-> down;
		level = 1;
	}

	/* the level is 0 or 1 for the sake of rightAdjust */
	addFirst = ( PMenuItemReg) my-> new_menu( self, menuItems, level, NULL);
	if ( !addFirst) return; /* error in menuItems */

	addLast = addFirst;
	while ( addLast-> next) addLast = addLast-> next;

	if ( index == 0)
	{
		addLast-> next = *up;
		*up = addFirst;
	} else {
		int i = 1;
		while ( m-> next)
		{
			if ( i++ == index) break;
			m = m-> next;
		}
		addLast-> next = m-> next;
		m-> next = addFirst;
	}

	if ( m && m-> flags. rightAdjust) {
		while ( addFirst != addLast-> next) {
			addFirst-> flags. rightAdjust = true;
			addFirst = addFirst-> next;
		}
	}

	if ( var-> stage <= csNormal && var-> system)
		apc_menu_update( self, branch, branch);
	notify( self, "<ssU", "Change", "insert", rootName, branch->flags.utf8_variable);
}

Bool
AbstractMenu_is_custom( Handle self, char * varName)
{
	PMenuItemReg m = find_menuitem( self, varName, true);
	return m && m-> flags. custom_draw;
}

Bool
AbstractMenu_is_separator( Handle self, char * varName)
{
	PMenuItemReg m = find_menuitem( self, varName, true);
	return m && m-> flags. divider;
}

Bool
AbstractMenu_is_submenu( Handle self, char * varName)
{
	PMenuItemReg m = find_menuitem( self, varName, true);
	return m && m-> down;
}

Bool
AbstractMenu_execute( Handle self, char * varName) 
{
	PMenuItemReg m = find_menuitem( self, varName, true);
	if ( !m ) return false;
	return my->sub_call(self, m);
}

void
AbstractMenu_handle_event( Handle self, PEvent event)
{
	inherited handle_event ( self, event);
	if ( var-> stage > csNormal) return;

	if ( event-> cmd == cmMenuItemMeasure || event-> cmd == cmMenuItemPaint) {
		char buffer[16], *context;
		PMenuItemReg m;
		m = ( PMenuItemReg) my-> first_that( self, (void*)id_match, &event->gen.i, false);
		if ( m == NULL ) return;

		context = AbstractMenu_make_var_context( self, m, buffer);
		if ( event-> cmd == cmMenuItemMeasure ) {
			AV * pt = newAV();
			SV * ref = newRV_noinc((SV*)pt);
			av_push(pt, newSViv(event->gen.P.x));
			av_push(pt, newSViv(event->gen.P.y));
			my->notify( self, "<sUS", "ItemMeasure",
				context, m-> flags.utf8_variable, ref );
			if ( !prima_read_point( ref, (int*)&event->gen.P, 2, NULL))
				warn("bad size array returned from onItemMeasure");
			sv_free(ref);
		} else {
			Handle drawable = (Handle) create_object("Prima::Drawable", "");
			++SvREFCNT( SvRV((( PAnyObject) drawable)-> mate));
			PDrawable(drawable)-> w = event->gen.P.x;
			PDrawable(drawable)-> h = event->gen.P.y;
			protect_object(drawable);
			PObject(drawable)-> options. optSystemDrawable = 1;

			event-> gen.H = drawable;
			if ( apc_menu_item_begin_paint(self, event)) {
				PComponent(self)->self->notify( self, "<sUHiR", "ItemPaint",
					context, m->flags.utf8_variable, event->gen.H, event->gen.B, event->gen.R);
				apc_menu_item_end_paint(self, event);
			}

			--SvREFCNT( SvRV((( PAnyObject) drawable)-> mate));
			unprotect_object(drawable);
			Object_destroy(event->gen.H);
		}
	}
}


#ifdef __cplusplus
}
#endif

