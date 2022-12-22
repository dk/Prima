package Prima::PS::CFF;

use strict;
use warnings;
use Prima::PS::Glyphs;
use Prima::PS::TempFile;
use Prima::PS::Unicode;
use Prima::Utils;
use base qw(Prima::PS::Glyphs);

sub create_font_entry
{
	my ( $self, $key, $font ) = @_;

	return {
		n_glyphs    => 0,
		bbox        => [ undef, undef, undef, undef ],
		scale       => ($font->{height} - $font->{internalLeading}) / $font->{size},

		fixed       => ($font->{pitch} == fp::Fixed) ? 1 : 0,
		weight      => ($font->{style} & fs::Bold)   ? 1 : 0,
		italic      => ($font->{style} & fs::Italic) ? -10 : 0,

		ascent      => $font->{ascent},
		descent     => -$font->{descent},
	};
}

use constant full_name       => "\x{2}";
use constant family_name     => "\x{3}";
use constant weight          => "\x{4}";
use constant font_bbox       => "\x{5}";
use constant blue_values     => "\x{6}";
use constant endchar         => "\x{e}";
use constant charset         => "\x{f}";
use constant encoding        => "\x{10}";
use constant charstrings     => "\x{11}";
use constant private         => "\x{12}";
use constant is_fixed_pitch  => "\x{c}\x{1}";
use constant italic_angle    => "\x{c}\x{2}";
use constant paint_type      => "\x{c}\x{5}";
use constant font_matrix     => "\x{c}\x{7}";

sub int32($)
{
	my ( $self, $n ) = @_;
	$n = Prima::Utils::nearest_i( $n );
	if (-107 <= $n && $n <= 107) {
		return chr($n + 139);
	} elsif (108 <= $n && $n <= 1131) {
		$n -= 108;
		return chr(($n >> 8) + 247).chr($n & 0xff);
	} elsif (-1131 <= $n && $n <= -108) {
		$n = -$n - 108;
		return chr(($n >> 8) + 251).chr($n & 0xff);
	} elsif (-32768 <= $n && $n < 32767) {
		return pack('Cn', 28, $n);
	} else {
		return pack('CN', 29, $n);
	}
}


sub mk_index
{
	my $ret = '';

	my $n = @_;
	$ret .= pack('n', $n);
	$ret .= pack('C', 4); # offset size is always 4 bytes, for simplicity
	$ret .= pack('N', 1);
	my $ofs = 1;
	$ret .= pack('N', $ofs += length ) for @_;
	return $ret . join('', @_);
}

sub mk_offset
{
	my $n = shift;
	if (-32768 <= $n && $n < 32767) {
		return chr(28).chr(($n >> 8) & 0xff).chr($n & 0xff);
	} else {
		return chr(29).chr(($n >> 24) & 0xff).chr(($n >> 16) & 0xff).chr(($n >> 8) & 0xff).chr($n & 0xff);
	}
}


sub mk_header
{
	my ( $const_data, $offsets, $private_len ) = @_;
	mk_index( join('',
		@$const_data,
		mk_offset($offsets->{charstrings}) , charstrings,
		mk_offset($offsets->{charset})     , charset,
		mk_offset($private_len), mk_offset($offsets->{private}) , private
	))
}

sub begin_evacuate
{
	my ( $self, $fn ) = @_;

	my $v = $self->{fonts}->{$fn};
	$v->{glyphs_left} = $v->{n_glyphs};
	$v->{tmpfile}->reset if $v->{n_glyphs};
}

