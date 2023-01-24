package Prima::Widget::Time;

use strict;
use warnings;
use base qw(Prima::InputLine);
use POSIX qw(strftime);

sub profile_default
{
	return {
		%{ $_[0]->SUPER::profile_default },
		time      => $_[0]->time2hms(time),
		format    => $_[0]->default_format,
	};
}

sub init
{
	my $self = shift;
	my %profile = @_;
	$self->{time}   = [0,0,0];
	$self->{format} = '';
	$self->{mask}   = [''];
	$profile{indertMode} = 0;
	%profile = $self-> SUPER::init(%profile);
	$self-> $_($profile{$_}) for qw(format time);
	$self-> {insertMode} = 0;
}

sub time2hms { [ (localtime($_[1]))[0,1,2] ] }
sub now      { $_[0]-> time2hms(time) }

sub time2str
{
	my $self = shift;
	my @time = ref($_[0]) ? @{ $_[0] } : @_;
	my $mask  = $self->{mask};
	my @pp;
	if ( defined $mask->[4]) {
		if ( $time[2] == 0 ) {
			@time[2,3] = (12, 'AM');
		} elsif ( $time[2] == 12 ) {
			$time[3] = 'PM';
		} elsif ( $time[2] < 12 ) {
			$time[3] = 'AM';
		} else {
			$time[3] = 'PM';
			$time[2] -= 12;
		}
		$time[3] = lc $time[3] unless $mask->[9];
		@pp = map { defined ? $time[$_] : () } @$mask[1..4];
	} else {
		@pp = map { defined ? $time[$_] : () } @$mask[1..3];
	}
	return sprintf $mask->[0], @pp;
}

sub str2time
{
	my ( $self, $str ) = @_;
	my @offsets = @{ $self->{mask} }[ 5..8 ];
	my @lengths = (2,2,2,2);
	my @ret     = @{ $self-> now };
	for ( my $i = 0; $i < 4; $i++) {
		next unless defined $offsets[$i];
		next if length($str) < $offsets[$i] + $lengths[$i];
		my $n = substr($str, $offsets[$i], $lengths[$i] );
		if ( $i == 3 ) {
			next if $n !~ /^[ap]m$/i;
			if ( lc($n) eq 'am' && $ret[2] == 12 ) {
				$ret[2] = 0;
			} elsif ( lc($n) eq 'pm' && $ret[2] < 12 ) {
				$ret[2] += 12;
			}
		} else {
			next if $n !~ /^\d+$/;
			$ret[$i] = $n;
		}
	}

	@ret = $self->validate_time(@ret);
	return \@ret;
}

sub default_format
{
	 my $f = strftime("%X", 33, 22, 11, 4, 4, 66);
	 $f =~ s/11/hh/;
	 $f =~ s/22/mm/;
	 $f =~ s/33/ss/;
	 $f =~ s/[ap]m/aa/;
	 $f =~ s/[AP]M/AA/;
	 return $f;
}

sub format
{
	return $_[0]->{format} unless $#_;

	my ( $self, $format ) = @_;
	$self->{format} = $format;

	my $offset = 0;
	my %print;
	my $uc_ampm;
	my @offsets;
	my $fmt    = '';
	local $_ = $format;
	while ( 1 ) {
		if ( !defined $print{h} && m/\G(hh)/gcs ) {
			$fmt     .= "%02d";
			$print{h} = $offset++;
			$offsets[2] = pos($_) - 2;
		} elsif ( !defined $print{m} && m/\G(mm)/gcs ) {
			$fmt     .= "%02d";
			$print{m} = $offset++;
			$offsets[1] = pos($_) - 2;
		} elsif ( !defined $print{s} && m/\G(ss)/gcs ) {
			$fmt     .= "%02d";
			$print{s} = $offset++;
			$offsets[0] = pos($_) - 2;
		} elsif ( !defined $print{a} && m/\G(aa)/gcs ) {
			$fmt     .= "%02s";
			$print{a} = $offset++;
			$offsets[3] = pos($_) - 2;
		} elsif ( !defined $print{a} && m/\G(AA)/gcs ) {
			$fmt     .= "%02s";
			$print{a} = $offset++;
			$offsets[3] = pos($_) - 2;
			$uc_ampm = 1;
		} elsif ( m/\G([hmsA]+|[^hmsA]+)/gcs ) {
			my $r    = $1;
			$r       =~ s/%/%%/g;
			$fmt    .= $r;
		} elsif ( m/\G$/gcs ) {
			last;
		}
	}
	$self->{mask} = [ $fmt, @print{qw(s m h a)}, @offsets, $uc_ampm ];
	$self-> text( $self-> time2str( $self->time ) );
}

