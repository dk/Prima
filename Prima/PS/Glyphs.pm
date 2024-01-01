package Prima::PS::Glyphs;

use strict;
use warnings;
use Carp;
use Prima;
use Prima::Utils;
use base qw(Exporter);

sub new
{
	return bless {
		fonts     => {},
	}, shift;
}

sub create_font_entry { Carp::confess }

sub get_font
{
	my ( $self, $font ) = @_;

	my $key = $font->{name};
	$key =~ s/\s+/-/g;
	$key =~ s/([^-a-z0-9])/sprintf("x%x", ord($1))/gei;
	$key = 'PS-' . $key;
	$key .= '-Bold'   if $font->{style} & fs::Bold;
	$key .= '-Italic' if $font->{style} & fs::Italic;
	$self->{fonts}->{$key} //= $self->create_font_entry($key, $font);
	return $key;
}

sub num
{
	my $self = shift;
	return join '', map { $self->int32($_) } @_;
}

use constant endchar         => "\x{e}";

sub conic2curve
{
	my ($x0, $y0, $x1, $y1, $x2, $y2) = @_;
	my (@cp1, @cp2);
	$cp1[0] = $x0 + 2 / 3 * ($x1 - $x0);
	$cp1[1] = $y0 + 2 / 3 * ($y1 - $y0);
	$cp2[0] = $x2 + 2 / 3 * ($x1 - $x2);
	$cp2[1] = $y2 + 2 / 3 * ($y1 - $y2);
	return $x0, $y0, @cp1, @cp2, $x2, $y2;
}

sub rrcurveto
{
	my ($self, $x0, $y0, @rest) = @_;
	my @out;
	for ( my $i = 0; $i < @rest; $i += 2 ) {
		my @p = @rest[$i,$i+1];
		$rest[$i]   -= $x0;
		$rest[$i+1] -= $y0;
		push @out, @rest[$i,$i+1];
		($x0, $y0) = @p;
	}
	return $self->num(@out) . "\x{8}";
}

sub hsbw    { shift->num(@_) . "\x{0d}" }
sub rmoveto { shift->num(@_) . "\x{15}" }
sub rlineto { shift->num(@_) . "\x{05}" }
sub hmoveto { shift->num(@_) . "\x{16}" }

sub get_outline
{
	my ( $self, $canvas, $key, $charid, $with_hsbw) = @_;

	my $f = $self->{fonts}->{$key} // return;

	my $outline = $canvas->render_glyph($charid, glyph => 1) or return;

	my @abc  = map { $_ / $f->{scale} } @{$canvas-> get_font_abc(($charid) x 2, to::Glyphs)};
	my $bbox = $f->{bbox};

	my $size = scalar(@$outline);

	my $code = '';

	my $first_move;
	if ($with_hsbw) {
		my @hsbw = ($abc[0], $abc[0] + $abc[1] + $abc[2]);
		$code = $self->hsbw(@hsbw);
		if ( $size && $outline->[0] != ggo::Move ) {
			$code .= $self->hmoveto($hsbw[0]);
		} else {
			$first_move = $hsbw[0];
		}
	} else {
		$first_move = 0;
	}

	my @p = (0,0);
	my $scale = $f->{scale} * 64;
	for ( my $i = 0; $i < $size; ) {
		my $cmd = $outline->[$i++];
		my $pts = $outline->[$i++] * 2;

		my $fill_bbox = $i == 2 && !defined $bbox->[0];
		my @pts = map { $outline->[$i++] / $scale } 0 .. $pts - 1;
		if ( $fill_bbox ) {
			$$bbox[0] = $$bbox[2] = $pts[0];
			$$bbox[1] = $$bbox[3] = $pts[1];
		}

		for ( my $j = 0; $j < @pts; $j += 2 ) {
			my ( $x, $y ) = @pts[$j,$j+1];
			$$bbox[0] = $x if $$bbox[0] > $x;
			$$bbox[1] = $y if $$bbox[1] > $y;
			$$bbox[2] = $x if $$bbox[2] < $x;
			$$bbox[3] = $y if $$bbox[3] < $y;
		}

		if ( $cmd == ggo::Move ) {
			my @r = ($pts[0] - $p[0], $pts[1] - $p[1]);
			if ( defined $first_move ) {
				$r[0] -= $first_move;
				undef $first_move;
			}
			$code .= $self->rmoveto(@r);
		} elsif ( $cmd == ggo::Line ) {
			for ( my $j = 0; $j < @pts; $j += 2 ) {
				my @r = ($pts[$j] - $p[0], $pts[$j + 1] - $p[1]);
				@p = @pts[$j,$j+1];
				$code .= $self->rlineto(@r);
			}
		} elsif ( $cmd == ggo::Conic ) {
			if ( @pts == 4 ) {
				$code .= $self->rrcurveto(conic2curve(@p, @pts));
			} else {
				my @xts = (@p, @pts[0,1]);
				for ( my $j = 0; $j < @pts - 4; $j += 2 ) {
					push @xts,
						($pts[$j + 2] + $pts[$j + 0]) / 2,
						($pts[$j + 3] + $pts[$j + 1]) / 2,
						$pts[$j + 2], $pts[$j + 3],
				}
				push @xts, @pts[-2,-1];
				my $max = @xts - 4;
				for ( my $j = 0; $j < $max; $j += 4) {
					$code .= $self->rrcurveto(conic2curve(@xts[$j .. $j + 5]));
				}
			}
		} elsif ( $cmd == ggo::Cubic ) {
			$code .= $self->rrcurveto(@p, @pts);
		}
		@p = @pts[-2,-1];
	}
	$code .= endchar;

	return $code, \@abc;
}

1;

=pod

=head1 NAME

Prima::PS::Glyphs - glyph outlines as adobe charstrings

=head1 DESCRIPTION

This module contains helper procedures to query vector font outlines for
storing them in Type1 fonts.

=cut