sub evacuate_next_subfont
{
	my ( $self, $fn ) = @_;

	my $v = $self->{fonts}->{$fn};
	return unless $v->{glyphs_left};


	my @glyphs;
	for my $gid ( 0 .. 255 ) {
		my $unicode = $v->{tmpfile}->read_str;
		my $width   = $v->{tmpfile}->read_str;
		my $code    = $v->{tmpfile}->read_str;
		push @glyphs, [ "a$gid", $unicode, $width, $code ];
		last unless --$v->{glyphs_left};
	}

	my (@ret_charset, @ret_unicode, @ret_width, $ret_content);
	my (@const_data, @strings, %offsets);

	my @charset;
	my $charstrings_len =
		2 + 1 + 4 +          # header
		4 + length(endchar); # .notdef
	for my $g ( @glyphs ) {
		push @strings, $g->[0];
		push @charset, 391 + $#strings;
		$charstrings_len += 4 + length($g->[3]);

		push @ret_charset, $g->[0];
		push @ret_unicode, $g->[1];
		push @ret_width,   $g->[2];
	}

	# charsets and privates
	my @bbox        = Prima::Utils::nearest_i( map { $_ // 0 } @{ $v->{bbox} } );
	my $charset_str = pack('Cn*', 0, @charset); # mode 0, glyph list
	my $private_str = $self->num( $bbox[1], -$bbox[1] ) . blue_values;

	# header
	my $header          = pack("C*", 1, 0, 4, 2) . mk_index($fn);
	push @strings,      $fn;
	push @const_data,   $self->int32(391 + $#strings) . full_name;
	push @const_data,   $self->int32(391 + $#strings) . family_name;
	push @const_data,   $self->int32($v->{bold} ? 384 : 388) . weight;
	push @const_data,   $self->int32($v->{fixed})     . is_fixed_pitch;
	push @const_data,   $self->int32($v->{italic})    . italic_angle;
	push @const_data,   $self->num( @bbox )           . font_bbox;

	my $strings_str = mk_index(@strings);
	my $subr_str    = pack('n', 0);

	# the offsets are affected by the encoded header length, but
	# the header itself contains references to the offsets, that
	# in turn may change the header length. So make several shots at it
	$offsets{charstrings} = 100;
	$offsets{charset}     = 500;
	$offsets{private}     = 2500;

	my $dict_str = mk_header(\@const_data, \%offsets, length($private_str));
	my $safeguard = 10;
	while ( $safeguard-- ) {
		$offsets{charset}     = length($header) + length($dict_str) + length($strings_str) + length($subr_str);
		$offsets{charstrings} = $offsets{charset}     + length($charset_str);
		$offsets{private}     = $offsets{charstrings} + $charstrings_len;
		my $real_dict_str = mk_header(\@const_data, \%offsets, length($private_str));
		my $length_match  = length($real_dict_str) == length($dict_str);
		$dict_str = $real_dict_str;
		last if $length_match;
	}
	if ( $safeguard <= 0 ) {
		warn "panic: cannot encode font $fn as CFF";
		return;
	}


	$ret_content = join('', $header, $dict_str, $strings_str, $subr_str, $charset_str);
	$ret_content .= mk_index(endchar, map {$_->[3]} @glyphs);
	$ret_content .= $private_str;

	return $v, \@ret_charset, \@ret_unicode, \@ret_width, $ret_content;
}

sub use_char
{
	my ( $self, $canvas, $key, $charid, $unicode) = @_;
	my $f = $self->{fonts}->{$key} // return;

	if (exists $f->{subfonts}->{$charid}) {
		my $n = $f->{subfonts}->{$charid};
		return unless defined $n;
		return $n >> 8, $n & 0xff
	}

	$f->{tmpfile} //= Prima::PS::TempFile->new;
	my ($code, $abc) = $self->get_outline( $canvas, $key, $charid, 0 );
	unless (defined($code) && length($code)) {
		$f->{subfonts}->{$charid} = undef;
		return;
	}

	my $n = $f->{subfonts}->{$charid} = $f->{n_glyphs}++;

	$unicode //= '';
	$unicode = ( 1 == length $unicode ) ? ord($unicode) : 0;
	$f->{tmpfile}->write(join('',
		map { pack('n', length) . $_ } (
			$unicode,
			int( $abc->[0] + $abc->[1] + $abc->[2] + .5), 
			$code
		)
	));

	return $n >> 8, $n & 0xff
}

1;

=pod

=head1 NAME

Prima::PS::CFF - create compressed Type1 fonts

=head1 DESCRIPTION

This module contains helper procedures to store compressed Type1 fonts (CFF).

=cut
