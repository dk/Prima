package Prima::PS::PDF;

use strict;
use warnings;
use Encode;
use Prima;
use Prima::PS::CFF;
use Prima::PS::Format;
use Prima::PS::TempFile;
use base qw(Prima::PS::Drawable);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		compress         => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	$self-> {compress}    = 1;
	my %profile = $self-> SUPER::init(@_);
	$self-> $_( $profile{$_}) for qw( compress);
	return %profile;
}

sub cmd_rgb
{
	my ( $r, $g, $b) = (
		int((($_[1] & 0xff0000) >> 16) * 100 / 256 + 0.5) / 100,
		int((($_[1] & 0xff00) >> 8) * 100 / 256 + 0.5) / 100,
		int(($_[1] & 0xff)*100/256 + 0.5) / 100);
	unless ( $_[0]-> {grayscale}) {
		return "$r $g $b RG";
	} else {
		my $i = int( 100 * ( 0.31 * $r + 0.5 * $g + 0.18 * $b) + 0.5) / 100;
		return "$i G";
	}
}

sub emit
{
	my ($self, $data, $raw) = @_;
	return 0 unless $self-> {can_draw};
	my $eol = $raw ? '' : "\n";
	$self-> {ps_data} .= $data . $eol;
	$self-> {content_size} += length($data . $eol);

	if ( length($self-> {ps_data}) > 10240) {
		$self-> abort_doc unless $self-> spool( $self-> {ps_data});
		$self-> {ps_data} = '';
	}

	return 1;
}

sub emit_content
{
	my $self = $_[0];
	return 0 unless $self-> {can_draw};

	my $obj = $self->{objects}->[$self->{page_content}] or return 0;
	return $obj->write($_[1] . "\n");
}

sub change_transform
{
	my ( $self, $gsave ) = @_;
	return if $self-> {delay};

	my ($ps, $pm) = @{ $self }{ qw(pageSize pageMargins) };
	my @pm = (
		@$pm[0,1],
		$ps->[0] - $pm->[2] - $pm->[0],
		$ps->[1] - $pm->[3] - $pm->[1]
	);
	@pm[0,1] = (0,0) if $self->{lock_margins};

	my ($doClip, @cr);
	my $rg = $self-> region;

	unless ( $rg ) {
		@cr = $self-> clipRect;
		$cr[2] -= $cr[0];
		$cr[3] -= $cr[1];
		$doClip = grep { $_ != 0 } @cr;

		@cr = $self-> pixel2point( @cr);
		float_inplace(@cr);
	}

	$self-> emit_content('Q') unless $gsave;
	$self-> emit_content('q');

	$self-> emit_content("@pm re W n");

	my $m = $self-> get_matrix;
	if ( $self->{reversed}) {
		$$m[5] -= ($self->point2pixel($pm[2]))[0];
		$m->rotate(90);
	}
	my @tm = $self-> pixel2point( @$m[4,5] );
	$tm[$_] += $pm[$_] for 0,1;
	my @xm = @$m[0..3];
	float_inplace(@pm, @xm, @tm);
	$self-> emit_content("@xm @tm cm");

	if ( $rg ) {
		$self-> emit_content($rg-> apply_clip($self));
	} elsif ( $doClip ) {
		$self-> emit_content("h @cr re W n");
	}
	$self-> {changed}-> {$_} = 1 for qw(fill linePattern lineWidth lineJoin lineEnd miterLimit font);
}

sub fill
{
	my ( $self, $code) = @_;
	my ( $r1, $r2) = ( $self-> rop, $self-> rop2);
	return if
		$r1 == rop::NoOper &&
		$r2 == rop::NoOper;

	if ( $self-> {changed}-> {alpha}) {
		my $al = $self-> alpha;
		$self-> emit_content( "/GSA$al gs");
		$self-> {changed}-> {alpha} = 0;
	}
	if ( $r2 != rop::NoOper && $self-> {fpType} ne 'F' && !$self->{color_patterns}->{$self->{fpType}}) {
		my $bk =
			( $r2 == rop::Blackness) ? 0 :
			( $r2 == rop::Whiteness) ? 0xffffff : $self-> backColor;

		$self-> {changed}-> {fill} = 1;
		$self-> emit_content( lc $self-> cmd_rgb( $bk));
		$self-> emit_content( $code);
	}
	if ( $r1 != rop::NoOper && $self-> {fpType} ne 'B') {
		my $c =
			( $r1 == rop::Blackness) ? 0 :
			( $r1 == rop::Whiteness) ? 0xffffff : $self-> color;
		if ($self-> {changed}-> {fill}) {
			if ( $self-> {fpType} eq 'F') {
				$self-> emit_content( lc $self-> cmd_rgb( $c));
			} elsif ( $self->{color_patterns}->{$self->{fpType}} ) {
				$self-> emit_content("/Pattern cs /P$self->{fpType} scn");
			} else {
				my ( $r, $g, $b) = (
					int((($c & 0xff0000) >> 16) * 100 / 256 + 0.5) / 100,
					int((($c & 0xff00) >> 8) * 100 / 256 + 0.5) / 100,
					int(($c & 0xff)*100/256 + 0.5) / 100);
				my $color;
				if ( $self-> {grayscale}) {
					my $i = int( 100 * ( 0.31 * $r + 0.5 * $g + 0.18 * $b) + 0.5) / 100;
					$color = $i;
				} else {
					$color = "$r $g $b";
				}
				$self-> emit_content("/CS cs $color /P$self->{fpType} scn");
			}
			$self-> {changed}-> {fill} = 0;
		}
		$self-> emit_content( $code);
	}
}

