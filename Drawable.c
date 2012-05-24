/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
 *
 * --------------------------------------------------------------------
 *  Parabolic spline procedures taken from TclTk's tkTrig.c
 *  
 *  Copyright (c) 1992-1994 The Regents of the University of California.
 *  Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" in TclTk distribution 
 * for information on usage and redistribution
 * of this code, and for a DISCLAIMER OF ALL WARRANTIES.
 * ---------------------------------------------------------------------
 *
 * $Id$
 */

#include "apricot.h"
#include "Drawable.h"
#include "Image.h"
#include <Drawable.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef my
#define inherited CComponent->
#define my  ((( PDrawable) self)-> self)
#define var (( PDrawable) self)

#define gpARGS            Bool inPaint = opt_InPaint
#define gpENTER(fail)     if ( !inPaint) if ( !my-> begin_paint_info( self)) return (fail)
#define gpLEAVE           if ( !inPaint) my-> end_paint_info( self)

void
Drawable_init( Handle self, HV * profile)
{
   dPROFILE;
   inherited init( self, profile);
   apc_gp_init( self);
   var-> w = var-> h = 0;
   my-> set_color        ( self, pget_i ( color));
   my-> set_backColor    ( self, pget_i ( backColor));
   my-> set_fillWinding  ( self, pget_B ( fillWinding));
   my-> set_fillPattern  ( self, pget_sv( fillPattern));
   my-> set_lineEnd      ( self, pget_i ( lineEnd));
   my-> set_lineJoin     ( self, pget_i ( lineJoin));
   my-> set_linePattern  ( self, pget_sv( linePattern));
   my-> set_lineWidth    ( self, pget_i ( lineWidth));
   my-> set_region       ( self, pget_H ( region));
   my-> set_rop          ( self, pget_i ( rop));
   my-> set_rop2         ( self, pget_i ( rop2));
   my-> set_textOpaque   ( self, pget_B ( textOpaque));
   my-> set_textOutBaseline( self, pget_B ( textOutBaseline));
   my-> set_splinePrecision( self, pget_i ( splinePrecision));
   if ( pexist( translate))
   {
      AV * av = ( AV *) SvRV( pget_sv( translate));
      Point tr = {0,0};
      SV ** holder = av_fetch( av, 0, 0);
      if ( holder) tr.x = SvIV( *holder); else warn("RTC0059: Array panic on 'translate'");
      holder = av_fetch( av, 1, 0);
      if ( holder) tr.y = SvIV( *holder); else warn("RTC0059: Array panic on 'translate'");
      my-> set_translate( self, tr);
   }
   SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::init");
   my-> set_font( self, Font_buffer);
   my-> set_palette( self, pget_sv( palette));
   CORE_INIT_TRANSIENT(Drawable);
}

static void
clear_font_abc_caches( Handle self)
{
   PList u;
   if (( u = var-> font_abc_unicode)) {
      int i;
      for ( i = 0; i < u-> count; i += 2) 
         free(( void*) u-> items[ i + 1]);
      plist_destroy( u);
      var-> font_abc_unicode = nil;
   } 
   if ( var-> font_abc_ascii) {
      free( var-> font_abc_ascii);
      var-> font_abc_ascii = nil;
   }
}

void
Drawable_done( Handle self)
{
   clear_font_abc_caches( self);
   apc_gp_done( self);
   inherited done( self);
}

void
Drawable_cleanup( Handle self)
{
   if ( is_opt( optInDrawInfo))
      my-> end_paint_info( self);
   if ( is_opt( optInDraw))
      my-> end_paint( self);
   inherited cleanup( self);
}

Bool
Drawable_begin_paint( Handle self)
{
   if ( var-> stage > csFrozen) return false;
   if ( is_opt( optInDrawInfo)) my-> end_paint_info( self);
   opt_set( optInDraw);
   var-> splinePrecision_saved = var-> splinePrecision;
   return true;
}

void
Drawable_end_paint( Handle self)
{
   clear_font_abc_caches( self);
   opt_clear( optInDraw);
   var-> splinePrecision = var-> splinePrecision_saved;
}

Bool
Drawable_begin_paint_info( Handle self)
{
   if ( var-> stage > csFrozen) return false;
   if ( is_opt( optInDraw))     return true;
   if ( is_opt( optInDrawInfo)) return false;
   opt_set( optInDrawInfo);
   var-> splinePrecision_saved = var-> splinePrecision;
   return true;
}

void
Drawable_end_paint_info( Handle self)
{
   clear_font_abc_caches( self);
   opt_clear( optInDrawInfo);
   var-> splinePrecision = var-> splinePrecision_saved;
}

void 
Drawable_set( Handle self, HV * profile)
{
   dPROFILE;
   if ( pexist( font))
   {
      SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::set");
      my-> set_font( self, Font_buffer);
      pdelete( font);
   }
   if ( pexist( translate))
   {
      AV * av = ( AV *) SvRV( pget_sv( translate));
      Point tr = {0,0};
      SV ** holder = av_fetch( av, 0, 0);
      if ( holder) tr.x = SvIV( *holder); else warn("RTC0059: Array panic on 'translate'");
      holder = av_fetch( av, 1, 0);
      if ( holder) tr.y = SvIV( *holder); else warn("RTC0059: Array panic on 'translate'");
      my-> set_translate( self, tr);
      pdelete( translate);
   }
   if ( pexist( width) && pexist( height)) {
      Point size;
      size. x = pget_i( width);
      size. y = pget_i( height);
      my-> set_size( self, size);
      pdelete( width);
      pdelete( height);
   }
   inherited set( self, profile);
}


Font *
Drawable_font_match( char * dummy, Font * source, Font * dest, Bool pick)
{
   if ( pick) 
      apc_font_pick( nilHandle, source, dest);
   else
      Drawable_font_add( nilHandle, source, dest);
   return dest;
}

