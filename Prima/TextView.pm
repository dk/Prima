#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk> 
#
#  $Id$

use strict;
use Prima;
use Prima::IntUtils;
use Prima::ScrollBar;

package 
    tb;
use vars qw(@oplen);

@oplen = ( 4, 2, 3, 4, 3, 2, 4);   # lengths of tb::OP_XXX constants ( see below ) + 1

# basic opcodes
use constant OP_TEXT               =>  0; # (3) text offset, text length, text width
use constant OP_COLOR              =>  1; # (1) 0xRRGGBB or COLOR_INDEX | palette_index
use constant OP_FONT               =>  2; # (2) op_font_mode, font info
use constant OP_TRANSPOSE          =>  3; # (3) move current point to delta X, delta Y
use constant OP_CODE               =>  4; # (2) code pointer and parameters 

# formatting opcodes
use constant OP_WRAP               =>  5; # (1) on / off
use constant OP_MARK               =>  6; # (3) id, x, y

# OP_TEXT 
use constant T_OFS                => 1;
use constant T_LEN                => 2;
use constant T_WID                => 3;

# OP_FONT
use constant F_MODE                => 1;
use constant F_DATA                => 2;

# OP_COLOR
use constant COLOR_INDEX           => 0x01000000; # index in colormap() array
use constant BACKCOLOR_FLAG        => 0x02000000; # OP_COLOR flag for backColor
use constant BACKCOLOR_DEFAULT     => BACKCOLOR_FLAG|COLOR_INDEX|1;
use constant COLOR_MASK            => 0xFCFFFFFF;

# OP_TRANSPOSE - indices
use constant X_X     => 1;
use constant X_Y     => 2;
use constant X_FLAGS => 3;

# OP_TRANSPOSE - X_FLAGS constants
use constant X_DIMENSION_PIXEL       => 0;
use constant X_TRANSPOSE             => 0;
use constant X_EXTEND                => 1;
# formatting flags
use constant X_DIMENSION_FONT_HEIGHT => 2; # multiply by font height
use constant X_DIMENSION_POINT       => 4; # multiply by resolution / 72

# block header indices
use constant  BLK_FLAGS            => 0;
use constant  BLK_WIDTH            => 1;
use constant  BLK_HEIGHT           => 2;
use constant  BLK_X                => 3;
use constant  BLK_Y                => 4;
use constant  BLK_APERTURE_X       => 5;
use constant  BLK_APERTURE_Y       => 6;
use constant  BLK_TEXT_OFFSET      => 7;
use constant  BLK_DATA_START       => 8;
use constant  BLK_FONT_ID          => BLK_DATA_START; 
use constant  BLK_FONT_SIZE        => 9; 
use constant  BLK_FONT_STYLE       => 10;
use constant  BLK_COLOR            => 11; 
use constant  BLK_DATA_END         => 12;
use constant  BLK_BACKCOLOR        => BLK_DATA_END;
use constant  BLK_START            => BLK_DATA_END + 1;

# OP_FONT again
use constant  F_ID    => BLK_FONT_ID;
use constant  F_SIZE  => BLK_FONT_SIZE;
use constant  F_STYLE => BLK_FONT_STYLE;
use constant  F_HEIGHT=> 1000000; 

# BLK_FLAGS constants
use constant T_SIZE      => 0x1;
use constant T_WRAPABLE  => 0x2;

# realize_state mode

use constant REALIZE_FONTS   => 0x1;
use constant REALIZE_COLORS  => 0x2;
use constant REALIZE_ALL     => 0x3;


use constant YMAX => 1000;

sub block_create
{
	my $ret = [ ( 0 ) x BLK_START ];
	$$ret[ BLK_FLAGS ] |= T_SIZE;
	push @$ret, @_;
	return $ret;
}

sub block_count
{
	my $block = $_[0];
	my $ret = 0;
	my ( $i, $lim) = ( BLK_START, scalar @$block);
	$i += $oplen[$$block[$i]], $ret++ while $i < $lim;
	return $ret;
}

# creates a new opcode for custom use
sub opcode
{
	my $len = $_[0] || 0;
	$len = 0 if $len < 0;
	push @oplen, $len + 1;
	return scalar(@oplen) - 1;
}


sub text           { return OP_TEXT, $_[0], $_[1], $_[2] || 0 }
sub color          { return OP_COLOR, $_[0] } 
sub backColor      { return OP_COLOR, $_[0] | BACKCOLOR_FLAG}
sub colorIndex     { return OP_COLOR, $_[0] | COLOR_INDEX }  
sub backColorIndex { return OP_COLOR, $_[0] | COLOR_INDEX | BACKCOLOR_FLAG}  
sub fontId         { return OP_FONT, F_ID, $_[0] }
sub fontSize       { return OP_FONT, F_SIZE, $_[0] }
sub fontHeight     { return OP_FONT, F_SIZE, $_[0] + F_HEIGHT }
sub fontStyle      { return OP_FONT, F_STYLE, $_[0] }
sub moveto         { return OP_TRANSPOSE, $_[0], $_[1],  $_[2] || 0 } 
sub extend         { return OP_TRANSPOSE, $_[0], $_[1], ($_[2] || 0) | X_EXTEND } 
sub code           { return OP_CODE, $_[0], $_[1] } 
sub wrap           { return OP_WRAP, $_[0] } 
sub mark           { return OP_MARK, $_[0], 0, 0 } 

package Prima::TextView::EventContent;

sub on_mousedown {}
sub on_mousemove {}
sub on_mouseup   {}

package Prima::TextView;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller Prima::GroupScroller);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		autoHScroll       => 1,
		autoVScroll       => 0,
		borderWidth     => 2,
		colorMap        => [ $def-> {color}, $def-> {backColor} ],
		fontPalette     => [ { 
			name     => $def-> {font}-> {name},
			encoding => '',
			pitch    => fp::Default,
		}],
		hScroll         => 1,
		offset          => 0,
		paneWidth       => 0,
		paneHeight      => 0,
		paneSize        => [0,0],
		resolution      => [ $::application-> resolution ],
		topLine         => 0,
		scaleChildren   => 0,
		selectable      => 1,
		textOutBaseline => 1,
		textRef         => '',
		vScroll         => 1,
		widgetClass     => wc::Edit,
		pointer         => cr::Text,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	if ( exists( $p-> { paneSize})) {
		$p-> { paneWidth}  = $p-> { paneSize}-> [ 0];
		$p-> { paneHeight} = $p-> { paneSize}-> [ 1];
	}
	$p-> { text} = '' if exists( $p-> { textRef});
	$p-> {autoHScroll} = 0 if exists $p-> {hScroll};
	$p-> {autoVScroll} = 0 if exists $p-> {vScroll};
}   

sub init
{
	my $self = shift;
	for ( qw( topLine scrollTransaction hScroll vScroll offset 
		paneWidth paneHeight borderWidth autoVScroll autoHScroll))
		{ $self-> {$_} = 0; }
	my %profile = $self-> SUPER::init(@_);
	$self-> {paneSize} = [0,0];
	$self-> {colorMap} = [];
	$self-> {fontPalette} = [];
	$self-> {blocks} = [];
	$self-> {resolution} = [];
	$self-> {defaultFontSize} = $self-> font-> size;
	$self-> {selection}   = [ -1, -1, -1, -1];
	$self-> {selectionPaintMode} = 0;
	$self-> {ymap} = [];
	$self-> setup_indents;
	$self-> resolution( @{$profile{resolution}});
	for ( qw( autoHScroll autoVScroll colorMap fontPalette 
				hScroll vScroll borderWidth paneWidth paneHeight 
				offset topLine textRef))
		{ $self-> $_( $profile{ $_}); }
	return %profile;
}

sub reset_scrolls
{
	my $self = shift;
	my @sz = $self-> get_active_area( 2, @_);
	if ( $self-> {scrollTransaction} != 1) {
		if ( $self-> {autoVScroll}) {
			my $vs = ($self-> {paneHeight} > $sz[1]) ? 1 : 0;
			if ( $vs != $self-> {vScroll}) {
				$self-> vScroll( $vs);
				@sz = $self-> get_active_area( 2, @_);
			}
		}
		$self-> {vScrollBar}-> set(
			max      => $self-> {paneHeight} - $sz[1],
			pageStep => int($sz[1] * 0.9),
			step     => $self-> font-> height,
			whole    => $self-> {paneHeight},
			partial  => $sz[1],
			value    => $self-> {topLine},
		) if $self-> {vScroll};
	}
	if ( $self-> {scrollTransaction} != 2) {
		if ( $self-> {autoHScroll}) {
			my $hs = ($self-> {paneWidth} > $sz[0]) ? 1 : 0;
			if ( $hs != $self-> {hScroll}) {
				$self-> hScroll( $hs); 
				@sz = $self-> get_active_area( 2, @_);
			}
		}
		$self-> {hScrollBar}-> set(
			max      => $self-> {paneWidth} - $sz[0],
			whole    => $self-> {paneWidth},
			value    => $self-> {offset},
			partial  => $sz[0],
			pageStep => int($sz[0] * 0.75),
		) if $self-> {hScroll};
	}
}

sub on_size
{
	my ( $self, $oldx, $oldy, $x, $y) = @_;
	$self-> reset_scrolls( $x, $y);
}

sub on_fontchanged
{
	my $f = $_[0]-> font;
	$_[0]-> {defaultFontSize}           = $f-> size;
	$_[0]-> {fontPalette}-> [0]-> {name} = $f-> name;
}

sub set
{
	my ( $self, %set) = @_;
	if ( exists $set{paneSize}) {
		$self-> paneSize( @{$set{paneSize}});
		delete $set{paneSize};
	}
	$self-> SUPER::set( %set);
}

