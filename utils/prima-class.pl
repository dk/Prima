#!/usr/bin/perl -w
# dumps hierarchy of widget classes.
#
# Used by podview ( see File/Run/p-class )
#
use strict;
use warnings;

my $glob_path;
my $debug = 0;
my $want_all;
my $want_hier;
my @want_class;
my $otype_pod = 1;
my $ftype_pod = 0;

for ( @ARGV) {
	if ( m/^--help$/ || m/^-h$/) {
		usage();
	} if ( m/^--debug/ || m/^-d$/) {
		$debug = 1;
	} elsif ( m/^--path=(.+)$/) {
		$glob_path = $1;
	} elsif ( m/^--perldoc$/ || m/^-c$/) {
		$ftype_pod = 2;
	} elsif ( m/^--podview$/ || m/^-p$/) {
		$ftype_pod = 1;
	} elsif ( m/^--text$/ || m/^-t$/) {
		$otype_pod = 0;
	} elsif ( m/^--hier$/) {
		$want_hier = 1;
	} elsif ( m/^--all$/) {
		$want_all = 1;
	} elsif ( !m/^-/) {
		$_ = "Prima::$_" unless /^Prima::/;
		push @want_class, $_;
	} else {
		die "Unknown option `$_'\n";
	}
}

die "The '--all' option and explicit classes names cannnot be set together\n"
if $want_all && @want_class;

usage() if !$want_all && !@want_class;

sub usage
{
		print <<HELP;

p-class - generates documentation on Prima classes hierarchy

format:
   p-class [--option] [--option=VALUE] class_name

options:
  --path=PATH    - search Prima installation in the path, instead of \@INC
  [-t|--text]    - output in text format, instead of pod ( default )
  [-d|--debug]   - verbose debug info
  [-h|--help]    - display help
  --all          - dump information for all Prima classes found
  --hier         - produce only hierarchy tree
  [-p|--podview] - run podview
  [-c|--perldoc] - run perldoc

examples:
    p-class -p Edit
    p-class -t --hier Button
    p-class --all --hier -c

HELP
		exit;
}

unless ( $glob_path) {
	for ( '../..', '..', '.', @INC) {
		next unless -f "$_/Prima.pm";
		$glob_path = $_;
		last;
	}
}

die "Cannot find Prima.pm\n" unless defined $glob_path;
print "Using $glob_path as root\n" if $debug;

my %paths = (
	'pod/Prima/*.pod' => { # source tree
		type      => 'pod',
		classes   => 'kernel',
		exclude   => qr/\/([a-z]|X11)[^\s\/]*\.pod$/, # no lowercase
		invariant => 1,
	},
	'Prima/*.pod' => { # installed
		type      => 'pod',
		classes   => 'kernel',
		exclude   => qr/\/([a-z]|X11)[^\s\/]*\.pod$/, # no lowercase
		invariant => 1,
	},
	'Prima/*.pm' => {
		type     => 'pm',
		classes  => 'user',
		exclude  => qr/\b(Classes|Application|Make|Themes|Tie|Const|IniFile|noX11|StdBitmap|Stress|Utils|StartupWindow|Config|EventHook|MsgBox|Utils|Gencls)\.pm$/,
	},
	'Prima/Classes.pm' => {
		type     => 'pm',
		classes  => 'kernel',
	},
	'Prima/Application.pm' => {
		type     => 'pm',
		classes  => 'kernel',
	},
	'Prima/PS/*.pm' => {
		type     => 'pm',
		classes  => 'user',
		exclude  => qr/(Setup|Glyphs|Unicode)\.pm$/,
	},
);

# the script deduces the property type from the head name, but sometimes fails.
# here are the hints to proper types
my @hints = (
	{
		match    => qr/Prima\/Object.pod\/Events/,
		property => undef,
	},
);

my ( $pod_root, @itemgroups, @stack, %invariants);
my (%ascendants, %class_priority, %all_items, %pods);

sub new_entry
{
	my $entry = { @_, children => [] };
	$entry->{path} = join('/', map { $_->{topic}} @stack);
	$entry->{pod_root} = $pod_root;
	push @itemgroups, $entry;
	$entry;
}

