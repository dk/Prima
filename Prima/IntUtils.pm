package Prima::IntUtils;

use strict;
use warnings;
use Prima::Const;

package Prima::MouseScroller;

my $scrollTimer;

sub scroll_timer_active
{
	return 0 unless defined $scrollTimer;
	return $scrollTimer-> {active};
}

sub scroll_timer_semaphore
{
	return 0 unless defined $scrollTimer;
	$#_ ?
		$scrollTimer-> {semaphore} = $_[1] :
		return $scrollTimer-> {semaphore};
}

sub scroll_timer_stop
{
	return unless defined $scrollTimer;
	$scrollTimer-> stop;
	$scrollTimer-> {active} = 0;
	$scrollTimer-> timeout( $scrollTimer-> {firstRate});
	$scrollTimer-> {newRate} = $scrollTimer-> {nextRate};
}

sub scroll_timer_start
{
	my $self = $_[0];
	$self-> scroll_timer_stop;
	unless ( defined $scrollTimer) {
		my @rates = $::application-> get_scroll_rate;
		$scrollTimer = Prima::Timer-> create(
			owner      => $::application,
			timeout    => $rates[0],
			name       => q(ScrollTimer),
			onTick     => sub { $_[0]-> {delegator}-> ScrollTimer_Tick( @_)},
			onDestroy  => sub { undef $scrollTimer },
		);
		@{$scrollTimer}{qw(firstRate nextRate newRate)} = (@rates,$rates[1]);
	}
	$scrollTimer-> {delegator} = $self;
	$scrollTimer-> {semaphore} = 1;
	$scrollTimer-> {active} = 1;
	$scrollTimer-> start;
}

sub ScrollTimer_Tick
{
	my ( $self, $timer) = @_;
	if ( exists $scrollTimer-> {newRate})
	{
		$timer-> timeout( $scrollTimer-> {newRate});
		delete $scrollTimer-> {newRate};
	}
	$scrollTimer-> {semaphore} = 1;
	$self-> notify(q(MouseMove), 0, $self-> pointerPos);
	$self-> scroll_timer_stop unless defined $self-> {mouseTransaction};
}

package Prima::IntIndents;

sub indents
{
	return wantarray ? @{$_[0]-> {indents}} : [@{$_[0]-> {indents}}] unless $#_;
	my ( $self, @indents) = @_;
	@indents = @{$indents[0]} if ( scalar(@indents) == 1) && ( ref($indents[0]) eq 'ARRAY');
	for ( @indents) {
		$_ = 0 if $_ < 0;
	}
	$self-> {indents} = \@indents;
}

sub get_active_area
{
	my @r = ( scalar @_ > 2) ? @_[2,3] : $_[0]-> size;
	my $i = $_[0]-> {indents};
	if ( !defined($_[1]) || $_[1] == 0) {
		# returns inclusive - exclusive
		return $$i[0], $$i[1], $r[0] - $$i[2], $r[1] - $$i[3];
	} elsif ( $_[1] == 1) {
		# returns inclusive - inclusive
		return $$i[0], $$i[1], $r[0] - $$i[2] - 1, $r[1] - $$i[3] - 1;
	} else {
		# returns size
		return $r[0] - $$i[0] - $$i[2], $r[1] - $$i[1] - $$i[3];
	}
}

package Prima::GroupScroller;
use vars qw(@ISA);
@ISA = qw(Prima::IntIndents);

use Prima::ScrollBar;

sub setup_indents
{
	my ($self) = @_;
	$self-> {indents} = [ 0,0,0,0];
	my $bw = $self-> {borderWidth};
	$self-> {indents}-> [$_] += $bw for 0..3;
	$self-> {indents}-> [1] += $self-> {hScrollBar}-> height - 1 if $self-> {hScroll};
	$self-> {indents}-> [2] += $self-> {vScrollBar}-> width - 1 if $self-> {vScroll};
}

