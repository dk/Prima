local @IconHandle
{
   Handle xorMask;
   Handle andMask;
}

object Icon( Image)
{
   unsigned char*    mask;
   int               maskLine;
   int               maskSize;
   method Handle     dup();
   method IconHandle split();
   method void       combine( Handle xorMask, Handle andMask);
   c_only void       create_empty( int width, int height, int type);
   c_only void       update_change();
   c_only void       update_mask_change();
}
