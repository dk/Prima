/*
      stock.c

      Stock objects - pens, brushes, fonts
*/

#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "win32\win32guts.h"
#include <gbm.h>
#include "Window.h"

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

// Stylus section
PDCStylus
stylus_alloc( PStylus data)
{
   Bool extPen = data-> extPen. actual;
   PDCStylus ret = hash_fetch( stylusMan, data, sizeof( Stylus) - ( extPen ? 0 : sizeof( EXTPEN)));
   if ( ret == nil) {
      LOGPEN * p;
      ret = malloc( sizeof( DCStylus));
      memcpy( &ret-> s, data, sizeof( Stylus));
      ret-> refcnt = 0;
      p = &ret-> s. pen;
      if ( !extPen) {
         if ( !( ret-> hpen = CreatePenIndirect( p))) apiErr;
      } else {
         LOGBRUSH pb = { BS_SOLID, ret-> s. pen. lopnColor, 0};
         if ( !( ret-> hpen   = ExtCreatePen( ret-> s. extPen. style, p-> lopnWidth. x, &pb,
            ret-> s. extPen. patResource-> styleCount,
            ret-> s. extPen. patResource-> dotsPtr
         ))) apiErr;
      }
      if ( ret-> s. brush. lb. lbStyle == BS_DIBPATTERNPT) {
         int i;
         for ( i = 0; i < 8; i++) bmiHatch. bmiData[ i * 4] = ret-> s. brush. pattern[ i];
         bmiHatch. bmiColors[ 0]. rgbBlue  =  ( ret-> s. brush. backColor & 0xFF);
         bmiHatch. bmiColors[ 0]. rgbGreen = (( ret-> s. brush. backColor >> 8) & 0xFF);
         bmiHatch. bmiColors[ 0]. rgbRed   = (( ret-> s. brush. backColor >> 16) & 0xFF);
         bmiHatch. bmiColors[ 1]. rgbRed   =  ( ret-> s. pen. lopnColor & 0xFF);
         bmiHatch. bmiColors[ 1]. rgbGreen = (( ret-> s. pen. lopnColor >> 8) & 0xFF);
         bmiHatch. bmiColors[ 1]. rgbBlue  = (( ret-> s. pen. lopnColor >> 16) & 0xFF);
      }
      if ( !( ret-> hbrush = CreateBrushIndirect( &ret-> s. brush. lb))) apiErr;
      hash_store( stylusMan, &ret-> s, sizeof( Stylus) - ( extPen ? 0 : sizeof( EXTPEN)), ret);
   }
   ret-> refcnt++;
   return ret;
}

void
stylus_free( PDCStylus res, Bool permanent)
{
   if ( !res || --res-> refcnt > 0) return;
   if ( !permanent) {
      res-> refcnt = 0;
      return;
   }
   if ( res-> hpen)   DeleteObject( res-> hpen);
   if ( res-> hbrush) DeleteObject( res-> hbrush);
   hash_delete( stylusMan, &res-> s, sizeof( Stylus) - ( res-> s. extPen. actual ? 0 : sizeof( EXTPEN)), true);
}

void
stylus_change( Handle self)
{
   PDCStylus p;
   PDCStylus newP;
   if ( is_apt( aptDCChangeLock)) return;
   p    = sys stylusResource;
   newP = stylus_alloc( &sys stylus);
   if ( p != newP) {
      sys stylusResource = newP;
      sys stylusFlags = 0;
   }
   stylus_free( p, false);
}

static Bool _st_cleaner( PDCStylus s, int keyLen, void * key, void * dummy) {
   if ( s-> refcnt <= 0) stylus_free( s, true);
   return false;
}

void
stylus_clean()
{
   hash_first_that( stylusMan, _st_cleaner, nil, nil, nil);
}

Bool
stylus_extpenned( PStylus s, int excludeFlags)
{
   Bool ext = false;
   if ( s-> pen. lopnWidth. x > 1) {
       if (( s-> pen. lopnStyle != PS_SOLID) && ( s-> pen. lopnStyle != PS_NULL))
          return true;
   } else if ( s-> pen. lopnStyle == PS_USERSTYLE)
      return true;
   if ( !( excludeFlags & exsLinePattern))
      ext |= s-> pen. lopnStyle == PS_USERSTYLE;
   if ( !( excludeFlags & exsLineEnd))
      ext |= s-> extPen. lineEnd != PS_ENDCAP_ROUND;
   return ext;
}