Bool
Drawable_font_add( Handle self, Font * source, Font * dest)
{
   Bool useHeight = source-> height    != C_NUMERIC_UNDEF;
   Bool useWidth  = source-> width     != C_NUMERIC_UNDEF;
   Bool useSize   = source-> size      != C_NUMERIC_UNDEF;
   Bool usePitch  = source-> pitch     != C_NUMERIC_UNDEF;
   Bool useStyle  = source-> style     != C_NUMERIC_UNDEF;
   Bool useDir    = source-> direction != C_NUMERIC_UNDEF;
   Bool useName   = strcmp( source-> name, C_STRING_UNDEF) != 0;
   Bool useEnc    = strcmp( source-> encoding, C_STRING_UNDEF) != 0;

   /* assignning values */
   if ( dest != source) {
      if ( useHeight) dest-> height    = source-> height;
      if ( useWidth ) dest-> width     = source-> width;
      if ( useDir   ) dest-> direction = source-> direction;
      if ( useStyle ) dest-> style     = source-> style;
      if ( usePitch ) dest-> pitch     = source-> pitch;
      if ( useSize  ) dest-> size      = source-> size;
      if ( useName  ) strcpy( dest-> name, source-> name);
      if ( useEnc   ) strcpy( dest-> encoding, source-> encoding);
   }

   /* nulling dependencies */
   if ( !useHeight && useSize)
      dest-> height = 0;
   if ( !useWidth && ( usePitch || useHeight || useName || useSize || useDir || useStyle))
      dest-> width = 0;
   if ( !usePitch && ( useStyle || useName || useDir || useWidth))
      dest-> pitch = fpDefault;
   if ( useHeight)
      dest-> size = 0;
   if ( !useHeight && !useSize && ( dest-> height <= 0 || dest-> height > 16383)) 
      useSize = 1;

   /* validating entries */
   if ( dest-> height <= 0) dest-> height = 1;
      else if ( dest-> height > 16383 ) dest-> height = 16383;
   if ( dest-> width  <  0) dest-> width  = 1;
      else if ( dest-> width  > 16383 ) dest-> width  = 16383;
   if ( dest-> size   <= 0) dest-> size   = 1;
      else if ( dest-> size   > 16383 ) dest-> size   = 16383;
   if ( dest-> name[0] == 0)
      strcpy( dest-> name, "Default");
   if ( dest-> pitch < fpDefault || dest-> pitch > fpFixed)
      dest-> pitch = fpDefault;
   if ( dest-> direction == C_NUMERIC_UNDEF)
      dest-> direction = 0;
   if ( dest-> style == C_NUMERIC_UNDEF)
      dest-> style = 0;

   return useSize && !useHeight;
}


int
Drawable_get_paint_state( Handle self)
{
   if ( is_opt( optInDraw))
      return 1;
   else if ( is_opt( optInDrawInfo))
      return 2;
   else
      return 0;
}

int
Drawable_get_bpp( Handle self)
{
   gpARGS;
   int ret;
   gpENTER(0);
   ret = apc_gp_get_bpp( self);
   gpLEAVE;
   return ret;
}

SV *
Drawable_linePattern( Handle self, Bool set, SV * pattern)
{
   if ( set) {
      STRLEN len;
      unsigned char *pat = ( unsigned char *) SvPV( pattern, len);
      if ( len > 255) len = 255;
      apc_gp_set_line_pattern( self, pat, len);
   } else {
      unsigned char ret[ 256];
      int len = apc_gp_get_line_pattern( self, ret);
      return newSVpvn((char*) ret, len);
   }
   return nilSV;
}

Color
Drawable_get_nearest_color( Handle self, Color color)
{
   gpARGS;
   gpENTER(clInvalid);
   color = apc_gp_get_nearest_color( self, color);
   gpLEAVE;
   return color;
}

Point
Drawable_resolution( Handle self, Bool set, Point resolution)
{
   if ( set)
      croak("Attempt to write read-only property %s", "Drawable::resolution");
   resolution = apc_gp_get_resolution( self);
   return resolution;
}

SV *
Drawable_get_physical_palette( Handle self)
{
   gpARGS;
   int i, nCol;
   AV * av = newAV();
   PRGBColor r;

   gpENTER(newRV_noinc(( SV *) av));
   r = apc_gp_get_physical_palette( self, &nCol);
   gpLEAVE;

   for ( i = 0; i < nCol; i++) {
      av_push( av, newSViv( r[ i].b));
      av_push( av, newSViv( r[ i].g));
      av_push( av, newSViv( r[ i].r));
   }
   free( r);
   return newRV_noinc(( SV *) av);
}

SV *
Drawable_get_font_abc( Handle self, int first, int last, Bool unicode)
{
   int i;
   AV * av;
   PFontABC abc;

   if ( first < 0) first = 0;
   if ( last  < 0) last  = 255;
   if ( !unicode) {
      if ( first > 255) first = 255;
      if ( last  > 255) last  = 255;
   }

   if ( first > last)
     abc = nil;
   else {
     gpARGS;
     gpENTER( newRV_noinc(( SV *) newAV()));
     abc = apc_gp_get_font_abc( self, first, last, unicode );
     gpLEAVE;
   }

   av = newAV();
   if ( abc != nil) {
      for ( i = 0; i <= last - first; i++) {
         av_push( av, newSVnv( abc[ i]. a));
         av_push( av, newSVnv( abc[ i]. b));
         av_push( av, newSVnv( abc[ i]. c));
      }
      free( abc);
   }
   return newRV_noinc(( SV *) av);
}

SV *
Drawable_get_font_ranges( Handle self)
{
   int count = 0;
   unsigned long * ret;
   AV * av = newAV();
   gpARGS;
   
   gpENTER( newRV_noinc(( SV *) av));
   ret = apc_gp_get_font_ranges( self, &count);
   gpLEAVE;
   if ( ret) {
      int i;
      for ( i = 0; i < count; i++) 
         av_push( av, newSViv( ret[i]));
      free( ret);
   }
   return newRV_noinc(( SV *) av);
}


