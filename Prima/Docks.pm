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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#
# contains:
#    AbstractDocker::Interface
#      SimpleWidgetDocker
#      ClientWidgetDocker
#      LinearWidgetDocker 
#      FourPartDocker
#    ExternalDockerShuttle
#    InternalDockerShuttle 
#      LinearDockerShuttle 
#      SingleLinearWidgetDocker 
#
# provides basic interface and basic widgets for dockable widgets

package Prima::Docks;

use Prima;
use strict;
use Tie::RefHash;

# This is the interface to the dock widget where other widgets can be set in.
# The followings subs provide minimal necessary interface to negotiate between
# two parties.

package Prima::AbstractDocker::Interface;

# ::open_session:
# opens a docking session. when client wants to dock, it could easily trash down
# the server with ::mouse_move's, when on the every event docking request would be 
# called. the sessions are used to calculate as much as possible once, to make
# ::query call lighter. default behaviour caches all children docks, and provides
# queue-like invocation on ::next_docker. the ::query call returns either the docking
# position ( should be implemented in descendants) or the child dock. in that case
# ::query and ::next_docker works like findfirst/findnext sequence. the ::query call
# implemented here sets the children dock queue pointer to the start.
# 
# profile format:
#   self  => InternalDockerShuttle descendant
#   sizes => [[x,y], [x,y], [x,y], ...] set of shapes that docking window can wear
#   position => [ x, y], the wanted position ( in widget's coordinates)
#   sizeable => [ bool, bool], if none of shapes accepted, whether docking window can resize itself
#   sizeMin  => [ x, y], the minimal size that docking window can accept ( used only if sizeable)

sub open_session
{
   my ( $self, $profile) = @_;
   return unless $self-> check_session( $profile);
   my @mgrs = grep { $_-> isa( 'Prima::AbstractDocker::Interface') } $self-> get_components;
   if ($self-> {subdockers}) {
      @{$self-> {subdockers}} = grep { $_-> alive} @{$self-> {subdockers}};
      push( @mgrs, @{$self-> {subdockers}});
   }
   return { 
      SUBMGR    => \@mgrs,
      SUBMGR_ID => -1,
   };
}


# more-less debugging-state function
sub check_session
{
   my $p = $_[1];
   return 1 if $$p{CHECKED_OK};
   warn("No 'self' given\n"), return 0 unless $$p{self};
   for ( qw( sizes)) {
      warn("No '$_' array specified\n"), return 0 if !defined($$p{$_});
   }
   for ( qw( sizes sizeable position sizeMin)) {
      warn("'$_' is not an array\n"), return 0 if defined($$p{$_}) && ( ref($$p{$_}) ne 'ARRAY');
   }   
   my $i = 0;
   for ( @{$$p{sizes}}) {
      warn("Size #$i is not an valid array"), return 0 if (ref($_) ne 'ARRAY') || ( @$_ != 2);
   }
   $$p{sizeable} = [0,0] unless defined $$p{sizeable};
   warn("No 'sizes' given, and not sizeable"), return 0
      if (( 0 == @{$$p{sizes}}) && !$p->{sizeable}->[0] &&!$p->{sizeable}->[1]);
   $$p{sizeMin}  = [0,0] unless defined $$p{sizeMin};
   $$p{position} = [] unless defined $$p{position};
   $$p{CHECKED_OK} = 1;
   return 1;
}   

# this sub returns either rectangle, where widget can be set, or the another dock manager.
# @rect may be empty, then ::query finds first appropriate answer.
# Once the client finds the ::query' answer suitable, it changes owner to $self,
# changes it's origin and size correspondingly to the ::query response, and then
# calls ->dock.
sub query
{
   my ( $self, $session_id, @rect) = @_;
   return unless (ref($session_id) eq 'HASH') && 
        exists($session_id-> {SUBMGR}) && exists($session_id-> {SUBMGR_ID});
   $session_id->{SUBMGR_ID} = 0;
   return $session_id-> {SUBMGR}-> [0]; 
}

# once ::query returned the dock manager, it's possible to enumerate all of
# the dock managers that $self can offer.

sub next_docker
{
   my ( $self, $session_id, $posx, $posy) = @_;
   return unless (ref($session_id) eq 'HASH') && 
        exists($session_id-> {SUBMGR}) && exists($session_id-> {SUBMGR_ID});
   my ( $id, $array) =  ( $session_id->{SUBMGR_ID}, $session_id->{SUBMGR});
   while ( 1) {
      return if $id < -1 || $id >= scalar(@$array) - 1;
      $session_id->{SUBMGR_ID}++; $id++;
      return $$array[$id] if defined( $$array[$id]) && Prima::Object::alive($$array[$id]);
   }
   undef;
}   

# close_session must be called as the session is over.
sub close_session
{
#  my ( $self, $session_id) = @_;   
   undef $_[1];
}   


# these two subs must nto be called directly, but rather to client
# response on being docked or undocked.
# the $self usually reshapes itself or rearranges it's docklings within
# these subs.
sub undock
{
   my ( $self, $who) = @_;
#   print $self-> name . "($self): ". $who-> name . " is undocked\n";
   return unless $self-> {docklings}; 
   @{$self-> {docklings}} = grep { $who != $_ } @{$self-> {docklings}};
}   

sub dock
{
   my ( $self, $who) = @_;
#   print $self-> name . "($self): ". $who-> name . " is docked\n";
   $self-> {docklings} = [] unless $self-> {docklings};
   push ( @{$self-> {docklings}}, $who);
}   

sub dock_bunch
{
   my $self = shift;
   push ( @{$self-> {docklings}}, @_);
   $self-> rearrange;
}   

sub docklings
{
   return $_[0]->{docklings} ? @{$_[0]->{docklings}} : ();
}   

sub replace
{
   my ( $self, $wijFrom, $wijTo) = @_;
#   print $self-> name . "($self): ". $wijFrom-> name . " is replaced by ". $wijTo-> name ."\n";
   for (@{$self->{docklings}}) {
      next unless $_ == $wijFrom;
      $_ = $wijTo;
      $wijTo-> owner( $wijFrom-> owner) unless $wijTo-> owner == $wijFrom-> owner;
      $wijTo-> rect( $wijFrom-> rect);
      last;
   }   
}


sub redock_widget
{
   my ( $self, $wij) = @_;
   if ( $wij-> can('redock')) {
      $wij-> redock;
   } else {
      my @r = $wij-> owner-> client_to_screen( $wij-> rect);
      my %prf = (
         sizes     => [[ $r[2] - $r[0], $r[3] - $r[1]]],
         sizeable  => [0,0],
         self      => $wij,
      );
      my $sid = $self-> open_session( \%prf);
      return unless defined $sid;
      my @rc = $self-> query( $sid, @r);
      $self-> close_session( $sid);
      if ( 4 == scalar @rc) {
         if (( $rc[2] - $rc[0] == $r[2] - $r[0]) && ( $rc[3] - $rc[1] == $r[3] - $r[1])) {
            my @rx = $wij-> owner-> screen_to_client( @rc[0,1]);
            $wij-> origin( $wij-> owner-> screen_to_client( @rc[0,1])) if $rc[0] != $r[0] || $rc[1] != $r[1];
         } else {
            $wij-> rect( $wij-> owner-> screen_to_client( @rc));
         } 
         $self-> undock( $wij);
         $self-> dock( $wij);
      } 
   }   
}   

sub rearrange 
{
   my $self = $_[0];
   return unless $self->{docklings};
   my @r = @{$self->{docklings}};
   @{$self->{docklings}} = ();
   $self-> redock_widget($_) for @r;
}

# result of the ::fingerprint doesn't serve any particular reason. however,
# in the InternalDockerShuttle implementation it's value AND'ed with client's
# fingerprint value, thus in early stage rejecting non-suitable docks.

sub fingerprint {
   return exists($_[0]->{fingerprint})?$_[0]->{fingerprint}:0xFFFF unless $#_;
   $_[0]->{fingerprint} = $_[1];
}

sub add_subdocker
{
   my ( $self, $subdocker) = @_;
   push( @{$self-> {subdockers}}, $subdocker);
}   

sub remove_subdocker
{
   my ( $self, $subdocker) = @_; 
   return unless $self-> {subdockers};
   @{$self-> {subdockers}} = grep { $_ != $subdocker} @{$self-> {subdockers}};
}   

