package Prima::Drawable::Glyphs;

use strict;
use warnings;
use Carp qw(cluck);
use Prima;

use constant GLYPHS    => 0;
use constant INDEXES   => 1;
use constant ADVANCES  => 2;
use constant POSITIONS => 3;

sub new         { bless [grep { defined } @_[1..4]], $_[0] }
sub glyphs      { $_[0]->[GLYPHS] }
sub indexes     { $_[0]->[INDEXES] }
sub advances    { $_[0]->[ADVANCES] }
sub positions   { $_[0]->[POSITIONS] }
sub text_length { $_[0]->[INDEXES]->[-1] }

sub new_empty   { bless [[],[0]], $_[0] }

sub overhangs
{
	return @{$_[0]->[ADVANCES]}[-2,-1] if $_[0]->[2];
	my ( $self, $canvas ) = @_;
	cluck "cannot query glyph metrics without canvas" unless $canvas;
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
	my $advances = $self->[ADVANCES];
	if ( $advances ) {
		my $x = 0;
		$x += $_ for @$advances;
		return $x;
	} else {
		cluck "cannot calculate glyph width without canvas" unless $canvas;
		return $canvas-> get_text_width($self->[GLYPHS]);
	}
}

sub get_box
{
	my ( $self, $canvas ) = @_;
	return $canvas-> get_text_box($self->[GLYPHS]);
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
	my $last        = $indexes->[-1] & ~to::RTL;
	return 0,0,0 if $first >= $last;

	for ( my $ix = 0; $ix < $#$indexes; $ix++) {
		my $c = $indexes->[$ix] & ~to::RTL;
		$last = $c if $c > $first && $c < $last;
	}

	return $first, $last - $first, $rtl;
}

sub offset2cluster
{
	my ( $self, $index ) = @_;

	return 0 if $index <= 0;

	my $indexes  = $self->indexes;
	return 0 unless $#$indexes;

	my $curr_cluster = -1;
	my $last_index = -1;
	my $ret_cluster;
	if ( $index >= $indexes->[-1] ) {
		# eol
		my $max_cluster = $indexes->[0] & ~to::RTL;
		$ret_cluster = ($indexes->[0] & to::RTL) ? -1 : 0;
		for ( my $ix = 0; $ix < $#$indexes; $ix++) {
			my $c = $indexes->[$ix] & ~to::RTL;
			if ( $last_index != $c ) {
				$curr_cluster++;
				$last_index = $c;
			}
			if ($max_cluster < $c) {
				$max_cluster = $c;
				$ret_cluster = $curr_cluster + (($indexes->[$ix] & to::RTL) ? -1 : 0);
			}
		}
		return 1 + $ret_cluster;
	} else {
		my $diff = 0xffff;
		for ( my $ix = 0; $ix < $#$indexes; $ix++) {
			my $c = $indexes->[$ix] & ~to::RTL;
			if ( $last_index != $c ) {
				$curr_cluster++;
				$last_index = $c;
			}
			if ( $index >= $c && $index - $c < $diff) {
				$ret_cluster = $curr_cluster + (($indexes->[$ix] & to::RTL) ? 1 : 0);
				$diff = $index - $c;
			}
		}
	}

	return $ret_cluster;
}

sub get_sub_width
{
	my ( $self, $canvas, $from, $length ) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	return 0 if $length <= 0;
	my $advances = $self->[ADVANCES];
	if ( $advances ) {
		my $x = 0;
		my $to = $from + $length - 1;
		$x += $advances->[$_] for $from .. $to;
		return $x;
	} else {
		cluck "cannot calculate glyph width without canvas" unless $canvas;
		return $canvas-> get_text_width(Prima::array::substr($self->[GLYPHS], $from, $length));
	}
}

sub get_sub_box
{
	my ( $self, $canvas, $from, $length ) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	return [0,0,0,0,0] if $length <= 0;
	return $canvas-> get_text_box(Prima::array::substr($self->[GLYPHS], $from, $length));
}

