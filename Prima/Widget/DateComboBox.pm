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
use Prima qw(EventHook Utils);

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self->{calendar} = $self-> insert( Calendar =>
		pack     => { fill => 'both', expand => 1 },
		current  => 1,
	);
	$self->{calendar}->Day-> onMouseClick( sub {
		my ( undef, $btn, $mod, $x, $y, $nth ) = @_;
		return if $nth != 2;
		$self-> owner-> date( $self-> date );
		$self-> owner-> listVisible(0);
	});
	$self->packPropagate(1);
	return %profile;
}

sub before_show
{
	my ( $self, $w ) = @_;
	my $gsw = $self->calendar->geomWidth;
	$self-> width(( $w < $gsw ) ? $gsw : $w);
}

sub hook_proc
{
	my $self = shift;

	Prima::Utils::alarm(0.1, sub {
		my $f = $::application->get_focused_widget;
		return if $f && $self->is_owner($f) != 0;
		$self-> deinstall_hook;
		$self-> owner-> listVisible(0);
	});
}

sub deinstall_hook { Prima::EventHook::deinstall(\&hook_proc) }

sub on_show
{
	my ( $self ) = @_;
	Prima::Utils::post( sub { $self->calendar->select } );

	Prima::EventHook::install( \&hook_proc,
		event    => 'Leave',
		children => 1,
		object   => $self,
		param    => $self,
	);
}

sub on_translateaccel
{
	my ( $self, $chr, $key, $mod ) = @_;
	$self-> notify(q(KeyDown), $chr, $key, $mod);
}

sub calendar { $_[0]->{calendar} }
sub date     { $#_ ? shift->calendar->date(@_) : shift->calendar->date }

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
	$profile{style} = cs::DropDown;
	@{$profile{listDelegations}} = grep { $_ ne 'SelectItem' } @{$profile{listDelegations}};
	$profile{listProfile}->{growMode} = 0;
	%profile = $self-> SUPER::init(%profile);
	$self-> $_($profile{$_}) for qw(format date);
	$self->List->selectable(1);
}

sub style { cs::DropDown }

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
	if ( $nlv ) {
		my $l = $self-> List;
		$l-> date( $self-> date );
		$l-> before_show( $self-> width );
	}
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

sub List_Click
{
	my ( $self, $list) = @_;
	$self-> date( $list-> date );
	$self-> listVisible(0);
	$self-> notify( q(Change));
}

1;