Bool
stylus_complex( PStylus s, HDC dc)
{
   int rop;
   if ( s-> brush. lb. lbStyle == BS_DIBPATTERNPT)
      return true;
   if (( s-> pen. lopnStyle != PS_SOLID) &&
       ( s-> pen. lopnStyle != PS_NULL))
      return true;
   rop = GetROP2( dc);
   if (( rop != R2_COPYPEN) &&
       ( rop != R2_WHITE  ) &&
       ( rop != R2_NOP    ) &&
       ( rop != R2_BLACK  )) return true;
   return false;
}


DWORD
stylus_get_extpen_style( PStylus s)
{
   return s-> extPen. lineEnd | s-> pen. lopnStyle | PS_GEOMETRIC;
}

PPatResource
patres_fetch( DWORD pattern)
{
   PPatResource r = hash_fetch( patMan, &pattern, sizeof( DWORD));
   DWORD storage[ 16];
   int i, cnt, idx, last;
   DWORD pat = pattern;
   if ( r)
      return r;
   if ( pattern == 0)
      croak(__FILE__);
   last = 2;
   idx  = -1;

   while(( pattern & ( 1 << 15)) == 0) pattern <<= 1;

   for ( i = 0; i < 16; i++) {
      int mark = ( pattern & ( 1 << ( 15 - i))) ? 1 : 0;
      if ( mark == last)
         cnt++;
      else {
			if ( idx >= 0) storage[ idx] = cnt;
			idx++;
         cnt             = mark ? 0 : 2;
         last            = mark;
      }
   }
   storage[ idx] = cnt;
   r = malloc( sizeof( PatResource) + sizeof( DWORD) * idx);
   r-> style      = pat;
   r-> styleCount = ++idx;
   r-> dotsPtr = r-> dots;
   memcpy( r-> dotsPtr, storage, sizeof( DWORD) * idx);
   hash_store( patMan, &pattern, sizeof( DWORD), r);
   return r;
}

// Stylus end

// Font section


#define FONTHASH_SIZE 563
#define FONTIDHASH_SIZE 23

typedef struct _FontInfo {
   Font          font;
   int           vectored;
   FAMTEXTMETRIC fm;
} FontInfo;

typedef struct _FontHashNode
{
   struct _FontHashNode *next;
   struct _FontHashNode *next2;
   Font key;
   FontInfo value;
} FontHashNode, *PFontHashNode;

typedef struct _FontHash
{
   PFontHashNode buckets[ FONTHASH_SIZE];
} FontHash, *PFontHash;

static FontHash fontHash;
static FontHash fontHashBySize;

static unsigned long
elf_hash( const char *name, int size)
{
   unsigned long   h = 0, g;

   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   while ( *name)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
   }
   return h;
}

static unsigned long
elf_hash_by_size( const Font *f)
{
   unsigned long   h = 0, g;
   char *name = (char *)&(f-> style);
   int size = (char *)(&(f-> name)) - (char *)name;

   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   while ( *name)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
   }
   name = (char *)&(f-> size);
   size = sizeof( f-> size);
   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   return h;
}

static PFontHashNode
find_node( const PFont font, Bool bySize)
{
   unsigned long i;
   PFontHashNode node;
   int sz;

   if ( font == nil) return nil;
   if (bySize) {
      sz = (char *)(&(font-> name)) - (char *)&(font-> style);
      i = elf_hash_by_size( font) % FONTHASH_SIZE;
   } else {
      sz = (char *)(&(font-> name)) - (char *)font;
      i = elf_hash((const char *)font, sz) % FONTHASH_SIZE;
   }
   if (bySize)
      node = fontHashBySize. buckets[ i];
   else
      node = fontHash. buckets[ i];
   if ( bySize) {
      while ( node != nil)
      {
         if (( memcmp( &(font-> style), &(node-> key. style), sz) == 0) &&
             ( strcmp( font-> name, node-> key. name) == 0 ) &&
             (font-> size == node-> key. size))
            return node;
         node = node-> next2;
      }
   } else {
      while ( node != nil)
      {
         if (( memcmp( font, &(node-> key), sz) == 0) && ( strcmp( font-> name, node-> key. name) == 0 ))
            return node;
         node = node-> next;
      }
   }
   return nil;
}

Bool
add_font_to_hash( const PFont key, const PFont font, int vectored, FAMTEXTMETRIC * fm, Bool addSizeEntry)
{
   PFontHashNode node;
   unsigned long i;
   unsigned long j;
   int sz;

//   if ( find_node( key) != nil)
//      return false ;
   node = malloc( sizeof( FontHashNode));
   if ( node == nil) return false;
   memcpy( &(node-> key), key, sizeof( Font));
   memcpy( &(node-> value. font), font, sizeof( Font));
   node-> value. vectored = vectored;
   memcpy( &(node-> value. fm), fm, sizeof( FAMTEXTMETRIC));
   sz = (char *)(&(key-> name)) - (char *)key;
   i = elf_hash((const char *)key, sz) % FONTHASH_SIZE;
   node-> next = fontHash. buckets[ i];
   fontHash. buckets[ i] = node;
   if ( addSizeEntry) {
      j = elf_hash_by_size( key) % FONTHASH_SIZE;
      node-> next2 = fontHashBySize. buckets[ j];
      fontHashBySize. buckets[ j] = node;
   }
   return true;
}

