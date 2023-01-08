package Prima::sys::GUIException;

use Prima;
use Prima::Classes;

$Prima::Application::GUI_EXCEPTION = 1;

1;

=pod

=head1 NAME

Prima::sys::GUIException - shortcut for L<Prima::Application/guiException>

=head1 SYNOPSIS

   use Prima qw(Buttons Application sys::GUIException);

=cut
