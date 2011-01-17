#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
# $Id$

object Prima::Component( Prima::Object)
{
    void        *sysData;            # system-dependent data, defined by apc
    char        *name;
    ApiHandle   handle;              # apc handle

    property Bool      eventFlag;
    property SV *      delegations;
    property SV *      name;
    property Handle    owner;

    method   UV        add_notification( char * name, SV * subroutine, Handle referer = nilHandle, int index = -1);
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
    method   SV      * get_handle();
    public   void      get_notification( char * name, int index);
    c_only   void      handle_event ( PEvent event);
    method   void      init( HV * profile);
    method   int       is_owner( Handle objectHandle);
    c_only   Bool      message( PEvent event);
    c_only   Bool      migrate( Handle attachTo);
    import   SV *      notification_types();
    public   Bool      notify( char * format, ...);
    method   Bool      pop_event();
    method   void      post_message( SV * info1, SV * info2);
    import   SV *      profile_default();
    method   void      push_event();
    c_only   void      recreate();
    method   void      remove_notification( UV id);
    method   void      set( HV * profile);
    method   void      setup();
    public   void      set_notification( char * name, SV * subroutine);
    method   void      unlink_notifier( Handle referer);
    c_only   void      update_sys_handle( HV * profile);
    c_only   Bool      validate_owner( Handle * newOwner, HV * profile);
# internal variables
    char * evStack;
    int    evPtr;
    int    evLimit;
    PList  postList;
    PList  components;
    PList  evQueue;
    PList  refs;
    PList  events;
    PHash  eventIDs;
    int    eventIDCount;
}
