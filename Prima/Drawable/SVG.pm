package Prima::Drawable::SVG;

use strict;
use warnings;

package Prima::Drawable::SVG::Parser;
use Prima::sys::HTMLEscapes;

sub new
{
	my ( $class, %opt ) = @_;
	my $self = bless {
		%opt
	}, $class;
	return $self;
}

sub error
{
	my ( $self, $error ) = @_;
	if (defined $error) {
		my $pos = pos(${ $self->{parser}->{content} } );
		my @lines = split "\n", substr( ${ $self->{parser}->{content} }, 0, $pos);
		substr($lines[-1], 20) = '...' if length($lines[-1]) > 20;
		$self->{parser}->{error} = $error . " -  at line " . scalar(@lines) . ", near '$lines[-1]'";
	}
	return $self->{parser}->{error};
}

sub parse_attr
{
	my ( $self, $tag, $attr, $ct, $storage) = @_;
	$storage->{$attr} = $ct;
	return 1;
}

sub parse_tag
{
	my ($self, $open, $tag) = @_;
	my %ret = (
		name     => '',
		attr     => {},
		children => [],
		content  => '',
	);

	return $self->error("no tag in $tag") unless $tag =~ m/\G(\w+)\s*/gcs;
	$ret{name} = $1;

	while ( 1 ) {
		$tag =~ m/\G\s+/gcs and redo;
		$open and $tag =~ m/\G(\w+)=/gcs and do {
			my $attr = $1;
			my $ct;
			if ( $tag =~ m/\G\"([^\"]*)\"/gcs) {
				$ct = $1;
			} elsif ( $tag =~ m/\G\'([^\']*)\'/gcs) {
				$ct = $1;
			} elsif ( $tag =~ m/\G([^\s\'\"]+)/gcs) {
				$ct = $1;
			} else {
				return $self->error("syntax error in $tag");
			}

			$self->parse_attr($ret{name}, $attr, $ct, $ret{attr}) or return $self->error;
			redo;
		};
		$tag =~ m/\G(\S+)/gcs and return $self->error("syntax error in $1");
		$tag =~ m/\G$/gcs and last;
	}


	if ( $open ) {
		return $self->error unless $self->validate_tag(\%ret);
	}

	return \%ret;
}

sub parse_text
{
	my $text = pop;
	$text =~ s[&(\w+);][$HTML_Escapes{$1} // '']ge;
	$text =~ s/&#([0-9a-f]);/chr(hex($1))/ge;
	$text =~ s/\n+/ /gs;
	$text =~ s/^\s+//;
	$text =~ s/\s+$//;
	return $text;
}

sub parse
{
	my ( $self, $content ) = @_;

	reset($content);

	my $root  = { name => 'root', attr => {}, children => [], content => '' };
	my @stack = ($root);

	local $self->{parser} = {
		root => $root,
		ids  => {},
		content => \ $content,
	};

	while ( 1 ) {
		$content =~ m/\G\s+/gcs and redo;
		$content =~ m/\G<\?[^?]*\?>/gcs and redo;
		$content =~ m/\G<([^>]*)\>/gcs and do {
			my $c = $1;
			if ( $c =~ m/^\/(.*)/) {
				return $self->error("stack underflow") if 1 == @stack;
				my $tag = $self->parse_tag(0, $1);
				return $self->error unless ref $tag;
				return $self->error("tag mismatch: got $tag->{name}, $stack[-1]->{name} expected") if 
					$tag->{name} ne $stack[-1]->{name};
				pop @stack;
			} else {
				my $tail;
				if ( $c =~ m/^(.*)\/$/s) {
					$tail = 1;
					$c = $1;
				}
				my $tag = $self->parse_tag(1, $c);
				return $self->error unless ref $tag;

				if ( 1 == @stack ) {
					return $self->error('first tag is not svg') unless $tag->{name} eq 'svg';
					return $self->error('svg tag is already defined svg') if @{$root->{children}} == 1;
				}

				$self->{parse}->{ids}->{$tag->{attr}->{id}} = $tag
					if exists $tag->{attrs}->{id};
				push @{$stack[-1]->{children}}, $tag;
				push @stack, $tag unless $tail;
			}
			redo;
		};
		$content =~ m/\G([^<]+)/gcs and do {
			return $self->error("unexpected content") if 1 == @stack;
			$stack[-1]->{content} = $self->parse_text($1);
			redo;
		};
		$content =~ m/\G(.+)/gcs and return $self->error("unexpected command: $1");
		$content =~ m/\G$/gcs and last;
	}
	return $self->error("tag <$stack[-1]->{name}> not closed") if 1 < @stack;
	return $self->error("no svg tag") unless $root->{children}->[0];

	return Prima::Drawable::SVG::Tree->new( tree => $root->{children}->[0] );
}

my %types = (
	ge0   => sub { (${$_[0]} >= 0) ? undef : "must be positive numeric" },
	len   => 'int&ge0',
	int   => qr/^\d+$/,
	pct   => qr/^\d+(\.\d+)?%$/,
	lp    => 'len|pct',
	X     => '=0:lp',
	W     => '=100%:lp',
	coord => '=100:lp',
);

sub type_check
{
	my ( $type, $v) = @_;
	die "no such typedef: $type" unless my $t = $types{$type};
	$t = type_replace($type, type_compile($type)) unless ref($t);
	#print "CHECK ".($$v // 'undef') . " with $type ( $t )\n";

	if ( ref($t) eq 'Regexp') {
		return $$v =~ /$t/ ? undef : "does not match regexp $t for type constraint '$type'";
	} elsif ( ref($t) eq 'CODE') {
		return $t->($v);
	} else {
		die "bad typedef $type $t";
	}
}

sub type_replace { $types{$_[0]} = $_[1] }

sub type_compile
{
	my $type = shift;
	return $types{$type} if ref($types{$type});

	my $default;
	my $typedef = $types{$type};
	#print "$type => $typedef\n";
	if ( $typedef =~ m/^=([^:|]*):?(.*)$/) {
		$default = $1;
		$typedef = $2;
		return type_replace($type, sub { ${$_[0]} //= $default; 1 }) unless length $typedef;
	} else {
		return type_replace($type, sub { 1 }) unless length $typedef;
	}

	if ( $typedef =~ m/\|/) {
		my @ors = split qr/\|/, $typedef;
		return type_replace( $type, sub {
			if ( defined($default) && !(defined ${$_[0]})) {
				${$_[0]} = $default;
				return undef;
			}
			my @errs;
			for my $or ( @ors ) {
				my $err = type_check($or, $_[0]);
				return undef unless defined $err;
				push @errs, $err;
			}
			return join('; ', @errs);
		});
	} elsif ( $typedef =~ m/\&/) {
		my @ands = split qr/\&/, $typedef;
		return type_replace( $type, sub {
			if ( defined($default) && !(defined ${$_[0]})) {
				${$_[0]} = $default;
				return undef;
			}
			for my $and ( @ands ) {
				my $err = type_check($and, $_[0]);
				return $err if defined $err;
			}
			return undef;
		});
	} elsif ( defined $default) {
		return type_replace( $type, sub {
			if ( !(defined ${$_[0]})) {
				${$_[0]} = $default;
				return undef;
			}
			return type_check($typedef, $_[0]);
		});
	} else {
		return type_replace($type, type_compile($typedef));
	}
}


my %tags = (
	svg     => [qw(height:W width:W x:X y:X)],
	ellipse => [qw(cx:X cy:X rx:coord ry:coord) ],
);

sub validate_tag
{
	my ( $self, $hash ) = @_;

	my $attr = $hash->{attr};

	if ( my $types = $tags{$hash->{name}}) {
		for my $type ( @$types ) {
			if ($type =~ m/^(\w+):(\w+)$/) {
				my ( $subtag, $subtype ) = ($1,$2);
				die "no such typedef: $type" unless exists $types{$subtype};
				if ( exists $attr->{$subtag}) {
					# print "? $subtag=$attr->{$subtag} :: $subtype\n";
					my $err = type_check($subtype, \$attr->{$subtag});
					next unless defined $err;

					$self->error("'$subtag' value '$attr->{$subtag}' is invalid in <$hash->{name}> ($err) ");
					return 0;
				} else {
					my $new_value = undef;
					# print "? $subtag=undef :: $subtype\n";
					my $err = type_check($subtype, \$new_value);
					if ( defined $err ) {
						$self->error("'$subtag' expected in $hash->{name}");
						return 0;
					}
					$attr->{$subtag} = $new_value;
				}
			} else {
				die "bad typedef $type";
			}
		}
	}

	return 1;
}

package Prima::Drawable::SVG::Tree;

sub new
{
	my ( $class, %opt ) = @_;
	my $self = bless {
		%opt
	}, $class;
	return $self;
}

sub apply_transform
{
	my $self = shift;
	my $pos  = shift;
	my $attr = shift->{attr};
	my $m    = $self->{matrix};
	my $v    = $self->{view};
	my @ret;

	for (my $i = 0; $i < @_; $i += 2 ) {
		my @r = map { $attr->{$_} // 0 } @_[$i,$i+1];
		for ( my $j = 0; $j < 2; $j++) {
			if ( $r[$j] =~ m/^([+-]?\d+(?:\.\d+)?)(%)$/) {
				my ( $val, $unit) = ($1, $2);
				if ( $unit eq '%') {
					$r[$j] = $val * $v->[2 + $j] / 100;
				} else { # XXX
					$r[$j] = $val;
				}
			}
		}
		my @o = @r;
		if ( $pos ) {
			for ( my $j = 0; $j < 2; $j++) {
				$r[$j] = $$m[ $j ] * $o[0] + $$m[$j + 2] * $o[1] + $$m[ $j + 4 ];
			}
		}
		print "@_[$i,$i+1] $pos:@o / @$m => @r\n";
		push @ret, @r;
	}

	return @ret;
}

sub apply_position { shift->apply_transform(1, @_) }
sub apply_size     { shift->apply_transform(0, @_) }

sub draw_svg
{
	my ( $self, $canvas, $tag ) = @_;
	my ( $x, $y ) = $self->apply_position($tag, qw(x y));
	my ( $w, $h ) = $self->apply_size($tag, qw(width height));

	my @cr = $canvas->clipRect;
	my @nr = ($x, $y, $x + $w, $y - $h);
	$nr[0] = $cr[0] if $nr[0] < $cr[0];
	$nr[1] = $cr[1] if $nr[1] < $cr[1];
	$nr[2] = $cr[2] if $nr[2] > $cr[2];
	$nr[3] = $cr[3] if $nr[3] > $cr[3];
	$canvas->clipRect(@nr);
	my ($m, $v) = ($self->{matrix}, $self->{view});
	local $self->{matrix} = [
		@$m[0..3],
		$x,
		$y
	];
	local $self->{view} = [
		$x, $y, $w, $h
	];
	$self->draw_tag( $canvas, $_ ) for @{$tag->{children}};
	$canvas->clipRect(@cr);
}

sub draw_ellipse
{
	my ( $self, $canvas, $tag ) = @_;
	my ( $cx, $cy ) = $self-> apply_position( $tag, qw(cx cy));
	my ( $rx, $ry ) = $self-> apply_size( $tag, qw(rx ry));
	$canvas->new_path->ellipse($cx, $cy, $rx, $ry)->stroke;
}

sub draw_rect
{
	my ( $self, $canvas, $tag ) = @_;
	my ( $x, $y ) = $self-> apply_position( $tag, qw(x y));
	my ( $w, $h ) = $self-> apply_size( $tag, qw(width height));
	print "$x $y $w $h\n";
	$canvas->bar($x, $y, $x + $w, $y - $h);
}

sub draw_tag
{
	my ( $self, $canvas, $tag ) = @_;
	my %state;
	my $meth = $self->can("draw_" . $tag->{name});
	return unless $meth;
	$meth->($self, $canvas, $tag);
}


sub draw
{
	my ( $self, $canvas, $x, $y ) = @_;

	return unless $self->{tree};

	$x //= 0;
	$y //= 0;

	local $self->{view}   = [0,0,$canvas->size];
	local $self->{matrix} = [1,0,0,-1,$x,$canvas->height - $y - 1];
	$self->draw_tag( $canvas, $self->{tree});
}

1;