SV *
Drawable_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_gp_get_handle( self));
   return newSVpv( buf, 0);
}

int
Drawable_height( Handle self, Bool set, int height)
{
   Point p = my-> get_size( self);
   if ( !set)
      return p. y;
   p. y = height;
   my-> set_size( self, p);
   return height;
}

Point
Drawable_size ( Handle self, Bool set, Point size)
{
   if ( set)
      croak("Attempt to write read-only property %s", "Drawable::size");
   size. x = var-> w;
   size. y = var-> h;
   return size;
}

int
Drawable_width( Handle self, Bool set, int width)
{
   Point p = my-> get_size( self);
   if ( !set)
      return p. x;
   p. x = width;
   my-> set_size( self, p);
   return width;
}

Bool
Drawable_put_image_indirect( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
   Bool ok;
   if ( image == nilHandle) return false;
   if ( xLen == xDestLen && yLen == yDestLen) 
      ok = apc_gp_put_image( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);
   else    
      ok = apc_gp_stretch_image( self, image, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
   if ( !ok) perl_error();
   return ok;
}

Bool
Drawable_text_out( Handle self, SV * text, int x, int y)
{
   Bool ok;
   STRLEN dlen;
   char * c_text = SvPV( text, dlen);
   Bool   utf8 = prima_is_utf8_sv( text);
   if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
   ok = apc_gp_text_out( self, c_text, x, y, dlen, utf8);
   if ( !ok) perl_error();
   return ok;
}

Point *
Drawable_polypoints( SV * points, char * procName, int mod, int * n_points)
{
   AV * av;
   int i, count;
   Point * p;

   if ( !SvROK( points) || ( SvTYPE( SvRV( points)) != SVt_PVAV)) {
      warn("RTC0050: Invalid array reference passed to %s", procName);
      return nil;
   }
   av = ( AV *) SvRV( points);
   count = av_len( av) + 1;
   if ( count % mod) {
      warn("RTC0051: Drawable::%s: Number of elements in an array must be a multiple of %d",
           procName, mod);
      return nil;
   }
   count /= 2;
   if ( count < 2) return nil;
   if (!( p = allocn( Point, count))) return false;
   for ( i = 0; i < count; i++)
   {
       SV** psvx = av_fetch( av, i * 2, 0);
       SV** psvy = av_fetch( av, i * 2 + 1, 0);
       if (( psvx == nil) || ( psvy == nil)) {
          free( p);
          warn("RTC0052: Array panic on item pair %d on Drawable::%s", i, procName);
          return nil;
       }
       p[ i]. x = SvIV( *psvx);
       p[ i]. y = SvIV( *psvy);
   }
   *n_points = count;
   return p;
}

static Bool
polypoints( Handle self, SV * points, char * procName, int mod, Bool (*procPtr)(Handle,int,Point*))
{
   int count;
   Point * p;
   Bool ret = false;
   if (( p = Drawable_polypoints( points, procName, mod, &count))) {
      ret = procPtr( self, count, p);
      if ( !ret) perl_error();
      free( p);
   }
   return ret;
}

Bool
Drawable_polyline( Handle self, SV * points)
{
   return polypoints( self, points, "Drawable::polyline", 2, apc_gp_draw_poly);
}

Bool
Drawable_lines( Handle self, SV * points)
{
   return polypoints( self, points, "Drawable::lines", 4, apc_gp_draw_poly2);
}

Bool
Drawable_fillpoly( Handle self, SV * points)
{
   return polypoints( self, points, "Drawable::fillpoly", 2, apc_gp_fill_poly);
}

/*
 *--------------------------------------------------------------
 *
 * TkBezierScreenPoints --
 *
 *	Given four control points, create a larger set of XPoints
 *	for a Bezier spline based on the points.
 *
 * Results:
 *	The array at *xPointPtr gets filled in with numSteps XPoints
 *	corresponding to the Bezier spline defined by the four 
 *	control points.  Note:  no output point is generated for the
 *	first input point, but an output point *is* generated for
 *	the last input point.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
TkBezierScreenPoints(
    double control[],			/* Array of coordinates for four
					 * control points:  x0, y0, x1, y1,
					 * ... x3 y3. */
    int numSteps,			/* Number of curve points to
					 * generate.  */
    register Point *xPointPtr)		/* Where to put new points. */
{
    int i;
    double u, u2, u3, t, t2, t3;

    for (i = 1; i <= numSteps; i++, xPointPtr++) {
	t = ((double) i)/((double) numSteps);
	t2 = t*t;
	t3 = t2*t;
	u = 1.0 - t;
	u2 = u*u;
	u3 = u2*u;
	xPointPtr-> x =	control[0]*u3 + 3.0 * (control[2]*t*u2 + control[4]*t2*u) + control[6]*t3;
	xPointPtr-> y =	control[1]*u3 + 3.0 * (control[3]*t*u2 + control[5]*t2*u) + control[7]*t3;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkMakeBezierCurve --
 *
 *	Given a set of points, create a new set of points that fit
 *	parabolic splines to the line segments connecting the original
 *	points.  
 *
 *	Note: in spite of this procedure's name, it does *not* generate
 *	Bezier curves.  Since only three control points are used for
 *	each curve segment, not four, the curves are actually just
 *	parabolic.
 *
 * Results:
 *	xPoints array is always filled
 *	in.  The return value is the number of points placed in the
 *	array.  Note:  if the first and last points are the same, then
 *	a closed curve is generated.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
TkMakeBezierCurve(
    int *pointPtr,			/* Array of input coordinates:  x0,
					 * y0, x1, y1, etc.. */
    int numPoints,			/* Number of points at pointPtr. */
    int numSteps,			/* Number of steps to use for each
					 * spline segments (determines
					 * smoothness of curve). */
    Point xPoints[])			/* Array of Points to fill in (e.g.
					 * for display.  NULL means don't
					 * fill in any Points. */
{
    int closed, outputPoints, i;
    int numCoords = numPoints*2;
    double control[8];

    /*
     * If the curve is a closed one then generate a special spline
     * that spans the last points and the first ones.  Otherwise
     * just put the first point into the output.
     */

    if (!pointPtr) {
	/* Of pointPtr == NULL, this function returns an upper limit.
	 * of the array size to store the coordinates. This can be
	 * used to allocate storage, before the actual coordinates
	 * are calculated. */
	return 1 + numPoints * numSteps;
    }

    outputPoints = 0;
    if ((pointPtr[0] == pointPtr[numCoords-2])
	    && (pointPtr[1] == pointPtr[numCoords-1])) {
	closed = 1;
	control[0] = 0.5*pointPtr[numCoords-4] + 0.5*pointPtr[0];
	control[1] = 0.5*pointPtr[numCoords-3] + 0.5*pointPtr[1];
	control[2] = 0.167*pointPtr[numCoords-4] + 0.833*pointPtr[0];
	control[3] = 0.167*pointPtr[numCoords-3] + 0.833*pointPtr[1];
	control[4] = 0.833*pointPtr[0] + 0.167*pointPtr[2];
	control[5] = 0.833*pointPtr[1] + 0.167*pointPtr[3];
	control[6] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	control[7] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	if (xPoints != NULL) {
	    xPoints-> x = control[0];
            xPoints-> y = control[1];
	    TkBezierScreenPoints( control, numSteps, xPoints+1);
	    xPoints += numSteps+1;
	}
	outputPoints += numSteps+1;
    } else {
	closed = 0;
	if (xPoints != NULL) {
	    xPoints->x = pointPtr[0];
            xPoints->y = pointPtr[1];
	    xPoints += 1;
	}
	outputPoints += 1;
    }

    for (i = 2; i < numPoints; i++, pointPtr += 2) {
	/*
	 * Set up the first two control points.  This is done
	 * differently for the first spline of an open curve
	 * than for other cases.
	 */

	if ((i == 2) && !closed) {
	    control[0] = pointPtr[0];
	    control[1] = pointPtr[1];
	    control[2] = 0.333*pointPtr[0] + 0.667*pointPtr[2];
	    control[3] = 0.333*pointPtr[1] + 0.667*pointPtr[3];
	} else {
	    control[0] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	    control[1] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	    control[2] = 0.167*pointPtr[0] + 0.833*pointPtr[2];
	    control[3] = 0.167*pointPtr[1] + 0.833*pointPtr[3];
	}

	/*
	 * Set up the last two control points.  This is done
	 * differently for the last spline of an open curve
	 * than for other cases.
	 */

	if ((i == (numPoints-1)) && !closed) {
	    control[4] = .667*pointPtr[2] + .333*pointPtr[4];
	    control[5] = .667*pointPtr[3] + .333*pointPtr[5];
	    control[6] = pointPtr[4];
	    control[7] = pointPtr[5];
	} else {
	    control[4] = .833*pointPtr[2] + .167*pointPtr[4];
	    control[5] = .833*pointPtr[3] + .167*pointPtr[5];
	    control[6] = 0.5*pointPtr[2] + 0.5*pointPtr[4];
	    control[7] = 0.5*pointPtr[3] + 0.5*pointPtr[5];
	}

	/*
	 * If the first two points coincide, or if the last
	 * two points coincide, then generate a single
	 * straight-line segment by outputting the last control
	 * point.
	 */

	if (((pointPtr[0] == pointPtr[2]) && (pointPtr[1] == pointPtr[3]))
		|| ((pointPtr[2] == pointPtr[4])
		&& (pointPtr[3] == pointPtr[5]))) {
	    if (xPoints != NULL) {
                xPoints[0].x = control[6];
                xPoints[0].y = control[7];
		xPoints++;
	    }
	    outputPoints += 1;
	    continue;
	}

	/*
	 * Generate a Bezier spline using the control points.
	 */


	if (xPoints != NULL) {
	    TkBezierScreenPoints(control, numSteps, xPoints);
	    xPoints += numSteps;
	}
	outputPoints += numSteps;
    }
    return outputPoints;
}

#define STATIC_ARRAY_SIZE 200

static Bool
plot_spline( Handle self, int count, Point * points, Bool fill)
{
   Bool ret;
   int array_size;
   Point static_array[STATIC_ARRAY_SIZE], *array;
   
   array_size = TkMakeBezierCurve( NULL, count, var-> splinePrecision, NULL);
   if ( array_size >= STATIC_ARRAY_SIZE) {
      if ( !( array = malloc( array_size * sizeof( Point)))) {
         warn("Not enough memory");
         return false;
      }
   } else 
      array = static_array;

   array_size = TkMakeBezierCurve((int*) points, count, var-> splinePrecision, array);

   if ( fill && ( my-> fillpoly == Drawable_fillpoly)) {
      ret = apc_gp_fill_poly( self, array_size, array);
      if ( !ret) perl_error();
   } else if ( !fill && ( my-> polyline == Drawable_polyline)) {
      ret = apc_gp_draw_poly( self, array_size, array);
      if ( !ret) perl_error();
   } else {
      int i;
      AV * av = newAV();
      SV * sv = newRV(( SV*) av);
      for ( i = 0; i < array_size; i++) {
         av_push( av, newSViv( array[i]. x));
         av_push( av, newSViv( array[i]. y));
      }
      ret = fill ? 
         my-> fillpoly( self, sv) :
         my-> polyline( self, sv);
      sv_free( sv);
   }

   if ( array != static_array) free( array);
   
   return ret; 
}  

static Bool
spline( Handle self, int count, Point * points)
{
   return plot_spline( self, count, points, false);
}

static Bool
fill_spline( Handle self, int count, Point * points)
{
   return plot_spline( self, count, points, true);
}

Bool
Drawable_spline( Handle self, SV * points)
{
   return polypoints( self, points, "Drawable::spline", 2, spline);
}

Bool
Drawable_fill_spline( Handle self, SV * points)
{
   return polypoints( self, points, "Drawable::fill_spline", 2, fill_spline);
}

SV * 
Drawable_render_spline( SV * obj, SV * points, int precision)
{
   int i, n_p, array_size;
   Point static_array[STATIC_ARRAY_SIZE], *array, *p;
   AV * av;
   
   if ( precision < 0) {
      Handle self;
      self = gimme_the_mate( obj);
      precision = self ? var-> splinePrecision : 24;
   }

   av = newAV();
   p = Drawable_polypoints( points, "Drawable::render_spline", 2, &n_p);
   if ( p) {
      array_size = TkMakeBezierCurve( NULL, n_p, precision, NULL);
      if ( array_size >= STATIC_ARRAY_SIZE) {
         if ( !( array = malloc( array_size * sizeof( Point)))) {
            warn("Not enough memory");
	    free( p);
            return newRV_noinc(( SV *) av);
         }
      } else 
        array = static_array;

      array_size = TkMakeBezierCurve((int*) p, n_p, precision, array);
      for ( i = 0; i < array_size; i++) {
         av_push( av, newSViv( array[i]. x));
         av_push( av, newSViv( array[i]. y));
      }
      if ( array != static_array) free( array);
      free( p);
   }

   return newRV_noinc(( SV *) av);
}

int
Drawable_get_text_width( Handle self, SV * text, Bool addOverhang)
{
   gpARGS;
   int res;
   STRLEN dlen;
   char * c_text = SvPV( text, dlen);
   Bool   utf8 = prima_is_utf8_sv( text);
   if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
   gpENTER(0);
   res = apc_gp_get_text_width( self, c_text, dlen, addOverhang, utf8);
   gpLEAVE;
   return res;
}

SV *
Drawable_get_text_box( Handle self, SV * text)
{
   gpARGS;
   Point * p;
   AV * av;
   int i;
   STRLEN dlen;
   char * c_text = SvPV( text, dlen);
   Bool   utf8 = prima_is_utf8_sv( text);
   if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
   gpENTER( newRV_noinc(( SV *) newAV()));
   p = apc_gp_get_text_box( self, c_text, dlen, utf8);
   gpLEAVE;

   av = newAV();
   if ( p) {
      for ( i = 0; i < 5; i++) {
         av_push( av, newSViv( p[ i]. x));
         av_push( av, newSViv( p[ i]. y));
      };
      free( p);
   }
   return newRV_noinc(( SV *) av);
}

static PFontABC
query_abc_range( Handle self, TextWrapRec * t, unsigned int base)
{
   PFontABC abc;

   /* find if present in cache */
   if ( t-> utf8_text) {
      if ( *(t-> unicode)) {
         int i;
         PList p;
         if (( p = *(t-> unicode)))
            for ( i = 0; i < p-> count; i += 2)
               if (( unsigned int) p-> items[ i] == base)
                  return ( PFontABC) p-> items[i + 1];
      }
   } else
      if ( *( t-> ascii)) return *(t-> ascii);

   /* query ABC information */
   if ( !self) {
      abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, t-> utf8_text);
      if ( !abc) return nil;
   } else if ( my-> get_font_abc == Drawable_get_font_abc) {
      gpARGS;
      gpENTER(nil);
      abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, t-> utf8_text);
      gpLEAVE;
      if ( !abc) return nil;
   } else {
      SV * sv;
      if ( !( abc = malloc( 256 * sizeof( FontABC)))) return nil;
      sv = my-> get_font_abc( self, base * 256, base * 256 + 255, t-> utf8_text);
      if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
         AV * av = ( AV*) SvRV( sv);
         int i, j = 0, n = av_len( av) + 1;
         if ( n > 256 * 3) n = 256 * 3;
         n = ( n / 3) * 3;
         if ( n < 256) memset( abc, 0, 256 * sizeof( FontABC));
         for ( i = 0; i < n; i += 3) {
            SV ** holder = av_fetch( av, i, 0);
            if ( holder) abc[j]. a = ( float) SvNV( *holder);
            holder = av_fetch( av, i + 1, 0);
            if ( holder) abc[j]. b = ( float) SvNV( *holder);
            holder = av_fetch( av, i + 2, 0);
            if ( holder) abc[j]. c = ( float) SvNV( *holder);
            j++;
         }
      } else
         memset( abc, 0, 256 * sizeof( FontABC));
      sv_free( sv);
   }

   /* store in cache */
   if ( t-> utf8_text) {
      PList p;
      if ( !*(t-> unicode))
         *(t-> unicode) = plist_create( 8, 8);
      if (( p = *(t-> unicode))) {
         list_add( p, ( Handle) base);
         list_add( p, ( Handle) abc);
      } else {
         free( abc);
         return nil;
      }
   } else
      *(t-> ascii) = abc;

   return abc;
}

