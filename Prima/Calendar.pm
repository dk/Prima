use strict;
use warnings;

use Prima;
use Prima::Classes;
use Prima::Label;
use Prima::ComboBox;
use Prima::Sliders;

package Prima::Calendar;
use vars qw(@ISA @non_locale_months @days_in_months $OB_format);
@ISA = qw(Prima::Widget);

my $posix_state;

my @non_locale_months = qw(
	January February March April May June
	July August September October November December);

@days_in_months = (31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31);

sub profile_default
{
	my @date = (localtime(time))[3,4,5];
	return {
		%{$_[ 0]-> SUPER::profile_default},
		scaleChildren  => 0,
		date           => \@date,
		useLocale      => 1,
		day            => $date[0],
		month          => $date[1],
		year           => $date[2],
		firstDayOfWeek => 0,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> {date} = $default-> {date} unless exists $p-> {date};
	$p-> {date}-> [0] = $p-> {day}   if exists $p-> {day};
	$p-> {date}-> [1] = $p-> {month} if exists $p-> {month};
	$p-> {date}-> [2] = $p-> {year}  if exists $p-> {year};
	$self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
	my $self = shift;
	$self-> {$_} = 0 for qw( day month year useLocale firstDayOfWeek);
	$self-> {date} = [0,0,0];
	my %profile = $self-> SUPER::init(@_);
	$self-> {useLocale} = can_use_locale() if $profile{useLocale};

	my $fh = $self-> font-> height;
	my ( $w, $h) = $self-> size;
	$self-> reset_days;

	my $right_offset = $self-> font-> width * 9;
	$self-> insert( ComboBox =>
		origin   => [ 5, $h - $fh * 2 - 10 ],
		size     => [ $w - $right_offset - 15, $fh + 4],
		name     => 'Month',
		items    => $self-> make_months,
		style    => cs::DropDownList,
		growMode => gm::GrowHiX | gm::GrowLoY,
		delegations => [ 'Change' ],
	);

	$self-> insert( Label =>
		origin => [ 5, $h - $fh - 4],
		size   => [ $w - $right_offset - 15, $fh + 2],
		text   => '~Month',
		focusLink => $self-> Month,
		growMode => gm::GrowHiX | gm::GrowLoY,
	);

	$self-> insert( SpinEdit =>
		origin => [ $w - $right_offset - 5, $h - $fh * 2 - 10 ],
		size   => [ $right_offset, $fh + 4],
		name   => 'Year',
		min    => 1900,
		max    => 2099,
		growMode => gm::GrowLoX | gm::GrowLoY,
		delegations => [ 'Change' ],
	);

	$self-> insert( Label =>
		origin => [ $w - $right_offset - 5, $h - $fh - 4],
		size   => [ $right_offset, $fh + 2],
		text   => '~Year',
		focusLink => $self-> Year,
		growMode => gm::GrowLoX | gm::GrowLoY,
	);

	$self-> insert( Widget =>
		origin      => [ 5, 5 ],
		size        => [ $w - 10, $h - $fh * 3 - 22 ],
		name        => 'Day',
		selectable  => 1,
		current     => 1,
		delegations => [ qw(
			Paint MouseDown MouseMove MouseUp MouseWheel MouseLeave
			KeyDown Size FontChanged Enter Leave
		)],
		growMode    => gm::Client,
	);

	$self-> insert( Label =>
		origin => [ 5, $h - $fh * 3 - 15],
		size   => [ $w - 10, $fh + 2],
		text   => '~Day',
		focusLink => $self-> Day,
		growMode => gm::GrowHiX | gm::GrowLoY,
	);
	$self-> $_($profile{$_}) for qw( date useLocale firstDayOfWeek);
}


sub can_use_locale
{
	return $posix_state if defined $posix_state;
	undef $@;
	eval "use POSIX q(strftime);";
	return $posix_state = 1 unless $@;
	return $posix_state = 0;
}

sub month2str
{
	return $non_locale_months[$_[1]] unless $_[0]-> {useLocale};
	return POSIX::strftime ( "%B", 0, 0, 0, 1, $_[1], 0 );
}

sub make_months
{
	return \@non_locale_months unless $_[0]-> {useLocale};
	unless ( defined $OB_format) {
		# %OB is a BSD extension for locale-specific date string
		# for use without date
		$OB_format = (
			POSIX::strftime ( "%OB", 0, 0, 0, 1, 0, 0 ) eq
			POSIX::strftime ( "%OB", 0, 0, 0, 1, 1, 0 )
		) ? '%B' : '%OB';
	}
	return [ map {
		POSIX::strftime ( $OB_format, 0, 0, 0, 1, $_, 0 )
	} 0 .. 11 ];
}

sub day_of_week
{
	my ( $self, $day, $month, $year, $useFirstDayOfWeek) = @_;
	$month++; $year += 1900;

	$useFirstDayOfWeek = 1 unless defined $useFirstDayOfWeek;

	if ( $month < 3) {
		$month += 10;
		$year--;
	} else {
		$month -= 2;
	}
	my $century = int($year / 100);
	$year %= 100;
	my $dow = ( int(( 26 * $month - 2) / 10) + $day + $year + int($year / 4) +
		int($century / 4) - ( 2 * $century) -
		(( $useFirstDayOfWeek ? 1 : 0) * $self-> {firstDayOfWeek}) + 7)
		% 7;
	return ($dow < 0) ? $dow + 7 : $dow;
}

sub reset_days
{
	my $self = $_[0];
	my $dow = $self-> {firstDayOfWeek};
	$self-> {days} = $self-> {useLocale} ?
	[ map { strftime("%a", 0, 0, 0, $_, 0, 0) } 0 .. 6 ] :
	[ qw( Sun Mon Tue Wed Thu Fri Sat ) ];
	push @{$self-> {days}}, splice( @{$self-> {days}}, 0, $dow) if $dow;
}

sub useLocale
{
	return $_[0]-> {useLocale} unless $#_;
	my ( $self, $useLocale) = @_;
	$useLocale = can_use_locale if $useLocale;
	$self-> {useLocale} = $useLocale;
	$self-> Month-> items( $self-> make_months);
	$self-> Month-> text( $self-> Month-> List-> get_item_text( $self-> Month-> focusedItem));
	$self-> reset_days;
	my $day = $self-> Day;
	$self-> day_reset( $day, $day-> size);
	$day-> repaint;
}

sub Day_Paint
{
	my ( $owner, $self, $canvas) = @_;

	my @sz = $self-> size;
	$canvas-> set( color => $self-> disabledColor, backColor => $self-> disabledBackColor)
		unless $self-> enabled;
	$canvas-> rect3d( 0, 0, $sz[0]-1, $sz[1]-1, 2,
		$self-> dark3DColor, $self-> light3DColor, $self-> backColor);

	my @zs = ( $self-> {X}, $self-> {Y}, $self-> {CX1}, $self-> {CY});
	my $i;
	my $c = $canvas-> color;
	$canvas-> color( $self-> prelight_color($canvas-> backColor));
	$canvas-> bar( 2, $sz[1] - $zs[1] - 3, $sz[0] - 3, $sz[1] - 3);
	$canvas-> color($c);
	$canvas-> clipRect( 2, 2, $sz[0] - 3, $sz[1] - 3);
	for ( $i = 0; $i < 7; $i++) {
		my $tw = $canvas->get_text_width( $owner-> {days}-> [$i] );
		$canvas-> text_shape_out( $owner-> {days}-> [$i],
			$i * $zs[0] + ($self->{X} - $tw)/2, $sz[1]-$zs[1]+$zs[3],
		);
	}

	my ( $d, $m, $y) = @{$owner-> {date}};
	my $dow = $owner-> day_of_week( 1, $m, $y);
	my $v = $days_in_months[ $m] + (((( $y % 4) == 0) && ( $m == 1)) ? 1 : 0);
	$y = $sz[1] - $zs[1] * 2 + $zs[3] - 3;
	$d--;
	my $prelight = ($self->{prelight} || 0) - 1;
	for ( $i = 0; $i < $v; $i++) {
		if ( $d == $i || $prelight == $i) {
			my $bk = ($d == $i) ? cl::Hilite : cl::Back;
			$bk = $self->prelight_color($bk) if $prelight == $i;
			$canvas-> color($bk);
			$canvas-> bar(
				$dow * $zs[0] + 2, $y - $zs[3],
				( 1 + $dow) * $zs[0] - 1, $y - $zs[3] + $zs[1] - 1
			);
			$canvas-> color(( $d == $i) ? cl::HiliteText : cl::Fore);
			$canvas-> text_shape_out( $i + 1, $dow * $zs[0] + $zs[2], $y);
			$canvas-> rect_focus(
				$dow * $zs[0] + 2, $y - $zs[3],
				( 1 + $dow) * $zs[0] - 1, $y - $zs[3] + $zs[1] - 1
			) if $d == $i && $self-> focused;
			$canvas-> color( $c);
		} else {
			$canvas-> text_shape_out( $i + 1, $dow * $zs[0] + $zs[2], $y);
		}
		$zs[2] = $self-> {CX2} if $i == 8;
		next unless $dow++ == 6;
		$y -= $zs[1];
		$dow = 0;
	}
}

sub Day_KeyDown
{
	my ( $owner, $self, $code, $key, $mod, $repeat) = @_;
	return unless grep { $key == $_ } (
		kb::Left, kb::Right, kb::Down, kb::Up, kb::PgUp, kb::PgDn
	);
	my ( $d, $m, $y) = @{$owner-> {date}};
	$d--  if $key == kb::Left;
	$d++  if $key == kb::Right;
	$d+=7 if $key == kb::Down;
	$d-=7 if $key == kb::Up;
	if ( $key == kb::PgDn) {
		( $m == 11) ? ($y++, $m = 0) : $m++;
	}
	if ( $key == kb::PgUp) {
		( $m == 0) ? ($y--, $m = 11) : $m--;
	}
	$self-> clear_event;
	$owner-> date( $d, $m, $y);
}

sub xy2day
{
	my ($self, $x, $y) = @_;
	my $widget = $self->Day;
	my ( $day, $month, $year) = @{$self-> {date}};
	my @zs = ( $widget-> {X}, $widget-> {Y});
	my $v = $days_in_months[ $month] + (((( $year % 4) == 0) && ( $month == 1)) ? 1 : 0);
	my @sz = $widget-> size;
	$day = (int(($sz[1] - $y - 2) / $zs[1]) - 1) * 7 +
		int(($x - 2) / $zs[0]) - $self-> day_of_week( 1, $month, $year) + 1;
	return ($day <= 0 || $day > $v) ? undef : $day;
}

sub Day_MouseDown
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return unless $btn == mb::Left;
	my ( undef, $month, $year) = @{$owner-> {date}};
	my $day = $owner->xy2day($x,$y);
	$self-> clear_event;
	return unless defined $day;
	$self-> {mouseTransaction} = 1;
	$owner-> date( $day, $month, $year);
}