sub set_border_width
{
	my ( $self, $bw) = @_;

	my @size = $self-> size;
	$bw = 0 if $bw < 0;
	$bw = 1 if $bw > $size[1] / 2;
	$bw = 1 if $bw > $size[0] / 2;
	return if $bw == $self-> {borderWidth};

	my $obw  = $self-> {borderWidth};
	$self-> {borderWidth} = $bw;

	$self-> {hScrollBar}-> set(
		left   => $bw - 1,
		bottom => $bw - 1,
		width  => $size[0] -
			$bw * 2 +
			2 -
			( $self-> {vScroll} ?
				$self-> {vScrollBar}-> width - 2 :
				0
			),
	) if $self-> {hScroll};

	$self-> {vScrollBar}-> set(
		top    => $size[1] - $bw + 1,
		right  => $size[0] - $bw + 1,
		bottom => $bw + ( $self-> {hScroll} ?
			$self-> {hScrollBar}-> height - 2 :
			0
		),
	) if $self-> {vScroll};

	$self-> insert_bone if defined $self-> {bone};

	$self-> setup_indents;
	$self-> reset_indents;
}

sub reset_indents {}

sub insert_bone
{
	my $self = $_[0];
	my $bw   = $self-> {borderWidth};
	$self-> {bone}-> destroy if defined $self-> {bone};

	$self-> {bone} = Prima::Widget-> new(
		owner  => $self,
		name   => q(Bone),
		pointerType => cr::Arrow,
		origin => [ $self-> width - $self-> {vScrollBar}-> width + 3 - $bw, $bw - 1],
		size   => [ $self-> {vScrollBar}-> width-2, $self-> {hScrollBar}-> height-1],
		growMode  => gm::GrowLoX,
		widgetClass => wc::ScrollBar,
		designScale => undef,
		onPaint   => sub {
			my ( $self, $canvas, $owner, $w, $h) =
				($_[0], $_[1], $_[0]-> owner, $_[0]-> size);
			$canvas-> color( $self-> backColor);
			$canvas-> bar( 0, 1, $w - 2, $h - 1);
			$canvas-> color( $owner-> light3DColor);
			$canvas-> line( 0, 0, $w - 1, 0);
			$canvas-> line( $w - 1, 0, $w - 1, $h - 1);
		},
	);
}

sub set_h_scroll
{
	my ( $self, $hs) = @_;
	return if ($hs ? 1 : 0) == $self-> {hScroll};
	my $bw = $self-> {borderWidth} || 0;
	if ( $hs) {
		$self-> {hScrollBar} = $self->{scrollBarClass}-> new(
			owner       => $self,
			name        => q(HScroll),
			vertical    => 0,
			origin      => [ $bw-1, $bw-1],
			growMode    => gm::GrowHiX,
			pointerType => cr::Arrow,
			width       => $self-> width -
				2 * $bw + 2 -
				( $self-> {vScroll} ?
					$self-> {vScrollBar}-> width - 2 :
					0),
			delegations => ['Change'],
			designScale => undef,
			%{ $self->{hScrollBarProfile} || {} },
		);
		$self-> {hScroll} = 1;

		$self-> setup_indents;

		if ( $self-> {vScroll}) {
			my $h = $self-> {hScrollBar}-> height;
			$self-> {vScrollBar}-> set(
				bottom => $self-> {vScrollBar}-> bottom + $h - 2,
				top    => $self-> {vScrollBar}-> top,
			);
			$self-> insert_bone;
		}
	} else {
		$self-> {hScroll} = 0;
		$self-> setup_indents;
		$self-> {hScrollBar}-> destroy;

		if ( $self-> {vScroll})
		{
			$self-> {vScrollBar}-> set(
				bottom => $bw,
				height => $self-> height - $bw * 2,
			);
			$self-> {bone}-> destroy;
			delete $self-> {bone};
		}
	}
	$self-> reset_indents;
}

