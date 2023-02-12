/************************/
/*                      */
/*    XRM database      */
/*                      */
/************************/

#include <apricot.h>
#include "unix/guts.h"
#define X_COLOR_TO_RGB(xc)     (ARGB(((xc).red>>8),((xc).green>>8),((xc).blue>>8)))

static XrmQuark
get_class_quark( const char *name)
{
	XrmQuark quark;
	char *s, *t;

	t = s = prima_normalize_resource_string( duplicate_string( name), true);
	if ( t && *t == 'P' && strncmp( t, "Prima__", 7) == 0)
		s = t + 7;
	if ( s && *s == 'A' && strcmp( s, "Application") == 0)
		strcpy( s, "Prima"); /* we have enough space */
	quark = XrmStringToQuark( s);
	free( t);
	return quark;
}

static XrmQuark
get_instance_quark( const char *name)
{
	XrmQuark quark;
	char *s;

	s = duplicate_string( name);
	quark = XrmStringToQuark( prima_normalize_resource_string( s, false));
	free( s);
	return quark;
}

Bool
prima_update_quarks_cache( Handle self)
{
	PComponent me = PComponent( self);
	XrmQuark qClass, qInstance;
	int n;
	DEFXX;
	PDrawableSysData UU;

	if (!XX)
		return false;

	qClass = get_class_quark( self == prima_guts.application ? "Prima" : me-> self-> className);
	qInstance = get_instance_quark( me-> name ? me-> name : "noname");

	free( XX-> q_class_name); XX-> q_class_name = NULL;
	free( XX-> q_instance_name); XX-> q_instance_name = NULL;

	if ( me-> owner && me-> owner != self && PComponent(me-> owner)-> sysData && X(PComponent( me-> owner))-> q_class_name) {
		UU = X(PComponent( me-> owner));
		XX-> n_class_name = n = UU-> n_class_name + 1;
		if (( XX-> q_class_name = malloc( sizeof( XrmQuark) * (n + 3))))
			memcpy( XX-> q_class_name, UU-> q_class_name, sizeof( XrmQuark) * n);
		XX-> q_class_name[n-1] = qClass;
		XX-> n_instance_name = n = UU-> n_instance_name + 1;
		if (( XX-> q_instance_name = malloc( sizeof( XrmQuark) * (n + 3))))
			memcpy( XX-> q_instance_name, UU-> q_instance_name, sizeof( XrmQuark) * n);
		XX-> q_instance_name[n-1] = qInstance;
	} else {
		XX-> n_class_name = n = 1;
		if (( XX-> q_class_name = malloc( sizeof( XrmQuark) * (n + 3))))
			XX-> q_class_name[n-1] = qClass;
		XX-> n_instance_name = n = 1;
		if (( XX-> q_instance_name = malloc( sizeof( XrmQuark) * (n + 3))))
			XX-> q_instance_name[n-1] = qInstance;
	}
	return true;
}

int
prima_rm_get_int( Handle self, XrmQuark class_detail, XrmQuark name_detail, int default_value)
{
	DEFXX;
	XrmRepresentation type;
	XrmValue value;
	long int r;
	char *end;

	if ( XX && guts.db && XX-> q_class_name && XX-> q_instance_name) {
		XX-> q_class_name[XX-> n_class_name] = class_detail;
		XX-> q_class_name[XX-> n_class_name + 1] = 0;
		XX-> q_instance_name[XX-> n_instance_name] = name_detail;
		XX-> q_instance_name[XX-> n_instance_name + 1] = 0;
		if ( XrmQGetResource( guts.db,
									XX-> q_instance_name,
									XX-> q_class_name,
									&type, &value)) {
			if ( type == guts.qString) {
				r = strtol((char *)value. addr, &end, 0);
				if (*(value. addr) && !*end)
					return (int)r;
			}
		}
	}
	return default_value;
}

