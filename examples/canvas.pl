# $Id$
# todo:
# - objects: 
#      polyline -> line(arrow,smooth)
#      polygon(smooth)
#      text(font)
#      widget
# - gp:tileoffset
use strict;
use Prima qw(Application StdBitmap);

use Prima qw(ScrollWidget);
# A widget with two scrollbars. Contains set of objects, that know
# how to draw themselves. The graphic objects hierarchy starts
# from GraphicObject class

package Prima::Canvas;
use vars qw(@ISA);
@ISA = qw(Prima::ScrollWidget);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      zoom       => 1,
      paneSize   => [ 0, 0],
      paneWidth  => 0,
      paneHeight => 0,
      alignment  => ta::Left,
      valignment => ta::Bottom,
      selectable => 1,
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   if ( exists( $p-> { paneSize})) {
      $p-> { paneWidth}  = $p-> { paneSize}-> [ 0];
      $p-> { paneHeight} = $p-> { paneSize}-> [ 1];
   }
}   

sub init
{
   my ( $self, %profile) = @_;
   $self->{zoom} = 1;
   $self->{$_} = 0 for qw(paneWidth paneHeight alignment valignment);
   $self->{objects} = [];
   %profile = $self-> SUPER::init(%profile);
   $self-> $_($profile{$_}) for qw(zoom paneWidth paneHeight alignment valignment);
   return %profile;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   $canvas-> clear;
   my $zoom = $self->{zoom};
   my @c = $canvas-> clipRect;
   my %props;
   my %defaults = map { $_ => $canvas-> $_() } @Prima::CanvasObject::uses;
   for my $obj ( @{$self->{objects}}) {
      my @r = $self-> object2screen( $obj-> rect);
      $r[2]--; $r[3]--;
      next if !$obj->visible || $r[0] > $c[2] || $r[1] > $c[3] || $r[2] < $c[0] || $r[3] < $c[1];

      my @uses = $obj->uses;
      delete @props{@uses};
      $canvas-> set( 
         (map { $_ => $obj-> $_()   } @uses),
	 (map { $_ => $defaults{$_} } keys %props)
      );
      %props = map { $_ => 1 } @uses;
      
      $canvas-> translate( @r[0,1]);
      $canvas-> clipRect( @r);
      $r[2] -= $r[0];
      $r[3] -= $r[1];
      $obj-> on_paint( $canvas, $r[2]+1,$r[3]+1);
   }
   $canvas-> translate(0,0);
   $canvas-> clipRect(@c);
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   $self-> propagate_mouse_event( 'on_mousedown', $x, $y, $btn, $mod, $x, $y);
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   $self-> propagate_mouse_event( 'on_mouseup', $x, $y, $btn, $mod, $x, $y);
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   $self-> propagate_mouse_event( 'on_mousemove', $x, $y, $mod, $x, $y);
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   $self-> propagate_mouse_event( 'on_mousemove', $x, $y, $mod, $x, $y, $dbl);
}

sub on_keydown
{
   my ( $self, $code, $key, $mod, $repeat) = @_;
   $self-> propagate_event( 'on_keydown', $code, $key, $mod, $repeat);
}

sub on_keyup
{
   my ( $self, $code, $key, $mod) = @_;
   $self-> propagate_event( 'on_keyup', $code, $key, $mod);
}

sub delete_object
{
   my ( $self, $obj) = ( shift, shift);
   @{$self->{objects}} = grep { $_ != $obj } @{$self->{objects}};
   $self-> {selection} = undef 
      if $self->{selection} && $self->{selection} == $obj;
   my @r = $self-> object2screen( $obj-> rect);
   $self-> invalidate_rect( @r) if $obj-> visible;
}

sub insert_object
{
   my ( $self, $class) = ( shift, shift);
   my $obj;
   $self-> attach_object( $obj = $class-> new(
      @_,
      owner => $self,
   ));
   $obj;
}

sub attach_object 
{
   push @{$_[0]->{objects}}, $_[1];
   $_[1]-> {owner} = $_[0];
   $_[1]-> repaint;
}

