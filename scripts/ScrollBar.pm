package ScrollBar;
use vars qw(@ISA @stdMetrics);
@ISA = qw(Widget MouseScroller);

@stdMetrics = Application-> get_default_scrollbar_metrics;

use strict;
use Const;
use IntUtils;

=head1 NAME

ScrollBar - Prima scroll bars class

=head1 SYNOPSIS

    use ScrollBar;

    my $sb = ScrollBar-> create( owner => $group, %rest_of_profile);
    my $sb = $group-> insert( 'ScrollBar', %rest_of_profile);

    my $isAutoTrack = $sb-> autoTrack;
    $sb-> autoTrack( $yesNo);

    my $val = $sb-> value;
    $sb-> value( $value);
    $sb-> set_value( $value);

    my $min = $sb-> min;
    my $max = $sb-> max;
    $sb-> min( $min);
    $sb-> max( $max);
    $sb-> set_bounds( $min, $max);

    my $step = $sb-> step;
    my $pageStep = $sb-> pageStep;
    $sb-> step( $step);
    $sb-> pageStep( $pageStep);
    $sb-> set_steps( $step, $pageStep);

    my $partial = $sb-> partial;
    my $whole = $sb-> whole;
    $sb-> partial( $partial);
    $sb-> whole( $whole);
    $sb-> set_proportion( $partial, $whole);

    my $size = $sb-> minThumbSize;
    $sb-> minThumbSize( $size);
    $sb-> set_min_thumb_size( $size);

    my $isVertical = $sb-> vertical;
    $sb-> vertical( $yesNo);
    $sb-> set_vertical( $yesNo);

    my ($width,$height) = $sb-> get_default_size;

=head1 DESCRIPTION

The class C<ScrollBar> implements both vertical and horizontal
scrollbars in I<Prima>.  This is a purely Perl class without any
C-implemented parts except those inherited from C<Widget>.

For the basic organization of I<Prima> classes see L<Prima>.

=head2 Options

Methods init() and set() understand the following options
(I describe here only those new to C<ScrollBar> or with default
values different of those set in C<Widget>).

First, there are options which found new defaults in C<ScrollBar>
class:

=over 4

=item selectable

Default value is 0 (logical false).  Set this to 1 if you ever
want C<ScrollBar> to receive keyboard focus.

=item height

Default value is $ScrollBar::stdMetrics[1], which is an operating
system dependent value determined with a call to
C<Application-E<gt> get_default_scrollbar_metrics>.  The height is
affected because by default the horizontal C<ScrollBar> will be
created.

=item widgetClass

This is to provide default color scheme.
Default value is C<wc::ScrollBar>, where C<cc> is a package
located in C<Const> library.

=item tabStop

Default value is 0 (logical false).  Indeed, there is no
need for a non-selectable widget to be tab-stoppable. :-)

=item growMode

Default value is gm::GrowHiX, i.e. the scrollbar will try
to maintain the constant distance from its right edge to its
owner's right edge as the owner changes its size.
This is useful for horizontal scrollbars.

=back

Next, C<ScrollBar> introduces the following new options:

=over 4

=item autoTrack

This boolean option says the C<ScrollBar> whether it should send
C<Change> notification during mouse tracking events.
Generally it should only be set to 0 on slow computers.

Default value is 1 (logical true).

=item vertical

This logical option determines the main scrollbar style.
Set this to 1 if you wish to create a vertical scrollbar,
0 - to create a horizontal one. This option is a run-time
property, so you can in fact morph scrollbars from
horizontal to vertical and vice versa.  Looks
funny.

Default value is 0 (logical false).

=item value

The most important characteristic of the scrollbar.
Corresponds directly to the position of a thumb.

Default value is 0.

=item min

This options sets the lower limit for C<value>.

Default value is 0.

=item max

This options sets the upper limit for C<value>.

Default value is 100.

=item minThumbSize

The thumb cannot have main dimension lesser than this.

Default value is 21 pixels.

=item step

This determines the minimal increment/decrement
to C<value> during mouse/keyboard interaction.

