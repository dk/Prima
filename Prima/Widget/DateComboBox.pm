use strict;
use warnings;

package Prima::Widget::DateComboBox::Input;
use base qw(Prima::InputLine);

sub insertMode { $#_ ? shift->SUPER::insertMode(0) : shift->SUPER::insertMode }

sub on_validate
{
	my ( $self, $textref) = @_;
	my $owner = $self->owner;
	my $date  = $owner->str2date($$textref);
	my $newt  = $owner->date2str($date);
	return if $newt eq $$textref;
	$$textref = $newt;
	$self->blink;
}

package Prima::Widget::DateComboBox::List;
use base qw(Prima::Widget);

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self->{calendar} = $self-> insert( Calendar =>
		origin   => [0,0],
		size     => [ $self->size ],
		growMode => gm::Client,
		onChange => sub { $_[0]->owner->owner->date( $_[0]-> date ) },
	);
	return %profile;
}

sub calendar { $_[0]->{calendar} }
sub date     { $#_ ? shift->calendar->date(@_) : shift->calendar }

package Prima::Widget::DateComboBox;

use POSIX qw(strftime);
use base qw(Prima::ComboBox);
use Prima qw(Calendar);

sub profile_default
{
	return {
		%{ $_[0]->SUPER::profile_default },
		date      => $_[0]->time2date(time),
		format    => $_[0]->default_format,
		style     => cs::DropDown,
		editClass => 'Prima::Widget::DateComboBox::Input',
		listClass => 'Prima::Widget::DateComboBox::List',
	};
}

sub init
{
	my $self = shift;
	my %profile = @_;
	$self->{date}   = [1,1,1];
	$self->{format} = '';
	$self->{mask}   = [''];
	@{$profile{listDelegations}} = grep { $_ ne 'SelectItem' } @{$profile{listDelegations}};
	%profile = $self-> SUPER::init(%profile);
	$self-> $_($profile{$_}) for qw(format date);
	$self->List->selectable(1);
}

sub time2date { [ (localtime($_[1]))[3,4,5] ] }
sub today     { $_[0]-> time2date(time) }

sub date2str
{
	my ( $self, $date ) = @_;

	my @p = @$date;
	$p[2] += 1900;
	$p[1] ++;

	my $mask  = $self->{mask};
	my @pp    = map { defined ? $p[$_] : () } @$mask[1..3];
	return sprintf $mask->[0], @pp;
}

sub str2date
{
	my ( $self, $str ) = @_;
	my @offsets = @{ $self->{mask} }[ 4..6 ];
	my @lengths = (2,2,4);
	my @add     = (0,1,1900);
	my @ret     = @{ $self-> today };
	for ( my $i = 0; $i < 3; $i++) {
		next unless defined $offsets[$i];
		next if length($str) < $offsets[$i] + $lengths[$i];
		my $n = substr($str, $offsets[$i], $lengths[$i] );
		next if $n !~ /^\d+$/;
		$ret[$i] = $n - $add[$i];
	}
	@ret = $self->validate_date(@ret);
	return \@ret;
}

sub default_format
{
	 my $f = strftime("%x", 0, 0, 0, 22, 10, 99);
	 $f =~ s/1999/YYYY/;
	 $f =~ s/11/MM/;
	 $f =~ s/22/DD/;
	 return $f;
}

sub format
{
	return $_[0]->{format} unless $#_;

	my ( $self, $format ) = @_;
	warn "Need YYYY in DateComboBox::format" if $format =~ /(Y+)/ and length($1) != 4;
	warn "Need MM in DateComboBox::format" if $format =~ /(M+)/ and length($1) != 2;
	warn "Need DD in DateComboBox::format" if $format =~ /(D+)/ and length($1) != 2;
	$self->{format} = $format;

	my $offset = 0;
	my %print;
	my @offsets;
	my $fmt    = '';
	local $_ = $format;
	while ( 1 ) {
		if ( !defined $print{y} && m/\G(YYYY)/gcs ) {
			$fmt     .= "%04d";
			$print{y} = $offset++;
			$offsets[2] = pos($_) - 4;
		} elsif ( !defined $print{m} && m/\G(MM)/gcs ) {
			$fmt     .= "%02d";
			$print{m} = $offset++;
			$offsets[1] = pos($_) - 2;
		} elsif ( !defined $print{d} && m/\G(DD)/gcs ) {
			$fmt     .= "%02d";
			$print{d} = $offset++;
			$offsets[0] = pos($_) - 2;
		} elsif ( m/\G([YMD]+|[^YMD]+)/gcs ) {
			my $r    = $1;
			$r       =~ s/%/%%/g;
			$fmt    .= $r;
		} elsif ( m/\G$/gcs ) {
			last;
		}
	}
	$self->{mask} = [ $fmt, @print{qw(d m y)}, @offsets ];
}

sub validate_date
{
	my ($self, $day, $month, $year) = @_;
	$month = 11 if $month > 11;
	$month = 0  if $month < 0;
	$year = 0   if $year < 0;
	$year = 199 if $year > 199;
	$day = 1 if $day < 1;
	my $v = $Prima::Calendar::days_in_months[ $month] +
		(((( $year % 4) == 0) && ( $month == 1)) ? 1 : 0);
	$day = $v if $day > $v;
	return $day, $month, $year;
}

sub date
{
	return @{$_[0]-> {date}} unless $#_;
	my $self = shift;
	my ($day, $month, $year) = $#_ ? @_ : @{$_[0]} ;
	($day, $month, $year) = $self->validate_date($day, $month, $year);
	my @od = @{$self-> {date}};
	return if $day == $od[0] && $month == $od[1] && $year == $od[2];
	$self-> {date} = [ $day, $month, $year ];
	$self-> text( $self-> date2str( $self->{date} ) );
}

sub set_list_visible
{
	my ( $self, $nlv ) = @_;
	$self-> List-> date( $self-> date ) if $nlv;
	$self-> SUPER::set_list_visible($nlv);
}

sub InputLine_Change {}
sub InputLine_Leave  {}

sub InputLine_MouseWheel
{
	my ( $self, $edit, $mod, $x, $y, $z) = @_;

	my $tofs    = $edit->cursor2text_offset($edit->charOffset);
	my @offsets = @{ $self->{mask} }[ 4..6 ];
	my @lengths = (2,2,4);
	my $str     = $edit-> text;
	my @date    = $self-> date;
	my @add     = (0,1,1900);
	for ( my $i = 0; $i < 3; $i++) {
		next unless defined $offsets[$i];
		next if length($str) < $offsets[$i] + $lengths[$i];
		next if $tofs < $offsets[$i] or $tofs > $offsets[$i] + $lengths[$i];
		my $n = substr($str, $offsets[$i], $lengths[$i] );
		next if $n !~ /^\d+$/;
		$n += ( $z > 0 ) ? -1 : 1;
		$date[$i] = $n - $add[$i];

		$edit-> clear_event;
		$self-> date(@date);
	}
}

1;
