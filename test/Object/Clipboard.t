# $Id$
print "1..6 alive,predefined formats,text,image,user-defined format,clear\n";

my $c = $::application-> Clipboard;
ok( $c && $c-> alive);

my %rc = map { $_ => 1 } $c-> get_registered_formats;
ok( exists $rc{'Text'} && exists $rc{'Image'});

$c-> store( "Text", 'jabba dabba du');
my $res = $c-> fetch( 'Text');
my %fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Text} && defined $res && $res eq 'jabba dabba du');

my $i = Prima::Image-> create( width => 32, height => 32);
$c-> store( "Image", $i);
$i = $c-> fetch( 'Image');
%fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Image} && defined $i && $i-> alive && $i-> width == 32 && $i-> height == 32);
$i-> destroy if $i;

$c-> register_format("Mumbo-Jumbo");
%rc = map { $_ => 1 } $c-> get_registered_formats;
$c-> store( "Mumbo-Jumbo", pack( 'C*', 0,1,2,3,4,5,6,7,8,9));
$res = $c-> fetch( "Mumbo-Jumbo");
%fm = map { $_ => 1 } $c-> get_formats;
ok(exists $rc{"Mumbo-Jumbo"} && exists $fm{"Mumbo-Jumbo"} && defined $res && $res eq pack( 'C*', 0,1,2,3,4,5,6,7,8,9));
$c-> deregister_format("Mumbo-Jumbo");

$c-> clear;
my @f = $c-> get_formats;
ok( scalar(@f) == 0);

1;