static Bool
precalc_abc_buffer( PFontABC src, float * width, PFontABC dest)
{
   int i;
   if ( !dest) return false;
   for ( i = 0; i < 256; i++) {
      width[i] = src[i]. a + src[i]. b + src[i]. c;
      dest[i]. a = ( src[i]. a < 0) ? - src[i]. a : 0;
      dest[i]. b = src[i]. b;
      dest[i]. c = ( src[i]. c < 0) ? - src[i]. c : 0;
   }
   return true;
}

static Bool
add_wrapped_text( TextWrapRec * t, int start, int utfstart, int end, int utfend,
                  int tildeIndex, int * tildePos, int * tildeLPos, int * tildeLine,
                  char *** lArray, int * lSize)
{
   int l = end - start;
   char *c = nil;
   if (!( t-> options & twReturnChunks)) {
      if ( !( c = allocs( l + 1))) return false;
      memcpy( c, t-> text + start, l);
      c[ l] = 0;
   }                                               
   if ( tildeIndex >= 0 && tildeIndex >= start && tildeIndex < end) {                                               
      *tildeLine = t-> t_line = t-> count;
      *tildePos = *tildeLPos = tildeIndex - start;
      if ( tildeIndex == end - 1) {
         t-> t_line++;
         tildeLPos = 0;
      }
   }
   if ( t-> count == *lSize) {
      char ** n = allocn( char*, *lSize + 16);
      if ( !n) return false;
      memcpy( n, *lArray, sizeof( char*) * (*lSize));
      *lSize += 16;
      free( *lArray);
      *lArray = n;
   }
   if ( t-> options & twReturnChunks) {
      (*lArray)[ t-> count++] = INT2PTR(char*,utfstart);
      (*lArray)[ t-> count++] = INT2PTR(char*,utfend - utfstart);
   } else
      (*lArray)[ t-> count++] = c;
   return true;
}
   
