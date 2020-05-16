package Prima::PS::Drawable;
use vars qw(@ISA);
@ISA = qw(Prima::Drawable);

use strict;
use warnings;
use Prima;
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
	);
	@$def{keys %prf} = values %prf;
	return $def;
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
	my %profile = $self-> SUPER::init(@_);
	$self-> $_( $profile{$_}) for qw( grayscale copies pageDevice
		rotate reversed isEPS);
	$self-> $_( @{$profile{$_}}) for qw( pageSize pageMargins resolution scale );
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

sub save_state
{
	my $self = $_[0];

	$self-> {save_state} = {};
	$self-> {save_state}-> {$_} = $self-> $_() for qw(
		color backColor fillPattern lineEnd linePattern lineWidth miterLimit
		rop rop2 textOpaque textOutBaseline font lineJoin fillMode
	);
	$self-> {save_state}-> {$_} = [$self-> $_()] for qw(
		translate clipRect
	);
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
	$self->defer_emission(1);
	$self->emit("%%Page: 1 1\n");

	$self-> {page_prefix} = <<PREFIX;
@{$self->{pageMargins}}[0,1] T
N 0 0 M 0 $y L $x 0 L 0 -$y L X C
PREFIX

	$self-> {page_prefix} .= "0 0 M 90 R 0 -$x T\n" if $self-> {reversed};

	$self-> {changed} = { map { $_ => 0 } qw(
		fill lineEnd linePattern lineWidth lineJoin miterLimit font)};

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
		qw (save_state ps_data changed page_prefix);
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

	$self-> {can_draw} = 0;
	$self-> SUPER::end_paint;
	$self-> restore_state;
	delete $self-> {$_} for
		qw (save_state changed ps_data page_prefix glyph_keeper glyph_font);
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
	$self-> emit( $self-> {page_prefix});
	$self-> change_transform(1);
	$self-> {changed}->{font} = 1;
	return 1;
}

sub pages { $_[0]-> {pages} }

