#define APPNAME "Generic"

#include "win32\win32guts.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <memory.h>
#include <process.h>
#ifndef CYGWIN32
#include <sys/stat.h>
#endif


#define maxLogListBoxItems          256
#define LOGGER_BUF_SIZE             16384
#define BUFIDX( x)                  ((size_t)( ( x) % LOGGER_BUF_SIZE))

static const char *debug_file = "C:\\POPUPLOG.W32";
static unsigned char loggerBuf[ LOGGER_BUF_SIZE];
static size_t bHead = 0, bTail = 0;
static int hp[2];

#define CM_WRITEHOG                0x6401
#define CM_CLEANHOG                0x6402


static void setLoggerLine( short index, unsigned char *str)
{
   SendMessage( guts. loggerListBox, LB_DELETESTRING, ( WPARAM) index, 0);
   SendMessage( guts. loggerListBox, LB_INSERTSTRING, ( WPARAM) index, ( LPARAM) str);
}

static short addLoggerLine()
{
   int i;
   short index = ( short) SendMessage( guts.loggerListBox, LB_ADDSTRING, 0, ( LPARAM) "");
   SendMessage( guts.loggerListBox, LB_SETTOPINDEX, ( WPARAM) index, 0);
   if  ( index > maxLogListBoxItems) {
      for ( i = 0; i < maxLogListBoxItems / 2; i++)
         SendMessage( guts.loggerListBox, LB_DELETESTRING, 0, 0);
      index = ( short) SendMessage( guts.loggerListBox, LB_GETCOUNT, 0, 0);
   }
   return index;
}


Bool
log_write( const char *format, ...)
{
   int rc;
   va_list arg_ptr;
   va_start( arg_ptr, format);
   rc=vfprintf( stderr, format, arg_ptr);
   fputc( '\n', stderr);
   va_end( arg_ptr);
   return ( rc != EOF);
}

Bool
debug_write( const char *format, ...)
{
   FILE *f;
   int rc;
   va_list arg_ptr;

   if( ( f = fopen( debug_file, "at")) == NULL) return FALSE;
   va_start( arg_ptr, format);
   rc = vfprintf( f, format, arg_ptr);
   va_end( arg_ptr);
   fclose( f);

   return ( rc != EOF);
}

void
dump_logger()
{
   int h;
   debug_write( "**************************************\n");
   debug_write( "---   Post Mortem Logger Context   ---\n");
   debug_write( "--------------------------------------\n");

   h = open( debug_file, O_WRONLY | O_APPEND | O_CREAT | O_SYNC, S_IREAD | S_IWRITE);
   if ( h > 0) {
       if ( bHead < bTail) {
           write( h, loggerBuf + bHead, bTail - bHead);
       }
       else if ( bHead > bTail) {
           write( h, loggerBuf + bHead, LOGGER_BUF_SIZE - bHead);
           if ( bTail > 0) {
               write( h, loggerBuf, bTail - 1);
           }
       }
       // And the case when bHead == bTail must not be processed: the buffer is empty.
       close( h);
   }

   debug_write( "\n--------------------------------------\n");
   debug_write( "**************************************\n");
}

static void add_to_logger_buf( unsigned char *buf, size_t size)
{
    int i = bTail;
    size_t j;
    for ( j = 0; j < size; j++) {
        if ( BUFIDX( i + 1) == bHead)
            bHead = BUFIDX( bHead + size);
        loggerBuf[ i] = buf[ j];
        i = BUFIDX( ++i);
    }
    bTail = i;
}


