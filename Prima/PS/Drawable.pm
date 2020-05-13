package Prima::PS::Drawable;
use vars qw(@ISA);
@ISA = qw(Prima::Drawable);

use strict;
use warnings;
use Prima;
use Prima::PS::Fonts;
use Prima::PS::Encodings;
use Prima::PS::Glyphs;
use Prima::PS::TempFile;
use File::Temp;
use Encode;

{
my %RNT = (
	%{Prima::Drawable-> notification_types()},
	Spool => nt::Action,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		copies           => 1,
		font             => {
			%{$def-> {font}},
			name => $Prima::PS::Fonts::defaultFontName,
		},
		grayscale        => 0,
		pageDevice       => undef,
		pageSize         => [ 598, 845],
		pageMargins      => [ 12, 12, 12, 12],
		resolution       => [ 300, 300],
		reversed         => 0,
		rotate           => 0,
		scale            => [ 1, 1],
		isEPS            => 0,
		textOutBaseline  => 0,
		useDeviceFonts   => 1,
		useDeviceFontsOnly => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	Prima::Component::profile_check_in( $self, $p, $default);
	$p-> { font} = {} unless exists $p-> { font};
	$p-> { font} = Prima::Drawable-> font_match( $p-> { font}, $default-> { font}, 0);
}

sub init
{
	my $self = shift;
	$self-> {clipRect}    = [0,0,0,0];
	$self-> {pageSize}    = [0,0];
	$self-> {pageMargins} = [0,0,0,0];
	$self-> {resolution}  = [72,72];
	$self-> {scale}       = [ 1, 1];
	$self-> {isEPS}       = 0;
	$self-> {copies}      = 1;
	$self-> {rotate}      = 1;
	$self-> {font}        = {};
	$self-> {useDeviceFonts} = 1;
	my %profile = $self-> SUPER::init(@_);
	$self-> $_( $profile{$_}) for qw( grayscale copies pageDevice
		useDeviceFonts rotate reversed useDeviceFontsOnly isEPS);
	$self-> $_( @{$profile{$_}}) for qw( pageSize pageMargins resolution scale );
	$self-> {encoding_names} = [];
	$self-> set_font($profile{font}); # update to the changed resolution, device fonts etc
	return %profile;
}

# internal routines

sub cmd_rgb
{
	my ( $r, $g, $b) = (
		int((($_[1] & 0xff0000) >> 16) * 100 / 256 + 0.5) / 100,
		int((($_[1] & 0xff00) >> 8) * 100 / 256 + 0.5) / 100,
		int(($_[1] & 0xff)*100/256 + 0.5) / 100);
	unless ( $_[0]-> {grayscale}) {
		return "$r $g $b A";
	} else {
		my $i = int( 100 * ( 0.31 * $r + 0.5 * $g + 0.18 * $b) + 0.5) / 100;
		return "$i G";
	}
}

sub defer_emission
{
	my ($self, $defer) = @_;
	if ( $defer ) {
		return if defined $self->{deferred};
		if ( length($self-> {ps_data})) {
			my $d = $self->{ps_data};
			$self-> {ps_data} = '';
			return $self-> abort_doc unless $self-> spool($d);
		}

		$self->abort_doc unless $self->{deferred} = Prima::PS::TempFile->new;
	} else {
		return unless defined $self->{deferred};
		$self-> abort_doc unless delete($self->{deferred})->evacuate( sub { $self-> spool($_[0]) } );
	}
}

sub emit
{
	my $self = $_[0];
	return 0 unless $self-> {can_draw};
	if ( defined $self->{deferred} ) {
		unless ($self->{deferred}->write($_[1] . "\n")) {
			$self->abort_doc;
			return 0;
		}
	} else {
		$self-> {ps_data} .= $_[1] . "\n";
		if ( length($self-> {ps_data}) > 10240) {
			$self-> abort_doc unless $self-> spool( $self-> {ps_data});
			$self-> {ps_data} = '';
		}
	}
	return 1;
}

sub emit_font_resources
{
	my $self = $_[0];
	return 0 unless $self-> {can_draw};
}

sub save_state
{
	my $self = $_[0];

	$self-> {save_state} = {};
	if ($self-> {useDeviceFonts}) {
		# force-fill font data
		my $f = $self->get_font;
		delete $f->{size} if exists $f->{height} and exists $f->{size};
		$self-> set_font( $f );
	}
	$self-> {save_state}-> {$_} = $self-> $_() for qw(
		color backColor fillPattern lineEnd linePattern lineWidth miterLimit
		rop rop2 textOpaque textOutBaseline font lineJoin fillMode
	);
	delete $self->{save_state}->{font}->{size};
	$self-> {save_state}-> {$_} = [$self-> $_()] for qw(
		translate clipRect
	);
	$self-> {save_state}-> {encoding_names} =
		$self-> {useDeviceFonts} ? [ @{$self-> {encoding_names}}] : [];
}

sub restore_state
{
	my $self = $_[0];
	for ( qw( color backColor fillPattern lineEnd linePattern lineWidth miterLimit
			rop rop2 textOpaque textOutBaseline font lineJoin fillMode)) {
		$self-> $_( $self-> {save_state}-> {$_});
	}
	for ( qw( translate clipRect)) {
		$self-> $_( @{$self-> {save_state}-> {$_}});
	}
	$self-> {encoding_names} = $self-> {save_state}-> {encoding_names};
}

sub pixel2point
{
	my $self = shift;
	my $i;
	my @res;
	for ( $i = 0; $i < scalar @_; $i+=2) {
		my ( $x, $y) = @_[$i,$i+1];
		push @res, int( $x * 7227 / $self-> {resolution}-> [0] + 0.5) / 100;
		push @res, int( $y * 7227 / $self-> {resolution}-> [1] + 0.5) / 100 if defined $y;
	}
	return @res;
}

sub point2pixel
{
	my $self = shift;
	my $i;
	my @res;
	for ( $i = 0; $i < scalar @_; $i+=2) {
		my ( $x, $y) = @_[$i,$i+1];
		push @res, $x * $self-> {resolution}-> [0] / 72.27;
		push @res, $y * $self-> {resolution}-> [1] / 72.27 if defined $y;
	}
	return @res;
}


sub change_transform
{
	my ( $self, $gsave ) = @_;
	return if $self-> {delay};

	my @tp = $self-> translate;
	my @cr = $self-> clipRect;
	my @sc = $self-> scale;
	my $ro = $self-> rotate;
	$cr[2] -= $cr[0];
	$cr[3] -= $cr[1];
	my $doClip = grep { $_ != 0 } @cr;
	my $doTR   = grep { $_ != 0 } @tp;
	my $doSC   = grep { $_ != 0 } @sc;

	if ( !$doClip && !$doTR && !$doSC && !$ro) {
		$self-> emit(':') if $gsave;
		return;
	}

	@cr = $self-> pixel2point( @cr);
	@tp = $self-> pixel2point( @tp);
	my $mcr3 = -$cr[3];

	$self-> emit(';') unless $gsave;
	$self-> emit(':');
	$self-> emit(<<CLIP) if $doClip;
N $cr[0] $cr[1] M 0 $cr[3] L $cr[2] 0 L 0 $mcr3 L X C
CLIP
	$self-> emit("@tp T") if $doTR;
	$self-> emit("@sc Z") if $doSC;
	$self-> emit("$ro R") if $ro != 0;
	$self-> {changed}-> {$_} = 1 for qw(fill linePattern lineWidth lineJoin lineEnd miterLimit font);
}

sub fill
{
	my ( $self, $code) = @_;
	my ( $r1, $r2) = ( $self-> rop, $self-> rop2);
	return if
		$r1 == rop::NoOper &&
		$r2 == rop::NoOper;

	if ( $r2 != rop::NoOper && $self-> {fpType} ne 'F') {
		my $bk =
			( $r2 == rop::Blackness) ? 0 :
			( $r2 == rop::Whiteness) ? 0xffffff : $self-> backColor;

		$self-> {changed}-> {fill} = 1;
		$self-> emit( $self-> cmd_rgb( $bk));
		$self-> emit( $code);
	}
	if ( $r1 != rop::NoOper && $self-> {fpType} ne 'B') {
		my $c =
			( $r1 == rop::Blackness) ? 0 :
			( $r1 == rop::Whiteness) ? 0xffffff : $self-> color;
		if ($self-> {changed}-> {fill}) {
			if ( $self-> {fpType} eq 'F') {
				$self-> emit( $self-> cmd_rgb( $c));
			} else {
				my ( $r, $g, $b) = (
					int((($c & 0xff0000) >> 16) * 100 / 256 + 0.5) / 100,
					int((($c & 0xff00) >> 8) * 100 / 256 + 0.5) / 100,
					int(($c & 0xff)*100/256 + 0.5) / 100);
				if ( $self-> {grayscale}) {
					my $i = int( 100 * ( 0.31 * $r + 0.5 * $g + 0.18 * $b) + 0.5) / 100;
					$self-> emit(<<GRAYPAT);
[\/Pattern \/DeviceGray] SS
$i Pat_$self->{fpType} SC
GRAYPAT
				} else {
					$self-> emit(<<RGBPAT);
[\/Pattern \/DeviceRGB] SS
$r $g $b Pat_$self->{fpType} SC
RGBPAT
				}
			}
			$self-> {changed}-> {fill} = 0;
		}
		$self-> emit( $code);
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

	if ( $self-> {changed}-> {lineWidth}) {
		my ($lw) = $self-> pixel2point($self-> lineWidth);
		$self-> emit( $lw . ' SW');
		$self-> {changed}-> {lineWidth} = 0;
	}

	if ( $self-> {changed}-> {lineEnd}) {
		my $le = $self-> lineEnd;
		my $id = ( $le == le::Round) ? 1 : (( $le == le::Square) ? 2 : 0);
		$self-> emit( "$id SL");
		$self-> {changed}-> {lineEnd} = 0;
	}

	if ( $self-> {changed}-> {lineJoin}) {
		my $lj = $self-> lineJoin;
		my $id = ( $lj == lj::Round) ? 1 : (( $lj == lj::Bevel) ? 2 : 0);
		$self-> emit( "$id SJ");
		$self-> {changed}-> {lineJoin} = 0;
	}
	
	if ( $self-> {changed}-> {miterLimit}) {
		my $ml = $self-> miterLimit;
		$self-> emit( "$ml ML");
		$self-> {changed}-> {miterLimit} = 0;
	}

	if ( $r2 != rop::NoOper && $lp ne lp::Solid ) {
		my $bk =
			( $r2 == rop::Blackness) ? 0 :
			( $r2 == rop::Whiteness) ? 0xffffff : $self-> backColor;

		$self-> {changed}-> {linePattern} = 1;
		$self-> {changed}-> {fill}        = 1;
		$self-> emit('[] 0 SD');
		$self-> emit( $self-> cmd_rgb( $bk));
		$self-> emit( $code);
	}

	if ( $r1 != rop::NoOper && length( $lp)) {
		my $fk =
			( $r1 == rop::Blackness) ? 0 :
			( $r1 == rop::Whiteness) ? 0xffffff : $self-> color;

		if ( $self-> {changed}-> {linePattern}) {
			if ( length( $lp) == 1) {
				$self-> emit('[] 0 SD');
			} else {
				my @x = split('', $lp);
				push( @x, 0) if scalar(@x) % 1;
				@x = map { ord($_) } @x;
				$self-> emit("[@x] 0 SD");
			}
			$self-> {changed}-> {linePattern} = 0;
		}

		if ( $self-> {changed}-> {fill}) {
			$self-> emit( $self-> cmd_rgb( $fk));
			$self-> {changed}-> {fill} = 0;
		}
		$self-> emit( $code);
	}
}

# Prima::Printer interface

sub begin_doc
{
	my ( $self, $docName) = @_;
	return 0 if $self-> get_paint_state;
	$self-> {ps_data}  = '';
	$self-> {can_draw} = 1;

	$docName = $::application ? $::application-> name : "Prima::PS::Drawable"
		unless defined $docName;
	my $data = scalar localtime;
	my @b2 = (
		int($self-> {pageSize}-> [0] - $self-> {pageMargins}-> [2] + .5),
		int($self-> {pageSize}-> [1] - $self-> {pageMargins}-> [3] + .5)
	);

	$self-> {fp_hash}  = {};
	$self-> {pages}   = 1;

	my ($x,$y) = (
		$self-> {pageSize}-> [0] - $self-> {pageMargins}-> [0] - $self-> {pageMargins}-> [2],
		$self-> {pageSize}-> [1] - $self-> {pageMargins}-> [1] - $self-> {pageMargins}-> [3]
	);

	my $extras = '';
	my $setup = '';
	my %pd = defined( $self-> {pageDevice}) ? %{$self-> {pageDevice}} : ();

	if ( $self-> {copies} > 1) {
		$pd{NumCopies} = $self-> {copies};
		$extras .= "\%\%Requirements: numcopies($self->{copies})\n";
	}

	if ( scalar keys %pd) {
		my $jd = join( "\n", map { "/$_ $pd{$_}"} keys %pd);
		$setup .= <<NUMPAGES;
%%BeginFeature
<< $jd >> SPD
%%EndFeature
NUMPAGES
	}
	$self-> {font_encoding_data} = {};

	my $header = "%!PS-Adobe-2.0";
	$header .= " EPSF-2.0" if $self->isEPS;

	$self-> emit( <<PSHEADER);
$header
%%Title: $docName
%%Creator: Prima::PS::Drawable
%%CreationDate: $data
%%Pages: (atend)
%%BoundingBox: @{$self->{pageMargins}}[0,1] @b2
$extras
%%LanguageLevel: 2
%%DocumentNeededFonts: (atend)
%%DocumentSuppliedFonts: (atend)
%%EndComments

/d/def load def/,/load load d/~/exch , d/S/show , d/:/gsave , d/;/grestore ,
d/N/newpath , d/M/moveto , d/L/rlineto , d/X/closepath , d/C/clip ,
d/T/translate , d/R/rotate , d/Y/glyphshow , d/P/showpage , d/Z/scale , d/I/imagemask ,
d/@/dup , d/G/setgray , d/A/setrgbcolor , d/l/lineto , d/F/fill ,
d/FF/findfont , d/XF/scalefont , d/SF/setfont ,
d/O/stroke , d/SD/setdash , d/SL/setlinecap , d/SW/setlinewidth ,
d/SJ/setlinejoin , d/E/eofill , d/ML/setmiterlimit ,
d/SS/setcolorspace , d/SC/setcolor , d/SM/setmatrix , d/SPD/setpagedevice ,
d/SP/setpattern , d/CP/currentpoint , d/MX/matrix , d/MP/makepattern ,
d/b/begin , d/e/end , d/t/true , d/f/false , d/?/ifelse , d/a/arc ,
d/dummy/_dummy

%%BeginSetup
$setup
%%EndSetup

PSHEADER
	$self->defer_emission(1) unless $self->{useDeviceFontsOnly};
	$self->emit("%%Page: 1 1\n");

	$self-> {page_prefix} = <<PREFIX;
@{$self->{pageMargins}}[0,1] T
N 0 0 M 0 $y L $x 0 L 0 -$y L X C
PREFIX

	$self-> {page_prefix} .= "0 0 M 90 R 0 -$x T\n" if $self-> {reversed};

	$self-> {changed} = { map { $_ => 0 } qw(
		fill lineEnd linePattern lineWidth lineJoin miterLimit font)};
	$self-> {doc_font_map} = {};

	$self-> SUPER::begin_paint;
	$self-> save_state;

	$self-> {delay} = 1;
	$self-> restore_state;
	$self-> {delay} = 0;

	$self-> emit( $self-> {page_prefix});
	$self-> change_transform( 1);
	$self-> {changed}-> {linePattern} = 0;

	return 1;
}

sub abort_doc
{
	my $self = $_[0];
	return unless $self-> {can_draw};
	$self-> {can_draw} = 0;
	$self-> SUPER::end_paint;
	$self-> restore_state;
	delete $self-> {$_} for
		qw (save_state ps_data changed font_encoding_data page_prefix);
	$self-> {plate}-> destroy, $self-> {plate} = undef if $self-> {plate};
}

sub end_doc
{
	my $self = $_[0];
	return 0 unless $self-> {can_draw};
	$self-> {can_draw} = 0;

	$self->{glyph_keeper}-> evacuate( sub { $self->spool( $_[0] ) } )
		if $self-> {glyph_keeper};
	$self-> defer_emission(0);
	my $ret = $self-> spool($self->{ps_data} . <<PSFOOTER);
; P

%%Trailer
%%DocumentNeededFonts:
%%DocumentSuppliedFonts:
%%Pages: $_[0]->{pages}
%%EOF
PSFOOTER

	# if ( $self-> {encoding}) {
	# 	my @z = map { '/' . $_ } keys %{$self-> {doc_font_map}};
	# 	my $xcl = "/FontList [@z] d\n";
	# }

	$self-> {can_draw} = 0;
	$self-> SUPER::end_paint;
	$self-> restore_state;
	delete $self-> {$_} for
		qw (save_state changed font_encoding_data ps_data page_prefix
			glyph_keeper glyph_font 
		);
	$self-> {plate}-> destroy, $self-> {plate} = undef if $self-> {plate};
	return $ret;
}

# Prima::Drawable interface

sub begin_paint { return $_[0]-> begin_doc; }
sub end_paint   {        $_[0]-> abort_doc; }

sub begin_paint_info
{
	my $self = $_[0];
	return 0 if $self-> get_paint_state;
	my $ok = $self-> SUPER::begin_paint_info;
	return 0 unless $ok;
	$self-> save_state;
}

sub end_paint_info
{
	my $self = $_[0];
	return if $self-> get_paint_state != ps::Information;
	$self-> SUPER::end_paint_info;
	$self-> restore_state;
}

sub new_page
{
	return 0 unless $_[0]-> {can_draw};
	my $self = $_[0];
	$self-> {pages}++;
	$self-> emit("; P\n%%Page: $self->{pages} $self->{pages}\n");
	{
		local $self->{delay} = 1;
		$self-> $_( @{$self-> {save_state}-> {$_}}) for qw( translate clipRect);
	}
	$self-> change_transform(1);
	$self-> emit( $self-> {page_prefix});
	$self-> {changed}->{font} = 1;
	return 1;
}

sub pages { $_[0]-> {pages} }

sub spool
{
	shift-> notify( 'Spool', @_);
	return 1;
	# my $p = $_[1];
	# open F, ">> ./test.ps";
	# print F $p;
	# close F;
}

# properties

sub color
{
	return $_[0]-> SUPER::color unless $#_;
	$_[0]-> SUPER::color( $_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {fill} = 1;
}

sub fillPattern
{
	return $_[0]-> SUPER::fillPattern unless $#_;
	$_[0]-> SUPER::fillPattern( $_[1]);
	return unless $_[0]-> {can_draw};

	my $self = $_[0];
	my @fp  = @{$self-> SUPER::fillPattern};
	my $solidBack = ! grep { $_ != 0 } @fp;
	my $solidFore = ! grep { $_ != 0xff } @fp;
	my $fpid;
	my @scaleto = $self-> pixel2point( 8, 8);
	if ( !$solidBack && !$solidFore) {
		$fpid = join( '', map { sprintf("%02x", $_)} @fp);
		unless ( exists $self-> {fp_hash}-> {$fpid}) {
			$self-> emit( <<PATTERNDEF);
<<
\/PatternType 1 \% Tiling pattern
\/PaintType 2 \% Uncolored
\/TilingType 1
\/BBox [ 0 0 @scaleto]
\/XStep $scaleto[0]
\/YStep $scaleto[1]
\/PaintProc { b
:
@scaleto Z
8 8 t
[8 0 0 8 0 0]
< $fpid > I
;
e
} bind
>> MX MP
\/Pat_$fpid ~ d

PATTERNDEF
			$self-> {fp_hash}-> {$fpid} = 1;
		}
	}
	$self-> {fpType} = $solidBack ? 'B' : ( $solidFore ? 'F' : $fpid);
	$self-> {changed}-> {fill} = 1;
}

sub fillPatternOffset
{
	return $_[0]-> SUPER::fillPatternOffset unless $#_;
	$_[0]-> SUPER::fillPatternOffset($_[1], $_[2]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {fillPatternOffset} = 1;
}

sub lineEnd
{
	return $_[0]-> SUPER::lineEnd unless $#_;
	$_[0]-> SUPER::lineEnd($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {lineEnd} = 1;
}

sub lineJoin
{
	return $_[0]-> SUPER::lineJoin unless $#_;
	$_[0]-> SUPER::lineJoin($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {lineJoin} = 1;
}

sub fillMode
{
	return $_[0]-> SUPER::fillMode unless $#_;
	$_[0]-> SUPER::fillMode($_[1]);
}

sub linePattern
{
	return $_[0]-> SUPER::linePattern unless $#_;
	$_[0]-> SUPER::linePattern($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {linePattern} = 1;
}

sub lineWidth
{
	return $_[0]-> SUPER::lineWidth unless $#_;
	$_[0]-> SUPER::lineWidth($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {lineWidth} = 1;
}

sub miterLimit
{
	return $_[0]-> SUPER::miterLimit unless $#_;
	my ( $self, $ml ) = @_;
	$ml = 1.0 if $ml < 0;
	$self-> SUPER::miterLimit($ml);
	return unless $self-> {can_draw};
	$self-> {changed}-> {miterLimit} = 1;
}

sub rop
{
	return $_[0]-> SUPER::rop unless $#_;
	my ( $self, $rop) = @_;
	$rop = rop::CopyPut if
		$rop != rop::Blackness || $rop != rop::Whiteness || $rop != rop::NoOper;
	$self-> SUPER::rop( $rop);
}

sub rop2
{
	return $_[0]-> SUPER::rop2 unless $#_;
	my ( $self, $rop) = @_;
	$rop = rop::CopyPut if
		$rop != rop::Blackness && $rop != rop::Whiteness && $rop != rop::NoOper;
	$self-> SUPER::rop2( $rop);
}

sub translate
{
	return $_[0]-> SUPER::translate unless $#_;
	my $self = shift;
	$self-> SUPER::translate(@_);
	$self-> change_transform;
}

sub clipRect
{
	return @{$_[0]-> {clipRect}} unless $#_;
	$_[0]-> {clipRect} = [@_[1..4]];
	$_[0]-> change_transform;
}

sub region
{
	return undef;
}

sub scale
{
	return @{$_[0]-> {scale}} unless $#_;
	my $self = shift;
	$self-> {scale} = [@_[0,1]];
	$self-> change_transform;
}

sub isEPS { $#_ ? $_[0]-> {isEPS} = $_[1] : $_[0]-> {isEPS} }

sub reversed
{
	return $_[0]-> {reversed} unless $#_;
	my $self = $_[0];
	$self-> {reversed} = $_[1] unless $self-> get_paint_state;
	$self-> calc_page;
}


sub rotate
{
	return $_[0]-> {rotate} unless $#_;
	my $self = $_[0];
	$self-> {rotate} = $_[1];
	$self-> change_transform;
}


sub resolution
{
	return @{$_[0]-> {resolution}} unless $#_;
	return if $_[0]-> get_paint_state;
	my ( $x, $y) =  @_[1..2];
	return if $x <= 0 || $y <= 0;
	$_[0]-> {resolution} = [$x, $y];
	$_[0]-> calc_page;
}

sub copies
{
	return $_[0]-> {copies} unless $#_;
	$_[0]-> {copies} = $_[1] unless $_[0]-> get_paint_state;
}

sub pageDevice
{
	return $_[0]-> {pageDevice} unless $#_;
	$_[0]-> {pageDevice} = $_[1] unless $_[0]-> get_paint_state;
}

sub useDeviceFonts
{
	return $_[0]-> {useDeviceFonts} unless $#_;
	if ( $_[1]) {
		delete $_[0]-> {font}-> {width};
		$_[0]-> set_font( $_[0]-> get_font);
	}
	$_[0]-> {useDeviceFonts} = $_[1] unless $_[0]-> get_paint_state;
	$_[0]-> {useDeviceFonts} = 1 if $_[0]-> {useDeviceFontsOnly};
	if ( !$::application && !$_[1] ) {
		warn "warning: ignored .useDeviceFonts(0) because Prima::Application is not instantiated\n";
		$_[0]->{useDeviceFonts} = 1;
	}
}

sub useDeviceFontsOnly
{
	return $_[0]-> {useDeviceFontsOnly} unless $#_;
	$_[0]-> useDeviceFonts(1)
		if $_[0]-> {useDeviceFontsOnly} = $_[1] && !$_[0]-> get_paint_state;
}

sub grayscale
{
	return $_[0]-> {grayscale} unless $#_;
	$_[0]-> {grayscale} = $_[1] unless $_[0]-> get_paint_state;
}

sub is_ps_font     { 1 == $_[0]-> {font_type} }
sub is_bitmap_font { 2 == $_[0]-> {font_type} }
sub is_vector_font { 3 == $_[0]-> {font_type} }

sub set_encoding
{
	my ( $self, $loc) = @_;

	$self-> {encoding} = $loc;
	return unless $self->{can_draw};
	$self-> {encoding_names} = $self->is_ps_font ? Prima::PS::Encodings::load($loc) : [];
}

sub calc_page
{
	my $self = $_[0];
	my @s =  @{$self-> {pageSize}};
	my @m =  @{$self-> {pageMargins}};
	if ( $self-> {reversed}) {
		@s = @s[1,0];
		@m = @m[1,0,3,2];
	}
	$self-> {size} = [
		int(( $s[0] - $m[0] - $m[2]) * $self-> {resolution}-> [0] / 72.27 + 0.5),
		int(( $s[1] - $m[1] - $m[3]) * $self-> {resolution}-> [1] / 72.27 + 0.5),
	];
}

sub pageSize
{
	return @{$_[0]-> {pageSize}} unless $#_;
	my ( $self, $px, $py) = @_;
	return if $self-> get_paint_state;
	$px = 1 if $px < 1;
	$py = 1 if $py < 1;
	$self-> {pageSize} = [$px, $py];
	$self-> calc_page;
}

sub pageMargins
{
	return @{$_[0]-> {pageMargins}} unless $#_;
	my ( $self, $px, $py, $px2, $py2) = @_;
	return if $self-> get_paint_state;
	$px = 0 if $px < 0;
	$py = 0 if $py < 0;
	$px2 = 0 if $px2 < 0;
	$py2 = 0 if $py2 < 0;
	$self-> {pageMargins} = [$px, $py, $px2, $py2];
	$self-> calc_page;
}

sub size
{
	return @{$_[0]-> {size}} unless $#_;
	$_[0]-> raise_ro("size");
}

# primitives

sub arc
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	my $try = $dy / $dx;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
	my $rx = $dx / 2;
	$end -= $start;
	$self-> stroke( <<ARC );
$x $y M : $x $y T 1 $try Z $start R
N $rx 0 M 0 0 $rx 0 $end a O ;
ARC
}

sub chord
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	my $try = $dy / $dx;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
	my $rx = $dx / 2;
	$end -= $start;
	$self-> stroke(<<CHORD);
$x $y M : $x $y T 1 $try Z $start R
N $rx 0 M 0 0 $rx 0 $end a X O ;
CHORD
}

sub ellipse
{
	my ( $self, $x, $y, $dx, $dy) = @_;
	my $try = $dy / $dx;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
	my $rx = $dx / 2;
	$self-> stroke(<<ELLIPSE);
$x $y M : $x $y T 1 $try Z
N $rx 0 M 0 0 $rx 0 360 a O ;
ELLIPSE
}

sub fill_chord
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	my $try = $dy / $dx;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
	my $rx = $dx / 2;
	$end -= $start;
	my $F = (($self-> fillMode & fm::Winding) == fm::Alternate) ? 'E' : 'F';
	$self-> fill( <<CHORD );
$x $y M : $x $y T 1 $try Z
N $rx 0 M 0 0 $rx 0 $end a X $F ;
CHORD
}

sub fill_ellipse
{
	my ( $self, $x, $y, $dx, $dy) = @_;
	my $try = $dy / $dx;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
	my $rx = $dx / 2;
	$self-> fill(<<ELLIPSE);
$x $y M : $x $y T 1 $try Z
N $rx 0 M 0 0 $rx 0 360 a F ;
ELLIPSE
}

sub sector
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	my $try = $dy / $dx;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
	my $rx = $dx / 2;
	$end -= $start;
	$self-> stroke(<<SECTOR);
$x $y M : $x $y T 1 $try Z $start R
N 0 0 M 0 0 $rx 0 $end a 0 0 l O ;
SECTOR
}

sub fill_sector
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
	my $try = $dy / $dx;
	( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
	my $rx = $dx / 2;
	$end -= $start;
	my $F = (($self-> fillMode & fm::Winding) == fm::Alternate) ? 'E' : 'F';
	$self-> fill(<<SECTOR);
$x $y M : $x $y T 1 $try Z $start R
N 0 0 M 0 0 $rx 0 $end a 0 0 l $F ;
SECTOR
}

sub utf8_text_to_chunks
{
	my ( $self, $text ) = @_;

	my $enc = $self->{encoding_to_downgrade_utf} // 'latin1';
	my $new_text = '';
	my @text;
	while ( 1) {
		my $conv = Encode::encode(
			$enc, $text,
			Encode::FB_QUIET|Encode::LEAVE_SRC
		);
		if ( !length $conv ) {
			#
		} elsif ( !@text || $text[-1][0] == 0 ) {
			push @text, [ 1, $conv ];
		} else {
			$text[-1][1] .= $conv;
		}
		substr( $text, 0, length($conv), '');
		last unless length $text;
		my $c = substr( $text, 0, 1, '');
		if ( !@text || $text[-1][0] == 1 ) {
			push @text, [ 0, $c ];
		} else {
			$text[-1][1] .= $c;
		}
	}
	return \@text;
}

sub text_out_ps_polyfont
{
	my ( $self, $chunks ) = @_;
	my %f = %{$self->{font}};
	my $font_changed;
	my $ps_font = $self->{font}->{docname};
	$self->emit(":");
	for (@$chunks) {
		my ( $plain, $text ) = @$_;
		my $dw;
		if ( $plain ) {
			Encode::_utf8_off($text);
			if ($font_changed) {
				$self->set_font(\%f);
				$self-> emit( "/$ps_font FF $f{size} XF SF");
			}
			$self->text_out_ps($text);
		} else {
			local $self->{useDeviceFonts} = 0;
			my %dst = ( %f, name => $::application-> get_default_font->{name} );
			delete $f{height};
			delete $f{width};
			$self->set_font(\%f);
			$text = $self->text_shape($text, reorder => 0);
			$self-> glyph_out_outline($text, 0, scalar @{$text->glyphs})
				if $text;
			$font_changed = 1;
		}
		($dw) = $self->pixel2point($self->get_text_width($text));
		$self->emit("$dw 0 T 0 0 M");
	}
	$self->set_font(\%f) if $font_changed;
	$self->emit(";");
}

sub text_out_ps
{
	my ( $self, $text) = @_;

	if (Encode::is_utf8($text)) {
		my $chunks = $self->utf8_text_to_chunks($text);
		unless ( @$chunks ) {
			return;
		} elsif ( 1 == @$chunks && $chunks->[0][0] ) {
			$text = $chunks->[0][1];
		} elsif ( $self->{useDeviceFontsOnly} || !$::application ) {
			$text = '';
			$text .= $$_[0] ? $$_[1] : ('?' x length($$_[1])) for @$chunks;
		} else {
			return $self->text_out_ps_polyfont($chunks);
		}
	}

	my $le  = $self-> {encoding_names};
	my $adv = 0;
	my ( $rm, $nd) = $self-> get_rmap;
	my $resolution = 72.27 / $self->{resolution}->[0];
	my $emit = '';
	for my $j (map { ord } split //, $text ) {
		
		my $adv2 = int( $adv * 100 + 0.5) / 100;
		my $gid  = $le->[$j];
		my $xr   = ($gid eq '.notdef') ? $nd : $rm-> [$j];
		$adv    += ( $$xr[1] + $$xr[2] + $$xr[3] ) * $resolution;
		$emit .= "$adv2 0 M " if $adv2 != 0;
		$emit .= "/$gid Y\n";
	}
	$self-> emit($emit);
}

sub text_out_outline
{
	my ( $self, $text ) = @_;
	my $is_bytes = !Encode::is_utf8($text) && $text =~ /[^\x{0}-\x{7f}]/;
	my $shaped   = $self->text_shape($text, level => $is_bytes ? ts::Bytes : ts::Glyphs ) or return;
	$self-> glyph_out_outline($shaped, 0, scalar @{$shaped->glyphs});
}

sub glyph_out_outline
{
	my ( $self, $text, $from, $len ) = @_;

	my $glyphs     = $text-> glyphs;
	my $indexes    = $text-> indexes;
	my $advances   = $text-> advances;
	my $positions  = $text-> positions;
	my $fonts      = $text-> fonts;
	my $plaintext  = $text-> [Prima::Drawable::Glyphs::CUSTOM()];
	my @ix_lengths = defined($plaintext) ? $text-> index_lengths : ();
	my $adv        = 0;
	my $canvas     = $self->glyph_canvas;
	my $resolution = 72.27 / $self->{resolution}->[0];
	my $keeper     = $self->{glyph_keeper};
	my $font       = $self->{glyph_font};
	my $div        = $self->{font_char_height} / $canvas->{pixel_scale};
	my $restore_font;

	my $set_font = sub {
		my %f = %{$_[0]};
		delete $f{height};
		delete $f{width};
		delete $f{direction};
		$f{size} = 1000;
		$canvas->font(\%f);
	};

	$len += $from;
	my $emit = '';
	my $fid  = 0;
	for ( my $i = $from; $i < $len; $i++) {
		my $advance;
		my $glyph     = $glyphs->[$i];
		my ($x2, $y2) = ($adv, 0);
		my $nfid = $fonts ? $fonts->[$i] : 0;
		if ( $nfid != $fid ) {
			my $newfont;
			if ( $nfid == 0 ) {
				$newfont = $self->{font};
				$restore_font = 0;
			} else {
				my $src  = $self-> fontMapperPalette($nfid);
				my $dst  = \%{$self->{font}};
				$newfont = Prima::Drawable->font_match( $src, $dst );
				$restore_font = 1;
			}
			$set_font->($newfont);
			$font = $nfid ? $keeper->get_font($canvas->font) : $self->{glyph_font};
			$emit .= "/$font FF $self->{font}->{size} XF SF\n";
			$fid = $nfid;
		}
		my $gid = $keeper-> use_char($canvas, $font, $glyph,
			defined($plaintext) ?
				substr( $plaintext, $indexes->[$i] & ~to::RTL, $ix_lengths[$i]) :
				undef
		);
		if ( $advances) {
			$advance = $advances->[$i];
			$x2 += $positions->[$i*2];
			$y2 += $positions->[$i*2 + 1];
		} else {
			my $xr = $canvas->get_font_abc($glyph, $glyph, to::Glyphs);
			$advance = ($$xr[0] + $$xr[1] + $$xr[2]) * $div;
		}
		$adv += $advance;

		($x2, $y2) = map { int( $_ * 100 + 0.5) / 100 } $self->pixel2point($x2, $y2);
		$emit .= "$x2 $y2 M " if $x2 != 0 || $y2 != 0;
		$emit .= "/$gid Y\n";
	}

	if ($restore_font) {
		$emit .= "/$self->{glyph_font} FF $self->{font}->{size} XF SF\n";
		$set_font->($self->{font});
	}
	$self-> emit($emit);
}

sub text_out_bitmaps
{
	my ( $self, $text ) = @_;

	my $adv        = 0;
	for my $j ( split //, $text) {
		my ( $pg, $a, $b, $c) = $self-> place_glyph($j);
		my $advance;
		if ( length $pg) {
			my ( $x, $y ) = map { int( $_ * 100 + 0.5) / 100 }
				$self->pixel2point($adv + $a, $self->{plate}->{descent});
			$self-> emit( "$x $y M : CP T");
			$self-> emit( $pg);
			$self-> emit(";");
		}
		$adv += $a + $b + $c;
	}
}

sub glyph_out_bitmaps
{
	my ( $self, $text, $from, $len ) = @_;

	my $advances   = $text-> advances;
	my $positions  = $text-> positions;
	my $adv        = 0;
	for ( my $i = $from; $i < $from + $len; $i++) {
		my ( $pg, $a, $b, $c) = $self-> place_glyph($text, $i);
		if ( length $pg) {
			my ( $x2, $y2 ) = ($adv + $a, $self->{plate}->{descent});
			if ( $positions ) {
				$x2 += $positions->[$i * 2];
				$y2 += $positions->[$i * 2 + 1];
			}
			( $x2, $y2 ) = map { int( $_ * 100 + 0.5) / 100 } $self->pixel2point($x2, $y2);
			$self-> emit( "$x2 $y2 M : CP T");
			$self-> emit( $pg);
			$self-> emit(";");
		}
		$adv += $advances ? $advances->[$i] : ($a + $b + $c);
	}
}

sub text_out
{
	my ( $self, $text, $x, $y, $from, $len) = @_;

	my $is_ps_font = $self->is_ps_font;
	my $is_vector_font = $self->is_vector_font;
	$from //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
		return if $is_ps_font;
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

	if ( $self-> {changed}-> {font}) {
		if ( $is_ps_font || $is_vector_font) {
			my $fn = $is_ps_font ? $self-> {font}-> {docname} : $self->{glyph_font};
			$self-> emit( "/$fn FF $self->{font}->{size} XF SF");
		}
		$self-> {changed}-> {font} = 0;
	}

	my $wmul = $self-> {font}-> {width} / $self-> {font_width_divisor};
	$self-> emit(": $x $y T");
	$self-> emit("$wmul 1 Z") if $wmul != 1;
	$self-> emit("0 0 M");
	if ( $self-> {font}-> {direction} != 0) {
		my $r = $self-> {font}-> {direction};
		$self-> emit("$r R");
	}

	my @rb;
	if ( $self-> textOpaque || $self-> {font}-> {style} & (fs::Underlined|fs::StruckOut)) {
		my ( $ds, $bs) = ( $self-> {font}-> {direction}, $self-> textOutBaseline);
		$self-> {font}-> {direction} = 0;
		$self-> textOutBaseline(1) unless $bs;
		@rb = $self-> pixel2point( @{$self-> get_text_box( $text, $from, $len)});
		$self-> {font}-> {direction} = $ds;
		$self-> textOutBaseline($bs) unless $bs;
	}
	if ( $self-> textOpaque) {
		$self-> emit( $self-> cmd_rgb( $self-> backColor));
		$self-> emit( ": N @rb[0,1] M @rb[2,3] l @rb[6,7] l @rb[4,5] l X F ;");
	}

	$self-> emit( $self-> cmd_rgb( $self-> color));

	if ($is_ps_font) {
		$self-> text_out_ps($text);
	} elsif ($is_vector_font) {
		if ( $glyphs ) {
			$self->glyph_out_outline($text, $from, $len);
		} else {
			$self->text_out_outline($text);
		}
	} else {
		if ( $glyphs ) {
			$self->glyph_out_bitmaps($text, $from, $len);
		} else {
			$self->text_out_bitmaps($text);
		}
	}


	if ( $self-> {font}-> {style} & (fs::Underlined|fs::StruckOut)) {
		my $lw = int($self-> {font}-> {size} / 40 + .5); # XXX empiric
		$self-> emit("[] 0 SD 0 SL $lw SW");
		if ( $self-> {font}-> {style} & fs::Underlined) {
			$self-> emit("N @rb[0,3] M $rb[4] 0 L O");
		}
		if ( $self-> {font}-> {style} & fs::StruckOut) {
			$rb[3] += $rb[1]/2;
			$self-> emit("N @rb[0,3] M $rb[4] 0 L O");
		}
	}
	$self-> emit(";");
	return 1;
}

sub bar
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
	$self-> fill( "N $x1 $y1 M $x1 $y2 l $x2 $y2 l $x2 $y1 l X F");
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
		$z .= "N @a[$i,$i+1] M @a[$i,$i+3] l @a[$i+2,$i+3] l @a[$i+2,$i+1] l X F ";
	}
	$self-> stroke( $z);
}

sub rectangle
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
	$self-> stroke( "N $x1 $y1 M $x1 $y2 l $x2 $y2 l $x2 $y1 l X O");
}

sub alpha {}

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
	my $c = $self-> cmd_rgb( $self-> backColor);
	$self-> emit(<<CLEAR);
$c
N $x1 $y1 M $x1 $y2 l $x2 $y2 l $x2 $y1 l X F
CLEAR
	$self-> {changed}-> {fill} = 1;
}

sub line
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
	$self-> stroke("N $x1 $y1 M $x2 $y2 l O");
}

