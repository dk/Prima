=pod

=head1 NAME

examples/tiger.pl - draw an eps file

=head1 FEATURES

Demonstrates capabilities of C<Prima::Drawable::Path> - curves, lines, and
matrix transformations, similar to those in the PostScript language. The PS
interpreter is very dumb and minimal, to parse the F<tiger.eps> file only.

=cut


use strict;
use warnings;
use FindBin qw($Bin);
use Prima qw(Application);

my ( $device, %globals, %sym, %def, $line, @stack, @code_stack, @array_stack, @graphics_stack);
my $debug = 0;

use constant CLASS => 0;
use constant VALUE => 1;

$SIG{__DIE__} = sub {
	print "stack:\n";
	print "* @$_\n" for @stack;
	die @_;
};

sub ntok($$)  { [ @_ ] }
sub spush($)  { push @stack, $_[0] }
sub spushn($) { spush ntok num => shift }
sub spop      { pop @stack or die "stack undeflow\n" }
sub spopx($)
{
	my ($pop, $class) = (spop, shift);
	die "$class expected\n" unless $pop->[CLASS] eq $class;
	return $pop->[VALUE];
}
sub spopn  { spopx 'num' }
sub spopy  { spopx 'sym' }
sub spopc  { spopx 'code' }
sub spopnx($) { reverse ( map { spopn } 1 .. $_[0] ) }

sub gc { $graphics_stack[-1] }
sub newpath {
	my $m = gc->{path}->last_matrix;
	gc->{path} = $device->new_path;
	gc->{path}->matrix( @$m ) if $m;
	gc->{path_actual} = 0;
}
sub path    { gc->{path}  }

sub execute
{
	my $code = shift;
	for my $t ( @$code ) {
		if ( $debug ) {
			my $ns = @stack;
			print STDERR "$ns:$t->[CLASS] $t->[VALUE]\n";
			if ( $debug > 1 ) {
				for ( @stack ) {
					print STDERR "  * $_->[CLASS] $_->[VALUE]\n";
				}
			}
		}
		if ( $t->[CLASS] =~ /^(id|num|sym|code|array|dict)/) {
			push @stack, $t;
		} elsif ( $t->[CLASS] eq 'fun') {
			my $m = $t->[VALUE];
			main->can($m)->();
		} elsif ( $t->[CLASS] eq 'call' ) {
			my $d = $def{ $t->[VALUE] };
			die "$t->[VALUE] is undefined\n" unless $d;
			execute($d->[VALUE]);
		} else {
			die "Unknown opcode $t->[CLASS]\n";
		}
	}
}

sub __log { use Data::Dumper; print STDERR Dumper(spop) }

