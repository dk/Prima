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
=item NAME

A menu usage example

=item FEATURES

Demonstrates the usage of Prima menu API:

- one-call ( large array ) menu set
- text and image menu item manipulations

Note the "Edit/Kill menu" realisation.

=cut

use strict;
use Prima qw( InputLine Label Application);

package TestWindow;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

#    Menu item must be an array with up to 5 items in -
# [variable, text or image, accelerator text, shortcut key, sub or command]
# if there are 0 or 1 items, it's treated as divisor line;
# same divisor in main level of menu makes next part right-adjacent.
# The priority is: text(2), then sub(5), then variable(1), then accelerator(3)
# and shortcut(4). So, if there are 2 items, they are text and sub,
# 3 - variable, text and sub, 4 - text, accelerator, key and sub
# ( because of accelerator and key must be present both and never separately),
# and 5 - all of the above.
# See example below.
# Notes:
#   1. When adding image, scalar must be object derived from Image component
#  or Image component by itself.
#   2. You cannot assign to shortcut key modificator keys.

sub create_menu
{
	my $img = Prima::Image-> create;
	$0 =~ /^(.*)(\\|\/)[^\\\/]+$/;
	$img-> load(( $1 || '.') . '/Hand.gif');
	return [
		[ "~File" => [
			[ "Anonymous" => "Ctrl+D" => '^d' => sub { print "sub!\n";}],   # anonymous sub
			[ $img => sub {
				my $img = $_[0]-> menu-> image( $_[1]);
				my @r = @{$img-> palette};
				$img-> palette( [reverse @r]);
				$_[0]-> menu-> image( $_[1], $img);
			}],                        # image
			[],                                       # division line
			[ "E~xit" => "Exit"    ]    # calling named function of menu owner
		]],
		[ ef => "~Edit" => [                  # example of system commands usage
			[ "Cop~y"  => sub { $_[0]-> foc_action('copy')}  ],     # try these with input line focused
			[ "Cu~t"   => sub { $_[0]-> foc_action('cut')}   ],
			[ "Pa~ste" => sub { $_[0]-> foc_action('paste')} ],
			[],
			["~Kill menu"=>sub{ $_[0]-> menuItems(
				[
					[ "~Restore all" => sub {
						$_[0]-> menuItems( $_[0]-> create_menu)
					}],
					[ "~Incline" => sub {
						$_[0]-> menu-> insert( $_[0]-> create_menu, '', 1);
					}],
				]);
			}],
			["~Duplicate menu"=>sub{ TestWindow-> create( menu=>$_[0]-> menu)}],
		]],
		[ "~Input line" => [
			[ "Print ~text" => "Text"],
			[ "Print ~selected" => "Selected"],
			[ "Try \"selText\"" => "SelText"],
			[],
			[ "Toggle insert mode" => "InsMode"],
			["Toggle password mode" => "PassMode"],
			["Toggle border existence" => "BorderMode"],
			[ coexistentor => "Coexistentor"=> ""],
		]],
		[],                             # divisor in main menu opens
		[ "~Clusters" => [              # right-adjacent part
		[ "*".checker =>  "Checking Item"   => "Check"     ],
		[],
		[ "-".slave   =>  "Disabled state"   => "PrintText"],
		[ master  =>  "~Enable item above" => "Enable"     ]   # enable/disable and text sample
		]]
	];
}

sub foc_action
{
	my ( $self, $action) = @_;
	my $foc = $self-> selectedWidget;
	return unless $foc and $foc-> alive;
	my $ref = $foc-> can( $action);
	$ref-> ( $foc) if $ref;
}

sub Exit
{
	$::application-> destroy;
}

sub Check
{
	my $menu = $_[ 0]-> menu;
	$menu-> checked( 'checker', ! $menu-> checked( 'checker'));
}

sub PrintText
{
	print $_[ 0]-> menu-> slave-> text;
}

sub Enable
{
	my $slave  = $_[0]-> menu-> slave;
	my $master = $_[0]-> menu-> master;
	if ( $slave-> enabled) {
		$slave -> text( "Disabled state");
		$master-> text( "~Enable item above");
	} else {
		$slave -> text( "Enabled state");
		$master-> text( "~Disable item above");
	}
	$slave-> enabled( ! $slave-> enabled);
}

sub Text
{
	print $_[ 0]-> InputLine1-> text;
}

sub Selected
{
	print $_[ 0]-> InputLine1-> selText;
}

sub SelText
{
	$_[ 0]-> InputLine1-> selText ("SEL");
}

sub InsMode
{
	my $e = $_[ 0]-> InputLine1;
	$e-> insertMode ( !$e-> insertMode);
}

sub PassMode
{
	my $e = $_[ 0]-> InputLine1;
	$e-> writeOnly ( !$e-> writeOnly);
}


sub BorderMode
{
	my $e = $_[ 0]-> InputLine1;
	$e-> borderWidth (( $e-> borderWidth == 1) ? 0 : 1);
}


package UserInit;

my $w = TestWindow-> create(
	text      => "Menu and input line example",
	bottom    => 300,
	size      => [ 360, 120],
	menuItems => TestWindow::create_menu,
	onDestroy => sub {$::application-> close},
);
$w-> insert( "InputLine",
	pack      => { pady => 20, padx => 20, fill => 'x', side => 'bottom'},
	text      => $w-> text,
	maxLen    => 200,
	onChange  => sub {
		$_[0]-> owner-> text( $_[0]-> text);
		$_[0]-> owner-> Label1-> text( $_[0]-> text);
		$_[0]-> owner-> menu-> coexistentor-> text( $_[0]-> text)
			if $_[0]-> owner-> menu-> has_item( 'coexistentor');
	},
);

$w-> insert( "Label",
	pack      => { pady => 20, padx => 20, fill => 'both', expand => 1},
	text   => "Type here something",
	backColor => cl::Green,
	valignment => ta::Center,
	focusLink => $w-> InputLine1,
);
run Prima;
