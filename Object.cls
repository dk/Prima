object Object
{
   int  stage;
   int  protectCount;
   ObjectOptions options;
   method SV*	 can( char *name, Bool cacheIt = 1);
   method void   cleanup();
   method void   done();
   method void   init( HV * profile);
   import void   profile_add      ( SV * profile);
   import void   profile_check_in ( SV * profile);
   import SV *   profile_default  ();
   import void   set( HV * profile);
   method void   setup();
}
