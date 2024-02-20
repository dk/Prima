package Prima::Image::Exif;

use strict;
use warnings;

sub parse
{
	my %c = (
		ptr   => 0,
		ref   => \ $_[1],
		error => undef,
		data  => {},
	);
	my $self = bless \%c, $_[0];
	$self->read_all;
	$c{data} = undef unless keys %{$c{data}};
	return $c{data}, $c{error};
}

my %codes = (
	V => ['N', 4],
	v => ['n', 2],
);

sub error   { $_[0]->{error} = $_[1]; undef }

sub _unpack {
	$_[0]->{big_endian} ?
		join('', map { $codes{$_}->[0] } split '', $_[1]) :
		$_[1]
}

sub _read
{
	my ($self, $howmany) = @_;
	if ( $howmany + $self->{ptr} > length ${ $self->{ref} } ) {
		$self->{error} = 'read beyond exif data';
		return undef;
	} else {
		my $k = substr(${ $self->{ref} }, $self->{ptr}, $howmany );
		$self->{ptr} += $howmany;
		return $k;
	}
}

sub _read_value
{
	my ( $self, $type ) = @_;
	my $data = $self->_read($codes{$type}->[1]) // return;
	return unpack($self->_unpack($type), $data);
}

sub seek
{
	my ( $self, $ptr ) = @_;
	Carp::confess unless defined $ptr;
	$self->{ptr} = $ptr + 6;
}

