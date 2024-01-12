/*********************************/
/*                               */
/*  Gtk file dialog              */
/*                               */
/*********************************/

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include "unix/guts.h"
#include "img.h"

#if !(defined(__FreeBSD__ ) && PERL_REVISION == 5 && PERL_VERSION < 22)
/* Observed hangs on 2 processors with DBUS session running */
#define SAFE_DBUS
#endif

#ifdef WITH_GTK

#undef GT

#undef dirty

#define Window  XWindow

#ifndef WITH_GTK_NONX11
#include <gdk/gdkx.h>
#endif
#include <gtk/gtk.h>

static int           gtk_initialized        = 0;
static GApplication* gtk_app                = NULL;
static GtkWidget*    gtk_dialog             = NULL;
static char          gtk_dialog_title[256];
static char*         gtk_dialog_title_ptr   = NULL;
static Bool          gtk_select_multiple    = FALSE;
static Bool          gtk_overwrite_prompt   = FALSE;
static Bool          gtk_show_hidden_files  = FALSE;
static char          gtk_current_folder[MAXPATHLEN+1];
static char*         gtk_current_folder_ptr = NULL;
static List*         gtk_filters            = NULL;
static int           gtk_filter_index       = 0;
static pid_t         gtk_screenshot_pid     = 0;
static pid_t         gtk_screenshot_ppid    = 0;
static int           gtk_screenshot_sockets[2];

static GdkDisplay * display = NULL;

#if GTK_MAJOR_VERSION == 2
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

static GTFStruct widget_types[] = {
		{ GT(button),       "GtkButton",         wcButton      , NULL },
		{ GT(check_button), "GtkCheckButton",    wcCheckBox    , NULL },
		{ GT(combo_box),    "GtkCombo",          wcCombo       , NULL },
		{ GT(dialog),       "GtkDialog",         wcDialog      , NULL },
		{ GT(entry),        "GtkEditable",       wcEdit        , NULL },
		{ GT(entry),        "GtkEntry",          wcInputLine   , NULL },
		{ GT(label),        "GtkLabel",          wcLabel       , &guts. default_msg_font },
		{ GT(menu),         "GtkMenuItem",       wcMenu        , &guts. default_menu_font },
		{ GT(menu_item),    "GtkMenuItem",       wcPopup       , NULL },
		{ GT(check_button), "GtkRadioButton",    wcRadio       , NULL },
		{ GT(scrollbar),    "GtkScrollBar",      wcScrollBar   , NULL },
		{ GT(widget),       "GtkWidget",         wcWidget      , &guts. default_widget_font },
		{ GT(window),       "GtkWindow",         wcWindow      , &guts. default_caption_font },
		{ GT(widget),       "GtkWidget",         wcApplication , &guts. default_font },
#if GTK_MAJOR_VERSION == 2
		{ GT(list),         "GtkList",           wcListBox     , NULL },
		{ GT(ruler),        "GtkRuler",          wcSlider      , NULL },
#else
		{ GT(list_box),     "GtkListBox",        wcListBox     , NULL },
		{ GT(spin_button),  "GtkSpinButton",     wcSlider      , NULL },
#endif
};
#undef GT

#endif

#if GTK_MAJOR_VERSION == 3

static Color
gdk_color(GdkRGBA * c)
{
	int r = c->red   * 255 + .5;
	int g = c->green * 255 + .5;
	int b = c->blue  * 255 + .5;
	if ( r < 0 ) r = 0;
	if ( g < 0 ) g = 0;
	if ( b < 0 ) b = 0;
	if ( r > 255 ) r = 255;
	if ( g > 255 ) g = 255;
	if ( b > 255 ) b = 255;
	return (r << 16) | (g << 8) | b;
}

static const char * gt_color_properties[] = {
	"theme_text_color",
	"theme_bg_color",
	"theme_selected_fg_color",
	"theme_selected_bg_color",
	"insensitive_fg_color",
	"insensitive_bg_color",
	"theme_fg_color",
	"theme_unfocused_bg_color"
};
#define GTK_COLORLIST_SIZE (sizeof(gt_color_properties)/sizeof(char*))