Default value is 1.

=item pageStep

This determines the increment/decrement to
C<value> during "page"-related operations, like clicking the mouse
somewhere on the strip outside the thumb, or pressing C<PgDn> or C<PgUp>.

Default value is 10.

=item partial

This tells the scrollbar how many of imaginary
units the thumb should occupy. See C<whole> below.

Default value is 10.

=item whole

This tells the scrollbar how many of imaginary
units correspond to the whole length of the scrollbar.  This value
has nothing in common with C<min> and C<max>.  You may think of the
combination of C<partial> and C<whole> as of the proportion between
the visible size of something (document, for example) and the whole
size of that "something".  Useful to struggle against infamous "bird"
size of Windows scrollbar thumbs.

Default value is 100.

=back

=head2 Properties.  Set/get methods.

=over 4

=item autoTrack

Retrieves the current value/sets the new value of
auto tracking.  See option autoTrack above.
There are no corresponding set/get methods.

=item value

Retrieves the current thumb position/sets the
thumb position.  This property has no corresponding
get() method.

=item min

Retrieves and sets the lower limit for C<value>.
There is a method set_bounds() which sets both
C<min> and C<max> in a single swoop.  There is no
corresponding get() method.

=item max

Retrieves and sets the upper limit for C<value>.
There is a method set_bounds() which sets both
C<min> and C<max> in a single swoop.  There is no
corresponding get() method.

=item step

Retrieves and sets the "single" stepping value
of the thumb.  There is also a method set_steps()
which sets both C<step> and C<pageStep>.  There is
no corresponding get() method.

=item pageStep

Retrieves and sets the "page" stepping value
of the thumb.  There is also a method set_steps()
which sets both C<step> and C<pageStep>.  There is
no corresponding get() method.

=item partial

Retrieves and sets the length of a thumb in logical
units.  There is also a method set_proportion()
which sets both C<partial> and C<whole>.  There is
no corresponding get() method.

=item whole

Retrieves and sets the length of a whole scrollbar in logical
units.  There is also a method set_proportion()
which sets both C<partial> and C<whole>.  There is
no corresponding get() method.

=item minThumbSize

Retrieves and sets the minimal size of main dimension
of a thumb.  There is no corresponding get() method,
though set_min_thumb_size() is here.

=item vertical

Retrieves and sets the horizontal/vertical style of a
scrollbar.  There is no corresponding get() method,
though set_vertical() is defined.

=back

=head2 Public methods

=over 4

=item get_default_size()

Returns as ($x,$y) the default (platform dependant)
width for a vertical scrollbar and height for a horizontal
scrollbar.  There are no corresponding set() method and
corresponding property.

=back

=head2 Important callbacks

There is only one callback designed to be generally useful: C<Change>.
The C<Change> notification is sent whenever the thumb position of
scrollbar is changed, subject to a certain limitations when C<autoTrack>
is false.  The notification conforms the general I<Prima> rule:
it is sent when appropriate, regardless to whether this was a
result of user interaction, or a side effect of some method
programmer has called.

=head2 Other methods (Methods the user should not call)

Those methods and event handlers (callbacks) are purely for internal use.
Don't call them directly!

  profile_default();
  profile_check_in();
  init();
  on_size();
  on_paint();
  translate_point();
  draw_part();
  ScrollTimer_Tick();
  on_keydown();
  on_mousedown();
  on_mouseclick();
  on_mouseup();
  on_mousemove();
  reset();

=head1 EXAMPLE

  #! /usr/local/bin/prima

  use Classes;
  use Const;
  use Application name => 'ScrollBar test';
  use ScrollBar;
  use Label;

  my $w = Window-> create(
			  text => 'ScrollBar test',
			  size => [300,200]);
  my $lbl = $w-> insert( Label =>
			 text => 'Nothing interesting',
			 autoWidth => 0,
			 width => 280,
			 left => 10,
			 bottom => 100,
			 alignment => ta::Center);
  my $sb = $w-> insert( ScrollBar =>
			width => 280,
			left => 10,
			bottom => 50,
			onChange => sub {
			  $lbl-> text( $_[0]-> value);
			});
  1;

