#
#  Copyright (c) 1997-2007 Dmitry Karasik
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

use Prima::Utils;

return 1 if gui::GTK2 != Prima::Utils::get_gui;

package Prima::sys::gtk2::FileDialog;
use vars qw(@ISA);
@ISA = qw(Prima::Component);

use Prima;
use Prima::MsgBox;
use Cwd;
use strict;


sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},

		defaultExt  => '',
		fileName    => '',
		filter      => [[ 'All files' => '*.*']],
		filterIndex => 0,
		directory   => '.',

		createPrompt       => 0,
		multiSelect        => 0,
		noTestFileCreate   => 0,
		overwritePrompt    => 1,
		pathMustExist      => 1,
		fileMustExist      => 1,
		showHelp           => 0,
		showDotFiles       => 0,

		openMode           => 1,
		text               => undef,

		noReadOnly         => 0,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	for ( qw( filterIndex openMode)) { $self->{$_}=$profile{$_} }
	for ( qw( filter directory multiSelect showDotFiles
		createPrompt fileMustExist noReadOnly noTestFileCreate
		overwritePrompt pathMustExist showHelp text
	)) { $self->$_($profile{$_}) }
	return %profile;
}

sub quoted_split
{
	my @ret;
	$_ = $_[0];
	s/(\\[^\\\s])/\\$1/g;
	study;
	{
		/\G\s+/gc && redo;
		/\G((?:[^\\\s]|\\.)+)\s*/gc && do { 
			my $z = $1; 
			$z =~ s/\\(.)/$1/g; 
			push(@ret, $z); 
			redo; 
		};
		/\G(\\)$/gc && do { push(@ret, $1); redo; };
	}
	return @ret;
}

sub filter
{
	if ( $#_) {
		my $self   = $_[0];
		my @filter = @{$_[1]};
		@filter = [[ '' => '*']] unless scalar @filter;
		my @exts;
		my @mdts;
		for ( @filter) {
			push @exts, $$_[0];
			push @mdts, $$_[1];
		}
		$self-> {filterIndex} = scalar @exts - 1 
			if $self-> { filterIndex} >= scalar @exts;
		$self-> {filter} = \@filter;
	} else {
		return @{$_[0]-> {filter}};
	}
}

sub filterIndex
{
	if ( $#_) {
		$_[0]-> {filterIndex} = $_[1];
	} else {
		return $_[0]-> {filterIndex};
	}
}

sub directory 
{
	return $_[0]->{directory} unless $#_;
	my ( $self, $dir) = @_;

	if ( not defined($dir) or $dir eq '') {
		$dir = '';
	} elsif ( $dir !~ /^\//) {
		# gtk doesn't like non-absolute paths
		$dir = Cwd::abs_path($dir);
	}
	$self-> {directory} = $dir;
}

sub multiSelect
{
	return $_[0]->{multi_select} unless $#_;
	$_[0]->{multi_select} = $_[1] ? 1 : 0;
}
sub showDotFiles
{
	return $_[0]->{show_hidden} unless $#_;
	$_[0]->{show_hidden} = $_[1];
}

sub overwritePrompt
{
	return $_[0]->{overwrite_prompt} unless $#_;
	$_[0]->{overwrite_prompt} = $_[1] ? 1 : 0;
}


sub fileName
{
	unless ( $#_) {
		return $_[0]->{fileName} unless $_[0]->multiSelect;
		my @s = quoted_split( $_[0]-> {fileName});
		return $s[0] unless wantarray;
		return @s;
	}
	$_[0]->{fileName} = $_[1];
}

sub defaultExt
{
	# unimplemented in GTK
	return $_[0]->{defaultExt} unless $#_;
	$_[0]->{defaultExt} = $_[1];
}

sub openMode
{
	return $_[0]->{openMode} unless $#_;
	$_[0]->{openMode} = $_[1];
}

sub text
{
	return $_[0]->{text} unless $#_;
	$_[0]->{text} = $_[1];
}

sub pathMustExist
{
	return $_[0]->{pathMustExist} unless $#_;
	$_[0]->{pathMustExist} = $_[1];
}

sub fileMustExist
{
	return $_[0]->{fileMustExist} unless $#_;
	$_[0]->{fileMustExist} = $_[1];
}

