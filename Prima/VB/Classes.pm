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
# $Id$
use strict;
package Prima::VB::Classes;

sub classes
{
   return (
      'Prima::Widget' => {
         RTModule => 'Prima::Classes',
         class  => 'Prima::VB::Widget',
         page   => 'Abstract',
         icon   => 'VB::VB.gif:0',
      },
   );
}

use Prima::Classes;


package Prima::VB::Object;


my %hooks = ();

sub init_profiler
{
   my ( $self, $prf) = @_;
   $self-> {class}      = $prf->{class};
   $self-> {realClass}  = $prf->{realClass} if defined $prf->{realClass};
   $self-> {module}  = $prf->{module};
   $self-> {creationOrder} = $prf->{creationOrder};
   $self-> {creationOrder} = 0 unless defined $self-> {creationOrder};
   $self-> {profile} = {};
   $self-> {extras}  = $prf->{extras} if $prf->{extras};
   my %events = $self-> prf_events;
   for ( keys %{$prf->{class}->notification_types}) {
      $events{"on$_"} = '' unless exists $events{"on$_"};
   }
   for ( keys %events) {
      $events{$_} = 'my $self = $_[0];' unless length $events{$_};
   }
   $self-> {events}  = \%events;
   $self-> {default} = {%{$prf->{class}-> profile_default}, %events};
   $self-> prf_adjust_default( $self-> {profile}, $self-> {default});
   $self-> prf_set( name  => $prf-> {profile}-> {name})  if exists $prf-> {profile}-> {name};
   $self-> prf_set( owner => $prf-> {profile}-> {owner}) if exists $prf-> {profile}-> {owner};
   delete $prf-> {profile}-> {name};
   delete $prf-> {profile}-> {owner};
   $self-> prf_set( %{$prf-> {profile}});
   my $types = $self-> prf_types;
   my %xt = ();
   for ( keys %{$types}) {
      my ( $props, $type) = ( $types->{$_}, $_);
      $xt{$_} = $type for @$props;
   }
   $xt{$_} = 'event' for keys %events;
   $self-> {types} = \%xt;
   $self-> {prf_types} = $types;
}

sub add_hooks
{
   my ( $object, @properties) = @_;
   for ( @properties) {
      $hooks{$_} = [] unless $hooks{$_};
      push( @{$hooks{$_}}, $object);
   }
}

sub remove_hooks
{
   my ( $object, @properties) = @_;
   @properties = keys %hooks unless scalar @properties;
   for ( keys %hooks) {
      my $i = scalar @{$hooks{$_}};
      while ( $i--) {
         last if $hooks{$_}->[$i - 1] == $object;
      }
      next if $i < 0;
      splice( @{$hooks{$_}}, $i, 1);
   }
}

sub prf_set
{
   my ( $self, %profile) = @_;
   my $name = $self-> prf('name');
   for ( keys %profile) {
      my $key = $_;
      next unless $hooks{$key};
      my $o = exists $self->{profile}->{$key} ? $self->{profile}->{$key} : $self->{default}->{$key};
      $_-> on_hook( $name, $key, $o, $profile{$key}) for @{$hooks{$key}};
   }
   $self->{profile} = {%{$self->{profile}}, %profile};
   my $check = $VB::inspector && ( $VB::inspector->{opened}) && ( $VB::inspector->{current} eq $self);
   for ( keys %profile) {
      my $cname = 'prf_'.$_;
      # ObjectInspector::widget_changed(0) if $check && ( $check eq $_);
      ObjectInspector::widget_changed(0, $_) if $check;
      $self-> $cname( $profile{$_}) if $self-> can( $cname);
   }
}

sub prf_delete
{
   my ( $self, @dellist) = @_;
   my $df = $self->{default};
   my $pr = $self->{profile};
   my $check = $VB::inspector && ( $VB::inspector->{opened}) && ( $VB::inspector->{current} eq $self);
   for ( @dellist) {
      delete $pr->{$_};
      my $cname = 'prf_'.$_;
      if ( $check) {
         ObjectInspector::widget_changed(1, $_); # if $check eq $_;
      }
      $self-> $cname( $df->{$_}) if $self-> can( $cname);
   }
}

sub prf
{
   my ( $self, @property) = @_;
   my @ret = ();
   for ( @property) {
      push ( @ret, exists $self->{profile}->{$_} ?
         $self->{profile}->{$_} :
         $self->{default}->{$_});
      warn( "cannot query $_") unless exists $self->{default}->{$_};
   }
   return wantarray ? @ret : $ret[0];
}

sub prf_adjust_default
{
}

sub prf_events
{
   return ();
}

sub ext_profile
{
   return;
}

sub act_profile
{
   return;
}

package Prima::VB::Component;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::VB::Object);

