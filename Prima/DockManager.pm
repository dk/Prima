#
#  Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#
# contains
#    DockManager
#    DockManager::LaunchPad
#    DockManager::Toolbar
#    DockManager::ToolbarDocker
#    DockManager::Panelbar
#    DockManager::S::SpeedButton;
#
# provides management for dockable toolbars and panels

use strict;
use Prima;
use Prima::Docks;
use Prima::Notebooks;
use Prima::MDI;
use Prima::Lists;
use Prima::StdBitmap;


# widget used for spawning new tools and destroying unneeded ones

package Prima::DockManager::LaunchPad;
use vars qw(@ISA);
@ISA = qw(Prima::Notebook Prima::SimpleWidgetDocker);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = ( 
      fingerprint => 0x0000FFFF,
      dockup      => undef,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);
   $self-> $_( $profile{$_}) for ( qw(fingerprint dockup));
   return %profile;
}   

# inner part of toolbar tandem. accepts tools

package Prima::DockManager::ToolbarDocker;
use vars qw(@ISA);
@ISA = qw(Prima::SingleLinearWidgetDocker);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = ( 
       parentDocker => undef,
       instance     => undef,
       x_sizeable   => 0,
       y_sizeable   => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   

sub init
{
   my ($self, %profile) = @_;
   %profile = $self-> SUPER::init( %profile);
   $self-> $_($profile{$_}) for qw( parentDocker instance);
   return %profile;
}   

sub get_extent
{
   my $self = $_[0];
   my @ext = (0,0);
   for ($self-> docklings) {
      my @z = $_-> rect;
      for (0,1) {
         $ext[$_] = $z[$_+2] if $ext[$_] < $z[$_+2]
      }
   }   
   return @ext; 
}   

sub update_size
{
   my ( $self, @sz) = @_;
   my $o = $self-> parentDocker;
   return unless $o;
   @sz = $self-> size unless scalar @sz;
   my @r  = $o-> client2frame( 0, 0, @sz);
   $o-> size( $r[2] - $r[0], $r[3] - $r[1]);
   $self-> size( @sz);
   if ( $o-> dock) {
      $o-> redock;
   } else {   
      @r = $o-> externalDocker-> client2frame( 0,0, @sz);
      $o-> externalDocker-> size($r[2] - $r[0], $r[3] - $r[1]);
      $self-> rect( 0,0,@sz); # respect growMode
   }
}   


# this part is responsible for changing toolbar's size when
# new toole are docked inside.
sub dock
{
   return $_[0]-> {dock} unless $#_;
   my $self = shift;
   my @sz = $self-> size;
   $self-> SUPER::dock(@_);
   my @sz1 = $self-> size;
   my $resize = 0;
   my @ext = $self-> get_extent;
   for ( 0, 1) {
      $resize = 1, $sz1[$_] = $ext[$_] if $sz1[$_] > $ext[$_];
   }
   return if !$resize && ($sz[0] == $sz1[0] && $sz[1] == $sz1[1]);
   $self-> size( @sz1);
   $self-> update_size( @sz1);
}   

sub rearrange
{
   my $self = $_[0];
# fast version of rearrange, without real redocking
   my $v = $self-> vertical;
   my @ext = (0,0);
   my ( $xid, $yid) = ( $v ? 0 : 1, $v ? 1 : 0);
   my $a;
   for ( $self-> docklings) {
      $a = $_ unless $a; # 
      my @sz = $_-> size;
      $_-> origin( $v ? ( 0, $ext[1]) : ( $ext[0], 0));
      $ext[$xid] = $sz[$xid] if $ext[$xid] < $sz[$xid];
      $ext[$yid] += $sz[$yid];
   }  
   if ( $a) {
      $self-> size( @ext);
      #innvoke dock-undock, just to be sure, but for only one widget
      $self-> redock_widget( $a);
   }
}   

sub parentDocker
{
   return $_[0]-> {parentDocker} unless $#_;
   $_[0]-> {parentDocker} = $_[1];
}   

sub instance
{
   return $_[0]-> {instance} unless $#_;
   $_[0]-> {instance} = $_[1];
}   


sub on_dockerror
{
   my ( $self, $urchin) = @_;
   $self-> redock_widget( $urchin);
}   


sub on_undock
{
   my $self = $_[0];
   return if scalar(@{$self->{docklings}});
   my $i = $self-> instance;
   $i-> post( sub {
      return if scalar(@{$self->{docklings}});
      if ( $self-> parentDocker-> autoClose) {
         $self-> parentDocker-> destroy;
      } else {
         $i-> toolbar_visible( $self-> parentDocker, 0);
         $i-> notify(q(ToolbarChange)); 
      }   
   });
}   

package Prima::DockManager::Bar;

sub childDocker
{
   return $_[0]-> {childDocker} unless $#_;
   $_[0]-> {childDocker} = $_[1];
}   

sub instance
{
   return $_[0]-> {instance} unless $#_;
   $_[0]-> {instance} = $_[1];
}   

# external part of toolbar tandem. main difference from ancestor that is
# autoClose property, that is used to distinguish whether toolbar would 
# destroy after all tools were undocked, or just hide. the particular 
# behavior is implemented in Prima::DockManager::ToolbarDocker, which is 
# ::childDocker.

package Prima::DockManager::Toolbar;
use vars qw(@ISA);
@ISA = qw(Prima::LinearDockerShuttle Prima::DockManager::Bar);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = ( 
      instance    => undef,
      childDocker => undef,
      autoClose   => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);
   $self-> $_( $profile{$_}) for ( qw(instance childDocker autoClose));
   return %profile;
}   

