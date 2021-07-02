package Prima::Drawable::Glyphs;

use strict;
use warnings;
use Carp qw(cluck);
use Prima;

use constant GLYPHS    => 0;
use constant INDEXES   => 1;
use constant ADVANCES  => 2;
use constant POSITIONS => 3;
use constant FONTS     => 4;
use constant CUSTOM    => 5;

sub new         { bless [@_[1..5]], $_[0] }
sub glyphs      { $_[0]->[GLYPHS]    }
sub indexes     { $_[0]->[INDEXES]   }
sub advances    { $_[0]->[ADVANCES]  }
sub positions   { $_[0]->[POSITIONS] }
sub fonts       { $_[0]->[FONTS]     }
sub text_length { $_[0]->[INDEXES]->[-1] }

sub new_array
{
	my ( $self, $storage ) = @_;
	my ( $letter, $n, $size );
	$storage //= 'glyphs';
	$size = scalar @{ $self->[GLYPHS] };
	if ( $storage =~ /^(glyphs|advances|fonts)$/) {
		($letter, $n) = ('S', $size);
	} elsif ( $storage eq 'indexes') {
		($letter, $n) = ('S', $size + 1);
	} elsif ( $storage eq 'positions') {
		($letter, $n) = ('s', $size * 2);
	} else {
		return undef;
	}
	return Prima::array->new($letter, pack($letter.'*', (0) x $n));
}

sub new_empty
{
	my ($class) = @_;
	my @self = ( 
		Prima::array->new('S'), 
		Prima::array->new('S'),
		undef,undef,undef
	);
	push @{$self[1]}, 0;
	return bless \@self, $class;
}

sub _debug
{
	my $self = shift;
	my $noprint = shift;
	my $g = $self->glyphs;

	my $out = scalar(@$g) . " glyphs: @$g\n";
	$g = $self->indexes;
	$out .= "indexes: ";
	for ( my $i = 0; $i < $#$g; $i++) {
		my $ix = $g->[$i];
		$out .= (( $ix & to::RTL ) ? '-' : '') . ( $ix & ~to::RTL ) . " ";
	}
	$out .= ": $g->[-1]\n";
	if ( $g = $self->advances ) {
		$out .= "advances: @$g\n";
		$g = $self->positions;
		$out .= "positions: ";
		for ( my $i = 0; $i < @$g; $i += 2 ) {
			my ($x, $y) = @{$g}[$i,$i+1];
			$out .= "($x,$y) ";
		}
		$out .= "\n";
	}
	if ( $g = $self->fonts ) {
		$out .= "fonts: @$g\n";
		my %f = map { $_ => 1 } @$g;
		delete $f{0};
		if ( $::application ) {
			for my $fid ( sort keys %f ) {
				my $f = $::application->fontMapperPalette($fid);
				$out .= "  #$fid: $f->{name}\n";
			}
		}
		$out .= "\n";
	}
	if ( $noprint ) {
		return $out;
	} else {
		print STDERR $out;
	}
}

sub reverse
{
	my $self = shift;
	my @svs;

	my $nglyphs = @{ $self->[GLYPHS] };

	push @svs, Prima::array->new('S');
	push @{ $svs[-1] }, reverse @{ $self->[GLYPHS] };

	push @svs, Prima::array->new('S');
	push @{ $svs[-1] }, reverse @{ $self->[INDEXES] }[0 .. $nglyphs-1];
	push @{ $svs[-1] }, $self->[INDEXES]->[-1];

	if ( my $advances = $self->[ADVANCES] ) {
		push @svs, Prima::array->new('S');
		push @{ $svs[-1] }, reverse @{ $self->[ADVANCES] };

		push @svs, Prima::array->new('s');
		my $positions = $self->[POSITIONS];
		for ( my $i = $nglyphs * 2 - 2; $i >= 0; $i -= 2 ) {
			push @{ $svs[-1] }, @{$positions}[$i,$i+1];
		}
	} else {
		push @svs, undef, undef;
	}

	if ( my $advances = $self->[FONTS] ) {
		push @svs, Prima::array->new('S');
		push @{ $svs[-1] }, reverse @{ $self->[FONTS] };
	} else {
		push @svs, undef;
	}

	return __PACKAGE__->new(@svs);
}

sub log2vis
{
	my $map  = shift->indexes;
	my @newmap = (undef) x $map->[-1];
	my $last;
	$newmap[ $map->[$_] & ~to::RTL ] = $_ for 0 .. $#$map - 1;
	defined($_) ? ($last = $_) : ($_ = $last) for @newmap;
	return \@newmap;
}

sub glyph_lengths
{
	my $map  = shift->indexes;
	my $curr = $map->[0];
	my $len  = 1;
	my @ret;
	for ( my $i = 1; $i < @$map; $i++) {
		if ( $curr == $$map[$i]) {
			$len++;
		} else {
			push @ret, ($len) x $len;
			$curr = $$map[$i];
			$len = 1;
		}
	}
	return @ret;
}

sub index_lengths
{
	my @map = map { $_ & ~to::RTL } @{ shift->indexes };
	my @sorted = sort { $a <=> $b } @map;
	my @lengths;
	for ( my $i = 0; $i < $#sorted; $i++) {
		$lengths[$sorted[$i]] = $sorted[$i + 1] - $sorted[$i];
	}
	my @ret;
	for ( my $i = 0; $i < $#map; $i++) {
		push @ret, $lengths[$map[$i]]; 
	}
	return @ret;
}

sub overhangs
{
	my ( $self, $canvas ) = @_;
	my $abc = $canvas->get_font_abc(($self->[GLYPHS]->[0]) x 2, to::Glyphs);
	return (
		(($abc->[0] < 0) ? -$abc->[0] : 0),
		(($abc->[2] < 0) ? -$abc->[2] : 0)
	);
}

sub left_overhang  { [shift->overhangs(@_)]->[0] }
sub right_overhang { [shift->overhangs(@_)]->[1] }

sub clone
{
	my $self = shift;
	my @svs = map { defined($_) ? Prima::array::clone($_) : undef } @$self;
	return __PACKAGE__->new(@svs);
}

sub abc
{
	my ( $self, $canvas, $index ) = @_;
	my $glyph = $self->[GLYPHS]->[$index];
	return @{ $canvas-> get_font_abc($glyph, $glyph, to::Glyphs) };
}

sub def
{
	my ( $self, $canvas, $index ) = @_;
	my $glyph = $self->[GLYPHS]->[$index];
	return @{ $canvas-> get_font_def($glyph, $glyph, to::Glyphs) };
}

sub get_width
{
	my ( $self, $canvas, $with_overhangs ) = @_;
	return $canvas-> get_text_width($self, $with_overhangs ? to::AddOverhangs : 0);
}

sub get_box
{
	my ( $self, $canvas ) = @_;
	return $canvas-> get_text_box($self);
}

