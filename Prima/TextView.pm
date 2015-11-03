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
use warnings;
use Prima;
use Prima::IntUtils;
use Prima::ScrollBar;

package 
    tb;
use vars qw(@oplen %opnames);

@oplen = ( 4, 2, 3, 4, 3, 2, 4, 3);   # lengths of tb::OP_XXX constants ( see below ) + 1
# basic opcodes
use constant OP_TEXT               =>  0; # (3) text offset, text length, text width
use constant OP_COLOR              =>  1; # (1) 0xRRGGBB or COLOR_INDEX | palette_index
use constant OP_FONT               =>  2; # (2) op_font_mode, font info
use constant OP_TRANSPOSE          =>  3; # (3) move current point to delta X, delta Y
use constant OP_CODE               =>  4; # (2) code pointer and parameters 

# formatting opcodes
use constant OP_WRAP               =>  5; # (1) on / off
use constant OP_MARK               =>  6; # (3) id, x, y
use constant OP_BIDIMAP            =>  7; # (2) visual, $map

%opnames = (
	text      => OP_TEXT,
	color     => OP_COLOR,
	font      => OP_FONT,
	transpose => OP_TRANSPOSE,
	code      => OP_CODE,
	wrap      => OP_WRAP,
	mark      => OP_MARK,
	bidimap   => OP_BIDIMAP,
);


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
use constant X_TRANSPOSE             => 0;
use constant X_EXTEND                => 1;
use constant X_DIMENSION_PIXEL       => 0;
use constant X_DIMENSION_FONT_HEIGHT => 2; # multiply by font height
use constant X_DIMENSION_POINT       => 4; # multiply by resolution / 72


# OP_MARK
use constant MARK_ID                 => 1;
use constant MARK_X                  => 2;
use constant MARK_Y                  => 3;

# OP_BIDIMAP
use constant BIDI_VISUAL             => 1;
use constant BIDI_MAP                => 2;

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
use constant T_IS_BIDI   => 0x4;

# realize_state mode

use constant REALIZE_FONTS   => 0x1;
use constant REALIZE_COLORS  => 0x2;
use constant REALIZE_ALL     => 0x3;

# trace constants
use constant TRACE_FONTS          => 0x01;
use constant TRACE_COLORS         => 0x02;
use constant TRACE_PENS           => TRACE_COLORS | TRACE_FONTS;
use constant TRACE_POSITION       => 0x04;
use constant TRACE_TEXT           => 0x08;
use constant TRACE_GEOMETRY       => TRACE_FONTS | TRACE_POSITION;
use constant TRACE_UPDATE_MARK    => 0x10;
use constant TRACE_PAINT_STATE    => 0x20;
use constant TRACE_REALIZE        => 0x40;
use constant TRACE_REALIZE_FONTS  => TRACE_FONTS | TRACE_REALIZE;
use constant TRACE_REALIZE_COLORS => TRACE_COLORS | TRACE_REALIZE;
use constant TRACE_REALIZE_PENS   => TRACE_PENS | TRACE_REALIZE;

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
	my $name = $_[1];
	$len = 0 if $len < 0;
	push @oplen, $len + 1;
	$opnames{$name} = scalar(@oplen) - 1 if defined $name;
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
sub bidimap        { return OP_BIDIMAP, $_[0], $_[1] }

package Prima::TextView::EventContent;

sub on_mousedown {}
sub on_mousemove {}
sub on_mouseup   {}

package Prima::TextView;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller Prima::GroupScroller);
use Prima::Bidi qw(:methods is_bidi);

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
		scrollBarClass  => 'Prima::ScrollBar',
		hScrollBarProfile=> {},
		vScrollBarProfile=> {},
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
	$self->{$_} = $profile{$_} for qw(scrollBarClass hScrollBarProfile vScrollBarProfile);
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

sub begin_paint_info
{
	my $self = shift;
	delete $self->{currentFont};
	return $self->SUPER::begin_paint_info;
}

sub end_paint_info
{
	my $self = shift;
	delete $self->{currentFont};
	return $self->SUPER::end_paint_info;
}

sub _hash { my $k = shift; join("\0", map { ($_, $k->{$_}) } sort keys %$k) }

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

		goto SKIP if 
			exists $self->{currentFont} &&
			_hash($self->{currentFont}) eq _hash(\%f);
		$self->{currentFont} = \%f;
		$canvas-> set_font( \%f);
	SKIP:
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

sub block_walk_abort { shift->{blockWalk} = 0        }

