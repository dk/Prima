use strict;
use warnings;
use Prima qw(Application Sliders);

=pod

=head1 NAME

examples/progressbar.pl - display progress bars

=cut

my $w = Prima::MainWindow->new(
	text => 'Progress bars',
	onKeyDown => sub {
		my ( $self, $code, $key, $mod ) = @_;
		if ( $key == kb::Left ) {
			my $v = $self->P1->value;
			$self->$_->value($v - 5) for qw(P1 P2 P3);
		}
		if ( $key == kb::Right ) {
			my $v = $self->P1->value;
			$self->$_->value($v + 5) for qw(P1 P2 P3);
		}
	},
	size => [ 600, 500 ],
);

$w->insert('Prima::ProgressBar',
	name => 'P1',
	pack => { expand => 1, fill => 'x', pad => 20 },
	height => 40,
	value => 50,
	color => cl::LightRed,
);
$w->insert('Prima::ProgressBar',
	name => 'P2',
	pack => { expand => 1, fill => 'x', pad => 20 },
	height => 40,
	value => 50,
	color => cl::Yellow,
);
$w->insert('Prima::ProgressBar',
	name => 'P3',
	pack => { expand => 1, fill => 'x', pad => 20 },
	height => 40,
	value => 50,
	color => cl::Green,
);
$w->insert('Prima::ProgressBar',
	name => 'P4',
	pack => { expand => 1, fill => 'x', pad => 20 },
	height => 40,
	value => 100,
	color => cl::Blue,
);

Prima->run;
