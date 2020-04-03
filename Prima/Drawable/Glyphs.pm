package Prima::Drawable::Glyphs;

use strict;
use warnings;
use Carp qw(cluck);

sub new         { bless [grep { defined } @_[1..4]], $_[0] }
sub glyphs      { $_[0]->[0] }
sub clusters    { $_[0]->[1] }
sub advances    { $_[0]->[2] }
sub positions   { $_[0]->[3] }
sub text_length { $_[0]->[0]->[-1] }

sub overhangs
{
	return @{$_[0]->[2]}[-2,-1] if $_[0]->[2];
	my ( $self, $canvas ) = @_;
	cluck "cannot query glyph metrics without canvas" unless $canvas;
	my $abc = $canvas->get_font_abc(($self->glyphs->[0]) x 2, to::Glyphs);
	return (
		(($abc->[0] < 0) ? -$abc->[0] : 0),
		(($abc->[2] < 0) ? -$abc->[2] : 0)
	);

}

sub left_overhang  { [shift->overhangs(@_)]->[0] }
sub right_overhang { [shift->overhangs(@_)]->[1] }

sub clone
{
	my $self = shift;
	my @svs = map { defined ? Prima::array::clone($_) : undef } @$self;
	return __PACKAGE__->new(@svs);
}

sub abc
{
	my ( $self, $canvas, $index ) = @_;
	my $glyph = $self->glyphs->[$index];
	return @{ $canvas-> get_font_abc($glyph, $glyph, to::Glyphs) };
}

sub def
{
	my ( $self, $canvas, $index ) = @_;
	my $glyph = $self->glyphs->[$index];
	return @{ $canvas-> get_font_def($glyph, $glyph, to::Glyphs) };
}

sub get_width
{
	my ( $self, $canvas, $with_overhangs ) = @_;
	my $advances = $self->advances;
	if ( $advances ) {
		my $x = 0;
		$x += $_ for @$advances;
		return $x;
	} else {
		cluck "cannot calculate glyph width without canvas" unless $canvas;
		return $canvas-> get_text_width($self->glyphs);
	}
}

sub get_box
{
	my ( $self, $canvas ) = @_;
	return $canvas-> get_text_box($self->glyphs);
}

sub get_sub_width
{
	my ( $self, $canvas, $from, $length ) = @_;
	$from //= 0;
	my $glyphs = $self->glyphs;
	my $size = @$glyphs;
	$length = $size - $from unless defined $length;
	return 0 unless $length;
	my $advances = $self->advances;
	if ( $advances ) {
		my $x = 0;
		my $to = $from + $length;
		$to = $size - 1 if $to >= $size;
		$x += $advances->[$_] for $from .. $to;
		return $x;
	} else {
		cluck "cannot calculate glyph width without canvas" unless $canvas;
		return $canvas-> get_text_width(Prima::array::substr($glyphs, $from, $length));
	}
}

sub get_sub_box
{
	my ( $self, $canvas, $from, $length ) = @_;
	$from //= 0;
	my $glyphs = $self->glyphs;
	my $size = @$glyphs;
	$length = $size - $from unless defined $length;
	return 0 unless $length;
	return $canvas-> get_text_box(Prima::array::substr($glyphs, $from, $length));
}

1;

