use strict;
package Prima::VB::Classes;

sub classes
{
   return (
      'Prima::Widget' => {
         icon => undef,
      },
   );
}

use Prima::Classes;


package Prima::VB::Object;

sub init_profiler
{
   my ( $self, $prf) = @_;
   $self-> {class}      = $prf->{class};
   $self-> {realClass}  = $prf->{realClass} if defined $prf->{realClass};
   $self-> {module}  = $prf->{module};
   $self-> {creationOrder} = $prf->{creationOrder};
   $self-> {creationOrder} = 0 unless defined $self-> {creationOrder};
   $self-> {profile} = $prf->{profile};
   $self-> {default} = $prf->{class}-> profile_default;
   $self-> prf_adjust_default( $self-> {profile}, $self-> {default});
   $self-> prf_set( %{$self-> {profile}});
   my $types = $self-> prf_types;
   my %xt = ();
   for ( keys %{$types}) {
      my ( $props, $type) = ( $types->{$_}, $_);
      $xt{$_} = $type for @$props;
   }
   $self-> {types} = \%xt;
}

sub prf_set
{
   my ( $self, %profile) = @_;
   $self->{profile} = {%{$self->{profile}}, %profile};
   my $check = $VB::inspector && ( $VB::inspector->{opened}) && ( $VB::inspector->{current} eq $self);
   $check    = $VB::inspector->{opened}->{id} if $check;
   for ( keys %profile) {
      my $cname = 'prf_'.$_;
      if ( $check) {
         ObjectInspector::widget_changed(0) if $check eq $_;
      }
      $self-> $cname( $profile{$_}) if $self-> can( $cname);
   }
}

sub prf_delete
{
   my ( $self, @dellist) = @_;
   my $df = $self->{default};
   my $pr = $self->{profile};
   my $check = $VB::inspector && ( $VB::inspector->{opened}) && ( $VB::inspector->{current} eq $self);
   $check    = $VB::inspector->{opened}->{id} if $check;
   for ( @dellist) {
      delete $pr->{$_};
      my $cname = 'prf_'.$_;
      if ( $check) {
         ObjectInspector::widget_changed(1) if $check eq $_;
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


package Prima::VB::Component;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::VB::Object);


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
      sizeMin    => [11,11],
      selectingButtons => mb::Right,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub prf_types
{
   return {
      name       => ['name'],
      Handle     => ['owner'],
      sibling    => ['delegateTo'],
   };
}

sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{delegateTo};
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
   for ( qw( marked sizeable)) {
      $self->$_( $profile{$_});
   }
   $profile{profile}->{name} = $self-> name;
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
   if ( $self-> {marked} or $self-> focused) {
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
      return q(SizeNESW0) if $y < $bwx;
      return q(SizeNWSE0) if $y >= $size[1] - $bwx;
      return q(SizeWE0);
   } elsif ( $x >= $size[0] - $bw) {
      return q(SizeNWSE1) if $y < $bwx;
      return q(SizeNESW1) if $y >= $size[1] - $bwx;
      return q(SizeWE1);
   } elsif (( $y < $bw) or ( $y >= $size[1] - $bw)) {
      return ( $y < $bw) ? q(SizeNESW0) : q(SizeNWSE0) if $x < $bwx;
      return ( $y < $bw) ? q(SizeNWSE1) : q(SizeNESW1) if $x >= $size[0] - $bwx;
      return q(SizeNS) . ($y < $bw ? '0' : '1');
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

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   if ( $btn == mb::Left) {
      if ( defined $VB::main->{currentClass}) {
         $VB::form-> insert_new_control( $self-> left + $x, $self-> bottom + $y, $self);
         return;
      }

      if ( $mod & km::Shift) {
         $self-> marked( !$self-> marked);
         return;
      }

      my $part = $self-> xy2part( $x, $y);
      my @mw = ();
      if ( $part eq q(client)) {
         if ( $self-> focused) {
            @mw = $VB::form-> marked_widgets;
         } else {
            @mw = $VB::form-> marked_widgets if $self-> marked;
            $self-> focus;
         }
      }
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
            push( @rects, [$_->client_to_screen(0,0), $_->client_to_screen($_->size)]);
         }
         $self-> {sav} = [$self-> origin];
         $self-> {drag} = 1;
         $VB::form-> dm_init( $self);
         $self-> {extraWidgets} = \@mw;
         $self-> {extraRects}   = \@rects;
         $self-> {prevRect} = [$self-> client_to_screen(0,0), $self-> client_to_screen( $self-> size)];
         $self-> update_view;
         $VB::form->{saveHdr} = $VB::form-> text;
         $self-> xorrect( @{$self-> {prevRect}}, 1);
         return;
      }

      $self-> focus;

      if ( $part =~ /Size/) {
         $self-> {sav} = [$self-> rect];
         $part =~ s/Size//;
         $self-> {sizeAction} = $part;
         my ( $xa, $ya) = ( 0,0);
         if    ( $part eq q(NS0))   { ( $xa, $ya) = ( 0,-1); }
         elsif ( $part eq q(NS1))   { ( $xa, $ya) = ( 0, 1); }
         elsif ( $part eq q(WE0))   { ( $xa, $ya) = (-1, 0); }
         elsif ( $part eq q(WE1))   { ( $xa, $ya) = ( 1, 0); }
         elsif ( $part eq q(NESW0)) { ( $xa, $ya) = (-1,-1); }
         elsif ( $part eq q(NESW1)) { ( $xa, $ya) = ( 1, 1); }
         elsif ( $part eq q(NWSE0)) { ( $xa, $ya) = (-1, 1); }
         elsif ( $part eq q(NWSE1)) { ( $xa, $ya) = ( 1,-1); }
         $self-> {dirData} = [$xa, $ya];
         $self-> {prevRect} = [$self-> client_to_screen(0,0), $self-> client_to_screen( $self-> size)];
         $self-> update_view;
         $VB::form->{saveHdr} = $VB::form-> text;
         $self-> xorrect( @{$self-> {prevRect}}, 1);
         return;
      }
   }
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   return unless $dbl;
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
      if ( $VB::main-> {gsnap}) {
         my @og = $self-> origin;
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
            my ( $xa, $ya) = @{$self->{dirData}};

            if ( $VB::main-> {gsnap}) {
               my @og = $self-> origin;
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
               $self-> {prevRect} = [$self-> owner-> client_to_screen( @new[0,1]), $self-> owner-> client_to_screen( @new[2,3])];
               $self-> xorrect( @{$self-> {prevRect}}, 1);
            }
            return;
         } else {
            my $part = $self-> xy2part( $x, $y);
            my $crx;
            return if !$self-> enabled;
            if ( $part =~ /Size/) {
               chop($part);
               $crx = &{$cr::{$part}};
            } else {
               $crx = cr::Arrow;
            }
            $self-> pointer( $crx);
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
         @r = ( $self-> owner-> screen_to_client(@r[0,1]), $self-> owner-> screen_to_client(@r[2,3]));
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
   }
}