{
my %RNT = (
   %{Prima::Widget->notification_types()},
   Load => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      class      => 'Prima::Widget',
      module     => 'Prima::Classes',
      profile    => {},
      selectable => 1,
      sizeable   => 1,
      marked     => 0,
      mainEvent  => undef,
      sizeMin    => [11,11],
      selectingButtons => 0,
      accelItems => [
         ['altpopup',0,0, km::Shift|km::Ctrl|kb::F9 => sub{
             $_[0]-> altpopup;
             $_[0]-> clear_event;
         }],
      ],
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub prf_types
{
   return {
      name       => ['name'],
      Handle     => ['owner'],
      FMAction   => [qw( onBegin onFormCreate onCreate onChild onChildCreate onEnd )],
   };
}

sub prf_events
{
   return (
      onPostMessage  => 'my ( $self, $info1, $info2) = @_;',
   );
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   $pf-> {owner} = '';
}


sub prf_types_add
{
   my ( $self, $pt, $de) = @_;
   for ( keys %{$de}) {
      if ( exists $pt-> {$_}) {
         push( @{$pt->{$_}}, @{$de->{$_}});
      } else {
         $pt->{$_} = [@{$de->{$_}}];
      }
   }
}


sub init
{
   my $self = shift;
   for ( qw( marked sizeable)) {
      $self->{$_}=0;
   };
   my %profile = $self-> SUPER::init(@_);
   for ( qw( marked sizeable mainEvent)) {
      $self->$_( $profile{$_});
   }
   my %names = map { $_->name => 1} grep { $_ != $self } $VB::form-> widgets;
   $names{$VB::form-> name} = 1;
   my $xname = $self-> name;
   my $yname = $xname;
   my $cnt = 0;
   $yname = sprintf("%s.%d", $xname, $cnt++) while exists $names{$yname};
   $profile{profile}->{name} = $yname;
   $self-> init_profiler( \%profile);
   ObjectInspector::renew_widgets();
   return %profile;
}

sub get_profile_default
{
   my $self = $_[0];
}

sub common_paint
{
   my ( $self, $canvas) = @_;
   if ( $self-> {marked}) {
      my @sz = $canvas-> size;
      $canvas-> color( cl::Black);
      $canvas-> rectangle( 1, 1, $sz[0] - 2, $sz[1] - 2);
      $canvas-> rop( rop::XorPut);
      $canvas-> color( cl::White);
      my ( $hw, $hh) = ( int($sz[0]/2), int($sz[1]/2));
      $canvas-> bar( 0,0,4,4);
      $canvas-> bar( $hw-2,0,$hw+2,4);
      $canvas-> bar( $sz[0]-5,0,$sz[0]-1,4);
      $canvas-> bar( 0,$sz[1]-5,4,$sz[1]-1);
      $canvas-> bar( $hw-2,$sz[1]-5,$hw+2,$sz[1]-1);
      $canvas-> bar( $sz[0]-5,$sz[1]-5,$sz[0]-1,$sz[1]-1);
      $canvas-> bar( 0,$hh-2,2,$hh+2);
      $canvas-> bar( $sz[0]-5,$hh-2,$sz[0]-1,$hh+2);
      $canvas-> rop( rop::CopyPut);
   }
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   $canvas-> color( cl::LightGray);
   $canvas-> bar( 1,1,$sz[0]-2,$sz[1]-2);
   $canvas-> color( cl::Gray);
   $canvas-> rectangle( 0,0,$sz[0]-1,$sz[1]-1);
   $self-> common_paint( $canvas);
}

sub get_o_delta
{
   my $w = $_[0];
   return ( 0, 0) if $w == $VB::form;

   my $ownerName = $w-> prf( 'owner');
   return ( 0, 0) if ( $ownerName eq '') || ( $ownerName eq $VB::form-> name);

   my $owidget = $VB::form-> bring( $ownerName);

   return $owidget-> origin;
}


sub xy2part
{
   my ( $self, $x, $y) = @_;
   my @size = $self-> size;
   my $minDim = $size[0] > $size[1] ? $size[1] : $size[0];
   my $bw   = ($minDim < 12) ? (($minDim < 7)  ? 1 : 3) : 5;
   my $bwx  = ($minDim < 26) ? (($minDim < 14) ? 1 : 7) : $bw + 8;
   return q(client) unless $self->{sizeable};
   if ( $x < $bw) {
      return q(SizeSW) if $y < $bwx;
      return q(SizeNW) if $y >= $size[1] - $bwx;
      return q(SizeW);
   } elsif ( $x >= $size[0] - $bw) {
      return q(SizeSE) if $y < $bwx;
      return q(SizeNE) if $y >= $size[1] - $bwx;
      return q(SizeE);
   } elsif (( $y < $bw) or ( $y >= $size[1] - $bw)) {
      return ( $y < $bw) ? q(SizeSW) : q(SizeNW) if $x < $bwx;
      return ( $y < $bw) ? q(SizeSE) : q(SizeNE) if $x >= $size[0] - $bwx;
      return $y < $bw ? 'SizeS' : 'SizeN';
   }
   return q(client);
}

sub xorrect
{
   my ( $self, $r0, $r1, $r2, $r3, $adj) = @_;
   if ( $adj) {
      my @d = $self-> owner-> screen_to_client(0,0);
      my @o = $self-> get_o_delta();
      $d[$_] -= $o[$_] for 0..1;
      $VB::form-> text( '['.($r0+$d[0]).', '.($r1+$d[1]).'] - ['.($r2+$d[0]).', '.($r3+$d[1]).']');
   }

   my $o = $::application;
   $o-> begin_paint;
   my @cr = $self-> owner-> rect;
   $o-> clipRect( @cr);
   $cr[2]--;
   $cr[3]--;
   my $dsize = $self-> {sizeable} ? 5 : 1;

   $o-> rect_focus( $r0,$r1,$r2-1,$r3-1,$dsize);

   if ( defined $self-> {extraRects}) {
      my ( $ax, $ay) = @{$self-> {sav}};
      my @org = $self-> owner-> client_to_screen( $ax, $ay);
      $org[0] = $r0 - $org[0];
      $org[1] = $r1 - $org[1];
      $o-> rect_focus( $$_[0] + $org[0], $$_[1] + $org[1], $$_[2] + $org[0], $$_[3] + $org[1],
         $dsize) for @{$self-> {extraRects}};
   }
   $o-> end_paint;
}


sub maintain_children_origin
{
   my ( $self, $oldx, $oldy) = @_;
   my ( $x, $y) = $self-> origin;
   return if $x == $oldx && $y == $oldy;
   $x -= $oldx;
   $y -= $oldy;
   my $name = $self-> name;

   for ( $VB::form-> widgets) {
      next unless $_->prf('owner') eq $name;
      my @o = $_-> origin;
      $_-> origin( $o[0] + $x, $o[1] + $y) unless $_->marked;
      $_-> maintain_children_origin( @o);
   }
}

sub iterate_children
{
   my ( $self, $sub, @xargs) = @_;
   my $name = $self-> name;
   for ( $VB::form-> widgets) {
      next unless $_->prf('owner') eq $name;
      $sub-> ( $_, $self, @xargs);
      $_-> iterate_children( $sub, @xargs);
   }
}   

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   if ( $btn == mb::Left) {
      if ( defined $VB::main->{currentClass}) {
         $VB::form-> insert_new_control( $self-> left + $x, $self-> bottom + $y, $self);
         return;
      }

      if ( $mod & km::Shift) {
         $self-> marked( $self-> marked ? 0 : 1);
         $self-> focus;
         return;
      }
      
      $self-> bring_to_front;
      $self-> focus;
      $VB::inspector->{selectorChanging} = 1; # disallow auto single-select
      ObjectInspector::enter_widget( $self);
      $VB::inspector->{selectorChanging} = 0;

      $self-> iterate_children( sub { $_[0]-> bring_to_front; $_[0]-> update_view; }); 

      my $part = $self-> xy2part( $x, $y);
      my @mw;
      @mw = $VB::form-> marked_widgets if $part eq q(client) && $self-> marked;
      $self-> marked( 1, 1) unless @mw;    
      $self-> clear_event;
      $self-> capture(1, $self-> owner);
      $self-> {spotX} = $x;
      $self-> {spotY} = $y;
      $VB::form->{modified} = 1;
      if ( $part eq q(client)) {
         my @rects = ();
         for ( @mw) {
            next if $_ == $self;
            $_-> marked(1);
            push( @rects, [$_->client_to_screen(0,0,$_->size)]);
         }
         $self-> {sav} = [$self-> origin];
         $self-> {drag} = 1;
         $VB::form-> dm_init( $self);
         $self-> {extraWidgets} = \@mw;
         $self-> {extraRects}   = \@rects;
         $self-> {prevRect} = [$self-> client_to_screen(0,0,$self-> size)];
         $self-> update_view;
         $VB::form->{saveHdr} = $VB::form-> text;
         $self-> xorrect( @{$self-> {prevRect}}, 1);
         return;
      }

      if ( $part =~ /^Size/) {
         $self-> {sav} = [$self-> rect];
         $part =~ s/^Size//;
         $self-> {sizeAction} = $part;
         my ( $xa, $ya) = ( 0,0);
         if    ( $part eq q(S))   { ( $xa, $ya) = ( 0,-1); }
         elsif ( $part eq q(N))   { ( $xa, $ya) = ( 0, 1); }
         elsif ( $part eq q(W))   { ( $xa, $ya) = (-1, 0); }
         elsif ( $part eq q(E))   { ( $xa, $ya) = ( 1, 0); }
         elsif ( $part eq q(SW)) { ( $xa, $ya) = (-1,-1); }
         elsif ( $part eq q(NE)) { ( $xa, $ya) = ( 1, 1); }
         elsif ( $part eq q(NW)) { ( $xa, $ya) = (-1, 1); }
         elsif ( $part eq q(SE)) { ( $xa, $ya) = ( 1,-1); }
         $self-> {dirData} = [$xa, $ya];
         $self-> {prevRect} = [$self-> client_to_screen(0,0,$self-> size)];
         $self-> update_view;
         $VB::form->{saveHdr} = $VB::form-> text;
         $self-> xorrect( @{$self-> {prevRect}}, 1);
         return;
      }
   }

   if ( $btn == mb::Right && $mod & km::Ctrl) {
      $self-> altpopup;
      $self-> clear_event;
      return;
   }
}

sub altpopup
{
   my $self  = $_[0];
   while ( 1) {
      my $p = $self-> bring( 'AltPopup');
      if ( $p) {
         $p-> popup( $self-> pointerPos);
         last;
      }
      last if $self == $VB::form;
      my $o = $self-> prf('owner');
      $self = ( $o eq $VB::form-> name) ? $VB::form : $VB::form-> bring( $o);
      last unless $self;
   }
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   return unless $dbl;
   $mod &= km::Alt|km::Shift|km::Ctrl;
   if ( $mod == 0 && defined $self-> mainEvent) {
      my $a = $self-> mainEvent;
      $self-> marked(1,1);
      $VB::inspector-> set_monger_index( 1);
      my $list = $VB::inspector-> {currentList};
      my $ix = $list-> {index}-> {$a};
      if ( defined $ix) {
         $list-> focusedItem( $ix);
         $list-> notify(q(Click)) unless $list-> {check}->[$ix];
      }
      return;
   }
   $self-> notify( q(MouseDown), $btn, $mod, $x, $y);
}


sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   if ( $self->{drag}) {
      my $dm = $VB::form-> dragMode;
      if ( $dm != 3) {
         my @o = $self-> screen_to_client( @{$self-> {prevRect}}[0,1]);
         $y = $o[1] + $self-> {spotY} if $dm == 1;
         $x = $o[0] + $self-> {spotX} if $dm == 2;
      }
      $self-> xorrect( @{$self-> {prevRect}});
      my @sz = $self-> size;
      my @og = $self-> origin;
      if ( $VB::main-> {ini}-> {SnapToGrid}) {
         $x -= ( $x - $self->{spotX} + $og[0]) % 4;
         $y -= ( $y - $self->{spotY} + $og[1]) % 4;
      }

      if ( $VB::main-> {ini}-> {SnapToGuidelines}) {
         my $xline = $VB::form->{guidelineX} - $og[0];
         my $yline = $VB::form->{guidelineY} - $og[1];
         $x = $xline + $self->{spotX} if abs( $xline - $x + $self->{spotX}) < 8;
         $y = $yline + $self->{spotY} if abs( $yline - $y + $self->{spotY}) < 8;
         $x = $xline + $self->{spotX} - $sz[0] if abs( $xline - $x + $self->{spotX} - $sz[0]) < 8;
         $y = $yline + $self->{spotY} - $sz[1] if abs( $yline - $y + $self->{spotY} - $sz[1]) < 8;
      }
      my @xorg = $self-> client_to_screen( $x - $self->{spotX}, $y - $self->{spotY});
      $self-> {prevRect} = [ @xorg, $sz[0] + $xorg[0], $sz[1] + $xorg[1]];
      $self-> xorrect( @{$self-> {prevRect}}, 1);
   } else {
      if ( $self->{sizeable}) {
         if ( $self-> {sizeAction}) {
            my @org = $_[0]-> rect;
            my @new = @org;
            my @min = $self-> sizeMin;
            my @og = $self-> origin;
            my ( $xa, $ya) = @{$self->{dirData}};

            if ( $VB::main-> {ini}-> {SnapToGrid}) {
               $x -= ( $x - $self->{spotX} + $og[0]) % 4;
               $y -= ( $y - $self->{spotY} + $og[1]) % 4;
            }

            if ( $VB::main-> {ini}-> {SnapToGuidelines}) {
               my @sz = $self-> size;
               my $xline = $VB::form->{guidelineX} - $og[0];
               my $yline = $VB::form->{guidelineY} - $og[1];
               if ( $xa != 0) {
                  $x = $xline + $self->{spotX} if abs( $xline - $x + $self->{spotX}) < 8;
                  $x = $xline + $self->{spotX} - $sz[0] if abs( $xline - $x + $self->{spotX} - $sz[0]) < 8;
               }
               if ( $ya != 0) {
                  $y = $yline + $self->{spotY} if abs( $yline - $y + $self->{spotY}) < 8;
                  $y = $yline + $self->{spotY} - $sz[1] if abs( $yline - $y + $self->{spotY} - $sz[1]) < 8;
               }
            }

            if ( $xa < 0) {
               $new[0] = $org[0] + $x - $self->{spotX};
               $new[0] = $org[2] - $min[0] if $new[0] > $org[2] - $min[0];
            } elsif ( $xa > 0) {
               $new[2] = $org[2] + $x - $self->{spotX};
               if ( $new[2] < $org[0] + $min[0]) {
                  $new[2] = $org[0] + $min[0];
               }
            }

            if ( $ya < 0) {
               $new[1] = $org[1] + $y - $self->{spotY};
               $new[1] = $org[3] - $min[1] if $new[1] > $org[3] - $min[1];
            } elsif ( $ya > 0) {
               $new[3] = $org[3] + $y - $self->{spotY};
               if ( $new[3] < $org[1] + $min[1]) {
                  $new[3] = $org[1] + $min[1];
               }
            }

            if ( $org[1] != $new[1] || $org[0] != $new[0] || $org[2] != $new[2] || $org[3] != $new[3]) {
               $self-> xorrect( @{$self-> {prevRect}});
               $self-> {prevRect} = [$self-> owner-> client_to_screen( @new)];
               $self-> xorrect( @{$self-> {prevRect}}, 1);
            }
            return;
         } else {
            return if !$self-> enabled;
            my $part = $self-> xy2part( $x, $y);
            $self-> pointer( $part =~ /^Size/ ? &{$cr::{$part}} : cr::Arrow);
         }
      }
   }
}


sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   if ( $btn == mb::Left)
   {
      if ( $self->{drag}) {
         $self-> pointer( cr::Default);
         $self-> capture(0);
         $self-> {drag} = 0;
         $self-> xorrect( @{$self-> {prevRect}});
         my @o = $self-> origin;
         $self-> origin( $self-> owner-> screen_to_client(@{$self-> {prevRect}}[0,1]));
         $self-> maintain_children_origin( @o);
         if ( defined $self->{extraRects}) {
            my @org = $self-> owner-> client_to_screen( @{$self-> {sav}});
            $org[0] = $self->{prevRect}->[0] - $org[0];
            $org[1] = $self->{prevRect}->[1] - $org[1];
            for ( @{$self->{extraWidgets}}) {
               next if $_ == $self;
               $_-> origin( $_-> left + $org[0], $_-> bottom + $org[1]);
            }
         }
         $VB::form-> text( $VB::form->{saveHdr});
         $self-> {extraRects} = $self-> {extraWidgets} = undef;
      }
      if ( $self->{sizeAction}) {
         my @r = @{$self-> {prevRect}};
         $self-> xorrect( @r);
         @r = $self-> owner-> screen_to_client(@r);
         my @o = $self-> origin;
         $self-> rect( @r);
         $self-> maintain_children_origin( @o);
         $self-> pointer( cr::Default);
         $self-> capture(0);
         $self-> {sizeAction} = 0;
         $VB::form-> text( $VB::form->{saveHdr});
      }
   }
}

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   if ( $key == kb::Delete) {
      $self-> clear_event;
      $_-> destroy for $VB::form-> marked_widgets;
      ObjectInspector::renew_widgets();
      return;
   }
   if ( $key == kb::Esc) {
      if ( $self-> {drag} || $self->{sizeAction}) {
         $self-> xorrect( @{$self-> {prevRect}});
         $self-> {drag} = $self->{sizeAction} = 0;
         $self-> {dirData} = $self-> {spotX} = $self-> {spotY} = undef;
         $self-> pointer( cr::Default);
         $self-> capture(0);
         $VB::form-> text( $VB::form->{saveHdr});
         return;
      }
   }
   if ( $key == kb::Tab && $self->{drag}) {
      $VB::form-> dm_next( $self);
      my @pp = $::application-> pointerPos;
      $self-> {spotX} = $pp[0] - $self->{prevRect}->[0];
      $self-> {spotY} = $pp[1] - $self->{prevRect}->[1];
      $self-> clear_event;
      return;
   }
}

sub marked
{
   if ( $#_) {
       my ( $self, $mark, $exlusive) = @_;
       $mark = $mark ? 1 : 0;
       $mark = 0 if $self == $VB::form;
       return if ( $mark == $self->{marked}) && !$exlusive;
       if ( $exlusive) {
          $_-> marked(0) for $VB::form-> marked_widgets;
       }
       $self-> {marked} = $mark;
       $self-> repaint;
       $VB::main-> update_markings();
   } else {
      return 0 if $_[0] == $VB::form;
      return $_[0]->{marked};
   }
}