=head1 BUGS

Numerous, to say the least.

=head1 SEE ALSO

L<Prima>, L<Widget>, L<MouseScroller>, L<Classes>, L<Const>, L<IntUtils>

=head1 COPYRIGHT

Copyright 1997, 1998 The Protein Laboratory, University of Copenhagen.
All rights reserved.

This library is free software;  you can redistribute it and/or
modify it under the same terms as Perl itself.

=head1 AUTHORS

Dmitry Karasik E<lt>dk@plab.ku.dkE<gt>,
Anton Berezin E<lt>tobez@plab.ku.dkE<gt> - documentation

=cut

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      autoTrack     => 1,
      growMode      => gm::GrowHiX,
      height        => $stdMetrics[1],
      min           => 0,
      minThumbSize  => 21,
      max           => 100,
      pageStep      => 10,
      partial       => 10,
      selectable    => 0,
      step          => 1,
      tabStop       => 0,
      value         => 0,
      vertical      => 0,
      widgetClass   => wc::ScrollBar,
      whole         => 100,
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   my $vertical = exists $p-> {vertical} ? $p-> {vertical} : $default->{ vertical};
   if ( $vertical)
   {
      $p-> { width}    = $stdMetrics[0] unless exists $p->{ width};
      $p-> { growMode} = gm::GrowLoX | gm::GrowHiY if !exists $p-> { growMode};
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   for ( qw( b1 b2 tab left right)) { $self-> { $_}-> { pressed} = 0; }
   for ( qw( b1 b2 tab)) { $self-> { $_}-> { enabled} = 0; }
   for ( qw( autoTrack minThumbSize vertical min max step pageStep whole partial value))
      { $self->{$_} = 1; }
   for ( qw( autoTrack minThumbSize vertical min max step pageStep whole partial value))
      { $self->$_( $profile{ $_}); }
   $self->{suppressNotify} = undef;
   $self-> reset;
   return %profile;
}


sub set
{
   my ( $self, %profile) = @_;
   if ( exists $profile{partial} and exists $profile{whole}) {
      $self-> set_proportion($profile{partial}, $profile{whole});
      delete $profile{partial};
      delete $profile{whole};
   }
   if ( exists $profile{min} and exists $profile{max}) {
      $self-> set_bounds($profile{max}, $profile{max});
      delete $profile{min};
      delete $profile{max};
   }
   if ( exists $profile{step} and exists $profile{pageStep}) {
      $self-> set_steps($profile{step}, $profile{pageStep});
      delete $profile{step};
      delete $profile{pageStep};
   }
   $self-> SUPER::set( %profile);
}