sub dockup
{
   return $_[0]->{dockup} unless $#_;
   $_[0]-> {dockup}-> remove_subdocker( $_[0]) if $_[0]->{dockup};
   $_[1]-> add_subdocker( $_[0]) if $_[1];
}   

# very simple dock, accepts everything that fits in; allows tiling
package Prima::SimpleWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::AbstractDocker::Interface);

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


sub open_session
{
   my ( $self, $profile) = @_;
   return unless $self-> enabled && $self-> showing;
   return unless $self-> check_session( $profile);

   my @sz = $self-> size;
   my @asz;
   my @able  = @{$profile->{sizeable}};
   my @minSz = @{$profile->{sizeMin}}; 
   for ( @{$profile-> {sizes}}) {
      my @xsz = @$_; 
      for ( 0, 1) {
         next if ( $xsz[$_] >= $sz[$_]) && !$able[$_];
         next if $sz[$_] < $minSz[$_];
         $asz[$_] = $xsz[$_];
      }
   }

   return if !defined($asz[0]) || !defined($asz[1]);

   my @offs = $self-> client_to_screen(0,0);
   return {
      minpos => \@offs,
      maxpos => [ $offs[0] + $sz[0] - $asz[0] - 0, $offs[1] + $sz[1] - $asz[1] - 0,],
      size   => \@asz,
   };
}   

sub query
{
   my ( $self, $p, @rect) = @_;
   my @npx;
   my @pos = @rect[0,1];
   if ( scalar @rect) {
      @npx = @pos;
      for ( 0, 1) {
         $npx[$_] = $$p{minpos}->[$_] if $npx[$_] <  $$p{minpos}->[$_];
         $npx[$_] = $$p{maxpos}->[$_] if $npx[$_] >= $$p{maxpos}->[$_];
      }
   } else {
      @npx = @{$$p{minpos}};
   }
   return @npx[0,1], $$p{size}->[0] + $npx[0], $$p{size}->[1] + $npx[1];
}   

package Prima::ClientWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::SimpleWidgetDocker);


sub open_session
{
   my ( $self, $profile) = @_;
   return unless $self-> enabled && $self-> showing;
   return unless $self-> check_session( $profile);

   my @sz = $self-> size;
   my @asz;
   my @able = @{$profile->{sizeable}};
   my @minSz = @{$profile->{sizeMin}}; 
   for ( @{$profile-> {sizes}}) {
      my @xsz = @$_; 
      for ( 0, 1) {
         next if ( $xsz[$_] != $sz[$_]) && !$able[$_];
         next if $sz[$_] < $minSz[$_];
         $asz[$_] = $sz[$_];
      }
   }

   return if !defined($asz[0]) || !defined($asz[1]);

   my @offs = $self-> client_to_screen(0,0);
   return {
      retval => [@offs, $offs[0] + $sz[0], $offs[1] + $sz[1]],
   };
}   

sub query { return @{$_[1]->{retval}}}   

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $self-> size;
   $canvas-> clear( 1, 1, $sz[0]-2, $sz[1]-2);
   $canvas-> rect3d( 0, 0, $sz[0]-1, $sz[1]-1, 1, $self-> dark3DColor, $self-> light3DColor);
}   

package grow;
# direct, ::vertical-independent
use constant ForwardLeft   => 0x01;
use constant ForwardDown   => 0x02;
use constant ForwardRight  => 0x04;
use constant ForwardUp     => 0x08;
use constant BackLeft      => 0x10;
use constant BackDown      => 0x20;
use constant BackRight     => 0x40;
use constant BackUp        => 0x80;
use constant Left          => ForwardLeft | BackLeft;
use constant Down          => ForwardDown | BackDown;
use constant Right         => ForwardRight| BackRight;
use constant Up            => ForwardUp   | BackUp;

# indirect, ::vertical-dependent
use constant ForwardMajorLess => 0x0100; 
use constant ForwardMajorMore => 0x0200;
use constant ForwardMinorLess => 0x0400; 
use constant ForwardMinorMore => 0x0800;
use constant BackMajorLess    => 0x1000; 
use constant BackMajorMore    => 0x2000;
use constant BackMinorLess    => 0x4000; 
use constant BackMinorMore    => 0x8000;
use constant MajorLess        => ForwardMajorLess | BackMajorLess;
use constant MajorMore        => ForwardMajorMore | BackMajorMore;
use constant MinorLess        => ForwardMinorLess | BackMinorLess;
use constant MinorMore        => ForwardMinorMore | BackMinorMore;

# masks
use constant Forward          => 0x0F0F;
use constant Back             => 0xF0F0;

# Implementation of the dock, that is useful for toolbars etc.
# It doesn't allow tiling, and it's smart enough to reshape itself and
# to rearrange it's docklings. 