sub sizeable
{
   if ( $#_) {
     return if $_[1] == $_[0]->{sizeable};
     $_[0]->{sizeable} = $_[1];
     $_[0]-> pointer( cr::Default) unless $_[1];
   } else {
     return $_[0]->{sizeable};
   }
}

sub mainEvent
{
   return $_[0]-> {mainEvent} unless $#_;
   $_[0]-> {mainEvent} = $_[1];
}

sub prf_name
{
   my $old = $_[0]->name;
   $_[0]-> name($_[1]);
   $_[0]-> name_changed( $old, $_[1]);

   return unless $VB::inspector;
   my $s = $VB::inspector-> Selector;
   $VB::inspector->{selectorChanging}++;
   my @it = @{$s-> List-> items};
   my $sn = $s-> text;
   my $si = $s-> focusedItem;
   for ( @it) {
      $sn = $_ = $_[1] if $_ eq $old;
   }
   $s-> List-> items( \@it);
   $s-> text( $sn);
   $s-> focusedItem( $si);
   $VB::inspector->{selectorChanging}--;
}

sub name_changed
{
   return unless $VB::form;
   my ( $self, $oldName, $newName) = @_;

   for ( $VB::form, $VB::form-> widgets) {
      my $pf = $_-> {prf_types};
      next unless defined $pf->{Handle};
      my $widget = $_;
      for ( @{$pf->{Handle}}) {
         my $val = $widget->prf( $_);
         next unless defined $val;
         $widget-> prf_set( $_ => $newName) if $val eq $oldName;
      }
   }
}

sub owner_changed
{
   my ( $self, $from, $to) = @_;
   $self-> {lastOwner} = $to;
   return unless $VB::form;
   return if $VB::form == $self;
   if ( defined $to && defined $from) {
      return if $to eq $from;
   }
   return if !defined $to && !defined $from;
   if ( defined $from) {
      $from = $VB::form-> bring( $from);
      $from = $VB::form unless defined $from;
      $from-> prf_detach( $self);
   }
   if ( defined $to) {
      $to = $VB::form-> bring( $to);
      $to = $VB::form unless defined $to;
      $to-> prf_attach( $self);
   }
}

sub prf_attach {}
sub prf_detach {}

sub prf_owner
{
   my ( $self, $owner) = @_;
   $self-> owner_changed( $self-> {lastOwner}, $owner);
}


sub on_destroy
{
   my $self = $_[0];
   my $n = $self-> name;
   $self-> remove_hooks;
   $self-> name_changed( $n, $VB::form-> name) if $VB::form;
   $self-> owner_changed( $self-> prf('owner'), undef);
   if ( $hooks{DESTROY}) {
      $_-> on_hook( $self-> name, 'DESTROY') for @{$hooks{DESTROY}};
   }
   $VB::main->update_markings();
}

package Prima::VB::Drawable;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Component);


sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      color         => ['color','backColor'],
      fillPattern   => ['fillPattern'],
      font          => ['font'],
      lineEnd       => ['lineEnd'],
      linePattern   => ['linePattern'],
      lineWidth     => ['lineWidth'],
      rop           => ['rop', 'rop2'],
      bool          => ['textOutBaseline', 'textOpaque'],
      point         => ['transform'],
      palette       => ['palette'],
      image         => ['region'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

package Prima::VB::Widget;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Drawable);

sub prf_adjust_default
{
   my ( $self, $prf, $def) = @_;
   $self-> SUPER::prf_adjust_default( $prf, $def);
   $def->{size} = [$def->{width}, $def->{height}];
   $self-> size(@{$def->{size}});

   delete $def->{$_} for qw (
      accelTable
      clipOwner
      current
      currentWidget
      delegations
      focused
      popup
      selected
      selectedWidget
      capture
      hintVisible

      left
      right
      top
      bottom
      width
      height
      rect

      lineEnd
      linePattern
      lineWidth
      fillPattern
      region
      rop
      rop2
      textOpaque
      textOutBaseline
      transform
   );
   $def->{text} = '' unless defined $def->{text};
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      mainEvent => 'onMouseClick',
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      menu          => ['accelTable', 'popup',],
      menuItems     => ['accelItems', 'popupItems'],
      color         => ['dark3DColor', 'light3DColor', 'disabledBackColor',
         'disabledColor', 'hiliteBackColor', 'hiliteColor', 'popupColor',
         'popupBackColor','popupHiliteColor','popupHiliteBackColor',
         'popupDisabledColor','popupDisabledBackColor',
         'popupLight3DColor','popupDark3DColor',
      ],
      font          => ['popupFont'],
      bool          => ['briefKeys','buffered','capture','clipOwner',
         'centered','current','cursorVisible','enabled','firstClick','focused',
         'hintVisible','ownerColor','ownerBackColor','ownerFont','ownerHint',
         'ownerShowHint','ownerPalette','scaleChildren',
         'selectable','selected','showHint','syncPaint','tabStop','transparent',
         'visible','x_centered','y_centered','originDontCare','sizeDontCare',
      ],
      iv            => ['bottom','height','left','right','top','width'],
      tabOrder      => ['tabOrder'],
      rect          => ['rect'],
      point         => ['cursorPos'],
      origin        => ['origin'],
      upoint        => ['cursorSize', 'designScale', 'size', 'sizeMin', 'sizeMax', 'pointerHotSpot'],
      widget        => ['currentWidget', 'selectedWidget'],
      pointer       => ['pointer',],
      growMode      => ['growMode'],
      uiv           => ['helpContext'],
      string        => ['hint'],
      text          => ['text'],
      selectingButtons=> ['selectingButtons'],
      widgetClass   => ['widgetClass'],
      image         => ['shape'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub prf_events
{
   return (
      $_[0]-> SUPER::prf_events,
      onColorChanged   => 'my ($self, $colorIndex) = @_;',
      onDragDrop       => 'my ($self, $x, $y) = @_;',
      onDragOver       => 'my ($self, $x, $y, $state) = @_;',
      onEndDrag        => 'my ($self, $x, $y) = @_;',
      onHint           => 'my ($self, $show) = @_;',
      onKeyDown        => 'my ($self, $code, $key, $mod, $repeat) = @_;',
      onKeyUp          => 'my ($self, $code, $key, $mod) = @_;',
      onMenu           => 'my ($self, $menu, $variable) = @_;',
      onMouseDown      => 'my ($self, $btn, $mod, $x, $y) = @_;',
      onMouseUp        => 'my ($self, $btn, $mod, $x, $y) = @_;',
      onMouseClick     => 'my ($self, $btn, $mod, $x, $y, $dblclk) = @_;',
      onMouseMove      => 'my ($self, $mod, $x, $y) = @_;',
      onMouseWheel     => 'my ($self, $mod, $x, $y, $z) = @_;',
      onMouseEnter     => 'my ($self, $mod, $x, $y) = @_;',
      onMove           => 'my ($self, $oldx, $oldy, $x, $y) = @_;',
      onPaint          => 'my ($self, $canvas) = @_;',
      onPopup          => 'my ($self, $mouseDriven, $x, $y) = @_;',
      onSize           => 'my ($self, $oldx, $oldy, $x, $y) = @_;',
      onTranslateAccel => 'my ($self, $code, $key, $mod) = @_;',
   );
}

sub prf_color        { $_[0]-> color($_[1]); }
sub prf_backColor    { $_[0]-> backColor( $_[1]); }
sub prf_light3DColor { $_[0]-> light3DColor($_[1]); }
sub prf_dark3DColor  { $_[0]-> dark3DColor($_[1]); }
sub prf_hiliteColor       { $_[0]-> hiliteColor($_[1]); }
sub prf_disabledColor     { $_[0]-> disabledColor($_[1]); }
sub prf_hiliteBackColor   { $_[0]-> hiliteBackColor($_[1]); }
sub prf_disabledBackColor { $_[0]-> disabledBackColor($_[1]); }
sub prf_name         { $_[0]->SUPER::prf_name($_[1]); $_[0]-> repaint;      }
sub prf_text         { $_[0]-> text($_[1]); $_[0]-> repaint; }
sub prf_font         { $_[0]-> font( $_[1]); }
sub prf_left         { $_[0]-> rerect($_[1], 'left'); }
sub prf_right        { $_[0]-> rerect($_[1], 'right'); }
sub prf_top          { $_[0]-> rerect($_[1], 'top'); }
sub prf_bottom       { $_[0]-> rerect($_[1], 'bottom'); }
sub prf_width        { $_[0]-> rerect($_[1], 'width'); }
sub prf_height       { $_[0]-> rerect($_[1], 'height'); }
sub prf_origin       { $_[0]-> rerect($_[1], 'origin'); }
sub prf_size         { $_[0]-> rerect($_[1], 'size'); }
sub prf_rect         { $_[0]-> rerect($_[1], 'rect'); }
sub prf_centered     { $_[0]-> centered(1) if $_[1]; }
sub prf_x_centered   { $_[0]-> x_centered(1) if $_[1]; }
sub prf_y_centered   { $_[0]-> y_centered(1) if $_[1]; }

sub rerect
{
   my ( $self, $data, $who) = @_;
   return if $self-> {syncRecting};
   $self-> {syncRecting} = $who;
   $self-> set(
      $who => $data,
   );
   if (( $who eq 'left') || ( $who eq 'bottom')) {
      $self-> prf_origin( [$self-> origin]);
   }
   if (( $who eq 'width') || ( $who eq 'height') || ( $who eq 'right') || ( $who eq 'top')) {
      $self-> prf_size( [ $self-> size]);
   }
   $self-> {syncRecting} = undef;
}

sub on_move
{
   my ( $self, $ox, $oy, $x, $y) = @_;
   return if $self-> {syncRecting};
   $self-> {syncRecting} = $self;
   $self-> prf_set( origin => [$x, $y]);
   $self-> {syncRecting} = undef;
}

sub on_size
{
   my ( $self, $ox, $oy, $x, $y) = @_;
   return if $self-> {syncRecting};
   $self-> {syncRecting} = $self;
   $self-> prf_set( size => [$x, $y]);
   $self-> {syncRecting} = undef;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   my $cc = $self-> color;
   $canvas-> color( $self-> backColor);
   $canvas-> bar( 1,1,$sz[0]-2,$sz[1]-2);
   $canvas-> color( $cc);
   $canvas-> rectangle( 0,0,$sz[0]-1,$sz[1]-1);
   $self-> common_paint( $canvas);
}

package Prima::VB::Control;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Widget);


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      briefKeys
      cursorPos
      cursorSize
      cursorVisible
      designScale
      pointer
      pointerType
      pointerHotSpot
      pointerIcon
      scaleChildren
      selectable
      selectingButtons
      popupColor
      popupBackColor
      popupHiliteBackColor
      popupDisabledBackColor
      popupHiliteColor
      popupDisabledColor
      popupDark3DColor
      popupLight3DColor
      popupFont
      widgetClass
   );
}


package Prima::VB::Window;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Control);

sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
       menu
       modalResult
       menuColor
       menuBackColor
       menuHiliteBackColor
       menuDisabledBackColor
       menuHiliteColor
       menuDisabledColor
       menuDark3DColor
       menuLight3DColor
       menuFont
   );
}

