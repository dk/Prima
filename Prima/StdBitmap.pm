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
package Prima::StdBitmap;
use vars qw($sysimage);

use strict;
use Prima;
use Prima::Utils;

my %bmCache;
my $warned;

sub _warn
{
	my ($img, $fail) = @_;
	return if $warned;
	$warned++;
	if ( defined $fail) {
		warn "Failed to load standard bitmap '$img':$@. Did you compile Prima with GIF support?\n";
	} else {
		warn "Failed to load standard bitmap '$img'. Did you install Prima correctly?\n";
	}
}

sub load_std_bmp
{
	my ( $index, $asIcon, $copy, $imageFile) = @_;
	my $class = ( $asIcon ? q(Prima::Icon) : q(Prima::Image));
	return undef if !defined $index || !defined $imageFile || $index < 0;
	$asIcon = ( $asIcon ? 1 : 0);
	if ( $copy) {
		my $i = $class-> create(name => $index);
		undef $i unless $i-> load( $imageFile, index => $index);
		_warn($imageFile, $@) unless $i;
		return $i;
	}
	$bmCache{$imageFile} = {} unless exists $bmCache{$imageFile};
	my $x = $bmCache{$imageFile};
	return $x-> {$index}-> [$asIcon] if exists $x-> {$index} && defined $x-> {$index}-> [$asIcon];
	$x-> {$index} = [ undef, undef] unless exists $x-> {$index};
	my $i = $class-> create(name => $index);
	undef $i unless $i-> load( $imageFile, index => $index);
	_warn($imageFile, $@) unless $i;
	$x-> {$index}-> [$asIcon] = $i;
	return $i;
}

$sysimage = Prima::Utils::find_image(
	((Prima::Application-> get_system_info-> {apc} == apc::Win32) ? 'sys/win32/' : '') .
	"sysimage.gif") 
	unless defined $sysimage;
_warn('sysimage.gif') unless defined $sysimage;

sub icon { return load_std_bmp( $_[0], 1, 0, $sysimage); }
sub image{ return load_std_bmp( $_[0], 0, 0, $sysimage); }

1;

__DATA__

=pod

=head1 NAME

Prima::StdBitmap - shared access to the standard toolkit bitmaps

=head1 DESCRIPTION

The toolkit contains F<sysimage.gif> image library, which consists of 
a predefined set of images, used in several toolkit modules. To provide
a unified access to the images this module can be used. The images are
assigned a C<sbmp::> constant, which is used as an index on a load
request. If loaded successfully, images are cached and the successive
requests return the cached values.

The images can be loaded as C<Prima::Image> and C<Prima::Icon> instances.
To discriminate, two methods are used, correspondingly C<image> and C<icon>.

=head1 SYNOPSIS

	use Prima::StdBitmap;
	my $logo = Prima::StdBitmap::icon( sbmp::Logo );

=head1 API

=head2 Methods

=over

=item icon INDEX

Loads INDEXth image frame and returns C<Prima::Icon> instance.

=item image INDEX

Loads INDEXth image frame and returns C<Prima::Image> instance.

=item load_std_bmp INDEX, AS_ICON, USE_CACHED_VALUE, IMAGE_FILE 

Loads INDEXth image frame from IMAGE_FILE and returns it as either a C<Prima::Image> or
as a C<Prima::Icon> instance, depending on value of boolean AS_ICON flag. If
USE_CACHED_VALUE boolean flag is set, the cached images loaded previously 
can be used. If this flag is unset, the cached value is never used, and the
created image is not stored in the cache. Since the module's intended use
is to provide shared and read-only access to the image library, USE_CACHED_VALUE
set to 0 can be used to return non-shared images.

=back

=head2 Constants

An index value passed to the methods must be one of C<sbmp::> constants:

	sbmp::Logo
	sbmp::CheckBoxChecked
	sbmp::CheckBoxCheckedPressed
	sbmp::CheckBoxUnchecked
	sbmp::CheckBoxUncheckedPressed
	sbmp::RadioChecked
	sbmp::RadioCheckedPressed
	sbmp::RadioUnchecked
	sbmp::RadioUncheckedPressed
	sbmp::Warning
	sbmp::Information
	sbmp::Question
	sbmp::OutlineCollaps
	sbmp::OutlineExpand
	sbmp::Error
	sbmp::SysMenu
	sbmp::SysMenuPressed
	sbmp::Max
	sbmp::MaxPressed
	sbmp::Min
	sbmp::MinPressed
	sbmp::Restore
	sbmp::RestorePressed
	sbmp::Close
	sbmp::ClosePressed
	sbmp::Hide
	sbmp::HidePressed
	sbmp::DriveUnknown
	sbmp::DriveFloppy
	sbmp::DriveHDD
	sbmp::DriveNetwork
	sbmp::DriveCDROM
	sbmp::DriveMemory
	sbmp::GlyphOK
	sbmp::GlyphCancel
	sbmp::SFolderOpened
	sbmp::SFolderClosed
	sbmp::Last

=head2 Scalars

C<$sysimage> scalar is initialized to the file name to be used
as a source of standard image frames by default. It is possible
to alter this scalar at run-time, which causes all subsequent
image frame request to be redirected to the new file.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Image>, L<Prima::Const>.

=cut
