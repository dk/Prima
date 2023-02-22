package Prima::Widget::BidiInput;

use strict;
use warnings;

sub handle_bidi_input
{
	my ( $self, %opt ) = @_;

	my @ret;
	if ( $opt{action} eq 'backspace') {
		return unless length $opt{text};
		if ( $opt{at} == ($opt{rtl} ? 0 : $opt{n_clusters})) {
			chop $opt{text};
			@ret = ( $opt{text}, length($opt{text}));
		} else {
			my $curpos = $opt{glyphs}->cursor2offset($opt{at}, $opt{rtl});
			if ( $curpos > 0 ) {
				substr( $opt{text}, $curpos - 1, 1) = '';
				@ret = ( $opt{text}, $curpos - 1);
			}
		}
	} elsif ( $opt{action} eq 'delete') {
		my $curpos = $opt{glyphs}->cursor2offset($opt{at}, $opt{rtl});
		if ( $curpos < length($opt{text}) ) {
			substr( $opt{text}, $curpos, 1, '');
			@ret = ( $opt{text}, $curpos);
		}
	} elsif ( $opt{action} eq 'cut') {
		if ( $opt{at} != ($opt{rtl} ? 0 : $opt{n_clusters})) {
			my ($pos, $len, $rtl) = $opt{glyphs}-> cluster2range( 
				$opt{at} - ($opt{rtl} ? 1 : 0));
			substr( $opt{text}, $pos + $len - 1, 1, '');
			@ret = ( $opt{text}, $pos + $len - 1);
		}
	} elsif ( $opt{action} eq 'insert') {
		my $curpos = $opt{glyphs}->cursor2offset($opt{at}, $opt{rtl});
		substr( $opt{text}, $curpos, 0) = $opt{input};
		@ret = ( $opt{text}, $curpos );
	} elsif ( $opt{action} eq 'overtype') {
		my ($append, $shift) = $opt{rtl} ? (0, 1) : ($opt{n_clusters}, 0);
		my $curpos;
		if ( $opt{at} == $append) {
			$opt{text} .= $opt{input};
			$curpos = length($opt{text});
		} else {
			my ($pos, $len, $rtl) = $opt{glyphs}-> cluster2range( $opt{at} - $shift );
			$pos += $len - 1 if $rtl;
			substr( $opt{text}, $pos, 1) = $opt{input};
			$curpos = $pos + ($rtl ? 0 : 1);
		}
		@ret = ( $opt{text}, $curpos );
	} else {
		Carp::cluck("bad input $opt{action}");
	}
	return @ret;
}

1;

=head1 NAME

Prima::Widget::BidiInput - heuristics for i18n input

=head1 DESCRIPTION

Provides the common functionality for the bidirectional input to be used in
editable widgets

=head1 Methods

=over

=item handle_bidi_input %OPTIONS

Given C<action> and C<text> in C<%OPTIONS>, returns new text and suggested new cursor position.

The following options are understood:

=over

=item action

One of: backspace, delete, cut, insert, overtype

=item at INTEGER

Current cursor position in clusters

=item glyphs Prima::Drawable::Glyphs object

Shaped text

=item n_clusters INTEGER

Amount of clusters in the text

=item rtl BOOLEAN

Is widget or currently selected method is RTL (right-to-left)

=item text STRING

A text to edit

=back

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Drawable::Glyphs>

=cut