sub Day_MouseMove
{
	my ( $owner, $self, $mod, $x, $y) = @_;
	my ( undef, $month, $year) = @{$owner-> {date}};
	my $day = $owner->xy2day($x,$y);
	unless ($self-> {mouseTransaction}) {
		if (( $self->{prelight} // -1 ) != ( $day // -1 )) {
			$self->{prelight} = $day;
			$self-> invalidate_rect( 2, 2, $self-> width - 3, $self-> height - $self-> {Y} - 3);
		}
		return;
	}
	$self-> clear_event;
	$owner-> date( $day, $month, $year) if defined $day;
}

sub Day_MouseUp
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return unless $btn == mb::Left && $self-> {mouseTransaction};
	delete $self-> {mouseTransaction};
	$self-> clear_event;
}

sub Day_MouseLeave
{
	my $self = $_[1];
	$self->repaint if delete $self->{prelight};
}

sub Day_MouseWheel
{
	my ( $self, $widget, $mod, $x, $y, $z) = @_;
	my ( $day, $month, $year) = @{$self-> {date}};
	if ( $z > 0) {
		if ( --$day < 1) {
			if ( --$month < 0) {
				return if --$year < 0;
				$month = 11;
			}
			$day = $days_in_months[$month];
		}
	} elsif ( ++$day > $days_in_months[$month]) {
		if ( ++$month > 11) {
			return if ++$year > 199;
			$month = 0;
		}
		$day = 1;
	}
	$self-> date( $day, $month, $year);
	$widget-> clear_event;
}

