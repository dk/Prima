/*
 * Copyright (c) 2007 Dmitry Karasik
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

/*********************************/
/*                               */
/*  Gtk file dialog              */
/*                               */
/*********************************/

#include "unix/guts.h"

#ifdef WITH_GTK2

#undef dirty

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

Bool
prima_gtk_init(void)
{
	int argc = 0;

	gboolean r;
	
	switch ( gtk_initialized) {
	case -1:
		return false;
	case 1:
		return true;
	}

	r = gtk_init_check( &argc, NULL);

	if ( r == gtk_true()) {
		XSetErrorHandler( guts. main_error_handler);
		gtk_initialized = 1;
		return true;
	} else {
		gtk_initialized = -1;
		warn("** Cannot initialize GTK");
		return false;
	}
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

static gboolean do_events(gpointer data)
{
	prima_one_loop_round( false, true);
	return gtk_dialog != NULL;
}

static char *
gtk_openfile( Bool open)
{

	char *result = NULL;
   	struct MsgDlg message_dlg, **storage;

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

	g_idle_add( do_events, NULL);

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
	if ( strncmp( params, "version", 7) == 0)
		return duplicate_string( GTK_VERSION);

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
