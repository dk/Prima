package Prima::PS::Fonts;


use strict;
use warnings;
use Prima;
use Prima::Utils;
use Prima::PS::Encodings;
use vars qw(%files %enum_families $defaultFontName
	$variablePitchName $fixedPitchName $symbolFontName);

my %cache;
$defaultFontName   = 'Helvetica';
$variablePitchName = 'Helvetica';
$fixedPitchName    = 'Courier';
$symbolFontName    = 'Symbol';

sub query_metrics
{
	my $f = $_[0];
	my ( $file, $name, $family);
	if ( exists $enum_families{$f}) {
		$family = $f;
		$f = $enum_families{$f};
	}
	$name = $f;
	if ( exists $files{$f}) {
		$file = $files{ $f};
		unless ( defined $family) {
		# pick up family
			for ( sort keys %enum_families) {
				$family = $_, last if index($f, $_) == 0;
			}
		}
		$family = $defaultFontName unless defined $family;
	} else {
		$family = $defaultFontName;
		$name   = $enum_families{$family};
		$file   = $files{ $name};
	}

	return $cache{$file} if exists $cache{$file};

	my $defFN = $files{ $defaultFontName};
	my $fx = Prima::Utils::find_image( $file);

	unless ( $fx) {
		if ( $name eq $defaultFontName) {
			warn("Prima::PS::Fonts: can't load default font\n");
			return undef;
		} else {
			warn("Broken Prima::PS::Fonts installation: $name not found\n"),
				return query_metrics( $defaultFontName)
		}
	}
	unless ( open F, $fx) {
		warn( "Prima::PS::Fonts: cannot open file: $fx\n") ;
		return undef if $f eq $defaultFontName;
		return query_metrics( $defaultFontName);
	}
	{
		local $/;
		my @ra;
		my $z = '@ra = ' . <F>;
		close F;
		eval($z);

		if ( $@ || scalar(@ra) < 2) {
			warn( "Prima::PS::Fonts: invalid file: $fx\n");
			return undef if $name eq $defaultFontName;
			return query_metrics( $defaultFontName);
		}
		$ra[1]-> {docname}  = $name;
		$ra[1]-> {name}     = $name;
		$ra[1]-> {family}   = $family;
		$ra[1]-> {charheight} = $ra[1]-> {height};
		$cache{$file} = $ra[1];
	}
	return $cache{$file};
}

sub enum_fonts
{
	my ( $family, $encoding) = @_;
	my @names;
	$family   = undef if defined $family   && !length $family;
	$encoding = undef if defined $encoding && !length $encoding;
	if ( defined $family) {
		return [] unless defined $enum_families{$family};
		for ( sort keys %enum_families) {
			push @names, $_ if $_ eq $family;
		}
	} else {
		@names = sort keys %enum_families;
	}
	my @ret;
	for ( @names) {
		my %x = %{query_metrics( $_)};
		$x{name} = $x{family};
		delete $x{docname};
		delete $x{chardata};
		$_ = \%x;
		my $latin = exists( $Prima::PS::Encodings::files{$x{encoding}});
		next if defined($encoding) && (
			$latin ?
			! exists $Prima::PS::Encodings::files{$encoding} :
			($x{encoding} ne $encoding)
			);
		if ( !defined $family && !defined $encoding) {
			$x{encodings} = $latin ?  [ Prima::PS::Encodings::unique ] : [ $x{encoding} ];
		}
		if (
			defined $family &&
			!defined $encoding &&
			exists( $Prima::PS::Encodings::files{$x{encoding}})
		) {
			push @ret, map {
				my %n = %x;
				$n{encoding} = $_;
				\%n;
			} Prima::PS::Encodings::unique;
		} else {
			$_-> {encoding} = $encoding if defined( $encoding) && $latin;
			push @ret, $_;
		}
	}
	return \@ret;
}

sub enum_family
{
	my $family = $_[0];
	return $enum_families{$family} ?
		grep { index($_, $family) == 0 } sort keys %files : 
		();
}


