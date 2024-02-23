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
	0x00fe	=> 'SubfileType',
	0x00ff	=> 'OldSubfileType',
	0x0100	=> 'ImageWidth',
	0x0101	=> 'ImageHeight',
	0x0102	=> 'BitsPerSample',
	0x0103	=> 'Compression',
	0x0106	=> 'PhotometricInterpretation',
	0x0107	=> 'Thresholding',
	0x0108	=> 'CellWidth',
	0x0109	=> 'CellLength',
	0x010a	=> 'FillOrder',
	0x010d	=> 'DocumentName',
	0x010e	=> 'ImageDescription',
	0x010f	=> 'Make',
	0x0110	=> 'Model',
	0x0111	=> 'StripOffsets',
	0x0112	=> 'Orientation',
	0x0115	=> 'SamplesPerPixel',
	0x0116	=> 'RowsPerStrip',
	0x0117	=> 'StripByteCounts',
	0x0118	=> 'MinSampleValue',
	0x0119	=> 'MaxSampleValue',
	0x011a	=> 'XResolution',
	0x011b	=> 'YResolution',
	0x011c	=> 'PlanarConfiguration',
	0x011d	=> 'PageName',
	0x011e	=> 'XPosition',
	0x011f	=> 'YPosition',
	0x0120	=> 'FreeOffsets',
	0x0121	=> 'FreeByteCounts',
	0x0122	=> 'GrayResponseUnit',
	0x0123	=> 'GrayResponseCurve',
	0x0124	=> 'T4Options',
	0x0125	=> 'T6Options',
	0x0128	=> 'ResolutionUnit',
	0x0129	=> 'PageNumber',
	0x012c	=> 'ColorResponseUnit',
	0x012d	=> 'TransferFunction',
	0x0131	=> 'Software',
	0x0132	=> 'ModifyDate',
	0x013b	=> 'Artist',
	0x013c	=> 'HostComputer',
	0x013d	=> 'Predictor',
	0x013e	=> 'WhitePoint',
	0x013f	=> 'PrimaryChromaticities',
	0x0140	=> 'ColorMap',
	0x0141	=> 'HalftoneHints',
	0x0142	=> 'TileWidth',
	0x0143	=> 'TileLength',
	0x0144	=> 'TileOffsets',
	0x0145	=> 'TileByteCounts',
	0x0146	=> 'BadFaxLines',
	0x0147	=> 'CleanFaxData',
	0x0148	=> 'ConsecutiveBadFaxLines',
	0x014a	=> 'SubIFD',
	0x014c	=> 'InkSet',
	0x014d	=> 'InkNames',
	0x014e	=> 'NumberofInks',
	0x0150	=> 'DotRange',
	0x0151	=> 'TargetPrinter',
	0x0152	=> 'ExtraSamples',
	0x0153	=> 'SampleFormat',
	0x0154	=> 'SMinSampleValue',
	0x0155	=> 'SMaxSampleValue',
	0x0156	=> 'TransferRange',
	0x0157	=> 'ClipPath',
	0x0158	=> 'XClipPathUnits',
	0x0159	=> 'YClipPathUnits',
	0x015a	=> 'Indexed',
	0x015b	=> 'JPEGTables',
	0x015f	=> 'OPIProxy',
	0x0190	=> 'GlobalParametersIFD',
	0x0191	=> 'ProfileType',
	0x0192	=> 'FaxProfile',
	0x0193	=> 'CodingMethods',
	0x0194	=> 'VersionYear',
	0x0195	=> 'ModeNumber',
	0x01b1	=> 'Decode',
	0x01b2	=> 'DefaultImageColor',
	0x01b3	=> 'T82Options',
	0x01b5	=> 'JPEGTables',
	0x0200	=> 'JPEGProc',
	0x0201	=> 'ThumbnailOffset',
	0x0202	=> 'ThumbnailLength',
	0x0203	=> 'JPEGRestartInterval',
	0x0205	=> 'JPEGLosslessPredictors',
	0x0206	=> 'JPEGPointTransforms',
	0x0207	=> 'JPEGQTables',
	0x0208	=> 'JPEGDCTables',
	0x0209	=> 'JPEGACTables',
	0x0211	=> 'YCbCrCoefficients',
	0x0212	=> 'YCbCrSubSampling',
	0x0213	=> 'YCbCrPositioning',
	0x0214	=> 'ReferenceBlackWhite',
	0x022f	=> 'StripRowCounts',
	0x02bc	=> 'ApplicationNotes',
	0x0303	=> 'RenderingIntent',
	0x03e7	=> 'USPTOMiscellaneous',
	0x1000	=> 'RelatedImageFileFormat',
	0x1001	=> 'RelatedImageWidth',
	0x1002	=> 'RelatedImageHeight',
	0x4746	=> 'Rating',
	0x4747	=> 'XP_DIP_XML',
	0x4748	=> 'StitchInfo',
	0x4749	=> 'RatingPercent',
	0x5001	=> 'ResolutionXUnit',
	0x5002	=> 'ResolutionYUnit',
	0x5003	=> 'ResolutionXLengthUnit',
	0x5004	=> 'ResolutionYLengthUnit',
	0x5005	=> 'PrintFlags',
	0x5006	=> 'PrintFlagsVersion',
	0x5007	=> 'PrintFlagsCrop',
	0x5008	=> 'PrintFlagsBleedWidth',
	0x5009	=> 'PrintFlagsBleedWidthScale',
	0x500a	=> 'HalftoneLPI',
	0x500b	=> 'HalftoneLPIUnit',
	0x500c	=> 'HalftoneDegree',
	0x500d	=> 'HalftoneShape',
	0x500e	=> 'HalftoneMisc',
	0x500f	=> 'HalftoneScreen',
	0x5010	=> 'JPEGQuality',
	0x5011	=> 'GridSize',
	0x5012	=> 'ThumbnailFormat',
	0x5013	=> 'ThumbnailWidth',
	0x5014	=> 'ThumbnailHeight',
	0x5015	=> 'ThumbnailColorDepth',
	0x5016	=> 'ThumbnailPlanes',
	0x5017	=> 'ThumbnailRawBytes',
	0x5018	=> 'ThumbnailLength',
	0x5019	=> 'ThumbnailCompressedSize',
	0x501a	=> 'ColorTransferFunction',
	0x501b	=> 'ThumbnailData',
	0x5020	=> 'ThumbnailImageWidth',
	0x5021	=> 'ThumbnailImageHeight',
	0x5022	=> 'ThumbnailBitsPerSample',
	0x5023	=> 'ThumbnailCompression',
	0x5024	=> 'ThumbnailPhotometricInterp',
	0x5025	=> 'ThumbnailDescription',
	0x5026	=> 'ThumbnailEquipMake',
	0x5027	=> 'ThumbnailEquipModel',
	0x5028	=> 'ThumbnailStripOffsets',
	0x5029	=> 'ThumbnailOrientation',
	0x502a	=> 'ThumbnailSamplesPerPixel',
	0x502b	=> 'ThumbnailRowsPerStrip',
	0x502c	=> 'ThumbnailStripByteCounts',
	0x502d	=> 'ThumbnailResolutionX',
	0x502e	=> 'ThumbnailResolutionY',
	0x502f	=> 'ThumbnailPlanarConfig',
	0x5030	=> 'ThumbnailResolutionUnit',
	0x5031	=> 'ThumbnailTransferFunction',
	0x5032	=> 'ThumbnailSoftware',
	0x5033	=> 'ThumbnailDateTime',
	0x5034	=> 'ThumbnailArtist',
	0x5035	=> 'ThumbnailWhitePoint',
	0x5036	=> 'ThumbnailPrimaryChromaticities',
	0x5037	=> 'ThumbnailYCbCrCoefficients',
	0x5038	=> 'ThumbnailYCbCrSubsampling',
	0x5039	=> 'ThumbnailYCbCrPositioning',
	0x503a	=> 'ThumbnailRefBlackWhite',
	0x503b	=> 'ThumbnailCopyright',
	0x5090	=> 'LuminanceTable',
	0x5091	=> 'ChrominanceTable',
	0x5100	=> 'FrameDelay',
	0x5101	=> 'LoopCount',
	0x5102	=> 'GlobalPalette',
	0x5103	=> 'IndexBackground',
	0x5104	=> 'IndexTransparent',
	0x5110	=> 'PixelUnits',
	0x5111	=> 'PixelsPerUnitX',
	0x5112	=> 'PixelsPerUnitY',
	0x5113	=> 'PaletteHistogram',
	0x7000	=> 'SonyRawFileType',
	0x7010	=> 'SonyToneCurve',
	0x7031	=> 'VignettingCorrection',
	0x7032	=> 'VignettingCorrParams',
	0x7034	=> 'ChromaticAberrationCorrection',
	0x7035	=> 'ChromaticAberrationCorrParams',
	0x7036	=> 'DistortionCorrection',
	0x7037	=> 'DistortionCorrParams',
	0x7038	=> 'SonyRawImageSize',
	0x7310	=> 'BlackLevel',
	0x7313	=> 'WB_RGGBLevels',
	0x74c7	=> 'SonyCropTopLeft',
	0x74c8	=> 'SonyCropSize',
	0x800d	=> 'ImageID',
	0x80a3	=> 'WangTag1',
	0x80a4	=> 'WangAnnotation',
	0x80a5	=> 'WangTag3',
	0x80a6	=> 'WangTag4',
	0x80b9	=> 'ImageReferencePoints',
	0x80ba	=> 'RegionXformTackPoint',
	0x80bb	=> 'WarpQuadrilateral',
	0x80bc	=> 'AffineTransformMat',
	0x80e3	=> 'Matteing',
	0x80e4	=> 'DataType',
	0x80e5	=> 'ImageDepth',
	0x80e6	=> 'TileDepth',
	0x8214	=> 'ImageFullWidth',
	0x8215	=> 'ImageFullHeight',
	0x8216	=> 'TextureFormat',
	0x8217	=> 'WrapModes',
	0x8218	=> 'FovCot',
	0x8219	=> 'MatrixWorldToScreen',
	0x821a	=> 'MatrixWorldToCamera',
	0x827d	=> 'Model2',
	0x828d	=> 'CFARepeatPatternDim',
	0x828e	=> 'CFAPattern2',
	0x828f	=> 'BatteryLevel',
	0x8290	=> 'KodakIFD',
	0x8298	=> 'Copyright',
	0x829a	=> 'ExposureTime',
	0x829d	=> 'FNumber',
	0x82a5	=> 'MDFileTag',
	0x82a6	=> 'MDScalePixel',
	0x82a7	=> 'MDColorTable',
	0x82a8	=> 'MDLabName',
	0x82a9	=> 'MDSampleInfo',
	0x82aa	=> 'MDPrepDate',
	0x82ab	=> 'MDPrepTime',
	0x82ac	=> 'MDFileUnits',
	0x830e	=> 'PixelScale',
	0x8335	=> 'AdventScale',
	0x8336	=> 'AdventRevision',
	0x835c	=> 'UIC1Tag',
	0x835d	=> 'UIC2Tag',
	0x835e	=> 'UIC3Tag',
	0x835f	=> 'UIC4Tag',
	0x83bb	=> 'IPTC-NAA',
	0x847e	=> 'IntergraphPacketData',
	0x847f	=> 'IntergraphFlagRegisters',
	0x8480	=> 'IntergraphMatrix',
	0x8481	=> 'INGRReserved',
	0x8482	=> 'ModelTiePoint',
	0x84e0	=> 'Site',
	0x84e1	=> 'ColorSequence',
	0x84e2	=> 'IT8Header',
	0x84e3	=> 'RasterPadding',
	0x84e4	=> 'BitsPerRunLength',
	0x84e5	=> 'BitsPerExtendedRunLength',
	0x84e6	=> 'ColorTable',
	0x84e7	=> 'ImageColorIndicator',
	0x84e8	=> 'BackgroundColorIndicator',
	0x84e9	=> 'ImageColorValue',
	0x84ea	=> 'BackgroundColorValue',
	0x84eb	=> 'PixelIntensityRange',
	0x84ec	=> 'TransparencyIndicator',
	0x84ed	=> 'ColorCharacterization',
	0x84ee	=> 'HCUsage',
	0x84ef	=> 'TrapIndicator',
	0x84f0	=> 'CMYKEquivalent',
	0x8546	=> 'SEMInfo',
	0x8568	=> 'AFCP_IPTC',
	0x85b8	=> 'PixelMagicJBIGOptions',
	0x85d7	=> 'JPLCartoIFD',
	0x85d8	=> 'ModelTransform',
	0x8602	=> 'WB_GRGBLevels',
	0x8606	=> 'LeafData',
	0x8649	=> 'PhotoshopSettings',
	0x8769	=> 'ExifOffset',
	0x8773	=> 'ICC_Profile',
	0x877f	=> 'TIFF_FXExtensions',
	0x8780	=> 'MultiProfiles',
	0x8781	=> 'SharedData',
	0x8782	=> 'T88Options',
	0x87ac	=> 'ImageLayer',
	0x87af	=> 'GeoTiffDirectory',
	0x87b0	=> 'GeoTiffDoubleParams',
	0x87b1	=> 'GeoTiffAsciiParams',
	0x87be	=> 'JBIGOptions',
	0x8822	=> 'ExposureProgram',
	0x8824	=> 'SpectralSensitivity',
	0x8825	=> 'GPSInfo',
	0x8827	=> 'ISO',
	0x8828	=> 'Opto-ElectricConvFactor',
	0x8829	=> 'Interlace',
	0x882a	=> 'TimeZoneOffset',
	0x882b	=> 'SelfTimerMode',
	0x8830	=> 'SensitivityType',
	0x8831	=> 'StandardOutputSensitivity',
	0x8832	=> 'RecommendedExposureIndex',
	0x8833	=> 'ISOSpeed',
	0x8834	=> 'ISOSpeedLatitudeyyy',
	0x8835	=> 'ISOSpeedLatitudezzz',
	0x885c	=> 'FaxRecvParams',
	0x885d	=> 'FaxSubAddress',
	0x885e	=> 'FaxRecvTime',
	0x8871	=> 'FedexEDR',
	0x888a	=> 'LeafSubIFD',
	0x9000	=> 'ExifVersion',
	0x9003	=> 'DateTimeOriginal',
	0x9004	=> 'CreateDate',
	0x9009	=> 'GooglePlusUploadCode',
	0x9010	=> 'OffsetTime',
	0x9011	=> 'OffsetTimeOriginal',
	0x9012	=> 'OffsetTimeDigitized',
	0x9101	=> 'ComponentsConfiguration',
	0x9102	=> 'CompressedBitsPerPixel',
	0x9201	=> 'ShutterSpeedValue',
	0x9202	=> 'ApertureValue',
	0x9203	=> 'BrightnessValue',
	0x9204	=> 'ExposureCompensation',
	0x9205	=> 'MaxApertureValue',
	0x9206	=> 'SubjectDistance',
	0x9207	=> 'MeteringMode',
	0x9208	=> 'LightSource',
	0x9209	=> 'Flash',
	0x920a	=> 'FocalLength',
	0x920b	=> 'FlashEnergy',
	0x920c	=> 'SpatialFrequencyResponse',
	0x920d	=> 'Noise',
	0x920e	=> 'FocalPlaneXResolution',
	0x920f	=> 'FocalPlaneYResolution',
	0x9210	=> 'FocalPlaneResolutionUnit',
	0x9211	=> 'ImageNumber',
	0x9212	=> 'SecurityClassification',
	0x9213	=> 'ImageHistory',
	0x9214	=> 'SubjectArea',
	0x9215	=> 'ExposureIndex',
	0x9216	=> 'TIFF-EPStandardID',
	0x9217	=> 'SensingMethod',
	0x923a	=> 'CIP3DataFile',
	0x923b	=> 'CIP3Sheet',
	0x923c	=> 'CIP3Side',
	0x923f	=> 'StoNits',
	0x927c	=> 'MakerNoteApple',
	0x9286	=> 'UserComment',
	0x9290	=> 'SubSecTime',
	0x9291	=> 'SubSecTimeOriginal',
	0x9292	=> 'SubSecTimeDigitized',
	0x932f	=> 'MSDocumentText',
	0x9330	=> 'MSPropertySetStorage',
	0x9331	=> 'MSDocumentTextPosition',
	0x935c	=> 'ImageSourceData',
	0x9400	=> 'AmbientTemperature',
	0x9401	=> 'Humidity',
	0x9402	=> 'Pressure',
	0x9403	=> 'WaterDepth',
	0x9404	=> 'Acceleration',
	0x9405	=> 'CameraElevationAngle',
	0x9999	=> 'XiaomiSettings',
	0x9a00	=> 'XiaomiModel',
	0x9c9b	=> 'XPTitle',
	0x9c9c	=> 'XPComment',
	0x9c9d	=> 'XPAuthor',
	0x9c9e	=> 'XPKeywords',
	0x9c9f	=> 'XPSubject',
	0xa000	=> 'FlashpixVersion',
	0xa001	=> 'ColorSpace',
	0xfffd	=> '=',
	0xfffe	=> '=',
	0xffff	=> '=',
	0xa002	=> 'ExifImageWidth',
	0xa003	=> 'ExifImageHeight',
	0xa004	=> 'RelatedSoundFile',
	0xa005	=> 'InteropOffset',
	0xa010	=> 'SamsungRawPointersOffset',
	0xa011	=> 'SamsungRawPointersLength',
	0xa101	=> 'SamsungRawByteOrder',
	0xa102	=> 'SamsungRawUnknown?',
	0xa20b	=> 'FlashEnergy',
	0xa20c	=> 'SpatialFrequencyResponse',
	0xa20d	=> 'Noise',
	0xa20e	=> 'FocalPlaneXResolution',
	0xa20f	=> 'FocalPlaneYResolution',
	0xa210	=> 'FocalPlaneResolutionUnit',
	0xa211	=> 'ImageNumber',
	0xa212	=> 'SecurityClassification',
	0xa213	=> 'ImageHistory',
	0xa214	=> 'SubjectLocation',
	0xa215	=> 'ExposureIndex',
	0xa216	=> 'TIFF-EPStandardID',
	0xa217	=> 'SensingMethod',
	0xa300	=> 'FileSource',
	0xa301	=> 'SceneType',
	0xa302	=> 'CFAPattern',
	0xa401	=> 'CustomRendered',
	0xa402	=> 'ExposureMode',
	0xa403	=> 'WhiteBalance',
	0xa404	=> 'DigitalZoomRatio',
	0xa405	=> 'FocalLengthIn35mmFormat',
	0xa406	=> 'SceneCaptureType',
	0xa407	=> 'GainControl',
	0xa408	=> 'Contrast',
	0xa409	=> 'Saturation',
	0xa40a	=> 'Sharpness',
	0xa40b	=> 'DeviceSettingDescription',
	0xa40c	=> 'SubjectDistanceRange',
	0xa420	=> 'ImageUniqueID',
	0xa430	=> 'OwnerName',
	0xa431	=> 'SerialNumber',
	0xa432	=> 'LensInfo',
	0xa433	=> 'LensMake',
	0xa434	=> 'LensModel',
	0xa435	=> 'LensSerialNumber',
	0xa436	=> 'Title',
	0xa437	=> 'Photographer',
	0xa438	=> 'ImageEditor',
	0xa439	=> 'CameraFirmware',
	0xa43a	=> 'RAWDevelopingSoftware',
	0xa43b	=> 'ImageEditingSoftware',
	0xa43c	=> 'MetadataEditingSoftware',
	0xa460	=> 'CompositeImage',
	0xa461	=> 'CompositeImageCount',
	0xa462	=> 'CompositeImageExposureTimes',
	0xa480	=> 'GDALMetadata',
	0xa481	=> 'GDALNoData',
	0xa500	=> 'Gamma',
	0xafc0	=> 'ExpandSoftware',
	0xafc1	=> 'ExpandLens',
	0xafc2	=> 'ExpandFilm',
	0xafc3	=> 'ExpandFilterLens',
	0xafc4	=> 'ExpandScanner',
	0xafc5	=> 'ExpandFlashLamp',
	0xb4c3	=> 'HasselbladRawImage',
	0xbc01	=> 'PixelFormat',
	0xbc02	=> 'Transformation',
	0xbc03	=> 'Uncompressed',
	0xbc04	=> 'ImageType',
	0xbc80	=> 'ImageWidth',
	0xbc81	=> 'ImageHeight',
	0xbc82	=> 'WidthResolution',
	0xbc83	=> 'HeightResolution',
	0xbcc0	=> 'ImageOffset',
	0xbcc1	=> 'ImageByteCount',
	0xbcc2	=> 'AlphaOffset',
	0xbcc3	=> 'AlphaByteCount',
	0xbcc4	=> 'ImageDataDiscard',
	0xbcc5	=> 'AlphaDataDiscard',
	0xc427	=> 'OceScanjobDesc',
	0xc428	=> 'OceApplicationSelector',
	0xc429	=> 'OceIDNumber',
	0xc42a	=> 'OceImageLogic',
	0xc44f	=> 'Annotations',
	0xc4a5	=> 'PrintIM',
	0xc519	=> 'HasselbladXML',
	0xc51b	=> 'HasselbladExif',
	0xc573	=> 'OriginalFileName',
	0xc580	=> 'USPTOOriginalContentType',
	0xc5e0	=> 'CR2CFAPattern',
	0xc612	=> 'DNGVersion',
	0xc613	=> 'DNGBackwardVersion',
	0xc614	=> 'UniqueCameraModel',
	0xc615	=> 'LocalizedCameraModel',
	0xc616	=> 'CFAPlaneColor',
	0xc617	=> 'CFALayout',
	0xc618	=> 'LinearizationTable',
	0xc619	=> 'BlackLevelRepeatDim',
	0xc61a	=> 'BlackLevel',
	0xc61b	=> 'BlackLevelDeltaH',
	0xc61c	=> 'BlackLevelDeltaV',
	0xc61d	=> 'WhiteLevel',
	0xc61e	=> 'DefaultScale',
	0xc61f	=> 'DefaultCropOrigin',
	0xc620	=> 'DefaultCropSize',
	0xc621	=> 'ColorMatrix1',
	0xc622	=> 'ColorMatrix2',
	0xc623	=> 'CameraCalibration1',
	0xc624	=> 'CameraCalibration2',
	0xc625	=> 'ReductionMatrix1',
	0xc626	=> 'ReductionMatrix2',
	0xc627	=> 'AnalogBalance',
	0xc628	=> 'AsShotNeutral',
	0xc629	=> 'AsShotWhiteXY',
	0xc62a	=> 'BaselineExposure',
	0xc62b	=> 'BaselineNoise',
	0xc62c	=> 'BaselineSharpness',
	0xc62d	=> 'BayerGreenSplit',
	0xc62e	=> 'LinearResponseLimit',
	0xc62f	=> 'CameraSerialNumber',
	0xc630	=> 'DNGLensInfo',
	0xc631	=> 'ChromaBlurRadius',
	0xc632	=> 'AntiAliasStrength',
	0xc633	=> 'ShadowScale',
	0xc634	=> 'SR2Private',
	0xc635	=> 'MakerNoteSafety',
	0xc640	=> 'RawImageSegmentation',
	0xc65a	=> 'CalibrationIlluminant1',
	0xc65b	=> 'CalibrationIlluminant2',
	0xc65c	=> 'BestQualityScale',
	0xc65d	=> 'RawDataUniqueID',
	0xc660	=> 'AliasLayerMetadata',
	0xc68b	=> 'OriginalRawFileName',
	0xc68c	=> 'OriginalRawFileData',
	0xc68d	=> 'ActiveArea',
	0xc68e	=> 'MaskedAreas',
	0xc68f	=> 'AsShotICCProfile',
	0xc690	=> 'AsShotPreProfileMatrix',
	0xc691	=> 'CurrentICCProfile',
	0xc692	=> 'CurrentPreProfileMatrix',
	0xc6bf	=> 'ColorimetricReference',
	0xc6c5	=> 'SRawType',
	0xc6d2	=> 'PanasonicTitle',
	0xc6d3	=> 'PanasonicTitle2',
	0xc6f3	=> 'CameraCalibrationSig',
	0xc6f4	=> 'ProfileCalibrationSig',
	0xc6f5	=> 'ProfileIFD',
	0xc6f6	=> 'AsShotProfileName',
	0xc6f7	=> 'NoiseReductionApplied',
	0xc6f8	=> 'ProfileName',
	0xc6f9	=> 'ProfileHueSatMapDims',
	0xc6fa	=> 'ProfileHueSatMapData1',
	0xc6fb	=> 'ProfileHueSatMapData2',
	0xc6fc	=> 'ProfileToneCurve',
	0xc6fd	=> 'ProfileEmbedPolicy',
	0xc6fe	=> 'ProfileCopyright',
	0xc714	=> 'ForwardMatrix1',
	0xc715	=> 'ForwardMatrix2',
	0xc716	=> 'PreviewApplicationName',
	0xc717	=> 'PreviewApplicationVersion',
	0xc718	=> 'PreviewSettingsName',
	0xc719	=> 'PreviewSettingsDigest',
	0xc71a	=> 'PreviewColorSpace',
	0xc71b	=> 'PreviewDateTime',
	0xc71c	=> 'RawImageDigest',
	0xc71d	=> 'OriginalRawFileDigest',
	0xc71e	=> 'SubTileBlockSize',
	0xc71f	=> 'RowInterleaveFactor',
	0xc725	=> 'ProfileLookTableDims',
	0xc726	=> 'ProfileLookTableData',
	0xc740	=> 'OpcodeList1',
	0xc741	=> 'OpcodeList2',
	0xc74e	=> 'OpcodeList3',
	0xc761	=> 'NoiseProfile',
	0xc763	=> 'TimeCodes',
	0xc764	=> 'FrameRate',
	0xc772	=> 'TStop',
	0xc789	=> 'ReelName',
	0xc791	=> 'OriginalDefaultFinalSize',
	0xc792	=> 'OriginalBestQualitySize',
	0xc793	=> 'OriginalDefaultCropSize',
	0xc7a1	=> 'CameraLabel',
	0xc7a3	=> 'ProfileHueSatMapEncoding',
	0xc7a4	=> 'ProfileLookTableEncoding',
	0xc7a5	=> 'BaselineExposureOffset',
	0xc7a6	=> 'DefaultBlackRender',
	0xc7a7	=> 'NewRawImageDigest',
	0xc7a8	=> 'RawToPreviewGain',
	0xc7aa	=> 'CacheVersion',
	0xc7b5	=> 'DefaultUserCrop',
	0xc7d5	=> 'NikonNEFInfo',
	0xc7e9	=> 'DepthFormat',
	0xc7ea	=> 'DepthNear',
	0xc7eb	=> 'DepthFar',
	0xc7ec	=> 'DepthUnits',
	0xc7ed	=> 'DepthMeasureType',
	0xc7ee	=> 'EnhanceParams',
	0xcd2d	=> 'ProfileGainTableMap',
	0xcd2e	=> 'SemanticName',
	0xcd30	=> 'SemanticInstanceID',
	0xcd31	=> 'CalibrationIlluminant3',
	0xcd32	=> 'CameraCalibration3',
	0xcd33	=> 'ColorMatrix3',
	0xcd34	=> 'ForwardMatrix3',
	0xcd35	=> 'IlluminantData1',
	0xcd36	=> 'IlluminantData2',
	0xcd37	=> 'IlluminantData3',
	0xcd38	=> 'MaskSubArea',
	0xcd39	=> 'ProfileHueSatMapData3',
	0xcd3a	=> 'ReductionMatrix3',
	0xcd3b	=> 'RGBTables',
	0xcd40	=> 'ProfileGainTableMap2',
	0xcd41	=> 'JUMBF',
	0xcd43	=> 'ColumnInterleaveFactor',
	0xcd44	=> 'ImageSequenceInfo',
	0xcd46	=> 'ImageStats',
	0xcd47	=> 'ProfileDynamicRange',
	0xcd48	=> 'ProfileGroupName',
	0xea1c	=> 'Padding',
	0xea1d	=> 'OffsetSchema',
	0xfde8	=> 'OwnerName',
	0xfde9	=> 'SerialNumber',
	0xfdea	=> 'Lens',
	0xfe00	=> 'KDC_IFD',
	0xfe4c	=> 'RawFile',
	0xfe4d	=> 'Converter',
	0xfe4e	=> 'WhiteBalance',
	0xfe51	=> 'Exposure',
	0xfe52	=> 'Shadows',
	0xfe53	=> 'Brightness',
	0xfe54	=> 'Contrast',
	0xfe55	=> 'Saturation',
	0xfe56	=> 'Sharpness',
	0xfe57	=> 'Smoothness',
	0xfe58	=> 'MoireFilter',
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