GdkDisplay *
my_gdk_display_open_default (void)
{
	GdkDisplay *display;

	display = gdk_display_get_default ();
	if (display)
		return display;

	display = gdk_display_open (gdk_get_display_arg_name ());

	return display;
}
#endif

#if defined(SAFE_DBUS) && (GLIB_MAJOR_VERSION > 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 34))
#define DBUS_SCREENSHOT

/* GIO wants that callback, even empty */
static void gtk_application_activate (GApplication *app) {}

static int
make_screenshot(int x, int y, int w, int h)
{
	GApplication    *app;
	GDBusConnection *conn;
	GVariant        *params, *results;
	GError          *error = NULL;
	char             filename[256];

	app = g_application_new("org.gnome.Screenshot", 
#ifdef GLIB_AVAILABLE_ENUMERATOR_IN_2_74
		G_APPLICATION_DEFAULT_FLAGS
#else
		G_APPLICATION_FLAGS_NONE
#endif
	);
	if ( !g_application_register (app, NULL, NULL)) {
		g_object_unref(app);
		Mdebug("cannot register another gtk application");
		return false;
	}

	if (!( conn = g_application_get_dbus_connection (app))) {
		g_object_unref(app);
		Mdebug("cannot get dbus connection");
		return false;
	}

	snprintf(filename, 256, "/tmp/%d-sc.png", gtk_screenshot_ppid);
	params = g_variant_new("(iiiibs)",
		x, y, w, h,
		0, filename);

	results = g_dbus_connection_call_sync (conn,
		"org.gnome.Shell.Screenshot",
		"/org/gnome/Shell/Screenshot",
		"org.gnome.Shell.Screenshot",
		"ScreenshotArea",
		params,
		NULL,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error
	);

	if ( results )
		g_variant_unref( results );
	if (error != NULL) {
		Mdebug("cannot get gnome shell screenshot:%s", error->message);
      		g_error_free (error);
		g_object_unref(app);
		return false;
	}

	g_object_unref(app);
	return true;
}

static void
terminate_screenshot_app(void)
{
	int status;

	if ( !gtk_screenshot_pid ) return;

	close( gtk_screenshot_sockets[0]);
	kill( gtk_screenshot_pid, SIGINT);
	waitpid( gtk_screenshot_pid, &status, 0);
	gtk_screenshot_pid = 0;
}

static void
run_screenshot_app(void)
{
	int s = gtk_screenshot_sockets[1];
	int buf[20];
	while (1) {
		int n = read( s, buf, sizeof(int) * 4 );
		if ( n < sizeof(int) * 4 ) {
			Mdebug("bad screenshot request");
			break;
		}
		buf[0] = make_screenshot(buf[0], buf[1], buf[2], buf[3]);
		n = write( s, buf, sizeof(int) );
		if ( n < sizeof(int) ) {
			Mdebug("screenshot: cannot respond");
			break;
		}
	}
}

static Bool
request_screenshot(int x, int y, int w, int h)
{
	int s = gtk_screenshot_sockets[0];
	int n, buf[4] = { x, y, w, h };
	n = write( s, buf, sizeof(int) * 4 );
	if ( n < sizeof(int) * 4 ) {
		Mdebug("bad write to screenshot app");
		terminate_screenshot_app();
		return false;
	}
	n = read( s, buf, sizeof(int));
	if ( n < sizeof(int) ) {
		Mdebug("bad read from screenshot app");
		terminate_screenshot_app();
		return false;
	}
	return n;
}

#endif

static int
get_int( GtkSettings * settings, const char * prop_name)
{
	gint v;
	g_object_get(G_OBJECT(settings), prop_name, &v, NULL);
	return v;
}

Display*
prima_gtk_init(void)
{
	int  argc = 0;
	Display *ret;
	GtkSettings * settings;
	Color ** stdcolors;
	int i;
	PangoWeight weight;

	switch ( gtk_initialized) {
	case -1:
		return NULL;
	case 1:
#ifdef WITH_GTK_NONX11
		{
		}
		return (void*)1;
#else
		return gdk_x11_display_get_xdisplay(display);
#endif
	}

#ifdef DBUS_SCREENSHOT
	gtk_screenshot_ppid = getpid();
	if ( socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, gtk_screenshot_sockets) == 0) {
		gtk_screenshot_pid  = fork();
		if ( gtk_screenshot_pid == 0 ) {
			close(gtk_screenshot_sockets[0]);
			run_screenshot_app();
			exit(0);
		} else {
			close(gtk_screenshot_sockets[1]);
		}
	} else {
		gtk_screenshot_pid = 0;
		Mdebug("socketpair() error");
	}
