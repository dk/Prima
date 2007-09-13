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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#
#  $Id$

use strict;
use Carp;
use Prima::Classes;
use Prima::Buttons;
use Prima::Label;
use Prima::ComboBox;

package Prima::PrintSetupDialog;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		text        => 'Printer setup',
		width       => 309,
		height      => 143,
		designScale => [7,16],
		centered    => 1,
		visible     => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $cb = $self-> insert( ComboBox =>
		origin => [ 4, 89],
		style  => cs::DropDownList,
		name   => 'Printers',
		size   => [ 299, 20],
		delegations => ['Change'],
	);
	$self-> insert( Button =>
		origin => [ 208, 4],
		size => [ 96, 36],
		text   => 'P~roperties...',
		name   => 'Properties',
		delegations => ['Click'],
	);
	$self-> insert( Label =>
		origin => [ 4, 116],
		size => [ 299, 20],
		text => '~Printer',
		focusLink => $cb,
	);
	$self-> insert( Label =>
		origin => [ 3, 50],
		name => 'Device',
		size => [ 299, 20],
		text => '',
	);
	$self-> insert( Button =>
		origin => [ 4, 4],
		size => [ 96, 36],
		name => 'OK',
		text => '~OK',
		default => 1,
		delegations => ['Click'],
	);
	$self-> insert( Button =>
		origin => [ 107, 4],
		size => [ 96, 36],
		text => 'Cancel',
		name => 'Cancel',
		delegations => ['Click'],
	);
	return %profile;
}

sub on_execute
{
	my $self = $_[0];
	my $oldp = $self-> Printers-> text;
	my @prs = @{$::application-> get_printer-> printers};
	unless ( scalar @prs) {
		$self-> cancel;
		Prima::message("No printers found");
		return;
	}
	$self-> {list} = [ @prs];
	my $p = $self-> Printers;
	my @newprs = @prs;
	@newprs = map {$_ = $_-> {name}} @newprs;
	my $found = 0;
	for ( @newprs) {
		$found = 1, last if $_ eq $oldp;
	}
	unless ( $found) {
		for ( @prs) {
			$oldp = $_-> {name}, last if $_-> {defaultPrinter}
		}
	}
	$p-> items( \@newprs ),
	$p-> text( $oldp);
	my $i = $p-> List-> focusedItem;
	$self-> Device-> text( 'Device: ' . $prs[ $i]-> {device});
	$self-> {oldPrinter} = $oldp;
}

sub Printers_Change
{
	my ( $self, $combic) = @_;
	$::application-> get_printer-> printer( $combic-> text);
	$self-> Device-> text( 'Device: ' . 
		$self-> {list}-> [ $combic-> List-> focusedItem]-> {device}
	);
}


sub Properties_Click
{
	$::application-> get_printer-> setup_dialog;
}

sub OK_Click
{
	$_[0]-> ok;
}

sub Cancel_Click
{
	$::application-> get_printer-> printer( $_[0]-> {oldPrinter});
	$_[0]-> cancel;
}

sub execute
{
	return $_[0]-> SUPER::execute == mb::OK;
}

1;

__DATA__

=pod

=head1 NAME

Prima::PrintDialog - standard printer setup dialog

=head1 DESCRIPTION

Provides a standard dialog that allows the user to select a printer
and its options. The toolkit does not provide the in-depth management
of the printer options; this can only be accessed by executing a printer-specific
setup window, called by C<Prima::Printer::setup_dialog>. The class invokes
this method when the user presses 'Properties' button. Otherwise, the class
provides only selection of a printer from the printer list.

When the dialog finished successfully, the selected printer is set as the current
by writing to C<Prima::Printer::printer> property. This technique allows direct use of
the user-selected printer and its properties without prior knowledge of the
selection process.

=head1 SYNOPSIS

	use Prima::PrintDialog;

	$dlg = Prima::PrintSetupDialog-> create;
	if ( $dlg-> execute) {
		my $p = $::application-> get_printer;
		if ( $p-> begin_doc ) {
			$p-> text_out( 'Hello world', 10, 10);
			$p-> end_doc;
		}
	}
	$dlg-> destroy;

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Window>, L<Prima::Printer>.

=cut
