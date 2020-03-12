use strict;
use warnings;
use Prima qw(Application MsgBox);

my %opt = (
	le  => le::Round,
	lep => 'le::Round',
	lj  => lj::Round,
	ljp => 'lj::Round',
	lp  => lp::DotDot,
	lpp => 'lp::DotDot',
	ml  => 10,
	lw  => 28,
);
$opt{ml} = 10;
my $aperture = 12;
my $capture;
my $prelight;
my @points;
@points = (100,200,200,200,200,100,50,10);

sub menu($$)
{
	my ( $class, $id ) = @_;
	"$class\:\:$id" => "~$id" => sub {
		$_[0]->menu->uncheck($opt{$class . 'p'});
		$_[0]->menu->check($_[1]);
		$opt{$class . 'p'} = $_[1];
		$opt{$class} = eval $_[1];
		$_[0]->repaint;
	}
}

my $mw;

sub lw
{
	$opt{lw} = shift;
	$mw->repaint;
}

$mw = Prima::MainWindow->new(
	size => [ 400, 431 ],
	text => 'Line plotter',
	designScale => [ 7, 16 ],
	menuItems => [
		['~Options' => [ 
			[ '@closed'    => '~Closed'   => sub{shift->repaint} ],
			[ '@compare'   => '~Compare'  => sub{shift->repaint} ],
			[ '@*hairline' => '~Hairline' => sub{shift->repaint} ],
			[],
			[ 'E~xit' => sub { $_[0]->destroy } ],
		]],
		[ '~End' => [ map { [ menu le => $_ ] } qw(Flat Square Round) ]],
		[ '~Join' => [
			(map { [ menu lj => $_ ] } qw(Round Bevel Miter)),
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
		[ '~Pattern' => [ map { [ menu lp => $_ ] } qw(
			Null Solid Dash LongDash ShortDash
			Dot DotDot DashDot DashDotDot
		) ]],
		[ '~Width' => [
			(map { my $k = $_; [ $k , sub { lw($k) } ] } (0,1,2,3,5,10,20,30)),
		]],
	],
	buffered => 1,
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		my ( $w, $h ) = $self-> size;
		$canvas->clear();
		$canvas->lineEnd($opt{le});
		$canvas->lineJoin($opt{lj});
		$canvas->miterLimit($opt{ml});
		my $c = $prelight // $capture // -1;
		$canvas->color(cl::Black);
		my @xpoints = 
			$self->menu->checked('closed') ?
			((@points < 6) ? ( 100, 100, @points, 100, 100 ) : @points[0..$#points,0,1]):
			( 30, 30, @points, $w - 30, $h - 30);
		my $cmp;
		if ( $cmp = $self-> menu->checked('compare')) {
			$canvas->lineWidth($opt{lw}+2);
			$canvas->linePattern($opt{lp});
			$canvas->polyline( \@xpoints);
			$canvas->lineWidth(1);
			$canvas->linePattern(lp::Solid);
		}

		my $p = $canvas->new_path;
		$p->line(\@xpoints);
		$p = $p->widen( 
			lineWidth   => $opt{lw},
			lineEnd     => $opt{le},
			lineJoin    => $opt{lj},
			linePattern => $opt{lp},
			miterLimit  => $opt{ml},
		);
		$canvas->color(cl::LightRed);
		$canvas->rop(rop::OrPut) if $cmp;
		$canvas->fillMode(fm::Winding|fm::Overlay);
		$p->fill;
		$canvas->rop(rop::CopyPut);

		if ( $cmp = $self-> menu->checked('hairline')) {
			$canvas->color(cl::White);
			$canvas->lineWidth(1);
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

$mw->menu->check($opt{$_}) for qw(lep ljp lpp);

run Prima;
