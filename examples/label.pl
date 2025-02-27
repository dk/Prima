=pod

=head1 NAME

examples/label.pl - Prima label widget

=head1 FEATURES

Demonstrates the basic usage of the L<Prima::Label> class
capabilites, in particular text wrapping and links.

=head1 Misc

=over

=item tooltip

=over

=item *

This is tooltip text. It is shown as a preview

=item *

The tooltip mechanics respect native B<pod> I<formatting>

=for podview <img src=data:base64 title="and images">
R0lGODdhMAAwAJEAAAAAAGhoaL+/v////ywAAAAAMAAwAIEAAABoaGi/v7////8CwZyPqcvtD6Oc
tIGLs968ezB84kiW5tlV6rKt7tG+biyrdE3duKRHl9xzBH2aScpWJP5Cy8pQgUFEnUnI1HA1Vh/X
rDYjCRyhIMaTvJV6YemHeI1tM8G7eO9MvQjmZTYd/QWwZ9fH16QGhyaY8HeHlzHIOKUD9qcYqWhY
2AgHyeXhV8gnqacUYxkaivnZgooI4pnzwRqbh0e4uDJmVTpzy1jzWxd6OExbbGwhl6yIzNws+gyN
Ql1tXa15jS3N3e3tUAAAOw==

=back

=back

=cut

use strict;
use warnings;
use Prima qw(InputLine Label MsgBox Application);

my $w = Prima::MainWindow-> new(
	size => [ 600, 200],
	text => "Static texts",
	designScale => [7, 16],
);

my $b1 = $w->insert( InputLine =>
	left => 20,
	bottom => 10,
	width => 80,
	text => 'Press Alt-L to have me focused',
);
$w->insert( InputLine =>
	left => 120,
	bottom => 10,
	width => 80,
	text => '',
);

$w->insert( Label =>
	origin => [ 20, 60],
	text => "~Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et ".
"dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo ".
"consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. ".
"Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
	focusLink => $b1,
	wordWrap => 1,
	height => 80,
	width => 212,
	growMode => gm::Client,
	showPartial => 0,
	textJustify => 1,
);

$w->insert(
	Label      => origin   => [ 260, 140],
	size       => [300, 36],
	wordWrap   => 1,
	text       => \ "Label with a L<tip://$0/tooltip|tooltip> - click does not function",
	growMode   => gm::GrowLoX,
);

$w->insert(
	Label      => origin   => [ 260, 100],
	size       => [300, 36],
	wordWrap   => 1,
	text       => \ "Label with a L<pod://$0/FEATURES|pod> - both click and preview work and a L<http://prima.eu.org/|http link>",
	growMode   => gm::GrowLoX,
);

$w->insert(
	Label      => origin   => [ 260, 60],
	size       => [300, 36],
	wordWrap   => 1,
	text       => \ "Label with L<custom link|custom stuff> - both click and preview are overloadable",
	growMode   => gm::GrowLoX,
	onLink     => sub {
		my ( $self, $link_handler, $url) = @_;
		message($url);
		$self->clear_event;
	},
	onLinkPreview => sub {
		my ( $self, $link_handler, $url, $btn, $mod) = @_;
		$self->clear_event;
		$$url  = localtime;
	},
);

$w->insert(
	Label      => origin   => [ 260, 20],
	text       => 'Disab~led label',
	autoHeight => 1,
	enabled    => 0,
	growMode   => gm::GrowLoX,
	textJustify => 1,
);

Prima->run;

