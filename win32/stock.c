/*
      stock.c

      Stock objects - pens, brushes, fonts
*/

#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "win32\win32guts.h"
#include <ctype.h>
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

      if ( IS_WIN95 && ( hash_count( stylusMan) > 128))
         stylus_clean();

      ret = malloc( sizeof( DCStylus));
      memcpy( &ret-> s, data, sizeof( Stylus));
      ret-> refcnt = 0;
      p = &ret-> s. pen;
      if ( !extPen) {
         if ( !( ret-> hpen = CreatePenIndirect( p))) {
            apiErr;
            ret-> hpen = CreatePen( PS_SOLID, 0, 0);
         }
      } else {
         LOGBRUSH pb = { BS_SOLID, ret-> s. pen. lopnColor, 0};
         if ( !( ret-> hpen   = ExtCreatePen( ret-> s. extPen. style, p-> lopnWidth. x, &pb,
            ret-> s. extPen. patResource-> styleCount,
            ret-> s. extPen. patResource-> dotsPtr
         ))) {
            if ( !IS_WIN95) apiErr;
            ret-> hpen = CreatePen( PS_SOLID, 0, 0);
         }
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
      if ( !( ret-> hbrush = CreateBrushIndirect( &ret-> s. brush. lb))) {
         apiErr;
         ret-> hbrush = CreateSolidBrush( RGB( 255, 255, 255));
      }
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


static Bool _ft_cleaner( PDCFont f, int keyLen, void * key, void * dummy) {
   if ( f-> refcnt <= 0) font_free( f);
   return false;
}


PDCFont
font_alloc( Font * data, Point * resolution)
{
   PDCFont ret = hash_fetch( fontMan, data, FONTSTRUCSIZE + strlen( data-> name));
   if ( ret == nil) {
      LOGFONT logfont;
      PFont   f;

      if ( IS_WIN95 && ( hash_count( fontMan) > 128))
         font_clean();

      ret = malloc( sizeof( DCFont));
      memcpy( f = &ret-> font, data, sizeof( Font));
      ret-> refcnt = 0;
      font_font2logfont( f, &logfont);
      if ( !( ret-> hfont  = CreateFontIndirect( &logfont))) {
         LOGFONT lf;
         apiErr;
         memset( &lf, 0, sizeof( lf));
         ret-> hfont = CreateFontIndirect( &lf);
      }
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
   if ( sys ps)
       SelectObject( sys ps, newP-> hfont);
}

void
font_clean()
{
    hash_first_that( fontMan, _ft_cleaner, nil, nil, nil);
}

void
font_logfont2font( LOGFONT * lf, Font * f, Point * res)
{
   TEXTMETRIC tm;
   HDC dc = dc_alloc();
   HFONT hf = SelectObject( dc, CreateFontIndirect( lf));

   GetTextMetrics( dc, &tm);
   DeleteObject( SelectObject( dc, hf));
   dc_free();

   if ( !res) res = &guts. displayResolution;
   f-> height              = tm. tmHeight;
   f-> size                = ( f-> height - tm. tmInternalLeading) * 72.0 / res-> y + 0.5;
   f-> width               = lf-> lfWidth;
   f-> direction           = lf-> lfEscapement;
   f-> style               = 0 |
      ( lf-> lfItalic     ? fsItalic     : 0) |
      ( lf-> lfUnderline  ? fsUnderlined : 0) |
      ( lf-> lfStrikeOut  ? fsStruckOut  : 0) |
     (( lf-> lfWeight >= 700) ? fsBold   : 0);
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
   lf-> lfWeight           = ( f-> style & fsBold)       ? 800 : 400;
   lf-> lfItalic           = ( f-> style & fsItalic)     ? 1 : 0;
   lf-> lfUnderline        = ( f-> style & fsUnderlined) ? 1 : 0;
   lf-> lfStrikeOut        = ( f-> style & fsStruckOut)  ? 1 : 0;
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
font_textmetric2font( TEXTMETRIC * tm, Font * fm, Bool readonly)
{
   if ( !readonly) {
      fm-> size                   = ( tm-> tmHeight - tm-> tmInternalLeading) * 72.0 / guts. displayResolution.y + 0.5;
      fm-> width                  = tm-> tmAveCharWidth;
      fm-> height                 = tm-> tmHeight;
      fm-> style                  = 0 |
                                 ( tm-> tmItalic     ? fsItalic     : 0) |
                                 ( tm-> tmUnderlined ? fsUnderlined : 0) |
                                 ( tm-> tmStruckOut  ? fsStruckOut  : 0) |
                                 (( tm-> tmWeight >= 700) ? fsBold   : 0);
   }
   fm-> pitch                  =
      (( tm-> tmPitchAndFamily & TMPF_FIXED_PITCH)               ? fpVariable : fpFixed);
   fm-> vector                 =
      (( tm-> tmPitchAndFamily & ( TMPF_VECTOR | TMPF_TRUETYPE)) ? true       : false   );
   fm-> weight                 = tm-> tmWeight / 100;
   fm-> ascent                 = tm-> tmAscent;
   fm-> descent                = tm-> tmDescent;
   fm-> maximalWidth           = tm-> tmMaxCharWidth;
   fm-> internalLeading        = tm-> tmInternalLeading;
   fm-> externalLeading        = tm-> tmExternalLeading;
   fm-> xDeviceRes             = tm-> tmDigitizedAspectX;
   fm-> yDeviceRes             = tm-> tmDigitizedAspectY;
   fm-> firstChar              = tm-> tmFirstChar;
   fm-> lastChar               = tm-> tmLastChar;
   fm-> breakChar              = tm-> tmBreakChar;
   fm-> defaultChar            = tm-> tmDefaultChar;
   fm-> codepage               = tm-> tmCharSet;
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

static FAMTEXTMETRIC fmtx;

/*static int
font_font2gp_internal( PFont font, Point res, Bool forceSize, HDC theDC)
{
#define out( retVal)  if ( !theDC) dc_free(); return retVal
   HDC  dc = theDC ? theDC : dc_alloc();
   TEXTMETRIC tm;
   LOGFONT lf;
   HFONT hfont;

   if ( forceSize)
      font-> height = -font-> size * res. y / 72.0 + 0.5;

   font_font2logfont( font, &lf);

   hfont = SelectObject( dc, CreateFontIndirect( &lf));
   GetTextMetrics( dc, &tm);
   GetTextFace( dc, 256, font-> name);
   DeleteObject( SelectObject( dc, hfont));
   font_textmetric2font( &tm, font, false);

   return ( font-> type == ftVector) ? fgVector : fgBitmap;
}*/

static Bool recursiveFF = 0;

typedef struct _FEnumStruc
{
   int           count;
   int           resValue;
   int           heiValue;
   int           widValue;
   int           fType;
   Bool          useWidth;
   Bool          useVector;
   Bool          usePitch;
   Bool          forceSize;
   Bool          vecId;
   Bool          matchILead;
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
   long hei, res;

#define do_copy                                                         \
   es-> count++;                                                        \
   es-> fType = type;                                                   \
   memcpy( &es-> tm, t, sizeof( TEXTMETRIC));                           \
   memcpy( es-> tm. family, e-> elfFullName, LF_FULLFACESIZE);          \
   memcpy( &es-> lf,    &e-> elfLogFont, sizeof( LOGFONT));             \
   memcpy( es-> name,   e-> elfLogFont. lfFaceName, LF_FACESIZE)

   if ( es-> usePitch)
   {
      int fpitch = ( t-> tmPitchAndFamily & TMPF_FIXED_PITCH) ? fpVariable : fpFixed;
      if ( fpitch != font-> pitch)
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

   // bitmapped fonts:
   // determining best match for primary measure - size or height
   if ( es-> forceSize) {
      long xs  = ( t-> tmHeight - ( es-> matchILead ? t-> tmInternalLeading : 0))
                 * 72.0 / es-> res. y + 0.5;
      hei = ( xs - font-> size) * ( xs - font-> size);
   } else {
      long dv = t-> tmHeight - font-> height - ( es-> matchILead ? 0 : t-> tmInternalLeading);
      hei = dv * dv;
   }

   // resolution difference
   res = ( t-> tmDigitizedAspectY - es-> res. y) * ( t-> tmDigitizedAspectY - es-> res. y);

   if ( hei < es-> heiValue) {
      // current height is closer than before...
      es-> heiValue = hei;
      es-> resValue = res;
      if ( es-> useWidth)
         es-> widValue = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
      do_copy;
   } else if ( es-> useWidth && ( hei == es-> heiValue)) {
      // height is same close, but width is requested and close that before
      long wid = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
      if ( wid < es-> widValue) {
         es-> widValue = wid;
         es-> resValue = res;
         do_copy;
      } else if (( wid == es-> widValue) && ( res < es-> resValue)) {
         // height and width are same close, but resolution is closer
         es-> resValue = res;
         do_copy;
      }
   } else if (( res < es-> resValue) && ( hei == es-> heiValue)) {
      // height is same close, but resolution is closer
      es-> resValue = res;
      do_copy;
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
   HDC  dc            = theDC ? theDC : dc_alloc();
   Bool useNameSubplacing = false;

   es. count          = es. vecId = 0;
   es. resValue       = es. heiValue = es. widValue = INT_MAX;
   es. useWidth       = font-> width != 0;
   es. useVector      = font-> direction != 0;
   es. usePitch       = font-> pitch != fpDefault;
   es. res            = res;
   es. forceSize      = forceSize;
   es. font           = font;
   es. matchILead     = forceSize ? ( font-> size >= 0) : ( font-> height >= 0);

   useNameSubplacing = es. usePitch;
   if ( font-> height < 0) font-> height *= -1;
   if ( font-> size   < 0) font-> size   *= -1;

   EnumFontFamilies( dc, font-> name, ( FONTENUMPROC) fep, ( LPARAM) &es);

   // checking matched font, if available
   if ( es. count > 0) {
      if ( es. useVector) {
         if ( !es. vecId) useNameSubplacing = 1; // try other font if bitmap wouldn't fit
         es. resValue = es. heiValue = INT_MAX;  // cancel bitmap font selection
      }
      // have resolution ok? so using raster font mtx
      if (
           ( es. heiValue < 2) &&
           ( es. fType & RASTER_FONTTYPE) &&
           ( !es. useWidth  || (( es. widValue == 0) && ( es. heiValue == 0)))
         )
      {
         font-> height   = es. tm. t.tmHeight;
         font_textmetric2font( &es. tm. t, font, true);

         memcpy( &fmtx, &es. tm, sizeof( fmtx));
         strncpy( font-> family, fmtx. family, LF_FULLFACESIZE);
         font-> size     = ( es. tm. t.tmHeight - es. tm. t.tmInternalLeading) * 72.0 / res.y + 0.5;
         font-> width    = es. lf. lfWidth;
         font-> codepage = es. tm. t.tmCharSet;
         out( fgBitmap);
      }

      // or vector font - for any purpose?
      // if so, it could guranteed that font-> height == t.tmHeight
      if ( es. vecId) {
         LOGFONT lpf = es. lf;
         HFONT   hf;
         TEXTMETRIC tm;

         // since proportional computation of small items as InternalLeading
         // gives incorrect result, querying the only valid source - GetTextMetrics
         if ( forceSize) {
            lpf. lfHeight = font-> size * res. y / 72.0 + 0.5;
            if ( es. matchILead) lpf. lfHeight *= -1;
         } else
            lpf. lfHeight = es. matchILead ? font-> height : -font-> height;

         lpf. lfWidth = es. useWidth ? font-> width : 0;

         hf = SelectObject( dc, CreateFontIndirect( &lpf));
         GetTextMetrics( dc, &tm);
         DeleteObject( SelectObject( dc, hf));

         if ( forceSize)
            font-> height = tm. tmHeight;
         else
            font-> size = ( tm. tmHeight - tm. tmInternalLeading) * 72.0 / res. y + 0.5;

         if ( !es. useWidth)
            font-> width = tm. tmAveCharWidth;

         font_textmetric2font( &tm, font, true);
         memcpy( &fmtx, &es. tm, sizeof( fmtx));
         strncpy( font-> family, fmtx. family, LF_FULLFACESIZE);
         font-> codepage = tm .tmCharSet;
         out( fgVector);
      }
   }

   // if strict match not found, use subplacing
   if ( useNameSubplacing)
   {
       int ret;
       int ogp  = font-> pitch;
       // setting some other( maybe other) font name
       strcpy( font-> name, font-> pitch == fpFixed ?
          guts. defaultFixedFont : guts. defaultVariableFont);
       font-> pitch = fpDefault;
       ret = font_font2gp( font, res, forceSize, dc);
       // if that alternative match succeeded with name subplaced again, skip
       // that result and use DEFAULT_SYSTEM_FONT match
       if (( ogp == fpFixed) && ( strcmp( font-> name, guts. defaultFixedFont) != 0)) {
          strcpy( font-> name, DEFAULT_SYSTEM_FONT);
          font-> pitch = fpDefault;
          ret = font_font2gp( font, res, forceSize, dc);
       }
       out( ret);
   }

   // font not found, so use general representation for height and width
   strcpy( font-> name, guts. defaultSystemFont);
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
   if ( self) objCheck;
   font_font2gp( dest, self ? sys res : guts. displayResolution,
      Drawable_font_add( self, source, dest), self ? sys ps : 0);
}

int CALLBACK
fep2( ENUMLOGFONT FAR *e, NEWTEXTMETRIC FAR *t, int type, PList lst)
{
   PFont fm = malloc( sizeof( Font));
   font_textmetric2font(( TEXTMETRIC *) t, fm, false);
   strncpy( fm-> name,     e-> elfLogFont. lfFaceName, LF_FACESIZE);
   strncpy( fm-> family,   e-> elfFullName,            LF_FULLFACESIZE);
   list_add( lst, ( Handle) fm);
   return 1;
}


PFont
apc_fonts( char * facename, int * retCount)
{
   PFont fmtx = nil;
   int  i;
   HDC  dc = dc_alloc();
   List lst;
   apcErrClear;

   list_create( &lst, 256, 256);
   EnumFontFamilies( dc, facename, ( FONTENUMPROC) fep2, ( LPARAM) &lst);
   dc_free();
   if ( lst. count == 0) goto Nothing;
   *retCount = lst. count;
   fmtx = malloc( *retCount * sizeof( Font));
   for ( i = 0; i < lst. count; i++)
      memcpy( &fmtx[ i], ( void *) lst. items[ i], sizeof( Font));
   list_delete_all( &lst, true);
Nothing:
   list_destroy( &lst);
   return fmtx;
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
      if ( clr == 0) return 0xFFFFFF; // clInvalid
      c = GetSysColor( scheme[ clr - 1]);
      return c;
   }
   cp-> r = cp-> b;
   cp-> b = sw;
   return clr;
}

Color
apc_lookup_color( const char * colorName)
{
   char buf[ 256];
   char *b;
   int len;

#define xcmp( name, stlen, retval)  if (( len == stlen) && ( strcmp( name, buf) == 0)) return retval

   strncpy( buf, colorName, 256);
   len = strlen( buf);
   for ( b = buf; *b; b++) *b = tolower(*b);

   switch( buf[0]) {
   case 'a':
       xcmp( "aqua", 4, 0x00FFFF);
       xcmp( "azure", 5, ARGB(240,255,255));
       break;
   case 'b':
       xcmp( "black", 5, 0x000000);
       xcmp( "blanchedalmond", 14, ARGB( 255,235,205));
       xcmp( "blue", 4, 0x000080);
       xcmp( "brown", 5, 0x808000);
       xcmp( "beige", 5, ARGB(245,245,220));
       break;
   case 'c':
       xcmp( "cyan", 4, 0x008080);
       xcmp( "chocolate", 9, ARGB(210,105,30));
       break;
   case 'd':
       xcmp( "darkgray", 8, 0x404040);
       break;
   case 'e':
       break;
   case 'f':
       xcmp( "fuchsia", 7, 0xFF00FF);
       break;
   case 'g':
       xcmp( "green", 5, 0x008000);
       xcmp( "gray", 4, 0x808080);
       xcmp( "gray80", 6, ARGB(204,204,204));
       xcmp( "gold", 4, ARGB(255,215,0));
       break;
   case 'h':
       xcmp( "hotpink", 7, ARGB(255,105,180));
       break;
   case 'i':
       xcmp( "ivory", 5, ARGB(255,255,240));
       break;
   case 'j':
       break;
   case 'k':
       xcmp( "khaki", 5, ARGB(240,230,140));
       break;
   case 'l':
       xcmp( "lime", 4, 0x00FF00);
       xcmp( "lightgray", 9, 0xC0C0C0);
       xcmp( "lightblue", 9, 0x0000FF);
       xcmp( "lightgreen", 10, 0x00FF00);
       xcmp( "lightcyan", 9, 0x00FFFF);
       xcmp( "lightmagenta", 12, 0xFF00FF);
       xcmp( "lightred", 8, 0xFF0000);
       xcmp( "lemon", 5, ARGB(255,250,205));
       break;
   case 'm':
       xcmp( "maroon", 6, 0x800000);
       xcmp( "magenta", 7, 0x800080);
       break;
   case 'n':
       xcmp( "navy", 4, 0x000080);
       break;
   case 'o':
       xcmp( "olive", 5, 0x808000);
       xcmp( "orange", 6, ARGB(255,165,0));
       break;
   case 'p':
       xcmp( "purple", 6, 0x800080);
       xcmp( "peach", 5, ARGB(255,218,185));
       xcmp( "peru", 4, ARGB(205,133,63));
       xcmp( "pink", 4, ARGB(255,192,203));
       xcmp( "plum", 4, ARGB(221,160,221));
       break;
   case 'q':
       break;
   case 'r':
       xcmp( "red", 3, 0x800000);
       xcmp( "royalblue", 9, ARGB(65,105,225));
       break;
   case 's':
       xcmp( "silver", 6, 0xC0C0C0);
       xcmp( "sienna", 6, ARGB(160,82,45));
       break;
   case 't':
       xcmp( "teal", 4, 0x008080);
       xcmp( "turquoise", 9, ARGB(64,224,208));
       xcmp( "tan", 3, ARGB(210,180,140));
       xcmp( "tomato", 6, ARGB(255,99,71));
       break;
   case 'u':
       break;
   case 'w':
       xcmp( "white", 5, 0xFFFFFF);
       xcmp( "wheat", 5, ARGB(245,222,179));
       break;
   case 'v':
       xcmp( "violet", 6, ARGB(238,130,238));
       break;
   case 'x':
       break;
   case 'y':
       xcmp( "yellow", 6, 0xFFFF00);
       break;
   case 'z':
       break;
   }

#undef xcmp

   return clInvalid;
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
   if ( !sys stockPalette)
      sys stockPalette = GetCurrentObject( sys ps, OBJ_PAL);
   font_free( sys fontResource);
   sys stylusResource = nil;
   sys fontResource   = nil;
   sys stylusFlags    = 0;
   sys stylus. extPen. actual = false;
   apt_set( aptDCChangeLock);
   sys bpp = GetDeviceCaps( sys ps, BITSPIXEL);
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
   SelectObject( sys ps,  sys stockPen);
   SelectObject( sys ps,  sys stockBrush);
   SelectObject( sys ps,  sys stockFont);
   SelectPalette( sys ps, sys stockPalette, 0);
   sys stockPen = sys stockBrush = sys stockFont = sys stockPalette = nilHandle;
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
   sys bpp = 0;
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

Handle
hwnd_frame_top_level( Handle self)
{
   while ( self && ( self != application)) {
      if (( sys className == WC_FRAME) ||
         ( !is_apt( aptClipOwner) && ( var owner != application))) return self;
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

typedef struct _SzList
{
   List l;
   int  sz;
   PRGBColor p;
} SzList, *PSzList;

static Bool
pal_count( Handle window, Handle self, PSzList l)
{
   if ( !is_apt( aptClipOwner) && ( window != application))
      return false;
   if ( var palSize > 0) {
      list_add( &l->l, self);
      l->sz += var palSize;
   }
   if ( var widgets. count > 0)
      CWidget( self)-> first_that( self, pal_count, l);
   return false;
}

static Bool
pal_collect( Handle self, PSzList l)
{
   memcpy( l-> p, var palette, var palSize * sizeof( RGBColor));
   l-> p  += var palSize;
   return false;
}

Bool
palette_change( Handle self)
{
   SzList l;
   PRGBColor p;
   PRGBColor d;
   int nColors = ( 1 << (
       guts. displayBMInfo. bmiHeader. biBitCount *
       guts. displayBMInfo. bmiHeader. biPlanes
   )) & 0x1FF;
   int i;
   HPALETTE pal;
   XLOGPALETTE xlp = {0x300};
   HDC dc;
   int rCol = 0;

   if ( nColors == 0)
      return false;

   self = hwnd_frame_top_level( self);
   if ( self == nilHandle) return false;

   list_create( &l.l, 32, 32);
   l. sz = 0;
   if ( var palSize > 0) {
      list_add( &l.l, self);
      l.sz += var palSize;
   }
   if ( var widgets. count > 0)
      CWidget( self)-> first_that( self, pal_count, &l);

   if ( l. l. count == 0) {
      list_destroy( &l.l);
      return false;
   }


   xlp. palNumEntries = l. sz;
   p = malloc( sizeof( RGBColor) * l. sz);
   d = malloc( sizeof( RGBColor) * nColors);

   l. p = p;
   list_first_that( &l.l, pal_collect, &l);
   cm_squeeze_palette( p, xlp. palNumEntries, d, nColors);
   xlp. palNumEntries = nColors;

   for ( i = 0; i < nColors; i++) {
      xlp. palPalEntry[ i]. peRed    = d[ i]. r;
      xlp. palPalEntry[ i]. peGreen  = d[ i]. g;
      xlp. palPalEntry[ i]. peBlue   = d[ i]. b;
      xlp. palPalEntry[ i]. peFlags  = 0;
   }

   free( d);
   free( p);

   pal = CreatePalette(( LOGPALETTE *) &xlp);

   dc  = GetDC( HANDLE);
   pal  = SelectPalette( dc, pal, 0);
   rCol = RealizePalette( dc);
   DeleteObject( SelectPalette( dc, pal, 0));
   ReleaseDC( HANDLE, dc);

   if ( rCol > 0) apc_widget_invalidate_rect( self, nil);

   list_destroy( &l.l);
   return true;
}

int
palette_match_color( XLOGPALETTE * lp, long clr, int * diffFactor)
{
   int diff = 0x10000, cdiff = 0, ret = 0, nCol = lp-> palNumEntries;
   RGBColor color;

   if ( nCol == 0) {
      if ( diffFactor) *diffFactor = 0;
      return clr;
   }

   color. r = clr & 0xFF;
   color. g = ( clr >> 8)  & 0xFF;
   color. b = ( clr >> 16) & 0xFF;

   while( nCol--)
   {
      int dr=abs((int)color. r - (int)lp-> palPalEntry[ nCol]. peRed),
          dg=abs((int)color. g - (int)lp-> palPalEntry[ nCol]. peGreen),
          db=abs((int)color. b - (int)lp-> palPalEntry[ nCol]. peBlue);
      cdiff=dr*dr+dg*dg+db*db;
      if ( cdiff < diff) {
         ret  = nCol;
         diff = cdiff;
         if ( cdiff == 0) break;
      }
   }

   if ( diffFactor) *diffFactor = cdiff;
   return ret;
}


long
palette_match( Handle self, long clr)
{
   XLOGPALETTE lp;
   int cdiff;
   RGBColor color;

   lp. palNumEntries = GetPaletteEntries( sys pal, 0, 256, lp. palPalEntry);

   if ( lp. palNumEntries == 0) {
      apiErr;
      return clr;
   }

   color. r = clr & 0xFF;
   color. g = ( clr >> 8)  & 0xFF;
   color. b = ( clr >> 16) & 0xFF;

   palette_match_color( &lp, clr, &cdiff);

   if ( cdiff >= COLOR_TOLERANCE)
      return clr;

   lp. palNumEntries = GetSystemPaletteEntries( sys ps, 0, 256, lp. palPalEntry);

   palette_match_color( &lp, clr, &cdiff);

   if ( cdiff >= COLOR_TOLERANCE)
      return clr;

   return PALETTERGB( color.r, color.g, color.b);
}