package Prima::LinearWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::AbstractDocker::Interface);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = (
       dockup      => undef,
       vertical    => 0,
       growable    => 0, # grow::XXXX
       hasPocket   => 1,
       fingerprint => 0x0000FFFF
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   

{
my %RNT = (
   %{Prima::Widget->notification_types()},
   Dock      => nt::Notification,
   Undock    => nt::Notification,
   DockError => nt::Action,
);

sub notification_types { return \%RNT; }
}

sub init
{
   my $self = shift;
   $self-> {$_} = 0 for qw(growable vertical hasPocket fingerprint dockup);
   my %profile = $self-> SUPER::init( @_);
   $self-> $_( $profile{$_}) for ( qw( fingerprint growable hasPocket vertical dockup));
   return %profile;
}   

sub vertical
{
   return $_[0]->{vertical} unless $#_;
   my ( $self, $v) = @_;
   $self-> {vertical} = $v;
}   

sub hasPocket
{
   return $_[0]->{hasPocket} unless $#_;
   my ( $self, $v) = @_;
   $self-> {hasPocket} = $v;
}   


sub growable
{
   return $_[0]->{growable} unless $#_;
   my ( $self, $g) = @_;
   $self-> {growable} = $g;
}   

sub __docklings
{
   my ( $self, $exclude) = @_;
   my %hmap;
   my $xid = $self-> {vertical} ? 0 : 1; # minor axis, further 'vertical'
   my $yid = $self-> {vertical} ? 1 : 0; # major axis, further 'horizontal   
   my $min;
   for ( @{$self-> {docklings}}) {    
      next if $_ == $exclude;  # if redocking to another position, for example      
      my @rt = $_-> rect;
      $hmap{$rt[$xid]} = [0,0,0,[],[]] unless $hmap{$rt[$xid]};
      my $sz = $rt[$xid+2] - $rt[$xid];
      my $xm = $hmap{$rt[$xid]};
      $min = $rt[$xid] if !defined($min) || $min > $rt[$xid];
      $$xm[0] = $sz if $$xm[0] < $sz; # max vert extension
      $$xm[1] += $rt[$yid + 2] - $rt[$yid]; # total length occupied
      $$xm[2] = $rt[$yid+2] if $rt[$yid+2] > $$xm[2]; # farthest border
      push( @{$$xm[4]}, $_); # widget
   }  

   # checking widgets 
   my @ske = sort { $a <=> $b }keys %hmap;
   my @sz  = $self-> size;
   my $i;
   for ( $i = 0; $i < @ske - 1; $i++) {
      my $ext = $hmap{$ske[$i]}-> [0];
      if ( $ext + $ske[$i] < $ske[$i+1]) { # some gap here
         $hmap{$ext + $ske[$i]} = [$ske[$i+1] - $ske[$i] - $ext, 0, 0, [], []];
         @ske = sort { $a <=> $b }keys %hmap;
      }   
   }   
   if ( @ske) {
      my $ext = $hmap{$ske[-1]}-> [0]; # last row
      $hmap{$ext + $ske[-1]} = [$sz[$xid] - $ske[-1] - $ext, 0, 0, [], []]; 
   } else {
      $hmap{0} = [ $sz[$xid], 0, 0, [], []];
   }
   $hmap{0} = [ $min, 0, 0, [], []] unless $hmap{0};
# hmap structure:
# 0 - max vert extension   
# 1 - total length occupied
# 2 - farther border by major axis
# 3 - array of accepted sizes 
# 4 - widget list
   return \%hmap;
}

sub read_growable
{
   my ($self,$directionMask) = @_;
   my $g   = $self-> {growable} & $directionMask;
   my $xid = $self-> {vertical} ? 0 : 1;
   my $gMaxG = ( $g & grow::MajorMore) || ($g & ($xid ? grow::Right : grow::Up));
   my $gMaxL = ( $g & grow::MajorLess) || ($g & ($xid ? grow::Left  : grow::Down));
   my $gMinG = ( $g & grow::MinorMore) || ($g & ($xid ? grow::Up    : grow::Right));
   my $gMinL = ( $g & grow::MinorLess) || ($g & ($xid ? grow::Down  : grow::Left));
   return ( $gMaxG, $gMaxL, $gMinG, $gMinL);
}

sub open_session
{
   my ( $self, $profile) = @_;
   return unless $self-> enabled && $self-> visible;
   return unless $self-> check_session( $profile);

   my @sz  = $self-> size;
   my @msz = $self-> sizeMax;
   my $xid = $self-> {vertical} ? 0 : 1; # minor axis, further 'vertical'
   my $yid = $self-> {vertical} ? 1 : 0; # major axis, further 'horizontal'
   my ( $gMaxG, $gMaxL, $gMinG, $gMinL) = $self-> read_growable( grow::Forward);
   my %hmap = %{$self-> __docklings( $$profile{self})};

   # calculating row extension
   my $rows   = scalar keys %hmap;
   my $majExt = ( $gMaxG || $gMaxL) ? $msz[ $yid] : $sz[ $yid];
   my $minExt = ( $gMinG || $gMinL) ? $msz[ $xid] : $sz[ $xid];

   push( @{$$profile{sizes}}, [ @sz]) unless @{$$profile{sizes}};
   
   # total vertical occupied size
   my ( $gap, $vo) = (0, 0);
   for ( keys %hmap) {
      $hmap{$_}->[1] ?
         ( $vo  += $hmap{$_}->[0]) : 
         ( $gap += $hmap{$_}->[0]) ;
   }   

   # put acceptable set of sizes for every line
   my @minSz = @{$$profile{sizeMin}};
   for ( keys %hmap) {
      my ( $y, $ext, $total, $border, $array) = ( $_, @{$hmap{$_}});
      for ( @{$$profile{sizes}}) {
         my @asz = @$_;
         #print "@asz:ext:$ext, minext:$minExt, vo:$vo\n";
         #print "row $y:($total $majExt)";
         #if ( $asz[$xid] > $minExt - $vo) {
         if (( $asz[$xid] > $ext) && ($asz[$xid] > $minExt - $vo)) {
            next unless $profile->{sizeable}->[$xid]; 
            my $n_ext = $minExt - $vo;
            next if $n_ext < $minSz[$xid] && $n_ext < $asz[$xid];
            $asz[$xid] = $n_ext;
         }   
         #print "step1 $y :@asz|$ext $total $border = $majExt\n";
         if ($total + $asz[$yid] > $majExt) { 
            if ( !$self-> {hasPocket} || ( $border >= $majExt)) {
               next unless $profile->{sizeable}->[$yid]; 
               my $nb = ( $self-> {hasPocket} ? $border : $majExt) - $total;
               #print "3: $nb $yid\n";
               next if $nb < $minSz[$yid] && $nb < $asz[$yid]; 
               $asz[$yid] = $nb;
            }   
         }  
         # print "@$_:@asz\n";
         push ( @$array, \@asz);
      }   
      # print "$_(" . scalar(@{$hmap{$_}->[4]}). ')';
   }   

   # add decrement line
   if ( $vo) {
      # print " and - ";
      for (@{$$profile{sizes}}) { 
         my @asz = @$_; 
         next if $hmap{- $asz[$xid]};
         next if $asz[$xid] > $minExt - $vo;
         $hmap{ - $asz[$xid]} = [ $asz[$xid], 0, 0, [\@asz], []];
         # print "|$asz[$xid] ";
      }   
   }
   # print "\n";


   # sort out accepted sizes by 'verticalness'
   for ( keys %hmap) {
      my $s = $hmap{$_}->[3];
      @$s = map {
         [$$_[0], $$_[1]]             # remove ratio field
      } sort {
         $$a[2] <=> $$b[2]          # sort by xid/yid ratio
      } map {
         [@$_, $$_[$xid] / ($$_[$yid]||1)] # calculate xid/yid ratio
      } @$s;
   }   
   return {
      offs     => [ $self-> client_to_screen(0,0)],
      size     => \@sz,
      sizeMax  => \@msz,
      hmap     => \%hmap,
      rows     => scalar keys %hmap,
      vmap     => [ sort { $a <=> $b } keys %hmap],
      sizes    => [ sort { $$a[2] <=> $$b[2]} map { [ @$_, $$_[$yid] / ($$_[$xid]||1)]} @{$$profile{sizes}}],
      sizeable => $$profile{sizeable},
      sizeMin  => $$profile{sizeMin}, 
      grow     => [ $gMinG, $gMinL, $gMaxG, $gMaxL],
   };
}

sub query
{
   my ( $self, $p, @rect) = @_;
   my $xid = $self-> {vertical} ? 0 : 1; 
   my $yid = $self-> {vertical} ? 1 : 0; 
   my @asz;
   my @offs = @{$p-> {offs}};
   my $hmap = $$p{hmap};
   my $vmap = $$p{vmap};
   my ( $i, $closest, $idx, $side);
   my $rows = $$p{rows};
   my $useSZ = 1;

   $useSZ = 0, @rect = ( 0, 0, 1, 1) unless scalar @rect;
   my %skip = ();
AGAIN:      
   $i = 0; $idx = undef;
   for ( $i = 0; $i < $rows; $i++) {
      next if $skip{$$vmap[$i]};
      my $dist = ( $rect[ $xid] - ( $offs[$xid] + $$vmap[$i]));
      $dist *= $dist;
      $side = 0, $idx = $$vmap[$i], $closest = $dist if !defined($idx) || ( $closest > $dist);
      if ( $$vmap[$i] == 0 && !$$p{noDownSide}) {
         $dist = ( $rect[ $xid + 2] - ( $offs[$xid] + $$vmap[$i]));
         $dist *= $dist;
         $side = 1, $idx = $$vmap[$i], $closest = $dist if $closest > $dist;
      }
   } 
   return unless defined $idx;
   if ( @{$hmap->{$idx}->[3]}) {
      @asz = @{$hmap->{$idx}->[3]->[0]};
   } else {
      # print "$idx rejected\n";
      $skip{$idx} = 1;
      goto AGAIN;   
   }

   @rect = ( 0, 0, @asz) unless $useSZ;
   $idx -= $rect[$xid+2] - $rect[$xid] if $side;
   if ( $rect[$yid] < $offs[$yid]) {
#     $asz[$yid] -= $offs[$yid] - $rect[$yid] if $$p{sizeable}->[$yid];
      $rect[$yid] = $offs[$yid];
   }
   my $sk = ( $p->{sizeMin}->[$yid] > $asz[$yid]) ? $asz[$yid] : $p->{sizeMin}->[$yid];
   $rect[ $yid] = $offs[$yid] + $p->{size}->[$yid] - $sk if 
      $rect[$yid] > $offs[$yid] + $p->{size}->[$yid] - $sk;
#   unless ( $self-> {vertical}) {
#my @r = ( $rect[0], $idx + $offs[1], $rect[0] + $asz[0], $idx + $offs[1] + $asz[1]);
#print "q :@r\n";
#   } 
   return $self-> {vertical} ? 
      ( $idx + $offs[0], $rect[1], $idx + $offs[0] + $asz[0], $rect[1] + $asz[1]) :
      ( $rect[0], $idx + $offs[1], $rect[0] + $asz[0], $idx + $offs[1] + $asz[1]);
}  

sub dock
{
   my ( $self, $who) = @_;
   $self-> SUPER::dock( $who);
   my $xid = $self-> {vertical} ? 0 : 1; 
   my $yid = $self-> {vertical} ? 1 : 0; 
   my @rt = $who-> rect;
   my @sz = $self-> size;
   my $hmap = $self-> __docklings( $who); 
   my ( $gMaxG, $gMaxL, $gMinG, $gMinL) = $self-> read_growable( grow::Forward);

   # for ( keys %$hmap) { print "hmap:$_\n"; }

   if ( !exists $hmap->{$rt[$xid]}) {
       if ( $rt[$xid] >= 0 || $rt[$xid+2] != 0) {
          $self-> notify(q(DockError), $who);
          return;
       }
       $hmap->{$rt[$xid]} = [$rt[$xid+2]-$rt[$xid], 0, 0, [], [], 0];
   }   

   # minor axis
   my $doMajor = $hmap->{$rt[$xid]}-> [1];

   my $gap = 0;
   for ( keys %$hmap) {
      next if $_ < 0 || $hmap->{$_}->[1];
      $gap += $hmap->{$_}->[0];
   }   
   
#   print "key : $rt[$xid] $rt[$xid+2]\n";
   my $maxY = $hmap->{$rt[$xid]}->[1] ? $hmap->{$rt[$xid]}->[0] : 0;
   #my $tail = $rt[$xid+2] - $rt[$xid] - $hmap->{$rt[$xid]}->[0];
   my $tail = $rt[$xid+2] - $rt[$xid] - $maxY;
   #print "$self:tail:$tail $maxY @rt\n"; 
   if ( $tail > 0 || $rt[$xid] < 0) {
       my @fmp  = sort { $a <=> $b} keys %$hmap;
       my $prop = $self-> {vertical} ? 'left' : 'bottom';
       my $last = 0;
       for ( @fmp) { 
          my @rp = @{$hmap->{$_}->[4]};
          my $ht = $hmap->{$_}->[0]; 
          if ( $_ == $rt[$xid]) {
             push ( @rp, $who);
             $ht = $rt[$xid+2] - $rt[$xid] if $ht < $rt[$xid+2] - $rt[$xid];
          }   
          next unless scalar @rp;
          $_->$prop( $last) for @rp;
          $last += $ht;
       #   print "adde $hmap->{$_}->[0]\n";
       }
       $tail = ($last > $sz[$xid]) ? ( $last - $sz[$xid]) : 0;
       @rt = $who-> rect;
      # print "last:$last, tail:$tail\n";
   } else {
      $tail = 0;
   }   

   if ( $tail) {
      if ( $gMinG) {
         $sz[$xid] += $tail;
         $self-> size( @sz);
      } else {
         my @rect = $self-> rect;
         $rect[ $xid] -= $tail;
         $self-> rect( @rect);
      }  
      @sz = $self-> size;
   }  
   
   # major axis
   
   unless ( $self-> {hasPocket}) {
      my @o = @rt[0,1];
      $o[$yid] = $sz[$yid] - $rt[$yid+2] + $rt[$yid] if $rt[$yid+2] > $sz[$yid];
      $o[$yid] = 0 if $o[$yid] < 0;
   #  print "@o:@rt\n";
      $who-> origin( @o) if $o[$yid] != $rt[$yid];
      @rt[0,1] = @o;
   }  

   my @fmp;
   my $edge = 0;
   for ( $who, @{$hmap->{$rt[$xid]}->[4]}) {
      my @rxt = $_-> rect;
      push ( @fmp, [ $_, $rxt[ $yid], $rxt[ $yid + 2] - $rxt[ $yid]]);
      $edge = $rxt[$yid+2] if $edge < $rxt[$yid+2];
   }

   if ( $doMajor) {
      @fmp = sort { $$a[1] <=> $$b[1]} @fmp;
      my $prop = $self-> {vertical} ? 'bottom' : 'left';
      my $overlap;
      my $last = 0;
      for ( @fmp) {
         $overlap = 1, last if $$_[1] < $last;
         $last = $$_[1] + $$_[2];
      }  
      if ( $overlap) {
         $last = 0;
         my $i = 0;
         my @sizeMax = $self-> sizeMax;
         my @msz = ( $gMaxG || $gMaxL) ? @sizeMax : @sz;
         my $stage = 0;
         for ( @fmp) {
            $$_[1] = $last;
            $$_[0]-> $prop( $last);
            $last += $$_[2];
            $i++;
         } 
         $edge = $last;
      }
   }

   if ( $edge > $sz[$yid] && ($gMaxL || $gMaxG)) {
      if ( $gMaxG) {
         $sz[$yid] = $edge;
         $self-> size( @sz);
      } else {
         my @r = $self-> rect;
         $r[$yid] -= $edge - $sz[$yid];
         $self-> rect( @r);
      }   
      @sz = $self-> size;
   }   

   # redocking non-fit widgets
   my $stage = 0;
   my @repush;
   for ( @fmp) {
      if ( $self-> {hasPocket}) {
         next if $$_[1] <= $sz[$yid] - 5; 
         $stage = 1, next unless $stage;
      } else {
         next if $$_[1] + $$_[2] <= $sz[$yid];
      }   
      push( @repush, $$_[0]);
   }   
   $self-> redock_widget($_) for @repush; 

   $self-> notify(q(Dock), $who);
}

sub undock
{
   my ( $self, $who) = @_;
   $self-> SUPER::undock( $who);
   my $xid = $self-> {vertical} ? 0 : 1; 
   my $yid = $self-> {vertical} ? 1 : 0; 
   my @rt = $who-> rect;
   my @sz = $self-> size;
   my $hmap = $self-> __docklings( $who); 
   my ( $gMaxG, $gMaxL, $gMinG, $gMinL) = $self-> read_growable( grow::Back);   
   
# collapsing minor axis
   my $xd = $rt[$xid+2] - $rt[$xid];
   if (( !$hmap->{$rt[$xid]}->[1] || ($hmap->{$rt[$xid]}->[0] < $xd)) && ( $gMinG || $gMinL)) {
       my $d = $xd - ( $hmap->{$rt[$xid]}->[1] ? $hmap->{$rt[$xid]}->[0] : 0);
       my @asz = @sz;
       $asz[$xid] -= $d;
       $self-> size( @asz);
       @sz = $self-> size;
       my $prop = $self-> {vertical} ? 'left' : 'bottom';
       for ( keys %$hmap) {
          next if $_ <= $rt[$xid];
          $_->$prop( $_-> $prop() - $d) for @{$hmap->{$_}->[4]};
       }   
       if ( $gMinL) {
          my @o = $self-> origin;
          $o[$xid] += $d;
          $self-> origin( @o);
       }   
   }   
# collapsing major axis   
   my @fmp;
   my $adjacent;
   for ( @{$hmap->{$rt[$xid]}->[4]}) {
      my @rxt = $_-> rect;
      next if $rxt[$yid] < $rt[$yid];
      push( @fmp, $_);
      $adjacent = 1 if $rxt[$yid] == $rt[$yid + 2];
   }  
   if ( $adjacent) {
      my $d = $rt[$yid+2] - $rt[$yid];
      my $prop = $self-> {vertical} ? 'bottom' : 'left';
      $_-> $prop( $_-> $prop() - $d) for @fmp;
   }   
   
   if ( $gMaxG || $gMaxL) {
      my $edge = 0;
      for ( keys %$hmap) {
         for ( @{$hmap->{$_}->[4]}) {
            my @rxt = $_-> rect;
            $edge = $rxt[$yid+2] if $edge < $rxt[$yid+2];
         }   
      }   
      if ( $edge < $sz[$yid]) {
         if ( $gMaxG) {
            $sz[$yid] = $edge;
            $self-> size( @sz);
         } else {
            my @r = $self-> rect;
            $r[$yid] += $edge - $sz[$yid];
            $self-> rect( @r);
         }   
      }   
   }

   $self-> notify(q(Undock), $who);
}   

sub on_dockerror
{
   my ( $self, $urchin) = @_;
   my @rt = $urchin-> rect;
   my $xid = $self-> {vertical} ? 0 : 1;
   warn "The widget $urchin didn't follow docking conventions. Info: $rt[$xid] $rt[$xid+2]\n";
   $self-> redock_widget( $urchin);
}   

# the implementation of MFC-like docking window, where four sides
# of the window are 'pop-up' docking areas.

package Prima::FourPartDocker;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::AbstractDocker::Interface);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = (
      indents             => [ 0, 0, 0, 0],
      growMode            => gm::Client,
      dockup              => undef,
      fingerprint         => 0x0000FFFF,
      dockerClassLeft     => 'Prima::LinearWidgetDocker', 
      dockerClassRight    => 'Prima::LinearWidgetDocker', 
      dockerClassTop      => 'Prima::LinearWidgetDocker', 
      dockerClassBottom   => 'Prima::LinearWidgetDocker', 
      dockerClassClient   => 'Prima::ClientWidgetDocker', 
      dockerProfileLeft   => {},
      dockerProfileRight  => {},
      dockerProfileTop    => {},
      dockerProfileBottom => {},
      dockerProfileClient => {},
      dockerDelegationsLeft   => [qw(Size)],
      dockerDelegationsRight  => [qw(Size)],
      dockerDelegationsTop    => [qw(Size)],
      dockerDelegationsBottom => [qw(Size)],
      dockerDelegationsClient => [],
      dockerCommonProfile     => {},
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   for ( qw( Left Right Top Bottom)) {
      my $x = "dockerDelegations$_"; 
      # append user-specified delegations - it may not be known beforehead
      # which delegations we are using internally
      next unless exists $p->{$x};
      splice( @{$p->{$x}}, scalar(@{$p->{$x}}), 0, @{$default->{$x}});
   }    
}   

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);
   $self-> $_( $profile{$_}) for ( qw( dockup indents fingerprint));
   my @sz = $self-> size;
   my @i  = @{$self-> indents};
   $self-> insert([ $profile{dockerClassLeft} => 
       origin   => [ 0, $i[1]],    
       size     => [ $i[0], $sz[1] - $i[3] - $i[1]],
       vertical => 1,
       growable => grow::Right,
       growMode => gm::GrowHiY,
       name     => 'LeftDocker',
       delegations => $profile{dockerDelegationsLeft},
       %{$profile{dockerProfileLeft}},
       %{$profile{dockerCommonProfile}},
   ], [ $profile{dockerClassRight} => 
       origin   => [ $sz[0] - $i[2], $i[1]],    
       size     => [ $i[2], $sz[1] - $i[3] - $i[1]],
       vertical => 1,
       growable => grow::Left,
       growMode => gm::GrowHiY|gm::GrowLoX,
       name     => 'RightDocker',
       delegations => $profile{dockerDelegationsRight},
       %{$profile{dockerProfileRight}},
       %{$profile{dockerCommonProfile}},
   ], [ $profile{dockerClassTop} => 
       origin   => [ 0, $sz[1] - $i[3]],    
       size     => [ $sz[0], $i[3]],
       vertical => 0,
       growable => grow::Down,
       growMode => gm::GrowLoY|gm::GrowHiX,
       name     => 'TopDocker',
       delegations => $profile{dockerDelegationsTop},       
       %{$profile{dockerProfileTop}},
       %{$profile{dockerCommonProfile}},
   ], [ $profile{dockerClassBottom} => 
       origin   => [ 0, 0],    
       size     => [ $sz[0], $i[1]],
       vertical => 0,
       growable => grow::Up,
       growMode => gm::GrowHiX,
       name     => 'BottomDocker',
       delegations => $profile{dockerDelegationsBottom},       
       %{$profile{dockerProfileBottom}},
       %{$profile{dockerCommonProfile}},
   ], [ $profile{dockerClassClient} => 
       origin   => [ @i[0,1]],    
       size     => [ $sz[0]-$i[2], $sz[1]-$i[3]],
       growMode => gm::Client,
       name     => 'ClientDocker',
       delegations => $profile{dockerDelegationsClient},       
       %{$profile{dockerProfileClient}},
       %{$profile{dockerCommonProfile}},
   ]);
   return %profile;
}   

