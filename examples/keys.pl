
use Prima;
use Prima::Buttons;
use strict;
use Prima::Label;

$::application = Prima::Application-> create(name => 'keys');

my $propagate = 1;
my $repeat    = 0;

my $w = Prima::Window-> create(
    onDestroy => sub {$::application-> destroy},
    size => [500,250],
    menuItems => [['~Options' => [
       [ '*pp' => '~Propagate key event' => sub {
         $propagate = $propagate ? 0 : 1;
         $_[0]-> menu-> checked( 'pp', $propagate);
       }],
       [ 'rr' => '~Repeat key event' => sub {
         $repeat = $repeat ? 0 : 1;
         $_[0]-> menu-> checked( 'rr', $repeat);
       }],
    ]]],
);

sub keydump
{
   my ( $prefx, $self, $code, $key, $mod) = @_;
   my $t = '';
   $mod =
       (( $mod & km::Alt)   ? 'Alt+' : '') .
       (( $mod & km::Ctrl)  ? 'Ctrl+' : '') .
       (( $mod & km::Shift) ? 'Shift+' : '') .
       (( $mod & km::KeyPad) ? 'KeyPad+' : '');
   chop($mod) if $mod;
   my $lcode = $code ? chr( $code) : 'n/a';
   $lcode = 'n/a' if $lcode =~ /\0/;
   for ( keys %kb::) {
      next if $_ eq 'AUTOLOAD';
      next if $_ eq 'BEGIN';
      next if $_ eq 'constant';
      $key = "kb::$_", last if $key == &{$kb::{$_}}();
   }
   my $tran = '';
   if ( $key eq 'kb::NoKey') {
      $tran = $code ? $lcode : '';
      $tran = 'Space' if $tran eq ' ';
   } else {
      $tran = $key;
      $tran =~ s/kb:://;
   }
   if ( $mod) {
      if ( $tran ne '') {
         $tran = "$mod+$tran";
      } else {
         $tran = $mod;
      }
   }
   $mod = '0' if $mod eq '';
   $t = "$prefx code: $code|$lcode|, key: $key, mod: $mod => $tran\n";
   print $t;
   $self-> text( $t);
}


my $l = $w-> insert( Label =>
   origin    => [10,10],
   text      => 'Press a key',
   size      => [$w-> width - 20, $w-> height - 220],
   growMode  => gm::Client,
   font      => {name => 'Arial CYR'},
   selectable=> 1,
   onKeyDown => sub {
      my ( $self, $code, $key, $mod) = @_;
      keydump( '', $self, $code, $key, $mod);
      $self-> clear_event unless $propagate;
      $self-> key_event( cm::KeyUp, $code, $key, $mod, 1, 1) if $repeat;
   },
   onKeyUp => sub {
      my ( $self, $code, $key, $mod) = @_;
      keydump( 'up', $self, $code, $key, $mod);
   },
   onMouseClick => sub {
       shift;
       print sprintf "%d %04x [%d %d] %d\n", @_;
   },
);

$l-> focus;

$w-> insert( Button =>
   origin => [10,160],
   text   => 'kb::F10 test',
   selectable => 0,
   onClick => sub {
       $l-> key_event( cm::KeyDown, 0, kb::F10, 0, 1, 0);
   },
);


run Prima;
