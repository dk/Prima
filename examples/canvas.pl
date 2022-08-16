use strict;
use warnings;

use Prima qw(Widget::ScrollWidget);
# A widget with two scrollbars. Contains set of objects, that know
# how to draw themselves. The graphic objects hierarchy starts
# from Prima::CanvasObject:: class

package Prima::Canvas;
use vars qw(@ISA);
@ISA = qw(Prima::Widget::ScrollWidget);

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
	$self-> {zoom} = 1;
	$self-> {$_} = 0 for qw(paneWidth paneHeight alignment valignment);
	$self-> {objects} = [];
	%profile = $self-> SUPER::init(%profile);
	$self-> $_($profile{$_}) for qw(zoom paneWidth paneHeight alignment valignment);
	return %profile;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	$canvas-> clear;
	my $zoom = $self-> {zoom};
	my @c = $canvas-> clipRect;
	my %props;
	my %defaults = map { $_ => $canvas-> $_() } @Prima::CanvasObject::uses;
	for my $obj ( @{$self-> {objects}}) {
		my @r = $self-> object2screen( $obj-> rect, $obj-> inner_rect);
		$r[$_]-- for 2,3;
		next if !$obj-> visible ||
			$r[0] > $c[2] || $r[1] > $c[3] ||
			$r[2] < $c[0] || $r[3] < $c[1];

		my @uses = $obj-> uses;
		delete @props{@uses};
		my $f = $obj-> font;
		$canvas-> set(
			(map { $_ => $obj-> $_()   } @uses),
			(map { $_ => $defaults{$_} } keys %props)
		);
		%props = map { $_ => 1 } @uses;

		$canvas-> fillPatternOffset( @r[0,1] );
		$canvas-> translate( @r[4,5]);
		$canvas-> clipRect( @r[0..3]);
		$obj-> on_paint( $canvas, $r[6]-$r[4], $r[7]-$r[5]);
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
	$self-> propagate_event( nt::Command, 'on_keydown', $code, $key, $mod, $repeat);
}

sub on_keyup
{
	my ( $self, $code, $key, $mod) = @_;
	$self-> propagate_event( nt::Command, 'on_keyup', $code, $key, $mod);
}

sub delete_object
{
	my ( $self, $obj) = ( shift, shift);
	@{$self-> {objects}} = grep { $_ != $obj } @{$self-> {objects}};
	$self-> {selection} = undef
		if $self-> {selection} && $self-> {selection} == $obj;
	my @r = $self-> object2screen( $obj-> rect);
	$self-> invalidate_rect( @r) if $obj-> visible;
}

sub insert_object
{
	my ( $self, $class) = ( shift, shift);
	return $class-> new( @_, owner => $self );
}

sub attach_object
{
	push @{$_[0]-> {objects}}, $_[1];
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
	$d[$_] -= $self-> {indents}-> [$_] for 0,1;
	my $zoom = $self-> {zoom};
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
	my $zoom = $self-> {zoom};
	my @ret;
	$d[$_] -= $self-> {indents}-> [$_] for 0,1;
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
	for my $obj ( reverse @{$self-> {objects}}) {
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
	my ( $self, $flow, $event, @params) = @_;
	$self-> push_event;
	my $stop = $flow & nt::SMASK;
	for (
		( $flow & nt::FluxReverse) ?
		$self-> objects :
		reverse $self-> objects
	) {
		$_-> $event( @params);
		last if
		( $stop == nt::Single) ||
		( $stop == nt::Event && !$self-> eventFlag);
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
	return $_[0]-> {alignment} unless $#_;
	$_[0]-> {alignment} = $_[1];
	$_[0]-> repaint;
}

sub valignment
{
	return $_[0]-> {valignment} unless $#_;
	$_[0]-> {valignment} = $_[1];
	$_[0]-> repaint;
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
	return if $ph == $self-> {paneHeight} && $pw == $self-> {paneWidth};
	$self-> {paneWidth}  = $pw;
	$self-> {paneHeight} = $ph;
	$self-> reset_zoom;
	$self-> repaint;
}

sub zoom
{
	return $_[0]-> {zoom} unless $#_;
	my ( $self, $zoom) = @_;
	return if $zoom == $self-> {zoom};
	$self-> {zoom} = $zoom;
	$self-> reset_zoom;
	$self-> reset_layout;
	$self-> repaint;
}

sub set_deltas
{
	my $self = shift;
	$self-> SUPER::set_deltas(@_);
	$self-> reset_layout;
}

sub reset_layout
{
	$_[0]-> propagate_event( nt::Notification, 'on_layoutchanged');
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

sub objects {@{$_[0]-> {objects}}}

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
	@r = $self-> object2screen($self-> {selection}-> rect);
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
			$self-> focused_object( $found = $self-> {transaction} = $obj);
			$self-> capture(1, $self);
		}
	}
	$self-> focused_object(undef) if $self-> {selection} && !$found;
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
		$x -= $self-> {anchor}-> [0];
		$y -= $self-> {anchor}-> [1];
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
		my $new = $self-> focused_object;
		if ( $key == kb::Tab) {
			$new = $self-> zorder( $new, $new ? 'prev' : 'last');
			$new = $self-> zorder( undef, 'last') unless $new;
		} else {
			$new = $self-> zorder( $new, $new ? 'next' : 'first');
			$new = $self-> zorder( undef, 'first') unless $new;
		}
		if ( $new) {
			$self-> focused_object( $new);
			$self-> clear_event;
			return;
		}
	}

	if ( $key == kb::Left || $key == kb::Right || $key == kb::Up || $key == kb::Down) {
		my $obj = $self-> focused_object;
		if ( $obj) {
			my ( $dx, $dy) = (0,0);
			if ( $key == kb::Left) {
				$dx = -5;
			} elsif ( $key == kb::Right) {
				$dx = +5;
			} elsif ( $key == kb::Down) {
				$dy = -5;
			} elsif ( $key == kb::Up) {
				$dy = +5;
			}
			my @sz = $obj-> size;
			$sz[0] += $dx;
			$sz[1] += $dy;
			$sz[0] = 5 if $sz[0] < 5;
			$sz[1] = 5 if $sz[1] < 5;
			$obj-> size( @sz);
		}
	}

	$self-> SUPER::on_keydown( $code, $key, $mod, $repeat);
}

