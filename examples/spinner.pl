=pod

=head1 NAME

examples/spinner.pl - standard spinner widget

=head1 FEATURES

Demonstrates use of various spinner styles provided by the C<Prima::Spinner> API.

=cut

use strict;
use warnings;
use Prima qw(Application Buttons Spinner Sliders);

my $mw = Prima::MainWindow->new(
		size => [400, 400],
		text => 'Spinners');

my $spinner = $mw->insert('Spinner',
	style => 'circle',
	color => cl::Blue,
	hiliteColor => cl::White,
	pack => { side => 'left', fill => 'both', expand => 1 },
);

my $spinner2 = $mw->insert('Spinner',
	style => 'drops',
	pack => { side => 'left', fill => 'both', expand => 1 },
	color => cl::Green,
);

my $spinner3 = $mw->insert('Spinner',
	style => 'spiral',
	pack => { side => 'left', fill => 'both', expand => 1 },
);

$mw->insert(
	'Button',
	text => \ 'C<Green|U<S>tart>/C<Red|Stop>',
	hotKey => 's',
	checkable => 1,
	checked => 0,
	origin => [0,0],
	onClick => sub { $_->toggle for $spinner, $spinner2, $spinner3 },
	growMode => gm::XCenter
);

$spinner->insert(
	'Slider',
	min => 0,
	max => 100,
	origin => [0,0],
	onChange => sub { $spinner->value( shift->value ) },
	growMode => gm::XCenter,
	current => 1,
);

Prima->run;

