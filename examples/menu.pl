#
#  Example of menu extended usage
#
use strict;
use Prima qw( InputLine Label);

package TestWindow;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

#    Menu item must be an array with up to 5 items in -
# [variable, text or image, accelerator text, shortcut key, sub or command]
# if there are 0 or 1 items, it's treated as divisor line;
# same divisor in main level of menu makes next part right-adjacent.
# The priority is: text(2), then sub(5), then variable(1), then accelerator(3)
# and shortcut(4). So, if there are 2 items, they are text and sub,
# 3 - variable, text and sub, 4 - text, accelerator, key and sub
# ( because of accelerator and key must be present both and never separately),
# and 5 - all of the above.
# See example below.
# Notes:
#   1. When adding image, scalar must be object derived from Image component
#  or Image component by itself.
#   2. Some platforms cannot keep pure definition of "right-adjacent part";
#  OS/2, for example, can keep no more than two right-adgacent items.
#   3. You cannot assign to shortcut key modificator keys.

sub create_menu
{
   my $img = Prima::Image-> create;
   $img-> load( 'h:/var/backup/os2/os2/bitmap/bossback.bmp');
   return [
      [ "~File" => [
          [ "Anonymous" => "Ctrl+D" => '^d' => sub { print "sub!";}],   # anonymous sub
          [ $img => sub {
             my $img = Prima::Image-> create;
             $img-> load( 'h:/var/backup/os2/os2/bitmap/gold.bmp');
             $_[0]->menu->image( $_[1], $img);
          }],                        # image
          [],                                       # division line
          [ "E~xit" => "Exit"    ]    # calling named function of menu owner
      ]],
      [ ef => "~Edit" => [                  # example of system commands usage
         [ "Cop~y"  => sub { $_[0]->foc_action('copy')}  ],     # try these with input line focused
         [ "Cu~t"   => sub { $_[0]->foc_action('cut')}   ],
         [ "Pa~ste" => sub { $_[0]->foc_action('paste')} ],
         [],
         ["~Kill menu"=>sub{ $_[0]->menuItems(
            [
               [ "~Restore all" => sub {
                  $_[0]->menuItems( $_[0]->create_menu)
               }],
               [ "~Incline" => sub {
                  $_[0]->menu-> insert( $_[0]->create_menu, '', 1);
               }],
            ]);
               }],
         ["~Duplicate menu"=>sub{ TestWindow->create( menu=>$_[0]->menu)}],
      ]],
      [ "~Input line" => [
         [ "Print ~text" => "Text"],
         [ "Print ~selected" => "Selected"],
         [ "Try \"selText\"" => "SelText"],
         [],
         [ "Toggle insert mode" => "InsMode"],
         ["Toggle password mode" => "PassMode"],
         ["Toggle border existence" => "BorderMode"],
         [ coexistentor => "Coexistentor"=> ""],
      ]],
      [],                             # divisor in main menu opens
      [ "~Clusters" => [              # right-adjacent part
        [ "*".checker =>  "Checking Item"   => "Check"     ],
        [],
        [ "-".slave   =>  "Disabled state"   => "PrintText"],
        [ master  =>  "~Enable item above" => "Enable"     ]   # enable/disable and text sample
      ]]
   ];
}

sub foc_action
{
   my ( $self, $action) = @_;
   my $foc = $self-> selectedWidget;
   return unless $foc and $foc-> alive;
   my $ref = $foc-> can( $action);
   $ref->( $foc) if $ref;
}

sub Exit
{
  $::application-> destroy;
}

sub Check
{
   my $menu = $_[ 0]-> menu;
   $menu-> set_check( 'checker', ! $menu-> get_check( 'checker'));
}

sub PrintText
{
   print $_[ 0]-> menu-> slave-> text;
}

sub Enable
{
   my $slave  = $_[0]-> menu-> slave;
   my $master = $_[0]-> menu-> master;
   if ( $slave-> enabled) {
      $slave -> text( "Disabled state");
      $master-> text( "~Enable item above");
   } else {
      $slave -> text( "Enabled state");
      $master-> text( "~Disable item above");
   }
   $slave-> enabled( ! $slave-> enabled);
}

sub Text
{
  print $_[ 0]-> InputLine1-> text;
}

sub Selected
{
  print $_[ 0]-> InputLine1-> selText;
}

sub SelText
{
  $_[ 0]-> InputLine1-> selText ("SEL");
}

sub InsMode
{
  my $e = $_[ 0]-> InputLine1;
  $e-> insertMode ( !$e-> insertMode);
}

sub PassMode
{
  my $e = $_[ 0]-> InputLine1;
  $e-> writeOnly ( !$e-> writeOnly);
}


sub BorderMode
{
  my $e = $_[ 0]-> InputLine1;
  $e-> borderWidth (( $e-> borderWidth == 1) ? 0 : 1);
}


package UserInit;
$::application = Prima::Application-> create( name => "Menu.pm");

my $w = TestWindow-> create(
  text   => "Menu and input line example",
  bottom    => 300,
  size      => [ 360, 120],
  menuItems => TestWindow::create_menu,
  onDestroy => sub {$::application-> close},
);
$w-> insert( "InputLine",
              origin    => [ 50, 10],
              width     => 260,
              text      => $w-> text,
              maxLen    => 200,
              onChange  => sub {
                 $_[0]-> owner-> text( $_[0]-> text);
                 $_[0]-> owner-> Label1-> text( $_[0]-> text);
                 $_[0]-> owner-> menu-> coexistentor-> text( $_[0]-> text)
                   if $_[0]-> owner-> menu-> has_item( 'coexistentor');
              },
            );

$w-> insert( "Label",
              text   => "Type here something",
              left      => 48,
              backColor => cl::Green,
              bottom    => 20 + $w-> InputLine1-> height,
              width     => 200,
              height    => 60,
              valignment => ta::Center,
              focusLink => $w-> InputLine1,
           );
run Prima;