sub n_clusters
{
	my $last = -1;
	my $indexes = $_[0]->indexes;
	my $n_clusters = 0;
	for ( my $ix = 0; $ix < $#$indexes; $ix++) {
		my $c = $indexes->[$ix] & ~to::RTL;
		if ( $last != $c ) {
			$n_clusters++;
			$last = $c;
		}
	}
	return $n_clusters;
}

sub clusters
{
	my $last = -1;
	my $indexes = $_[0]->indexes;
	my @arr;
	for ( my $ix = 0; $ix < $#$indexes; $ix++) {
		my $c = $indexes->[$ix] & ~to::RTL;
		if ( $last != $c ) {
			push @arr, $c;
			$last = $c;
		}
	}
	return \@arr;
}

sub reorder_text
{
	my $map = shift->indexes;
	my $src = shift;
	my @newmap = (undef) x $map->[-1];
	$newmap[$_] = $map->[$_] & ~to::RTL for 0 .. $#$map - 1;
	my $last;
	return join('', map {
		$last = $newmap[$_] // ($last + 1);
		substr($src, $last, 1)
	} 0..$#newmap);
}

sub cluster2glyph
{
	my ( $self, $from, $length ) = @_;

	my $indexes = $self->indexes;
	$length //= $#$indexes;

	return 0,0 if $length < 0;

	my $last = -1;
	my $curr_cluster = -1;

	my $to = $length ? $from + $length - 1 : undef;
	my ( $gl_from, $gl_to );

	for ( my $ix = 0; $ix < $#$indexes; $ix++) {
		my $c = $indexes->[$ix] & ~to::RTL;
		if ( $last != $c ) {
			if ( $length && $to == $curr_cluster ) {
				$gl_to = $ix;
				last;
			}
			$curr_cluster++;
			$last = $c;
			$gl_from = $ix if $from == $curr_cluster;
		}
	}

	return 0,0 unless defined $gl_from;
	if ( $length ) {
		$gl_to //= $#$indexes;
		return $gl_from, $gl_to - $gl_from;
	} else {
		return wantarray ? ($gl_from, 0) : $gl_from;
	}
}

sub cluster2index
{
	my ( $self, $from ) = @_;

	my $indexes = $self->indexes;

	my $last = -1;
	my $curr_cluster = -1;
	$from = 0 if $from < 0;

	my $ix_from;
	for ( my $ix = 0; $ix < $#$indexes; $ix++) {
		my $c = $indexes->[$ix];
		next if $last == $c;
		$curr_cluster++;
		$last = $c;
		return $c if $from == $curr_cluster;
	}

	return $indexes->[-1];
}

sub cluster2range
{
	my ( $self, $from ) = @_;

	my $indexes = $self->indexes;
	my $first_index = $self->cluster2index($from);
	my $first       = $first_index & ~to::RTL;
	my $rtl         = ($first_index & to::RTL) ? 1 : 0;
	my $last        = $indexes->[-1];
	return 0,0,0 if $first >= $last;

	for ( my $ix = 0; $ix < $#$indexes; $ix++) {
		my $c = $indexes->[$ix] & ~to::RTL;
		$last = $c if $c > $first && $c < $last;
	}

	return $first, $last - $first, $rtl;
}

sub glyph2cluster
{
	my ( $self, $glyph ) = @_;

	return 0 if $glyph < 0;

	my $indexes  = $self->indexes;
	return 0 unless $#$indexes;

	my $curr_cluster = -1;
	my $last_index = -1;
	for ( my $ix = 0; $ix < $#$indexes; $ix++) {
		my $c = $indexes->[$ix] & ~to::RTL;
		if ( $last_index != $c ) {
			$curr_cluster++;
			$last_index = $c;
		}
		return $curr_cluster if $ix == $glyph;
	}

	return $curr_cluster + 1;
}

sub index2cluster
{
	my ( $self, $index, $rtl_advance ) = @_;

	return 0 if $index < 0;

	my $indexes  = $self->indexes;
	return 0 unless $#$indexes;

	my $curr_cluster = -1;
	my $last_index = -1;
	my $ret_cluster;
	# print "indexes($index): ", join(' ', map { ($_ & to::RTL) ? "-" . ($_ & ~to::RTL) : $_ } @{$indexes}[0..$#$indexes-1]), "\n";
	if ( $index >= $indexes->[-1] ) {
		# eol
		my $max_cluster = $indexes->[0] & ~to::RTL;
		$ret_cluster = ($indexes->[0] & to::RTL) ? 0 : 1;
		# print "max_cluster $max_cluster\n";
		for ( my $ix = 0; $ix < $#$indexes; $ix++) {
			my $c = $indexes->[$ix] & ~to::RTL;
			if ( $last_index != $c ) {
				$curr_cluster++;
				$last_index = $c;
			}
			if ($max_cluster < $c) {
				$max_cluster = $c;
				$ret_cluster = $curr_cluster;
				$ret_cluster += (($indexes->[$ix] & to::RTL) ? 0 : 1)
					if $rtl_advance;
				# print "ret $ret_cluster, max $c\n";
			}
		}
		return $ret_cluster + 1;
	} else {
		my $diff = 0xffff;

		for ( my $ix = 0; $ix < $#$indexes; $ix++) {
			my $c = $indexes->[$ix] & ~to::RTL;
			if ( $last_index != $c ) {
				$curr_cluster++;
				$last_index = $c;
			}
			if ( $index >= $c && $index - $c < $diff) {
				$ret_cluster = $curr_cluster;
				$ret_cluster += (($indexes->[$ix] & to::RTL) ? 0 : 1)
					if $rtl_advance;
				$diff = $index - $c;
			}
		}
	}

	return $ret_cluster;
}

sub get_sub
{
	my ( $self, $from, $length) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	my $glyphs = $self->[GLYPHS];
	return if $length <= 0;

	return $self if $from == 0 && $length == @$glyphs;

	my @sub;
	$sub[GLYPHS]    = Prima::array::substr($glyphs, $from, $length);
	$sub[INDEXES]   = Prima::array::substr($self->[INDEXES], $from, $length);
	push @{$sub[INDEXES]}, $self->[INDEXES]->[-1];

	if ( $self-> [ADVANCES] ) {
		$sub[ADVANCES]  = Prima::array::substr($self->[ADVANCES], $from, $length);
		$sub[POSITIONS] = Prima::array::substr($self->[POSITIONS], $from * 2, $length * 2);
	} else {
		$sub[ADVANCES]  = undef;
		$sub[POSITIONS] = undef;
	}

	if ( $self->[FONTS]) {
		$sub[FONTS] = Prima::array::substr($self->[FONTS], $from, $length);
	} else {
		$sub[FONTS]  = undef;
	}

	return __PACKAGE__->new( @sub );
}

sub get_sub_width
{
	my ( $self, $canvas, $from, $length ) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	return $canvas-> get_text_width($self, 0, $from, $length);
}

sub get_sub_box
{
	my ( $self, $canvas, $from, $length ) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	return $canvas-> get_text_box($self, $from, $length);
}

sub sub_text_out
{
	my ( $self, $canvas, $from, $length, $x, $y) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	return if $length <= 0;
	return $canvas-> text_out($self, $x, $y, $from, $length);
}

sub sub_text_wrap
{
	my ( $self, $canvas, $from, $length, $width, $opt, $tabs) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	return if $length <= 0;
	$opt //= tw::Default;
	$tabs //= 8;
	my $wrap = $canvas-> text_wrap($self, $width, $opt, $tabs, $from, $length);
	return $wrap unless $opt & tw::ReturnChunks;
	return $wrap unless defined $wrap;

	my $indexes = $self->[INDEXES];
	unless ( ref $wrap) {
		# n glyphs to n clusters
		my $last = -1;
		my $n_clusters = 0;
		for ( my $ix = 0; $ix < $wrap; $ix++) {
			my $c = $indexes->[$ix] & ~to::RTL;
			if ( $last != $c ) {
				$n_clusters++;
				$last = $c;
			}
		}
		return $n_clusters;
	}

	# convert glyph chunks to cluster chunks
	my @new_wrap;
	my $step = 0;
	for ( my $i = 0; $i < @$wrap; $i += 2 ) {
		my ( $ofs, $size ) = @{$wrap}[$i,$i+1];

		my $last = -1;
		my $n_clusters = 0;
		for ( my $ix = $ofs; $ix < $ofs + $size; $ix++) {
			my $c = $indexes->[$ix] & ~to::RTL;
			if ( $last != $c ) {
				$n_clusters++;
				$last = $c;
			}
		}
		push @new_wrap, $step, $n_clusters;
		$step += $n_clusters;
	}

	return \@new_wrap;
}

sub _map2chunks
{
	my $selection_map = shift;

	my @chunks;
	return [] unless @$selection_map;
	push @chunks, 0 if $selection_map->[0];
	my $last_selected = -1;
	for my $selected ( @$selection_map ) {
		if ( $selected == $last_selected ) {
			$chunks[-1]++;
		} else {
			push @chunks, 1;
			$last_selected = $selected;
		}
	}

	return \@chunks;
}

sub _chunks2map
{
	my @map;
	shift->selection_walk( shift, 0, undef, sub {
		my ( $offset, $length, $selected ) = @_;
		push @map, ($selected) x $length;
	});
	return \@map;
}

sub selection2range
{
	my ( $self, $cluster_start, $cluster_end ) = @_;

	($cluster_start, $cluster_end) = ($cluster_end, $cluster_start)
		if $cluster_end < $cluster_start;
	my ($s, $sl) = $self->cluster2range($cluster_start);
	my ($e, $el) = $self->cluster2range($cluster_end);
	($s,$sl,$e,$el) = ($e,$el,$s,$sl) if $s > $e;
	my $text_start = $s;
	my $text_end   = $e + $el - 1;
	($text_start, $text_end) = ($text_end, $text_start) if $text_start > $text_end;
	return ($text_start, $text_end);
}

sub selection_map_glyphs
{
	my ( $self, $text_start, $text_end ) = @_;

	my @selection_map;
	my $indexes = $self->indexes;
	for ( my $i = 0; $i < @$indexes; $i++) {
		my $ix = $indexes->[$i] & ~to::RTL;
		push @selection_map, ($ix >= $text_start && $ix <= $text_end) ? 1 : 0;
	}
	return \@selection_map;
}

sub selection_map_clusters
{
	my ( $self, $text_start, $text_end ) = @_;

	my @selection_map;
	my $clusters = $self->clusters;
	for ( my $i = 0; $i < @$clusters; $i++) {
		push @selection_map, ($clusters->[$i] >= $text_start && $clusters->[$i] <= $text_end) ? 1 : 0;
	}
	return \@selection_map;
}

sub selection_chunks_clusters { _map2chunks( selection_map_clusters( @_ )) }
sub selection_chunks_glyphs   { _map2chunks( selection_map_glyphs  ( @_ )) }

sub selection_diff
{
	my $self = shift;
	my ( $old, $new) = map { _chunks2map($self, $_) } @_;
	my @diff;
	my $max = ( @$old > @$new ) ? @$old : @$new;
	for ( my $i = 0; $i < $max; $i++) {
		$diff[$i] = (($old->[$i] // 0) == ($new->[$i] // 0) ) ? 0 : 1 ;
	}
	pop @diff while @diff and !$diff[-1];
	return _map2chunks( \@diff );
}

sub selection_walk
{
	my ( $self, $selection_chunks, $from, $to, $sub ) = @_;
	my ( $ptr, $selected ) = (0,1);
	for my $chunk ( @$selection_chunks ) {
		$selected = not $selected;
		my $offset = $ptr - $from;
		$ptr += $chunk;
		my $length = $chunk;
		if ( $offset < 0 ) {
			$length += $offset;
			$offset = 0;
		}
		if ( defined $to ) {
			last if $offset >= $to;
			$length = $from - $offset if $offset + $length > $to;
		}
		next if $length <= 0;
		$sub->( $offset, $length, $selected );
	}
}

sub is_strong($)   { $_[-1] =~ m/^[\p{bc=L}\p{bc=R}\p{bc=AL}]$/ }
sub is_weak($)     { $_[-1] =~ m/^[\p{bc=EN}\p{bc=ES}\p{bc=ET}\p{bc=AN}\p{bc=CS}\p{bc=NSM}\p{bc=BN}]$/ }
sub is_neutral($)  { $_[-1] =~ m/^[\p{bc=B}\p{bc=S}\p{bc=WS}\p{bc=ON}]$/ }
sub is_explicit($) { $_[-1] =~ m/^[\p{bc=LRE}\p{bc=LRO}\p{bc=RLE}\p{bc=RLO}\p{bc=PDF}\p{bc=LRI}\p{bc=RLI}\p{bc=FSI}\p{bc=PDI}]$/ }
sub is_rtl($)      { $_[-1] =~ m/^[\p{bc=R}\p{bc=AL}\p{bc=RLE}\p{bc=RLO}\p{bc=RLI}]$/ }
sub is_ltr($)      { $_[-1] =~ m/^[\p{bc=L}\p{bc=LRE}\p{bc=LRO}\p{bc=LRI}]$/ }

sub x2cluster
{
	my ( $self, $canvas, $width, $from, $length ) = @_;
	$from //= 0;
	$length //= -1;
	my $glyph = $canvas-> text_wrap($self, $width,
		tw::ReturnFirstLineLength | tw::BreakSingle, 8,
		$from, $length
	);
	return $self-> glyph2cluster($glyph);
}

sub cursor2offset
{
	my ( $self, $at_cluster, $preferred_rtl ) = @_;

	my $limit = $self-> n_clusters;
	return 0 unless $limit;
	my $maxlen = $self->[INDEXES]->[-1];

	my ($pos_left,$pos_right,$len_left,$len_right,$rtl_left,$rtl_right);
	($pos_left, $len_left, $rtl_left) = $self-> cluster2range( $at_cluster - 1 )
		if $at_cluster > 0;
	($pos_right, $len_right, $rtl_right) = $self-> cluster2range( $at_cluster )
		if $at_cluster <= $limit;

#	print "L: ",
#		(defined($pos_left) ? "pos:$pos_left len:$len_left rtl:$rtl_left" : "()"),
#		" R: ",
#		(defined($pos_right) ? "pos:$pos_right len:$len_right rtl:$rtl_right" : "()"),
#		"\n";

	if (defined $pos_left && defined $pos_right) {
		# cursor between two of the same direction
		return 
			$rtl_left ? 
				$len_right + $pos_right : 
				$len_left  + $pos_left
			if $rtl_left == $rtl_right;

		# cursor between two of different directions
		return $preferred_rtl ?
			$len_right + $pos_right :
			$len_left  + $pos_left;
	}

	# cursor at the start or at the end
	return $rtl_right ? $limit : 0 unless defined $pos_left;
	return $rtl_left ? 0 : $limit;
}

# poor man's kashida justifier, doesn't use the JSTF table from the font
sub justify_arabic
{
	my ($self, $canvas, $text, $width, %opt) = @_;
	return unless $text =~ /[\x{600}-\x{6ff}]/s;

	my $glyphs = $self->[GLYPHS];
	my $length = @$glyphs;
	return if @$glyphs < 2;

	# find suitable codepoints
	my $indexes = $self->[INDEXES];
	my @lengths = $self->index_lengths;
	my @kashidas;
	my $l2v     = $self->log2vis;

	reset $text;
	while ( 1 ) {
		$text =~ m/\G\s+/gcs and next;
		$text =~ m/\G$/gcs and last;
		$text =~ m/\G(\S+)/gcs or return;
		my $m = $1;
		next unless $m =~ /[\x{600}-\x{6ff}]/;
		my ($from, $to) = @$l2v[(pos($text) - length($m), pos($text) - 1)];
		($to, $from) = ($from, $to) if $from > $to;

		for ( my $i = $from; $i < $to; $i++) {
			my ( $ix0, $ix1 ) = @$indexes[$i,$i+1];

			# don't justify LTR arabic
			next unless $ix0 & to::RTL;
			$ix0 &= ~to::RTL;
			$i++, next unless $ix1 & to::RTL;
			$ix1 &= ~to::RTL;

			my $chr = substr($text, $ix0, $lengths[$i]);
			# all cps marked as LETTER from https://www.unicode.org/charts/PDF/U0600.pdf
			next if $chr !~ /^[\x{620}-\x{64A}\x{674}-\x{6D3}\x{6D5}\x{6EE}\x{6EF}\x{6FA}-\x{6FC}\x{6FF}]+$/;

			$chr = substr($text, $ix1, $lengths[$i]);
			$i++, next if $chr !~ /^[\x{620}-\x{64A}\x{674}-\x{6D3}\x{6D5}\x{6EE}\x{6EF}\x{6FA}-\x{6FC}\x{6FF}]+$/;
			push @kashidas, [
				scalar(@kashidas), # original index to retain when sorting
				$ix0,              # codepoint index
				$i + 1,            # glyph index
				0                  # number of tatweels inserted
			];
			last;
		}
	}
	return unless @kashidas;

	# calculate how many tatweels to insert
	my $curr_width = $canvas-> get_text_width( $self, to::AddOverhangs );
	my $expansion  = $width - $curr_width;
	my $min_width  = delete $opt{min_kashida} // 0;
	$min_width = 0 if $min_width < 0;
	return if $expansion <= 0;

	my $k_width;
	if ( defined $opt{kashida_width}) {
		$k_width = $opt{kashida_width};
	} else {
		my $ins = $canvas-> text_shape( "\x{640}",
			advances => 1,
			reorder  => 0,
			rtl      => 0,
			level    => ts::Glyphs,
		) or return 0;
		$k_width    = $ins->[ADVANCES]->[0];
	}
	return unless $k_width;

	my $n_tatweels = int($expansion / $k_width);
	my @ins_points = sort { $b->[1] <=> $a->[1] } @kashidas;
	my @tatweel_lengths;
	my $avg_tatweel = $n_tatweels / @kashidas;
	my $diff = 0;
	for ( my $i = 0; $i < @ins_points; $i++) {
		my $n_tats = int($avg_tatweel + $diff);
		$n_tats = 0 if $min_width > 0 && $n_tats * $k_width < $min_width;
		substr($text, $ins_points[$i]->[1], 0, "\x{640}" x $n_tats) if $n_tats > 0;
		$diff += $avg_tatweel - $n_tats;
		$kashidas[ $ins_points[$i]->[0] ]->[3] = $n_tats;
	}

	return wantarray ? ($text, $k_width) : $text if $opt{as_text};

	my $new = $canvas->text_shape($text, %opt);
	return unless $new;

	$indexes      = $new->[INDEXES];
	my $advances  = $new->[ADVANCES]  or goto SUCCESS;
	my $positions = $new->[POSITIONS] or goto SUCCESS;

	# move glyph references after new tatweels
	my $dg = 0;
	for my $k (sort { $a->[2] <=> $b->[2] } @kashidas) {
		$k->[2] += $dg;
		$dg += $k->[3];
	}

	$length = @$glyphs;
	for my $k ( @kashidas ) {
		my (undef, undef, $at_glyph, $n_tatweels) = @$k;

		# are tatweels rendered as monotonically increased indexes?
		for (
			my ($i,$cmp) = ($at_glyph, $indexes->[$at_glyph] & ~to::RTL);
			$i < $at_glyph + $n_tatweels;
			$i++, $cmp--
		) {
			my $ix = $indexes->[$i];
			return unless $ix & to::RTL;
			$ix &= ~to::RTL;
			return unless $ix == $cmp;
		}

		# fix clusters
		my $left_cluster = $indexes->[$at_glyph - 1];
		my $right_cluster = $indexes->[$at_glyph + $n_tatweels - 1] & ~to::RTL;
		for ( my $i = $at_glyph; $i < $at_glyph + $n_tatweels; $i++) {
			$indexes->[$i] = $left_cluster;
			$advances->[$at_glyph - 1] += $advances->[$i];
			$positions->[$i * 2] -= $advances->[$i] * ($n_tatweels + $at_glyph - $i);
			$advances->[$i] = 0;
		}

		# fix text references
		$length = @$indexes;
		for ( my $i = 0; $i < $length; $i++) {
			my $ix = $indexes->[$i];
			my $fl = $ix & to::RTL;
			$ix &= ~to::RTL;
			next if $ix < $right_cluster;
			$indexes->[$i] = ($ix - $n_tatweels) | $fl;
		}
	}

SUCCESS:
	@$self = @$new;
	return wantarray ? (1, $k_width) : 1;
}

sub justify_interspace
{
	my ($self, $canvas, $text, $width, %opt) = @_;
	my $interletter = $opt{letter} // 1;
	my $interword   = $opt{word}   // 1;
	return unless $interletter || $interword;

	my $curr_width  = $canvas->get_text_width($self, to::AddOverhangs);
	return if $curr_width > $width || $curr_width == 0;
	my $advances = $self->[ADVANCES] or return 0;
	my $indexes = $self->[INDEXES];
	my $n_glyphs = scalar @{$self->[GLYPHS]};

	if ( $opt{as_text} ) {
		return unless $interword && $width > $curr_width;
		my $spaces = scalar @{[$text =~ m/\s+/g]} or return;
		my $space_width;
		for ( my $i = 0; $i < $n_glyphs; $i++) {
			my $ix = $indexes->[$i] & ~to::RTL;
			next unless substr($text, $ix, 1) =~ /\s/;
			$space_width = $advances->[$i];
			last;
		}
		my $avg_space_incr   = ($width - $curr_width) / ($spaces * $space_width);
		my $accumulated_incr = 0.0;
		reset $text;
		my @insertions;
		while ( 1 ) {
			$text =~ m/\G\S+/gcs and next;
			$text =~ m/\G$/gcs and last;
			$text =~ m/\G(\s+)/gcs or return;
			my $dx = int( $avg_space_incr + $accumulated_incr );
			unshift @insertions, pos($text), $dx;
			$accumulated_incr += $avg_space_incr - $dx;
		}
		for ( my $i = 0; $i < @insertions; $i += 2 ) {
			substr($text, $insertions[$i], 0, ' ' x $insertions[$i+1]);
		}
		return wantarray ? ($text, $space_width) : $text;
	}

	# (Bringhurst 2008) suggests about 3% expansion or contraction of
	# intercharacter spacing and about 2% expansion or contraction of
	# glyphs as the largest permissible deviations -- Wikipedia
	if ( $interletter) {
		my $diff  = 1.0 + ($width - $curr_width) / $curr_width;
		my $max   = $opt{max_interletter} // 1.05;
		my $dw    = ( $diff > $max ) ? $max : $diff;
		for ( my $i = 0; $i < $n_glyphs; $i++) {
			my $xa = $advances->[$i];
			$curr_width -= $xa;
			$curr_width += $advances->[$i] = int($xa * $dw);
		}
	}

	# the rest goes between words
	my $spaces = scalar @{[$text =~ m/\s+/g]};
	if ( $interword && $width > $curr_width && $spaces) {
		my $avg_space_incr   = ($width - $curr_width) / $spaces;
		my $accumulated_incr = 0.0;
		for ( my $i = 0; $i < $n_glyphs; $i++) {
			my $ix = $indexes->[$i] & ~to::RTL;
			next unless substr($text, $ix, 1) =~ /\s/;

			my $dx = int( $avg_space_incr + $accumulated_incr );
			$advances->[$i]   += $dx;
			$accumulated_incr += $avg_space_incr - $dx;
		}
	}

	return 1;
}

sub justify_tabs
{
	my ($self, $canvas, $text, %opt) = @_;
	return unless $text =~ /\t/s;
	my $glyphs   = $self->[GLYPHS]   or return;
	my $indexes  = $self->[INDEXES]  or return;
	my $advances = $self->[ADVANCES] or return;
	my $fonts    = $self->[FONTS];
	my $length   = @$advances;
	my $ntabs    = $opt{tabs} // 8;
	my ($space, $width);
	if ( exists $opt{glyph}) {
		$space = $opt{glyph};
		$width = $opt{width};
	} elsif ( my $s = $canvas->text_shape("\x{20}",
			advances => 1,
			reorder  => 0,
			rtl      => 0,
			level    => ts::Glyphs,
		)
	) {
		$space = $s->[GLYPHS]->[0];
		$width = $s->[ADVANCES]->[0];
	} else {
		return;
	}
	$width *= $ntabs;
	for ( my $i = 0; $i < $length; $i++) {
		my $ix = $indexes->[$i] & ~to::RTL;
		next unless substr($text, $ix, 1) eq "\t";
		$glyphs->[$i]    = $space;
		$advances->[$i]  = $width;
		$fonts->[$i]     = 0 if $fonts;
	}
	return wantarray ? ( 1, $space, $width ) : 1;
}

sub justify
{
	my ($self, $canvas, $text, $width, %opt) = @_;
	my $ok = 0;
	$ok |= ($self->justify_arabic($canvas, $text, $width, %opt) || 0) if
		$opt{kashida};
	$ok |= ($self->justify_interspace($canvas, $text, $width, %opt) || 0) if
		$opt{letter} || $opt{word};
	$ok |= ($self->justify_tabs($canvas, $text, %opt) || 0) if
		exists $opt{tabs};
	return $ok;
}

1;

=pod

=head1 NAME

Prima::Drawable::Glyphs - helper routines for bi-directional text input and complex scripts output

=head1 SYNOPSIS

=encoding utf-8

=for latex-makedoc header
\usepackage{amsmath,amssymb}
\DeclareFontFamily{U}{rcjhbltx}{}
\DeclareFontShape{U}{rcjhbltx}{m}{n}{<->rcjhbltx}{}
\DeclareSymbolFont{hebrewletters}{U}{rcjhbltx}{m}{n}
\DeclareMathSymbol{\alef}{\mathord}{hebrewletters}{39}
\DeclareMathSymbol{\pe}{\mathord}{hebrewletters}{112}
\DeclareMathSymbol{\samekh}{\mathord}{hebrewletters}{115}

=begin latex-makedoc

=begin latex

\begin{tt}
~ ~\\
\hspace*{1.5em}use Prima;\\
\hspace*{1.5em}\$::application->begin\_paint;\\
\hspace*{1.5em}\$application->text\_out('$\alef\pe\samekh123$',100,100);\\
 \\
\hspace*{1.5em}123$\samekh\pe\alef$\\
\end{tt}

=end latex

=end latex-makedoc

=for latex-makedoc cut

   use Prima;
   $::application-> begin_paint;
   ‭$::application-> text_shape_out('אפס123', 0,0);

   ‭123ספא

=for latex-makedoc cut

=head1 DESCRIPTION

The class implements an abstraction over a set of glyphs that can be rendered
to represent text strings. Objects of the class are created and returned from
C<Prima::Drawable::text_shape> calls, see more in
L<Prima::Drawable/text_shape>. A C<Prima::Drawable::Glyphs> object is a blessed
array reference that can contain either two, four, or five packed arrays with
16-bit integers, representing, correspondingly, a set of glyph indexes, a set
of character indexes, a set of glyph advances, a set of glyph position offsets
per glyph, and a font index. Additionally, the class implements several sets of
helper routines that aim to address common tasks when displaying glyph-based
strings.

=head2 Structure

Each sub-array is an instance of C<Prima::array>, an effective plain memory
structure that provides standard perl interface over a string scalar filled
with fixed-width integers. 

The following methods provide read-only access to these arrays:

=over

=item glyphs

Contains a set of unsigned 16-bit integers where each is a glyph number
corresponding to the font that was used for shaping the text. These glyph
numbers are only applicable to that font. Zero is usually treated as a default
glyph in vector fonts, when shaping cannot map a character; in bitmap fonts
this number is usually same as C<defaultChar>.

This array is recognized as a special case when is sent to C<text_out> or
C<get_text_width>, that can process it without other arrays. In this case, no
special advances and glyph positions are taken into the account though.

Each glyph is not necessarily mapped to a character, and quite often is not,
even in english left-to-right texts. F ex character combinations like C<"ff">,
C<"fi">, C<"fl"> may be mapped to single ligature glyphs. When right-to-left, I<RTL>,
text direction is taken into the account, the glyph positions may change, too.
See C<indexes> below that addresses mapping of glyphs to characters.

=item indexes

Contains a set of unsigned 16-bit integers where each is a text offset corresponding to 
the text was used in shaping. Each glyph position thus points to a first character
in the text that maps to the glyph.

There can be more than one character per glyph, such as the above example
with a C<"ff"> ligature. There can also be cases with more than one character
per more than one glyph, f ex in indic scripts. In these cases it
is easier to operate neither by character offsets nor by glyph offsets, but rather
by I<clusters>, where each cluster is an individual syntax unit that contains one or
more characters per one or more glyphs.

In addition to the text offset, each index value can be flagged with a
C<to::RTL> bit, signifying that the character in question has RTL direction.
This is not necessarily semitic characters from RTL languages that only have
that attribute set; spaces in these languages are normally attributed the RTL
bit too, sometimes also numbers. Use of explicit direction control characters
from U+20XX block can result in any character being assigned or not assigned
the RTL bit.

The array has an extra item added to its end, the length of the text that was
used for the shaping. This helps for easy calculation of cluster length in
characters, especially of the last one, where the difference between indexes is,
basically, the cluster length.

The array is not used for text drawing or calculation, but only for conversion
between character, glyph, and cluster coordinates (see C<Coordinates> below).

=item advances

Contains a set of unsigned 16-bit integers where each is a pixel distance of
how much space the corresponding glyph occupies. Where the advances array is
not present, or was force-filled by C<advances> options in C<text_shape>, a
glyph advance value is basically a sum of a, b, and c widths of the corresponding glyph.
However there are cases when depending on shaping input, these values can
differ.

One of those cases is the combining graphemes, where the text consisting of two
characters, C<"A"> and combining grave accent U+300 should be drawn as a single
"E<Agrave>" symbol, and where the font doesn't have that single glyph but rather two
individual glyphs C<"A"> and C<"`">. There, where the grave glyph has its own
advance for standalone usage, in this case it should be ignored though, and
that is achieved by the shaper setting the advance of the C<"`"> to zero.

The array content is respected by C<text_out> and C<get_text_width>, and its
content can be changed at will to produce gaps in the text quite easily. F ex
C<Prima::Edit> uses that to display tab characters as spaces with 8x advance.

=item positions

Contains a set of pairs of signed 16-bit integers where each is a X and Y pixel
offset for each glyph. Like in the previous example with the "E<Agrave>"
symbol, the grave glyph C<"`"> may be positioned differently on the vertical axis in
"E<Agrave>" and "E<agrave>" graphemes, for example.

The array is respected by C<text_out> (but not by C<get_text_width>).

=item fonts

Contains a set of unsigned 16-bit integers where each is an index in the font
substitution list (see L<Prima::Drawable/fontMapperPalette>). Zero means the
current font.

The font substitution is applied by C<text_shape> when C<polyfont> options is
set (it is by default), and when the shaper cannot match all fonts. If the
current font contains all needed glyphs, this entry is not present at all.

The array is respected by C<text_out> and C<get_text_width>.

=back

=head2 Coordinates

In addition to the natural character coordinates, where each index is a text
offset that can be directly used in C<substr> perl function, the
C<Prima::Drawable::Glyphs> class offers two additional coordinate systems that
help abstract the object data for display and navigation.

The glyph coordinate system is a rather straighforward copy of the character
coordinate system, where each number is an offset in the C<glyphs> array. Similarly,
these offsets can be used to address individual glyphs, indexes, advances, and
positions. However these are not easy to use when one needs, for example, to
select a grapheme with a mouse, or break set of glyphs in such a way so that a
grapheme is not broken. These can be managed easier in the cluster coordinate system.

The cluster coordinates represent a virtually superimposed set of offsets where each
corresponds to a set of one or more characters displayed by a one or more
glyphs. Most useful functions below operate in this system.

=head2 Selection

Practically, most useful coordinates that can be used for implementing selection
is either character or cluster, but not glyphs. The charater-based selections makes
trivial extraction or replacement of the selected text, while the cluster-based makes
it easier to manipulate (f ex with Shift- arrow keys) the selection itself.

The class supports both, by operating on I<selection maps> or I<selection
chunks>, where each represent same information but in different ways.
For example, consider embedded number in a bidi text. For the sake of clarity
I'll use latin characters here. Let's have a text scalar containing
these characters:

   ABC123

where I<ABC> is right-to-left text, and which, when rendered on screen, should be
displayed as

   123CBA

(and index array is (3,4,5,2,1,0) ).

Next, the user clicks the mouse between A and B (in text offset 1), drags the
mouse then to the left, and finally stops between characters 2 and 3 (text
offset 4). The resulting selection then should not be, as one might naively
expect, this:

   123CBA
   __^^^_

but this instead:

   123CBA
   ^^_^^_

because the next character after C is 1, and the I<range> of the selected
sub-text is from characters 1 to 4.

The class offers to encode such information in a I<map>, i.e. array of integers
C<1,1,0,1,1,0>, where each entry is either 0 or 1 depending on whether the
cluster is or is not selected.  Alternatively, the same information can be
encoded in I<chunks>, or RLE sets, as array C<0,2,1,2,1>, where the first
integer signifies number of non-selected clusters to display, the second -
number of selected clusters, the third the non-selected again, etc. If the
first character belongs to the selected chunk, the first integer in the result
is set to 0.

=head2 Bidi input

When sending input to a widget in order to type in text, the otherwise trivial
case of figuring out at which position the text should be inserted (or removed,
for that matter), becomes interesting when there are characters with mixed
direction.

F ex it is indeed trivial, when the latin text is C<AB>, and the cursor is
positioned between C<A> and C<B>, to figure out that whenever the user types
C<C>, the result should become C<ACB>. Likewise, when the text is RTL and both
text and input is arabic, the result is the same. However when f.ex. the text
is C<A1>, that is displayed as C<1A> because of RTL shaping, and the cursor is
positioned between C<1> (LTR) and C<A> (RTL), it is not clear whether that
means the new input should be appended after C<1> and become C<A1C>, or after
C<A>, and become, correspondingly, C<AC1>.

There is no easy solution for this problem, and different programs approach
this differently, and some go as far as to provide two cursors for both
directions. The class offers its own solution that uses some primitive
heuristics to detect whether cursor belongs to the left or to the right glyph.
This is the area that can be enhanced, and any help from native users of RTL
languages can be greatly appreciated.

=head1 API

=over

=item abc $CANVAS, $INDEX

Returns a, b, c metrics from the glyph C<$INDEX>

=item advances

Read-only accessor to the advances array, see L<Structure> above.

=item clone

Clones the object

=item cluster2glyph $FROM, $LENGTH

Maps a range of clusters starting with C<$FROM> with size C<$LENGTH> into the
corresponding range of glyphs. Undefined C<$LENGTH> calculates the range from
C<$FROM> till the object end.

=item cluster2index $CLUSTER

Returns character offset of the first character in cluster C<$CLUSTER>.

Note: result may contain C<to::RTL> flag.

=item cluster2range $CLUSTER

Returns character offset of the first character in cluster C<$CLUSTER>
and how many characters are there in the cluster.

=item clusters

Returns array of integers where each is a first character offsets per cluster.

=item cursor2offset $AT_CLUSTER, $PREFERRED_RTL

Given a cursor positioned next to the cluster C<$AT_CLUSTER>, runs simple heuristics
to see what character offset it corresponds to. C<$PREFERRED_RTL> is used when object
data are not enough.

See L<Bidi input> above.

=item def $CANVAS, $INDEX

Returns d, e, f metrics from the glyph C<$INDEX>

=item fonts

Read-only accessor to the font indexes, see L<Structure> above.

=item get_box $CANVAS

Return box metrics of the glyph object.

See L<Prima::Drawable/get_text_box>.

=item get_sub $FROM, $LENGTH

Extracts and clones a new object that constains data from cluster offset
C<$FROM>, with cluster length C<$LENGTH>.

=item get_sub_box $CANVAS, $FROM, $LENGTH

Calculate box metrics of a glyph string from the cluster C<$FROM> with size C<$LENGTH>.

=item get_sub_width $CANVAS, $FROM, $LENGTH

Calculate pixel width of a glyph string from the cluster C<$FROM> with size C<$LENGTH>.

=item get_width $CANVAS, $WITH_OVERHANGS

Return width of the glyph objects, with overhangs if requested.

=item glyph2cluster $GLYPH

Return the cluster that contains C<$GLYPH>.

=item glyphs

Read-only accessor to the glyph indexes, see L<Structure> above.

=item glyph_lengths

Returns array where each glyph position is set to a number showing how many glyphs the
cluster occupies at this position

=item index2cluster $INDEX

Returns the cluster that contains the character offset C<$INDEX>.

=item indexes

Read-only accessor to the indexes, see L<Structure> above.

=item index_lengths

Returns array where each glyph position is set to a number showing how many characters the
cluster occupies at this position

=item justify CANVAS, TEXT, WIDTH, %OPTIONS

Umbrella call for C<justify_interspace> if C<$OPTIONS{letter}> or
C<$OPTIONS{word}> if set; for C<justify_arabic> if C<$OPTIONS{kashida}> is
set; and for C<justify_tabs> if C<$OPTIONS{tabs}> is set.

Returns a boolean flag whether the glyph object was changed or not.

=item justify_arabic CANVAS, TEXT, WIDTH, %OPTIONS

Performs justifications of arabic TEXT with kashida to the given WIDTH, returns
either success flag, or new text with explicit I<tatweel> characters inserted.

   my $text = "\x{6a9}\x{634}\x{6cc}\x{62f}\x{647}";
   my $g = $canvas->text_shape($text) or return;
   $canvas->text_out($g, 10, 50);
   $g->justify_arabic($canvas, $text, 200) or return;
   $canvas->text_out($g, 10, 10);

=for podview <img src="Prima/kashida.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/kashida.gif">

Inserts tatweels only between arabic letters that did not form any ligatures in
the glyph object, max one tatweel set per word (if any). Does not apply the
justification if the letters in the word are rendered as LTR due to embedding
or explcit shaping options; only does justification on RTL letters. If for some
reason newly inserted tatweels do not form a monotonically increasing series
after shaping, skips the justifications in that word.

Note: Does not use JSTF font table, on Windows results may be different from native rendering.

Options:

If justification is found to be needed, eventual ligatures with newly inserted
tatweel glyphs are resolved via a call to C<text_shape(%OPTIONS)> - so any
needed shaping options, such as C<language>, may be passed there.

=over

=item as_text BOOL = 0

If set, returns new text with inserted tatweels, or undef if no justification
is possible.

If unset, runs inplace justification on the caller glyph object,
and returns the boolean success flag.

=item min_kashida INTEGER = 0

Specifies minimal width of a kashida strike to be inserted.

=item kashida_width INTEGER

During the calculation a width of a tatweel glyph is needed - unless supplied
by this option, it is calculated dynamically. Also, when called in list
context, and succeeded, returns C< 1, kashida_width > that can be reused in
subsequent calls.

=back

=item justify_interspace CANVAS, TEXT, WIDTH, %OPTIONS

Performs inplace inter-letter and/or inter-word justifications of TEXT to the
given WIDTH. Returns either a boolean flag whether there were any change made,
or, new text with explicit space characters inserted.

Options:

=over

=item as_text BOOL = 0

If set, returns new text with inserted spaces, or undef if no justification is
possible.

If unset, runs inplace justification on the caller glyph object, and returns
the boolean success flag.

=item letter BOOL = 1

If set, runs an inter-letter spacing on all glyphs.

=item max_interletter FLOAT = 1.05

When the inter-letter spacing is applied, it is applied first, and can take up
to C<$OPTIONS{max_interletter} * glyph_width> space.

Inter-word spacing does not have such limit, and in worst case, can produce two
words moved to the left and to the right edges of the enclosing 0 - WIDTH-1
rectangle.

=item space_width INTEGER

C<as_text> mode: during the calculation the width of space glyph may be needed
- unless supplied by C<$OPTIONS{space_width}>, it is calculated dynamically.
Also, when called in list context, and succeeded, returns C< 1, space_width >
that can be reused in subsequent calls.

=item word BOOL = 1

If set, runs an inter-word spacing by extending advances on all space glyphs.

=back

=item justify_tabs CANVAS, TEXT, %OPTIONS

Expands tabs as C<$OPTIONS{tabs}> (default:8) spaces.

Needs glyph and the advance of the space glyph to replace the tab glyph.
If no C<$OPTIONS{glyph}> and C<$OPTIONS{width}> are specified, calculates them.

Returns a boolean flag whether there were any change made. On success, if
called in the list context, returns also space glyph ID and space glyph width
for eventual use on the later calls.

=item left_overhang

First integer from the C<overhangs> result.

=item log2vis

Returns a map of integers where each character position corresponds to a glyph
position. The name is a rudiment from pure fribidi shaping, where C<log2vis>
and C<vis2log> were mapper functions with the same functionality.

=item n_clusters

Calculates how many clusters the object contains.

=item new @ARRAYS

Create new object. Not used directly, but rather from inside C<text_shape> calls.

=item new_array NAME

Creates an array suitable for the object for direct insertion, if manual
construction of the object is needed. F ex one may set missing C<fonts> array
like this:

   $obj->[ Prima::Drawable::Glyphs::FONTS() ] = $obj->new_array('fonts');
   $obj->fonts->[0] = 1;

The newly created array is filled with zeros.

=item new_empty

Creates a new empty object.

=item overhangs

Calculates two pixel widths for overhangs in the beginning and in the end of the glyph string.
This is used in emulation of a C<get_text_width> call with the C<to::AddOverhangs> flag.

=item positions

Read-only accessor to the positions array, see L<Structure> above.

=item reorder_text TEXT

Returns a visual representation of C<TEXT> assuming it was the input of the
C<text_shape> call that created the object.

=item reverse

Creates a new object that has all arrays reversed. User for calculation
of pixel offset from the right end of a glyph string.

=item right_overhang

Second integer from the C<overhangs> result.

=item selection2range $CLUSTER_START $CLUSTER_END

Converts cluster selection range into text selection range

=item selection_chunks_clusters, selection_chunks_glyphs $START, $END

Calculates a set of chunks of texts, that, given a text selection from
positions C<$START> to C<$END>, represent each either a set of selected and
non-selected clusters/glyphs.

=item selection_diff $OLD, $NEW

Given set of two chunk lists, in format as returned by
C<selection_chunks_clusters> or C<selection_chunks_glyphs>, calculates the list
of chunks affected by the selection change. Can be used for efficient repaints
when the user interactively changes text selection, to redraw only the changed
regions.

=item selection_map_clusters, selection_map_glyphs $START, $END

Same as C<selection_chunks_XXX>, but instead of RLE chunks returns full array for each
cluster/glyph, where each entry is a boolean value corresponding to whether that cluster/glyph is
to be displayed as selected, or not.

=item selection_walk $CHUNKS, $FROM, $TO = length, $SUB

Walks the selection chunks array, returned by C<selection_chunks>, between
C<$FROM> and C<$TO> clusters/glyphs, and for each chunk calls the provided C<<
$SUB->($offset, $length, $selected) >>, where each call contains 2 integers to
chunk offset and length, and a boolean flag whether the chunk is selected or
not.

Can be also used on a result of C<selection_diff>, in which case
C<$selected> flag is irrelevant.

=item sub_text_out $CANVAS, $FROM, $LENGTH, $X, $Y

Optimized version of C<< $CANVAS->text_out( $self->get_sub($FROM, $LENGTH), $X, $Y ) >>.

=item sub_text_wrap $CANVAS, $FROM, $LENGTH, $WIDTH, $OPT, $TABS

Optimized version of C<< $CANVAS->text_wrap( $self->get_sub($FROM, $LENGTH), $WIDTH, $OPT, $TABS ) >>.
The result is also converted to chunks.

=item text_length

Returns the length of the text that was shaped and that produced the object.

=item x2cluster $CANVAS, $X, $FROM, $LENGTH

Given sub-cluster from C<$FROM> with size C<$LENGTH>, calculates how many
clusters would fit in width C<$X>.

=item _debug

Dumps glyph object content in a readable format.

=back

=for latex-makedoc cut

=head1 EXAMPLES

This section is only there to test proper rendering

=over

=item Latin

B<Lorem ipsum> dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.

   Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.

=item Latin combining

D̍üi̔s͙ a̸u̵t͏eͬ ịr͡u̍r͜e̥ d͎ǒl̋o̻rͫ i̮n̓ r͐e̔p͊rͨe̾h̍e͐n̔ḋe͠r̕i̾t̅ ịn̷ vͅo̖lͦuͦpͧt̪ątͅe̪ 

   v̰e̷l̳i̯t̽ e̵s̼s̈e̮ ċi̵l͟l͙u͆m͂ d̿o̙lͭo͕r̀e̯ ḛu̅ fͩuͧg̦iͩa̓ť n̜u̼lͩl͠a̒ p̏a̽r̗i͆a͆t̳űr̀

=item Cyrillic

B<Lorem Ipsum> используют потому, что тот обеспечивает более или менее стандартное заполнение шаблона.

  а также реальное распределение букв и пробелов в абзацах

=item Hebrew

זוהי עובדה מבוססת שדעתו של הקורא תהיה מוסחת על ידי טקטס קריא כאשר הוא יביט בפריסתו.

  המטרה בשימוש ב-Lorem Ipsum הוא שיש לו פחות או יותר תפוצה של אותיות, בניגוד למלל

=item Arabic

العديد من برامح النشر المكتبي وبرامح تحرير صفحات الويب تستخدم لوريم إيبسوم بشكل إفتراضي 

  كنموذج عن النص، وإذا قمت بإدخال "lorem ipsum" في أي محرك بحث ستظهر العديد من


=item Hindi

Lorem Ipsum के अंश कई रूप में उपलब्ध हैं, लेकिन बहुमत को किसी अन्य रूप में परिवर्तन का सामना करना पड़ा है, हास्य डालना या क्रमरहित शब्द , 

  जो तनिक भी विश्वसनीय नहीं लग रहे हो. यदि आप Lorem Ipsum के एक अनुच्छेद का उपयोग करने जा रहे हैं, तो आप को यकीन दिला दें कि पाठ के मध्य में वहाँ कुछ भी शर्मनाक छिपा हुआ नहीं है.

=item Chinese

无可否认，当读者在浏览一个页面的排版时，难免会被可阅读的内容所分散注意力。

  Lorem Ipsum的目的就是为了保持字母多多少少标准及平

=item Largest well-known grapheme cluster in Unicode

ཧྐྵྨླྺྼྻྂ

L<http://archives.miloush.net/michkap/archive/2010/04/28/10002896.html>.

=back

=for latex-makedoc cut

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

F<examples/bidi.pl>

=cut