sub block_walk
{
	my ( $self, $block, %commands ) = @_;

	my $trace    = delete($commands{trace})    // 0;
	my $position = delete($commands{position}) // [0,0];
	my $canvas   = delete($commands{canvas})   // $self;
	my $state    = delete($commands{state})    // [];
	my $other    = delete $commands{other};
	my $ptr      = delete $commands{pointer}   // \(my $_i);

	my @commands;
	$commands[ $tb::opnames{$_} ] = $commands{$_} for keys %commands;
	my $ret;
	local $self-> {blockWalk} = 1;

	my ( $text, $text_offset, $f_taint, $font, $c_taint, $paint_state, %save_properties );

	# save paint state
	if ( $trace & tb::TRACE_PAINT_STATE ) {
		$paint_state = $canvas-> get_paint_state;
		if ($paint_state) {
			$save_properties{set_font} = $canvas->get_font if $trace & tb::TRACE_FONTS;
			if ($trace & tb::TRACE_COLORS) {
				$save_properties{$_} = $canvas->$_() for qw(color backColor textOpaque);
			}
		} else {
			$canvas-> begin_paint_info;
		}
	}

	( $text, $text_offset) = ( $self-> {text}, $$block[ tb::BLK_TEXT_OFFSET])
		if $trace & tb::TRACE_TEXT;
	@$state = @$block[ 0 .. tb::BLK_DATA_END ]
		if !@$state && $trace & tb::TRACE_PENS;
	$$position[0] += $$block[ tb::BLK_APERTURE_X], $$position[1] += $$block[ tb::BLK_APERTURE_Y]
		if $trace & tb::TRACE_POSITION;

	# go
	my $lim = scalar(@$block);
	for ( $$ptr = tb::BLK_START; $$ptr < $lim; $$ptr += $tb::oplen[ $$block[ $$ptr ]] ) {
		my $i   = $$ptr;
		my $cmd = $$block[$i];
		my $sub = $commands[ $$block[$i] ];
		my @opcode;
		if ( !$sub && $other ) {
			$sub = $other;
			@opcode = ($cmd);
		}
		if ($cmd == tb::OP_TEXT) {
			next unless $$block[$i + tb::T_LEN] > 0;

			if (( $trace & tb::TRACE_FONTS) && ($trace & tb::TRACE_REALIZE) && !$f_taint) {
				$self-> realize_state( $canvas, $state, tb::REALIZE_FONTS);
				$f_taint = 1;
			}
			if (( $trace & tb::TRACE_COLORS) && ($trace & tb::TRACE_REALIZE) && !$c_taint) {
				$self-> realize_state( $canvas, $state, tb::REALIZE_COLORS);
				$c_taint = 1;
			}
			$ret = $sub->(
				@opcode,
				@$block[$i + 1 .. $i + $tb::oplen[ $$block[ $$ptr ]] - 1],
				(( $trace & tb::TRACE_TEXT ) ?
					substr( $$text, $text_offset + $$block[$i + tb::T_OFS], $$block[$i + tb::T_LEN] ) : ())
			) if $sub;
			$$position[0] += $$block[ $i + tb::T_WID] if $trace & tb::TRACE_POSITION;
			last unless $self-> {blockWalk};
			next;
		} elsif (($cmd == tb::OP_FONT) && ($trace & tb::TRACE_FONTS)) {
			if ( $$block[$i + tb::F_MODE] == tb::F_SIZE && $$block[$i + tb::F_DATA] < tb::F_HEIGHT ) {
				$$state[ $$block[$i + tb::F_MODE]] = $self-> {defaultFontSize} + $$block[$i + tb::F_DATA];
			} else {
				$$state[ $$block[$i + tb::F_MODE]] = $$block[$i + tb::F_DATA];
			}
			$font = $f_taint = undef;
		} elsif (($cmd == tb::OP_COLOR) && ($trace & tb::TRACE_COLORS)) {
			if ( ($$block[ $i + 1] & tb::BACKCOLOR_FLAG) ) {
				$$state[ tb::BLK_BACKCOLOR ] = $$block[$i + 1] & ~tb::BACKCOLOR_FLAG;
			} else {
				$$state[ tb::BLK_COLOR ] = $$block[$i + 1];
			}
			$c_taint = undef;
		} elsif ( $cmd == tb::OP_TRANSPOSE) {
			my $x = $$block[ $i + tb::X_X];
			my $y = $$block[ $i + tb::X_Y];
			my $f = $$block[ $i + tb::X_FLAGS];
			if (($trace & tb::TRACE_FONTS) && ($trace & tb::TRACE_REALIZE)) {
				if ( $f & tb::X_DIMENSION_FONT_HEIGHT) {
					unless ( $f_taint) {
						$self-> realize_state( $canvas, $state, tb::REALIZE_FONTS); 
						$f_taint = 1;
					}
					$font //= $canvas-> get_font;
					$x *= $font-> {height};
					$y *= $font-> {height};
					$f &= ~tb::X_DIMENSION_FONT_HEIGHT;
				}
			}
			if ( $f & tb::X_DIMENSION_POINT) {
				$x *= $self-> {resolution}-> [0] / 72;
				$y *= $self-> {resolution}-> [1] / 72;
				$f &= ~tb::X_DIMENSION_POINT;
			}
			$ret = $sub->( $x, $y, $f ) if $sub;
			if (!($f & tb::X_EXTEND) && ($trace & tb::TRACE_POSITION)) {
				$$position[0] += $x;
				$$position[1] += $y;
			}
			last unless $self-> {blockWalk};
			next;
		} elsif (( $cmd == tb::OP_CODE) && ($trace & tb::TRACE_PENS) && ($trace & tb::TRACE_REALIZE)) {
			unless ( $f_taint) {
				$self-> realize_state( $canvas, $state, tb::REALIZE_FONTS);
				$f_taint = 1;
			}
			unless ( $c_taint) {
				$self-> realize_state( $canvas, $state, tb::REALIZE_COLORS);
				$c_taint = 1;
			}
		} elsif (($cmd == tb::OP_BIDIMAP) && ( $trace & tb::TRACE_TEXT )) {
			$text = \ $$block[$i + tb::BIDI_VISUAL];
			$text_offset = 0;
		} elsif (( $cmd == tb::OP_MARK) & ( $trace & tb::TRACE_UPDATE_MARK)) {
			$$block[ $i + tb::MARK_X] = $$position[0];
			$$block[ $i + tb::MARK_Y] = $$position[1];
		} elsif ( $cmd > $#tb::oplen ) {
			warn "corrupted block, $cmd at $$ptr\n";
			$self->_debug_block($block);
			last;
		}
		$ret = $sub->( @opcode, @$block[$i + 1 .. $i + $tb::oplen[ $$block[ $$ptr ]] - 1]) if $sub;
		last unless $self-> {blockWalk};
	}

	# restore paint state
	if ( $trace & tb::TRACE_PAINT_STATE ) {
		if ( $paint_state ) {
			delete $save_properties{set_font} if
				exists $self->{currentFont} && 
				exists $save_properties{set_font} && 
				_hash($self->{currentFont}) eq _hash($save_properties{set_font})
				;
			$canvas->$_( $save_properties{$_} ) for keys %save_properties;
		} else {
			$canvas->end_paint_info;
		}
	}

	return $ret;
}

