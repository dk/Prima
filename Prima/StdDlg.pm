warn("Warning: Prima::StdDlg is deprecated. Use Prima::Dialog:: modules instead\n");

#  Created by:
#     Anton Berezin  <tobez@tobez.org>
#     Dmitry Karasik <dk@plab.ku.dk>
#

# Contains stubs for load-on-demand of the following modules:
#   Prima::OpenDialog       => Prima/FileDialog.pm
#   Prima::SaveDialog       => Prima/FileDialog.pm
#   Prima::ChDirDialog      => Prima/FileDialog.pm
#   Prima::FontDialog       => Prima/FontDialog.pm
#   Prima::FindDialog       => Prima/EditDialog.pm
#   Prima::ReplaceDialog    => Prima/EditDialog.pm
#   Prima::PrintDialog      => Prima/PrintDialog.pm
#   Prima::ColorDialog      => Prima/ColorDialog.pm
#   Prima::ImageOpenDialog  => Prima/ImageDialog.pm
#   Prima::ImageSaveDialog  => Prima/ImageDialog.pm

package Prima::StdDlg;
use strict;
use warnings;


package Prima::ColorDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::ColorDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::ColorDialog::}{AUTOLOAD};
	eval "use Prima::ColorDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::FontDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::FontDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::FontDialog::}{AUTOLOAD};
	eval "use Prima::FontDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}


package Prima::OpenDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::OpenDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::OpenDialog::}{AUTOLOAD};
	delete ${Prima::SaveDialog::}{AUTOLOAD};
	delete ${Prima::ChDirDialog::}{AUTOLOAD};
	eval "use Prima::FileDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::SaveDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::SaveDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::OpenDialog::}{AUTOLOAD};
	delete ${Prima::SaveDialog::}{AUTOLOAD};
	delete ${Prima::ChDirDialog::}{AUTOLOAD};
	eval "use Prima::FileDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::ChDirDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::ChDirDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::OpenDialog::}{AUTOLOAD};
	delete ${Prima::SaveDialog::}{AUTOLOAD};
	delete ${Prima::ChDirDialog::}{AUTOLOAD};
	eval "use Prima::FileDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::FindDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::FindDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::FindDialog::}{AUTOLOAD};
	delete ${Prima::ReplaceDialog::}{AUTOLOAD};
	eval "use Prima::EditDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::ReplaceDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::ReplaceDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::FindDialog::}{AUTOLOAD};
	delete ${Prima::ReplaceDialog::}{AUTOLOAD};
	eval "use Prima::EditDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::PrintDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::PrintDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::PrintDialog::}{AUTOLOAD};
	eval "use Prima::PrintDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::ImageOpenDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::ImageOpenDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::ImageOpenDialog::}{AUTOLOAD};
	delete ${Prima::ImageSaveDialog::}{AUTOLOAD};
	eval "use Prima::ImageDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

package Prima::ImageSaveDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::ImageSaveDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::ImageOpenDialog::}{AUTOLOAD};
	delete ${Prima::ImageSaveDialog::}{AUTOLOAD};
	eval "use Prima::ImageDialog"; die "$@\n" if $@;
	shift-> $method(@_);
}

1;

=pod

=head1 NAME

Prima::StdDlg - [DEPRECATED] wrapper module to the toolkit standard dialogs

=head1 DESCRIPTION

This module is deprecated and is on the way out. Use standard dialogs by using
individual modules in C<Prima::Dialog::> namespace.

Provides a unified access to the toolkit dialogs, so there is
no need to C<use> the corresponding module explicitly.

=head1 SYNOPSIS

	use Prima::StdDlg;

	Prima::FileDialog-> create-> execute;
	Prima::FontDialog-> create-> execute;

	# open standard file open dialog
	my $file = Prima::open_file;
	print "You've selected: $file\n" if defined $file;

=head1 API

The module accesses the following dialog classes:

=over

=item Prima::open_file

Invokes standard file open dialog and return the selected file(s).
Uses system-specific standard file open dialog, if available.

=item Prima::save_file

Invokes standard file save dialog and return the selected file(s).
Uses system-specific standard file save dialog, if available.

=item Prima::OpenDialog

File open dialog.

See L<Prima::FileDialog/Prima::OpenDialog>

=item  Prima::SaveDialog

File save dialog.

See L<Prima::FileDialog/Prima::SaveDialog>

=item Prima::ChDirDialog

Directory change dialog.

See L<Prima::FileDialog/Prima::ChDirDialog>

=item Prima::FontDialog

Font selection dialog.

See L<Prima::FontDialog>.

=item Prima::FindDialog

Generic 'find text' dialog.

See L<Prima::EditDialog>.

=item Prima::ReplaceDialog

Generic 'find and replace text' dialog.

See L<Prima::EditDialog>.

=item Prima::PrintDialog

Printer selection and setup dialog.

See L<Prima::PrintDialog>.

=item Prima::ColorDialog

Color selection dialog.

See L<Prima::ColorDialog/Prima::ColorDialog>.

=item Prima::ImageOpenDialog

Image file load dialog.

See L<Prima::ImageDialog/Prima::ImageOpenDialog>.

=item Prima::ImageSaveDialog

Image file save dialog.

See L<Prima::ImageDialog/Prima::ImageSaveDialog>.

=back

=head1 AUTHORS

Anton Berezin E<lt>tobez@plab.ku.dkE<gt>,
Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Classes>


=cut