sub stroke
{
	my ( $self, $code) = @_;

	my ( $r1, $r2) = ( $self-> rop, $self-> rop2);
	my $lp = $self-> linePattern;
	return if
		$r1 == rop::NoOper &&
		$r2 == rop::NoOper;

	my $lw_changed;
	if ( $self-> {changed}-> {lineWidth}) {
		my ($lw) = $self-> pixel2point($self-> lineWidth);
		$self-> emit_content( float_format($lw) . ' w');
		$self-> {changed}-> {lineWidth} = 0;
		$lw_changed = 1;
	}

	if ( $self-> {changed}-> {lineEnd}) {
		my $le = $self-> lineEnd;
		my $id = ( $le == le::Round) ? 1 : (( $le == le::Square) ? 2 : 0);
		$self-> emit_content( "$id J");
		$self-> {changed}-> {lineEnd} = 0;
	}

	if ( $self-> {changed}-> {lineJoin}) {
		my $lj = $self-> lineJoin;
		my $id = ( $lj == lj::Round) ? 1 : (( $lj == lj::Bevel) ? 2 : 0);
		$self-> emit_content( "$id j");
		$self-> {changed}-> {lineJoin} = 0;
	}

	if ( $self-> {changed}-> {miterLimit}) {
		my $ml = $self-> miterLimit;
		$self-> emit_content( float_format($ml) . " M");
		$self-> {changed}-> {miterLimit} = 0;
	}

	if ( $self-> {changed}-> {alpha}) {
		my $al = $self-> alpha;
		$self-> emit_content( "/GSA$al gs");
		$self-> {changed}-> {alpha} = 0;
	}

	if ( $r2 != rop::NoOper && $lp ne lp::Solid ) {
		my $bk =
			( $r2 == rop::Blackness) ? 0 :
			( $r2 == rop::Whiteness) ? 0xffffff : $self-> backColor;

		$self-> {changed}-> {linePattern} = 1;
		$self-> {changed}-> {fill}        = 1;
		$self-> emit_content('[] 0 d');
		$self-> emit_content( uc $self-> cmd_rgb( $bk));
		$self-> emit_content( $code);
	}

	if ( $r1 != rop::NoOper && length( $lp)) {
		my $fk =
			( $r1 == rop::Blackness) ? 0 :
			( $r1 == rop::Whiteness) ? 0xffffff : $self-> color;

		if ( $self-> {changed}-> {linePattern} || $lw_changed) {
			if ( length( $lp) == 1) {
				$self-> emit_content('[] 0 d') if $self->{changed}->{linePattern};
			} else {
				my $lw = $self->lineWidth || 2.0; # because to be divided by 2
				my @x = map { ord } split('', $lp);
				push( @x, 0) if scalar(@x) % 1;
				my @lp;
				for ( my $i = 0; $i < @x; $i += 2 ) {
					push @lp, ($x[$i] - 1) * $lw / 2;
					push @lp, ($x[$i+1]) * $lw / 2;
				}
				@lp = map { $_ || 1 } @lp;
				$self-> emit_content("[@lp] 0 d");
			}
			$self-> {changed}-> {linePattern} = 0;
		}

		if ( $self-> {changed}-> {fill}) {
			$self-> emit_content( uc $self-> cmd_rgb( $fk));
			$self-> {changed}-> {fill} = 0;
		}

		$self-> emit_content( $code);
	}
}

sub new_dummy_obj
{
	my $self = shift;
	my $xid = @{ $self->{objects} };
	push @{ $self->{objects} }, undef;
	return $xid;
}

sub new_file_obj
{
	my ($self, %opt) = @_;
	my $obj = Prima::PS::TempFile->new(compress => $self->{compress}, %opt) or return;
	my $xid = @{ $self->{objects} };
	push @{ $self->{objects} }, $obj;
	$obj->{__xid} = $xid;
	return wantarray ? ( $xid, $obj) : $xid;
}

sub new_stream_obj
{
	my $self = shift;
	my $xid = $self->new_dummy_obj;
	return $xid, { content => '', xid => $xid };
}

sub emit_to_stream
{
	my ( $self, $obj, $text ) = @_;
	$obj->{content} .= $text;
}

sub emit_stream_obj
{
	my ( $self, $obj, $text ) = @_;
	$self-> add_xref($obj->{xid});
	$self-> emit("$obj->{xid} 0 obj\n<<\n/Length ".length $obj->{content});
	$self-> emit( $text ) if defined $text;
	$self-> emit(">>\nstream");
	$self-> emit($obj->{content});
	$self-> emit("endstream\nendobj\n");
}

sub emit_new_stream_object
{
	my ( $self, $stream, $text ) = @_;
	my $xid = $self->new_dummy_obj;
	$self-> add_xref($xid);
	my $length = length($stream);
	$self-> emit("$xid 0 obj\n<<\n/Length ".length($stream));
	$self-> emit( $text ) if defined $text;
	$self-> emit(">>\nstream");
	$self-> emit($stream);
	$self-> emit("endstream\nendobj\n");
	return $xid;
}

sub emit_file_obj
{
	my ( $self, $obj, $text ) = @_;
	$self-> add_xref($obj->{__xid});
	my $compress = $obj-> is_deflated;
	$obj-> reset;
	$self-> emit("$obj->{__xid} 0 obj\n<<\n/Length ".$obj->{size});
	$self-> emit("/Filter /FlateDecode") if $compress;
	$self-> emit( $text ) if defined $text;
	$self-> emit(">>\nstream");
	$obj->  evacuate( sub { $self->emit( $_[0], 1 ) } );
	$self-> emit("\nendstream\nendobj\n");
}

sub add_xref
{
	my ($self, $xid) = @_;
	$self->{xref}->[ $xid ] = $self->{content_size};
}

sub emit_new_object
{
	my ($self, $xid, $emit) = @_;
	$self-> add_xref($xid);
	$self-> emit("$xid 0 obj");
	$self-> emit($emit) if defined $emit;
}

sub emit_new_dummy_object
{
	my ($self, $emit) = @_;
	my $xid = $self-> new_dummy_obj;
	$self-> add_xref($xid);
	$self-> emit("$xid 0 obj\n<<");
	$self-> emit($emit) if defined $emit;
	$self-> emit(">>\nendobj\n");
	return $xid;
}