sub on_size
{
   $_[0]-> reset;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my @clr;
   my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
   if ( $self-> enabled)
   {
      @clr = ($self-> color, $self-> backColor);
   } else { @clr = ($self-> disabledColor, $self-> disabledBackColor); }
   my @size = $canvas-> size;
   my $btx  = $self-> { btx};
   my $v    = $self-> { vertical};
   my ( $maxx, $maxy) = ( $size[0]-1, $size[1]-1);
   $canvas-> color( $c3d[0]);
   $canvas-> line( 0, 0, $maxx - 1, 0);
   $canvas-> line( $maxx, 0, $maxx, $maxy - 1);
   $canvas-> color( $c3d[1]);
   $canvas-> line( 0, $maxy, $maxx, $maxy);
   $canvas-> line( 0, 0, 0, $maxy);
   {
      my @border = @c3d;
      push ( @border, shift @border) if $self-> {b1}-> { pressed};
      $canvas-> rect3d( @{$self->{b1}->{rect}}, 2, @border, $clr[1]);
      $canvas-> color( $self-> {b1}-> { enabled} ? $clr[ 0] : $self-> disabledColor);
      my $a = $self-> { b1}-> { pressed} ? 1 : 0;
      my @spot = $v ? (
         $maxx * 0.5 + $a, $maxy - $btx * 0.40 - $a,
         $maxx * 0.3 + $a, $maxy - $btx * 0.55 - $a,
         $maxx * 0.7 + $a, $maxy - $btx * 0.55 - $a,
      ) : (
         $btx * 0.40 + $a, $maxy * 0.5 - $a,
         $btx * 0.55 + $a, $maxy * 0.7 - $a,
         $btx * 0.55 + $a, $maxy * 0.3 - $a
      );
      $canvas-> fillpoly( [ @spot]);
   }
   {
      my @border = @c3d;
      push ( @border, shift @border) if $self-> {b2}-> { pressed};
      $canvas-> rect3d( @{$self->{b2}->{rect}}, 2, @border, $clr[1]);
      $canvas-> color( $self-> {b2}-> { enabled} ? $clr[ 0] : $self-> disabledColor);
      my $a = $self-> { b2}-> { pressed} ? 1 : 0;
      my @spot = $v ? (
         $maxx * 0.5 + $a, $btx * 0.40 + 1 - $a,
         $maxx * 0.3 + $a, $btx * 0.55 + 1 - $a,
         $maxx * 0.7 + $a, $btx * 0.55 + 1 - $a
      ) : (
         $maxx - $btx * 0.45 + 1 + $a, $maxy * 0.5 - $a,
         $maxx - $btx * 0.60 + 1 + $a, $maxy * 0.7 - $a,
         $maxx - $btx * 0.60 + 1 + $a, $maxy * 0.3 - $a
      );
      $canvas-> fillpoly( [ @spot]);
      $canvas-> color( $clr[ 1]);
   }
   if ( $self-> { tab}-> { enabled})
   {
      my @border = @c3d;
      push ( @border, shift @border) if $self-> {tab}-> { pressed};
      my @rect = @{$self-> { tab}-> { rect}};
      my $lenx = $self-> { tab}-> { q(length)};

      for ( qw( left right))
      {
         if ( defined $self-> { $_}->{rect})
         {
            my @r = @{$self-> {$_}->{rect}};
            $canvas-> color( $c3d[1]);
            $v ? ( $canvas-> line( $r[0]-1, $r[1], $r[0]-1, $r[3])):
                 ( $canvas-> line( $r[0], $r[3]+1, $r[2], $r[3]+1));
            $canvas-> color( $self-> {$_}->{pressed} ? $clr[0] : $clr[1]);
            $canvas-> bar( @r);
         }
      }

      $canvas-> rect3d( @rect, 2, @border, $clr[1]);
      if ( $self-> {minThumbSize} > 8)
      {
         if ( $v)
         {
            my $sty = $rect[1] + int($lenx / 2) - 4 - ( $self-> { tab} ->{ pressed} ? 1 : 0) ;
            my $stx = int($maxx / 3) + ( $self-> { tab} ->{ pressed} ? 1 : 0);
            my $lnx = int($maxx / 3);
            my $i;
            $canvas-> color( $c3d[ 0]);
            for ( $i = 0; $i < 6; $i++) {
               $canvas-> line( $stx, $sty + $i * 2 - 1, $stx + $lnx, $sty + $i * 2 - 1);
            }
            $canvas-> color( $clr[ 0]);
            for ( $i = 0; $i < 6; $i++) {
               $canvas-> line( $stx, $sty + $i * 2, $stx + $lnx, $sty + $i * 2);
            }
         } else {
            my $stx = $rect[0] + int($lenx / 2) - 6 + ( $self-> { tab} ->{ pressed} ? 1 : 0) ;
            my $sty = int($maxy / 3) - ( $self-> { tab} ->{ pressed} ? 1 : 0);
            my $lny = int($maxy / 3);
            my $i;
            $canvas-> color( $c3d[ 0]);
            for ( $i = 0; $i < 6; $i++) {
               $canvas-> line( $stx + $i * 2 + 1, $sty, $stx + $i * 2 + 1, $sty + $lny);
            }
            $canvas-> color( $clr[ 0]);
            for ( $i = 0; $i < 6; $i++) {
               $canvas-> line( $stx + $i * 2, $sty, $stx + $i * 2, $sty + $lny);
            }
         }
      }
   } else {
      my @r = @{$self->{groove}->{rect}};
      $canvas-> color( $c3d[1]);
      $v ? ( $canvas-> line( $r[0]-1, $r[1], $r[0]-1, $r[3])):
           ( $canvas-> line( $r[0], $r[3]+1, $r[2], $r[3]+1));
      $canvas-> color( $clr[1]);
      $canvas-> bar( @r);
   }
}