sub lines
{
	my ( $self, $array) = @_;
	my $i;
	my $c = scalar @$array;
	my @a = $self-> pixel2point( @$array);
	$c = int( $c / 4) * 4;
	my $z = '';
	for ( $i = 0; $i < $c; $i += 4) {
		$z .= "N @a[$i,$i+1] M @a[$i+2,$i+3] l O ";
	}
	$self-> stroke( $z);
}

sub polyline
{
	my ( $self, $array) = @_;
	my $i;
	my $c = scalar @$array;
	my @a = $self-> pixel2point( @$array);
	$c = int( $c / 2) * 2;
	return if $c < 2;
	my $z = "N @a[0,1] M ";
	for ( $i = 2; $i < $c; $i += 2) {
		$z .= "@a[$i,$i+1] l ";
	}
	$z .= "O";
	$self-> stroke( $z);
}

sub fillpoly
{
	my ( $self, $array) = @_;
	my $i;
	my $c = scalar @$array;
	$c = int( $c / 2) * 2;
	return if $c < 2;
	my @a = $self-> pixel2point( @$array);
	my $x = "N @a[0,1] M ";
	for ( $i = 2; $i < $c; $i += 2) {
		$x .= "@a[$i,$i+1] l ";
	}
	$x .= 'X ' . ((($self-> fillMode & fm::Winding) == fm::Alternate) ? 'E' : 'F');
	$self-> fill( $x);
}

