#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

static char primaPath[ _MAX_PATH];
static char tmpPath[ _MAX_PATH * 3];
static int (*prima)( const char *, int, char **);
static void (*set_platform_data)( HINSTANCE, int);
static HINSTANCE instance;
static int cmdShow;

static void
fill_prima_path( void)
{
   char primaName[ _MAX_PATH];
   char primaDrive[ _MAX_DRIVE];
   char primaDir[ _MAX_DIR];

   GetModuleFileName( instance, primaName, _MAX_PATH - 1);
   _splitpath( primaName, primaDrive, primaDir, NULL, NULL);
   _makepath( primaPath, primaDrive, primaDir, NULL, NULL);
}

static void
fatal_error( const char *error)
{
   MessageBox( NULL, error, "Prima Fatal Error", MB_OK | MB_ICONEXCLAMATION);
   exit( 1);
}

static void
load_prima_guts( void)
{
   char errMsg[1024];
   HINSTANCE lib;

   sprintf( tmpPath, "%s\\Guts\\primguts.dll", primaPath);
   lib = LoadLibrary( tmpPath);
   if ( lib == NULL) {
      DWORD err = GetLastError();
      LPVOID lpMsgBuf;
      FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         ( LPTSTR) &lpMsgBuf, 0, NULL);
      sprintf( errMsg, "Error loading %s: (%08x) %s", tmpPath, err, lpMsgBuf);
      LocalFree( lpMsgBuf);
      fatal_error( errMsg);
   }

   prima = (void*)GetProcAddress( lib, "prima");
   if ( prima == NULL)
      fatal_error( "Corrupted file PRIMGUTS.DLL");

   set_platform_data = (void*)GetProcAddress( lib, "set_platform_data");
   if ( set_platform_data == NULL)
      fatal_error( "Corrupted file PRIMGUTS.DLL");
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
   instance = hInstance;
   cmdShow  = nCmdShow;
   fill_prima_path();

   sprintf( tmpPath, "%sGuts;%sModules;", primaPath, primaPath);
   GetEnvironmentVariable("PATH", tmpPath+strlen(tmpPath), sizeof(tmpPath)-strlen(tmpPath)-1);
   SetEnvironmentVariable("PATH", tmpPath);

   load_prima_guts();
   set_platform_data( instance, cmdShow);
   exit( prima( primaPath, __argc, __argv));
   return 1;
}