sub indents
{
   return $_[0]->{indents} unless $#_;
   my @i = @{$_[1]};
   for ( @i) {
      $_ = 0 if $_ < 0;
   }
   return unless 4 == @i;
   $_[0]-> {indents} = \@i;
}   

sub LeftDocker_Size
{
   my ( $self, $dock, $ox, $oy, $x, $y) = @_;
   return if $self-> {indents}-> [0] == $x;
   $self-> {indents}-> [0] = $x;
   $self-> ClientDocker-> set(
      left  => $x,
      right => $self-> ClientDocker-> right,
   );
   $self-> repaint;
}   

sub RightDocker_Size
{
   my ( $self, $dock, $ox, $oy, $x, $y) = @_;
   return if $self-> {indents}-> [2] == $x;
   $self-> {indents}-> [2] = $x;
   $self-> ClientDocker-> width( $self-> width - $x - $self-> {indents}->[0]);
   $self-> repaint;
}   

sub TopDocker_Size
{
   my ( $self, $dock, $ox, $oy, $x, $y) = @_;
   return if $self-> {indents}-> [3] == $y; 
   $self-> {indents}-> [3] = $y;
   my $h = $self-> height - $y - $self-> {indents}-> [1];
   
   $self-> LeftDocker-> height( $h);
   $self-> RightDocker-> height( $h);
   $self-> ClientDocker-> height( $h);
   $self-> repaint;
}   