sub begin_doc
{
	my ( $self, $docName) = @_;
	$@ = 'Bad object', return 0 if $self-> get_paint_state;

	$self-> {ps_data}  = '';
	$self-> {can_draw} = 1;
	$self-> {content_size} = 0;

	$docName = $::application ? $::application-> name : "Prima::PS::PDF"
		unless defined $docName;
	$docName = Encode::encode('UTF-16', $docName)
		if Encode::is_utf8($docName);
	$self-> {fp_hash}  = {};
	$self-> {xref} = [];

	my ($sec,$min,$hour,$mday,$mon,$year) = localtime;
	my $date = sprintf("%04d%02d%02d%02d%02d%02d", $year + 1900, $mon, $mday, $hour, $min, $sec);
	my $four = pack('C*', 0xde, 0xad, 0xbe, 0xef);
	$self-> emit( <<PDFHEADER);
%PDF-1.4
%$four
PDFHEADER

	$self-> emit_new_object(1, <<PDFINFO);
<<
/CreationDate (D:$date+00'00)
/Creator (Prima::PS::PDF)
/Title ($docName)
>>
endobj
PDFINFO
	$self-> emit_new_object(2, <<ROOT);
<<
/Type /Catalog
/Pages 3 0 R
>>
endobj
ROOT

	$self-> {objects} = [(undef) x 4];
	$self-> {page_object}   = $self->new_dummy_obj;
	$self-> {pages}         = [$self->{page_object} ];
	$self-> {page_refs}     = [];
	$self-> {page_patterns} = {};
	$self-> {color_patterns}= {};
	$self-> {page_images}   = [];
	$self-> {page_fonts}    = {};
	$self-> {page_rops}     = {};
	$self-> {page_alphas}   = {};
	$self-> {all_rops}      = {};
	$self-> {all_fonts}     = {};
	$self-> {all_alphas}    = {};
	unless ($self-> {page_content} = $self->new_file_obj) {
		$self-> abort_doc;
		$@ = 'Cannot create temp file', return 0;
	}

	$self-> {changed} = { map { $_ => 0 } qw(
		fill lineEnd linePattern lineWidth lineJoin miterLimit font)};

	$self-> {paint_state} = ps::Enabled;
	$self-> save_state;

	$self-> {delay} = 1;
	$self-> restore_state;
	$self-> {delay} = 0;

	$self-> change_transform( 1);
	$self-> {changed}-> {linePattern} = 0;

	return 1;
}

sub end_page
{
	my $self = shift;

	$self-> emit_content('Q');

	my @ps = @{ $self->{pageSize} };
	$self-> emit_new_object($self->{page_object}, <<PAGE);
<<
/Type /Page
/Parent 3 0 R
/MediaBox [ 0 0 @ps ]
/StructParents 0
/Contents $self->{page_content} 0 R
/ProcSet [ /PDF /Text /ImageB /ImageC /ImageI ]
/Resources <<
/ColorSpace <<
/CS [ /Pattern /Device${ \( $self->{grayscale} ? 'Gray' : 'RGB' ) } ]
>>
PAGE
	if ( keys %{ $self->{page_patterns} } ) {
		$self-> emit("/Pattern <<");
		for my $xid ( keys %{ $self->{page_patterns} } ) {
			$self-> emit("/P$xid $xid 0 R");
		}
		$self-> emit(">>");
	}
	if ( @{$self->{page_images} } ) {
		$self-> emit("/XObject <<");
		for my $xid ( @{ $self->{page_images} } ) {
			$self-> emit("/I$xid $xid 0 R");
		}
		$self-> emit(">>");
	}
	if ( keys %{ $self->{page_fonts} } ) {
		$self-> emit("/Font <<");
		for my $xid ( keys %{ $self->{page_fonts} } ) {
			$self-> emit("/F$xid $xid 0 R");
		}
		$self-> emit(">>");
	}

	if ( keys %{ $self->{page_rops} } || keys %{ $self->{page_alphas} } ) {
		$self-> emit("/ExtGState <<");
		while ( my ( $name, $xid ) = each %{ $self->{page_rops} } ) {
			$self-> emit("/GS$name $xid 0 R");
		}
		while ( my ( $name, $xid ) = each %{ $self->{page_alphas} } ) {
			$self-> emit("/GSA$name $xid 0 R");
		}
		$self-> emit(">>");
	}

	$self-> emit(">>"); # % Resources

	if ( @{ $self->{page_refs} } ) {
		$self-> emit("/XObject <<");
		for my $xid ( @{ $self->{page_refs} } ) {
			$self-> emit("/X$xid $xid 0 R");
		}
		$self-> emit(">>");
	}
	$self-> emit(">>\nendobj\n");

	$self-> emit_file_obj($self->{objects}->[$self->{page_content}]);
	undef $self->{objects}->[$self->{page_content}];
}

sub abort_doc
{
	my $self = $_[0];
	return unless $self-> {can_draw};
	$self-> {can_draw} = 0;
	delete $self->{paint_state};
	$self-> abandon_state;
	delete $self-> {$_} for
		qw (save_state ps_data changed );
}

sub begin_paint { return $_[0]-> begin_doc; }
sub end_paint   {        $_[0]-> abort_doc; }

sub end_doc
{
	my $self = $_[0];
	$@ = 'Bad object', return 0 unless $self-> {can_draw};
	$self-> end_page;

	my $pages = scalar @{ $self->{pages} };
	my @kids = map { "$_ 0 R" } @{ $self->{pages} };

	$self-> emit_new_object(3, <<ENDS);
<<
/Type /Pages
/Count $pages
/Kids [@kids]
>>
endobj
ENDS

	my $encoding = $self-> new_dummy_obj;
	$self-> emit_new_object($encoding, <<ENCODING);
<<
/Type /Encoding
/Differences [ 0
ENCODING
	for my $x (0..15) {
		my $n = $x * 16;
		$self-> emit( join(' ', map { "/a" . ($n + $_) } 0..15));
	}
	$self-> emit( <<END );
]
>>
endobj
END

	while ( my ( $font, $v ) = each %{ $self->{all_fonts} }) {
		next if $v->{native};

		$self-> {glyph_keeper}-> begin_evacuate( $font );

		for my $xid ( @{ $v->{xids} } ) {
			my ( $frec, $charset, $unicode, $width, $content) = $self-> {glyph_keeper}-> evacuate_next_subfont( $font );

			my $font_file = $self-> emit_new_stream_object( $content, "/Subtype /Type1C");

			my $font_desc = $self-> new_dummy_obj;
			my $charset_str = join('', map { "/$_" } @$charset);
			my @bbox = Prima::Utils::nearest_i( map { $_ // 0 } @{ $frec->{bbox} } );

			$self-> emit_new_object($font_desc, <<FONT);
<<
/Type /FontDescriptor
/CharSet ($charset_str)
/FontBBox [ @bbox ]
/FontFile3 $font_file 0 R
/FontName /$font
/Flags 4
/ItalicAngle $frec->{italic}
>>
endobj

FONT

			my ($unicode_xid, $unicode_stream) = $self-> new_stream_obj;
			my $n_cps = 0;
			my $maps = '';
			$self-> emit_to_stream( $unicode_stream, <<UNICODE);
/CIDInit /ProcSet findresource begin
12 dict begin
begincmap
/CMapType 2 def
1 begincodespacerange
<00><ff>
endcodespacerange
UNICODE
			my @codes;
			while ( my ( $i, $u ) = each @$unicode ) {
				$u += 0;
				if ( $u >= 0x10000 && $u <= 0x10FFFF ) {
					$u -= 0x10000;
					push @codes, sprintf("<%02x><%04x%04x>", $i,
						0xd800 + ($u & 0x3ff),
						0xdc00 + ($u >> 10)
					);
				} elsif (( $u >= 0xD800 && $u <= 0xDFFF ) || ( $u > 0x10FFFF ) || ( $u == 0 )) {
					next;
				} else {
					push @codes, sprintf("<%02x><%04x>", $i, $u);
				}
			}
			while ( @codes ) {
				my @section = splice( @codes, 0, 99 ); # spec says max 100
				$self-> emit_to_stream( $unicode_stream, scalar(@section). " beginbfchar\n");
				$self-> emit_to_stream( $unicode_stream, join("\n", @section ));
				$self-> emit_to_stream( $unicode_stream, "\nendbfchar\n");
			}
			$self-> emit_to_stream( $unicode_stream, <<UNICODE);
endcmap
CMapName currentdict /CMap defineresource pop
end end
UNICODE
			$self-> emit_stream_obj( $unicode_stream);

			my $lastchar = $#$charset;
			$self-> emit_new_object($xid, <<FONT);
<<
/Type /Font
/Subtype /Type1
/BaseFont /$font
/Encoding $encoding 0 R
/ToUnicode $unicode_xid 0 R
/FontDescriptor $font_desc 0 R
/FirstChar 0
/LastChar $lastchar
/Widths [
FONT
			$self-> emit( join(' ', splice( @$width, 0, 16 )) )
				while @$width;
			$self-> emit( <<END );
]
>>
endobj
END
		}
	}


	my $xref_offset = $self->{content_size};
	$self->emit("xref");
	my @xrefs = grep { defined } @{ $self->{xref} };
	my $xrefs = 1 + @xrefs;
	$self->emit("0 $xrefs");
	$self->emit(sprintf("%010d %05d f ", 0, 65535));
	for my $xref ( @xrefs ) {
		$self->emit(sprintf("%010d %05d n ", $xref, 0));
	}
	$self->emit(<<TRAILER);
trailer
<<
/Info 1 0 R
/Root 2 0 R
/Size $xrefs
>>
startxref
$xref_offset
%%EOF
TRAILER

	my $ret = $self->spool( $self-> {ps_data} );
	$self->{ps_data} = '';

	$self-> {can_draw} = 0;
	delete $self->{paint_state};
	$self-> restore_state;
	delete $self-> {$_} for
		qw (save_state changed ps_data glyph_keeper glyph_font);
	return $ret;
}

sub new_page
{
	return 0 unless $_[0]-> {can_draw};
	my $self = $_[0];

	$self-> end_page;
	$self-> {page_object}  = $self->new_dummy_obj;
	push @{$self-> {pages}}, $self->{page_object};
	$self-> {page_refs}      = [];
	$self-> {page_patterns}  = {};
	$self-> {color_patterns} = {};
	$self-> {page_images}    = [];
	$self-> {page_fonts}     = {};
	$self-> {page_rops}      = {};
	unless ($self-> {page_content} = $self->new_file_obj) {
		$self-> abort_doc;
		return 0;
	}

	{
		local $self->{delay} = 1;
		$self-> $_( @{$self-> {save_state}-> {$_}}) for qw( translate clipRect);
	}
	$self-> change_transform(1);
	$self-> {changed}->{font} = 1;
	return 1;
}

sub pages { scalar @{ $_[0]-> {pages} } }

sub alpha
{
	return $_[0]->{alpha} unless $#_;
	my ( $self, $alpha ) = @_;
	$alpha = int( $alpha + .5);
	$alpha = 0 if $alpha < 0;
	$alpha = 255 if $alpha > 255;
	return if ($self->{alpha} // -1 ) == $alpha;
	$self->{alpha} = $alpha;
	return unless $self-> get_paint_state;

	my $ca = int( $alpha / 2.55 ) / 100;
	$self-> {all_alphas}->{ $alpha } //= {
		xid => $self-> emit_new_dummy_object("/Type /ExtGState /ca $ca /CA $ca"),
		id  => "GSA$alpha",
	};
	$self-> {page_alphas}-> {$alpha} = $self->{all_alphas}->{$alpha}->{xid};
	$self-> {changed}->{alpha} = 1;
}

sub emit_pattern
{
	my ( $self, $fpid, $fp) = @_;

	my $xid;
	unless ( exists $self-> {fp_hash}-> {$fpid}) {
		$xid  = $self-> new_dummy_obj;
		$self-> {fp_hash}-> {$fpid} = $xid;
		$self->{page_patterns}->{$xid}++;
	} else {
		$xid = $self-> {fp_hash}-> {$fpid};
		$self->{page_patterns}->{$xid}++;
		return $xid;
	}

	my (@sz, $imgdef, $depth, $paint, $proc, $cs, $imagemask, $maskdef);
	$imgdef  = '';
	$maskdef = '';
	$paint     = 2;
	$depth     = '1';
	$proc      = 'ImageB';
	$cs        = 'G';
	$imagemask = 'true';
	if ( ref($fp) eq 'ARRAY') {
		@sz        = (8,8);
		$imgdef    = pack('C*', @$fp);
	} else {
		@sz = $fp->size;
		my ( $ls, $stride, $data );
		if ( $fp->type == im::BW ) {
			$stride    = int($fp-> width / 8) +!!+ ($fp->width % 8);
		} else {
			$paint     = 1;
			$fp        = $self->prepare_image($fp);
			$depth     = $fp->get_bpp;
			$imagemask = 'false';
			if ( $depth != 8 ) {
				$depth   = 8;
				$proc    = 'ImageC';
				$cs      = 'RGB';
			} else {
				$proc    = 'ImageI';
				$cs      = 'G';
			}
			$stride  = $fp->get_bpp * $fp->width / 8;
			$self->{color_patterns}->{$xid} = 1;
		}
		$ls      = $fp->lineSize;
		$data    = $fp->data;
		for my $y ( 0 .. $fp->height - 1 ) {
			$imgdef .= substr($data, $y * $ls, $stride);
		}
	}
	my $patdef  = <<PAT;
q
BI
/IM $imagemask
/W $sz[0]
/H $sz[1]
/BPC $depth
/CS /$cs
ID $imgdef
EI Q
PAT

	# PatternType 1 = Tiling pattern
	# PaintType   1 = Colored
	# PaintType   2 = Uncolored
	$self-> emit_new_object( $xid, <<PATTERNDEF);
<<
/Type /Pattern
/BBox [0 0 1 1]
/Length ${ \length $patdef }
/PaintType $paint
/PatternType 1
/Resources <<
/ProcSet [ /PDF /$proc ]
>>
/TilingType 1
/XStep 1
/YStep 1
>>
stream
$patdef
endstream
endobj
PATTERNDEF

	return $xid;
}

sub fillPattern
{
	return $_[0]-> SUPER::fillPattern unless $#_;
	$_[0]-> SUPER::fillPattern( $_[1]);
	return unless $_[0]-> {can_draw};

	my $self = $_[0];
	my $fp   = $self-> SUPER::fillPattern;
	my ($solidBack, $solidFore) = (0,0);
	my $xid;
	if( (ref($fp) // '') eq 'ARRAY') {
		$solidBack = fp::is_empty($fp);
		$solidFore = fp::is_solid($fp);
		if ( !$solidBack && !$solidFore) {
			my $fpid = join( '', map { sprintf("%02x", $_)} @$fp);
			$xid  = $self-> emit_pattern($fpid, $fp);
		}
	} elsif ( UNIVERSAL::isa($fp, 'Prima::Image')) {
		my $fpid = "$fp";
		$fpid =~ s/^Prima::(Image|Icon)=HASH\((.*)\)/$2/; # most common case
		$fpid =~ s/(\W)/sprintf("%02x", ord($1))/ge;      # just in case
		$fpid = (( $fp->type == im::BW ) ? "Mono_" : "Color_") . $fpid;

		my $icon    = $fp->isa('Prima::Icon'); # XXX
		$xid  = $self-> emit_pattern($fpid, $fp);
	} else {
		$solidFore = 1;
	}
	$self-> {fpType} = $solidBack ? 'B' : ( $solidFore ? 'F' : $xid);
	$self-> {changed}-> {fill} = 1;
}

sub compress
{
	return $_[0]-> {compress} unless $#_;
	my $self = $_[0];
	$self-> {compress} = $_[1];
}

sub graphic_context_push
{
	my $self = shift;
	return 0 unless $self->SUPER::graphic_context_push;
	$self->emit_content('q');
	$self->{lock_margins}++;
	return 1;
}

sub graphic_context_pop
{
	my $self = shift;
	$self->emit_content('Q');
	return unless $self->SUPER::graphic_context_pop;
	$self->{lock_margins}--;
	return 1;
}

sub arc
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	return $self->primitive( arc => $x, $y, $dx, $dy, $start, $end) if $self-> is_custom_line;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);

	my $cubics  = Prima::Drawable::Path->arc2cubics($x, $y, $dx, $dy, $start, $end);
	my $content = "@{ $cubics->[0] }[0,1] m\n";
	$content   .= "@{$_}[2..7] c\n" for @$cubics;
	$self-> stroke( $content . " S");
}

sub chord
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	return $self->primitive( chord => $x, $y, $dx, $dy, $start, $end) if $self-> is_custom_line(1);
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);

	my $cubics  = Prima::Drawable::Path->arc2cubics($x, $y, $dx, $dy, $start, $end);
	my $content = "@{ $cubics->[0] }[0,1] m\n";
	$content   .= "@{$_}[2..7] c\n" for @$cubics;
	$self-> stroke( $content . " h S");
}