sub flood_fill { return 0; }

sub pixel
{
	my ( $self, $x, $y, $pix) = @_;
	return cl::Invalid unless defined $pix;
	my $c = $self-> cmd_rgb( $pix);
	($x, $y) = $self-> pixel2point( $x, $y);
	$self-> emit(<<PIXEL);
:
$c
N $x $y M 0 0 L F
;
PIXEL
	$self-> {changed}-> {fill} = 1;
}


# methods

sub put_image_indirect
{
	return 0 unless $_[0]-> {can_draw};
	my ( $self, $image, $x, $y, $xFrom, $yFrom, $xDestLen, $yDestLen, $xLen, $yLen) = @_;

	my $touch;
	$touch = 1, $image = $image-> image if $image-> isa('Prima::DeviceBitmap');

	unless ( $xFrom == 0 && $yFrom == 0 && $xLen == $image-> width && $yLen == $image-> height) {
		$image = $image-> extract( $xFrom, $yFrom, $xLen, $yLen);
		$touch = 1;
	}

	my $ib = $image-> get_bpp;
	if ( $ib != $self-> get_bpp) {
		$image = $image-> dup unless $touch;
		if ( $self-> {grayscale} || $image-> type & im::GrayScale) {
			$image-> type( im::Byte);
		} else {
			$image-> type( im::RGB);
		}
		$touch = 1;
	} elsif ( $self-> {grayscale} || $image-> type & im::GrayScale) {
		$image = $image-> dup unless $touch;
		$image-> type( im::Byte);
		$touch = 1;
	}

	$ib = $image-> get_bpp;
	if ($ib != 8 && $ib != 24) {
		$image = $image-> dup unless $touch;
		$image-> type( im::RGB);
		$touch = 1;
	}

	if ( $image-> type == im::RGB ) {
		# invert BGR -> RGB
		$image = $image-> dup unless $touch;
		$image-> set(data => $image->data, type => im::fmtBGR | im::RGB);
		$touch = 1;
	}

	my @is = $image-> size;
	($x, $y, $xDestLen, $yDestLen) = $self-> pixel2point( $x, $y, $xDestLen, $yDestLen);
	my @fullScale = (
		$is[0] / $xLen * $xDestLen,
		$is[1] / $yLen * $yDestLen,
	);

	my $g  = $image-> data;
	my $bt = ( $image-> type & im::BPP) * $is[0] / 8;
	my $ls = $image->lineSize;
	my ( $i, $j);

	$self-> emit(": $x $y T @fullScale Z");
	$self-> emit("/scanline $bt string d");
	$self-> emit("@is 8 [$is[0] 0 0 $is[1] 0 0]");
	$self-> emit('{currentfile scanline readhexstring pop}');
	$self-> emit(( $image-> type & im::GrayScale) ? "image" : "false 3 colorimage");

	for ( $i = 0; $i < $is[1]; $i++) {
		$self-> emit(unpack('H*', substr( $g, $ls * $i, $bt)));
	}
	$self-> emit(';');
	return 1;
}