Bool
get_font_from_hash( PFont font, int *vectored, FAMTEXTMETRIC * fm, Bool bySize)
{
   PFontHashNode node = find_node( font, bySize);
   if ( node == nil) return false;
   *font = node-> value. font;
   *vectored = node-> value. vectored;
   if ( fm) memcpy( fm, &(node-> value. fm), sizeof( FAMTEXTMETRIC));
   return true;
}

Bool
create_font_hash( void)
{
   memset ( &fontHash, 0, sizeof( FontHash));
   memset ( &fontHashBySize, 0, sizeof( FontHash));
   return true;
}

Bool
destroy_font_hash( void)
{
   PFontHashNode node, node2;
   int i;
   for ( i = 0; i < FONTHASH_SIZE; i++)
   {
      node = fontHash. buckets[ i];
      while ( node)
      {
         node2 = node;
         node = node-> next;
         free( node2);
      }
   }
   create_font_hash();     // just in case...
   return true;
}


PDCFont
font_alloc( Font * data, Point * resolution)
{
   PDCFont ret = hash_fetch( fontMan, data, FONTSTRUCSIZE + strlen( data-> name));
   if ( ret == nil) {
      LOGFONT logfont;
      PFont   f;
      ret = malloc( sizeof( DCFont));
      memcpy( f = &ret-> font, data, sizeof( Font));
      ret-> refcnt = 0;
      font_font2logfont( f, &logfont);
      if ( !(ret-> hfont  = CreateFontIndirect( &logfont))) apiErr;
      hash_store( fontMan, &ret-> font, FONTSTRUCSIZE + strlen( ret-> font. name), ret);
   }
   ret-> refcnt++;
   return ret;
}

void
font_free( PDCFont res)
{
   if ( !res || --res-> refcnt > 0) return;
   if ( res-> hfont) DeleteObject( res-> hfont);
   hash_delete( fontMan, &res-> font, FONTSTRUCSIZE + strlen( res-> font. name), true);
}

void
font_change( Handle self, Font * font)
{
   PDCFont p;
   PDCFont newP;
   if ( is_apt( aptDCChangeLock)) return;
   p    = sys fontResource;
   newP = ( sys fontResource = font_alloc( font, &sys res));
   font_free( p);
   SelectObject( sys ps, newP-> hfont);
}

void
font_logfont2font( LOGFONT * lf, Font * f, Point * res)
{
   if ( !res) res = &guts. displayResolution;
   if ( lf-> lfHeight <= 0) {
      HDC dc = dc_alloc();
      HFONT hf = SelectObject( dc, CreateFontIndirect( lf));
      TEXTMETRIC tm;
      GetTextMetrics( dc, &tm);
      f-> height = tm. tmHeight;
      DeleteObject( SelectObject( dc, hf));
      dc_free();
   } else
     f-> height            = lf-> lfHeight;
   f-> size                = f-> height * 72.0 / res-> y + 0.5;
   f-> width               = lf-> lfWidth ? lf-> lfWidth : C_NUMERIC_UNDEF;
   f-> direction           = lf-> lfEscapement;
   f-> style               = 0 |
      ( lf-> lfItalic     ? ftItalic     : 0) |
      ( lf-> lfUnderline  ? ftUnderlined : 0) |
      ( lf-> lfStrikeOut  ? ftStruckOut  : 0) |
     (( lf-> lfWeight >= 700) ? ftBold   : 0);
   f-> pitch               = ((( lf-> lfPitchAndFamily & 3) == DEFAULT_PITCH) ? fpDefault :
      ((( lf-> lfPitchAndFamily & 3) == VARIABLE_PITCH) ? fpVariable : fpFixed));
   strncpy( f-> name, lf-> lfFaceName, LF_FACESIZE);
   f-> codepage            = lf-> lfCharSet;
   f-> name[ LF_FACESIZE] = 0;
}

