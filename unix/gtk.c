/*********************************/
/*                               */
/*  Gtk file dialog              */
/*                               */
/*********************************/

#include "unix/guts.h"

#ifdef WITH_GTK2

#undef GT

#undef dirty

#define Window  XWindow

#ifndef WITH_GTK2_NONX11
#include <gdk/gdkx.h>
#endif
#include <gtk/gtk.h>

static int gtk_initialized = 0;

static GtkWidget *gtk_dialog           = NULL;
static char	gtk_dialog_title[256];
static char*	gtk_dialog_title_ptr   = NULL;
static Bool	gtk_select_multiple    = FALSE;
static Bool	gtk_overwrite_prompt   = FALSE;
static Bool	gtk_show_hidden_files  = FALSE;
static char	gtk_current_folder[MAXPATHLEN+1];
static char*	gtk_current_folder_ptr = NULL;
static List*	gtk_filters            = NULL;
static int	gtk_filter_index       = 0;

static GdkDisplay * display = NULL;

static Color 
gdk_color(GdkColor * c)
{
		return ((c->red >> 8) << 16) | ((c->green >> 8) << 8) | (c->blue >> 8);
}

typedef struct {
		GType (*func)(void);
		char * name;
		char * gtk_class;
		int    prima_class;
		Font * prima_font;
} GTFStruct;

#define GT(x) gtk_##x##_get_type, #x

static GType gtf_type_null(void) { return G_TYPE_NONE; }

static GTFStruct widget_types[] = {
		{ GT(button),       "GtkButton",         wcButton      , NULL },  
		{ GT(check_button), "GtkCheckButton",    wcCheckBox    , NULL },  
		{ GT(combo_box),    "GtkCombo",          wcCombo       , NULL },  
		{ GT(dialog),       "GtkDialog",         wcDialog      , NULL },  
		{ GT(entry),        "GtkEditable",       wcEdit        , NULL },  
		{ GT(entry),        "GtkEntry",          wcInputLine   , NULL },  
		{ GT(label),        "GtkLabel",          wcLabel       , &guts. default_msg_font },  
		{ GT(list),         "GtkList",           wcListBox     , NULL },  
		{ GT(menu),         "GtkMenuItem",       wcMenu        , &guts. default_menu_font },  
		{ GT(menu_item),    "GtkMenuItem",       wcPopup       , NULL },  
		{ GT(check_button), "GtkRadioButton",    wcRadio       , NULL },  
		{ GT(scrollbar),    "GtkScrollBar",      wcScrollBar   , NULL },  
		{ GT(ruler),        "GtkRuler",          wcSlider      , NULL },  
		{ GT(widget),       "GtkWidget",         wcWidget      , &guts. default_widget_font },
		{ GT(window),       "GtkWindow",         wcWindow      , &guts. default_caption_font },  
		{ GT(widget),       "GtkWidget",         wcApplication , &guts. default_font },  
};
#undef GT

