package fontdlg;

=pod

=head1 NAME

examples/fontdlg.pl - An alternate font selection window

=head1 FEATURES

Demonstrates Prima font API and its usage.

Note the inability to set a font with a particular size and width factor in one
call ( in $re_sample sub ). A font size and width is accepted, however.

Tests the Prima font interface implementation. A constant pain here is the
correspondence of a font metrics before and after the font load.  X is known
for the problem, that can not be solved easily and without certain compromises.
See L<prima-gp-problems> manpage for details.

Note the left-mouse drag effect from a font screen widget.

=cut


use strict;
use warnings;
use Carp;
use Prima;
use Prima::Application name => "Font Dialog";
use Prima qw(Lists Sliders Buttons FileDialog);
use Encode;

# try to use perl5.8 glyph names
eval "use charnames qw(:full);";
my $use_charnames = $@ ? 0 : 1;

sub run {
my $w = 0;

my @fontItems = ();
my %fontList  = ();
my $displayRes;


my $fs = 0;
my $fd = 0;
my $fpitch = fp::Default;
my $fvector = fv::Default;
my $fwidth = 0;

my $re_sample = sub {
	return if $w-> {exampleFontSet};
	my $fid = $w-> NameList-> get_item_text( $w-> NameList-> focusedItem);
	my $fn = ($fid =~ /not defined/) ? 'Default' : $fontList{$fid}{name};
	$w-> {exampleFontSet} = 1;
	my $i = $w-> SizeList-> focusedItem;
	my $enc = $w-> Encoding-> get_item_text( $w-> Encoding-> focusedItem);
	$enc = '' if $enc eq '(any)';

	$w-> Example-> lock;
	my %font = (
		name        => $fn,
		size        => $w-> SizeList-> get_item_text( $i),
		style       => $fs,
		direction   => $fd,
		pitch       => $fpitch,
		vector      => $fvector,
		encoding    => $enc,
	);

	$w-> Example-> font( %font,
		width => 0,
	);

	$w-> Example-> font( %font,
		width => $w-> Example-> font-> width * $fwidth,
	) if $fwidth;

	$w-> Example-> unlock;

	$w-> {exampleFontSet} = undef;
};

my $lastSizeSel = 12;
my $lastEncSel = "";

my $re_size = sub {
	my $name_changed = $_[0];
	my $fid = $w-> NameList-> get_item_text( $w-> NameList-> focusedItem);
	my $fn = ($fid =~ /not defined/) ? undef : $fontList{$fid}{name};
	my @sizes = ();
	my $current_encoding = ( $lastEncSel eq '(any)' || $name_changed) ? '' : $lastEncSel;
	my @list = $fn ? 
		@{$::application-> fonts( $fn, $name_changed ? '' : $current_encoding)} :
		({vector => 1});

	if ( $name_changed) {
		my %enc;
		my @enc_items;
		for ( grep { defined } map { $_-> {encoding}} @list) {
			next if $enc{$_};
			push ( @enc_items, $_ );
			$enc{$_} = 1;
		}
		unshift @enc_items, "(any)";
		my $found = 0;
		my $i = 0;
		for ( @enc_items) {
			$found = $i, last if $_ eq $lastEncSel;
			$i++;
		}
		$w-> Encoding-> set_items( \@enc_items);
		$w-> Encoding-> set_focused_item( $found);
	}

	for ( @list)
	{
		next if length( $current_encoding) && ( $current_encoding ne $_-> {encoding});
		if ( $_-> { vector})
		{
			@sizes = qw( 8 9 10 11 12 14 16 18 20 22 24 26 28 32 48 72);
			last;
		} else {
			push ( @sizes, $_-> {size});
		}
	}
	my %k = map { $_ => 1 } @sizes;
	@sizes = sort { $a <=> $b } keys %k;
	@sizes = (10) unless scalar @sizes;

	my $i;
	my $found = 0;
	for ( $i = 0; $i < scalar @sizes; $i++)
	{
		if ( $sizes[$i] == $lastSizeSel)
		{
			$found = 1;
			last;
		}
	}
	unless ( $found)
	{
		for ( $i = 0; $i < scalar @sizes; $i++)
		{
			last if ( $sizes[$i] > $lastSizeSel);
		}
		$i-- if $i = scalar @sizes;
	}
	$w-> SizeList-> set_items(\@sizes);
	$w-> SizeList-> set_focused_item($i);
};

$w = Prima::MainWindow-> create( text => "Font Window",
	origin => [ 200, 200],
	size   => [ 500, 590],
	designScale => [7,16],
	borderStyle => bs::Dialog,
);

sub create_info_window
{
	my $w = shift;
	my $f = $w-> Example-> font;
	my $ww = Prima::Window-> create(
		size => [ 500, $f-> height * 3 + $f-> externalLeading + $f-> descent + 450 ],
		font => $f,
		text => $f-> size.'.['.$f-> height.'x'.$f-> width.']'.$f-> name,
		onPaint => sub {
			my ( $self, $p) = @_;
			my @size = $p-> size;
			$p-> clear;
			$p-> font-> direction(0);

			my $m = $p-> get_font;
			my $xtext = Encode::decode('latin1', "\x{c5}Mg");
			my $s = $size[1] - $m-> {height} - $m-> {externalLeading} - 20;
			my $w = $p-> get_text_width($xtext) + 66;
			$p-> textOutBaseline(1);
			$p-> text_out($xtext, 20, $s);

			my $cachedFacename = $p-> font-> name;
			my $hsf = $p-> font-> height / 6;
			$hsf = 10 if $hsf < 10;
			$p-> font-> set(
				height   => $hsf,
				style    => fs::Italic,
				name     => '',
				encoding => '',
			);

			$p-> line( 2, $s, $w, $s);
			$p-> textOutBaseline(0);
			$p-> text_out( "Baseline", $w - 8, $s);
			my $sd = $s - $m-> {descent};
			$p-> line( 2, $sd, $w, $sd);
			$p-> text_out( "Descent",  $w - 8, $sd);
			$sd = $s + $m-> {ascent};
			$p-> line( 2, $sd, $w, $sd);
			$p-> text_out( "Ascent",  $w - 8, $sd);
			$sd = $s + $m-> {ascent} + $m-> {externalLeading};

			if ( $m-> {ascent} > 4) {
				$p-> line( $w - 12, $s + 1, $w - 12, $s + $m-> {ascent});
				$p-> line( $w - 12, $s + $m-> {ascent}, $w - 14, $s + $m-> {ascent} - 2);
				$p-> line( $w - 12, $s + $m-> {ascent}, $w - 10, $s + $m-> {ascent} - 2);
			}
			if ( ($m-> {ascent}-$m-> {internalLeading}) > 4) {
				my $pt = $m-> {ascent}-$m-> {internalLeading};
				$p-> line( $w - 16, $s + 1, $w - 16, $s + $pt);
				$p-> line( $w - 16, $s + $pt, $w - 18, $s + $pt - 2);
				$p-> line( $w - 16, $s + $pt, $w - 14, $s + $pt - 2);
			}
			if ( $m-> {descent} > 4) {
				$p-> line( $w - 13, $s - 1, $w - 13, $s - $m-> {descent});
				$p-> line( $w - 13, $s - $m-> {descent}, $w - 15, $s - $m-> {descent} + 2);
				$p-> line( $w - 13, $s - $m-> {descent}, $w - 11, $s - $m-> {descent} + 2)
			}

			my $str;
			$p-> text_out( "External Leading",  2, $sd);
			$p-> line( 2, $sd, $w, $sd);
			$sd = $s + $m-> {ascent} - $m-> {internalLeading};
			$str = "Point size in device units";
			$p-> text_out( $str,  $w - 16 - $p-> get_text_width( $str), $sd);
			$p-> linePattern( lp::Dash);
			$p-> line( 2, $sd, $w, $sd);


			$p-> font-> set(
				height => 16,
				pitch  => fp::Fixed,
			);
			my $fh = $p-> font-> height;
			$sd = $s - $m-> {descent} - $fh * 3;
			$p-> text_out( 'nominal size        : '.$m-> {size}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'cell height         : '.$m-> {height   }, 2, $sd); $sd -= $fh;
			$p-> text_out( 'average width       : '.$m-> {width    }, 2, $sd); $sd -= $fh;
			$p-> text_out( 'ascent              : '.$m-> {ascent   }, 2, $sd); $sd -= $fh;
			$p-> text_out( 'descent             : '.$m-> {descent  }, 2, $sd); $sd -= $fh;
			$p-> text_out( 'weight              : '.$m-> {weight   }, 2, $sd); $sd -= $fh;
			$p-> text_out( 'internal leading    : '.$m-> {internalLeading}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'external leading    : '.$m-> {externalLeading}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'maximal width       : '.$m-> {maximalWidth}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'vector              : '.$m-> {vector}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'horizontal dev.res. : '.$m-> {xDeviceRes}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'vertical dev.res.   : '.$m-> {yDeviceRes}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'first char          : '.$m-> {firstChar}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'last char           : '.$m-> {lastChar }, 2, $sd); $sd -= $fh;
			$p-> text_out( 'break char          : '.$m-> {breakChar}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'default char        : '.$m-> {defaultChar}, 2, $sd); $sd -= $fh;
			$p-> text_out( 'family              : '.$m-> {family   }, 2, $sd); $sd -= $fh;
			$p-> text_out( 'face name           : '.$cachedFacename, 2, $sd); $sd -= $fh;
		},
	);

	my %wf = ( %$f, direction => 0 );
	my @gsize= ( $f->maximalWidth + 10 , $f->height + 10 );
	my $glyph = $ww-> insert( Widget =>
		text => ' ',
		font => \%wf,
		size => \@gsize,
		origin => [ $ww-> width - $gsize[0] - 10, $ww-> height - $gsize[1] - 10 ],
		growMode => gm::GrowLoX|gm::GrowLoY,
		backColor => cl::White,
		#buffered => 1,
		name    => 'Glyph',
		textOutBaseline => 1,
		onPaint => sub {
			my $self = shift;
			$self-> clear;
			my $C = $self-> text;

			my ( $a, $b, $c ) = @{ $self->get_font_abc( ord($C), ord($C), utf8::is_utf8($C)) };
			my ( $d, $e, $f ) = @{ $self->get_font_def( ord($C), ord($C), utf8::is_utf8($C)) };

			my $w = (( $a < 0 ) ? 0 : $a) + $b + (( $c < 0 ) ? 0 : $c);
			my $h = (( $d < 0 ) ? 0 : $d) + $e + (( $f < 0 ) ? 0 : $f);
			$w = ( $self-> width  - $w ) / 2;
			$h = ( $self-> height - $h ) / 2;
			$self-> translate($w, $h);

			my $dx = 0;
			my $dy = 0;
			$dx -= $a if $a < 0;
			$dy -= $d if $d < 0;

			my $fh = $self-> font->height;
			$self-> text_out( $C, $dx, $dy + $self->font-> descent);

			$dx = abs($a);
			$dy = abs($d);
			$self-> linePattern(lp::Dot);
			$self-> line($dx, 0, $dx, $self->height);
			$dx = (( $a < 0 ) ? 0 : $a) + $b + (( $c < 0 ) ? 0 : $c) - abs($c);
			$self-> line($dx, 0, $dx, $self->height);
			$self-> line(-$w, $self->font->descent, $self->width, $self->font->descent);

			$self-> line(0, $dy, $self->width, $dy);
			$dy = (( $d < 0 ) ? 0 : $d) + $e + (( $f < 0 ) ? 0 : $f) - abs($f);
			$self-> line(0, $dy, $self->width, $dy);

			$self-> linePattern(lp::Solid);
			$self-> color(cl::LightRed);
			my $path = $self-> new_path;
			$path->translate( -$a, 0 ) if $a < 0;
			$path-> text( $C, baseline => 0 );
			$path->stroke;
		},
	);


	my @ranges = ([]);
	for ( @{$w-> Example-> get_font_ranges}) {
		( 2 > scalar @{$ranges[-1]}) ?
			push @{$ranges[-1]}, $_ :
			push @ranges, [$_];
	}
	@ranges = sort { $a->[0] <=> $b-> [0] } @ranges;
	my %charmap;
	my $count = 0;
	for ( @ranges) {
		$charmap{$count++} = $_ for $$_[0] .. $$_[1];
	}
	my $ih = int($f-> height * 1.5);
	my $l = $ww-> insert( AbstractListViewer =>
		origin => [0,0],
		size   => [$ww-> width, $ww-> height - $f-> height - $f-> externalLeading - $f-> descent - 360],
		growMode => gm::Client,
		font     => $f,
		multiColumn => 1,
		itemWidth   => $ih,
		itemHeight  => $ih,
		gridColor   => cl::Back,
		hScroll     => 1,
		onSelectItem => sub {
			my ( $self, $item, $sel) = @_;
			if ( defined( my $c = $charmap{$item->[0]} )) {
				my $pretty = sprintf( "0x%x", $c);
				if ( $use_charnames) {
					my $x = charnames::viacode($c);
					$pretty .= " - $x" if defined $x;
				}
				$self-> hint( $pretty );
				$self-> hintVisible(1);
				$glyph->text(chr($c));
				$glyph->repaint;
			}
		},
		onDrawItem => sub {
			my ($self, $canvas, $itemIndex, $x, $y, $x2, $y2, $selected, $focused, $prelight) = @_;
			$canvas-> line( $x, $y, $x2, $y);
			$canvas-> line( $x2+1, $y, $x2+1, $y2);
			my @cs;
			if ( $focused || $prelight) {
				@cs = ( $canvas-> color, $canvas-> backColor);
				my $fo = $focused ? $canvas-> hiliteColor : $canvas-> color ;
				my $bk = $focused ? $canvas-> hiliteBackColor : $canvas-> backColor ;
				$canvas-> set( color => $fo, backColor => $bk );
			}
			$self-> draw_item_background( $canvas, $x, $y + 1, $x2, $y2, $prelight );
			if ( defined( my $c = $charmap{$itemIndex} )) {
				$canvas-> text_out( chr($c), $x + $ih / 4, $y + $ih / 4);
			}
			$canvas-> set( color => $cs[0], backColor => $cs[1]) if $focused || $prelight;
		},
	);
	$l-> count( $count);
	$ww-> select;
}

$displayRes = ($w-> resolution)[1];

my $load_fonts = sub {
	%fontList = ();
	@fontItems = ("<not defined>");
	for ( sort { $a-> {name} cmp $b-> {name}} @{$::application-> fonts})
	{
		$fontList{$_-> {name}} = $_;
		push ( @fontItems, $_-> {name});
	}
};

$load_fonts->();

$w-> insert( ListBox =>
	name   => "NameList",
	origin => [25, 25],
	size   => [ 225, 315],
	items => [@fontItems],
	onSelectItem => sub {
		&$re_size(1);
		&$re_sample;
	},
);

$w-> insert( ListBox =>
	name   => 'SizeList',
	origin => [ 270, 230],
	size   => [ 200, 110],
	onSelectItem => sub {
		$lastSizeSel = $_[0]-> get_item_text( $_[0]-> focusedItem);
		&$re_sample;
	},
);

$w-> insert( ListBox =>
	origin      => [ 270, 160],
	size        => [ 200, 55],
	name        => 'Encoding',
	onSelectItem => sub {
		$lastEncSel = $_[0]-> get_item_text( $_[0]-> focusedItem);
		&$re_size(0);
		&$re_sample;
	},

);

my $scaling = $::application->font->height / ($w->designScale)[1];

$w-> insert( Button =>
	origin => [ 24, 348],
	size   => [ 32, 32],
	text   => 'B',
	name   => 'Bold',
	selectable => 0,
	font   => {
		height => 20 * $scaling,
		style  => fs::Bold,
	},
	checkable => 1,
	onClick   => sub {
		$fs = ( $fs & fs::Bold ? $fs & ~fs::Bold : $fs | fs::Bold);
		&$re_sample;
	},
);

$w-> insert( Button =>
	origin => [ 60, 348],
	size   => [ 32, 32],
	text   => 'I',
	name   => 'Italic',
	selectable => 0,
	font   => {
		height => 20 * $scaling,
		style  => fs::Italic,
	},
	checkable => 1,
	onClick   => sub {
		$fs = (( $fs & fs::Italic) ? ($fs & ~fs::Italic) : ($fs | fs::Italic));
		&$re_sample;
	},
);

$w-> insert( Button =>
	origin => [ 96, 348],
	size   => [ 32, 32],
	text   => 'U',
	selectable => 0,
	name   => 'Underlined',
	font   => {
		height => 20 * $scaling,
		style  => fs::Underlined,
	},
	checkable => 1,
	onClick   => sub {
		$fs = (( $fs & fs::Underlined) ? ($fs & ~fs::Underlined) : ($fs | fs::Underlined));
		&$re_sample;
	},
);

$w-> insert( Button =>
	origin => [ 142, 348],
	size   => [ 32, 32],
	text   => 'i',
	selectable => 0,
	name   => 'Info',
	color  => cl::Blue,
	font   => { height => 28 * $scaling, style => fs::Bold },
	onClick   => sub { create_info_window($w) },
);

$w-> insert( Button =>
	origin => [ 180, 348],
	size   => [ 32, 32],
	text   => '+',
	hint   => 'Add new font',
	selectable => 0,
	name   => 'Load',
	font   => { height => 20 * $scaling },
	onClick   => sub {
		my $d = Prima::FileDialog-> new;
		my $f = $d->execute;
		return unless defined $f;
		my $error = '';
		local $SIG{__WARN__} = sub { $error = ": @_" };
		unless ($::application->load_font($d->fileName)) {
			$error =~ s/at examples.*//;
			return Prima::message("Error loading font$error");
		}

		$load_fonts->();
		$w->NameList->items(\@fontItems);
	}
);


my $csl = $w-> insert( CircularSlider =>
	origin      => [ 370, 348],
	size        => [ 100, 100],
	name        => 'Angle',
	buttons     => 0,
	font        => {size => 5},
	min         => -180,
	max         => 180,
	scheme      => ss::Axis,
	increment   => 30,
	step        => 10,
	onChange    => sub {  $fd = $_[0]-> value; &$re_sample; },
);

$csl-> insert( Button =>
	origin => [ 10, 10],
	size   => [ 14, 14],
	text   => 'o',
	onClick => sub { $_[0]-> owner-> value(0); },
);

my $rg = $w-> insert( RadioGroup =>
	origin      => [ 25, 525],
	size        => [ 445, 58],
	name        => 'Pitch',
);

$rg-> insert( Radio =>
	name   => 'Default',
	origin => [ 15, 5],
	onClick =>  sub { $fpitch = fp::Default;  &$re_sample; },
	checked => 1,
);

$rg-> insert( Radio =>
	name   => 'Fixed',
	origin => [ 160, 5],
	onClick =>  sub { $fpitch = fp::Fixed;  &$re_sample; },
	font    => { style => fs::Bold, pitch => fp::Fixed},
);

$rg-> insert( Radio =>
	name   => 'Variable',
	origin => [ 305, 5],
	font    => { style => fs::Bold|fs::Italic, pitch => fp::Variable},
	onClick =>  sub { $fpitch = fp::Variable; &$re_sample; },
);

my $rg2 = $w-> insert( RadioGroup =>
	origin      => [ 25, 460],
	size        => [ 445, 58],
	name        => 'Vector',
);

$rg2-> insert( Radio =>
	name   => 'Default',
	origin => [ 15, 5],
	onClick =>  sub { $fvector = fv::Default;  &$re_sample; },
	checked => 1,
);

$rg2-> insert( Radio =>
	name   => 'Bitmap',
	origin => [ 160, 5],
	onClick =>  sub { $fvector = fv::Bitmap;  &$re_sample; },
	font    => { style => fs::Bold, pitch => fp::Fixed}, # yes, fixed
);

$rg2-> insert( Radio =>
	name   => 'Outline',
	origin => [ 305, 5],
	font    => { vector => fv::Outline, style => fs::Italic },
	onClick =>  sub { $fvector = fv::Outline; &$re_sample; },
);


$w-> insert( Slider =>
	name     => 'Stretcher',
	origin   => [ 25, 382],
	size     => [ 225, 58],
	vertical => 0,
	min      => -5,
	max      => 5,
	scheme   => ss::Axis,
	step     => 0.5,
	increment=> 5,
	value    => 0,
	onChange => sub {
		$fwidth = $_[0]-> value;
		if ( $fwidth > 0) {
			$fwidth += 1;
		} elsif ( $fwidth < 0) {
			$fwidth = -1 / ( $fwidth - 1);
		}
		&$re_sample;
	},
);

$w-> insert( Button =>
	origin => [ 130, 440],
	size   => [ 14, 14],
	text   => 'o',
	font   => {size => 5},
	onClick => sub { $w-> Stretcher-> value(0); },
);


$w-> insert( Widget =>
	name      => 'Example',
	origin    => [ 270, 25],
	size      => [ 200, 120],
	backColor => cl::White,
	onPaint   => sub {
		my ($fore, $back, $x, $y) =
			($_[0]-> color, $_[0]-> backColor, $_[1]-> width, $_[1]-> height);
		$_[1]-> color( $back);
		$_[1]-> bar( 0, 0, $x, $y);
		$_[1]-> color( $fore);
		my $m = $_[1]-> get_font;
		my $probe;
		my @ranges = @{$_[1]-> get_font_ranges};
		my $vec = '';
		my ($i,$j);
		my ($latin,$non_latin)=('','');
		RANGES: for ( $i = 0; $i < @ranges; $i += 2) {
			for ( $j = $ranges[$i]; $j < $ranges[$i+1]; $j++) {
				$latin     .= chr($j) if $j > 45 && $j < 128;
				$non_latin .= chr($j) if $j > 256 && ( !$use_charnames || (charnames::viacode($j) || '') !~
					/(space|punctuation|mark|accent|point|combining|modifier)/i);
				last RANGES if length($latin) > 32 || length($non_latin) > 32;
			}
		}
		$probe = (length($latin) > 32) ? $_[1]-> font-> size.".".$_[1]-> font-> name : substr($non_latin, 0, 12);

		my @box = @{$_[1]-> get_text_box( $probe)};
		pop @box;
		pop @box;
		my $width = $_[1]-> get_text_width( $probe);
		my ( $ox, $oy) = (( $x - $width) / 2, ( $y - $_[1]-> font-> height) / 2);
		$box[$_] += $ox for 0,2,4,6;
		$box[$_] += $oy for 1,3,5,7;
		@box[4,5,6,7] = @box[6,7,4,5];
		$_[1]-> color( cl::Yellow);
		$_[1]-> fillpoly(\@box);
		$_[1]-> color( cl::Black);
		$_[1]-> text_out( $probe, $ox, $oy);
	},
	onFontChanged => sub {
		unless ( defined $w-> {exampleFontSet})
		{
			my $font = $_[0]-> font;
			my $name = $font-> name;
			my $size = $font-> size;
			$fs = $font-> style;
			$fd = $font-> direction;
			my ( $i, $j);
			for ( $i = 0; $i < scalar @fontItems; $i++)
			{
				last if $name eq $fontItems[ $i];
			}
			$w-> NameList-> set_focused_item( $i);
			my @sizes = @{$w-> SizeList-> items};
			for ( $j = 0; $j < scalar @sizes; $j++)
			{
				last if $size == $sizes[ $j];
			}
			$w-> SizeList-> set_focused_item( $j);
			$w-> Bold-> checked( $fs & fs::Bold);
			$w-> Italic-> checked( $fs & fs::Italic);
			$w-> Underlined-> checked( $fs & fs::Underlined);
			$w-> Angle-> value( $fd);
		}
	},
	onMouseDown => sub {
		return if $_[0]-> {drag};
		$_[0]-> {drag} = 1;
		$_[0]-> capture(1);
		$_[0]-> pointer( cr::Invalid)
	},
	onMouseUp   => sub {
		return unless $_[0]-> {drag};
		$_[0]-> capture(0);
		$_[0]-> {drag} = 0;
		$_[0]-> pointer( cr::Default);
		my $x = $::application-> get_widget_from_point(
			$_[0]-> client_to_screen( $_[3], $_[4])
		);
		return unless $x;
		$x-> font( $_[0]-> font);
	},
);

&$re_size(1);
&$re_sample;
}

run;
run Prima;


