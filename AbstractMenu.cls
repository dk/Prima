#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
object Prima::AbstractMenu ( Prima::Component)
{
   PMenuItemReg  tree;
   property Bool   selected;
   property char * accel  ( char * varName);
   property SV   * action ( char * varName);
   property Bool   checked( char * varName);
   property Bool   enabled( char * varName);
   property Handle image  ( char * varName);
   property SV   * key    ( char * varName);
   property char * text   ( char * varName);

   method void   done();
   method void   delete( char * varName);
   c_only void * first_that( void * actionProc, void * params, Bool useDisabled);
   method SV   * get_handle();
   method SV   * get_items( char * varName);
   method Bool   has_item( char * varName);
   method void   init( HV * profile);
   method void   insert( SV * menuItems, char * rootName, int index);
   import SV *   profile_default ();
   method void   set( HV * profile);
   method void   set_command( char * key, Bool enabled);
   method void   set_items( SV * menuItems);
   method void   set_variable ( char * varName, char * newName);
   static int    translate_accel( Handle self, char * accel);
   static int    translate_key( Handle self, int code, int key, int mod);
#  private
   Bool   anchored;
   c_only void   dispose_menu( void * menu);
   c_only void * new_menu( SV * sv, int level);
   c_only Bool   sub_call_id ( int sysId);
   c_only Bool   sub_call_key ( int key);
}
