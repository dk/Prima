package Prima::StdBitmap;
use vars qw($sysimage);

use strict;
use warnings;
use Prima qw(Utils);

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

sub scale
{
	my ($i, %opt) = @_;
	my $scaling; # with exceptions below
	my $index = delete $opt{index};
	delete @opt{qw(class file icon copy)};
	$scaling = ist::Box if $index == sbmp::OutlineCollapse || $index == sbmp::OutlineExpand;
	$i->ui_scale( scaling => $scaling, %opt );
}

sub load_image
{
	my %opt = @_;
	my $i = $opt{class}-> create(name => $opt{index});
	unless ($i-> load( $opt{file}, index => $opt{index})) {
		_warn($opt{file}, $@);
		undef $i;
		return undef;
	} else {
		scale($i, %opt) unless $opt{raw};
		return $i;
	}
}

sub load_std_bmp
{
	my %opt = @_;
	$opt{class} = $opt{icon} ? q(Prima::Icon) : q(Prima::Image);

	return undef if !defined $opt{index} || !defined $opt{file} || $opt{index} < 0;
	return load_image(%opt) if $opt{copy};

	my $icon   = $opt{icon} ? 1 : 0;
	my $index  = $opt{index};
	my $cache  = $bmCache{$opt{file}} //= {};
	return $cache-> {$index}-> [$icon] //= load_image(%opt);
}

$sysimage = Prima::Utils::find_image(
	((Prima::Application-> get_system_info-> {apc} == apc::Win32) ? 'sys/win32/' : '') .
	"sysimage.gif")
	unless defined $sysimage;
_warn('sysimage.gif') unless defined $sysimage;

sub icon { return load_std_bmp( index => shift, file => $sysimage, icon => 1, @_); }
sub image{ return load_std_bmp( index => shift, file => $sysimage, icon => 0, @_); }

1;

=pod

=head1 NAME

Prima::StdBitmap - shared access to the standard bitmaps

=head1 DESCRIPTION

The toolkit provides the F<sysimage.gif> file that contains the standard Prima
image library and consists of a predefined set of images used by different
modules. To provide unified access to the images inside the file, this
module's API can be used. Every image is assigned to a C<sbmp::> constant that
is used as an index for an image loading request. If an image is loaded
successfully, the result is cached and the successive requests use the cached
image.

The images can be loaded as C<Prima::Image> and C<Prima::Icon> instances,
by two methods, correspondingly C<image> and C<icon>.

=head1 SYNOPSIS

   use Prima::StdBitmap;
   my $logo = Prima::StdBitmap::icon( sbmp::Logo );

=head1 API

=head2 Methods

=over

=item icon INDEX

Loads the INDEXth image frame and returns a C<Prima::Icon> instance.

=item image INDEX

Loads the INDEXth image frame and returns a C<Prima::Image> instance.

=item load_std_bmp %OPTIONS

Loads the C<index>th image frame from C<file> and returns it as either a
C<Prima::Image> or a C<Prima::Icon> instance, depending on the value of the
boolean C<icon> flag. If the C<copy> boolean flag is unset, a cached image can
be used. If this flag is set, a cached image is never used and the created
image is neither stored in the cache. Since the module's intended use is to
provide shared and read-only access to the image library, C<copy> set to 1 can
be used to return non-shareable images.

The loader automatically scales images if the system dpi suggests so. If
layering is supported, the icon scaling will use that as well. To disable these
optimizations use the C<< raw => 1 >> flag to disable all optimizations, and
C<< argb => 0 >> to disable producing ARGB icons.

=back

=head2 Constants

An index value passed to the methods must be one of the C<sbmp::> constants:

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
	sbmp::OutlineCollapse
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

The C<$sysimage> scalar is initialized to the file name to be used as a source
of standard images. It is possible to alter this scalar at run-time, which
causes all subsequent image frame requests to be redirected to the new file.

=head2 Scaling and ARGB-shading

The loading routine scales and visually enhances the images automatically
according to the system settings that are reported by the C<Prima::Application>
class.  It is therefore advisable to load images after the Application object
is created.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Image>, L<Prima::Const>.

=cut
