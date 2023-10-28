#
#  Module for simulated extreme situations, useful for testing.
#
package Prima::Stress;
use strict;
no warnings;
use Prima;

our %stress = (
	fs  => undef,
	dpi => undef,
	scr => undef,
);

sub import
{
	shift;

	for my $p ( @_ ) {
		if ( $p =~ /^(\w+)=(\d+)$/ && exists $stress{$1}) {
			$stress{$1} = $2;
		} else {
			warn "bad command: '$p'\n";
		}
	}

	unless ( @_ ) {
		$stress{fs}  = ( 5, 7, 14, 18) [ int(rand(4)) ] ;
		$stress{scr}  = ( 7, 14, 30, 40) [ int(rand(4)) ] ;
		$stress{dpi} = ( 48, 96, 192, 288 ) [int(rand(4)) ];
		warn "** Prima::Stress: fs=$stress{fs} dpi=$stress{dpi} scr=$stress{scr}\n";
	}

	Prima::Application::add_startup_notification( sub {
		if ( $stress{dpi}) {
			$::application->sys_action("resolution $stress{dpi} $stress{dpi}");
			$::application->uiScaling(0);
			$::application->font( $::application-> get_default_font );
			$::application->HintWidget->font( $::application-> get_default_font );
		}
		if ($stress{fs}) {
			$::application->font->size( $stress{fs});
		}
	});

	if ( $stress{fs}) {
		*Prima::Widget::get_default_font = sub {
			Prima::Drawable-> font_match( { size => $Prima::Stress::stress{fs} }, {});
		};
		*Prima::Application::get_default_font = sub {
			Prima::Drawable-> font_match( { size => $Prima::Stress::stress{fs} }, {});
		};
	}

	if ( $stress{scr}) {
		my $gsv = \&Prima::Application::get_system_value;
		*Prima::Application::get_system_value = sub {
			my ( undef, $val ) = @_;
			return $stress{scr} if $val == sv::XScrollbar || $val == sv::YScrollbar;
			goto $gsv;
		};
		*Prima::Application::get_default_scrollbar_metrics = sub { $stress{scr}, $stress{scr} };
	}
}

1;

=pod

=head1 NAME

Prima::Stress - stress test module

=head1 DESCRIPTION

The module is intended for use in test purposes, to check the functionality of
a program or a module under particular conditions that might be overlooked
during the design. Currently, the stress factors implemented are the default
font size, default scrollbar sizes, and the UI-scaling factor, which are set to
different random values every time the module is invoked.

To use the module it is enough to include a typical

	use Prima::Stress;

code, or, if the program is invoked by calling perl, by using the

	perl -MPrima::Stress program

syntax. The module does not provide any methods, however, one may address
individual aspects of the UI defaults.

=head1 API

=head2 Font size

   use Prima::Stress q(fs=18);
   perl -MPrima::Stress=fs=18 program

This syntax changes the default font size to 18 points.

=head2 Display resolution

   use Prima::Stress q(dpi=192);
   perl -MPrima::Stress=dpi=192 program

This syntax changes the display resolution to 192 pixels per inch.

=head2 Scrollbar sizes

   use Prima::Stress q(src=40);
   perl -MPrima::Stress=src=40 program

This syntax changes the default width of vertical scrollbars, and the default
height of horizontal scrollbars to 40 pixels

=head1 AUTHOR

Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>,

=cut
