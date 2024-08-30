=pod

=head1 NAME

examples/launch.pl - a Prima example launcher

=head1 FEATURES

Can execute several Prima examples in one program.  A couple of Prima examples
can interact with other widget, f ex by changing their font or color (see cv.pl
or fontdlg.pl).  The launcher can run these examples together with examples
with another set of widgets, so that these experimental changes can be tested
on these widgets as well.

=cut

use strict;
use warnings;

use Prima;
use Prima::Classes;
use Prima::Dialog::FileDialog;

my $dlg;
my $closeAction = 0;
my $myApp;

package MyOpenDialog;
use vars qw( @ISA);
@ISA = qw( Prima::Dialog::OpenDialog);

sub ok
{
	Launcher::launch( $_[0]-> Name-> text);
}

package MyApp;
use vars qw( @ISA);
@ISA = qw( Prima::Application);

sub close
{
	$_[0]-> SUPER::close if $closeAction;
}

sub destroy
{
	$_[0]-> SUPER::destroy if $closeAction;
}


package Generic;

sub start {
	$myApp = $::application = MyApp-> new( name => "Launcher");
	$dlg = MyOpenDialog-> new(
		name   => 'Launcher',
		filter => [
			['Scripts' => '*.pl'],
			['All files' => '*'],
		],
		onEndModal => sub {
			$closeAction++;
			$::application-> close;
			$closeAction--;
		},
	);
	my $cl = $dlg-> Cancel;

	$cl-> text('Close');
	$cl-> set(
		onClick => sub {
			$closeAction++;
			$dlg-> cancel;
			$closeAction--;
		},
	);
	$dlg-> execute_shared;
}

package Prima::Application;

sub new
{
	my $x = shift;
	return $myApp ? $myApp : $x-> SUPER::new( @_);
}

package Launcher;

sub launch
{
	my $pgm = $_[0];
	my $ok;
	my $app = $::application;
	{
		my $package = $pgm;
		$package =~ s{^.*[\\/]([^\\/]+)\..*$}{$1};
		print "require '$pgm' != 1 || run $package;";
		$ok = eval( "require '$pgm' != 1 || run $package; 1;");
	}
	$::application = $app;
	print $@ unless $ok;
}

Generic::start;

run Prima;

