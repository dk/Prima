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

	for my $rc (
		($or < 0) ? () : $self->id2rectangles( $or ),
		($r  < 0) ? () : $self->id2rectangles( $r  ),
	) {
		my @rc = @$rc;
		$owner-> notify(qw(LinkAdjustRect), $self, \@rc);
		$owner-> invalidate_rect(@rc[0..3]);
	}

	if ( $r >= 0 ) {
		my $hint = $self-> {references}-> [$r];
		$self-> on_linkpreview( $owner, \$hint);
		goto NO_HINT unless length($hint // '');
		$owner->hint( $hint );
		$owner->showHint(1);
		$::application->set_hint_action($owner, 1, 1);
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
		$canvas-> color( $self->color );
		$canvas-> translate(0,0);

		my $tip = ($self->references->[$self->{last_link_pointer}->[0]] // '') =~ /^tip:/;
		$canvas-> linePattern($tip ? lp::ShortDash : lp::Solid);

		for my $rc ( $self->id2rectangles( $self->{last_link_pointer}->[0] )) {
			my @rc = @$rc;
			$owner-> notify(qw(LinkAdjustRect), $self, \@rc);
			$canvas-> line( @rc[0,3,2,3]);
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