Display*
prima_gtk_init(void)
{
	int i, argc = 0;
	Display *ret;
	GtkSettings * settings;
	Color ** stdcolors;
	PangoWeight weight;
	PangoStyle style;

	switch ( gtk_initialized) {
	case -1:
		return NULL;
	case 1:
#ifdef WITH_GTK2_NONX11
		{
		}
		return (void*)1;
#else
		return gdk_x11_display_get_xdisplay(display);
#endif
	}

#ifdef WITH_GTK2_NONX11
	{
		char * display_str = getenv("DISPLAY");
		if ( display_str ) {
			struct stat s;
			if ((stat( display_str, &s) < 0) || !S_ISSOCK(s.st_mode))  /* not a socket */
				return (void*)0;
		}
	}
#endif

#if PERL_REVISION == 5 && PERL_VERSION == 20
/* perl bug in 5.20.0, see more at https://rt.perl.org/Ticket/Display.html?id=122105 */
	gtk_disable_setlocale();
#endif
	if ( !gtk_parse_args (&argc, NULL) || (display = gdk_display_open_default_libgtk_only()) == NULL) {
		gtk_initialized = -1;
		return false;
	} else {
		gtk_initialized = 1;
		XSetErrorHandler( guts.main_error_handler );
#ifdef WITH_GTK2_NONX11
		ret = (void*)1;
#else
		ret = gdk_x11_display_get_xdisplay(display);
#endif
	}

	settings  = gtk_settings_get_default();
	stdcolors = prima_standard_colors();
	for ( i = 0; i < sizeof(widget_types)/sizeof(GTFStruct); i++) {
		GTFStruct * s = widget_types + i;
		Color     * c = stdcolors[ s-> prima_class >> 16 ]; 
		Font      * f = s->prima_font;
		GtkStyle  * t = gtk_rc_get_style_by_paths(settings, NULL, s->gtk_class, s->func());
		int selected  = ( 
			s-> prima_class == wcRadio || 
			s->prima_class == wcCheckBox || 
			s->prima_class == wcButton
		) ? GTK_STATE_ACTIVE : GTK_STATE_SELECTED;
		if ( t == NULL ) {
			Pdebug("cannot query gtk style for %s\n", s->name);
			t = gtk_rc_get_style_by_paths(settings, NULL, NULL, GTK_TYPE_WIDGET);
			if ( !t ) continue;
		}
		c[ciFore]         = gdk_color( t-> fg + GTK_STATE_NORMAL );
		c[ciBack]         = gdk_color( t-> bg + GTK_STATE_NORMAL );
		c[ciHiliteText]   = gdk_color( t-> fg + selected );
		c[ciHilite]       = gdk_color( t-> bg + selected );
		c[ciDisabledText] = gdk_color( t-> fg + GTK_STATE_INSENSITIVE );
		c[ciDisabled]     = gdk_color( t-> bg + GTK_STATE_INSENSITIVE );
		/*
		c[ciLight3DColor] = gdk_color( t-> light + GTK_STATE_NORMAL );
		c[ciDark3DColor]  = gdk_color( t-> dark  + GTK_STATE_NORMAL );
		*/
		Pdebug("gtk-color: %s %06x %06x %06x %06x %06x\n", s->name, c[0], c[1], c[2], c[3], c[4], c[5]);

		if ( !f) continue;
		bzero(f, sizeof(Font));
		strncpy( f->name, pango_font_description_get_family(t->font_desc), 256);
		/* does gnome ignore X resolution? */
		f-> size = pango_font_description_get_size(t->font_desc) / PANGO_SCALE * (96.0 / guts. resolution. y);
		weight = pango_font_description_get_weight(t->font_desc);
		if ( weight <= PANGO_WEIGHT_LIGHT ) f-> style |= fsThin;
		if ( weight >= PANGO_WEIGHT_BOLD  ) f-> style |= fsBold;
		if ( pango_font_description_get_style(t->font_desc) == PANGO_STYLE_ITALIC)
			f-> style |= fsItalic;
		strcpy( f->encoding, "Default" );
		f-> width = f-> height = f->pitch = C_NUMERIC_UNDEF;
		apc_font_pick( application, f, f);
#define DEBUG_FONT(font) f->height,f->width,f->size,f->name,f->encoding
		Fdebug("gtk-font (%s): %d.[w=%d,s=%d].%s.%s\n", s->name, DEBUG_FONT(f));
	}

	return ret;
}

Bool
prima_gtk_done(void)
{
	if ( gtk_filters) {
		int i;
		for ( i = 0; i < gtk_filters-> count; i++) 
			g_object_unref(( GObject*) gtk_filters-> items[i]);
		plist_destroy( gtk_filters);
		gtk_filters = NULL;
	}
	gtk_initialized = 0;
	return true;
}

#ifndef WITH_GTK2_NONX11
static void
set_transient_for(void)
{
	static GdkWindow * gdk_toplevel = NULL;
	Handle toplevel = prima_find_toplevel_window(nilHandle);
	if ( toplevel ) {
		GdkWindow * g = NULL;

#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 14
		g = gtk_widget_get_window(GTK_WIDGET(gtk_dialog));
#else
		g = gtk_dialog->window;
#endif
		if ( g ) {
			Window w = gdk_x11_drawable_get_xid(g);
			if ( w )
				XSetTransientForHint( DISP, w, PWidget(toplevel)-> handle);
		}
	}
}
#endif