sub noTestFileCreate
{
	return $_[0]->{noTestFileCreate} unless $#_;
	$_[0]->{noTestFileCreate} = $_[1];
}

sub createPrompt
{
	return $_[0]->{createPrompt} unless $#_;
	$_[0]->{createPrompt} = $_[1];
}

sub noReadOnly
{
	return $_[0]->{noReadOnly} unless $#_;
	$_[0]->{noReadOnly} = $_[1];
}

# dummies
sub sorted { 1 }
sub showHelp { 0 } 

# mere callbacks if someone wants these to inherit
sub ok {} 
sub cancel {} 

sub execute
{
	my $self = $_[0];

	DIALOG: while ( 1) {
		Prima::Application-> sys_action( "gtk2.OpenFile.$_=". $self-> {$_})
			for qw(multi_select overwrite_prompt show_hidden);
		Prima::Application-> sys_action( 'gtk2.OpenFile.filters=' . 
			join("\0", map { "$$_[0] ($$_[1])\0$$_[1]" } @{$self->{filter}}) . "\0\0");
		Prima::Application-> sys_action( 'gtk2.OpenFile.filterindex=' . 
			($self->{filterIndex}));
		Prima::Application-> sys_action( 'gtk2.OpenFile.directory=' . 
			$self->{directory});
		Prima::Application-> sys_action( 'gtk2.OpenFile.title=' . 
			(defined $self->{text} ? $self->{text} : ''));
		my $ret = Prima::Application-> sys_action( 'gtk2.OpenFile.'.
			($self->{openMode}?'open':'save'));
		if ( !defined $ret) {
			$self-> cancel;
			return wantarray ? () : undef;
		}
		$self-> {directory}   = Prima::Application-> sys_action( 'gtk2.OpenFile.directory');
		$self-> {directory}  .= '/' unless $self-> {directory} =~ /\/$/;
		$self-> {fileName}    = $ret;
		$self-> {filterIndex} = Prima::Application-> sys_action( 'gtk2.OpenFile.filterindex');

		# emulate some flags now
		if ( $self-> {pathMustExist}) {
			unless ( -d $self-> {directory}) {
				message_box( $self-> text, "Directory $self->{directory} does not exist", mb::OK | mb::Error);
				next DIALOG;
			}
		}
		
		for my $file ( $self-> fileName) {
			if ( $self-> {fileMustExist}) {
				next if -f $file;
				message_box( $self-> text, "File $file does not exist", mb::OK | mb::Error);
				next DIALOG;
			}

			if ( $self-> {openMode}) {
				if ( $self-> {createPrompt}) {
					if ( Prima::MsgBox::message_box( $self-> text,
						"File $file does not exists. Create?", 
						mb::OKCancel|mb::Information
					) != mb::OK) {
						$self-> cancel;
						return wantarray ? () : undef;
					}
				}
			} else {
				if ( $self-> {noReadOnly} && !(-w $file)) {
					message_box( 
						$self-> text, 
						"File $file is read only", 
						mb::OK | mb::Error
					);
					next DIALOG;
				}
			
				if ( not $self-> {noTestFileCreate}) {
					if ( open FILE, ">>$file") { 
						close FILE; 
					} else {
						message_box( $self-> text, 
							"Cannot create file $file: $!", mb::OK | mb::Error);
						next DIALOG;
					}
				}
			}
		}

		last;
	}
	$self-> ok;
	return $self-> fileName;
}

package Prima::sys::gtk2::OpenDialog;
use vars qw(@ISA);
@ISA = qw(Prima::sys::gtk2::FileDialog);

package Prima::sys::gtk2::SaveDialog;
use vars qw(@ISA);
@ISA = qw(Prima::sys::gtk2::FileDialog);

sub profile_default
{
	return { %{$_[ 0]-> SUPER::profile_default},
		openMode        => 0,
		fileMustExist   => 0,
	}
}

1;

__DATA__

=head1 NAME

Prima::sys::gtk2::FileDialog - GTK2 file system dialogs.

=head1 DESCRIPTION 

The module mimics Prima file dialog classes C<Prima::OpenDialog> and
C<Prima::SaveDialog>, defined in L<Prima::FileDialog>. The class names
registered in the module are the same, but in C<Prima::sys::gtk2> namespace.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::FileDialog>

=cut


