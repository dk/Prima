=pod

=head1 NAME

examples/windowstate.pl - test maximization etc

=head1 FEATURES

Switches the windows between minimized, maximized, etc states

=cut

use strict;
use warnings;
use Prima qw(Buttons Application);

my @ws = qw(Normal Min Max Fullscreen);

my $w = Prima::MainWindow->new(
	sizeMin => [200, 200],
	text => '',
	onWindowState => sub {
		print "EVENT: ", $ws[$_[1]], "\n";
	},
);

my $blinkin = 0;
my $action;

sub unblink
{
	$blinkin = 0;
	undef $action;
	$w->text('');
	$w->Timer1->destroy if $w->bring('Timer1');
}

sub action
{
	my $action = shift;
	if ( $blinkin ) {
		$w->insert( Timer =>
			name => 'Timer1',
			timeout => 2000,
			onTick => sub {
				my $b = $blinkin;
				unblink();
				$w->$action();
				$w->show if $b == 2;
			},
		)->start;
		if ( $blinkin == 1 ) {
			$w->minimize;
		} else {
			$w->hide;
		}
	} else {
		$w->$action();
	}
}

$w->insert( Button =>
	text => '~Minimize',
	onClick => sub {
		action('minimize');
	},
	pack => {},
);


$w->insert( Button =>
	text => 'Minimi~ze and ...',
	onClick => sub { 
		if ( $blinkin ) {
			unblink();
		} else {
			$blinkin = 1;
			$w->text('select action');
		}
	},
	pack => {},
);

$w->insert( Button =>
	text => '~Hide and ...',
	onClick => sub {
		if ( $blinkin ) {
			unblink();
		} else {
			$blinkin = 2;
			$w->text('select action');
		}
	},
	pack => {},
);

$w->insert( Button =>
	text => 'Ma~ximize',
	onClick => sub { action('maximize') },
	pack => {},
);

$w->insert( Button =>
	text => '~Restore',
	onClick => sub { action('restore') },
	pack => {},
);

$w->insert( Button =>
	text => '~Fullscreen',
	onClick => sub { action('fullscreen') },
	pack => {},
);

$w->insert( Button =>
	text => '~Quit',
	onClick => sub { $w->destroy },
	pack => {},
);

run Prima;
