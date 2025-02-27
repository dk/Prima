=pod

=head1 NAME

examples/lineplot.pl - line plotting API

=head1 FEATURES

Demonstrates Prima line joins, patterns, endings etc.

=cut

use strict;
use warnings;
use Prima qw(Application MsgBox);

my %opt = (
	le  => [le::Round, undef, undef, undef],
	lj  => lj::Round,
	ljp => 'lj::Round',
	lp  => lp::DotDot,
	lpp => 'lp::DotDot',
	ml  => 10,
	lw  => 28,
	sc  => 1,
);
$opt{ml} = 10;
my $aperture = 12;
my $capture;
my $prelight;
my @points;
@points = (100,200,200,200,200,100,50,10);


sub group
{
	my $class = shift;
	my @items = map {[
		"$class\:\:$_" => "~$_" => sub {
			$opt{$class . 'p'} = $_[1];
			$opt{$class} = eval $_[1];
			$_[0]->repaint;
		}
	]} @_;
	$items[0][0]  = "(" . $items[0][0]; 
	$items[-1][0] = ")" . $items[-1][0]; 
	return @items;
}

sub legroup
{
	my $id = shift;
	my @items = qw(Flat Square Round Arrow Cusp InvCusp Knob Rect RoundRect Spearhead Tail);
	@items = map {[
		"le\:\:$_" => "~$_" => sub {
			$opt{le}->[$id] = eval $_[1];
			$_[0]->repaint;
		}
	]} @items;
	unshift @items, [
		'*' => "~Default" => sub {
			$opt{le}->[$id] = undef;
			$_[0]->repaint;
		}
	] if $id;
	$items[0][0]  = "(" . $items[0][0]; 
	$items[-1][0] = ")" . $items[-1][0]; 
	return @items;
}

my $mw;

sub lw
{
	$opt{lw} = shift;
	$mw->repaint;
}

sub scale
{
	$opt{sc} = $mw->menu->text($_[1]);
	$mw->repaint;
}