sub text
{
	unless ($#_) {
		my $hugeScalarRef = $_[0]-> textRef;
		return $$hugeScalarRef;
	} else {
		my $s = $_[1];
		$_[0]-> textRef( \$s);
	}
}

sub textRef 
{
	return $_[0]-> {text} unless $#_;
	$_[0]-> {text} = $_[1] if $_[1];
}

sub paneWidth
{
	return $_[0]-> {paneWidth} unless $#_;
	my ( $self, $pw) = @_;
	$pw = 0 if $pw < 0;
	return if $pw == $self-> {paneWidth};
	$self-> {paneWidth} = $pw;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub paneHeight
{
	return $_[0]-> {paneHeight} unless $#_;
	my ( $self, $ph) = @_;
	$ph = 0 if $ph < 0;
	return if $ph == $self-> {paneHeight};
	$self-> {paneHeight} = $ph;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub paneSize
{
	return $_[0]-> {paneWidth}, $_[0]-> {paneHeight} if $#_ < 2;
	my ( $self, $pw, $ph) = @_;
	$ph = 0 if $ph < 0;
	$pw = 0 if $pw < 0;
	return if $ph == $self-> {paneHeight} && $pw == $self-> {paneWidth};
	$self-> {paneWidth}  = $pw;
	$self-> {paneHeight} = $ph;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub offset
{
	return $_[0]-> {offset} unless $#_;
	my ( $self, $offset) = @_;
	$offset = int($offset);
	my @sz = $self-> size;
	my @aa = $self-> get_active_area(2, @sz);
	my $pw = $self-> {paneWidth};
	$offset = $pw - $aa[0] if $offset > $pw - $aa[0];
	$offset = 0 if $offset < 0;
	return if $self-> {offset} == $offset;
	my $dt = $offset - $self-> {offset};
	$self-> {offset} = $offset;
	if ( $self-> {hScroll} && $self-> {scrollTransaction} != 2) {
		$self-> {scrollTransaction} = 2;
		$self-> {hScrollBar}-> value( $offset);
		$self-> {scrollTransaction} = 0;
	}
	$self-> scroll( -$dt, 0, clipRect => [ $self-> get_active_area(0, @sz)]);
}

sub resolution
{
	return @{$_[0]->{resolution}} unless $#_;
	my ( $self, $x, $y) = @_;
	die "Invalid resolution\n" if $x <= 0 or $y <= 0;
	@{$self-> {resolution}} = ( $x, $y);
}

sub topLine
{
	return $_[0]-> {topLine} unless $#_;
	my ( $self, $top) = @_;
	$top = int($top);
	my @sz = $self-> size;
	my @aa = $self-> get_active_area(2, @sz);
	my $ph = $self-> {paneHeight};
	$top = $ph - $aa[1] if $top > $ph - $aa[1];
	$top = 0 if $top < 0;
	return if $self-> {topLine} == $top;
	my $dt = $top - $self-> {topLine};
	$self-> {topLine} = $top;
	if ( $self-> {vScroll} && $self-> {scrollTransaction} != 1) {
		$self-> {scrollTransaction} = 1;
		$self-> {vScrollBar}-> value( $top);
		$self-> {scrollTransaction} = 0;
	}
	$self-> scroll( 0, $dt, clipRect => [ $self-> get_active_area(0, @sz)]);
}

sub VScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction};
	$self-> {scrollTransaction} = 1;
	$self-> topLine( $scr-> value);
	$self-> {scrollTransaction} = 0;
}

sub HScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction};
	$self-> {scrollTransaction} = 2;
	$self-> offset( $scr-> value);
	$self-> {scrollTransaction} = 0;
}

sub colorMap
{
	return [ @{$_[0]-> {colorMap}}] unless $#_;
	my ( $self, $cm) = @_;
	$self-> {colorMap} = [@$cm];
	$self-> {colorMap}-> [1] = $self-> backColor if scalar @$cm < 2;
	$self-> {colorMap}-> [0] = $self-> color if scalar @$cm < 1;
	$self-> repaint;
}

sub fontPalette
{
	return [ @{$_[0]-> {fontPalette}}] unless $#_;
	my ( $self, $fm) = @_;
	$self-> {fontPalette} = [@$fm];
	$self-> {fontPalette}-> [0] = {
		name     => $self-> font-> name,
		encoding => '',
		pitch    => fp::Default,
	} if scalar @$fm < 1;
	$self-> repaint;
}

sub create_state
{
	my $self = $_[0];
	my $g = tb::block_create();
	$$g[ tb::BLK_FONT_SIZE] = $self-> {defaultFontSize};
	$$g[ tb::BLK_COLOR]     = tb::COLOR_INDEX;
	$$g[ tb::BLK_BACKCOLOR] = tb::BACKCOLOR_DEFAULT;
	return $g;
}

sub realize_state
{
	my ( $self, $canvas, $state, $mode) = @_;

	if ( $mode & tb::REALIZE_FONTS) {
		my %f = %{$self-> {fontPalette}-> [ $$state[ tb::BLK_FONT_ID]]};
		if ( $$state[ tb::BLK_FONT_SIZE] > tb::F_HEIGHT) {
			$f{height} = $$state[ tb::BLK_FONT_SIZE] - tb::F_HEIGHT;
		} else {
			$f{size} = $$state[ tb::BLK_FONT_SIZE];
		}
		$f{style} = $$state[ tb::BLK_FONT_STYLE];
		$canvas-> set_font( \%f);
	}

	return unless $mode & tb::REALIZE_COLORS;
	if ( $self-> {selectionPaintMode}) {
		$self-> selection_state( $canvas);
	} else {
		$canvas-> set(
			color     =>  (( $$state[ tb::BLK_COLOR] & tb::COLOR_INDEX) ? 
				( $self-> {colorMap}-> [$$state[ tb::BLK_COLOR] & tb::COLOR_MASK]) :
				( $$state[ tb::BLK_COLOR] & tb::COLOR_MASK)),
			backColor =>  (( $$state[ tb::BLK_BACKCOLOR] & tb::COLOR_INDEX) ? 
				( $self-> {colorMap}-> [$$state[ tb::BLK_BACKCOLOR] & tb::COLOR_MASK]) : 
				( $$state[ tb::BLK_BACKCOLOR] & tb::COLOR_MASK)),
			textOpaque => (( $$state[ tb::BLK_BACKCOLOR] == tb::BACKCOLOR_DEFAULT) ? 0 : 1),
		);
	}
}


sub recalc_ymap
{
	my ( $self, $from) = @_;
	# if $from is zero or not defined, clear the ymap; otherwise we append
	# to what was already calculated. This is optimized for *building* a
	# collection of blocks; if you need to change a collection of blocks,
	# you should always set $from to a false value.
	$self-> {ymap} = [] unless $from; # ok if $from == 0
	my $ymap = $self-> {ymap};
	my $blocks = $self-> {blocks};
	my ( $i, $lim) = ( defined($from) ? $from : 0, scalar(@{$blocks}));
	for ( ; $i < $lim; $i++) {
		my $block = $$blocks[$i];
		my $y1 = $block->[ tb::BLK_Y];
		my $y2 = $block->[ tb::BLK_HEIGHT] + $y1;
		for my $y ( int( $y1 / tb::YMAX) .. int ( $y2 / tb::YMAX)) {
			push @{$ymap-> [$y]}, $i;
		}
	}
}

