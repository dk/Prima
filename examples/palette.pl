use Prima;
use Prima::Const;
use Prima::Classes;

my $useImages = 0;

$::application = Prima::Application-> create( name => "Tester");


my $j = $::application;

my $i;
my @spal = ();
for ( $i = 0; $i < 33; $i++) {
   push( @spal, $i * 8, 0, 0);
};


my $aimg = Prima::Image-> create();
my $bimg = Prima::Image-> create();
my $cimg = Prima::Image-> create();

if ( $useImages) {
  $aimg-> load('winnt256.bmp');
  $bimg-> load('c:/home/tobez/aaa.tif');
  $cimg-> load('baba.bmp'); $cimg-> type( im::bpp8);
}

@spal = (@{$aimg->palette}, @{$bimg->palette}, @{$cimg->palette}) if $useImages;

my $w = Prima::Window->create(
   onDestroy => sub {$::application-> close},
   size    => [ 100, 33 * 8],
   palette => [ @spal],
   buffered => 1,
   onPaint => sub {
      my $self = $_[0];
      my $i;
      $self-> on_paint( $_[1]);
      if ( $useImages) {
         $self-> put_image( 0, 0, $bimg);
         $self-> put_image( 100, 100, $aimg);
         $self-> put_image( 400, 300, $cimg);
      } else {
         for ( $i = 0; $i < 33; $i++) {
            my $x = $i * 8;
            $x = $x > 255 ? 255 : $x;
            $self-> color( $self-> get_nearest_color( $x));
            $self-> bar( 0, $i * 8, 3000, $i * 8 + 8);
         }
         my @pal = @{$self-> get_physical_palette};
         for ( $i = 0; $i < scalar @pal / 3; $i++) {
            my $col = ( ($pal[$i*3]) | ($pal[$i*3+1] << 8) | ($pal[$i*3+2] << 16));
            $self-> color( $col);
            $self-> bar( 0, $i * 8, 3000, $i * 8 + 8);
         }
      }
   },
);

run Prima;

