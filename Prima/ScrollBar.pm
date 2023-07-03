#
#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk>
#  Modifications by:
#     Anton Berezin  <tobez@tobez.org>
#  Documentation by:
#     Anton Berezin  <tobez@tobez.org>
#
package Prima::ScrollBar;
use vars qw(@ISA @stdMetrics);
use strict;
use warnings;
use Prima;
use base qw(
	Prima::Widget
	Prima::Widget::Fader
	Prima::Widget::MouseScroller
);

@stdMetrics = Prima::Application-> get_default_scrollbar_metrics;

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		autoTrack     => 1,
		dndAware      => 1,
		growMode      => gm::GrowHiX,
		height        => $stdMetrics[1],
		min           => 0,
		minThumbSize  => 21 * $::application->uiScaling,
		max           => 100,
		pageStep      => 10,
		partial       => 10,
		selectable    => 0,
		step          => 1,
		tabStop       => 0,
		value         => 0,
		vertical      => 0,
		widgetClass   => wc::ScrollBar,
		whole         => 100,
	}
}

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	Track => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	my $vertical = exists $p-> {vertical} ? $p-> {vertical} : $default-> { vertical};
	if ( $vertical)
	{
		$p-> { width}    = $stdMetrics[0] unless exists $p-> { width};
		$p-> { growMode} = gm::GrowLoX | gm::GrowHiY if !exists $p-> { growMode};
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	for ( qw( b1 b2 tab left right)) { $self-> { $_}-> { pressed} = 0; }
	for ( qw( b1 b2 tab)) { $self-> { $_}-> { enabled} = 0; }
	for ( qw( autoTrack minThumbSize vertical min max step pageStep whole partial value ))
		{ $self-> {$_} = 1; }
	for ( qw( autoTrack minThumbSize vertical min max step pageStep whole partial value ))
		{ $self-> $_( $profile{ $_}); }
	$self-> {suppressNotify} = undef;
	$self-> reset;
	return %profile;
}


sub set
{
	my ( $self, %profile) = @_;
	if ( exists $profile{partial} and exists $profile{whole}) {
		$self-> set_proportion($profile{partial}, $profile{whole});
		delete $profile{partial};
		delete $profile{whole};
	}
	if ( exists $profile{min} or exists $profile{max}) {
		my ( $min, $max) = (
			exists($profile{min}) ? $profile{min} : $self-> {min},
			exists($profile{max}) ? $profile{max} : $self-> {max},
		);
		$self-> set_bounds($min,$max);
		delete $profile{min};
		delete $profile{max};
	}
	if ( exists $profile{step} and exists $profile{pageStep}) {
		$self-> set_steps($profile{step}, $profile{pageStep});
		delete $profile{step};
		delete $profile{pageStep};
	}
	$self-> SUPER::set( %profile);
}

sub on_size
{
	$_[0]-> reset;
}