sub block_wrap
{
	my ( $self, $canvas, $b, $state, $width) = @_;
	$width = 0 if $width < 0;
	my ( $i, $lim) = ( tb::BLK_START, scalar @$b);

	my $cmd;
	my ( $o, $t) = ( $$b[ tb::BLK_TEXT_OFFSET], $self-> {text});
	my ( $x, $y) = (0, 0);
	my $f_taint;
	my $wrapmode = 1;
	my $stsave = $state;
	$state = [ @$state ];
	my ( $haswrapinfo, @wrapret);
	my ( @ret, $z);
	my $lastTextOffset = $$b[ tb::BLK_TEXT_OFFSET];
	my $has_text;

	my $newblock = sub 
	{
		push @ret, $z = tb::block_create();
		@$z[ tb::BLK_DATA_START .. tb::BLK_DATA_END ] = 
			@$state[ tb::BLK_DATA_START .. tb::BLK_DATA_END];
		$$z[ tb::BLK_X] = $$b[ tb::BLK_X];
		$$z[ tb::BLK_FLAGS] &= ~ tb::T_SIZE;
		$$z[ tb::BLK_TEXT_OFFSET] = $$b [ tb::BLK_TEXT_OFFSET];
		$x = 0;
		undef $has_text;
	};

	my $retrace = sub 
	{
		$haswrapinfo = 0;
		splice( @{$ret[-1]}, $wrapret[0]); 
		@$state = @{$wrapret[1]};
		$newblock-> ();
		$i = $wrapret[2];
	};

	$newblock-> ();
	$$z[tb::BLK_TEXT_OFFSET] = $$b[tb::BLK_TEXT_OFFSET];

	my %state_hash;

# print "start - $$b[tb::BLK_TEXT_OFFSET] \n";

	# first state - wrap the block
# print "new wrap for $width\n";
	for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]]) {
		$cmd = $$b[$i];
		if ( $cmd == tb::OP_TEXT) {
# print "OP_TEXT @$b[$i+1..$i+3], x = $x\n";
			unless ( $f_taint) {
				$self-> realize_state( $canvas, $state, tb::REALIZE_FONTS);
				$f_taint = $canvas-> get_font;
				my $state_key = join('.', 
					@$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]
				);
				$state_hash{$state_key} = $f_taint 
					unless $state_hash{$state_key};
			}
			my $ofs  = $$b[ $i + tb::T_OFS];
			my $tlen = $$b[ $i + tb::T_LEN];
			$lastTextOffset = $ofs + $tlen unless $wrapmode;
		REWRAP: 
			my $tw = $canvas-> get_text_width( substr( $$t, $o + $ofs, $tlen), 1);
			my $apx = $f_taint-> {width};
# print "$x+$apx: new text $tw :|",substr( $$t, $o + $ofs, $tlen),"|\n";
			if ( $x + $tw + $apx <= $width) {
				push @$z, tb::OP_TEXT, $ofs, $tlen, $tw;
				$x += $tw;
				$has_text = 1;
# print "copied as is, advanced to $x, width $tw, $ofs\n";
			} elsif ( $wrapmode) {
				next if $tlen <= 0;
				my $str = substr( $$t, $o + $ofs, $tlen);
				my $leadingSpaces = '';
				if ( $str =~ /^(\s+)/) {
					$leadingSpaces = $1;
					$str =~ s/^\s+//;
				}
				my $l = $canvas-> text_wrap( $str, $width - $apx - $x,
					tw::ReturnFirstLineLength | tw::WordBreak | tw::BreakSingle);
# print "repo $l bytes wrapped in $width - $apx - $x\n";
				if ( $l > 0) {
					if ( $has_text) {
						push @$z, tb::OP_TEXT, 
							$ofs, $l + length $leadingSpaces, 
							$tw = $canvas-> get_text_width( 
								$leadingSpaces . substr( $str, 0, $l), 1
							);
					} else {
						push @$z, tb::OP_TEXT, 
							$ofs + length $leadingSpaces, $l, 
							$tw = $canvas-> get_text_width( 
								substr( $str, 0, $l), 1
							);
						$has_text = 1;
					}
# print "$x + advance $$z[-1]/$tw|", $leadingSpaces , "+", substr( $str, 0, $l), "|\n";
					$str = substr( $str, $l);
					$l += length $leadingSpaces;
					$newblock-> ();
					$ofs += $l;
					$tlen -= $l;
# print "tx shift $l, str=|$str|, x=$x\n";
					if ( $str =~ /^(\s+)/) {
						$ofs  += length $1;
						$tlen -= length $1;
						$x    += $canvas-> get_text_width( $1, 1);
						$str =~ s/^\s+//;
					}
					goto REWRAP if length $str;
				} else { # does not fit into $width
# print "new block: x = $x |$str|\n";
					my $ox = $x;
					$newblock-> ();
					$ofs  += length $leadingSpaces;
					$tlen -= length $leadingSpaces; 
					if ( length $str) { 
					# well, it cannot be fit into width,
					# but may be some words can be stripped?
						goto REWRAP if $ox > 0;
						if ( $str =~ m/^(\S+)(\s*)/) {
							$tw = $canvas-> get_text_width( $1, 1);
							push @$z, tb::OP_TEXT, $ofs, length $1, $tw;
							$has_text = 1;
							$x += $tw;
							$ofs  += length($1) + length($2);
							$tlen -= length($1) + length($2);
							goto REWRAP;
						}
					}
					push @$z, tb::OP_TEXT, $ofs, length($str), $x += $canvas-> get_text_width( $str, 1);
					$has_text = 1;
				}
			} elsif ( $haswrapinfo) { # unwrappable, and cannot be fit - retrace
				$retrace-> ();
# print "retrace\n";
				next;
			} else { # unwrappable, cannot be fit, no wrap info! - whole new block
# print "new empty block - |", substr( $$t,$o + $ofs, $tlen), "|\n";
				push @$z, tb::OP_TEXT, $ofs, $tlen, $tw;
				$newblock-> ();
			}
		} elsif ( $cmd == tb::OP_WRAP) {
			if ( $wrapmode == 1 && $$b[ $i + 1] == 0) {
				@wrapret = ( scalar @$z, [ @$state ], $i);
				$haswrapinfo = 1;
# print "wrap start record x = $x\n";
			}
			$wrapmode = $$b[ $i + 1];
# print "wrap: $wrapmode\n";
		} elsif ( $cmd == tb::OP_FONT) {
			if ( $$b[$i + tb::F_MODE] == tb::F_SIZE && $$b[$i + tb::F_DATA] < tb::F_HEIGHT ) {
				$$state[ $$b[$i + tb::F_MODE]] = $self-> {defaultFontSize} + $$b[$i + tb::F_DATA];
			} else {
				$$state[ $$b[$i + tb::F_MODE]] = $$b[$i + tb::F_DATA];
			}
			$f_taint = undef;
			push @$z, @$b[ $i .. ( $i + $tb::oplen[ $cmd] - 1)];
		} elsif ( $cmd == tb::OP_COLOR) {
			$$state[ tb::BLK_COLOR + (($$b[ $i + 1] & tb::BACKCOLOR_FLAG) ? 1 : 0)] = 
				$$b[$i + 1];
			push @$z, @$b[ $i .. ( $i + $tb::oplen[ $cmd] - 1)];
		} elsif ( $cmd == tb::OP_TRANSPOSE) {
			my @r = @$b[ $i .. $i + tb::X_FLAGS];
			if ( $$b[ $i + tb::X_FLAGS] & tb::X_DIMENSION_FONT_HEIGHT) {
				unless ( $f_taint) {
					$self-> realize_state( $canvas, $state, tb::REALIZE_FONTS); 
					$f_taint = $canvas-> get_font;
					my $state_key = join('.', 
						@$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]
					);
					$state_hash{$state_key} = $f_taint 
						unless $state_hash{$state_key};
				}
				$r[ tb::X_X] *= $f_taint-> {height};
				$r[ tb::X_Y] *= $f_taint-> {height};
				$r[ tb::X_FLAGS] &= ~ tb::X_DIMENSION_FONT_HEIGHT;
			}
			if ( $$b[ $i + tb::X_FLAGS] & tb::X_DIMENSION_POINT) {
				$r[ tb::X_X] *= $self-> {resolution}-> [0] / 72;
				$r[ tb::X_Y] *= $self-> {resolution}-> [1] / 72;
				$r[ tb::X_FLAGS] &= ~tb::X_DIMENSION_POINT;
			}
# print "advance block $x $r[tb::X_X]\n";
			if ( $x + $r[tb::X_X] >= $width) {
				if ( $wrapmode) {
					$newblock-> (); 
				} elsif ( $haswrapinfo) {
					$retrace-> ();
					next;
				} 
			} else {
				$x += $r[ tb::X_X];
			}
			push @$z, @r;
		} else {
			push @$z, @$b[ $i .. ( $i + $tb::oplen[ $cmd] - 1)];
		}
	}

	# remove eventual empty trailing blocks
	pop @ret while scalar ( @ret) && ( tb::BLK_START == scalar @{$ret[-1]});

	# second stage - position the blocks
	$state = $stsave;
	$f_taint = undef;
	my $start;
	if ( !defined $$b[ tb::BLK_Y]) { 
		# auto position the block if the creator didn't care
		$start = $$state[ tb::BLK_Y] + $$state[ tb::BLK_HEIGHT];
	} else {
		$start = $$b[ tb::BLK_Y];
	}

	$lastTextOffset = $$b[ tb::BLK_TEXT_OFFSET];
	my $lastBlockOffset = $lastTextOffset;

	for ( @ret) {
		$b = $_;
		$$b[ tb::BLK_Y] = $start;

		( $x, $y, $i, $lim) = ( 0, 0, tb::BLK_START, scalar @$b);
		for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]]) {
			$cmd = $$b[$i];
			if ( $cmd == tb::OP_TEXT) {
				$f_taint = $state_hash{ join('.', 
					@$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]
				) };
				$x += $$b[ $i + tb::T_WID];
				$$b[ tb::BLK_WIDTH] = $x 
					if $$b[ tb::BLK_WIDTH ] < $x;
				$$b[ tb::BLK_APERTURE_Y] = $f_taint-> {descent} - $y 
					if $$b[ tb::BLK_APERTURE_Y] < $f_taint-> {descent} - $y;
				$$b[ tb::BLK_APERTURE_X] = $f_taint-> {width}   - $x 
					if $$b[ tb::BLK_APERTURE_X] < $f_taint-> {width}   - $x;
				my $newY = $y + $f_taint-> {ascent} + $f_taint-> {externalLeading};
				$$b[ tb::BLK_HEIGHT] = $newY if $$b[ tb::BLK_HEIGHT] < $newY; 
#            print "OP_TEXT patch $$b[$i+1] => ";
				$lastTextOffset = 
					$$b[ tb::BLK_TEXT_OFFSET] + 
					$$b[ $i + tb::T_OFS] + 
					$$b[ $i + tb::T_LEN];
				$$b[ $i + tb::T_OFS] -= $lastBlockOffset - $$b[ tb::BLK_TEXT_OFFSET];
#            print "$$b[$i+1]\n";
			} elsif ( $cmd == tb::OP_FONT) {
				if ( $$b[$i + tb::F_MODE] == tb::F_SIZE && $$b[$i + tb::F_DATA] < tb::F_HEIGHT ) {
					$$state[ $$b[$i + tb::F_MODE]] = $self-> {defaultFontSize} + $$b[$i + tb::F_DATA];
				} else {
					$$state[ $$b[$i + tb::F_MODE]] = $$b[$i + tb::F_DATA];
				}
			} elsif ( $cmd == tb::OP_TRANSPOSE) {
				my ( $newX, $newY) = ( $x + $$b[ $i + tb::X_X], $y + $$b[ $i + tb::X_Y]);
				$$b[ tb::BLK_WIDTH]  = $newX 
					if $$b[ tb::BLK_WIDTH ] < $newX;
				$$b[ tb::BLK_HEIGHT] = $newY 
					if $$b[ tb::BLK_HEIGHT] < $newY;
				$$b[ tb::BLK_APERTURE_X] = -$newX 
					if $newX < 0 && $$b[ tb::BLK_APERTURE_X] > -$newX;
				$$b[ tb::BLK_APERTURE_Y] = -$newY 
					if $newY < 0 && $$b[ tb::BLK_APERTURE_Y] > -$newY;
				unless ( $$b[ $i + tb::X_FLAGS] & tb::X_EXTEND) {
					( $x, $y) = ( $newX, $newY);
				}
			} elsif ( $cmd == tb::OP_MARK) {
				$$b[ $i + 2] = $x;
				$$b[ $i + 3] = $y;
			}
		}
		$$b[ tb::BLK_TEXT_OFFSET] = $lastBlockOffset;
