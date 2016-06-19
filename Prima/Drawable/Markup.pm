package Prima::Drawable::Markup;

use strict;
use warnings;
use Prima qw(Bidi);
use base qw(Prima::Drawable::TextBlock);

=head1 NAME

Prima::Markup - Allow markup in Prima Widgets

=head1 SYNOPSIS

    use Prima qw(Application Buttons Drawable::Markup);
    Prima::Button->new(
        ...
	text => Prima::Drawable::Markup->new(text => "B<Bold> button"),
    );

=head1 DESCRIPTION

C<Prima::Markup> adds the ability to recognize POD-like markup to Prima
widgets. Supported markup sequences are C<B> (bold text), C<I> (italic text),
C<U> (underlined text), C<F> (change font), C<S> (change font size), C<C>
(change color), and C<W> (disable wrapping).

The C<F> sequence is used as follows: C<FE<lt>n|textE<gt>>, where C<n> is a
0-based index into the C<fontPalette>.

The C<S> sequence is used as follows: C<SE<lt>n|textE<gt>>, where C<n> is the
number of points relative to the current font size. The font size may
optionally be preceded by C<+> or C<->.

The C<C> sequence is used as follows: C<CE<lt>c|textE<gt>>, where C<c> is either: a color
in any form accepted by Prima, including the C<cl> constants (C<Black> C<Blue>
C<Green> C<Cyan> C<Red> C<Magenta> C<Brown> C<LightGray> C<DarkGray> C<LightBlue>
C<LightGreen> C<LightCyan> C<LightRed> C<LightMagenta> C<Yellow> C<White> C<Gray>).
Or, a 0-based index into the C<colorPalette>.

The text inside C<W> sequence will not be wrapped during C<text_wrap> calls.

The methods C<text_out> and C<get_text_width> are affected by C<Prima::Markup>.
C<text_out> will write formatted text to the canvas, and C<get_text_width> will
return the width of the formatted text.  B<NOTE>: These methods do not save state
between calls, so your markup cannot span lines (since each line is drawn or
measured with a separate call).

=cut

sub new
{
	my ($class, %opt) = @_;
	%opt = ( %opt,
		fontmap       => [{}],
		colormap      => [0,0],
	);
	my $self = $class->SUPER::new(%opt);
	$self-> $_( $opt{$_} || [] ) for qw(fontPalette colorPalette);
	$self-> markup( $opt{markup} || '');
	return $self;
}

sub parse_color
{
	my ( $self, $mode, $command, $stacks, $state, $block, $c ) = @_;

	if ( $mode ) {
		if ( $c =~ /^[0-9a-f]{6}$/ ) {
			$c = hex $c;
		} elsif ( $c =~ /^(\D.+)$/ && exists($cl::{$1})) {
			$c = &{$cl::{$1}}();
		} elsif ( $c =~ /^\d+$/) {
			if ( $c >= @{ $self->{colorPalette} } ) {
				warn "Color index outside palette: $c";
				return 1;
			}
			$c += 2;
			$c |= tb::COLOR_INDEX;
		} else {
			warn "Bad color: $c";
			return;
		}
		push @{$stacks->{color}}, $state->{color} = $c;
	} else {
		$state->{color} = pop @{$stacks->{color}};
	}
	push @$block, tb::color($state->{color});

	return 1;
}

sub parse_font_id
{
	my ( $self, $mode, $command, $stacks, $state, $block, $f ) = @_;

	if ( $mode ) {
		if ( $f !~ /^\d+$/) {
			warn "Bad fond id: $f";
			return;
		}
		if ( $f >= @{ $self->{fontPalette} } ) {
			warn "Font index outside palette: $f";
			return;
		}
		push @{$stacks->{fontId}}, $state->{fontId} = $f + 1;
	} else {
		$state->{fontId} = pop @{$stacks->{fontId}};
	}
	push @$block, tb::fontId($state->{fontId});
}