sub prf_events
{
   return (
      $_[0]-> SUPER::prf_events,
      onWindowState  => 'my ( $self, $windowState) = @_;',
   );
}

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      borderIcons   => ['borderIcons'],
      borderStyle   => ['borderStyle'],
      windowState   => ['windowState'],
      icon          => ['icon'],
      menu          => ['menu'],
      menuItems     => ['menuItems'],
      color         => ['menuColor', 'menuHiliteColor','menuDisabledColor',
        'menuBackColor', 'menuHiliteBackColor','menuDisabledBackColor',
        'menuLight3DColor', 'menuDark3DColor'],
      font          => ['menuFont'],
      bool          => ['modalHorizon', 'taskListed', 'ownerIcon'],
      uiv           => ['modalResult'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


package Prima::VB::Types;

use Prima::Sliders;
use Prima::InputLine;
use Prima::Edit;
use Prima::Buttons;
use Prima::Label;
use Prima::Outlines;

package Prima::VB::Types::generic;

sub new
{
   my $class = shift;
   my $self = {};
   bless( $self, $class);
   ( $self-> {container}, $self->{id}, $self->{widget}) = @_;
   $self-> {changeProc} = \&ObjectInspector::item_changed;
   $self-> open();
   return $self;
}

sub renew
{
   my $self = shift;
   ( $self->{id}, $self->{widget}) = @_;
   $self-> change_id();
}

sub change_id
{
}

sub open
{
}

sub set
{
}

sub valid
{
   return 1;
}

sub change
{
   $_[0]-> {changeProc}->( $_[0]);
}


sub write
{
   my ( $class, $id, $data) = @_;
   return 'undef' unless defined $data;
   return "'". Prima::VB::Types::generic::quotable($data)."'" unless ref $data;
   if ( ref( $data) eq 'ARRAY') {
      my $c = '';
      for ( @$data) {
         $c .= $class-> write( $id, $_) . ', ';
      }
      return "[$c]";
   }
   if ( ref( $data) eq 'HASH') {
      my $c = '';
      for ( keys %{$data}) {
         $c .= "'". Prima::VB::Types::generic::quotable($_)."' => ".
               $class-> write(  $id, $data->{$_}) . ', ';
      }
      return "{$c}";
   }
   return '';
}

sub quotable
{
   my $a = $_[0];
   $a =~ s/\\/\\\\/g;
   $a =~ s/\'/\\\'/g;
   return $a;
}

sub printable
{
   my $a = $_[0];
   # $a =~ s/([\0-\7])/'\\'.ord($1)/eg;
   $a =~ s/([\!\@\#\$\%\^\&\*\(\)\'\"\?\<\>\\])/'\\'.$1/eg;
   $a =~ s/([\x00-\x1f\x7f-\xff])/'\\x'.sprintf("%02x",ord($1))/eg;
   return $a;
}

sub preload_modules
{
   return ();
}

package Prima::VB::Types::textee;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub set
{
   my ( $self, $data) = @_;
   $self-> {A}-> text( $data);
}

sub get
{
   return $_[0]-> {A}-> text;
}

package Prima::VB::Types::string;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::textee);

sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   $self-> {A} = $self-> {container}-> insert( InputLine =>
      origin => [ 5, $h - 36],
      width  => $self-> {container}-> width - 10,
      growMode => gm::Ceiling,
      onChange => sub {
         $self-> change;
      },
   );
}

package Prima::VB::Types::char;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::string);

sub open
{
   my $self = $_[0];
   $self-> SUPER::open( @_);
   $self-> {A}-> maxLen( 1);
}


package Prima::VB::Types::name;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::string);

sub wake
{
   if ( defined $_[0]->{A}->{monger}) {
      $_[0]->{A}->{monger}-> stop;
      $_[0]->{A}->{monger}-> start;
      return;
   }

   my $bc = $_[0]->{A}->backColor;
   my $c = $_[0]->{A}->color;
   $_[0]->{A}->backColor(cl::LightRed);
   $_[0]->{A}->color(cl::Yellow);
   $_[0]->{A}->{monger} = $_[0]->{A}->insert( Timer =>
      timeout => 300,
      onTick  => sub {
         $_[0]-> owner->{monger} = undef;
         $_[0]-> owner-> color($c);
         $_[0]-> owner-> backColor($bc);
         $_[0]-> destroy;
      },
   );
   $_[0]->{A}->{monger}-> start;
}

sub valid
{
   my $self = $_[0];
   my $tx = $self->{A}->text;
   $self->wake, return 0 unless length( $tx);
   $self->wake, return 0 if $tx =~ /[\s\\\~\!\@\#\$\%\^\&\*\(\)\-\+\=\[\]\{\}\.\,\?\;\|\`\'\"]/;
   my $l = $VB::inspector-> Selector-> List;
   my $s = $l-> items;
   my $fi= $l-> focusedItem;
   my $i = 0;
   for ( @$s) {
      next if $i++ == $fi;
      $self->wake, return 0 if $_ eq $tx;
   }
   return 1;
}


package Prima::VB::Types::text;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::textee);

sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   my $i = $self-> {container};
   my @sz = $i-> size;
   my $fh = $i-> font-> height;
   $self-> {A} = $i-> insert( Edit =>
      origin   => [ 5, 5],
      size     => [ $sz[0]-10, $sz[1] - 34],
      growMode => gm::Client,
      onChange => sub {
         $self-> change;
      },
   );
   $i-> insert( SpeedButton =>
      origin => [5, $sz[1] - 28],
      size   => [27, 27],
      hint   => 'Load',
      growMode => gm::GrowLoY,
      image  => $VB::main-> {openbutton}-> image,
      glyphs => $VB::main-> {openbutton}-> glyphs,
      onClick => sub {
         my $d = VB::open_dialog;
         if ( $d-> execute) {
            my $f = $d-> fileName;
            if ( open F, $f) {
               local $/;
               $f = <F>;
               $self->{A}->text( $f);
               close F;
               $self-> change;
            } else {
               Prima::MsgBox::message("Cannot load $f");
            }
         }
      },
   );
   $self->{B} = $i-> insert( SpeedButton =>
      origin => [ 33, $sz[1] - 28],
      size   => [27, 27],
      hint   => 'Save',
      growMode => gm::GrowLoY,
      image  => $VB::main-> {savebutton}-> image,
      glyphs => $VB::main-> {savebutton}-> glyphs,
      onClick => sub {
         my $dlg  = VB::save_dialog( filter => [
            [ 'Text files' => '*.txt'],
            [ 'All files' => '*'],
         ]);
         if ( $dlg-> execute) {
            my $f = $dlg-> fileName;
            if ( open F, ">$f") {
               local $/;
               $f = $self->{A}->text;
               print F $f;
               close F;
            } else {
               Prima::MsgBox::message("Cannot save $f");
            }
         }
      },
   );
   $i-> insert( SpeedButton =>
      origin => [ 62, $sz[1] - 28],
      size   => [27, 27],
      hint   => 'Clear',
      growMode => gm::GrowLoY,
      image  => $VB::main-> {newbutton}-> image,
      glyphs => $VB::main-> {newbutton}-> glyphs,
      onClick => sub {
         $self-> set( '');
         $self-> change;
      },
   );
}

package Prima::VB::Types::iv;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::string);

sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   $self-> {A} = $self-> {container}-> insert( SpinEdit =>
      origin => [ 5, $h - 36],
      width  => 120,
      min    => -16383,
      max    => 16383,
      onChange => sub {
         $self-> change;
      },
   );
}

sub write
{
   my ( $class, $id, $data) = @_;
   return $data;
}

package Prima::VB::Types::uiv;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::iv);

sub open
{
   my $self = shift;
   $self-> SUPER::open( @_);
   $self-> {A}-> min(0);
}

package Prima::VB::Types::bool;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   $self-> {A} = $self-> {container}-> insert( CheckBox =>
      origin => [ 5, $h - 36],
      width  => 120,
      text   => $self-> {id},
      onClick  => sub {
         $self-> change;
      },
   );
}

sub change_id { $_[0]-> {A}-> text( $_[0]->{id});}
sub get       { return $_[0]-> {A}-> checked;}

sub set
{
   my ( $self, $data) = @_;
   $self-> {A}-> checked( $data);
}

sub write
{
   my ( $class, $id, $data) = @_;
   return $data ? 1 : 0;
}

package Prima::VB::Types::tabOrder;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::iv);

sub change
{
   my $self = $_[0];
   $self-> SUPER::change;
   my $val = $self-> {A}-> value;
   return if $val < 0;
   my @mine = ();
   my $owner = $self-> {widget}-> prf('owner');
   my $match = 0;
   for ( $VB::form-> widgets) {
      next unless $_-> prf('owner') eq $owner;
      next if $_ == $self->{widget};
      push( @mine, $_);
      $match = 1 if $_->prf('tabOrder') == $val;
   }
   return unless $match;
   for ( @mine) {
      my $to = $_->prf('tabOrder');
      next if $to < $val;
      $_-> prf_set( tabOrder => $to + 1);
   }
}

package Prima::VB::Types::Handle;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   $self-> {A} = $self-> {container}-> insert( ComboBox =>
      origin => [ 5, $h - 36],
      width  => $self-> {container}-> width - 10,
      height => $self-> {container}-> font-> height + 4,
      style  => cs::DropDownList,
      items  => [''],
      onChange => sub {
         $self-> change;
      },
   );
}

sub set
{
   my ( $self, $data) = @_;
   if ( $self->{widget} == $VB::form) {
      $data = '';
      $self->{A}-> items( ['']);
   } else {
      my %items = map { $_ => 1} @{$VB::inspector-> Selector-> items};
      delete $items{ $self->{widget}->name};
      $self->{A}-> items( [ keys %items]);
      $data = $VB::form-> name unless length $data;
   }
   $self->{A}-> text( $data);
}

sub get
{
   my $self = $_[0];
   return $self->{A}-> text;
}

package Prima::VB::Types::sibling;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::Handle);

sub set
{
   my ( $self, $data) = @_;
   if ( $self->{widget} == $VB::form) {
      $data = '';
      $self->{A}-> items( ['']);
   } else {
      my %items = map { $_ => 1} @{$VB::inspector-> Selector-> items};
      $self->{A}-> items( [ '', keys %items]);
      $data = $self->{widget}-> name if !defined $data || ( length( $data) == 0);
   }
   $self->{A}-> text( $data);
}


package Prima::VB::Types::color;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

my @uColors  = qw(Fore Back HiliteText Hilite DisabledText Disabled Light3DColor Dark3DColor);
my @uClasses = qw(Button CheckBox Combo Dialog Edit InputLine Label ListBox Menu
     Popup Radio ScrollBar Slider Custom Window);

sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   $self-> {A} = $self-> {container}-> insert( ColorComboBox =>
      origin   => [ 85, $h - 36],
      onChange => sub {
         return if $self-> {sync};
         $self-> {sync} = 1;
         $self-> {B}-> text('undef');
         $self-> {C}-> text('undef');
         $self-> {sync} = undef;
         $self-> change;
      },
   );

   $self-> {container}-> insert( Label =>
      origin => [ 5, $h - 36],
      width  => 80,
      autoWidth => 0,
      text      => 'RGB:',
      focusLink => $self->{A},
   );


   $self-> {B} = $self-> {container}-> insert( ComboBox =>
      origin   => [ 5, $self-> {A}-> bottom - $self-> {container}-> font-> height - 39],
      size     => [ 136, $self-> {container}-> font-> height + 4],
      style    => cs::DropDownList,
      items    => ['undef', @uClasses],
      onChange => sub {
         return if $self-> {sync};
         $self-> change;
      },
   );

   $self-> {container}-> insert( Label =>
      origin => [ 5, $self->{B}-> top + 5],
      width  => 80,
      autoWidth => 0,
      text      => '~Widget class:',
      focusLink => $self->{B},
   );


   $self-> {C} = $self-> {container}-> insert( ComboBox =>
      origin   => [ 5, $self-> {B}-> bottom - $self-> {container}-> font-> height - 39],
      size     => [ 136, $self-> {container}-> font-> height + 4],
      style    => cs::DropDownList,
      items    => ['undef', @uColors],
      onChange => sub {
         return if $self-> {sync};
         $self-> change;
      },
   );

   $self-> {container}-> insert( Label =>
      origin => [ 5, $self->{C}-> top + 5],
      width  => 80,
      autoWidth => 0,
      text      => '~Color index:',
      focusLink => $self->{B},
   );

}

