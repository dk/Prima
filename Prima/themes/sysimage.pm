# $Id$
# changes sysimage

package Prima::themes::sysimage;

my %state;

use Prima::StdBitmap;
use Prima::Utils;

sub install
{
	my ( $theme, $install) = @_;
	if ( $install) {
		# install
		my $new = ( $theme eq 'sysimage-win32') ? 
			'Prima::sys::win32' :
			'Prima';
		$new = Prima::Utils::find_image( $new, 'sysimage.gif');
		return 0 unless defined $new;
		$state{$theme} = [
			$Prima::StdBitmap::sysimage,
			$new,
		];
		$Prima::StdBitmap::sysimage = $new;
		return 1;
	} else {
		# uninstall
		if ( $state{$theme}->[1] eq $Prima::StdBitmap::sysimage) {
			$Prima::StdBitmap::sysimage = $state{$theme}->[0];
		}
		delete $state{$theme};
	}
}

Prima::Themes::register( 'Prima::themes::sysimage', 'sysimage-win32',    undef, undef, \&install);
Prima::Themes::register( 'Prima::themes::sysimage', 'sysimage-standard', undef, undef, \&install);