char **
Drawable_do_text_wrap( Handle self, TextWrapRec * t)
{
   unsigned int base = 0x10000000;
   float width[256];
   FontABC abc[256];
   int start = 0, utf_start = 0, split_start = -1, split_end = -1, i, utf_p, utf_split = -1;
   float w = 0, inc = 0;
   char **ret;
   Bool wasTab = 0, reassign_w = 1;
   Bool doWidthBreak = t-> width >= 0;
   int tildeIndex = -100, tildeLPos = 0, tildeLine = 0, tildePos = 0, tildeOffset = 0, lSize = 16;
   int spaceWidth = 0, spaceC = 0, spaceOK = 0;

#define lAdd(end, utfend) \
   if ( !add_wrapped_text( t, start, utf_start, end, utfend, tildeIndex, \
        &tildePos, &tildeLPos, &tildeLine, &ret, &lSize)) return ret;\
   start = end; \
   utf_start = utfend; \
   if (( t-> options & twReturnFirstLineLength) == twReturnFirstLineLength) return ret
      
   t-> count = 0;
   if (!( ret = allocn( char*, lSize))) return nil;

   /* determining ~ character location */
   if ( t-> options & twCalcMnemonic) 
      for ( i = 0; i < t-> textLen - 1; i++)
         if ( t-> text[ i] == '~') {
            unsigned char c = t-> text[ i + 1];
            if ( c == '~' || c < ' ') {
               i++;
               continue;
            } else {
               tildeIndex = i;
               break;
            }
         }

      
   /* process UV chars */
   for ( i = 0, utf_p = 0; i < t-> textLen; utf_p++) {
      UV uv;
      float winc;
      int p = i;
      
      if ( t-> utf8_text) {
         STRLEN len;
#if PERL_PATCHLEVEL >= 16
         uv = utf8_to_uvchr_buf(( U8*) t-> text + i, ( U8*) t-> text + t-> textLen, &len);
#else	 
         uv = utf8_to_uvchr(( U8*) t-> text + i, &len);
#endif
         i += len;
	 if ( len == 0 ) break;
      } else
         uv = (( unsigned char *)(t-> text))[i++];

      if ( uv / 256 != base) 
         if ( !precalc_abc_buffer( query_abc_range( self, t, base = uv / 256), width, abc))
            return ret;
      if ( reassign_w) w = abc[ uv & 0xff]. a;
      reassign_w = 0;
      
      switch ( uv ) {
      case '\t':
         split_start = p; split_end = i; utf_split = utf_p;
         if (!( t-> options & twCalcTabs)) goto _default;
         if ( t-> options & twSpaceBreak) {
            lAdd( p, utf_p); 
            start = i;
            utf_start++;
            reassign_w = 1;
            continue;
         }
         if ( !spaceOK) {
            PFontABC s = query_abc_range( self, t, 0);
            if ( !s) return ret;
            spaceWidth = (s[' '].a + s[' '].b + s[' '].c) * t-> tabIndent; 
            spaceC     = (s[' '].c < 0) ? - s[' ']. c : 0;
            spaceOK = 1;
         }
         winc = spaceWidth;
         inc  = spaceC;
         wasTab = true;
         break;
      case '\n':
      case 0x2028:
      case 0x2029:
         split_start = p; split_end = i; utf_split = utf_p; 
         if (!( t-> options & twNewLineBreak)) goto _default;
         lAdd( p, utf_p); 
         start = i;
         utf_start++;
         reassign_w = 1;
         continue;
      case ' ':
         split_start = p; split_end = i; utf_split = utf_p; 
         if (!( t-> options & twSpaceBreak)) goto _default;
         lAdd( p, utf_p); 
         start = i;
         utf_start++;
         reassign_w = 1;
         continue;
      case '~':
         if ( p == tildeIndex ) {
            tildeOffset = w;
            inc = winc = 0;
            break;
         }
      _default: default:
         winc = width[ uv & 0xff];
         inc  = abc[ uv & 0xff]. c;
      }

      if ( doWidthBreak && w + winc + inc > t-> width) {
         if (( p == start) || (( p == start - 1) && ( p - 1 == tildeIndex))) {
           /* case when even single char cannot be fit in  */
            if ( t-> options & twBreakSingle) {
               /* do not return anything in this case */
               int j;
               if (!( t-> options & twReturnChunks)) {
                  for ( j = 0; j < t-> count; j++) free( ret[ j]);
                  ret[ 0] = duplicate_string("");
               }
               t-> count = 0;
               return ret;
            } 
            /* or push this character disregarding the width */
            lAdd( i, utf_p + 1);
         } else { /* normal break condition */
            /* checking if break was at word boundary */ 
            if ( t-> options & twWordBreak) {
               if ( start <= split_start) {
                  lAdd( split_start, utf_split );
                  i = start = split_end;
                  utf_start = utf_split + 1;
                  utf_p = utf_split;
                  w = 0;
                  continue;
               } else if ( t-> options & twBreakSingle) { 
                  /* cannot be split, return nothing */
                  int j;
                  if (!( t-> options & twReturnChunks)) {
                     for ( j = 0; j < t-> count; j++) free( ret[ j]);
                     ret[ 0] = duplicate_string("");
                  }
                  t-> count = 0;
                  return ret;
               } 
            }
            /* repeat again */
            lAdd( p, utf_p );
            i = start = p;
            utf_start = utf_p;
            utf_p--;
         }
         w = 0;
         continue;
      } else
         w += winc;
   }

   /* adding or skipping last line */
   if ( t-> textLen - start > 0 || t-> count == 0) lAdd( t-> textLen, t-> utf8_textLen);
    
   /* removing ~ and determining it's location */
   if ( tildeIndex >= 0 && !(t-> options & twReturnChunks)) {
      PFontABC abc;
      char *l = ret[ tildeLine];
      t-> t_char = t-> text + tildePos + 1;
      if ( t-> options & twCollapseTilde)
         memmove( l + tildePos, l + tildePos + 1, strlen( l) - tildePos);
      abc = query_abc_range( self, t, 0) + '~';
      w = tildeOffset;
      t-> t_start = w - 1;
      t-> t_end   = w + abc->a + abc->b + abc->c;
   } else {
      t-> t_start = t-> t_end = t-> t_line = C_NUMERIC_UNDEF;
   }

   /* expanding tabs */
   if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab) {
      for ( i = 0; i < t-> count; i++) {
         int tabs = 0, len = 0;
         char *substr = ret[ i], *n;
         while (*substr) { 
            if ( *substr == '\t') tabs++; 
            substr++; 
            len++; 
         }
         if ( tabs == 0) continue;
         if ( !( n = allocs( len + tabs * t-> tabIndent + 1)))
            return ret;
         len = 0;
         substr = ret[ i];
         while ( *substr) {
            if ( *substr == '\t') {
               int j = t-> tabIndent;
               while ( j--) n[ len++] = ' ';
            } else
               n[ len++] = *substr;
            substr++;
         }
         free( ret[ i]);
         n[ len] = 0;
         ret[ i] = n;
      }
   }

   return ret;
}

