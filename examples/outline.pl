use Prima::Application;
use Prima::Outlines;

my $w = Prima::Window->create( size => [ 200, 200],
onDestroy=>sub{$::application-> close});
my $o = $w-> insert(
Prima::DirectoryOutline =>
#Outline =>
popupItems => [
  ['Delete this' => sub{
     my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
     $_[0]-> delete_item( $x);
  }],
  ['Insert updir below' => sub{
     my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
     my ( $p, $o) = $_[0]-> get_item_parent( $x);
     $_[0]-> insert_items( $p, $o + 1, [
        ['C:', ''], [], 0
        #'C:', [], 0
     ]);
  }],
  ['Insert updir inside' => sub{
     my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
     $_[0]-> insert_items( $x, 0, [
        ['C:', ''], [], 0
        #'C:', [], 0
     ]);
  }],
  ['Expand this' => sub{
     my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
     $_[0]-> expand_all( $x);
  }],

],
origin => [0,0],
buffered => 1,
hScroll => 1,
size=>[200,200],growMode => gm::Client,
onSelectItem => sub {
   my ($self, $index) = @_;
   #print $self-> path."\n";
},
items => [
   ['Single string'],
   ['Reference', [
      ['Str1'],
   ]],
   ['Single string'],
   ['Another single string', undef],
   ['Empty reference', []],
   ['Reference', [
      ['Str1 -------------------------------------------------'],
      ['Str2'],
      ['Str3', [
         ['Subref1'],
         ['Subref2'],
      ]]],
   ],
   ['XXL'],
]);
my ( $i, $l) = $o-> get_item( 1);
#$o-> expand_all;
#$o-> path('e:\Prima');

run Prima;


1;