sub parse_font_size
{
	my ( $self, $mode, $command, $stacks, $state, $block, $s ) = @_;

	if ( $mode ) {
		unless ($s =~ /^[+-]?\d+$/) {
			warn "Bad font size: $s";
			return;
		}
		push @{$stacks->{fontSize}}, $state->{fontSize};
		$state->{fontSize} += $s;
		push @$block, tb::fontSize($s);
	} else {
		$state->{fontSize} = pop @{$stacks->{fontSize}};
		push @$block, tb::fontSize($state->{fontSize});
	}
	return 1;
}

sub parse_font_style
{
	my ( $self, $mode, $command, $stacks, $state, $block ) = @_;

	if ( $mode ) {
		my %cmd = (
			I => fs::Italic,
			B => fs::Bold,
			U => fs::Underlined,
		);
		push @{$stacks->{fontStyle}}, $state->{fontStyle};
		$state->{fontStyle} |= $cmd{$command};
		push @$block, tb::fontStyle($state->{fontStyle});
	} else {
		$state->{fontStyle} = pop @{$stacks->{fontStyle}};
		push @$block, tb::fontStyle($state->{fontStyle});
	}
	return 1;
}

sub parse_transpose
{
	my ( $self, $mode, $command, $stacks, $state, $block, $dx, $dy, $subcmd ) = @_;
	my $fl = 0;
	for my $s ( split //, $subcmd // '') {
		if ( $s eq 'm') {
			$fl |= tb::X_DIMENSION_FONT_HEIGHT;
		} elsif ( $s eq 'p') {
			$fl |= tb::X_DIMENSION_POINT;
		} elsif ( $s eq 'x') {
			$fl |= tb::X_EXTEND;
		} else {
			warn "Bad extension flag: $s";
			return;
		}
	}
	push @$block, tb::moveto($dx || 0, $dy || 0, $fl);
}

sub parse_wrap
{
	my ( $self, $mode, $command, $stacks, $state, $block ) = @_;

	if ( $mode ) {
		push @{$stacks->{wrap}}, $state->{wrap};
		$state->{wrap} = tb::WRAP_MODE_OFF;
	} else {
		$state->{wrap} = pop @{$stacks->{wrap}};
	}
	push @$block, tb::wrap($state->{wrap});
	return 1;
}

sub commands
{
	return (
		C => [ 1, 1, \&parse_color ],
		F => [ 1, 1, \&parse_font_id ],
		S => [ 1, 1, \&parse_font_size ],
		I => [ 0, 1, \&parse_font_style ],
		B => [ 0, 1, \&parse_font_style ],
		U => [ 0, 1, \&parse_font_style ],
		M => [ 1, 0, \&parse_transpose ],
		W => [ 0, 1, \&parse_wrap ],
	);
}

sub init_state
{
	return {
		color     => 0 | tb::COLOR_INDEX,
		fontId    => 0,
		fontSize  => 0,
		fontStyle => 0,
		wrap      => tb::WRAP_MODE_ON,
	};
}

