=pod

=head1 NAME

examples/launch.pl - A Prima example launcher

=head1 FEATURES

Uses to execute several Prima examples in one task space.
Many examples inflict the view af a user-selected widget, but
lack existence of the appropriate one. The launcher helps
such experiments. See cv.pl or fontdlg.pl in particular.

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
	$myApp = $::application = MyApp-> create( name => "Launcher");
	$dlg = MyOpenDialog-> create(
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

sub create
{
	my $x = shift;
	return $myApp ? $myApp : $x-> SUPER::create( @_);
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

