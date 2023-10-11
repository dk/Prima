package Prima::Widget::StartupWindow;
use strict;
use warnings;
use Prima;
use vars qw(@windowStack @pointersStack);

sub import {
	shift;
	my %profileDefault = (
		size        => [$::application-> width/5,$::application-> height/6],
		centered    => 1,
		widgetClass => wc::Dialog,
		borderIcons => 0,
		borderStyle => bs::Dialog,
		color       => cl::Cyan,
		text        => 'Loading...',
		onPaint     => sub {
			my ($me,$canvas) = @_;
			my $fore = $me-> color;
			$canvas-> color( $me-> backColor);
			$canvas-> bar( 0, 0, $me-> size);
			$canvas-> color( $fore);
			$canvas-> font( name => "Tms Rmn", size => 24, style => fs::Bold);
			my $txt = $me-> text;
			$canvas-> text_shape_out( $txt, ($me-> width - $me-> get_text_width($txt))/2, ($me-> height - $me-> font-> height)/2);
		},
	);
	my %profile = (%profileDefault,@_);
	push @pointersStack, $::application-> pointer;
	$::application-> pointer( cr::Wait);
	my $w = Prima::Window-> create(%profile);
	push @windowStack, $w;
	$::application-> yield;
	$w-> repaint;
	$w-> update_view;
}

sub unimport {
	my $w = pop @windowStack;
	return unless $w;
	my $oautoClose = $::application-> autoClose;
	$::application-> autoClose(0);
	$w-> destroy;
	$::application-> autoClose( $oautoClose);
	$::application-> pointer( pop @pointersStack);
}

1;

=pod

=head1 NAME

Prima::Widget::StartupWindow - a simplistic startup banner window

=head1 DESCRIPTION

The module, when imported by the C<use> call, creates a temporary window
which appears with the 'loading...' text while the modules required by
a program are loading. The window parameters can be modified by
passing custom parameters after the C<use Prima::Widget::StartupWindow> statement,
that are passed to the C<Prima::Window> class as creation parameters.
The window is discarded by explicit unimporting of the module
( see L<"SYNOPSIS">  ).

=head1 SYNOPSIS

	use Prima;
	use Prima::Application;
	use Prima::Widget::StartupWindow; # the window is created here

	use Prima::Buttons;
	.... # lots of 'use' of other modules

	no Prima::Widget::StartupWindow; # the window is discarded here

=head1 AUTHORS

Anton Berezin E<lt>tobez@tobez.orgE<gt>,
Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt> ( documentation ).

=head1 SEE ALSO

L<Prima>, L<Prima::Application>.

=cut
