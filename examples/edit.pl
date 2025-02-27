=pod

=head1 NAME

examples/edit.pl - An input line

=head1 FEATURES

Demonstrates use of the standard L<Prima::InputLine> widget

=cut

use strict;
use warnings;

use Prima 'InputLine', 'Application';

my $w = Prima::MainWindow->new( size => [ 700, 300]);

my $l = $w->insert( InputLine =>
	text        => '0:::1234 5678 90ab cdef ghij klmn oprq stuv:1::1234 5678 90ab cdef ghij klmn oprq stuv:2::1234 5678 90ab cdef ghij klmn oprq stuv:3::1234 5678 90ab cdef ghij klmn oprq stuv:4::1234 5678 90ab cdef ghij klmn oprq stuv::End',
	centered    => 1,
	width       => 300,
#  firstChar   => 10,
	alignment   => ta::Center,
	font        => { size => 18, },
	growMode    => gm::GrowHiX,
	buffered    => 1,
	borderWidth => 3,
	autoSelect  => 0,
);

Prima->run;
