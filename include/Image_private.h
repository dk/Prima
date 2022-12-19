#ifndef Image_private_H
#define Image_private_H

#ifdef __cplusplus
extern "C" {
#endif

#undef  my
#undef  var
#undef inherited
#define inherited CDrawable->
#define my  ((( PImage) self)-> self)
#define var (( PImage) self)

#define VAR_MATRIX var->current_state.matrix

Bool
Image_read_pixel( Handle self, SV * pixel, ColorPixel *output );

Bool
Image_set_extended_data( Handle self, HV * profile);

void
Image_color2pixel( Handle self, Color color, Byte * pixel);

Handle
Icon_create_from_image( Handle self, int maskType );

#ifdef __cplusplus
}
#endif
#endif
