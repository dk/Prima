# $Id$
use strict;
use Prima qw(Application ScrollWidget);

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
   $self->{$_} = 0 for qw(paneWidth paneHeight);
   $self->{objects} = [];
   %profile = $self-> SUPER::init(%profile);
   $self-> $_($profile{$_}) for qw(zoom paneWidth paneHeight);
   return %profile;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   $canvas-> clear;
   my @d = $self-> deltas;
   my @i = $self-> indents;
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
      $obj-> on_paint( $canvas, @r[2,3]);
   }
   $canvas-> translate(0,0);
   $canvas-> clipRect(@c);
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

sub reset_zoom
{
   my ( $self ) = @_;
   $self-> limits( 
      $self-> {paneWidth} * $self-> {zoom},
      $self-> {paneHeight} * $self-> {zoom}
   );
}

sub screen2object
{
   my $self = $_[0];
   my $i;
   my @d = $self-> deltas;
   my @i = $self-> indents;
   my $zoom = $self->{zoom};
   my @ret;
   for ( $i = 1; $i <= $#_; $i+=2) {
      push @ret, int(($_[$i]   + $d[0] - $i[0])/$zoom + .5);
      push @ret, int(($_[$i+1] - $d[1] - $i[1])/$zoom + .5) if defined $_[$i+1];
   }
   return @ret;
}

sub object2screen
{
   my $self = $_[0];
   my $i;
   my @d = $self-> deltas;
   my @i = $self-> indents;
   my $zoom = $self->{zoom};
   my @ret;
   for ( $i = 1; $i <= $#_; $i+=2) {
      push @ret, -$d[0] + $_[$i]   * $zoom + $i[0];
      push @ret, +$d[1] + $_[$i+1] * $zoom + $i[1] if defined $_[$i+1];
   }
   return @ret;
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

sub position2object
{
   my ( $self, $x, $y, $skip_hittest) = @_;
   my ( $nx, $ny) = $self-> screen2object( $x, $y);
   $self-> push_event;
   for my $obj ( @{$self->{objects}}) {
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

sub insert_object
{
   my ( $self, $class) = ( shift, shift);
   push @{$self->{objects}}, $class-> new(
      @_,
      owner => $self,
   );
   $self->{objects}->[-1];
}

sub delete_object
{
   my ( $self, $obj) = ( shift, shift);
   @{$self->{objects}} = grep { $_ != $obj } @{$self->{objects}};
   my @r = $self-> object2screen( $obj-> rect);
   $self-> invalidate_rect( @r) if $obj-> visible;
}

sub zorder
{
   my ( $self, $obj, $command) = @_;
   my $robj = grep { $_ == $obj } @{$self->{objects}};
   return unless $robj;
   if ( $command eq 'front') {
      @{$self->{objects}} = grep { $_ != $obj } @{$self->{objects}}; 
      push @{$self->{objects}}, $obj;
   } elsif ( $command eq 'back') {
      @{$self->{objects}} = grep { $_ != $obj } @{$self->{objects}}; 
      unshift @{$self->{objects}}, $obj;
   } else {
      my $i;
      my @o = grep { $_ != $obj } @{$self->{objects}}; 
      return if @o == @{$self->{objects}};
      @{$self->{objects}} = @o;
      for ( $i = 0; $i < @{$self->{objects}}; $i++) {
         next unless $self->{objects}->[$i] != $command;
	 splice @{$self->{objects}}, $i, 0, $obj;
	 last;
      }
   }
   $obj-> repaint;
}

package Prima::CanvasEdit;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas);

sub on_paint
{
   my ( $self, $canvas) = @_;
   $self-> SUPER::on_paint( $canvas);
   return unless $self-> {selection};
   my @r = $self-> object2screen($self->{selection}-> rect);
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
      $x -= $self-> {anchor}->[0];
      $y -= $self-> {anchor}->[1];
      my @o = $self-> screen2object( $x, $y);
      $self-> {transaction}-> origin( $self-> screen2object( $x, $y));
   }
   $self-> SUPER::on_mousemove( $mod, $x, $y);
}

sub selected_object
{
   return $_[0]-> {selection} unless $#_;
   return if $_[1] && $_[1]-> owner != $_[0];
   $_[0]-> {selection}-> repaint if $_[0]-> {selection};
   $_[0]-> {selection} = $_[1];
   $_[0]-> zorder( $_[0]-> {selection}, 'front') if $_[0]-> {selection};
}


package Prima::CanvasObject;
use vars qw(%defaults @uses);

{
   @uses = qw( backColor color fillPattern font lineEnd linePattern
               lineWidth region rop rop2 splinePrecision textOpaque 
	       textOutBaseline);
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

sub lock
{
   $_[0]->{_lock_update}++;
}

sub unlock
{
   return unless $_[0]->{_lock_update};
   $_[0]-> _end_update unless --$_[0]->{_lock_update};
}

sub owner
{
   return $_[0]-> {owner};
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
      $canvas-> bar( 0, 0, $width, $height);
   }
   if ( $self-> {outline}) {
      $canvas-> color( $self-> {color});
      $canvas-> backColor( $self-> {outlineBackColor});
      my $lw = int($self-> {lineWidth} / 2);
      $canvas-> rectangle( $lw, $lw, $width - $lw, $height - $lw);
   }
}

package Prima::Canvas::Ellipse;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledOutlined);

sub on_paint
{
   my ( $self, $canvas, $width, $height) = @_;
   if ( $self-> {fill}) {
      $canvas-> color( $self-> {backColor});
      $canvas-> backColor( $self-> {fillBackColor});
      $canvas-> fill_ellipse( $width / 2, $height / 2, $width, $height);
   }
   if ( $self-> {outline}) {
      $canvas-> color( $self-> {color});
      $canvas-> backColor( $self-> {outlineBackColor});
      $canvas-> ellipse( $width / 2, $height / 2, 
         $width - $self-> {lineWidth} + 1, $height - $self-> {lineWidth} + 1);
   }
}

package main;

use Prima qw(ColorDialog);

my ( $colordialog );

my $w = Prima::MainWindow-> create(
  menuItems => [
     ['~Object' => [
        ['Rectangle' => '~Rectangle' => \&insert],
        ['Ellipse' => '~Ellipse' => \&insert],
	[],
	[ '~Delete' => 'Del' , kb::Delete , \&delete]
     ]],
     ['~Edit' => [
        ['color' => '~Foreground color' => \&set_color],
        ['backColor' => '~Background color' => \&set_color],
	['~Line width' => [ map { [ "lw$_", $_, \&set_line_width ] } 1..5, 7, 10, 15 ]],
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
        ['zoom0' =>  'Zoom in' => 'Ctrl+1' => '^1' => \&zoom],
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
   $c-> selected_object( $c-> insert_object( "Prima::Canvas::$obj"));
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

$c-> insert_object( 'Prima::Canvas::Rectangle');

run Prima;
