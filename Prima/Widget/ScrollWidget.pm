package Prima::Widget::ScrollWidget;
use strict;
use warnings;
use Prima;
use base qw(Prima::Widget Prima::Widget::GroupScroller);
__PACKAGE__->inherit_core_methods('Prima::Widget::GroupScroller');

{
my %RNT = (
	%{Prima::Widget->notification_types()},
	Scroll      => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		borderWidth    => 0,
		deltaX         => 0,
		deltaY         => 0,
		limitX         => 0,
		limitY         => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	for ( qw( scrollTransaction limitX limitY deltaX deltaY borderWidth winX winY))
		{ $self->{$_} = 0; }
	my %profile = $self-> SUPER::init(@_);
	$self-> setup_indents;
	$self-> limits( $profile{limitX}, $profile{limitY});
	$self-> deltas( $profile{deltaX}, $profile{deltaY});
	$self-> reset_scrolls;
	return %profile;
}

sub reset_scrolls
{
	my $self = $_[0];

	# protect against the worst case where it goes like this:
	# vScroll(0) -> hScroll(1) -> vScroll(0) -> hScroll(1) -> ... etc
	# which means that if we're deeper than 4 calls, we're never out of it
	local $self->{reentrant_scroll_calls} = 1 + (exists($self->{reentrant_scroll_calls}) ? $self->{reentrant_scroll_calls} : 0);
	return if $self->{reentrant_scroll_calls} > 4;

	my ($x, $y) = $self-> get_active_area(2);
	my ($w, $h) = $self-> limits;
	my $reread;
	@{$self}{qw(winX winY)} = ($x, $y);


	if ( $self-> {autoHScroll} and $self->{autoVScroll} and
			( $self-> {hScroll} or $self-> {vScroll})
		) {
		# avoid the special case when two scrollbars are unnecessary, but are present
		# since they obscure parts of the panel that would have been visible fully,
		# if not for the scrollbars
		my $dx = $self->{vScroll} ? $Prima::ScrollBar::stdMetrics[0] : 0;
		my $dy = $self->{hScroll} ? $Prima::ScrollBar::stdMetrics[1] : 0;
		if ( $x + $dx >= $w and $y + $dy >= $h) {
			$self-> hScroll(0) if $self->{hScroll};
			$self-> vScroll(0) if $self->{vScroll};
			@{$self}{qw(winX winY)} = $self-> get_active_area(2);
			$self-> set_deltas( $self->{deltaX}, $self->{deltaY});
			return;
		}
	}

	if ( $self-> {autoHScroll}) {
		my $hs = ( $x < $w) ? 1 : 0;
		if ( $hs != $self-> {hScroll}) {
			$self-> hScroll( $hs);
			$reread = 1;
		}
	}
	if ( $self-> {autoVScroll}) {
		if ( $reread) {
			@{$self}{qw(winX winY)} = ($x, $y) = $self-> get_active_area(2);
			$reread = 0;
		}
		my $vs = ( $y < $h) ? 1 : 0;
		if ( $vs != $self-> {vScroll}) {
			$self-> vScroll( $vs);
			$reread = 1;
		}
	}
	if ( $reread) {
		@{$self}{qw(winX winY)} = ($x, $y) = $self-> get_active_area(2);
	}

	if ( $self-> {hScroll}) {
		$self-> {hScrollBar}-> set(
			max     => $x < $w ? $w - $x : 0,
			whole   => $w,
			partial => $x < $w ? $x : $w,
		);
	}
	if ( $self-> {vScroll}) {
		$self-> {vScrollBar}-> set(
			max     => $y < $h ? $h - $y : 0,
			whole   => $h,
			partial => $y < $h ? $y : $h,
		);
	}
	$self-> set_deltas( $self->{deltaX}, $self->{deltaY});
}

sub set_limits
{
	my ( $self, $w, $h) = @_;
	$w = 0 if $w < 0;
	$h = 0 if $h < 0;
	$w = int( $w);
	$h = int( $h);
	return if $w == $self-> {limitX} and $h == $self->{limitY};

	$self-> {limitY} = $h;
	$self-> {limitX} = $w;
	$self-> reset_scrolls;
}

sub set_deltas
{
	my ( $self, $w, $h) = @_;
	my ($odx,$ody) = ($self->{deltaX},$self->{deltaY});
	$w = 0 if $w < 0;
	$h = 0 if $h < 0;
	$w = int( $w);
	$h = int( $h);
	my ($x, $y) = $self-> limits;
	my @sz = $self-> size;
	my ( $ww, $hh) = $self-> get_active_area( 2, @sz);
	$x -= $ww;
	$y -= $hh;
	$x = 0 if $x < 0;
	$y = 0 if $y < 0;

	$w = $x if $w > $x;
	$h = $y if $h > $y;
	return if $w == $odx && $h == $ody;
	$self-> {deltaY} = $h;
	$self-> {deltaX} = $w;
	$self-> notify('Scroll', $odx - $w, $h - $ody);
	$self-> {scrollTransaction} = 1;
	$self-> {hScrollBar}-> value( $w) if $self->{hScroll};
	$self-> {vScrollBar}-> value( $h) if $self->{vScroll};
	$self-> {scrollTransaction} = undef;
}

sub on_scroll
{
	my ( $self, $dx, $dy) = @_;
	$self-> scroll( $dx, $dy, clipRect => [$self->get_active_area(0)]);
}

sub on_size
{
	$_[0]-> reset_scrolls;
}

sub VScroll_Change
{
	$_[0]-> deltaY( $_[1]-> value) unless $_[0]-> {scrollTransaction};
}

sub HScroll_Change
{
	$_[0]-> deltaX( $_[1]-> value) unless $_[0]-> {scrollTransaction};
}

sub limitX        {($#_)?$_[0]->set_limits($_[1],$_[0]->{limitY}):return $_[0]->{'limitX'};  }
sub limitY        {($#_)?$_[0]->set_limits($_[0]->{'limitX'},$_[1]):return $_[0]->{'limitY'};  }
sub limits        {($#_)?$_[0]->set_limits         ($_[1], $_[2]):return ($_[0]->{'limitX'},$_[0]->{'limitY'});}
sub deltaX        {($#_)?$_[0]->set_deltas($_[1],$_[0]->{deltaY}):return $_[0]->{'deltaX'};  }
sub deltaY        {($#_)?$_[0]->set_deltas($_[0]->{'deltaX'},$_[1]):return $_[0]->{'deltaY'};  }
sub deltas        {($#_)?$_[0]->set_deltas         ($_[1], $_[2]):return ($_[0]->{'deltaX'},$_[0]->{'deltaY'}); }


package Prima::Widget::ScrollGroup;
use vars qw(@ISA);
@ISA = qw(Prima::Widget::ScrollWidget);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		rigid      		=> 1,
		clientSize		=> [100, 100],
		slaveClass		=> 'Prima::Widget',
		slaveProfile		=> {},
		slaveDelegations	=> [],
		clientClass		=> 'Prima::Widget::ScrollGroup::Client',
		clientProfile		=> {},
		clientDelegations	=> [],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	if ( exists $p-> {clientSize}) {
		$p-> {rigid} = 0 unless exists $p-> {rigid};
		$p-> {clientProfile}->{geometry} = gt::Default
			unless exists $p-> {clientProfile}->{geometry};
	}
}

sub init
{
	my ($self, %profile) = @_;
	%profile = $self-> SUPER::init(%profile);

	$self-> {$_} = 0 for qw(rigid);
	$self-> $_( $profile{$_}) for qw(rigid);

	$self-> {slave} = $profile{slaveClass}-> new(
		delegations => $profile{slaveDelegations},
		%{$profile{slaveProfile}},
		owner => $self,
		name => 'SlaveWindow',
		rect  => [ $self-> get_active_area(0) ],
#		growMode => gm::Client, do that manually because client's own resize causes changes too
	);

	$self-> {client_geomSize} = [0,0];
	$self-> {client} = $profile{clientClass}-> new(
		delegations => [ $self, 'Size', $self, 'Move', @{$profile{clientDelegations}}],
		( $profile{rigid} ? () : (
			origin => [0,0],
			size   => $profile{clientSize})
		),
		%{$profile{clientProfile}},
		owner => $self-> {slave},
		name  => 'ClientWindow',
		designScale  => undef,
		scaleChildren => $profile{scaleChildren},
	);

	$self-> {client}-> designScale( $self-> designScale);
	$self-> reset(1);

	return %profile;
}

sub reset_indents
{
	$_[0]-> reset(1);
}

sub ClientWindow_Size
{
	$_[0]-> reset unless $_[0]->{client_notification_block};
}

sub ClientWindow_Move
{
	$_[0]-> reset unless $_[0]->{client_notification_block};
}

sub ClientWindow_geomSize
{
	my ( $self, $client, $x, $y) = @_;
	$client-> sizeMin( $x, $y) if $self-> rigid;
	$self-> update_geom_size( $x, $y);
}

sub packPropagate
{
	return shift-> SUPER::packPropagate unless $#_;
	my ( $self, $pack_propagate) = @_;
	$self-> SUPER::packPropagate( $pack_propagate);
	$self-> propagate_size if $pack_propagate;
}

sub propagate_size
{
	my $self = $_[0];
	$self-> update_geom_size( $self-> {client}-> geomSize)
		if $self-> {client};
}

sub reset
{
	my ( $self, $forced) = @_;
	return unless $self-> {client};

	my @size = $self-> size;
	if ($forced) {
		local $self->{client_notification_block} = 1;
		$self-> {slave}-> rect( $self-> get_active_area(0, @size));
	}

	my @l = $self-> limits;
	my @s = $self-> {client}-> size;
	my @o = $self-> {client}-> origin;

	local $self-> {protect_scrolling} = 1;
	( $l[0] == $s[0] and $l[1] == $s[1]) ?
		$self-> reset_scrolls :
		$self-> limits( $s[0], $s[1]);
	$self-> deltas( -$o[0], $o[1] - $self-> {slave}-> height + $s[1]);
}

sub children_extensions
{
	my $self = $_[0];
	my @ext = ( 0,0 );
	for my $w ( $self-> {client}-> widgets) {
		my @r = $w-> rect;
		$r[2] -= $r[0] if $r[0] < 0;
		$r[3] -= $r[1] if $r[1] < 0;
		$ext[0] = $r[2] if $ext[0] < $r[2];
		$ext[1] = $r[3] if $ext[1] < $r[3];
	}
	return @ext;
}

sub update_geom_size
{
	my ( $self, $x, $y) = @_;
	return unless $self-> packPropagate;
	my @i = $self-> indents;
	$self-> geomSize(
		$x + $i[0] + $i[2],
		$y + $i[1] + $i[3]
	);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	$self-> draw_border( $canvas, $self-> backColor, $self-> size );
}

sub on_size
{
	$_[0]-> reset(1);
}

sub on_scroll
{
	my ( $self, $dx, $dy) = @_;
	return if $self-> {protect_scrolling};
	local $self-> {protect_scrolling} = 1;
	my @o = $self-> {client}-> origin;
	$self-> {client}-> origin(
		$o[0] + $dx,
		$o[1] + $dy,
	);
}

sub slave { $_[0]-> {slave} }
sub client { $_[0]-> {client} }
sub insert { shift-> {client}-> insert( @_ ) }

sub rigid
{
	return $_[0]-> {rigid} unless $#_;

	my ( $self, $rigid) = @_;
	return if $self-> {rigid} == $rigid;
	$self-> {rigid} = $rigid;
	$self-> reset if $rigid;
}

sub clientSize
{
	return $_[0]-> {client}-> size unless $#_;
	shift-> {client}-> size(@_);
}

sub use_current_size
{
	$_[0]-> {client}-> sizeMin( $_[0]-> children_extensions);
}

package Prima::Widget::ScrollGroup::Client;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		geometry  => gt::Pack,
		packInfo  => { expand => 1, fill => 'both'},
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub geomSize
{
	return $_[0]-> SUPER::geomSize unless $#_;
	my $self = shift;
	$self-> SUPER::geomSize( @_);
	$self-> owner-> owner-> ClientWindow_geomSize( $self, @_);
}


1;

=pod

=head1 NAME

Prima::Widget::ScrollWidget - scrollable generic document widget.

=head1 DESCRIPTION

C<Prima::Widget::ScrollWidget> is a simple class that declares two pairs of properties,
I<delta> and I<limit> for vertical and horizontal axes, which define a
a virtual document. I<limit> is the document dimension, and I<delta> is
the current offset.

C<Prima::Widget::ScrollWidget> is a descendant of C<Prima::Widget::GroupScroller>, and, as well as its
ascendant, provides same user navigation by two scrollbars. The scrollbars' C<partial>
and C<whole> properties are maintained if the document or widget extensions change.

C<Prima::Widget::ScrollGroup> in addition provides capability to host other widgets inside, and
scroll them. Useful for widget group panels that cannot. fit in window

=head1 Prima::Widget::ScrollWidget

=head2 Properties

=over

=item deltas X, Y

Selects horizontal and vertical document offsets.

=item deltaX INTEGER

Selects horizontal document offset.

=item deltaY INTEGER

Selects vertical document offset.

=item limits X, Y

Selects horizontal and vertical document extensions.

=item limitX INTEGER

Selects horizontal document extension.

=item limitY INTEGER

Selects vertical document extension.

=back

=head2 Events

=over

=item Scroll DX, DY

Called whenever the client area is to be scrolled. The default
action calls C<Widget::scroll> .

=back

=head1 Prima::Widget::ScrollGroup

=head2 Properties

=over

=item client

Return a parent widget to insert other widgets to. The client size is fixed,
and is panned through the slave widget when scrolling. The client is unaffected by
eventual automated pack/place/growMode size alteration the parent or slave might have.

=item clientClass

C<clientClass> widget is inserted in the C<slave> widget.

=item slave

Returns the slave widget. The slave widget designated the area desired to be
scrollable, and is just a normal widget that is allowed to be resized, moved,
etc.

=item slaveClass

C<slaveClass> widget is inserted directly in the scroll group widget.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::ImageViewer>, L<Prima::ScrollBar>, F<examples/e.pl>.


=cut