sub translate_point
{
   my ( $self, $x, $y) = @_;
   for( qw(b1 b2 tab left right groove))
   {
      next unless defined $self->{$_}->{rect};
      my @r = @{$self->{$_}->{rect}};
      return $_ if $x >= $r[0] && $y >= $r[1] && $x <= $r[2] && $y <= $r[3];
   }
   return undef;
}

sub draw_part
{
   my ( $self, $who) = @_;
   return unless defined $self->{$who}->{rect};
   my @r = @{$self->{$who}->{rect}};
   $r[2]++;
   $r[3]++;
   $self-> invalidate_rect( @r);
}

sub ScrollTimer_Tick
{
   my ( $self, $timer) = @_;
   unless ( defined $self->{mouseTransaction})
   {
      $self->scroll_timer_stop;
      return;
   }
   my $who = $self->{mouseTransaction};
   if ( $who eq q(b1) || $who eq q(b2))
   {
      return unless $self->{$who}->{pressed};
      my $oldValue = $self->{value};
      $self-> {suppressNotify} = 1;
      $self-> value( $oldValue + (( $who eq q(b1))?-1:1)*$self->{step});
      $self-> {suppressNotify} = undef;
      if ( $oldValue == $self->{value})
      {
         $self->{$who}->{pressed} = 0;
         $self-> draw_part( $who);
         $self->{mouseTransaction} = undef;
         $self-> capture(0);
         $self-> scroll_timer_stop;
         return;
      }
      $self-> notify(q(Change));
   }
   if ( $who eq q(left) || $who eq q(right))
   {
      return unless $self->{$who}->{pressed};
      my $upon = $self-> translate_point( $self-> pointerPos);
      if ( $upon ne $who) {
          if ( $self->{$who}->{pressed}) {
             $self->{$who}->{pressed} = 0;
             $self-> draw_part( $who);
          }
          return;
      }
      my $oldValue = $self->{value};
      $self-> {suppressNotify} = 1;
      $self-> value( $oldValue + (( $who eq q(left))?-1:1)*$self->{pageStep});
      $self-> {suppressNotify} = undef;
      if ( $oldValue == $self->{value})
      {
         $self->{$who}->{pressed} = 0;
         $self->{mouseTransaction} = undef;
         $self-> capture(0);
         $self-> scroll_timer_stop;
         return;
      }
      $self-> notify(q(Change));
   }
   if ( exists $timer->{newRate})
   {
      $timer-> timeout( $timer->{newRate});
      delete $timer->{newRate};
   }
}

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   $self-> clear_event;
   my ( $v, $x, $d, $s, $ps) = ($self-> vertical, $self-> value, 0, $self-> step, $self-> pageStep);
   $d = $s  if ( $key == kb::Right && !$v) || ( $key == kb::Down && $v);
   $d = -$s if ( $key == kb::Left  && !$v) || ( $key == kb::Up   && $v);
   $d = $ps  if $key == kb::PgDn && !($mod & kb::Ctrl);
   $d = -$ps if $key == kb::PgUp && !($mod & kb::Ctrl);
   $d = $self-> max - $x if ( $key == kb::PgDn && $mod & kb::Ctrl) || $key == kb::End;
   $d = -$x if ( $key == kb::PgUp && $mod & kb::Ctrl) || $key == kb::Home;
   $self-> value( $x + $d) if $d;
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   $self-> clear_event;
   return if defined $self->{mouseTransaction};
   my $who = $self-> translate_point( $x, $y);
   return if !defined $who or $who eq q(groove);
   if (( $who eq q(b1) || $who eq q(b2)))
   {
      if ( $self->{$who}->{enabled})
      {
         $self->{$who}->{pressed} = 1;
         $self->{mouseTransaction} = $who;
         $self-> capture(1);
         $self-> {suppressNotify} = 1;
         $self-> value( $self->{value} + (( $who eq q(b1))?-1:1) * $self->{step});
         $self-> {suppressNotify} = undef;
         $self-> draw_part($who);
         $self-> notify(q(Change));
         $self-> scroll_timer_start;
      } else {
         $self-> event_error;
      }
      return;
   }
   if (( $who eq q(left) || $who eq q(right)))
   {
      $self->{$who}->{pressed} = 1;
      $self->{mouseTransaction} = $who;
      $self-> value( $self->{value} + (( $who eq q(left))?-1:1) * $self->{pageStep});
      $self-> capture(1);
      $self-> scroll_timer_start;
      return;
   }
   if (( $who eq q(tab)) && ( $self->{tab}->{enabled}))
   {
      $self->{$who}->{pressed} = 1;
      $self->{mouseTransaction} = $who;
      my @r = @{$self->{tab}->{rect}};
      $self->{$who}->{aperture} = $self->{ vertical} ? $y - $r[1] : $x - $r[0];
      $self-> draw_part($who);
      $self-> capture(1);
   }
}