static gboolean
do_events(gpointer data)
{
	int* stage = ( int*) data;
	if ( gtk_dialog != NULL && !*stage ) {
		*stage = 1;
#ifdef WITH_GTK2_NONX11
		gtk_window_present(GTK_WINDOW(gtk_dialog));
#else
		set_transient_for();
#endif
	}
	prima_one_loop_round( WAIT_NEVER, true);
	return gtk_dialog != NULL;
}

static char *
gtk_openfile( Bool open)
{
	char *result = NULL;
	struct MsgDlg message_dlg, **storage;
	int stage = 0;

	if ( gtk_dialog) return NULL; /* we're not reentrant */

	gtk_dialog = gtk_file_chooser_dialog_new (
		gtk_dialog_title_ptr ? 
			gtk_dialog_title_ptr :
			( open ? "Open File" : "Save File"),
		NULL,
		open ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
#ifdef WITH_GTK2_NONX11
	gtk_window_set_position( GTK_WINDOW(gtk_dialog), GTK_WIN_POS_CENTER);
#endif

	gtk_file_chooser_set_local_only( GTK_FILE_CHOOSER (gtk_dialog), TRUE);
	if (open)
		gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER (gtk_dialog), gtk_select_multiple);

#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 8
	gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER (gtk_dialog), gtk_overwrite_prompt);
	gtk_file_chooser_set_show_hidden( GTK_FILE_CHOOSER (gtk_dialog), gtk_show_hidden_files);
#endif
	if ( gtk_current_folder_ptr)
		gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER (gtk_dialog), gtk_current_folder_ptr);

	if ( gtk_filters) {
		int i;
		for ( i = 0; i < gtk_filters-> count; i++) {
			gtk_file_chooser_add_filter( 
				GTK_FILE_CHOOSER (gtk_dialog), 
				GTK_FILE_FILTER (gtk_filters-> items[i])
			);
			if ( i == gtk_filter_index)
				gtk_file_chooser_set_filter( 
					GTK_FILE_CHOOSER (gtk_dialog), 
					GTK_FILE_FILTER (gtk_filters-> items[i])
				);
		}
	}

	/* lock prima interactions */
	bzero( &message_dlg, sizeof(message_dlg));
	storage = &guts. message_boxes;
	while ( *storage) storage = &((*storage)-> next);
	*storage = &message_dlg;

	g_idle_add( do_events, &stage);

	if (gtk_dialog_run (GTK_DIALOG (gtk_dialog)) == GTK_RESPONSE_ACCEPT) {

		/* files */
		if ( gtk_select_multiple) {
			int size;
			char * ptr;
			GSList *names, *iter;
	
			names = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER (gtk_dialog));
	
			/* count total length with escaped spaces and backslashes */
			size = 1;
			iter = names;
			while ( iter) {
				char * c = (char*) iter-> data;
				while ( *c) {
					if ( *c == ' ' || *c == '\\') 
						size++;
					size++;
					c++;
				}
				size++;
				iter = iter-> next;
			}
	
			if (( result = ptr = malloc( size))) {
				/* copy and encode */
				iter = names;
				while ( iter) {
					char * c = (char*) iter-> data;
					while ( *c) {
						if ( *c == ' ' || *c == '\\')
							*(ptr++) = '\\';
						*(ptr++) = *c;
						c++;
					}
					iter = iter-> next;
					*(ptr++) = ' ';
				}
				*(ptr - 1) = 0;
			} else {
					warn("gtk_openfile: cannot allocate %d bytes of memory", size);
			}
	
			/* free */
			iter = names;
			while ( iter) {
				g_free( iter-> data);
				iter = iter-> next;
			}
			g_slist_free( names);
		} else {
			char * filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (gtk_dialog));
			result = duplicate_string( filename);
			g_free (filename);
		}
	
		/* directory */
		{
			char * d = gtk_file_chooser_get_current_folder( GTK_FILE_CHOOSER (gtk_dialog));
			if ( d) {
				strncpy( gtk_current_folder, d, MAXPATHLEN);
				gtk_current_folder_ptr = gtk_current_folder;
				g_free( d);
			} else {
				gtk_current_folder_ptr = NULL;
			}
		}
	
		/* filter index */
		gtk_filter_index = 0;
		if ( gtk_filters) {
			int i;
			Handle f = ( Handle) gtk_file_chooser_get_filter( GTK_FILE_CHOOSER (gtk_dialog));
			for ( i = 0; i < gtk_filters-> count; i++)
				if ( gtk_filters-> items[i] == f) {
					gtk_filter_index = i;
					break;
				}
			
		}
	}
		
	if ( gtk_filters) {
		plist_destroy( gtk_filters);
		gtk_filters = NULL;
	}

	*storage = message_dlg. next; /* unlock */

	gtk_widget_destroy (gtk_dialog);
	gtk_dialog = NULL;

	while ( gtk_events_pending()) gtk_main_iteration();

	return result;
}