# load pod content from files
while ( my ($path, $path_hints) = each %paths) {

	# check invariant paths
	next if $path_hints->{invariant} && $invariants{$path_hints->{invariant}};
	my @glob = glob "$glob_path/$path";
	next unless @glob;
	$invariants{$path_hints->{invariant}} = 1 if $path_hints->{invariant};

	for ( @glob) {
		next if $path_hints->{exclude} && m/$path_hints->{exclude}/;
		my $filename = $_;
		open F, $filename or die "Cannot open $filename:$!\n";

		print "FILE $filename\n" if $debug;
		my $root = {
			type     => 'pod',
			topic    => $filename,
			children => [],
			path     => $filename,
		};
		my $cap_name = 0;
		$pod_root = $filename;
		$pod_root =~ s/^.*?(Prima)/$1/;
		$pod_root =~ s/\//::/g;
		$pod_root =~ s/\.[\w]+$//;
		my $class_priority = (( $path_hints->{classes} eq 'kernel' ) ? 1 : 0);

		@stack = ($root);
		my $over = 0;
		@itemgroups = ($root);
		my $last_package;

		while (<F>) {
			if ( $path_hints->{type} ne 'pod') {
				unless ( m/^=(pod|head)/ .. m/^=cut/) {
					if ( m/package (Prima::.*);/) {
						$last_package = $1;
					} elsif ( defined $last_package && m/\@ISA\s*=\s*qw\s*\(([^\)]*)\)/) {
						$ascendants{$last_package} = [ grep { /^Prima/} split ' ', $1];
						$class_priority{$last_package} = $class_priority;
						print "=> $path_hints->{classes} $last_package inherits @{$ascendants{$last_package}}\n"
							if $debug;
					}
					next;
				}
			}

			# store pod commands in a hierarchy
			my ($head,$topic,$parent,$entry); # any entry created?
			if ( m/^=(\S+)\s*(.*?)\s*$/) {
				( $head, $topic) = ( $1, $2);
				# print "$1 $2\n";
				if ( $head eq 'head1' && $topic eq 'NAME') {
					$cap_name = 1;
					next;
				}

				if ( $head eq 'head1') {
					$entry = new_entry( type => 'head1', topic => $topic );
					$parent = $root;
					@stack = ($root, $entry);
				} elsif ( $head eq 'head2') {
					pop @stack while @stack && $stack[-1]->{type} !~ /head1|pod/;
					$entry = new_entry( type => 'head2', topic => $topic);
					$parent = $stack[-1];
					push @stack, $entry;
				} elsif ( $head eq 'over') {
					$parent = $stack[-1];
					$entry = new_entry( type => 'over', topic => 'over', depth => $over++);
					push @stack, $entry;
				} elsif ( $head eq 'back') {
					$over--;
					pop @stack;
				} elsif ( $head eq 'item') {
					push @{$stack[-1]->{children}}, $topic;
				} elsif ( $head =~ m/for|cut|pod/ ) {
				} else {
					warn "unknown pod directive '$head'\n";
				}
			} else {
				# extract the full name from =head1 NAME
				if ( $cap_name) {
					next unless m/^\S+/m;
					chomp;
					$cap_name = 0;

					$entry = new_entry( type => 'head1', topic => $topic = $_, root_class => 1);
					$parent = $root;
				}
			}

			# check various dependencies in $entry
			if ( $entry) {
				# hierarchy
				push @{$parent->{children}}, $entry;

				# property
				if ( $topic =~ /(method)|(propert)|(event)/oi) {
					$entry->{property} = ( $1 ? 'Methods' : ( $2 ? 'Properties' : 'Events'));
				} elsif ( defined $parent->{property}) {
					$entry->{property} = $parent->{property}
				}

				# classes
				if ( $topic =~ /(Prima::[\w\d_\:]+)/) {
					$entry->{class} = $1;
					$pods{$1} = $pod_root;
				} elsif ( defined $parent->{class}) {
					$entry->{class} = $parent->{class};
					$pods{$entry->{class}} = $pod_root;
				}
				if ( $entry->{class} && $entry->{root_class}) {
					$parent->{class} = $entry->{class}; # for =head1 NAME
				}

				# apply hints
				for my $hint ( @hints) {
					if ( $entry->{path} =~ /$hint->{match}/) {
						$entry->{property} = $hint->{property} if exists $hint->{property};
					}
				}
			}
		}
		close F;
		# pod stream parse over - now parse dom

		# run
		for ( @itemgroups) {
			my $i = $_;
			my ( $prop, $class, $d_prop);
			if ( $debug) {
				print "$i->{path} $i->{topic}\n";
				$d_prop  = $i->{property} || '??';
				$class   = $i->{class} || '**';
				$d_prop  = '--' if $i->{type} eq 'over' && $i->{depth} > 0;
			} else {
				next if !defined $i->{property} || !defined $i->{class} ||
								($i->{type} eq 'over' && $i->{depth} > 0);
				$class = $i->{class};
			}
			$prop = $i->{property};

			for (@{$_->{children}}) {
				next if ref($_) eq 'HASH';
				if ( $otype_pod) {
					s/</\0xff/g;
					s/>/\0xfe/g;
					s/\0xff/E<lt>/g;
					s/\0xfe/E<gt>/g;
				}
				my $topic = $_;
				s/\s.*$//;
				my $link = $_;
				print " $d_prop  $class\:\:$topic => $pod_root/$link\n" if $debug;
				push @{$all_items{$class}->{$prop}}, [ $topic, $pod_root, $link ]
					if defined $prop; # just when debugging
				$pods{$class} = $pod_root;
			}
		}
	}
}

