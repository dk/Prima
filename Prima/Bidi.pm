package Prima::Bidi;

use strict;
use warnings;
our $available;
our $enabled = 0;
our $failure_text;
our $default_direction_rtl = 0;

sub import
{
	shift;
	for my $p ( @_ ) {
		if ( $p eq ':require' ) {
			my $error = enabled(1);
			die $error if $error;
		} elsif ( $p eq ':enable') {
			enabled(1);
		} elsif ( $p eq ':rtl') {
			$default_direction_rtl = 1;
		} elsif ( $p eq ':ltr') {
			$default_direction_rtl = 0;
		} else {
			die "no such keyword: $p\n";
		}
	}
}

sub enabled
{
	return $enabled unless @_;
	return $enabled = 0 unless $_[0];
	unless ( defined $available ) {
		eval "use Text::Bidi::Paragraph;";
		if ( $@ ) {
			$failure_text = "Bi-directional text services not available: $@\n";
			$available = $enabled = 0;
			return $failure_text;
		}
		$available = 1;
	}
	return ( $enabled = $available ) ? undef : $failure_text;
}

sub visual { paragraph(@_)->{text} }

sub paragraph
{
	my ( $text, $rtl, $flags ) = @_;
	my $p = Text::Bidi::Paragraph->new( $text,
		dir => $rtl ? $Text::Bidi::Par::RTL : $Text::Bidi::Par::LTR
	);
	my $off = 0;
	my $width = $p->len;
	my @text;
	while ( $off < $width ) {
		my $v = $p->visual($off, $width, $flags);
		my $l = length($v);
		$off += $l;
		push @text, $v;
	}
	return {
		text => join("\n", @text),
		map  => $p->map,
		rtl  => $p->is_rtl,
	};
}

sub selection_chunks
{
	my ( $map, $start, $end, $offset ) = @_;
	$offset //= 0;
	return [scalar @$map] if $offset > @$map || $start > $end;
	$start = 0      if $start < 0;
	$end   = 0      if $end   < 0;
	$start = $#$map if $start > $#$map;
	$end   = $#$map if $end   > $#$map;
	my ($text_start, $text_end) = @$map[$start, $end];
	($text_start, $text_end) = ($text_end, $text_start) if $text_start > $text_end;
	my @selection_map;
	for ( my $i = $offset; $i < @$map; $i++) {
		push @selection_map, ($map->[$i] >= $text_start && $map->[$i] <= $text_end) ? 1 : 0;
	}
	# warn "$start:$end > $text_start:$text_end > @selection_map\n";

	return _map2chunks( \@selection_map );
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
	selection_walk( shift, 0, undef, sub {
		my ( $offset, $length, $selected ) = @_;
		push @map, ($selected) x $length;
	});
	return \@map;
}

sub selection_diff
{
	my ( $old, $new) = map { _chunks2map($_) } @_;
	my @diff;
	for ( my $i = 0; $i < @$new; $i++) {
		$diff[$i] = (($old->[$i] // 0) == $new->[$i] ) ? 0 : 1 ;
	}
	return _map2chunks( \@diff );
}

sub selection_walk
{
	my ( $selection_chunks, $from, $to, $sub ) = @_;
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

1;
