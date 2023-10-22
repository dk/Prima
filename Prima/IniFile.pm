#  Created by Anton Berezin  <tobez@tobez.org>
package Prima::IniFile;

use strict;
use warnings;
use Carp;
use Cwd;

sub new { shift-> create(@_); }     # a shortcut

sub create
{
	my $class = shift;
	my %profile;
	%profile = @_ if scalar(@_)%2==0;
	%profile = (file => shift) if scalar(@_)%2==1;
	%profile = (%profile, @_) if scalar(@_)%2==0;
	my $self = {};
	bless( $self, $class);
	$self-> clean;
	$self-> {fileName} = $profile{-file} if exists $profile{-file};
	$self-> {fileName} = $profile{file} if exists $profile{file};
	$self-> read($self-> {fileName}, %profile) if exists $self-> {fileName};
	return $self;
}

sub DESTROY
{
	my $self = shift;
	$self-> write;
}

sub canonicalize_fname
{
	my $p = shift;
	return Cwd::abs_path($p) if -d $p;
	my $dir = $p;
	my $fn;
	if ($dir =~ s{[/\\]([^\\/]+)$}{}) {
		$fn = $1;
	} else {
		$fn = $p;
		$dir = '.';
	}
	unless ( scalar( stat $dir)) {
		$dir = "";
	} else {
		$dir = eval { Cwd::abs_path($dir) };
		$dir = "." if $@;
		$dir = "" unless -d $dir;
		$dir =~ s/(\\|\/)$//;
	}
	return "$dir/$fn";
}