sub focused_object
{
	return $_[0]-> {selection} unless $#_;
	return if $_[1] && $_[1]-> owner != $_[0];
	$_[0]-> {selection}-> repaint if $_[0]-> {selection};
	$_[0]-> {selection} = $_[1];
	$_[0]-> {selection}-> repaint if $_[0]-> {selection};
}


package Prima::CanvasObject;
use vars qw(%defaults @uses %list_properties);

{
	@uses = qw( backColor color fillPattern font lineEnd linePattern
		lineWidth region rop rop2 textOpaque
		textOutBaseline lineJoin fillMode
		antialias alpha
	);
	my $pd = Prima::Drawable-> profile_default();
	%defaults = map { $_ => $pd-> {$_} } @uses;
	%list_properties = map { $_ => 1 } qw(origin size rect resolution);
}

sub new
{
	my ( $class, %properties) = @_;
	my $self = bless {}, $class;
	$self-> lock;
	$self-> {adjust_in_progress} = 1;
	my %defaults = $self-> profile_default;
	$self-> {$_} = $defaults{$_} for keys %defaults;
	$self-> {font} = {%{$defaults{font}}};
	$self-> {indents} = [0,0,0,0];
	$self-> init( \%defaults, \%properties);
	$self-> set(%properties);
	$self-> on_create;
	delete $self-> {adjust_in_progress};
	$self-> adjust( exists $properties{size} or exists $properties{rect});
	$self-> unlock;
	return $self;
}

sub init
{
	my ( $self, $defaults, $properties) = @_;
}

sub DESTROY { shift-> on_destroy;  }

sub destroy
{
	my $self = $_[0];
	$self-> owner( undef);
}

sub profile_default
{
	%defaults,
	origin     => [ 0, 0],
	size       => [ 100, 100],
	visible    => 1,
	name       => '',
	resolution => [1,1],
	autoAdjust => 1,
}

sub uses
{
	return ();
}

sub set
{
	my $self = shift;
	my $i;
	for ( $i = 0; $i < @_; $i+=2) {
		my ( $prop, $val) = @_[$i,$i+1];
		if ( $list_properties{$prop}) {
			$self-> $prop( @$val);
		} else {
			$self-> $prop( $val);
		}
	}
}

sub clear_event
{
	$_[0]-> {owner}-> clear_event if $_[0]-> {owner};
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

sub on_adjust_data
{
	my ( $self, $x, $y) = @_;
}

sub on_adjust_size
{
	my ( $self) = @_;
}

sub on_layoutchanged
{
	my ( $self) = @_;
}

sub on_zorderchanged
{
	my ( $self) = @_;
}

sub on_paint
{
	my ( $self, $canvas, $width, $heigth) = @_;
}

sub on_render
{
	my ($self) = @_;
}

sub repaint
{
	delete $_[0]-> {_update} if $_[0]-> {_update};
	$_[0]-> _update( $_[0]-> origin, $_[0]-> size);
}

sub invalidate_rect
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	my @o = $self-> origin;
	$self-> _update( $o[0] + $x1, $o[1] + $y1, $x2 - $x1 + 1, $y2 - $y1 + 1);
}

sub resolution
{
	return @{$_[0]-> {resolution}} unless $#_;
	my ( $self, $x, $y) = @_;
	return if $x == $self-> {resolution}-> [0] && $y == $self-> {resolution}-> [1];
	$self-> {resolution} = [$x, $y];
	$self-> on_render();
}

sub _begin_update
{
	my $self = $_[0];
	return if !$self-> {visible} || $self-> {_lock_update};
	$self-> {_update} = [];
}

sub _update
{
	my ( $self, $x, $y, $w, $h) = @_;
	return unless $self-> {visible};
	my $auto = ! $self-> {_update};
	push @{$self-> {_update}}, $x, $y, $x + $w, $y + $h;
	$self-> _end_update if $auto && !$self-> {_lock_update};
}

sub _end_update
{
	my $self = $_[0];
	return if !$self-> {visible} || $self-> {_lock_update} || !$self-> {_update} || !$self-> {owner};
	my $o = $self-> {owner};
	my @o = $o-> object2screen( @{$self-> {_update}});
	my $i;
	for ($i = 0; $i < @o; $i+=4) {
		$o-> invalidate_rect( @o[$i..$i+3]);
	}
	delete $self-> {_update};
}