sub BottomDocker_Size
{
   my ( $self, $dock, $ox, $oy, $x, $y) = @_;
   return if $self-> {indents}-> [1] == $y; 
   $self-> {indents}-> [1] = $y;
   my $h = $self-> height;
   $self-> LeftDocker-> height( $h - $y - $self-> {indents}-> [3]); 
   $self-> LeftDocker-> bottom( $self-> {indents}-> [1]); 
   $self-> RightDocker-> height( $h - $y - $self-> {indents}-> [3]); 
   $self-> RightDocker-> bottom( $self-> {indents}-> [1]); 
   $self-> ClientDocker-> set(
      bottom  => $y,
      top     => $self-> ClientDocker-> top,
   );
   $self-> repaint;
}   


# Shuttles ( the clients)
# This object servers as a 'shuttle' for the docking object while it's not docked. 
# It's very simple implementation, and more useful one could be found in MDI.pm,
# the Prima::MDIExternalDockerShuttle.
#
# The interface for substitution must contain properties ::shuttle and ::client,
# and subs client2frame and frame2client.

package Prima::ExternalDockerShuttle;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my $fh = int($def->{font}->{height} / 1.5);
   my %prf = (
      font           => { height => $fh, width => 0, },
      titleHeight    => $fh + 4,
      clipOwner      => 0,
      shuttle        => undef,
      widgetClass    => wc::Window,
   );
   @$def{keys %prf} = values %prf;
   return $def;   
}   


sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self->$_($profile{$_}) for qw(shuttle titleHeight);
   $self->{client} = $self-> insert( Widget => 
      rect => [ $self-> get_client_rect],
      growMode => gm::Client,
   );
   return %profile;
}   

sub shuttle
{
   return $_[0]-> {shuttle} unless $#_;
   $_[0]-> {shuttle} = $_[1];
}   

sub titleHeight
{
   return $_[0]-> {titleHeight} unless $#_;
   $_[0]-> {titleHeight} = $_[1];
   $_[0]-> repaint;
}   

sub client
{
   return $_[0]-> {client} unless $#_;
   my ( $self, $c) = @_;
   $c-> owner( $self);
   $c-> clipOwner(1);
   $c-> growMode( gm::Client);
   $c-> rect( $self-> get_client_rect);
   if ( $self-> {client}) {
      my $d = $self-> {client};
      $self-> {client} = $c;   
      $d-> destroy;
   } else {
      $self-> {client} = $c;
   }   
}   

sub get_client_rect
{
   my ( $self, $x, $y) = @_;
   ( $x, $y) = $self-> size unless defined $x;
   my $bw = 1;
   my @r = ( $bw, $bw, $x - $bw, $y - $bw);
   $r[3] -= $self->{titleHeight} + 1;
   $r[3] = $r[1] if $r[3] < $r[1];
   return @r;
}

sub client2frame
{
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   my $bw = 1;
   my @r = ( $x1 - $bw, $y1 - $bw, $x2 + $bw, $y2 + $bw);
   $r[3] += $self->{titleHeight} + 1;
   return @r;
}