void
font_font2logfont( Font * f, LOGFONT * lf)
{
   lf-> lfHeight           = f-> height;
   lf-> lfWidth            = f-> width;
   lf-> lfEscapement       = f-> direction;
   lf-> lfOrientation      = f-> direction;
   lf-> lfWeight           = ( f-> style & ftBold)       ? 800 : 400;
   lf-> lfItalic           = ( f-> style & ftItalic)     ? 1 : 0;
   lf-> lfUnderline        = ( f-> style & ftUnderlined) ? 1 : 0;
   lf-> lfStrikeOut        = ( f-> style & ftStruckOut)  ? 1 : 0;
   lf-> lfCharSet          = f-> codepage;
   lf-> lfOutPrecision     = OUT_TT_PRECIS;
   lf-> lfClipPrecision    = CLIP_DEFAULT_PRECIS;
   lf-> lfQuality          = PROOF_QUALITY;

   lf-> lfPitchAndFamily   = FF_DONTCARE |
      (( f-> pitch == fpDefault)  ? DEFAULT_PITCH :
      (( f-> pitch == fpVariable) ? VARIABLE_PITCH : FIXED_PITCH));
   strncpy( lf-> lfFaceName, f-> name, LF_FACESIZE);
}

void
font_textmetric2fontmetric( TEXTMETRIC * tm, FontMetric * fm)
{
   fm-> nominalSize            = tm-> tmHeight * 72.0 / guts. displayResolution.y + 0.5;
   fm-> width                  = tm-> tmAveCharWidth;
   fm-> height                 = tm-> tmHeight;
   fm-> style                  = 0 |
                                 ( tm-> tmItalic     ? ftItalic     : 0) |
                                 ( tm-> tmUnderlined ? ftUnderlined : 0) |
                                 ( tm-> tmStruckOut  ? ftStruckOut  : 0) |
                                 (( tm-> tmWeight >= 700) ? ftBold   : 0);
   fm-> pitch                  =
      (( tm-> tmPitchAndFamily & TMPF_FIXED_PITCH)               ? fpVariable : fpFixed) |
      (( tm-> tmPitchAndFamily & ( TMPF_VECTOR | TMPF_TRUETYPE)) ? fpVector   : fpRaster);
   fm-> weight                 = tm-> tmWeight / 100;
   fm-> maxAscent              = tm-> tmAscent;
   fm-> maxDescent             = tm-> tmDescent;
   fm-> lcAscent               = tm-> tmAscent;     // these two values not supported in TEXTMETRICS so -
   fm-> lcDescent              = tm-> tmDescent;    // at least they're not zero and not greater than ucXscents
   fm-> averageWidth           = tm-> tmAveCharWidth;
   fm-> internalLeading        = tm-> tmInternalLeading;
   fm-> externalLeading        = tm-> tmExternalLeading;
   fm-> xDeviceRes             = tm-> tmDigitizedAspectX;
   fm-> yDeviceRes             = tm-> tmDigitizedAspectY;
   fm-> firstChar              = tm-> tmFirstChar;
   fm-> lastChar               = tm-> tmLastChar;
   fm-> breakChar              = tm-> tmBreakChar;
   fm-> defaultChar            = tm-> tmDefaultChar;
}

PFont
apc_font_default( PFont font)
{
   *font = guts. windowFont;
   font-> pitch = fpDefault;
   return font;
}

int
apc_font_load( char * filename)
{
   return 0;
}

#define fgBitmap 0
#define fgVector 1

static Bool recursiveFF = 0;
static FAMTEXTMETRIC fmtx;

typedef struct _FEnumStruc
{
   int           count;
   int           resValue;
   int           heiValue;
   int           widValue;
   Bool          useWidth;
   Bool          useVector;
   Bool          usePitch;
   Bool          usePrecis;
   Bool          forceSize;
   Bool          vecId;
   PFont         font;
   FAMTEXTMETRIC tm;
   LOGFONT       lf;
   Point         res;
   char          name[ LF_FACESIZE];
} FEnumStruc, *PFEnumStruc;

