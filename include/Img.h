#include "gbm.h"

#define ieOK                  GBM_ERR_OK
#define ieError               240
#define ieNotSupported        GBM_ERR_NOT_SUPP
#define ieFileNotFound        GBM_ERR_NOT_FOUND
#define ieInvalidType         GBM_ERR_BAD_ARG
#define ieInvalidOptions      GBM_ERR_BAD_OPTION

#define isRangeLo        0
#define isRangeHi        1
#define isMean           2
#define isVariance       3
#define isStdDev         4
#define isMaxIndex       4

#define itUnknown (-1)
#define itBMP  0
#define itGIF  1
#define itPCX  2
#define itTIF  3
#define itTGA  4
#define itLBM  5
#define itVID  6
#define itPGM  7
#define itPPM  8
#define itKPS  9
#define itIAX  10
#define itXBM  11
#define itSPR  12
#define itPSG  13
#define itGEM  14
#define itCVP  15
#define itJPG  16
#define itPNG  17

extern int     image_guess_type         ( int file);
