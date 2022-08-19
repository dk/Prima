package Prima::Widget::Link;
use strict;
use warnings;
use Prima::Drawable::TextBlock;
use Prima::Drawable::Markup;

our $OP_LINK = tb::opcode(1, 'link');
sub op_link_enter { $OP_LINK, 1 }
sub op_link_leave { $OP_LINK, 0 }

sub markup_parse_link_syntax
{
	my ( $self, $mode, $command, $stacks, $state, $block, $url ) = @_;

	if ( $mode ) {
		push @{$stacks->{color}}, $state->{color};

		$state->{color} = $self->local_property('link')->color;
		push @$block,
			tb::color($state->{color}),
			op_link_enter,
			;
		push @{ $self->{references} }, $url;
	} else {
		push @$block,
			op_link_leave,
			tb::color($state->{color}),
			;
	}
}

sub notification_types {{ Link => nt::Default }}
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
	return $self;
}

sub parse_markup
{
	my ( $class, $text ) = @_;

	my $self   = $class->new;
	my $markup = Prima::Drawable::Markup->new(
		Prima::Drawable::Markup->defaults,
		local_syntax   => { L => [1, 1, \&markup_parse_link_syntax ] },
	);
	$markup-> local_property( link => $self );
	$markup-> markup($text);
	return $markup;
}

sub markup_has_links { scalar @{ $_[0]->{references} } }

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
	$_[0]-> {color} = $_[1];
}

sub on_mousedown
{
	my ( $self, $owner, $btn, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	return 1 if $r < 0;
	$r = $self-> {rectangles}-> [$r];
	$r = $self-> {references}-> [$$r[4]];
	$owner->notify(qw(Link), $r, $btn, $mod, $x, $y);
	return 0;
}

sub on_mousemove
{
	my ( $self, $owner, $dx, $dy, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	$self->{owner_pointer} //= $owner->pointer;
	if ( $r != $self-> {last_link_pointer}) {
		my $was_hand = ($self->{last_link_pointer} >= 0) ? 1 : 0;
		my $is_hand  = ($r >= 0) ? 1 : 0;
		if ( $is_hand != $was_hand) {
			$owner-> pointer( $is_hand ? cr::Hand : $self->{owner_pointer} );
		}
		my $rr = $self->rectangles;
		my $or = $self->{last_link_pointer};
		$owner-> {last_link_pointer} = $r;
		if ( $was_hand ) {
			$or = $rr->[$or];
			$owner-> invalidate_rect($or->[0] + $dx, $dy - $or->[1], $or->[2] + $dx, $dy - $or->[3]);
		}
		if ( $is_hand ) {
			$or = $rr->[$r];
			$owner-> invalidate_rect($or->[0] + $dx, $dy - $or->[1], $or->[2] + $dx, $dy - $or->[3]);
		}
	}
}

sub on_mouseup   {}

sub on_paint
{
	my ( $self, $owner, $dx, $dy, $canvas ) = @_;
	return if $self->{last_link_pointer} < 0;

	my $r  = $self->rectangles->[ $self->{last_link_pointer} ];
	$canvas->graphic_context( sub {
		$canvas-> color( $self->color );
		$canvas-> translate(0,0);
		$canvas-> line( $r->[0] + $dx, $dy - $r->[3], $r->[2] + $dx, $dy - $r->[3]);
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