sub get_bpp              { return $_[0]-> {grayscale} ? 8 : 24 }
sub get_nearest_color    { return $_[1] }
sub get_physical_palette { return $_[0]-> {grayscale} ? [map { $_, $_, $_ } 0..255] : 0 }
sub get_handle           { return 0 }

# fonts
sub fonts
{
	my ( $self, $family, $encoding) = @_;
	$family   = undef if defined $family   && !length $family;
	$encoding = undef if defined $encoding && !length $encoding;

	my $f1 = $self-> {useDeviceFonts} ? Prima::PS::Fonts::enum_fonts( $family, $encoding) : [];
	return $f1 if !$::application || $self-> {useDeviceFontsOnly};

	my $f2 = $::application-> fonts( $family, $encoding);
	if ( !defined($family) && !defined($encoding)) {
		my %f = map { $_-> {name} => $_ } @$f1;
		my @add;
		for ( @$f2) {
			if ( $f{$_}) {
				push @{$f{$_}-> {encodings}}, @{$_-> {encodings}};
			} else {
				push @add, $_;
			}
		}
		push @$f1, @add;
	} else {
		push @$f1, @$f2;
	}
	return $f1;
}

sub font_encodings
{
	my @r;
	if ( $_[0]-> {useDeviceFonts}) {
		@r = Prima::PS::Encodings::unique, keys %Prima::PS::Encodings::fontspecific;
	}
	if ( $::application && !$_[0]-> {useDeviceFontsOnly}) {
		my %h = map { $_ => 1 } @r;
		for ( @{$::application-> font_encodings}) {
			next if $h{$_};
			push @r, $_;
		}
	}
	return \@r;
}

