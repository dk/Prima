package Prima;

BEGIN
{
   package TiedStdOut;

   sub TIEHANDLE
   {
      my $class = shift;
      my $isError = shift;
      bless \$isError, $class;
   }

   tie *STDOUT, 'TiedStdOut', 0;
   tie *STDERR, 'TiedStdOut', 1;

   $SIG{'__WARN__'} = sub { print STDERR $_[0]; };
}

package main;
$application = undef;

package Prima;

use strict;
use MakeConfig;
use Config;
use Carp;

BEGIN
{
   $Carp::MaxEvalLen = 12;
}

sub extproc {}

sub prima
{
   return unless $#_ == 0;
   my $mainModule = shift;
   $::application = undef;
   die "$@" unless eval( "require '$mainModule';");
   die "Prima was not properly initialized\n" unless defined $::application;
   $::application-> go if $::application-> alive;
print "Done ::go";
   $::application = undef;
print "Done undefing";
   1;
}

1;