#      print "block offset: $lastBlockOffset\n";
		$$b[ tb::BLK_HEIGHT] += $$b[ tb::BLK_APERTURE_Y];
		$$b[ tb::BLK_WIDTH]  += $$b[ tb::BLK_APERTURE_X];
		$start += $$b[ tb::BLK_HEIGHT];
		$lastBlockOffset = $lastTextOffset;
	}

	if ( $ret[-1]) {
		$b = $ret[-1];
		$$state[$_] = $$b[$_] for tb::BLK_X, tb::BLK_Y, tb::BLK_HEIGHT, tb::BLK_WIDTH;
	}

	return @ret;
}

sub selection_state
{
	my ( $self, $canvas) = @_;
	$canvas-> color( $self-> hiliteColor);
	$canvas-> backColor( $self-> hiliteBackColor);
	$canvas-> textOpaque(0);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size = $canvas-> size;
	unless ( $self-> enabled) {
		$self-> color( $self-> disabledColor);
		$self-> backColor( $self-> disabledBackColor);
	}
	my ( $t, $offset, @aa) = (
	$self-> { topLine}, $self-> { offset},
	$self-> get_active_area(1,@size));

	my @clipRect = $canvas-> clipRect;
	$self-> draw_border( $canvas, $self-> backColor, @size);

	my $bx = $self-> {blocks};
	my $lim = scalar @$bx;
	return unless $lim;

	my @cy = ( $aa[3] - $clipRect[3], $aa[3] - $clipRect[1]);
	$cy[0] = 0 if $cy[0] < 0;
	$cy[1] = $aa[3] - $aa[1] if $cy[1] > $aa[3] - $aa[1];
	$cy[$_] += $t for 0,1;

	$self-> clipRect( $self-> get_active_area( 1, @size));
	@clipRect = $self-> clipRect; 
	my $i = 0;
	my $b;

	my ( $sx1, $sy1, $sx2, $sy2) = @{$self-> {selection}};

	for my $ymap_i ( int( $cy[0] / tb::YMAX) .. int( $cy[1] / tb::YMAX)) {
		next unless $self-> {ymap}-> [$ymap_i];
		for my $j ( @{$self-> {ymap}-> [$ymap_i]}) {
			$b = $$bx[$j];
			my ( $x, $y) = ( 
				$aa[0] - $offset + $$b[ tb::BLK_X], 
				$aa[3] + $t - $$b[ tb::BLK_Y] - $$b[ tb::BLK_HEIGHT] 
			);
			next if 
				$x + $$b[ tb::BLK_WIDTH]  < $clipRect[0] || $x > $clipRect[2] ||
				$y + $$b[ tb::BLK_HEIGHT] < $clipRect[1] || $y > $clipRect[3] ||
				$$b[ tb::BLK_WIDTH] == 0 || $$b[ tb::BLK_HEIGHT] == 0;
					

			if ( $j == $sy1 || $j == $sy2) { # complex selection case
				my @cr = @clipRect;
				my $x1 = $x + $self-> text2xoffset(( $j == $sy1) ? $sx1 : $sx2, $j);
				my $eq = ( $j == $sy1 ) && ( $j == $sy2 );
				$self-> {selectionPaintMode} = ( $eq || $j == $sy1 ) ? 0 : 1;
				if ( $cr[0] <= $x1 ) { # left upper part
				$cr[2] = $x1 - 1 if $cr[2] > $x1 - 1;
				$cr[2] = $aa[2] if $cr[2] > $aa[2];
				$cr[2] = $aa[0] if $cr[2] < $aa[0];
				if ( $cr[0] <= $cr[2]) {
					$self-> selection_state( $canvas) 
						if $self-> {selectionPaintMode}; 
					$self-> clipRect( @cr);
					$self-> block_draw( $canvas, $b, $x, $y);
				}
					@cr = @clipRect;
				}
				$self-> {selectionPaintMode} = (( $eq || $j == $sy1 ) ? 1 : 0); 
				if ( $cr[2] >= $x1) { # right part
					$cr[0] = $x1 if $cr[0] < $x1;
					$cr[0] = $aa[0] if $cr[0] < $aa[0];
					$cr[0] = $aa[2] if $cr[0] > $aa[2];
					my $x2 = $x + $self-> text2xoffset( $sx2, $j);
					if ( $eq) { # selection is one block - center part
						if ( $cr[0] <= $x2) {
							my $cr2 = $cr[2];
							$cr[2] = $x2 - 1 if $cr[2] > $x2 - 1;
							$cr[2] = $aa[0] if $cr[2] < $aa[0];
							$cr[2] = $aa[2] if $cr[2] > $aa[2];
							if ( $cr[0] <= $cr[2]) {
								$self-> selection_state( $canvas) 
									if $self-> {selectionPaintMode}; 
								$self-> clipRect( @cr);
								$self-> block_draw( $canvas, $b, $x, $y);
							}
							@cr = @clipRect;
						}
						$cr[0] = $x2 if $cr[0] < $x2;
						$cr[0] = $aa[0] if $cr[0] < $aa[0];
						$cr[0] = $aa[2] if $cr[0] > $aa[2];
					}
					$self-> {selectionPaintMode} = ( $eq || $j == $sy2 ) ? 0 : 1;
					if ( $cr[0] <= $cr[2]) {
						$self-> selection_state( $canvas) if $self-> {selectionPaintMode}; 
						$self-> clipRect( @cr);
						$self-> block_draw( $canvas, $b, $x, $y);
					}
				}
				$self-> {selectionPaintMode} = 0;
				$self-> clipRect( @clipRect);
			} elsif ( $j > $sy1 && $j < $sy2) { # simple selection case
				$self-> {selectionPaintMode} = 1;
				$self-> selection_state( $canvas);
				$self-> block_draw( $canvas, $b, $x, $y);
				$self-> {selectionPaintMode} = 0;
			} else { # no selection case
				$self-> block_draw( $canvas, $b, $x, $y);
			}
		}
	}

	$self-> {selectionPaintMode} = 0;
}


sub block_draw
{
	my ( $self, $canvas, $b, $x, $y) = @_;
	my ( $i, $lim) = ( tb::BLK_START, scalar @$b);
	my $ret = 1;

	my $cmd;
	my ( $t, $o) = ( $self-> {text}, $$b[ tb::BLK_TEXT_OFFSET]);
	my @state = @$b[ 0 .. tb::BLK_DATA_END ];
	my ( $f_taint, $c_taint); 

	$canvas-> clear( $x, $y, $x + $$b[ tb::BLK_WIDTH] - 1, $y + $$b[ tb::BLK_HEIGHT] - 1)
	if $self-> {selectionPaintMode};

	$x += $$b[ tb::BLK_APERTURE_X];
	$y += $$b[ tb::BLK_APERTURE_Y];

	for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]] ) {
		$cmd = $$b[$i];
		if ( $cmd == tb::OP_TEXT) {
			if ( $$b[$i + tb::T_LEN] > 0) {
				unless ( $f_taint) {
					$self-> realize_state( $canvas, \@state, tb::REALIZE_FONTS); 
					$f_taint = $canvas-> get_font;
				}
				unless ( $c_taint) {
					$self-> realize_state( $canvas, \@state, tb::REALIZE_COLORS); 
					$c_taint = 1;
				}
				# Make sure we ultimately return "fail" if any text_out operation
				# in this block fails. XXX if there are multiple failures, $@
				# will only contain the last one. Consider consolidating
				# them somehow.
				$ret &&= $canvas-> text_out( substr( $$t, $o + $$b[$i + tb::T_OFS], $$b[$i + tb::T_LEN]), $x, $y);
			}
			$x += $$b[ $i + tb::T_WID];
		} elsif ( $cmd == tb::OP_FONT) {
			if ( $$b[$i + tb::F_MODE] == tb::F_SIZE && $$b[$i + tb::F_DATA] < tb::F_HEIGHT ) {
				$state[ $$b[$i + tb::F_MODE]] = $self-> {defaultFontSize} + $$b[$i + tb::F_DATA];
			} else {
				$state[ $$b[$i + tb::F_MODE]] = $$b[$i + tb::F_DATA];
			}
			$f_taint = undef;
		} elsif (( $cmd == tb::OP_TRANSPOSE) && !($$b[ $i + tb::X_FLAGS] & tb::X_EXTEND)) {
			$x += $$b[ $i + tb::X_X];
			$y += $$b[ $i + tb::X_Y];
		} elsif ( $cmd == tb::OP_CODE) {
			unless ( $f_taint) {
				$self-> realize_state( $canvas, \@state, tb::REALIZE_FONTS); 
				$f_taint = $canvas-> get_font;
			}
			unless ( $c_taint) {
				$self-> realize_state( $canvas, \@state, tb::REALIZE_COLORS); 
				$c_taint = 1;
			}
			$$b[ $i + 1]-> ( $self, $canvas, $b, \@state, $x, $y, $$b[ $i + 2]);
		} elsif ( $cmd == tb::OP_COLOR) {
			$state[ tb::BLK_COLOR + (($$b[ $i + 1] & tb::BACKCOLOR_FLAG) ? 1 : 0)] 
				= $$b[$i + 1];
			$c_taint = undef;
		} elsif ($cmd >= @tb::oplen) {
			die("Unknown Prima::TextView block op $cmd\n");
		}
	}

	return $ret;
}