sub font_pick
{
	my ( $src, $dest, %options) = @_;
	my $bySize = exists( $src-> {size}) && !exists( $src-> {height});
	$dest = Prima::Drawable-> font_match( $src, $dest, 0);

	$dest-> {encoding} = ''
		if ( ! exists ( $Prima::PS::Encodings::fontspecific{ $dest-> {encoding}}) &&
			! exists ( $Prima::PS::Encodings::files{ $dest-> {encoding}}));

	# find name
	my $m1   = query_metrics( $dest-> {name});

	# find encoding
	if ( length $dest-> {encoding}) {
		if ( defined $dest-> {encoding}) {
			if ( exists ( $Prima::PS::Encodings::files{ $dest-> {encoding}} ) &&
				! exists ( $Prima::PS::Encodings::files{ $m1-> {encoding}})) {
					$m1 = query_metrics( $fixedPitchName);
					$dest-> {encoding} = $m1-> {encoding};
					$dest-> {name}     = $m1-> {name};
					$dest-> {family}   = $m1-> {family};
			} elsif (
				exists ( $Prima::PS::Encodings::fontspecific{ $dest-> {encoding}}) &&
			! exists ( $Prima::PS::Encodings::fontspecific{ $m1-> {encoding}})
			) {
				$m1 = query_metrics( $symbolFontName);
				$dest-> {encoding} = $m1-> {encoding};
				$dest-> {name}     = $m1-> {name};
				$dest-> {family}   = $m1-> {family};
			}
		}
	} else {
		$dest-> {encoding} = $m1-> {encoding};
	}

	# find pitch
	if ( $dest-> {pitch} != fp::Default && $dest-> {pitch} != $m1-> {pitch}) {
		if ( $dest-> {pitch} == fp::Variable) {
			$m1 = query_metrics( $variablePitchName);
		} else {
			$m1 = query_metrics( $fixedPitchName);
		}
	}

	# get all family members
	my @famx = map { query_metrics( $_) } enum_family( $m1-> {family});

	# find style
	my $m2;
	for ( @famx) { # exact match
		$m2 = $_, last if $_-> {style} == $dest-> {style};
	}
	unless ( $m2) { # second pass
		my $maxDiff = 1000;
		my ( $italic, $bold) = (
			( $dest-> {style} & fs::Italic) ? 1 : 0,
			($dest-> {style} & fs::Bold) ? 1 :0
		);
		for ( @famx) {
			my ( $i, $b) = (
				( $_-> {style} & fs::Italic) ? 1 : 0,
				( $_-> {style} & fs::Bold) ? 1 : 0
			);
			my $diff = (( $i == $italic) ? 0 : 2) + (( $b == $bold) ? 0 : 1);
			$m2 = $_, $maxDiff = $diff if $diff < $maxDiff;
		}
	}
	$m2 = $m1 unless defined $m2;

	# scale dimensions
	my $res = $options{resolution} ? $options{resolution} : $m2-> {yDeviceRes};
	if ( $bySize) {
		$dest-> {height} = $dest->{size} * $m2->{height} * $res / ($m2->{size} * $m2->{yDeviceRes});
	} else {
		$dest-> {size}   = $dest->{height} * $m2->{size} * $m2->{yDeviceRes} / ($m2->{height} * $res);
	}
	my $a = $dest-> {height} / $m2-> {height};
	my %muls = %$m2;
	my $charheight = $muls{height};
	my $du   = $dest-> {style} & fs::Underlined;
	my $ds   = $dest-> {style} & fs::StruckOut;
	my $dw   = $dest-> {width};
	$muls{$_} = int ( $muls{$_} * $a + 0.5) for
	qw( height ascent descent width maximalWidth internalLeading externalLeading );
	delete $muls{size};
	my $enc = $dest-> {encoding};
	$dest-> {$_}     = $muls{$_} for keys %muls;
	$dest-> {referenceWidth} = $m2->{maximalWidth} * $a;
	$dest-> {encoding} = $enc;
	$dest-> {style} |= fs::Underlined if $du;
	$dest-> {style} |= fs::StruckOut if $ds;
	$dest->{ maximalWidth} = $dest-> {width} = $dw if $dw != 0;
	$dest-> {charheight} = $charheight;
	return $dest;
}

