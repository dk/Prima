=pod

=head1 NAME

examples/keys.pl - A key dump example

=head1 FEATURES

Tests the correct implementation of keyboard event subsystem.
The following features are tested:

- Direct flow of keyboard event
- Correspondence of a syntetic key name to the real name
- key_event() functionality
- Key event propagation. Not that if it is disabled ( a clear_event() is called )
	no system-bound event ( like menus etc. ) are expected to be initiated.
- Diaglyphs type ( f.ex. dieresis + 'e' )
- Alternative keyboard layout ( a Euro sign )

=cut

use strict;
use warnings;
use Prima;
use Prima::Buttons;
use Prima::Label;
use Prima::FontDialog;
use Prima::Application;

my $propagate = 1;
my $repeat    = 0;
my $fontDialog;

my $w = Prima::MainWindow-> create(
	size => [500,250],
	text => 'Keyboard events checker',
	menuItems => [['~Options' => [
		[ '@*' => '~Propagate key event' => sub { $propagate = $_[2] } ],
		[ '@'  => '~Repeat key event'    => sub { $repeat    = $_[2] } ],
		[ '@'.( $::application-> wantUnicodeInput()   ? '*' : '') . # is is on?
			( $::application-> get_system_value( sv::CanUTF8_Input) ? '' : '-'), # is it writable ?
			'~Unicode input' => sub { $::application-> wantUnicodeInput($_[2]) },
		],
		[],
		["Set ~font..." => "Ctrl+F" => '^F' => sub {
			my $d =  $fontDialog ? $fontDialog : Prima::FontDialog-> create(
				logFont => $_[0]-> Label1-> font,
			);
			$fontDialog = $d;
			return unless $d-> execute == mb::OK;
			$_[0]-> Label1-> font( $d-> logFont);
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
		(( $mod & km::KeyPad) ? 'KeyPad+' : '').
		(( $mod & km::DeadKey) ? 'DeadKey+' : '');
	chop($mod) if $mod;
	my $lcode = $code ? (( $code < 27) ? chr( $code + ord('@')) : chr( $code)) : 'n/a';
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
	$t = "$prefx code: $code|$lcode|, key: $key, mod: $mod => $tran";
	$self-> text( $t);
	print "$t\n";
}


my $l = $w-> insert( Label =>
	origin    => [10,10],
	text      => 'Press a key',
	size      => [$w-> width - 20, $w-> height - 220],
	growMode  => gm::Floor,
	font      => {name => 'Arial'},
	selectable=> 1,
	name      => 'Label1',
	autoHeight => 1,
	autoWidth  => 1,
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

$l-> select;

$w-> insert( Button =>
	origin => [10,160],
	text   => 'kb::F10 test',
	selectable => 0,
	onClick => sub {
		$l-> key_event( cm::KeyDown, 0, kb::F10, 0, 1, 0);
	},
);

run Prima;