sub block_wrap
{
	my ( $self, $canvas, $b, $state, $width) = @_;
	$width = 0 if $width < 0;

	my $cmd;
	my ( $o, $t) = ( $$b[ tb::BLK_TEXT_OFFSET], $self-> {text});
	my ( $x, $y) = (0, 0);
	my $wrapmode = 1;
	my $stsave = $state;
	$state = [ @$state ];
	my ( $haswrapinfo, @wrapret);
	my ( @ret, $z, $ptr);
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
		$ptr = $wrapret[2];
	};

	$newblock-> ();
	$$z[tb::BLK_TEXT_OFFSET] = $$b[tb::BLK_TEXT_OFFSET];

	my %state_hash;
	my $has_bidi;

	# first state - wrap the block
	$self-> block_walk( $b,
		pointer => \$ptr,
		canvas  => $canvas,
		state   => $state,
		trace   => tb::TRACE_REALIZE_PENS,
		text    => sub {
			my ( $ofs, $tlen ) = @_;
			my $state_key = join('.', @$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]);
			$state_hash{$state_key} = $canvas->get_font 
				unless $state_hash{$state_key};
			$lastTextOffset = $ofs + $tlen unless $wrapmode;
		REWRAP: 
			my $tw  = $canvas-> get_text_width( substr( $$t, $o + $ofs, $tlen), 1);
			my $apx = $state_hash{$state_key}-> {width};
			if ( $x + $tw + $apx <= $width) {
				push @$z, tb::OP_TEXT, $ofs, $tlen, $tw;
				$x += $tw;
				$has_text = 1;
			} elsif ( $wrapmode) {
				return if $tlen <= 0;
				my $str = substr( $$t, $o + $ofs, $tlen);
				my $leadingSpaces = '';
				if ( $str =~ /^(\s+)/) {
					$leadingSpaces = $1;
					$str =~ s/^\s+//;
				}
				my $l = $canvas-> text_wrap( $str, $width - $apx - $x,
					tw::ReturnFirstLineLength | tw::WordBreak | tw::BreakSingle);
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
					$str = substr( $str, $l);
					$l += length $leadingSpaces;
					$newblock-> ();
					$ofs += $l;
					$tlen -= $l;
					if ( $str =~ /^(\s+)/) {
						$ofs  += length $1;
						$tlen -= length $1;
						$x    += $canvas-> get_text_width( $1, 1);
						$str =~ s/^\s+//;
					}
					goto REWRAP if length $str;
				} else { # does not fit into $width
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
					push @$z, tb::OP_TEXT, $ofs, length($str),
						$x += $canvas-> get_text_width( $str, 1);
					$has_text = 1;
				}
			} elsif ( $haswrapinfo) { # unwrappable, and cannot be fit - retrace
				$retrace-> ();
			} else { # unwrappable, cannot be fit, no wrap info! - whole new block
				push @$z, tb::OP_TEXT, $ofs, $tlen, $tw;
				$newblock-> ();
			}
		},
		wrap => sub {
			my $mode = shift;
			if ( $wrapmode == 1 && $mode == 0) {
				@wrapret = ( scalar @$z, [ @$state ], $ptr);
				$haswrapinfo = 1;
			}
			$wrapmode = $mode;
		},
		transpose => sub {
			my ( $dx, $dy, $flags) = @_;
			if ( $x + $dx >= $width) {
				if ( $wrapmode) {
					$newblock-> (); 
				} elsif ( $haswrapinfo) {
					return $retrace-> ();
				} 
			} else {
				$x += $dx;
			}
			push @$z, tb::OP_TRANSPOSE, $dx, $dy, $flags;
		},
		other => sub { push @$z, @_ },
	);

	# remove eventual empty trailing blocks
	pop @ret while scalar ( @ret) && ( tb::BLK_START == scalar @{$ret[-1]});

	# second stage - position the blocks
	$state = $stsave;
	my $start;
	if ( !defined $$b[ tb::BLK_Y]) { 
		# auto position the block if the creator didn't care
		$start = $$state[ tb::BLK_Y] + $$state[ tb::BLK_HEIGHT];
	} else {
		$start = $$b[ tb::BLK_Y];
	}

	$lastTextOffset = $$b[ tb::BLK_TEXT_OFFSET];
	my $lastBlockOffset = $lastTextOffset;

	for my $b ( @ret) {
		$$b[ tb::BLK_Y] = $start;
		my @xy = (0,0);
		my $ptr;
		$self-> block_walk( $b,
			trace    => tb::TRACE_FONTS | tb::TRACE_POSITION | tb::TRACE_UPDATE_MARK,
			state    => $state,
			position => \@xy,
			pointer  => \$ptr,
			text     => sub {
				my ( $ofs, $len, $wid ) = @_;
				my $f_taint = $state_hash{ join('.', 
					@$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]
				) };
				my $x = $xy[0] + $wid;
				my $y = $xy[1];
				$$b[ tb::BLK_WIDTH] = $x 
					if $$b[ tb::BLK_WIDTH ] < $x;
				$$b[ tb::BLK_APERTURE_Y] = $f_taint-> {descent} - $y 
					if $$b[ tb::BLK_APERTURE_Y] < $f_taint-> {descent} - $y;
				$$b[ tb::BLK_APERTURE_X] = $f_taint-> {width}   - $x 
					if $$b[ tb::BLK_APERTURE_X] < $f_taint-> {width}   - $x;
				my $newY = $y + $f_taint-> {ascent} + $f_taint-> {externalLeading};
				$$b[ tb::BLK_HEIGHT] = $newY if $$b[ tb::BLK_HEIGHT] < $newY; 
				$lastTextOffset = $$b[ tb::BLK_TEXT_OFFSET] + $ofs + $len;

				$$b[ $ptr + tb::T_OFS] -= $lastBlockOffset - $$b[ tb::BLK_TEXT_OFFSET];
			},
			transpose => sub {
				my ( $dx, $dy, $f ) = @_;
				my ( $newX, $newY) = ( $xy[0] + $dx, $xy[1] + $dy);
				$$b[ tb::BLK_WIDTH]  = $newX 
					if $$b[ tb::BLK_WIDTH ] < $newX;
				$$b[ tb::BLK_HEIGHT] = $newY 
					if $$b[ tb::BLK_HEIGHT] < $newY;
				$$b[ tb::BLK_APERTURE_X] = -$newX 
					if $newX < 0 && $$b[ tb::BLK_APERTURE_X] > -$newX;
				$$b[ tb::BLK_APERTURE_Y] = -$newY 
					if $newY < 0 && $$b[ tb::BLK_APERTURE_Y] > -$newY;
			},
		);
		$$b[ tb::BLK_TEXT_OFFSET] = $lastBlockOffset;
		$$b[ tb::BLK_HEIGHT] += $$b[ tb::BLK_APERTURE_Y];
		$$b[ tb::BLK_WIDTH]  += $$b[ tb::BLK_APERTURE_X];
		$start += $$b[ tb::BLK_HEIGHT];
		$lastBlockOffset = $lastTextOffset;
	}

	if ( $ret[-1]) {
		$b = $ret[-1];
		$$state[$_] = $$b[$_] for tb::BLK_X, tb::BLK_Y, tb::BLK_HEIGHT, tb::BLK_WIDTH;
	}

	# third stage - map bidi characters to visual representation
	my @text_offsets = (( map { $$_[  tb::BLK_TEXT_OFFSET ] } @ret ), $lastTextOffset);
	for ( my $j = 0; $j < @ret; $j++) {
		my $substr  = substr( ${$self->{text}}, $text_offsets[$j], $text_offsets[$j+1] - $text_offsets[$j]);
		next unless $self->is_bidi($substr);
		splice(@ret, $j, 1, $self-> make_bidi_block($canvas, $ret[$j], $text_offsets[$j+1] - $text_offsets[$j]));
	}

	return @ret;
}