sub validate_time
{
	my ($self, $sec, $min, $hour) = @_;
	$sec  = 59 if $sec > 59;
	$sec  = 0  if $sec < 0;
	$min  = 59 if $min > 59;
	$min  = 0  if $min < 0;
	$hour = 0  if $hour < 0;
	$hour = 23 if $hour > 23;
	return $sec, $min, $hour;
}

sub time
{
	return @{$_[0]-> {time}} unless $#_;
	my $self = shift;
	my ($s, $m, $h) = $#_ ? @_ : @{$_[0]} ;
	($s, $m, $h) = $self->validate_time($s, $m, $h);
	my @ot = @{$self-> {time}};
	return if $s == $ot[0] && $m == $ot[1] && $h == $ot[2];
	$self-> {time} = [ $s, $m, $h ];
	$self-> text( $self-> time2str( $self->{time} ) );
}

sub time_plusminus_positional
{
	my ($self, $direction) = @_;
	my $tofs    = $self->cursor2text_offset($self->charOffset);
	my @offsets = @{ $self->{mask} }[ 5..8 ];
	my @lengths = (2,2,2,2);
	my $str     = $self-> text;
	my @time    = $self-> time;
	for ( my $i = 0; $i < 4; $i++) {
		next unless defined $offsets[$i];
		next if length($str) < $offsets[$i] + $lengths[$i];
		next if $tofs < $offsets[$i] or $tofs > $offsets[$i] + $lengths[$i];
		my $n = substr($str, $offsets[$i], $lengths[$i] );

		if ( $i == 3 ) {
			next if $n !~ /^[ap]m$/i;
		} else {
			next if $n !~ /^\d+$/;
		}

		if ( defined $self->{mask}->[4] && $i == 2) {
			if (length($str) >= $offsets[3] + $lengths[3]) {
				my $ampm = substr($str, $offsets[3], $lengths[3] );
				if ( lc($ampm) eq 'am' and $n == 12 ) {
					$n = 0;
				} elsif ( lc($ampm) eq 'pm' and $n < 12) {
					$n += 12;
				}
			}
		}

		if ( $i == 3 ) {
			$time[2] = ($time[2] + 12) % 24;
		} else {
			$n += $direction;
			$time[$i] = $n;

			$time[0] = 0,  $time[1]++ if $time[0] > 59;
			$time[0] = 59, $time[1]-- if $time[0] < 0;
			$time[1] = 0 , $time[2]++ if $time[1] > 59;
			$time[1] = 59, $time[2]-- if $time[1] < 0;
			$time[2] = 0  if $time[2] > 23;
			$time[2] = 23 if $time[2] < 0;
		}
		$self-> time(@time);
		return 1;
	}

	return 0;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;

	if ($key == kb::Down || $key == kb::Up) {
		$self-> time_plusminus_positional( ($key == kb::Down) ? -1 : 1);
		$self->clear_event;
		return;
	}

	$self->SUPER::on_keydown($code, $key, $mod, $repeat);
}

sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	return $self->clear_event if $self-> time_plusminus_positional(( $z > 0 ) ? 1 : -1);
}

sub on_validate
{
	my ( $self, $textref) = @_;
	my $time  = $self->str2time($$textref);
	my $newt  = $self->time2str($time);
	return if lc($newt) eq lc($$textref); # don't blink though on am/AM mismatch input
	$$textref = $newt;
	$self->blink;
}

sub on_change  { @{ $_[0]->{time} } = @{ $_[0]->str2time( $_[0]->text ) } }
sub insertMode { $#_ ? shift->SUPER::insertMode(0) : shift->SUPER::insertMode }

1;
