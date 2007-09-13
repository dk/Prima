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
#  Created by:
#     Anton Berezin  <tobez@tobez.org>
#     Dmitry Karasik <dk@plab.ku.dk> 
#
#  $Id$



# Contains stubs for load-on-demand of the following modules:
#   Prima::OpenDialog       => Prima/FileDialog.pm
#   Prima::SaveDialog       => Prima/FileDialog.pm
#   Prima::ChDirDialog      => Prima/FileDialog.pm
#   Prima::FontDialog       => Prima/FontDialog.pm
#   Prima::FindDialog       => Prima/EditDialog.pm
#   Prima::ReplaceDialog    => Prima/EditDialog.pm
#   Prima::PrintSetupDialog => Prima/PrintDialog.pm
#   Prima::ColorDialog      => Prima/ColorDialog.pm
#   Prima::ImageOpenDialog  => Prima/ImageDialog.pm
#   Prima::ImageSaveDialog  => Prima/ImageDialog.pm

no strict;

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

package Prima;

my ($openFileDlg, $saveFileDlg);
	
my @fileDlgProps = qw( defaultExt fileName filter filterIndex
directory createPrompt multiSelect noReadOnly noTestFileCreate overwritePrompt
pathMustExist fileMustExist sorted showDotFiles);

sub open_file
{
	my %profile = @_;
	$openFileDlg = Prima::OpenDialog-> create( 
		system => exists($profile{system}) ? $profile{system} : 1,
		onDestroy => sub { undef $openFileDlg},
	) unless $openFileDlg;
	delete $profile{system};
	my %a = %{$openFileDlg-> profile_default};
	$openFileDlg-> set(( map { $_ => $a{$_}} @fileDlgProps), %profile);
	return $openFileDlg-> execute;
}

sub save_file
{
	my %profile = @_;
	$saveFileDlg = Prima::SaveDialog-> create( 
		system => exists($profile{system}) ? $profile{system} : 1,
		onDestroy => sub { undef $saveFileDlg},
	) unless $saveFileDlg;
	delete $profile{system};
	my %a = %{$saveFileDlg-> profile_default};
	$saveFileDlg-> set(( map { $_ => $a{$_}} @fileDlgProps), %profile);
	return $saveFileDlg-> execute;
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

package mb;

use constant ChangeAll => 0xCA11;

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

package Prima::PrintSetupDialog;

sub AUTOLOAD
{
	my ($method) = $Prima::PrintSetupDialog::AUTOLOAD =~ /::([^:]+)$/;
	delete ${Prima::PrintSetupDialog::}{AUTOLOAD};
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

__DATA__

=pod

=head1 NAME

Prima::StdDlg - wrapper module to the toolkit standard dialogs

=head1 DESCRIPTION

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

=item Prima::PrintSetupDialog

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