sub _debug_block
{
	my ($self, $b) = @_;
	print STDERR "FLAGS      : ", (( $$b[tb::BLK_FLAGS] & tb::T_SIZE ) ? "T_SIZE" : ""), (( $$b[tb::BLK_FLAGS] & tb::T_WRAPABLE ) ? "T_WRAPABLE" : ""), "\n";
	print STDERR "WIDTH      : ", $$b[tb::BLK_WIDTH] // 'undef', "\n";
	print STDERR "HEIGHT     : ", $$b[tb::BLK_HEIGHT] // 'undef', "\n";
	print STDERR "X          : ", $$b[tb::BLK_X] // 'undef', "\n";
	print STDERR "Y          : ", $$b[tb::BLK_Y] // 'undef', "\n";
	print STDERR "AX         : ", $$b[tb::BLK_APERTURE_X] // 'undef', "\n";
	print STDERR "AY         : ", $$b[tb::BLK_APERTURE_Y] // 'undef', "\n";
	print STDERR "TEXT       : ", $$b[tb::BLK_TEXT_OFFSET] // 'undef', "\n";
	print STDERR "FONT_ID    : ", $$b[tb::BLK_FONT_ID] // 'undef', "\n";
	print STDERR "FONT_SIZE  : ", $$b[tb::BLK_FONT_SIZE] // 'undef', "\n";
	print STDERR "FONT_STYLE : ", $$b[tb::BLK_FONT_STYLE] // 'undef', "\n";
	my $color = $$b[tb::BLK_COLOR];
	unless ( defined $color ) {
		$color = "undef";
	} elsif ( $color & tb::COLOR_INDEX) {
		$color = "index(" . ( $color & ~tb::COLOR_INDEX) . ")";
	} else {
		$color = sprintf("%06x", $color);
	}
	print STDERR "COLOR      : $color\n";
	$color = $$b[tb::BLK_BACKCOLOR];
	unless ( defined $color ) {
		$color = "undef";
	} elsif ( $color & tb::COLOR_INDEX) {
		$color = "index(" . ( $color & ~tb::COLOR_INDEX) . ")";
	} else {
		$color = sprintf("%06x", $color);
	}
	print STDERR "BACK_COLOR : $color\n";

	my ($i, $lim) = (tb::BLK_START, scalar @$b);
	my $oplen;
	for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]]) {
		my $cmd = $$b[$i];
		if ( !defined($cmd) || $cmd > $#tb::oplen ) {
			$cmd //= 'undef';
			print STDERR "corrupted block: $cmd at $i/$lim\n";
			last;
		}
		if ($cmd == tb::OP_TEXT) {
			my $ofs = $$b[ $i + tb::T_OFS];
			my $len = $$b[ $i + tb::T_LEN];
			my $wid = $$b[ $i + tb::T_WID] // 'NULL';
			print STDERR ": OP_TEXT( $ofs $len : $wid )\n";
		} elsif ( $cmd == tb::OP_FONT ) {
			my $mode = $$b[ $i + tb::F_MODE ];
			my $data = $$b[ $i + tb::F_DATA ];
			if ( $mode == tb::F_ID ) {
				$mode = 'F_ID';
			} elsif ( $mode == tb::F_SIZE ) {
				$mode = 'F_SIZE';
			} elsif ( $mode == tb::F_STYLE) {
				$mode = 'F_STYLE';
				my @s;
				push @s, "italic" if $data & fs::Italic;
				push @s, "bold" if $data & fs::Bold;
				push @s, "thin" if $data & fs::Thin;
				push @s, "underlined" if $data & fs::Underlined;
				push @s, "struckout" if $data & fs::StruckOut;
				push @s, "outline" if $data & fs::Outline;
				@s = "normal" unless @s;
				$data = join(',', @s);
			}
			print STDERR ": OP_FONT.$mode $data\n";
		} elsif ( $cmd == tb::OP_COLOR ) {
			my $color = $$b[ $i + 1 ];
			my $bk = '';
			if ( $color & tb::BACKCOLOR_FLAG ) {
				$bk = 'backcolor,';
				$color &= ~tb::BACKCOLOR_FLAG;
			}
			if ( $color & tb::COLOR_INDEX) {
				$color = "index(" . ( $color & ~tb::COLOR_INDEX) . ")";
			} else {
				$color = sprintf("%06x", $color);
			}
			print STDERR ": OP_COLOR $bk$color\n";
		} elsif ( $cmd == tb::OP_TRANSPOSE) {
			my $x = $$b[ $i + tb::X_X ];
			my $y = $$b[ $i + tb::X_X ];
			my $f = $$b[ $i + tb::X_FLAGS ] ? 'EXTEND' : 'TRANSPOSE';
			print STDERR ": OP_TRANSPOSE $x $y $f\n";
		} elsif ( $cmd == tb::OP_CODE ) {
			my $code = $$b[ $i + 1 ];
			print STDERR ": OP_CODE $code\n";
		} elsif ( $cmd == tb::OP_BIDIMAP ) {
			my $visual = $$b[ $i + tb::BIDI_VISUAL ];
			my $map    = $$b[ $i + tb::BIDI_MAP ];
			$visual =~ s/([^\x{32}-\x{128}])/sprintf("\\x{%x}", ord($1))/ge;
			print STDERR ": OP_BIDIMAP: $visual / @$map\n";
		} elsif ( $cmd == tb::OP_WRAP ) {
			my $wrap = $$b[ $i + 1 ] ? 'ON' : 'OFF';
			print STDERR ": OP_WRAP $wrap\n";
		} elsif ( $cmd == tb::OP_MARK ) {
			my $id = $$b[ $i + tb::MARK_ID ];
			my $x  = $$b[ $i + tb::MARK_X ];
			my $y  = $$b[ $i + tb::MARK_Y ];
			print STDERR ": OP_MARK $id $x $y\n";
		} else {
			my $oplen = $tb::oplen[ $cmd ];
			my @o = ($oplen > 1) ? @$b[ $i + 1 .. $i + $oplen - 1] : ();
			print STDERR ": OP($cmd) @o\n"; 
		}
	}
}