sub ellipse
{
	my ( $self, $x, $y, $dx, $dy) = @_;
	return $self->primitive( ellipse => $x, $y, $dx, $dy) if $self-> is_custom_line(1);
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);

	my $cubics  = Prima::Drawable::Path->arc2cubics($x, $y, $dx, $dy, 0, 360);
	my $content = "@{ $cubics->[0] }[0,1] m\n";
	$content   .= "@{$_}[2..7] c\n" for @$cubics;
	$self-> stroke( $content . " h S");
}

sub fill_chord
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);

	my $cubics  = Prima::Drawable::Path->arc2cubics($x, $y, $dx, $dy, $start, $end);
	my $content = "@{ $cubics->[0] }[0,1] m\n";
	$content   .= "@{$_}[2..7] c\n" for @$cubics;
	my $F = (($self-> fillMode & fm::Winding) == fm::Alternate) ? 'f*' : 'f';
	$self-> fill( $content . " h $F");
}

sub fill_ellipse
{
	my ( $self, $x, $y, $dx, $dy) = @_;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);

	my $cubics  = Prima::Drawable::Path->arc2cubics($x, $y, $dx, $dy, 0, 360);
	my $content = "@{ $cubics->[0] }[0,1] m\n";
	$content   .= "@{$_}[2..7] c\n" for @$cubics;
	$self-> stroke( $content . " h f");
}