sub get_font
{
	my $z = {%{$_[0]-> {font}}};
	delete $z-> {charmap};
	delete $z-> {docname};
	return $z;
}

# we're asked to substitute a non-PS font, which most probably has its own definiton of box width
# let's find out what em-width the font has, and if we can adapt for it
#
# return the multiplication factor between the requested gui font and the currently selected PS font
sub _get_gui_font_ratio
{
	my ($self, %request) = @_;
	my $n = $request{name};

	return unless
		($n ne 'Default') && exists $request{width} && exists $request{height} && $::application &&
		!exists($Prima::PS::Fonts::enum_families{ $n}) && !exists($Prima::PS::Fonts::files{ $n})
		;

	my $ratio;
	my $paint_state = $::application->get_paint_state == ps::Disabled;
	my $save_font;
	$paint_state ? $::application->begin_paint_info : ( $save_font = \%{ $::application->get_font } );

	my $scale = ($request{height} < 20) ? 10 : 1; # scale font 10 times for better accuracy
	my $width = delete($request{width});
	$request{height} *= $scale;
	$::application->set_font(\%request);

	if ( $n eq $::application->font->name) {
		my $gui_scaling = $width / $::application->font->width;
		my $ps_scaling  = $self->{font}->{referenceWidth} / $self->{font}->{width};
		$ratio = $ps_scaling * $gui_scaling * $scale;
	}

	$paint_state ? $::application->end_paint_info   : ( $::application->set_font($save_font) );
	return $ratio;
}