sub spool
{
	shift-> notify( 'Spool', @_);
	return 1;
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

sub grayscale
{
	return $_[0]-> {grayscale} unless $#_;
	$_[0]-> {grayscale} = $_[1] unless $_[0]-> get_paint_state;
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

sub text_out_outline
{
	my ( $self, $text ) = @_;
	my $is_bytes = !Encode::is_utf8($text);
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
	my $div        = $self->{font_scale};
	my $restore_font;

	$len += $from;
	my $emit = '';
	my $fid  = 0;
	my $ff = $canvas->font;
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
			$self-> glyph_canvas_set_font( %$newfont );
			$font = $nfid ? $keeper->get_font($canvas->font) : $self->{glyph_font};
			$emit .= "/$font FF $self->{font}->{size} XF SF\n";
			$fid = $nfid;
		}
		my $char = defined($plaintext) ?
			substr( $plaintext, $indexes->[$i] & ~to::RTL, $ix_lengths[$i]) :
			undef;
		my $gid = $keeper-> use_char($canvas, $font, $glyph, $char);
		if ( $advances) {
			$advance = $advances->[$i];
			$x2 += $positions->[$i*2];
			$y2 += $positions->[$i*2 + 1];
		} else {
			my $xr = $canvas->get_font_abc($glyph, $glyph, to::Glyphs);
			$advance = ($$xr[0] + $$xr[1] + $$xr[2]) * $div;
		}
		$adv += $advance;
		if ( defined $gid ) {
			($x2, $y2) = map { int( $_ * 100 + 0.5) / 100 } $self->pixel2point($x2, $y2);
			$emit .= "$x2 $y2 M " if $x2 != 0 || $y2 != 0;
		} else {
			# not a single vector font found
			$gid //= $Prima::PS::Unicode->{$char} // 'question';
		}
		$emit .= "/$gid Y\n";
	}

	if ($restore_font) {
		$emit .= "/$self->{glyph_font} FF $self->{font}->{size} XF SF\n";
		$self-> glyph_canvas_set_font( %{ $self->{font} });
	}
	$self-> emit($emit);
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

	if ( $self-> {changed}-> {font}) {
		my $fn = $self->{glyph_font};
		$self-> emit( "/$fn FF $self->{font}->{size} XF SF");
		$self-> {changed}-> {font} = 0;
	}

	my $wmul = $self-> {font_x_scale};
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

	if ( $glyphs ) {
		$self->glyph_out_outline($text, $from, $len);
	} else {
		$self->text_out_outline($text);
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

	my $enc = 'iso10646-1'; # unicode only
	if ( !defined $family ) {
		my @fonts;
		my $num = $self->fontMapperPalette(-1);
		if ( $num > 0 ) {
			for my $fid ( 1 .. $num ) {
				my $f = $self->fontMapperPalette($fid) or next;
				$f->{encodings} = [$enc];
				$f->{encoding} = $enc;
				push @fonts, $f;
			}
		}
		return \@fonts;
	} else {
		return [] if defined($encoding) && $encoding ne '' && $encoding ne $enc;

		my @f = @{$::application->fonts($family) // []};
		return [] unless @f;
		$f[0]->{encoding} = $enc;
		return [$f[0]];
	}
}

sub glyph_canvas
{
	my $self = shift;
	return $self->{glyph_canvas} //= Prima::DeviceBitmap->create(
		width           => 1,
		height          => 1,
		textOutBaseline => 1,
	);
}

sub glyph_canvas_set_font
{
	my ($self, %font) = @_;

	my $g = $self-> glyph_canvas;
	$font{style} &= ~(fs::Underlined|fs::StruckOut);
	delete @font{qw(height width direction)};
	$font{size} = 1000;
	$g-> font(\%font);
}

sub get_font {+{%{$_[0]-> {font}}}}

sub set_font
{
	my ( $self, $font) = @_;

	my $canvas = $self-> glyph_canvas;
	my ($curr_font, $new_font) = ('', '');
	$curr_font = ($self->{font}->{size} // '-1'). '.' . ($self->{glyph_font} // '');

	$font = { %$font };
	my $wscale     = $font-> {width};
	delete $font-> {width};

	my $div        = 72.27 / $self-> {resolution}-> [1];
	my $by_height  = defined($font->{height});
	$font->{size} /= $div unless $by_height;
	$font = Prima::Drawable-> font_match( $font, $self-> {font});
	delete $font->{$by_height ? 'size' : 'height'};
	$canvas->set_font( $font );

	$font = $self-> {font} = { %{ $canvas->get_font } };
	$self-> {glyph_keeper} //= Prima::PS::Glyphs->new;
	$self-> {glyph_font} = $self-> {glyph_keeper}->get_font($font);
	$font->{size} *= $div;
	$font->{size} = int( $font->{size} * 100 + .5) / 100;

	my $font_width_divisor  = $font->{width};
	$font-> {width} = $wscale if $wscale;
	$self-> {font_x_scale}  = $font->{width} / $font_width_divisor;

	$new_font = $font->{size} . '.' . $self->{glyph_font};

	$self-> {changed}->{font} = 1 if $curr_font ne $new_font;

	$self-> glyph_canvas_set_font(%$font);
	my $f = $self->glyph_canvas->font;
	$self->{glyph_font}  = ($f->{pitch} == fp::Fixed) ? 'Courier' : 'Helvetica'
		if $f->{vector} != fv::Outline;
	my $point_scale     = ($f->height * ($f->height - $f->internalLeading) / $f->size);
	$self->{font_scale} = $font->{height} / $point_scale;
}

sub get_font_abc
{
	my ( $self, $first, $last, $flags) = @_;
	$first = 0     if !defined ($first) || $first < 0;
	$last = $first if !defined ($last) || $last < $first;
	my $canvas = $self-> glyph_canvas;
	my $scale  = $self->{font_scale} * $self->{font_x_scale};
	return [ map { $_ * $scale } @{ $canvas->get_font_abc($first, $last, $flags) } ];
}

sub get_font_def
{
	my ( $self, $first, $last, $flags) = @_;
	$first = 0     if !defined ($first) || $first < 0;
	$last = $first if !defined ($last) || $last < $first;
	my $canvas = $self-> glyph_canvas;
	my $scale  = $self->{font_scale};
	return [ map { $_ * $scale } @{ $canvas->get_font_def($first, $last, $flags) } ];
}

sub get_font_ranges    { shift->glyph_canvas->get_font_ranges    }
sub get_font_languages { shift->glyph_canvas->get_font_languages }

sub get_text_width
{
	my ( $self, $text, $flags, $from, $len) = @_;
	$flags //= 0;
	$from  //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->get_text_width($self, $flags, $from, $len);
	} else {
		$len = length($text) if !defined($len) || $len < 0 || $len > length($text);
	}
	return 0 unless $len;

	my $w = $self->glyph_canvas-> get_text_width( $text, $flags, $from, $len);
	$w *= $self->{font_scale} unless $glyphs && $text->advances;
	return int( $w * $self-> {font_x_scale} + .5);
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

	$from //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->get_text_box($self, $from, $len);
	} else {
		$len  = length($text) if !defined($len) || $len < 0 || $len > length($text);
	}
	return [ (0) x 10 ] unless $len;

	my $wmul = $self->{font_x_scale};
	my $dir  = $self->{font}->{direction};
	my @ret;

	@ret = @{ $self-> glyph_canvas-> get_text_box( $text, $from, $len) };
	my $div = $self->{font_scale};
	if ($glyphs && $text->advances) {
		$_ *= $div for @ret[1,3,5,7,9];
	} else {
		$_ *= $div for @ret;
	}

	if ( $wmul != 0.0 && $wmul != 1.0 ) {
		_rotate(-$dir, \@ret) if $dir != 0;
		$ret[$_] *= $wmul for 0,2,4,6,8;
		_rotate($dir, \@ret) if $dir != 0;
	}

	return \@ret;
}

sub text_shape
{
	my ( $self, $text, %opt ) = @_;

	my $canvas = $self-> glyph_canvas;
	my $shaped = $canvas->text_shape($text, %opt);
	return $shaped unless $shaped;
	$shaped->[Prima::Drawable::Glyphs::CUSTOM()] = $text;
	if ( $shaped-> advances ) {
		my $scale  = $self->{font_scale};
		$_ *= $scale for @{ $shaped->advances  };
		$_ *= $scale for @{ $shaped->positions };
	}
	return $shaped;
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

Returns Prima::Application::fonts, however with C<iso10646-1> encoding only.
That effectively allows only unicode output.

=back

=cut