char *
prima_gtk_openfile( char * params)
{
	if ( !DISP) 
		return NULL;
	if( !prima_gtk_init()) 
		return NULL;

	if ( strncmp( params, "directory", 9) == 0) {
		params += 9;
		if ( *params == '=') {
			params++;
			if ( *params == 0) {
				gtk_current_folder_ptr = NULL;
			} else {
				gtk_current_folder_ptr = gtk_current_folder;
				strncpy( gtk_current_folder, params, MAXPATHLEN);
				gtk_current_folder[MAXPATHLEN] = 0;
			}
		} else
			return duplicate_string( gtk_current_folder_ptr);
	} else if ( strncmp( params, "filters=", 8) == 0) {
		params += 8;
		if ( gtk_filters) {
			int i;
			for ( i = 0; i < gtk_filters-> count; i++) 
				g_object_unref(( GObject*) gtk_filters-> items[i]);
			plist_destroy( gtk_filters);
			gtk_filters = NULL;
		}
		if ( *params != 0) {
			gtk_filters = plist_create(8, 8);

			/* copy \0\0-terminated string */
			while ( *params) {
				char * pattern;
				GtkFileFilter * f = gtk_file_filter_new();

				/* name */
				gtk_file_filter_set_name( f, params);
				while ( *params) params++;
				params++;

				/* semicolon-separated shell globs */
				pattern = ( char *) params;
				while ( *params) {
					if ( *params == ';') {
						*params = 0;
						gtk_file_filter_add_pattern( f, pattern);
						pattern = params + 1;
					}
					params++;
				}
				gtk_file_filter_add_pattern( f, pattern);

				list_add( gtk_filters, (Handle) f);

				params++;
			}
		}
	} else if ( strncmp( params, "filterindex", 11) == 0) {
		params += 11;
		if ( *params == '=') {
			int fi = 0;
			sscanf( params + 1, "%d", &fi);
			gtk_filter_index = fi;
		} else {
			char buf[25];
			sprintf( buf, "%d", gtk_filter_index);
			return duplicate_string( buf);
		}
	} else if ( strncmp( params, "multi_select=", 13) == 0) {
		params += 13;
		gtk_select_multiple = (*params != '0');
	} else if ( strncmp( params, "overwrite_prompt=", 17) == 0) {
		params += 17;
		gtk_overwrite_prompt = (*params != '0');
	} else if (
		( strncmp( params, "open", 4) == 0) || 
		( strncmp( params, "save", 4) == 0)
	) {
		return gtk_openfile( strncmp( params, "open", 4) == 0);
	} else if ( strncmp( params, "show_hidden=", 12) == 0) {
		params += 12;
		gtk_show_hidden_files = (*params != '0');
	} else if ( strncmp( params, "title=", 6) == 0) {
		params += 6;
		if ( *params == 0) {
			gtk_dialog_title_ptr = NULL;
		} else {
			gtk_dialog_title_ptr = gtk_dialog_title;
			strncpy( gtk_dialog_title, params, 255);
			gtk_dialog_title[255] = 0;
		}
	} else {
		warn("gtk2.OpenFile: Unknown function %s", params);
	}

	return NULL;
}

#endif
