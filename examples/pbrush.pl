=pod

=head1 NAME

examples/pbrush.pl - A minimalistic graphic editor window

=head1 FEATURES

Outlines features required for a graphic editor window -
color selection, and, mainly, non-standart L<Prima::ImageViewer>
usage.

Using L<Prima::Classes>, L<Prima::ScrollWidget>, L<Prima::Application>,
L<Prima::Dialog::ColorDialog>, L<Prima::ImageViewer>.

=cut

use strict;
use warnings;

use Prima;
use Prima::Classes;
use Prima::ScrollWidget;
use Prima::Application;
use Prima::Dialog::ColorDialog;
use Prima::ImageViewer;

package ImageEdit::Painter;
use vars qw(@ISA);
@ISA = qw(Prima::ImageViewer);

sub profile_default
{
	my %d = %{$_[ 0]-> SUPER::profile_default};
	return {
		%d,
		zoom     => 1,
		growMode => gm::Client,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $i = $self-> {image} = Prima::Image-> create(
		width  => $profile{limitX},
		height => $profile{limitY},
		type   => im::RGB,
	);
	$i-> begin_paint;
	$i-> color( cl::White);
	$i-> bar( 0,0,$i-> size);
	$i-> end_paint;
	$self-> image( $i);
	return %profile;
}


sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction};
	$self-> {mouseTransaction} = 1;
	$self-> capture(1);
	$self-> {lmx} = $x;
	$self-> {lmy} = $y;
	$self-> {estorage} = undef;
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransaction};
	my @ln = $self-> screen2point( $self-> {lmx}, $self-> {lmy}, $x, $y);
	$self-> begin_paint;
	$self-> set( %{$self-> owner-> {attrs}});
	$self-> line( $self-> {lmx}, $self-> {lmy}, $x, $y);
	$self-> end_paint;
	push( @{$self-> {estorage}-> {line}}, [@ln]);
	$self-> {lmx} = $x;
	$self-> {lmy} = $y;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransaction};
	$self-> {mouseTransaction} = undef;
	$self-> capture(0);
	$self-> {lmx} = undef;
	$self-> {lmy} = undef;
	return unless defined $self-> {estorage};
	my $i = $self-> {image};
	$i-> begin_paint;
	$i-> set( %{$self-> owner-> {attrs}});
	if ( exists $self-> {estorage}-> {line})
	{
		for ( @{$self-> {estorage}-> {line}})
		{
			$i-> line( @$_);
		}
	}
	$i-> end_paint;
}

sub done
{
	my $self = $_[0];
	$self-> {image}-> destroy;
	$self-> SUPER::done;
}


package ImageEdit::ColorSelector;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

my @colors = (
0x000000,0x848284,0x840000,0x848200,0x008200,0x008284,0x000084,0x840084,0x848242,
0x004142,0x0082ff,0x004184,0x4200ff,0x844100,0xffffff,0xc6c3c6,0xff0000,0xffff00,
0x00ff00,0x00ffff,0x0000ff,0xff00ff,0xffff84,0x00ff84,0x84ffff,0x8482ff,0xff0084,
0xff8242
);

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	$canvas-> color( cl::Black);
	$canvas-> rectangle( 0, 0, $sz[0]-1, $sz[1] - 1);
	my $cx  = scalar @colors / 2;
	my $cdx = ( $sz[0] - 2) / $cx;
	my @c3d = ( $self-> dark3DColor, $self-> light3DColor);
	my $i;
	for ( $i = 0; $i < $cx; $i++) {
		$canvas-> rect3d( 1 + $i * $cdx, $sz[1] / 2 + 1,
			1 + $i * $cdx + $cdx - 1, $sz[1] - 2,
			1, @c3d, $colors[$i]);
		$canvas-> rect3d( 1 + $i * $cdx, 1,
			1 + $i * $cdx + $cdx - 1, $sz[1] / 2,
			1, @c3d, $colors[$i + $cx]);
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	my @sz = $self-> size;
	my $cx  = scalar @colors / 2;
	my $cdx = ( $sz[0] - 2) / $cx;
	my $index = 0;
	$index += $cx if $y < $sz[1] / 2;
	$index += int(( $x - 1) / $cdx);
	if ( $btn == mb::Left) {
		$self-> owner-> {attrs}-> {color} = $colors[$index];
	} elsif ( $btn == mb::Right) {
		$self-> owner-> {attrs}-> {backColor} = $colors[$index];
	}
	$self-> owner-> {indicator}-> repaint;
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	return unless $dbl;
	my @sz = $self-> size;
	my $cx  = scalar @colors / 2;
	my $cdx = ( $sz[0] - 2) / $cx;
	my $index = 0;
	$index += $cx if $y < $sz[1] / 2;
	$index += int(( $x - 1) / $cdx);
	my $d = $self-> {colorDlg} ?
		$self-> {colorDlg} :
		Prima::Dialog::ColorDialog-> create(
			centered => 1,
			visible  => 0,
			name     => 'Edit color',
		);
	$self-> {colorDlg} = $d;
	$d-> value( $colors[$index]);
	return unless  $d-> execute == mb::OK;
	$colors[ $index] = $d-> value;
	$self-> owner-> {indicator}-> repaint;
}

package ImageEdit::Indicator;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my @c3d = ( $self-> dark3DColor, $self-> light3DColor);
	$canvas-> rect3d( 0, 0, $sz[0]-1, $sz[1]-1, 1, @c3d, $self-> backColor);
	$canvas-> rect3d( 3, 3, $sz[0] * 0.6, $sz[1] * 0.6, 1,
		@c3d, $self-> owner-> {attrs}-> {backColor});
	$canvas-> rect3d( $sz[0] * 0.4, $sz[1] * 0.4, $sz[0] - 4,
		$sz[1] - 4, 1, @c3d, $self-> owner-> {attrs}-> {color});
}


package ImageEdit::EditorWindow;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub profile_default
{
	my %d = %{$_[ 0]-> SUPER::profile_default};
	return {
		%d,
		imageSize => [ 100, 100],
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	my %attrs = (
		color     => cl::Black,
		backColor => cl::White,
	);
	$self-> {attrs} = \%attrs;

	$self-> {painter} = $self-> insert( 'ImageEdit::Painter' =>
		origin     => [ 64, 64],
		size       => [ $self-> width - 64, $self-> height - 64],
		limitX     => $profile{imageSize}-> [0],
		limitY     => $profile{imageSize}-> [1],
	);

	$self-> {colorsel} = $self-> insert( 'ImageEdit::ColorSelector' =>
		origin     => [ 64, 0],
		size       => [ $self-> width - 64, 63],
		growMode   => gm::Floor,
	);

	$self-> {indicator} = $self-> insert( 'ImageEdit::Indicator' =>
		origin     => [ 0, 0],
		size       => [ 61, 63],
	);

	return %profile;
}

package Editor;

my $w = ImageEdit::EditorWindow-> create(
	text    => 'Edit sample',
	size       => [ 400, 400],
	centered   => 1,
	onDestroy  => sub {$::application-> close},
	imageSize  => [ 320, 320],
);

run Prima;