sub day_reset
{
	my ( $owner, $self, $x, $y) = @_;
	$self-> {X} = int(( $x - 4) / 7 );
	$self-> {Y} = int(( $y - 4) / 7 );
	$self-> begin_paint_info;
	my ($x1,$x2,$x3) = (
		$self-> get_text_width('8'),
		$self-> get_text_width('28'),
		$self-> get_text_width( $owner-> {days}-> [0])
	);
	$x3 += $x1/2;
	$y = $self-> font-> height;
	$self-> end_paint_info;
	$self-> {X} = $x2 if $self-> {X} < $x2;
	$self-> {X} = $x3 if $self-> {X} < $x3;
	$self-> {Y} = $y if $self-> {Y} < $y;
	$self-> {CX1} = int(( $self-> {X} - $x1 ) / 2) + 4;
	$self-> {CX2} = int(( $self-> {X} - $x2 ) / 2) + 4;
	$self-> {CY} = int(( $self-> {Y} - $y ) / 2);
}

sub Day_Size
{
	my ( $owner, $self, $ox, $oy, $x, $y) = @_;
	$owner-> day_reset( $self, $x, $y);
}

sub Day_FontChanged
{
	$_[0]-> day_reset( $_[1], $_[1]-> size);
}

sub Day_Enter { $_[1]-> repaint }
sub Day_Leave { $_[1]-> repaint }