sub xy2info
{
	my ( $self, $x, $y) = @_;

	my $bx = $self-> {blocks};
	my ( $pw, $ph) = $self-> paneSize;
	$x = 0 if $x < 0;
	$x = $pw if $x > $pw;
	return (0,0) if $y < 0 || !scalar(@$bx) ;
	$x = $pw, $y = $ph if $y > $ph;
	
	my ( $b, $bid);

	my $xhint = 0;

	# find if there's a block that has $y in its inferior
	my $ymapix = int( $y / tb::YMAX);
	if ( $self-> {ymap}-> [ $ymapix]) {
		my ( $minxdist, $bdist, $bdistid) = ( $self-> {paneWidth} * 2, undef, undef);
		for ( @{$self-> {ymap}-> [ $ymapix]}) {
			my $z = $$bx[$_];
			if ( $y >= $$z[ tb::BLK_Y] && $y < $$z[ tb::BLK_Y] + $$z[ tb::BLK_HEIGHT]) {
				if ( 
					$x >= $$z[ tb::BLK_X] && 
					$x < $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH]
				) {
					$b = $z;
					$bid = $_;
					last;
				} elsif ( 
					abs($$z[ tb::BLK_X] - $x) < $minxdist || 
					abs($$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH] - $x) < $minxdist
				) {
					$minxdist = ( abs( $$z[ tb::BLK_X] - $x) < $minxdist) ? 
						abs( $$z[ tb::BLK_X] - $x) :
						abs( $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH] - $x);
					$bdist = $z;
					$bdistid = $_;
				}
			}
		}
		if ( !$b && $bdist) {
			$b   = $bdist;
			$bid = $bdistid;
			$xhint = (( $$b[ tb::BLK_X] > $x) ? -1 : 1);
		}
	}

	# if still no block found, find the closest block down 
	unless ( $b) {
		my $minydist = $self-> {paneHeight} * 2;
		my $ymax = scalar @{$self-> {ymap}};
		while ( $ymapix < $ymax) {
			if ( $self-> {ymap}-> [ $ymapix]) {
				for ( @{$self-> {ymap}-> [ $ymapix]}) {
					my $z = $$bx[$_];
					if ( 
						$minydist > $$z[ tb::BLK_Y] - $y && 
						$$z[ tb::BLK_Y] >= $y
					) {
						$minydist = $$z[ tb::BLK_Y] - $y;
						$b = $z;
						$bid = $_;
					}
				}
			}
			last if $b;
			$ymapix++;
		}
		$ymapix = int( $y / tb::YMAX);
		$xhint = -1;
	}

	# if still no block found, assume EOT
	unless ( $b) {
		$b = $$bx[-1];
		$bid = scalar @{$bx} - 1;
		$xhint = 1;
	}

	if ( $xhint < 0) { # start of line
		return ( 0, $bid);
	} elsif ( $xhint > 0) { # end of line
		if ( $bid < ( scalar @{$bx} - 1)) {
			return ( 
				$$bx[ $bid + 1]-> [ tb::BLK_TEXT_OFFSET] - $$b[ tb::BLK_TEXT_OFFSET], 
				$bid
			);
		} else {
			return ( length( ${$self-> {text}}) - $$b[ tb::BLK_TEXT_OFFSET], $bid);
		}
	}

	# find text offset
	my $bofs = $$b[ tb::BLK_TEXT_OFFSET];
	my ( $ofs, $unofs) = (0,0);
	my $pm = $self-> get_paint_state;
	$self-> begin_paint_info unless $pm;
	my $savefont  = $self-> get_font;
	my @state = @$b[ 0 .. tb::BLK_DATA_END ];
	my $f_taint;

	my ( $i, $lim) = ( tb::BLK_START, scalar @$b);
	my $px = $$b[ tb::BLK_X];
	for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]] ) {
		my $cmd = $$b[$i];
		if ( $cmd == tb::OP_TEXT) {
			my $npx = $px + $$b[$i + tb::T_WID];
			if ( $px > $x) {
				$ofs = $$b[ $i + tb::T_OFS]; 
				undef $unofs;
				last;
			} elsif ( $px <= $x && $npx > $x) {
				unless ( $f_taint) {
					$self-> realize_state( $self, \@state, tb::REALIZE_FONTS); 
					$f_taint = $self-> get_font;
				}
				$ofs = $$b[ $i + tb::T_OFS] + $self-> text_wrap( 
					substr( 
						${$self-> {text}}, $bofs + $$b[ $i + tb::T_OFS], $$b[ $i + tb::T_LEN]
					),
					$x - $px, 
					tw::ReturnFirstLineLength | tw::BreakSingle
				);
				undef $unofs;
				last;
			} 
			$unofs = $$b[ $i + tb::T_OFS] + $$b[ $i + tb::T_LEN];
			$px = $npx;
		} elsif (( $cmd == tb::OP_TRANSPOSE) && !($$b[ $i + tb::X_FLAGS] & tb::X_EXTEND)) {
			$px += $$b[ $i + tb::X_X];
		} elsif ( $cmd == tb::OP_FONT) {
			if ( $$b[$i + tb::F_MODE] == tb::F_SIZE && $$b[$i + tb::F_DATA] < tb::F_HEIGHT ) {
				$state[ $$b[$i + tb::F_MODE]] = $self-> {defaultFontSize} + $$b[$i + tb::F_DATA];
			} else {
				$state[ $$b[$i + tb::F_MODE]] = $$b[$i + tb::F_DATA];
			}
			$f_taint = undef;
		}
	}

	$pm ? 
		$self-> set_font( $savefont) : 
		$self-> end_paint_info;

	return defined( $unofs) ? $unofs : $ofs, $bid;
}

sub screen2point
{
	my ( $self, $x, $y, @size) = @_;

	@size = $self-> size unless @size;
	my @aa = $self-> get_active_area( 0, @size);

	$x -= $aa[0];
	$y  = $aa[3] - $y;
	$y += $self-> {topLine};
	$x += $self-> {offset};

	return $x, $y;
}

sub text2xoffset
{
	my ( $self, $x, $bid) = @_;

	my $b = $self-> {blocks}-> [$bid];
	return 0 unless $b;
	return 0 if $x <= 0; # XXX

	my $pm = $self-> get_paint_state;
	$self-> begin_paint_info unless $pm;
	my $savefont = $self-> get_font;
	my @state = @$b[ 0 .. tb::BLK_DATA_END ];
	my $f_taint;

	my ( $i, $lim) = ( tb::BLK_START, scalar @$b);
	my $px   = $$b[tb::BLK_APERTURE_X];
	my $bofs = $$b[tb::BLK_TEXT_OFFSET];
	for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]] ) {
		my $cmd = $$b[$i];
		if ( $cmd == tb::OP_TEXT) {
			if ( $x >= $$b[$i + tb::T_OFS]) {
				if ( $x < $$b[$i + tb::T_OFS] + $$b[$i + tb::T_LEN]) {
					unless ( $f_taint) {
						$self-> realize_state( 
							$self, \@state, 
							tb::REALIZE_FONTS
						);
						$f_taint = $self-> get_font;
					}
					$px += $self-> get_text_width( 
						substr( 
							${$self-> {text}}, 
							$bofs + $$b[$i+tb::T_OFS], 
							$x - $$b[$i+tb::T_OFS] 
						)
					);
					last;
				} elsif ( $x == $$b[$i+tb::T_OFS] + $$b[$i+tb::T_LEN]) {
					$px += $$b[$i+tb::T_WID];
					last;
				}
			}
			$px += $$b[$i+tb::T_WID];
		} elsif (( $cmd == tb::OP_TRANSPOSE) && !($$b[ $i + tb::X_FLAGS] & tb::X_EXTEND)) {
			$px += $$b[ $i + tb::X_X];
		} elsif ( $cmd == tb::OP_FONT) {
			if ( $$b[$i + tb::F_MODE] == tb::F_SIZE && $$b[$i + tb::F_DATA] < tb::F_HEIGHT ) {
				$state[ $$b[$i + tb::F_MODE]] = $self-> {defaultFontSize} + $$b[$i + tb::F_DATA];
			} else {
				$state[ $$b[$i + tb::F_MODE]] = $$b[$i + tb::F_DATA];
			}
			$f_taint = undef;
		}
	}
	$pm ? $self-> set_font( $savefont) : $self-> end_paint_info;
	return $px;
}

sub info2text_offset
{
	my ( $self, $ofs, $blk) = @_;
	if ( $blk >= 0 && $ofs >= 0) {
		return $self-> {blocks}-> [$blk]-> [tb::BLK_TEXT_OFFSET] + $ofs;
	} else {
		return length ${$self-> {text}};
	}
}

sub text_offset2info
{
	my ( $self, $ofs) = @_;
	my $blk = $self-> text_offset2block( $ofs);
	return undef unless defined $blk;
	return $ofs - $self-> {blocks}-> [$blk]-> [ tb::BLK_TEXT_OFFSET], $blk;
}

