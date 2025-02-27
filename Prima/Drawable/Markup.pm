package Prima::Drawable::Markup;

use strict;
use warnings;
use Prima qw(Drawable::TextBlock);
use base qw(Prima::Drawable::TextBlock Exporter);
our @EXPORT_OK = 'M';

our (@FONTS, @COLORS, @IMAGES);
our $LINK_COLOR = cl::Green;

our $OP_LINK = tb::opcode(1, 'link');
sub op_link_enter { $OP_LINK, 1 }
sub op_link_leave { $OP_LINK, 0 }

sub M($) {
	return Prima::Drawable::Markup->new(
		markup  => $_[0],
		defaults(),
	)
}

sub defaults
{
	fontPalette    => \@FONTS,
	picturePalette => \@IMAGES,
	colorPalette   => \@COLORS,
	linkColor      => $LINK_COLOR,
}

sub new
{
	my ($class, %opt) = @_;
	%opt = ( %opt,
		fontmap       => [{}],
		colormap      => [0,0],
	);
	my $self = $class->SUPER::new(%opt);
	$self-> $_( $opt{$_} || [] ) for qw(fontPalette colorPalette picturePalette linkColor);
	$self-> markup( $opt{markup} || '');
	return $self;
}

sub parse_color
{
	my ( $self, $mode, $command, $stacks, $state, $block, $c ) = @_;

	my $key = ($command eq 'C') ? 'color' : 'backColor';

	if ( $mode ) {
		if ( $c =~ /^[0-9a-f]{6}$/ ) {
			$c = hex $c;
		} elsif ( $c =~ /^(\D.+)$/ && exists($cl::{$1})) {
			$c = &{$cl::{$1}}();
		} elsif ( $c =~ /^\d+$/) {
			if ( $c >= @{ $self->{colorPalette} } ) {
				warn "Color index outside palette: $c";
				return;
			}
			$c += 2;
			$c |= tb::COLOR_INDEX;
		} elsif ( lc($c) eq 'default' ) {
			$c = $block->[($command eq 'G') ? tb::BLK_BACKCOLOR : tb::BLK_COLOR];
		} elsif ( $command eq 'G' && lc($c) eq 'off' ) {
			$c = tb::BACKCOLOR_OFF;
		} else {
			warn "Bad color: $c";
			return;
		}
		push @{$stacks->{$key}}, $state->{$key};
		$state->{$key} = $c | (( $command eq 'G') ? tb::BACKCOLOR_FLAG : 0);
	} else {
		$state->{$key} = pop @{$stacks->{$key}};
	}
	push @$block, tb::color($state->{$key});

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
		push @{$stacks->{fontId}}, $state->{fontId};
		$state->{fontId} = $f + 1;
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

sub paint_picture
{
	my ( $self, $canvas, $block, $state, $x, $y, $r) = @_;
	my ( $img, $zoom ) = @$r;
	$y += ($block->[tb::BLK_HEIGHT] - $img->height * $zoom ) / 2 - $block->[tb::BLK_APERTURE_Y];
	$canvas-> stretch_image( $x, $y, $img-> width * $zoom, $img-> height * $zoom, $img);
}

sub parse_picture
{
	my ( $self, $mode, $command, $stacks, $state, $block, $pic, $zoom ) = @_;
	unless ($pic =~ /^\d+$/ && $pic < @{ $self->{picturePalette} } ) {
		warn "Bad picture id: $pic";
		return;
	}
	if ( defined $zoom && $zoom !~ /^\d+(\.\d+)?$/) {
		warn "Bad picture zoom: $zoom";
		return;
	}
	$pic = $self->{picturePalette}->[$pic];
	$zoom //= 1;

	push @$block,
		tb::wrap(tb::WRAP_MODE_OFF),
		tb::extend( $pic->width * $zoom, $pic->height * $zoom, 0),
		tb::code( \&paint_picture, [$pic, $zoom]),
		tb::moveto( $pic->width * $zoom, 0, 0),
		tb::wrap(tb::WRAP_MODE_ON)
		;
}

sub parse_quote
{
	my ( $self, $mode, $command, $stacks, $state, $block ) = @_;
	$state->{quote} = $mode;
	return 1;
}

sub parse_link_syntax
{
	my ( $self, $mode, $command, $stacks, $state, $block, $url ) = @_;

	if ( $mode ) {
		push @{$stacks->{color}}, $state->{color};

		$state->{color} = $self->linkColor;
		push @$block,
			tb::color($state->{color}),
			op_link_enter,
			;
		push @{ $self->{link_urls} }, $url;
	} else {
		$state->{color} = pop @{$stacks->{color}};

		push @$block,
			op_link_leave,
			tb::color($state->{color}),
			;
	}
}

sub commands
{
	return (
		# has params, has text, callback
		C => [ 1, 1, \&parse_color ],
		G => [ 1, 1, \&parse_color ],
		F => [ 1, 1, \&parse_font_id ],
		S => [ 1, 1, \&parse_font_size ],
		I => [ 0, 1, \&parse_font_style ],
		B => [ 0, 1, \&parse_font_style ],
		U => [ 0, 1, \&parse_font_style ],
		M => [ 1, 0, \&parse_transpose ],
		W => [ 0, 1, \&parse_wrap ],
		P => [ 1, 0, \&parse_picture ],
		Q => [ 0, 1, \&parse_quote ],
		L => [ 1, 1, \&parse_link_syntax ],
	);
}

sub init_state
{
	return {
		color     => 0 | tb::COLOR_INDEX,
		backColor => tb::BACKCOLOR_DEFAULT,
		fontId    => 0,
		fontSize  => 0,
		fontStyle => 0,
		wrap      => tb::WRAP_MODE_ON,
		quote     => 0,
	};
}

sub parse
{
	my ( $self, $text ) = @_;
	my (%stacks, @cmd_stack, @delim_stack );

	@{$self->{link_urls}} = ();

	my %commands = $self->commands;

	my @tokens = split /([A-Z]<(?:<+\s+)?|\n\r*)/, $text;
	my $block  = tb::block_create();

	my $plaintext = '';

	my $state = $self->init_state;
	$$block[ tb::BLK_COLOR      ] = $state->{color};
	$$block[ tb::BLK_BACKCOLOR  ] = $state->{backColor};
	$$block[ tb::BLK_FONT_ID    ] = $state->{fontId};
	$$block[ tb::BLK_FONT_SIZE  ] = $state->{fontSize};
	$$block[ tb::BLK_FONT_STYLE ] = $state->{fontStyle};

	while ( @tokens ) {
		my $token = shift @tokens;
		# Look for the beginning of a sequence
		if ( $token =~ /^[\n\r]+$/) {
			push @$block, tb::extend( 1, 1 ) if
				@$block > tb::BLK_START && $block->[-2] == tb::OP_WRAP && $block->[-1] == tb::WRAP_IMMEDIATE;
			push @$block, tb::wrap( tb::WRAP_IMMEDIATE );
		} elsif ( $token =~ /^([A-Z])(<(?:<+\s+)?)$/s && ! $state->{quote} ) {
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
				pop @delim_stack;
				push @$block, tb::text( length($plaintext), length($t) )
					if length($t);
				$plaintext .= $t;

				my $rest = substr($token, length($1) + length($2));
				length($rest) and unshift @tokens, $rest;

				my $cmd = pop(@cmd_stack) // '';
				if( $self->{quote} && $cmd ne 'Q') {
					push @cmd_stack, $cmd;
					next;
				} else {
					next unless exists $commands{$cmd};
				}

				my ( $has_params, $has_text, $callback ) = @{ $commands{$cmd} };
				$callback->($self, 0, $cmd, \%stacks, $state, $block) if $has_text;
			} # end of if block for close sequence
			else { # if we get here, we're non-escaped text
				push @$block, tb::text( length($plaintext), length($token) )
					if length $token;
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

	$self-> {markup} = $markup;
	$self-> text( $text );
	$self-> {block} = $block;
}

sub reparse    { $_[0]->markup( $_[0]-> markup ) }
sub text_block { $_[0]->{block} }
sub link_urls  { $_[0]->{link_urls} }
sub has_links  { scalar( @{ $_[0]->{link_urls} } ) > 0 }

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
		$self->{block}->[tb::BLK_COLOR]     = $self->{colormap}->[0] = $canvas->color;
		$self->{colormap}->[1] = $canvas-> backColor;
		$self->{block}->[tb::BLK_BACKCOLOR] =
			($canvas-> textOpaque ? $canvas-> backColor : tb::BACKCOLOR_DEFAULT);
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
	splice( @{$self->{colormap}}, 2 );
	push @{ $self->{colormap}}, @$cp;
}

sub picturePalette
{
	return $_[0]->{picturePalette} unless $#_;
	my ( $self, $pp ) = @_;
	$self->{picturePalette} = $pp;
}

sub linkColor
{
	return $_[0]->{linkColor} unless $#_;
	$_[0]->{linkColor} = $_[1];
}

sub text_wrap
{
	my ( $self, $canvas, $width, $opt, $indent) = @_;

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

	# initials will be overwritten by acquire(), force copy them
	for my $block ( @blocks ) {
		my $b = $block->{block};
		splice( @$b, tb::BLK_START, 0,
			tb::color( $$b[tb::BLK_COLOR]),
			tb::backColor( $$b[tb::BLK_BACKCOLOR]),
			tb::fontId( $$b[tb::BLK_FONT_ID]),
			tb::fontSize( $$b[tb::BLK_FONT_SIZE] - $self->{baseFontSize}),
			tb::fontStyle( $$b[tb::BLK_FONT_STYLE])
		);
	}

	return [ @blocks, @other ];
}

=head1 NAME

Prima::Drawable::Markup - allow markup in widgets

=head1 SYNOPSIS

    use Prima qw(Application Buttons);
    use Prima::Drawable::Markup q(M);
    my $m = Prima::MainWindow->new;
    $m-> insert( Button =>
	text   => Prima::Drawable::Markup->new(markup => "B<Bold> bU<u>tton"),
	hotKey => 'u',
	pack   => {},
    );
    $m->insert( Button => pack => {}, text => M "I<Italic> button" );
    $m->insert( Button => pack => {}, text => \ "Not an Q<I<italic>> button" );

    Prima->run;

=for podview <img src="Prima/markup.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/markup.gif">

=head1 DESCRIPTION

C<Prima::Drawable::Markup> adds the ability to recognize POD-like markup to Prima
widgets. Supported markup sequences are C<B> (bold text), C<I> (italic text),
C<U> (underlined text), C<F> (change font), C<S> (change font size), C<C>
(change foreground color), C<G> (change background color), C<M> (move pointer),
C<W> (disable wrapping), and C<P> (picture).

The C<F> sequence is used as follows: C<< F<n|text> >>, where C<n> is a 0-based
index into the C<fontPalette>.

The C<S> sequence is used as follows: C<< S<n|text> >>, where C<n> is the
number of points relative to the current font size. The font size may
optionally be preceded by C<+> or C<->.

The C<C> and C<G> sequences are used as follows: C<< C<c|text> >>, where C<c>
is either: a color in any form accepted by Prima, including the C<cl> constants
(C<Black> C<Blue> C<Green> C<Cyan> C<Red> C<Magenta> C<Brown> C<LightGray>
C<DarkGray> C<LightBlue> C<LightGreen> C<LightCyan> C<LightRed> C<LightMagenta>
C<Yellow> C<White> C<Gray>).  Or, a 0-based index into the C<colorPalette>.
Also, C<default> can be used to set the color that the canvas originally had.
For C<G> a special value C<off> can be used to turn off the background color
and set it as transparent.

The C<M> command has three parameters, comma-separated: X, Y, and flags.  X and
Y are coordinates of how much to move the current pointer. By default X and are in
pixels, and do not extend block width. C<flags> is a set of characters, where
each is:

    m - set units to font height
    p - set units to points
    x - also extend the block width

The text inside the C<W> sequence will not be wrapped during C<text_wrap> calls.

The text inside the C<Q> sequence will not be treated as markup.

The C<P> sequence is used as follows:C<< PE<lt>nE<gt> >>, where C<n> is a
0-based index into the C<picturePalette>.

The L<URL|text> sequence parsing results in making 1) C<text> of color
C<linkColor>, and 2) wrapping the text with C<OP_LINK> commands in the block,
that do nothing by default but could be used by whoever uses the block. See
L<Prima::Widget::Link> for more and L<Prima::Label> as an example.

