object Clipboard( Component)
{
   int    openCount;
   int    formatCount;
   void * formats;
   method void   clear();
   c_only void   close();
   method void   deregister_format( char * format);
   method void   done();
   method SV *   fetch( char * format);
   method Bool   format_exists( char * format);
   method SV *   get_handle();
   method int    get_format_count();
   public void   get_formats();
   method int    get_registered_format_count();
   public void   get_registered_formats();
   method void   init( HV * profile);
   c_only Bool   open();
   method Bool   register_format( char * format);
   method void   store( char * format, SV * data);
}