sub frame2client
{
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   my $bw = 1;
   my @r = ( $x1 + $bw, $y1 + $bw, $x2 - $bw, $y2 - $bw);
   $r[3] -= $self->{titleHeight} + 1;
   $r[3] = $r[1] if $r[3] < $r[1];
   return @r;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $self-> size;
   my $th = $self-> {titleHeight};
   my @rc = ($self-> light3DColor, $self-> dark3DColor);
   $canvas-> rect3d( 0, 0, $sz[0]-1, $sz[1]-1, 1, @rc);
   $canvas-> clear( 1, 1, $sz[0] - 2, $sz[1] - $th - 3);
   my ( $x, $y) = ( $th / 2 - 1, );
   my $i = Prima::StdBitmap::image( $self-> {pressState} ? sbmp::ClosePressed : sbmp::Close);
   $canvas-> stretch_image( 1, $sz[1] - $th - 2, $th+1, $th+1, $i);
   $canvas-> backColor( $self-> hiliteBackColor);
   $canvas-> color( $self-> hiliteColor);
   $canvas-> clear( $th + 2, $sz[1] - $th - 2, $sz[0] - 2, $sz[1] - 2);
   $canvas-> clipRect( 1, 1, $sz[0] - 2, $sz[1] - 2);
   my $tx = $self-> text;
   my $w  = $sz[0] - $th - 8;
   if ( $canvas-> get_text_width( $tx) > $w) {
      $w -= $canvas-> get_text_width( '...');
      $tx = $canvas-> text_wrap( $tx, $w, 0)->[0].'...';
   }
   $canvas-> text_out( $tx, $th + 6, ( $th - $canvas-> font-> height) / 2 + $sz[1] - $th - 2);
}   

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return if $self-> {mouseTransaction};
   $self-> clear_event;
   my @sz = $self-> size;
   my $th = $self-> {titleHeight};
   return if $x < 2 || $x > $sz[0] - 2 || $y > $sz[1] - 2 || $y < $sz[1] - 2 - $th;
   if ( $x < $th + 2) {
      $self-> {pressState} = 1;
      $self-> {mouseTransaction} = 1;
      $self-> capture(1);
      $self-> invalidate_rect( 1, $sz[1] - 2 - $th, $th + 3, $sz[1] - 1);
   } else {
      my $s = $self-> shuttle;
      if ( $s-> client) {
         $s-> rect( $s-> client2frame( $s-> client-> rect));
      } else {
         $s-> rect( $self-> frame2client( $self-> rect));
      }
      $self-> bring_to_front;
      $self-> update_view;
      $s-> drag( 1, [ $self-> rect], $s-> screen_to_client( $self-> client_to_screen($x, $y)));
   }   
}   

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self-> {mouseTransaction};
   $self-> clear_event; 
   my @sz = $self-> size;
   my $th = $self-> {titleHeight};
   my $ps = ( $x < 2 || $x > $th + 2 || $y > $sz[1] - 2 || $y < $sz[1] - 2 - $th) ? 0 : 1;
   return if $ps == $self-> {pressState};
   $self-> {pressState} = $ps;
   $self-> invalidate_rect( 1, $sz[1] - 2 - $th, $th + 3, $sz[1] - 1);
}   

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   my @sz = $self-> size;
   my $th = $self-> {titleHeight};
   return if $x < 2 || $x > $sz[0] - 2 || $y > $sz[1] - 2 || $y < $sz[1] - 2 - $th; 
   return unless $dbl;
   $self-> clear_event;
   if ( $x < $th + 2) {
      $self-> notify(q(MouseDown), $btn, $mod, $x, $y);
   } else {
      $self-> shuttle-> dock_back; 
   }   
}   

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless $self-> {mouseTransaction};
   $self-> clear_event;
   $self-> {pressState} = 0;
   my @sz = $self-> size;
   my $th = $self-> {titleHeight};
   delete $self-> {mouseTransaction};
   $self-> invalidate_rect( 1, $sz[1] - 2 - $th, $th + 3, $sz[1] - 1);
   $self-> capture(0);
   return if $x < 2 || $x > $th + 2 || $y > $sz[1] - 2 || $y < $sz[1] - 2 - $th;
   $self-> close;
}   

# This shuttle hosts the widgets while it's docked to some AbstractDocker::Interface 
# descendant. It communicates with docks, and provides the UI for dragging itself
# with respect of the suggested positions. Although the code piece is large, it's rather
# an implementation of a 'widget that can dock' idea, because all the API is provided by
# AbstractDocker::Interface.

package Prima::InternalDockerShuttle;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

{
my %RNT = (
   %{Prima::Widget->notification_types()},
   GetCaps  => nt::Command,
   Landing  => nt::Request,
   Dock     => nt::Notification,
   Undock   => nt::Notification,
   FailDock => nt::Notification,
   EDSClose => nt::Command,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      externalDockerClass => 'Prima::ExternalDockerShuttle',
      dockingRoot         => undef,
      dock                => undef,
      snapDistance        => 10, # undef for none
      indents             => [ 5, 5, 5, 5],
      x_sizeable          => 0,
      y_sizeable          => 0,
      fingerprint         => 0x0000FFFF,
   );
   @$def{keys %prf} = values %prf;
   return $def;   
}   

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);
   $self-> $_( $profile{$_}) for ( qw( indents x_sizeable y_sizeable
      externalDockerClass fingerprint
      dockingRoot snapDistance));
   $self-> {__dock__} = $profile{dock};
   return %profile;
}   


sub setup
{
   $_[0]-> SUPER::setup;
   $_[0]-> dock( $_[0]-> {__dock__});
   delete $_[0]->{__dock__};
}   

sub cleanup
{
   my $self = $_[0];
   $self-> SUPER::cleanup;
   $self-> {dock}-> undock( $self) if $self-> {dock};
   my $d = $self-> {externalDocker};
   $self-> {externalDocker} = $self-> {dock} = undef;
   $d-> destroy if $d;
}   


sub snapDistance { 
   return $_[0]->{snapDistance} unless $#_;
   my $sd = $_[1];
   $sd = 0 if defined( $sd) && ($sd < 0);
   $_[0]->{snapDistance} = $sd;
}

sub externalDockerClass { 
   return $_[0]->{externalDockerClass} unless $#_;
   $_[0]->{externalDockerClass} = $_[1];
}

sub dockingRoot {
   return $_[0]->{dockingRoot} unless $#_;
   $_[0]->{dockingRoot} = $_[1] if !defined($_[1]) || $_[1]->isa('Prima::AbstractDocker::Interface');
}

sub x_sizeable {
   return $_[0]->{x_sizeable} unless $#_;
   $_[0]->{x_sizeable} = $_[1];
}

sub y_sizeable {
   return $_[0]->{y_sizeable} unless $#_;
   $_[0]->{y_sizeable} = $_[1];
}

sub fingerprint {
   return $_[0]->{fingerprint} unless $#_;
   $_[0]->{fingerprint} = $_[1];
}

sub client
{
   return $_[0]->{client} unless $#_;
   my ( $self, $c) = @_;
   if ( !defined($c)) {
      return if !$self-> {client};
   } else {
      return if defined( $self-> {client}) && ($c == $self-> {client});
   }   
   $self-> {client} = $c;
   return unless defined $c;
   return unless $self-> {externalDocker};
   my $ed = $self-> {externalDocker};
   my @cf = $ed-> client2frame( $c-> rect);
   $ed-> size( $cf[2] - $cf[0], $cf[3] - $cf[1]);
   $c-> owner( $ed-> client);
   $c-> clipOwner(1);
   $c-> origin( 0, 0);
}  

sub frame2client
{
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   my @i = @{$self->{indents}};
   return ( $x1 + $i[0], $y1 + $i[1], $x2 - $i[2], $y2 - $i[3]);
}   

sub client2frame
{
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   my @i = @{$self->{indents}};
   return ( $x1 - $i[0], $y1 - $i[1], $x2 + $i[2], $y2 + $i[3]);
}   

sub xorrect
{
   my ( $self, $x1, $y1, $x2, $y2, $width) = @_;
   $::application-> begin_paint;
   $x2--; $y2--;
   $width = $self-> {lastXorrectWidth} unless defined $width;
   $::application-> rect_focus( $x1, $y1, $x2, $y2, $width);
   $::application-> end_paint;
   $self-> {lastXorrectWidth} = $width;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   $canvas-> clear( 1, 1, $sz[0]-2, $sz[1]-2);
   $canvas-> rectangle( 0, 0, $sz[0]-1, $sz[1]-1);
}  