sub read
{
	my ($self, $fname, %profile) = @_;
	$self-> write;             # save the old contents
	$self-> clean;
	$self-> {fileName} = canonicalize_fname($fname);
	eval
	{
		my $f;
		open $f, "<", $fname or do
		{
			open $f, ">", $fname or die "Cannot create $fname: $!\n";
			close $f;
			open $f, "<", $fname or die "Cannot open $fname: $!\n";
		};
		binmode $f, ":utf8";
		my @chunks;
		my %sectionChunks = ('' => [0]);
		my %sectionItems = ('' => []);
		my $currentChunk = [];
		my $items = {};
		my $chunkNum = 0;
		my $line = 0;
		push @chunks, $currentChunk;
		push @{$sectionItems{''}}, $items;
		while (<$f>)
		{
			chomp;
			if ( /^\s*\[(.*?)\]/)         # new section?
			{
				my $section = $1;
				$currentChunk = [];
				$items = {};
				push @chunks, $currentChunk;
				$chunkNum++;
				$line = 0;
				if ( exists $sectionChunks{$section})
				{
					push @{$sectionChunks{$section}}, $chunkNum;
					push @{$sectionItems{$section}}, $items;
				}
				else
				{
					$sectionChunks{$section} = [$chunkNum];
					$sectionItems{$section} = [$items];
				}
				next;
			}
			next   if /^\s*[;#]/;      # comment
			next   unless /^\s*(.*?)\s*=/;
			# another value found
			my $item = $1;
			if ( exists $items-> {$item})
			{
				# duplicate
				push @{$items-> {$item}}, $line;
			}
			else
			{
				# first such $item in this portion of the $section
				$items-> {$item} = [$line];
			}
		}
		continue
		{
			push( @$currentChunk, $_);
			$line++;
		}
		close $f;
		push( @{$chunks[-1]}, '') if scalar(@{$chunks[-1]}) && $chunks[-1]-> [-1] !~ /^\s*$/;
		$self-> {chunks} = [@chunks];
		$self-> {sectionChunks} = {%sectionChunks};
		$self-> {sectionItems} = {%sectionItems};

		# default values
		my $def;
		$def = $profile{default} || $profile{-default};
		# flatten hash to an array if neccessary
		$def = [%$def] if ref($def) eq q/HASH/;
		if ( ref($def) eq q/ARRAY/ && scalar(@$def)%2 == 0)
		{
			while (scalar(@$def))
			{
				my $sectName = shift @$def;
				my $sectValue = shift @$def;
				# flatten hash to an array if neccessary
				$sectValue = [%$sectValue] if ref($sectValue) eq q/HASH/;
				if ( ref($sectValue) eq q/ARRAY/ && scalar(@$sectValue)%2 == 0)
				{
					while (scalar(@$sectValue))
					{
						my $itemName = shift @$sectValue;
						my $itemValue = shift @$sectValue;
						my @defaultValues = (ref($itemValue) eq q/ARRAY/ ? (@$itemValue) : ($itemValue));
						my @vals = $self-> get_values( $sectName, $itemName);
						if (scalar(@defaultValues) > scalar(@vals))
						{
							splice @defaultValues, 0, scalar(@vals);
							$self-> add_values( $sectName, $itemName, @defaultValues);
						}
					}
				}
			}
		}
	};
	$self-> clean if $@;
	warn($@) if $@;
}

sub clean
{
	my $self = $_[0];
	$self-> {fileName} = undef;
	$self-> {changed} = undef;
	$self-> {chunks} = undef;
	$self-> {sectionChunks} = undef;
	$self-> {sectionItems} = undef;
}

sub sections
{
	my $self = $_[0];
	if ( wantarray)
	{
		return () unless defined $self-> {fileName};
		my $h = $self-> {sectionChunks};
		return sort { $h-> {$a}-> [0] <=> $h-> {$b}-> [0] } keys %$h;
	}
	else
	{
		return 0 unless defined $self-> {fileName};
		return scalar values %{$self-> {sectionChunks}};
	}
}

sub items
{
	my ($self,$section) = @_;
	my $all = ($#_ == 2) ? ($_[2]) : ( $#_ == 3 && ($_[2] eq 'all' || $_[2] eq '-all') ? $_[3] : 0);
	return wantarray ? () : 0
		unless defined($self-> {fileName}) &&
				defined($section) &&
				defined($self-> {sectionItems}-> {$section});
	my %items;
	my @items;
	if ( $all)
	{
		for ( @{$self-> {sectionChunks}-> {$section}})
		{
			# analyze every chunk once-again
			for ( @{$self-> {chunks}-> [$_]})
			{
				next   if /^\s*\[(.*?)\]/;     # section header
				next   if /^\s*[;#]/;          # comment
				next   unless /^\s*(.*?)\s*=/; # not item=value pair
				push @items, $1;
			}
		}
	}
	else
	{
		for ( @{$self-> {sectionItems}-> {$section}})
		{
			push @items, map { if (exists $items{$_}) {()} else { $items{$_}=1; $_ }} sort { $_-> {$a}-> [0] <=> $_-> {$b}-> [0] } keys %$_;
		}
	}
	return @items;
}

sub nvalues
{
	my ($self,$section,$item) = @_;
	return 0
		unless defined($self-> {fileName}) &&
				defined($section) &&
				defined($self-> {sectionItems}-> {$section}) &&
				defined($item);
	my $n = 0;
	for (@{$self-> {sectionItems}-> {$section}})
	{
		next unless exists $_-> {$item};
		$n += scalar @{$_-> {$item}}
	}
	return $n;
}

sub get_values
{
	my ($self,$section,$item) = @_;
	return wantarray ? () : undef
		unless defined($self-> {fileName}) &&
				defined($section) &&
				defined($self-> {sectionItems}-> {$section}) &&
				defined($item);
	my @vals;
	my $chunk = 0;
	for (@{$self-> {sectionItems}-> {$section}})
	{
		next unless exists $_-> {$item};
		my $txt = $self-> {chunks}-> [$self-> {sectionChunks}-> {$section}-> [$chunk]];
		for (@{$_-> {$item}})
		{
			my $line = $txt-> [$_];
			warn( "IniFile: Internal error 1\n"), next
				unless $line =~ /^\s*(.*?)\s*=(.*)$/ && $1 eq $item;
			push @vals, $2;
		}
	}
	continue
	{
		$chunk++;
	}
	return wantarray ? @vals : (scalar @vals ? $vals[0] : undef);
}

sub set_values
{
	my ($self,$section,$item,@vals) = @_;
	return unless defined($self-> {fileName}) &&
					defined($section) &&
					defined($item) &&
					scalar(@vals);
	@vals = map { defined $_ ? $_ : '' } @vals;

	$self-> {changed} = 1;

	my $chunk = 0;
	my $lastLine = -1;
	my $lastChunk = -1;
	if (!defined($self-> {sectionItems}-> {$section}))
	{
		# No such section, adding
		push @{$self-> {chunks}}, ["[$section]",''];
		$self-> {sectionChunks}-> {$section} = [scalar(@{$self-> {chunks}}) - 1];
		$self-> {sectionItems}-> {$section} = [{}];
	}
	SECTION: for (@{$self-> {sectionItems}-> {$section}})
	{
		next unless exists $_-> {$item};
		my $txt = $self-> {chunks}-> [$self-> {sectionChunks}-> {$section}-> [$chunk]];
		for (@{$_-> {$item}})
		{
			warn( "IniFile: Internal error 2\n"), next
				unless $txt-> [$_] =~ /^\s*(.*?)\s*=(.*)$/ && $1 eq $item;
			$txt-> [$_] =~ s{^(\s*.*?\s*=).*$}{$1$vals[0]};
			shift @vals;
			last SECTION unless scalar @vals;
			$lastChunk = $chunk;
			$lastLine = $_;
		}
	}
	continue
	{
		$chunk++;
	}
	return unless scalar @vals;
	if ( $lastChunk < 0)
	{
		# there was no such $item, not a single time
		$lastChunk = $chunk - 1;
		# find last non-empty line in the chunk
		my $txt = $self-> {chunks}-> [$self-> {sectionChunks}-> {$section}-> [$lastChunk]];
		$lastLine = scalar(@$txt) - 1;
		$lastLine-- while ($lastLine >= 0) && (($txt-> [$lastLine] =~ /^\s*$/) || ($txt-> [$lastLine] =~ /^\s*[;#]/));
		warn( "IniFile: Internal error 3\n"), return if $lastLine < 0;
	}
	# add the rest
	while ( scalar @vals)
	{
		$lastLine++;
		my $txt = $self-> {chunks}-> [$self-> {sectionChunks}-> {$section}-> [$lastChunk]];
		my $items = $self-> {sectionItems}-> {$section}-> [$lastChunk];
		splice @$txt, $lastLine, 0, "$item=$vals[0]";
		shift @vals;
		# modify line numbering
		for (keys %$items)
		{
			my $ary = $items-> {$_};
			for (@$ary)
			{
				$_++ if $_ >= $lastLine;
			}
		}
		if (exists $items-> {$item})
		{
			push @{$items-> {$item}}, $lastLine;
		}
		else
		{
			$items-> {$item} = [$lastLine];
		}
	}
}

sub add_values
{
	my ($self,$section,$item,@vals) = @_;
	return unless defined($self-> {fileName}) &&
					defined($section) &&
					defined($item) &&
					scalar(@vals);
	@vals = map { defined $_ ? $_ : '' } @vals;
	$self-> set_values($section,$item,$self-> get_values($section,$item),@vals);
}

sub replace_values
{
	my ($self,$section,$item,@vals) = @_;
	return unless defined($self-> {fileName}) &&
					defined($section) &&
					defined($item);
	@vals = () if $#vals == 0 && !defined $vals[0];
	@vals = map { defined $_ ? $_ : '' } @vals;
	my $nv = scalar @vals;
	$self-> {changed} = 1;
	$self-> set_values($section,$item,@vals);
	return unless defined($self-> {sectionItems}-> {$section});
	# and now circle through and delete the rest
	my $chunk = 0;
	my $n = 0;
	for (@{$self-> {sectionItems}-> {$section}})
	{
		next unless exists $_-> {$item};
		my $txt = $self-> {chunks}-> [$self-> {sectionChunks}-> {$section}-> [$chunk]];
		my $ary = $_-> {$item};
		for (my $i = 0; $i < scalar(@$ary); $i++)
		{
			if ( $n >= $nv)
			{
				# actual deletion
				my $line = $ary-> [$i];
				splice @$txt, $ary-> [$i], 1;
				splice @$ary, $i, 1;
				$i--;
				# and line numbering adjustment
				my $items = $_;
				for (keys %$items)
				{
					my $ary = $items-> {$_};
					for (@$ary)
					{
						$_-- if $_ > $line;
					}
				}
			}
			else
			{
				$n++;
			}
		}
		if (scalar(@$ary) == 0)
		{
			delete $_-> {$item};
		}
	}
	continue
	{
		$chunk++;
	}
}

package Prima::IniFile::Section::Helper::to::Tie;

sub TIEHASH
{
	my ($class, $ini, $section) = @_;
	my $self = {
		ini     => $ini,
		section => $section,
	};
	bless( $self, $class);
	return $self;
}

sub DESTROY
{
}

sub FETCH
{
	my ($self, $item) = @_;
	my @vals = $self-> {ini}-> get_values($self-> {section}, $item);
	return scalar(@vals) ? ($#vals ? [@vals] : $vals[0]) : undef;
}

sub STORE
{
	my ($self, $item, $val) = @_;
	$self-> {ini}-> replace_values($self-> {section}, $item, ref($val) eq q/ARRAY/ ? @$val : ($val));
}

sub DELETE
{
	my ($self, $item) = @_;
	$self-> {ini}-> replace_values($self-> {section}, $item);
}

sub CLEAR         # Well, dangerous
{
	my $self = $_[0];
	my @items = $self-> {ini}-> items($self-> {section});
	for (@items)
	{
		$self-> {ini}-> replace_values($self-> {section}, $_);
	}
}

sub EXISTS
{
	my ($self, $item) = @_;
	return $self-> {ini}-> nvalues($self-> {section},$item) > 0;
}

sub FIRSTKEY
{
	my $self = $_[0];
	$self-> {iterator} = [$self-> {ini}-> items($self-> {section})];
	return $self-> NEXTKEY;
}

sub NEXTKEY
{
	my $self = $_[0];
	unless ( exists $self-> {iterator} && scalar @{$self-> {iterator}})
	{
		return wantarray ? () : undef;
	}
	my $key = shift @{$self-> {iterator}};
	return wantarray ? ($key, $self-> FETCH($key)) : $key;
}

package Prima::IniFile;

sub section
{
	my %tied;
	tie %tied, q/Prima::IniFile::Section::Helper::to::Tie/, $_[0], $_[1];
	return \%tied;
}

sub write
{
	my $self = $_[0];
	return unless defined($self-> {fileName}) && $self-> {changed};
	my $fname = $self-> {fileName};
	eval {
		my $f;
		open $f, ">", $fname or die "Cannot write to the $fname: $!\n";
		binmode $f, ":utf8";
		pop @{$self-> {chunks}-> [-1]} if scalar(@{$self-> {chunks}-> [-1]}) && $self-> {chunks}-> [-1]-> [-1] =~ /^\s*$/;
		for ( @{$self-> {chunks}})
		{
			for (@$_) { print $f "$_\n" }
		}
		push( @{$self-> {chunks}-> [-1]}, '') if scalar(@{$self-> {chunks}-> [-1]}) && $self-> {chunks}-> [-1]-> [-1] !~ /^\s*$/;
		close $f;
	};
	$self-> {changed} = undef if $@;
	warn($@) if $@;
}

1;

=pod

=head1 NAME

Prima::IniFile - support of Windows-like initialization files

=head1 DESCRIPTION

The module provides mapping of a text initialization file to a two-level hash
structure. The first level is I<sections>, which groups the second level
hashes, I<items>.  Sections must have unique keys. The values of the I<items> hashes are
arrays of text strings. The methods that operate on these arrays are
L<get_values>, L<set_values>, L<add_values>, and L<replace_values>.

=head1 SYNOPSIS

	use Prima::IniFile;

	my $ini = create Prima::IniFile;
	my $ini = create Prima::IniFile FILENAME;
	my $ini = create Prima::IniFile FILENAME,
					default => HASHREF_OR_ARRAYREF;
	my $ini = create Prima::IniFile file => FILENAME,
					default => HASHREF_OR_ARRAYREF;

	my @sections = $ini->sections;
	my @items = $ini->items(SECTION);
	my @items = $ini->items(SECTION, 1);
	my @items = $ini->items(SECTION, all => 1);

	my $value = $ini-> get_values(SECTION, ITEM);
	my @vals = $ini-> get_values(SECTION, ITEM);
	my $nvals = $ini-> nvalues(SECTION, ITEM);

	$ini-> set_values(SECTION, ITEM, LIST);
	$ini-> add_values(SECTION, ITEM, LIST);
	$ini-> replace_values(SECTION, ITEM, LIST);

	$ini-> write;
	$ini-> clean;
	$ini-> read( FILENAME);
	$ini-> read( FILENAME, default => HASHREF_OR_ARRAYREF);

	my $sec = $ini->section(SECTION);
	$sec->{ITEM} = VALUE;
	my $val = $sec->{ITEM};
	delete $sec->{ITEM};
	my %everything = %$sec;
	%$sec = ();
	for ( keys %$sec) { ... }
	while ( my ($k,$v) = each %$sec) { ... }

=head1 METHODS

=over

=item add_values SECTION, ITEM, @LIST

Adds LIST of string values to the ITEM in SECTION.

=item clean

Cleans all internal data in the object, including the name of the file.

=item create PROFILE

Creates an instance of the class. The PROFILE is treated partly as an array,
and partly as a hash. If PROFILE consists of a single item, the item is treated as
a filename. Otherwise, PROFILE is treated as a hash, where the following keys
are allowed:

=over

=item file FILENAME

Selects the name of the file.

=item default %VALUES

Selects the initial values for the file, where VALUES is a two-level hash of
sections and items. It is passed to L<read>, where it is merged with the file
data.

=back

=item get_values SECTION, ITEM

Returns an array of values for ITEM in SECTION. If called in scalar context
and there is more than one value, the first value in the list is returned.

=item items SECTION [ HINTS ]

Returns items in SECTION. HINTS parameters are used to tell if a multiple-valued
item must be returned as several items of the same name;
HINTS can be supplied in the following forms:

items( $section, 1 )
items( $section, all => 1);

=item new PROFILE

Same as L<create>.

=item nvalues SECTION, ITEM

Returns the number of values in ITEM in SECTION.

=item read FILENAME, %PROFILE

Flushes the old content and opens a new file. FILENAME is a text string, PROFILE
is a two-level hash of default values for the new file. PROFILE is merged with
the data from the file, and the latter keeps the precedence.  Does not return
any success values but warns if any error is occurred.

=item replace_values SECTION, ITEM, @VALUES

Removes all values from ITEM in SECTION and assigns it to the new
list of VALUES.

=item section SECTION

Returns a tied hash for SECTION. All its read and write operations are reflected
in the caller object which allows the following syntax:

	my $section = $inifile-> section( 'Sample section');
	$section-> {Item1} = 'Value1';

which is identical to

	$inifile-> set_items( 'Sample section', 'Item1', 'Value1');

=item sections

Returns an array of section names.

=item set_values SECTION, ITEM, @VALUES

Assigns VALUES to ITEM in SECTION. If the number of new values is equal to or greater
than the number of the old, the method is the same as L<replace_values>. Otherwise,
the values with indices higher than the number of new values are not touched.

=item write

Rewrites the file with the object content. The object keeps an internal
modification flag C<{changed}>; in case it is C<undef>, no actual write is
performed.

=back

=head1 AUTHORS

Anton Berezin, E<lt>tobez@plab.ku.dkE<gt>

Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>

=cut