sub object2screen
{
   my $self = $_[0];
   my $i;
   my @d = $self-> deltas;
   my ( $ha, $va) = ( $self-> {alignment}, $self-> {valignment});
   my ($x, $y) = $self-> get_active_area(2);
   my @l = $self-> limits;
   if ( $l[0] < $x) {
      if ( $ha == ta::Left) {
      } elsif ( $ha != ta::Right) {
         $d[0] -= ($x - $l[0])/2;
      } else {
         $d[0] -= $x - $l[0];
      }
   }
   if ( $l[1] < $y) {
      if ( $va == ta::Top) {
         $d[1] -= $y - $l[1];
      } elsif ( $va != ta::Bottom) {
         $d[1] -= ($y - $l[1])/2;
      }
   } else {
      $d[1] = $l[1] - $y - $d[1];
   }
   $d[$_] -= $self->{indents}->[$_] for 0,1;
   my $zoom = $self->{zoom};
   my @ret;
   for ( $i = 1; $i <= $#_; $i+=2) {
      push @ret, $_[$i] * $zoom - $d[0];
      push @ret, $_[$i+1] * $zoom - $d[1] if defined $_[$i+1];
   }
   return map {
      ( $_ < 0) ?
         int( $_ - .5) :
         int( $_ + .5)
   } @ret;
}

sub screen2object
{
   my $self = $_[0];
   my $i;
   my @d = $self-> deltas;
   my ( $ha, $va) = ( $self-> {alignment}, $self-> {valignment});
   my ($x, $y) = $self-> get_active_area(2);
   my @l = $self-> limits;
   if ( $l[0] < $x) {
      if ( $ha == ta::Left) {
      } elsif ( $ha != ta::Right) {
         $d[0] -= ($x - $l[0])/2;
      } else {
         $d[0] -= $x - $l[0];
      }
   }
   if ( $l[1] < $y) {
      if ( $va == ta::Top) {
         $d[1] -= $y - $l[1];
      } elsif ( $va != ta::Bottom) {
         $d[1] -= ($y - $l[1])/2;
      }
   } else {
      $d[1] = $l[1] - $y - $d[1];
   }
   my $zoom = $self->{zoom};
   my @ret;
   $d[$_] -= $self->{indents}->[$_] for 0,1;
   for ( $i = 1; $i <= $#_; $i+=2) {
      push @ret, ($_[$i]   + $d[0]) / $zoom;
      push @ret, ($_[$i+1] + $d[1]) / $zoom if defined $_[$i+1];
   }
   @ret;
}

sub position2object
{
   my ( $self, $x, $y, $skip_hittest) = @_;
   my ( $nx, $ny) = $self-> screen2object( $x, $y);
   $self-> push_event;
   for my $obj ( reverse @{$self->{objects}}) {
      next unless $obj-> visible;
      my @r = $obj-> rect;
      if ( $r[0] <= $nx && $r[1] <= $ny && $r[2] >= $nx && $r[3] >= $ny) {
         my @s = $self-> object2screen(@r[0,1]);
         if ( $skip_hittest || $obj-> on_hittest( $x - $s[0], $y - $s[1])) {
	    $self-> pop_event;
	    return ($obj, $x - $s[0], $y - $s[1]);
	 }
      }
   }
   $self-> pop_event;
   return;
}

sub propagate_mouse_event
{
   my ( $self, $event, $x, $y, @params) = @_;
   my ( $obj, $nx, $ny) = $self-> position2object( $x, $y);
   return unless $obj;
   $self-> push_event;
   $obj-> $event( @params);
   $self-> pop_event;
}

sub propagate_event
{
   my ( $self, $event, @params) = @_;
   $self-> push_event;
   for ( reverse $self-> objects) {
      $_-> $event( @params);
      last unless $self-> eventFlag;
   }
   $self-> pop_event;
}

sub reset_zoom
{
   my ( $self ) = @_;
   $self-> limits( 
      $self-> {paneWidth} * $self-> {zoom},
      $self-> {paneHeight} * $self-> {zoom}
   );
}

sub alignment
{
   return $_[0]->{alignment} unless $#_;
   $_[0]->{alignment} = $_[1];
   $_[0]->repaint;
}

sub valignment
{
   return $_[0]->{valignment} unless $#_;
   $_[0]->{valignment} = $_[1];
   $_[0]->repaint;
}


sub paneWidth
{
   return $_[0]-> {paneWidth} unless $#_;
   my ( $self, $pw) = @_;
   $pw = 0 if $pw < 0;
   return if $pw == $self-> {paneWidth};
   $self-> {paneWidth} = $pw;
   $self-> reset_zoom;
   $self-> repaint;
}