sub set
{
   my ( $self, $data) = @_;
   if ( $data & 0x80000000) {
      $self-> {A}-> value( cl::Gray);
      my ( $acl, $awc) = ( sprintf("%d",$data & 0x80000FFF), $data & 0x0FFF0000);
      my $tx = 'undef';
      for ( @uClasses) {
         $tx = $_, last if $awc == &{$wc::{$_}}();
      }
      $self-> {B}-> text( $tx);
      $tx = 'undef';
      for ( @uColors) {
         $tx = $_, last if $acl == &{$cl::{$_}}();
      }
      $self-> {C}-> text( $tx);
   } else {
      $self-> {A}-> value( $data);
      $self-> {B}-> text( 'undef');
      $self-> {C}-> text( 'undef');
   }
}

sub get
{
   my $self = $_[0];
   my ( $a, $b, $c) = ( $self->{A}-> value, $self->{B}->text, $self->{C}-> text);
   if ( $b eq 'undef' and $c eq 'undef') {
      return $a;
   } else {
      $b = ( $b eq 'undef') ? 0 : &{$wc::{$b}}();
      $c = ( $c eq 'undef') ? 0 : &{$cl::{$c}}();
      return $b | $c;
   }
}

sub write
{
   my ( $class, $id, $data) = @_;
   my $ret = 0;
   if ( $data & 0x80000000) {
      my ( $acl, $awc) = ( sprintf("%d",$data & 0x80000FFF), $data & 0x0FFF0000);
      my $tcl = '0';
      for ( @uClasses) {
         $tcl = "wc::$_", last if $awc == &{$wc::{$_}}();
      }
      my $twc = '0';
      for ( @uColors) {
         $twc = "cl::$_", last if $acl == &{$cl::{$_}}();
      }
      
      $ret = "$tcl | $twc";
   } else {
      $ret = '0x'.sprintf("%06x",$data);
   }
   return $ret;
}

package Prima::VB::Types::point;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   $self-> {A} = $self-> {container}-> insert( SpinEdit =>
      origin   => [ 5, $h - 72],
      width    => 120,
      min      => -16383,
      max      => 16383,
      onChange => sub {
         $self-> change;
      },
   );

   $self-> {L} = $self-> {container}-> insert( Label =>
      origin => [ 5, $self->{A}-> top + 5],
      width  => 80,
      autoWidth => 0,
      text      => $self->{id}.'.x:',
      focusLink => $self->{A},
   );


   $self-> {B} = $self-> {container}-> insert( SpinEdit =>
      origin   => [ 5, $self-> {A}-> bottom - $self-> {container}-> font-> height - 39],
      width    => 120,
      min      => -16383,
      max      => 16383,
      onChange => sub {
         $self-> change;
      },
   );

   $self-> {M} = $self-> {container}-> insert( Label =>
      origin => [ 5, $self->{B}-> top + 5],
      width  => 80,
      autoWidth => 0,
      text      => $self->{id}.'.y:',
      focusLink => $self->{B},
   );
}

sub change_id {
   $_[0]-> {L}-> text( $_[0]->{id}.'.x:');
   $_[0]-> {M}-> text( $_[0]->{id}.'.y:');
}

sub set
{
   my ( $self, $data) = @_;
   $data = [] unless defined $data;
   $self->{A}-> value( defined $data->[0] ? $data->[0] : 0);
   $self->{B}-> value( defined $data->[1] ? $data->[1] : 0);
}

sub get
{
   my $self = $_[0];
   return [$self->{A}-> value,$self->{B}-> value];
}

sub write
{
   my ( $class, $id, $data) = @_;
   return '[ '.$data->[0].', '.$data->[1].']';
}

package Prima::VB::Types::upoint;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::point);

sub open
{
   my $self = shift;
   $self-> SUPER::open( @_);
   $self-> {A}-> min(0);
   $self-> {B}-> min(0);
}

package Prima::VB::Types::origin;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::point);

sub set
{
   my ( $self, $data) = @_;
   $data = [] unless defined $data;
   my $x = defined $data->[0] ? $data->[0] : 0;
   my $y = defined $data->[1] ? $data->[1] : 0;
   my @delta = $self-> {widget}-> get_o_delta;
   $self->{A}-> value( $x - $delta[0]);
   $self->{B}-> value( $y - $delta[1]);
}

sub get
{
   my $self = $_[0];
   my ( $x, $y) = ($self->{A}-> value,$self->{B}-> value);
   my @delta = $self-> {widget}-> get_o_delta;
   return [$x + $delta[0], $y + $delta[1]];
}


package Prima::VB::Types::cluster;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open_indirect
{
   my ( $self, %extraProfile) = @_;
   my $h = $self-> {container}-> height;
   $self-> {A} = $self-> {container}-> insert( ListBox =>
      left   => 5,
      top    => $h - 5,
      width  => $self-> {container}-> width - 9,
      height => $h - 10,
      growMode => gm::Client,
      onSelectItem => sub {
         $self-> change;
         $self->on_change();
      },
      items => [$self->IDS],
      %extraProfile,
   );
}

sub IDS    {}
sub packID {}
sub on_change {}


package Prima::VB::Types::radio;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::cluster);

sub open
{
   my $self = $_[0];
   $self-> open_indirect();
}

sub IDS    {}
sub packID {}

sub set
{
   my ( $self, $data) = @_;
   my $i = 0;
   $self->{A}->focusedItem(-1), return unless defined $data;
   my $packID = $self->packID;
   for ( $self-> IDS) {
      if ( $packID->$_() == $data) {
         $self->{A}->focusedItem($i);
         last;
      }
      $i++;
   }
}

sub get
{
   my $self = $_[0];
   my @IDS = $self-> IDS;
   my $ix  = $self->{A}->focusedItem;
   return 0 if $ix < 0;
   $ix = $IDS[$ix];
   return $self->packID->$ix();
}

sub write
{
   my ( $class, $id, $data) = @_;
   my $packID = $class->packID;
   for ( $class->IDS) {
      if ( $packID->$_() == $data) {
         return $packID.'::'.$_;
      }
   }
   return $data;
}

package Prima::VB::Types::checkbox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::cluster);

sub open
{
   my $self = $_[0];
   $self-> open_indirect(
      multiSelect => 1,
      extendedSelect => 0,
   );
}

sub set
{
   my ( $self, $data) = @_;
   my $i = 0;
   my $packID = $self->packID;
   my @seli   = ();
   for ( $self-> IDS) {
      push ( @seli, $i) if $data & $packID->$_();
      $i++;
   }
   $self->{A}-> selectedItems(\@seli);
}

sub get
{
   my $self = $_[0];
   my @IDS  = $self-> IDS;
   my $res  = 0;
   my $packID = $self->packID;
   for ( @{$self->{A}-> selectedItems}) {
      my $ix = $IDS[ $_];
      $res |= $packID->$ix();
   }
   return $res;
}

sub write
{
   my ( $class, $id, $data) = @_;
   my $packID = $class->packID;
   my $i = 0;
   my $res;
   for ( $class-> IDS) {
      if ( $data & $packID->$_()) {
         $res = (( defined $res) ? "$res | " : '').$packID."::$_";
      }
      $i++;
   }
   return defined $res ? $res : 0;
}


package Prima::VB::Types::borderStyle;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(None Sizeable Single Dialog); }
sub packID { 'bs'; }

package Prima::VB::Types::align;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Left Center Right); }
sub packID { 'ta'; }

package Prima::VB::Types::valign;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Top Middle Bottom); }
sub packID { 'ta'; }

package Prima::VB::Types::windowState;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Normal Minimized Maximized); }
sub packID { 'ws'; }

package Prima::VB::Types::borderIcons;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::checkbox);
sub IDS    { qw(SystemMenu Minimize Maximize TitleBar); }
sub packID { 'bi'; }

package Prima::VB::Types::selectingButtons;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::checkbox);
sub IDS    { qw(Left Middle Right); }
sub packID { 'mb'; }

package Prima::VB::Types::widgetClass;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Button CheckBox Combo Dialog Edit InputLine Label ListBox Menu
     Popup Radio ScrollBar Slider Custom Window); }
sub packID { 'wc'; }

package Prima::VB::Types::rop;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(CopyPut XorPut AndPut OrPut NotPut NotBlack
  NotDestXor NotDestAnd NotDestOr NotSrcXor NotSrcAnd NotSrcOr
  NotXor NotAnd NotOr NotBlackXor NotBlackAnd NotBlackOr NoOper
  Blackness Whiteness Erase Invert Pattern XorPattern AndPattern
  OrPattern NotSrcOrPat SrcLeave DestLeave ); }
sub packID { 'rop'; }

package Prima::VB::Types::comboStyle;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Simple DropDown DropDownList); }
sub packID { 'cs'; }
sub preload_modules { return 'Prima::ComboBox' };

package Prima::VB::Types::gaugeRelief;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Sink Border Raise); }
sub packID { 'gr'; }
sub preload_modules { return 'Prima::Sliders' };

package Prima::VB::Types::sliderScheme;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Gauge Axis Thermometer StdMinMax); }
sub packID { 'ss'; }
sub preload_modules { return 'Prima::Sliders' };

package Prima::VB::Types::tickAlign;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Normal Alternative Dual); }
sub packID { 'tka'; }
sub preload_modules { return 'Prima::Sliders' };

package Prima::VB::Types::growMode;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::checkbox);
sub IDS    { qw( GrowLoX GrowLoY GrowHiX GrowHiY XCenter YCenter DontCare); }
sub packID { 'gm'; }

sub open
{
   my $self = shift;
   $self-> SUPER::open( @_);
   my $fh = $self->{A}->font->height;
   $self->{A}-> set(
      bottom => $self->{A}->bottom + $fh * 5 + 5,
      height => $self->{A}->height - $fh * 5 - 5,
   );
   my @ai = qw(Client Right Left Floor Ceiling);
   my $hx = $self->{A}->bottom;
   my $wx = $self->{A}->width;
   for ( @ai) {
      $hx -= $fh + 1;
      my $xgdata = $_;
      $self->{container}-> insert( Button =>
         origin => [ 5, $hx],
         size   => [ $wx, $fh],
         text   => $xgdata,
         growMode => gm::GrowHiX,
         onClick => sub {
            my $xg = $self->get & ~gm::GrowAll;
            $self->set( $xg | &{$gm::{$xgdata}}());
         },
      );
   }
}


package Prima::VB::Types::font;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

my @pitches = qw(Default Fixed Variable);
my @styles  = qw(Bold Italic Underlined StruckOut);

sub refresh_fontlist
{
   my $self = $_[0];
   my %fontList  = ();
   my @fontItems = qw();

   for ( sort { $a->{name} cmp $b->{name}} @{$::application-> fonts})
   {
      $fontList{$_->{name}} = $_;
      push ( @fontItems, $_->{name});
   }

   $self->{fontList}  = \%fontList;
   $self->{fontItems} = \@fontItems;

   $self-> {name}-> items( \@fontItems);
   #$self-> {name}-> text( $self->{logFont}->{name});
   #$self-> reset_sizelist;
}


