use strict;
use warnings;
use subs qw(reset);
use lib '.', 'blib/arch';
use Prima qw(Application MsgBox);

my %opt = (
	le  => le::Flat,
	lep => 'le::Flat',
	lj  => lj::Round,
	ljp => 'lj::Round',
	ml  => 10,
);
$opt{ml} = 10;
my $aperture = 12;
my $capture;
my $prelight;
my @points;

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

my $mw = Prima::MainWindow->new(
	size => [ 400, 431 ],
	text => 'Line plotter',
	designScale => [ 7, 16 ],
	menuItems => [
		['~Options' => [ 
			[ closed => '~Closed' => sub { $_[0]->menu->toggle($_[1]); $_[0]->repaint } ],
			[ compare => '~Compare' => sub { $_[0]->menu->toggle($_[1]); $_[0]->repaint } ],
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
	],
	buffered => 1,
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		my ( $w, $h ) = $self-> size;
		$canvas->clear();
		$canvas->lineEnd($opt{le});
		$canvas->lineJoin($opt{lj});
		my $c = $prelight // $capture // -1;
		$canvas->color(cl::Black);
		my @xpoints = 
			$self->menu->checked('closed') ?
			( 100, 100, @points, 100, 100 ) :
			( 30, 30, @points, $w - 30, $h - 30);
		my $cmp;
		if ( $cmp = $self-> menu->checked('compare')) {
			$canvas->lineWidth(30);
			$canvas-> polyline( \@xpoints);
			$canvas->lineWidth(1);
		}

		my $p = $canvas->new_path;
		$p->line(\@xpoints);
		$p = $p->widen( 
			lineWidth  => 28, 
			lineEnd    => $opt{le}, 
			lineJoin   => $opt{lj}, 
			miterLimit => $opt{ml} 
		);
		$canvas->color(cl::LightRed);
		$canvas->rop(rop::OrPut) if $cmp;
		$canvas->fillWinding(1);
		$p->fill;
		$canvas->rop(rop::CopyPut);

		$canvas->color(cl::White);
		$canvas->lineWidth(1);
		$canvas->linePattern(lp::Solid);
		$canvas-> polyline( \@xpoints);
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

$mw->menu->check($opt{$_}) for qw(lep ljp);

run Prima;