# convert block with bidicharacters to its visual representation
sub make_bidi_block
{
	my ( $self, $canvas, $b, $len) = @_;
	my $substr = substr( ${$self->{text}}, $$b[tb::BLK_TEXT_OFFSET], $len);

	my ($p, $visual) = $self->bidi_paragraph($substr);
	my $map     = $p->map;
	my $revmap  = $self->bidi_revmap($map);
	my @new     = ( @$b[0..tb::BLK_DATA_END], tb::bidimap( $visual, $map ) );
	$new[tb::BLK_FLAGS] |= tb::T_IS_BIDI;
	my $oplen;
	my ($x, $y, $i, $lim) = (0,0,tb::BLK_START, scalar @$b);

	# step 1 - record how each character is drawn with font/color, and also 
	# where other ops are positioned
	my @default_fc        = @$b[ 0 .. tb::BLK_DATA_END ]; # fc == fonts and colors
	my %id_hash           = ( join(".", @default_fc[tb::BLK_DATA_START .. tb::BLK_DATA_END]) => 0 );
	my @fonts_and_colors  = ( \@default_fc ); # this is the number #0, default char state
	my @current_fc        = @default_fc;
	my $current_state     = 0;
	
	my @char_states       = (-1) x length($visual); # not all chars are displayed
	my $char_offset       = 0;
	my %other_ops_after;

	my $font_and_color = sub {
		my $key = join(".", @current_fc[ tb::BLK_DATA_START .. tb::BLK_DATA_END ]);
		my $state;
		if (defined ($state = $id_hash{$key}) ) {
			$current_state = $state;
		} else {
			push @fonts_and_colors, [ @current_fc ];
			$id_hash{$key} = ++$current_state;
		}
	};

	$self-> block_walk( $b,
		trace => tb::TRACE_PENS,
		state => \@current_fc,
		text => sub {
			my ( $ofs, $len ) = @_;
			for ( my $k = 0; $k < $len; $k++) {
				$char_states[ $revmap->[ $ofs + $k ]] = $current_state;
			}
			$char_offset = $revmap->[$ofs + $len - 1];
		},
		font  => $font_and_color,
		color => $font_and_color,
		other  => sub { push @{$other_ops_after{ $char_offset }}, @_ }
	);

	# step 2 - produce RLEs for text and stuff font/colors/other ops in between
	my $last_char_state = 0;
	my $current_text_offset;

	# find first character displayed
	for ( my $i = 0; $i < @char_states; $i++) {
		next if $char_states[$i] < 0;
		$current_text_offset = $i;
		last;
	}

	my @initialized;
	push @char_states, -1;
	for ( my $i = 0; $i < @char_states; $i++) {
		my $char_state = $char_states[$i];
		if ( $char_state != $last_char_state ) {
			# push accumulated text
			my $ofs = $current_text_offset;
			my $len = $i - $ofs;
			$current_text_offset = ($char_state < 0) ? $i + 1 : $i;
			if ( $len > 0 ) {
				push @new, tb::OP_TEXT, $ofs, $len, substr( $visual, $ofs, $len); # temporarily putting a string there
			}

			# change to the new font/color
			if ( $char_state >= 0 ) {
				my $old_state = $fonts_and_colors[ $last_char_state ];
				my $new_state = $fonts_and_colors[ $char_state ];
				if ( $$old_state[ tb::BLK_COLOR ] != $$new_state[ tb::BLK_COLOR ] ) {
					if ( $initialized[ tb::BLK_COLOR ]++ ) {
						push @new, tb::OP_COLOR, $$new_state[ tb::BLK_COLOR ];
					} else {
						$new[ tb::BLK_COLOR ] = $$new_state[ tb::BLK_COLOR ];
					}
				}
				if ( $$old_state[ tb::BLK_BACKCOLOR ] != $$new_state[ tb::BLK_BACKCOLOR ] ) {
					if ( $initialized[ tb::BLK_BACKCOLOR ]++ ) {
						push @new, tb::OP_COLOR, $$new_state[ tb::BLK_BACKCOLOR ] | tb::BACKCOLOR_FLAG;
					} else {
						$new[ tb::BLK_BACKCOLOR ] = $$new_state[ tb::BLK_BACKCOLOR ];
					}
				}
				for ( my $font_mode = tb::BLK_FONT_ID; $font_mode <= tb::BLK_FONT_STYLE; $font_mode++) {
					next if $$old_state[ $font_mode ] == $$new_state[ $font_mode ];
					if ( $initialized[ $font_mode ]++ ) {
						my $new_value = $$new_state[ $font_mode ];
						$new_value -= $self->{defaultFontSize} if $font_mode == tb::F_SIZE && $new_value < tb::F_HEIGHT;
						push @new, tb::OP_FONT, $font_mode, 
					} else {
						$new[ $font_mode ] = $$new_state[ $font_mode ];
					}
				}
				$last_char_state = $char_state;
			}
		}

		# push other ops
		if ( my $ops = $other_ops_after{ $i } ) {
			push @new, @$ops;
		}
	}

	# step 3 -- update widths and positions etc
	my @xy    = (0,0);
	my $ptr;

	$new[ tb::BLK_WIDTH] = 0;
	$self-> block_walk( \@new,
		canvas   => $canvas,
		trace    => tb::TRACE_REALIZE_FONTS | tb::TRACE_UPDATE_MARK | tb::TRACE_POSITION,
		position => \@xy,
		pointer  => \$ptr,
		text     => sub { $new[ $ptr + tb::T_WID ] = $canvas->get_text_width( $_[2], 1 ) },
	);
	$new[ tb::BLK_WIDTH] = $xy[0] if $new[ tb::BLK_WIDTH ] < $xy[0];

	return \@new;
}

