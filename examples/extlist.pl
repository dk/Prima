use Prima;
use Prima::ExtLists;
use Prima::Application;



my $w = Prima::Window->create(
   size => [ 200, 200],
   onDestroy => sub { $_[0]-> owner-> close; },
);
$w-> insert( Prima::CheckList =>
   origin => [0,0],
   size   => [ $w-> size],
   growMode => gm::Client,
   items    => [qw(Tra lala tri rubla hop hey lala ley comin down the judgement day)],
   multiColumn => 1,
   multiSelect => 1,
   vector   => 0x3,
   extendedSelect =>0,
);

run Prima;