sub on_mouseclick
{
   my $self = shift;
   $self-> clear_event;
   return unless pop;
   $self-> clear_event unless $self-> notify( "MouseDown", @_);
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   $self-> clear_event;
   return unless defined $self->{mouseTransaction};

   my $who = $self->{mouseTransaction};
   if ( $who eq q(b1) || $who eq q(b2) || $who eq q(left) || $who eq q(right) || $who eq q(tab))
   {
      $self->{$who}->{pressed} = 0;
      $self->{mouseTransaction} = undef;
      $self-> capture(0);
      $self-> repaint;
      $self-> notify(q(Change)) if !$self->{autoTrack} && $who eq q(tab);
   }
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   $self-> clear_event;
   return unless defined $self->{mouseTransaction};
   my $who   = $self->{mouseTransaction};

   if ( $who eq q(b1) || $who eq q(b2) || $who eq q(left) || $who eq q(right))
   {
      my $upon  = $self-> translate_point( $x, $y);
      my $oldPress = $self->{$who}->{pressed};
      $self->{$who}->{pressed} = ( defined $upon && ( $upon eq $who)) ? 1 : 0;
      my $useRepaint = $self->{$who}->{pressed} != $oldPress;
      $self-> repaint if $useRepaint;
   }

   if ( $who eq q(tab))
   {
      my @groove = @{$self->{groove}->{rect}};
      my @tab    = @{$self->{tab}->{rect}};
      my $val    = eval{$self->{vertical} ?
         $self->{max} -
            (( $y - $self->{$who}->{aperture} - $self->{btx}) *
             ( $self->{max} - $self->{min})) /
            ($groove[3] - $groove[1] - $tab[3] + $tab[1])
         :
         $self->{min} +
             (( $x - $self->{$who}->{aperture} - $self->{btx}) *
              ( $self->{max} - $self->{min})) /
              ( $groove[2] - $groove[0] - $tab[2] + $tab[0])};
      my $ov = $self-> {value};
      $self-> {suppressNotify} = $self->{autoTrack} ? undef : 1;
      $self-> value( $val);
      $self-> {suppressNotify} = undef;
   }
}