sub set_font
{
	my ( $self, $font) = @_;
	$font = { %$font };
	my $n = exists($font-> {name}) ? $font-> {name} : $self-> {font}-> {name};
	my $gui_font;
	$n = $self-> {useDeviceFonts} ? $Prima::PS::Fonts::defaultFontName : 'Default'
		unless defined $n;
	my ($curr_font, $new_font) = ('', '');
	if (( $self-> {type_name} // 2) != 2) {
		my $fn = ($self->{type_name} == 1) ? $self-> {font}-> {docname} : $self->{glyph_font};
		$curr_font = $self->{font}->{size} . '.' . $fn;
	}

AGAIN:
	if ( $self-> {useDeviceFontsOnly} || !$::application ||
			( $self-> {useDeviceFonts} &&
			(
			# enter, if there's a device font
				exists $Prima::PS::Fonts::enum_families{ $n} ||
				exists $Prima::PS::Fonts::files{ $n} ||
				(
					# or the font encoding is PS::Encodings-specific,
					# not present in the GUI space
					exists $font-> {encoding} &&
					(
						exists $Prima::PS::Encodings::fontspecific{$font-> {encoding}} ||
						exists $Prima::PS::Encodings::files{$font-> {encoding}}
					) && (
						!grep { $_ eq $font-> {encoding} } @{$::application-> font_encodings}
					)
				)
			) &&
			# and, the encoding is supported
			(
				!exists $font-> {encoding} || !length ($font-> {encoding}) ||
				(
					exists $Prima::PS::Encodings::fontspecific{$font-> {encoding}} ||
					exists $Prima::PS::Encodings::files{$font-> {encoding}}
				)
			)
		)
	)
	{
		$self-> {font} = Prima::PS::Fonts::font_pick( $font, $self-> {font},
			resolution => $self-> {resolution}-> [1]);
		$self-> {font_char_height} = $self-> {font}-> {charheight};
		$self-> {doc_font_map}-> {$self-> {font}-> {docname}} = 1;
		$self-> {font_type} = 1;
		$self-> {font_width_divisor} = $self-> {font}-> {referenceWidth};
		$self-> set_encoding( $self-> {font}-> {encoding});

		my %request = ( %$font, name => $n );
		$request{height} = $self->{font}->{height} unless defined $request{height};
		delete $request{size};
		if ( my $ratio = $self->_get_gui_font_ratio(%request)) {
			$self->{font}->{width}        *= $ratio;
			$self->{font}->{maximalWidth} *= $ratio;
		}
		eval { Encode::encode( $self->{font}->{encoding}, ''); };
		$self->{encoding_to_downgrade_utf} = $@ ? undef : $self->{font}->{encoding};
		$new_font = $self->{font}->{size} . '.' . $self->{font}->{docname};
	} else {
		my $wscale = $font-> {width};
		delete $font-> {width};
		my $by_height = defined($font->{height});
		$gui_font = Prima::Drawable-> font_match( $font, $self-> {font});
		if ( $gui_font-> {name} ne $n && $self-> {useDeviceFonts}) {
			# back up
			my $pitch = (exists ( $font-> {pitch} ) ?
				$font-> {pitch} : $self-> {font}-> {pitch}) || fp::Variable;
			$n = $font-> {name} = ( $pitch == fp::Variable) ?
				$Prima::PS::Fonts::variablePitchName :
				$Prima::PS::Fonts::fixedPitchName;
			$font-> {width}    = $wscale if defined $wscale;
			if ( defined $font->{encoding}) {
				delete $font-> {encoding} unless
					exists $Prima::PS::Encodings::fontspecific{$font-> {encoding}} ||
					exists $Prima::PS::Encodings::files{$font-> {encoding}};
			}
			goto AGAIN;
		}

		$self-> {font} = $gui_font;
		if ( $self->{font}->{vector} && $self->{can_draw} ) {
			my $canvas = $self->glyph_canvas;
			goto BITMAP unless $canvas-> render_glyph($canvas->font->defaultChar);
			$self-> {font_type} = 3;
			$self-> defer_emission(1);
			$self->{glyph_keeper} //= Prima::PS::Glyphs->new;
			$self-> {glyph_font} = $self-> {glyph_keeper}->get_font($self->{font});
			$self-> set_encoding( $self-> {font}-> {encoding});
			$new_font = $self->{font}->{size} . '.' . $self->{glyph_font};
		} else {
		BITMAP:
			$self-> {font_type} = 2;
			undef $self-> {glyph_font};
		}
		my $div = 72.27 / $self-> {resolution}-> [1];
		my $f  = $self->{font};
		$self-> {font_char_height} = $f->{height};
		if ( $by_height ) {
			$f-> {size} = int(( $f-> {height} - $f->{internalLeading}) * $div + 0.5);
		} else {
			my $new_h = $f->{size} / $div + $f->{internalLeading};
			my $ratio = $f->{height} / $new_h;
			$f->{height}  = $new_h;
			$f->{ascent}  = int( $f->{ascent} / $ratio + .5 );
			$f->{descent} = $new_h - $f->{ascent};
		}
		$self-> glyph_canvas-> {for_vector} = -1; # force update
		$self-> {font_width_divisor}        = $f-> {width};
		$f-> {width}                        = $wscale if $wscale;
		$self-> {encoding_to_downgrade_utf} = undef;
	}
	$self-> {plate}-> destroy, $self-> {plate} = undef if $self-> {plate};
	$self-> {changed}-> {font} = 1 if $curr_font ne $new_font;
}

my %fontmap =
(Prima::Application-> get_system_info-> {apc} == apc::Win32) ? (
	'Helvetica' => 'Arial',
	'Times'     => 'Times New Roman',
	'Courier'   => 'Courier New',
) : ();

sub get_gui_font
{
	my ($self, $for_vector) = @_;
	my %f = %{$self-> {font}};
	$f{style} &= ~(fs::Underlined|fs::StruckOut);
	delete $f{size};
	delete $f{width};
	delete $f{direction};
	if ($for_vector) {
		$f{size} = 1000;
		delete $f{height};
	}
	return \%f;
}

my $x11_bug;

sub plate
{
	my ($self) = @_;
	return $self-> {plate} if $self-> {plate};

	unless ( defined $x11_bug ) {
		my $test = Prima::DeviceBitmap->new(
			type => dbt::Bitmap,
			size => [16, 16],
		);
		$test->clear;
		$test->text_out(3,3,3);
		$x11_bug = ($test->image->sum == 0) ? 1 : 0;
	}

	my $g = $self-> {plate} = Prima::Image-> create(
		type            => $x11_bug ? im::RGB : im::BW,
		width           => $self-> {font}-> {maximalWidth} * 4, # for clusters
		height          => $self-> {font}-> {height},
		font            => $self-> get_gui_font(0),
		backColor       => cl::Black,
		color           => cl::White,
		textOutBaseline => 1,
		preserveType    => 1,
		conversion      => ict::None,
	);

	return $g;
}

sub glyph_canvas
{
	my ($self, $for_vector) = @_;

	$for_vector //= ( $self->is_vector_font ? 1 : 0 );

	my $update;
	unless ($self->{glyph_canvas}) {
		my $g = $self->{glyph_canvas} = Prima::DeviceBitmap->create(
			width           => 1,
			height          => 1,
			textOutBaseline => 1,
		);
		$g->{for_vector} = $for_vector;
		$update = 1;
	}

	$update = 1 if $for_vector != $self->{glyph_canvas}->{for_vector};

	if ( $update ) {
		my $g = $self->{glyph_canvas};
		$g-> font( $self->get_gui_font($for_vector));
		my $f = $g->font;
		$g->{point_scale} = $for_vector ? 
			($f->height * ($f->height - $f->internalLeading) / $f->size) :
			1;
		($g->{pixel_scale}) = $self->pixel2point($g->{point_scale});
		$g->{for_vector}  = $for_vector;
	}

	return $self->{glyph_canvas};
}

sub place_glyph
{
	my ( $self, $char, $glyph_offset ) = @_;
	my $z   = $self-> plate;
	my ( $dimx, $dimy) = $z-> size;
	my $x   = defined($glyph_offset) ? $char->glyphs->[$glyph_offset] : ord $char;
	my $d   = $z-> font-> descent;
	my $ls  = int(( $dimx + 31) / 32) * 4;
	my $la  = int ($dimx / 8) + (( $dimx & 7) ? 1 : 0);
	my $ax  = ( $dimx & 7) ? (( 0xff << (7-( $dimx & 7))) & 0xff) : 0xff;
	my $xsf = 0;

	my ( $a, $b, $c) = @{ $self-> glyph_canvas-> get_font_abc($x, $x, 
		defined($glyph_offset) ?
			to::Glyphs : 
			(Encode::is_utf8($char) ? to::Unicode : 0)
	) };
	return '', $a, $b, $c if $b <= 0;

	$z-> begin_paint;
	$z-> clear;
	if ( defined($glyph_offset)) {
		$z-> text_out( $char, ($a < 0) ? -$a : 0, $d, $glyph_offset, 1 );
	} else {
		$z-> text_out( $char, ($a < 0) ? -$a : 0, $d);
	}
	$z-> end_paint;
	my $dd = $x11_bug ? $z->clone(type => im::BW)-> data : $z-> data;
	my ($j, $k);
	my @emmap = (0) x $dimy;
	my @bbox = ( $a, 0, $b - $a, $dimy - 1);
	for ( $j = $dimy - 1; $j >= 0; $j--) {
		my @xdd = map { ord $_ } split( '', substr( $dd, $ls * $j, $la));
		$xdd[-1] &= $ax;
		$emmap[$j] = 1 unless grep { $_ } @xdd;
	}
	for ( $j = 0; $j < $dimy; $j++) {
		last unless $emmap[$j];
		$bbox[1]++;
	}
	for ( $j = $dimy - 1; $j >= 0; $j--) {
		last unless $emmap[$j];
		$bbox[3]--;
	}

	if ( $bbox[3] >= 0) {
		$bbox[1] -= $d;
		$bbox[3] -= $d;
		my $zd = $z-> extract(
			( $a < 0) ? 0 : $a,
			$bbox[1] + $d,
			$b,
			$bbox[3] - $bbox[1] + 1,
		);
		$zd->type(im::BW) if $x11_bug;
#		$zd->color(cl::White);$zd->rectangle(0,0,$zd->width-1,$zd->height-1);
#		$z-> save("a.gif");

		my $bby = $bbox[3] - $bbox[1] + 1;
		my $zls = int(( $b + 31) / 32) * 4;
		my $zla = int ($b / 8) + (( $b & 7) ? 1 : 0);
		$zd = $zd-> data;
		my $cd = '';
		for ( $j = $bbox[3] - $bbox[1]; $j >= 0; $j--) {
			$cd .= substr( $zd, $j * $zls, $zla);
		}

		my $cdz = '';
		for ( $j = 0; $j < length $cd; $j++) {
			$cdz .= sprintf("%02x", ord substr( $cd, $j, 1));
		}

		$z-> {descent} = $bbox[1];
		my @scale = $self->pixel2point($b, $bby);
		return
			"@scale scale $b $bby true [$b 0 0 -$bby 0 $bby] <$cdz> imagemask",
			$a, $b, $c;
	}
	return '', $a, $b, $c;
}

sub get_rmap
{
	my $self = shift;

	return unless $self-> is_ps_font;

	my @rmap;
	my $c  = $self-> {font}-> {chardata};
	my $le = $self-> {encoding_names};
	my $nd = $c-> {'.notdef'};
	my $fs = $self-> {font}-> {height} / $self-> {font_char_height};
	if ( defined $nd) {
		$nd = [ @$nd ];
		$$nd[$_] *= $fs for 1..3;
	} else {
		$nd = [0,0,0,0];
	}

	for ( my $i = 0; $i < 256; $i++) {
		if (defined($le->[$i]) && ($le-> [$i] ne '.notdef') && $c-> { $le-> [ $i]}) {
			$rmap[$i] = [ $i, map { $_ * $fs } @{$c-> { $le-> [ $i]}}[1..3]];
		}
	}

	return \@rmap, $nd;
}

sub get_font_abc
{
	my ( $self, $first, $last, $flags) = @_;
	$first = 0     if !defined ($first) || $first < 0;
	$last = $first if !defined ($last) || $last < $first;

	my @ret;
	if ( $self-> is_ps_font ) {
		my ($rmap, $nd) = $self-> get_rmap;
		for ( my $i = $first; $i <= $last; $i++) {
			my $cd = $rmap-> [$i] || $nd;
			push @ret, @$cd[1..3];
		}
	} elsif ( $self->is_vector_font ) {
		my $canvas = $self-> glyph_canvas(1);
		my $scale  = $self->{font_char_height} / $canvas->{pixel_scale};
		@ret = map { $_ * $scale } @{ $canvas->get_font_abc($first, $last, $flags) };
	} else {
		@ret = @{ $self-> glyph_canvas(0)-> get_font_abc($first, $last, $flags) };
	}

	my $wmul = $self-> {font}-> {width} / $self-> {font_width_divisor};
	$_ *= $wmul for @ret;
	return \@ret;
}

sub get_font_def
{
	my ( $self, $first, $last, $flags) = @_;
	$first = 0     if !defined ($first) || $first < 0;
	$last = $first if !defined ($last) || $last < $first;
	my @ret;

	my $def;
	if ( $self-> is_ps_font ) {
		my $fs = $self-> {font}-> {height} / $self-> {font_char_height};
		my $c  = $self-> {font}-> {chardata};
		my $le = $self-> {encoding_names};
		my $nd = $c-> {'.notdef'};
		if ( defined $nd) {
			$nd = [ map { $_ * $fs } @$nd[4..6] ];
		} else {
			$nd = [0,0,0];
		}
		for ( my $i = $first; $i <= $last; $i++) {
			my @rmap;
			if (defined($le->[$i]) && ( $le-> [$i] ne '.notdef') && $c-> { $le-> [ $i]}) {
				push @ret, map { $_ * $fs } @{$c-> { $le-> [ $i]}}[4..6];
			} else {
				push @ret, @$nd;
			}
		}
		return \@ret;
	} elsif ( $self->is_vector_font ) {
		my $canvas = $self-> glyph_canvas(1);
		my $scale  = $self->{font_char_height} / $canvas->{pixel_scale};
		return [ map { $_ * $scale } @{ $canvas->get_font_def($first, $last, $flags) } ];
	} else {
		@ret = @{ $self-> glyph_canvas(0)-> get_font_def($first, $last, $flags) };
	}
}

sub get_font_ranges
{
	my $self = $_[0];
	return $self-> is_ps_font ? 
		[ $self-> {font}-> {firstChar}, $self-> {font}-> {lastChar}] :
		$self-> glyph_canvas-> get_font_ranges;
}

sub get_font_languages
{
	my $self = shift;
	return $self-> is_ps_font ? 
		Prima::PS::Encodings::get_languages($self-> {encoding}) :
		$self->glyph_canvas->get_font_languages;
}

sub get_text_width
{
	my ( $self, $text, $flags, $from, $len) = @_;
	$flags //= 0;

	my $is_ps_font = $self->is_ps_font;
	my $is_vector_font = $self->is_vector_font;
	$from //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
		return 0 if $is_ps_font;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->get_text_width($self, $flags, $from, $len);
	} else {
		$len   = length($text) if !defined($len) || $len < 0 || $len > length($text);
		$text  = substr($text, $from, $len);
		$from  = 0;
		$len   = length($text);
	}
	return 0 unless $len;

	my $w = 0;
	if ( $is_ps_font ) {
		my ( $rmap, $nd) = $self-> get_rmap;
		my $cd;
		for ( my $i = 0; $i < $len; $i++) {
			my $cd = $rmap-> [ ord( substr( $text, $i, 1))] || $nd;
			$w += $cd-> [1] + $cd-> [2] + $cd-> [3];
		}
		if ( $flags & to::AddOverhangs) {
			$cd = $rmap-> [ ord( substr( $text, 0, 1))] || $nd;
			$w += ( $cd-> [1] < 0) ? -$cd-> [1] : 0;
			$cd = $rmap-> [ ord( substr( $text, $len - 1, 1))] || $nd;
			$w += ( $cd-> [3] < 0) ? -$cd-> [3] : 0;
		}
	} else {
		my $canvas = $self-> glyph_canvas($is_vector_font);
		$w = $canvas-> get_text_width( $text, $flags, $from, $len);
		if ( $is_vector_font ) {
			$w *= $self->{font_char_height} / $canvas->{pixel_scale} 
				unless $glyphs && $text->advances;
		}
	}

	return int( $w * $self-> {font}-> {width} / $self-> {font_width_divisor} + .5);
}

