package Prima::PS::Format;

use strict;
use warnings;
use base 'Exporter';
our @EXPORT_OK = qw(float_format float_inplace);
our @EXPORT = qw(float_format float_inplace);

sub float_format
{
	my @r;
	for ( @_ ) {
		my $k = sprintf "%.4f", $_;
		$k =~ s/\.?0+$//;
		push @r, $k;
	}
	return (wantarray || @_ > 1) ? @r : $r[0];
}

sub float_inplace(@) { $_ = float_format $_ for @_ }

1;