sub autoClose
{
   return $_[0]-> {autoClose} unless $#_;
   $_[0]-> {autoClose} = $_[1];
}   

sub on_getcaps
{
   my ( $self, $docker, $prf) = @_;
   $self-> SUPER::on_getcaps( $docker, $prf);
   my @cd = $self-> {childDocker}-> size;
   my @i = @{$self-> {indents}};
   my @sz = ($i[2] + $i[0] + $cd[0], $i[3] + $i[1] + $cd[1]);
   $prf-> {sizeMin} = [ @sz];
   my $vs = $docker-> can('vertical') ? $docker-> vertical : 0;
   my $v  = $self-> {vertical};
   $prf-> {sizes} = ( $v == $vs) ? [[@sz]] : [[@sz[1,0]]];
}   

sub on_dock
{
   my $self = $_[0];
   my $nv   = $self-> dock-> can('vertical') ? $self-> dock-> vertical : 0; 
   return if $nv == $self-> {vertical};
   $self-> vertical( $nv);
   my $c = $self-> {childDocker};
   $c-> vertical( $nv);
   $c-> rect( $self-> frame2client( 0, 0, $self-> size));
   $c-> rearrange;
}   

# Shuttle for panels. Panels are same docking windows as toolbars,
# but they are: 
#  - always destroyed by close action
#  - they are sizeable
#  - they usually contain only one widget ( e.g. panel)

package Prima::DockManager::Panelbar;
use vars qw(@ISA);
@ISA = qw(Prima::LinearDockerShuttle Prima::DockManager::Bar);


sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = ( 
      vertical    => 1,
      instance    => undef,
      childDocker => undef,
      externalDockerClass => 'Prima::MDIExternalDockerShuttle',
      x_sizeable  => 1,
      y_sizeable  => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);
   $self-> $_( $profile{$_}) for ( qw(instance childDocker));
   return %profile;
}   

# flags for fingerprints - for different dockers and stages.
package dmfp;

use constant Tools     => 0x0F000; # those who want tools, must set this
use constant Toolbar   => 0x10000; # those who want toolbars, must set this
use constant LaunchPad => 0x20000; # tools that want to be disposable, set this


# main instance for docking. 
# it servers as:
#  - docking root - e.g. all docking relations must be connected somehow to 
#    it's hierarhy ( use ::add_subdocker).
#  - command state holder - especially good for speed buttons, that should
#    be enabled/disabled in different states.
#  - toolbar and panel visibility manager - they could hide as your window
#    minimizes
#  .. and some amount of helping routines

package Prima::DockManager;
use vars qw(@ISA);
@ISA = qw(Prima::Component Prima::AbstractDocker::Interface);

