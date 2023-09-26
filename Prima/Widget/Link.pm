package Prima::Widget::Link;
use strict;
use warnings;
use base qw(Prima::Widget::EventRectangles);
use Prima::Drawable::TextBlock;
use Prima::Drawable::Markup;

sub notification_types {{
	Link           => nt::Action,
	LinkPreview    => nt::Action,
	LinkAdjustRect => nt::Action,
}}
sub profile_default    {{ color => cl::Green  }}

*op_link_enter = \&Prima::Drawable::Markup::op_link_enter;
*op_link_leave = \&Prima::Drawable::Markup::op_link_leave;

sub new
{
	my $class = shift;
	my %profile = @_;
	my $self = {
		last_link_pointer => [-1, cr::Default],
	};
	bless( $self, $class);
	$self-> {$_} = $profile{$_} ? $profile{$_} : []
		for qw( rectangles references);
	$self->{color} = $profile{color} // profile_default->{color};
	$self->bind_markup($profile{markup}) if defined $profile{markup};
	return $self;
}

sub contains
{
	my ( $self, $x, $y) = @_;
	my $rec = 0;

	for ( @{$self-> {rectangles}}) {
		return $rec if $x >= $$_[0] && $y >= $$_[1] && $x < $$_[2] && $y < $$_[3];
		$rec++;
	}
	return -1;
}

sub id2rectangles
{
	my ( $self, $link_id ) = @_;
	return grep { $_->[4] == $link_id } @{ $self->{rectangles} };
}

sub rectangles
{
	return $_[0]-> {rectangles} unless $#_;
	$_[0]-> {rectangles} = $_[1];
}

sub references
{
	return $_[0]-> {references} unless $#_;
	$_[0]-> {references} = $_[1];
}

sub color
{
	return $_[0]-> {color} unless $#_;
	my ( $self, $color ) = @_;
	$self->{color} = $color;
	$self->{colormap}->[ $self->{link_color_index} ] = $color if $self->{colormap};
}

sub bind_markup
{
	my ( $self, $m ) = @_;
	if ( $m ) {
		my $cm = $m->colormap;
		my $ix = @$cm;
		push @$cm, $self->color;
		$m->linkColor( $ix | tb::COLOR_INDEX );
		$m->reparse;
		$self->{link_color_index} = $ix;
		$self->{colormap}         = $cm;
		$self->{references}       = $m->link_urls;
	} else {
		$self->{references}       = [];
		$self->{link_color_index} = undef;
		$self->{colormap}         = undef;
	}
}

sub open_podview
{
	my ( $self, $url, $btn, $mod ) = @_;
	$::application->open_help($url);
}

sub open_browser
{
	my ( $self, $url, $btn, $mod ) = @_;

	if ( Prima::Application-> get_system_info-> {apc} == apc::Win32) {
		open UNIQUE_FILE_HANDLE_NEVER_TO_BE_CLOSED, "|start $url";
		close UNIQUE_FILE_HANDLE_NEVER_TO_BE_CLOSED if 0;
	} elsif ( $^O eq 'darwin') {
		return !system( open => $url );
	} else {
		my $pg;
		CMD: for my $cmd ( qw(xdg-open x-www-browser www-browser firefox mozilla sensible-browser netscape)) {
			for ( split /:/, $ENV{PATH} ) {
				$pg = "$_/$cmd", last CMD if -x "$_/$cmd";
			}
		}
		return 0 unless defined $pg && ! system( "$pg $url &");
	}

	return 1;
}