SV*
Drawable_text_wrap( Handle self, SV * text, int width, int options, int tabIndent)
{
   TextWrapRec t;
   Bool retChunks;
   char** c;
   int i;
   AV * av;
   STRLEN tlen;

   t. text      = SvPV( text, tlen);
   t. utf8_text = prima_is_utf8_sv( text);
   if ( t. utf8_text) {
      t. utf8_textLen = prima_utf8_length( t. text);
      t. textLen = utf8_hop(( U8*) t. text, t. utf8_textLen) - (U8*) t. text; 
   } else {
      t. utf8_textLen = t. textLen = tlen;
   }
   t. width     = ( width < 0) ? 0 : width;
   t. tabIndent = ( tabIndent < 0) ? 0 : tabIndent;
   t. options   = options;
   retChunks    = t. options & twReturnChunks;
   t. ascii     = &var-> font_abc_ascii;
   t. unicode   = &var-> font_abc_unicode;
   t. t_char    = nil;

   c = Drawable_do_text_wrap( self, &t);

   if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) {
      IV rlen = 0;
      if ( c) {
         if ( t. count > 0) rlen = PTR2IV(c[1]);
         free( c);
      }
      return newSViv( rlen);
   }

   if ( !c) return nilSV;

   av = newAV();
   for ( i = 0; i < t. count; i++) {
      SV * sv = retChunks ? newSViv( PTR2IV(c[i])) : newSVpv( c[ i], 0);
      if ( !retChunks) { 
          if ( t. utf8_text) SvUTF8_on( sv);
          free( c[i]);
      }
      av_push( av, sv);
   }

   free( c);

   if  ( t. options & ( twCalcMnemonic | twCollapseTilde)) {
      HV * profile = newHV();
      SV * sv_char;
      if ( t. t_char) {
         STRLEN len = t. utf8_text ? utf8_hop(( U8*) t. t_char, 1) - ( U8*) t. t_char : 1;
         sv_char = newSVpv( t. t_char, len);
         if ( t. utf8_text) SvUTF8_on( sv_char);
         pset_i( tildeStart, t. t_start);
         pset_i( tildeEnd,   t. t_end);
         pset_i( tildeLine,  t. t_line);
      } else {
         sv_char = newSVsv( nilSV);
         pset_sv( tildeStart, nilSV);
         pset_sv( tildeEnd,   nilSV);
         pset_sv( tildeLine,  nilSV);
      }
      pset_sv_noinc( tildeChar, sv_char);
      av_push( av, newRV_noinc(( SV *) profile));
   }

   return newRV_noinc(( SV *) av);
}


