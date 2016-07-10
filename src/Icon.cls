local @IconHandle
{
   Handle xorMask;
   Handle andMask;
}

object Prima::Icon( Prima::Image)
{
   unsigned char*    mask;
   int               maskType;
   int               maskLine;
   int               maskSize;
   Color             maskColor;
   int               maskIndex;
   int               autoMasking;
   property SV *     mask;
   property int      maskType;
   property Color    maskColor;
   property int      maskIndex;
   property int      autoMasking;
   method void       init( HV * profile);
   method Handle     extract( int x, int y, int width, int height);
   method Handle     dup();
   method void       set( HV * profile);
   method IconHandle split();
   method void       combine( Handle xorMask, Handle andMask);
   c_only void       create_empty( int width, int height, int type);
   c_only void       create_empty_icon( int width, int height, int type, int maskType);
   c_only void       update_change();
   c_only void       stretch( int width, int height);
   c_only Byte*      convert_mask( int type );
} 
