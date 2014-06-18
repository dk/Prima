#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
#  $Id$
#
=pod 

=head1 NAME

examples/print.pl - A printing example

=head1 FEATURES

Demonstrates the usage of Prima printing interface.
A particular incoherence between *nix and win32 systems
in their printing system is particularly solved by a implicit 
Prima::PS modules set usage.

=cut

use Prima;
use Prima::Lists;
use Prima::Const;
use Prima::Label;
use Prima::MsgBox;
use Prima::PS::Printer;

$::application = Prima::Application-> create( name => "Generic.pm");
my $p = $::application-> get_printer;
my $curr;
my @printers;
my $ps_printer = ($::application->{PrinterClass} =~ /PS/) ? undef : Prima::PS::Printer->new;

my $l;
my $w;
my $display = "DISPLAY";
my $wDisplay;

my $i = Prima::Image-> create;
$i-> load( 'Hand.gif' );
$i-> size( 600, 600);

sub paint
{
	my $p = $_[0];
	my @size = $p-> size;

	$p-> color( cl::LightGray);
	$p-> fill_ellipse( $size[0], $size[1], 300, 300);

	$p-> color( cl::Black);
	$p-> rectangle( 10, 10, $size[0]-10, $size[1]-10);
	$p-> font-> size( 24);
	$p-> text_out( "Print example", 200, 200);
	my $y = 180;
	my $x;
	$p-> font-> name( 'Helv');
	for ( $x = 8; $x < 48; $x+=2)
	{
		$p-> font-> size( $x);
		$p-> text_out( "$x.Helv", 80, $y);
		$y += $p-> font-> height + 4;
	}

	$p-> put_image( 320, 300, $i);
}

sub set_options
{
	my ( $opt, $val) = @_;

	return if $curr == $::application;

	return if $curr-> options( $opt, $val);
	message("Error setting printer option '$opt'");
}

sub print_sample
{
	if ( $curr == $::application) {
		$wDisplay-> destroy if $wDisplay;
		$wDisplay = Prima::Window-> create(
			text    => 'DISPLAY',
			onPaint => sub {
				my ( $self, $canvas) = @_;
				my $c = $canvas-> color;
				$canvas-> color( cl::White);
				$canvas-> bar( 0, 0, $canvas-> size);
				$canvas-> color( $c);
				paint( $canvas);
			},
		);
		$wDisplay-> select;
		return;
	}

	if ( !$curr-> begin_doc)
	{
		message_box( $w-> name, "Error starting print document", mb::Ok|mb::Error);
		return;
	}

	my $ww = Prima::Window-> create(
		borderIcons => 0,
		borderStyle => bs::None,
		size        => [300, 100],
		centered    => 1,
	);
	$ww-> insert( Label =>
		x_centered  => 1,
		text     => 'Printing...',
		font        => {size => 18},
		height      => $ww-> height,
		bottom      => 0,
		valignment  => ta::Center,
	);
	
	paint( $curr);

	$curr-> end_doc;
	$ww-> destroy;
}

sub refresh
{
	my $wasnt = 1;
	@printers = @{$p-> printers};
	my $x = $w-> menu;
	$w-> menu-> A2-> enabled( scalar @printers);

	my @ps_printers;
	@ps_printers = @{ $ps_printer->printers } if $ps_printer;
	$_->{name} = "PostScript: $_->{name}" for @ps_printers;
	push @printers, @ps_printers;

	push( @printers, { name => $display});
	$l-> items([ map {$_ = $_-> {name}} @{[@printers]}]),
	$l-> focusedItem( scalar grep { 
		my $isnt = !$_-> {defaultPrinter}; ($wasnt &&= $isnt) && $isnt 
	} @printers);
	$w-> menu-> A1-> enabled( scalar @printers);
}

$w = Prima::MainWindow-> create(
	text   => 'Print example',
	size      => [400, 200],
	centered  => 1,
	menuItems => [
		['~Print' => [
			[A1 => '~Print sample' => \&print_sample],
			[A2 => 'Printer ~setup...' => 'F2' => 'F2' => sub {$curr-> setup_dialog if $curr != $::application}],
			[A3 => 'Print available fonts...' => sub {
				my $item  = $w-> ListBox1-> get_items($w-> ListBox1-> focusedItem);
				print "\n$item\n---\n";
				for ( @{$curr-> fonts}) { print $_-> {name}."\n"};
				print "---\n";
			}],
			[],
			['~Refresh list' => \&refresh],
		]],
		[ '~Options' => [
			[ "~Orientation" => [
				[ '~Landscape' => sub { set_options( Orientation => 'Landscape' ) } ],
				[ '~Portrait' => sub { set_options( Orientation => 'Portrait' ) } ],
			]],
			[ "~Color" => [
				[ '~Color' => sub { set_options( Color => 'Color' ) } ],
				[ '~Monochrome' => sub { set_options( Color => 'Monochrome' ) } ],
			]],
			[ "~PaperSize" => [
				[ '~A4' => sub { set_options( PaperSize => 'A4' ) } ],
				[ '~Letter' => sub { set_options( PaperSize => 'Letter' ) } ],
			]],
			[],
			[ "Print all options" => sub {
				return if $curr == $::application;
				for my $opt ( $curr-> options) {
					my $val = $curr-> options( $opt);
					$val = "<undefined>" unless defined $val;
					print "$opt:$val\n";
				}
				print "---\n";
			}]
		]],
		['E~xit' => sub {$::application-> close}],
	],
);

$l = $w-> insert( ListBox =>
	pack     => { expand => 1, fill => 'both'},
	name     => 'ListBox1',
	onSelectItem => sub {
		my ( $self, $ref, $sel) = @_;
		return unless $sel;
		my $name = $printers[$$ref[0]]-> {name};

		if ( $name eq $display) {
			$w-> menu-> A2-> enabled( 0);
			$curr = $::application;
			return;
		}

		$w-> menu-> A2-> enabled( 1);
		if ( $name =~ /^PostScript: (.*)/) {
			$ps_printer-> printer( $1);
			$curr = $ps_printer;
		} else {
			$p-> printer( $name);
			$curr = $p;
		}
	},
);

refresh;
$l-> focus;

run Prima;