sub paneHeight
{
   return $_[0]-> {paneHeight} unless $#_;
   my ( $self, $ph) = @_;
   $ph = 0 if $ph < 0;
   return if $ph == $self-> {paneHeight};
   $self-> {paneHeight} = $ph;
   $self-> reset_zoom;
   $self-> repaint;
}

sub paneSize
{
   return $_[0]-> {paneWidth}, $_[0]-> {paneHeight} if $#_ < 2;
   my ( $self, $pw, $ph) = @_;
   $ph = 0 if $ph < 0;
   $pw = 0 if $pw < 0;
   return if $ph == $self-> {paneHeight} && $pw == $self->{paneWidth};
   $self-> {paneWidth}  = $pw;
   $self-> {paneHeight} = $ph;
   $self-> reset_zoom;
   $self-> repaint;
}

sub zoom
{
   return $_[0]->{zoom} unless $#_;
   my ( $self, $zoom) = @_;
   return if $zoom == $self->{zoom};
   $self->{zoom} = $zoom;
   $self-> reset_zoom;
   $self-> repaint;
}

sub zorder
{
   my ( $self, $obj, $command) = @_;
   my $idx;
   my $o = $self-> {objects};
   if ( $command ne 'first' and $command ne 'last') {
      for ( $idx = 0; $idx < @$o; $idx++) {
	 last if $obj == $$o[$idx];
      }
      return if $idx == @$o;
   }
   if ( $command eq 'front') {
      @$o = grep { $_ != $obj } @$o;
      push @$o, $obj;
   } elsif ( $command eq 'back') {
      @$o = grep { $_ != $obj } @$o;
      unshift @$o, $obj;
   } elsif ( $command eq 'first') {
      return $$o[0];
   } elsif ( $command eq 'last') {
      return $$o[-1];
   } elsif ( $command eq 'next') {
      return $$o[$idx+1];
   } elsif ( $command eq 'prev') {
      return $idx ? $$o[$idx-1] : undef;
   } else {
      my $i;
      my @o = grep { $_ != $obj } @$o;
      return if @o == @$o;
      @$o = @o;
      for ( $i = 0; $i < @$o; $i++) {
         next unless $$[$i] != $command;
	 splice @$o, $i, 0, $obj;
	 last;
      }
   }
   $obj-> on_zorderchanged();
   $obj-> repaint;
}

sub objects {@{$_[0]->{objects}}}

package Prima::CanvasEdit;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas);

sub on_paint
{
   my ( $self, $canvas) = @_;
   $self-> SUPER::on_paint( $canvas);
   $canvas-> set(
      linePattern => lp::Solid,
      rop => rop::CopyPut,
      lineWidth => 0,
      color => 0,
   );
   my @r = $self-> object2screen( 0, 0, $self-> paneSize);
   $canvas-> rectangle( $r[0]-1, $r[1]-1, $r[2], $r[3]);
   return unless $self-> {selection};
   @r = $self-> object2screen($self->{selection}-> rect);
   $r[2]--;
   $r[3]--;
   $canvas-> rect_focus(@r);
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   my $found;
   if ( $btn == mb::Left && !$self-> {transaction}) {
      my ( $obj, $nx, $ny) = $self-> position2object( $x, $y);
      if ( $obj) {
         $self-> {anchor} = [ $nx, $ny ];
         $obj-> bring_to_front;
         $self-> selected_object( $found = $self-> {transaction} = $obj);
	 $self-> capture(1, $self);
      }
   }
   $self-> selected_object(undef) if $self-> {selection} && !$found;
   $self-> SUPER::on_mousedown( $btn, $mod, $x, $y);
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   if ( $self-> {transaction} && $btn == mb::Left) {
      $self-> {transaction} = undef;
      $self-> capture(0);
   }
   $self-> SUPER::on_mouseup( $btn, $mod, $x, $y);
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   if ( $self-> {transaction}) {
      my @p = $self-> paneSize;
      $x -= $self-> {anchor}->[0];
      $y -= $self-> {anchor}->[1];
      my @o = $self-> screen2object( $x, $y);
      my @s = $self-> {transaction}-> size;
      for ( 0..1) {
         $o[$_] = 0 if $o[$_] < 0;
	 $o[$_] = $p[$_] - $s[$_] - 1 if $o[$_] >= $p[$_] - $s[$_];
      }
      $self-> {transaction}-> origin( @o);
   }
   $self-> SUPER::on_mousemove( $mod, $x, $y);
}

