package Prima;

use strict;
require DynaLoader;
use vars qw($VERSION @ISA);
@ISA = qw(DynaLoader);

$VERSION = '0.01';
bootstrap Prima $VERSION;
$::application = undef;

sub END
{
   &Prima::cleanup();
}

sub run
{
   die "Prima was not properly initialized\n" unless defined $::application;
   $::application-> go if $::application-> alive;
}

1;