Bool
apc_fetch_resource( const char *className, const char *name,
						const char *resClass, const char *res,
						Handle owner, int resType,
						void *result)
{
	PDrawableSysData XX;
	XrmQuark *classes, *instances, backup_classes[3], backup_instances[3];
	XrmRepresentation type;
	XrmValue value;
	int nc, ni;
	char *s;
	XColor clr;

	if ( owner == NULL_HANDLE) {
		classes           = backup_classes;
		instances         = backup_instances;
		nc = ni = 0;
	} else {
		if (!prima_update_quarks_cache( owner)) return false;
		XX                   = X(owner);
		if (!XX) return false;
		classes              = XX-> q_class_name;
		instances            = XX-> q_instance_name;
		if ( classes == NULL || instances == NULL) return false;
		nc                   = XX-> n_class_name;
		ni                   = XX-> n_instance_name;
	}
	classes[nc++]        = get_class_quark( className);
	instances[ni++]      = get_instance_quark( name);
	classes[nc++]        = get_class_quark( resClass);
	instances[ni++]      = get_instance_quark( res);
	classes[nc]          = 0;
	instances[ni]        = 0;

	if (guts. debug & DEBUG_XRDB) {
		int i;
		_debug( "misc: inst: ");
		for ( i = 0; i < ni; i++) {
			_debug( "%s ", XrmQuarkToString( instances[i]));
		}
		_debug( "\nmisc: class: ");
		for ( i = 0; i < nc; i++) {
			_debug( "%s ", XrmQuarkToString( classes[i]));
		}
		_debug( "\n");
	}

	if ( XrmQGetResource( guts.db,
				instances,
				classes,
				&type, &value)) {
		if ( type == guts.qString) {
			s = (char *)value.addr;
			Xdebug("found %s\n", s);
			switch ( resType) {
			case frString:
				*((char**)result) = duplicate_string( s);
				break;
			case frColor:
				if (!XParseColor( DISP, DefaultColormap( DISP, SCREEN), s, &clr))
					return false;
				*((Color*)result) = X_COLOR_TO_RGB(clr);
				Xdebug("color: %06x\n", *((Color*)result));
				break;
			case frFont:
				prima_font_pp2font( s, ( Font *) result);
#define DEBUG_FONT(font) font.height,font.width,font.size,font.name,font.encoding
				Xdebug("font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT((*(( Font *) result))));
				break;
			case frUnix_int:
				*((int*)result) = atoi( s);
				Xdebug("int: %d\n", *((int*)result));
				break;
			default:
				return false;
			}
			return true;
		}
	}

	return false;
}

Color
apc_lookup_color( const char * colorName)
{
	char buf[ 256];
	char *b;
	int len;
	XColor clr;

	if ( DISP && XParseColor( DISP, DefaultColormap( DISP, SCREEN), colorName, &clr))
		return X_COLOR_TO_RGB(clr);

#define xcmp( name, stlen, retval)  if (( len == stlen) && ( strcmp( name, buf) == 0)) return retval

	strlcpy( buf, colorName, 255);
	len = strlen( buf);
	for ( b = buf; *b; b++) *b = tolower(*b);

	switch( buf[0]) {
	case 'a':
		xcmp( "aqua", 4, 0x00FFFF);
		xcmp( "azure", 5, ARGB(240,255,255));
		break;
	case 'b':
		xcmp( "black", 5, 0x000000);
		xcmp( "blanchedalmond", 14, ARGB( 255,235,205));
		xcmp( "blue", 4, 0x000080);
		xcmp( "brown", 5, 0x808000);
		xcmp( "beige", 5, ARGB(245,245,220));
		break;
	case 'c':
		xcmp( "cyan", 4, 0x008080);
		xcmp( "chocolate", 9, ARGB(210,105,30));
		break;
	case 'd':
		xcmp( "darkgray", 8, 0x404040);
		break;
	case 'e':
		break;
	case 'f':
		xcmp( "fuchsia", 7, 0xFF00FF);
		break;
	case 'g':
		xcmp( "green", 5, 0x008000);
		xcmp( "gray", 4, 0x808080);
		xcmp( "gray80", 6, ARGB(204,204,204));
		xcmp( "gold", 4, ARGB(255,215,0));
		break;
	case 'h':
		xcmp( "hotpink", 7, ARGB(255,105,180));
		break;
	case 'i':
		xcmp( "ivory", 5, ARGB(255,255,240));
		break;
	case 'j':
		break;
	case 'k':
		xcmp( "khaki", 5, ARGB(240,230,140));
		break;
	case 'l':
		xcmp( "lime", 4, 0x00FF00);
		xcmp( "lightgray", 9, 0xC0C0C0);
		xcmp( "lightblue", 9, 0x0000FF);
		xcmp( "lightgreen", 10, 0x00FF00);
		xcmp( "lightcyan", 9, 0x00FFFF);
		xcmp( "lightmagenta", 12, 0xFF00FF);
		xcmp( "lightred", 8, 0xFF0000);
		xcmp( "lemon", 5, ARGB(255,250,205));
		break;
	case 'm':
		xcmp( "maroon", 6, 0x800000);
		xcmp( "magenta", 7, 0x800080);
		break;
	case 'n':
		xcmp( "navy", 4, 0x000080);
		break;
	case 'o':
		xcmp( "olive", 5, 0x808000);
		xcmp( "orange", 6, ARGB(255,165,0));
		break;
	case 'p':
		xcmp( "purple", 6, 0x800080);
		xcmp( "peach", 5, ARGB(255,218,185));
		xcmp( "peru", 4, ARGB(205,133,63));
		xcmp( "pink", 4, ARGB(255,192,203));
		xcmp( "plum", 4, ARGB(221,160,221));
		break;
	case 'q':
		break;
	case 'r':
		xcmp( "red", 3, 0x800000);
		xcmp( "royalblue", 9, ARGB(65,105,225));
		break;
	case 's':
		xcmp( "silver", 6, 0xC0C0C0);
		xcmp( "sienna", 6, ARGB(160,82,45));
		break;
	case 't':
		xcmp( "teal", 4, 0x008080);
		xcmp( "turquoise", 9, ARGB(64,224,208));
		xcmp( "tan", 3, ARGB(210,180,140));
		xcmp( "tomato", 6, ARGB(255,99,71));
		break;
	case 'u':
		break;
	case 'w':
		xcmp( "white", 5, 0xFFFFFF);
		xcmp( "wheat", 5, ARGB(245,222,179));
		break;
	case 'v':
		xcmp( "violet", 6, ARGB(238,130,238));
		break;
	case 'x':
		break;
	case 'y':
		xcmp( "yellow", 6, 0xFFFF00);
		break;
	case 'z':
		break;
	}

#undef xcmp

	return clInvalid;
}