sub name { $#_ ? $_[0]-> {name} = $_[1] : $_[0]-> {name} }

sub lock { $_[0]-> {_lock_update}++ }

sub unlock
{
	return unless $_[0]-> {_lock_update};
	$_[0]-> _end_update unless --$_[0]-> {_lock_update};
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
		$_[0]-> {origin}-> [0]
}

sub bottom
{
	$#_ ?
		$_[0]-> origin( $_[0]-> {origin}-> [0], $_[1]) :
		$_[0]-> {origin}-> [1]
}

sub right
{
	$#_ ?
		$_[0]-> size( $_[1] - $_[0]-> {origin}-> [0], $_[0]-> {size}-> [1]) :
		$_[0]-> {origin}-> [0] + $_[0]-> {size}-> [0]
}

sub top
{
	$#_ ?
		$_[0]-> size( $_[1] - $_[0]-> {origin}-> [0], $_[0]-> {size}-> [1]) :
		$_[0]-> {origin}-> [0] + $_[0]-> {size}-> [0]
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
		my @o = @{$_[0]-> {origin}};
		my @s = @{$_[0]-> {size}};
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
	return @{$_[0]-> {origin}} unless $#_;
	my ( $self, $x, $y) = @_;
	return if $x == $self-> {origin}-> [0] and $y == $self-> {origin}-> [1];
	my @o = @{$self-> {origin}};
	$self-> _begin_update;
	$self-> _update( @{$self-> {origin}}, @{$self-> {size}});
	@{$self-> {origin}} = ( $x, $y);
	$self-> _update( @{$self-> {origin}}, @{$self-> {size}});
	$self-> on_move( @o, $x, $y);
	$self-> _end_update;
}

sub size
{
	return @{$_[0]-> {size}} unless $#_;
	my ( $self, $x, $y) = @_;
	$x = 0 if $x < 0;
	$y = 0 if $y < 0;
	return if $x == $self-> {size}-> [0] and $y == $self-> {size}-> [1];
	my @s = @{$self-> {size}};
	$self-> _begin_update;
	$self-> _update( @{$self-> {origin}}, @{$self-> {size}});
	@{$self-> {size}} = ( $x, $y);
	$self-> _update( @{$self-> {origin}}, @{$self-> {size}});
	$self-> adjust( 1) unless $self-> {adjust_flag};
	$self-> on_size( @s, $x, $y);
	$self-> _end_update;
}

sub inner_size
{
	return map {
		$_[0]-> {size}-> [$_] - $_[0]-> {indents}-> [$_] - $_[0]-> {indents}-> [$_+2]
	} 0, 1 unless $#_;
	my ( $self, $x, $y) = @_;
	$x += $self-> {indents}-> [0] + $self-> {indents}-> [2];
	$y += $self-> {indents}-> [1] + $self-> {indents}-> [3];
	my $adjust_flag = $self-> {adjust_flag};
	$self-> {adjust_flag} = 1;
	$self-> size( $x, $y);
	$self-> {adjust_flag} = $adjust_flag;
}

sub inner_rect
{
	return
		$_[0]-> {origin}-> [0] + $_[0]-> {indents}-> [0],
		$_[0]-> {origin}-> [1] + $_[0]-> {indents}-> [1],
		$_[0]-> {origin}-> [0] + $_[0]-> {size}-> [0] - $_[0]-> {indents}-> [2],
		$_[0]-> {origin}-> [1] + $_[0]-> {size}-> [1] - $_[0]-> {indents}-> [3],
		unless $#_;
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	$x1 -= $self-> {indents}-> [0];
	$y1 -= $self-> {indents}-> [1];
	$x2 += $self-> {indents}-> [2];
	$y2 += $self-> {indents}-> [3];
	my $adjust_flag = $self-> {adjust_flag};
	$self-> {adjust_flag} = 1;
	$self-> rect( $x1, $y1, $x2, $y2);
	$self-> {adjust_flag} = $adjust_flag;
}

sub indents
{
	return @{$_[0]-> {indents}} unless $#_;
	my ( $self, @indents) = @_;
	@indents = @{$indents[0]} unless $#indents;
	$self-> origin(
		$self-> {origin}-> [0] + $self-> {indents}-> [0] - $indents[0],
		$self-> {origin}-> [1] + $self-> {indents}-> [1] - $indents[1]
	);
	@{$self-> {indents}} = @indents;
}

sub adjust
{
	my ( $self, $data_from_size) = @_;
	return if $self-> {adjust_in_progress} or !$self-> {autoAdjust};
	$self-> {adjust_in_progress} = 1;
	$self-> lock;
	$data_from_size ?
		$self-> on_adjust_data(@{$self-> {size}}) :
		$self-> on_adjust_size();
	$self-> unlock;
	delete $self-> {adjust_in_progress};
}

sub autoAdjust
{
	return $_[0]-> {autoAdjust} unless $#_;
	$_[0]-> {autoAdjust} = $_[1];
}


sub bring_to_front { $_[0]-> {owner}-> zorder( $_[0], 'front') if $_[0]-> {owner} }
sub send_to_back   { $_[0]-> {owner}-> zorder( $_[0], 'back')  if $_[0]-> {owner} }
sub insert_behind  { $_[0]-> {owner}-> zorder( $_[0], $_[1])   if $_[0]-> {owner} }
sub first          { $_[0]-> {owner}-> zorder( $_[0], 'first') if $_[0]-> {owner} }
sub last           { $_[0]-> {owner}-> zorder( $_[0], 'last')  if $_[0]-> {owner} }
sub next           { $_[0]-> {owner}-> zorder( $_[0], 'next')  if $_[0]-> {owner} }
sub prev           { $_[0]-> {owner}-> zorder( $_[0], 'prev')  if $_[0]-> {owner} }

sub visible
{
	return $_[0]-> {visible} unless $#_;
	return if $_[0]-> {visible} == $_[1];
	$_[0]-> {visible} = $_[1];
	$_[0]-> {owner}-> invalidate_rect( $_[0]-> owner-> object2screen( $_[0]-> rect))
		if $_[0]-> {owner};
}

sub alpha
{
	return $_[0]-> {alpha} unless $#_;
	$_[0]-> {alpha} = $_[1];
	$_[0]-> repaint;
}

sub antialias
{
	return $_[0]-> {antialias} unless $#_;
	$_[0]-> {antialias} = $_[1];
	$_[0]-> repaint;
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
	my ( $self, $font) = @_;
	for ( keys %$font) {
		$self-> {font}-> {$_} = $font-> {$_};
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

sub fillMode
{
	return $_[0]-> {fillMode} unless $#_;
	$_[0]-> {fillMode} = $_[1];
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

sub uses { return qw( rop rop2 backColor color lineWidth linePattern lineEnd antialias alpha ); }

package Prima::Canvas::Filled;
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub uses { return qw( rop rop2 color backColor fillPattern lineEnd antialias alpha ); }

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
	my @ret = qw(rop rop2 color backColor antialias alpha);
	push @ret, qw(lineWidth linePattern lineEnd) if $self-> {outline};
	push @ret, qw(fillPattern) if $self-> {fill};
	@ret;
}

sub fill
{
	return $_[0]-> {fill} unless $#_;
	return if $_[0]-> {fill} == $_[1];
	$_[0]-> {fill} = $_[1];
	$_[0]-> repaint;
}

sub outline
{
	return $_[0]-> {outline} unless $#_;
	return if $_[0]-> {outline} == $_[1];
	$_[0]-> {outline} = $_[1];
	$_[0]-> repaint;
}

sub fillBackColor
{
	return $_[0]-> {fillBackColor} unless $#_;
	return if $_[0]-> {fillBackColor} == $_[1];
	$_[0]-> {fillBackColor} = $_[1];
	$_[0]-> repaint;
}

sub outlineBackColor
{
	return $_[0]-> {outlineBackColor} unless $#_;
	return if $_[0]-> {outlineBackColor} == $_[1];
	$_[0]-> {outlineBackColor} = $_[1];
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

package Prima::Canvas::arc_properties;

sub start
{
	return $_[0]-> {start} unless $#_;
	$_[0]-> {start} = $_[1];
	$_[0]-> repaint;
}

sub end
{
	return $_[0]-> {end} unless $#_;
	$_[0]-> {end} = $_[1];
	$_[0]-> repaint;
}


package Prima::Canvas::Arc;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::Outlined Prima::Canvas::arc_properties);

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
	$canvas-> arc( $cx, $cy, $width - $lw, $height - $lw, $self-> {start}, $self-> {end});
}

package Prima::Canvas::FilledArc;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledOutlined Prima::Canvas::arc_properties);

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
	my $mode1 = ($self-> {mode} eq 'chord') ? 'chord' : 'sector';
	my $mode2 = ($self-> {mode} eq 'chord') ? 'fill_chord' : 'fill_sector';
	if ( $self-> {fill}) {
		$canvas-> color( $self-> {backColor});
		$canvas-> backColor( $self-> {fillBackColor});
		$canvas-> $mode2( $cx, $cy, $width, $height, $self-> {start}, $self-> {end});
	}
	if ( $self-> {outline}) {
		my $lw = ($self-> {lineWidth} || 1) - 1;
		$canvas-> color( $self-> {color});
		$canvas-> backColor( $self-> {outlineBackColor});
		$canvas-> $mode1( $cx, $cy, $width - $lw, $height - $lw, $self-> {start}, $self-> {end});
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

package Prima::Canvas::line_properties;

sub points
{
	return $_[0]-> {points} unless $#_;
	my $self = shift;
	my $p = ( defined($_[0]) && ref($_[0]) eq 'ARRAY') ? $_[0] : \@_;
	die "Number of points is not multiple of 2" if @$p % 2;
	push @$p, @$p[0,1]
		if $self-> {fix_last_point} && ( $$p[0] != $$p[-2] || $$p[1] != $$p[1]);
	$self-> {points} = $p;
	$self-> adjust;
}

sub zoom_points
{
	my ( $self, $w, $h) = @_;
	my ( $x, $y) = $self-> inner_size;
	return [] if $w < 1 || $h < 1 || $x < 1 || $y < 1;
	unless ( defined $self-> {cosa}) {
		my $a = $self-> {rotate} / 57.295779;
		$self-> {cosa} = cos( $a);
		$self-> {sina} = sin( $a);
	}
	my ( $cos, $sin) = ( $self-> {cosa}, $self-> {sina});
	my @anchor = @{$self-> {anchor}};
	my @aspect = @{$self-> {aspect}};
	my @shift  = @{$self-> {shift}};
	my @offset = ($self-> {offset} && $self-> {autoAdjust}) ? @{$self-> {offset}} : (0,0);
	$x /= $w;
	$y /= $h;
	$h = $self-> {points};
	my @ret;
	for ( $w = 0; $w < @$h; $w += 2) {
		my $X = $$h[$w]    - $anchor[0] + $shift[0];
		my $Y = $$h[$w+1]  - $anchor[1] + $shift[1];
		my $A = ($X * $cos - $Y * $sin);
		my $B = ($X * $sin + $Y * $cos);
		$A = ( $A + $anchor[0]) * $aspect[0] + $offset[0];
		$B = ( $B + $anchor[1]) * $aspect[1] + $offset[1];
		push @ret, $A / $x;
		push @ret, $B / $y;
	}
	\@ret;
}

sub extents
{
	my ( $self, $points) = @_;
	my $p;
	if ( $points) {
		$p = $points;
	} else {
		local $self-> {offset};
		$p = $self-> zoom_points( $self-> inner_size);
	}
	my $lw = int(($self-> lineWidth || 1) / 2);
	return -$lw,-$lw,$lw,$lw if 0 == @$p;
	my $i;
	my @r = @$p[0,1,0,1];
	for ( $i = 2; $i < @$p; $i += 2) {
		$r[0] = $$p[$i] if $r[0] > $$p[$i];
		$r[1] = $$p[$i+1] if $r[1] > $$p[$i+1];
		$r[2] = $$p[$i] if $r[2] < $$p[$i];
		$r[3] = $$p[$i+1] if $r[3] < $$p[$i+1];
	}
	$r[$_] -= $lw, $r[$_+2] += $lw for 0,1;
	return @r;
}

sub anchor
{
	return @{$_[0]-> {anchor}} unless $#_;
	$_[0]-> {anchor} = [($#_ == 1) ? @{$_[1]} : @_[1,2]];
	$_[0]-> adjust;
}

sub aspect
{
	return @{$_[0]-> {aspect}} unless $#_;
	$_[0]-> {aspect} = [(($#_ == 1) ? @{$_[1]} : @_[1,2])];
	$_[0]-> adjust;
}

sub shift
{
	return @{$_[0]-> {shift}} unless $#_;
	$_[0]-> {shift} = [($#_ == 1) ? @{$_[1]} : @_[1,2]];
	$_[0]-> adjust;
}

sub smooth
{
	return $_[0]-> {smooth} unless $#_;
	$_[0]-> {smooth} = $_[1];
	$_[0]-> repaint;
}

sub rotate
{
	return $_[0]-> {rotate} unless $#_;
	my ( $self, $angle) = @_;
	$angle += 360 while $angle < 0;
	$angle %= 360;
	return if $self-> {rotate} == $angle;
	$self-> {rotate} = $angle;
	delete $self-> {sina};
	delete $self-> {cosa};
	$self-> adjust;
}

package Prima::Canvas::Line;
use vars qw(@ISA %arrowheads);
@ISA = qw(Prima::Canvas::Outlined Prima::Canvas::line_properties);

%arrowheads = (
	feather   => [1,0, -1,-1,-0.5,-0.7,-0.15,-0.4, 0,0, -0.15, 0.4, -0.5,0.7,-1,1, 1,0],
	default   => [1,0, -1,-1, -1,1, 1,0],
	flying    => [1,0, -1,-1, 0,0, -1,1, 1,0],
	square    => [0.5,0, 0,-0.5, -0.5,-0.5, 0, 0, -0.5, 0.5, 0,0.5, 0.5,0],
);

sub profile_default
{
	$_[0]-> SUPER::profile_default,
	anchor => [0,0],
	aspect => [1,1],
	shift  => [0,0],
	arrows => [undef,undef],
	points => [],
	smooth => 0,
	rotate => 0,
}

sub uses
{
	my $self = $_[0];
	my @ret = $self-> SUPER::uses;
	push @ret, 'lineJoin';
	@ret;
}

sub arrows
{
	return @{$_[0]-> {arrows}} unless $#_;
	my $self = $_[0];
	$self-> lock;
	my @arrows = ($#_ == 1) ? @{$_[1]} : @_[1,2];
	$self-> arrow( $_, $arrows[$_]) for 0, 1;
	$self-> unlock;
}

sub arrow
{
	return $_[0]-> {arrows}-> [$_[1]] if $#_ == 1;
	my ( $self, $idx, $arrow) = @_;
	return if $idx < 0 || $idx > 1;
	my $mul;
	if ( defined ($arrow) && (!ref($arrow) || ref($arrow) eq 'ARRAY')) {
		unless (ref($arrow)) {
			if ( $arrow =~ /^([^\:]*)\:(\-?[\d\.]+)$/) {
				( $arrow,$mul) = ($1,$2);
				goto ASPECT if !length $arrow && $self-> {arrows}-> [$idx];
			}
			$arrow = exists ($arrowheads{$arrow}) ?
			$arrowheads{$arrow} :
			$arrowheads{default};
		}
		if ( defined $self-> {arrows}-> [$idx] && $self-> {arrows}-> [$idx]-> isa('Prima::Canvas::Polygon')) {
			$self-> {arrows}-> [$idx]-> points( $arrow);
		} else {
			$self-> {arrows}-> [$idx] = Prima::Canvas::Polygon-> new(
				points => $arrow,
				fill   => 1,
				outline => 0,
			);
		}
	ASPECT:
		$self-> {arrows}-> [$idx]-> aspect( $mul, $mul) if defined $mul;
	} else {
		$self-> {arrows}-> [$idx] = $arrow;
	}
	$self-> {arrows}-> [$idx]-> autoAdjust( 0) if $self-> {arrows}-> [$idx];
	$self-> adjust;
}

sub on_adjust_size
{
	my ( $self) = @_;
	delete $self-> {offset};
	my $p = $self-> zoom_points( $self-> inner_size);

	my @inner = $self-> extents( $p);
	$inner[$_+2] -= $inner[$_] for 0,1;
	my @delta = @inner[0,1];
	$self-> {offset} = [map {-1*$_} @delta];
	@inner[0,1] = (0,0);
	my @outer = @inner;

	my $flip = 0;
	my $lw = ($self-> {lineWidth} || 1);
	for ( 0..1) {
		my ( $x1, $y1, $x2, $y2) = @$p[ $flip++ ? (2,3,0,1) : (-4..-1)];
		next unless $_ = $self-> {arrows}-> [$_];
		$_-> rotate( atan2($y2 - $y1, $x2 - $x1) * 57.295779);
		my @r = map { $_ * $lw } $_-> extents;
		my @arrow_box = ( $x2 + $r[0] - $delta[0], $y2 + $r[1] - $delta[1],
								$x2 + $r[2] - $delta[0], $y2 + $r[3] - $delta[1]);
		for ( 0,1) {
			$outer[$_] = $arrow_box[$_] if $outer[$_] > $arrow_box[$_];
			$outer[$_+2] = $arrow_box[$_+2] if $outer[$_+2] < $arrow_box[$_+2];
		}
	}
	$self-> indents(
		$inner[0] - $outer[0],
		$inner[1] - $outer[1],
		$outer[2] - $inner[2],
		$outer[3] - $inner[3],
	);
	$self-> inner_size( @inner[2,3]);
}

sub on_adjust_data
{
	my ( $self, $x, $y) = @_;
}

sub on_paint
{
	my ( $self, $canvas, $width, $height) = @_;
	my $lw = ($self-> {lineWidth} || 1);
	my @size  = $self-> inner_size;
	my $p = $self-> zoom_points( $width, $height);
	return if 4 > @$p;
	$canvas-> lineWidth( $self-> lineWidth * $width / int $size[0]);
	$self-> {smooth} ?
		$canvas-> spline( $p) :
		$canvas-> polyline( $p);
	my $flip = 0;
	my @t = $canvas-> translate;
	for my $arrow ( @{$self-> {arrows}}) {
		my ( $x1, $y1, $x2, $y2) = @$p[ $flip++ ? (2,3,0,1) : (-4..-1)];
		next unless $arrow;
		my @asize = $arrow-> size;
		$canvas-> translate( $t[0] + $x2, $t[1] + $y2);
		$arrow-> set(
			rotate    => atan2($y2 - $y1, $x2 - $x1) * 57.295779,
			backColor => $canvas-> color,
		);
		$arrow-> on_paint( $canvas,
			$lw * $width * $asize[0] / int $size[0],
			$lw * $height * $asize[1] / int $size[1]);
	}
}

sub lineWidth
{
	return $_[0]-> SUPER::lineWidth unless $#_;
	my $self = shift;
	$self-> SUPER::lineWidth(@_);
	$self-> adjust;
}

package Prima::Canvas::Polygon;
use vars qw(@ISA);
@ISA = qw(Prima::Canvas::FilledOutlined Prima::Canvas::line_properties);

sub profile_default
{
	$_[0]-> SUPER::profile_default,
	anchor => [0,0],
	aspect => [1,1],
	shift  => [0,0],
	points => [],
	smooth => 0,
	rotate => 0,
	fix_last_point => 1,
}

sub uses
{
	my $self = $_[0];
	my @ret = $self-> SUPER::uses;
	push @ret, 'lineJoin' if $self-> {outline};
	push @ret, 'fillMode' if $self-> {fill};
	@ret;
}

sub on_paint
{
	my ( $self, $canvas, $width, $height) = @_;
	my $p = $self-> zoom_points( $width, $height);
	return unless @$p;
	if ( $self-> {fill}) {
		$canvas-> color( $self-> {backColor});
		$canvas-> backColor( $self-> {fillBackColor});
		$self-> {smooth} ?
			$canvas-> fill_spline( $p) :
			$canvas-> fillpoly( $p);
	}
	if ( $self-> {outline}) {
		$canvas-> lineWidth( $self-> lineWidth * $width / $self-> width);
		$canvas-> color( $self-> {color});
		$canvas-> backColor( $self-> {outlineBackColor});
		$self-> {smooth} ?
			$canvas-> spline( $p) :
			$canvas-> polyline( $p);
	}
}

sub lineWidth
{
	return $_[0]-> SUPER::lineWidth unless $#_;
	my $self = shift;
	$self-> SUPER::lineWidth(@_);
	$self-> adjust;
}

package Prima::Canvas::Image;
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub profile_default
{
	$_[0]-> SUPER::profile_default,
	image  => undef,
}

sub uses
{
	my $i = $_[0]-> {image};
	my @ret;
	if ( $i) {
		push @ret, 'rop';
		push @ret, qw(color backColor) if
			$i-> isa('Prima::DeviceBitmap') && $i-> type == dbt::Bitmap;
	}
	@ret;
}

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
		$canvas-> stretch_image( 0,0, $width, $height, $i);
	}
}

sub image
{
	return $_[0]-> {image} unless $#_;
	$_[0]-> {image} = $_[1];
	$_[0]-> repaint;
}

package Prima::Canvas::Text;
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub profile_default
{
	$_[0]-> SUPER::profile_default,
	text       => '',
	flags      => dt::Default|dt::DrawSingleChar|dt::DrawPartial,
	tab        => 8,
	textOpaque => 0,
}

sub uses
{
	my $self = $_[0];
	my @ret = qw(font color rop alpha);
	push @ret, qw(backColor textOpaque) if $self-> {textOpaque};
	@ret;
}

sub on_paint
{
	my ( $self, $canvas, $width, $height) = @_;
	$canvas-> draw_text( $self-> {text}, 0, 0,
		$width-1, $height-1, $self-> {flags}, $self-> {tab});
}

sub text
{
	return $_[0]-> {text} unless $#_;
	$_[0]-> {text} = $_[1];
	$_[0]-> repaint;
}

sub flags
{
	return $_[0]-> {flags} unless $#_;
	$_[0]-> {flags} = $_[1];
	$_[0]-> repaint;
}

sub tab
{
	return $_[0]-> {tab} unless $#_;
	$_[0]-> {tab} = $_[1];
	$_[0]-> repaint;
}

package Prima::Canvas::Widget;
use vars qw(@ISA);
@ISA = qw(Prima::CanvasObject);

sub profile_default
{
	$_[0]-> SUPER::profile_default,
	widget     => undef,
	scalable   => 1,
}

sub init
{
	my ( $self, $defaults, $properties) = @_;
	$self-> {base_size} = [0,0];
	if ( !exists $properties-> {size} && !exists $properties-> {rect} &&
			defined $properties-> {widget}) {
		$properties-> {size} = [$properties-> {widget}-> size];
	}
	if ( !exists $properties-> {origin} && !exists $properties-> {rect} &&
			defined $properties-> {widget}) {
		$properties-> {origin} = [$properties-> {widget}-> origin];
	}
}

sub on_destroy
{
	return unless $_[0]-> {widget};
	$_[0]-> {widget}-> destroy;
}

sub destroy
{
	my $self = $_[0];
	if ( $self-> {widget}) {
		$self-> {widget}-> destroy;
		$self-> {widget} = undef;
	}
	$self-> SUPER::destroy;
}

sub scalable
{
	return $_[0]-> {scalable} unless $#_;
	$_[0]-> {scalable} = $_[1];
}

sub instance { $_[1]-> {__PRIMA__CANVAS__OBJECT__}}

sub widget
{
	return $_[0]-> {widget} unless $#_;
	my ( $self, $widget) = @_;
	return unless $self-> {widget} = $widget;
	$widget-> {__PRIMA__CANVAS__OBJECT__} = $self;
	my @sz = $widget-> size;
	if ( $self-> {owner}) {
		$widget-> owner( $self-> {owner});
		$widget-> send_to_back;
	} else {
		$widget-> visible(0);
		$widget-> owner( $::application);
	}
	$self-> {base_size} = \@sz;
	$self-> on_layoutchanged;
}

sub visible
{
	return $_[0]-> SUPER::visible unless $#_;
	$_[0]-> SUPER::visible( $_[1]);
	$_[0]-> {widget}-> visible( $_[1])
		if $_[0]-> {widget} && $_[0]-> {owner};
}

sub owner
{
	return $_[0]-> SUPER::owner unless $#_;
	my ( $self, $owner) = @_;
	$self-> SUPER::owner( $owner);
	return unless $self-> {widget};
	if ( $owner) {
		$self-> {widget}-> owner( $owner);
		$self-> {widget}-> visible( 1) if $self-> {visible};
		$self-> {widget}-> send_to_back;
		$self-> on_layoutchanged;
	} else {
		$self-> {widget}-> owner( $::application);
		$self-> {widget}-> visible( 0);
	}
}

sub on_size { $_[0]-> on_layoutchanged }
sub on_move { $_[0]-> on_layoutchanged }

sub on_layoutchanged
{
	my $self = $_[0];
	return unless $self-> {widget} && $self-> {owner};
	my @r = $self-> {owner}-> object2screen( $self-> rect);
	if ( $self-> {scalable}) {
		$self-> {widget}-> rect(@r);
	} else {
		$self-> {widget}-> origin(@r[0,1]);
	}
}

package main;

use Prima qw(Application StdBitmap Dialog::ColorDialog Dialog::FontDialog Buttons);

my ( $colordialog, $logo, $bitmap, $fontdialog);

$logo = Prima::StdBitmap::icon(0);
( $bitmap, undef) = $logo-> split;
$bitmap-> set( conversion => ict::None, type => im::BW);
$bitmap = $bitmap-> bitmap;

my $w = Prima::MainWindow-> create(
text => 'Canvas demo',
menuItems => [
	['~Object' => [
		(map { [ $_  => "~$_" => \&insert_from_menu] }
			qw(Rectangle Ellipse Arc Chord Sector Image Bitmap Line Polygon Text Button InputLine)),
		[],
		[ '~Delete' => 'Del' , kb::Delete , \&delete]
	]],
	['~Edit' => [
		['color' => '~Foreground color' => \&set_color],
		['backColor' => '~Background color' => \&set_color],
		['alpha' => '~Alpha' => [ map { [ $_, $_, \&set_alpha ] } 0, 32, 64, 96, 128, 164, 192, 224, 255]],
		[],
		['~Line width' => [ map { [ "lw$_", $_, \&set_line_width ] } 0..7, 10, 15 ]],
		['Line ~pattern' => [ map { [ "lp:linePattern=$_", $_, \&set_constant ] }
				sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %lp:: ]],
		['Line ~end' => [ map { [ "le:lineEnd=$_", $_, \&set_constant ] }
				sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %le:: ]],
		['Line ~join' => [ map { [ "lj:lineJoin=$_", $_, \&set_constant ] }
				sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %lj:: ]],
		['Fill ~pattern' => [
				[ Icon   => Icon   => \&set_fill_pattern ],
				[ Bitmap => Bitmap => \&set_fill_pattern ],
				[],
				map { [ "fp:fillPattern=$_", $_, \&set_fill_pattern ] }
					sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %fp::
		]],
		['~Rop' => [ map { [ "rop:rop=$_", $_, \&set_constant ] } qw(
				CopyPut
				XorPut
				AndPut
				OrPut
				NotPut
				Invert
				Blackness
				NotDestAnd
				NotDestOr
				Whiteness
				NotSrcAnd
				NotSrcOr
				NotXor
				NotAnd
				NotOr
				NoOper
		)]],
		['Rop~2' => [ map { [ "rop:rop2=$_", $_, \&set_constant ] } qw(
				CopyPut
				NoOper
		)]],
		['Fill r~ule' => [ map { [ "fm:fillMode=$_", $_, \&set_constant ] }
				sort grep { !m/AUTOLOAD|constant|BEGIN|END/ } keys %fm:: ]],
		[],
		['antialias' => 'Antialia~s' => \&toggle],
		['fill' => 'Toggle ~fill' => \&toggle],
		['outline' => 'Toggle ~outline' => \&toggle],
		[],
		['Arc,Chord,Sector' => [
			['arc-' => 'Rotate ~right' => \&arc_rotate],
			['arc+' => 'Rotate ~left' => \&arc_rotate],
			['arc++' => 'E~xtend' => \&arc_rotate],
			['arc--' => '~Shrink' => \&arc_rotate],
		]],
		['Line,Polygon' => [
			['smooth1' => '~Spline' => \&smooth],
			['smooth0' => '~Straigth' => \&smooth],
			['rotate-' => 'Rotate ~right' => \&line_rotate],
			['rotate+' => 'Rotate ~left' => \&line_rotate],
			[],
			['Set ~arrows' => [
				map {["arrow=$_", ucfirst, \&set_arrowhead]} 'none', keys %Prima::Canvas::Line::arrowheads,
			]],
			['Set arrowhead ~size' => [
				map {["arrow=$_", $_, \&set_arrowhead]} 1,2,3,4,5
			]],
		]],
		['Filled shapes' => [
			['fillBackColor'    => '~Fill back color' => \&set_color],
			['outlineBackColor' => '~Outline back color' => \&set_color],
		]],
		['Te~xt' => [
			['font' => '~Font' => \&set_font],
			[],
			['textOpaque1' => '~Opaque' => \&set_text_opaque],
			['textOpaque0' => '~Transparent' => \&set_text_opaque],
			[],
			(map { [ "dt:$_:".(dt::Left|dt::Right|dt::Center), $_, \&set_text_flags ]}
				qw(Left Right Center) ),
			[],
			(map { [ "dt:$_:".(dt::Top|dt::Bottom|dt::VCenter), $_, \&set_text_flags ]}
				qw(Top Bottom VCenter)),
			[],
			(map { [ "dt:$_", $_, \&set_text_flags ]}
				qw(DrawPartial NewLineBreak WordBreak ExpandTabs UseExternalLeading))
		]],
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
	alignment => ta::Center,
	valignment => ta::Middle,
);

my $widget_popup =
[
	[ '~Move' => sub {
		my ( $self, $obj, $owner);
		return unless $obj = Prima::Canvas::Widget-> instance( $self = $_[0]);
		return unless $owner = $obj-> owner;
		my @pp = $owner-> object2screen(
			$obj-> left + $obj-> width / 2,
			$obj-> bottom + $obj-> height / 2);
		$owner-> pointerPos( @pp);
		$owner-> mouse_down( mb::Left, 0, @pp, 1);
	}],
	[ '~Delete' => sub {
		return unless $_ = Prima::Canvas::Widget-> instance( $_[0]);
		$_-> destroy;
	}],
];

sub insert
{
	my ( $self, $obj, %profile) = @_;
	$profile{image} = $logo if $obj eq 'Image';
	$profile{image} = $bitmap, $obj = 'Image' if $obj eq 'Bitmap';
	if ( $obj eq 'Line') {
		$profile{points} = [ 10,10,10,50,50,40,100,0,50,60,90,90];
		$profile{shift}  = [ 50,50];
		$profile{arrows} = [ 'feather:2','feather:-2'];
		$profile{size}   = [ 200,200];
		$profile{anchor} = [ 50,50];
		$profile{lineEnd} = le::Flat;
		$profile{lineWidth} = 3,
		$profile{smooth}  = 1;
	}
	if ( $obj eq 'Polygon') {
		$profile{points} = [ 20,0,50,100,80,0,0,65,100,65];
		$profile{anchor} = [50,50];
	}
	if ( $obj eq 'Button') {
		$profile{widget} = Prima::Button-> create( owner => $c);
		$obj = 'Widget';
	}
	if ( $obj eq 'InputLine') {
		$profile{widget} = Prima::InputLine-> create( owner => $c);
		$profile{scalable} = 0;
		$obj = 'Widget';
	}
	if ( $obj eq 'Widget') {
		$profile{widget}-> popupItems( $widget_popup);
	}
	$profile{text} = "use Prima qw(Application);\nMainWindow-> create();\nrun Prima;"
		if $obj eq 'Text';
	$c-> focused_object( $c-> insert_object( "Prima::Canvas::$obj", %profile));
}

sub insert_from_menu
{
	my ( $self, $obj ) = @_;
	insert($self, $obj);
}

sub delete
{
	my $obj;
	return unless $obj = $_[0]-> Canvas-> focused_object;
	$_[0]-> Canvas-> delete_object( $obj);
}

sub set_alpha
{
	my ( $self, $alpha) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	$obj-> alpha( $alpha);
}

sub set_color
{
	my ( $self, $property) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	return unless $obj->can($property);
	$colordialog = Prima::Dialog::ColorDialog-> create unless $colordialog;
	$colordialog-> value( $obj-> $property());
	$obj-> $property( $colordialog-> value) if $colordialog-> execute != mb::Cancel;
}

sub set_font
{
	my ( $self, $property) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	$fontdialog = Prima::Dialog::FontDialog-> create unless $fontdialog;
	$fontdialog-> logFont( $obj-> font);
	$obj-> font( $fontdialog-> logFont) if $fontdialog-> execute != mb::Cancel;
}

sub set_fill_pattern
{
	my ( $self, $fp) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;

	if ( $fp eq 'Icon') {
		$obj-> fillPattern($logo);
	} elsif ( $fp eq 'Bitmap') {
		$obj-> fillPattern($bitmap->image);
	} else {
		return unless $fp =~ /^(\w+)\:(\w+)\=(.*)$/;
		$obj-> $2( eval "$1::$3");
	}
}

sub set_line_width
{
	my ( $self, $lw) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	$lw =~ s/^lw//;
	$obj-> lineWidth( $lw);
}

sub set_constant
{
	my ( $self, $cc) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	return unless $cc =~ /^(\w+)\:(\w+)\=(.*)$/;
	$obj-> $2( eval "$1::$3");
}

sub toggle
{
	my ( $self, $property) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
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

sub arc_rotate
{
	my ( $self, $arc) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	return unless $obj-> isa('Prima::Canvas::Arc') || $obj-> isa('Prima::Canvas::FilledArc');
	$arc =~ s/^arc//;
	if ( $arc eq '+') {
		$obj-> start( $obj-> start + 22.5);
		$obj-> end( $obj-> end + 22.5);
	} elsif ( $arc eq '-') {
		$obj-> start( $obj-> start - 22.5);
		$obj-> end( $obj-> end - 22.5);
	} elsif ( $arc eq '++') {
		$obj-> end( $obj-> end + 22.5);
	} elsif ( $arc eq '--') {
		$obj-> end( $obj-> end - 22.5);
	}
}

sub line_rotate
{
	my ( $self, $line) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	return unless $obj-> isa('Prima::Canvas::line_properties');
	$line =~ s/^rotate//;
	if ( $line eq '+') {
		$obj-> rotate( $obj-> rotate + 10);
	} elsif ( $line eq '-') {
		$obj-> rotate( $obj-> rotate - 10);
	}
}

sub set_arrowhead
{
	my ( $self, $arrow) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	return unless $obj-> isa('Prima::Canvas::Line');
	$arrow =~ s/^arrow\=//;
	if ( $arrow =~ /^\d+$/) {
		for ( $obj-> arrows) {
			$_-> aspect( $arrow, $arrow) if $_;
		}
		$obj-> adjust;
		$obj-> repaint;
	} else {
		$arrow = undef if $arrow eq 'none';
		$obj-> arrows( $arrow, $arrow);
	}
}

sub smooth
{
	my ( $self, $smooth) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	return unless $obj-> can('smooth');
	$smooth =~ s/^smooth//;
	$obj-> smooth( $smooth);
}

sub set_text_opaque
{
	my ( $self, $o) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	$o =~ s/^textOpaque//;
	$obj-> textOpaque( $o);
}

sub set_text_flags
{
	my ( $self, $flags) = @_;
	my $obj;
	return unless $obj = $self-> Canvas-> focused_object;
	return unless $obj-> isa('Prima::Canvas::Text');
	my @f = split(':', $flags);
	$flags = $obj-> flags;
	$f[1] = eval "dt::$f[1]";
	if ( 2 == @f) {
		$flags = (( $flags & $f[1]) ?
			$flags & ~$f[1] :
			$flags | $f[1]
		);
	} elsif ( 3 == @f) {
		$flags &= ~($f[2]+0);
		$flags |= $f[1];
	}
	$obj-> flags( $flags);
}

insert( $c, 'Button', origin => [ 0, 0]);
insert( $c, 'Rectangle', linePattern => lp::DotDot, lineWidth => 10, origin => [ 50, 50], fillPattern => $logo);
insert( $c, 'Line', origin => [ 200, 200], antialias => 1);
insert( $c, 'Polygon', origin => [ 150, 150]);
insert( $c, 'Bitmap', origin => [ 350, 350], backColor => cl::LightGreen, color => cl::Green);

run Prima;