sub reset
{
   my $self = $_[0];
   $self-> { b1} -> { enabled} = $self-> { value} > $self-> { min};
   $self-> { b2} -> { enabled} = $self-> { value} < $self-> { max};
   my $fullDisable = $self-> { partial} == $self-> { whole};
   $self-> { tab}-> { enabled} = ( $self-> { min} != $self-> { max}) && !$fullDisable;
   $self-> { b1}-> { enabled} = 0 if ( $self-> { value} == $self-> { min});
   $self-> { b2}-> { enabled} = 0 if ( $self-> { value} == $self-> { max});
   my $btx  = $self-> { minThumbSize};
   my $v    = $self-> { vertical};
   my @size = $self-> size;
   my ( $maxx, $maxy) = ( $size[0]-1, $size[1]-1);
   if ( $v) { $btx = int($size[1] / 2) if $btx * 2 >= $maxy; }
       else { $btx = int($size[0] / 2) if $btx * 2 >= $maxx; }
   my @rect = $v ? ( 1, $maxy - $btx + 1, $maxx - 1, $maxy - 1)
                 : ( 1, 1, $btx - 1, $maxy - 1);
   $self-> { b1}-> { rect} = [ @rect];
   @rect    = $v ? ( 1, 1, $maxx - 1, $btx - 1)
                 : ( $maxx - $btx + 1, 1, $maxx - 1, $maxy - 1);
   $self-> { b2}-> { rect} = [ @rect];
   $self-> { btx} = $btx;
   @rect = $v ? (
      2, $btx, $maxx - 1, $maxy - $btx
   ) : (
      $btx, 1, $maxx - $btx, $maxy - 2
   );
   $self-> {groove}->{rect} = [@rect];
   my $startx  = $v ? $size[1]: $size[0];
   my $groovex = $startx - $btx * 2;
   $self-> { tab}-> { enabled} = 0 if $groovex < $self->{minThumbSize};
   if ( $self-> { tab}-> { enabled})
   {
      my $lenx = int( $groovex * $self-> { partial} / $self-> { whole});
      $lenx = $self->{minThumbSize} if $lenx < $self->{minThumbSize};
      my $atx  = int(( $self-> { value} - $self->{min}) * ( $groovex - $lenx) / ( $self->{ max} - $self->{ min}));
      $atx = $groovex - $lenx if $lenx + $atx > $groovex;
      ( $lenx, $atx) = ( $groovex - 1, 0) if $atx < 0;
      @rect = $v ? (
         1, $maxy - $btx - $lenx - $atx, $maxx - 1, $maxy - $btx - $atx
      ) : (
         $btx + $atx, 1, $btx + $atx + $lenx - 1, $maxy - 1
      );
      $self->{ tab}->{ rect} = [@rect];
      $self->{ tab}->{ q(length)} = $lenx;
      if ( $v)
      {
         my @r2 = ( 2, $btx, $maxx - 1, $rect[1] - 1);
         my @r1 = ( 2, $rect[3], $maxx - 1, $maxy - $btx);
         $self-> { left}->{rect}  = ( $r1[ 1] < $r1[ 3]) ? [ @r1] : undef;
         $self-> { right}->{rect} = ( $r2[ 1] < $r2[ 3]) ? [ @r2] : undef;
      } else {
         my @r1 = ( $btx, 1, $rect[ 0] - 1, $maxy - 2);
         my @r2 = ( $rect[ 2], 1, $maxx - $btx, $maxy - 2);
         $self-> { left}->{rect}  = ( $r1[ 0] < $r1[ 2]) ? [ @r1] : undef;
         $self-> { right}->{rect} = ( $r2[ 0] < $r2[ 2]) ? [ @r2] : undef;
      }
   }
}

sub set_bounds
{
   my ( $self, $min, $max) = @_;
   $max = $min if $max < $min;
   ( $self-> { min}, $self-> { max}) = ( $min, $max);
   my $oldValue = $self->{value};
   $self-> value( $max) if $self->{value} > $max;
   $self-> value( $min) if $self->{value} < $min;
   $self-> reset;
   $self-> repaint;
}

sub set_steps
{
   my ( $self, $step, $pStep) = @_;
   $step  = 0 if $step < 0;
   $pStep = 0 if $pStep < 0;
   $pStep = $step if $pStep < $step;
   ( $self-> { step}, $self-> { pageStep}) = ( $step, $pStep);
}

