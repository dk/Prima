/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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
 */

/* Main executable for os/2 */
#define INCL_WIN
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>

static char primaPath[ _MAX_PATH];
static char tmpPath[ _MAX_PATH * 3];
static char tmpPath2[ _MAX_PATH * 3];
static int (*prima)( const char *, int, char **);

static void
fill_prima_path( void)
{
   PPIB pib = NULL;
   PTIB tib = NULL;
   char primaName[ _MAX_PATH];
   char primaDrive[ _MAX_DRIVE];
   char primaDir[ _MAX_DIR];

   DosGetInfoBlocks( &tib, &pib);
   DosQueryModuleName( pib-> pib_hmte, _MAX_PATH - 1, primaName);
   _splitpath( primaName, primaDrive, primaDir, NULL, NULL);
   _makepath( primaPath, primaDrive, primaDir, NULL, NULL);
}

static void
fatal_error( const char *error)
{
   HAB hab = WinInitialize(0);
   HMQ hmq = WinCreateMsgQueue( hab, 0);
   WinMessageBox( HWND_DESKTOP, NULLHANDLE, error, "Prima Fatal Error",
                  0, MB_OK | MB_ERROR);
   WinDestroyMsgQueue( hmq);
   WinTerminate( hab);
   exit( 1);
}

static void
load_prima_guts( void)
{
   char failureInfo[ 256];
   char errMsg[1024];
   HMODULE guts;
   APIRET rc;

   snprintf( tmpPath, _MAX_PATH * 3, "%s\\Guts\\primguts.dll", primaPath);
   rc = DosLoadModule( failureInfo, 256, tmpPath, &guts);
   if ( rc != 0) {
      snprintf( errMsg, 1024, "Error loading %s: %s", tmpPath, failureInfo);
      fatal_error( errMsg);
   }

   rc = DosQueryProcAddr( guts, 0, "prima", (PFN*)&prima);
   if ( rc != 0)
      fatal_error( "Corrupted file PRIMGUTS.DLL");
}

int
main( int argc, char **argv, char **arge)
{
   ULONG minor;
   DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, &minor, sizeof( minor));
   if ( minor < 30)
      fatal_error( "Required OS/2 version 3.0 or higher");
   fill_prima_path();

   tmpPath2[0] = '\0';
   DosQueryExtLIBPATH( tmpPath2, BEGIN_LIBPATH);
   snprintf( tmpPath, _MAX_PATH * 3, "%s\\Guts;%s\\Modules;%s;", primaPath, primaPath, tmpPath2);
   DosSetExtLIBPATH( tmpPath, BEGIN_LIBPATH);

   load_prima_guts();
   exit( prima( primaPath, argc, argv));
}