# inheritance tree
my %descendants;
while ( my ( $class, $inh) = each %ascendants) {
	print "$class => @$inh\n" if $debug;
	for ( @$inh) {
		push @{$descendants{$_}}, $class;
	}
}

# hacks hacks!
$class_priority{'Prima::Object'} = 2;
$class_priority{'Prima::Widget'} = 1;
$pods{'Prima::AbstractMenu'} = $pods{'Prima::Menu'};
$pods{'Prima::Dialog::ReplaceDialog'} = $pods{'Prima::Dialog::FindDialog'};

my $prio = 3;
my %processed_classes;

for ( keys %descendants) {
	$class_priority{$_} = -1 unless defined $class_priority{$_}; # roots except Prima::Object
}

my $header;
my $links_body;# = ( $otype_pod ? "=head1 HIERARCHY\n\n" : '');
my @classes;

if ( @want_class) {
	for ( @want_class) {
		if ( $all_items{$_} || $descendants{$_} || $ascendants{$_}) {
			$header = "$_ - hierarchy";
			push @classes, $_;
		} else {
			print "No information for `$_'\n";
			exit;
		}
	}
} else {
	$header = "Prima - hierarchy of Prima classes";
	while ( $prio-- >= 0) {
		for ( grep { $class_priority{$_} == $prio } keys %descendants) {
			my @big_class_list = ($_);
			while ( $_ = shift @big_class_list) {
				next if $processed_classes{$_};
				next if ($class_priority{$_} < $prio - 1);
				$processed_classes{$_} = 1;
				push @big_class_list, @{$descendants{$_}} if $descendants{$_};
				# print "$_ => @{$descendants{$_}} \n" if $descendants{$_};
				push @classes, $_;
			}
		}
	}
}

sub dump_class
{
	my $class = $_[0];
	my %items;
	my @traverse = ( $class);
	my @all_classes;
	# run inheritance traversal
	print "Traverse $class\n" if $debug;
	$links_body .= "=head1 $class\n\n" if $otype_pod;

	while ( $_ = shift @traverse) {
		push @traverse, @{$ascendants{$_}} if $ascendants{$_};
		push @all_classes, $_;
	}

	$links_body .= ( $otype_pod ? "=head2 Related classes\n\n" : "* Related classes\n\n")
		unless $want_hier;
	for ( reverse @all_classes) {
		my $pod = $pods{$_} ? " in $pods{$_} manpage" : '';
		if ( $otype_pod) {
			$links_body .= ( $pods{$_} ? "L<$_|$pods{$_}/>$pod\n\n" : "$_\n\n");
		} else {
			$links_body .= "    $_$pod\n";
		}
	}

	return if $want_hier;

	for ( @all_classes) {
		my $curr_class = $_;
		print "-> $curr_class\n" if $debug;
		$links_body .= ( $otype_pod ? "=head2 $curr_class\n\n" : "\n\n* $curr_class\n");
		if ( $all_items{$curr_class}) {
			while ( my ( $prop, $items) = each %{$all_items{$curr_class}}) { # e.g. METHOD, EVENT, PROPERTY
				print "  -> $prop\n" if $debug;
				$links_body .= ( $otype_pod ? "B<$prop>\n\n=over 4\n\n" : "\n - $prop\n");
				for ( @$items) {
					my ( $topic, $root, $name) = @$_;
					$items{$prop}->{$name} = "L<$topic|$root/$name>";
					print "    +-> $name\n" if $debug;
					$links_body .=  ( $otype_pod ? $items{$prop}->{$name} . "\n\n" : "    $topic\n");
				}
				$links_body .= "\n\n=back\n\n" if $otype_pod;
			}
		}
	}
}

dump_class($_) for @classes;


my $text;
if ( $otype_pod) {
	$text = "=pod\n\n=head1 NAME\n\n$header\n\n$links_body\n=cut\n\n";
} else {
	$text = "\n$header\n\n$links_body\n";
}

if ( $ftype_pod) {
	my $rname = ( $want_all ? 'prima-classes' : $want_class[0]);
	$rname =~ s/[\\:\/]/_/g;
	my $d = ($ENV{TEMP}?$ENV{TEMP}:'/tmp')."/$rname.$$";
	open F, "> $d" or die "Cannot write $d:$!\n";
	print F $text;
	close F;
	my $proc = ( $ftype_pod == 1 ? 'podview' : 'perldoc');
	system( $proc, $d) == 0 or warn "Error running $proc $d:$?$!\n";
	unlink $d;
} else {
	print $text;
}
