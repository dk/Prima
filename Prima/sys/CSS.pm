package Prima::sys::CSS;
use strict;
use warnings;

package Prima::sys::CSS::Parser;

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
		my $pos = pos(${ $self->{parser}->{content} } ) // 0;
		my @lines = split "\n", substr( ${ $self->{parser}->{content} }, 0, $pos);
		$self->{parser}->{error} = $error . " - at line " . scalar(@lines);
		if ( @lines ) {
			substr($lines[-1], 20) = '...' if length($lines[-1]) > 20;
			$self->{parser}->{error} .= ", near '$lines[-1]'";
		}
	}
	return $self->{parser}->{error};
}

sub _class($)    { $_[0]->{class} // '' }
sub _id($)       { $_[0]->{id} // '' }
sub _name($)     { $_[0]->{name} // '' }
sub _parent($)   { $_[0]->{parent} }
sub _children($) { $_[0]->{children} // [] }
sub _attr($$)    { $_[0]->{$_[1]} }

# https://www.w3schools.com/cssref/css_selectors.asp
sub _match_xid
{
	my ($match, $item) = @_;

	# *			*			Selects all elements
	# .class		.intro			Selects all elements with class="intro"
	# .class1.class2	.name1.name2		Selects all elements with both name1 and name2 set within its class attribute
	# #id			#firstname		Selects the element with id="firstname"
	# element		p			Selects all <p> elements
	# element.class		p.intro			Selects all <p> elements with class="intro"

	my $ok = $match->{wildcard} ? 1 : 0;

	if ( exists $match->{class}) {
		my %classes = map { $_ => 1 } split /\s+/, _class $item;
		for my $c ( @{ $match->{class} } ) {
			return 0 unless $classes{$c};
		}
		$ok = 1;
	}

	if ( exists $match->{id}) {
		return 0 unless $match->{id} eq _id $item;
		$ok = 1;
	}

	if ( exists $match->{element}) {
		return 0 unless $match->{element} eq _name $item;
		$ok = 1;
	}

	return $ok;
}

sub parse_xid_selector
{
	my ($self, $selector) = @_;

	$selector =~ s/^\s+//;
	$selector =~ s/\s+$//;

	return $self->error("empty selector") unless length $selector;

	my %match;
	reset $selector;
	while ( 1 ) {
		$selector =~ m/\G$/gcs and last;
		# *			*			Selects all elements
		$selector =~ m/\G\*/gcs and do {
			return $self->error("wildcard match is already defined") if defined $match{wildcard};
			$match{wildcard} = 1;
			next;
		};

		# .class		.intro			Selects all elements with class="intro"
		$selector =~ m/\G\.([-\w]+)/gcs and do {
			push @{$match{class}}, $1;
			next;
		};


		# #id			#firstname		Selects the element with id="firstname"
		$selector =~ m/\G\#([-\w]+)/gcs and do {
			return $self->error("id match is already defined") if defined $match{id};
			$match{id} = $1;
			next;
		};

		# element		p			Selects all <p> elements
		$selector =~ m/\G([-\w]+)/gcs and do {
			return $self->error("element match is already defined") if defined $match{element};
			$match{element} = $1;
			next;
		};

		return $self->error("bad selector declaration");
	}

	return $self->error("no selectors declared") unless keys %match;

	return sub { _match_xid(\%match, $_[0]) }
}

# [attribute=value]	[target=_blank]		Selects all elements with target="_blank"
sub _attr_eq
{
	my ($attr, $val) = @_;
	return sub { $val eq ((_attr($_[0], $attr) // '')) };
}

# [attribute~=value]	[title~=flower]		Selects all elements with a title attribute containing the word "flower"
sub _attr_word
{
	my ($attr, $val) = @_;
	return sub { (_attr($_[0], $attr) // '') =~ /(^|\s)\Q$val\E(\s|$)/ };
}

# [attribute|=value]	[lang|=en]		Selects all elements with a lang attribute value equal to "en" or starting with "en-"
sub _attr_starts_with_word
{
	my ($attr, $val) = @_;
	return sub { (_attr($_[0], $attr) // '') =~ /^\Q$val\E(-|$)/ };
}

# [attribute^=value]	a[href^="https"]	Selects every <a> element whose href attribute value begins with "https"
sub _attr_starts_with
{
	my ($attr, $val) = @_;
	return sub { (_attr($_[0], $attr) // '') =~ /^\Q$val\E/ };
}

# [attribute$=value]	a[href$=".pdf"]		Selects every <a> element whose href attribute value ends with ".pdf"
sub _attr_ends_width
{
	my ($attr, $val) = @_;
	return sub { (_attr($_[0], $attr) // '') =~ /\Q$val\E$/ };
}

# [attribute*=value]	a[href*="w3schools"]	Selects every <a> element whose href attribute value contains the substring "w3schools"
sub _attr_substr
{
	my ($attr, $val) = @_;
	return sub { index((_attr($_[0], $attr) // ''), $val) >= 0 };
}

my %attr_ops = (
	''  => \&_attr_eq,
	'~' => \&_attr_word,
	'|' => \&_attr_starts_with_word,
	'^' => \&_attr_starts_with,
	'$' => \&_attr_ends_width,
	'*' => \&_attr_substr,
);

sub parse_attr_selector
{
	my $self = shift;
	my $content = $self->{parser}->{content};

	# [attribute=value]	[target=_blank]		Selects all elements with target="_blank"
	# [attribute~=value]	[title~=flower]		Selects all elements with a title attribute containing the word "flower"
	# [attribute|=value]	[lang|=en]		Selects all elements with a lang attribute value equal to "en" or starting with "en-"
	# [attribute^=value]	a[href^="https"]	Selects every <a> element whose href attribute value begins with "https"
	# [attribute$=value]	a[href$=".pdf"]		Selects every <a> element whose href attribute value ends with ".pdf"
	# [attribute*=value]	a[href*="w3schools"]	Selects every <a> element whose href attribute value contains the substring "w3schools"
	if ( $$content =~ m/\G['"]?([-\w]+)['"]?\s*([\~\|\^\$\*]?)\=\s*/gcs) {
		my ($attr, $op) = ($1, $2 || '');
		my $xop = $attr_ops{$op} or die "panic: attr op '$op' is undefined";

		my $val;
		if ( $$content =~ m/\G'([^']*)'/gcs) {
			$val = $1;
		} elsif ( $$content =~ m/\G"([^"]*)"/gcs) {
			$val = $1;
		} elsif ( $$content =~ m/\G([-\w]*)/gcs) {
			$val = $1;
		} else {
			return $self->error("bad attribute value");
		}

		return $xop->($attr, $val);
	}

	# [attribute]		[target]		Selects all elements with a target attribute
	elsif ( $$content =~ m/\G([-\w]+)/gcs) {
		my $attr = $1;
		return sub { defined _attr $_[0], $attr };
	}

	return $self->error("bad attribute selector declaration");
}

sub get_selector_operand
{
	my $self    = shift;
	my $content = $self->{parser}->{content};

	if ( $$content =~ m/\G([\*\-\#\.\w]+)/gcs ) {
		my $s_id     = $self->parse_xid_selector($1);
		my $selector = $s_id;
		if ( $$content =~ m/\G\[/gcs ) {
			my $s_attr = $self->parse_attr_selector;
			return $self->error unless ref $s_attr;
			$$content =~ m/\G\s*\]/gcs or return $self->error("] is expected");
			$selector = sub { $s_id->(@_) && $s_attr->(@_) };
		}
		return $selector;
	} elsif ( $$content =~ m/\G\[/gcs ) {
		my $s_attr = $self->parse_attr_selector;
		return $self->error unless ref $s_attr;
		$$content =~ m/\G\s*\]/gcs or return $self->error("] is expected");
		return $s_attr;
	} else {
		return $self->error('bad selector');
	}
}

sub _match_parent
{
	# .class1 .class2	.name1 .name2		Selects all elements with name2 that is a descendant of an element with name1
	# element element	div p			Selects all <p> elements inside <div> elements
	my ( $cb, $item ) = @_;
	while ( _parent $item) {
		$item = _parent $item;
		return 1 if $cb->($item);
	}
	return 0;
}

sub _match_direct_parent
{
	# element>element	div > p			Selects all <p> elements where the parent is a <div> element
	my ( $cb, $item ) = @_;
	my $p = _parent $item or return 0;
	return $cb->($p);
}

sub _match_right_before
{
	# element+element	div + p			Selects the first <p> element that is placed immediately after <div> elements
	my ( $cb, $item ) = @_;

	return 0 unless my $p = _parent $item;
	$p = _children $p;

	for ( my $i = 0; $i < @$p; $i++) {
		next if $p->[$i] != $item;
		return 0 if $i == 0;
		return $cb->($p->[$i-1]);
	}

	return 0;
}

sub _match_before
{
	# element1~element2	p ~ ul			Selects every <ul> element that is preceded by a <p> element
	my ( $cb, $item ) = @_;

	return 0 unless my $p = _parent $item;
	$p = _children $p;

	for ( my $i = 0; $i < @$p; $i++) {
		next if $p->[$i] != $item;
		return if $i == 0;

		for ( $i = $i - 1; $i >= 0; $i--) {
			return 1 if $cb->($p->[$i]);
		}
		last;
	}

	return 0;
}

my %hier_ops = (
	' ' => \&_match_parent,
	'>' => \&_match_direct_parent,
	'+' => \&_match_right_before,
	'~' => \&_match_before,
);

sub _match_chain
{
	my $chain = shift;

	return 0 unless $chain->[-1]->(@_);

	for ( my $i = $#$chain - 2; $i >= 0; $i -= 2) {
		my ($cb, $op) = @{$chain}[$i,$i+1];
		die "panic: bad op '$op'" unless $hier_ops{$op};
		return 0 unless $hier_ops{$op}->($cb, @_);
	}

	return 1;
}

sub parse_selector
{
	my $self = shift;

	my $content = $self->{parser}->{content};

	my @chain = ($self->get_selector_operand);
	return $self->error unless ref $chain[-1];

	while ( 1 ) {
		$$content =~ m/\G$/gcs and return $self->error('selector expected');
		$$content =~ m/\G[\n\t\s]+/gcs and next;
		$$content =~ m/\G([\>\+\~])/gcs and do {
			return $self->error("xid expected") unless ref $chain[-1];
			push @chain, $1;
			next;
		};
		$$content =~ m/\G([,{])/gcs and do {
			pos($$content) = pos($$content) - 1;
			last;
		};
		my $operand = $self->get_selector_operand;
		return $self->error unless ref $operand;
		push @chain, ' ' if ref $chain[-1];
		push @chain, $operand;
	}
	return $self->error('xid expected') unless ref $chain[-1];

	return $chain[0] if 1 == @chain;
	return sub { _match_chain(\@chain, @_) };
}

sub parse_selectors
{
	my $self = shift;
	my $content = $self->{parser}->{content};

	my @selectors;
	my $comma;

	while ( 1 ) {
		$$content =~ m/\G$/gcs and return $self->error("unexpected end of content");
		$$content =~ m/\G\s*/gcs and next;

		$$content =~ m/\G\{/gcs and do {
			return $self->error('selector expected') if $comma;
			last;
		};
		$$content =~ m/\G\,/gcs and do {
			$comma = 1;
			next;
		};
		$comma = 0;

		my $selector = $self->parse_selector;
		return $self->error unless ref $selector;
		push @selectors, $selector;
	}

	return $self->error("No selectors defined") unless @selectors;
	return $selectors[0] if 1 == @selectors;

	return sub {
		for my $s ( @selectors ) {
			return 1 if $s->(@_);
		}
		return undef;
	};
}

sub parse_attribute
{
	my $self = shift;
	my $content = $self->{parser}->{content};

	my $val = '';
	my $quoted = 0;
	while ( 1 ) {
		$$content =~ m/\G$/gcs and return $self->error("unexpected end of attribute value");
		$$content =~ m/\G\s*;\s*/gcs and last;
		$$content =~ m/\G[\n\r\t\s]+/gcs and do {
			$val .= ' ' if length $val;
			next;
		};
		$$content =~ m/\G'([^']*)'/gcs and do {
			return $self->error("unexpected quote") if length $val;
			$val = $1;
			$quoted = 1;
			next;
		};
		$$content =~ m/\G"([^"]*)"/gcs and do {
			return $self->error("unexpected double quote") if length $val;
			$val = $1;
			$quoted = 1;
			next;
		};
		$$content =~ m/\G([-\w]+)/gcs and do {
			$val = $1;
			next;
		};
		return $self->error("bad attribute decalaration");
	}

	$val =~ s/\s+$// unless $quoted;
	return \$val;
}

sub parse_attributes
{
	my $self = shift;
	my $content = $self->{parser}->{content};

	my %attributes;
	while ( 1 ) {
		$$content =~ m/\G$/gcs and return $self->error("unexpected end of attribute list");
		$$content =~ m/\G\s*}\s*/gcs and last;
		$$content =~ m/\G[\n\r\t\s]+/gcs and next;
		$$content =~ m/\G([-\w]+)\s*\:\s*/gcs and do {
			my $attr = $1;
			my $value = $self->parse_attribute;
			return $self->error unless ref $value;
			$attributes{$attr} = $$value;
			next;
		};
		return $self->error("bad attribute");
	}

	return \%attributes;
}

sub parse
{
	my ( $self, $content ) = @_;

	reset($content);

	local $self->{parser} = {
		content => \ $content,
	};
	my @rules;

	my $selector;
	while ( 1 ) {
		$content =~ m/\G\s+/gcs and redo;

		if ( !defined $selector) {
			$content =~ m/\G$/gcs and last;

			$selector = $self->parse_selectors;
			return $self->error unless ref $selector;
		} else {
			my $attributes = $self->parse_attributes;
			return $self->error unless ref $attributes;

			push @rules, $selector, $attributes;
			undef $selector;
		}

	}

	return Prima::sys::CSS::Ruleset->new( set => \@rules);
}

package Prima::sys::CSS::Ruleset;

sub new
{
	my ( $class, %opt ) = @_;
	my $self = bless {
		%opt
	}, $class;
	return $self;
}

sub match
{
	my ( $self, $item ) = @_;

	my $set = $self->{set} // [];
	my %attr;

	for ( my $i = 0; $i < @$set; $i += 2 ) {
		my ( $selector, $attributes ) = @{$set}[$i,$i+1];
		next unless $selector->($item);
		%attr = (%attr, %$attributes);
	}

	return %attr;
}

1;
