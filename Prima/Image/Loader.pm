package Prima::Image::Loader;

use strict;
use warnings;
use Prima;

sub new
{
	my ( $class, $source, %opt ) = @_;

	$opt{className} = 'Prima::Icon' if delete $opt{icons};

	my %events;
	while ( my ($k, $v) = each %opt) {
		$events{$k} = $v if $k =~ /^on/;
	}

	my $img = Prima::Image->new( %events );
	my $ok = $img->load( $source,
		loadExtras => 1,
		wantFrames => 1,
		%opt,
		session    => 1
	);
	unless ($ok) {
		$img->destroy;
		return (undef, $@);
	}
	if ( ($img->{extras}->{frames} // -1) < 0 ) {
		$img->destroy;
		return (undef, "cannot read number of frames");
	}

	return bless {
		source  => $source,
		image   => $img,
		extras  => $img->{extras},
		frames  => $img->{extras}->{frames},
		current => 0,
		events  => \%events,
	}, $class;
}

sub next
{
	my ( $self, %opt ) = @_;

	return (undef, $self->{error}) if defined $self->{error};

	my $new_frame;
	$new_frame = $opt{index} = delete $self->{rewind_request}
		if defined $self->{rewind_request};
	my @img = $self->{image}->load( undef, %{ $self->{events} } , %opt, session => 1 );
	if ( $img[0] ) {
		my $e = $img[0]->{extras};
		%{ $self->{extras} } = ( %{ $self->{extras} }, %$e ) if $e;
		$self->{current} = $new_frame if defined $new_frame;
		$self->{current}++;
		return $img[0];
	} else {
		$self->{error} = @img ? $@ : "End of image list";
		$self->{image}->destroy;
		$self->{image} = undef;
		return (undef, $self->{error});
	}
}

sub eof
{
	my $self = shift;
	return 1 if defined $self->{error};
	my $current = $self->{current} // 0;
	return $current >= $self->{frames};
}

sub current
{
	return $_[0]->{current} unless $#_;

	my ( $self, $index ) = @_;

	if (defined $self->{error} && $self->{error} ne "End of image list") {
		Carp::carp("cannot rewind after an error is encountered");
		return;
	}

	$self->{current} = $self->{rewind_request} = $index;
	delete $self->{error};
}

sub extras  { shift->{extras} }
sub frames  { shift->{frames} }
sub source  { shift->{source} }
sub DESTROY { $_[0]->{image}-> destroy if $_[0]->{image} }

package Prima::Image::Saver;

sub new
{
	my ( $class, $target, %opt ) = @_;

	my $img = Prima::Image->new;

	my $ok = $img->save( $target,
		unlink     => 0,
		%opt,
		session    => 1
	);
	unless ($ok) {
		$img->destroy;
		return (undef, $@);
	}

	return bless {
		target => $target,
		image  => $img,
		saved  => 0,
	}, $class;
}

sub save
{
	my ( $self, $image, %opt ) = @_;

	my $ok = $self->{image}->save( $image, %opt, session => 1 );
	unlink $self->{target} if !$ok && $self->{unlink};

	return $ok ? 1 : (0, $@);
}

sub DESTROY { $_[0]->{image}-> destroy if $_[0]->{image} }

1;

=pod

=head1 NAME

Prima::Image::Loader - progressive loading and saving for multiframe images

=head1 DESCRIPTION

The toolkit provides functionality for session-based loadign and saving of
multiframe images to that is is not needed to store all images in memory at
once. Instead, the C<Prima::Image::Loader> and C<Prima::Image::Saver> classes
provide the API for operating on a single frame at a time.

=head1 Prima::Image::Loader

	use Prima::Image::Loader;
	my $l = Prima::Image::Loader->new($ARGV[0]);
	printf "$ARGV[0]: %d frames\n", $l->frames;
	while ( !$l->eof ) {
		my ($i,$err) = $l->next;
		die $err unless $i;
		printf "$n: %d x %d\n", $i->size;
	}

=over

=item new FILENAME|FILEHANDLE, %OPTIONS

Opens a filename or a file handle, tries to deduce if the toolkit can recognize
the image, and creates an image loading handler. The C<%OPTIONS> are same as
recognized by L<Prima::Image/load> except C<map>, C<loadAll>, and C<profiles>.
The other options apply to each frame that will be coonsequently loaded, but these
options could be overridden by supplying parameters to the C<next> call.

Returns either a new loader object, or C<undef> and the error string.

Note that it is possible to supply the C<onHeaderReady> and C<onDataReady>
callbacks in the options, however, note that the first arguments in these
callbacks will the newly created image.

=item current INDEX

Manages the index of the frame that will be loaded next.  When set, requests
repositioning of the frame pointer so that the next call to C<next> would load
the INDEXth image.

=item eof

Return the boolean flag is the end of the file is reached.

=item extras

Returns the hash of extra file data filled by the codec

=item frames

Returns number of frames in the file

=item next %OPTIONS

Loads the next image frame.

Returns either a newly loaded image, or C<undef> and the error string.

=item source

Return the filename or the file handle passed to the C<new> call.

=back

=head1 Prima::Image::Saver

	my $fn = '1.webp';
	open F, ">", $fn or die $!;
	my ($s,$err) = Prima::Image::Saver->new(\*F, frames => scalar(@images));
	die $err unless $s;
	for my $image (@images) {
		my ($ok,$err) = $s->save($image);
		next if $ok;
		unlink $fn;
		die $err;
	}

=item new FILENAME|FILEHANDLE, %OPTIONS

Opens a filename or a file handle and an image saving handler. The C<%OPTIONS>
are same as recognized by L<Prima::Image/save> except C<images>.  The other
options apply to each frame that will be coonsequently saved, but these options
could be overridden by supplying parameters to the C<save> call.

Returns either a new saver object, or C<undef> and the error string.

=item save %OPTIONS

Saves the next image frame.

Returns a success boolean flag and an eventual error string

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Dialog::Animate>.

=cut