PRGBColor
read_palette( int * palSize, SV * palette)
{
   AV * av;
   int i, count;
   Byte * buf;

   if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV)) {
      *palSize = 0;
      return nil;
   }
   av = (AV *) SvRV( palette);
   count = av_len( av) + 1;
   *palSize = count / 3;
   count = *palSize * 3;
   if ( count == 0) return nil;

   if ( !( buf = allocb( count))) return nil;

   for ( i = 0; i < count; i++)
   {
      SV **itemHolder = av_fetch( av, i, 0);
      if ( itemHolder == nil)
         return ( PRGBColor) buf;
      buf[ i] = SvIV( *itemHolder);
   }

   return ( PRGBColor) buf;
}

Color
Drawable_backColor( Handle self, Bool set, Color color)
{
   if (!set) return apc_gp_get_back_color( self);
   apc_gp_set_back_color( self, color);
   return color;
}

Color
Drawable_color( Handle self, Bool set, Color color)
{
   if (!set) return apc_gp_get_color( self);
   apc_gp_set_color( self, color);
   return color;
}

Rect
Drawable_clipRect( Handle self, Bool set, Rect clipRect)
{
   if ( !set)
      return apc_gp_get_clip_rect( self);
   apc_gp_set_clip_rect( self, clipRect);
   return clipRect;
}

Bool
Drawable_fillWinding( Handle self, Bool set, Bool fillWinding)
{
   if (!set) return apc_gp_get_fill_winding( self);
   apc_gp_set_fill_winding( self, fillWinding);
   return fillWinding;
}