sub on_keydown
{
   my ( $self, $code, $key, $mod, $repeat) = @_;
   if ( $key == kb::Tab || $key == kb::BackTab) {
      my $new = $self-> selected_object;
      if ( $key == kb::Tab) {
         $new = $self-> zorder( $new, $new ? 'prev' : 'last');
	 $new = $self-> zorder( undef, 'last') unless $new;
      } else {
         $new = $self-> zorder( $new, $new ? 'next' : 'first');
	 $new = $self-> zorder( undef, 'first') unless $new;
      }
      if ( $new) {
         $self-> selected_object( $new);
	 $self-> clear_event;
	 return;
      }
   }
   $self-> SUPER::on_keydown( $code, $key, $mod, $repeat);
}

sub selected_object
{
   return $_[0]-> {selection} unless $#_;
   return if $_[1] && $_[1]-> owner != $_[0];
   $_[0]-> {selection}-> repaint if $_[0]-> {selection};
   $_[0]-> {selection} = $_[1];
   $_[0]-> {selection}-> repaint if $_[0]-> {selection};
}


package Prima::CanvasObject;
use vars qw(%defaults @uses);

{
   @uses = qw( backColor color fillPattern font lineEnd linePattern
               lineWidth region rop rop2 splinePrecision textOpaque 
	       textOutBaseline lineJoin fillWinding);
   my $pd = Prima::Drawable-> profile_default();
   %defaults = map { $_ => $pd->{$_} } @uses;
}

sub new
{
   my ( $class, @whatever) = @_;
   my $self = bless {
      @whatever,
   }, $class;
   my %defaults = $self-> profile_default;
   for ( keys %defaults) {
      next if exists $self-> {$_};
      $self-> {$_} = $defaults{$_};
   }
   die "No ``owner'' is specified" unless $self-> {owner};
   $self-> on_create;
   $self-> repaint;
   return $self;
}

sub DESTROY { shift-> on_destroy }

sub profile_default 
{
   %defaults,
   origin  => [ 0, 0],
   size    => [ 100, 100],
   visible => 1,
   name    => '',
}

sub uses
{
   return ();
}

sub clear_event
{
   $_[0]-> owner-> clear_event;
}

sub on_create
{
}

sub on_destroy
{
}

sub on_hittest
{
   my ( $self, $x, $y) = @_;
   1;
}

sub on_keydown
{
   my ( $self, $code, $key, $mod, $repeat) = @_;
}

sub on_keyup
{
   my ( $self, $code, $key, $mod) = @_;
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
}

sub on_move
{
   my ( $self, $oldx, $oldy, $x, $y) = @_;
}

sub on_size
{
   my ( $self, $oldx, $oldy, $x, $y) = @_;
}

sub on_zorderchanged
{
   my ( $self) = @_;
}

sub repaint
{
   delete $_[0]->{_update} if $_[0]->{_update};
   $_[0]-> _update( $_[0]-> origin, $_[0]-> size);
}

sub _begin_update
{
   my $self = $_[0];
   return if !$self-> {visible} || $self-> {_lock_update};
   $self->{_update} = [];
}

sub _update
{
   my ( $self, $x, $y, $w, $h) = @_;
   return unless $self->{visible};
   my $auto = ! $self->{_update};
   push @{$self->{_update}}, $x, $y, $x + $w, $y + $h;
   $self-> _end_update if $auto && !$self->{_lock_update};
}

sub _end_update
{
   my $self = $_[0];
   return if !$self->{visible} || $self-> {_lock_update} || !$self->{_update};
   my $o = $self-> owner;
   my @o = $o-> object2screen( @{$self->{_update}});
   my $i;
   for ($i = 0; $i < @o; $i+=4) {
      $o-> invalidate_rect( @o[$i..$i+3]);
   }
   delete $self->{_update};
}

