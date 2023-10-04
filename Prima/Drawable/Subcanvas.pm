package Prima::Drawable::Subcanvas;

use strict;
use warnings;
use Prima;
our @ISA = qw(Prima::Drawable);
use Carp;

# Supply a default subcanvas offset:
sub profile_default {
	my %def = %{ shift->SUPER::profile_default };

	return {
		%def,
		offset => [0, 0],
	};
}

# Graphics properties must be stored internally. Note the font attribute
# needs to be handled with a tied hash at some point.
my @easy_props = qw(color backColor fillMode fillPattern font lineEnd
	lineJoin linePattern lineWidth rop rop2 miterLimit
	textOpaque textOutBaseline
);

sub init {
	my $self = shift;

	# We need to set up the parent canvas before calling the parent initializer
	# code. This is because the superclass will call the various setters during
	# its init, and our setters need the parent canvas.
	my %profile = @_;
	croak('You must provide a parent canvas')
		unless $self->{parent_canvas} = delete $profile{parent_canvas};

	$self->{offset} = [0, 0];
	$self->{translate} = [0, 0];

	# OK, now we can call the SUPER's init()
	%profile = $self->SUPER::init(%profile);
	$self->offset(@{delete $profile{offset}});
	return %profile;
}

sub offset {
	my ($self, @new_offset) = @_;
	# Handle the setter code
	if (@new_offset) {
		$self->{offset} = \@new_offset;
		return @new_offset;
	}
	# return the attribute.
	return @{$self->{offset}};
}

