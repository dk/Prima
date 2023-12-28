package Prima::Drawable::Pod;
use strict;
use warnings;

package
	pod;

use constant DEF_INDENT        => 4;
use constant DEF_FIRST_INDENT  => 1;

use constant STYLE_CODE        => 0;
use constant STYLE_TEXT        => 1;
use constant STYLE_HEAD_1      => 2;
use constant STYLE_HEAD_2      => 3;
use constant STYLE_HEAD_3      => 4;
use constant STYLE_HEAD_4      => 5;
use constant STYLE_ITEM        => 6;
use constant STYLE_LINK        => 7;
use constant STYLE_VERBATIM    => 8;
use constant STYLE_MAX_ID      => 8;

# model layout indices
use constant M_TYPE            => 0; # T_XXXX
                                     # T_NORMAL
use constant M_TEXT_OFFSET     => 1; # contains same info as BLK_TEXT_OFFSET
use constant M_INDENT          => 2; # pod-content driven indent
use constant M_FONT_ID         => 3; # 0 or 1 ( i.e., variable or fixed )
use constant M_START           => 4; # start of data, same purpose as BLK_START
                                     # T_DIV
use constant MDIV_TAG          => 2;
use constant MDIV_STYLE        => 3;

# model entries
use constant T_NORMAL          => 0x010;
use constant T_DIV             => 0x020;
use constant T_TYPE_MASK       => 0x030;
use constant T_LOOKAHEAD       => 0x007;
use constant TDIVTAG_OPEN      => 0;
use constant TDIVTAG_CLOSE     => 1;
use constant TDIVSTYLE_SOLID   => 0;
use constant TDIVSTYLE_OUTLINE => 1;

# topic layout indices
use constant T_MODEL_START     => 0; # beginning of topic
use constant T_MODEL_END       => 1; # end of a topic
use constant T_DESCRIPTION     => 2; # topic name
use constant T_STYLE           => 3; # style of STYLE_XXX
use constant T_ITEM_DEPTH      => 4; # depth of =item recursion
use constant T_LINK_OFFSET     => 5; #

package
	Prima::Drawable::Pod;
use vars qw(%HTML_Escapes);
use Encode;
use Prima::Drawable::TextBlock;
use Prima::Drawable::Markup;

sub default_colors
{(
	0x337ab7, # link foreground
	0x0000ff, # code foreground
	0xf5f5f5, # code background
)}

sub default_styles
{
	my @colors = $_[0]->default_colors;
	return (
		{ fontId    => 1,                         # STYLE_CODE
		color     => $colors[1],
		backColor => $colors[2]
		},
		{ },                                      # STYLE_TEXT
		{ fontSize => 4, fontStyle => fs::Bold }, # STYLE_HEAD_1
		{ fontSize => 2, fontStyle => fs::Bold }, # STYLE_HEAD_2
		{ fontSize => 1, fontStyle => fs::Bold }, # STYLE_HEAD_3
		{ fontSize => 1, fontStyle => fs::Bold }, # STYLE_HEAD_4
		{ fontStyle => fs::Bold },                # STYLE_ITEM
		{ color     => $colors[0]},               # STYLE_LINK
		{ fontId    => 1,                         # STYLE_VERBATIM
		color     => $colors[1],
		},
	);
}

sub new
{
	my ( $class, %opt ) = @_;
	my $self = bless {
		manpath => '',
		styles  => [$class->default_styles],
		op_link => [],
		%opt,
	}, $class;
	$self->update_styles;
	$self->reset;
	return $self;
}

sub reset
{
	my $self = $_[0];
	$self-> {index_ends_at} = undef;
	$self-> {model}          = [];
	$self-> {topics}         = [];
	$self-> {images}         = [];
	$self-> {links}          = [];
	$self-> {link_map}       = {};
	my $text = '';
	$self-> {text}           = \$text;
	$self-> {read_state}     = undef;
	$self-> {has_index}      = 0;
}