sub name { $#_ ? $_[0]->{name} = $_[1] : $_[0]->{name} }

sub lock { $_[0]->{_lock_update}++ }

sub unlock
{
   return unless $_[0]->{_lock_update};
   $_[0]-> _end_update unless --$_[0]->{_lock_update};
}

sub owner
{
   return $_[0]-> {owner} unless $#_;
   $_[0]-> {owner}-> delete_object( $_[0]) if $_[0]-> {owner};
   $_[0]-> {owner} = undef;
   $_[1]-> attach_object( $_[0]) if $_[1];
}

sub left   
{ 
   $#_ ? 
      $_[0]-> origin( $_[1], $_[0]-> {origin}-> [1]) :
      $_[0]-> {origin}->[0]
}

sub bottom 
{ 
   $#_ ? 
       $_[0]-> origin( $_[0]-> {origin}-> [0], $_[1]) :
       $_[0]-> {origin}->[1] 
}

sub right
{
   $#_ ?
      $_[0]-> size( $_[1] - $_[0]->{origin}->[0], $_[0]-> {size}-> [1]) :
      $_[0]-> {origin}->[0] + $_[0]->{size}->[0]
}

sub top
{
   $#_ ?
      $_[0]-> size( $_[1] - $_[0]->{origin}->[0], $_[0]-> {size}-> [1]) :
      $_[0]-> {origin}->[0] + $_[0]->{size}->[0]
}

sub width
{
   $#_ ?
      $_[0]-> size( $_[1], $_[0]-> {size}-> [0]) :
      $_[0]-> {size}-> [0]
}

sub height
{
   $#_ ?
      $_[0]-> size( $_[0]-> {size}-> [1], $_[1]) :
      $_[0]-> {size}-> [1]
}

sub rect
{
   unless ( $#_) {
      my @o = @{$_[0]->{origin}};
      my @s = @{$_[0]->{size}};
      return @o, $s[0] + $o[0], $s[1] + $o[1];
   }
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   ( $x1, $x2) = ( $x2, $x1) if $x2 > $x1;
   ( $y1, $y2) = ( $y2, $y1) if $y2 > $y1;
   $self-> lock;
   $self-> origin( $x1, $y1);
   $self-> size( $x2 - $x1, $y2 - $y1);
   $self-> unlock;
}

sub origin
{
   return @{$_[0]->{origin}} unless $#_;
   my ( $self, $x, $y) = @_;
   return if $x == $self->{origin}->[0] and $y == $self->{origin}->[1];
   my @o = @{$self->{origin}};
   $self-> _begin_update;
   $self-> _update( @{$self->{origin}}, @{$self->{size}});
   @{$self->{origin}} = ( $x, $y);
   $self-> _update( @{$self->{origin}}, @{$self->{size}});
   $self-> on_move( @o, $x, $y);
   $self-> _end_update;
}

sub size
{
   return @{$_[0]->{size}} unless $#_;
   my ( $self, $x, $y) = @_;
   $x = 0 if $x < 0;
   $y = 0 if $y < 0;
   return if $x == $self->{size}->[0] and $y == $self->{size}->[1];
   my @s = @{$self->{size}};
   $self-> _begin_update;
   $self-> _update( @{$self->{origin}}, @{$self->{size}});
   @{$self->{size}} = ( $x, $y);
   $self-> _update( @{$self->{origin}}, @{$self->{size}});
   $self-> on_size( @s, $x, $y);
   $self-> _end_update;
}

sub bring_to_front { $_[0]-> owner-> zorder( $_[0], 'front') }
sub send_to_back   { $_[0]-> owner-> zorder( $_[0], 'back') }
sub insert_behind  { $_[0]-> owner-> zorder( $_[0], $_[1]) }
sub first          { $_[0]-> owner-> zorder( $_[0], 'first') }
sub last           { $_[0]-> owner-> zorder( $_[0], 'last') }
sub next           { $_[0]-> owner-> zorder( $_[0], 'next') }
sub prev           { $_[0]-> owner-> zorder( $_[0], 'prev') }

sub visible
{
   return $_[0]->{visible} unless $#_;
   return if $_[0]-> {visible} == $_[1];
   $_[0]-> {visible} = $_[1];
   $_[0]-> owner-> invalidate_rect( $_[0]-> owner-> object2screen( $_[0]-> rect));
}

sub color
{
   return $_[0]-> {color} unless $#_;
   $_[0]-> {color} = $_[1];
   $_[0]-> repaint;
}

sub backColor
{
   return $_[0]-> {backColor} unless $#_;
   $_[0]-> {backColor} = $_[1];
   $_[0]-> repaint;
}

sub fillPattern
{
   return $_[0]-> {fillPattern} unless $#_;
   $_[0]-> {fillPattern} = $_[1];
   $_[0]-> repaint;
}

sub font
{
   return $_[0]-> {font} unless $#_;
   my ( $self, %font) = @_;
   for ( keys %font) {
      $self-> {font}->{$_} = $font{$_};
   }
   $_[0]-> repaint;
}

sub lineWidth
{
   return $_[0]-> {lineWidth} unless $#_;
   $_[0]-> {lineWidth} = $_[1];
   $_[0]-> repaint;
}

sub linePattern
{
   return $_[0]-> {linePattern} unless $#_;
   $_[0]-> {linePattern} = $_[1];
   $_[0]-> repaint;
}

sub lineEnd
{
   return $_[0]-> {lineEnd} unless $#_;
   $_[0]-> {lineEnd} = $_[1];
   $_[0]-> repaint;
}

sub lineJoin
{
   return $_[0]-> {lineJoin} unless $#_;
   $_[0]-> {lineJoin} = $_[1];
   $_[0]-> repaint;
}

sub fillWinding
{
   return $_[0]-> {fillWinding} unless $#_;
   $_[0]-> {fillWinding} = $_[1];
   $_[0]-> repaint;
}

sub rop
{
   return $_[0]-> {rop} unless $#_;
   $_[0]-> {rop} = $_[1];
   $_[0]-> repaint;
}

sub rop2
{
   return $_[0]-> {rop2} unless $#_;
   $_[0]-> {rop2} = $_[1];
   $_[0]-> repaint;
}

sub splinePrecision
{
   return $_[0]-> {splinePrecision} unless $#_;
   $_[0]-> {splinePrecision} = $_[1];
   $_[0]-> repaint;
}

sub textOutBaseline
{
   return $_[0]-> {textOutBaseline} unless $#_;
   $_[0]-> {textOutBaseline} = $_[1];
   $_[0]-> repaint;
}

sub textOpaque
{
   return $_[0]-> {textOpaque} unless $#_;
   $_[0]-> {textOpaque} = $_[1];
   $_[0]-> repaint;
}

package Prima::Canvas::Outlined;
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub uses { return qw( rop rop2 backColor color lineWidth linePattern lineEnd); }

package Prima::Canvas::Filled; 
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub uses { return qw( rop rop2 color backColor fillPattern lineEnd); }

package Prima::Canvas::FilledOutlined;
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub profile_default 
{
   $_[0]-> SUPER::profile_default,
   fill    => 1,
   outline => 1,
   fillBackColor => cl::Black,
   outlineBackColor => cl::Black,
}

sub uses { 
   my $self = $_[0];
   my @ret = qw(rop rop2 color backColor);
   push @ret, qw(lineWidth linePattern lineEnd) if $self->{outline};
   push @ret, qw(fillPattern) if $self->{fill};
   @ret;
}

sub fill
{
   return $_[0]-> {fill} unless $#_;
   return if $_[0]->{fill} == $_[1];
   $_[0]->{fill} = $_[1];
   $_[0]-> repaint;
}

sub outline
{
   return $_[0]-> {outline} unless $#_;
   return if $_[0]->{outline} == $_[1];
   $_[0]->{outline} = $_[1];
   $_[0]-> repaint;
}

sub fillBackColor
{
   return $_[0]-> {fillBackColor} unless $#_;
   return if $_[0]->{fillBackColor} == $_[1];
   $_[0]->{fillBackColor} = $_[1];
   $_[0]-> repaint;
}

sub outlineBackColor
{
   return $_[0]-> {outlineBackColor} unless $#_;
   return if $_[0]->{outlineBackColor} == $_[1];
   $_[0]->{outlineBackColor} = $_[1];
   $_[0]-> repaint;
}

package Prima::Canvas::Rectangle; 
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledOutlined);