sub width  { $#_ ? $_[0]->raise_ro('width')  : $_[0]->{size}->[0] }
sub height { $#_ ? $_[0]->raise_ro('height') : $_[0]->{size}->[1] }
sub size   { $#_ ? $_[0]->raise_ro('size')	 : @{$_[0]->{size}}   }

sub resolution { $#_ ? $_[0]->raise_ro('resolution') : $_[0]->{parent_canvas}->resolution}

# we're always in the paint state
sub begin_paint		 { 0 }
sub begin_paint_info { 0 }
sub end_paint		 {}
sub end_paint_info	 {}
sub get_paint_state  { ps::Enabled }

### For these methods, just call the method on the parent widget directly
for my $direct_method (qw(
	fonts get_bpp get_font_abc get_font_ranges get_handle
	get_nearest_color get_physical_palette get_text_width text_wrap get_text_box
)) {
	no strict 'refs';
	*{$direct_method} = sub {
		use strict 'refs';
		my $self = shift;

		# Indicate the property name subref we're writing (thanks to
		# http://www.perlmonks.org/?node_id=304883 for the idea)
		*__ANON__ = $direct_method;

		# use goto to make this as transparent (or hidden) as possible
		unshift @_, $self->{parent_canvas};
		goto &{$self->{parent_canvas}->can($direct_method)};
	};
}

# Build the sub to handle the getting and setting of the property.
for my $prop_name (@easy_props) {
	no strict 'refs';
	*{$prop_name} = sub { shift->{parent_canvas}->$prop_name(@_) };
}

sub palette
{
	return $_[0]->{parent_canvas}->palette unless $#_;
	my ( $self, $palette ) = @_;
	# XXX do not set anything so far
}

# Primitives must apply the clipping and translating before calling on the
# parent.
my @primitives = qw(
	arc bar chord draw_text ellipse
	fill_chord fill_ellipse fillpoly fill_sector flood_fill
	line lines polyline put_image put_image_indirect
	rectangle sector stretch_image
	text_out clear graphic_context_push graphic_context_pop
);

for my $primitive_name (@primitives) {
	no strict 'refs';
	*{$primitive_name} = sub {
		use strict 'refs';
		my ($self, @args_for_method) = @_;

		# Indicate the name of the subref we're writing (thanks to
		# http://www.perlmonks.org/?node_id=304883 for the idea)
		*__ANON__ = $primitive_name;

		# Do not perform the draw if we are in a null clip region.
		return if $self->{null_clip_region};

		# Otherwise apply the method, working under the assumption that the
		# clipRect and translate will do their job.
		$self->{parent_canvas}->$primitive_name(@args_for_method);
	};
}

sub pixel {
	my ($self, $x, $y, @color_arg) = @_;

	# This could be wrong since we could inquire about the pixel at a
	# covered location, in which case we don't have the value (it was
	# never stored in the first place if it was covered). But then
	# again, subcanvas is mostly meant for drawn output, not deep
	# inspection. The alternative, to use the factory-provided method above,
	# would return null in that case, and that wouldn't be very nice.
	return $self->{parent_canvas}->pixel($x, $y, @color_arg);
}

# Apply the subcanvas's clipRect, using the internal translation and boundaries
sub clipRect {
	my ($self, @r) = @_;

	# If we are called as a getter, return what we have
	return @{$self->{clipRect}} unless @r;

	# validate
	my ($left, $bottom, $right, $top) = @r;
	my $width  = $right - $left;
	my $height = $top	- $bottom;
	$width	= 0 if $width  < 0;
	$height = 0 if $height < 0;
	$right = $left	 + $width;
	$top   = $bottom + $height;

	# Store the rectangle to be returned
	@{$self->{clipRect}} = ($left, $bottom, $right, $top);

	# If the clipRect is outside the widget's
	# boundaries, set a flag that will prevent drawing operations.
	my ($w, $h) = $self->size;
	if ($left >= $w or $right < 0 or $bottom >= $h or $top < 0) {
		$self->{parent_canvas}->clipRect(0,0,0,0); # XXX Prima can't completely block out drawing by clipping
		$self->{null_clip_region} = 1;
		return;
	}

	# If we're here, we are going to clip, so remove that flag.
	delete $self->{null_clip_region};

	# Trim the translated boundaries so that they are clipped by the widget's
	# actual edges.
	$left  = 0    if $left	<  0;   $bottom = 0    if $bottom < 0;
	$right = 0    if $right <  0;   $top    = 0    if $top	  < 0;
	$left  = $w-1 if $left	>= $w;  $bottom = $h-1 if $bottom >= $h;
	$right = $w-1 if $right >= $w;	$top    = $h-1 if $top	  >= $h;

	# Finally, calculate the clipping rectangle with respect to the parent's origin.
	my ($x_off, $y_off) = $self->offset;
	$left  += $x_off; $bottom += $y_off;
	$right += $x_off; $top	  += $y_off;

	$self->{parent_canvas}->clipRect($left, $bottom, $right, $top);
}

sub translate {
	my ($self, @new_trans) = @_;

	# As getter, return what we have
	return @{$self->{translate}} unless @new_trans;

	# store the new translation in case it's inquired about
	$self->{translate} = [@new_trans];

	# Apply (to the parent's canvas)
	my ($left, $bottom) = $self->offset;
	$self->{parent_canvas}->translate(
		$new_trans[0] + $left, $new_trans[1] + $bottom);
}

sub region {
	return $_[0]->{region} unless $#_;

	my ( $self, $region ) = @_;
	$self->{region} = $region;
	if ( $region ) {
		my ($x, $y) = $self->offset;
		if ( $x != 0 || $y != 0 ) {
			# Manually translate the region
			my $r = Prima::Image->new(
				width  => $region->width  + $x,
				height => $region->height + $y,
			);
			$r->put_image( $x, $y, $region );
			$self->{parent_canvas}->region($r);
		} else {
			$self->{parent_canvas}->region($region);
		}
	} else {
		$self->{parent_canvas}->region(undef);
	}
}

sub AUTOLOAD {
	my $self = shift;

	# Remove qualifier from original method name...
	(my $called = our $AUTOLOAD) =~ s/.*:://;

	my $parent_canvas = $self->{parent_canvas};
	if (my $subref = $parent_canvas->can($called)) {
		return $subref->($parent_canvas, @_);
	}
	else {
		die "Don't know how to $called\n";
	}
}

sub paint_widgets
{
	my ( $self, $root, $x, $y ) = @_;
	return unless $root->visible;

	$self->offset($x,$y);
	$self->{size} = [ $root->size ];
	$self->{current_widget} = $root;

	for my $property (@easy_props) {
		next if $property eq 'font';
		$self->$property($root->$property);
	}

	# font is special because widget and canvas resolution can differ - and if they do,
	# font.size(in points) and font.width(in pixels) will get in conflict, because
	# if there's .size, .height is ignored (but .width isn't)
	my %font = %{$root->font};
	delete $font{size} if exists $font{size} && exists $font{height};
	$self->font(\%font);

	$self->translate(0,0);
	$self->clipRect(0,0,$root->width-1,$root->height-1);
	$self->region( $root->shape ) if $root->shape;

	$root->push_event;
	$root->begin_paint_info;
	$root->notify('Paint', $self);
	$self->color(cl::White);
	$root->end_paint_info;
	$root->pop_event;

	$self->{current_widget} = undef;

	# Paint children in z-order
	my @widgets = $root->get_widgets;
	if ( $widgets[0] ) {
		my $w = $widgets[0]->last;
		@widgets = ();
		while ( $w ) {
			push @widgets, $w;
			$w = $w->prev;
		}
	}

	for my $widget (@widgets) {
		$self->paint_widgets( $widget, $x + $widget->left, $y + $widget->bottom );
	}
}

sub Prima::Drawable::paint_with_widgets {
	my ($self, $canvas, $x, $y) = @_;
	return unless $canvas->get_paint_state == ps::Enabled;
	# XXX handle paletted image
	my $subcanvas = Prima::Drawable::Subcanvas->new( parent_canvas => $canvas );
	$subcanvas->paint_widgets($self,$x || 0, $y || 0);
}

sub Prima::Drawable::screenshot {
	my ($self, %opt) = @_;
	my $screenshot = Prima::Image->new(
		width  => $self->width,
		height => $self->height,
		type   => im::RGB,
		%opt
	);
	$screenshot->begin_paint;
	$self->paint_with_widgets($screenshot);
	$screenshot->end_paint;
	return $screenshot;
}

1;

__END__

=pod

=head1 NAME

Prima::Drawable::Subcanvas - paint a hierarchy of widgets to any drawable

=head1 DESCRIPTION

Needed for painting a screenshot on an image, printer, etc.  Adds two methods
to the C<Prima::Drawable> namespace: L<paint_with_widgets> and L<screenshot>.

=head1 SYNOPSIS

    use Prima qw(Application Button);
    my $w = Prima::MainWindow-> create;
    $w->insert( 'Button' );
    $w->screenshot->save('a.bmp');

=head1 METHODS

=over

=item paint_with_widgets $canvas, $x=0, $y=0

Given a C<$canvas> is in the paint mode, traverses all widgets as they are seen on
the screen, and paints them on the canvas with given C<$x,$y> offsets.

=item screenshot $canvas, %opt

Syntax sugar over the paint_with_widgets. Creates an image with the C<$self>'s, size,
and calls C<paint_with_widgets> with it. Returns the screenshot.

=back

=head1 AUTHOR

David Mertens

=head1 SEE ALSO

F<examples/grip.pl>

=cut