$mw = Prima::MainWindow->new(
	size => [ 400, 431 ],
	text => 'Line plotter',
	designScale => [ 7, 16 ],
	menuItems => [
		['~Options' => [ 
			[ '@closed'    => '~Closed'    => sub{shift->repaint} ],
			[ '@*hairline' => '~Hairline'  => sub{shift->repaint} ],
			[ '@aa'        => '~Antialias' => sub{shift->repaint} ],
			[ '@curve'     => 'C~urve'     => sub{shift->repaint} ],
			[],
			[ 'E~xit' => sub { $_[0]->destroy } ],
		]],
		[ '~Ends' => [
			[ '~Line head'  => [ legroup 0 ]],
			[ 'Line ~tail'  => [ legroup 1 ]],
			[ 'Arrow ~head' => [ legroup 2 ]],
			[ '~Arrow tail' => [ legroup 3 ]],
		]],
		[ 'End ~scale' => [
			[ '(' , 0.5  => \&scale ],
			[ ''  , 0.75 => \&scale ],
			[ '*' , 1    => \&scale ],
			[ ''  , 1.25 => \&scale ],
			[ ''  , 1.5  => \&scale ],
			[ ')' , 1.75 => \&scale ],
		]],
		[ '~Join' => [
			group(lj => qw(Round Bevel Miter)),
			[],
			['~Set limit..' => sub {
				while ( 1 ) {
					my $ml = Prima::MsgBox::input_box(
						'Set miter limit',
						'Value:',
						$opt{ml}
					);
					last unless defined $ml;
					unless ( $ml =~ /^[\.\d]+$/ && ($ml+0) > 0 && ($ml+0) <= 20) {
						Prima::MsgBox::message('Wrong value, must be between 0 and 20');
						next;
					}
					$opt{ml} = $ml;
					$_[0]->repaint;
					last;
				}
			}],
		]],
		[ '~Pattern' => [ group lp => qw(
			Null Solid Dash LongDash ShortDash
			Dot DotDot DashDot DashDotDot
		) ]],
		[ '~Width' => [
			(map { my $k = $_; [ $k , sub { lw($k) } ] } (0,0.5,1,2,3,5,10,20,30)),
		]],
	],
	buffered => 1,
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		my ( $w, $h ) = $self-> size;
		$canvas->clear();
		$canvas->lineEnd([
			map {
				defined ? le::scale( $_, $opt{sc} ) : undef
			}
			@{ $opt{le} }
		]);
		$canvas->lineJoin($opt{lj});
		$canvas->miterLimit($opt{ml});
		my $c = $prelight // $capture // -1;
		$canvas->color(cl::Black);
		my @xpoints = 
			$self->menu->checked('closed') ?
			((@points < 6) ? ( 100, 100, @points, 100, 100 ) : @points[0..$#points,0,1]):
			( 30, 30, @points, $w - 30, $h - 30);

		if ( $self-> menu->checked('curve')) {
			@xpoints = @{ $canvas->render_spline(\@xpoints) }
		}


		$canvas->color(cl::LightRed);
		$canvas->lineWidth($opt{lw});
		$canvas->linePattern($opt{lp});
		$canvas->antialias($self->menu->checked('aa'));
		$canvas->polyline(\@xpoints);
		$canvas->antialias(0);

		$canvas->rop(rop::CopyPut);

		if ( $self-> menu->checked('hairline')) {
			$canvas->color(cl::White);
			$canvas->lineWidth(0);
			$canvas->linePattern(lp::Solid);
			$canvas-> polyline( \@xpoints);
		}
		for ( my $i = 0; $i < @points; $i+=2) {
			$canvas-> color(($i == $c) ? cl::White : cl::Black);
			$canvas-> fill_ellipse(
				$points[$i], $points[$i+1],
				$aperture, $aperture
			);
			$canvas-> color(($i == $c) ? cl::Black : cl::White);
			$canvas-> fill_ellipse(
				$points[$i], $points[$i+1],
				$aperture/2, $aperture/2
			);
		}
	},
	onSize   => sub {
		my ( $self, $ox, $oy, $x, $y) = @_;
		my $i;
		for ( $i = 0; $i < @points; $i+=2) {
			$points[$i] = $x   if $points[$i] > $x;
			$points[$i+1] = $y if $points[$i+1] > $y;
		}
	},
	onMouseDown => sub {
		my ( $self, $btn, $mod, $x, $y) = @_;
		my $i;
		$capture = undef;
		for ( $i = 0; $i < @points; $i+=2) {
			if ( $points[$i] > $x - $aperture && $points[$i] < $x + $aperture &&
				$points[$i+1] > $y - $aperture && $points[$i+1] < $y + $aperture) {
				if ( $btn == mb::Left ) {
					$capture = $i;
				} elsif ( $btn == mb::Right ) {
					splice(@points, $i, 2);
				}
				$self->repaint;
				return;
			}
		}

	},
	onMouseClick => sub {
		my ( $self, $mod, $btn, $x, $y, $dbl) = @_;
		return unless $dbl;
		push @points, $x, $y;
		$self->repaint;
	},
	onMouseUp => sub {
		undef $capture;
		shift->repaint;
	},
	onMouseMove => sub {
		my ( $self, $btn, $x, $y) = @_;
		if (defined $capture && $capture >= 0 ) {
			my @bounds = $self->size;
			$points[$capture] = $x if $x >= 0 && $x < $bounds[0];
			$points[$capture+1] = $y if $y >= 0 && $y < $bounds[1];
			$self->repaint;
		} else {
			my $i;
			my $p;
			for ( $i = 0; $i < @points; $i+=2) {
				if ( $points[$i] > $x - $aperture && $points[$i] < $x + $aperture &&
					$points[$i+1] > $y - $aperture && $points[$i+1] < $y + $aperture) {
					$p = $i;
					goto FOUND;
				}
			}
		FOUND:
			if (( $p // -20 ) != ($prelight // -20)) {
				$prelight = $p;
				$self->repaint;
			}
		}
	},
	onMouseLeave => sub {
		if ( $prelight ) {
			undef $prelight;
			shift->repaint;
		}
	},
);

$mw->menu->check($opt{$_}) for qw(ljp lpp);

Prima->run;