sub on_paint
{
   my ( $self, $canvas, $width, $height) = @_;
   if ( $self-> {fill}) {
      $canvas-> color( $self-> {backColor});
      $canvas-> backColor( $self-> {fillBackColor});
      $canvas-> bar( 0, 0, $width - 1, $height - 1);
   }
   if ( $self-> {outline}) {
      my $lw1 = int(($self-> {lineWidth} || 1) / 2);
      my $lw2 = int((($self-> {lineWidth} || 1) - 1) / 2) + 1;
      $canvas-> color( $self-> {color});
      $canvas-> backColor( $self-> {outlineBackColor});
      $canvas-> rectangle( $lw1, $lw1, $width - $lw2, $height - $lw2);
   }
}

package Prima::Canvas::Ellipse;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledOutlined);

sub on_paint
{
   my ( $self, $canvas, $width, $height) = @_;
   my ( $cx, $cy) = (int(($width - 1) / 2), int(($height - 1)/ 2));
   if ( $self-> {fill}) {
      $canvas-> color( $self-> {backColor});
      $canvas-> backColor( $self-> {fillBackColor});
      $canvas-> fill_ellipse( $cx, $cy, $width, $height);
   }
   if ( $self-> {outline}) {
      my $lw = ($self-> {lineWidth} || 1) - 1;
      $canvas-> color( $self-> {color});
      $canvas-> backColor( $self-> {outlineBackColor});
      $canvas-> ellipse( $cx, $cy, $width - $lw, $height - $lw);
   }
}

