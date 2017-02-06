use strict;
use warnings;
use Prima qw(Application Buttons Spinner);

my $mw = Prima::MainWindow->new(
		size => [400, 400],
		text => 'Button Example');

my $spinner = $mw->insert('Spinner',
	size => [200,400],
	origin => [0,0],
	color => cl::White,
	hiliteColor => cl::Blue,
	growMode => gm::Left
);

my $spinner2 = $mw->insert('Spinner',
	style => 'drops',
	size => [200,400],
	origin => [200,0],
	color => cl::White,
	hiliteColor => cl::Green,
	growMode => gm::Left
);

my $button = $mw->insert(
	'Button',
	text => 'Start/Stop',
	checkable => 1,
	checked => 0,
	origin => [0,0],
	onClick => \&do_clicked,
	growMode => gm::XCenter
);

run Prima;

sub do_clicked {
	my ($self) = @_;
	if ($spinner->active) {
		$spinner->active(0);
		$spinner2->active(0);
	}
	else {
		$spinner->active(1);
		$spinner2->active(1);
	}
};
