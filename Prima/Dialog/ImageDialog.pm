package Prima::Dialog::ImageDialog;

use strict;
use warnings;
use Prima qw(ImageViewer ComboBox Label Dialog::FileDialog);

sub filtered_codecs
{
	my $codecs = defined($_[0]) ? $_[0] : Prima::Image-> codecs;
	return map {
		my $n = uc $_-> {fileExtensions}->[0];
		my $x = join( ';', map {"*.$_"} @{$_-> {fileExtensions}});
		[ "$n - $_->{fileType}" => $x ]
	} sort {
		$a-> {fileExtensions}->[0] cmp $b-> {fileExtensions}->[0]
	} @$codecs;
}

sub filtered_codecs2all
{
	my $codecs = defined($_[0]) ? $_[0] : Prima::Image-> codecs;
	return join(';', map {"*.$_"} map { @{$_-> {fileExtensions}}} @$codecs)
}

package Prima::Dialog::ImageOpenDialog;
use vars qw( @ISA);
@ISA = qw(Prima::Dialog::OpenDialog);
use Prima::sys::FS;

{
my %RNT = (
	%{Prima::Dialog-> notification_types()},
	HeaderReady   => nt::Default,
	DataReady     => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_default {
	my $codecs = [ grep { $_-> {canLoad} } @{Prima::Image-> codecs}];
	return {
		%{$_[ 0]-> SUPER::profile_default},
		preview  => 1,
		sizeMin  => [380,400],
		text     => 'Open image',
		filter  => [
			['Images' => Prima::Dialog::ImageDialog::filtered_codecs2all( $codecs)],
			['All files' => '*'],
			Prima::Dialog::ImageDialog::filtered_codecs($codecs),
		],
	}
}


sub init
{
	my $self = shift;
	$self-> {preview} = 0;
	my %profile = $self-> SUPER::init(@_);

	my $pk = $self-> insert( ImageViewer =>
		origin     => [ 524, 120],
		name       => 'PickHole',
		alignment  => ta::Center,
		valignment => ta::Center,
		growMode   => gm::GrowLoX | gm::GrowLoY,
		hScroll    => 0,
		vScroll    => 0,
		zoomPrecision => 1000,
	);
	$pk-> size(($self-> Cancel-> width) x 2); # force square dimension

	$self-> insert( ScrollBar =>
		origin      => [ $pk-> left, $pk-> top + 2],
		width       => $pk-> width,
		selectable  => 1,
		tabStop     => 1,
		name        => 'FrameSelector',
		designScale => undef,
		visible     => 0,
		value       => 0,
		delegations => ['Change'],
		growMode    => gm::GrowLoX | gm::GrowLoY,
	);

	$self-> {Preview} = $self-> insert( CheckBox =>
		origin => [ 524, 80],
		text   => '~Preview',
		size   => [ 96, 36],
		name   => 'Preview',
		delegations => [qw(Click)],
		growMode    => gm::GrowLoX | gm::GrowLoY,
	);

	$self-> insert( Label =>
		origin => [ 524, 5],
		size   => [ 96, 76],
		name   => 'Info',
		text   => '',
		alignment => ta::Center,
		valignment => ta::Top,
		wordWrap => 1,
		growMode   => gm::GrowLoX | gm::GrowHiY,
	);

	$self-> preview( $profile{preview});
	$self-> {frameIndex} = 0;
	return %profile;
}


sub update_preview
{
	my $self = $_[0];
	my $i = $self-> PickHole;
	my $j = $self-> Info;
	my $s = $self-> FrameSelector;

	$i-> image( undef);
	$j-> text('');
	$s-> hide unless $s-> {block};
	$self-> {frameIndex} = 0;
	return unless $self-> preview;

	my $x = $self-> Files;
	$x = $x-> get_items( $x-> focusedItem);
	$i-> image( undef);
	return unless defined $x;

	$x = $self-> directory . $x;
	return unless _f $x;

	$x = Prima::Icon-> load( $x,
		loadExtras => 1,
		wantFrames => 1,
		iconUnmask => 1,
		index => $s-> {block} ? $s-> value : 0,
	);
	return unless defined $x;

	$i-> image( $x);
	my @szA = $x-> size;
	my @szB = $i-> get_active_area(2);
	my $xx = $szB[0]/$szA[0];
	my $yy = $szB[1]/$szA[1];
	my $codecs = $x-> codecs;
	$i-> zoom( $xx < $yy ? $xx : $yy);
	my $text = $szA[0].'x'.$szA[1].'x'. ($x-> type & im::BPP) . " bpp ".
		$codecs-> [$x-> {extras}-> {codecID}]-> {fileShortType};
	$text .= ',grayscale' if $x-> type & im::GrayScale;
	if ( $x-> {extras}-> {frames} > 1) {
		unless ( $s-> {block}) {
			$text .= ",$x->{extras}->{frames} frames";
			$s-> {block} = 1;
			$s-> set(
				visible => 1,
				max     => $x-> {extras}-> {frames} - 1,
				value   => 0,
			);
			$s-> {block} = 0;
		} else {
			$text .= ",frame ". ($s-> value + 1) . " of $x->{extras}->{frames}";
			$self-> {frameIndex} = $s-> value;
		}
	}
	$j-> text( $text);
}


sub preview
{
	return $_[0]-> {Preview}-> checked unless $#_;
	$_[0]-> {Preview}-> checked( $_[1]);
	$_[0]-> update_preview;
}

sub Preview_Click
{
	$_[0]-> update_preview;
}

sub on_endmodal
{
	my $self = $_[0];
	my $i = $self-> PickHole;
	$i-> image(undef);
}


sub Files_SelectItem
{
	my ( $self, $lst) = @_;
	$self-> SUPER::Files_SelectItem( $lst);
	$self-> update_preview if $self-> preview;
}

sub Dir_Change
{
	my ( $self, $lst) = @_;
	$self-> SUPER::Dir_Change( $lst);
	$self-> update_preview if $self-> preview;
}

sub FrameSelector_Change
{
	my ( $self, $fs) = @_;
	return if $fs-> {block};
	my $v = $fs-> value;
	$fs-> {block} = 1;
	$self-> update_preview;
	$fs-> {block} = 0;
}

sub PreviewImage_HeaderReady
{
	my ( $self, $image) = @_;
	$self-> notify(q(HeaderReady), $image);
}

sub PreviewImage_DataReady
{
	my ( $self, $image, $x, $y, $w, $h) = @_;
	$self-> notify(q(DataReady), $x, $y, $w, $h);
}

sub load
{
	my ( $self, %profile) = @_;
	return undef unless defined $self-> execute;
	$profile{loadExtras} = 1 unless exists $profile{loadExtras};
	$profile{index} = $self-> {frameIndex};
	my %im_profile;

	if ( $self-> get_notify_sub('HeaderReady') || $self-> get_notify_sub('DataReady')) {
		$im_profile{name}           = 'PreviewImage';
		$im_profile{delegations}    = [ $self, qw(HeaderReady DataReady)];
	}
	my $img = Prima::Image-> new( %im_profile);
	my $pv  = $profile{progressViewer};
	$pv-> watch_load_progress($img) if $pv;
	my @r = $img-> load( $self-> fileName, %profile);
	$pv-> unwatch_load_progress if $pv;
	unless ( defined $r[-1]) {
		Prima::MsgBox::message("Error loading " . $self-> fileName . ":$@");
		pop @r;
		return undef unless scalar @r;
	}
	return undef, Prima::MsgBox::message( "Empty image") unless scalar @r;
	return $r[0] if !wantarray && ( 1 == scalar @r);
	return @r;
}

package Prima::Dialog::ImageSaveDialog;
use vars qw( @ISA);
@ISA = qw(Prima::Dialog::SaveDialog);

sub profile_default  {
	my $codecs = [ grep { $_-> {canSave} } @{Prima::Image-> codecs}];
	return {
		%{$_[ 0]-> SUPER::profile_default},
		text     => 'Save image',
		filter   => [ Prima::Dialog::ImageDialog::filtered_codecs($codecs) ],
		image    => undef,
		filterDialog => 1,
		useBase64 => 0,
		noTestFileCreate => 1,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {ConvertTo} = $self-> insert( ComboBox =>
		origin => [ 524, 120],
		items  => [],
		enabled => 0,
		name => 'ConvertTo',
		style => cs::DropDownList,
		size  => [ 96, 25],
		growMode   => gm::GrowLoX,
	);
	$self-> insert( Label =>
		origin => [ 524, 150],
		text   => '~Convert to:',
		size  => [ 96, 20],
		name => 'ConvertToLabel',
		focusLink => $self-> {ConvertTo},
		growMode   => gm::GrowLoX,
	);
	$self-> {UseFilter} = $self-> insert( CheckBox =>
		origin => [ 524, 20],
		text   => '~Use filter',
		size   => [ 96, 36],
		name   => 'UseFilter',
		delegations => [qw(Click)],
		growMode   => gm::GrowLoX,
	);
	$self-> {UseBase64} = $self-> insert( CheckBox =>
		origin => [ 524, 60],
		text   => 'As ~base64',
		size   => [ 96, 36],
		name   => 'UseBase64',
		delegations => [qw(Click)],
		growMode   => gm::GrowLoX,
	);

	$self-> {codecFilters} = [];
	$self-> {allCodecs} = Prima::Image-> codecs;
	$self-> {codecs} = [
		sort { $a-> {fileExtensions}->[0] cmp $b-> {fileExtensions}->[0] }
		grep { $_-> {canSave}} @{$self-> {allCodecs}}
		];
	my $codec = $self-> {codecs}-> [$self-> filterIndex];

	$self-> image( $profile{image});
	$self-> filterDialog( $profile{filterDialog});
	$self-> useBase64( $profile{useBase64});
	$self-> ExtensionsLabel-> text("Sav~e as type ($codec->{fileShortType})");

	return %profile;
}

sub on_destroy
{
	for ( @{$_[0]-> {codecFilters}}) {
		next unless $_;
		$_-> destroy;
	}
}

sub update_conversion
{
	my ( $self, $codec) = @_;
	my $i = $self-> {image};
	my $x = $self-> {ConvertTo};
	my $xl = $self-> ConvertToLabel;

	$x-> items([]);
	$x-> text('');
	$x-> enabled(0);
	$xl-> enabled(0);
	return unless $i;

	my $t = $i-> type;
	my @st = @{$codec-> {types}};
	return unless @st;

	my $max = 0;
	my $j = 0;
	for ( @st) {
		return if $_ == $t;
		next if $_ & (im::RealNumber|im::ComplexNumber|im::TrigComplexNumber); # not by default, at least
		$max = $j if ( $st[$max] & im::BPP) < ( $_ & im::BPP);
		$j++;
	}
	for ( @st) {
		my $x = ( 1 << ( $_ & im::BPP));
		$x = '24-bit' if $x == 16777216;
		$x .= ' gray' if $_ & im::GrayScale;
		$x .= ' colors';
		$x = 'float' if $_ == im::Float;
		$x = 'double' if $_ == im::Double;
		$x = 'complex' if $_ == im::Complex;
		$x = 'double complex' if $_ == im::DComplex;
		$_ = $x;
	}

	$x-> items( \@st);
	$x-> focusedItem( $max);
	$x-> enabled(1);
	$xl-> enabled(1);
}

sub Ext_Change
{
	my ( $self, $ext) = @_;
	$self-> SUPER::Ext_Change( $ext);
	my $codec = $self-> {codecs}-> [ $self-> filterIndex];
	my $old   = $self-> Name->text;
	my $new   = $codec->{fileExtensions}->[0];
	if ( $old =~ /\.\w+$/ ) {
		$old =~ s/\.\w+$/.$new/;
		$self-> Name->text($old);
	}
	$self-> ExtensionsLabel-> text("Sav~e as type ($codec->{fileShortType})");
	$self-> update_conversion( $codec);
}

sub filterDialog
{
	return $_[0]-> {UseFilter}-> checked unless $#_;
	$_[0]-> {UseFilter}-> checked( $_[1]);
}

sub useBase64
{
	return $_[0]-> {UseBase64}-> checked unless $#_;
	$_[0]-> {UseBase64}-> checked( $_[1]);
}

sub image
{
	return $_[0]-> {image} unless $#_;
	my ( $self, $image) = @_;
	$self-> {image} = $image;
	if (
		$image &&
		exists($image-> {extras}) &&
		(ref($image-> {extras}) eq 'HASH') &&
		defined($image-> {extras}-> {codecID})
	) {
		my $c = $self-> {allCodecs}-> [$image-> {extras}-> {codecID}];
		my $i = 0;
		for ( @{$self-> {codecs}}) {
			$self-> filterIndex( $i) if $_ == $c;
			$i++;
		}
		$self-> update_conversion( $c);
	}
}

sub on_endmodal
{
	$_[0]-> SUPER::on_endmodal();
	$_[0]-> image( undef); # just freeing the reference
}

sub save
{
	my ( $self, $image, %profile) = @_;
	my $ret;
	my $dup;
	return 0 unless $image;

	$dup = $image;
	my $extras = $image-> {extras};
	$self-> image( $image);

	goto EXIT unless defined $self-> execute;

	unlink $self-> fileName unless $self-> noTestFileCreate;

	$image-> {extras} = { map { $_ => $extras-> {$_} } keys %$extras } ;

	my $fi = $self-> filterIndex;
	my $codec = $self-> {codecs}-> [ $fi];


# loading filter dialog, if any
	if ( $self-> filterDialog) {
		unless ( $self-> {codecFilters}-> [ $fi]) {
			if (
				$image &&
				$codec &&
				length($codec-> {module}) &&
				length( $codec-> {package})
			) {
				my $x = "use $codec->{module};";
				eval $x;
				if ($@) {
					Prima::MsgBox::message(
						"Error invoking $codec->{fileShortType} filter dialog:$@"
					);
				} elsif ( $codec->{package}->can('save_dialog')) {
					$self-> {codecFilters}-> [$fi] =
						$codec-> {package}-> save_dialog( $codec);
				}
			}
		}
	}

	if ( $self-> ConvertTo-> enabled) {
		$dup = $image-> dup;
		$dup-> type( $codec-> {types}-> [ $self-> ConvertTo-> focusedItem]);
	}

# invoking filter dialog
	if ( $self-> filterDialog && $self-> {codecFilters}-> [ $fi]) {
		my $dlg = $self-> {codecFilters}-> [ $fi];
		$dlg-> notify( q(Change), $codec, $dup);
		unless ( $dlg-> execute == mb::OK) {
			$self-> cancel;
			goto EXIT;
		}
	}

# selecting codec
	my $j = 0;
	for ( @{$self-> {allCodecs}}) {
		$dup-> {extras}-> {codecID} = $j, last if $_ == $codec;
		$j++;
	}

	if ( $self->useBase64) {
		my $error;
		my $ok = $dup-> save_stream;
		if (defined $ok) {
			my $f;
			my $r_ok = open $f, ">", $self->fileName;
			$error = "$!", goto FAIL unless $r_ok;
			$error = "$!", goto FAIL unless print $f $ok;
			$error = "$!", goto FAIL unless close $f;
			$ret = 1;
		} else {
			$error = "$@";
		FAIL:
			Prima::MsgBox::message("Error saving " . $self-> fileName . ":$error");
		}
	} else {
		if ( $dup-> save( $self-> fileName, %profile)) {
			$ret = 1;
		} else {
			Prima::MsgBox::message("Error saving " . $self-> fileName . ":$@");
		}
	}

EXIT:
	$self-> image( undef);
	$image-> {extras} = $extras;
	return $ret;
}

1;

=head1 NAME

Prima::Dialog::ImageDialog - image file open and save dialogs

=head1 DESCRIPTION

The module provides standard dialogs specially adjusted for image
loading and saving.

=head1 Prima::Dialog::ImageOpenDialog

Provides a preview widget, allowing the user to view the image file before
loading, and the selection of a frame index for the multi-framed image files.
Instead of the C<execute> call, the L<load> method is used to invoke the dialog
and returns the loaded image as a C<Prima::Image> object.  The loaded object
contains the C<{extras}> hash with the file and image information filled by the
loader. See L<Prima::image-load> for more information.

=head2 SYNOPSIS

	use Prima qw(Application Dialog::ImageDialog);
	my $dlg = Prima::Dialog::ImageOpenDialog-> new;
	my $img = $dlg-> load;
	return unless $img;
	print "$_:$img->{extras}->{$_}\n" for sort keys %{$img-> {extras}};

=for podview <img src="imagedlg.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/Dialog/imagedlg.gif">

=head2 Properties

=over

=item preview BOOLEAN

Selects if the preview functionality is active.
The user can switch it on and off interactively.

Default value: 1

=back

=head2 Methods

=over

=item load %PROFILE

Executes the dialog, and, if successful, loads the image file and frame
selected by the user. Returns the loaded image as a C<Prima::Image> object.
PROFILE is a hash, passed to the C<Prima::Image::load> method. In particular,
it can be used to disable the default loading of extra information in the
C<{extras}> hash variable or to specify a non-default loading option.  For
example, C<< {extras}->{className} = 'Prima::Icon' >> would return the
loaded image as an icon object. See L<Prima::image-load> for more.

C<load> can report the progress of the image loading to the caller, and/or to an
instance of C<Prima::ImageViewer>, if desired. If either (or both)
C<onHeaderReady> and C<onDataReady> notifications are specified, these are
called from the respective event handlers of the image being loaded ( see
L<Prima::image-load/"Loading with progress indicator"> for details).  If
the profile key C<progressViewer> is supplied, its value is treated as a
C<Prima::ImageViewer> instance, and it is used to display the loading
progress. See L<Prima::ImageViewer/watch_load_progress>.

=back

=head2 Events

=over

=item HeaderReady IMAGE

See L<Prima::Image/HeaderReady>.

=item DataReady IMAGE, X, Y, WIDTH, HEIGHT

See L<Prima::Image/DataReady>.

=back

=head1 Prima::Dialog::ImageSaveDialog

Provides the standard image save dialog where the user can select the desired
image format, the bit depth, and other format-specific options. The
format-specific options can be set if a dialog for the file format is provided
by the toolkit.  The standard toolkit dialogs reside under the C<Prima::Image>
namespace, in the F<Prima/Image> subdirectory. For example,
C<Prima::Image::gif> provides the selection of a transparent color, and
C<Prima::Image::jpeg> provides the image quality control. If the image passed
to the L<image> property contains the C<{extras}> hash variable, its data are used
as the default values.  In particular, the C<< {extras}->{codecID} >> field,
responsible for the file format, affects the default file format selection.

=head2 SYNOPSIS

	my $dlg = Prima::Dialog::ImageSaveDialog-> new;
	return unless $dlg-> save( $image );
	print "saved as ", $dlg-> fileName, "\n";

=head2 Properties

=over

=item image IMAGE

Selects the image to be saved. The property is to be used
for the standard invocation of dialog, via the C<execute> method. It is not
needed when the execution and saving are invoked via the L<save> method.

=back

=head2 Methods

=over

=item save IMAGE, %PROFILE

Invokes the dialog, and, if the execution is successful, saves the IMAGE
according to the user selection and PROFILE hash.  PROFILE is not used as a
source of the default options, but is passed directly to the
C<Prima::Image::save> call, possibly overriding the selection of the user.

Returns 1 on success, 0 on error. If the error occurs, the
user is notified before the method returns.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Window>, L<Prima::codecs>, L<Prima::image-load>,
L<Prima::Image>, L<Prima::Dialog::FileDialog>, L<Prima::ImageViewer>, F<examples/iv.pl>.

=cut
