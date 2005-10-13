# $Id$
print "1..8 init,create result,alive,method override,done,destroy,done on croak,croak during init\n";

my $stage = 1;

package GumboJumboObject;
use vars qw(@ISA);
@ISA = qw(Prima::Object);

sub init
{
	my $self = shift;
	main::ok( $self) if $stage == 1;
	my %profile = @_;
	%profile = $self-> SUPER::init( %profile);
	croak("test!") if $stage == 2;
	return %profile;
}


sub setup
{
	$main::dong = 1;
	$_[0]-> SUPER::setup;
}

sub done
{
	my $self = $_[0];
	$self-> SUPER::done;
	main::ok(1);
}

package main;

$dong = 0;
my $o = GumboJumboObject-> create;
ok( $o);
ok( $o-> alive);
ok( $dong);
$o-> destroy;
ok( !$o-> alive);
$stage++;
undef $o;
eval { $o = GumboJumboObject-> create; };
ok( !defined $o);

1;