sub Month_Change
{
	my ( $owner, $self) = @_;
	$owner-> month( $self-> focusedItem);
}

sub Year_Change
{
	my ( $owner, $self) = @_;
	$owner-> year( $self-> value - 1900);
}

sub date
{
	return @{$_[0]-> {date}} unless $#_;
	my $self = shift;
	my ($day, $month, $year) = $#_ ? @_ : @{$_[0]} ;
	$month = 11 if $month > 11;
	$month = 0 if $month < 0;
	$year = 0 if $year < 0;
	$year = 199 if $year > 199;
	$day = 1 if $day < 1;
	my $v = $days_in_months[ $month] +
		(((( $year % 4) == 0) && ( $month == 1)) ? 1 : 0);
	$day = $v if $day > $v;
	my @od = @{$self-> {date}};
	return if $day == $od[0] && $month == $od[1] && $year == $od[2];
	$self-> {date} = [ $day, $month, $year ];
	$self-> Year-> value( $year + 1900);
	$self-> Month-> focusedItem( $month);
	$day = $self-> Day;
	$day-> invalidate_rect( 2, 2, $day-> width - 3, $day-> height - $day-> {Y} - 3);
	$self-> notify(q(Change));
}

sub day
{
	return $_[0]-> {date}-> [0] unless $#_;
	return if $_[0]-> {date}-> [0] == $_[1];
	$_[0]-> date( $_[1], $_[0]-> {date}-> [1],$_[0]-> {date}-> [2]);
}

sub month
{
	return $_[0]-> {date}-> [1] unless $#_;
	return if $_[0]-> {date}-> [1] == $_[1];
	$_[0]-> date( $_[0]-> {date}-> [0],$_[1],$_[0]-> {date}-> [2]);
}

