#include "img_conv.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

static void
rotate90( PImage i, Byte * new_data, int new_line_size)
{
   int y;
   int w   = i->w;
   int pixel_size    = (i-> type & imBPP) / 8;
   int tail = i-> lineSize - w * pixel_size  ;
   Byte * src = i->data, * dst0;

   switch (i->type & imBPP) {
   case 1: return ;
   case 4: return ;

   case 8: 
      dst0 = new_data + i->w * new_line_size;
      for ( y = 0; y < i-> h; y++) {
         register int x = w;
         register Byte * dst = dst0++;
         while (x--) 
            *(dst -= new_line_size) = *src++;
         src += tail;
      }
      return;

   default: 
      dst0 = new_data + ( i-> w - 1) * new_line_size;
      new_line_size += pixel_size;
      for ( y = 0; y < i-> h; y++) {
         register int x = w;
         register Byte * dst = dst0;
         while (x--) {
            register int b = pixel_size;
            while ( b--) 
               *dst++ = *src++;
            dst -= new_line_size;
         }
         src += tail;
         dst0 += pixel_size;
      }
      return ;
   }     
}

static void
rotate180( PImage i, Byte * new_data)
{
   int y, bs2;
   int w          = i->w;
   int pixel_size = (i-> type & imBPP) / 8;
   int tail       = i-> lineSize - w * pixel_size  ;
   Byte * src = i->data, *dst = new_data + i-> h * i-> lineSize - tail - pixel_size;

   switch (i->type & imBPP) {
   case 1: return ;
   case 4: return ;

   case 8: 
      for ( y = 0; y < i-> h; y++) {
         register int x = w;
         while (x--) 
            *dst-- = *src++;
         src += tail;
         dst -= tail;
      }
      return;

   default: 
      bs2 = pixel_size + pixel_size;
      for ( y = 0; y < i-> h; y++) {
         register int x = w;
         while (x--) {
            register int b = bs2;
            while ( b--) 
               *dst++ = *src++;
            dst -= bs2;
         }
         src += tail;
         dst -= tail;
      }
      return ;
   }     
}

static void
rotate270( PImage i, Byte * new_data, int new_line_size)
{
   int y;
   int w   = i->w;
   int pixel_size    = (i-> type & imBPP) / 8;
   int tail = i-> lineSize - w * pixel_size  ;
   Byte * src = i->data, *dst0;

   switch (i->type & imBPP) {
   case 1: return ;
   case 4: return ;

   case 8: 
      dst0 = new_data + i->h - new_line_size - 1;
      for ( y = 0; y < i-> h; y++) {
         register int x = w;
         register Byte * dst = dst0--;
         while (x--) 
            *(dst += new_line_size) = *src++;
         src += tail;
      }
      return;

   default: 
      dst0 = new_data + ( i->h - 1) * pixel_size;
      new_line_size -= pixel_size;
      for ( y = 0; y < i-> h; y++) {
         register int x = w;
         register Byte * dst = dst0;
         while (x--) {
               register int b = pixel_size;
            while ( b--) 
               *dst++ = *src++;
            dst += new_line_size;
            
         }
         src += tail;
         dst0 -= pixel_size;
      }
      return ;
   }     
}

void 
img_rotate( Handle self, Byte * new_data, int degrees)
{
   PImage i = ( PImage ) self;

   switch ( degrees ) {
   case 90:
      rotate90(i, new_data, (( i-> h * ( i->type & imBPP) + 31) / 32) * 4);
      break;
   case 180:
      rotate180(i, new_data);
      break;
   case 270:
      rotate270(i, new_data, (( i-> h * ( i->type & imBPP) + 31) / 32) * 4);
      break;
   }
}

#ifdef __cplusplus
}
#endif

