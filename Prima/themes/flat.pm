use strict;
use warnings;
use Prima qw(Themes);
package Prima::Themes::flat;

sub install
{
	my ( $theme, $install) = @_;
	if ( $install) {
		$::application-> skin($theme);
		return 1;
	} else {
		$::application-> skin('default');
	}
}

Prima::Themes::register( 'Prima::themes::flat', 'classic',  undef, undef, \&install);
Prima::Themes::register( 'Prima::themes::flat', 'flat',     undef, undef, \&install);