sub indents
{
   return $_[0]->{indents} unless $#_;
   my @i = @{$_[1]};
   for ( @i) {
      $_ = 0 if $_ < 0;
   }
   return unless 4 == @i;
   $_[0]-> {indents} = \@i;
}   

sub drag
{
   return $_[0]->{drag} unless $#_;
   my ( $self, $drag, $rect, $x, $y) = @_;
   my @rrc;
   if ( $drag) {
      return if $self-> {drag};
      $self-> {orgRect} = $rect;
      $self-> {anchor}  = [$x, $y];
      $self-> {drag}  = 1;
      $self-> {pointerSave} = $self-> pointer;
      $self-> {focSave}     = $::application-> get_focused_widget;
      $self-> capture(1);
      $self-> {oldRect} = [@{$self->{orgRect}}];
      $self-> {sessions} = {};
      tie %{$self-> {sessions}}, 'Tie::RefHash';
      @rrc = @{$self->{oldRect}};
      $self-> pointer( cr::Move);
      $self-> xorrect( @rrc, 3);
   } else {
      return unless $self-> {drag};
      $self-> capture(0);
      @rrc = @{$self-> {oldRect}};
      $self-> pointer( $self-> {pointerSave});
      $_-> close_session( $self->{sessions}->{$_}) for keys %{$self->{sessions}};
      delete $self-> {$_} for qw( anchor drag orgRect oldRect pointerSave sessions dockInfo);
      $self-> xorrect( @rrc);
   }
   
   unless ( $drag) {
      $self-> {focSave}-> focus if
         $self-> {focSave} && ref($self-> {focSave}) && $self-> {focSave}-> alive;
      delete $self-> {focSave};
   }
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return unless $btn == mb::Left;
   $self-> drag(1, [$self-> owner-> client_to_screen( $self-> rect)], $x, $y);
   $self-> clear_event;
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return unless ($btn == mb::Left) && $self-> {drag};
   my @rc;
   $rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [0] + $x for ( 0, 2);
   $rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [1] + $y for ( 1, 3);
   my ( $dm, $rect);
   if ( $self-> {dockingRoot}) { 
      ( $dm, $rect) = $self-> find_docking($self-> {dockingRoot}, \@rc);
   }
   $self-> drag(0);
   if ( $self-> {dockingRoot}) {
      if ( $dm) {
         $self-> dock( $dm, @$rect); # dock or redock
      } elsif ( $self-> {externalDocker}) { 
         $self-> {externalDocker}-> origin( @rc[0,1]); # just move external docker
         $self-> notify(q(FailDock), @rc[0,1]);
      } else {
         $self-> dock( undef, @rc); # convert to external state
      }   
   }
   $self-> clear_event;
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self-> {drag};
   my @rc;
   my $w = 3;
   $rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [0] + $x for ( 0, 2);
   $rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [1] + $y for ( 1, 3);
   goto LEAVE unless $self-> {dockingRoot};
   my ( $dm, $rect) = $self-> find_docking($self-> {dockingRoot}, \@rc);
   goto LEAVE unless $dm;
   @rc = @$rect;
   $w = 1;
LEAVE:   
   $self-> xorrect( @{$self-> {oldRect}});
   $self->{oldRect} = \@rc;
   $self-> xorrect( @{$self-> {oldRect}}, $w);
   $self-> clear_event;
}

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   if ( $self-> {drag} && $key == kb::Esc) {
      $self-> drag(0);
      $self-> clear_event;
   }
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   return unless $dbl;
   $self-> dock( undef);
}   

sub on_getcaps
{
   my ( $self, $docker, $prf) = @_;
   push( @{$prf->{sizes}}, [$self-> size]);
   $prf-> {sizeable} = [ $self-> {x_sizeable}, $self-> {y_sizeable}];
   $prf-> {sizeMin}  = [ $self-> {indents}->[2] + $self-> {indents}->[0], $self-> {indents}->[3] + $self-> {indents}-> [1]]; 
}   

# returns ( docker, rectangle)
sub find_docking
{
   my ( $self, $dm, $pos) = @_;
   my $sid;
   unless ( exists $self-> {sessions}-> {$dm}) {
      if ( $self-> fingerprint & $dm-> fingerprint) {
         my %caps;
         $self-> notify(q(GetCaps), $dm, \%caps);
         if ( keys %caps) { # $dm is user-approved
            $caps{position} = [ @$pos] if $pos;
            $caps{self}     = $self;
            $sid = $dm-> open_session( \%caps);
         }
      } 
      $self-> {sessions}-> {$dm} = $sid;
   } else {
      $sid = $self-> {sessions}-> {$dm};
   }
   return unless $sid;
   my $relocationCount;
AGAIN:
   #print "{$dm:@$pos:";
   my @retval;
   my @rc = $dm-> query( $sid, $pos ? @$pos : ());
   #print "(@rc)\n";
   goto EXIT unless scalar @rc;
   if ( 4 == scalar @rc) { # rect returned
      my $sd = $self->{snapDistance};
      if ( $pos && defined($sd)) {
         if ( $self-> {drag} &&
               ( # have to change the shape 
                 (( $$pos[2] - $$pos[0]) != ( $rc[2] - $rc[0])) ||
                 (( $$pos[3] - $$pos[1]) != ( $rc[3] - $rc[1])))) {
            my @pp = $::application-> pointerPos;
            my @newpos;
            #print '.';
            for ( 0, 1) {
                my ( $a, $b) = ( $_, $_ + 2);
                my $lb = (( $$pos[$a] + $$pos[$b]) / 2) > $pp[$a]; # 1 if pointer is closer to left/bottom
                my $pdist = $lb ? $pp[$a] - $$pos[$a] : $$pos[$b] - $pp[$a];
                my $sz1 = $rc[$b] - $rc[$a];     
                if ( $sz1 <= $pdist * 2) {
                   $newpos[$a] = $pp[$a] - int( $sz1/2);
                } else {
                   $newpos[$a] = $lb ? ( $pp[$a] - $pdist) : ( $pp[$a] + $pdist - $sz1);
                }   
                $newpos[$b] = $newpos[$a] + $sz1;
            }   
            # asking for the new position for the shape, if $dm can accept that...
            if ( 2 >= $relocationCount++) {
              #print "case1: @newpos\n";
              $pos = \@newpos;
              goto AGAIN; 
            }
         } elsif ( $self-> {drag} && ( # have to change the position
                 ( $$pos[0] != $rc[0]) || ( $$pos[1] != $rc[1]))) {
            my @pp = $::application-> pointerPos; 
            my @newpos = @pp; 
            #print ',';
            for ( 0, 1) {
               my ( $a, $b) = ( $_, $_ + 2);
               $newpos[$a] = $rc[$a] if $newpos[$a] < $rc[$a];
               $newpos[$a] = $rc[$b] if $newpos[$a] > $rc[$b];
            }  
            goto EXIT  if ( $sd < abs($pp[0] - $newpos[0])) || ( $sd < abs($pp[1] - $newpos[1]));
            # asking for the new position, and maybe new shape...
            if ( 2 >= $relocationCount++) {
             #print "case2: @rc\n";
              $pos = [@rc];
              goto AGAIN; 
            }
         }   
         goto EXIT if ($sd < abs($rc[0] - $$pos[0])) || ($sd < abs($rc[1] - $$pos[1]));
      }   
      goto EXIT unless $self-> notify(q(Landing), $dm, @rc);
      #print "@rc\n";
      @retval = ($dm, \@rc);
   } elsif ( 1 == scalar @rc) { # new docker returned
      my $next = $rc[0];
      while ( $next) {
         my ( $dm_found, $npos) = $self-> find_docking( $next, $pos);
         @retval = ($dm_found, $npos), goto EXIT if $npos;
         $next = $dm-> next_docker( $sid, $pos ? @$pos[0,1] : ());
      }   
   }   
EXIT:   
   unless ( $self-> {drag}) {
      $dm-> close_session( $sid);
      delete $self-> {sessions};
   }
   return @retval;
}   