int CALLBACK
fep( ENUMLOGFONT FAR *e, NEWTEXTMETRIC FAR *t, int type, PFEnumStruc es)
{
   Font * font = es-> font;
#define do_copy                                                         \
   es-> count++;                                                        \
   memcpy( &es-> tm, t, sizeof( TEXTMETRIC));                           \
   memcpy( es-> tm. family, e-> elfFullName, sizeof( LF_FULLFACESIZE)); \
   memcpy( &es-> lf,    &e-> elfLogFont, sizeof( LOGFONT));             \
   memcpy( es-> name,   e-> elfLogFont. lfFaceName, sizeof( LF_FACESIZE))

   if ( es-> usePitch)
   {
      int fpitch = ( t-> tmPitchAndFamily & TMPF_FIXED_PITCH) ? fpVariable : fpFixed;
      if ( fpitch != ( font-> pitch & fpPitchMask))
         return 1; // so this font cannot be selected due pitch pickup failure
   }

   if ( type & TRUETYPE_FONTTYPE) {
      do_copy;
      es-> vecId = 1;
      return 0; // enough; no further enumeration requred, since Win renders TrueType quite good
                // to not mix these with raster fonts.
   }
   if ( type == 0) {
      do_copy;
      es-> vecId = 1;
      return 1; // it's vector font; but keep enumeration in case we'll get better match
   }
   if ( es-> forceSize) {
      long res = ( t-> tmDigitizedAspectY - es-> res. y) * ( t-> tmDigitizedAspectY - es-> res. y);
      long xs  = abs( e-> elfLogFont. lfHeight * 72.0 / es-> res. y + 0.5);
      long hei = ( xs - font-> size) * ( xs - font-> size);
      if ( hei < es-> heiValue) {
         es-> heiValue = hei;
         es-> resValue = res;
         do_copy;
      } else if ( es-> useWidth && ( hei == es-> heiValue)) {
         long wid = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
         if ( wid < es-> widValue) {
            es-> widValue = wid;
            es-> resValue = res;
            do_copy;
         }
      } else if (( res < es-> resValue) && ( hei == es-> heiValue)) {
         es-> resValue = res;
         do_copy;
      }
   } else {
      long hei = ( e-> elfLogFont. lfHeight - font-> height) * ( e-> elfLogFont. lfHeight - font-> height);
      if ( hei < es-> heiValue) {
         es-> heiValue = hei;
         if ( es-> useWidth)
            es-> widValue = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
         do_copy;
      } else if ( es-> useWidth && ( hei == es-> heiValue)) {
         long wid = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
         if ( wid < es-> widValue) {
            es-> widValue = wid;
            do_copy;
         }
      }
   }
   return 1;
#undef do_copy
}

extern int font_font2gp( PFont font, Point res, Bool forceSize, HDC dc);

static int
font_font2gp_internal( PFont font, Point res, Bool forceSize, HDC theDC)
{
#define out( retVal)  if ( !theDC) dc_free(); return retVal
   FEnumStruc es;
   HDC dc             = theDC ? theDC : dc_alloc();
   es. count          = es. vecId = 0;
   es. resValue       = es. heiValue = es. widValue = INT_MAX;
   es. useWidth       = font-> width != 0;
   es. useVector      = font-> direction != 0;
   es. usePitch       = ( font-> pitch & fpPitchMask)      != fpDefault;
   es. usePrecis      = ( font-> pitch & fpPrecisionMask)  != fpDontCare;
   es. res            = res;
   es. forceSize      = forceSize;
	es. font           = font;
   if ( forceSize)
      font-> height = font-> size * res. y / 72.0 + 0.5;
   // querying font for given name
   EnumFontFamilies( dc, font-> name, ( FONTENUMPROC) fep, ( LPARAM) &es);

   // checking matched font, if available
   if ( es. count > 0) {
      if ( es. useVector) {
         if ( !es. vecId && !es. usePitch) {
            // preserve pitch in case bitmap font cannot be selected
            font-> pitch = ( es. tm. t. tmPitchAndFamily & TMPF_FIXED_PITCH) ? fpVariable : fpFixed;
            es. usePitch = 1;
         }
         es. resValue = es. heiValue = INT_MAX; // cancel bitmap font selection
      }
      // have resolution ok?
      // if ( es. resValue < 20 && ( es. forceSize || ( es. heiValue < 16)))
      if (
           ( es. heiValue < 16) &&
           ( !es. usePrecis || (( font-> pitch & fpPrecisionMask) == fpVector)) &&
           ( !es. useWidth  || ( es. widValue == 0 && es. heiValue == 0))
         )
      {
         font-> height   = abs( es. lf. lfHeight);
         font-> ascent   = es. tm. t.tmAscent;
         font-> descent  = es. tm. t.tmDescent;
         memcpy( &fmtx, &es. tm, sizeof( fmtx));
         font-> size     = abs( es. lf. lfHeight * 72.0 / res.y + 0.5);
         font-> width    = es. lf. lfWidth;
         font-> codepage = es. lf. lfCharSet;
         out( fgBitmap);
      }
      // or vector font - for any purpose?
      if ( es. vecId) {
         if ( forceSize)
            font-> height = font-> size * res. y / 72.0 + 0.5;
         else
            font-> size   = font-> height * 72.0 / res. y + 0.5;
         if ( !es. useWidth)
            font-> width = es. lf. lfWidth *
               (( float) font-> height / abs( es. lf. lfHeight)) + 0.5;
         font-> ascent   = es. tm. t.tmAscent  * (( float) font-> height / es. tm. t.tmHeight) + 0.5;
         font-> descent  = es. tm. t.tmDescent * (( float) font-> height / es. tm. t.tmHeight) + 0.5;
         font-> codepage = es. lf. lfCharSet;
         memcpy( &fmtx, &es. tm, sizeof( fmtx));
         #define fmtx_adj( field) fmtx. t. field = fmtx. t. field * (( float) font-> height / es. tm. t.tmHeight) + 0.5;
         fmtx_adj( tmAveCharWidth    );
         fmtx_adj( tmHeight          );
         fmtx_adj( tmAscent          );
         fmtx_adj( tmDescent         );
         fmtx_adj( tmInternalLeading );
         fmtx_adj( tmExternalLeading );
         fmtx_adj( tmMaxCharWidth    );
         fmtx_adj( tmOverhang        );
         out( fgVector);
      }
   }

   // if strict match not found, use subplacing
   if ( es. usePitch)
   {
       int ogp  = font-> pitch & fpPitchMask;
       int ret;
       int ogpp = font-> pitch & fpPrecisionMask;
       strcpy( font-> name, (( font-> pitch & fpPitchMask) == fpFixed) ?
          DEFAULT_FIXED_FONT : DEFAULT_VARIABLE_FONT);
       font-> pitch = fpDefault | ogpp;
       ret = font_font2gp( font, res, forceSize, dc);
       if ( ogp == fpFixed && ( strcmp( font-> name, DEFAULT_FIXED_FONT) != 0))
       {
          strcpy( font-> name, DEFAULT_SYSTEM_FONT);
          font-> pitch = fpDefault | ogpp;
          ret = font_font2gp( font, res, forceSize, dc);
       }
       out( ret);
   }

   // Font not found. So use general representation for height and width
   strcpy( font-> name, DEFAULT_FONT_NAME);
   if ( recursiveFF == 0)
   {
      // trying to catch default font with correct values
      int r;
      recursiveFF++;
      r = font_font2gp( font, res, forceSize, dc);
      recursiveFF--;
      out( r);
   } else {
      int r;
      // if not succeeded, to avoid recursive call use "wild guess".
      // This could be achieved if system does not have "System" font
      *font = guts. windowFont;
      font-> pitch = fpDefault;
      r = font_font2gp( font, res, forceSize, dc);
      out( r);
   }
   return fgBitmap;
#undef out
}

