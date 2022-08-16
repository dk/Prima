package Prima::IntUtils;

use strict;
use warnings;
use Prima;

warn "** WARNING: Prima::IntUtils is deprecated. Use Prima::Widget::* instead\n";

package Prima::MouseScroller;
use base qw(Prima::Widget::MouseScroller);

package Prima::IntIndents;
use base qw(Prima::Widget::IntIndents);

package Prima::GroupScroller;
use base qw(Prima::Widget::GroupScroller);

package Prima::UndoActions;
use base qw(Prima::Widget::UndoActions);

package Prima::ListBoxUtils;
use base qw(Prima::Widget::ListBoxUtils);

package Prima::BidiInput;
use base qw(Prima::Widget::BidiInput);

1;

=head1 NAME

Prima::IntUtils - internal functions. Deprecated.

=head1 DESCRIPTION

Use Prima::Widget:: individual modules instead.

=cut
