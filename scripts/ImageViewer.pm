use ScrollWidget;

package ImageViewer;
use vars qw(@ISA);
@ISA = qw( ScrollWidget);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = (
      image        => undef,
      imageFile    => undef,
      zoom         => 1,
      alignment    => ta::Left,
      valignment   => ta::Bottom,
      quality      => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   if ( defined $p->{imageFile} && !defined $p->{image})
   {
      $p->{image} = Image-> create;
      delete $p->{image} unless $p->{image}-> load($p->{imageFile});
   }
}


sub init
{
   my $self = shift;
   for ( qw( image ImageFile))
      { $self->{$_} = undef; }
   for ( qw( alignment quality valignment))
      { $self->{$_} = 0; }
   for ( qw( zoom integralScreen integralImage))
      { $self->{$_} = 1; }
   my %profile = $self-> SUPER::init(@_);
   $self-> { imageFile}     = $profile{ imageFile};
   for ( qw( image zoom alignment valignment quality)) {
      $self->$_($profile{$_});
   }
   return %profile;
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size   = $canvas-> size;
   my $clr    = $self-> backColor;
   my $bw     = $self-> {borderWidth};

   unless ( $self-> {image}) {
      $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, $self-> dark3DColor, $self-> light3DColor, $clr);
      return;
   }

   $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, $self-> dark3DColor, $self-> light3DColor) if $bw;
   my @r = $self-> get_active_area( @size);
   $canvas-> clipRect( @r);
   $canvas-> transform( $r[0],$r[1]);
   $canvas-> color( $clr);
   my $imY  = $self->{image}->height;
   my $imX  = $self->{image}->width;
   my $z = $self->{zoom};
   my $imYz = int($imY * $z);
   my $imXz = int($imX * $z);
   my $winY = $size[1] - $bw * 2 - $self->{marginY};
   my $winX = $size[0] - $bw * 2 - $self->{marginX};
   my $deltaY = ($imYz - $winY - $self->{deltaY} > 0) ? $imYz - $winY - $self->{deltaY}:0;
   my ($xa,$ya) = ($self->{alignment}, $self->{valignment});
   my ($iS, $iI) = ($self->{integralScreen}, $self->{integralImage});
   my ( $atx, $aty, $xDest, $yDest);

   if ( $imYz < $winY) {
      if ( $ya == ta::Top) {
         $aty = $winY - $imYz;
      } elsif ( $ya == ta::Center) {
         $aty = ($winY - $imYz)/2;
      } else {
         $aty = 0;
      }
      $canvas-> bar( 0, 0, $winX-1, $aty-1) if $aty > 0;
      $canvas-> bar( 0, $aty + $imYz, $winX-1, $winY-1) if $aty + $imYz < $winY;
      $yDest = 0;
   } else {
      $aty   = -($deltaY % $iS);
      $yDest = ($deltaY + $aty) / $iS * $iI;
      $imYz = int(($winY - $aty + $iS - 1) / $iS) * $iS;
      $imY = $imYz / $iS * $iI;
   }

   if ( $imXz < $winX) {
      if ( $xa == ta::Right) {
         $atx = $winX - $imXz;
      } elsif ( $xa == ta::Center) {
         $atx = ($winX - $imXz)/2;
      } else {
         $atx = 0;
      }
      $canvas-> bar( 0, $aty, $atx - 1, $aty + $imYz - 1) if $atx > 0;
      $canvas-> bar( $atx + $imXz, $aty, $winX - 1, $aty + $imYz - 1) if $atx + $imXz < $winX;
      $xDest = 0;
   } else {
      $atx   = -($self->{deltaX} % $iS);
      $xDest = ($self->{deltaX} + $atx) / $iS * $iI;
      $imXz = int(($winX - $atx + $iS - 1) / $iS) * $iS;
      $imX = $imXz / $iS * $iI;
   }

   $canvas-> put_image_indirect(
      $self->{image},
      $atx, $aty,
      $xDest, $yDest,
      $imXz, $imYz, $imX, $imY,
      rop::CopyPut
   );
}

sub set_alignment
{
   $_[0]->{alignment} = $_[1];
   $_[0]->repaint;
}

sub set_valignment
{
   $_[0]->{valignment} = $_[1];
   $_[0]->repaint;
}

sub set_image
{
   my ( $self, $img) = @_;
   $self->{image} = $img;
   unless ( defined $img) {
      $self->limits(0,0);
      $self->palette([]);
      return;
   }
   my ( $x, $y) = ($img-> width, $img-> height);
   $x *= $self->{zoom};
   $y *= $self->{zoom};
   $self-> limits($x,$y);
   $self-> palette( $img->palette) if $self->{quality};
   $self-> repaint;
}

sub set_image_file
{
   my ($self,$file,$img) = @_;
   $img = Image-> create;
   return unless $img-> load($file);
   $self->{imageFile} = $file;
   $self->image($img);
}

sub set_quality
{
   my ( $self, $quality) = @_;
   return if $quality == $self->{quality};
   $self->{quality} = $quality;
   return unless defined $self->{image};
   $self-> palette( $quality ? $self->{image}-> palette : []);
   $self-> repaint;
}

sub set_zoom
{
   my ( $self, $zoom) = @_;

   $zoom = 100 if $zoom > 100;
   $zoom = 1 if $zoom <= 0.01;

   my $dv = int( 100 * ( $zoom - int( $zoom)) + 0.5);
   $dv-- if ($dv % 2) and ( $dv % 5);
   $zoom = int($zoom) + $dv / 100;
   my ($r,$n,$m) = (1,100,$dv);
   while(1) {
      $r = $m % $n;
      last unless $r;
      ($m,$n) = ($n,$r);
   }

   return if $zoom == $self->{zoom};
   $self->{zoom} = $zoom;
   $self->{integralScreen} = int( 100 / $n) * int( $zoom) + int( $dv / $n);
   $self->{integralImage}  = int( 100 / $n);

   return unless defined $self->{image};
   my ( $x, $y) = ($self->{image}-> width, $self-> {image}->height);
   $x *= $self->{zoom};
   $y *= $self->{zoom};

   $self-> limits($x,$y);
   $self-> repaint;
   $self->{hScrollBar}->set_steps( $zoom, $zoom * 10) if $self->{hScroll};
   $self->{vScrollBar}->set_steps( $zoom, $zoom * 10) if $self->{vScroll};
}


sub alignment    {($#_)?($_[0]->set_alignment(    $_[1]))               :return $_[0]->{alignment}    }
sub valignment   {($#_)?($_[0]->set_valignment(    $_[1]))              :return $_[0]->{valignment}   }
sub image        {($#_)?$_[0]->set_image($_[1]):return $_[0]->{image} }
sub imageFile    {($#_)?$_[0]->set_image_file($_[1]):return $_[0]->{imageFile}}
sub zoom         {($#_)?$_[0]->set_zoom($_[1]):return $_[0]->{zoom}}
sub quality      {($#_)?$_[0]->set_quality($_[1]):return $_[0]->{quality}}

1;