sub set_v_scroll
{
	my ( $self, $vs) = @_;
	return if ($vs ? 1 : 0) == $self-> {vScroll};

	my $bw = $self-> {borderWidth} || 0;
	my @size = $self-> size;

	if ( $vs) {
		my $width = exists( $self->{vScrollBarProfile}->{width} ) ?
			$self->{vScrollBarProfile}->{width} :
			$Prima::ScrollBar::stdMetrics[0];
		$self-> {vScrollBar} = $self->{scrollBarClass}-> new(
			owner    => $self,
			name     => q(VScroll),
			vertical => 1,
			left     => $size[0] - $bw - $width + 1,
			top      => $size[1] - $bw + 1,
			bottom   => $bw + ( $self-> {hScroll} ? $self-> {hScrollBar}-> height - 2 : 0),
			growMode => gm::GrowLoX | gm::GrowHiY,
			pointerType  => cr::Arrow,
			delegations  => ['Change'],
			designScale => undef,
			%{ $self->{vScrollBarProfile} || {} },
		);
		$self-> {vScroll} = 1;

		$self-> setup_indents;

		if ( $self-> {hScroll}) {
			$self-> {hScrollBar}-> width(
				$self-> {hScrollBar}-> width -
				$self-> {vScrollBar}-> width + 2,
			);
			$self-> insert_bone;
		}
	} else {
		$self-> {vScroll} = 0;
		$self-> setup_indents;
		$self-> {vScrollBar}-> destroy;
		if ( $self-> {hScroll})
		{
			$self-> {hScrollBar}-> width( $size[0] - 2 * $bw + 2);
			$self-> {bone}-> destroy;
			delete $self-> {bone};
		}
	}
	$self-> reset_indents;
}

sub autoHScroll
{
	return $_[0]-> {autoHScroll} unless $#_;
	my $v = ( $_[1] ? 1 : 0);
	return unless $v != $_[0]-> {autoHScroll};
	$_[0]-> {autoHScroll} = $v;
}

sub autoVScroll
{
	return $_[0]-> {autoVScroll} unless $#_;
	my $v = ( $_[1] ? 1 : 0);
	return unless $v != $_[0]-> {autoVScroll};
	$_[0]-> {autoVScroll} = $v;
}