package Prima::Canvas::Arc;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::Outlined);

sub profile_default 
{
   $_[0]-> SUPER::profile_default,
   start => 0,
   end   => 90,
}

sub on_paint
{
   my ( $self, $canvas, $width, $height) = @_;
   my ( $cx, $cy) = (int(($width - 1) / 2), int(($height - 1)/ 2));
   my $lw = ($self-> {lineWidth} || 1) - 1;
   $canvas-> arc( $cx, $cy, $width - $lw, $height - $lw, $self->{start}, $self->{end});
}

package Prima::Canvas::FilledArc;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledOutlined);

sub profile_default 
{
   $_[0]-> SUPER::profile_default,
   start => 0,
   end   => 90,
   mode  => 'chord',
}

sub on_paint
{
   my ( $self, $canvas, $width, $height) = @_;
   my ( $cx, $cy) = (int(($width - 1) / 2), int(($height - 1)/ 2));
   my $mode1 = ($self->{mode} eq 'chord') ? 'chord' : 'sector';
   my $mode2 = ($self->{mode} eq 'chord') ? 'fill_chord' : 'fill_sector';
   if ( $self-> {fill}) {
      $canvas-> color( $self-> {backColor});
      $canvas-> backColor( $self-> {fillBackColor});
      $canvas-> $mode2( $cx, $cy, $width, $height, $self->{start}, $self->{end});
   }
   if ( $self-> {outline}) {
      my $lw = ($self-> {lineWidth} || 1) - 1;
      $canvas-> color( $self-> {color});
      $canvas-> backColor( $self-> {outlineBackColor});
      $canvas-> $mode1( $cx, $cy, $width - $lw, $height - $lw, $self->{start}, $self->{end});
   }
}

package Prima::Canvas::Chord;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledArc);

package Prima::Canvas::Sector;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledArc);

sub profile_default 
{
   $_[0]-> SUPER::profile_default,
   mode  => 'sector',
}

package Prima::Canvas::Image;
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub profile_default 
{
   $_[0]-> SUPER::profile_default,
   image  => undef,
}

sub uses { 'rop' }

sub on_paint
{
   my ( $self, $canvas, $width, $height) = @_;
   my $i = $self-> {image};
   unless ( defined $i) {
      my @save = $canvas-> get( qw(color fillPattern));
      $canvas-> set(
         color       => cl::Gray,
	 fillPattern => fp::BkSlash,
      );
      $canvas-> bar( 0,0,$width-1,$height-1);
      $canvas-> set( @save);
   } else {
   #$canvas-> put_image_indirect( $i, 0,0, 0,0, $width, $height, $width, $height+10, rop::CopyPut);
   $canvas-> stretch_image( 0,0, $width, $height, $i);
   }
}

package main;

use Prima qw(ColorDialog);

my ( $colordialog, $logo );

$logo = Prima::StdBitmap-> icon(0);

