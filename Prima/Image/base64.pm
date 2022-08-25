package Prima::Image::base64;

use strict;
use warnings;
use Prima;

my $has_mime;

sub check_mime
{
	unless ( defined $has_mime ) {
		eval "use MIME::Base64;";
		$has_mime = $@ ? $@ : 1;
	}
}

sub load_class
{
	my $class = shift;
	shift if ($_[0] // '') eq __PACKAGE__;

	check_mime();
	return (undef, $has_mime) unless 1 eq $has_mime;

	my $scalar = decode_base64(shift);
	return (undef, "$!") unless open my $f, '<', \ $scalar;
	return $class->load( $f, @_ );
}

sub load       { load_class('Prima::Image', @_ ) }
sub load_image { load_class('Prima::Image', @_ ) }
sub load_icon  { load_class('Prima::Icon',  @_ ) }

my %codecs;

sub save
{
	check_mime();
	return (undef, $has_mime) unless 1 eq $has_mime;

	my $scalar = '';
	return (undef, "$!") unless open my $f, '>', \ $scalar;

	my ($i, %param) = @_;

	$param{codecID} //= $i->{extras}->{codecID};

	unless ( $param{codecID}) {
		my @want;
		if ( $i->isa('Prima::Icon')) {
			if ( (($i-> type & im::BPP) > 8) || ($i-> maskType > 1)) {
				@want = qw(png webp gif);
			} else {
				@want = qw(gif png webp);
			}
		} elsif (( $i-> type & im::BPP ) > 8) {
			@want = qw(png webp tiff heif bmp gif xpm);
		} else {
			@want = qw(gif png webp tiff bmp xpm);
		}

		unless ( keys %codecs ) {
			for my $codec ( @{Prima::Image->codecs} ) {
				$codecs{ lc $codec->{ fileShortType} } = $codec->{codecID};
			}
		}

		for my $candidate ( @want ) {
			next unless exists $codecs{$candidate};
			$param{codecID} = $codecs{$candidate};
			last;
		}

		$param{codecID} //= $codecs{bmp};
	}

	my $ok = $i->save($f, %param);
	return (undef, "$@") unless $ok;

	return encode_base64( $scalar );
}

1;

=pod

=head1 NAME

Prima::Image::base64 - hard-coded image files

=head1 DESCRIPTION

Handles base64-encoded data streams to load images directly from source code.

=head1 SYNOPSIS

	my $icon = Prima::Icon->load_stream(<<~'ICON');
		R0lGODdhIAAgAIAAAAAAAP///ywAAAAAIAAgAIAAAAD///8CT4SPqcvtD6OctNqLcwogcK91nEhq
		3gim2Umm4+W2IBzX0fvl8jTr9SeZiU5E4a1XLHZ4yaal6XwFoSwMVUVzhoZSaQW6ZXjD5LL5jE6r
		DQUAOw==
		ICON

	print $icon->save_stream;

=head1 API

=over

=item load, load_image BASE64_STRING, %OPTIONS

Decodes BASE64_STRING and tries to load an image from it.
Returns image reference(s) on success, or C<undef, ERROR_STRING> on failure.

=item load_icon BASE64_STRING, %OPTION

Same as C<load_image> but returns a C<Prima::Icon> instance.

=item save IMAGE_OR_ICON, %OPTIONS

Saves image into a datastream and return it encoded in base64.
Unless C<$OPTIONS{codecID}> or C<$image->{extras}->{codecID}> is set, tries to find the best codec for the job.

Returns encoded content on success, or C<undef, ERROR_STRING> on failure.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Image>, L<Prima::image-load>

=cut
