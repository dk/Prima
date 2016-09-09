#  
#  Module for simulated extreme situations, useful for testing.
#
use strict;
no warnings;
use Prima;

my $rtfs = (  5, 7, 14, 18) [ int(rand(4)) ] ;

Prima::Application::add_startup_notification( sub {
	$::application-> font-> size( $rtfs);
});

package Prima::Widget;

sub get_default_font
{
	return Prima::Drawable-> font_match( { size => $rtfs }, {});
}

package Prima::Application;

sub get_default_font
{
	return Prima::Drawable-> font_match( { size => $rtfs }, {});
}


1;

=pod

=head1 NAME

Prima::Stress - stress test module

=head1 DESCRIPTION

The module is intended for use in test purposes, to check the functionality of
a program or a module under particular conditions that
might be overlooked during the design. Currently, the only stress factor implemented
is change of the default font size, which is set to different value every time 
the module is invoked.

To use the module functionality it is enough to include a typical

	use Prima::Stress;

code, or, if the program is invoked by calling perl, by using

	perl -MPrima::Stress program

syntax. The module does not provide any methods.

=head1 AUTHOR

Dmitry Karasik E<lt>dmitry@karasik.eu.orgkE<gt>,

=cut