sub borderWidth     {($#_)?($_[0]-> set_border_width( $_[1])):return $_[0]-> {borderWidth}}
sub hScroll         {($#_)?$_[0]-> set_h_scroll       ($_[1]):return $_[0]-> {hScroll}}
sub vScroll         {($#_)?$_[0]-> set_v_scroll       ($_[1]):return $_[0]-> {vScroll}}

sub draw_border
{
	my ( $self, $canvas, $backColor, @size) = @_;

	@size = $self-> size unless @size;
	$self-> rect_bevel(
		$canvas,
		0, 0,
		$size[0]-1, $size[1]-1,
		width => $self-> {borderWidth},
		panel => 1,
		fill  => $backColor,
	);
}

package Prima::UndoActions;

sub init_undo
{
	my ($self, $profile) = @_;
	$self-> {undo} = [];
	$self-> {redo} = [];
	$self-> {undoLimit} = $profile->{undoLimit};
}

sub begin_undo_group
{
	my $self = $_[0];
	return if !$self-> {undoLimit};
	if ( $self-> {undo_in_action}) {
		push @{$self-> {redo}}, [] unless $self-> {grouped_undo}++;
	} else {
		push @{$self-> {undo}}, [] unless $self-> {grouped_undo}++;
		$self-> {redo} = [] if !$self-> {redo_in_action};
	}
}

sub end_undo_group
{
	my $self = $_[0];
	return if !$self-> {undoLimit};

	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	$self-> {grouped_undo}-- if $self-> {grouped_undo} > 0;
	# skip last record if empty
	pop @{$self-> {$ref}}
		if !$self-> {grouped_undo} &&
			@{$self-> {$ref}} &&
			0 == @{$self-> {$ref}-> [-1]};
	shift @{$self-> {$ref}} if @{$self-> {$ref}} > $self-> {undoLimit};
}

sub push_undo_action
{
	my $self = shift;
	return if !$self-> {undoLimit};

	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	my $action = [ @_ ];
	if ( $self-> {grouped_undo}) {
		push @{$self-> {$ref}}, [] unless @{$self-> {$ref} // []};
		push @{$self-> {$ref}-> [-1]}, $action;
	} else {
		push @{$self-> {$ref}}, [ $action ];
		shift @{$self-> {$ref}} if @{$self-> {$ref}} > $self-> {undoLimit};
		$self-> {redo} = []
			if !$self-> {redo_in_action} && !$self-> {undo_in_action};
	}
}

sub has_undo_action
{
	my ($self, $method) = @_;
	my $has = 0;
	if ( !$self-> {undo_in_action} && @{$self-> {undo} // []} && @{$self-> {undo}-> [-1]}) {
		my $ok = 1;
		for ( @{$self-> {undo}-> [-1]}) {
			$ok = 0, last if $$_[0] ne $method;
		}
		$has = 1 if $ok;
	}
	return $has;
}

sub push_group_undo_action
{
	my $self = shift;
	return if !$self-> {undoLimit};
	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	return $self-> push_undo_action(@_) if $self-> {grouped_undo};

	push @{$self-> {$ref}}, [] unless @{$self-> {$ref}};
	$self-> {grouped_undo} = 1;
	$self-> push_undo_action(@_);
	$self-> {grouped_undo} = 0;
}

sub can_undo { scalar shift @{ shift->{undo} } }
sub can_redo { scalar shift @{ shift->{redo} } }

sub undo
{
	my $self = $_[0];
	return if $self-> {undo_in_action} || !$self-> {undoLimit};
	return unless @{$self-> {undo}};
	my $group = pop @{$self-> {undo}};
	return unless $group && @$group;

	$self-> {undo_in_action} = 1;
	$self-> begin_undo_group;
	for ( reverse @$group) {
		my ( $method, @params) = @$_;
		next unless $self-> can($method);
		$self-> $method( @params);
	}
	$self-> end_undo_group;
	$self-> {undo_in_action} = 0;
}

sub redo
{
	my $self = $_[0];
	return if !$self-> {undoLimit};
	return unless @{$self-> {redo}};
	my $group = pop @{$self-> {redo}};
	return unless $group && @$group;

	$self-> {redo_in_action} = 1;
	$self-> begin_undo_group;
	for ( reverse @$group) {
		my ( $method, @params) = @$_;
		next unless $self-> can($method);
		$self-> $method( @params);
	}
	$self-> end_undo_group;
	$self-> {redo_in_action} = 0;
}

sub undoLimit
{
	return $_[0]-> {undoLimit} unless $#_;

	my ( $self, $ul) = @_;
	$self-> {undoLimit} = $ul if $ul >= 0;
	splice @{$self-> {undo}}, 0, $ul - @{$self-> {undo}} if @{$self-> {undo}} > $ul;
}

package Prima::ListBoxUtils;

sub draw_item_background
{
	my ( $self, $canvas, $left, $bottom, $right, $top, $prelight, $back_color ) = @_;
	if ( $prelight ) {
		$back_color //= $canvas-> backColor;
		my $c = $self-> color;
		$canvas-> new_gradient(
			spline   => [ 0.75, 0.25 ],
			palette  => [ $self->prelight_color($back_color), $back_color ],
		)-> bar( $left, $bottom, $right, $top, 0 );
		$self-> color($c);
	} else {
		$canvas-> backColor($back_color) if $back_color;
		$canvas-> clear( $left, $bottom, $right, $top);
	}
}

package Prima::BidiInput;

sub handle_bidi_input
{
	my ( $self, %opt ) = @_;

	my @ret;
	if ( $opt{action} eq 'backspace') {
		if ( $opt{at} == ($opt{rtl} ? 0 : $opt{n_clusters})) {
			chop $opt{text};
			@ret = ( $opt{text}, length($opt{text}));
		} else {
			my $curpos = $opt{glyphs}->cursor2offset($opt{at}, $opt{rtl});
			if ( $curpos > 0 ) {
				substr( $opt{text}, $curpos - 1, 1) = '';
				@ret = ( $opt{text}, $curpos - 1);
			}
		}
	} elsif ( $opt{action} eq 'delete') {
		my $curpos = $opt{glyphs}->cursor2offset($opt{at}, $opt{rtl});
		if ( $curpos < length($opt{text}) ) {
			substr( $opt{text}, $curpos, 1, '');
			@ret = ( $opt{text}, $curpos);
		}
	} elsif ( $opt{action} eq 'cut') {
		if ( $opt{at} != ($opt{rtl} ? 0 : $opt{n_clusters})) {
			my ($pos, $len, $rtl) = $opt{glyphs}-> cluster2range( 
				$opt{at} - ($opt{rtl} ? 1 : 0));
			substr( $opt{text}, $pos + $len - 1, 1, '');
			@ret = ( $opt{text}, $pos + $len - 1);
		}
	} elsif ( $opt{action} eq 'insert') {
		my $curpos = $opt{glyphs}->cursor2offset($opt{at}, $opt{rtl});
		substr( $opt{text}, $curpos, 0) = $opt{input};
		@ret = ( $opt{text}, $curpos );
	} elsif ( $opt{action} eq 'overtype') {
		my ($append, $shift) = $opt{rtl} ? (0, 1) : ($opt{n_clusters}, 0);
		my $curpos;
		if ( $opt{at} == $append) {
			$opt{text} .= $opt{input};
			$curpos = length($opt{text});
		} else {
			my ($pos, $len, $rtl) = $opt{glyphs}-> cluster2range( $opt{at} - $shift );
			$pos += $len - 1 if $rtl;
			substr( $opt{text}, $pos, 1) = $opt{input};
			$curpos = $pos + ($rtl ? 0 : 1);
		}
		@ret = ( $opt{text}, $curpos );
	} else {
		Carp::cluck("bad input $opt{action}");
	}
	return @ret;
}

1;

=head1 NAME

Prima::IntUtils - internal functions

=head1 DESCRIPTION

The module provides packages, containing common functionality
for some standard classes. The packages are designed as a code
containers, not as widget classes, and are to be used as
secondary ascendants in the widget inheritance declaration.

=head1 Prima::MouseScroller

Implements routines for emulation of auto repeating mouse events.
A code inside C<MouseMove> callback can be implemented by
the following scheme:

	if ( mouse_pointer_inside_the_scrollable_area) {
		$self-> scroll_timer_stop;
	} else {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore( 0);
	}

The class uses a semaphore C<{mouseTransaction}>, which should
be set to non-zero if a widget is in mouse capture state, and set
to zero or C<undef> otherwise.

The class starts an internal timer, which sets a semaphore and
calls C<MouseMove> notification when triggered. The timer is
assigned the timeouts, returned by C<Prima::Application::get_scroll_rate>
( see L<Prima::Application/get_scroll_rate> ).

=head2 Methods

=over

=item scroll_timer_active

Returns a boolean value indicating if the internal timer is started.

=item scroll_timer_semaphore [ VALUE ]

A semaphore, set to 1 when the internal timer was triggered. It is advisable
to check the semaphore state to discern a timer-generated event from
the real mouse movement. If VALUE is specified, it is assigned to the semaphore.

=item scroll_timer_start

Starts the internal timer.

=item scroll_timer_stop

Stops the internal timer.

=back

=head1 Prima::IntIndents

Provides the common functionality for the widgets that delegate part of their
surface to the border elements. A list box can be of an example, where its
scroll bars and 3-d borders are such elements.

=head2 Properties

=over

=item indents ARRAY

Contains four integers, specifying the breadth of decoration elements for
each side. The first integer is width of the left element, the second - height
of the lower element, the third - width of the right element, the fourth - height
of the upper element.

The property can accept and return the array either as a four scalars, or as
an anonymous array of four scalars.

=back

=head2 Methods

=over

=item get_active_area [ TYPE = 0, WIDTH, HEIGHT ]

Calculates and returns the extension of the area without the border elements,
or the active area.
The extension are related to the current size of a widget, however, can be
overridden by specifying WIDTH and HEIGHT. TYPE is an integer, indicating
the type of calculation:

=over

=item TYPE = 0

Returns four integers, defining the area in the inclusive-exclusive coordinates.

=item TYPE = 1

Returns four integers, defining the area in the inclusive-inclusive coordinates.

=item TYPE = 2

Returns two integers, the size of the area.

=back

=back

=head1 Prima::GroupScroller

The class is used for widgets that contain optional scroll bars, and provides means for
their maintenance. The class is the descendant of L<Prima::IntIndents>, and adjusts
the L<indents> property when scrollbars are shown or hidden, or L<borderWidth> is changed.

The class does not provide range selection for the scrollbars; the descentant classes
must implement that.

The descendant classes must follow the guidelines:

=over

=item *

A class must provide C<borderWidth>, C<hScroll>, and C<vScroll> property keys in profile_default() .
A class may provide C<autoHScroll> and C<autoVScroll> property keys in profile_default() .

=item *

A class' init() method must set C<{borderWidth}>, C<{hScroll}>, and C<{vScroll}>
variables to 0 before the initialization, call C<setup_indents> method,
and then assign the properties from the object profile.

If a class provides C<autoHScroll> and C<autoVScroll> properties, these must be set to
0 before the initialization.

=item *

If a class needs to overload one of C<borderWidth>, C<hScroll>, C<vScroll>,
C<autoHScroll>, and C<autoVScroll> properties,
it is mandatory to call the inherited properties.

=item *

A class must implement the scroll bar notification callbacks: C<HScroll_Change> and C<VScroll_Change>.

=item *

A class must not use the reserved variable names, which are:

	{borderWidth}  - internal borderWidth storage
	{hScroll}      - internal hScroll value storage
	{vScroll}      - internal vScroll value storage
	{hScrollBar}   - pointer to the horizontal scroll bar
	{vScrollBar}   - pointer to the vertical scroll bar
	{bone}         - rectangular widget between the scrollbars
	{autoHScroll}  - internal autoHScroll value storage
	{autoVScroll}  - internal autoVScroll value storage

The reserved method names:

	set_h_scroll
	set_v_scroll
	insert_bone
	setup_indents
	reset_indents
	borderWidth
	autoHScroll
	autoVScroll
	hScroll
	vScroll

The reserved widget names:

	HScroll
	VScroll
	Bone

=back

=head2 Properties

=over

=item autoHScroll BOOLEAN

Selects if the horizontal scrollbar is to be shown and hidden dynamically,
depending on the widget layout.

=item autoVScroll BOOLEAN

Selects if the vertical scrollbar is to be shown and hidden dynamically,
depending on the widget layout.

=item borderWidth INTEGER

Width of 3d-shade border around the widget.

Recommended default value: 2

=item hScroll BOOLEAN

Selects if the horizontal scrollbar is visible. If it is, C<{hScrollBar}>
points to it.

=item vScroll BOOLEAN

Selects if the vertical scrollbar is visible. If it is, C<{vScrollBar}>
points to it.

=item scrollBarClass STRING = Prima::ScrollBar

Create-only property that allows to change scrollbar class

=item hScrollBarProfile, vScrollBarProfile HASH

Create-only property that allows to change scrollbar parameters when it is being created

=back

=head2 Properties

=over

=item setup_indents

The method is never called directly; it should be called whenever widget
layout is changed so that indents are affected. The method is a request
to recalculate indents, depending on the widget layout.

The method is not reentrant; to receive this callback and update the widget
layout, that in turn can result in more C<setup_indents> calls, overload
C<reset_indents> .

=item reset_indents

Called after C<setup_indents> is called and internal widget layout is updated,
to give a chance to follow-up the layout changes.

=back

=head1 Prima::UndoActions

Used for classes that can edit and undo and redo its content.

=head2 Properties

=over

=item undoLimit INTEGER

Sets limit on number of stored atomic undo operations. If 0,
undo is disabled.

=back

=head2 Methods

=over

=item begin_undo_group

Opens bracket for group of actions, undone as single operation.
The bracket is closed by calling C<end_undo_group>.

=item end_undo_group

Closes bracket for group of actions, opened by C<begin_undo_group>.

=item redo

Re-applies changes, formerly rolled back by C<undo>.

=item undo

Rolls back changes into internal array, which size cannot extend C<undoLimit>
value. In case C<undoLimit> is 0, no undo actions can be made.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::InputLine>, L<Prima::Lists>, L<Prima::Edit>,
L<Prima::Outlines>, L<Prima::ScrollBar>.

=cut