sub sector
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	return $self->primitive( sector => $x, $y, $dx, $dy, $start, $end) if $self-> is_custom_line(1);
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);

	my $cubics  = Prima::Drawable::Path->arc2cubics($x, $y, $dx, $dy, $start, $end);
	my $content = "$x $y m @{ $cubics->[0] }[0,1] l\n";
	$content   .= "@{$_}[2..7] c\n" for @$cubics;
	$self-> stroke( $content . " h S");
}

sub fill_sector
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);

	my $cubics  = Prima::Drawable::Path->arc2cubics($x, $y, $dx, $dy, $start, $end);
	my $content = "$x $y m @{ $cubics->[0] }[0,1] l\n";
	$content   .= "@{$_}[2..7] c" for @$cubics;
	my $F = (($self-> fillMode & fm::Winding) == fm::Alternate) ? 'f*' : 'f';
	$self-> fill( $content . " h $F");
}

sub text_out_outline
{
	my ( $self, $text ) = @_;
	my $shaped   = $self->text_shape($text, level => ts::Glyphs ) or return;
	$self-> glyph_out_outline($shaped, 0, scalar @{$shaped->glyphs});
}

sub glyph_out_outline
{
	my ( $self, $text, $from, $len) = @_;

	my $glyphs     = $text-> glyphs;
	my $indexes    = $text-> indexes;
	my $advances   = $text-> advances;
	my $positions  = $text-> positions;
	my $fonts      = $text-> fonts;
	my $plaintext  = $text-> [Prima::Drawable::Glyphs::CUSTOM()];
	my @ix_lengths = defined($plaintext) ? $text-> index_lengths : ();
	my $canvas     = $self->glyph_canvas;
	my $resolution = 72.0 / $self->{resolution}->[0];
	my $keeper     = $self->{glyph_keeper};
	my $font       = $self->{glyph_font};
	my $div        = $self->{font_scale};
	my $restore_font;

	$len += $from;
	my $emit = '';
	my $fid  = 0;
	my $curr_subfont = -1;
	my @multi_tj;
	for ( my $i = $from; $i < $len; $i++) {
		my $glyph     = $glyphs->[$i];
		my $nfid = $fonts ? $fonts->[$i] : 0;
		if ( $nfid != $fid ) {
			my $newfont;
			if ( $nfid == 0 ) {
				$newfont = $self->{font};
				$restore_font = 0;
			} else {
				my $src  = $self->font_mapper->get($nfid);
				my $dst  = \%{$self->{font}};
				$newfont = Prima::Drawable->font_match( $src, $dst );
				$restore_font = 1;
			}
			$self-> glyph_canvas_set_font( %$newfont );
			$font = $nfid ? $keeper->get_font($canvas->font) : $self->{glyph_font};
			$fid = $nfid;
			$curr_subfont = -1;
		}
		my $char = defined($plaintext) ?
			substr( $plaintext, $indexes->[$i] & ~to::RTL, $ix_lengths[$i]) :
			undef;
		my ($subfont, $gid) = $keeper-> use_char($canvas, $font, $glyph, $char);
		if ( defined($gid) && $subfont != $curr_subfont ) {
			$curr_subfont = $subfont;
			my $xid = $self-> {all_fonts}-> {$font}-> {xids}-> [ $subfont ] //= $self->new_dummy_obj;
			$self->{page_fonts}->{$xid} //= 1;
			if ( @multi_tj ) {
				$emit .= "(".join('',@multi_tj).") Tj\n";
				@multi_tj = ();
			}
			$emit .= "/F$xid $self->{font}->{size} Tf\n";
		}
		push @multi_tj, sprintf("\\%03o", $gid) if defined $gid;
	}
	if ( @multi_tj ) {
		$emit .= "(".join('',@multi_tj).") Tj\n";
		@multi_tj = ();
	}

	if ($restore_font) {
		$self-> glyph_canvas_set_font( %{ $self->{font} });
	}
	$self-> emit_content($emit);
}

