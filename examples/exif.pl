=head1 NAME

examples/exif.pl - print EXIF image data

=cut

use strict;
use warnings;
use Prima::noX11;
use Prima;
use Prima::Image::Exif;

sub usage
{
	print <<USAGE;

$0 - print exif info from supported formats

format: exif filename.jpg
USAGE
	exit 1;
}

usage unless @ARGV;

my $i = Prima::Image->load( $ARGV[0], loadExtras => 1 ) or die "Cannot load $ARGV[0]: $@\n";
my ($e, $ok) = Prima::Image::Exif->read_extras($i, tag_as_string => 1, load_thumbnail => 1);
die "Cannot read exif data in $ARGV[0]: $ok\n" unless $e;

for my $k ( sort keys %$e ) {
	if ( $k eq 'XMP data') {
		print "$k:\n==================\n$e->{$k}\n=====================\n";
	} elsif ( $k eq 'thumbnail' ) {
		my $i = $e->{$k};
		if ( ! ref $i) {
			print "** warning: thumbnail present but cannot be loaded: $e->{$k}\n";
		} else {
			print "Thumbnail: ", $i->width, "x", $i->height, " ", $i->get_bpp, " BPP\n";
		}
	} else {
		for my $v ( @{ $e->{$k} } ) {
			print "$k.@$v\n";
		}
	}
}