sub info2xy
{
	my ( $self, $ofs, $blk) = @_;
	$blk = $self-> {blocks}-> [$blk];
	return undef unless defined $blk;
	return @$blk[ tb::BLK_X, tb::BLK_Y];
}

sub text_offset2block
{
	my ( $self, $ofs) = @_;
	
	my $bx = $self-> {blocks};
	my $end = length ${$self-> {text}};
	my $ret = 0;
	return undef if $ofs < 0 || $ofs >= $end;

	my ( $l, $r) = ( 0, scalar @$bx);
	while ( 1) {
		my $i = int(( $l + $r) / 2);
		last if $i == $ret;
		$ret = $i;
		my ( $b1, $b2) = ( $$bx[$i], $$bx[$i+1]);

		last if $ofs == $$b1[ tb::BLK_TEXT_OFFSET];

		if ( $ofs > $$b1[ tb::BLK_TEXT_OFFSET]) { 
			if ( $b2) {
				last if $ofs < $$b2[ tb::BLK_TEXT_OFFSET];
				$l = $i;
			} else {
				last;
			}
		} else {
		$r = $i;
		}
	}
	return $ret;
}


sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction};

	my @size = $self-> size;
	my @aa = $self-> get_active_area( 0, @size);
	return if $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3];

	( $x, $y) = $self-> screen2point( $x, $y, @size);

	for my $obj ( @{$self-> {contents}}) {
		unless ( $obj-> on_mousedown( $self, $btn, $mod, $x, $y)) {
			$self-> clear_event;
			return;
		}
	}

	return if $btn != mb::Left;
	
	my ( $text_offset, $bid) = $self-> xy2info( $x, $y);

	$self-> {mouseTransaction} = 1;
	$self-> {mouseAnchor} = [ $text_offset, $bid ]; 
	$self-> selection( -1, -1, -1, -1);

	$self-> capture(1);
	$self-> clear_event;
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	return unless $dbl;
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;
	
	my @size = $self-> size;
	my @aa = $self-> get_active_area( 0, @size);
	if ( $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3]) {
		if ( $self-> has_selection) {
			$self-> selection( -1, -1, -1, -1);
			my $cp = $::application-> bring('Primary');
			$cp-> text( '') if $cp;
		}
		return;
	}

	( $x, $y) = $self-> screen2point( $x, $y, @size);
	my ( $text_offset, $bid) = $self-> xy2info( $x, $y);
	my $ln = ( $bid + 1 == scalar @{$self-> {blocks}}) ? 
		length ${$self-> {text}} : $self-> {blocks}-> [$bid+1]-> [tb::BLK_TEXT_OFFSET];
	$self-> selection( 0, $bid, $ln - $self-> {blocks}-> [$bid]-> [tb::BLK_TEXT_OFFSET], $bid);
	$self-> clear_event;
	
	my $cp = $::application-> bring('Primary');
	$cp-> text( $self-> get_selected_text) if $cp;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;

	unless ( $self-> {mouseTransaction}) {
		( $x, $y) = $self-> screen2point( $x, $y);
		for my $obj ( @{$self-> {contents}}) {
			unless ( $obj-> on_mouseup( $self, $btn, $mod, $x, $y)) {
				$self-> clear_event;
				return;
			}
		}
		return;
	}

	return if $btn != mb::Left;
	
	$self-> capture(0);
	$self-> {mouseTransaction} = undef;
	$self-> clear_event;

	my $cp = $::application-> bring('Primary');
	$cp-> text( $self-> get_selected_text) if $cp;
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;

	unless ( $self-> {mouseTransaction}) {
		( $x, $y) = $self-> screen2point( $x, $y);
		for my $obj ( @{$self-> {contents}}) {
			unless ( $obj-> on_mousemove( $self, $mod, $x, $y)) {
				$self-> clear_event;
				return;
			}
		}
		return;
	}


	my @size = $self-> size;
	my @aa = $self-> get_active_area( 0, @size);
	if ( $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3]) {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
	} else {
		$self-> scroll_timer_stop;
	}

	my ( $nx, $ny) = $self-> screen2point( $x, $y, @size);
	my ( $text_offset, $bid) = $self-> xy2info( $nx, $ny);

	$self-> selection( @{$self-> {mouseAnchor}}, $text_offset, $bid);

	if ( $x < $aa[0] || $x >= $aa[2]) {
		my $px = $self-> {paneWidth} / 8;
		$px = 5 if $px < 5;
		$px *= -1 if $x < $aa[0];
		$self-> offset( $self-> {offset} + $px);
	}
	if ( $y < $aa[1] || $y >= $aa[3]) {
		my $py = $self-> font-> height;
		$py = 5 if $py < 5;
		$py *= -1 if $y >= $aa[3];
		$self-> topLine( $self-> {topLine} + $py);
	}
}


sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$z = int( $z/120) * 3;
	$z *= $self-> font-> height + $self-> font-> externalLeading unless $mod & km::Ctrl;
	my $newTop = $self-> {topLine} - $z;
	$self-> topLine( $newTop > $self-> {paneHeight} ? $self-> {paneHeight} : $newTop);
	$self-> clear_event;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;

	$mod &= km::Alt|km::Ctrl|km::Shift;
	return if $mod & km::Alt;

	if ( grep { $key == $_ } ( 
		kb::Up, kb::Down, kb::Left, kb::Right, 
		kb::Space, kb::PgDn, kb::PgUp, kb::Home, kb::End
	)) {
		my ( $dx, $dy) = (0,0);
		if ( $key == kb::Up || $key == kb::Down) {
			$dy = $self-> font-> height;
			$dy = 5 if $dy < 5;
			$dy *= $repeat;
			$dy = -$dy if $key == kb::Up;
		} elsif ( $key == kb::Left || $key == kb::Right) {
			$dx = $self-> {paneWidth} / 8;
			$dx = 5 if $dx < 5;
			$dx *= $repeat;
			$dx = -$dx if $key == kb::Left;
		} elsif ( $key == kb::PgUp || $key == kb::PgDn || $key == kb::Space) {
			my @aa = $self-> get_active_area(0);
			$dy = ( $aa[3] - $aa[1]) * 0.9;
			$dy = 5 if $dy < 5;
			$dy *= $repeat;
			$dy = -$dy if $key == kb::PgUp;
		} 

		$dx += $self-> {offset};
		$dy += $self-> {topLine};
		
		if ( $key == kb::Home) {
			$dy = 0;
		} elsif ( $key == kb::End) {
			$dy = $self-> {paneHeight};
		}
		$self-> offset( $dx);
		$self-> topLine( $dy);
		$self-> clear_event; 
	}

	if (((( $key == kb::Insert) && ( $mod & km::Ctrl)) ||
		chr($code & 0xff) eq "\cC") && $self-> has_selection)
	{
		$self-> copy;
		$self-> clear_event;
	}
}

sub has_selection
{
	return ( grep { $_ != -1 } @{$_[0]-> {selection}} ) ? 1 : 0;
}

