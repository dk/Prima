=head1 NAME

examples/emoji.pl - displays color glyphs of an emoji font

=cut


use strict;
use warnings;
use Prima qw(Application Lists);

# try to use perl5.8 glyph names
eval "use charnames qw(:full);";
my $use_charnames = $@ ? 0 : 1;

my $font;
if ( @ARGV ) {
	$font = $ARGV[0];
} else {
	for ( @{$::application->fonts} ) {
		next unless $_->{name} =~ /emoji/i;
		$font = $_->{name};
		last;
	}
	warn "You don't seem to have an emoji font\n" unless $font;
}

my @glyphs;
$::application->begin_paint_info;
$::application->font( {name => $font, encoding => '' } );
my $ranges = $::application-> get_font_ranges;
for ( my $i = 0; $i < @$ranges; $i += 2 ) {
	next if $ranges->[$i] < 0x100;
	push @glyphs, $ranges->[$i] .. $ranges->[$i+1];
}
$::application->end_paint_info;

my $w = Prima::MainWindow->new(
	text => 'Emojis',
	packPropagate => 0,
	font => { name => $font, encoding => '' },
);

my $ih = $w->font->height * 4;
my $cr;
my $dx;
my $dy;
$w->insert( AbstractListViewer => 
	pack => { fill => 'both', expand => 1 },
	multiColumn => 1,
	itemWidth   => $ih,
	itemHeight  => $ih,
	hScroll     => 1,
	vScroll     => 1,
	drawGrid    => 0,
	onPaint     => sub {
		my ( $self, $canvas ) = @_;
		$self->clear;
		$self->on_paint($canvas);
		undef $cr;
	},
	onDrawItem  => sub {
		my ( $self, $canvas, $index, $x1, $y1, $x2, $y2, $selected, $focused, $prelight ) = @_;
		my @cs;
		if ( $focused || $prelight) {
			@cs = ( $canvas-> color, $canvas-> backColor);
			my $fo = $focused ? $canvas-> hiliteColor : $canvas-> color ;
			my $bk = $focused ? $canvas-> hiliteBackColor : $canvas-> backColor ;
			$canvas-> set( color => $fo, backColor => $bk );

		}
		$self-> draw_item_background( $canvas, $x1, $y1, $x2, $y2, $prelight );
		$canvas->font->size( 25 );
		$dx //= ($ih - $canvas->font->width) / 2;
		$dy //= ($ih - $canvas->font->height) / 2 + $canvas->font->descent;
		$canvas->text_out(chr($glyphs[$index]) , $x1 + $dx, $y1 + $dy);
		$canvas->font->size( 8 );
		my $tx = sprintf("%x", $glyphs[$index] );
		$canvas-> text_out( $tx, $x1 + ( $x2 - $x1 - $canvas->get_text_width($tx)) / 2, $y1);
		$canvas-> set( color => $cs[0], backColor => $cs[1]) if $focused || $prelight;
	},
	onSelectItem => sub {
		return unless $use_charnames;
		my $x = charnames::viacode($glyphs[$_[1]->[0]]);
		$w->text($x) if defined $x;
	},
)->count(scalar @glyphs);

run Prima;