sub year
{
	return $_[0]-> {date}-> [2] unless $#_;
	return if $_[0]-> {date}-> [2] == $_[1];
	$_[0]-> date( $_[0]-> {date}-> [0],$_[0]-> {date}-> [1],$_[1]);
}

sub date_as_string
{
	my $self = shift;
	my ( $d, $m, $y) = ( @_ ? ( $#_ ? @_ : @{$_[0]}) : @{ $self-> {date}});
	$y += 1900;
	return $self-> month2str( $m) . " $d, $y";
}

sub date_from_time
{
	$_[0]-> date( @_[4,5,6]);
}

sub firstDayOfWeek
{
	return $_[0]-> {firstDayOfWeek} unless $#_;
	my ( $self, $dow) = @_;
	$dow %= 7;
	return if $dow == $self-> {firstDayOfWeek};
	$self-> {firstDayOfWeek} = $dow;
	$self-> reset_days;
	$self-> Day-> repaint;
}

1;

=pod

=head1 NAME

Prima::Calendar - standard calendar widget

=head1 SYNOPSIS

	use Prima qw(Calendar Application);
	my $cal = Prima::Calendar-> create(
		useLocale => 1,
		size      => [ 150, 150 ],
		onChange  => sub {
			print $_[0]-> date_as_string, "\n";
		},
	);
	$cal-> date_from_time( localtime );
	$cal-> month( 5);
	run Prima;

=for podview <img src="calendar.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/calendar.gif">

=head1 DESCRIPTION

Provides interactive selection of date between 1900 and 2099 years.
The main property, L<date>, is a three-integer array, day, month, and year,
in the format of perl localtime ( see L<perlfunc/localtime> ) -
day can be in range from 1 to 31,month from 0 to 11, year from 0 to 199.

=head1 API

=head2 Events

=over

=item Change

Called when the L<date> property is changed.

=back

=head2 Properties

=over

=item date DAY, MONTH, YEAR

Accepts three integers in format of C<localtime>.
DAY can be from 1 to 31, MONTH from 0 to 11, YEAR from 0 to 199.

Default value: today's date.

=item day INTEGER

Selects the day in month.

=item firstDayOfWeek INTEGER

Selects the first day of week, an integer between 0 and 6,
where 0 is Sunday is the first day, 1 is Monday etc.

Default value: 0

=item month

Selects the month.

=item useLocale BOOLEAN

If 1, the locale-specific names of months and days of week are used.
These are read by calling C<POSIX::strftime>. If invocation of POSIX module
fails, the property is automatically assigned to 0.

If 0, the English names of months and days of week are used.

Default value: 1

See also: L<date_as_string>

=item year

Selects the year.

=back

=head2 Methods

=over

=item can_use_locale

Returns boolean value, whether the locale information can be retrieved
by calling C<strftime>.

=item month2str MONTH

Returns MONTH name according to L<useLocale> value.

=item make_months

Returns array of 12 month names according to L<useLocale> value.

=item day_of_week DAY, MONTH, YEAR, [ USE_FIRST_DAY_OF_WEEK = 1 ]

Returns integer value, from 0 to 6, of the day of week on
DAY, MONTH, YEAR date. If boolean USE_FIRST_DAY_OF_WEEK is set,
the value of C<firstDayOfWeek> property is taken into the account,
so 0 is a Sunday shifted forward by C<firstDayOfWeek> days.

The switch from Julian to Gregorian calendar is ignored.

=item date_as_string [ DAY, MONTH, YEAR ]

Returns string representation of date on DAY, MONTH, YEAR according
to L<useLocale> property value.

=item date_from_time SEC, MIN, HOUR, M_DAY, MONTH, YEAR, ...

Copies L<date> from C<localtime> or C<gmtime> result. This helper method
allows the following syntax:

	$calendar-> date_from_time( localtime( time));

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<POSIX>, L<perlfunc/localtime>, L<perlfunc/time>,
F<examples/calendar.pl>.

=cut