sub dock
{
   return $_[0]-> {dock} unless $#_;
   my ( $self, $dm, @rect) = @_;
   if ( $dm) {
      my %caps;
      my $stage = 0;
      my ( $sid, @rc, @s1rc);
AGAIN:
      if ( $self-> fingerprint && $dm-> fingerprint) { 
         $self-> notify(q(GetCaps), $dm, \%caps);
         if ( keys %caps) { # $dm is user-approved
            unshift(@{$caps{sizes}}, [$rect[2] - $rect[0], $rect[3] - $rect[1]]) if scalar @rect;
            $caps{position} = [ @rect[0,1]] if scalar @rect;
            $caps{self}     = $self;
            $sid = $dm-> open_session( \%caps);
         }
      }
      return 0 unless $sid;
      @rc = $dm-> query( $sid, scalar(@rect) ? @rect : ());
      @s1rc = $dm-> rect;
      $dm-> close_session( $sid);
      if ( 1 == scalar @rc) { # readdress
         my ( $dm2, $rc) = $self-> find_docking( $dm, @rect ? [@rect] : ());
         $self-> dock( $dm2, $rc ? @$rc : ());
         return;
      }   
      return 0 if 4 != scalar @rc;
      return 0 unless $self-> notify(q(Landing), $dm, @rc);

      unless ( $stage) {
         $self-> {dock}-> undock( $self) if $self-> {dock};
   # during the undock $dm may change it's position ( and/or size), so retrying
         my @s2rc = $dm-> rect;
         if ( grep { $s1rc[$_] != $s2rc[$_] } (0..3)) {
            $stage = 1;
            goto AGAIN;
         }   
      }
      $self-> hide;
      $self-> owner( $dm);
      my @sz = $self-> size;
      $dm-> close_session( $sid);
      
      if ( $rc[2] - $rc[0] == $sz[0] && $rc[3] - $rc[1] == $sz[1]) {
         $self-> origin( $self-> owner-> screen_to_client( @rc[0,1])); 
      } else { 
         $self-> rect( $self-> owner-> screen_to_client( @rc));
      }
      unless ( $self-> {dock}) {
         my $c = $self-> client;
         if ( $c) {
            $c-> owner( $self);
            $c-> clipOwner(1);
            $c-> rect( $self-> frame2client( 0, 0, $self-> width, $self-> height));
         }   
         if ($self-> {externalDocker}) {
            my $d = $self-> {externalDocker};
            delete $self-> {externalDocker};
            $d-> destroy;   
         }
      }  
      $self-> {dock} = $dm; 
      $self-> show;
      $dm-> dock( $self);
      $self-> notify(q(Dock));
   } else {
      return if $self-> {externalDocker};  
      my $c = $self-> client;
      my $s = $c || $self;
      my $ed = $self->{externalDockerClass}-> create( 
         visible => 0,
         shuttle => $self,
         owner   => $::application,
         text    => $self-> text,
         onClose => sub { $_[0]-> clear_event unless $self-> notify(q(EDSClose))},   
      );
      my @r = $s-> owner-> client_to_screen( $s-> rect);
      $ed-> rect( $ed-> client2frame( @r));
      $ed-> origin( @rect[0,1]) if @rect;
      if ( $c) {
         $c-> owner( $ed-> client);
         $c-> clipOwner(1);
         $c-> origin( 0, 0);
      }
      if ( $self-> visible) {
         $ed-> show;
         $ed-> bring_to_front;
      }
      $self-> {externalDocker} = $ed;
      if ( $self->{dock}) {
         $self-> {lastUsedDock} = [ $self-> {dock}, [$self-> owner-> client_to_screen( $self-> rect)]];
         $self-> {dock}-> undock( $self) if $self->{dock};
         $self-> {dock} = undef;
      } 
      $self-> hide;
      $self-> owner( $::application);
      $self-> notify(q(Undock));
   }   
}   

sub externalDocker
{
   return $_[0]-> {externalDocker} unless $#_;
}   

sub dock_back
{
   my $self = $_[0];
   return if $self->{dock};
   my ( $dm, $rect);
   if ( $self-> {lastUsedDock}) {
      ( $dm, $rect) = @{$self-> {lastUsedDock}};
      delete $self-> {lastUsedDock}; 
   }   
   if ( !defined($dm) || !Prima::Object::alive( $dm)) {
      ( $dm, $rect) = $self-> find_docking( $self-> {dockingRoot});
   }
   return unless $dm;
   $self-> dock( $dm, $rect ? @$rect : ());
}   

sub redock
{
   my $self = $_[0];
   return unless $self-> {dock};
   $self-> dock( undef);
   $self-> dock_back;
}   

sub text
{
   return $_[0]-> SUPER::text unless $#_;
   $_[0]-> SUPER::text( $_[1]);
   $_[0]-> {externalDocker}-> text($_[1]) if $_[0]-> {externalDocker};
}   


# the client that is useful for the oriented toolbars. 
# draws a tiny header along the major axis

package Prima::LinearDockerShuttle;
use vars qw(@ISA);
@ISA = qw(Prima::InternalDockerShuttle);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      indent        => 2,
      headerBreadth => 8,
      vertical      => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;   
}   

sub init
{
   my $self = shift;
   $self-> {$_} = 0 for ( qw(indent headerBreadth vertical)); 
   my %profile = $self-> SUPER::init( @_);
   $self-> $_( $profile{$_}) for ( qw(indent headerBreadth vertical));
   return %profile;
}   

sub indent
{
   return $_[0]-> {indent} unless $#_;
   my ($self, $i) = @_;
   $i ||= 0;
   $i = 0 if $i < 0;
   return if $i == $self-> {indent};
   $self-> {indent} = $i;
   $self-> update_indents;
}   

sub headerBreadth
{
   return $_[0]-> {headerBreadth} unless $#_;
   my ($self, $i) = @_;
   $i ||= 0;
   $i = 0 if $i < 0;
   return if $i == $self-> {headerBreadth};
   $self-> {headerBreadth} = $i;
   $self-> update_indents;
}   


sub vertical
{
   return $_[0]-> {vertical} unless $#_;
   my ($self, $i) = @_;
   $i ||= 0;
   $i = 0 if $i < 0;
   return if $i == $self-> {vertical};
   $self-> {vertical} = $i;
   $self-> update_indents;
}   

sub update_indents
{
   my $self = $_[0];
   my $vs   = $self-> { vertical};
   my $i    = $self-> {indent};
   my $hb   = $self-> {headerBreadth};
   $self-> indents([ $vs ? $i : $i + $hb, $i, $i, $vs ? $i + $hb : $i]); 
}   

sub on_paint
{
   my ( $self, $canvas) = @_;
   my $vs = $self-> {vertical};
   my $i  = $self-> {indent};
   my $hb = $self-> {headerBreadth};
   my @sz = $self-> size;
   my @rc = ( $self-> light3DColor, $self-> dark3DColor);
   $canvas-> clear( 1, 1, $sz[0] - 2, $sz[1] - 2);
   $canvas-> color( $rc[1]);
   $canvas-> rectangle( 0, 0, $sz[0] - 1, $sz[1] - 1);
   my $j;
   for ( $j = $i; $j < $hb; $j += 4) {
      $vs ?
        $canvas-> rect3d( $i, $sz[1] - 3 - $j, $sz[0] - $i - 1, $sz[1] - 1 - $j, 1, @rc) :
        $canvas-> rect3d( $j, $i, $j+2, $sz[1] - $i - 1, 1, @rc); 
   }   
}  

# allows only one row of docklings; useful for button bars

package Prima::SingleLinearWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::LinearWidgetDocker);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = (
      growMode    => gm::Client,
      hasPocket   => 0,
      growable    => grow::ForwardMajorMore,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}   


sub open_session
{
   my ( $self, $profile) = @_; 
   my $res = $self-> SUPER::open_session( $profile);
   return unless $res;
# keep only one row of docklings
   my %hmap = %{$res->{hmap}};
   my @k    = keys %hmap;
   for ( @k) {
      delete $hmap{$_} if $_ != 0;
   }  
   $res->{noDownSide} = 1;
   return $res if scalar(keys %hmap) == scalar(@k);
   $res->{hmap} = \%hmap;
   $res->{rows} = scalar keys %hmap;
   $res->{vmap} = [sort { $a <=> $b } keys %hmap];
   return $res;    
} 

