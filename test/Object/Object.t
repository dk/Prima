print "1..5 init,create result,alive,method override,destroy\n";

package GumboJumboObject;
use vars qw(@ISA);
@ISA = qw(Prima::Object);

sub init
{
   my $self = shift;
   main::ok( $self);
   my %profile = @_;
   %profile = $self-> SUPER::init( %profile);
   return %profile;
}


sub setup
{
   $main::dong = 1;
   $_[0]-> SUPER::setup;
}


package main;

$dong = 0;
my $o = GumboJumboObject-> create;
ok( $o);
ok( $o-> alive);
ok( $dong);
$o-> destroy;
ok( !$o-> alive);