sub _add { spushn spopn + spopn }
sub _begin { }
sub _bind { }
sub _cleartomark { }
sub _closepath { path->close }
sub _copy {
	my $n = spopn;
	return if $n < 1;
	push @stack, ((splice( @stack, -$n )) x 2);
}
sub _countdictstack { spushn  0 }
sub _currentflat { spushn 24 }
sub _currentpoint {
	spushn gc->{point}->[0];
	spushn gc->{point}->[1];
}
sub _curveto {
	gc->{path_actual} = 1;
	my @p = spopnx 6;
	path->spline( [ @{ gc->{point} }, @p ], degree => 3 );
	gc->{point} = [ @p[4,5] ];
}
sub _def {
	my $v = spop;
	$v = ntok code => [$v] unless $v->[CLASS] eq 'code';
	$def{spopy()} = $v;
}
sub _dict { spopn; spush ntok dict => {} }
sub _dup { my $p = spop; spush $p; spush $p }
sub _end { }
sub _eq {
	my ( $y, $x ) = ( spop, spop );
	spushn $x->[CLASS] eq $x->[CLASS] && $x->[VALUE] eq $y->[VALUE];
}
sub _exch { push @stack, spop, spop }
sub _fill { path-> fill if gc->{path_actual}; newpath }
sub _get { spush ntok fun => "_" . spopy }
sub _grestore {
	pop @graphics_stack;
	$device->lineWidth(gc->{lw});
	$device->color(gc->{color});
}
sub _gsave {
	my %last = %{ gc() };
	$last{path} = $last{path}->dup;
	$last{point} = [ @{$last{point}} ];
	push @graphics_stack, \%last;
}
sub _if {
	my ( $code, $bool ) = ( spopc, spopn );
	execute($code) if $bool;
}
sub _ifelse {
	my ( $code2, $code1, $bool ) = ( spopc, spopc, spopn );
	execute($bool ? $code1 : $code2);
}
sub _index { spush ($stack[ spopn ] or die "No such value at index") }
sub _itransform { }
sub _lineto {
	unless ( gc->{path_actual}) {
		path->line( @{ gc->{point} });
		gc->{path_actual} = 1;
	}
	path->line( @{gc->{point}} = spopnx 2 );
}
sub _load {
	my $arg = spopy;
	return spush $def{$arg} if exists $def{$arg};
	return spush ntok fun => "_$arg" if main->can("_$arg");
	die "No such symbol: $arg\n";
}
sub _lt { spushn( spopn() >= spopn()) }
sub _mark { }
sub _moveto {
	newpath if gc->{path_actual};
	gc->{point} = [spopnx 2];
}
sub _neg { spushn - spopn }
sub _newpath { newpath }
sub _pop { spop }
sub _repeat { my ($c, $n) = (spopc, spopn); execute($c) for 1..$n }
sub _restore { }
sub _roll {
	my ( $n, $j ) = spopnx 2;
	return unless $j;
	my @r = splice( @stack, -$n );
	if ( $j < 0 ) {
		push @r, shift @r for 1 .. -$j;
	} else {
		unshift @r, pop @r for 1 .. $j;
	}
	push @stack, @r;
}
sub _round { spushn int spopn }
sub _save { }
sub _scale { path->scale(spopnx 2) }
sub _setdash { spop, spop }
sub _setflat { spopn }
sub _setgray {$device->color(gc->{color} = int( 255 * spopn ) * ( 65536 + 256 + 1 )) }
sub _setmiterlimit { spopn }
sub _setlinecap { spopn }
sub _setlinejoin { spopn }
sub _setlinewidth { $device->lineWidth( gc->{lw} = spopn )  }
sub _setcmybcolor {
	my @k = map { 1 - spopn } 0 .. 3;
	my @c = map { int 255 * $k[0] * $_ } @k[1..3];
	$device->color(gc->{color} = ($c[2] << 16) | ($c[1] << 8) | $c[0]);
}
sub _setrgbcolor {
	my @c = map { int( 255 * spopn ) } 0 .. 2;
	$device->color(gc->{color} = ($c[2] << 16) | ($c[1] << 8) | $c[0]);
}
sub _showpage { }
sub _stroke { path->stroke if gc->{path_actual}; newpath }
sub _sub { spushn( spopn - spopn )}
sub _transform { }
sub _translate { path->translate(spopnx 2) }
sub _where { spushn(main->can('_' . spopy) // 0) }

sub tok
{{
		if ( m/\G([_a-z][\.\w]*)/gcsi ) {
			return ntok call => $1 if exists $sym{$1};
			return ntok fun  => "_$1" if main->can("_$1");
			die "Unknown identifier: $1 at line $line\n";
		}
		m/\G([+-]?(?:\.\d+|\d+(?:\.\d+)?))/gcs and return ntok num        => $1;
		m/\G\/([\.\w]+)/gcs                    and return $sym{$1} = ntok sym => $1;
		m/\G\{/gcs                             and return ntok code_begin => undef;
		m/\G\}/gcs                             and return ntok code_end   => undef;
		m/\G\[/gcs                             and return ntok arr_begin  => undef;
		m/\G\]/gcs                             and return ntok arr_end    => undef;
		m/\G\s+/gcs and redo;
		m/\G$/gcs and return;
		m/\G(.*)/gcs;
		die "Unknown token: $1 at line $line\n";
}}

# parse
my $f = "$Bin/tiger.eps";
open F, "<", $f or die "cannot open $f:$!\n";
$line = 0;
push @code_stack, [];
while (<F>) {
	$line++;
	chomp;
	if ( m/^%(.)(.*)/ ) {
		my ( $s, $t ) = ( $1, $2 );
		if ( $s eq '%' ) {
			my $v;
			($t, $v) = ($1, $2) if $t =~ /(.*)\s*\:\s*(.*)/;
			$globals{$t} = $v;
		}
		next;
	}
	s/^\s+//;
	s/\s*$//;
	next unless length;
	while ( my $t = tok ) {
		if ( $t->[CLASS] =~ /^(id|num|call|fun|sym)/) {
			push @{ $code_stack[-1] }, $t;
		} elsif ( $t->[CLASS] eq 'code_begin' ) {
			push @code_stack, [];
		} elsif ( $t->[CLASS] eq 'code_end' ) {
			@code_stack > 1 or die "Unexpected } at line $line\n";
			my $nt = ntok code => pop(@code_stack);
			push @{ $code_stack[-1] }, $nt;
		} elsif ( $t->[CLASS] eq 'arr_begin' ) {
			push @code_stack, [];
			push @array_stack, 1;
		} elsif ( $t->[CLASS] eq 'arr_end' ) {
			pop @array_stack or die "Unexpected ] at line $line\n";
			push @{ $code_stack[-1] }, ntok array => pop(@code_stack);
		} else {
			die "Unknown token $t->[CLASS] at line $line\n";
		}
	}
}
close F;
die "Unclosed array\n" if @array_stack;
die "Unclosed code\n" if 1 < @code_stack;
die "No code\n" unless @code_stack;

# execute
my @bb = split ' ', ($globals{BoundingBox} // '0 0 1000 1000');

#$device = Prima::DeviceBitmap->new(
#	type  => dbt::Pixmap,
#	width  => $bb[2] - $bb[0],
#	height => $bb[3] - $bb[1],
#	backColor => cl::White,
#	color => cl::Black,
#);
#$device->clear;
#$device->translate( -$bb[0], -$bb[1]);
#execute( $code_stack[0] );

sub init
{
	@graphics_stack = ({
		color       => 0,
		point       => [0,0],
		path        => $device->new_path,
		path_actual => 0,
		lw          => 0,
	});
	@stack = ();
}

my $w = Prima::MainWindow->new(
	text => 'Tiger',
	backColor => cl::White,
	color => cl::Black,
	antialias => (($ARGV[0] // '') eq 'aa'),
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		$canvas->clear;

		my @sz = $canvas-> size;
		my @ps = ( $bb[2] - $bb[0], $bb[3] - $bb[1] );
		my @ratios = map { $sz[$_] / $ps[$_] } 0, 1;
		my ( $dx, $dy ) = (0,0);
		my $r;
		if ( $ratios[0] < $ratios[1] ) {
			$r = $ratios[0];
			$dy = ( $sz[1] - $ps[1] * $r ) / 2;
		} else {
			$r = $ratios[1];
			$dx = ( $sz[0] - $ps[0] * $r ) / 2;
		}

		$device = $canvas;
		$device->translate( $dx - $r * $bb[0], $dy - $r * $bb[1]);

		init;
		path->scale($r);
		execute( $code_stack[0] );
	}
);

Prima->run;