int
Drawable_lineEnd( Handle self, Bool set, int lineEnd)
{
   if (!set) return apc_gp_get_line_end( self);
   apc_gp_set_line_end( self, lineEnd);
   return lineEnd;
}

int
Drawable_lineJoin( Handle self, Bool set, int lineJoin)
{
   if (!set) return apc_gp_get_line_join( self);
   apc_gp_set_line_join( self, lineJoin);
   return lineJoin;
}

int
Drawable_lineWidth( Handle self, Bool set, int lineWidth)
{
   if (!set) return apc_gp_get_line_width( self);
   apc_gp_set_line_width( self, lineWidth);
   return lineWidth;
}

SV *
Drawable_palette( Handle self, Bool set, SV * palette)
{
   int colors;
   if ( var-> stage > csFrozen) return nilSV;
   colors = var-> palSize;
   if ( set) {
      free( var-> palette);
      var-> palette = read_palette( &var-> palSize, palette);
      if ( colors == 0 && var-> palSize == 0) return nilSV; /* do not bother apc */
      apc_gp_set_palette( self);
   } else {
      AV * av = newAV();
      int i;
      Byte * pal = ( Byte*) var-> palette;
      for ( i = 0; i < colors * 3; i++) av_push( av, newSViv( pal[ i]));
      return newRV_noinc(( SV *) av);
   }
   return nilSV;
}

SV *
Drawable_pixel( Handle self, Bool set, int x, int y, SV * color)
{
   if (!set)
      return newSViv( apc_gp_get_pixel( self, x, y));
   apc_gp_set_pixel( self, x, y, SvIV( color));
   return nilSV;
}

Handle
Drawable_region( Handle self, Bool set, Handle mask)
{
   if ( var-> stage > csFrozen) return nilHandle;

   if ( set) {
      if ( mask && !kind_of( mask, CImage)) {
         warn("RTC005A: Illegal object reference passed to Drawable::region");
         return nilHandle;
      }

      if ( mask && (( PImage( mask)-> type & imBPP) != imbpp1)) {
         Handle i = CImage( mask)-> dup( mask);
         ++SvREFCNT( SvRV( PImage( i)-> mate));
         CImage( i)-> set_conversion( i, ictNone);
         CImage( i)-> set_type( i, imBW);
         apc_gp_set_region( self, i);
         --SvREFCNT( SvRV( PImage( i)-> mate));
         Object_destroy( i);
      } else
         apc_gp_set_region( self, mask);

   } else if ( apc_gp_get_region( self, nilHandle)) {
      HV * profile = newHV();
      Handle i = Object_create( "Prima::Image", profile);
      sv_free(( SV *) profile);
      apc_gp_get_region( self, i);
      --SvREFCNT( SvRV((( PAnyObject) i)-> mate));
      return i;
   }

   return nilHandle;
}

int
Drawable_rop( Handle self, Bool set, int rop)
{
   if (!set) return apc_gp_get_rop( self);
   apc_gp_set_rop( self, rop);
   return rop;
}

int
Drawable_rop2( Handle self, Bool set, int rop2)
{
   if (!set) return apc_gp_get_rop2( self);
   apc_gp_set_rop2( self, rop2);
   return rop2;
}

int
Drawable_splinePrecision( Handle self, Bool set, int splinePrecision)
{
   if ( !set) return var-> splinePrecision;
   if ( splinePrecision < 1) return -1;
   var-> splinePrecision = splinePrecision;
   return splinePrecision;
}

Bool
Drawable_textOpaque( Handle self, Bool set, Bool opaque)
{
   if (!set) return apc_gp_get_text_opaque( self);
   apc_gp_set_text_opaque( self, opaque);
   return opaque;
}

Bool
Drawable_textOutBaseline( Handle self, Bool set, Bool textOutBaseline)
{
   if (!set) return apc_gp_get_text_out_baseline( self);
   apc_gp_set_text_out_baseline( self, textOutBaseline);
   return textOutBaseline;
}

Point
Drawable_translate( Handle self, Bool set, Point translate)
{
   if (!set) return apc_gp_get_transform( self);
   apc_gp_set_transform( self, translate. x, translate. y);
   return translate;
}

SV *
Drawable_fillPattern( Handle self, Bool set, SV * svpattern)
{
   int i;
   if ( !set) {
      AV * av;
      FillPattern * fp = apc_gp_get_fill_pattern( self);
      if ( !fp) return nilSV;
      av = newAV();
      for ( i = 0; i < 8; i++) av_push( av, newSViv(( int) (*fp)[i]));
      return newRV_noinc(( SV *) av);
   } else {
      if ( SvROK( svpattern) && ( SvTYPE( SvRV( svpattern)) == SVt_PVAV)) {
          FillPattern fp;
          AV * av = ( AV *) SvRV( svpattern);
          if ( av_len( av) != 7) {
             warn("RTC0056: Illegal fillPattern passed to Drawable::fillPattern");
             return nilSV;
          }
          for ( i = 0; i < 8; i++) {
            SV ** holder = av_fetch( av, i, 0);
            if ( !holder) {
               warn("RTC0057: Array panic on Drawable::fillPattern");
               return nilSV;
            }
            fp[ i] = SvIV( *holder);
         }
         apc_gp_set_fill_pattern( self, fp);
      } else {
         int id = SvIV( svpattern);
         if (( id < 0) || ( id > fpMaxId)) {
            warn("RTC0058: fillPattern index out of range passed to Drawable::fillPattern");
            return nilSV;
         }
         apc_gp_set_fill_pattern( self, fillPatterns[ id]);
      }
   }
   return nilSV;
}

Font
Drawable_get_font( Handle self)
{
   return var-> font;
}

void
Drawable_set_font( Handle self, Font font)
{
   clear_font_abc_caches( self);
   apc_font_pick( self, &font, &var-> font);
   apc_gp_set_font( self, &var-> font);
}


#ifdef __cplusplus
}
#endif