#endif

#ifdef WITH_GTK_NONX11
	{
		char * display_str = getenv("DISPLAY");
		if ( display_str ) {
			struct stat s;
			if ((stat( display_str, &s) < 0) || !S_ISSOCK(s.st_mode))  /* not a socket */
				return (void*)0;
		}
	}
#endif

        /* after 5.20.0 perl doesn't tolerate changes to setlocale, see more at:
	   https://rt.perl.org/Ticket/Display.html?id=122105
           https://rt.perl.org/Ticket/Display.html?id=133945 */
	gtk_disable_setlocale();

	if ( !gtk_parse_args (&argc, NULL) || (
		display =
#if GTK_MAJOR_VERSION == 2
			gdk_display_open_default_libgtk_only()
#else
			my_gdk_display_open_default()
#endif
		) == NULL) {
		gtk_initialized = -1;
		return false;
	} else {
		gtk_initialized = 1;
		XSetErrorHandler( guts.main_error_handler );
#ifdef WITH_GTK_NONX11
		ret = (void*)1;
#else
		ret = gdk_x11_display_get_xdisplay(display);
#endif
	}

#ifdef DBUS_SCREENSHOT
	gtk_app = g_application_new ("org.prima", G_APPLICATION_NON_UNIQUE);

	g_signal_connect (gtk_app, "activate", G_CALLBACK (gtk_application_activate), NULL);
	if ( !g_application_register (gtk_app, NULL, NULL)) {
  		g_object_unref (gtk_app);
		gtk_app = NULL;
	}
#endif

	settings  = gtk_settings_get_default();

#if GTK_MAJOR_VERSION == 2
	stdcolors = prima_standard_colors(NULL);
	for ( i = 0; i < sizeof(widget_types)/sizeof(GTFStruct); i++) {
		GTFStruct * s = widget_types + i;
		Color     * c = stdcolors[ s-> prima_class >> 16 ];
		Font      * f = s->prima_font;
		GtkStyle  * t = gtk_rc_get_style_by_paths(settings, NULL, s->gtk_class, s->func());
		int selected  = (
			s->prima_class == wcRadio ||
			s->prima_class == wcCheckBox ||
			s->prima_class == wcButton
		) ? GTK_STATE_ACTIVE : GTK_STATE_SELECTED;
		if ( t == NULL ) {
			Pdebug("cannot query gtk style for %s", s->name);
			t = gtk_rc_get_style_by_paths(settings, NULL, NULL, GTK_TYPE_WIDGET);
			if ( !t ) continue;
		}
		c[ciFore]         = gdk_color( t-> fg + GTK_STATE_NORMAL );
		c[ciBack]         = gdk_color( t-> bg + GTK_STATE_NORMAL );
		c[ciDisabledText] = gdk_color( t-> fg + GTK_STATE_INSENSITIVE );
		c[ciDisabled]     = gdk_color( t-> bg + GTK_STATE_INSENSITIVE );

		if ( s-> prima_class == wcMenu || s-> prima_class == wcPopup) {
			/* Observed on Centos7 - GTK_STATE_SELECTED gives white
			on white, while GTK_STATE_PRELIGHT gives correct colors.
			OTOH, on Ubuntu it is other way around. Without digging
			too much into GTK guts, just select the one that gives
			best contrast */

			int da, db;
			Color ca1, ca2, cb1, cb2;
			ca1 = gdk_color( t-> fg + selected );
			ca2 = gdk_color( t-> bg + selected );
			da =
				abs( (int)(ca1 & 0xff)-(int)(ca2 & 0xff) ) +
				abs( (int)((ca1 & 0xff00)>>8)-(int)((ca2 & 0xff00)>>8) ) +
				abs( (int)((ca1 & 0xff0000)>>16)-(int)((ca2 & 0xff0000)>>16) )
			;
			cb1 = gdk_color( t-> fg + GTK_STATE_PRELIGHT );
			cb2 = gdk_color( t-> bg + GTK_STATE_PRELIGHT );
			db =
				abs( (int)(cb1 & 0xff)-(int)(cb2 & 0xff) ) +
				abs( (int)((cb1 & 0xff00)>>8)-(int)((cb2 & 0xff00)>>8) ) +
				abs( (int)((cb1 & 0xff0000)>>16)-(int)((cb2 & 0xff0000)>>16) )
			;
			c[ciHiliteText]   = (da > db) ? ca1 : cb1;
			c[ciHilite]       = (da > db) ? ca2 : cb2;
		} else {
			c[ciHiliteText]   = gdk_color( t-> fg + selected );
			c[ciHilite]       = gdk_color( t-> bg + selected );
		}
		Pdebug("gtk-color: %s %06x %06x %06x %06x %06x", s->name, c[0], c[1], c[2], c[3], c[4], c[5]);

		if ( !f) continue;
		bzero(f, sizeof(Font));
		strlcpy( f->name, pango_font_description_get_family(t->font_desc), 255);
		if ( strncmp(f->name, ".Apple", 6) != 0) {
			/* does gnome ignore X resolution? */
			f-> size = pango_font_description_get_size(t->font_desc) / PANGO_SCALE * (96.0 / guts. resolution. y) + .5;
			weight = pango_font_description_get_weight(t->font_desc);
			if ( weight <= PANGO_WEIGHT_LIGHT ) f-> style |= fsThin;
			if ( weight >= PANGO_WEIGHT_BOLD  ) f-> style |= fsBold;
			if ( pango_font_description_get_style(t->font_desc) == PANGO_STYLE_ITALIC)
				f-> style |= fsItalic;
			strcpy( f->encoding, "Default" );
			f-> undef. width = f-> undef. height = f-> undef. pitch = f-> undef. vector = 1;
			apc_font_pick( f, NULL );
#define DEBUG_FONT(font) f->height,f->width,f->size,f->name,f->encoding
			Fdebug("gtk-font (%s): %d.[w=%d,s=%d].%s.%s", s->name, DEBUG_FONT(f));
#undef DEBUG_FONT
		}
	}
