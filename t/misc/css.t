use strict;
use warnings;
use Test::More;
use Prima::sys::CSS;

my $css = Prima::sys::CSS::Parser->new->parse(<<'CSS');
.intro			{ 1:1; }
.name1.name2		{ 2:1; }
.name1 .name2		{ 3:1; }
#firstname		{ 4:1; }
*			{ 5:1; }
p			{ 6:1; }
p.intro			{ 7:1; }
div, p			{ 8:1; }
div p			{ 9:1; }
div > p			{ 10:1; }
div + p			{ 11:1; }
p ~ ul			{ 12:1; }
[target]		{ 13:1; }
[target=_blank]		{ 14:1; }
[title~=flower]		{ 15:1; }
[lang|=en]		{ 16:1; }
a[href^="https"]	{ 17:1; }
a[href$=".pdf"]		{ 18:1; }
a[href*="w3schools"]	{ 19:1; }
CSS

ok( ref($css), "css parsed" );
diag($css) unless ref($css);

sub _id
{
	my $i = shift;
	return join(' ', map { "$_=$i->{$_}" } grep { !/^(children)$/ } sort keys %$i);
}

sub match
{
	my ( $item, @wanted ) = @_;
	$css->study($item);
	my %attr = $css->match($item);
	my @attr = sort keys %attr;
	my $id = _id($item);
	@wanted = sort 5, @wanted;
	is("@attr", "@wanted", $id);
}

sub cmatch
{
	my ( $item, $path, @wanted ) = @_;
	$css->study($item);
	$item = $item->{children}->[$_] for @$path;
	my %attr = $css->match($item);
	my @attr = sort keys %attr;
	my $id = _id($item);
	@wanted = sort 5, @wanted;
	is("@attr", "@wanted", $id);
}

sub umatch
{
	my ( $item, @wanted ) = @_;
	$css->study($item);
	my %attr = $css->match($item);
	my @attr = sort keys %attr;
	my $id = _id($item);
	@wanted = sort 5, @wanted;
	isnt("@attr", "@wanted", "NO $id");
}

match ({ class => 'intro'}, 1);
umatch({ class => 'xintro'}, 1);
match ({ class => 'name1 name2 name3'}, 2);
match ({ class => 'name1 name2 name3 intro'}, 1,2);
cmatch({ class => 'name1', children => [{ class => 'name2'}]}, [0], 3);
match ({ id => 'firstname'}, 4);
match ({ id => 'xarr'});
match ({ name => 'p'}, 6, 8);
match ({ name => 'p', class => 'intro'}, 1, 6, 7, 8);
match ({ name => 'div'}, 8);
cmatch({ name => 'div', children => [{ name => 'p'}]}, [0], 6, 8, 9, 10);
cmatch({ name => 'div', children => [{ children => [{ name => 'p'}]}]}, [0,0], 6, 8, 9);
cmatch({ children => [{name => 'div'}, {name => 'p'}]}, [1], 6, 8, 11);
cmatch({ children => [{name => 'p'}, {name => 'foo'}, {name => 'ul'}]}, [2], 12);
match ({ target => 'foo' }, 13);
match ({ target => '_blank' }, 13, 14);
match ({ title => 'black flower' }, 15);
umatch({ title => 'blackflower' }, 15);
match ({ lang => 'en' }, 16);
match ({ lang => 'en-EN' }, 16);
umatch({ lang => 'enEN' }, 16);
match ({ name => 'a', href => 'https://' }, 17);
umatch({ name => 'a', href => 'xhttps://' }, 17);
match ({ name => 'a', href => '/a.pdf' }, 18);
umatch({ name => 'a', href => '/a.pdf#a' }, 18);
match ({ name => 'a', href => '_w3schools_' }, 19);


done_testing;