{
my %RNT = (
   %{Prima::Component->notification_types()},
   CommandChange  => nt::Notification, # when command set changes
   ToolbarChange  => nt::Notification, # on toolbar visiblity change
   PanelChange    => nt::Notification, # on panel visibility change 
                                       #( actually, panel is destroyed if not visible)
   Command        => nt::Command,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = ( 
      interactiveDrag => 0,
      commands        => {},
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   

sub init
{
   my $self = shift;
   $self-> {commands} = {};
   $self-> {interactiveDrag} = 0;
   my %profile = $self-> SUPER::init( @_);
   $self-> {classes} = [];
   $self-> {hiddenToolbars} = [];
   $self-> {toolbars} = [];
   $self-> {panels} = [];
   $self-> $_( $profile{$_}) for ( qw( commands interactiveDrag));
   return %profile;
}   

# tools may be created and destroyed interactively in the modal state ::interactiveDrag(1) 

sub interactiveDrag
{
   return $_[0]-> {interactiveDrag} unless $#_;
   my ( $self, $id) = @_;
   return if $id == $self-> {interactiveDrag};
   $self-> {interactiveDrag} = $id;
   if ( $id) {
      for ( $self-> toolbars)  {
         $_-> enabled(1) for $_-> childDocker-> docklings;
      }   
   } else {   
      my $c = $self-> {commands};
      for ( $self-> toolbars)  {
         for ( $_-> childDocker-> docklings) {
            next if !defined $_->{CLSID} || !exists($c-> {$_->{CLSID}}) || $c-> {$_->{CLSID}};
            $_-> enabled(0);
         }   
      }   
   }   
}   

sub toolbars { return @{$_[0]->{toolbars}}}   
sub panels   { return @{$_[0]->{panels}}}  

sub fingerprint { return 0xFFFFFFFF }   

sub register_tool
{
   my ( $self, $CLSID, $hash) = @_;
   $hash-> {tool} = 1;
   push @{$self-> {classes}}, $CLSID, $hash;
}   

sub register_panel
{
   my ( $self, $CLSID, $hash) = @_;
   $hash-> {tool} = 0;
   push @{$self-> {classes}}, $CLSID, $hash; 
}   

sub get_class
{
   my ( $self, $CLSID) = @_;
   my $i;
   my $c = $self-> {classes};
   my $x = scalar @$c;
   for ( $i = 0; $i < $x; $i+=2) {
      return $$c[$i+1] if $CLSID eq $$c[$i];
   }   
}   

sub panel_by_id
{
   my ( $self, $name) = @_;
   for ( @{$self-> {panels}}) {
      return $_ if $_-> {CLSID} eq $name;
   }   
}   


sub toolbar_by_name
{
   my ( $self, $name) = @_;
   for ( @{$self-> {toolbars}}) {
      return $_ if $_-> name eq $name;
   }   
}   

sub post
{
   my ( $self, $sub, @parms) = @_;
   $self-> post_message( 'sub', [ $sub, @parms]);
}   

sub on_postmessage
{
   my ( $self, $id, $val) = @_;
   return unless $id eq 'sub';
   my $sub = shift @$val;
   $sub->( $self, @$val);
}   

# creates the navigation controls on given widget and returns the docker created.

sub create_manager
{
   my ( $self, $where, %profile) = @_; 
   my @o   = exists($profile{origin}) ? @{$profile{origin}} : (0,0);
   my @sz  = exists($profile{size})   ? @{$profile{size}}   : ($where-> size);
   my ( @items, %items);
   my @lclasses;
   my $i = 0;
   my $cls = $self-> {classes};
   my $xcls = scalar @$cls;
   for ( $i = 0; $i < $xcls; $i += 2) {
      my ( $CLSID, $hash) = @$cls[$i, $i+1];
      next unless $hash->{tool};
      push ( @lclasses, $CLSID);
      $CLSID =~ m/^([^\:]+)(?:\:|$)/;
      next if !$1 || exists($items{$1});
      $items{$1} = 1;
      push( @items, $1);
   }
   $i = 0;
   my $nb;
   my $lb = $where-> insert( ListBox => 
      origin  => [@o],
      size    => [ int($sz[0] / 3), $sz[1]],
      name    => 'GroupSelector',
      vScroll => 1,
      items   => \@items,
      growMode=> gm::Client,
      focusedItem => 0,
      onSelectItem => sub {
         my ($self,$lst,$state) = @_;
         return unless $state;
         $nb-> pageIndex( $self-> focusedItem);
      },   
      exists($profile{listboxProfile}) ? %{$profile{listboxProfile}} : (),
   );
   $nb = $where-> insert( 'Prima::DockManager::LaunchPad' => 
      exists($profile{dockerProfile}) ? %{$profile{dockerProfile}} : (), 
      origin      => [ $o[0] + int($sz[0] / 3), 0],
      size        => [ $sz[0] - int($sz[0] / 3), $sz[1]],
      name        => 'PageFlipper',
      pageCount   => scalar(@items),
      growMode    => gm::GrowHiY|gm::GrowLoX,
      fingerprint => dmfp::LaunchPad, 
      dockup      => $self,
   );
   $nb-> {dockingRoot} = $self;
   $i = 0;
   my %x = @$cls;
   my @szn = $nb-> size;
   
   for ( @items) {
      my $iid = $_;
      my @d = grep { m/^([^\:]+)(?:\:|$)/; my $j = $1 || ''; $j eq $iid; } @lclasses;
      my @org  = (5,5);
      my $maxy = 0;
      my @ctrls;
      my $ix = 0;
      for ( @d) {
         my %acp = exists($x{$_}-> {profile}) ? %{$x{$_}-> {profile}} : ();
         my $ctrl = $nb-> insert_to_page( $i, $x{$_}->{class} => 
            growMode => gm::GrowLoY,
            %acp,
            onMouseDown => \&Control_MouseDown_FirstStage,
            onKeyDown   => \&Control_KeyDown,
            origin      => [ @org],
         );
         $ctrl-> {CLSID} = $_;
         $ctrl-> {DOCKMAN} = $self;
         push ( @ctrls, $ctrl);
         my @s = $ctrl-> size;
         if (( $s[0] + $org[0] > $szn[0] - 5) && ( $ix > 0)) {
            $ctrl-> origin( $org[0] = 5, $org[1] += $maxy + 3);
            $maxy = 0;
         } else {
            $org[0] += $s[0] + 1;
            $maxy = $s[1] if $maxy < $s[1];
         }   
         $ix++;
      }   
      if ( $org[1] + $maxy < $szn[1] - 5) {
         my $d = $sz[1] - 5 - $org[1] - $maxy;
         for ( @ctrls) {
            my @o = $_-> origin;
            $_-> origin( $o[0], $o[1] + $d);
         }   
      }   
      $i++;
   }   
   return $lb, $nb;
}   

# creates tool on given widget with rect given
# disables it if it's command is disabled

sub create_tool
{
   my ( $self, $where, $CLSID, @rect) = @_;
   my $x = $self-> get_class( $CLSID);
   return unless $x;
   my %acp = exists($x-> {profile}) ? %{$x-> {profile}} : ();
   %acp = ( %acp,
      onMouseDown => \&Control_MouseDown,
      onKeyDown   => \&Control_KeyDown,
      onDestroy   => \&Control_Destroy,
   );
   $acp{rect} = \@rect if 4 == scalar @rect;
   my $ctrl = $where-> insert( $x->{class} => %acp);
   $ctrl-> {CLSID}   = $CLSID;
   $ctrl-> {DOCKMAN} = $self;
   $ctrl-> enabled(0) if !$self-> {interactiveDrag} && exists( $self-> {commands}->{$CLSID})
     && !$self-> {commands}->{$CLSID};
   return $ctrl;
}   

# creates toolbar that is a tandem of Toolbar and ToolbarDocker.
# accepted profile keys:
#  - vertical
#  - dock ( dock manager or undef)
#  - visible
#  - rect ( client rectangle relative to dock ( or application))
#  - dockerProfile  - widget profile for Toolbar
#  - toolbarProfile - widget profile for ToolbarDocker

sub create_toolbar
{
   my ( $self, %profile) = @_;
   my $v    = $profile{vertical} || 0; 
   my $dock = $profile{dock};
   my $auto = exists( $profile{autoClose}) ? $profile{autoClose} : ( exists($profile{name}) ? 1 : 0);
   my $name = $profile{name} || $self-> auto_toolbar_name;
   my $visible = exists($profile{visible}) ? $profile{visible} : 1;
   my @r    = $profile{rect} ? @{$profile{rect}} : ( 0, 0, 10, 10); 
   my $acd  = $profile{dockerProfile}  || {};
   my $aci  = $profile{toolbarProfile} || {};

   my $x = Prima::DockManager::Toolbar-> create(
      dockingRoot => $self,
      name        => $name,
      text        => $name,
      visible     => $visible,
      vertical    => $v,
      instance    => $self,
      autoClose   => $auto,
      onEDSClose  => \&Toolbar_EDSClose,
      %$acd,
   );
   
   my @i = @{$x-> indents};
   $x-> rect( $r[0] - $i[0], $r[1] - $i[1], $r[2] + $i[2], $r[3] + $i[3]);
   @r = $x-> frame2client( $x-> rect);
   
   my $xcl = $x-> insert( 'Prima::DockManager::ToolbarDocker' => 
      %$aci,
      origin       => [ @i[0,1]],
      size         => [ $r[2] - $r[0], $r[3] - $r[1]],
      vertical     => $v,
      growMode     => gm::Client,
      fingerprint  => dmfp::Toolbar,
      parentDocker => $x,
      name         => $name,
      instance     => $self,
      onDestroy   => \&Toolbar_Destroy,
   );
   $x-> client( $xcl);
   $x-> childDocker( $xcl);
   $self-> add_subdocker( $xcl);
   if ( $profile{dock}) {
      $x-> dock( $dock, $dock-> client_to_screen( $x-> rect));
   } else {
      $x-> externalDocker-> rect( $x-> externalDocker-> client2frame( @r));
   }   
   push ( @{$self-> {toolbars}}, $x);   
   $self-> toolbar_visible( $x, 0) unless $visible;
   $self-> notify(q(ToolbarChange));
   return $x, $xcl;
}   

# creates panel ( Panelbar) of given CLSID
# profile keys accepted:
#  - dockerProfile - widget profile for Panelbar
#  - profile       - widget profile for CLSID class

sub create_panel
{
   my ( $self, $CLSID, %profile) = @_;
   my $prf = $self-> get_class( $CLSID);
   return unless $prf;
   my %prf = (
      dockingRoot         => $self,
      x_sizeable          => 1,
      y_sizeable          => 1,
      instance            => $self,
   );
   $prf{text} = $prf-> {text} if exists $prf-> {text};
   my $x = Prima::DockManager::Panelbar-> create( %prf,
      $prf-> {dockerProfile} ? %{$prf-> {dockerProfile}} : (),
      $profile{dockerProfile} ? %{$profile{dockerProfile}} : (),
      dock  => undef,
   );
   $prf-> {text} = $x-> text unless exists $prf-> {text};
   my @rrc = $x-> frame2client( 0, 0, $x-> size);
   my $xcl = $x-> insert( $prf->{class} => 
      growMode => gm::Client,
      $prf-> {profile} ? %{$prf-> {profile}} : (),
      $profile{profile} ? %{$profile{profile}} : (),
      rect      => [@rrc],
   );
   $xcl-> add_notification( 'Destroy', \&Panelbar_Destroy, $x);
   $x-> client( $xcl);
   push( @{$self-> {panels}}, $x);   
   $x-> {CLSID} = $CLSID;
   $x-> childDocker($xcl);
   if ( $prf-> {dockerProfile}-> {dock} || $profile{dockerProfile}-> {dock}) {
      my $dock = $prf-> {dockerProfile}-> {dock} || $profile{dockerProfile}-> {dock};
      $x-> dock( $dock, $dock-> client_to_screen( $x-> rect));
   } 
   return ( $x, $xcl);
}   

# returns synthesized unique toolbar name, a la Toolbar1, Toolbar2 etc

sub auto_toolbar_name
{
   my $name;
   my $self = $_[0];
   my @ids;
   for ( @{$self->{toolbars}}) {
      my $x = $_-> name;
      next unless $x =~ m/^ToolBar(\d+)$/;
      $ids[$1] = 1;
   }
   my $i = 0;
   for ( @ids) {
      $i++, next unless $i; # skip ToolBar0
      $name = $i, last unless $_;
      $i++;
   }   
   $name = scalar(@ids) unless defined $name;
   $name++ unless $name;
   return "ToolBar$name";
}   


# productivity routines, returns typical pre-built menu items, that are
# responsive for toolbars' and panels' visibility

sub toolbar_menuitems
{
   my ( $self, $sub) = @_;
   my @items;
   for ( @{$self->{toolbars}}) {
      my $vis = $_-> dock() ? $_-> visible : $_-> externalDocker-> visible;
      $vis = ( $vis ? '*' : '') . $_-> name;
      push ( @items, [ $vis => $_-> name => $sub ]);
   }
   return \@items;
}   

sub panel_menuitems
{
   my ( $self, $sub) = @_;
   my @items;
   my %h = map { $_->{CLSID} => 1} @{$self-> {panels}};
   my $i;
   my $c = $self-> {classes};
   for ( $i = 0; $i < @$c; $i += 2) {
      my ( $CLSID, $hash) = @$c[$i,$i+1];
      next if $hash->{tool};
      my $vis = ( $h{$CLSID} ? '*' : '') . $CLSID;
      push ( @items, [ $vis => $hash-> {text} => $sub ]);
   }
   return \@items;
}   


# toolbar_visible and panel_visible are good for calling from
# the menu handlers

sub toolbar_visible
{
   my ( $self, $d, $visible) = @_;
   return unless $d;
   if ( $d-> dock) {
      return if $visible == $d-> visible;
      if ( $visible) {
         $d-> dock_back;
      } else {
         $d-> dock( undef);
         $d-> externalDocker-> visible( $visible);
      }   
   } else {
      return if $visible == $d-> externalDocker-> visible;
      $d-> externalDocker-> visible( $visible);
   }   
}   

sub panel_visible
{
   my ( $self, $panelbarCLSID, $visible) = @_;
   my $d = $self-> panel_by_id( $panelbarCLSID);
   my $hash = $self-> get_class( $panelbarCLSID);
   if ( $visible) {
      return if $d;
      my %pf;
      if ( $hash-> {lastUsedDock} && Prima::Object::alive($hash-> {lastUsedDock})) {
         $pf{dockerProfile}->{dock} = $hash-> {lastUsedDock};
      }
      if ( $hash-> {lastUsedRect}) {
         $pf{dockerProfile}->{rect} = $hash-> {lastUsedRect};
      }   
      my ( $x, $xcl) = $self-> create_panel( $panelbarCLSID, %pf);
      $x-> bring_to_front;
   } else {   
      return unless $d;
      $hash-> {lastUsedDock} = $d-> dock;
      $hash-> {lastUsedRect} = [$d-> dock ? $d-> rect : $d-> externalDocker-> rect],
      $d-> close;
   }
}   

# predefined toolbars are created unless they already present - 
# for example, application may need some toolbar always presented.
# profile keys recognized:
# - name   - the name of toolbar 
# - dock   - where to dock, if any
# - origin - relative to dock ( size cannot be set this time, so origin instead of rect)
# - list   - list of CLSIDs, which tools should be inserted

sub predefined_toolbars
{
   my $self = shift;
   my %toolbars = map { $_-> name => $_ } @{$self-> {toolbars}};
   my %c = @{$self-> {classes}};
   my @o  = ( 10, $::application-> height - 100);
   my @as = $::application-> size;
   for ( @_) {
      my $rec = $_;
      next if $toolbars{$_-> {name}};
      my @org = (0,0);
      my $maxy = 0;
      my @ctrls;
      my @list = $rec->{list} ? @{$rec->{list}} : ();
      for ( @list) {
         my $ctrl = $self-> create_tool( $::application, $_);
         next unless $ctrl;
         $ctrl-> origin( @org);
         my @sz = $ctrl-> size;
         $org[0] += $sz[0];
         $maxy = $sz[1] if $maxy < $sz[1];
         push ( @ctrls, $ctrl);
      }
      my @oz = $rec->{origin} ? @{$rec->{origin}} : ( $rec->{dock} ? (0,0) : @o);
      my ( $x, $xcl) = $self-> create_toolbar( 
         name    => $_->{name}, 
         rect    => [ @oz, $oz[0]+$org[0], $oz[1]+$maxy],
         visible => 1,
         dock    => $rec->{dock},
      );
      for ( @ctrls) {
         $_-> owner( $xcl);
         $xcl-> dock( $_);
      }   
      $xcl-> rearrange;
      $o[0] += 25;
      $o[1] -= 25;
   }   
}   

# panels created unless already present
# format is: pairs of CLSID and dock, f.ex. "CLSID1" => $dock1, "CLSID2" => $dock2
sub predefined_panels
{
   my ( $self, @rec) = @_;
   my $i;
   my %pan = map { $_-> {CLSID} => 1 } @{$self-> {panels}};
   for ( $i = 0; $i < scalar @rec; $i += 2) {
      my ( $CLSID, $dock) = @rec[$i, $i+1];
      next if $pan{$CLSID};
      my ( $a, $b) = $self-> create_panel( $CLSID, dockerProfile => {dock => $dock});
   }   
}   

# helpers for window state events
#
# brings to front all visible undocked bars

sub activate
{
   my $self = $_[0];
   for ( @{$self->{panels}}, @{$self-> {toolbars}}) {
      next if $_-> dock;
      $_-> externalDocker-> bring_to_front if $_-> externalDocker;
   }   
}   

# reflects window state of an imaginary window. bars become hidden when $ws is ws::Minimized,
# and back again visible otherwise

sub windowState
{
   my ( $self, $ws) = @_;
   if ( $ws == ws::Minimized) {
     for ( @{$self->{panels}}, @{$self-> {toolbars}}) { 
         next if $_-> dock;
         my $e = $_-> externalDocker;
         next unless $e;
         $e-> hide;
         push ( @{$self->{hiddenToolbars}}, $e);
      }   
   } else { 
      $_-> show for @{$self->{hiddenToolbars}};
      @{$self->{hiddenToolbars}} = ();
   }   
}  

# shortcut to enable or disable selected commands, since following 
# ::commands affects all commands.

sub commands_enable
{
   my ( $self, $enable) = ( shift, shift);
   my %cmds = map { $_ => 1 } @_;
   unless ( $self-> interactiveDrag) {
      for ( $self-> toolbars)  {
         for ( $_-> childDocker-> docklings) {
            next if !defined $_->{CLSID} || !$cmds{$_->{CLSID}} || $enable == $self-> {commands}->{$_->{CLSID}};
            $_-> enabled( $enable);
         }   
      }   
   }
   for ( keys %{$self->{commands}}) {
      next unless $cmds{$_};
      $self-> {commands}-> {$_} = $enable;
   }   
   $self-> notify(q(CommandChange));
}   

sub commands 
{
   return $_[0]-> {commands} unless $#_;
   my ( $self, $cmds) = @_;
   $self-> {commands} = $cmds;
   unless ( $self-> interactiveDrag) {
      for ( $self-> toolbars)  {
         for ( $_-> childDocker-> docklings) {
            next if !defined $_->{CLSID} || !$cmds-> {$_->{CLSID}};
            $_-> enabled( $cmds-> {$_-> {CLSID}});
         }   
      }
   }
   $self-> notify(q(CommandChange));
}   

# internals

sub autodock
{
   my ( $self, $ctrl) = @_;
   my $dock = $ctrl-> owner;
   $dock-> undock( $ctrl);

   my ( $x, $xcl) = $self-> create_toolbar( 
      vertical  => $dock-> can('vertical') ? $dock-> vertical : 0,
      dock      => $dock,
      rect      => [$ctrl-> rect],
      autoClose => 1,
   );
   $ctrl-> owner( $xcl);
   $ctrl-> origin( 0,0);
   $xcl-> dock( $ctrl);
   return $x;
}


sub Control_KeyDown
{
   return unless $_[0]-> {DOCKMAN}-> interactiveDrag;
   $_[0]-> clear_event;
}   

sub Control_MouseDown_FirstStage
{
   my ($self,$btn, $mod, $x,$y) = @_;
   return unless $btn == mb::Left;
   my $man = $self-> {DOCKMAN};
   my $c = Prima::InternalDockerShuttle-> create(
      owner       => $self-> owner,
      rect        => [$self-> rect],
      dockingRoot => $man,
      fingerprint => dmfp::LaunchPad | dmfp::Toolbar | dmfp::Tools, # allow all docks 
      backColor   => cl::Yellow,
      onLanding   => \&InternalDockerShuttle_Landing,
      name        => 'FirstStage',
      onDock      => sub {
         my $me = $_[0];
         if ( $me-> owner-> isa(q(Prima::DockManager::LaunchPad))) {
            $man-> post( sub { $me-> destroy; });
            return;
         }
         my $ctrl = $man-> create_tool( $me-> owner, $self-> {CLSID}, $me-> rect);
         return unless $ctrl;
         $me-> {dock} = undef;
         $me-> owner-> replace( $me, $ctrl);
         $man-> post( sub { $me-> destroy; });   
         $man-> autodock( $ctrl) unless $me-> owner-> isa(q(Prima::DockManager::ToolbarDocker));
      },   
      onFailDock => sub {
         my ( $me, $ax, $ay) = @_;
         my ( $x, $xcl) = $man-> create_toolbar( rect => [$me-> rect], autoClose => 1);
         $xcl-> dock( $man-> create_tool( $xcl, $self-> {CLSID}));
         $x-> externalDocker-> origin( $ax, $ay);
         $man-> post( sub { $me-> destroy; });   
      },   
   );
   $c-> externalDocker-> hide;
   $::application-> yield;
   $c-> drag( 1, [ $self-> client_to_screen(0,0,$self-> size)], $c-> screen_to_client( $self-> client_to_screen($x, $y)));
   $self-> clear_event;
}   

sub Control_Destroy
{
   $_[0]-> owner-> undock( $_[0]);
}   

sub Control_MouseDown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   my $man = $self-> {DOCKMAN};
   return unless $man-> interactiveDrag;
   $self-> clear_event;
   return unless $btn == mb::Left;
   
   my $c;
   $c = Prima::InternalDockerShuttle-> create(
      owner       => $self-> owner,
      rect        => [$self-> rect],
      dockingRoot => $man,
      fingerprint => dmfp::LaunchPad | dmfp::Toolbar | dmfp::Tools, # allow all docks  
      backColor   => cl::White,
      onLanding   => \&InternalDockerShuttle_Landing,
      name        => 'SecondStage',
      onDock      => sub {
         my $me = $_[0];
         $me-> {dock} = undef;
         $me-> owner-> replace( $me, $self);
         $man-> post( sub { $me-> destroy; });   
         if ( $me-> owner-> isa(q(Prima::DockManager::LaunchPad))) {
            $man-> post( sub { $self-> destroy; });   
            return;
         }
         $man-> autodock( $self) unless $me-> owner-> isa(q(Prima::DockManager::ToolbarDocker));
      }, 
      onFailDock => sub {
         $self-> owner-> replace( $c, $self);
         $c-> {dock} = undef;
         $man-> post( sub { $c-> destroy; });
      },   
   );
   $c-> {dock} = $self-> owner;
   $self-> owner-> replace( $self, $c);
   $c-> externalDocker-> hide;
   $c-> hide;
   $::application-> yield;
   $c-> drag( 1, [ $self-> client_to_screen(0,0,$self-> size)], $c-> screen_to_client( $self-> client_to_screen($x, $y)));
}   

sub Panelbar_Destroy
{
   my $self = $_[0];
   my $i = $self-> instance;
   return unless $i;
   @{$i-> {panels}} = grep { $_ != $self } @{$i->{panels}};
   @{$i-> {hiddenToolbars}} = grep { $_ != $self } @{$i->{hiddenToolbars}};
   $i-> notify(q(PanelChange));
}   

sub Toolbar_Destroy
{
   my $self = $_[0]-> parentDocker;
   my $i = $self-> instance;
   return unless $i;
   @{$i-> {toolbars}} = grep { $_ != $self } @{$i->{toolbars}};
   @{$i-> {hiddenToolbars}} = grep { $_ != $self } @{$i->{hiddenToolbars}};
   $i-> notify(q(ToolbarChange));
}   

sub Toolbar_EDSClose
{
   my $e = $_[0]-> externalDocker;
   $e-> hide;
   $_[0]-> clear_event;
   $_[0]-> instance-> notify(q(ToolbarChange));
}  

sub InternalDockerShuttle_Landing
{
   my ( $self, $dm, @rc) = @_;
   return unless $self-> {drag}; # only for interactive
   my $wi = $::application-> get_widget_from_point($::application-> pointerPos);
   return if !$wi || $wi == $dm;
   unless ( $wi-> can('dock')) {
      $wi = $wi-> owner;
      return if $wi == $dm;
   }
   return unless $wi-> can('dock') && $wi-> isa('Prima::DockManager::ToolbarDocker');
   $self-> clear_event;
}

# this package simplify a bit the creation of a speed button, that are able to dock

package Prima::DockManager::S::SpeedButton;

sub class
{
   my ( $image, $action, %profile) = @_;
   $image =~ s/\:(\d+)$//;
   my $index = $1 || 0;
   my $i = Prima::Icon-> create;
   undef($i) unless $i-> load(Prima::find_image($image), index => $index);
   return $action, {
      class   => 'Prima::SpeedButton',
      profile => {
         size        => [ 24, 24],
         image       => $i,
         borderWidth => 1,
         onClick     => \&on_click,
         %profile,
      },   
   },   
}   

sub on_click
{
   $_[0]-> owner-> instance-> notify(q(Command), $_[0]-> {CLSID});
}  



1;
