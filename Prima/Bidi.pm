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
		} elsif ( $p eq ':locale') {
			# http://stackoverflow.com/questions/18996183/identifyng-rtl-language-in-android 
			$default_direction_rtl = ( $ENV{LANG} =~ /^(
				ar| # arabic
				dv| # divehi
				fa| # persian (farsi)
				ha| # hausa
				he| # hebrew
				iw| # hebrew (old code)
				ji| # yiddish (old code)
				ps| # pashto, pushto
				ur| # urdu
				yi  # yiddish
			)/x ? 1 : 0)
				if defined $ENV{LANG};
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
		eval "use Text::Bidi::Paragraph; use Text::Bidi::Constants;";
		if ( $@ ) {
			$failure_text = "Bi-directional text services not available: $@\n";
			$available = $enabled = 0;
			return $failure_text;
		}
		$available = 1;
	}
	return ( $enabled = $available ) ? undef : $failure_text;
}

sub visual { scalar paragraph(@_) }

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
	return ($p, join("\n", @text));
}

sub _par
{
	my ( $text, @opt ) = @_;
	my $p = Text::Bidi::Paragraph->new( $text, @opt );
	my $off = 0;
	my $width = $p->len;
	while ( $off < $width ) {
		my $v = $p->visual($off, $width);
		my $l = length($v);
		$off += $l;
	}
	return $p;
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

sub is_strong($) { $_[0] & $Text::Bidi::Mask::STRONG }
sub is_weak($)   { !($_[0] & $Text::Bidi::Mask::STRONG) }
sub is_rtl($)    { $_[0] & $Text::Bidi::Mask::RTL    }
sub is_ltr($)    { !($_[0] & $Text::Bidi::Mask::RTL) }

sub edit_insert
{
	my ( $src_p, $visual_pos, $new_str, $rtl ) = @_;
	
	# non-bidi compatibility
	return length($new_str) unless $src_p;

	my $new_p     = _par($new_str);
	my $map       = $src_p->map;
	my $t         = $src_p->types;
	my $new_types = $new_p->types;
	my $limit     = $#$map;
	
	# XXX - just a simple method, please extend if you know how
	if ( $rtl ) {
		if ( $new_str =~ /^\p{bc=R}+$/ ) {
			return $visual_pos, 0;
		} else {
			return $visual_pos, 1;
		}
	} else {
		if ( $new_str =~ /^\p{bc=R}+$/ ) {
			return $visual_pos, 0;
		} else {
			return $visual_pos, 1;
		}
	}
}

sub edit_delete
{
	my ( $p, $visual_pos, $backspace ) = @_;

	# non-bidi compatibility
	return ($visual_pos > 0) ? (1, $visual_pos - 1, -1) : (0, 0, 0) unless $p;

	my $map   = $p->map;
	my $t     = $p->types;
	my $limit = $#$map;

	my ($il,$ir,$l,$r,$pl,$pr) = (0,0,0,0,$limit,0);

	if ( $visual_pos > 0 ) {
		$il = $t->[$map->[$pl = $visual_pos - 1]];
		$pl-- while is_weak($l = $t->[$map->[$pl]]) && $pl > 0;
	}
	if ( $visual_pos <= $limit ) {
		$ir = $t->[$map->[$pr = $visual_pos]];
		$pr++ while is_weak($r = $t->[$map->[$pr]]) && $pr < $limit;
	}

	#warn "il: ", (is_strong($il) ? 'strong' : 'weak'), ' ', (is_rtl($il) ? 'rtl' : 'ltr'), " at ", $visual_pos - 1, "\n";
	#warn "ir: ", (is_strong($ir) ? 'strong' : 'weak'), ' ', (is_rtl($ir) ? 'rtl' : 'ltr'), " at ", $visual_pos, "\n";
	#warn "l: ", (is_strong($l) ? 'strong' : 'weak'), ' ', (is_rtl($l) ? 'rtl' : 'ltr'), " at $pl\n";
	#warn "r: ", (is_strong($r) ? 'strong' : 'weak'), ' ', (is_rtl($r) ? 'rtl' : 'ltr'), " at $pr\n";
	#warn "vp: $visual_pos, limit: $limit\n";

	if ( $backspace ) {
		# strong ltr immediately left, kill immediately left
		return 1, $map->[$visual_pos - 1], -1 if $visual_pos > 0       && is_strong $il && is_ltr $il;
		# strong rtl immediately right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && is_strong $ir && is_rtl $ir;
		# any ltr immediately left, kill immediately left
		return 1, $map->[$visual_pos - 1], -1 if $visual_pos > 0 && is_ltr $il;
		# any rtl on right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && is_rtl $r;
		# any rtl on left, kill greedy leftmost
		if ($visual_pos > 0 && is_rtl $l) {
			my $L = $l;
			$pl-- while !(is_strong($L = $t->[$map->[$pl]]) && is_ltr $L) && $pl > 0;
			return 1, $map->[$pl], $pl - $visual_pos
		}
	} else {
		# strong ltr immediately right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && is_strong $il && is_ltr $il;
		# strong rtl immediately left, kill immediately left
		return 1, $map->[$visual_pos - 1], 0  if $visual_pos > 0       && is_strong $ir && is_rtl $ir;
		# any ltr immediately right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && is_ltr $ir;
		# any rtl on left, kill immediately left
		return 1, $map->[$visual_pos - 1], -1 if $visual_pos > 0 && is_rtl $l;
		# any ltr on right, kill greedy rightmost
		if ($visual_pos <= $limit && is_ltr $r) {
			my $R = $r;
			$pr-- while !(is_strong($R = $t->[$map->[$pr]]) && is_rtl $R) && $pr > 0;
			return 1, $map->[$pr], $pr - $visual_pos
		}
	}

	# nothing
	return 0, 0, 0;
}

sub debug_str
{
	my ( $p, $str ) = @_;
	my $t = $p->types;
	my $b = $p->bd;
	for ( my $i = 0; $i < length($str); $i++) {
		my $chr = ord( substr( $str, $i, 1));
		my $typ = $t->[$i];
		my $tn  = $b->get_bidi_type_name($typ);
		my @mas;
		no strict 'refs';
		for my $name ( keys %Text::Bidi::Mask::) {
			my $value = ${"Text::Bidi::Mask::$name"};
			next unless $typ & $value;
			push @mas, $name;
		}
		printf("$i: %03x: %06x / %s\n", $chr, $typ, join(',', @mas));
	}
}

1;