static int
font_font2gp( PFont font, Point res, Bool forceSize, HDC dc)
{
   int vectored;
   Font key;
   Bool addSizeEntry;

   font-> resolution = res. y * 0x10000 + res. x;
   if ( get_font_from_hash( font, &vectored, &fmtx, forceSize))
      return vectored;
   memcpy( &key, font, sizeof( Font));
   vectored = font_font2gp_internal( font, res, forceSize, dc);
   font-> resolution = res. y * 0x10000 + res. x;
   if ( forceSize) {
      key. height = font-> height;
      key. width  = font-> width;
      addSizeEntry = true;
   } else {
      key. size  = font-> size;
      addSizeEntry = vectored ? (( key. height == key. width)  || ( key. width == 0)) : true;
   }
   add_font_to_hash( &key, font, vectored, &fmtx, addSizeEntry);
   return vectored;
}

void
apc_font_pick( Handle self, PFont source, PFont dest)
{
   font_font2gp( dest, self ? sys res : guts. displayResolution,
      Drawable_font_add( self, source, dest), self ? sys ps : 0);
}

int CALLBACK
fep2( ENUMLOGFONT FAR *e, NEWTEXTMETRIC FAR *t, int type, PList lst)
{
   PFontMetric fm = malloc( sizeof( FontMetric));
   font_textmetric2fontmetric(( TEXTMETRIC *) t, fm);
   strncpy( fm-> family,   e-> elfFullName,            LF_FULLFACESIZE);
   strncpy( fm-> facename, e-> elfLogFont. lfFaceName, LF_FACESIZE);
   list_add( lst, ( Handle) fm);
	return 1;
}


PFontMetric
apc_fonts( char * facename, int * retCount)
{
   PFontMetric fmtx = nil;
   int  i;
   HDC  dc = dc_alloc();
   List lst;
   apcErrClear;

   list_create( &lst, 256, 256);
   EnumFontFamilies( dc, facename, ( FONTENUMPROC) fep2, ( LPARAM) &lst);
   dc_free();
   if ( lst. count == 0) goto Nothing;
   *retCount = lst. count;
   fmtx = malloc( *retCount * sizeof( FontMetric));
   for ( i = 0; i < lst. count; i++)
      memcpy( &fmtx[ i], ( void *) lst. items[ i], sizeof( FontMetric));
   list_delete_all( &lst, true);
Nothing:
   list_destroy( &lst);
   return fmtx;
}