sub set_proportion
{
   my ( $self, $partial, $whole) = @_;
   $whole   = 1 if $whole   <= 0;
   $partial = 1 if $partial <= 0;
   $partial = $whole if $partial > $whole;
   return if $partial == $self-> { partial} and $whole == $self-> { whole};
   ( $self-> { partial}, $self-> { whole}) = ( $partial, $whole);
   $self-> reset;
   $self-> repaint;
}

sub set_value
{
   my ( $self, $value) = @_;
   my ( $min, $max) = ( $self->{min}, $self->{max});
   $value = $min if $value < $min;
   $value = $max if $value > $max;
   $value -= $min;
   $max   -= $min;
   my $div    = eval{ int($value / $self->{step} + 0.5 * (( $value >= 0) ? 1 : -1))} || 0;
   my $lbound = $value == 0 ? 0 : $div * $self->{step};
   my $rbound = $value == $max ? $max : ( $div + 1) * $self->{step};
   $value = ( $value >= (( $lbound + $rbound) / 2)) ? $rbound : $lbound;
   $value = 0    if $value < 0;
   $value = $max if $value > $max;
   $value += $min;
   my $oldValue = $self-> {value};
   my %v = ();
   $v{b1ok}     = $self->{b1}->{enabled}?1:0;
   $v{b2ok}     = $self->{b2}->{enabled}?1:0;
   $v{grooveok} = $self->{tab}->{enabled}?1:0;
   $v{grooveok} .= join(q(x),@{$self->{tab}->{rect}}) if $v{grooveok};
   return if $oldValue == $value;
   $self-> { value} = $value;
   $self-> reset;
   $v{b1ok2}     = $self->{b1}->{enabled}?1:0;
   $v{b2ok2}     = $self->{b2}->{enabled}?1:0;
   $v{grooveok2} = $self->{tab}->{enabled}?1:0;
   $v{grooveok2}.= join(q(x),@{$self->{tab}->{rect}}) if $v{grooveok2};
   for ( qw( b1 b2 groove))
   {
      if (( $v{"${_}ok"} ne $v{"${_}ok2"}))
      {
         my @r = @{$self->{$_}->{rect}};
         $r[0]--; $r[1]--; $r[2]++; $r[3]+=2;
         $self->invalidate_rect(@r);
      }
   }
   $self-> notify(q(Change)) unless $self->{suppressNotify};
}

sub set_min_thumb_size
{
   my ( $self, $mts) = @_;
   $mts = 1 if $mts < 0;
   return if $mts == $self-> {minThumbSize};
   $self->{minThumbSize} = $mts;
   $self-> reset;
   $self-> repaint;
}

sub set_vertical
{
   my ( $self, $vertical) = @_;
   return if $vertical == $self-> { vertical};
   $self-> { vertical} = $vertical;
   $self-> reset;
   $self-> repaint;
}

sub get_default_size
{
   return @stdMetrics;
}

sub value        {($#_)?$_[0]->set_value       ($_[1])                 : return $_[0]->{value}       }
sub autoTrack    {($#_)?$_[0]->{autoTrack} =    $_[1]                  : return $_[0]->{autoTrack}   }
sub min          {($#_)?$_[0]->set_bounds($_[1], $_[0]-> {'max'})      : return $_[0]->{min};}
sub minThumbSize {($#_)?$_[0]->set_min_thumb_size($_[1])               : return $_[0]->{minThumbSize};}
sub max          {($#_)?$_[0]->set_bounds($_[0]-> {'min'}, $_[1])      : return $_[0]->{max};}
sub step         {($#_)?$_[0]->set_steps ($_[1], $_[0]-> {'pageStep'}) : return $_[0]->{step};}
sub pageStep     {($#_)?$_[0]->set_steps ($_[0]-> {'step'}, $_[1])     : return $_[0]->{pageStep};}
sub partial      {($#_)?$_[0]->set_proportion ($_[1], $_[0]->{'whole'}): return $_[0]->{partial};}
sub whole        {($#_)?$_[0]->set_proportion($_[0]->{'partial'},$_[1]): return $_[0]->{whole};}
sub vertical     {($#_)?$_[0]->set_vertical  ($_[1])                   : return $_[0]->{vertical}    }

1;
