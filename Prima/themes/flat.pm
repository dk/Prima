use strict;
use warnings;
use Prima qw(Themes);
package Prima::Themes::flat;

sub install
{
	my ( $theme, $install) = @_;
	if ( $install) {
		$::application-> designStyle('flat');
		return 1;
	} else {
		$::application-> designStyle('default');
	}
}

Prima::Themes::register( 'Prima::themes::flat', 'flat',  undef, undef, \&install);
