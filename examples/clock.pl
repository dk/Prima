=pod

=head1 NAME

examples/clock.pl - xclock port

=head1 FEATURES

Demonstrates alpha drawing, and the standard widgets
C<Prima::Widget::Date> and C<Prima::Widget::Time>

=cut


use strict;
use warnings;
use Prima qw(Application Widget::Date Drawable::Path Widget::Time);

sub set_time_format
{
	my ( $mw, $menu ) = @_;

	my $text = $mw->menu->text($menu);
	if ( $text =~ /System/) {
		$mw->Time->format( $mw->Time->default_format );
	} else {
		$mw->Time->format( $text );
	}
}

sub set_date_format
{
	my ( $mw, $menu ) = @_;

	my $text = $mw->menu->text($menu);
	if ( $text =~ /System/) {
		$mw->Picker->format( $mw->Picker->default_format );
	} else {
		$mw->Picker->format( $text );
	}
}

my $diff = 0;

my $mw = Prima::MainWindow->new(
	size  => [200, 200],
	text  => 'Clock',
	menuItems => [
		['~Options' => [
			['~Time format' => [
				[ 'system' => '~System' => \&set_time_format ],
				[],
				[ 'hh:mm:ss' => \&set_time_format ],
				[ 'hh:mm AA' => \&set_time_format ],
				[ 'hr:hh min:mm sec:ss' => \&set_time_format ],
			]],
			['~Date format' => [
				[ 'system' => '~System' => \&set_date_format ],
				[],
				[ 'YYYY-MM-DD' => \&set_date_format ],
				[ 'MM/DD/YYYY' => \&set_date_format ],
				[ 'MM YYYY' => \&set_date_format ],
				[ 'DD/MM/YY'   => \&set_date_format ],
				[ 'year:YYYY month:MM day:DD' => \&set_date_format ],
			]],
			[],
			[ 'E~xit' => 'Alt+X' => '@X' => sub { shift-> close } ],
		]],
		['~Reset' => sub {
			my $mw = shift;
			$mw->Time->time($mw->Time->now);
			$mw->Picker->date($mw->Picker->today);
		}],
	],
);

$mw->insert( 'Widget::Time' =>
	name => 'Time',
	pack => { side => 'bottom', fill => 'x', pad => 20 },
	onChange => sub {
		my ($s,$m,$h) = shift->time;
		my $d1 = $h * 3600 + $m * 60 + $s;
		($s,$m,$h) = localtime(time);
		my $d2 = $h * 3600 + $m * 60 + $s;
		my $ndiff = $d1 - $d2;
		if ( $ndiff != $diff ) {
			$diff = $ndiff;
			$mw->Clock->repaint;
		}
	},
);

$mw->insert( 'Widget::Date' =>
	name => 'Picker',
	pack => { side => 'bottom', fill => 'x', pad => 20 },
);


sub draw_hand
{
	my ( $canvas, $color, $rotate, $x, $y ) = @_;
	$canvas->color($color);
	$canvas-> new_path->
		rotate(-$rotate * 360)->
		scale( $x, $y)->
		translate( 0, 0.75)->
		line(1, -1, 0, 1, -1, -1)->
		fill;
}

$mw->insert( 'Widget' =>
	pack      => { side => 'bottom', fill => 'both', pad => 20, expand => 1 },
	name      => 'Clock',
	antialias => 1,
	buffered  => 1,
	onPaint   => sub {
		my ( $self, $canvas) = @_;
		$canvas-> clear;

                my ($sec,$min,$hour) = localtime(time + $diff);
		my @sz = $canvas-> size;
		my $d  = (( $sz[0] < $sz[1] ) ? $sz[0] : $sz[1] ) / 2;
		my $w  = $d / 15;

		$canvas-> translate( map { $_ / 2 } @sz );

		$canvas-> lineWidth($w / 4);
		for my $r ( 0..11 ) {
			$canvas-> new_path->
				rotate($r / 12 * 360)->
				scale( $w/4, $d)->
				line(0,0.8,0,0.95)->
				stroke;
		}

		$canvas->alpha(192);
		draw_hand($canvas, cl::Cyan, ($hour % 12 + $min / 60 + $sec / 3600) / 12, $w, $d/2.5);
		draw_hand($canvas, cl::Green, ($min + $sec / 60) / 60, $w, $d/2);
		draw_hand($canvas, cl::Red,   $sec / 60, $w/4, $d/2);

		$canvas->color(cl::White);
		$canvas->fill_ellipse(0,0,$w,$w);
	},
);

$mw-> insert( Timer => 
	timeout => 1000,
	onTick  => sub {
		my $ldiff = $diff;
		$mw->Clock->repaint;
		my ($sec,$min,$hour) = localtime(time + $diff);
		$mw->Time->time([$sec,$min,$hour]);
		$diff = $ldiff;
	},
)-> start;

Prima->run;
