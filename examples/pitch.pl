use Prima;
use Prima::Utils;
=pod 
=item NAME

A sound example

=item FEATURES

Tests the implementation of apc_beep_tone() function.

=cut


my %octave = (
  'C'  => 523,
  'B'  => 493,
  'A#' => 466,
  'A'  => 440,
  'G#' => 415,
  'G'  => 391,
  'F#' => 369,
  'F'  => 349,
  'E'  => 329,
  'D#' => 311,
  'D'  => 293,
  'C#' => 277,
);

$_ = 'E2D#EF2E2  G#2A4';
#$_ = 'AEAEAG#2 2EG#EG#A2 2EAEAG#2 2EG#EG#A2 AB2 BC2 C2BAG#A2 AB2 BC2 C2BAG#A3';

for ( m/\G([A-G\s][\#\d]*)/g) {
   my $d = (s/(\d+)$//) ? $1 : 1;
   $octave{$_} ? 
      Prima::Utils::sound( $octave{$_}, 100 * $d) : 
      select(undef,undef,undef,0.1 * $d);
}

