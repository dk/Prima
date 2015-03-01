# $Id$
print "1..8 init,create result,alive,method override,done,destroy,done on croak,croak during init\n";

my $stage = 1;

package GumboJumboObject;
use vars qw(@ISA);
use Test::More;
@ISA = qw(Prima::Object);

sub init
{
	my $self = shift;
	ok( $self, "init") if $stage == 1;
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
	pass("done" );
}

package main;
use Prima::Test qw(dong);
use Test::More;

$dong = 0;
my $o = GumboJumboObject-> create;
ok( $o, "create result" );
ok( $o-> alive, "alive" );
ok( Prima::Test->set_dong(), "method override" );
$o-> destroy;
ok( !$o-> alive, "destroy");
$stage++;
undef $o;
eval { $o = GumboJumboObject-> create; };
ok( !defined $o, "croak during init" );

done_testing();