sub selection
{
	return @{$_[0]-> {selection}} unless $#_;
	my ( $self, $sx1, $sy1, $sx2, $sy2) = @_;

	$sy1 = 0 if $sy1 < 0;
	$sy2 = 0 if $sy2 < 0;
	my $lim = scalar @{$self-> {blocks}} - 1;
	$sy1 = $lim if $sy1 > $lim;
	$sy2 = $lim if $sy2 > $lim;

	my $empty = ! $self-> has_selection;
	my ( $osx1, $osy1, $osx2, $osy2) = @{$self-> {selection}};
	my ( $x1, $y1, $x2, $y2) = (0,0,0,0);
	
	unless ( grep { $_ != -1 } $sx1, $sy1, $sx2, $sy2 ) { # new empty selection
	EMPTY:
		return if $empty;     
		$y1 = $osy1;
		$y2 = $osy2;
		if ( $y1 == $y2) {
			$x1 = $osx1;
			$x2 = $osx2;
		}
	} else {
		( $sy1, $sy2, $sx1, $sx2) = ( $sy2, $sy1, $sx2, $sx1) if $sy2 < $sy1;
		( $sx1, $sx2) = ( $sx2, $sx1) if $sy2 == $sy1 && $sx2 < $sx1;
		( $sx1, $sx2, $sy1, $sy2) = ( -1, -1, -1, -1), goto EMPTY 
			if $sy1 == $sy2 && $sx1 == $sx2;
		if ( $empty) {
			$y1 = $sy1;
			$y2 = $sy2;
			if ( $y1 == $y2) {
				$x1 = $sx1;
				$x2 = $sx2;
			}
		} else {
			if ( $sy1 == $osy1 && $sx1 == $osx1) {
				return if $sy2 == $osy2 && $sx2 == $osx2;
				$y1 = $sy2;
				$y2 = $osy2;
				if ( $sy2 == $osy2) {
					$x1 = $sx2;
					$x2 = $osx2;
				}
			} elsif ( $sy2 == $osy2 && $sx2 == $osx2) {
				$y1 = $sy1;
				$y2 = $osy1;
				if ( $sy1 == $osy1) {
					$x1 = $sx1;
					$x2 = $osx1;
				}
			} else {
				$y1 = ( $sy1 < $osy1) ? $sy1 : $osy1;
				$y2 = ( $sy2 > $osy2) ? $sy2 : $osy2;
				if ( $sy1 == $sy2 && $osy1 == $osy2 && $sy2 == $osy1) {
					$x1 = ( $sx1 < $osx1) ? $sx1 : $osx1;
					$x2 = ( $sx2 > $osx2) ? $sx2 : $osx2;
				}
			}
			( $y1, $y2, $x1, $x2) = ( $y2, $y1, $x2, $x1) if $y2 < $y1;
		}
	}

	my $bx = $self-> {blocks};
	my @clipRect;
	my @size = $self-> size;
	my @aa   = $self-> get_active_area( 0, @size);

	if ( $y2 != $y1) {
		my $b = $$bx[ $y1];
		my @a = ( $$b[ tb::BLK_X], $$b[tb::BLK_Y], $$b[ tb::BLK_X], $$b[ tb::BLK_Y]);
		for ( $y1 .. $y2) {
			my $z = $$bx[ $_];
			my @b = ( $$z[ tb::BLK_X], $$z[tb::BLK_Y], $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH], $$z[ tb::BLK_Y] + $$z[ tb::BLK_HEIGHT]);
			for ( 0, 1) { $a[$_] = $b[$_] if $a[$_] > $b[$_] }
			for ( 2, 3) { $a[$_] = $b[$_] if $a[$_] < $b[$_] }
		}
		$clipRect[0] = $aa[0] - $self-> {offset}  + $a[0];
		$clipRect[1] = $aa[3] + $self-> {topLine} - $a[1] - 1;
		$clipRect[2] = $aa[0] - $self-> {offset}  + $a[2];
		$clipRect[3] = $aa[3] + $self-> {topLine} - $a[3] - 1;
	} else {
		my $b = $$bx[ $y1]; 
		( $x2, $x1) = ( $x1, $x2) if $x1 > $x2;
		$clipRect[0] = $aa[0] - $self-> {offset} + 
			$$b[ tb::BLK_X] + $self-> text2xoffset( $x1, $y1); 
		$clipRect[1] = $aa[3] - $$b[ tb::BLK_Y] - 
			$$b[ tb::BLK_HEIGHT] + $self-> {topLine} - 1;
		$clipRect[2] = $aa[0] - $self-> {offset} + 
			$$b[ tb::BLK_X] + $self-> text2xoffset( $x2, $y1);
		$clipRect[3] = $aa[3] - $$b[ tb::BLK_Y] + 
			$self-> {topLine} - 1;
	}

	for ( 0, 1) {
		@clipRect[$_,$_+2] = @clipRect[$_+2,$_] 
			if $clipRect[$_] > $clipRect[$_+2];
		$clipRect[$_] = $aa[$_] if $clipRect[$_] < $aa[$_]; 
		$clipRect[$_+2] = $aa[$_+2] if $clipRect[$_+2] > $aa[$_+2];
	}

	$self-> {selection} = [ $sx1, $sy1, $sx2, $sy2 ];
	my @cpr = $self-> get_invalid_rect;
	if ( $cpr[0] != $cpr[2] || $cpr[1] != $cpr[3]) {
		for ( 0,1) {
			$clipRect[$_] = $cpr[$_] if $clipRect[$_] > $cpr[$_];
			$clipRect[$_+2] = $cpr[$_+2] if $clipRect[$_+2] < $cpr[$_+2];
		}
	}
	$self-> invalidate_rect( @clipRect);
}

sub get_selected_text
{
	my $self = $_[0];
	return unless $self-> has_selection;
	my ( $sx1, $sy1, $sx2, $sy2) = $self-> selection;
	my ( $a1, $a2) = (
		$self-> {blocks}-> [$sy1]-> [tb::BLK_TEXT_OFFSET] + $sx1,
		$self-> {blocks}-> [$sy2]-> [tb::BLK_TEXT_OFFSET] + $sx2,
	);
	return substr( ${$self-> {text}}, $a1, $a2 - $a1);
}

sub copy
{
	my $self = $_[0];
	my $text = $self-> get_selected_text;
	$::application-> Clipboard-> store( 'Text', $text) if defined $text;
}

sub clear_all
{
	my $self = $_[0];
	$self-> selection(-1,-1,-1,-1);
	$self-> {blocks} = [];
	$self-> paneSize( 0, 0);
	$self-> text('');
}


package Prima::TextView::EventRectangles;

sub new
{
	my $class = shift;
	my %profile = @_;
	my $self = {};
	bless( $self, $class);
	$self-> {$_} = $profile{$_} ? $profile{$_} : [] 
		for qw( rectangles references);
	return $self;
}

sub contains 
{ 
	my ( $self, $x, $y) = @_;
	my $rec = 0;
	for ( @{$self-> {rectangles}}) {
		return $rec if $x >= $$_[0] && $y >= $$_[1] && $x < $$_[2] && $y < $$_[3];
		$rec++;
	}
	return -1;
}

sub rectangles
{
	return $_[0]-> {rectangles} unless $#_;
	$_[0]-> {rectangles} = $_[1];
}

sub references
{
	return $_[0]-> {references} unless $#_;
	$_[0]-> {references} = $_[1];
}

1;

__END__

=pod

=head1 NAME 

Prima::TextView - rich text browser widget

=head1 SYNOPSIS

 use strict;
 use warnings;
 use Prima qw(TextView Application);
 
 my $w = Prima::MainWindow-> create(
     name => 'TextView example',
 );
 
 my $t = $w->insert(TextView =>
     text     => 'Hello from TextView!',
     pack     => { expand => 1, fill => 'both' },
 );
 
 # Create a single block that renders all the text using the default font
 my $tb = tb::block_create();
 my $text_width_px = $t->get_text_width($t->text);
 my $font_height_px = $t->font->height;
 $tb->[tb::BLK_WIDTH]  = $text_width_px;
 $tb->[tb::BLK_HEIGHT] = $font_height_px;
 $tb->[tb::BLK_BACKCOLOR] = cl::Back;
 $tb->[tb::BLK_FONT_SIZE] = int($font_height_px) + tb::F_HEIGHT;
 # Add an operation that draws the text:
 push @$tb, tb::text(0, length($t->text), $text_width_px);
 
 # Set the markup block(s) and recalculate the ymap
 $t->{blocks} = [$tb];
 $t->recalc_ymap;
 
 # Additional step needed for horizontal scroll as well as per-character
 # selection:
 $t->paneSize($text_width_px, $font_height_px);
 
 run Prima;

=head1 DESCRIPTION

Prima::TextView accepts blocks of formatted text, and provides
basic functionality - scrolling and user selection. The text strings
are stored as one large text chunk, available by the C<::text> and C<::textRef> properties.
A block of a formatted text is an array with fixed-length header and 
the following instructions. 

A special package C<tb::> provides the block constants and simple functions
for text block access. 

=head2 Capabilities

Prima::TextView is mainly the text block functions and helpers. It provides
function for wrapping text block, calculating block dimensions, drawing
and converting coordinates from (X,Y) to a block position. Prima::TextView
is centered around the text functionality, and although any custom graphic of
arbitrary complexity can be embedded in a text block, the internal coordinate
system is used ( TEXT_OFFSET, BLOCK ), where TEXT_OFFSET is a text offset from 
the beginning of a block and BLOCK is an index of a block.

The functionality does not imply any text layout - this is up to the class
descendants, they must provide they own layout policy. The only policy
Prima::TextView requires is that blocks' BLK_TEXT_OFFSET field must be
strictly increasing, and the block text chunks must not overlap. The text gaps
are allowed though. 

A text block basic drawing function includes change of color, backColor and font,
and the painting of text strings. Other types of graphics can be achieved by
supplying custom code.

=head2 Block header

A block's fixed header consists of C<tb::BLK_START - 1> integer scalars,
each of those is accessible via the corresponding C<tb::BLK_XXX> constant.
The constants are separated into two logical groups:

	BLK_FLAGS        
	BLK_WIDTH        
	BLK_HEIGHT      
	BLK_X           
	BLK_Y           
	BLK_APERTURE_X  
	BLK_APERTURE_Y  
	BLK_TEXT_OFFSET 

and

	BLK_FONT_ID          
	BLK_FONT_SIZE        
	BLK_FONT_STYLE       
	BLK_COLOR            
	BLK_BACKCOLOR        

The second group is enclosed in C<tb::BLK_DATA_START> - C<tb::BLK_DATA_END>
range, like the whole header is contained in 0 - C<tb::BLK_START - 1> range.
This is done for the backward compatibility, if the future development changes 
the length of the header. 

The first group fields define the text block dimension, aperture position
and text offset ( remember, the text is stored as one big chunk ). The second
defines the initial color and font settings. Prima::TextView needs all fields
of every block to be initialized before displaying. L<block_wrap> method
can be used for automated assigning of these fields.

=head2 Block parameters

The scalars, beginning from C<tb::BLK_START>, represent the commands to the renderer.
These commands have their own parameters, that follow the command. The length of
a command is located in C<@oplen> array, and must not be changed. The basic command
set includes C<OP_TEXT>, C<OP_COLOR>, C<OP_FONT>, C<OP_TRANSPOSE>, and C<OP_CODE>.
The additional codes are C<OP_WRAP> and C<OP_MARK>, not used in drawing but are
special commands to L<block_wrap>.

=over

=item OP_TEXT - TEXT_OFFSET, TEXT_LENGTH, TEXT_WIDTH

C<OP_TEXT> commands to draw a string, from offset C<tb::BLK_TEXT_OFFSET + TEXT_OFFSET>,
with a length TEXT_LENGTH. The third parameter TEXT_WIDTH contains the width of the text
in pixels. Such the two-part offset scheme is made for simplification or an imaginary code,
that would alter ( insert to, or delete part of ) the big text chunk; the updating procedure
would not need to traverse all commands, but just the block headers.

Relative to: C<tb::BLK_TEXT_OFFSET>.

=item OP_COLOR - COLOR