void docatch( void *dummy)
{
    int rh = hp[ 0];
    while ( ! guts.logger) Sleep( 1);
    do {
        char *buf = malloc( 1024);
        long readed;

        readed = read( rh, buf, 1023);
        buf[ readed] = 0;
        if ( readed <= 0) {
           Sleep(1);
           continue;
        }
        add_to_logger_buf( buf, readed);
        if ( !loggerDead)
            SendMessage( guts.logger, WM_WRITE_TO_LOG, ( WPARAM) buf, 0);
    } while ( 1);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   static long cursorPos = 0;
   BOOL        processed = TRUE;

   switch ( message) {
      case WM_CLOSE:
         loggerDead = TRUE;
         PostThreadMessage( guts. mainThreadId, appDead ? WM_QUIT : WM_TERMINATE, 0, 0);
         break;
      case WM_DESTROY:
         PostQuitMessage(0);
         loggerDead = TRUE;
         processed  = FALSE;
         break;
      case WM_SIZE:
         {
            RECT client;
            GetClientRect( hWnd, &client);
            SetWindowPos( guts.loggerListBox, NULL, 0, 0, client. right, client. bottom,
                             SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
         }
         break;
      case WM_SETFOCUS:
         SetFocus( guts. loggerListBox);
         break;
      case WM_SYSCOMMAND:
         if ( HIWORD( wParam) == 0)
           switch(( int) wParam)
           {
           case CM_WRITEHOG:
               dump_logger();
               return 0;
           case CM_CLEANHOG:
               SendMessage( guts. loggerListBox, LB_RESETCONTENT, 0, 0);
               return 0;
           }
         goto DEFAULT;
      case WM_WRITE_TO_LOG:
         {
            short index, i;
            unsigned char * line = ( unsigned char *) wParam, *newline;
            long newlength, itemlength;
            BOOL doCR = FALSE, doNL = FALSE;
            newlength = strlen( line) + cursorPos;
            // To be sure we won't overrun the new line buffer...
            for ( i = 0; line[ i]; i++)
               if ( line[ i] == '\t')
                  newlength += 7;
            newline = ( unsigned char*) malloc( newlength + 1);

            index = ( short) SendMessage( guts.loggerListBox, LB_GETCOUNT, 0, 0) - 1;
            if ( index < 0) {
                index = addLoggerLine();
                cursorPos = 0;
            }
            itemlength =  SendMessage( guts.loggerListBox, LB_GETTEXT, ( WPARAM) index, ( LPARAM) newline);

            for ( i = 0; line[ i]; i++) {
                if ( line[ i] < ' ') {
                    long tabpos;
                    switch ( line[ i]) {
                        case '\n':
                            doNL = TRUE;
                            break;
                        case '\r':
                            doCR = TRUE;
                            break;
                        case '\t':
                            tabpos = ( cursorPos / 8 + 1) * 8;
                            for (; cursorPos < tabpos; cursorPos++) {
                                newline[ cursorPos] = ' ';
                            }
                            break;
                        default:
                            newline[ cursorPos++] = ' ';
                    }
                }
                else {
                    newline[ cursorPos++] = line[ i];
                }
                if ( itemlength < cursorPos) {
                    itemlength = cursorPos;
                }
                if ( itemlength >= 256) doNL = TRUE;
                if ( doNL) {
                    newline[ itemlength] = 0;
                    itemlength = 0;
                    setLoggerLine( index, newline);
                    index = addLoggerLine();
                    doCR = TRUE;
                    doNL = FALSE;
                }
                if ( doCR) {
                    cursorPos = 0;
                    doCR = FALSE;
                }
            }

            if ( itemlength > 0) {
                newline[ itemlength] = 0;
                setLoggerLine( index, newline);
            }

            free( line);
            free( newline);
         }
         break;

      DEFAULT:
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void sigh( int sig)
{
    if ( sig == SIGSEGV) {
       // cleanup code here
        dump_logger();
    }
    // if ( sig == SIGTERM) {
    //     my_exit( 60);
    //     if ( application) Object_destroy( application);
    // }
    signal( sig, SIG_ACK);
}

void
create_logger_window2( void * dummy)
{
   WNDCLASS  wc;
   RECT r;
   HWND sm;

   memset( &wc, 0, sizeof( wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = (WNDPROC)WndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = guts. instance;
   wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
   wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
   wc.lpszClassName = "Logger";
   if ( !RegisterClass( &wc))
		rc = GetLastError();

   GetWindowRect( GetDesktopWindow(), &r);
   if ( r. bottom > 500) {
      r. left   = 400;
      r. top    = r. bottom - 600;
      r. bottom = r. right = 500;
   } else {
      r. top    = r. bottom - 400;
      r. bottom = r. right = r. left = 300;
   }
   guts.logger = CreateWindow("Logger", "Prima Log", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      r.left, r.top, r.right, r.bottom ,NULL, NULL, guts. instance, NULL);
   if ( !guts. logger)
		rc = GetLastError();
   GetClientRect( guts.logger, &r);
   guts.loggerListBox = CreateWindow( "LISTBOX", "",
      WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_HSCROLL | LBS_HASSTRINGS,
      0, 0, r. right - r. top, r. bottom - r. top,
      guts.logger, NULL, guts. instance, NULL);
   if ( !guts. loggerListBox)
		rc = GetLastError();
   SendMessage( guts. loggerListBox, LB_SETHORIZONTALEXTENT, 4000, 0);

   sm = GetSystemMenu( guts. logger, false);
   AppendMenu( sm, MF_SEPARATOR, 0, nil);
   AppendMenu( sm, MF_STRING,    CM_WRITEHOG, "&Write log to disk");
   AppendMenu( sm, MF_STRING,    CM_CLEANHOG, "Cle&an log");

   ShowWindow( guts.logger, guts. cmdShow);
   UpdateWindow( guts.logger);
   {
      MSG msg;
      while ( GetMessage( &msg, guts.logger, 0, 0)) {
         TranslateMessage( &msg);
         DispatchMessage( &msg);
      }
   }
}

void
start_logger( void)
{
   guts. mainThreadId = GetCurrentThreadId();
   signal( SIGSEGV, sigh);
   signal( SIGTERM, sigh);
    // To handle all standart output...
   _pipe( hp, 256, O_BINARY);
   if (dup2( hp[ 1], 1) == 0) {
      *stdout = *fdopen( 1, "wt");
      setbuf( stdout, NULL);
   }
   if (dup2( hp[ 1], 2) == 0) {
      *stderr = *fdopen( 2, "wt");
      setbuf( stderr, NULL);
   }
   guts. ioThread = ( HANDLE) _beginthread( docatch, 40960, NULL);
   SetThreadPriority( guts. ioThread, THREAD_PRIORITY_ABOVE_NORMAL);
   _beginthread( ( void ( *)( void*))create_logger_window2, 40960, NULL);
}