sub is_reading    { defined $_[0]->{read_state}                       }
sub model         { $_[0]->{model}                                    }
sub links         { $_[0]->{links}                                    }
sub link_map      { $_[0]->{link_map}                                 }
sub topics        { $_[0]->{topics}                                   }
sub has_index     { $_[0]->{has_index}                                }
sub text_length   { length ${ $_[0]->{text} }                         }
sub model_length  { scalar @{ $_[0]->{model} }                        }
sub manpath       { $#_ ? $_[0]->{manpath} = $_[1] : $_[0]->{manpath} }
sub text_ref      { $#_ ? $_[0]->{text}    = $_[1] : $_[0]->{text}    }
sub images        { $#_ ? $_[0]->{images}  = $_[1] : $_[0]->{images}  }
sub index_ends_at { $_[0]->{index_ends_at}                            }

*op_link_enter = \&Prima::Drawable::Markup::op_link_enter;
*op_link_leave = \&Prima::Drawable::Markup::op_link_leave;

sub open_read
{
	my ($self, @opt) = @_;
	return if $self-> {read_state};
	$self-> reset;
	$self-> {read_state} = {
		cutting       => 1,
		pod_cutting   => 1,
		begun         => '',
		bulletMode    => 0,

		indent        => pod::DEF_INDENT,
		indentStack   => [],

		bigofs        => 0,
		wrapstate     => '',
		wrapindent    => 0,

		topicStack    => [[-1]],

		createIndex   => 1,
		encoding      => undef,
		bom           => undef,
		utf8          => undef,
		verbatim      => undef,

		@opt,
	};
}

sub read_paragraph
{
	my ( $self, $line ) = @_;
	my $r = $self-> {read_state};

	for ( $line ) {
		if ($r-> {cutting}) {
			next unless /^=/;
			$r-> {cutting} = 0;
		}

		unless ($r-> {pod_cutting}) {
			next unless /^=/;
		}

		if ($r-> {begun}) {
			my $begun = $r-> {begun};
			if (/^=end\s+$begun/ || /^=cut/) {
				$r-> {begun} = '';
				$self-> add_new_line; # end paragraph
				$r-> {cutting} = 1 if /^=cut/;
			} else {
				$self-> add_formatted( $r-> {begun}, $_);
			}
			next;
		}

		1 while s{^(.*?)(\t+)(.*)$}{
			$1
			. (' ' x (length($2) * 8 - length($1) % 8))
			. $3
		}me;

		# Translate verbatim paragraph
		if (/^\s/) {
			$self-> add_verbatim_mark(1) unless defined $r->{verbatim};
			$self-> add($_,pod::STYLE_VERBATIM,$r-> {indent}) for split "[\x0a\x0d]+", $_;
			$self-> add_new_line;
			next;
		}
		$self-> add_verbatim_mark(0);

		if (/^=for\s+(\S+)\s*(.*)/s) {
			$self-> add_formatted( $1, $2) if defined $2;
			next;
		} elsif (/^=begin\s+(\S+)\s*(.*)/s) {
			$r-> {begun} = $1;
			$self-> add_formatted( $1, $2) if defined $2;
			next;
		}

		if (s/^=//) {
			my ($Cmd, $args) = split(' ', $_, 2);
			$args = '' unless defined $args;
			if ($Cmd eq 'cut') {
				$r-> {cutting} = 1;
			}
			elsif ($Cmd eq 'pod') {
				$r-> {cutting} = 0;
			}
			elsif ($Cmd eq 'head1') {
				$self-> add( $args, pod::STYLE_HEAD_1, pod::DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'head2') {
				$self-> add( $args, pod::STYLE_HEAD_2, pod::DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'head3') {
				$self-> add( $args, pod::STYLE_HEAD_3, pod::DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'head4') {
				$self-> add( $args, pod::STYLE_HEAD_4, pod::DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'over') {
				push(@{$r-> {indentStack}}, $r-> {indent});
				$r-> {indent} += ( $args =~ m/^(\d+)$/ ) ? $1 : pod::DEF_INDENT;
			}
			elsif ($Cmd eq 'back') {
				$self-> _close_topic( pod::STYLE_ITEM);
				$r-> {indent} = pop(@{$r-> {indentStack}}) || 0;
			}
			elsif ($Cmd eq 'item') {
				$self-> add( $args, pod::STYLE_ITEM, $r-> {indentStack}-> [-1] || pod::DEF_INDENT);
			}
			elsif ($Cmd eq 'encoding') {
				$r->{encoding} = Encode::find_encoding($args); # or undef
			}
		}
		else {
			s/\n/ /g;
			$self-> add($_, pod::STYLE_TEXT, $r-> {indent});
		}

		$self-> add_new_line unless $r->{bulletMode};
	}
}

sub read
{
	my ( $self, $pod) = @_;
	my $r = $self-> {read_state};
	return unless $r;

	unless ( defined $r->{bom} ) {
		if ( $pod =~ s/^(\x{ef}\x{bb}\x{bf})// ) { # don't care about other BOMs so far
			$r-> {bom} = $1;
			$r-> {encoding} = Encode::find_encoding('utf-8');
		}
	}

	my $odd = 0;
	for ( split ( "(\n)", $pod)) {
		next unless $odd = !$odd;
		$_ = $r->{encoding}->decode($_, Encode::FB_HTMLCREF) if $r->{encoding};

		if (defined $r-> {paragraph_buffer}) {
			if ( /^\s*$/) {
				my $pb = $r-> {paragraph_buffer};
				undef $r-> {paragraph_buffer};
				$self-> read_paragraph($pb);
			} else {
				$r-> {paragraph_buffer} .= "\n$_";
				next;
			}
		} elsif ( !/^$/) {
		    $r->{paragraph_buffer} = $_;
		    next;
		}
	}
}

# internal sub, called when a new topic is emerged.
# responsible to what topics can include others ( =headX to =item)
sub _close_topic
{
	my ( $self, $style, $topicToPush) = @_;

	my $r = $self-> {read_state};
	my $t = $r-> {topicStack};
	my $state = ( $style >= pod::STYLE_HEAD_1 && $style <= pod::STYLE_HEAD_4) ?
		0 : scalar @{$r-> {indentStack}};

	if ( $state <= $$t[-1]-> [0]) {
		while ( scalar @$t && $state <= $$t[-1]-> [0]) {
			my $nt = pop @$t;
			$nt = $$nt[1];
			$$nt[ pod::T_MODEL_END] = scalar @{$self-> {model}} - 1;
		}
		push @$t, [ $state, $topicToPush ] if $topicToPush;
	} else {
		# assert defined $topicToPush
		push @$t, [ $state, $topicToPush ];
	}
}

sub close_read
{
	my ( $self) = @_;
	return unless $self-> {read_state};

	my $r = $self-> {read_state};
	if ( defined $r->{paragraph_buffer}) {
		my $pb = $r-> {paragraph_buffer};
		undef $r-> {paragraph_buffer};
		$self-> read_paragraph("$pb\n");
	}

	$self-> add_new_line; # end
	$self-> add_verbatim_mark(0);

	unless ($self-> {read_state}-> {createIndex}) {
		$self-> _close_topic( pod::STYLE_HEAD_1);
		goto NO_INDEX;
	}

	my $secid = 0;
	my $msecid = scalar(@{$self-> {topics}});
	delete $self->{index_ends_at};

	unless ( $msecid) {
		push @{$self-> {topics}}, [
			0, scalar @{$self-> {model}} - 1,
			"Document", pod::STYLE_HEAD_1, 0, 0
		] if scalar @{$self-> {model}} > 2; # no =head's, but some info
		$self-> _close_topic( pod::STYLE_HEAD_1);
		goto NO_INDEX;
	}

	## this code creates the Index section, adds it to the end of text,
	## and then uses black magic to put it in the front.

	# remember the current end state
	$self-> _close_topic( pod::STYLE_HEAD_1);
	my @text_ends_at = (
		$r-> {bigofs},
		scalar @{$self->{model}},
		scalar @{$self->{topics}},
		scalar @{$self->{links}},
	);

	# generate index list
	my $ofs = $self-> {model}-> [$self-> {topics}-> [0]-> [pod::T_MODEL_START]]-> [pod::M_TEXT_OFFSET];
	my $firstText = substr( ${$self-> {text}}, 0, ( $ofs > 0) ? $ofs : 0);
	if ( $firstText =~ /[^\n\s\t]/) { # the 1st lines of text are not =head
		unshift @{$self-> {topics}}, [
			0, $self-> {topics}-> [0]-> [pod::T_MODEL_START] - 1,
			"Preface", pod::STYLE_HEAD_1, 0, 0
		];
		$text_ends_at[2]++;
		$msecid++;
	}
	my $start = scalar @{ $self->{model} };
	$self-> add_new_line;
	$self-> add_verbatim_mark(1);
	$self-> add( " Contents",  pod::STYLE_HEAD_1, pod::DEF_FIRST_INDENT);
	$self-> {has_index} = 1;
	$self-> {topics}->[-1]->[pod::T_MODEL_START] = $start;
	my $last_style = pod::STYLE_HEAD_1;
	for my $k ( @{$self-> {topics}}) {
		last if $secid == $msecid; # do not add 'Index' entry
		my ( $ofs, $end, $text, $style, $depth, $linkStart) = @$k;
		if ( $style == pod::STYLE_ITEM ) {
			$style = $last_style;
		} else {
			$last_style = $style;
		}
		my $indent = pod::DEF_INDENT + ( $style - pod::STYLE_HEAD_1 + $depth ) * 2;
		$self-> add("L<$text|topic://$secid>", pod::STYLE_TEXT, $indent);
		$secid++;
	}
	$self-> add_new_line;
	$self-> add_verbatim_mark(0);

	$self-> _close_topic( pod::STYLE_HEAD_1);

	# remember the state after index is added
	my @index_ends_at = (
		$r-> {bigofs},
		scalar @{$self->{model}},
		scalar @{$self->{topics}},
		scalar @{$self->{links}},
	);

	# exchange places for index and body
	my @offsets = map { $index_ends_at[$_] - $text_ends_at[$_] } 0 .. 3;
	my $m = $self-> {model};
	# first shift the offsets
	$$_[pod::M_TEXT_OFFSET] += $offsets[0]      for @$m[0..$text_ends_at[1]-1];
	$$_[pod::M_TEXT_OFFSET] -= $text_ends_at[0] for @$m[$text_ends_at[1]..$index_ends_at[1]-1];
	# next reshuffle the model
	my @index_section = splice( @$m, $text_ends_at[1]);
	unshift @$m, @index_section;
	$self->{index_ends_at} = @index_section;
	undef @index_section;
	# text
	my $t = $self-> {text};
	my $ts = substr( $$t, $text_ends_at[0]);
	substr( $$t, $text_ends_at[0]) = '';
	substr( $$t, 0, 0) = $ts;
	# topics
	$t = $self-> {topics};
	for ( @$t[0..$text_ends_at[2]-1]) {
		$$_[pod::T_MODEL_START] += $offsets[1];
		$$_[pod::T_MODEL_END]   += $offsets[1];
		$$_[pod::T_LINK_OFFSET] += $offsets[3];
	}
	for ( @$t[$text_ends_at[2]..$index_ends_at[2]-1]) {
		$$_[pod::T_MODEL_START] -= $text_ends_at[1];
		$$_[pod::T_MODEL_END]   -= $text_ends_at[1];
		$$_[pod::T_LINK_OFFSET] -= $text_ends_at[3];
	}
	unshift @$t, splice( @$t, $text_ends_at[2]);
	# update the map of blocks that contain OP_LINKs
	$self-> {link_map} = {
		map {
			( $_ >= $text_ends_at[1]) ?
			( $_ - $text_ends_at[1] ) :
			( $_ + $offsets[1] ),
			1
		} keys %{$self-> {link_map}}
	};
	# links
	my $l = $self-> {links};
	s/^(topic:\/\/)(\d+)$/$1 . ( $2 + $offsets[2])/e for @$l;
	unshift @{$self->{links}}, splice( @{$self->{links}}, $text_ends_at[3]);

NO_INDEX:
	# finalize
	undef $self-> {read_state};

	return {
		topic_id => $msecid,
		success  => scalar(@{$self-> {model}}) > 1,
	};
}

sub model_create
{
	my %opt = @_;
	return (
		pod::T_NORMAL | (($opt{lookahead} // 0) & pod::T_LOOKAHEAD),
		$opt{offset} // 0,
		$opt{indent} // 0,
		$opt{font}   // 0
	);
}

sub div_create
{
	my %opt = @_;
	return (
		pod::T_DIV,
		$opt{offset} // 0,
		$opt{open}  ? pod::TDIVTAG_OPEN : pod::TDIVTAG_CLOSE,
		$opt{style} // pod::TDIVSTYLE_SOLID,
	);
}


sub load_image
{
	my ( $self, $src, $frame, $rest ) = @_;
	return Prima::Image::base64->load_icon($rest, index => $frame, iconUnmask => 1)
		if $src eq 'data:base64';

	return Prima::Icon-> load( $src, index => $frame, iconUnmask => 1)
		if -f $src;

	$src =~ s!::!/!g;
	for my $path (
		map {( "$_", "$_/pod")}
		grep { defined && length && -d }
		( length($self-> {manpath}) ? $self-> {manpath} : (), @INC)
	) {
		return Prima::Icon-> load( "$path/$src", index => $frame, iconUnmask => 1)
			if -f "$path/$src" && -r _;
	}
	return;
}

sub img_paint
{
	my ( $self, $canvas, $block, $state, $x, $y, $img) = @_;
	my ( $dx, $dy) = @{$img->{stretch}};
	my @r = $canvas->resolution;
	$dx *= $r[0] / 72;
	$dy *= $r[1] / 72;
	$canvas-> stretch_image( $x, $y, $dx, $dy, $img);
}

sub add_image
{
	my ( $self, $src, %opt ) = @_;

	my $w = $opt{width} // $src-> width;
	my $h = $opt{height} // $src-> height;
	# images are so far assumed to be designed for 96 dpi, but their dimensions are specified in 72-dpi points
	my $W = $w * 72 / 96;
	my $H = $h * 72 / 96;
	$src-> {stretch} = [$W, $H];

	my $r = $self-> {read_state};
	$r-> {pod_cutting} = $opt{cut} ? 0 : 1
		if defined $opt{cut};

	my @imgop = (
		tb::moveto( 2, 0, tb::X_DIMENSION_FONT_HEIGHT),
		tb::wrap(tb::WRAP_MODE_OFF),
		tb::extend( $W, $H, tb::X_DIMENSION_POINT),
		tb::code( $self->can('img_paint'), $src),
		tb::moveto( $W, 0, tb::X_DIMENSION_POINT),
		tb::wrap(tb::WRAP_MODE_ON)
	);

	push @{$self-> {model}},
		$opt{title} ? [div_create(
			open     => 1,
			style    => pod::TDIVSTYLE_OUTLINE,
		)] : (),
		[model_create(
			indent    => $r-> {indent},
			offset    => $r-> {bigofs},
			lookahead => $opt{title} ? 4 : 1,
		), @imgop],
		;

	if ( $opt{title}) {
		my @g = model_create(
			indent => $r-> {indent},
			offset => $r-> {bigofs}
		);
		push @g,
			tb::moveto( 2, 0, tb::X_DIMENSION_FONT_HEIGHT),
			tb::fontStyle(fs::Italic),
			tb::text(0, length $opt{title}),
			tb::fontStyle(fs::Normal),
			;
		$opt{title} .= "\n";
		${$self->{text}} .= $opt{title};
		$r->{bigofs} += length $opt{title};

		push @{$self-> {model}},
			[model_create, tb::moveto(0, 1, tb::X_DIMENSION_FONT_HEIGHT)],
			\@g,
			[model_create, tb::moveto(0, 1, tb::X_DIMENSION_FONT_HEIGHT)],
			[div_create(open => 0, style => pod::TDIVSTYLE_OUTLINE) ]
			;
	}
	push @{$self-> {model}}, [model_create, tb::moveto(0, 1, tb::X_DIMENSION_FONT_HEIGHT)];
}

sub add_formatted
{
	my ( $self, $format, $text) = @_;

	return unless $self-> {read_state};

	if ( $format eq 'text') {
		$self-> add($text,pod::STYLE_CODE,0);
		$self-> add_new_line;
	} elsif ( $format eq 'podview') {
		while ( $text =~ m/<\s*([^<>]*)\s*>(.*)/gcs) {
			my $cmd = $1;
			my $rest = $2;
			if ( lc($cmd) eq 'cut') {
				$self-> {read_state}-> {pod_cutting} = 0;
			} elsif ( lc($cmd) eq '/cut') {
				$self-> {read_state}-> {pod_cutting} = 1;
			} elsif ( $cmd =~ /^img\s*(.*)$/i) {
				$cmd = $1;
				my %opt;
				while ( $cmd =~ m/\s*([a-z]*)\s*\=\s*(?:(?:'([^']*)')|(?:"([^"]*)")|(\S*))\s*/igcs) {
					my ( $option, $value) = ( lc $1, defined($2)?$2:(defined $3?$3:$4));
					if ( $option =~ /^(width|height|frame)$/ && $value =~ /^\d+$/) { $opt{$option} = $value }
					elsif ( $option =~ /^(src|cut|title)$/) { $opt{$option} = $value }
				}
				if ( defined $opt{src}) {
					my $img = $self->load_image($opt{src}, $opt{frame} // 0, $rest);
					$self->add_image($img, %opt) if $img;
				} elsif ( defined $opt{frame} && defined $self->{images}->[$opt{frame}]) {
					$self->add_image($self->{images}->[$opt{frame}], %opt);
				}
			}
		}
	}
}

sub bullet_paint
{
	my ( $self, $canvas, $block, $state, $x, $y, $filled) = @_;
	$y -= $$block[ tb::BLK_APERTURE_Y];
	my $fh = $canvas-> font-> height * 0.3;
	$filled ?
		$canvas-> fill_ellipse( $x + $fh / 2, $y + $$block[ tb::BLK_HEIGHT] / 2, $fh, $fh) :
		$canvas-> ellipse     ( $x + $fh / 2, $y + $$block[ tb::BLK_HEIGHT] / 2, $fh, $fh);
}

sub noremap {
	my $a = $_[0];
	$a =~ tr/\000-\177/\200-\377/;
	return $a;
}

sub add
{
	my ( $self, $p, $style, $indent) = @_;

	my $cstyle;
	my $r = $self-> {read_state};
	return unless $r;

	$p =~ s/\n//g;
	my $g = [ model_create(
		indent    => $indent,
		offset    => $r-> {bigofs},
		# #0 =head1   =item TEXT  =item *
		# #1 .        .           .
		# #2 text     text
		# #3 .        .
		lookahead =>
			($style == pod::STYLE_ITEM && $r->{bulletMode}) ? 1 : (
				(($style >= pod::STYLE_HEAD_1) && ($style <= pod::STYLE_ITEM)) ? 3 : 0
			),
	) ];
	my $styles = $self-> {styles};
	my $no_push_block;
	my $itemid = scalar @{$self-> {model}};

	if ( $r-> {bulletMode}) {
		if ( $style == pod::STYLE_TEXT || $style == pod::STYLE_CODE || $style == pod::STYLE_VERBATIM) {
			return unless length $p;
			$g = $self-> {model}-> [-1];
			$$g[pod::M_TEXT_OFFSET] = $r-> {bigofs};
			$no_push_block = 1;
			$itemid--;
		}
		$r-> {bulletMode} = 0;
	}

	if ( $style == pod::STYLE_CODE || $style == pod::STYLE_VERBATIM) {
		$$g[ pod::M_FONT_ID] = $styles-> [$style]-> {fontId} || 1; # fixed font
		push @$g, tb::wrap(tb::WRAP_MODE_OFF);
	}

	push @$g, @{$self-> {styleInfo}-> [$style * 2]};
	$cstyle = $styles-> [$style]-> {fontStyle} || 0;

	if ( $style == pod::STYLE_CODE || $style == pod::STYLE_VERBATIM) {
		push @$g, tb::text( 0, length $p),
	} elsif (( $style == pod::STYLE_ITEM) && ( $p =~ /^\*\s*$/ || $p =~ /^\d+\.?$/)) {
		push @$g,
			tb::wrap(tb::WRAP_MODE_OFF),
			tb::color(tb::COLOR_INDEX | 0),
			tb::code( $self->can('bullet_paint'), ($p =~ /^\*\s*$/) ? 1 : 0),
			tb::moveto( 1, 0, tb::X_DIMENSION_FONT_HEIGHT),
			tb::wrap(tb::WRAP_MODE_ON);
		$r-> {bulletMode} = 1;
		$p = '';
	} else { # wrapable text
		$p =~ s/[\s\t]+/ /g;
		$p =~ s/([\200-\377])/"E<".ord($1).">"/ge;
		$p =~ s/(E<[^<>]+>)/noremap($1)/ge;
		my $maxnest = 10;
		my $linkStart = scalar @{$self-> {links}};
		my $m = $p;
		my @ids = ( [-2, 'Z', 2], [ length($m), 'z', 1]);
		while ( $maxnest--) {
			while ( $m =~ m/([A-Z])(<<+) /gcs) {
				my ( $pos, $cmd, $left, $right) = ( pos($m), $1, $2, ('>' x ( length($2))));
				if ( $m =~ m/\G.*? $right(?!>)/gcs) {
					if ( $cmd eq 'X') {
						my $d = length($cmd) + length($left) + 1;
						substr( $m, $pos - $d, pos($m) - $pos + $d, '');
					} else {
						push @ids, [
							$pos - length($left) - 2,
							$cmd,
							length($cmd)+length($left)
						], [
							pos($m) - length($right),
							lc $cmd,
							length($right)
						];
						substr $m, $ids[$_][0], $ids[$_][2], '_' x $ids[$_][2]
							for -2,-1;
					}
				}
			}
			while ( $m =~ m/([A-Z])<([^<>]*)>/gcs) {
				if ( $1 eq 'X') {
					my $d = length($2) + length($1) + 2;
					substr( $m, pos($m) - $d, $d, '');
				} else {
					push @ids,
						[ pos($m) - length($2) - 3, $1, 2],
						[ pos($m) - 1, lc $1, 1];
					substr $m, $ids[$_][0], $ids[$_][2], '_' x $ids[$_][2] for -2,-1;
				}
			}
			last unless $m =~ m/[A-Z]</;
		}

		my %stack = map {[]} qw( fontStyle fontId fontSize wrap color backColor);
		my %val = (
			fontStyle => $cstyle,
			fontId    => 0,
			fontSize  => 0,
			wrap      => 1,
			color     => tb::COLOR_INDEX | 0,
			backColor => tb::BACKCOLOR_DEFAULT,
		);
		my ( $link, $linkHREF) = ( 0, '');

		my $pofs = 0;
		$p = '';
		for ( sort { $$a[0] <=> $$b[0] } @ids) {
			my $ofs = $$_[0] + $$_[2];
			if ( $pofs < $$_[0]) {
				my $s = substr( $m, $pofs, $$_[0] - $pofs);
				$s =~ tr/\200-\377/\000-\177/;
				$s =~ s{
						E<
						(
							( \d+ )
							| ( [A-Za-z]+ )
						)
						>
				} {
					do {
							defined $2
							? chr($2)
							:
						defined $HTML_Escapes{$3}
							? do { $HTML_Escapes{$3} }
							: do { "E<$1>"; }
					}
				}egx;

				if ( $link) {
					my $l;
					if ( $s =~ m/^([^\|]*)\|(.*)$/) {
						$l = $2;
						$s = $1;
						$linkHREF = '';
					} else {
						$l = $s;
					}
					unless ( $s =~ /^\w+\:\/\//) {
						my ( $page, $section) = ( '', '');
						if ( $s =~ /^([^\/]*)\/(.*)$/) {
							( $page, $section) = ( $1, $2);
						} else {
							$section = $s;
						}
						$section =~ s/^\"(.*?)\"$/$1/;
						$s = length( $page) ? "$page: $section" : $section;
					}
					$linkHREF .= $l;
				}

				push @$g, tb::text( length $p, length $s);
				$p .= $s;
			}
			$pofs = $ofs;

			if ( $$_[1] ne lc $$_[1]) { # open
				if ( $$_[1] eq 'I' || $$_[1] eq 'F') {
					push @{$stack{fontStyle}}, $val{fontStyle};
					push @$g, tb::fontStyle( $val{fontStyle} |= fs::Italic);
				} elsif ( $$_[1] eq 'C') {
					push @{$stack{wrap}}, $val{wrap};
					push @$g, tb::wrap( $val{wrap} = tb::WRAP_MODE_OFF);
					my $z = $styles-> [pod::STYLE_CODE];
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @{$stack{$_}}, $val{$_};
						push @$g, $tb::{$_}-> ( $val{$_} = $z-> {$_});
					}
				} elsif ( $$_[1] eq 'L') {
					my $z = $styles-> [pod::STYLE_LINK];
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @{$stack{$_}}, $val{$_};
						push @$g, $tb::{$_}-> ( $val{$_} = $z-> {$_});
					}
					unless ($link) {
						push @$g, $self->op_link_enter;
						$link = 1;
						$linkHREF = '';
					}
				} elsif ( $$_[1] eq 'S') {
					push @{$stack{wrap}}, $val{wrap};
					push @$g, tb::wrap( $val{wrap} = tb::WRAP_MODE_OFF);
				} elsif ( $$_[1] eq 'B') {
					push @{$stack{fontStyle}}, $val{fontStyle};
					push @$g, tb::fontStyle( $val{fontStyle} |= fs::Bold);
				}
			} else { # close
				if ( $$_[1] eq 'i' || $$_[1] eq 'f' || $$_[1] eq 'b') {
					push @$g, tb::fontStyle( $val{fontStyle} = pop @{$stack{fontStyle}});
				} elsif ( $$_[1] eq 'c') {
					my $z = $styles-> [pod::STYLE_CODE];
					push @$g, tb::wrap( $val{wrap} = pop @{$stack{wrap}});
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @$g, $tb::{$_}-> ( $val{$_} = pop @{$stack{$_}});
					}
				} elsif ( $$_[1] eq 'l') {
					my $z = $styles-> [pod::STYLE_LINK];
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @$g, $tb::{$_}-> ( $val{$_} = pop @{$stack{$_}});
					}
					if ( $link) {
						push @$g, $self->op_link_leave;
						$link = 0;
						push @{$self-> {links}}, $linkHREF;
						$self-> {link_map}-> { $itemid} = 1;
					}
				} elsif ( $$_[1] eq 's') {
					push @$g, tb::wrap( $val{wrap} = pop @{$stack{wrap}});
				}
			}
		}
		if ( $link) {
			push @$g, $self->op_link_leave;
			$link = 0;
			push @{$self-> {links}}, $linkHREF;
			$self-> {link_map}-> { $itemid} = 1;
		}

		# add topic
		if (
	        	( $style >= pod::STYLE_HEAD_1 && $style <= pod::STYLE_HEAD_4 ) ||
			(( $style == pod::STYLE_ITEM) && $p !~ /^[0-9*]+\.?$/)
		) {
			my $itemDepth = ( $style == pod::STYLE_ITEM) ?
				scalar @{$r-> {indentStack}} : 0;
			my $pp = $p;
			$pp =~ s/\|//g;
			$pp =~ s/([<>])/'E<' . (($1 eq '<') ? 'lt' : 'gt') . '>'/ge;
			my $newTopic = [ $itemid, 0, $pp, $style, $itemDepth, $linkStart];
			$self-> _close_topic( $style, $newTopic);
			push @{$self-> {topics}}, $newTopic;
		}
	}

	# add text
	$p .= "\n";
	${$self-> {text}} .= $p;

	# all-string format options - close brackets
	push @$g, @{$self-> {styleInfo}-> [$style * 2 + 1]};

	# finish block
	$r-> {bigofs} += length $p;
	push @{$self-> {model}}, $g unless $no_push_block;
}

sub add_new_line
{
	my $self = $_[0];
	my $r = $self-> {read_state};
	return unless $r;
	my $p = " \n";
	${$self-> {text}} .= $p;
	push @{$self-> {model}}, [ model_create( offset => $r->{bigofs} ), tb::text(0, 1) ];
	$r-> {bigofs} += length $p;
}

sub add_verbatim_mark
{
	my ($self, $on) = @_;
	my $r = $self-> {read_state};
	return unless $r;

	my $open;
	if ( $on ) {
		return if defined $r->{verbatim};
		$open = 1;
		$r->{verbatim} = 1;
	} else {
		return unless defined $r->{verbatim};
		$open = 0;
		undef $r->{verbatim};
	}

	push @{$self-> {model}}, [ div_create(open => $open, style => pod::TDIVSTYLE_SOLID) ];
}


sub podpath2file
{
	my ($self, $manpage) = @_;

	my $path = '';
	my ( $fn, $mpath);
	my @ext =  ( '.pod', '.pm', '.pl' );
	push @ext, ( '.bat' ) if $^O =~ /win32/i;
	push @ext, ( '.com' ) if $^O =~ /VMS/;
	for (
		map  { $_, "$_/pod", "$_/pods" }
		grep { defined && length && -d }
			@INC,
			split( $Config::Config{path_sep}, $ENV{PATH})
	) {
		if ( -f "$_/$manpage") {
			$manpage = "$_/$manpage";
			$path = $_;
			goto FOUND;
		}
		$fn = "$_/$manpage";
		$fn =~ s/::/\//g;
		$mpath = $fn;
		$mpath =~ s/\/[^\/]*$//;
		for ( @ext ) {
			if ( -f "$fn$_") {
				$manpage = "$fn$_";
				$path = $mpath;
				goto FOUND;
			}
		}
	}
	return undef;

FOUND:
	return $manpage, $path;
}

sub styles
{
	return $_[0]-> {styles} unless $#_;
	my ( $self, @styles) = @_;
	@styles = @{$styles[0]} if ( scalar(@styles) == 1) && ( ref($styles[0]) eq 'ARRAY');
	if ( $#styles < pod::STYLE_MAX_ID) {
		my @as = @{$_[0]-> {styles}};
		my @pd = @{$_[0]-> default_styles};
		while ( $#styles < pod::STYLE_MAX_ID) {
			if ( $as[ $#styles]) {
				$styles[ $#styles + 1] = $as[ $#styles + 1];
			} else {
				$styles[ $#styles + 1] = $pd[ $#styles + 1];
			}
		}
	}
	$self-> {styles} = \@styles;
	$self-> update_styles;
}

sub update_styles # used for the direct {styles} hacking
{
	my $self = $_[0];
	my @styleInfo;
	for ( @{$self-> {styles}}) {
		my $x = $_;
		my ( @forw, @rev);
		for ( qw( fontId fontSize fontStyle color backColor)) {
			next unless exists $x-> {$_};
			push @forw, $tb::{$_}-> ( $x-> {$_});
			push @rev,  $tb::{$_}-> ( 0);
		}
		push @styleInfo, \@forw, \@rev;
	}
	$self-> {styleInfo} = \@styleInfo;
}


%HTML_Escapes = (
	'amp'         =>        '&',           #   ampersand
	'lt'          =>        '<',           #   left chevron, less-than
	'gt'          =>        '>',           #   right chevron, greater-than
	'quot'        =>        '"',           #   double quote

	"Aacute"      =>        "\xC1",        #   capital A, acute accent
	"aacute"      =>        "\xE1",        #   small a, acute accent
	"Acirc"       =>        "\xC2",        #   capital A, circumflex accent
	"acirc"       =>        "\xE2",        #   small a, circumflex accent
	"AElig"       =>        "\xC6",        #   capital AE diphthong (ligature)
	"aelig"       =>        "\xE6",        #   small ae diphthong (ligature)
	"Agrave"      =>        "\xC0",        #   capital A, grave accent
	"agrave"      =>        "\xE0",        #   small a, grave accent
	"Aring"       =>        "\xC5",        #   capital A, ring
	"aring"       =>        "\xE5",        #   small a, ring
	"Atilde"      =>        "\xC3",        #   capital A, tilde
	"atilde"      =>        "\xE3",        #   small a, tilde
	"Auml"        =>        "\xC4",        #   capital A, dieresis or umlaut mark
	"auml"        =>        "\xE4",        #   small a, dieresis or umlaut mark
	"Ccedil"      =>        "\xC7",        #   capital C, cedilla
	"ccedil"      =>        "\xE7",        #   small c, cedilla
	"Eacute"      =>        "\xC9",        #   capital E, acute accent
	"eacute"      =>        "\xE9",        #   small e, acute accent
	"Ecirc"       =>        "\xCA",        #   capital E, circumflex accent
	"ecirc"       =>        "\xEA",        #   small e, circumflex accent
	"Egrave"      =>        "\xC8",        #   capital E, grave accent
	"egrave"      =>        "\xE8",        #   small e, grave accent
	"ETH"         =>        "\xD0",        #   capital Eth, Icelandic
	"eth"         =>        "\xF0",        #   small eth, Icelandic
	"Euml"        =>        "\xCB",        #   capital E, dieresis or umlaut mark
	"euml"        =>        "\xEB",        #   small e, dieresis or umlaut mark
	"Iacute"      =>        "\xCD",        #   capital I, acute accent
	"iacute"      =>        "\xED",        #   small i, acute accent
	"Icirc"       =>        "\xCE",        #   capital I, circumflex accent
	"icirc"       =>        "\xEE",        #   small i, circumflex accent
	"Igrave"      =>        "\xCD",        #   capital I, grave accent
	"igrave"      =>        "\xED",        #   small i, grave accent
	"Iuml"        =>        "\xCF",        #   capital I, dieresis or umlaut mark
	"iuml"        =>        "\xEF",        #   small i, dieresis or umlaut mark
	"Ntilde"      =>        "\xD1",        #   capital N, tilde
	"ntilde"      =>        "\xF1",        #   small n, tilde
	"Oacute"      =>        "\xD3",        #   capital O, acute accent
	"oacute"      =>        "\xF3",        #   small o, acute accent
	"Ocirc"       =>        "\xD4",        #   capital O, circumflex accent
	"ocirc"       =>        "\xF4",        #   small o, circumflex accent
	"Ograve"      =>        "\xD2",        #   capital O, grave accent
	"ograve"      =>        "\xF2",        #   small o, grave accent
	"Oslash"      =>        "\xD8",        #   capital O, slash
	"oslash"      =>        "\xF8",        #   small o, slash
	"Otilde"      =>        "\xD5",        #   capital O, tilde
	"otilde"      =>        "\xF5",        #   small o, tilde
	"Ouml"        =>        "\xD6",        #   capital O, dieresis or umlaut mark
	"ouml"        =>        "\xF6",        #   small o, dieresis or umlaut mark
	"szlig"       =>        "\xDF",        #   small sharp s, German (sz ligature)
	"THORN"       =>        "\xDE",        #   capital THORN, Icelandic
	"thorn"       =>        "\xFE",        #   small thorn, Icelandic
	"Uacute"      =>        "\xDA",        #   capital U, acute accent
	"uacute"      =>        "\xFA",        #   small u, acute accent
	"Ucirc"       =>        "\xDB",        #   capital U, circumflex accent
	"ucirc"       =>        "\xFB",        #   small u, circumflex accent
	"Ugrave"      =>        "\xD9",        #   capital U, grave accent
	"ugrave"      =>        "\xF9",        #   small u, grave accent
	"Uuml"        =>        "\xDC",        #   capital U, dieresis or umlaut mark
	"uuml"        =>        "\xFC",        #   small u, dieresis or umlaut mark
	"Yacute"      =>        "\xDD",        #   capital Y, acute accent
	"yacute"      =>        "\xFD",        #   small y, acute accent
	"yuml"        =>        "\xFF",        #   small y, dieresis or umlaut mark

	"lchevron"    =>        "\xAB",        #   left chevron (double less than)
	"rchevron"    =>        "\xBB",        #   right chevron (double greater than)
);


1;

__END__

=pod

=head1 NAME

Prima::Drawable::Pod - POD parser and renderer

=head1 SYNOPSIS

	use Prima::Drawable::Pod;

	my $pod = Prima::Drawable::Pod->new;
	$pod-> open_read;
	$pod-> read("=head1 NAME\n\nI'm also a pod!\n\n");
	$pod-> close_read;

=head1 DESCRIPTION

Prima::Drawable::Pod contains a formatter ( in terms of L<perlpod> ) and a renderer of
the POD content.

=head1 USAGE

The package consists of several logically separated parts. These include
file locating and loading, formatting, and navigation.

=head2 Content methods

=over

=item open_read %OPTIONS

Clears the current content and enters the reading mode. In this mode, the
content can be appended by repeatedly calling the C<read> method that pushes
the raw POD content to the parser.

=item read TEXT

Supplies the TEXT string to the parser. Parses basic indentation,
but the main formatting is performed inside L<add> and L<add_formatted>.

Must be called only within the open_read/close_read brackets

=item add TEXT, STYLE, INDENT

Formats the TEXT string of a given STYLE ( one of the C<pod::STYLE_XXX> constants) with
the INDENT space.

Must be called only within the open_read/close_read brackets.

=item add_formatted FORMAT, TEXT

Adds a pre-formatted TEXT with a given FORMAT, supplied by the C<=begin> or
C<=for> POD directives. Prima::PodView understands 'text' and 'podview'
FORMATs; the latter format is for Prima::PodView itself and contains a small
number of commands for rendering images in documents.

The 'podview' commands are:

=over

=item cut

Example:

	=for podview <cut>

	=for text just text-formatter info

		....
		text-only info
		...

	=for podview </cut>

The E<lt>cut<gt> clause skips all POD input until canceled.
It is used in conjunction with the following command, L<img>, to allow
a POD manpage to provide both graphic ('podview', 'html', etc ) and text ( 'text' )
content.

=item img [src="SRC"] [width="WIDTH"] [height="HEIGHT"] [cut="CUT"] [frame="FRAME"]

An image inclusion command, where I<src> is a relative or an absolute path to
an image file. In case scaling is required, C<width> and C<height> options
can be set. If the image is a multiframe image, the frame index can be
set by the C<frame> option. A special C<cut> option, if set to a true value, activates the
L<cut> behavior if ( and only if ) the image load operation is unsuccessful.
This makes possible simultaneous use of 'podview' and 'text' :

	=for podview <img src="graphic.gif" cut=1 >

	=begin text

	y     .
	|  .
	|.
	+----- x

	=end text

	=for podview </cut>

In the example above 'graphic.gif' will be shown if it can be found and loaded,
otherwise, the poor-man drawings will be selected.

If C<src> is omitted, the image is retrieved from the C<images> array, from the index C<frame>.

It is also possible to embed images in the pod, by using a special C<src> tag for base64-encoded
images. The format should preferably be GIF, as this is Prima default format, or BMP for
very small images, as it is supported without third-party libraries:

	=for podview <img src="data:base64">
	R0lGODdhAQABAIAAAAAAAAAAACwAAAAAAQABAIAAAAAAAAACAkQBADs=

=back

=item close_read

Closes the reading mode.  Returns C<undef> if there is no POD context, or a
hash with C<topic_id> (ID of the first topic containing the content) and the
C<success> flag otherwise.

=back

=head2 Topics

Topics reside in the C<{topics}> array, where each is an array with the
following indices of the C<pod::T_XXX> constants:

	pod::T_MODEL_START - start of topic
	pod::T_MODEL_END   - end of a topic
	pod::T_DESCRIPTION - topic name
	pod::T_STYLE       - pod::STYLE_XXX constant
	pod::T_ITEM_DEPTH  - depth of =item recursion
	pod::T_LINK_OFFSET - offset in the links array

=head2 Styles

The C<::styles> property provides access to the styles, applied to different pod
text parts. These styles are:

	pod::STYLE_CODE     - style for C<>
	pod::STYLE_TEXT     - normal text
	pod::STYLE_HEAD_1   - =head1
	pod::STYLE_HEAD_2   - =head2
	pod::STYLE_HEAD_3   - =head3
	pod::STYLE_HEAD_4   - =head4
	pod::STYLE_ITEM     - =item
	pod::STYLE_LINK     - style for L<> text
	pod::STYLE_VERBATIM - style for pre-formatted text

Each style is a hash with the following keys: C<fontId>, C<fontSize>, C<fontStyle>,
C<color>, and C<backColor>, fully analogous to the tb::BLK_DATA_XXX options.
This functionality provides another layer of accessibility to the pod formatter.

=back

=cut