sub on_enter
{
   $_[0]->marked(1,1);
   ObjectInspector::enter_widget( $_[0]);
};

sub on_leave {$_[0]->marked(0,1)};

sub marked
{
   if ( $#_) {
     my ( $self, $mark, $exlusive) = @_;
     return if $self == $VB::form && $mark != 0;
     return if $mark == $self->{marked} && !$exlusive;
     if ( $exlusive) {
        $_-> marked(0) for $VB::form-> marked_widgets;
     }
     $self-> {marked} = $mark;
     $self-> repaint;
   } else {
      return 0 if $_[0] == $VB::form;
      return 1 if $_[0]-> focused;
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

sub prf_name
{
   my $old = $_[0]->name;
   $_[0]->name($_[1]);
   return unless $VB::inspector;
   my $s = $VB::inspector-> Selector;
   $VB::inspector->{selectorChanging} = 1;
   my @it = @{$s-> List-> items};
   my $sn = $s-> text;
   my $si = $s-> focusedItem;
   for ( @it) {
      $_ = $_[1] if $_ eq $old;
   }
   $s-> List-> items( \@it);
   $s-> text( $sn);
   $s-> focusedItem( $si);
   $VB::inspector->{selectorChanging} = 0;

   $_[0]-> name_changed( $old, $_[1]);
}

sub name_changed
{
   return unless $VB::form;
   my ( $self, $oldName, $newName) = @_;

   for ( $VB::form-> widgets) {
      my $pf = $_-> prf_types;
      next unless defined $pf->{Handle};
      my $widget = $_;
      for ( @{$pf->{Handle}}) {
         my $val = $widget->prf( $_);
         next unless defined $val;
         $widget-> prf_set( $_ => $newName) if $val eq $oldName;
      }
   }
}

sub on_destroy
{
   my $self = $_[0];
   $_[0]-> name_changed( $_[0]-> name, $VB::form-> name) if $VB::form;
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
      rop
      rop2
      textOpaque
      textOutBaseline
      transform
   );
   $def->{text} = '' unless defined $def->{text};
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
         'ownerShowHint','ownerPalette','pointerVisible','scaleChildren',
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
      helpContext   => ['helpContext'],
      string        => ['hint'],
      text          => ['text'],
      selectingButtons=> ['selectingButtons'],
      widgetClass   => ['widgetClass'],
      image         => ['shape'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
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
   $self-> prf_set( origin => [$x, $y]);
}

sub on_size
{
   my ( $self, $ox, $oy, $x, $y) = @_;
   return if $self-> {syncRecting};
   $self-> prf_set( size => [$x, $y]);
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
      pointerVisible
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
@ISA = qw(Prima::VB::Widget);

sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
       menu
       modalResult
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
      bool          => ['modalHorizon', 'taskListed'],
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
use Prima::ColorDialog;
use Prima::Label;
use Prima::FontDialog;

package Prima::VB::Types::generic;

sub new
{
   my $class = shift;
   my $self = {};
   bless( $self, $class);
   ( $self-> {container}, $self->{id}, $self->{widget}) = @_;
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

sub write
{
   my ( $class, $id, $data) = @_;
   return "'". Prima::VB::Types::generic::quotable($data)."'";
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
         ObjectInspector::item_changed();
      },
   );
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

   my $c = $_[0]->{A}->backColor;
   $_[0]->{A}->backColor(cl::LightRed);
   $_[0]->{A}->{monger} = $_[0]->{A}->insert( Timer =>
      timeout => 300,
      onTick  => sub {
         $_[0]-> owner->{monger} = undef;
         $_[0]-> owner-> backColor($c);
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
   my $s = $VB::inspector-> Selector-> List-> items;
   for ( @$s) {
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
      origin   => [ 5, $fh * 4 + 10],
      size     => [ $sz[0]-10, $sz[1] - $fh * 4 - 16],
      growMode => gm::Client,
      onChange => sub {
         ObjectInspector::item_changed();
      },
   );
   $i-> insert( Button =>
      origin => [5, $fh * 2 + 16],
      size   => [$sz[0]-10,$fh+6],
      text   => '~Load',
      onClick => sub {
         my $d = Prima::OpenDialog-> create(
            text => 'Load text',
            icon => $VB::ico,
         );
         if ( $d-> execute) {
            my $f = $d-> fileName;
            if ( open F, $f) {
               local $/;
               $f = <F>;
               $self->{A}->text( $f);
               close F;
               ObjectInspector::item_changed();
            } else {
               Prima::MsgBox::message("Cannot load $f");
            }
         }
         $d-> destroy;
      },
   );
   $self->{B} = $i-> insert( Button =>
      origin => [5, $fh + 10],
      size   => [$sz[0]-10,$fh+6],
      text   => '~Save',
      onClick => sub {
         my $dlg  = Prima::SaveDialog-> create(
            icon => $VB::ico,
         );
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
         $dlg-> destroy;
      },
   );
   $i-> insert( Button =>
      origin => [5, 4],
      size   => [$sz[0]-10,$fh+6],
      text   => '~Clear',
      onClick => sub {
         $self-> set( '');
         ObjectInspector::item_changed();
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
         ObjectInspector::item_changed();
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
         ObjectInspector::item_changed();
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
         ObjectInspector::item_changed();
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

sub write
{
   my ( $class, $id, $data) = @_;
   return "'". Prima::VB::Types::generic::quotable($data)."'";
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
         ObjectInspector::item_changed();
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
         ObjectInspector::item_changed();
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
         ObjectInspector::item_changed();
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
      my ( $acl, $awc) = ( $data & 0x80000FFF, $data & 0x0FFF0000);
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
      my ( $acl, $awc) = ( $data & 0x80000FFF, $data & 0x0FFF0000);
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
         ObjectInspector::item_changed();
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
         ObjectInspector::item_changed();
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
   $_[0]-> {L}-> text( $_[0]->{id}.'x:');
   $_[0]-> {M}-> text( $_[0]->{id}.'y:');
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
         ObjectInspector::item_changed();
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

package Prima::VB::Types::gaugeRelief;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Sink Border Raise); }
sub packID { 'gr'; }

package Prima::VB::Types::sliderScheme;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Gauge Axis Thermometer StdMinMax); }
sub packID { 'ss'; }


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
   ObjectInspector::item_changed();
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
      origin   => [ 5, $fh * 4 + 10],
      size     => [ $sz[0]-10, $sz[1] - $fh * 4 - 16],
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

   $i-> insert( Button =>
      origin => [5, $fh * 2 + 16],
      size   => [$sz[0]-10,$fh+6],
      text   => '~Load',
      onClick => sub {
         my $d = Prima::OpenDialog-> create(
            icon => $VB::ico,
            text => 'Load image',
            filter    => [
               ['Images' => '*.bmp;*.pcx;*.gif;*.jpg;*.png;*.tif'],
               ['All files' => '*.*']
            ],
         );
         if ( $d-> execute) {
            my $in = $self-> imgClass();
            my $i = $in-> create;
            my $f = $d-> fileName;
            if ( $i-> load( $f)) {
               $self-> set( $i);
               ObjectInspector::item_changed();
            } else {
               Prima::MsgBox::message("Cannot load $f");
            }
         }
         $d-> destroy;
      },
   );
   $self->{B} = $i-> insert( Button =>
      origin => [5, $fh + 10],
      size   => [$sz[0]-10,$fh+6],
      text   => '~Save',
      onClick => sub {
         my $dlg  = Prima::SaveDialog-> create(
            icon => $VB::ico,
            filter    => [
               ['Images' => '*.bmp;*.pcx;*.gif;*.jpg;*.png;*.tif'],
               ['All files' => '*.*']
            ],
         );
         if ( $dlg-> execute) {
            unless ( $self->{A}->{icon}->save( $dlg-> fileName)) {
               Prima::MsgBox::message('Cannot save '.$dlg-> fileName);
            }
         }
         $dlg-> destroy;
      },
   );
   $i-> insert( Button =>
      origin => [5, 4],
      size   => [$sz[0]-10,$fh+6],
      text   => '~Clear',
      onClick => sub {
         $self-> set( undef);
         ObjectInspector::item_changed();
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
   $self->{B}-> enabled( defined $data);
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



1;