#endif

#if GTK_MAJOR_VERSION == 3
	{
		/* font */
		GValue value = {0};
		Font font;
		PangoFontDescription *pfd;

		g_value_init(&value, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(settings), "gtk-font-name", &value);

		bzero(&font, sizeof(Font));

		pfd = pango_font_description_from_string((char*) g_value_peek_pointer(&value));

		strlcpy( font.name, pango_font_description_get_family(pfd), 255);
		if ( strncmp(font.name, ".Apple", 6) != 0) {
			font.size = FONT_SIZE_ROUND(pango_font_description_get_size(pfd) / 1000.0);
			weight    = pango_font_description_get_weight(pfd);
			if ( weight <= PANGO_WEIGHT_LIGHT ) font.style |= fsThin;
			if ( weight >= PANGO_WEIGHT_BOLD  ) font.style |= fsBold;
			if ( pango_font_description_get_style(pfd) == PANGO_STYLE_ITALIC)
				font.style |= fsItalic;
			strcpy( font.encoding, "Default" );
			font.undef.width = font.undef.height = font.undef.pitch = font.undef.vector = 1;
			prima_font_pick( &font, NULL, NULL, 0 );
#define DEBUG_FONT font.height,font.width,font.size,font.name,font.encoding
			Fdebug("gtk-font (%s): %d.[w=%d,s=%g].%s.%s", g_value_peek_pointer(&value), DEBUG_FONT);
#undef DEBUG_FONT
			guts.default_msg_font     =
			guts.default_menu_font    =
			guts.default_widget_font  =
			guts.default_caption_font =
			guts.default_font         = font;

			pango_font_description_free(pfd);
		}

	}

	{
		/* colors */
		GtkStyleContext *ctx;
		GdkRGBA color;
		int n_classes;
		Color colors[GTK_COLORLIST_SIZE], template1[ciMaxId+1], template2[ciMaxId+1];

		stdcolors = prima_standard_colors(&n_classes);
		ctx = gtk_style_context_new();

		for ( i = 0; i < GTK_COLORLIST_SIZE; i++)
			colors[i] = clInvalid;
		memcpy( template1, stdcolors[wcApplication >> 16], sizeof(template1));
		memcpy( template2, stdcolors[wcButton      >> 16], sizeof(template2));

		for ( i = 0; i < sizeof(gt_color_properties)/sizeof(char*); i++) {
			if (!gtk_style_context_lookup_color( ctx, gt_color_properties[i], &color))
				continue;
			Mdebug("gtk-color: %s %g %g %g", gt_color_properties[i], color.red, color.green, color.blue);
			colors[i] = gdk_color(&color);
		}

		if ( colors[0] != clInvalid) template1[ciFore]         = colors[0];
		if ( colors[1] != clInvalid) template1[ciBack]         = colors[1];
		if ( colors[2] != clInvalid) template1[ciHiliteText]   = colors[2];
		if ( colors[3] != clInvalid) template1[ciHilite]       = colors[3];
		if ( colors[4] != clInvalid) template1[ciDisabledText] = colors[4];
		if ( colors[5] != clInvalid) template1[ciDisabled]     = colors[5];

		if ( colors[0] != clInvalid) template2[ciFore]         = colors[0];
		if ( colors[1] != clInvalid) template2[ciBack]         = colors[1];
		if ( colors[6] != clInvalid) template2[ciHiliteText]   = colors[6];
		if ( colors[7] != clInvalid) template2[ciHilite]       = colors[7];
		if ( colors[4] != clInvalid) template2[ciDisabledText] = colors[4];
		if ( colors[5] != clInvalid) template2[ciDisabled]     = colors[5];

		for ( i = 1; i < n_classes; i++) {
			Color *template;
			template = (
				((i << 16) == wcButton) ||
				((i << 16) == wcRadio) ||
				((i << 16) == wcCheckBox)
			) ? template2 : template1;
			memcpy( stdcolors[i], template, sizeof(template1));
		}

		/* if backcolor is too close to ciLight3DColor that is 0xffffff, the latter is not visible much */
		colors[0] = stdcolors[wcInputLine >> 16][ciBack];
		colors[1] = stdcolors[wcInputLine >> 16][ciLight3DColor];
		colors[2] = stdcolors[wcInputLine >> 16][ciDark3DColor];
		if (
			(abs( COLOR_R((int)colors[0]) - COLOR_R((int)colors[1])) < 16) &&
			(abs( COLOR_G((int)colors[0]) - COLOR_G((int)colors[1])) < 16) &&
			(abs( COLOR_B((int)colors[0]) - COLOR_B((int)colors[1])) < 16)
		) {
			int r = ( COLOR_R(colors[0]) * 0.75 + COLOR_R(colors[2]) * 0.25);
			int g = ( COLOR_G(colors[0]) * 0.75 + COLOR_G(colors[2]) * 0.25);
			int b = ( COLOR_B(colors[0]) * 0.75 + COLOR_B(colors[2]) * 0.25);
			colors[3] = (r << 16) | (g << 8) | b;
			for ( i = 1; i < n_classes; i++) {
				stdcolors[i][ciLight3DColor] = colors[3];
			}
		}

		g_object_unref(ctx);
	}