sub selection_state
{
	my ( $self, $canvas) = @_;
	$canvas-> color( $self-> hiliteColor);
	$canvas-> backColor( $self-> hiliteBackColor);
	$canvas-> textOpaque(0);
}

sub paint_selection
{
	my ( $self, $canvas, $block, $index, $x, $y, $sx1, $sx2) = @_;

	my $len = $self->get_block_text_length($index);
	$sx2 = $len - 1 if $sx2 < 0;

	my @selection_map;
	unless ($$block[ tb::BLK_FLAGS ] & tb::T_IS_BIDI) {
		@selection_map = (0) x $len;
		$selection_map[$_] = 1 for $sx1 .. $sx2;
	}
	my @state;
	my @xy = ($x,$y);
	local $self->{selectionPaintMode} = 0;

	my $draw_text = sub {
		my ( $x, $text ) = @_;
		my $f = $canvas->get_font;
		my $w = $canvas->get_text_width($text);
		$self-> realize_state( $canvas, \@state, tb::REALIZE_COLORS); 
		$canvas->clear(
			$x, $xy[1] - $f->{descent},
			$x + $w - 1, $xy[1] + $f->{ascent} + $f->{externalLeading} - 1
			) if $self->{selectionPaintMode};
		$canvas-> text_out($text, $x, $xy[1]);
		return $w;
	};

	$self-> block_walk( $block,
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE_PENS | tb::TRACE_TEXT,
		canvas   => $canvas,
		position => \@xy,
		state    => \@state,
		text     => sub {
			my ($offset, $length, undef, $text) = @_;
			my $accumulated = '';
			my $x = $xy[0];
			for ( my $i = 0; $i < $length; $i++) {
				if ( $selection_map[$offset + $i] != $self->{selectionPaintMode} ) {
					$x += $draw_text->( $x, $accumulated );
					$accumulated = '';
					$self->{selectionPaintMode} = $selection_map[$offset + $i];
				}
				$accumulated .= substr($text, $i, 1);
			}
			$draw_text->( $x, $accumulated ) if length $accumulated;
		},
		code     => sub {
			my ( $code, $data ) = @_;
			$code-> ( $self, $canvas, $b, \@state, @xy, $data);
		},
		bidimap  => sub {
			my $map = pop;
			for ( my $i = 0; $i < @$map; $i++) {
				$selection_map[$i] = ($map->[$i] >= $sx1 && $map->[$i] <= $sx2) ? 1 : 0;
			}
		},
	);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	delete $self->{currentFont};
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
					
			if ( $j == $sy1 && $j == $sy2 ) {
				# selection within one line
				$self->paint_selection( $canvas, $b, $j, $x, $y, $sx1, $sx2 - 1);
			} elsif ( $j == $sy1 ) {
				# upper selected part
				$self->paint_selection( $canvas, $b, $j, $x, $y, $sx1, -1);
			} elsif ( $j == $sy2 ) {
				# lower selected part
				$self->paint_selection( $canvas, $b, $j, $x, $y, 0, $sx2 - 1);
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

	my $ret = 1;
	$canvas-> clear( $x, $y, $x + $$b[ tb::BLK_WIDTH] - 1, $y + $$b[ tb::BLK_HEIGHT] - 1)
		if $self-> {selectionPaintMode};

	my @xy = ($x, $y);
	my @state;
	$self-> block_walk( $b, 
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE_PENS | tb::TRACE_TEXT,
		canvas   => $canvas,
		position => \@xy,
		state    => \@state,
		text     => sub {
			$self-> block_walk_abort( $ret = 0 ) unless $canvas-> text_out($_[-1], @xy);
		},
		code     => sub {
			my ( $code, $data ) = @_;
			$code-> ( $self, $canvas, $b, \@state, @xy, $data);
		},
	);
	
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
	my $ofs = 0;
	my $bidimap;
	my @pos = ($$b[ tb::BLK_X] - $x,0);

	$self-> block_walk( $b,
		position => \@pos,
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE | tb::TRACE_PAINT_STATE | tb::TRACE_TEXT,
		text     => sub {
			my ( $offset, $length, $width, $text) = @_;
			my $npx = $pos[0] + $width;
			if ( $pos[0] > 0) {
				$ofs = $offset;
				$self-> block_walk_abort;
			} elsif ( $pos[0] <= 0 && $npx > 0) {
				$ofs = $offset + $self-> text_wrap( $text, -$pos[0], tw::ReturnFirstLineLength | tw::BreakSingle);
				$self-> block_walk_abort;
			} else {
				$ofs = $offset + $length - 1;
			}
		},
		bidimap => sub { $bidimap = pop },
	);

	$ofs = $bidimap->[$ofs] if $bidimap;

	return $ofs, $bid;
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

	my @pos = (0,0);

	$self-> block_walk( $b,
		position => \@pos,
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE | tb::TRACE_PAINT_STATE | tb::TRACE_TEXT,
		text     => sub {
			my ( $offset, $length, $width, $text) = @_;
			return if $x < $offset;

			if ( $x < $offset + $length ) {
				$pos[0] += $self-> get_text_width( substr( $text, 0, $x - $offset), 1);
				$self-> block_walk_abort;
			} elsif ( $x == $offset + $length ) {
				$pos[0] += $width;
				$self-> block_walk_abort;
			}
		},
	);

	return $pos[0];
}

sub get_block_text
{
	my ( $self, $block ) = @_;
	my $ptr = $self-> {blocks}-> [$block]-> [tb::BLK_TEXT_OFFSET];
	my $len = $self->get_block_text_length( $block );
	return substr( ${$self->{text}}, $ptr, $len);
}

sub get_block_text_length
{
	my ( $self, $block ) = @_;
	my $ptr1 = $self-> {blocks}-> [$block]-> [tb::BLK_TEXT_OFFSET];
	my $ptr2 = ( $block + 1 < @{$self-> {blocks}}) ? 
		$self-> {blocks}-> [$block+1]-> [tb::BLK_TEXT_OFFSET] :
		length ${$self-> {text}};
	return $ptr2 - $ptr1;
}

sub info2text_offset
{
	my ( $self, $offset, $block) = @_;
	return length ${$self-> {text}} unless $block >= 0 && $offset >= 0;

	my $ptr = $self-> {blocks}-> [$block]-> [tb::BLK_TEXT_OFFSET];
	my $len = $self->get_block_text_length( $block );
	if (
		$offset < $len &&
		$self->is_bidi( my $str = substr( ${$self-> {text}}, $ptr, $len ) )
	) {
		$offset = $self->bidi_map($str)->[$offset];
	}
	return $ptr + $offset;
}

sub text_offset2info
{
	my ( $self, $ofs) = @_;
	my $blk = $self-> text_offset2block( $ofs);
	return undef unless defined $blk;
	$ofs -= $self-> {blocks}-> [$blk]-> [ tb::BLK_TEXT_OFFSET];

	if ( $self->is_bidi( my $str = $self-> get_block_text($blk))) {
		$ofs = $self->bidi_map_find( $self-> bidi_map($str), $ofs );
	}
	return $ofs, $blk;
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

	# my $p = $self-> get_selected_text;
	# Prima::Bidi::debug_str($p) if defined $p;

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
	my ( $y1, $y2) = (0,0);
	my ( @old, @new);

	unless ( grep { $_ != -1 } $sx1, $sy1, $sx2, $sy2 ) { # new empty selection
	EMPTY:
		return if $empty;     
		$y1 = $osy1;
		$y2 = $osy2;
		if ( $y1 == $y2) {
			@old = ( $osx1, $osx2 - 1 );
			@new = (1,0);
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
				@new = ( $sx1, $sx2 - 1 );
				@old = (1,0);
			}
		} else {
			if ( $sy1 == $osy1 && $sx1 == $osx1) {
				return if $sy2 == $osy2 && $sx2 == $osx2;
				$y1 = $sy2;
				$y2 = $osy2;
				if ( $sy2 == $osy2) {
					@old = ( 0, $osx2 - 1 );
					@new = ( 0, $sx2  - 1 );
				}
			} elsif ( $sy2 == $osy2 && $sx2 == $osx2) {
				$y1 = $sy1;
				$y2 = $osy1;
				if ( $sy1 == $osy1) {
					@old = ( $osx1, -1 );
					@new = ( $sx1,  -1 );
				}
			} else {
				$y1 = ( $sy1 < $osy1) ? $sy1 : $osy1;
				$y2 = ( $sy2 > $osy2) ? $sy2 : $osy2;
				if ( $sy1 == $sy2 && $osy1 == $osy2 && $sy2 == $osy1) {
					@old = ( $osx1, $osx2 - 1 );
					@new = ( $sx1,  $sx2  - 1 );
				}
			}
			( $y1, $y2) = ( $y2, $y1) if $y2 < $y1;
		}
	}

	my $bx = $self-> {blocks};
	my @clipRect;
	my @size = $self-> size;
	my @aa   = $self-> get_active_area( 0, @size);
	my @invalid_rects;

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

	for ( 0, 1) {
		@clipRect[$_,$_+2] = @clipRect[$_+2,$_] 
			if $clipRect[$_] > $clipRect[$_+2];
		$clipRect[$_] = $aa[$_] if $clipRect[$_] < $aa[$_]; 
		$clipRect[$_+2] = $aa[$_+2] if $clipRect[$_+2] > $aa[$_+2];
	}

	push @invalid_rects, \@clipRect;

	my @cpr = $self-> get_invalid_rect;
	if ( $cpr[0] != $cpr[2] || $cpr[1] != $cpr[3]) {
		for my $cr ( @invalid_rects ) {
			for ( 0,1) {
				$cr->[$_] = $cpr[$_]     if $cr->[$_] > $cpr[$_];
				$cr->[$_+2] = $cpr[$_+2] if $cr->[$_+2] < $cpr[$_+2];
			}
		}
	}
	$self-> {selection} = [ $sx1, $sy1, $sx2, $sy2 ];
	$self->invalidate_rect(@$_) for @invalid_rects;
}