sub draw_pad
{
	my ( $self, $canvas, $part, $base_color, $skin) = @_;

	if ( $skin eq 'flat') {
		my $hbc = $self-> hiliteBackColor;
		my $clr;
		if ( $self->{$part}->{pressed}) {
			$clr = $hbc;
		} elsif ( $self->enabled) {
			my $pct = 0.33;
			$pct *= 1 + ( $self-> fader_current_value // 1 )
				if ( $self->{prelight} // '') eq $part;
			$clr = cl::blend(
				$self->map_color( $base_color ),
				$self->map_color( $hbc ),
				$pct
			);
		} else {
			$clr = $base_color;
		}
		$canvas-> color( $clr );
		$canvas-> bar( @{$self-> {$part}-> {rect}} );
	} else {
		if ( defined $self->{prelight} && $self->{prelight} eq $part && $self->enabled ) {
			$base_color = $self-> prelight_color($base_color, 1 + (.25 * ($self->fader_current_value // 0)));
		}
		my @palette = ( $base_color, $self->dark3DColor );
		my @spline  = ( 1, 0 );
		unless ($self-> { vertical } ) {
			@palette = reverse @palette;
			@spline  = reverse @spline;
		}
		$self-> rect_bevel( $canvas, @{$self-> {$part}-> {rect}},
			width   => 2,
			concave => $self->{$part}->{pressed},
			fill    => $canvas->new_gradient(
				palette   => \@palette,
				spline    => \@spline,
				vertical  => $self->{vertical},
			),
		);
	}
}

sub fix_triangle
{
	my ( $v, $spot ) = @_;
	if ( $v ) {
		my $d = $$spot[4] - $$spot[2];
		if ($d % 2) {
			$$spot[0] = $$spot[2] + ($d - 1) / 2;
			$$spot[2]--;
		}
	} else {
		my $d = $$spot[3] - $$spot[5];
		if ($d % 2) {
			$$spot[1] = $$spot[5] + ($d - 1) / 2;
			$$spot[5]--;
		}
	}
}


sub on_paint
{
	my ($self,$canvas) = @_;
	my $skin = $self->skin;
	my @clr;
	my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
	if ( $self-> enabled) {
		@clr = ($self-> color, $self-> backColor);
		$clr[0] = $self->hiliteColor if $skin eq 'flat';
	} else {
		@clr = ($self-> disabledColor, $self-> disabledBackColor);
	}
	my @size = $canvas-> size;
	my $btx  = $self-> { btx};
	my $v    = $self-> { vertical};
	my ( $maxx, $maxy) = ( $size[0]-1, $size[1]-1);

	if ( $skin ne 'flat') {
		$canvas-> color( $c3d[0]);
		$canvas-> line( 0, 0, $maxx - 1, 0);
		$canvas-> line( $maxx, 0, $maxx, $maxy - 1);
		$canvas-> color( $c3d[1]);
		$canvas-> line( 0, $maxy, $maxx, $maxy);
		$canvas-> line( 0, 0, 0, $maxy);
	}

	my $collapsed = ($v ? $maxy : $maxx) < $self->{minThumbSize};

	unless ( $collapsed ) {
		$self-> draw_pad( $canvas, b1 => $clr[1], $skin);
		$canvas-> color( $self-> {b1}-> { enabled} ? $clr[ 0] : $self-> disabledColor);
		my $a = $self-> { b1}-> { pressed} ? 1 : 0;
		my @spot = map { int($_ + .5) } $v ? (
			$maxx * 0.5 + $a, $maxy - $btx * 0.40 - $a,
			$maxx * 0.3 + $a, $maxy - $btx * 0.55 - $a,
			$maxx * 0.7 + $a, $maxy - $btx * 0.55 - $a,
		) : (
			$btx * 0.40 + $a, $maxy * 0.5 - $a,
			$btx * 0.55 + $a, $maxy * 0.7 - $a,
			$btx * 0.55 + $a, $maxy * 0.3 - $a
		);
		fix_triangle($v, \@spot);
		$canvas-> fillpoly( [ @spot])
			if $maxx > 10 && $maxy > 10;
	}

	unless ( $collapsed ) {
		$self-> draw_pad( $canvas, b2 => $clr[1], $skin);
		$canvas-> color( $self-> {b2}-> { enabled} ? $clr[ 0] : $self-> disabledColor);
		my $a = $self-> { b2}-> { pressed} ? 1 : 0;
		my @spot = map { int($_ + .5) } $v ? (
			$maxx * 0.5 + $a, $btx * 0.40 - $a,
			$maxx * 0.3 + $a, $btx * 0.55 - $a,
			$maxx * 0.7 + $a, $btx * 0.55 - $a
		) : (
			$maxx - $btx * 0.40 + $a, $maxy * 0.5 - $a,
			$maxx - $btx * 0.55 + $a, $maxy * 0.7 - $a,
			$maxx - $btx * 0.55 + $a, $maxy * 0.3 - $a
		);
		fix_triangle($v, \@spot);
		$canvas-> fillpoly( [ @spot])
			if $maxx > 10 && $maxy > 10;
		$canvas-> color( $clr[ 1]);
	}

	if ( $self-> { tab}-> { enabled})
	{
		my @rect = @{$self-> { tab}-> { rect}};
		my $lenx = $self-> { tab}-> { q(length)};

		for ( qw( left right))
		{
			if ( defined $self-> { $_}-> {rect})
			{
				my @r = @{$self-> {$_}-> {rect}};
				if ( $skin ne 'flat') {
					$canvas-> color( $c3d[1]);
					$v ? ( $canvas-> line( $r[0]-1, $r[1], $r[0]-1, $r[3])):
						( $canvas-> line( $r[0], $r[3]+1, $r[2], $r[3]+1));
				}
				$canvas-> color( $self-> {$_}-> {pressed} ? $clr[0] : $clr[1]);
				if ( $skin eq 'xp') {
					$canvas-> backColor( $c3d[0]);
					$canvas-> rop2(rop::CopyPut);
					$canvas-> fillPattern([(0xAA,0x55) x 4]);
					$canvas-> bar( @r);
					$canvas-> fillPattern(fp::Solid);
					$canvas-> rop2(rop::NoOper);
				} else {
					$canvas-> bar( @r);
				}
			}
		}

		$self-> draw_pad( $canvas, tab => $clr[1], $skin);
		if (
			$self-> {minThumbSize} > 8 &&
			$skin eq 'classic' &&
			(( $v ? $maxx : $maxy) > 10)
		) {
			$canvas-> color( $clr[ 0]);
			$canvas-> backColor( $c3d[ 0]);
			if ( $v)
			{
				my $sty = $rect[1] +
					int($lenx / 2) -
					4 -
					( $self-> { tab} -> { pressed} ? 1 : 0);
				my $stx = int($maxx / 3) + ( $self-> { tab} -> { pressed} ? 1 : 0);
				my $lnx = int($maxx / 3);
				$lnx += $maxx - $lnx * 3;
				$canvas-> rop2(rop::CopyPut);
				$canvas-> fillPattern([(0xff, 0) x 4]);
				$canvas-> fillPatternOffset($stx, $sty);
				$canvas-> bar( $stx, $sty - 1, $stx + $lnx, $sty + 9);
				$canvas-> rop2(rop::NoOper);
			} else {
				my $stx = $rect[0] +
					int($lenx / 2) -
					4 +
					( $self-> { tab}-> { pressed} ? 1 : 0) ;
				my $sty = int($maxy / 3) - ( $self-> { tab} -> { pressed} ? 1 : 0);
				my $lny = int($maxy / 3);
				$lny += $maxy - $lny * 3;
				$canvas-> rop2(rop::CopyPut);
				$canvas-> fillPattern([(0xAA) x 8]);
				$canvas-> fillPatternOffset($stx, $sty);
				$canvas-> bar( $stx - 1, $sty, $stx + 9, $sty + $lny);
				$canvas-> rop2(rop::NoOper);
			}
		}
	} else {
		my @r = $collapsed ? 
			( 0, 0, $maxx, $maxy ) :
			@{$self-> {groove}-> {rect}};
		if ( $skin ne 'flat') {
			$canvas-> color( $c3d[1]);
			$v ?
				( $canvas-> line( $r[0]-1, $r[1], $r[0]-1, $r[3])):
				( $canvas-> line( $r[0], $r[3]+1, $r[2], $r[3]+1));
		}
		$canvas-> color( $clr[1]);

		if ( $skin eq 'xp') {
			$canvas-> backColor( $c3d[1]);
			$canvas-> fillPattern([(0xAA,0x55) x 4]);
			$canvas-> rop2(rop::CopyPut);
			$canvas-> bar( @r);
			$canvas-> fillPattern(fp::Solid);
		} else {
			$canvas-> bar( @r);
		}
	}
}

sub translate_point
{
	my ( $self, $x, $y) = @_;
	return undef if ($self->vertical ? $self->height : $self->width) < $self->{minThumbSize};
	for( qw(b1 b2 tab left right groove))
	{
		next unless defined $self-> {$_}-> {rect};
		my @r = @{$self-> {$_}-> {rect}};
		return $_ if $x >= $r[0] && $y >= $r[1] && $x <= $r[2] && $y <= $r[3];
	}
	return undef;
}

sub draw_part
{
	my ( $self, $who) = @_;
	return unless defined $self-> {$who}-> {rect};

	my @r = @{$self-> {$who}-> {rect}};
	$r[2]++;
	$r[3]++;
	$self-> invalidate_rect( @r);
}

sub ScrollTimer_Tick
{
	my ( $self, $timer) = @_;
	unless ( defined $self-> {mouseTransaction}) {
		$self-> scroll_timer_stop;
		return;
	}

	my $who = $self-> {mouseTransaction};
	if ( $who eq q(b1) || $who eq q(b2)) {
		return unless $self-> {$who}-> {pressed};

		my $oldValue = $self-> {value};
		$self-> {suppressNotify} = 1;
		$self-> value( $oldValue + (( $who eq q(b1))?-1:1)*$self-> {step});
		$self-> {suppressNotify} = undef;
		if ( $oldValue == $self-> {value})
		{
			$self-> {$who}-> {pressed} = 0;
			$self-> draw_part( $who);
			$self-> {mouseTransaction} = undef;
			$self-> capture(0);
			$self-> scroll_timer_stop;
			return;
		}
		$self-> notify(q(Change));
	}

	if ( $who eq q(left) || $who eq q(right)) {
		return unless $self-> {$who}-> {pressed};
		my $upon = $self-> translate_point( $self-> pointerPos);
		if ( $upon ne $who) {
			if ( $self-> {$who}-> {pressed}) {
				$self-> {$who}-> {pressed} = 0;
				$self-> draw_part( $who);
			}
			return;
		}

		my $oldValue = $self-> {value};
		$self-> {suppressNotify} = 1;
		$self-> value( $oldValue + (( $who eq q(left))?-1:1)*$self-> {pageStep});
		$self-> {suppressNotify} = undef;
		if ( $oldValue == $self-> {value}) {
			$self-> {$who}-> {pressed} = 0;
			$self-> {mouseTransaction} = undef;
			$self-> capture(0);
			$self-> scroll_timer_stop;
			return;
		}
		$self-> notify(q(Change));
	}

	if ( exists $timer-> {newRate}) {
		$timer-> timeout( $timer-> {newRate});
		delete $timer-> {newRate};
	}
}

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	my ( $v, $x, $d, $s, $ps) = ($self-> vertical, $self-> value, 0, $self-> step, $self-> pageStep);
	$d = $s  if ( $key == kb::Right && !$v) || ( $key == kb::Down && $v);
	$d = -$s if ( $key == kb::Left  && !$v) || ( $key == kb::Up   && $v);
	$d = $ps  if $key == kb::PgDn && !($mod & km::Ctrl);
	$d = -$ps if $key == kb::PgUp && !($mod & km::Ctrl);
	$d = $self-> max - $x if ( $key == kb::PgDn && $mod & km::Ctrl) || $key == kb::End;
	$d = -$x if ( $key == kb::PgUp && $mod & km::Ctrl) || $key == kb::Home;
	$self-> clear_event, $self-> value( $x + $d) if $d;
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	$self-> clear_event;
	return if $btn != mb::Left;
	return if defined $self-> {mouseTransaction};
	my $who = $self-> translate_point( $x, $y);
	return if !defined $who or $who eq q(groove);
	return if $self->{drop_transaction} and $who !~ /^b\d$/;
	if (( $who eq q(b1) || $who eq q(b2))) {
		if ( $self-> {$who}-> {enabled}) {
			$self-> {$who}-> {pressed} = 1;
			$self-> {mouseTransaction} = $who;
			$self-> capture(1) unless $self->{drop_transaction};
			$self-> {suppressNotify} = 1;
			$self-> value( $self-> {value} + (( $who eq q(b1))?-1:1) * $self-> {step});
			$self-> {suppressNotify} = undef;
			$self-> draw_part($who);
			$self-> notify(q(Change));
			$self-> scroll_timer_start;
		} else {
			$self-> event_error;
		}
		return;
	}
	if (( $who eq q(left) || $who eq q(right))) {
		$self-> {$who}-> {pressed} = 1;
		$self-> {mouseTransaction} = $who;
		$self-> value( $self-> {value} + (( $who eq q(left))?-1:1) * $self-> {pageStep});
		$self-> capture(1) unless $self->{drop_transaction};
		$self-> scroll_timer_start;
		return;
	}
	if (( $who eq q(tab)) && ( $self-> {tab}-> {enabled})) {
		$self-> {$who}-> {pressed} = 1;
		$self-> {mouseTransaction} = $who;
		my @r = @{$self-> {tab}-> {rect}};
		$self-> {$who}-> {aperture} = $self-> { vertical} ? $y - $r[1] : $x - $r[0];
		$self-> draw_part($who);
		$self-> capture(1) unless $self->{drop_transaction};
	}
}

sub on_mouseclick
{
	my $self = shift;
	$self-> clear_event;
	return unless pop;
	$self-> clear_event unless $self-> notify( "MouseDown", @_);
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	$self-> clear_event;
	return unless defined $self-> {mouseTransaction};

	my $who = $self-> {mouseTransaction};
	return if $self->{drop_transaction} and $who !~ /^b\d$/;
	if (
		$who eq q(b1) ||
		$who eq q(b2) ||
		$who eq q(left) ||
		$who eq q(right) ||
		$who eq q(tab)
	) {
		$self-> {$who}-> {pressed} = 0;
		$self-> {mouseTransaction} = undef;
		$self-> capture(0) unless $self->{drop_transaction};
		$self-> repaint;
		$self-> notify(q(Change)) if !$self-> {autoTrack} && $who eq q(tab);
	}
}

sub on_mouseenter
{
	my $self = shift;
	$self-> fader_in_mouse_enter if $self->enabled && $self->skin eq 'flat';
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	$self-> clear_event;

	my $who  = $self-> {mouseTransaction};
	unless (defined $who) {
		my $who = $self-> translate_point( $x, $y);
		my $prelight;
		if ( $self->enabled && (($who // '') =~ /^(b1|b2|tab)$/) && $self-> {$who}-> {enabled}) {
			$prelight = $who;
		} else {
			$prelight = undef;
		}
		if (($prelight // '') ne ($self->{prelight} // '')) {
			$self->{prelight} = $prelight;
			$self->repaint;
		}
		return;
	}

	return if $self->{drop_transaction} and $who !~ /^b\d$/;
	if ( $who eq q(tab)) {
		my @groove = @{$self-> {groove}-> {rect}};
		my @tab    = @{$self-> {tab}-> {rect}};
		my $val    = eval{$self-> {vertical} ?
			$self-> {max} -
				(( $y - $self-> {$who}-> {aperture} - $self-> {btx}) *
				( $self-> {max} - $self-> {min})) /
				($groove[3] - $groove[1] - $tab[3] + $tab[1])
			:
			$self-> {min} +
				(( $x - $self-> {$who}-> {aperture} - $self-> {btx}) *
				( $self-> {max} - $self-> {min})) /
				( $groove[2] - $groove[0] - $tab[2] + $tab[0])};
		if ( defined $val) {
			my $ov = $self-> {value};
			$self-> {suppressNotify} = $self-> {autoTrack} ? undef : 1;
			$self-> set_value( $val);
			$self-> {suppressNotify} = undef;
			$self-> notify(q(Track)) if !$self-> {autoTrack} && $ov != $self-> {value};
		}
	} elsif (
		$who eq q(b1) ||
		$who eq q(b2) ||
		$who eq q(left) ||
		$who eq q(right)
	) {
		my $upon  = $self-> translate_point( $x, $y);
		my $oldPress = $self-> {$who}-> {pressed};
		$self-> {$who}-> {pressed} = ( defined $upon && ( $upon eq $who)) ? 1 : 0;
		my $useRepaint = $self-> {$who}-> {pressed} != $oldPress;
		$self-> repaint if $useRepaint;
	}
}

sub on_mouseleave
{
	my $self = shift;
	if ( $self->skin eq 'flat') {
		$self-> fader_out_mouse_leave( sub { delete $self->{prelight} } );
	} else {
		delete $self->{prelight};
		$self->repaint;
	}
}

sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$z = (abs($z) > 120) ? int($z/120) : (($z > 0) ? 1 : -1);
	$self-> value( $self-> value - $self-> step * $z);
	$self-> clear_event;
}

sub on_dragbegin
{
	my $self = shift;
	$self->{drop_transaction} = [0,0];
}

sub on_dragover
{
	my ($self, $clipboard, $action, $mod, $x, $y, $ref) = @_;
	$ref->{allow} = 0;
	if ( $self-> {mouseTransaction} ) {
		$self->notify(q(MouseMove), 0, $x, $y);
		@{$self->{drop_transaction}} = [$x, $y];
	} else {
		$self->notify(q(MouseDown), mb::Left, 0, $x, $y);
	}
}

sub on_dragend
{
	my ($self, $clipboard, $action, $mod, $x, $y, $ref) = @_;
	$ref->{allow} = 0;
	$self->notify(q(MouseUp), mb::Left, 0, $x, $y);
	undef $self->{drop_transaction};
}

sub reset
{
	my $self = $_[0];

	$self-> { b1} -> { enabled} = $self-> { value} > $self-> { min};
	$self-> { b2} -> { enabled} = $self-> { value} < $self-> { max};
	my $fullDisable = $self-> { partial} == $self-> { whole};
	$self-> { tab}-> { enabled} = ( $self-> { min} != $self-> { max}) && !$fullDisable;
	$self-> { b1}-> { enabled} = 0 if ( $self-> { value} == $self-> { min});
	$self-> { b2}-> { enabled} = 0 if ( $self-> { value} == $self-> { max});

	my $btx  = $self-> { minThumbSize};
	my $v    = $self-> { vertical};
	my @size = $self-> size;
	my ( $maxx, $maxy) = ( $size[0]-1, $size[1]-1);

	if ( $v) {
		$btx = int($size[1] / 2) if $btx * 2 >= $maxy;
	} else {
		$btx = int($size[0] / 2) if $btx * 2 >= $maxx;
	}
	my @rect = $v ?
		( 1, $maxy - $btx + 1, $maxx - 1, $maxy - 1) :
		( 1, 1, $btx - 1, $maxy - 1);
	$self-> { b1}-> { rect} = [ @rect];
	@rect  = $v ?
		( 1, 1, $maxx - 1, $btx - 1) :
		( $maxx - $btx + 1, 1, $maxx - 1, $maxy - 1);
	$self-> { b2}-> { rect} = [ @rect];
	$self-> { btx} = $btx;

	@rect = $v ? (
		2, $btx, $maxx - 1, $maxy - $btx
	) : (
		$btx, 1, $maxx - $btx, $maxy - 2
	);

	$self-> {groove}-> {rect} = [@rect];
	my $startx  = $v ? $size[1]: $size[0];
	my $groovex = $startx - $btx * 2;
	$self-> { tab}-> { enabled} = 0 if $groovex < $self-> {minThumbSize};

	if ( $self-> { tab}-> { enabled}) {
		my $lenx = int( $groovex * $self-> { partial} / $self-> { whole});
		$lenx = $self-> {minThumbSize} if $lenx < $self-> {minThumbSize};
		my $atx  =
			int(( $self-> { value} - $self-> {min}) *
			( $groovex - $lenx) /
			( $self-> { max} - $self-> { min}));
		$atx = $groovex - $lenx if $lenx + $atx > $groovex;
		( $lenx, $atx) = ( $groovex - 1, 0) if $atx < 0;

		@rect = $v ? (
			1, $maxy - $btx - $lenx - $atx, $maxx - 1, $maxy - $btx - $atx
		) : (
			$btx + $atx, 1, $btx + $atx + $lenx, $maxy - 1
		);
		$self-> set(
			cursorPos  => [ $rect[0] + 1, $rect[1] + 1],
			cursorSize => [ $rect[2]-$rect[0]-2, $rect[3]-$rect[1]-2],
		);
		$self-> { tab}-> { rect} = [@rect];
		$self-> { tab}-> { q(length)} = $lenx;

		if ( $v) {
			my @r2 = ( 2, $btx, $maxx - 1, $rect[1] - 1);
			my @r1 = ( 2, $rect[3], $maxx - 1, $maxy - $btx);
			$self-> { left}-> {rect}  = ( $r1[ 1] < $r1[ 3]) ? [ @r1] : undef;
			$self-> { right}-> {rect} = ( $r2[ 1] < $r2[ 3]) ? [ @r2] : undef;
		} else {
			my @r1 = ( $btx, 1, $rect[ 0] - 1, $maxy - 2);
			my @r2 = ( $rect[ 2], 1, $maxx - $btx, $maxy - 2);
			$self-> { left}-> {rect}  = ( $r1[ 0] < $r1[ 2]) ? [ @r1] : undef;
			$self-> { right}-> {rect} = ( $r2[ 0] < $r2[ 2]) ? [ @r2] : undef;
		}
	}

	if ($self->skin eq 'flat') {
		for my $part ( qw(b1 b2 tab left right groove)) {
			my $r = $self->{$part}->{rect} or next;
			if ( $v ) {
				$r->[0]--;
				$r->[2]++;
			} else {
				$r->[1]--;
				$r->[3]++;
			}
		}
		for my $part ( qw(left right groove)) {
			my $r = $self->{$part}->{rect} or next;
			$v ? $r->[0]-- : $r->[3]++;
		}
		if ( $v ) {
			$self->{b1}->{rect}->[3]++;
			$self->{b2}->{rect}->[1]--;
			$self->{left}->{rect}->[1]++ if $self->{left}->{rect};
		} else {
			$self->{b1}->{rect}->[0]--;
			$self->{b2}->{rect}->[2]++;
			$self->{right}->{rect}->[0]++ if $self->{right}->{rect};
		}
	}

	$self-> cursorVisible( $self-> { tab}-> { enabled});
}

sub set_bounds
{
	my ( $self, $min, $max) = @_;
	$max = $min if $max < $min;
	( $self-> { min}, $self-> { max}) = ( $min, $max);

	my $oldValue = $self-> {value};
	$self-> value( $max) if $self-> {value} > $max;
	$self-> value( $min) if $self-> {value} < $min;
	$self-> reset;
	$self-> repaint;
}

sub set_steps
{
	my ( $self, $step, $pStep) = @_;

	$step  = 0 if $step < 0;
	$pStep = 0 if $pStep < 0;
	$pStep = $step if $pStep < $step;
	( $self-> { step}, $self-> { pageStep}) = ( $step, $pStep);
}

sub set_proportion
{
	my ( $self, $partial, $whole) = @_;
	$whole   = 1 if $whole   <= 0;
	$partial = 1 if $partial <= 0;
	$partial = $whole if $partial > $whole;
	return if $partial == $self-> { partial} and $whole == $self-> { whole};

	( $self-> { partial}, $self-> { whole}) = ( $partial, $whole);
	$self-> reset;
	$self-> repaint;
}

sub set_value
{
	my ( $self, $value) = @_;

	my ( $min, $max) = ( $self-> {min}, $self-> {max});
	$value = $min if $value < $min;
	$value = $max if $value > $max;
	$value -= $min;
	$max   -= $min;
	my $div = eval{ int($value / $self-> {step} + 0.5 * (( $value >= 0) ? 1 : -1))} || 0;
	my $lbound = $value == 0 ? 0 : $div * $self-> {step};
	my $rbound = $value == $max ? $max : ( $div + 1) * $self-> {step};
	$value = ( $value >= (( $lbound + $rbound) / 2)) ? $rbound : $lbound;
	$value = 0    if $value < 0;
	$value = $max if $value > $max;
	$value += $min;
	my $oldValue = $self-> {value};
	return if $oldValue == $value;

	my %v;
	$v{b1ok}     = $self-> {b1}-> {enabled}?1:0;
	$v{b2ok}     = $self-> {b2}-> {enabled}?1:0;
	$v{grooveok} = $self-> {tab}-> {enabled}?1:0;
	$v{grooveok} .= join(q(x),@{$self-> {tab}-> {rect}}) if $v{grooveok};
	$self-> { value} = $value;
	$self-> reset;
	$v{b1ok2}     = $self-> {b1}-> {enabled}?1:0;
	$v{b2ok2}     = $self-> {b2}-> {enabled}?1:0;
	$v{grooveok2} = $self-> {tab}-> {enabled}?1:0;
	$v{grooveok2}.= join(q(x),@{$self-> {tab}-> {rect}}) if $v{grooveok2};

	for ( qw( b1 b2 groove)) {
		if (( $v{"${_}ok"} ne $v{"${_}ok2"})) {
			my @r = @{$self-> {$_}-> {rect}};
			$r[0]--; $r[1]--; $r[2]++; $r[3]+=2;
			$self-> invalidate_rect(@r);
		}
	}
	$self-> notify(q(Change)) unless $self-> {suppressNotify};
}

sub set_min_thumb_size
{
	my ( $self, $mts) = @_;
	$mts = 1 if $mts < 0;
	return if $mts == $self-> {minThumbSize};
	$self-> {minThumbSize} = $mts;
	$self-> reset;
	$self-> repaint;
}

sub set_vertical
{
	my ( $self, $vertical) = @_;
	return if $vertical == $self-> { vertical};
	$self-> { vertical} = $vertical;
	$self-> reset;
	$self-> repaint;
}

sub get_default_size
{
	return @stdMetrics;
}

sub skin
{
	return $_[0]->SUPER::skin unless $#_;
	my $self = shift;
	$self->SUPER::skin($_[1]);
	$self->reset if defined $self->{value};
	$self->repaint;
}

sub autoTrack    {($#_)?$_[0]-> {autoTrack} =    $_[1]                  : return $_[0]-> {autoTrack}   }
sub max          {($#_)?$_[0]-> set_bounds($_[0]-> {'min'}, $_[1])      : return $_[0]-> {max};}
sub min          {($#_)?$_[0]-> set_bounds($_[1], $_[0]-> {'max'})      : return $_[0]-> {min};}
sub minThumbSize {($#_)?$_[0]-> set_min_thumb_size($_[1])               : return $_[0]-> {minThumbSize};}
sub pageStep     {($#_)?$_[0]-> set_steps ($_[0]-> {'step'}, $_[1])     : return $_[0]-> {pageStep};}
sub partial      {($#_)?$_[0]-> set_proportion ($_[1], $_[0]-> {'whole'}): return $_[0]-> {partial};}
sub skins        {(shift->SUPER::skins, 'xp')}
sub step         {($#_)?$_[0]-> set_steps ($_[1], $_[0]-> {'pageStep'}) : return $_[0]-> {step};}
sub value        {($#_)?$_[0]-> set_value       ($_[1])                 : return $_[0]-> {value}       }
sub vertical     {($#_)?$_[0]-> set_vertical  ($_[1])                   : return $_[0]-> {vertical}    }
sub whole        {($#_)?$_[0]-> set_proportion($_[0]-> {'partial'},$_[1]): return $_[0]-> {whole};}

1;

=head1 NAME

Prima::ScrollBar - standard scroll bars class

=head1 DESCRIPTION

The class C<Prima::ScrollBar> implements both vertical and horizontal
scrollbars in I<Prima>. This is a purely Perl class without any
C-implemented parts except those inherited from C<Prima::Widget>.

=head1 SYNOPSIS

	use Prima::ScrollBar;

	my $sb = Prima::ScrollBar-> create( owner => $group, %rest_of_profile);
	my $sb = $group-> insert( 'ScrollBar', %rest_of_profile);

	my $isAutoTrack = $sb-> autoTrack;
	$sb-> autoTrack( $yesNo);

	my $val = $sb-> value;
	$sb-> value( $value);

	my $min = $sb-> min;
	my $max = $sb-> max;
	$sb-> min( $min);
	$sb-> max( $max);
	$sb-> set_bounds( $min, $max);

	my $step = $sb-> step;
	my $pageStep = $sb-> pageStep;
	$sb-> step( $step);
	$sb-> pageStep( $pageStep);

	my $partial = $sb-> partial;
	my $whole = $sb-> whole;
	$sb-> partial( $partial);
	$sb-> whole( $whole);
	$sb-> set_proportion( $partial, $whole);

	my $size = $sb-> minThumbSize;
	$sb-> minThumbSize( $size);

	my $isVertical = $sb-> vertical;
	$sb-> vertical( $yesNo);

	my ($width,$height) = $sb-> get_default_size;


=head1 API

=head2 Properties

=over

=item autoTrack BOOLEAN

Tells the widget if it should send
C<Change> notification during mouse tracking events.
Generally it should only be set to 0 on very slow computers.

Default value is 1 (logical true).

=item growMode INTEGER

Default value is gm::GrowHiX, i.e. the scrollbar will try
to maintain the constant distance from its right edge to its
owner's right edge as the owner changes its size.
This is useful for horizontal scrollbars.

=item height INTEGER

Default value is $Prima::ScrollBar::stdMetrics[1], which is an operating
system dependent value determined with a call to
C<Prima::Application-E<gt> get_default_scrollbar_metrics>.  The height is
affected because by default the horizontal C<ScrollBar> will be
created.

=item max INTEGER

Sets the upper limit for C<value>.

Default value: 100.

=item min INTEGER

Sets the lower limit for C<value>.

Default value: 0

=item minThumbSize INTEGER

A minimal thumb breadth in pixels. The thumb cannot have
main dimension lesser than this.

Default value: 21

=item pageStep INTEGER

This determines the increment/decrement to
C<value> during "page"-related operations, for example clicking the mouse
on the strip outside the thumb, or pressing C<PgDn> or C<PgUp>.

Default value: 10

=item partial INTEGER

This tells the scrollbar how many of imaginary
units the thumb should occupy. See C<whole> below.

Default value: 10

=item selectable BOOLEAN

Default value is 0 (logical false).  If set to 1 the widget receives keyboard
focus; when in focus, the thumb is blinking.

=item step INTEGER

This determines the minimal increment/decrement to C<value> during
mouse/keyboard interaction.

Default value is 1.

=item value INTEGER

A basic scrollbar property; reflects the imaginary position between C<min> and
C<max>, which corresponds directly to the position of the thumb.

Default value is 0.

=item vertical BOOLEAN

Determines the main scrollbar style.  Set this to 1 when the scrollbar style is
vertical, 0 - horizontal. The property can be changed at run-time, so the
scrollbars can morph from horizontal to vertical and vice versa.

Default value is 0 (logical false).

=item whole INTEGER

This tells the scrollbar how many of imaginary units correspond to the whole
length of the scrollbar.  This value has nothing in common with C<min> and
C<max>.  You may think of the combination of C<partial> and C<whole> as of the
proportion between the visible size of something (document, for example) and
the whole size of that "something".  Useful to struggle against infamous "bird"
size of Windows scrollbar thumbs.

Default value is 100.

=back

=head2 Methods

=over

=item get_default_size

Returns two integers, the default platform dependant width
of a vertical scrollbar and height of a horizontal scrollbar.

=back

=head2 Events

=over

=item Change

The C<Change> notification is sent whenever the thumb position of scrollbar is
changed, subject to a certain limitations when C<autoTrack> is 0. The
notification conforms the general I<Prima> rule: it is sent when appropriate,
regardless to whether this was a result of user interaction, or a side effect
of some method programmer has called.

=item Track

If C<autoTrack> is 0, called when the user changes the thumb position by the
mouse instead of C<Change>.

=back

=head1 EXAMPLE

	use Prima;
	use Prima::Application name => 'ScrollBar test';
	use Prima::ScrollBar;

	my $w = Prima::Window-> create(
		text => 'ScrollBar test',
		size => [300,200]);

	my $sb = $w-> insert( ScrollBar =>
		width => 280,
		left => 10,
		bottom => 50,
		onChange => sub {
			$w-> text( $_[0]-> value);
		});

	run Prima;

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, F<examples/rtc.pl>, F<examples/scrolbar.pl>

=head1 AUTHORS

Dmitry Karasik E<lt>dk@plab.ku.dkE<gt>,
Anton Berezin E<lt>tobez@plab.ku.dkE<gt> - documentation

=cut

