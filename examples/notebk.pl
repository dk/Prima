=pod

=head1 NAME

examples/notebk.pl - notebook widget

=head1 FEATURES

Demonstrates the L<Prima::TabbedNotebook> standard widget

=cut

use strict;
use warnings;
use Prima qw(Buttons Notebooks Widget::ScrollWidget Application MsgBox);

package Bla;
use vars qw(@ISA);
@ISA = qw(Prima::MainWindow);

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);

	my $n = $self-> insert( TabbedScrollNotebook =>
		pack => { fill => 'both', expand => 1, padx => 20, pady => 20 },
#     		pageCount => 11,
		tabs => [0..5,5,5..10],
		name => 'book',
	);

	$n-> insert_to_page( 0 => 'Button');
	my $j = $n-> insert_to_page( 1 => 'CheckBox' => left => 200);
	$n-> insert_to_page( 2,
		[ Button => origin => [ 0, 0], ],
		[ Button => origin => [ 10, 40], ],
		[ Button => origin => [ 10, 70], ],
		[ Button => origin => [ 10,100], ],
		[ Button => origin => [ 110, 10], ],
		[ Button => origin => [ 110, 40], ],
		[ Button => origin => [ 110, 70], ],
		[ Button => origin => [ 110,100], ],
	);
	$n-> insert_transparent('Button',
		name   => 'TopMostButton',
		text   => 'Toggle Orientation',
		origin => [0,30],
		size   => [200,20],
		growMode => gm::XCenter,
		onClick => sub { $n-> orientation($n-> orientation ? 0 : 1) },
	);

	$n-> insert_transparent('Button',
		name   => 'StyleButton',
		text   => 'Toggle Style',
		origin => [0,5],
		size   => [200,20],
		growMode => gm::XCenter,
		onClick => sub { $n-> style($n-> style ? 0 : 1) },
	);

	$n-> insert_transparent('Button',
		name   => 'StyleButton',
		text   => 'Toggle Skin',
		origin => [0,55],
		size   => [200,20],
		growMode => gm::XCenter,
		onClick => sub { $n-> skin(($n-> skin eq 'flat') ? 'classic' : 'flat') },
	);

	$n-> insert_transparent('Button',
		name   => 'StyleButton',
		text   => 'Random Colors',
		origin => [0,80],
		size   => [200,20],
		growMode => gm::XCenter,
		onClick => sub { $n->colorset([ map { rand cl::White } 0 .. (1 + rand 20) ]) },
	);
	$n-> use_current_size;
	return %profile;
}


package Generic;

my $w = Bla-> create(
	size => [ 600, 300],
	y_centered  => 1,
	designScale => [7, 16],
	menuItems => [[ '~Action' => [
		[ '~New tab', 'Ctrl+N', '^N', sub {
			my $book   = shift->book;
			my $tabid  = scalar(@{$book->TabSet->tabs}) + 1;
			my $pageno = $book->insert_page("tab$tabid");
			$book->insert_to_page($pageno, Button =>
				origin  => [ 20, 20 ],
				text    => "$tabid",
			),
		}],
		[ 'New ~page', 'Ctrl+M', '^M', sub {
			my $book = shift->book;
			my $tabid  = $book->page2tab($book->pageIndex) + 1;
			my $pageid = $book->pageIndex + 1;
			my $pageno = $book->insert_page("tab$tabid", $pageid - 1);
			$book->insert_to_page($pageno, Button =>
				origin  => [ 20, 20 ],
				text    => "$tabid/$pageid",
			),
		}],
		[ '~Delete tab', 'Ctrl+W', '^W', sub {
			my $book = shift->book;
			$book->delete_page($book->pageIndex, 1);
		}],
	]]],
);

run Prima;