PFontMetric
apc_font_metrics( Handle self, PFont font, PFontMetric metrics)
{
   apcErrClear;
   font_font2gp( font, sys res, false, 0);
   font_textmetric2fontmetric( &fmtx. t, metrics);
   strncpy( metrics-> facename, font-> name, 256);
   strncpy( metrics-> family,   fmtx. family, LF_FULLFACESIZE);
   return metrics;
}

// Font end
// Colors section


#define stdDisabled  COLOR_GRAYTEXT        ,  COLOR_WINDOW
#define stdHilite    COLOR_HIGHLIGHTTEXT   ,  COLOR_HIGHLIGHT
#define std3d        COLOR_BTNHIGHLIGHT    ,  COLOR_BTNSHADOW

static int buttonScheme[] = {
   COLOR_BTNTEXT,   COLOR_BTNFACE,
   COLOR_BTNTEXT,   COLOR_BTNFACE,
   COLOR_GRAYTEXT,  COLOR_BTNFACE,
   std3d
};
static int sliderScheme[] = {
   COLOR_WINDOWTEXT,       COLOR_SCROLLBAR,
   COLOR_WINDOWTEXT,       COLOR_SCROLLBAR,
   stdDisabled,
   std3d
};

static int dialogScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   COLOR_CAPTIONTEXT, COLOR_ACTIVECAPTION,
   COLOR_INACTIVECAPTIONTEXT, COLOR_INACTIVECAPTION,
   std3d
};
static int staticScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   stdDisabled,
   std3d
};
static int editScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   stdHilite,
   stdDisabled,
   std3d
};
static int menuScheme[] = {
   COLOR_MENUTEXT, COLOR_MENU,
   stdHilite,
   stdDisabled,
   std3d
};

static int scrollScheme[] = {
   COLOR_WINDOWTEXT,    COLOR_SCROLLBAR,
   stdHilite,
   stdDisabled,
   std3d
};

static int windowScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   COLOR_CAPTIONTEXT, COLOR_ACTIVECAPTION,
   COLOR_INACTIVECAPTIONTEXT, COLOR_INACTIVECAPTION,
   std3d
};
static int customScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   stdHilite,
   stdDisabled,
   std3d
};

static int ctx_wc2SCHEME[] =
{
   wcButton    , ( int) &buttonScheme,
   wcCheckBox  , ( int) &buttonScheme,
   wcRadio     , ( int) &buttonScheme,
   wcDialog    , ( int) &dialogScheme,
   wcSlider    , ( int) &sliderScheme,
   wcLabel     , ( int) &staticScheme,
   wcInputLine , ( int) &editScheme,
   wcEdit      , ( int) &editScheme,
   wcListBox   , ( int) &editScheme,
   wcCombo     , ( int) &editScheme,
   wcMenu      , ( int) &menuScheme,
   wcPopup     , ( int) &menuScheme,
   wcScrollBar , ( int) &scrollScheme,
   wcWindow    , ( int) &windowScheme,
   wcWidget    , ( int) &customScheme,
   endCtx
};


long
remap_color( long clr, Bool toSystem)
{
   PRGBColor cp = ( PRGBColor) &clr;
   unsigned char sw = cp-> r;
   if ( toSystem && clr < 0) {
      long c = clr;
      int * scheme = ( int *) ctx_remap_def( clr & wcMask, ctx_wc2SCHEME, true, ( int) &customScheme);
      if (( clr = ( clr & ~wcMask)) > clMaxSysColor) clr = clMaxSysColor;
      c = GetSysColor( scheme[ clr - 1]);
      return c;
   }
   cp-> r = cp-> b;
   cp-> b = sw;
   return clr;
}

// Colors end
// Miscellaneous

static HDC cachedScreenDC = nil;
static int cachedScreenRC = 0;
static HDC cachedCompatDC = nil;
static int cachedCompatRC = 0;


HDC dc_alloc()
{
   if ( cachedScreenRC++ == 0)
      cachedScreenDC = GetDC( 0);
   return cachedScreenDC;
}

void dc_free()
{
   if ( --cachedScreenRC <= 0)
      ReleaseDC( 0, cachedScreenDC);
   if ( cachedScreenRC < 0)
      cachedScreenRC == 0;
}

HDC dc_compat_alloc( HDC compatDC)
{
   if ( cachedCompatRC++ == 0)
      cachedCompatDC = CreateCompatibleDC( compatDC);
   return cachedCompatDC;
}

void dc_compat_free()
{
   if ( --cachedCompatRC <= 0)
      DeleteDC( cachedCompatDC);
   if ( cachedCompatRC < 0)
      cachedCompatRC == 0;
}

