/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber			IBM PC Ver 0.1,	Jun. 1989    *
******************************************************************************
* Handle error reporting for the GIF library.				     *
******************************************************************************
* History:								     *
* 17 Jun 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/


#include <stdio.h>
#include "gif_lib.h"


#ifdef __cplusplus
extern "C" {
#endif


#define PROGRAM_NAME	"GIF_LIBRARY"

int _GifError = 0;

#ifdef SYSV
char *VersionStr =
        "Gif library module,\t\tEric S. Raymond\n\
	(C) Copyright 1997 Eric S. Raymond\n";
#else
char *VersionStr =
	PROGRAM_NAME
	"	IBMPC "
	GIF_LIB_VERSION
	"	Eric S. Raymond,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1997 Eric S. Raymond\n";
#endif /* SYSV */

/*****************************************************************************
* Return the last GIF error (0 if none) and reset the error.		     *
*****************************************************************************/
int GifLastError(void)
{
    int i = _GifError;

    _GifError = 0;

    return i;
}

#ifdef __cplusplus
}
#endif