sub _tag_value
{
	my ( $self, $tag ) = @_;
	for ( @{ $self->{data}->{image} // [] }) {
		return $$_[2] if $$_[0] eq $tag;
	}
	return undef;
}

sub read_all
{
	my $self = shift;

	return unless $self->read_header;

	while ( 1 ) {
		my $ofs = $self->_read_value('V');
		last if $ofs == 0;
		my $sz = length ${$self->{ref}};
		return $self->error("IFD points to $ofs beyond exif data (size=$sz)")
			if $ofs > $sz;
		$self->seek($ofs);

		return unless $self->read_ifd('image');
	}

	if ( my $photo = $self->_tag_value(0x8769)) {
		$self->seek( $photo );
		$self->read_ifd('photo');
	}

	if ( my $gps = $self->_tag_value(0x8825)) {
		$self->seek( $gps );
		$self->read_ifd('gpsinfo');
	}

	my $compression = $self->_tag_value(0x103) // -1;
	my ($ofs, $len);
	if ($compression == 6) {
		$ofs = $self->_tag_value(0x201);
		$len = $self->_tag_value(0x202);
	} elsif ( $compression == 1 ) {
		$ofs = $self->_tag_value(0x111);
		$len = $self->_tag_value(0x117);
	}
	if ( defined $ofs && defined $len ) {
		$self->seek( $ofs );
		$self->{data}->{thumbnail} = $self->_read($len);
	}

	return 1;
}

my @formats = (
	undef,
	['uint8'    ,1, 'C',],
	['ascii'    ,1      ],
	['uint16'   ,2, 'S' ],
	['uint32'   ,4, 'L' ],
	['urational',8, 'L' ],
	['int8'     ,1, 'c' ],
	['byte'     ,1      ],
	['int16'    ,2, 's' ],
	['int32'    ,4, 'L' ],
	['rational' ,8, 'l' ],
	['float'    ,4, 'f' ],
	['double'   ,8, 'd' ],
);

our %tags = (
	0x0000	=> 'GPSVersionID',
	0x0001	=> 'GPSLatitudeRef',
	0x0002	=> 'GPSLatitude',
	0x0003	=> 'GPSLongitudeRef',
	0x0004	=> 'GPSLongitude',
	0x0005	=> 'GPSAltitudeRef',
	0x0006	=> 'GPSAltitude',
	0x0007	=> 'GPSTimeStamp',
	0x0008	=> 'GPSSatellites',
	0x0009	=> 'GPSStatus',
	0x000a	=> 'GPSMeasureMode',
	0x000b	=> 'GPSDOP',
	0x000c	=> 'GPSSpeedRef',
	0x000d	=> 'GPSSpeed',
	0x000e	=> 'GPSTrackRef',
	0x000f	=> 'GPSTrack',
	0x0010	=> 'GPSImgDirectionRef',
	0x0011	=> 'GPSImgDirection',
	0x0012	=> 'GPSMapDatum',
	0x0013	=> 'GPSDestLatitudeRef',
	0x0014	=> 'GPSDestLatitude',
	0x0015	=> 'GPSDestLongitudeRef',
	0x0016	=> 'GPSDestLongitude',
	0x0017	=> 'GPSDestBearingRef',
	0x0018	=> 'GPSDestBearing',
	0x0019	=> 'GPSDestDistanceRef',
	0x001a	=> 'GPSDestDistance',
	0x001b	=> 'GPSProcessingMethod',
	0x001c	=> 'GPSAreaInformation',
	0x001d	=> 'GPSDateStamp',
	0x001e	=> 'GPSDifferential',
	0x001f	=> 'GPSHPositioningError',
	0x00fe	=> 'NewSubfileType',
	0x00ff	=> 'SubfileType',
	0x0100	=> 'ImageWidth',
	0x0101	=> 'ImageLength',
	0x0102	=> 'BitsPerSample',
	0x0103	=> 'Compression',
	0x0106	=> 'PhotometricInterpretation',
	0x010e	=> 'ImageDescription',
	0x010f	=> 'Make',
	0x0110	=> 'Model',
	0x0111	=> 'StripOffsets',
	0x0112	=> 'Orientation',
	0x0115	=> 'SamplesPerPixel',
	0x0116	=> 'RowsPerStrip',
	0x0117	=> 'StripByteConunts',
	0x011a	=> 'XResolution',
	0x011b	=> 'YResolution',
	0x011c	=> 'PlanarConfiguration',
	0x0128	=> 'ResolutionUnit',
	0x012d	=> 'TransferFunction',
	0x0131	=> 'Software',
	0x0132	=> 'DateTime',
	0x013b	=> 'Artist',
	0x013d	=> 'Predictor',
	0x013e	=> 'WhitePoint',
	0x013f	=> 'PrimaryChromaticities',
	0x0142	=> 'TileWidth',
	0x0143	=> 'TileLength',
	0x0144	=> 'TileOffsets',
	0x0145	=> 'TileByteCounts',
	0x014a	=> 'SubIFDs',
	0x015b	=> 'JPEGTables',
	0x0201	=> 'JpegIFOffset',
	0x0202	=> 'JpegIFByteCount',
	0x0211	=> 'YCbCrCoefficients',
	0x0212	=> 'YCbCrSubSampling',
	0x0213	=> 'YCbCrPositioning',
	0x0214	=> 'ReferenceBlackWhite',
	0x828d	=> 'CFARepeatPatternDim',
	0x828e	=> 'CFAPattern',
	0x828f	=> 'BatteryLevel',
	0x8298	=> 'Copyright',
	0x829a	=> 'ExposureTime',
	0x829d	=> 'FNumber',
	0x8769	=> 'ExifOffset',
	0x8822	=> 'ExposureProgram',
	0x8824	=> 'SpectralSensitivity',
	0x8825	=> 'GPSInfo',
	0x8828	=> 'OECF',
	0x8829	=> 'Interlace',
	0x882a	=> 'TimeZoneOffset',
	0x882b	=> 'SelfTimerMode',
	0x8827	=> 'ISOSpeedRatings',
	0x83bb	=> 'IPTC/NAA',
	0x8773	=> 'InterColorProfile',
	0x9000	=> 'ExifVersion',
	0x9003	=> 'DateTimeOriginal',
	0x9004	=> 'DateTimeDigitized',
	0x9101	=> 'ComponentConfiguration',
	0x9102	=> 'CompressedBitsPerPixel',
	0x9201	=> 'ShutterSpeedValue',
	0x9202	=> 'ApertureValue',
	0x9203	=> 'BrightnessValue',
	0x9204	=> 'ExposureBiasValue',
	0x9205	=> 'MaxApertureValue',
	0x9206	=> 'SubjectDistance',
	0x9207	=> 'MeteringMode',
	0x9208	=> 'LightSource',
	0x9209	=> 'Flash',
	0x920a	=> 'FocalLength',
	0x920b	=> 'FlashEnergy',
	0x920c	=> 'SpatialFrequencyResponse',
	0x920d	=> 'Noise',
	0x9211	=> 'ImageNumber',
	0x9212	=> 'SecurityClassification',
	0x9213	=> 'ImageHistory',
	0x9215	=> 'ExposureIndex',
	0x9216	=> 'TIFF/EPStandardID',
	0x9290	=> 'SubSecTime',
	0x9291	=> 'SubSecTimeOriginal',
	0x9292	=> 'SubSecTimeDigitized',
	0x927c	=> 'MakerNote',
	0x9286	=> 'UserComment',
	0xa000	=> 'FlashPixVersion',
	0xa001	=> 'ColorSpace',
	0xa002	=> 'ExifImageWidth',
	0xa003	=> 'ExifImageHeight',
	0xa004	=> 'RelatedSoundFile',
	0xa005	=> 'ExifInteroperabilityOffset',
	0xa20c	=> 'SpatialFrequencyResponse',
	0xa20b	=> 'FlashEnergy',
	0xa20e	=> 'FocalPlaneXResolution',
	0xa20f	=> 'FocalPlaneYResolution',
	0xa210	=> 'FocalPlaneResolutionUnit',
	0xa214	=> 'SubjectLocation',
	0xa215	=> 'ExposureIndex',
	0xa217	=> 'SensingMethod',
	0xa300	=> 'FileSource',
	0xa301	=> 'SceneType',
	0xa302	=> 'CFAPattern',
);

our %revtags;

sub read_ifd
{
	my ($self, $section) = @_;

	my $n_entries = $self->_read_value('v') // return;
	my @entries;
	while ( @entries < $n_entries ) {
		my $direntry = $self->_read(12) // return;
		my ( $tag, $format, $n_components, $data_offset ) = unpack($self->_unpack('vvVV'), $direntry);

		my $at = "at IFD $section" .
			' entry #' . @entries;

		my $format_desc = $formats[$format] || return $self->error("bad format $format at $at");
		my ( $f_name, $f_size, $f_handler) = @$format_desc;

		next if $n_components == 0;
		my $sz = $n_components * $f_size;
		my $raw_data;
		if ( $sz > 4 ) {
			local $self->{ptr};
			$self->seek( $data_offset );
			$raw_data = $self->_read( $sz ) // return $self->error("bad component length $sz at $at");
		} else {
			$raw_data = substr($direntry, 8, 4);
		}

		my @data;
		if ($f_name eq 'rational' || $f_name eq 'urational') {
			$n_components *= 2;
			$f_size /= 2;
		}
		if ( $f_name eq 'ascii' ) {
			@data = unpack('Z*', $raw_data );
		} elsif ( $f_name eq 'byte' ) {
			@data = ( $raw_data );
		} elsif ( $f_size == 1 || !$self->{need_byte_reverse} || $f_name eq 'float' || $f_name eq 'double') {
			@data = unpack( $f_handler . '*', $raw_data );
		} else {
			for ( my $i = 0; $i < $n_components; $i++) {
				push @data, unpack($f_handler, reverse substr($raw_data, $i * $f_size, $f_size));
			}
		}
		push @entries, [ $tag, $f_name, @data ];
	}

	$self->{data}->{$section} //= [];
	push @{$self->{data}->{$section}}, @entries;

}

sub read_header
{
	my $self = shift;

	my $header = $self->_read(10) // return 0;
	use bytes;
	return $self->error('no exif header') unless $header =~ m/^
		Exif
		\x{00}\x{00}
		(II\x{2a}\x{00}|MM\x{00}\x{2a})
	$/x;
	no bytes;

	$self->{big_endian} = (substr($1,0,1) eq 'I') ? 0 : 1;
	my $is_be = (pack(S => 1) eq "\0\1") ? 1 : 0;
	$self->{need_byte_reverse} = $is_be != $self->{big_endian};
	return 1;
}

sub compile
{
	my ( $class, $data, %opt ) = @_;
	my %c = (
		%opt,
		data   => $data,
		error  => undef,
		str    => '',
		tail   => '',
		offset => 0,
	);
	my $self = bless \%c, $class;
	$self->compile_all;
	return $c{str}, $c{error};
}

my %formats = (
	'uint8'    => [ 1,'C'],
	'ascii'    => [ 2    ],
	'uint16'   => [ 3,'S'],
	'uint32'   => [ 4,'L'],
	'urational'=> [ 5,'L'],
	'int8'     => [ 6,'c'],
	'byte'     => [ 7    ],
	'int16'    => [ 8,'s'],
	'int32'    => [ 9,'L'],
	'rational' => [10,'l'],
	'float'    => [11,'f'],
	'double'   => [12,'d'],
);

sub compile_ifd
{
	my ( $self, $ifd ) = @_;

	my $head = pack('S', scalar @$ifd);
	my $body = '';

	for my $entry ( @$ifd ) {
		my ( $tag, $type, @data ) = @$entry;
		my ( $n_components, $value );
		if ( $type eq 'ascii' ) {
			$n_components = length($data[0]) + 1;
			$value = $data[0] . "\x{00}";
		} elsif ($type eq 'byte') {
			$n_components = length($data[0]);
			$value = $data[0];
		} elsif ( $type eq 'rational' || $type eq 'urational') {
			$n_components = @data / 2;
			$value = pack( $formats{$type}->[1] . '*', @data );
		} elsif ( exists $formats{$type}) {
			$n_components = @data;
			$value = pack( $formats{$type}->[1] . '*', @data );
		} else {
			return $self->error("bad format $type");
		}

		if ( length($value) > 4) {
			my $v = pack('L', $self->{offset} + length($self->{tail}));
			$self->{tail} .= $value;
			$value = $v;
		} elsif ( length($value) < 4) {
			$value .= "\x{00}" while length($value) < 4;
		}

		$body .= pack('SSL', $tag, $formats{$type}->[0], $n_components) . $value;
	}

	return $head.$body."\x{00}\x{00}\x{00}\x{00}";
}

sub compile_all
{
	my $self = shift;

	my $image = $self->{data}->{image} // [];

	@$image = grep {
		$$_[0] != 0x8769 && # exif offset
		$$_[0] != 0x8825 && # gps
		$$_[0] != 0x0201 && # jpeg thumbnail
		$$_[0] != 0x0202 && #
		$$_[0] != 0x0111 && # tiff thumbnail
		$$_[0] != 0x0117    #
	} @$image;

	my $is_be = (pack(S => 1) eq "\0\1") ? 1 : 0;

	my $head =
		($is_be ? "MM\x{00}\x{2a}" : "II\x{2a}\x{00}") .
		pack('L', 8)
		;

	my @extras;
	my $n_new = 0;
	$self->{offset} = length($head) + 2 + scalar(@$image) * 12 + 4;

	if ( defined ( my $photo = $self->{data}->{photo} )) {
		push @extras, $photo, $self->{offset}, [ 0x8769, 'uint32', 0 ];
		$self->{offset} += 2 + scalar(@$photo) * 12 + 4;
		$n_new++;
	}

	if ( defined ( my $gps = $self->{data}->{gpsinfo} )) {
		push @extras, $gps, $self->{offset}, [ 0x8825, 'uint32', 0 ];
		$self->{offset} += 2 + scalar(@$gps) * 12 + 4;
		$n_new++;
	}

	if ( defined $self->{data}->{thumbnail} ) {
		my ($ofs, $len, $comp);
		for my $i ( @$image ) {
			next if $$i[0] != 0x103; # compression
			$comp = $$i[2];
		}
		if ( defined $comp ) {
			if ( $comp == 6 ) {
				($ofs, $len) = (0x0201, 0x0202);
			} elsif ( $comp == 1 ) {
				($ofs, $len) = (0x0111, 0x0117);
			} else {
				goto COMPLAIN;
			}
		} else {
		COMPLAIN:
			Carp::carp "* Thumbnail is requested but tag 0x103 (Compression) is not set or not recognized. Set it to 6 (jpeg) or 1 (tiff)";
		}

		if ( defined $ofs ) {
			$n_new += 2;
			push @$image,
				[ $ofs, 'uint32', $self->{offset} + length($self->{tail}) + $n_new * 12 ],
				[ $len, 'uint32', length $self->{data}->{thumbnail} ];
			$self->{tail} .= $self->{data}->{thumbnail};
		}
	}

	my $ifds = '';
	for ( my $i = 0; $i < @extras; $i += 3 ) {
		$extras[$i+1] += $n_new * 12;
	}
	$self->{offset} += $n_new * 12;

	for ( my $i = 0; $i < @extras; $i += 3 ) {
		my ( $dict, $offset, $entry ) = @extras[$i .. $i+2];
		my $ifd = $self->compile_ifd($dict) // return;
		$ifds .= $ifd;
		$entry->[2] = $offset;
		push @$image, $entry;
	}

	my $ifd  = $self->compile_ifd($image) // return;

	$self->{str} =
		"Exif\x{00}\x{00}" .
		$head .
		$ifd .
		$ifds .
		$self->{tail};

	return 1;
}

##############

sub read_jpeg
{
	my ($class, $i, %opt) = @_;

	my $data;
	return (undef, "no jpeg exif data") unless
		($i->codec // '') eq 'JPEG' and
		exists $i->{extras}->{appdata} and
		$data = $i->{extras}->{appdata}->[1];

	my ( $res, $error) = $class->parse($data);
	if ( ref($res) ) {
		if ( $res->{thumbnail} && $opt{load_thumbnail} ) {
			open my $f, "<", \$res->{thumbnail};
			binmode $f;
			my $t = Prima::Image->load( $f );
			$res->{thumbnail} = $t // $@;
			close $f;
		}
		if ( $opt{tag_as_string}) {
			for my $k ( supported_ifds() ) {
				my $ifd = $res->{$k};
				next unless (ref($ifd // '') // '') eq 'ARRAY';
				for my $e ( @$ifd ) {
					$e->[0] = $tags{ $e->[0] } if exists $tags{ $e->[0] };
				}
			}
		}
	}
	return $res, $error;
}

sub read_extras
{
	my ( $class, $i, %opt ) = @_;
	my $c = $i->codec // '';
	if ( $c eq 'JPEG') {
		return $class->read_jpeg($i, %opt);
	} else {
		return (undef, "not supported");
	}
}

sub write_jpeg
{
	my ($class, $i, $data) = @_;

	return undef, "jpeg is not supported" unless $i->codec('JPEG');

	my $e = $i->{extras};
	my %data = %$data;
	if ( $data{thumbnail} && ref($data{thumbnail})) {
		my $t = '';
		open my $f, ">", \$t;
		binmode $f;
		my $ok = $data{thumbnail}->save($f, codecID => Prima::Image->has_codec('JPEG')->{codecID});
		close $f;
		return (undef, $@) unless $ok;
		$data{thumbnail} = $t;
	}

	my $ii = $data{image} //= [];
	my $found_compression;
	for my $entry ( @$ii ) {
		next if $entry->[0] != 0x103;
		$entry->[2] = 6;
		$found_compression = 1;
		last;
	}
	push @$ii, [ 0x103, 'uint16', 6 ] unless $found_compression;

	my ($compiled, $error) = $class->compile(\%data);
	return (undef, $error) if !defined $compiled;
	$i->{extras}->{appdata}->[1] = $compiled;
	return 1;
}

sub supported_ifds { qw(image photo gpsinfo) }

sub revtags
{
	%revtags = reverse %tags unless scalar keys %revtags;
	return \%revtags;
}

sub write_extras
{
	my ($class, $i, @data) = @_;
	my $c = $i->codec // '';

	my $data = ( @data == 1 ) ? $data[0] : { @data };

	my $revtags;
	for my $k ( supported_ifds ) {
		next unless (ref($data->{$k} // '') // '') eq 'ARRAY';
		for my $v ( @{ $data->{$k} } ) {
			if ( $v->[0] =~ /^[A-Za-z]/ ) {
				$revtags //= revtags;
				return (undef, "don't know tag ID for `$v->[0]`, use a numeric value")
					unless exists $revtags->{ $v->[0] };
				$v->[0] = $revtags->{ $v->[0] };
			}
		}
	}

	if ( $c eq 'JPEG' ) {
		return $class->write_jpeg($i, $data);
	} else {
		return (undef, "not supported");
	}
}

1;

=pod

=head1 NAME

Prima::Image::Exif - manipulate Exif records

=head1 DESCRIPTION

The module allows to parse and create Exif records. The records
could be read from JPEG files, and stored in these using the
extra appdata hash field.

=head1 SYNOPSIS

	use Prima qw(Image::Exif);

	# load image with extras
	my $jpeg = Prima::Image->load($ARGV[0], loadExtras => 1);
	die $@ unless $jpeg;

	my ( $data, $error ) = Prima::Image::Exif->read_extras($jpeg,
		load_thumbnail => 1,
		tag_as_string  => 1
	);
	die "cannot read exif: $error\n" if defined $error;

	for my $k ( sort keys %$data ) {
		my $v = $data->{$k};
		if ( $k eq 'thumbnail' ) {
			if ( ref($v)) {
				print "thumbnail ", $v->width, 'x', $v->height, "\n";
			} else {
				print "error loading thumbnail: $v\n";
			}
			next;
		}
		for my $dir ( @$v ) {
			my ( $tag, $name, @data ) = @$dir;
			print "$k.$tag $name @data\n";
		}
	}

	# create new image
	$jpeg->size(300,300);

	# create a thumbnail - not too big as jpeg appdata max length is 64k
	my $thumbnail = $jpeg->dup;
	delete $thumbnail->{extras};
	$thumbnail->size(150,150);

	# compile an exif chunk
	my $ok;
	($ok, $error) = Prima::Image::Exif->write_extras($jpeg,
		thumbnail => $thumbnail,
		gpsinfo   => $data->{gpsinfo},
	);
	die "cannot create exif data: $error\n" unless $ok;

	$jpeg->save('new.jpg') or die $@;

=head1 API

=head2 parse $CLASS, $EXIF_STRING

Returns two scalars, a data reference and an error. If there is no data reference,
the error is fatal, otherwise a warning (i.e. assumed some data were parsed, but
most probabyl not all).

The data is a hash where there are the following keys may be set: image, photo,
gpsinfo, thumbnail.  These are individual categories containing the exif tags.
Each hash value contains an array of tags, except C<thumbnail> that contains a
raw image data. Each tag is an array in the following format: [ tag, format,
@data ] where the tag is a numeric tag value, the format is a type descriptor
(such as int8 and ascii), and data is 1 or more scalars containing the data.

The module recognized some common tags that can be accessed via
C<%Prima::Image::Exif::tags>.

=head2 read_extras $CLASS, $IMAGE, %OPTIONS

Given a loaded Prima image, loads exif data from extras; returns two
scalar, a data reference and an error.

Options supported:

=over

=item load_thumbnail

If set, tries to load thumbnail as a Prima image. In this case,
replaces the thumbnail raw data with the image loaded, or in case of an error, with
an error string

=item tag_as_string

If set, replaces known tag numeric values with their string names

=back

=head2 compile $CLASS, $DATA

Accepts DATA in the format described above, creates an exif string.
Returns two scalars, am exif string and an error. If the string
is not defined, the error is.

=head2 write_extras $CLASS, $IMAGE, %DATA

Checks if image codec is supported, creates Exif data and saves these in
C<< $IMAGE->{extras} >> . Return two scalars, a success flag and an error.

=head1 SEE ALSO

L<Prima::image-load>

The module does not provide comprehensive support for tags and their values.
For a more thorough dive see these modules: L<Image::ExifTool>, L<Image::EXIF>.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=cut
