/* Guts library, logger window, os2 */

#include "os2/os2guts.h"
#include "Application.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>

#define loggerWindowType            "LoGgEr"
#define maxLogListBoxItems          256

#define LOGGER_BUF_SIZE             16384
#define BUFIDX( x)                  ( ( x) % LOGGER_BUF_SIZE)

#define SYSMALLOC                   emx_malloc
#define SYSFREE                     emx_free

#define CM_CLEANHOG                 998
#define CM_WRITEHOG                 999

MRESULT EXPENTRY
handle_logger_window( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2);

extern Bool freePS( HPS ps, void * dummy);
extern Bool freeWinPS( HPS ps, void * dummy);

Bool loggerDead = false;

static const char *debug_file = "C:\\POPUPHOG.OS2";
static unsigned char loggerBuf[ LOGGER_BUF_SIZE];
static size_t bHead = 0, bTail = 0;
static int hp[2];
static ULONG minor;

// Note! Function not designed to work with a size bigger than LOGGER_BUF_SIZE.
void add_to_logger_buf( unsigned char *buf, size_t size)
{
    int i = bTail, j;
    for ( j = 0; j < size; j++) {
        if ( BUFIDX( i + 1) == bHead) {
            bHead = BUFIDX( bHead + size);
        }
        loggerBuf[ i] = buf[ j];
        i = BUFIDX( ++i);
    }
    bTail = i;
}

void docatch( void *dummy)
{
    int rh,wh;
    HAB catchHab;
    HMQ hmq;
    QMSG msg;

    (void)dummy;
    DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, &minor, sizeof( minor));

    rh = hp[ 0];
    wh = hp[ 1];

    catchHab = WinInitialize( 0);
    hmq = WinCreateMsgQueue( catchHab, minor <= 30 ? 256 : 0);

    // Working on this priority we can be sure that output catching will be
    // smooth enough or as good as possible at least.
    DosSetPriority( PRTYS_THREAD, PRTYC_TIMECRITICAL, 31, 0);

    do {
        char *buf = SYSMALLOC( 1024);
        ULONG readed;

        readed=read( rh, buf, 1023);
        buf[ readed]=0;

        add_to_logger_buf( buf, readed);

        if ( !loggerDead) {
            WinSendMsg( guts. logger,
                        WM_WRITE_TO_LOG,
                        MPFROMP( buf),
                        MPVOID);
        }

        while ( WinPeekMsg( hmq, &msg, NULLHANDLE, 0, 0, PM_REMOVE));
    } while ( 1);
}

void
dump_logger( void)
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


void sigh( int sig)
{
    if ( sig == SIGSEGV) {
        list_first_that( &guts. psList, freePS, nil);
        list_first_that( &guts. winPsList, freeWinPS, nil);
        dump_logger();
    }
    if ( ( sig == SIGTERM) || ( sig == SIGUSR1)) {
        if ( application) Object_destroy( application);
        if ( sig == SIGTERM) dump_logger();
        my_exit( 60);
    }
    signal( sig, SIG_ACK);
}

