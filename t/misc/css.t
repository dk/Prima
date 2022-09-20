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
a:active		{ 20:1; }
z::after		{ 21:1; }
p:empty			{ 22:1; }
p:first-child		{ 23:1; }
p:first-of-type		{ 24:1; }
z:lang(it)		{ 25:1; }
p:last-child		{ 26:1; }
p:last-of-type		{ 27:1; }
:not(argh)		{ 28:1; }
z:nth-child(2)		{ 29:1; }
z:nth-last-child(2)	{ 30:1; }
z:nth-last-of-type(2)	{ 31:1; }
z:nth-of-type(2)	{ 32:1; }
p:only-of-type		{ 33:1; }
p:only-child		{ 34:1; }
input:optional		{ 35:1; }
input::placeholder	{ 36:1; }
input:read-only		{ 37:1; }
input:read-write	{ 38:1; }
input:required		{ 39:1; }
:root			{ 40:1; }
CSS

ok( ref($css), "css parsed" );
diag($css), exit unless ref($css);

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
	@wanted = sort 5, 28, 40, @wanted;
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
	@wanted = sort 5, 28, @wanted;
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
match ({ type => 'p'}, 6, 8, 23, 24, 22, 26, 27, 33, 34);
match ({ type => 'p', class => 'intro'}, 1, 6, 7, 8, 22, 23, 24, 26, 27, 33, 34);
match ({ type => 'div'}, 8);
cmatch({ type => 'div', children => [{ type => 'p'}]}, [0], 6, 8, 9, 10, 22, 23, 24, 26, 27, 33, 34);
cmatch({ type => 'div', children => [{ children => [{ type => 'p'}]}]}, [0,0], 6, 8, 9, 22, 23, 24, 26, 27, 33, 34);
cmatch({ children => [{type => 'div'}, {type => 'p'}]}, [1], 6, 8, 11, 22, 24, 26, 27, 33);
cmatch({ children => [{type => 'p'}, {type => 'foo'}, {type => 'ul'}]}, [2], 12);
match ({ target => 'foo' }, 13);
match ({ target => '_blank' }, 13, 14);
match ({ title => 'black flower' }, 15);
umatch({ title => 'blackflower' }, 15);
match ({ lang => 'en' }, 16);
match ({ lang => 'en-EN' }, 16);
umatch({ lang => 'enEN' }, 16);
match ({ type => 'a', href => 'https://' }, 17);
umatch({ type => 'a', href => 'xhttps://' }, 17);
match ({ type => 'a', href => '/a.pdf' }, 18);
umatch({ type => 'a', href => '/a.pdf#a' }, 18);
match ({ type => 'a', href => '_w3schools_' }, 19);
match ({ type => 'a', active => 1 }, 20);
match ({ type => 'z', after  => 1 }, 21);
match ({ type => 'z', lang => 'it'}, 25);
cmatch ({children => [{type=>'x'},{type=>'z'},{type=>'y'}]}, [1], 29,30);
cmatch ({children => [{type=>'x'},{type=>'z'},{type=>'z'},{type=>'z'},{type=>'y'}]}, [2], 31,32);
match ({ type => 'input', required => 1}, 38, 39);
match ({ type => 'input', readonly => 1}, 35, 37);
match ({ type => 'input', placeholder => 1}, 35, 36, 38);

done_testing;
