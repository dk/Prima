#ifndef GIF_SUPPORT_H
#define GIF_SUPPORT_H

#include <gif_lib.h>

Bool __gif_load( int fd, const char *filename, PList imgInfo, Bool readAll);
Bool __gif_save( const char *filename, PList imgInfo);
Bool __gif_loadable( int fd, const char *filename, Byte *preread_buf, U32 buf_size);
Bool __gif_storable( const char *filename, PList imgInfo);
Bool __gif_getinfo( int fd, const char *filename, PList imgInfo, Bool readAll);
const char *__gif_geterror( char *errorMsgBuf, int bufLen);

extern ImgFormat gifFormat;

#endif