sub open
{
   my $self = $_[0];
   my $i = $self->{container};
   my @sz = $i-> size;
   $sz[1] -= 60;
   $self->{marker} = $i-> insert( Button =>
      origin   => [ 5, $sz[1]],
      size     => [ $sz[0]-9, 56],
      growMode => gm::Ceiling,
      text     => 'AaBbCcZz...',
      onClick  => sub {
         my %f = %{$_[0]-> font};
         delete $f{height};
         my $f = $self->{fontDlg} ? $self->{fontDlg} : Prima::FontDialog-> create(
            logFont => \%f,
            name    => 'Choose font',
            icon    => $VB::ico,
         );
         $_[0]-> font( $f-> logFont) if $f-> execute == cm::OK;
      },
      onFontChanged => sub {
         return if $self->{sync};
         $self->{sync} = 1;
         $self-> set( $_[0]->font);
         $self->{sync} = 0;
      },
   );

   my $fh = $self-> {container}-> font-> height;
   $sz[1] -= $fh + 4;
   $self->{namex} = $i-> insert( CheckBox =>
      origin  => [ 5, $sz[1]],
      size    => [ $sz[0]-9, $fh + 2],
      growMode => gm::Ceiling,
      text    => '~Name',
      onClick => sub { $self-> on_change(); },
   );

   $sz[1] -= $fh + 4;
   $self->{name} = $i-> insert( ComboBox =>
      origin   => [ 5, $sz[1]],
      size     => [ $sz[0]-9, $fh + 2],
      growMode => gm::Ceiling,
      style    => cs::DropDown,
      text     => '',
      onChange => sub { $self->{namex}->check; $self-> on_change(); },
   );

   $sz[1] -= $fh + 4;
   $self->{sizex} = $i-> insert( CheckBox =>
      origin  => [ 5, $sz[1]],
      growMode => gm::Ceiling,
      size    => [ $sz[0]-9, $fh + 2],
      text    => '~Size',
      onClick => sub { $self-> on_change(); },
   );
   $sz[1] -= $fh + 6;
   $self->{size} = $i-> insert( SpinEdit =>
      origin   => [ 5, $sz[1]],
      size     => [ $sz[0]-9, $fh + 4],
      growMode => gm::Ceiling,
      min      => 1,
      max      => 256,
      onChange => sub { $self->{sizex}->check;$self-> on_change(); },
   );
   $sz[1] -= $fh + 4;
   $self->{stylex} = $i-> insert( CheckBox =>
      origin  => [ 5, $sz[1]],
      growMode => gm::Ceiling,
      size    => [ $sz[0]-9, $fh + 2],
      text    => 'St~yle',
      onClick => sub { $self-> on_change(); },
   );
   $sz[1] -= $fh * 5 + 28;
   $self->{style} = $i-> insert( GroupBox =>
      origin   => [ 5, $sz[1]],
      size     => [ $sz[0]-9, $fh * 5 + 28],
      growMode => gm::Ceiling,
      style    => cs::DropDown,
      text     => '',
      onChange => sub { $self-> on_change(); },
   );
   my @esz = $self->{style}->size;
   $esz[1] -= $fh * 2 + 4;
   $self->{styleBold} = $self->{style}->insert( CheckBox =>
      origin  => [ 8, $esz[1]],
      size    => [ $esz[0] - 16, $fh + 4],
      text    => '~Bold',
      growMode => gm::GrowHiX,
      onClick => sub { $self->{stylex}->check;$self-> on_change(); },
   );
   $esz[1] -= $fh + 6;
   $self->{styleItalic} = $self->{style}->insert( CheckBox =>
      origin  => [ 8, $esz[1]],
      size    => [ $esz[0] - 16, $fh + 4],
      text    => '~Italic',
      growMode => gm::GrowHiX,
      onClick => sub { $self->{stylex}->check;$self-> on_change(); },
   );
   $esz[1] -= $fh + 6;
   $self->{styleUnderlined} = $self->{style}->insert( CheckBox =>
      origin  => [ 8, $esz[1]],
      size    => [ $esz[0] - 16, $fh + 4],
      text    => '~Underlined',
      growMode => gm::GrowHiX,
      onClick => sub { $self->{stylex}->check;$self-> on_change(); },
   );
   $esz[1] -= $fh + 6;
   $self->{styleStruckOut} = $self->{style}->insert( CheckBox =>
      origin  => [ 8, $esz[1]],
      size    => [ $esz[0] - 16, $fh + 4],
      text    => '~Strikeout',
      growMode => gm::GrowHiX,
      onClick => sub { $self->{stylex}->check;$self-> on_change(); },
   );

   $sz[1] -= $fh + 4;
   $self->{pitchx} = $i-> insert( CheckBox =>
      origin  => [ 5, $sz[1]],
      size    => [ $sz[0]-9, $fh + 2],
      growMode => gm::Ceiling,
      text    => '~Pitch',
      onClick => sub { $self-> on_change(); },
   );

   $sz[1] -= $fh + 4;
   $self->{pitch} = $i-> insert( ComboBox =>
      origin   => [ 5, $sz[1]],
      size     => [ $sz[0]-9, $fh + 2],
      growMode => gm::Ceiling,
      style    => cs::DropDownList,
      items    => \@pitches,
      onChange => sub { $self->{pitchx}->check;$self-> on_change(); },
   );

   $self-> refresh_fontlist;
}

sub on_change
{
   my $self = $_[0];
   $self-> change;
   $self->{sync} = 1;
   $self->{marker}-> font( $self->{marker}-> font_match(
      $self-> get,
      $self->{widget}->{default}->{$self->{id}},
   ));
   $self->{sync} = undef;
}

sub set
{
   my ( $self, $data) = @_;
   $self-> {sync}=1;
   my %ndata = ();
   my $def = $self->{widget}->{default}->{$self->{id}};
   for ( qw( name size style pitch)) {
      $self-> {$_.'x'}-> checked( exists $data->{$_});
      $ndata{$_} = exists $data->{$_}  ? $data->{$_} : $def->{$_};
   }
   $self->{name}->text( $ndata{name});
   $self->{size}->value( $ndata{size});
   for ( @pitches) {
      if ( &{$fp::{$_}}() == $ndata{pitch}) {
         $self-> {pitch}-> text( $_);
         last;
      }
   }
   for ( @styles) {
      $self-> {"style$_"}-> checked( &{$fs::{$_}}() & $ndata{style});
   }
   $self-> {sync}=0;
}

sub get
{
   my $self = $_[0];
   my $ret  = {};
   $ret->{name}  = $self->{name}->text  if $self->{namex}->checked;
   $ret->{size}  = $self->{size}->value if $self->{sizex}->checked;
   if ( $self->{pitchx}->checked) {
      my $i = $self->{pitch}->text;
      $ret->{pitch} = &{$fp::{$i}}();
   }
   if ( $self->{stylex}->checked) {
      my $o = 0;
      for ( @styles) {
         $o |= &{$fs::{$_}}() if $self-> {"style$_"}-> checked;
      }
      $ret->{style} = $o;
   }
   return $ret;
}

sub write
{
   my ( $class, $id, $data) = @_;
   my $ret = '{';
   $ret .= "name => '".Prima::VB::Types::generic::quotable($data->{name})."', " if exists $data->{name};
   $ret .= 'size => '.$data->{size}.', ' if exists $data->{size};
   if ( exists $data->{style}) {
      my $s;
      for ( @styles) {
         if ( &{$fs::{$_}}() & $data->{style}) {
            $s = ( defined $s ? ($s.'|') : '').'fs::'.$_;
         }
      }
      $s = '0' unless defined $s;
      $ret .= 'style => '.$s.', ';
   }
   if ( exists $data->{pitch}) {
      for ( @pitches) {
         if ( &{$fp::{$_}}() == $data->{pitch}) {
            $ret .= 'pitch => fp::'.$_;
         }
      }
   }
   return $ret.'}';
}

package Prima::VB::Types::icon;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
   my $self = $_[0];
   my $i = $self->{container};
   my @sz = $i-> size;

   my $fh = $i-> font-> height;
   $self->{A} = $i-> insert( Widget =>
      origin   => [ 5, 5],
      size     => [ $sz[0]-10, $sz[1] - 34],
      growMode => gm::Client,
      onPaint  => sub {
         my ( $self, $canvas) = @_;
         my @s = $canvas-> size;
         $canvas-> color( $self-> backColor);
         $canvas-> bar( 0,0, @s);
         if ( $self->{icon}) {
            my @is = $self->{icon}->size;
            $self-> put_image(
               ($s[0] - $is[0])/2,
               ($s[1] - $is[1])/2,
               $self->{icon});
         } else {
            $canvas-> color( cl::Black);
            $canvas-> rectangle(0,0,$s[0]-1,$s[1]-1);
            $canvas-> line(0,0,$s[0]-1,$s[1]-1);
            $canvas-> line(0,$s[1]-1,$s[0]-1,0);
         }
      },
   );

   $i-> insert( SpeedButton =>
      origin => [ 5, $sz[1] - 28],
      size   => [ 27, 27],
      hint   => 'Load',
      image  => $VB::main-> {openbutton}-> image,
      glyphs => $VB::main-> {openbutton}-> glyphs,
      growMode => gm::GrowLoY,
      onClick => sub {
         my @r = VB::image_open_dialog-> load( 
            className => $self-> imgClass,
            loadAll   => 1
         );
         return unless $r[-1];
         my $i = $r[0];
         my ( $maxH, $maxW) = (0,0);
         if ( @r > 1) {
            for ( @r) {
               my @sz = $_-> size;
               $maxH = $sz[1] if $sz[1] > $maxH;
               $maxW = $sz[0] if $sz[0] > $maxW;
            }
            $maxW += 2;
            $maxH += 2;
            my $dd = Prima::Dialog-> create(
               centered => 1,
               visible  => 0,
               borderStyle => bs::Sizeable,
               size     => [ 300, 300],
               name     => 'Select image',
            );
            my $l = $dd-> insert( ListViewer =>
               size     => [ $dd-> size],
               origin   => [ 0,0],
               itemHeight => $maxH,
               itemWidth  => $maxW,
               multiColumn => 1,
               autoWidth   => 0,
               growMode    => gm::Client,
               onDrawItem => sub {
                  my ($self, $canvas, $index, $left, $bottom, $right, $top, $hilite, $focusedItem) = @_;
                  my $bc;
                  if ( $hilite) {
                     $bc = $self-> backColor;
                     $self-> backColor( $self-> hiliteBackColor);
                  }
                  $canvas-> clear( $left, $bottom, $right, $top);
                  $canvas-> put_image( $left, $bottom, $r[$index]);
                  $self-> backColor( $bc) if $hilite;
               },
               onClick => sub {
                   my $self = $_[0];
                   $i = $r[ $self-> focusedItem];
                   $dd-> ok;
               },
            );
            $l-> set_count( scalar @r);
            if ( $dd-> execute == cm::OK) {
               $self-> set( $i);
               $self-> change;
            }
            $dd-> destroy;
         } else {
            $self-> set( $i);
            $self-> change;
         }
      },
   );
   $self->{B} = $i-> insert( SpeedButton =>
      origin => [33, $sz[1]- 28],
      size   => [27, 27],
      hint   => 'Save',
      image  => $VB::main-> {savebutton}-> image,
      glyphs => $VB::main-> {savebutton}-> glyphs,
      growMode => gm::GrowLoY,
      onClick => sub {
         VB::image_save_dialog-> save( $self->{A}->{icon});
      },
   );
   $self->{C} = $i-> insert( SpeedButton =>
      origin => [62, $sz[1] - 28],
      size   => [27, 27],
      hint   => 'Clear',
      glyphs => $VB::main-> {newbutton}-> glyphs,
      image  => $VB::main-> {newbutton}-> image,
      growMode => gm::GrowLoY,
      onClick => sub {
         $self-> set( undef);
         $self-> change;
      },
   );
}

sub imgClass
{
   return 'Prima::Icon';
}

sub set
{
   my ( $self, $data) = @_;
   $self->{A}->{icon} = $data;
   $self->{A}-> repaint;
   $self->{B}-> enabled( $data);
   $self->{C}-> enabled( $data);
}

sub get
{
   my ( $self) = @_;
   return $self->{A}->{icon};
}