void
create_logger_window2( void)
{
   ULONG frameFlags =   FCF_TITLEBAR | FCF_SYSMENU | FCF_TASKLIST | // FCF_ICON |
                        FCF_SIZEBORDER | FCF_MAXBUTTON | FCF_HIDEBUTTON | FCF_NOBYTEALIGN;
   HWND frameWindow;
   HAB hab;
   HMQ hmq;
   QMSG message;

   hab = WinInitialize( 0);
   hmq = WinCreateMsgQueue( hab, minor <= 30 ? 256 : 0);

   WinRegisterClass( guts. anchor,
                     loggerWindowType,
                     handle_logger_window,
                     CS_SIZEREDRAW | CS_CLIPCHILDREN,
                     0);

   frameWindow = WinCreateStdWindow( HWND_DESKTOP,
                                     0,                 // frame style
                                     &frameFlags,
                                     loggerWindowType,
                                     "Prima Log",       // title text
                                     WS_VISIBLE,        // client style
                                     nilHandle,         // resources location
                                     nilHandle,         // loggerIcon,
                                     &guts. logger);
   {
      HWND     sm = WinWindowFromID( frameWindow, FID_SYSMENU);
      MENUITEM menuItem;
      int      id = ( int) WinSendMsg( sm, MM_ITEMIDFROMPOSITION, 0, 0);
      WinSendMsg( sm, MM_QUERYITEM, MPFROM2SHORT( id, 0), &menuItem);
      sm = menuItem. hwndSubMenu;

      memset( &menuItem, 0, sizeof( menuItem));
      menuItem. iPosition   = MIT_END;
      menuItem. afStyle     = MIS_SEPARATOR;
      WinSendMsg( sm, MM_INSERTITEM, MPFROMP( &menuItem), "");
      menuItem. afStyle     = MIS_TEXT;
      menuItem. id          = CM_WRITEHOG;
      WinSendMsg( sm, MM_INSERTITEM, MPFROMP( &menuItem), "~Write log to disk");
      menuItem. id          = CM_CLEANHOG;
      WinSendMsg( sm, MM_INSERTITEM, MPFROMP( &menuItem), "Cle~an log");
   }

   WinSetWindowPos( frameWindow, HWND_TOP,
                    400, 100, 500, 500,
                    SWP_SIZE | SWP_MOVE | SWP_SHOW | SWP_ACTIVATE);

   while ( WinGetMsg( hab, &message, 0, 0, 0))
   {
      WinDispatchMsg( hab, &message);
   }

   WinDestroyMsgQueue( hmq);
   WinTerminate( hab);
   loggerDead = true;
   guts. logger = nilHandle;
   guts. loggerListBox = nilHandle;
}

void create_logger_window( void)
{
    DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, &minor, sizeof( minor));

    signal( SIGSEGV, sigh);
    signal( SIGTERM, sigh);
    signal( SIGUSR1, sigh);

    // To handle all standart output...
    pipe( hp);
    dup2( hp[ 1], fileno( stdout));
    dup2( hp[ 1], fileno( stderr));

    _beginthread( ( void ( *)( void*))create_logger_window2, NULL, 40960, NULL);
}

MRESULT setLoggerLine( short index, unsigned char *str)
{
    return WinSendMsg( guts. loggerListBox,
                       LM_SETITEMTEXT,
                       MPFROMSHORT( index),
                       MPFROMP( str));
                       //MPFROMP( guts. loggerBuf[ guts. loggerCurrent]));
}

MRESULT addLoggerLine( void)
{
    int i;
    short index = SHORT1FROMMR( WinSendMsg( guts. loggerListBox,
                                            LM_INSERTITEM,
                                            MPFROMSHORT( LIT_END),
                                            MPFROMP( "")));

    WinSendMsg( guts. loggerListBox,
                LM_SETTOPINDEX,
                MPFROMSHORT( index),
                MPVOID);

    if ( index > maxLogListBoxItems)
    {
       for ( i = 0; i < maxLogListBoxItems / 2; i++)
       {
          WinSendMsg( guts. loggerListBox,
                      LM_DELETEITEM,
                      0,
                      MPVOID);
       }
       index = SHORT1FROMMR( WinSendMsg( guts. loggerListBox,
                                         LM_QUERYITEMCOUNT,
                                         MPVOID,
                                         MPVOID)) - 1;
    }

    return MRFROMSHORT( index);
}