C<OP_COLOR> sets foreground or background color. To set the background,
COLOR must be or-ed with C<tb::BACKCOLOR_FLAG> value. In addition to the 
two toolkit supported color values ( RRGGBB and system color index ), 
COLOR can also be or-ed with C<tb::COLOR_INDEX> flags, in such case it is
an index in C<::colormap> property array.

Relative to: C<tb::BLK_COLOR>, C<tb::BLK_BACKCOLOR>.

=item OP_FONT - KEY, VALUE

As the font is a complex property, that itself includes font name, size, 
direction, etc keys, C<OP_FONT> KEY represents one of the three
parameters - C<tb::F_ID>, C<tb::F_SIZE>, C<tb::F_STYLE>. All three
have different VALUE meaning. 

Relative to: C<tb::BLK_FONT_ID>, C<tb::BLK_FONT_SIZE>, C<tb::BLK_FONT_STYLE>.  

=over

=item F_STYLE

Contains a combination of C<fs::XXX> constants, such as C<fs::Bold>, C<fs::Italic> etc. 

Default value: 0 

=item F_SIZE

Contains the relative font size. The size is relative to the current widget's font
size. As such, 0 is a default value, and -2 is the widget's default font decreased by
2 points. Prima::TextView provides no range checking ( but the toolkit does ), so
while it is o.k. to set the negative C<F_SIZE> values larger than the default font size,
one must be vary when relying on the combined font size value .

If C<F_SIZE> value is added to a C<F_HEIGHT> constant, then it is treated as a font height
in pixels rather than font size in points. The macros for these opcodes are named respectively
C<tb::fontSize> and C<tb::fontHeight>, while the opcode is the same.

=item F_ID

All other font properties are collected under an 'ID'. ID is a index in
the C<::fontPalette> property array, which contains font hashes with the other
font keys initialized - name, encoding, and pitch. These three are minimal required
set, and the other font keys can be also selected.

=back

=item OP_TRANSPOSE X, Y, FLAGS

Contains a mark for an empty space. The space is extended to the relative coordinates (X,Y), 
so the block extension algorithms take this opcode in the account. If FLAGS does not contain
C<tb::X_EXTEND>, then in addition to the block expansion, current coordinate is also
moved to (X,Y). In this regard, C<(OP_TRANSPOSE,0,0,0)> and C<(OP_TRANSPOSE,0,0,X_EXTEND)> are
identical and are empty operators. 

There are formatting-only flags,in effect with L<block_wrap> function.
C<X_DIMENSION_FONT_HEIGHT> indicates that (X,Y) values must be multiplied to
the current font height.  Another flag C<X_DIMENSION_POINT> does the same but
multiplies by current value of L<resolution> property divided by 72 (
basically, treats X and Y not as pixel but point values).

C<OP_TRANSPOSE> can be used for customized graphics, in conjunction with C<OP_CODE>
to assign a space, so the rendering
algorithms do not need to be re-written every time the new graphic is invented. As
an example, see how L<Prima::PodView> deals with the images.

=item OP_CODE - SUB, PARAMETER

Contains a custom code pointer SUB with a parameter PARAMETER, passed when
a block is about to be drawn. SUB is called with the following format:

	( $widget, $canvas, $text_block, $font_and_color_state, $x, $y, $parameter);

$font_and_color_state ( or $state, through the code ) contains the state of 
font and color commands in effect, and is changed as the rendering algorithm advances through a block.
The format of the state is the same as of text block, so one may notice that for readability
F_ID, F_SIZE, F_STYLE constants are paired to BLK_FONT_ID, BLK_FONT_SIZE and BLK_FONT_STYLE.

The SUB code is executed only when the block is about to draw. 

=item OP_WRAP ON_OFF

C<OP_WRAP> is only in effect in L<block_wrap> method. ON_OFF is a boolean flag,
selecting if the wrapping is turned on or off. L<block_wrap> does not support 
stacking for the wrap commands, so the C<(OP_WRAP,1,OP_WRAP,1,OP_WRAP,0)> has 
same effect as C<(OP_WRAP,0)>. If ON_OFF is 1, wrapping is disabled - all following
commands treated an non-wrapable until C<(OP_WRAP,0)> is met.

=item OP_MARK PARAMETER, X, Y

C<OP_MARK> is only in effect in L<block_wrap> method and is a user command.
L<block_wrap> only sets (!) X and Y to the current coordinates when the command is met. 
Thus, C<OP_MARK> can be used for arbitrary reasons, easy marking the geometrical positions
that undergo the block wrapping.

=back

As can be noticed, these opcodes are far not enough for the full-weight rich text
viewer. However, the new opcodes can be created using C<tb::opcode>, that accepts
the opcode length and returns the new opcode value.

=head2 Rendering methods

=over

=item block_wrap

C<block_wrap> is the function, that is used to wrap a block into a given width.
It returns one or more text blocks with fully assigned headers. The returned blocks
are located one below another, providing an illusion that the text itself is wrapped.
It does not only traverses the opcodes and sees if the command fit or not in the given width;
it also splits the text strings if these do not fit. 

By default the wrapping can occur either on a command boundary or by the spaces or tab characters
in the text strings. The unsolicited wrapping can be prevented by using C<OP_WRAP>
command brackets. The commands inside these brackets are not wrapped; C<OP_WRAP> commands
are removed from the output blocks.

In general, C<block_wrap> copies all commands and their parameters as is, ( as it is supposed
to do ), but some commands are treated especially:

- C<OP_TEXT>'s third parameter, C<TEXT_WIDTH>, is disregarded, and is recalculated for every
C<OP_TEXT> met.

- If C<OP_TRANSPOSE>'s third parameter, C<X_FLAGS> contains C<X_DIMENSION_FONT_HEIGHT> flag,
the command coordinates X and Y are multiplied to the current font height and the flag is
cleared in the output block.

- C<OP_MARK>'s second and third parameters assigned to the current (X,Y) coordinates.

- C<OP_WRAP> removed from the output.

=item block_draw CANVAS, BLOCK, X, Y

The C<block_draw> draws BLOCK onto CANVAS in screen coordinates (X,Y).
It can not only be used for drawing inside begin_paint/end_paint brackets;
CANVAS can be an arbitrary C<Prima::Drawable> descendant.

=back

=head2 Coordinate system methods

Prima::TextView employs two its own coordinate systems:
(X,Y)-document and (TEXT_OFFSET,BLOCK)-block. 

The document coordinate system is isometric and measured in pixels. Its origin is located 
into the imaginary point of the beginning of the document ( not of the first block! ),
in the upper-left point. X increases to the right, Y increases downwards.
The block header values BLK_X and BLK_Y are in document coordinates, and
the widget's pane extents ( regulated by C<::paneSize>, C<::paneWidth> and
C<::paneHeight> properties ) are also in document coordinates.

The block coordinate system in an-isometric - its second axis, BLOCK, is an index
of a text block in the widget's blocks storage, C<$self-E<gt>{blocks}>, and
its first axis, TEXT_OFFSET is a text offset from the beginning of the block.

Below described different coordinate system converters

=over

=item screen2point X, Y

Accepts (X,Y) in the screen coordinates ( O is a lower left widget corner ),
returns (X,Y) in document coordinates ( O is upper left corner of a document ).

=item xy2info X, Y

Accepts (X,Y) is document coordinates, returns (TEXT_OFFSET,BLOCK) coordinates,
where TEXT_OFFSET is text offset from the beginning of a block ( not related
to the big text chunk ) , and BLOCK is an index of a block.

=item info2xy TEXT_OFFSET, BLOCK

Accepts (TEXT_OFFSET,BLOCK) coordinates, and returns (X,Y) in document coordinates
of a block.

=item text2xoffset TEXT_OFFSET, BLOCK

Returns X coordinate where TEXT_OFFSET begins in a BLOCK index.

=item info2text_offset

Accepts (TEXT_OFFSET,BLOCK) coordinates and returns the text offset 
with regard to the big text chunk.

=item text_offset2info TEXT_OFFSET

Accepts big text offset and returns (TEXT_OFFSET,BLOCK) coordinates 

=item text_offset2block TEXT_OFFSET

Accepts big text offset and returns BLOCK coordinate.

=back

=head2 Text selection

The text selection is performed automatically when the user selects the
region with a mouse. The selection is stored in (TEXT_OFFSET,BLOCK)
coordinate pair, and is accessible via the C<::selection> property.
If its value is assigned to (-1,-1,-1,-1) this indicates that there is
no selection. For convenience the C<has_selection> method is introduced.

Also, C<get_selected_text> returns the text within the selection
(or undef with no selection ), and C<copy> copies automatically 
the selected text into the clipboard. The latter action is bound to 
C<Ctrl+Insert> key combination.

=head2 Event rectangles

Partly as an option for future development, partly as a hack a
concept of 'event rectangles' was introduced. Currently, C<{contents}>
private variable points to an array of objects, equipped with 
C<on_mousedown>, C<on_mousemove>, and C<on_mouseup> methods. These
are called within the widget's mouse events, so the overloaded classes
can define the interactive content without overloading the actual
mouse events ( which is although easy but is dependent on Prima::TextView 
own mouse reactions ).

As an example L<Prima::PodView> uses the event rectangles to catch
the mouse events over the document links. Theoretically, every 'content'
is to be bound with a separate logical layer; when the concept was designed,
a html-browser was in mind, so such layers can be thought as 
( in the html world ) links, image maps, layers, external widgets. 

Currently, C<Prima::TextView::EventRectangles> class is provided
for such usage. Its property C<::rectangles> contains an array of
rectangles, and the C<contains> method returns an integer value, whether
the passed coordinates are inside one of its rectangles or not; in the first
case it is the rectangle index.

=cut
