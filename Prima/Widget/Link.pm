package Prima::Widget::Link;
use strict;
use warnings;
use Prima::Drawable::TextBlock;
use Prima::Drawable::Markup;

sub notification_types {{
	Link        => nt::Default,
	LinkPreview => nt::Action,
}}
sub profile_default    {{ color => cl::Green  }}

sub new
{
	my $class = shift;
	my %profile = @_;
	my $self = {
		last_link_pointer => -1,
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

sub filter_rectangles_by_link_id
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
	return if $btn != mb::Left;

	$::application->open_help($url);
}

sub open_browser
{
	my ( $self, $url, $btn, $mod ) = @_;
	return 0 if $btn != mb::Left;

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
		return -1 unless defined $pg && ! system( "$pg $url &");
	}

	return 1;
}

sub on_link
{
	my ( $self, $owner, $url, $btn, $mod ) = @_;

	if ( $url =~ m[^pod://(.*)] ) {
		$self->open_podview($1, $btn, $mod);
	} elsif ( $url =~ m[^(ftp|https?)://]) {
		$self->open_browser($url, $btn, $mod);
	} else {
		$owner->notify(q(Link), $url, $btn, $mod);
	}
}

sub on_linkpreview
{
	my ( $self, $owner, $url ) = @_;

	if ( $$url =~ m[^pod://(.*)] ) {
		$$url = undef;

		require Prima::PodView;
		my $link = $1;
		my $topicView = ($link =~ m[/]) ? 1 : 0;
		my $pod = Prima::PodView->new(
			visible   => 0,
			size      => [ Prima::HintWidget-> max_extents ],
			topicView => $topicView,
		);
		if ( $pod->load_link($link, createIndex => 0, format => 0) ) {
			if ( my $polyblock = $pod->export_blocks( trim_footer => 1, trim_header => $topicView ) ) {
				$$url = $polyblock;
			}
		}
		$pod->destroy;
	} elsif ( $url =~ m[^(ftp|https?)://]) {
		# same
	} else {
		$owner->notify(q(LinkPreview), $url);
	}
}

sub on_mousedown
{
	my ( $self, $owner, $dx, $dy, $btn, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	return 1 if $r < 0;
	$r = $self-> {rectangles}-> [$r];
	$r = $self-> {references}-> [$$r[4]];
	$self->on_link( $owner, $r, $btn, $mod);
	return 0;
}

sub on_mousemove
{
	my ( $self, $owner, $dx, $dy, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	$self->{owner_pointer} //= $owner->pointer;
	$r = $self->rectangles->[$r]->[4] if $r >= 0;
	return if $r == $self-> {last_link_pointer};

	my $was_hand = ($self->{last_link_pointer} >= 0) ? 1 : 0;
	my $is_hand  = ($r >= 0) ? 1 : 0;
	if ( $is_hand != $was_hand) {
		$owner-> pointer( $is_hand ? cr::Hand : $self->{owner_pointer} );
		delete $self->{owner_pointer} unless $is_hand;
	}

	my $rr = $self->rectangles;
	my $or = $self->{last_link_pointer};
	$self-> {last_link_pointer} = $r;
	return if $was_hand == $is_hand;

	my $rx = $was_hand ? $or : $r;
	for my $rc ( $self->filter_rectangles_by_link_id( $rx )) {
		$owner-> invalidate_rect($rc->[0] + $dx, $rc->[1] + $dy, $rc->[2] + $dx, $rc->[3] + $dy);
	}
	if ( $is_hand ) {
		my $hint =$self-> {references}-> [$r];
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

sub on_mouseup   {}

sub on_paint
{
	my ( $self, $owner, $dx, $dy, $canvas ) = @_;
	return if $self->{last_link_pointer} < 0;

	$canvas->graphic_context( sub {
		$canvas-> color( $self->color );
		$canvas-> translate(0,0);
		for my $rc ( $self->filter_rectangles_by_link_id( $self->{last_link_pointer} )) {
			$canvas-> line( $rc->[0] + $dx, $rc->[1] + $dy, $rc->[2] + $dx, $rc->[1] + $dy);
		}
	});
}

sub reset_positions_blocks
{
	my ( $self, $blocks, %defaults ) = @_;
	my $linkState = 0;
	my $linkStart = 0;
	my $linkId    = 0;
	my @rect;
	my $rects = $self->{rectangles};
	@$rects = ();

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
}

sub reset_positions_markup
{
	my ($self, $blocks, %defaults) = @_;
	$self->reset_positions_blocks([map { $_->text_block } @$blocks ], %defaults);
}

1;
