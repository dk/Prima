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
Icon_create_from_image( Handle self, int maskType, SV * mask_fill );

Color
Image_premultiply_color( Handle self, int rop, Color color);

Bool
Image_stroke_primitive( Handle self, char * method, ...);

Bool
Image_fill_rect( Handle self, double x1, double y1, double x2, double y2);

Bool
Image_fill_poly( Handle self, int n_pts, NPoint *pts);

SV*
Application_fonts( Handle self, char * name, char * encoding);

SV*
Application_font_encodings( Handle self);

#ifdef __cplusplus
}
#endif
#endif
