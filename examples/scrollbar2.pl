=pod

=head1 NAME

examples/scrollbar2.pl - Prima scrollbar widget

=head1 FEATURES

A Prima toolkit demonstration example.
Tests the L<Prima::Scrollbar> widget and dynamic
change of its parameters.

=cut

use strict;
use warnings;

use Prima qw( Buttons ScrollBar);
use Prima::Application name => 'rtc', skin => 'flat';

package UserInit;

my $w = Prima::MainWindow-> create(
	text=> "Scrollbar",
	origin => [ 200, 200],
	size   => [ 250, 300],
	designScale => [7, 16],
);

$w-> insert( "Button",
	pack => { side => 'bottom', pady => 20 },
	text => "Change scrollbar direction",
	onClick=> sub {
		my $i = $_[0]-> owner-> scrollbar;
		$i-> vertical( ! $i-> vertical);
	}
);

$w-> insert( "ScrollBar",
	name    => "scrollbar",
	pack => { pady => 60, padx => 60, fill => 'both', expand => 1 },
	size => [ 150, 150],
	onCreate => sub {
		Prima::Timer-> create(
			timeout=> 1000,
			timeout=> 200,
			owner  => $_[0],
			onTick => sub{
				# $_[0]-> owner-> vertical( !$_[0]-> owner-> vertical);
				my $t = $_[0]-> owner;
				my $v = $t-> partial;
				$t-> partial( $v+1);
				$t-> partial(1) if $t-> partial == $v;
				#$_[0]-> timeout( $_[0]-> timeout == 1000 ? 200 : 1000);
			},
		)-> start;
	},
);

run Prima;