sub text_out
{
	my ( $self, $text, $x, $y, $from, $len) = @_;

	$from //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->text_out($self, $x, $y, $from, $len);
	} else {
		$len   = length($text) if !defined($len) || $len < 0 || $len > length($text);
		$text  = substr($text, $from, $len);
		$from  = 0;
		$len   = length($text);
	}
	return 0 unless $self-> {can_draw} and $len > 0;

	$y += $self-> {font}-> {descent} if !$self-> textOutBaseline;
	( $x, $y) = $self-> pixel2point( $x, $y);

	$self-> emit_content("q");
	my $wmul = $self-> {font_x_scale};
	if ( $self-> {font}-> {direction} != 0) {
		my $r = $self-> {font}-> {direction} / $Prima::matrix::RAD;
		my $sin1 = sin($r);
		my $cos  = cos($r);
		my $wcos = cos($r) * $wmul;
		my $sin2 = -$sin1;
		float_inplace($wcos, $sin1, $sin2, $cos, $x, $y);
		$self-> emit_content("$wcos $sin1 $sin2 $cos $x $y cm");
	} else {
		float_inplace($wmul, $x, $y);
		$self-> emit_content("$wmul 0 0 1 $x $y cm");
	}

	my @rb;
	if ( $self-> textOpaque || $self-> {font}-> {style} & (fs::Underlined|fs::StruckOut)) {
		my ( $ds, $bs) = ( $self-> {font}-> {direction}, $self-> textOutBaseline);
		$self-> {font}-> {direction} = 0;
		$self-> textOutBaseline(1) unless $bs;
		@rb = float_format($self-> pixel2point( @{$self-> get_text_box( $text, $from, $len)}));
		$self-> {font}-> {direction} = $ds;
		$self-> textOutBaseline($bs) unless $bs;
	}
	if ( $self-> textOpaque) {
		$self-> emit_content( lc $self-> cmd_rgb( $self-> backColor));
		$self-> emit_content( "h @rb[0,1] m @rb[2,3] l @rb[6,7] l @rb[4,5] l f");
	}

	$self-> emit_content( lc $self-> cmd_rgb( $self-> color));

	$self-> emit_content( "BT");
	if ( $glyphs ) {
		$self->glyph_out_outline($text, $from, $len);
	} else {
		$self->text_out_outline($text);
	}
	$self-> emit_content( "ET");

	if ( $self-> {font}-> {style} & (fs::Underlined|fs::StruckOut)) {
		$self-> emit_content( uc $self-> cmd_rgb( $self-> color));
		my $lw = int($self-> {font}-> {size} / 40 + .5); # XXX empiric
		$lw ||= 1;
		$self-> emit_content("[] 0 d 0 J $lw w");
		if ( $self-> {font}-> {style} & fs::Underlined) {
			$self-> emit_content("h @rb[0,3] m @rb[4,3] l S");
		}
		if ( $self-> {font}-> {style} & fs::StruckOut) {
			$rb[3] += $rb[1]/2;
			$self-> emit_content("h @rb[0,3] m @rb[4,3] l S");
		}
	}
	$self-> emit_content("Q");
	return 1;
}

sub rectangle
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	return $self->primitive( rectangle => $x1, $y1, $x2, $y2) if $self-> is_custom_line(1);
	( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
	$x2 -= $x1;
	$y2 -= $y1;
	$self-> stroke( "h $x1 $y1 $x2 $y2 re S");
}

sub bar
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
	$x2 -= $x1;
	$y2 -= $y1;
	$self-> fill( "h $x1 $y1 $x2 $y2 re f");
}