%files = map { $_ => "PS/fonts/$_" } (
	'Bookman-Demi'                ,
	'Bookman-DemiItalic'          ,
	'Bookman-Light'               ,
	'Bookman-LightItalic'         ,
	'Courier'                     ,
	'Courier-Oblique'             ,
	'Courier-Bold'                ,
	'Courier-BoldOblique'         ,
	'AvantGarde-Book'             ,
	'AvantGarde-BookOblique'      ,
	'AvantGarde-Demi'             ,
	'AvantGarde-DemiOblique'      ,
	'Helvetica'                   ,
	'Helvetica-Oblique'           ,
	'Helvetica-Bold'              ,
	'Helvetica-BoldOblique'       ,
	'Helvetica-Narrow'            ,
	'Helvetica-Narrow-Oblique'    ,
	'Helvetica-Narrow-Bold'       ,
	'Helvetica-Narrow-BoldOblique',
	'Palatino-Roman'              ,
	'Palatino-Italic'             ,
	'Palatino-Bold'               ,
	'Palatino-BoldItalic'         ,
	'NewCenturySchlbk-Roman'      ,
	'NewCenturySchlbk-Italic'     ,
	'NewCenturySchlbk-Bold'       ,
	'NewCenturySchlbk-BoldItalic' ,
	'Times-Roman'                 ,
	'Times-Italic'                ,
	'Times-Bold'                  ,
	'Times-BoldItalic'            ,
	'Symbol'                      ,
	'ZapfChancery-MediumItalic'   ,
	'ZapfDingbats'                ,
);


# The keys of %enum_families are only font names, - in Prima terms.
# The only problem that the family field is always the same as the
# font name

%enum_families = (
	'Bookman'                => 'Bookman-Light',
	'Courier'                => 'Courier',
	'Avant Garde'            => 'AvantGarde-Book',
	'Helvetica'              => 'Helvetica',
	'Helvetica Narrow'       => 'Helvetica-Narrow',
	'Palatino'               => 'Palatino-Roman',
	'New Century Schoolbook' => 'NewCenturySchlbk-Roman',
	'Times'                  => 'Times-Roman',
	'Symbol'                 => 'Symbol',
	'Zapf Chancery'          => 'ZapfChancery-MediumItalic',
	'Zapf Dingbats'          => 'ZapfDingbats',
);

1;

__END__

=pod

=head1 NAME

Prima::PS::Fonts - PostScript device fonts metrics

=head1 SYNOPSIS

	use Prima;
	use Prima::PS::Fonts;

=head1 DESCRIPTION

This module primary use is to be invoked from Prima::PS::Drawable module.
Assumed that some common fonts like Times and Courier are supported by PS
interpreter, and it is assumed that typeface is preserved more-less the
same, so typesetting based on font's a-b-c metrics can be valid.
35 font files are supplied with 11 font families. Font files with metrics
located into 'fonts' subdirectory.

=over

=item query_metrics( $fontName)

Returns font metric hash with requested font data, uses $defaultFontName
if give name is not found. Metric hash is the same as Prima::Types::Font
record, plus 3 extra fields: 'docname' containing font name ( equals
always to 'name'), 'chardata' - hash of named glyphs, 'charheight' -
the height that 'chardata' is rendered to. Every hash
entry in 'chardata' record contains four numbers - suggested character
index and a, b and c glyph dimensions with height equals 'charheight'.

=item enum_fonts( $fontFamily)

Returns font records for given family, or all families
perpesented by one member, if no family name given.
If encoding specified, returns only the fonts with the encoding given.
Compliant to Prima::Application::fonts interface.

=item files & enum_families

Hash with paths to font metric files. File names not necessarily
should be as font names, and it is possible to override font name
contained in the file just by specifying different font key - this
case will be recognized on loading stage and loaded font structure
patched correspondingly.

Example:

	$Prima::PS::Fonts::files{Standard Symbols} = $Prima::PS::Fonts::files{Symbol};

	$Prima::PS::Fonts::files{'Device-specific symbols, set 1'} = 'my/devspec/data.1';
	$Prima::PS::Fonts::files{'Device-specific symbols, set 2'} = 'my/devspec/data.2';
	$Prima::PS::Fonts::enum_families{DevSpec} = 'Device-specific symbols, set 1';

=item font_pick( $src, $dest, %options)

Merges two font records using Prima::Drawable::font_match, picks
the result and returns new record.  $variablePitchName and
$fixedPitchName used on this stage.

Options can include the following fields:

- resolution - vertical resolution. The default value is taken from
font resolution.

=item enum_family( $fontFamily)

Returns font names that are presented in given family

=back

=cut
