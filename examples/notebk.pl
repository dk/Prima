use Prima;
use Prima::Buttons;
use Prima::Notebooks;

package Bla;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);

   my $n = $self-> insert( TabbedNotebook =>
      origin => [10, 10],
      size => [ $self-> width - 20, $self-> height - 20],
      growMode => gm::Client,
#     pageCount => 11,
      tabs => [0..5,5,5..10],
   );

   $n-> insert_to_page( 0 => 'Button');
   my $j = $n-> insert_to_page( 1 => 'CheckBox' => left => 200);
   $n-> insert_to_page( 2,
      [ Button => origin => [ 0, 0], ],
      [ Button => origin => [ 10, 40], ],
      [ Button => origin => [ 10, 70], ],
      [ Button => origin => [ 10,100], ],
      [ Button => origin => [ 110, 10], ],
      [ Button => origin => [ 110, 40], ],
      [ Button => origin => [ 110, 70], ],
      [ Button => origin => [ 110,100], ],
   );
   return %profile;
}


package Generic;

$::application = Prima::Application-> create( name => "Generic.pm");

my $l;
my $w = Bla-> create(
  size => [ 600, 300],
  y_centered  => 1,
 # current  => 1,
  onDestroy => sub { $::application-> close},
);


run Prima;