sub parse
{
	my ( $self, $text ) = @_;
	my (%stacks, @cmd_stack, @delim_stack );

	my %commands = $self->commands;

	my @tokens = split /([A-Z]<(?:<+\s+)?|\n\r*)/, $text;
	my $block  = tb::block_create();
	my $plaintext = '';

	my $state = $self->init_state;

	while ( @tokens ) {
		my $token = shift @tokens;
		# Look for the beginning of a sequence
		if ( $token =~ /^[\n\r]+$/) {
			push @$block, tb::wrap( tb::WRAP_IMMEDIATE );
		} elsif ( $token =~ /^([A-Z])(<(?:<+\s+)?)$/s ) {
			# Push a new sequence onto the stack of those "in-progress"
			my ($cmd, $ldelim) = ($1, $2);
			$ldelim =~ s/\s+$//, (my $rdelim = $ldelim) =~ tr/</>/;
			push @cmd_stack, '<>'; # temporary noop
			push @delim_stack, $rdelim;

			unless ( exists $commands{$cmd}) {
				warn "Unknown command: $cmd\n";
				next;
			}

			my ( $has_params, $has_text, $callback ) = @{ $commands{$cmd} };
			my @params;
			if ( $has_params ) {
				my $t = shift @tokens;
				unless ( defined $t ) {
					warn "Unexpected end of input\n";
					last;
				}
				my ($ok, $param, $text);
				if ( $has_text ) {
					$ok = $t =~ /^([^|]+)\|(.*)$/s;
					($param, $text) = ($1, $2);
				} else {
					$ok = $t =~ /^([^>]*)>(.*)$/s;
					($param, $text) = ($1, $2);
				}

				if ( !$ok) {
					warn "Expected parameters to $cmd.\n";
					last;
				}
				unshift @tokens, $text;
				@params = split(',', $param);
			}
			next unless $callback->($self, 1, $cmd, \%stacks, $state, $block, @params);
			$cmd_stack[-1] = $cmd;
		} # end of if block for open sequence
		# Look for sequence ending
		else {
			my $dlm;
			# Make sure we match the right kind of closing delimiter
			if ( $dlm = $delim_stack[$#delim_stack] and (
			        ($dlm eq '>' and $token =~ /\A(.*?)(\>)/s) or
				($dlm ne '>' and $token =~ /\A(.*?)(\s{1,}$dlm)/s)
				)
			) {
				my $t = $1;
				push @$block, tb::text( length($plaintext), length($t) );
				$plaintext .= $t;

				my $rest = substr($token, length($1) + length($2));
				length($rest) and unshift @tokens, $rest;

				my $cmd = pop(@cmd_stack) // '';
				next unless exists $commands{$cmd};

				my ( $has_params, $has_text, $callback ) = @{ $commands{$cmd} };
				$callback->($self, 0, $cmd, \%stacks, $state, $block) if $has_text;
			} # end of if block for close sequence
			else { # if we get here, we're non-escaped text
				push @$block, tb::text( length($plaintext), length($token) );
				$plaintext .= $token;
			}
		} # end of else block after if block for open sequence
	} # end of while loop
	
	push @$block, tb::wrap(tb::WRAP_MODE_ON) if $state->{wrap} == tb::WRAP_MODE_OFF;

	return $plaintext, $block;
}

sub markup
{
	return $_[0]->{markup} unless $#_;

	my ( $self, $markup ) = @_;
	my ( $text, $block ) = $self-> parse( $markup );

	if ( Prima::Bidi::is_bidi($text) ) {
		$self-> {nonbidiblock} = $block;
		$block = tb::bidi_visualize( $block, text => $text );
	} else {
		$self-> {nonbidiblock} = undef;
	}

	$self-> {markup} = $markup;
	$self-> text( $text );
	$self-> {block} = $block;
}	

sub acquire
{
	my ($self, $canvas, %opt) = @_;
	my $font;
	if ( $opt{font} || $opt{dimensions} ) {
		$font = $canvas->get_font;
		$self->{fontmap}->[0]  = $font;
		$self->{block}->[tb::BLK_FONT_ID]    = 0;
		$self->{block}->[tb::BLK_FONT_SIZE]  = $self->{baseFontSize}  = $font->{size};
		$self->{block}->[tb::BLK_FONT_STYLE] = $self->{baseFontStyle} = $font->{style};
		$self->{direction} = $font->{direction}; 
	}
	if ( $opt{colors}) {
		$self->{block}->[tb::BLK_COLOR]      = $self->{colormap}->[0] = $canvas->color;
		$self->{block}->[tb::BLK_BACKCOLOR]  = $self->{colormap}->[1] = tb::BACKCOLOR_DEFAULT;
	}
	if ( $opt{dimensions} ) {
		my $signature = join('.', @{$font}{qw(name size height width style encoding direction)});
		if ( $signature ne $self->{fontSignature} ) {
			$self->{fontSignature} = $signature;
			$self->calculate_dimensions($canvas);
		}
	}
}

sub fontPalette
{
	return $_[0]->{fontPalette} unless $#_;
	my ( $self, $fp ) = @_;
	$self->{fontPalette} = $fp;
	splice( @{$self->{fontmap}}, 1 );
	push @{ $self->{fontmap}}, @$fp;
}