#endif

	/* misc settings */
	guts.double_click_time_frame = (Time) get_int( settings, "gtk-double-click-time");
	i = get_int( settings, "gtk-cursor-blink-time");
	guts.visible_timeout   = i * 2 / 3; /* from gtkentry.c */
	guts.invisible_timeout = i * 1 / 3;
        i = get_int( settings, "gtk-cursor-blink");
	if ( !i ) guts.invisible_timeout = 0;

	return ret;
}

Bool
prima_gtk_done(void)
{
#ifdef DBUS_SCREENSHOT
	terminate_screenshot_app();
#endif
	if ( gtk_filters) {
		int i;
		for ( i = 0; i < gtk_filters-> count; i++)
			g_object_unref(( GObject*) gtk_filters-> items[i]);
		plist_destroy( gtk_filters);
		gtk_filters = NULL;
	}
	if ( gtk_app ) {
		g_object_unref( gtk_app );
		gtk_app = NULL;
	}
	gtk_initialized = 0;
	return true;
}

#ifndef WITH_GTK_NONX11
static void
set_transient_for(void)
{
	Handle toplevel = prima_find_toplevel_window(NULL_HANDLE);
	if ( toplevel ) {
		GdkWindow * g = NULL;

#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 14)
		g = gtk_widget_get_window(GTK_WIDGET(gtk_dialog));
#else
		g = gtk_dialog->window;
#endif
		if ( g ) {
			Window w = GDK_WINDOW_XID(g);
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
	static struct timeval last_event = {0,0}, t;
	if ( gtk_dialog != NULL && !*stage ) {
		*stage = 1;
#ifdef WITH_GTK_NONX11
		gtk_window_present(GTK_WINDOW(gtk_dialog));
#else
		set_transient_for();
#endif
	}
	if (( t.tv_sec - last_event.tv_sec) * 1000000 + t.tv_usec - last_event.tv_usec > 10000) {
		last_event = t;
		prima_one_loop_round( WAIT_NEVER, true);
	}
	return gtk_dialog != NULL;
}

static int ignore_errors(Display *d, XErrorEvent *ev) { return 0; }

static char *
gtk_openfile( Bool open)
{
	char *result = NULL;
	struct MsgDlg message_dlg, **storage;
	int stage = 0;

	if ( gtk_dialog) return NULL; /* we're not reentrant */

	XFlush(DISP);
	XCHECKPOINT;
	XSetErrorHandler(ignore_errors);

	gtk_dialog = gtk_file_chooser_dialog_new (
		gtk_dialog_title_ptr ?
			gtk_dialog_title_ptr :
			( open ? "Open File" : "Save File"),
		NULL,
		open ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE,
#if GTK_MAJOR_VERSION == 3
		"_Cancel",
#else
		GTK_STOCK_CANCEL,
#endif
		GTK_RESPONSE_CANCEL,
#if GTK_MAJOR_VERSION == 3
		open ? "_Open" : "_Save",
#else
		open ? GTK_STOCK_OPEN : GTK_STOCK_SAVE,
#endif
		GTK_RESPONSE_ACCEPT,
		NULL);
#ifdef WITH_GTK_NONX11
	gtk_window_set_position( GTK_WINDOW(gtk_dialog), GTK_WIN_POS_CENTER);
#endif

	gtk_file_chooser_set_local_only( GTK_FILE_CHOOSER (gtk_dialog), TRUE);
	if (open)
		gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER (gtk_dialog), gtk_select_multiple);

#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 8)
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
				strlcpy( gtk_current_folder, d, MAXPATHLEN);
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

	XSync(DISP, false);
	XCHECKPOINT;
	XSetErrorHandler(guts.main_error_handler);

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
				strlcpy( gtk_current_folder, params, MAXPATHLEN);
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
			strlcpy( gtk_dialog_title, params, 255);
		}
	} else {
		warn("gtk.OpenFile: Unknown function %s", params);
	}

	return NULL;
}