sub on_link
{
	my ( $self, $owner, $url, $btn, $mod ) = @_;

	return unless $owner->notify(q(Link), $self, $url, $btn, $mod);
	return 0 if $btn != mb::Left;

	if ( $url =~ m[^pod://(.*)] ) {
		my $link = $1;
		# if it is a path or module? 
		if ( $link !~ /::/ && $link =~ m/^(.*?)\/([^\/]*)$/) {
			$link = "file://$1|$2";
		}
		$self->open_podview($link, $btn, $mod);
	} elsif ( $url =~ m[^(ftp|https?)://]) {
		$self->open_browser($url, $btn, $mod);
	} elsif ( $url =~ m[^tip://]) {
		# nothing
	}
}

sub on_linkpreview
{
	my ( $self, $owner, $url ) = @_;

	return unless $owner->notify(q(LinkPreview), $self, $url);

	if ( $$url =~ m[^(pod|tip)://(.*)] ) {
		$$url = undef;

		require Prima::PodView;
		my ($tip, $link) = ($1 eq 'tip', $2);
		my $topicView = ($link =~ m[/]) ? 1 : 0;

		# if it is a path or module? 
		if ( $link !~ /::/ && $link =~ m/^(.*?)\/([^\/]*)$/) {
			$link = "file://$1|$2";
		}

		my $pod = Prima::PodView->new(
			visible   => 0,
			size      => [ Prima::HintWidget-> max_extents ],
			topicView => $topicView,
			color     => $::application->hintColor,
			backColor => $::application->hintBackColor,
		);
		if ( $pod->load_link($link, createIndex => 0, format => 0) ) {
			if ( my $polyblock = $pod->export_blocks( trim_footer => 1, trim_header => $topicView || $tip) ) {
				$$url = $polyblock;
			}
		}
		$pod->destroy;
	} elsif ( $url =~ m[^(ftp|https?)://]) {
		# same
	}
}

sub on_mousedown
{
	my ( $self, $owner, $btn, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	return 1 if $r < 0;
	$r = $self-> {rectangles}-> [$r];
	$r = $self-> {references}-> [$$r[4]];
	$self->on_link( $owner, $r, $btn, $mod);
	return 0;
}

sub on_mousemove
{
	my ( $self, $owner, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	$self->{owner_pointer} //= $owner->pointer;
	$r = $self->rectangles->[$r]->[4] if $r >= 0;
	return if $r == $self-> {last_link_pointer}->[0];

	my $url     = ($r >= 0) ? $self->references->[$r] : '';
	my $new_ptr = ($r >= 0) ? (($url =~ /^tip:/ ? cr::QuestionArrow : cr::Hand)) : cr::Default;
	my $old_ptr = $self->{last_link_pointer}->[1];
	if ( $new_ptr != $old_ptr ) {
		$owner-> pointer(( $new_ptr == cr::Default) ? $self->{owner_pointer} : $new_ptr);
		delete $self->{owner_pointer} if $new_ptr == cr::Default;
	}

	my $rr = $self->rectangles;
	my $or = $self->{last_link_pointer}->[0];
	$self-> {last_link_pointer} = [$r, $new_ptr];

	my @around = (-1,-1,-1,-1);
	for my $rc (
		($or < 0) ? () : $self->id2rectangles( $or ),
		($r  < 0) ? () : $self->id2rectangles( $r  ),
	) {
		my @rc = @$rc;
		$owner-> notify(qw(LinkAdjustRect), $self, \@rc);
		$owner-> invalidate_rect(@rc[0..3]);
		@around = $owner->client_to_screen(@rc[0..3])
			if $rc[0] <= $x && $rc[1] <= $y && $rc[2] > $x && $rc[3] > $y;
	}

	if ( $r >= 0 ) {
		my $hint = $self-> {references}-> [$r];
		$self-> on_linkpreview( $owner, \$hint);
		goto NO_HINT unless length($hint // '');
		$owner->hint( $hint );
		$owner->showHint(1);
		$::application->set_hint_action($owner, 1, 1, @around);
	} else {
	NO_HINT:
		$owner->hint('');
		$owner->showHint(0);
	}
}

sub on_paint
{
	my ( $self, $owner, $canvas ) = @_;
	return if $self->{last_link_pointer} < 0;

	$canvas->graphic_context( sub {
		$canvas-> rop2(rop::NoOper);
		$canvas-> color( $self->color );
		$canvas-> antialias(0);
		$canvas-> lineWidth(1);
		$canvas-> translate(0,0);

		my $tip = ($self->references->[$self->{last_link_pointer}->[0]] // '') =~ /^tip:/;
		$canvas-> linePattern($tip ? lp::ShortDash : lp::Solid);

		for my $rc ( $self->id2rectangles( $self->{last_link_pointer}->[0] )) {
			my @rc = @$rc;
			$owner-> notify(qw(LinkAdjustRect), $self, \@rc);
			$rc[4] = $rc[1] < $rc[3] ? $rc[1] : $rc[3];
			$canvas-> line( @rc[0,4,2,4]);
		}
	});
}

sub clear_positions { shift->{rectangles} = [] }

sub add_positions_from_blocks
{
	my ( $self, $linkId, $blocks, %defaults ) = @_;
	my $linkState = 0;
	my $linkStart = 0;
	my @rect;
	my $rects = $self->{rectangles};
	$linkId //= 0;

	for my $b ( @$blocks ) {
		my @pos = ( $$b[tb::BLK_X], 0 );

		if ( $linkState) {
			$rect[0] = $$b[ tb::BLK_X];
			$rect[1] = $$b[ tb::BLK_Y];
		}

		tb::walk( $b, %defaults,
			position => \@pos,
			trace    => tb::TRACE_POSITION,
			link     => sub {
				if ( $linkState = shift ) {
					$rect[0] = $pos[0];
					$rect[1] = $$b[ tb::BLK_Y];
				} else {
					$rect[2] = $pos[0];
					$rect[3] = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
					push @$rects, [@rect, $linkId++];
				}
			},
		);

		if ( $linkState) {
			$rect[2] = $pos[0];
			$rect[3] = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
			push @$rects, [@rect, $linkId];
		}
	}

	return $linkId;
}

sub reset_positions_markup
{
	my ($self, $blocks, %defaults) = @_;
	$self->clear_positions;
	$self->add_positions_from_blocks(undef, [map { $_->text_block } @$blocks ], %defaults);
}

1;

=head1 NAME

Prima::Widget::Link - routines for interactive links

=head1 DESCRIPTION

The class can be used in widgets that need to feature links, i e a highlighted
rectangle, usually of text. When the user moves mouse or click on a link,
depending on the link type, various action can be executed. A "tooltip" link
type can display a hint with (rich) text, and a "hyperlink" link can open a
browser or pod viewer. The programmer can also customize these actions.

=head1 SYNOPSIS

  use Prima qw(Label Application);

  my $main_window = Prima::MainWindow->new( size => [400, 100] );
  $main_window->insert( Label =>
    centered => 1,
    text   => \ "L<tip://$0/ttt|tip>, L<pod://Prima/|podviewer>, L<http://google.com/|browser>, L<id|custom>",
    onLink => sub { print "$_[2]\n" },
  );
  run Prima;

  =pod

  =head1 ttt

  this is a tooltip

  =for podview <img src="data:base64">
  R0lGODdhFgAVAIAAAAAAAP///ywAAAAAFgAVAIAAAAD///8CLIyPqcutsKALQKI6qT11R69lWDJm
  5omm6jqBjucycEx+bVOSNNf1+/NjCREFADs=

=for podview <img src="link.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/Widget/link.gif">

=head1 Link types

Link types can be set with the I<url> syntax. There are four recognized link types that behave differently.

=head2 Tooltips

These are not really links in the strict sense, as clicking on them doesn't cause any action,
however when the user hovers mouse over a tooltip, the module loads the pod content from the url,
and displays it as a hint.

The idea behind this feature is to collect all tootip cards in a pod section, and referencing them
in a text like in the example code in L<SYNOPSIS> above.

Syntax: C<< LE<lt>tip://FILEPATH_OR_MODULE/SECTIONE<gt> >> or
C<< LE<lt>tip://FILEPATH_OR_MODULEE<gt> >> where C<FILEPATH_OR_MODULE> can refer either to
a file (path with slashes/backslashes) or a perl module (with C<::>s ).

Tooltip text, when selected, is underscored by a dashed line, vs all other link types that
draw solid line.

=head2 Pod sections

These links both diplay a pod section preview, like the toolkip, but also open a pod viewer
with the referred section when clicked on. 

Syntax: C<< LE<lt>pod://FILEPATH_OR_MODULE/SECTIONE<gt> >> or
C<< LE<lt>pod://FILEPATH_OR_MODULEE<gt> >> where C<FILEPATH_OR_MODULE> can refer either to
a file (path with slashes/backslashes) or a perl module (with C<::>s ).

=head2 Hyperlinks

Links with schemes C<ftp://>, C<http://>, and C<https://> open a browser when clicked on.

=head2 Custom links

All other urls, not matched by either scheme above, are expected to be handled programmatically.
The preview, if any is handled by the C<LinkPreview> event, and click by C<Link> event.

See L<Events> below.

=head1 Usage

Since C<Prima::Widget::Link> is not a widget by itself but a collection of routines in a class,
an object of such class should be instantiated programmatically and attached to a widget
that needs to display links, an I<owner> widget.

The owner needs to call mouse and paint methods from inside its C<on_mousedown> etc relevant events.
The owner might also want to overload link events, see below.

=head1 Markup

L<Prima::Drawable::Markup> understands the C<< LE<lt>..|..E<gt> >> command, which is, unlike perlpod, is formatted
with its arguments reversed, to stay consistent with the other markup commands (i e it is C<< LE<lt>http://google.com|searchE<gt> >>,
not C<< LE<lt>search|http://google.comE<gt> >> .

The simple way to incorporate rich text in both widget and link handler is to
use C<Prima::Drawable::Markup> to handle the markup parsing, and use the
resulting object from the same class both for widget drawing and for the link
reactions. One just needs to add C< markup => $markup_object > to C<
Prima::Widget::Link->new() >.

=head1 API

=head2 Properties

=over

=item rectangles

Contains array of rectangles in arbitrary coordinates that could be used to map screen coordinates to a url.
Filled automatically.

=item references

An array of URLs

=back

=head2 Methods

=over

=item add_positions_from_blocks LINK_ID, BLOCKS, %DEFAULTS

To be used when the link object is not bound to a markup object and link
rectangle recalculation is needed due to formatting change, f ex widget size,
font size etc. C<%DEFAULTS> is sent internally to C<tb::block_walk> that might
need eventual default parameters.

Scans BLOCKS and add monotonically increasing LINK_ID to new
link rectangles. Return new LINK_ID.

=item clear_positions

Clears the content of C<rectangles>

=item id2rectangles ID

Returns rectangles mapped to a link id. There can be more than 1 rectangle bound
to a single link ID since link text could be, f ex, wrapped.

=item open_podview URL

Opens a pod viewer with the URL

=item open_browser URL

Opens a web browser with the URL

=item reset_positions_markup BLOCKS, %DEFAULTS

To be used when the link object is bound to a markup object and link rectangle recalculation
is needed due to formatting change, f ex widget size, font size etc. C<%DEFAULTS> is sent
internally to C<tb::block_walk> that might need eventual default parameters.

=back

=head2 Events

All events are send to the owner, not to the link object itself, however the
C<SELF> parameter is sent and contains the link object.

=over

=item Link SELF, URL, BUTTON, MOD

Send to the owner, if any, from within C<on_mousedown> event, to indicate that
a link as pressed on.

=item LinkPreview SELF, URL_REF

Send to the owner, if any, from within C<on_mousemove> event.
The owner might want to fill URL_REF with (rich) text that will be displayed as 
a link preview

=item LinkAdjustRect SELF, RECT_REF

Since the owner might implement a scrollable view, or any other view that has a
coordinate system that is no necessary consistent with the rectangles stored in
the link object, this event will be called when a link rectangle needs to be
mapped to the owner coordinates.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Drawable::Markup>, L<Prima::Label>, L<Prima::PodView>

=cut
