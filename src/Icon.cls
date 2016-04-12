local @IconHandle
{
   Handle xorMask;
   Handle andMask;
}

object Prima::Icon( Prima::Image)
{
   unsigned char*    mask;
   int               maskLine;
   int               maskSize;
   Color             maskColor;
   int               maskIndex;
   int               autoMasking;
   property SV *     mask;
   property Color    maskColor;
   property int      maskIndex;
   property int      autoMasking;
   method void       init( HV * profile);
   method Handle     dup();
   method IconHandle split();
   method void       combine( Handle xorMask, Handle andMask);
   c_only void       create_empty( int width, int height, int type);
   c_only void       update_change();
   c_only void       stretch( int width, int height);
} 
