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

# https://www.w3schools.com/cssref/css_selectors.asp
sub _match_xid
{
	my ($match, $provider, $item) = @_;

	# *			*			Selects all elements
	# .class		.intro			Selects all elements with class="intro"
	# .class1.class2	.name1.name2		Selects all elements with both name1 and name2 set within its class attribute
	# #id			#firstname		Selects the element with id="firstname"
	# element		p			Selects all <p> elements
	# element.class		p.intro			Selects all <p> elements with class="intro"

	my $ok = $match->{wildcard} ? 1 : 0;

	if ( exists $match->{class}) {
		my %classes = map { $_ => 1 } split /\s+/, ( $provider->get_attribute( $item, 'class' ) // '' );
		for my $c ( @{ $match->{class} } ) {
			return 0 unless $classes{$c};
		}
		$ok = 1;
	}

	if ( exists $match->{id}) {
		my $id = $provider->get_attribute($item, 'id');
		return 0 unless defined($id) && $match->{id} eq $id;
		$ok = 1;
	}

	if ( exists $match->{element}) {
		my $name = $provider->get_name($item);
		return 0 unless defined($name) && $match->{element} eq $name;
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

	return sub { _match_xid(\%match, @_) }
}

# [attribute=value]	[target=_blank]		Selects all elements with target="_blank"
sub _attr_eq
{
	my ($attr, $val) = @_;
	return sub { $val eq ($_[0]->get_attribute($_[1], $attr) // '') };
}

# [attribute~=value]	[title~=flower]		Selects all elements with a title attribute containing the word "flower"
sub _attr_word
{
	my ($attr, $val) = @_;
	return sub { ($_[0]->get_attribute($_[1], $attr) // '') =~ /(^|\s)\Q$val\E(\s|$)/ };
}

# [attribute|=value]	[lang|=en]		Selects all elements with a lang attribute value equal to "en" or starting with "en-"
sub _attr_starts_with_word
{
	my ($attr, $val) = @_;
	return sub { ($_[0]->get_attribute($_[1], $attr) // '') =~ /^\Q$val\E(-|$)/ };
}

# [attribute^=value]	a[href^="https"]	Selects every <a> element whose href attribute value begins with "https"
sub _attr_starts_with
{
	my ($attr, $val) = @_;
	return sub { ($_[0]->get_attribute($_[1], $attr) // '') =~ /^\Q$val\E/ };
}

# [attribute$=value]	a[href$=".pdf"]		Selects every <a> element whose href attribute value ends with ".pdf"
sub _attr_ends_width
{
	my ($attr, $val) = @_;
	return sub { ($_[0]->get_attribute($_[1], $attr) // '') =~ /\Q$val\E$/ };
}

# [attribute*=value]	a[href*="w3schools"]	Selects every <a> element whose href attribute value contains the substring "w3schools"
sub _attr_substr
{
	my ($attr, $val) = @_;
	return sub { index(($_[0]->get_attribute($_[1], $attr) // ''), $val) >= 0 };
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
		return sub { defined $_[0]->get_attribute($_[1], $attr) };
	}

	return $self->error("bad attribute selector declaration");
}

# :empty	p:empty	Selects every <p> element that has no children (including text nodes)
sub _predicate_empty
{
	my ( $provider, $item ) = @_;
	return 0 == @{ $provider->get_children($item) };
}

# :first-child	p:first-child	Selects every <p> element that is the first child of its parent
sub _predicate_first_child
{
	my ( $provider, $item ) = @_;
	return ( $provider->get_siblings($item)->[0] // 0 ) == $item;
}

# :first-of-type	p:first-of-type	Selects every <p> element that is the first <p> element of its parent
sub _predicate_first_of_type
{
	my ( $provider, $item ) = @_;
	my $name = $provider->get_name($item);
	return 0 unless defined $name;
	my @r = grep {
		my $iname = $provider->get_name($_);
		defined($iname) ? ($iname eq $name) : 0;
	} @{ $provider->get_siblings($item) };
	return ( $r[0] // 0 ) == $item;
}

# :last-child	p:last-child	Selects every <p> element that is the last child of its parent
sub _predicate_last_child
{
	my ( $provider, $item ) = @_;
	return ( $provider->get_siblings($item)->[-1] // 0 ) == $item;
}

# :last-of-type	p:last-of-type	Selects every <p> element that is the last <p> element of its parent
sub _predicate_last_of_type
{
	my ( $provider, $item ) = @_;
	my $name = $provider->get_name($item);
	return 0 unless defined $name;
	my @r = grep {
		my $iname = $provider->get_name($_);
		defined($iname) ? ($iname eq $name) : 0;
	} @{ $provider->get_siblings($item) };
	return ( $r[-1] // 0 ) == $item;
}

# :only-of-type	p:only-of-type	Selects every <p> element that is the only <p> element of its parent
sub _predicate_only_of_type
{
	my ( $provider, $item ) = @_;
	my $name = $provider->get_name($item);
	return 0 unless defined $name;
	my @r = grep {
		my $iname = $provider->get_name($_);
		defined($iname) ? ($iname eq $name) : 0;
	} @{ $provider->get_siblings($item) };
	return 1 == @r && ( $r[0] // 0 ) == $item;
}

# :only-child	p:only-child	Selects every <p> element that is the only child of its parent
sub _predicate_only_child
{
	my ( $provider, $item ) = @_;
	my $p = $provider->get_siblings($item);
	return 1 == @$p && ( $p->[0] // 0 ) == $item;
}

# :optional	input:optional	Selects input elements with no "required" attribute
sub _predicate_optional
{
	my ( $provider, $item ) = @_;
	return !defined $provider->get_attribute($item, 'required');
}

# ::placeholder	input::placeholder	Selects input elements with the "placeholder" attribute specified
sub _predicate_placeholder
{
	my ( $provider, $item ) = @_;
	return defined $provider->get_attribute($item, 'placeholder');
}

# :read-only	input:read-only	Selects input elements with the "readonly" attribute specified
sub _predicate_read_only
{
	my ( $provider, $item ) = @_;
	return defined $provider->get_attribute($item, 'readonly');
}

# :read-write	input:read-write	Selects input elements with the "readonly" attribute NOT specified
sub _predicate_read_write
{
	my ( $provider, $item ) = @_;
	return !defined $provider->get_attribute($item, 'readonly');
}

# :required	input:required	Selects input elements with the "required" attribute specified
sub _predicate_required
{
	my ( $provider, $item ) = @_;
	return defined $provider->get_attribute($item, 'required');
}

# :root	:root	Selects the document's root element
sub _predicate_root
{
	my ( $provider, $item ) = @_;
	return $provider->get_root($item) == $item;
}

my %predicates = (
	empty           => [ 1, \&_predicate_empty ],
	'first-child'   => [ 1, \&_predicate_first_child ],
	'first-of-type' => [ 1, \&_predicate_first_of_type ],
	'last-child'    => [ 1, \&_predicate_last_child ],
	'last-of-type'  => [ 1, \&_predicate_last_of_type ],
	'only-of-type'  => [ 1, \&_predicate_only_of_type ],
	'only-child'    => [ 1, \&_predicate_only_child ],
	'optional'      => [ 1, \&_predicate_optional ],
	'placeholder'   => [ 2, \&_predicate_placeholder ],
	'read-only'     => [ 1, \&_predicate_read_only ],
	'read-write'    => [ 1, \&_predicate_read_write ],
	'required'      => [ 1, \&_predicate_required ],
	'root'          => [ 1, \&_predicate_root ],
);

# :lang(language)	p:lang(it)	Selects every <p> element with a lang attribute equal to "it" (Italian)
sub _predicate_lang
{
	my ( $lang ) = @_;
	return sub {
		my ( $provider, $item ) = @_;
		return $lang eq ($provider->get_attribute($item, 'lang') // '');
	}
}

# :nth-child(n)	p:nth-child(2)	Selects every <p> element that is the second child of its parent
sub _predicate_nth_child
{
	my ( $n ) = @_;
	$n--;
	return sub {
		my ( $provider, $item ) = @_;
		return ( $provider->get_siblings($item)->[$n] // 0 ) == $item;
	}
}

# :nth-last-child(n)	p:nth-last-child(2)	Selects every <p> element that is the second child of its parent, counting from the last child
sub _predicate_nth_last_child
{
	my ( $n ) = @_;
	return sub {
		my ( $provider, $item ) = @_;
		return ( $provider->get_siblings($item)->[-$n] // 0 ) == $item;
	}
}

# :nth-last-of-type(n)	p:nth-last-of-type(2)	Selects every <p> element that is the second <p> element of its parent, counting from the last child
sub _predicate_nth_last_of_type
{
	my ( $n ) = @_;
	return sub {
		my ( $provider, $item ) = @_;
		my $name = $provider->get_name($item);
		return 0 unless defined $name;
		my @r = grep {
			my $iname = $provider->get_name($_);
			defined($iname) ? ($iname eq $name) : 0;
		} @{ $provider->get_siblings($item) };
		return ( $r[-$n] // 0 ) == $item;
	}
}

# :nth-of-type(n)	p:nth-of-type(2)	Selects every <p> element that is the second <p> element of its parent
sub _predicate_nth_of_type
{
	my ( $n ) = @_;
	$n--;
	return sub {
		my ( $provider, $item ) = @_;
		my $name = $provider->get_name($item);
		return 0 unless defined $name;
		my @r = grep {
			my $iname = $provider->get_name($_);
			defined($iname) ? ($iname eq $name) : 0;
		} @{ $provider->get_siblings($item) };
		return ( $r[$n] // 0 ) == $item;
	}
}

my %predicates2 = (
	lang               => \&_predicate_lang,
	'nth-child'        => \&_predicate_nth_child,
	'nth-last-child'   => \&_predicate_nth_last_child,
	'nth-of-type'      => \&_predicate_nth_of_type,
	'nth-last-of-type' => \&_predicate_nth_last_of_type,
);

sub parse_predicate
{
	my ($self, $colons) = @_;

	my $content = $self->{parser}->{content};

	while ( 1 ) {
		if ( $$content =~ m/\G([-\w]+)\(/gcs ) {
			my $function = $1;
			my $ret;

			if ( $function eq 'not') {
				# :not(selector)	:not(p)	Selects every element that is not a <p> element
				my $selector = $self->get_selector_operand(')');
				return $self->error unless ref $selector;
				$ret = sub { !$selector->(@_) }
			} elsif ( my $cb = $predicates2{ $function } ) {
				return $self->error('too many colons') if 1 != $colons;
				my $param;
				if ( $function eq 'lang') {
					$$content =~ m/\G(\w+)/gcs or return $self->error('string expected');
					$param = $1;
				} else {
					$$content =~ m/\G([1-9]\d*)/gcs or return $self->error('positive integer expected');
					$param = $1;
				}
				$ret = $cb->($param);
			} else {
				return $self->error("unknown predicate function '$function'");
			}

			$$content =~ m/\G\)/gcs or return $self->error(') expected');
			return $ret;
		} elsif ( $$content =~ m/\G([-\w]+)/gcs ) {
			my $function = $1;
			if ( my $rec = $predicates{ $function } ) {
				return $self->error("should be preceded by $rec->[0] colon(s)") if $rec->[0] != $colons;
				return $rec->[1];
			} elsif ( $function =~ m/^(
				active|checked|default|disabled|enabled|focus|fullscreen|
				hover|in\-range|indeterminate|invalid|link|out\-of\-range|
				target|valid|visited
			)/x) {
# :active	a:active	Selects the active link
# :checked	input:checked	Selects every checked <input> element
# :default	input:default	Selects the default <input> element
# :disabled	input:disabled	Selects every disabled <input> element
# :enabled	input:enabled	Selects every enabled <input> element
# :focus	input:focus	Selects the input element which has focus
# :fullscreen	:fullscreen	Selects the element that is in full-screen mode
# :hover	a:hover	Selects links on mouse over
# :in-range	input:in-range	Selects input elements with a value within a specified range
# :indeterminate	input:indeterminate	Selects input elements that are in an indeterminate state
# :invalid	input:invalid	Selects all input elements with an invalid value
# :link	a:link	Selects all unvisited links
# :out-of-range	input:out-of-range	Selects input elements with a value outside a specified range
# :target	#news:target	Selects the current active #news element (clicked on a URL containing that anchor name)
# :valid	input:valid	Selects all input elements with a valid value
# :visited	a:visited	Selects all visited links
				return $self->error('too many colons') if 1 != $colons;
				return sub { $_[0]-> has_predicate($_[1], $function) };
			} elsif ( $function =~ m/^(after|before|marker|selection)$/) {
# ::after	p::after	Insert something after the content of each <p> element
# ::before	p::before	Insert something before the content of each <p> element
# ::marker	::marker	Selects the markers of list items
# ::selection	::selection	Selects the portion of an element that is selected by a user
				return $self->error('two colons expected') if 2 != $colons;
				return sub { $_[0]-> has_predicate($_[1], $function) };
			} else {
				return $self->error("unknown predicate function '$function'");
			}
		} elsif ( $$content =~ m/\G$/gcs ) {
			return $self->error("unexpected end of content");
		} else {
			return $self->error("bad predicate");
		}
	}
}

sub get_selector_operand
{
	my ($self, $terminator) = @_;
	$terminator //= '{,';
	my $content = $self->{parser}->{content};

	my %got;
	while ( 1 ) {
		if ( $$content =~ m/\G([\*\-\#\.\w]+)/gcs ) {
			return $self->error('xid is already declared') if defined $got{xid};
			$got{xid} = $self->parse_xid_selector($1);
		} elsif ( $$content =~ m/\G\[/gcs) {
			return $self->error('attribute selector is already declared') if defined $got{attr};
			$got{attr} = $self->parse_attr_selector;
			return $self->error unless ref $got{attr};
			$$content =~ m/\G\]/gcs or return $self->error('] expected');
		} elsif ( $$content =~ m/\G(\:+)/gcs) {
			my $p = $self->parse_predicate(length $1);
			return $self->error unless ref $p;
			push @{$got{p}}, $p;
		} elsif ( $$content =~ m/\G(?=[\s$terminator])/gcs ) {
			last;
		} elsif ( $$content =~ m/\G$/gcs ) {
			return $self->error("unexpected end of content");
		} else {
			return $self->error("bad selector");
		}
	}

	my @selectors = grep { defined } $got{xid}, $got{attr}, @{ $got{p} // [] };
	return $self->error('empty selector') unless @selectors;

	return $selectors[0] if 1 == @selectors;

	return sub {
		for my $s ( @selectors ) {
			return 0 unless $s->(@_);
		}
		return 1;
	};
}

sub _match_parent
{
	# .class1 .class2	.name1 .name2		Selects all elements with name2 that is a descendant of an element with name1
	# element element	div p			Selects all <p> elements inside <div> elements
	my ( $cb, $provider, $item ) = @_;
	while ($item = $provider->get_parent($item)) {
		return 1 if $cb->($provider, $item);
	}
	return 0;
}

sub _match_direct_parent
{
	# element>element	div > p			Selects all <p> elements where the parent is a <div> element
	my ( $cb, $provider, $item ) = @_;
	my $p = $provider->get_parent($item) or return 0;
	return $cb->($provider, $p);
}

sub _match_right_before
{
	# element+element	div + p			Selects the first <p> element that is placed immediately after <div> elements
	my ( $cb, $provider, $item ) = @_;

	my $p = $provider->get_siblings($item);
	for ( my $i = 0; $i < @$p; $i++) {
		next if $p->[$i] != $item;
		return 0 if $i == 0;
		return $cb->($provider, $p->[$i-1]);
	}

	return 0;
}

sub _match_before
{
	# element1~element2	p ~ ul			Selects every <ul> element that is preceded by a <p> element
	my ( $cb, $provider, $item ) = @_;

	my $p = $provider->get_siblings($item);
	for ( my $i = 0; $i < @$p; $i++) {
		next if $p->[$i] != $item;
		return if $i == 0;

		for ( $i = $i - 1; $i >= 0; $i--) {
			return 1 if $cb->($provider, $p->[$i]);
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

package Prima::sys::CSS::Provider;

sub new
{
	my ( $class, %opt ) = @_;
	my $self = bless {
		%opt
	}, $class;
	return $self;
}

sub get_attribute { $_[1]->{$_[2]} }
sub get_name      { $_[1]->{name} // '' }
sub get_parent    { $_[0]->{cache}->{"$_[1]"} }
sub get_children  { $_[1]->{children} // [] }
sub has_predicate { defined $_[1]->{$_[2]} }

sub set_attribute { $_[1]->{$_[2]} = $_[3] }

sub get_root
{
	my ( $self, $item ) = @_;
	while (1) {
		my $p = $self->get_parent($item);
		return $item unless $p;
		$item = $p;
	}
}

sub get_siblings
{
	my ( $self, $item ) = @_;
	my $p = $self->get_parent($item) or return [$item];
	return $self->get_children($p);
}

sub cache
{
	return $_[0]->{cache} unless $#_;
	$_[0]->{cache} = $_[1];
}

package Prima::sys::CSS::Ruleset;

sub new
{
	my ( $class, %opt ) = @_;
	my $self = bless {
		%opt
	}, $class;
	$self->provider( Prima::sys::CSS::Provider->new ) unless $self->provider;
	return $self;
}

sub provider
{
	return $_[0]->{provider} unless $#_;
	$_[0]->{provider} = $_[1];
}

sub _study
{
	my ( $provider, $cache, $item ) = @_;
	for my $i ( @{ $provider->get_children($item) } ) {
		$cache->{"$i"} = $item;
		_study( $provider, $cache, $i );
	}
}

sub study
{
	my ( $self, $root ) = @_;
	my $provider = $self->{provider};
	my $cache = {};
	$provider->cache($cache);
	_study($provider, $cache, $root) if $root;
}

sub match
{
	my ( $self, $item ) = @_;

	my $provider = $self->{provider};
	my $set = $self->{set} // [];
	my %attr;

	for ( my $i = 0; $i < @$set; $i += 2 ) {
		my ( $selector, $attributes ) = @{$set}[$i,$i+1];
		next unless $selector->($provider, $item);
		%attr = (%attr, %$attributes);
	}

	return %attr;
}

sub match_and_apply
{
	my ( $self, $item ) = @_;
	my $provider = $self->{provider};
	my %attr     = $self->match($item);
	while ( my ( $k, $v ) = each %attr ) {
		$provider->set_attribute( $item, $k, $v );
	}
}

sub _apply_all
{
	my ( $self, $root ) = @_;
	$self->match_and_apply($root);
	for my $c ( @{ $self->{provider}->get_children($root) } ) {
		$self->_apply_all( $c );
	}
}

sub apply_all
{
	my ( $self, $root ) = @_;
	$self->study($root);
	$self->_apply_all($root, $self->{provider});
	$self->study(undef);
}

1;