The methods C<text_out> and C<get_text_width> are affected by
C<Prima::Drawable::Markup>.  C<text_out> will write formatted text to the
canvas, and C<get_text_width> will return the width of the formatted text.
B<NOTE>: These methods do not save the state between calls, so your markup cannot
span lines (since each line is drawn or measured with a separate call).

The module can export a single method C<M> which is a shortcut over the creation of
a new markup object with default color, font, and image palettes. These can be
accessed directly as C<@COLORS, @FONTS, @IMAGES> correspondingly.

=head1 API

The following properties are used:

=over

=item colorPalette([@colorPalette])

Gets or sets the color palette to be used for C<C> sequences within this
widget.  Each element of the array should be a C<cl::> constant.

=item fontPalette([@fontPalette])

Gets or sets the font palette to be used for C<F> sequences within this widget.
Each element of the array should be a hashref suitable for setting a font.

=item picturePalette([@picturePalette])

Gets or sets the picture palette to be used for C<P> sequences within this
widget.  Each element of the array should be a C<Prima::Image> descendant.

=back

=head1 SEE ALSO

L<Prima::Drawable::TextBlock>, F<examples/markup.pl>

=head1 COPYRIGHT

Copyright 2003 Teo Sankaro

You may redistribute and/or modify this module under the same terms as Perl itself.
(Although a credit would be nice.)

=head1 AUTHOR

This module is based on work by Teo Sankaro, E<lt>teo_sankaro@hotmail.comE<gt>.

=cut

1;