/* Thanks to Cosimo Cecchi @ gnome-screenshot for the code below */
Bool
prima_gtk_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen)
{
#ifdef DBUS_SCREENSHOT
	DEFXX;
	int              i, found_png;
	PList            codecs;
	char             filename[256];

	if ( !gtk_screenshot_pid)
		return false;

	/* do we have png? it seems gnome only saves scheenshots as pngs */
	codecs = plist_create( 16, 16);
	apc_img_codecs( codecs);
	found_png = false;
	for ( i = 0; i < codecs-> count; i++) {
		PImgCodec c = ( PImgCodec ) codecs-> items[ i];
		if ( strcmp( c-> info-> fileShortType, "PNG" ) == 0 ) {
			found_png = true;
			break;
		}
	}
	plist_destroy( codecs);
	if ( !found_png ) {
		Mdebug("PNG decoder not found");
		return false;
	}

	/* execute gnome shell screenshot */
	if ( !request_screenshot( x, XX->size.y - y - yLen, xLen, yLen))
		return false;

	/* load */
	snprintf(filename, 256, "/tmp/%d-sc.png", gtk_screenshot_ppid);
	codecs = apc_img_load( image, filename, false, NULL, NULL, NULL);
	unlink( filename );
	if ( !codecs ) {
		Mdebug("error loading png back");
		return false;
	}
	plist_destroy(codecs);

	return true;
#else
	return false;
#endif
}

#endif
