package Prima::Dialog::PrintDialog;
use strict;
use warnings;
use Carp;
use Prima qw(Buttons Label ComboBox);

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
	my $pdf = ({
		name   => 'Save as PDF',
		device => 'file',
		pdf    => 1,
		object => undef,
	});
	push @prs, $pdf;
	$self-> {list} = [@prs];
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

	if ( $combic-> focusedItem < $combic-> List->count - 1 ) {
		$::application-> get_printer-> printer( $combic-> text);
	}
	$self-> Device-> text( 'Device: ' .
		$self-> {list}-> [ $combic-> List-> focusedItem]-> {device}
	);
}


sub Properties_Click
{
	my $self = shift;
	my $combic = $self-> Printers;
	if ( $combic-> focusedItem < $combic-> List->count - 1 ) {
		$::application-> get_printer-> setup_dialog;
	} else {
		my $p = $self-> create_pdf_printer;
		$p-> setup_dialog if $p;
	}
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

sub create_pdf_printer
{
	my $self = shift;
	unless ( $self->{list}->[1]->{object}) {
		eval 'use Prima::PS::Printer;';
		if ($@) {
			Prima::message($@);
			return undef;
		}
		$self->{list}->[1]->{object} = Prima::PS::PDF::Printer-> new;
	}
	return $self->{list}->[1]->{object};
}

sub printer
{
	my $self = shift;
	my $combic = $self-> Printers;
	if ( $combic-> focusedItem < $combic-> List->count - 1 ) {
		return $::application-> get_printer;
	} else {
		return $self-> create_pdf_printer;
	}
}

1;

=pod

=head1 NAME

Prima::Dialog::PrintDialog - standard printer setup dialog

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

	use Prima qw(Dialog::PrintDialog Application);

	my $dlg = Prima::Dialog::PrintDialog-> new;
	if ( $dlg-> execute) {
		my $p = $dlg-> printer;
		if ( $p-> begin_doc ) {
			$p-> text_out( 'Hello world', 10, 10);
			$p-> end_doc;
		}
	}
	$dlg-> destroy;

=for podview <img src="printdialog.gif" cut=1>

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/printdialog.gif">

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Window>, L<Prima::Printer>.

=cut