void
hwnd_enter_paint( Handle self)
{
   GetObject( sys stockPen   = GetCurrentObject( sys ps, OBJ_PEN),
      sizeof( LOGPEN), &sys stylus. pen);
   GetObject( sys stockBrush = GetCurrentObject( sys ps, OBJ_BRUSH),
      sizeof( LOGBRUSH), &sys stylus. brush);
   sys stockFont      = GetCurrentObject( sys ps, OBJ_FONT);
   sys stylusResource = nil;
   sys fontResource   = nil;
   sys stylusFlags    = 0;
   sys stylus. extPen. actual = false;
   apt_set( aptDCChangeLock);
   apc_gp_set_color( self, sys viewColors[ ciFore]);
   apc_gp_set_back_color( self, sys viewColors[ ciBack]);

   if ( sys psd == nil) sys psd = malloc( sizeof( PaintSaveData));
   apc_gp_set_text_opaque( self, sys textOpaque);
   apc_gp_set_line_width( self, sys lineWidth);
   apc_gp_set_line_end( self, sys lineEnd);
   apc_gp_set_line_pattern( self, sys linePattern);
   apc_gp_set_rop( self, sys rop);
   apc_gp_set_rop2( self, sys rop2);
   apc_gp_set_transform( self, sys transform. x, sys transform. y);
   apc_gp_set_fill_pattern( self, sys fillPattern2);
   sys psd-> font        = var font;
   sys psd-> lineWidth   = sys lineWidth;
   sys psd-> lineEnd     = sys lineEnd;
   sys psd-> linePattern = sys linePattern;
   sys psd-> rop         = sys rop;
   sys psd-> rop2        = sys rop2;
   sys psd-> transform   = sys transform;
   sys psd-> textOpaque  = sys textOpaque;

   apt_clear( aptDCChangeLock);
   stylus_change( self);
   apc_gp_set_font( self, &var font);
   var font. resolution = sys res. y * 0x10000 + sys res. x;
   SetTextAlign( sys ps, TA_BASELINE);
   SetStretchBltMode( sys ps, COLORONCOLOR);
}

void
hwnd_leave_paint( Handle self)
{
   SelectObject( sys ps, sys stockPen);
   SelectObject( sys ps, sys stockBrush);
   SelectObject( sys ps, sys stockFont);
   sys stockPen = sys stockBrush = sys stockFont = nilHandle;
   stylus_free( sys stylusResource, false);
   if ( sys psd != nil) {
      var font        = sys psd-> font;
      sys lineWidth   = sys psd-> lineWidth;
      sys lineEnd     = sys psd-> lineEnd;
      sys linePattern = sys psd-> linePattern;
      sys rop         = sys psd-> rop;
      sys rop2        = sys psd-> rop2;
      sys transform   = sys psd-> transform;
      sys textOpaque  = sys psd-> textOpaque;
      free( sys psd);
      sys psd = nil;
   }
   free( sys charTable);
   free( sys charTable2);
   (void*)sys charTable = (void*)sys charTable2 = nil;
}

Handle
hwnd_top_level( Handle self)
{
   while ( self) {
      if ( sys className == WC_FRAME) return self;
      self = var owner;
   }
   return nilHandle;
}


typedef struct _ModData
{
   BYTE  ks  [ 256];
   BYTE  kss [ 3];
   BYTE *gks;
} ModData;


BYTE *
mod_select( int mod)
{
   ModData * ks;

   ks = malloc( sizeof( ModData));
   GetKeyboardState( ks-> ks);
   ks-> kss[ 0]   = ks-> ks[ VK_MENU];
   ks-> kss[ 1]   = ks-> ks[ VK_CONTROL];
   ks-> kss[ 2]   = ks-> ks[ VK_SHIFT];
   ks-> ks[ VK_MENU   ] = ( mod & kbAlt  ) ? 0x80 : 0;
   ks-> ks[ VK_CONTROL] = ( mod & kbCtrl ) ? 0x80 : 0;
   ks-> ks[ VK_SHIFT  ] = ( mod & kbShift) ? 0x80 : 0;
   SetKeyboardState( ks-> ks);
   ks-> gks = guts. currentKeyState;
   guts. currentKeyState = ks-> ks;
   return ( BYTE*) ks;
}

void
mod_free( BYTE * modState)
{
   ModData * ks = ( ModData*) modState;
   ks-> ks[ VK_MENU   ] = ks-> kss[ 0];
   ks-> ks[ VK_CONTROL] = ks-> kss[ 1];
   ks-> ks[ VK_SHIFT  ] = ks-> kss[ 2];
   SetKeyboardState( ks-> ks);
   guts. currentKeyState = ks-> gks;
   free( ks);
}

