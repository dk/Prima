# $Id$

# extensive testing of raster operations:
# - Image::put_image ( img/put.c ) outside begin_paint/end_paint
# - Image::put_image ( platform-specific) inside begin_paint/end_paint
# - Image::map ( Image.c )

my @alu = qw(
   Blackness
   NotOr
   NotSrcAnd
   NotPut
   NotDestAnd
   Invert
   XorPut
   NotAnd
   AndPut
   NotXor
   NoOper
   NotSrcOr
   CopyPut
   NotDestOr
   OrPut
   Whiteness
);

print "1..48 ", join(',', 
	( map { "image $_" } @alu ), 
	( map { "gui $_" } @alu ), 
	( map { "map $_" } @alu ), 
);
	
my $src = Prima::Image-> create( 
	width        => 1,
	height       => 1,
	type         => im::BW,
	conversion   => ict::None,
	preserveType => 1,
);
my $dst = $src-> dup;

my @table = ( 
	[0,0,0],
	[0,0xffffff,1],
	[0xffffff,0,2],
	[0xffffff,0xffffff,3],
);

AGAIN:
for ( my $i = 0; $i < @alu; $i++) {
	my $res = 0;
	for ( @table) {
		my ($s, $d, $p) = @$_;
		$src-> pixel( 0, 0, $s);
		$dst-> pixel( 0, 0, $d);
		$dst-> put_image( 0, 0, $src, $rop::{$alu[$i]}->());
		$res |= ( $dst-> pixel(0, 0) & 1) << $p;
	}
	ok( $res == $i);
}

unless ( $src-> get_paint_state) {
	$src-> begin_paint;
	$dst-> begin_paint;
	goto AGAIN;
} else {
	$src-> end_paint;
	$dst-> end_paint;
}

# third stage, with map
$src-> type( im::Byte);
$dst-> type( im::Byte);
for ( my $i = 0; $i < @alu; $i++) {
	my $res = 0;
	for ( @table) {
		my ($s, $d, $p) = @$_;
		$src-> pixel( 0, 0, $d);
		my $g = $src-> pixel(0,0);
		$src-> set(
			color     => $s,
			backColor => $s,
			rop       => $rop::{$alu[$i]}->(),
			rop2      => $rop::{$alu[$i]}->(),
		);
		$src-> map( 0);
		$res |= ( $src-> pixel(0, 0) & 1) << $p;
	}
	ok( $res == $i);
}


1;