sub write
{
   my ( $class, $id, $data) = @_;
   my $c = $class->imgClass.'->create( '.
        'width=>'.$data->width.
      ', height=>'.$data->height;
   my $type = $data->type;
   my $xc = '';
   for ( qw(GrayScale RealNumber ComplexNumber TrigComplexNumber)) {
      $xc = "im::$_ | " if &{$im::{$_}}() & $type;
   }
   $xc .= 'im::bpp'.( $type & im::BPP);
   $c .= ", type => $xc";
   my $p = $data->palette;
   $c .= ", \npalette => [ ".join(',', @$p).']' if scalar @$p;
   $p = $data->data;
   my $i = 0;
   $xc = length( $p);
   $c .= ",\n data => \n";
   for ( $i = 0; $i < $xc; $i += 20) {
      my $a = Prima::VB::Types::generic::printable( substr( $p, $i, 20));
      $c .= "\"$a\".\n";
   }
   $c .= "'')";
   return $c;
}


package Prima::VB::Types::image;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::icon);

sub imgClass
{
   return 'Prima::Image';
}


package Prima::VB::Types::items;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::text);

sub set
{
   my ( $self, $data) = @_;
   $self-> {A}-> text( join( "\n", @$data));
}

sub get
{
   return [ split( "\n", $_[0]-> {A}-> text)];
}

sub write
{
   my ( $class, $id, $data) = @_;
   my $r = '[';
   for ( @$data) {
      $r .= "'".Prima::VB::Types::generic::quotable($_)."', ";
   }
   $r .= ']';
   return $r;
}

package Prima::VB::Types::multiItems;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::items);

sub set
{
   my ( $self, $data) = @_;
   $self-> {A}-> text( join( "\n", map { join( ' ', map { 
      my $x = $_; $x =~ s/(^|[^\\])(\\|\s)/$1\\$2/g; $x; } @$_)} @$data));
}

sub get
{
   my @ret;
   for ( split( "\n", $_[0]-> {A}-> text)) {
      my @x;
      while (m<((?:[^\s\\]|(?:\\\s))+)\s*|(\S+)\s*|\s+>gx) {
         push @x, $+ if defined $+;
      }
      @x = map { s/\\(\\|\s)/$1/g; $_ } @x;
      push( @ret, \@x);
   }
   return \@ret;
}

sub write
{
   my ( $class, $id, $data) = @_;
   my $r = '[';
   for ( @$data) {
      $r .= '[';
      $r .= "'".Prima::VB::Types::generic::quotable($_)."', " for @$_;
      $r .= '],';
   }
   $r .= ']';
   return $r;
}


package Prima::VB::Types::event;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::text);

sub open
{
   my $self = shift;
   $self-> SUPER::open( @_);
   $self-> {A}-> syntaxHilite(1);
}


sub write
{
   my ( $class, $id, $data, $asPL) = @_;
   return $asPL ? "sub { $data}" :
      'Prima::VB::VBLoader::GO_SUB(\''.Prima::VB::Types::generic::quotable($data).'\')';
}

package Prima::VB::Types::FMAction;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::event);


package MyOutline;

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   $self-> SUPER::on_keydown( $code, $key, $mod);
   $mod &= (km::Ctrl|km::Shift|km::Alt);
   if ( $key == kb::Delete && $mod == 0) {
      $self-> del;
      $self-> clear_event;
      return;
   }
   if ( $key == kb::Insert && $mod == 0) {
      $self-> new;
      $self-> clear_event;
      return;
   }
}

sub new_item
{
}

sub new
{
   my $f = $_[0]-> focusedItem;
   my ( $x, $l) = $_[0]-> get_item( $f);
   my ( $p, $o) = $_[0]-> get_item_parent( $x);
   $o = -1 unless defined $o;
   $_[0]-> insert_items( $p, $o + 1, $_[0]-> new_item);
   $_[0]-> {master}-> change;
   ( $x, $l) = $_[0]-> get_item( $f + 1);
   $_[0]-> focusedItem( $f + 1);
   $_[0]-> {master}-> enter_menuitem( $x);
}

sub makenode
{
   my $f = $_[0]-> focusedItem;
   my ( $x, $l) = $_[0]-> get_item( $f);
   return if !$x;
   if ( $x->[1]) {
      splice( @{$x->[1]}, 0, 0, $_[0]-> new_item);
      $_[0]-> reset_tree;
      $_[0]-> update_tree;
      $_[0]-> repaint;
      $_[0]-> reset_scrolls;
   } else {
      $x-> [1] = [$_[0]-> new_item];
      $x-> [2] = 0;
   }
   $_[0]-> adjust( $f, 1);
   $_[0]-> {master}-> change;
   ( $x, $l) = $_[0]-> get_item( $f + 1);
   $_[0]-> focusedItem( $f + 1);
   $_[0]-> {master}-> enter_menuitem( $x);
}

sub del
{
   my $f = $_[0]-> focusedItem;
   my ( $x, $l) = $_[0]-> get_item( $f);
   $_[0]-> delete_item( $x);
   $_[0]-> {master}-> change;
   $f-- if $f;
   ( $x, $l) = $_[0]-> get_item( $f);
   $_[0]-> focusedItem( $f);
   $_[0]-> {master}-> enter_menuitem( $x);
}


sub on_dragitem
{
   my $self = shift;
   $self-> SUPER::on_dragitem( @_);
   $self-> {master}-> change;
}

package MenuOutline;
use vars qw(@ISA);
@ISA = qw(Prima::Outline MyOutline);

sub new_item
{
   return  [['New Item', { text => 'New Item',
      action => $Prima::VB::Types::menuItems::menuDefaults{action}}], undef, 0];
}


package MPropListViewer;
use vars qw(@ISA);
@ISA = qw(PropListViewer);

sub on_click
{
   my $self = $_[0];
   my $index = $self-> focusedItem;
   my $current = $self-> {master}-> {current};
   return if $index < 0 or !$current;
   my $id = $self-> {'id'}->[$index];
   $self-> SUPER::on_click;
   if ( $self->{check}->[$index]) {
      $current-> [0]->[1]-> {$id} = $Prima::VB::Types::menuItems::menuDefaults{$id};
      $self->{master}-> item_changed;
   } else {
      delete $current-> [0]->[1]-> {$id};
      $self->{master}-> item_deleted;
   }
}


package Prima::VB::Types::menuItems;
use vars qw(@ISA %menuProps %menuDefaults);
@ISA = qw(Prima::VB::Types::generic);


%menuProps = (
   'key'     => 'key',
   'accel'   => 'string',
   'text'    => 'string',
   'name'    => 'menuname',
   'enabled' => 'bool',
   'checked' => 'bool',
   'image'   => 'image',
   'action'  => 'event',
);

%menuDefaults = (
   'key'     => kb::NoKey,
   'accel'   => '',
   'text'    => '',
   'name'    => 'MenuItem',
   'enabled' => 1,
   'checked' => 0,
   'image'   => undef,
   'action'  => 'my ( $self, $item) = @_;',
);


sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   my $w = $self-> {container}-> width;
   my $fh = $self-> {container}-> font-> height;

   my $divx = $h / 2;
   $self-> {A} = $self-> {container}-> insert( MPropListViewer =>
      origin => [ 0, 0],
      size   => [ 100, $divx],
      growMode => gm::Client,
      hScroll => 1,
      vScroll => 1,
      onSelectItem => sub {
         $self-> close_item;
         $self-> open_item;
      },
   );
   $self-> {A}->{master} = $self;

   $self-> {Div1} = $self-> {container}-> insert( Divider =>
      vertical => 0,
      origin => [ 0, $divx],
      size   => [ 100, 6],
      min    => 20,
      max    => 20,
      name   => 'Div',
      growMode => gm::Ceiling,
      onChange => sub {
         my $bottom = $_[0]-> bottom;
         $self-> {A}-> height( $bottom);
         $self-> {B}-> set(
            top    => $self-> {container}-> height,
            bottom => $bottom + 6,
         );
      }
   );

   $self-> {B} = $self-> {container}-> insert( MenuOutline =>
      origin => [ 0, $divx + 6],
      size   => [ 100, $h - $divx - 6],
      growMode => gm::Ceiling,
      hScroll => 1,
      vScroll => 1,
      popupItems => [
         ['~New' => q(new),],
         ['~Make node' => q(makenode),],
         ['~Delete' => q(del),],
      ],
      onSelectItem => sub {
         my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
         $self-> enter_menuitem( $x);
      },
   );
   $self-> {B}->{master} = $self;

   $self-> {Div2} = $self-> {container}-> insert( Divider =>
      vertical => 1,
      origin => [ 100, 0],
      size   => [ 6, $h - 1],
      min    => 50,
      max    => 50,
      name   => 'Div',
      onChange => sub {
         my $right = $_[0]-> right;
         $self-> {A}-> width( $_[0]-> left);
         $self-> {Div1}-> width( $_[0]-> left);
         $self-> {B}-> width( $_[0]-> left);
         $self-> {panel}-> set(
            width => $self-> {container}-> width - $right,
            left  => $right,
         );
      }
   );

   $self-> {panel} = $self-> {container}-> insert( Notebook =>
      origin    => [ 106, 0],
      size      => [ $w - 106, $h - 1],
      growMode  => gm::Right,
      name      => 'Panel',
      pageCount => 1,
   );
   $self-> {panel}->{pages} = {};
}

sub enter_menuitem
{
   my ( $self, $x ) = @_;
   if ( defined $x) {
      return if defined $self-> {current} and $self-> {current} == $x;
   } else {
      return unless defined $self-> {current};
   }
   $self-> {current} = $x;
   $self-> close_item;
   my $l = $self->{A};
   if ( $self-> {current}) {
      my @id = sort keys %menuProps;
      my @chk = ();
      my %ix  = ();
      my $num = 0;
      for ( @id) {
         push( @chk, exists $x->[0]->[1]->{$_} ? 1 : 0);
         $ix{$_} = $num++;
      }
      $l-> reset_items( \@id, \@chk, \%ix);
      $self-> open_item;
   } else {
      $l-> {id} = [];
      $l-> {check} = [];
      $l-> {index} = {};
      $l-> set_count( 0);
   }
}


sub close_item
{
   my ( $self ) = @_;
   return unless defined $self->{opened};
   $self->{opened} = undef;
}


sub open_item
{
   my ( $self) = @_;
   return if defined $self->{opened};
   my $list = $self->{A};
   my $f = $list-> focusedItem;

   if ( $f < 0) {
      $self->{panel}->pageIndex(0);
      return;
   }
   my $id   = $list->{id}->[$f];
   my $type = $VB::main-> get_typerec( $menuProps{ $id});
   my $p = $self->{panel};
   my $pageset;
   if ( exists $p->{pages}->{$type}) {
      $self-> {opened} = $self->{typeCache}->{$type};
      $pageset = $p->{pages}->{$type};
      $self-> {opened}-> renew( $id, $self);
   } else {
      $p-> pageCount( $p-> pageCount + 1);
      $p-> pageIndex( $p-> pageCount - 1);
      $p->{pages}->{$type} = $p-> pageIndex;
      $self-> {opened} = $type-> new( $p, $id, $self);
      $self-> {opened}-> {changeProc} = \&Prima::VB::Types::menuItems::item_changed_from_notebook;
      $self-> {typeCache}->{$type} = $self-> {opened};
   }
   my $drec = $self-> {current}->[0]->[1];
   my $data = exists $drec->{$id} ? $drec->{$id} : $menuDefaults{$id};
   $self-> {sync} = 1;
   $self-> {opened}-> set( $data);
   $self-> {sync} = undef;
   $p-> pageIndex( $pageset) if defined $pageset;
}

sub item_changed_from_notebook
{
   item_changed( $_[0]->{widget});
}