sub bars
{
	my ( $self, $array) = @_;
	my $i;
	my $c = scalar @$array;
	my @a = $self-> pixel2point( @$array);
	$c = int( $c / 4) * 4;
	my $z = '';
	for ( $i = 0; $i < $c; $i += 4) {
		$z .= "h @a[$i,$i+1] " . ($a[$i+2] - $a[$i]) . ' ' . ($a[$i+3] - $a[$i+1]) . " re f\n";
	}
	$self-> fill( $z);
}

sub clear
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	if ( grep { ! defined } $x1, $y1, $x2, $y2) {
		($x1, $y1, $x2, $y2) = $self-> clipRect;
		unless ( grep { $_ != 0 } $x1, $y1, $x2, $y2) {
			($x1, $y1, $x2, $y2) = (0,0,@{$self-> {size}});
		}
	}
	( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
	$x2 -= $x1;
	$y2 -= $y1;
	my $c = lc $self-> cmd_rgb( $self-> backColor);
	float_inplace($x1, $y1, $x2, $y2);
	$self-> emit_content(<<CLEAR);
$c
h $x1 $y1 $x2 $y2 re f
CLEAR
	$self-> {changed}-> {fill} = 1;
}

sub line
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	return $self->primitive(line => $x1, $y1, $x2, $y2) if $self-> is_custom_line;
	( $x1, $y1, $x2, $y2) = float_format($self-> pixel2point( $x1, $y1, $x2, $y2));
	$self-> stroke("h $x1 $y1 m $x2 $y2 l S");
}

sub lines
{
	my ( $self, $array) = @_;
	return $self->primitive( lines => $array ) if $self->is_custom_line;
	my $i;
	my $c = scalar @$array;
	my @a = float_format($self-> pixel2point( @$array));
	$c = int( $c / 4) * 4;
	my $z = '';
	for ( $i = 0; $i < $c; $i += 4) {
		$z .= "h @a[$i,$i+1] m @a[$i+2,$i+3] l S\n";
	}
	$self-> stroke( $z);
}

sub polyline
{
	my ( $self, $array) = @_;
	return $self->primitive( polyline => $array ) if $self->is_custom_line;
	my $i;
	my $c = scalar @$array;
	my @a = $self-> pixel2point( @$array);
	$c = int( $c / 2) * 2;
	return if $c < 2;
	my $z = "@a[0,1] m\n";
	for ( $i = 2; $i < $c; $i += 2) {
		$z .= "@a[$i,$i+1] l\n";
	}
	$self-> stroke($z . 'S');
}

sub fillpoly
{
	my ( $self, $array) = @_;
	my $i;
	my $c = scalar @$array;
	my @a = $self-> pixel2point( @$array);
	$c = int( $c / 2) * 2;
	return if $c < 2;

	my $z = "@a[0,1] m\n";
	for ( $i = 2; $i < $c; $i += 2) {
		$z .= "@a[$i,$i+1] l\n";
	}
	$self-> fill($z . 
		((($self-> fillMode & fm::Winding) == fm::Alternate) ? 'f*' : 'f')
	);
}

sub pixel
{
	my ( $self, $x, $y, $pix) = @_;
	return cl::Invalid unless defined $pix;
	my $c = lc $self-> cmd_rgb( $pix);
	my $w;
	($x, $y, $w) = float_format($self-> pixel2point( $x, $y, 1));
	$self-> emit_content(<<PIXEL);
q
$c
$x $y $w $w re f
Q
PIXEL
	$self-> {changed}-> {fill} = 1;
}

# methods
our @rops;
$rops[ &{$rop::{$_}}() ] = $_ for qw(
	Multiply Screen Overlay Darken Lighten ColorDodge
	ColorBurn HardLight SoftLight Difference Exclusion
);

sub put_image_indirect
{
	return 0 unless $_[0]-> {can_draw};
	my ( $self, $image, $x, $y, $xFrom, $yFrom, $xDestLen, $yDestLen, $xLen, $yLen, $rop) = @_;
	$rop //= rop::Default;
	$rop = $image->get_effective_rop($rop);
	return 1 if $rop == rop::NoOper;

	$image = $self->prepare_image($image, $xFrom, $yFrom, $xLen, $yLen);

	my @is = $image-> size;
	($x, $y, $xDestLen, $yDestLen) = $self-> pixel2point( $x, $y, $xDestLen, $yDestLen);
	my @fullScale = (
		$is[0] / $xLen * $xDestLen,
		$is[1] / $yLen * $yDestLen,
	);

	my $xid2;
	my $mask = '';
	if ( $image-> isa('Prima::Icon')) {
		my $obj;
		($xid2, $obj) = $self-> new_file_obj;
		my $g  = $image-> mask;
		my $ls = $image-> maskLineSize;
		my $bt = ( $image-> maskType == 1 ) ? int($is[0] / 8) + (($is[0] & 7) ? 1 : 0) : $is[0];
		my $xs = $bt * $is[1];
		for ( my $i = 0; $i < $is[1]; $i++) {
			$obj-> write( substr($g, ($is[1] - $i - 1) * $ls, $bt) );
		}
		my $prefix = <<IMAGE;
/Type /XObject
/Subtype /Image
/Width $is[0]
/Height $is[1]
IMAGE
		if ( $image-> maskType == 1 ) {
			$mask = "/Mask $xid2 0 R";
			$self-> emit_file_obj($obj, $prefix . <<OBJ);
/BitsPerComponent 1
/ImageMask true
OBJ
		} else {
			$mask = "/SMask $xid2 0 R";
			$self-> emit_file_obj($obj, $prefix . <<OBJ);
/BitsPerComponent 8
/ColorSpace /DeviceGray
OBJ
		}
		undef $g;
	}

	my ($xid, $obj) = $self-> new_file_obj;
	push @{ $self-> {page_images}}, $xid;

	my $g  = $image-> data;
	my $bt = ( $image-> type & im::BPP) * $is[0] / 8;
	my $ls = $image-> lineSize;
	for ( my $i = 0; $i < $is[1]; $i++) {
		$obj-> write( substr($g, ($is[1] - $i - 1) * $ls, $bt) );
	}
	undef $g;

	my $cs = (($image->type & im::GrayScale) ? 'Gray' : 'RGB');
	$self-> emit_file_obj($obj, <<OBJ);
/Type /XObject
/Subtype /Image
/Width $is[0]
/Height $is[1]
/ColorSpace /Device$cs
/BitsPerComponent 8
$mask
OBJ

	my $gs = '';
	if ( $rop != rop::CopyPut && $rop >= rop::Multiply && $rop <= rop::Exclusion) {
		my $text = $rops[$rop];
		$self-> {all_rops}->{ $text } //= {
			xid => $self-> emit_new_dummy_object("/Type /ExtGState /BM /$text /AIS false"),
			id  => "GS$text",
		};
		$self-> {page_rops}-> {$text} = $self->{all_rops}->{$text}->{xid};
		$gs = "/$self->{all_rops}->{$text}->{id} gs";
	}

	$self-> emit_content(<<PUT);