sub get_sub
{
	my ( $self, $canvas, $from, $length) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	my $glyphs = $self->[GLYPHS];
	return if $length <= 0;

	return $self if $from == 0 && $length == @$glyphs;

	my @sub;
	$sub[GLYPHS]    = Prima::array::substr($glyphs, $from, $length);
	$sub[INDEXES]   = Prima::array::substr($self->[INDEXES], $from, $length);
	my $max_index   = 0;
	my $next_index  = $self->[INDEXES]->[-1]; # next index (or text length) after 
	for my $ix ( map { $_ & ~to::RTL } @{ $sub[INDEXES] } ) {
		$max_index = $ix if $max_index < $ix;
	}
	for my $ix ( map { $_ & ~to::RTL } @{ $self->[INDEXES] } ) {
		$next_index = $ix if $ix > $max_index && $ix < $next_index;
	}
	push @{$sub[INDEXES]}, $next_index;

	my $sub = __PACKAGE__->new( @sub );
	if ( $self-> [ADVANCES] ) {
		my @ovx = $sub-> overhangs($canvas);
		$sub[ADVANCES]  = Prima::array::substr($self->[ADVANCES], $from, $length);
		$sub[POSITIONS] = Prima::array::substr($self->[POSITIONS], $from * 2, $length * 2);
		push @{ $sub[ADVANCES] }, @ovx;
	}

	return $sub;
}

sub sub_text_out
{
	my ( $self, $canvas, $from, $length, $x, $y) = @_;
	($from, $length) = $self-> cluster2glyph($from, $length);
	return if $length <= 0;

	return $canvas-> text_out( $self, $x, $y)
		if $from == 0 && $length == @{ $self->[GLYPHS] };
	if ( $self->advances ) {
		# quick hack
		my @sub;
		$sub[GLYPHS]    = Prima::array::substr($self->[GLYPHS], $from, $length);
		$sub[ADVANCES]  = Prima::array::substr($self->[ADVANCES], $from, $length);
		$sub[POSITIONS] = Prima::array::substr($self->[POSITIONS], $from * 2, $length * 2);
		return $canvas-> text_out(\@sub, $x, $y);
	} else {
		return $canvas-> text_out(Prima::array::substr($self->[GLYPHS], $from, $length), $x, $y);
	}
}

sub sub_text_wrap
{
	my ( $self, $canvas, $from, $length, $width, $opt, $tabs) = @_;
	$opt //= tw::Default;
	$tabs //= 8;
	my $sub = $self-> get_sub($canvas, $from, $length) or return [];
	return $canvas-> text_wrap($sub, $width, $opt, $tabs);
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
		push @{ $svs[-1] }, reverse @{ $self->[INDEXES] }[0 .. $nglyphs-3];
		push @{ $svs[-1] }, reverse @{ $self->[INDEXES] }[$nglyphs-2, $nglyphs-1];
		
		push @svs, Prima::array->new('S');
		my $positions = $self->[POSITIONS];
		for ( my $i = $nglyphs - 1; $i >= 0; $i -= 2 ) {
			push @{ $svs[-1] }, @{$positions}[$i,$i+1];
		}
	}

	return __PACKAGE__->new(@svs);
}

sub _map2chunks
{
	my $selection_map = shift;

	my @chunks;
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

sub selection_chunks
{
	my ( $self, $text_start, $text_end ) = @_;

	my $clusters = $self->clusters;
	($text_start, $text_end) = ($text_end, $text_start) if $text_start > $text_end;
	my @selection_map;
	for ( my $i = 0; $i < @$clusters; $i++) {
		push @selection_map, ($clusters->[$i] >= $text_start && $clusters->[$i] <= $text_end) ? 1 : 0;
	}
	return _map2chunks( \@selection_map );
}

sub selection_diff
{
	my $self = shift;
	my ( $old, $new) = map { _chunks2map($self, $_) } @_;
	my @diff;
	my $max = ( @$old > @$new ) ? @$old : @$new;
	for ( my $i = 0; $i < $max; $i++) {
		$diff[$i] = (($old->[$i] // 0) == ($new->[$i] // 0) ) ? 0 : 1 ;
	}
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

sub cursor2offset
{
	my ( $self, $at_cluster, $preferred_rtl ) = @_;

	my $limit = $self-> n_clusters;
	return 0 unless $limit;
	my $maxlen = $self->[INDEXES]->[-1];

	my ($pl,$pr, $ll,$lr, $rl,$rr); # position,length,rtl flag for left/right
	if ( $at_cluster > 0 ) {
		($pl, $ll, $rl) = $self-> cluster2range( $at_cluster - 1 );
	}
	if ( $at_cluster <= $limit ) {
		($pr, $lr, $rr) = $self-> cluster2range( $at_cluster );
	}

	if (defined $pl && defined $pr) {
		# cursor between two of the same direction
		return ($rl ? $lr : $ll) + ($rl ? $pr : $pl) if $rl == $rr;
		# cursor between two of different directions
		return ($preferred_rtl ? $rr : $lr) + ($preferred_rtl ? $pr : $pl);
	}
	
	# cursor at the start or at the end
	return $rr ? $limit : 0 unless defined $pl;
	return $rl ? 0 : $limit;
}

1;