sub colorPalette
{
	return $_[0]->{colorPalette} unless $#_;
	my ( $self, $cp ) = @_;
	$self->{colorPalette} = $cp;
	splice( @{$self->{colormap}}, 1 );
	push @{ $self->{colormap}}, @$cp;
}

sub text_wrap
{
	my ( $self, $canvas, $width, $opt, $indent) = @_;

	local $self->{block} = $self->{nonbidiblock} if $self->{nonbidiblock};

	my @ret = @{ $self-> SUPER::text_wrap( $canvas, $width, $opt, $indent ) };

	my ( @blocks, @other);
	for my $block ( @ret ) {
		if ( ref($block) eq 'Prima::Drawable::TextBlock') {
			$block = bless $block, __PACKAGE__;
			$block->{$_}     = [@{$self->{$_}}] for qw(fontmap colormap fontPalette colorPalette);
			$block->{$_}     = $self->{$_} for qw(restoreCanvas);
			push @blocks, $block;
		} else {
			push @other, $block;
		}
	}
	return @other unless @blocks;

	# Feed less text to bidi_visualize because it can take things into account
	# that were wrapped away for a sub-block. F.ex:
	# wrap("bidi . nonbidi") results in ("bidi n", "onbidi"). While the 1st
	# scalar is okay when bidified, noone knows whether the 2nd will look
	# the same when in the big string context and when torn away after the wrap.
	if ( Prima::Bidi::is_bidi($self->{text})) {
		my $text_offset = $blocks[-1]->{block}->[tb::BLK_TEXT_OFFSET];
		my $last_offset = 0;
		tb::walk( $blocks[-1]->{block}, text => sub {
			my ( $ofs, $len ) = @_;
			$last_offset = $ofs + $len;
		});
		my @text_offsets = (
			( map { $_->{block}->[ tb::BLK_TEXT_OFFSET ] } @blocks ),
			$text_offset + $last_offset);

		my $t = $self->{text};
		for ( my $j = 0; $j < @blocks; $j++) {
			my $substr  = substr( $t, $text_offsets[$j], $text_offsets[$j+1] - $text_offsets[$j]);
			next unless Prima::Bidi::is_bidi($substr);
			$blocks[$j]->{block} = tb::bidi_visualize(
				$blocks[$j]->{block}, text => $substr,
			);
		}
	}

	# initials will be overwritten by acquire(), force copy them
	for my $block ( @blocks ) {
		my $b = $block->{block};
		splice( @$b, tb::BLK_START, 0,
			tb::color( $$b[tb::BLK_COLOR]),
			tb::fontId( $$b[tb::BLK_FONT_ID]),
			tb::fontSize( $$b[tb::BLK_FONT_SIZE] - $self->{baseFontSize}),
			tb::fontStyle( $$b[tb::BLK_FONT_STYLE])
		);
	}

	return [ @blocks, @other ];
}

=head1 PROPERTIES

The following properties are used:

=over

=item colorPalette([@colorPalette])

Gets or sets the color palette to be used for C<C> sequences within this widget.
Each element of the array should be a C<cl::> constant.

=item fontPalette([@fontPalette])

Gets or sets the font palette to be used for C<F> sequences within this widget.
Each element of the array should be a hashref suitable for setting a font.

=back

=head1 BUGS

Text wrapping with C<text_wrap> is barely supported. For serious text wrapping,
look at L<Prima::TextView> and L<Prima::PodView>.

=head1 COPYRIGHT

Copyright 2003 Teo Sankaro

Portions of this code were copied or adapted from L<Pod::Text> by Russ Allbery
and L<Pod::Parser> by Brad Appleton, both of which derive from the original
C<Pod::Text> by Tom Christianson.

You may redistribute and/or modify this module under the same terms as Perl itself.
(Although a credit would be nice.)

=head1 AUTHOR

Teo Sankaro, E<lt>teo_sankaro@hotmail.comE<gt>.

=cut

1;