sub item_deleted
{
   my $self = $_[0];
   return unless $self;
   return unless $self-> {opened};
   return if $self-> {sync};
   $self-> {sync} = 1;
   my $id = $self->{A}->{id}->[$self->{A}-> focusedItem];
   $self-> {opened}-> set( $menuDefaults{$id});
   $self-> change;
   my $hash = $self->{current}->[0]->[1];
   if ( !exists $hash->{name} && !exists $hash->{text}) {
      my $newname = exists $hash->{image} ? 'Image' : '---';
      if ( $self->{current}->[0]->[0] ne $newname) {
         $self->{current}->[0]->[0] = $newname;
         $self->{B}-> reset_tree;
         $self->{B}-> update_tree;
         $self->{B}-> repaint;
      }
   }
   $self-> {sync} = 0;
}


sub item_changed
{
   my $self = $_[0];
   return unless $self;
   return unless $self-> {opened};
   return if $self-> {sync};
   if ( $self-> {opened}-> valid) {
      if ( $self-> {opened}-> can( 'get')) {

         my $list = $self-> {A};

         $self-> {sync} = 1;
         my $data = $self-> {opened}-> get;
         my $c = $self->{opened}->{widget}->{current};
         $c->[0]->[1]->{$self->{opened}->{id}} = $data;
         if ( $self->{opened}->{id} eq 'text') {
            $c->[0]->[0] = $data;
            $self->{B}-> reset_tree;
            $self->{B}-> update_tree;
            $self->{B}-> repaint;
         } elsif (( $self->{opened}->{id} eq 'name') && !exists $c->[0]->[1]->{text}) {
            $c->[0]->[0] = $data;
            $self->{B}-> reset_tree;
            $self->{B}-> update_tree;
            $self->{B}-> repaint;
         } elsif  (( $self->{opened}->{id} eq 'key') && !exists $c->[0]->[1]->{accel}) {
            $c->[0]->[1]->{accel} = $menuDefaults{accel};
            $list-> {check}->[$list-> {index}->{accel}] = 1;
            $list-> redraw_items($list-> {index}->{accel});
         } elsif  (( $self->{opened}->{id} eq 'accel') && !exists $c->[0]->[1]->{key}) {
            $c->[0]->[1]->{key} = $menuDefaults{key};
            $list-> {check}->[$list-> {index}->{key}] = 1;
            $list-> redraw_items($list-> {index}->{key});
         } elsif  (( $self->{opened}->{id} eq 'image') && exists $c->[0]->[1]->{text}) {
            delete $c->[0]->[1]->{text};
            $list-> {check}->[$list-> {index}->{text}] = 0;
            $list-> redraw_items($list-> {index}->{text});
         }

         my $ix = $list-> {index}->{$self->{opened}->{id}};
         unless ( $list-> {check}->[$ix]) {
            $list-> {check}->[$ix] = 1;
            $list-> redraw_items( $ix);
         }
         $self-> change;
         $self->{sync} = undef;
      }
   }
}

#use Data::Dumper;

sub set
{
   my ( $self, $data) = @_;
   $self-> {sync} = 1;
   my $setData = [];
   my $traverse;
   $traverse = sub {
      my ( $data, $set) = @_;
      my $c   = scalar @$data;
      my %items = ();
      if ( $c == 5) {
         @items{qw(name text accel key)} = @$data;
      } elsif ( $c == 4) {
         @items{qw(text accel key)} = @$data;
      } elsif ( $c == 3) {
         @items{qw(name text)} = @$data;
      } elsif ( $c == 2) {
         $items{text} = $$data[0];
      }
      if ( ref($items{text}) && $items{text}-> isa('Prima::Image')) {
         $items{image} = $items{text};
         delete $items{text};
      }
      if ( exists $items{name}) {
         $items{enabled} = 0 if $items{name} =~ s/^\-//;
         $items{checked} = 1 if $items{name} =~ s/^\*//;
         delete $items{name} unless length $items{name};
      }
      my @record = ([ defined $items{text} ? $items{text} :
       ( defined $items{text} ? $items{text} :
          ( defined $items{name} ? $items{name} :
             ( defined $items{image} ? "Image" : "---")
       )) , \%items], undef, 0);
      if ( ref($$data[-1]) eq 'ARRAY') {
         $record[1] = [];
         $record[2] = 1;
         $traverse-> ( $_, $record[1]) for @{$$data[-1]};
      } elsif ( $c > 1) {
         $items{action} = $$data[-1];
      }
      push( @$set, \@record);
   };
   $traverse->( $_, $setData) for @$data;
#print "set:";
#print Dumper( $setData);
   $self-> {B}-> items( $setData);
   $self-> {sync} = 0;
}

sub get
{
   my $self = $_[0];
   my $retData = [];
   my $traverse;
   $traverse = sub {
      my ($current, $ret) = @_;
      my @in = ();
      my @cmd = ();
      my $haschild = 0;
      my $i = $current->[0]->[1];
      my $hastext  = exists $i->{text};
      my $hasname  = exists $i->{name};
      my $hasimage = exists $i->{image};
      my $hasacc   = exists $i->{accel};
      my $hasact   = exists $i->{action};
      my $haskey   = exists $i->{key};
      my $hassub   = exists $i->{action};
      my $namepfx  = (( exists $i->{enabled} && !$i->{enabled}) ? '-' : '').
                     (( exists $i->{checked} &&  $i->{checked}) ? '*' : '');

      if ( $current-> [1] && scalar @{$current->[1]}) {
         $haschild = 1;
         $hasacc = $haskey = $hasact = 0;
         if ( $hastext || $hasimage) {
            push ( @cmd, qw(name text));
         }
      } elsif (( $hastext || $hasimage) && $hasacc && $haskey && $hasact) {
         push ( @cmd, qw( name text accel key action));
      } elsif (( $hastext || $hasimage) && $hasact) {
         push ( @cmd, qw( name text action));
      }

      for ( @cmd) {
         my $val = $i-> {$_};
         if (( $_ eq 'text') && $hasimage && !$hastext) {
            $_ = 'image';
            $val = $i-> {$_};
         } elsif ( $_ eq 'name') {
            $val = '' unless defined $val;
            $val = "$namepfx$val";
            next unless length $val;
         }
         push( @in, $val);
      }

      if ( $haschild) {
         my $cx = [];
         $traverse->( $_, $cx) for @{$current->[1]};
         push @in, $cx;
      }
      push @$ret, \@in;
   };
   $traverse->( $_, $retData) for @{$self->{B}-> items};
#print "get:";
#print Dumper( $retData);
   return $retData;
}


sub write
{
   my ( $class, $id, $data, $asPL) = @_;
   return 'undef' unless defined $data;
   my $c = '';
   my $traverse;
   $traverse = sub {
      my ( $data, $level) = @_;
      my $sc    = scalar @$data;
      my @cmd   = ();
      if ( $sc == 5) {
         @cmd = qw( name text accel key);
      } elsif ( $sc == 4) {
         @cmd = qw( text accel key);
      } elsif ( $sc == 3) {
         @cmd = qw( name text);
      } elsif ( $sc == 2) {
         @cmd = qw( text);
      }

      $c .= ' ' x ( $level * 3).'[ ';
      my $i = 0;
      for ( @cmd) {
         if ( $_ eq 'text' && ref($$data[$i]) && $$data[$i]-> isa('Prima::Image')) {
            $_ = 'image';
         }
         my $type = $VB::main-> get_typerec( $menuProps{$_}, $$data[$i]);
         $c .= $type-> write( $_, $$data[$i], $asPL) . ', ';
         $i++;
      }

      if ( ref($$data[-1]) eq 'ARRAY') {
         $level++;
         $c .= "[\n";
         $traverse-> ( $_, $level) for @{$$data[-1]};
         $c .= ' ' x ( $level * 3).']';
         $level--;
      } elsif ( $sc > 1) {
         $c .= $VB::main-> get_typerec( $menuProps{'action'}, $$data[$i])->
            write( 'action', $$data[$i], $asPL);
      }
      $c .= "], \n";
   };
   $traverse->( $_, 0) for @$data;
   return "\n[$c]";
}


package Prima::VB::Types::menuname;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::name);


sub valid
{
   my $self = $_[0];
   my $tx = $self->{A}->text;
   $self->wake, return 0 unless length( $tx);
   $self->wake, return 0 if $tx =~ /[\s\\\~\!\@\#\$\%\^\&\*\(\)\-\+\=\[\]\{\}\.\,\?\;\|\`\'\"]/;
   my $s = $self-> {widget}-> {B};
   my $ok = 1;
   my $fi = $s-> focusedItem;
   $s-> iterate( sub {
      my ( $current, $position, $level) = @_;
      return 0 if $position == $fi;
      $ok = 0, return 1 if defined $current->[0]->[1]->{name} && $current->[0]->[1]->{name} eq $tx;
      return 0;
   }, 1);
   $self->wake unless $ok;
   return $ok;
}

package Prima::VB::Types::key;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

use Prima::KeySelector;

sub open
{
   my $self = $_[0];
   my $i = $self->{container};
   $self-> {A} = $i-> insert( KeySelector => 
      origin   => [ 5, 5],
      size     => [ $i-> width - 10, $i-> height - 10],
      growMode => gm::Ceiling,
      onChange => sub { $self-> change; },
   );
}

sub get
{
   return $_[0]-> {A}-> key;
}

sub set
{
   my ( $self, $data) = @_;
   $self-> {A}-> key( $data); 
}

sub write
{
   my ( $class, $id, $data) = @_;
   return Prima::KeySelector::export( $data);
}


package ItemsOutline;
use vars qw(@ISA);
@ISA = qw(Prima::StringOutline MyOutline);

sub new_item
{
   return  ['New Item', undef, 0];
}

package Prima::VB::Types::treeItems;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);


sub open
{
   my $self = $_[0];
   my $h = $self-> {container}-> height;
   my $w = $self-> {container}-> width;
   my $fh = $self-> {container}-> font-> height;

   $self-> {A} = $self-> {container}-> insert( ItemsOutline =>
      origin => [ 0, $fh + 4],
      size   => [ $w - 1, $h - $fh - 4],
      growMode => gm::Client,
      hScroll => 1,
      vScroll => 1,
      popupItems => [
         ['~New' => q(new),],
         ['~Make node' => q(makenode),],
         ['~Delete' => q(del),],
      ],
      onSelectItem => sub {
         my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
         $self-> enter_menuitem( $x);
      },
   );
   $self-> {A}->{master} = $self;
   $self-> {B} = $self-> {container}-> insert( InputLine =>
      origin => [ 0, 1],
      width  => $w,
      growMode => gm::Floor,
      text => '',
      onChange => sub {
         my ( $x, $l) = $self-> {A}-> get_item( $self-> {A}-> focusedItem);
         if ( $x) {
            $x->[0] = $_[0]-> text;
            $self-> change;
            $self-> {A}-> reset_tree;
            $self-> {A}-> update_tree;
            $self-> {A}-> repaint;
         }
      },
   );
}

sub enter_menuitem
{
   my ( $self, $x ) = @_;
   $self-> {B}-> text( $x ? $x->[0] : '');
}

sub get
{
   return $_[0]-> {A}-> items;
}

sub set
{
   return $_[0]-> {A}-> items( $_[1]);
}


sub write
{
   my ( $class, $id, $data) = @_;
   return '[]' unless $data;
   my $c = '';
   my $traverse;
   $traverse = sub {
      my ($x,$lev) = @_;
      $c .= ' ' x ( $lev * 3);
      $c .= "['". Prima::VB::Types::generic::quotable($x->[0])."', ";
      if ( $x-> [1]) {
         $lev++;
         $c .= "[\n";
         $traverse->($_, $lev) for @{$x->[1]};
         $lev--;
         $c .= ' ' x ( $lev * 3)."], $$x[2]";
      }
      $c .= "],\n";
   };
   $traverse->($_, 0) for @$data;
   return "\n[$c]";
}


1;