q
$gs
$fullScale[0] 0 0 $fullScale[1] $x $y cm
/I$xid Do
Q
PUT
	return 1;
}

sub apply_canvas_font
{
	my ( $self, $f1000) = @_;

	if ($f1000->{vector} == fv::Outline) {
		$self-> {glyph_keeper} //= Prima::PS::CFF->new;
		$self-> {glyph_font} = $self-> {glyph_keeper}->get_font($f1000); # it wants size=1000
		$self-> {all_fonts}->{ $self->{glyph_font} }->{native} //= 0;
	} else {
		$self-> {glyph_font}  = ($f1000->{pitch} == fp::Fixed) ? 'Courier' : 'Helvetica';
		$self-> {all_fonts}->{ $self->{glyph_font} }->{native} //= 1;
	}
}

sub new_path
{
	return Prima::PS::PDF::Path->new(@_);
}

sub region
{
	return $_[0]->{region} unless $#_;
	my ( $self, $region ) = @_;
	if ( $region && !UNIVERSAL::isa($region, "Prima::PS::PDF::Region")) {
		if ( UNIVERSAL::isa($region, 'Prima::Region')) {
			$region = Prima::PS::PDF::Region-> new_from_region( $region, $self );
		} else {
			warn "Region is not a Prima::PS::PDF::Region";
			return undef;
		}
	}
	$self->{clipRect} = [0,0,0,0];
	$self->{region} = $region;
	$self-> change_transform;
}

package
	Prima::PS::PDF::Path;
use base qw(Prima::PS::Drawable::Path);

my %dict = (
	newpath   => '',
	lineto    => 'l',
	moveto    => 'm',
	curveto   => 'c',
	stroke    => 'S',
	closepath => 'h',
	fill_alt  => 'f*',
	fill_wind => 'f',
);

sub dict { \%dict }

sub set_current_point
{
	my ( $self, $x, $y ) = @_;
	$self-> emit($x, $y, $self->{move_is_line} ? 'l' : 'm');
	$self-> {move_is_line} = 1;
}

sub region
{
	my ($self, $mode) = @_;
	my $path = join "\n", @{$self-> entries};
	$path .= ' h' unless $path =~ /h$/;
	$path .= ' W';
	$path .= '*' if ($mode // fm::Winding) & fm::Alternate;
	return Prima::PS::PDF::Region->new( $path );
}

package
	Prima::PS::PDF::Region;
use base qw(Prima::PS::Drawable::Region);
use Prima::PS::Format;

sub other { UNIVERSAL::isa($_[0], "Prima::PS::PDF::Region") ? $_[0] : () }

sub equals
{
	my $self = shift;
	my $other = other(shift) or return;
	return $self->{path} eq $other->{path};
}

sub combine
{
	my $self = shift;
	my $other = other(shift) or return;
	$self->{path} .= "\n" . $other->apply_offset;
}

sub is_empty { shift->{path} !~ /[Sf]/ }

sub apply_offset
{
	my ($self, $canvas) = @_;
	my $path = $self->apply_offset;
	$path =~ s/\bn\b//;
	return $path;
}

sub apply_clip
{
	my ($self, $canvas) = @_;
	my $path = $self->{path};
	my @offset = @{ $self->{offset} };
	return "$path n" if 0 == grep { $_ != 0 } @offset;

	@offset = $canvas-> pixel2point(@offset) if $canvas;
	float_inplace(@offset);

	return "1 0 0 1 @offset cm $path n 1 0 0 1 -$offset[0] -$offset[1] cm";
}

1;

=pod

=head1 NAME

Prima::PS::PDF -  PDF interface to Prima::Drawable

=head1 SYNOPSIS

Recommended usage:

	use Prima::PS::Printer;
	my $x = Prima::PS::PDF::File-> new( file => 'out.pdf');

or

	my $x = Prima::PS::PDF::FileHandle-> new( handle => \&STDOUT);

Low-level:

	use Prima::PS::PDF;
	my $x = Prima::PS::PDF-> create( onSpool => sub {
		open F, ">> ./test.pdf";
		binmode F;
		print F $_[1];
		close F;
	});

Printing:

	die "error:$@" unless $x-> begin_doc;
	$x-> font-> size( 30);
	$x-> text_out( "hello!", 100, 100);
	$x-> end_doc;


=head1 DESCRIPTION

Implements the Prima library interface to PDF v1.4.
The module is designed to be compliant with the Prima::Drawable interface.
All properties' behavior is as same as Prima::Drawable's except those
described below.

=head2 Inherited properties

=over

=item ::resolution

Can be set while the object is in the normal stage - cannot be changed if the
document is opened. Applies to implementation of the fillPattern property and
general pixel-to-point and vice versa calculations

=back

=head2 Specific properties

=over

=item grayscale

could be 0 or 1

=item pageSize

The physical page dimension, in points

=item pageMargins

Non-printable page area, an array of 4 integers:
left, bottom, right, and top margins in points.

=item reversed

if 1, a 90 degrees rotated document layout is assumed

=back

=head2 Internal methods

=over

=item pixel2point and point2pixel

Helpers for translation from pixel to points and vice versa.

=item spool

Prima::PS::PDF is not responsible for the output of the generated
document, it only calls the C<::spool> method when the document is closed
through an C<::end_doc> call. By default discards the data.  The
C<Prima::PS::Printer> class handles the spooling logic.

=item fonts

Calls Prima::Application::fonts and returns its output filtered so that only
the fonts that support the C<iso10646-1> encoding are present. This effectively
allows only unicode output.

=back

=cut
