#!/usr/bin/env perl
use strict;
use warnings;
use Getopt::Long;
use Prima::noX11;
use Prima::noARGV;
use Prima qw(Drawable::Pod PS::Printer);

my %opt = (
	help  => 0,
	font  => ($^O =~ /win32/i) ? 'Arial' : 'Helvetica',
	mono  => ($^O =~ /win32/i) ? 'Courier New' : 'Courier',
	size  => 10,
);

sub usage
{
	print <<USAGE;

$0 - convert pod file to a pdf document

format: prima-pod2pdf [options] input.pod [output.pdf|-]

options:
	--help|-h
	--size|-s INT  - default font size ($opt{size})
	--font|-f FONT - default font ($opt{font})
	--mono|-m FONT - monospaced font ($opt{mono})

USAGE
	exit 1;
}

GetOptions(\%opt,
	"help|h",
	"size|s=i",
	"font|f=s",
	"mono|m=s",
) or usage;

usage if $opt{help};
usage if @ARGV < 1 || @ARGV > 2;
my ( $input, $output ) = @ARGV;
unless (defined $output) {
	$input =~ m/(.*)\.(pod|pm|pl)$/i;
	$output = ($1 // $input) . '.pdf';
}

my ($fin, $fout);
if ( $input eq '-') {
	$fin = \&STDIN;
} elsif ( !open $fin, "<", $input ) {
	die "Cannot open $input:$!\n";
}

my $pod = Prima::Drawable::Pod->new;
$pod->manpath($1) if $input =~ m/^(.*)[\\\/]([^\\\/]+)/;
$pod->style( pod::STYLE_CODE, backColor => undef);
$pod->open_read( create_index => 1 );
$pod->read($_) while <$fin>;
die "Error reading from $input:$!\n" if $!;
$pod->close_read;
close $fin;

if ( $output eq '-') {
	$fout = \&STDOUT;
} elsif ( !open $fout, ">", $output ) {
	die "Cannot open $output:$!\n";
}

my @font_palette = (
	{
		name     => $opt{font},
		encoding => '',
		pitch    => fp::Default,
	},{
		name     => $opt{mono},
		encoding => '',
		pitch    => fp::Fixed,
	}
);

my $ps = Prima::PS::PDF::FileHandle->new( handle => $fout );
$ps->begin_doc or die "Error printing to $output\n";
my $ok = $pod->print(
	canvas   => $ps,
	fontmap  => \@font_palette,
	justify  => 1,
);
if ( $ok ) {
	$ps->end_doc or die "Printing failed:$@\n";
	close $fout or die "Error printing to $output: $!\n";
} else {
	$ps->abort_doc;
	die "Printing failed\n";
}

=pod

=head1 NAME

prima-pod2pdf - convert pod file to a pdf document

=head1 SYNOPSIS

format: prima-pod2pdf [options] input.pod [output.pdf|-]

=head1 SEE ALSO

L<Prima::Drawable::Pod>, L<Prima::PS::PDF>.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.