sub get_selected_text
{
	my $self = $_[0];
	return unless $self-> has_selection;
	my ( $sx1, $sy1, $sx2, $sy2) = $self-> selection;
	my ( $a1, $a2) = ( 
		$self-> info2text_offset( $sx1    , $sy1 ),
		$self-> info2text_offset( $sx2 - 1, $sy2 ),
	);
	($a1, $a2) = ($a2, $a1) if $a1 > $a2;
	return substr( ${$self-> {text}}, $a1, $a2 - $a1 + 1);
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
in pixels. Such the two-part offset scheme is made for simplification of an imaginary code,
that would alter ( insert to, or delete part of ) the big text chunk; the updating procedure
would not need to traverse all commands, but just the block headers.

Relative to: C<tb::BLK_TEXT_OFFSET> when not preceded by L<OP_BIDIMAP>.

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

=item OP_BIDIMAP VISUAL, BIDIMAP

C<OP_BIDIMAP> is used when the text to be displayed is RTL (right-to-left) and requires
special handling. This opcode is automatically created by C<block_wrap>. It must be 
present before any C<OP_TEXT> opcode, because when in effect, the C<OP_TEXT> offset calculation
is different - instead of reading characters from C<< $self->{text} >>, it reads them from
C<VISUAL>, and C<BLK_TEXT_OFFSET> in the block header is not used.

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

- C<OP_BIDIMAP> added to the output, if the text to be displayed in the block
contains right-to-left characters.

=item block_draw CANVAS, BLOCK, X, Y

The C<block_draw> draws BLOCK onto CANVAS in screen coordinates (X,Y). It can
be used not only inside begin_paint/end_paint brackets; CANVAS can be an
arbitrary C<Prima::Drawable> descendant.

=item block_walk BLOCK, %OPTIONS

Cycles through block opcodes, calls supplied callbacks on each.

=back

=head2 Coordinate system methods

Prima::TextView employs two its own coordinate systems:
(X,Y)-document and (TEXT_OFFSET,BLOCK)-block. 

The document coordinate system is isometric and measured in pixels. Its origin is located 
into the imaginary point of the beginning of the document ( not of the first block! ),
in the upper-left pixel. X increases to the right, Y increases down.
The block header values BLK_X and BLK_Y are in document coordinates, and
the widget's pane extents ( regulated by C<::paneSize>, C<::paneWidth> and
C<::paneHeight> properties ) are also in document coordinates.

The block coordinate system in an-isometric - its second axis, BLOCK, is an index
of a text block in the widget's blocks storage, C<$self-E<gt>{blocks}>, and
its first axis, TEXT_OFFSET is a text offset from the beginning of the block.

Below different coordinate system converters are described

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

The text selection is performed automatically when the user selects a text
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

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::PodView>, F<examples/mouse_tale.pl>.


=cut
