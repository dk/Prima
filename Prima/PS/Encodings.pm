package Prima::PS::Encodings;
use vars qw(%files %fontspecific %cache);

use strict;
use warnings;
use Prima::Utils;

%files = (
	'default'    => 'PS/locale/ascii',
	'1276'       => 'PS/locale/ps-standart',
	'ps-standart'=> 'PS/locale/ps-standart',
	'1277'       => 'PS/locale/ps-iso-latin1',
	'ps-latin1'  => 'PS/locale/ps-iso-latin1',
	'Latin1'     => 'PS/locale/ps-iso-latin1',
	'Western'    => 'PS/locale/ps-iso-latin1',
	'iso8859-1'  => 'PS/locale/iso8859-1',
	'iso8859-2'  => 'PS/locale/iso8859-2',
	'iso8859-3'  => 'PS/locale/iso8859-3',
	'iso8859-4'  => 'PS/locale/iso8859-4',
	'iso8859-7'  => 'PS/locale/iso8859-7',
	'iso8859-9'  => 'PS/locale/iso8859-9',
	'iso8859-10' => 'PS/locale/iso8859-10',
	'iso8859-11' => 'PS/locale/iso8859-11',
	'iso8859-13' => 'PS/locale/iso8859-13',
	'iso8859-14' => 'PS/locale/iso8859-14',
	'iso8859-15' => 'PS/locale/iso8859-15',
	'ibm-437'    => 'PS/locale/ibm-cp437',
	'437'        => 'PS/locale/ibm-cp437',
	'ibm-850'    => 'PS/locale/ibm-cp850',
	'850'        => 'PS/locale/ibm-cp850',
	'win-1250'   => 'PS/locale/win-cp1250',
	'1250'       => 'PS/locale/win-cp1250',
	'win-1252'   => 'PS/locale/win-cp1252',
	'1252'       => 'PS/locale/win-cp1252',
);

my %languages = (
	'ascii'         => [qw(en)],
	'ps-standart'   => [qw(en)],
	'ps-iso-latin1' => [qw(en)],
	'iso8859-1'     => [qw(af br eu en co fo is ga id it ku ast lb ms gv no oc pt gd sco sma es sw sv tl wa)],
	'iso8859-2'     => [qw(sq bs hr cs de hu pl sr sk sl dsb hsb tk)],
	'iso8859-3'     => [qw(tr mt eo)],
	'iso8859-4'     => [qw(et lv lt kl se sma)],
	'iso8859-7'     => [qw(el)],
	'iso8859-9'     => [qw(af br eu en co fo is ga id it ku ast lb ms gv no oc pt gd sco sma es sw sv tl wa)],
	'iso8859-10'    => [qw(et lv lt kl se sma da nn nb no sv)],
	'iso8859-11'    => [qw(th)],
	'iso8859-13'    => [qw(et lv lt kl se sma pl)],
	'iso8859-14'    => [qw(ga gv gd cy kw br)],
	'iso8859-15'    => [qw(af br en ga it ku la lb ms no nn nb oc pt gd sco sma es sw sv tl wa sq ca da dl dut et fo fi fr de is)],
	'ibm-cp437'     => [qw(en de sv)],
	'ibm-cp850'     => [qw(af br eu en co fo is ga id it ku ast lb ms gv no oc pt gd sco sma es sw sv tl wa)],
	'win-cp1250'    => [qw(la pl cs sl sk hu bs hr sr ro sq de)],
	'win-cp1252'    => [qw(af br eu en co fo is ga id it ku ast lb ms gv no oc pt gd sco sma es sw sv tl wa)],
);

%fontspecific = (
	'Specific' => 1,
);

sub exists
{
	my $cp = defined( $_[0]) ? ( lc $_[0]) : 'default';
	$cp = $1 if $cp =~ /\.([^\.]*)$/;
	$cp =~ s/_//g;
	return exists($files{$cp});
}

sub unique
{
	my %h;
	my @ret;
	for (sort keys %files) {
		next if m/^\d+$/ || $h{$files{$_}};
		$h{$files{$_}} = 1;
		push @ret, $_;
	}
	return @ret;
}

sub load
{
	my $cp = defined( $_[0]) ? $_[0] : 'default';
	$cp = $1 if $cp =~ /\.([^\.]*)$/;
	$cp =~ s/_//g;

	if ( $cp eq 'null') {
		return $cache{null} if exists $cache{null};
		return $cache{null} = [('.notdef') x 256];
	}

	my $fx = exists($files{$cp}) ? $files{$cp} : $files{default};

	return $cache{$fx} if exists $cache{$fx};

	my $f = Prima::Utils::find_image( $fx);
	unless ( $f) {
		warn("Prima::PS::Encodings: cannot find encoding file for $cp\n");
		return load('default') unless $cp eq 'default';
		return;
	}

	unless ( open F, $f) {
		warn("Prima::PS::Encodings: cannot load $f\n");
		return load('default') unless $cp eq 'default';
		return;
	}
	my @f = map { chomp; length($_) ? $_ : ()} <F>;
	close F;
	splice( @f, 256) if 256 < @f;
	push( @f, '.notdef') while 256 > @f;
	return $cache{$fx} = \@f;
}

sub get_languages
{
	my $enc = shift;
	return $languages{$enc} // [];
}

__END__

=pod

=head1 NAME

Prima::PS::Encodings -  manage latin-based encodings

=head1 SYNOPSIS

	use Prima::PS::Encodings;

=head1 DESCRIPTION

This module provides code tables for major latin-based encodings, for
the glyphs that usually provided by every PS-based printer or interpreter.
Prima::PS::Drawable uses these encodings when it decides whether the document
have to be supplied with a bitmap character glyph or a character index,
thus relying on PS interpreter capabilities. Latter is obviously preferable,
but as it's not possible to know beforehand what glyphs are supported by
PS interpreter, the Latin glyph set was selected as a ground level.

=over

=item exists $encoding

Returns whether C<$encoding> exists in the list of internal list of recognized names

=item files

It's unlikely that users will need to supply their own encodings, however
this can be accomplished by:

	use Prima::PS::Encodings;
	$Prima::PS::Encodings::files{iso8859-5} = 'PS/locale/greek-iso';

=item fontspecific

The only non-latin encoding currently present is 'Specific'.
If any other specific-encoded fonts are to be added,
the encoding string must be added as a key to %fontspecific

=item load

Loads encoding file by given string. Tries to be smart to guess
actual file from identifier string returned from setlocale(NULL).
If fails, loads default encoding, which defines only glyphs from
32 to 126. Special case is 'null' encoding, returns array of
256 .notdef's.

=item unique

Returns list of Latin-based encoding string unique keys.

=back

=cut