my $w = Prima::MainWindow-> create(
  text => 'Canvas demo',
  menuItems => [
     ['~Object' => [
        (map { [ $_  => "~$_" => \&insert] } 
	   qw(Rectangle Ellipse Arc Chord Sector Image)),
	[],
	[ '~Delete' => 'Del' , kb::Delete , \&delete]
     ]],
     ['~Edit' => [
        ['color' => '~Foreground color' => \&set_color],
        ['backColor' => '~Background color' => \&set_color],
	['~Line width' => [ map { [ "lw$_", $_, \&set_line_width ] } 0..7, 10, 15 ]],
	['Line ~pattern' => [ map { [ "lp:linePattern=$_", $_, \&set_constant ] } 
	    sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %lp:: ]],
	['Line ~end' => [ map { [ "le:lineEnd=$_", $_, \&set_constant ] } 
	    sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %le:: ]],
	['Fill ~pattern' => [ map { [ "fp:fillPattern=$_", $_, \&set_constant ] } 
	    sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %fp:: ]],
	['~Rop' => [ map { [ "rop:rop=$_", $_, \&set_constant ] } 
	    sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %rop:: ]],
	['Rop~2' => [ map { [ "rop:rop2=$_", $_, \&set_constant ] } 
	    sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %rop:: ]],
	[],
	['fill' => 'Toggle ~fill' => \&toggle],
	['outline' => 'Toggle ~outline' => \&toggle],
     ]],
     ['~View' => [
        ['zoom+' =>  'Zoom in' => '+' => '+' => \&zoom],
        ['zoom-' =>  'Zoom out' => '-' => '-' => \&zoom],
        ['zoom0' =>  'Zoom 100%' => 'Ctrl+1' => '^1' => \&zoom],
	[],
	['Align ~horizontally' => [
	   map { [ "alignment=$_", $_, \&align ]} qw(Left Center Right)
	]],
	['Align ~vertically' => [
	   map { [ "valignment=$_", $_, \&align ]} qw(Top Center Bottom)
	]],
     ]],
  ],
);

my $c = $w-> insert( 'Prima::CanvasEdit' =>
   origin => [0,0],
   size   => [$w-> size],
   growMode => gm::Client,
   paneSize => [ 500, 500],
   hScroll => 1,
   vScroll => 1,
   name    => 'Canvas',
   buffered => 1,
);

sub insert
{
   my ( $self, $obj) = @_;
   my %profile;
   $profile{image} = $logo if $obj eq 'Image';
   $c-> selected_object( $c-> insert_object( "Prima::Canvas::$obj", %profile));
}

sub delete
{
   my $obj;
   return unless $obj = $_[0]-> Canvas-> selected_object;
   $_[0]-> Canvas-> delete_object( $obj);
}

sub set_color
{
   my ( $self, $property) = @_;
   my $obj;
   return unless $obj = $self-> Canvas-> selected_object;
   $colordialog = Prima::ColorDialog-> create unless $colordialog;
   $colordialog-> value( $obj-> $property);
   $obj-> $property( $colordialog-> value) if $colordialog-> execute != mb::Cancel;
}

sub set_line_width
{
   my ( $self, $lw) = @_;
   my $obj;
   return unless $obj = $self-> Canvas-> selected_object;
   $lw =~ s/^lw//;
   $obj-> lineWidth( $lw);
}

sub set_constant
{
   my ( $self, $cc) = @_;
   my $obj;
   return unless $obj = $self-> Canvas-> selected_object;
   return unless $cc =~ /^(\w+)\:(\w+)\=(.*)$/;
   $obj-> $2( eval "$1::$3");
}

sub toggle
{
   my ( $self, $property) = @_;
   my $obj;
   return unless $obj = $self-> Canvas-> selected_object;
   return unless $obj-> can( $property);
   $obj-> $property( !$obj-> $property());
}

sub zoom
{
   my ( $self, $zoom) = @_;
   $zoom =~ s/^zoom//;
   my $c = $self-> Canvas;
   if ( $zoom eq '+') {
      $c-> zoom( $c-> zoom * 1.1);
   } elsif ( $zoom eq '-') {
      $c-> zoom( $c-> zoom * 0.9);
   } elsif ( $zoom eq '0') {
      $c-> zoom( 1);
   }
}

sub align
{
   my ( $self, $align) = @_;
   my $c = $self-> Canvas;
   $align =~ m/([^\=]+)\=(.*)$/;
   $c-> $1( eval "ta::$2");
}

$c-> insert_object( 'Prima::Canvas::Rectangle', linePattern => lp::Solid, lineWidth => 10);
$c-> insert_object( 'Prima::Canvas::Image', image => $logo, origin => [ 50, 50]);

run Prima;