sub _rotate
{
	my ( $angle, $arr ) = @_;
	my $s = sin( $angle / 57.29577951);
	my $c = cos( $angle / 57.29577951);
	my $i;
	for ( $i = 0; $i < 10; $i+=2) {
		my ( $x, $y) = @$arr[$i,$i+1];
		$$arr[$i]   = $x * $c - $y * $s;
		$$arr[$i+1] = $x * $s + $y * $c;
	}
}

sub get_text_box
{
	my ( $self, $text, $from, $len) = @_;

	my $is_ps_font = $self->is_ps_font;
	my $is_vector_font = $self->is_vector_font;
	$from //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
		return 0 if $is_ps_font;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->get_text_box($self, $from, $len);
	} else {
		$len   = length($text) if !defined($len) || $len < 0 || $len > length($text);
		$text  = substr($text, $from, $len);
	}
	return [ (0) x 10 ] unless $len;

	my $wmul = $self-> {font}-> {width} / $self-> {font_width_divisor};
	my $dir  = $self-> {font}-> {direction};
	my @ret;

	if ( $is_ps_font ) {
		my ( $rmap, $nd) = $self-> get_rmap;
		my $cd;
		$cd = $rmap-> [ ord( substr( $text, 0, 1))] || $nd;
		my $ovxa = $wmul * (( $cd-> [1] < 0) ? -$cd-> [1] : 0);
		$cd = $rmap-> [ ord( substr( $text, $len - 1, 1))] || $nd;
		my $ovxb = $wmul * (( $cd-> [3] < 0) ? -$cd-> [3] : 0);

		my $w = $self-> get_text_width($text);
		@ret = (
			-$ovxa,      $self-> {font}-> {ascent} - 1,
			-$ovxa,     -$self-> {font}-> {descent},
			$w + $ovxb,  $self-> {font}-> {ascent} - 1,
			$w + $ovxb, -$self-> {font}-> {descent},
			$w, 0
		);
		unless ( $self-> textOutBaseline) {
			$ret[$_] += $self-> {font}-> {descent} for (1,3,5,7,9);
		}
		_rotate($dir, \@ret) if $dir != 0;
	} else {
		my $canvas = $self-> glyph_canvas($is_vector_font);
		@ret = @{ $canvas-> get_text_box( $text, $from, $len) };
		if ( $is_vector_font ) {
			my $div = $self->{font_char_height} / $canvas->{pixel_scale};
			if ($glyphs && $text->advances) {
				$_ *= $div for @ret[1,3,5,7,9];
			} else {
				$_ *= $div for @ret;
			}
		}
		if ( $wmul != 0 ) {
			_rotate(-$dir, \@ret) if $dir != 0;
			$ret[$_] *= $wmul for 0,2,4,6,8;
			_rotate($dir, \@ret) if $dir != 0;
		}
	}

	return \@ret;
}

