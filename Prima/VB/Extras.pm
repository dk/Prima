use Prima::VB::Classes;

package Prima::VB::Types::lineEnd;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw( Flat Square Round); }
sub packID { 'le'; }

sub open
{
   my $self = shift;
   $self-> SUPER::open( @_);
   $self->{A}-> set(
      bottom => $self->{A}->bottom + 36,
      height => $self->{A}->height - 36,
   );
   $self->{B} = $self->{container}-> insert( Widget =>
      origin => [ 5, 5],
      size   => [ $self->{A}->width, 32],
      growMode => gm::GrowHiX,
      onPaint  => sub {
         my ( $me, $canvas) = @_;
         my @sz = $canvas-> size;
         $canvas-> color( cl::White);
         $canvas-> bar(0,0,@sz);
         $canvas-> lineEnd( $self-> get);
         $canvas-> lineWidth( 14);
         $canvas-> color( cl::Gray);
         $canvas-> line( 14, $sz[1]/2, $sz[0]-14, $sz[1]/2);
         $canvas-> lineWidth( 2);
         $canvas-> color( cl::Black);
         $canvas-> lineEnd( le::Round);
         $canvas-> line( 8, $sz[1]/2, 20, $sz[1]/2);
         $canvas-> line( $sz[0]-20, $sz[1]/2, $sz[0]-8, $sz[1]/2);
         $canvas-> line( 14, $sz[1]/2-6, 14, $sz[1]/2+6);
         $canvas-> line( $sz[0]-14, $sz[1]/2-6, $sz[0]-14, $sz[1]/2+6);
      },
   );
}

sub on_change
{
   my $self = $_[0];
   $self-> {B}-> repaint;
}

1;
