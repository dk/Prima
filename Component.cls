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
object Component( Object)
{
    void        *sysData;            # system-dependent data, defined by apc
    char        *name;
    Handle      owner;
    Handle      delegateTo;
    ApiHandle   handle;              # apc handle
    method   void      attach( Handle objectHandle);
    method   Handle    bring( char * componentName);
    method   Bool      can_event();
    method   void      cleanup();
    method   void      clear_event();
    method   void      done();
    method   void      detach( Handle objectHandle, Bool kill);
    method   void      event_error();
    c_only   Handle    first_that_component( void * actionProc, void * params);
    public   void      get_components();
    method   Bool      get_event_flag();
    method   SV      * get_handle();
    method   char    * get_name();
    method   Handle    get_owner();
    method   Handle    get_delegate_to();
    c_only   HV      * get_dynas();
    c_only   HV      * get_delegators();
    c_only   void      handle_event ( PEvent event);
    method   void      init( HV * profile);
    c_only   Bool      message( PEvent event);
    c_only   Bool      migrate( Handle attachTo);
    import   SV *      notification_types();
    public   void      notify( char * methodName);
    method   Bool      pop_event();
    method   void      post_message( SV * info1, SV * info2);
    import   SV *      profile_default();
    method   void      push_event();
    c_only   void      recreate();
    method   void      set( HV * profile);
    method   void      setup();
    method   void      set_event_flag( Bool eventFlag);
    method   void      set_name( char * name);
    method   void      set_delegate_to( Handle delegateTo);
    c_only   void      set_dyna_method( char * methodName, SV * container, SV** variable);
    import   void      set_owner( Handle owner);
    c_only   void      update_sys_handle( HV * profile);
    method   void      update_delegator();
# virtual event handlers
    event    void      on_create();
    event    void      on_destroy();
    event    void      on_postmessage( SV * info1, SV * info2);
# dynamic event handlers
    SV *   onCreate;
    SV *   onDestroy;
    SV *   onPostMessage;
# internal variables
    DelegatedMessages delegatedMessages;
    char * evStack;
    int    evPtr;
    int    evLimit;
    PList  postList;
    PList  components;
    PList  evQueue;
    PList  refList;
}
