# contains:
#   Panel
package Prima::Widgets;

use strict;
use warnings;
use Prima::Const;
use Prima::Classes;

package Prima::Panel;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		ownerBackColor => 1,
		raise          => 1,
		borderWidth    => 1,
		image          => undef,
		imageFile      => undef,
		zoom           => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	if ( defined $p-> {imageFile} && !defined $p-> {image}) {
		$p-> {image} = Prima::Image-> create;
		delete $p-> {image} unless $p-> {image}-> load($p-> {imageFile});
	}
}

sub init
{
	my $self = shift;
	for ( qw( image ImageFile))
		{ $self-> {$_} = undef; }
	for ( qw( zoom raise borderWidth iw ih)) { $self-> {$_} = 1; }
	my %profile = $self-> SUPER::init(@_);
	$self-> { imageFile}  = $profile{ imageFile};
	for ( qw( image zoom borderWidth raise)) {
		$self-> $_($profile{$_});
	}
	return %profile;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size   = $canvas-> size;

	my $clr    = $self-> backColor;
	my $bw     = $self-> {borderWidth};
	my @c3d    = ( $self-> light3DColor, $self-> dark3DColor);
	@c3d = reverse @c3d unless $self-> {raise};
	my $cap = $self-> text;
	unless ( defined $self-> {image}) {
		$canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, @c3d, $clr);
		$canvas-> text_shape_out( $cap,
			( $size[0] - $canvas-> get_text_width( $cap)) / 2,
			( $size[1] - $canvas-> font-> height) / 2,
		) if defined $cap;
		return;
	}
	$canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, @c3d) if $bw > 0;
	my ( $x, $y) = ( $bw, $bw);
	my ( $dx, $dy ) = ( $self-> {iw}, $self-> {ih});
	$canvas-> clipRect( $bw, $bw, $size[0] - $bw - 1, $size[1] - $bw - 1) if $bw > 0;
	$canvas-> clear if $self-> {image}-> isa('Prima::Icon');
	while ( $x < $size[0] - $bw) {
		$y = $bw;
		while ( $y < $size[1] - $bw) {
			$canvas-> stretch_image( $x, $y, $dx, $dy, $self-> {image});
			$y += $dy;
		}
		$x += $dx;
	}
	$canvas-> text_shape_out( $cap,
		( $size[0] - $canvas-> get_text_width( $cap)) / 2,
		( $size[1] - $canvas-> font-> height) / 2,
	) if $cap;
}

sub set_border_width
{
	my ( $self, $bw) = @_;
	$bw = 0 if $bw < 0;
	$bw = int( $bw);
	return if $bw == $self-> {borderWidth};
	$self-> {borderWidth} = $bw;
	$self-> repaint;
}

sub text
{
	return $_[0]-> SUPER::text unless $#_;
	$_[0]-> SUPER::text( $_[1]);
	$_[0]-> repaint;
}


sub set_raise
{
	my ( $self, $r) = @_;
	return if $r == $self-> {raise};
	$self-> {raise} = $r;
	$self-> repaint;
}

sub set_image_file
{
	my ($self,$file,$img) = @_;
	$img = Prima::Image-> create;
	return unless $img-> load($file);
	$self-> {imageFile} = $file;
	$self-> image($img);
}

sub set_image
{
	my ( $self, $img) = @_;
	$self-> {image} = $img;
	return unless defined $img;
	( $self-> {iw}, $self-> {ih}) = ($img-> width * $self-> {zoom}, $img-> height * $self-> {zoom});
	$self-> {image} = undef if $self-> {ih} == 0 || $self-> {iw} == 0;
	$self-> repaint;
}


sub set_zoom
{
	my ( $self, $zoom) = @_;
	$zoom = int( $zoom);
	$zoom = 1 if $zoom < 1;
	$zoom = 10 if $zoom > 10;
	return if $zoom == $self-> {raise};
	$self-> {zoom} = $zoom;
	return unless defined $self-> {image};
	my $img = $self-> {img};
	( $self-> {iw}, $self-> {ih}) = ( $img-> width * $self-> {zoom}, $img-> height * $self-> {zoom});
	$self-> repaint;
}

sub image        {($#_)?$_[0]-> set_image($_[1]):return $_[0]-> {image} }
sub imageFile    {($#_)?$_[0]-> set_image_file($_[1]):return $_[0]-> {imageFile}}
sub zoom         {($#_)?$_[0]-> set_zoom($_[1]):return $_[0]-> {zoom}}
sub borderWidth  {($#_)?$_[0]-> set_border_width($_[1]):return $_[0]-> {borderWidth}}
sub raise        {($#_)?$_[0]-> set_raise($_[1]):return $_[0]-> {raise}}

1;

=pod

=head1 NAME

Prima::Widgets - miscellaneous widget classes

=head1 DESCRIPTION

The module was designed to serve as a collection of small widget
classes that do not group well with the other, more purposeful classes.
The current implementation contains the only class, C<Prima::Panel>.

=head1 Prima::Panel

Provides a simple panel widget, capable of displaying a single line
of centered text on a custom background. Probably this functionality
is better to be merged into C<Prima::Label>'s.

=head2 Properties

=over

=item borderWidth INTEGER

Width of 3d-shade border around the widget.

Default value: 1

=item image OBJECT

Selects image to be drawn as a tiled background.
If C<undef>, the background is drawn with the background color.

=item imageFile PATH

Set the image FILE to be loaded and displayed. Is rarely used since does not return
a loading success flag.

=item raise BOOLEAN

Style of 3d-shade border around the widget.
If 1, the widget is 'risen'; if 0 it is 'sunken'.

Default value: 1

=item zoom INTEGER

Selects zoom level for image display.
The acceptable value range is between 1 and 10.

Default value: 1

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>

=cut