sub text_shape
{
	my ( $self, $text, %opt ) = @_;

	unless ($self-> is_ps_font) {
		if ( $self->is_vector_font ) {
			my $canvas = $self-> glyph_canvas(1);
			my $shaped = $canvas->text_shape($text, %opt);
			$shaped->[Prima::Drawable::Glyphs::CUSTOM()] = $text;
			return $shaped unless $shaped && $shaped->advances;
			my $scale  = $self->{font_char_height} / $canvas->{pixel_scale};
			$_ *= $scale for @{ $shaped->advances  };
			$_ *= $scale for @{ $shaped->positions };
			return $shaped;
		} else {
			return $self->plate->text_shape($text, %opt);
		}
	}

	# convert to plain text
	my $shaped = $self->SUPER::text_shape($text, %opt, level => ts::None);
	return $shaped unless $shaped;

	my ($i, $x) = (0, $shaped->indexes);
	my @text = split '', $text;
	my $ret = join('', map { $text[ $_ & ~to::RTL ] } @$x[0..$#$x-1]);
	return $ret;
}

sub render_glyph {}

1;

__END__

=pod

=head1 NAME

Prima::PS::Drawable -  PostScript interface to Prima::Drawable

=head1 SYNOPSIS

	use Prima;
	use Prima::PS::Drawable;

	my $x = Prima::PS::Drawable-> create( onSpool => sub {
		open F, ">> ./test.ps";
		print F $_[1];
		close F;
	});
	die "error:$@" unless $x-> begin_doc;
	$x-> font-> size( 30);
	$x-> text_out( "hello!", 100, 100);
	$x-> end_doc;


=head1 DESCRIPTION

Realizes the Prima library interface to PostScript level 2 document language.
The module is designed to be compliant with Prima::Drawable interface.
All properties' behavior is as same as Prima::Drawable's, except those
described below.

=head2 Inherited properties

=over

=item ::resolution

Can be set while object is in normal stage - cannot be changed if document
is opened. Applies to fillPattern realization and general pixel-to-point
and vice versa calculations

=item ::region

- ::region is not realized ( yet?)

=back

=head2 Specific properties

=over

=item ::copies

amount of copies that PS interpreter should print

=item ::grayscale

could be 0 or 1

=item ::pageSize

physical page dimension, in points

=item ::pageMargins

non-printable page area, an array of 4 integers:
left, bottom, right and top margins in points.

=item ::reversed

if 1, a 90 degrees rotated document layout is assumed

=item ::rotate and ::scale

along with Prima::Drawable::translate provide PS-specific
transformation matrix manipulations. ::rotate is number,
measured in degrees, counter-clockwise. ::scale is array of
two numbers, respectively x- and y-scale. 1 is 100%, 2 is 200%
etc.

=item ::useDeviceFonts

1 by default; optimizes greatly text operations, but takes the risk
that a character could be drawn incorrectly or not drawn at all -
this behavior depends on a particular PS interpreter.

=item ::useDeviceFontsOnly

If 1, the system fonts, available from Prima::Application
interfaces can not be used. It is designed for
developers and the outside-of-Prima applications that wish to
use PS generation module without graphics. If 1, C<::useDeviceFonts>
is set to 1 automatically.

Default value is 0

=back

=head2 Internal methods

=over

=item emit

Can be called for direct PostScript code injection. Example:

	$x-> emit('0.314159 setgray');
	$x-> bar( 10, 10, 20, 20);

=item pixel2point and point2pixel

Helpers for translation from pixel to points and vice versa.

=item fill & stroke

Wrappers for PS outline that is expected to be filled or stroked.
Apply colors, line and fill styles if necessary.

=item spool

Prima::PS::Drawable is not responsible for output of
generated document, it just calls ::spool when document
is closed through ::end_doc. By default just skips data.
Prima::PS::Printer handles spooling logic.

=item fonts

Returns Prima::Application::font plus those that defined into Prima::PS::Fonts module.

=back

=cut