MRESULT EXPENTRY
handle_logger_window( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   MRESULT toReturn = nil;
   Bool processed = true;
   static int closings;
   static long cursorPos = 0;

   switch ( msg)
   {
      case WM_ERASEBACKGROUND:
         toReturn = ( MRESULT)true;
         break;

      case WM_CREATE:
         {
            RECTL client;
            COLOR clr;

            WinQueryWindowRect ( win, &client);
            guts. loggerListBox =
               WinCreateWindow( win,
                                WC_LISTBOX,
                                "",
                                WS_VISIBLE | LS_NOADJUSTPOS | LS_HORZSCROLL,
                                client. xLeft,
                                client. yBottom,
                                client. xRight,
                                client. yTop,
                                NULLHANDLE,
                                HWND_TOP,
                                0,  //?? id
                                nil,
                                nil);
            clr = SYSCLR_DIALOGBACKGROUND;
            WinSetPresParam( guts. loggerListBox, PP_BACKGROUNDCOLORINDEX,
                             sizeof( COLOR), &clr);
            WinSetPresParam( guts. loggerListBox, PP_FONTNAMESIZE, 23,
                             "13.System VIO");

            _beginthread( docatch, NULL, 40960, NULL);
         }
         break;

       case WM_CLOSE:
          closings++;
          if ( closings >= 2) kill( getpid(), SIGUSR1);
          WinPostQueueMsg( guts.queue, ( appDead ? WM_QUIT : WM_TERMINATE), MPVOID, MPVOID);
          return MRFROMLONG(0);

      case WM_DESTROY:
          loggerDead = true;
          processed = false;
          break;

      case WM_SIZE:
         {
            RECTL client;
            WinQueryWindowRect( win, &client);
            WinSetWindowPos( guts. loggerListBox,
                             nilHandle,
                             client. xLeft,
                             client. yBottom,
                             client. xRight,
                             client. yTop,
                             SWP_SIZE | SWP_MOVE);
         }
         break;

      case WM_WRITE_TO_LOG:
         {
            short index, i;
            unsigned char * line = PVOIDFROMMP ( mp1), *newline;
            long newlength, itemlength;
            Bool doCR = false, doNL = false;

            newlength = strlen( line) + cursorPos;
            // To be sure we won't overrun the new line buffer...
            for ( i = 0; line[ i]; i++) {
                if ( line[ i] == '\t') {
                    newlength += 7;
                }
            }
            newline = ( unsigned char*) SYSMALLOC( newlength + 1);

            index = SHORT1FROMMR( WinSendMsg( guts. loggerListBox, LM_QUERYITEMCOUNT, MPVOID, MPVOID)) - 1;
            if ( index < 0) {
                index = SHORT1FROMMR( addLoggerLine());
                cursorPos = 0;
            }
            itemlength = LONGFROMMR( WinSendMsg( guts. loggerListBox,
                                                 LM_QUERYITEMTEXT,
                                                 MPFROM2SHORT( index, newlength),
                                                 MPFROMP( newline)));

            for ( i = 0; line[ i]; i++) {
                if ( line[ i] < ' ') {
                    long tabpos;
                    switch ( line[ i]) {
                        case '\n':
                            doNL = true;
                            break;
                        case '\r':
                            doCR = true;
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
                if ( itemlength >= 256) {
                    doNL = true;
                }
                if ( doNL) {
                    newline[ itemlength] = 0;
                    itemlength = 0;
                    setLoggerLine( index, newline);
                    index = SHORT1FROMMR( addLoggerLine());
                    doCR = true;
                    doNL = false;
                }
                if ( doCR) {
                    cursorPos = 0;
                    doCR = false;
                }
            }

            if ( itemlength > 0) {
                newline[ itemlength] = 0;
                setLoggerLine( index, newline);
            }

            SYSFREE( line);
            SYSFREE( newline);
         }
         break;
      case WM_COMMAND:
         if ( SHORT1FROMMP( mp2) == CMDSRC_MENU)
           switch(( int) mp1)
           {
           case CM_WRITEHOG:
               dump_logger();
               return 0;
           case CM_CLEANHOG:
               WinSendMsg( guts. loggerListBox, LM_DELETEALL, 0, 0);
               return 0;
           }
         break;
      case WM_FOCUSCHANGE:
         processed = false;
         break;
      default:
         processed = false;
   }

   if ( !processed )
      toReturn = WinDefWindowProc( win, msg, mp1, mp2);

   return ( toReturn);
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

   if( ( f = fopen( debug_file, "at")) == NULL) {
       return false;
   }
   va_start( arg_ptr, format);
   rc = vfprintf( f, format, arg_ptr);
   va_end( arg_ptr);
   fclose( f);

   return ( rc != EOF);
}

