use strict;
use Prima;
use Prima::Classes;
use Prima::StdDlg;
use Prima::Notebooks;
use Prima::MsgBox;
use Prima::StdDlg;
use Prima::VB::VBLoader;
use Prima::VB::VBControls;


$Prima::VB::VBLoader::builderActive = 1;
my $config  = 'Prima::VB::Config';
my $classes = 'Prima::VB::Classes';

#$config  = 'Prima::VB::Lite::Config';
#$classes = 'Prima::VB::Lite::Classes';

eval "use $config; use $classes;";
die "$@" if $@;
use Prima::Application name => 'VB';

$SIG{__WARN__} = sub {
    my $text = shift;
    warn( $text) unless $text =~ /^Subroutine\sAUTOFORM_DEPLOY\sredefined/;
};

package VB;
use vars qw($inspector
            $main
            $form
            $ico
            $fastLoad);

$fastLoad = 1;



package OPropListViewer;
use vars qw(@ISA);
@ISA = qw(PropListViewer);

sub on_click
{
   my $self = $_[0];
   my $index = $self-> focusedItem;
   my $current = $VB::inspector-> {current};
   return if $index < 0 or !$current;
   my $id = $self-> {'id'}->[$index];
   $self-> SUPER::on_click;
   if ( $self->{check}->[$index]) {
      $current->prf_set( $id => $current->{default}->{$id});
   } else {
      $current->prf_delete( $id);
   }
}

sub on_selectitem
{
   my ( $self, $lst) = @_;
   $VB::inspector-> close_item;
   $VB::inspector-> open_item;
}


package ObjectInspector;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      text           => 'Object Inspector',
      width          => 280,
      height         => 350,
      left           => 6,
      sizeDontCare   => 0,
      originDontCare => 0,
      icon           => $VB::ico,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);

   my @sz = $self-> size;

   my $fh = $self-> font-> height + 6;

   $self-> insert( ComboBox =>
      origin   => [ 0, $sz[1] - $fh],
      size     => [ $sz[0], $fh],
      growMode => gm::Ceiling,
      style    => cs::DropDownList,
      name     => 'Selector',
      items    => [''],
   );

   $self-> {monger} = $self-> insert( Notebook =>
      origin  => [ 0, $fh],
      size    => [ 100,  $sz[1] - $fh * 2],
      growMode => gm::Client,
      pageCount => 2,
   );

   $self-> {mtabs} = $self-> insert( Button =>
      origin   => [ 0, 0],
      size     => [ 100, $fh],
      text     => '~Events',
      growMode => gm::Floor,
      name     => 'MTabs',
   );
   $self-> {mtabs}-> {mode} = 0;

   $self-> {plist} = $self-> {monger}-> insert_to_page( 0, OPropListViewer =>
      origin   => [ 0, 0],
      size     => [ 100, $sz[1] - $fh * 2],
      hScroll  => 1,
      name       => 'PList',
      delegateTo => $self,
      growMode   => gm::Client,
   );

   $self-> {elist} = $self-> {monger}-> insert_to_page( 1, OPropListViewer =>
      origin   => [ 0, 0],
      size     => [ 100, $sz[1] - $fh * 2],
      hScroll  => 1,
      name       => 'EList',
      delegateTo => $self,
      growMode   => gm::Client,
   );
   $self-> {currentList} = $self-> {'plist'};

   $self-> insert( Divider =>
      vertical => 1,
      origin => [ 100, 0],
      size   => [ 6, $sz[1] - $fh],
      min    => 50,
      max    => 50,
      name   => 'Div',
   );

   $self-> {panel} = $self-> insert( Notebook =>
      origin    => [ 106, 0],
      size      => [ $sz[0]-106, $sz[1] - $fh],
      growMode  => gm::Right,
      name      => 'Panel',
      pageCount => 1,
   );
   $self-> {panel}->{pages} = {};

   $self->{current} = undef;

   return %profile;
}

sub Div_Change
{
   my $self = $_[0];
   my $right = $self-> Div-> right;
   $self-> {monger}-> width( $self-> Div-> left);
   $self-> {mtabs}-> width( $self-> Div-> left);
   $self-> Panel-> set(
      width => $self-> width - $right,
      left  => $right,
   );
}

sub MTabs_Click
{
   my ( $self, $mtabs) = @_;
   my $ix = $mtabs-> {mode} ? 0 : 1;
   $mtabs->{mode} = $ix;
   $mtabs-> text( $ix ? '~Properties' : '~Events');

   $self-> {monger}-> pageIndex( $ix);
   $self-> {currentList} = $self-> { $ix ? 'elist' : 'plist' };
   $self-> close_item;
   $self-> open_item;
}

sub Selector_Change
{
   my $self = $_[0];
   return if $self->{selectorChanging};
   return unless $VB::form;
   my $t = $self-> Selector-> text;
   return unless length( $t);
   $self->{selectorRetrieving} = 1;
   if ( $t eq $VB::form-> text) {
      $VB::form-> focus;
   } else {
      my @f = $VB::form-> widgets;
      for ( @f) {
         $_->focus, last if $t eq $_->name;
      }
   }
   $self->{selectorRetrieving} = 0;
}


sub item_changed
{
   my  $self = $VB::inspector;
   return unless $self;
   return unless $self-> {opened};
   return if $self-> {sync};
   if ( $self-> {opened}-> valid) {
      if ( $self-> {opened}-> can( 'get')) {
         $self-> {sync} = 1;
         my $data = $self-> {opened}-> get;
         $self->{opened}->{widget}->prf_set( $self->{opened}->{id} => $data);
         my $list = $self-> {currentList};
         my $ix = $list-> {index}->{$self->{opened}->{id}};
         unless ( $list-> {check}->[$ix]) {
            $list-> {check}->[$ix] = 1;
            $list-> redraw_items( $ix);
         }
         $self->{sync} = undef;
      }
   }
}

sub widget_changed
{
   my $how = $_[0];
   my $self = $VB::inspector;
   return unless $self;
   return unless $self-> {opened};
   return if $self-> {sync};
   $self-> {sync} = 1;
   my $id   = $self->{opened}->{id};
   my $data = $self-> {opened}-> {widget}-> prf( $id);
   $self-> {opened}-> set( $data);
   my $list = $self-> {currentList};
   my $ix = $list->{index}->{$id};
   if ( $list-> {check}->[$ix] == $how) {
      $list-> {check}->[$ix] = $how ? 0 : 1;
      $list-> redraw_items( $ix);
   }
   $self-> {sync} = undef;
}


sub close_item
{
   my $self = $_[0];
   return unless defined $self->{opened};
   $self->{opened} = undef;
}

sub open_item
{
   my $self = $_[0];
   return if defined $self->{opened};
   my $list = $self->{currentList};
   my $f = $list-> focusedItem;

   if ( $f < 0) {
      $self->{panel}->pageIndex(0);
      return;
   }
   my $id   = $list->{id}->[$f];
   my $type = $VB::main-> get_typerec( $self->{current}-> {types}->{$id});
   my $p = $self->{panel};
   my $pageset;
   if ( exists $p->{pages}->{$type}) {
      $self-> {opened} = $self->{typeCache}->{$type};
      $pageset = $p->{pages}->{$type};
      $self-> {opened}-> renew( $id, $self-> {current});
   } else {
      $p-> pageCount( $p-> pageCount + 1);
      $p-> pageIndex( $p-> pageCount - 1);
      $p->{pages}->{$type} = $p-> pageIndex;
      $self-> {opened} = $type-> new( $p, $id, $self-> {current});
      $self-> {typeCache}->{$type} = $self-> {opened};
   }
   my $data = $self-> {current}-> prf( $id);
   $self-> {sync} = 1;
   $self-> {opened}-> set( $data);
   $self-> {sync} = undef;
   $p-> pageIndex( $pageset) if defined $pageset;
}


sub enter_widget
{
   return unless $VB::inspector;
   my ( $self, $w) = ( $VB::inspector, $_[0]);
   if ( defined $w) {
      return if defined $self-> {current} and $self-> {current} == $w;
   } else {
      return unless defined $self-> {current};
   }
   my $oid = $self->{opened}->{id} if $self->{opened};
   $self-> {current} = $w;
   $self-> close_item;

   if ( $self-> {current}) {
      my %df = %{$_[0]->{default}};
      my $pf = $_[0]->{profile};

      my @ef = sort keys %{$self->{current}-> {events}};
      my $ep = $self-> {elist};
      my @check = ();
      my %ix = ();
      my $num = 0;
      for ( @ef) {
         push( @check, exists $pf->{$_} ? 1 : 0);
         $ix{$_} = $num++;
         delete $df{$_};
      }
      $ep-> reset_items( [@ef], [@check], {%ix});
      $ep-> focusedItem( $ix{$oid}) if defined $oid and defined $ix{$oid};

      my $lp = $self-> {plist};
      @ef = sort keys %df;
      %ix = ();
      @check = ();
      $num = 0;
      for ( @ef) {
         push( @check, exists $pf->{$_} ? 1 : 0);
         $ix{$_} = $num++;
      }
      $lp-> reset_items( [@ef], [@check], {%ix});
      $lp-> focusedItem( $ix{$oid}) if defined $oid and defined $ix{$oid};

      $self-> Selector-> text( $self-> {current}-> name)
         unless $self->{selectorRetrieving};
      $self-> open_item;
   } else {
      $self-> {panel}-> pageIndex(0);
      for ( qw( plist elist)) {
         my $p = $self-> {$_};
         $p-> {check} = [];
         $p-> {index} = {};
         $p-> set_count(0);
      }
      $self-> Selector-> text( '');
   }
}

sub renew_widgets
{
   return unless $VB::inspector;
   $VB::inspector->{selectorChanging} = 1;
   $VB::inspector-> close_item;
   if ( $VB::form) {
      my @f = ( $VB::form, $VB::form-> widgets);
      $_ = $_->name for @f;
      $VB::inspector-> Selector-> items( \@f);
      my $fx = $VB::form-> focused ? $VB::form : $VB::form-> selectedWidget;
      $fx = $VB::form unless $fx;
      enter_widget( $fx);
   } else {
      $VB::inspector-> Selector-> items( ['']);
      $VB::inspector-> Selector-> text( '');
      enter_widget(undef);
   }
   $VB::inspector->{selectorChanging} = undef;
}

sub preload
{
   my $self = $VB::inspector;
   my $l = $self-> {plist};
   my $cnt = $l-> count;
   $self->{panel}->hide;
   $l->hide;
   $l->focusedItem( $cnt) while $cnt-- > 0;
   $l->show;
   $self->{panel}->show;
}

sub on_destroy
{
   $VB::inspector-> item_changed;
   $VB::inspector = undef;
}

package Form;
use vars qw(@ISA);
@ISA = qw( Prima::Window Prima::VB::Window);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      sizeDontCare   => 0,
      originDontCare => 0,
      width          => 300,
      height         => 200,
      centered       => 1,
      class          => 'Prima::Window',
      module         => 'Prima::Classes',
      selectable     => 1,
      popupItems     => $VB::main-> menu-> get_items( 'edit'),
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $profile{profile}->{name} = $self-> name;
   for ( qw( marked sizeable)) {
      $self->{$_}=0;
   };
   $self-> {dragMode} = 3;
   $self-> init_profiler( \%profile);
   $self-> {guidelineX} = $self-> width  / 2;
   $self-> {guidelineY} = $self-> height / 2;
   return %profile;
}

sub insert_new_control
{
   my ( $self, $x, $y, $where) = @_;
   my $class = $VB::main->{currentClass};
   my $xclass = $VB::main->{classes}->{$class}->{class};
   return unless defined $xclass;
   my $creatOrder = 0;
   my $wn = $where-> name;
   for ( $self-> widgets) {
      my $po = $_-> prf('owner');
      $po = $VB::form-> name unless length $po;
      next unless $po eq $wn;
      my $co = $_-> {creationOrder};
      $creatOrder++;
      next if $co < $creatOrder;
      $creatOrder = $co + 1;
   }
   my $j = $self-> insert( $xclass,
      profile => {
         origin => [$x-4,$y-4],
         owner  => $wn,
      },
      class         => $class,
      module        => $VB::main->{classes}->{$class}->{RTModule},
      creationOrder => $creatOrder,
   );
   $VB::main->{currentClass} = undef;
   $self->{modified} = 1;
   $j-> select;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   $canvas-> backColor( $self-> backColor);
   $canvas-> color( cl::Blue);
   $canvas-> fillPattern(0,0,0,0,4,0,0,0);
   my @sz = $canvas-> size;
   $canvas-> bar(0,0,@sz);
   $canvas-> fillPattern( fp::Solid);
   $canvas-> linePattern( lp::Dash);
   $canvas-> line( $self-> {guidelineX}, 0, $self-> {guidelineX}, $sz[1]);
   $canvas-> line( 0, $self-> {guidelineY}, $sz[0], $self-> {guidelineY});
}

sub on_move
{
   my ( $self, $ox, $oy, $x, $y) = @_;
   $self-> prf_set(
      origin => [$x, $y],
      originDontCare => 0,
   ) unless $self-> {syncRecting};
   $self-> {profile}->{left} = $x;
   $self-> {profile}->{bottom} = $y;
}

sub on_size
{
   my ( $self, $ox, $oy, $x, $y) = @_;
   $self-> prf_set(
      size => [$x, $y],
      sizeDontCare => 0,
   ) unless $self-> {syncRecting};
   $self-> {profile}->{width} = $x;
   $self-> {profile}->{height} = $y;
}

sub on_close
{
   my $self = $_[0];
   if ( $self->{modified}) {
      my $name = defined ( $VB::main->{fmName}) ? $VB::main->{fmName} : 'Untitled';
      my $r = Prima::MsgBox::message_box( 'Warning', "Save changes to $name?", mb::YesNoCancel|mb::Warning);
      if ( $r == mb::Yes) {
         $self-> clear_event, return unless $VB::main-> save;
         $self->{modified} = undef;
      } elsif ( $r == mb::Cancel) {
         $self-> clear_event;
      } else {
         $self->{modified} = undef;
      }
   }
}

sub on_enter
{
   ObjectInspector::enter_widget( $_[0]);
};


sub on_destroy
{
   if ( $VB::form && ( $VB::form == $_[0])) {
      $VB::form = undef;
      if ( defined $VB::main) {
         $VB::main->{fmName} = undef;
         $VB::main->update_menu();
      }
   }
   ObjectInspector::renew_widgets;
}

sub veil
{
   my $self = $_[0];
   $::application-> begin_paint;
   my @r = ( @{$self->{anchor}}, @{$self->{dim}});
   ( $r[0], $r[2]) = ( $r[2], $r[0]) if $r[2] < $r[0];
   ( $r[1], $r[3]) = ( $r[3], $r[1]) if $r[3] < $r[1];
   ( $r[0], $r[1]) = $self-> client_to_screen( $r[0], $r[1]);
   ( $r[2], $r[3]) = $self-> client_to_screen( $r[2], $r[3]);
   $::application-> clipRect( $self-> client_to_screen( 0,0), $self-> client_to_screen( $self-> size));
   $::application-> rect_focus( @r, 1);
   $::application-> end_paint;
}


sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self-> {transaction};
   if ( $btn == mb::Left) {

      unless ( $self-> {transaction}) {
         if ( abs( $self-> {guidelineX} - $x) < 3) {
            $self-> {transaction} = 2;
            $self-> capture(1, $self);
            $self->{saveHdr} = $self-> text;
            return;
         } elsif ( abs( $self-> {guidelineY} - $y) < 3) {
            $self-> {transaction} = 3;
            $self-> capture(1, $self);
            $self->{saveHdr} = $self-> text;
            return;
         }
      }

      if ( defined $VB::main->{currentClass}) {
         $self-> insert_new_control( $x, $y, $self);
      } else {
         $self-> focus;
         $self-> marked(0,1);
         $self-> {transaction} = 1;
         $self-> capture(1);
         $self-> {anchor} = [ $x, $y];
         $self-> {dim}    = [ $x, $y];
         $self-> veil;
      }
   }
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   unless ( $self-> {transaction}) {
      if ( abs( $self-> {guidelineX} - $x) < 3) {
         $self-> pointer( cr::SizeWE);
      } elsif ( abs( $self-> {guidelineY} - $y) < 3) {
         $self-> pointer( cr::SizeNS);
      } else {
         $self-> pointer( cr::Arrow);
      }
      return;
   }
   if ( $self-> {transaction} == 1) {
      return if $self-> {dim}->[0] == $x && $self-> {dim}->[1] == $y;
      $self-> veil;
      $self-> {dim} = [ $x, $y];
      $self-> veil;
      return;
   }

   if ( $self-> {transaction} == 2) {
      $self-> {guidelineX} = $x;
      $self-> text( $x);
      $self-> repaint;
      return;
   }

   if ( $self-> {transaction} == 3) {
      $self-> {guidelineY} = $y;
      $self-> text( $y);
      $self-> repaint;
      return;
   }

}


sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return unless $self-> {transaction};
   return unless $btn == mb::Left;

   $self-> capture(0);
   if ( $self-> {transaction} == 1) {
      $self-> veil;
      my @r = ( @{$self->{anchor}}, @{$self->{dim}});
      ( $r[0], $r[2]) = ( $r[2], $r[0]) if $r[2] < $r[0];
      ( $r[1], $r[3]) = ( $r[3], $r[1]) if $r[3] < $r[1];
      $self-> {transaction} = $self->{anchor} = $self->{dim} = undef;

      for ( $self-> widgets) {
         my @x = $_-> rect;
         next if $x[0] < $r[0] || $x[1] < $r[1] || $x[2] > $r[2] || $x[3] > $r[3];
         $_-> marked(1);
      }
      return;
   }

   if ( $self-> {transaction} == 2 || $self-> {transaction} == 3) {
      $self-> {transaction} = undef;
      $self-> text( $self->{saveHdr});
      return;
   }
}

sub on_leave
{
   $_[0]-> notify(q(MouseUp), mb::Left, 0, 0, 0) if $_[0]-> {transaction};
}

sub dragMode
{
   if ( $#_) {
      my ( $self, $dm) = @_;
      return if $dm == $self->{dragMode};
      $dm = 1 if $dm > 3 || $dm < 1;
      $self->{dragMode} = $dm;
   } else {
      return $_[0]->{dragMode};
   }
}

sub dm_init
{
   my ( $self, $widget) = @_;
   my $cr;
   if ( $self-> {dragMode} == 1) {
      $cr = cr::SizeWE;
   } elsif ( $self-> {dragMode} == 2) {
      $cr = cr::SizeNS;
   } else {
      $cr = cr::Move;
   }
   $widget-> pointer( $cr);
}

sub dm_next
{
   my ( $self, $widget) = @_;
   $self-> dragMode( $self-> dragMode + 1);
   $self-> dm_init( $widget);
}

sub fm_resetguidelines
{
   my $self = $VB::form;
   return unless $self;
   $self-> {guidelineX} = $self-> width  / 2;
   $self-> {guidelineY} = $self-> height / 2;
   $self-> repaint;
}

sub fm_reclass
{
   my $self = $VB::form;
   return unless $self;
   my $cx = $self-> selectedWidget;
   $self = $cx if $cx;
   my $lab_text = 'Class name';
   $lab_text =  "Temporary class for ".$self->{realClass} if defined $self->{realClass};
   my $dlg = Prima::Dialog-> create(
      icon => $VB::ico,
      size => [ 429, 55],
      centered => 1,
      text => 'Change class',
   );
   my ( $i, $b, $l) = $dlg-> insert([ InputLine =>
      origin => [ 5, 5],
      size => [ 317, 20],
      text => $self->{class},
   ], [ Button =>
       origin => [ 328, 4],
       size => [ 96, 36],
       text => '~Change',
       onClick => sub { $dlg-> ok },
       default => 1,
   ], [ Label =>
       origin => [ 5, 28],
       autoWidth => 0,
       size => [ 317, 20],
       text => $lab_text,
   ]);
   if ( $dlg-> execute == cm::OK) {
      $self-> {class} = $i-> text;
      delete $self-> {realClass};
   }
   $dlg->  destroy;
}

sub marked_widgets
{
   my $self = $_[0];
   my @ret = ();
   for ( $self-> widgets) {
      push ( @ret, $_) if $_-> marked;
   }
   return @ret;
}

sub fm_subalign
{
   my $self = $VB::form;
   return unless $self;
   my ( $id) = @_;
   my $cx = $self-> selectedWidget;
   return unless $cx;
   $id ?
      $cx-> bring_to_front :
      $cx-> send_to_back;
}

sub fm_stepalign
{
   my $self = $VB::form;
   return unless $self;
   my ($id) = @_;
   my $cx = $self-> selectedWidget;
   return unless $cx;
   if ( $id) {
      my $cz = $cx-> prev;
      if ( $cz) {
         $cz = $cz-> prev;
         $cz ? $cx-> insert_behind( $cz) : $cx-> bring_to_front;
      }
   } else {
      my $cz = $cx-> next;
      $cx-> insert_behind( $cz) if $cz;
   }
}

sub fm_duplicate
{
   my $self = $VB::form;
   return unless $self;
   my @r = ();
   for ( $self-> marked_widgets) {
      my $j = $self-> insert( $VB::main->{classes}->{$_->{class}}->{class},
         profile => {
            %{$_->{profile}},
            origin => [ $_-> left + 10, $_-> bottom + 10],
         },
         class  => $_->{class},
         module => $_->{module},
         creationOrder => $_->{creationOrder},
      );
      $j-> {realClass} = $_->{realClass} if defined $_->{realClass};
      $j-> focus unless scalar @r;
      push ( @r, $j);
      $j-> marked(1,0);
   }
}

sub fm_selectall
{
   my $self = $VB::form;
   return unless $self;
   $_-> marked(1) for $self-> widgets;
}

sub fm_delete
{
   my $self = $VB::form;
   return unless $self;
   $_-> destroy for $self-> marked_widgets;
   ObjectInspector::renew_widgets();
}

sub fm_creationorder
{
   my $self = $VB::form;
   return unless $self;

   my %cos;
   my @names = ();
   $cos{$_->{creationOrder}} = $_->name for $self-> widgets;
   push( @names, $cos{$_}) for sort {$a <=> $b} keys %cos;

   return unless scalar @names;

   my $d = Prima::Dialog-> create(
      icon => $VB::ico,
      origin => [ 358, 396],
      size => [ 243, 325],
      text => 'Creation order',
   );
   $d-> insert( [ Button =>
      origin => [ 5, 5],
      size => [ 96, 36],
      text => '~OK',
      default => 1,
      modalResult => cm::OK,
   ], [ Button =>
      origin => [ 109, 5],
      size => [ 96, 36],
      text => 'Cancel',
      modalResult => cm::Cancel,
   ], [ ListBox =>
      origin => [ 5, 48],
      name => 'Items',
      size => [ 199, 269],
      items => \@names,
      focusedItem => 0,
   ], [ SpeedButton =>
      origin => [ 209, 188],
      image => Prima::Icon->create( width=>13, height=>13, type => im::bpp1,
palette => [ 0,0,0,0,0,0],
 data =>
"\xff\xf8\x00\x00\x7f\xf0\x00\x00\x7f\xf0\x00\x00\?\xe0\x00\x00\?\xe0\x00\x00".
"\x1f\xc0\x00\x00\x1f\xc0\x00\x00\x0f\x80\x00\x00\x0f\x80\x00\x00\x07\x00\x00\x00".
"\x07\x00\x00\x00\x02\x00\x00\x00\x02\x00\x00\x00".
''),
       size => [ 29, 130],
       onClick => sub {
          my $i  = $d-> Items;
          my $fi = $i-> focusedItem;
          return if $fi < 1;
          my @i = @{$i-> items};
          @i[ $fi - 1, $fi] = @i[ $fi, $fi - 1];
          $i-> items( \@i);
          $i-> focusedItem( $fi - 1);
       },
    ], [ SpeedButton =>
       origin => [ 209, 48],
       image => Prima::Icon->create( width=>13, height=>13, type => im::bpp1,
palette => [ 0,0,0,0,0,0],
 data =>
"\x02\x00\x00\x00\x02\x00\x00\x00\x07\x00\x00\x00\x07\x00\x00\x00\x0f\x80\x00\x00".
"\x0f\x80\x00\x00\x1f\xc0\x00\x00\x1f\xc0\x00\x00\?\xe0\x00\x00\?\xe0\x00\x00".
"\x7f\xf0\x00\x00\x7f\xf0\x00\x00\xff\xf8\x00\x00".
''),
       size => [ 29, 130],
       onClick => sub {
          my $i   = $d-> Items;
          my $fi  = $i-> focusedItem;
          my $c   = $i-> count;
          return if $fi >= $c - 1;
          my @i = @{$i-> items};
          @i[ $fi + 1, $fi] = @i[ $fi, $fi + 1];
          $i-> items( \@i);
          $i-> focusedItem( $fi + 1);
       },
    ]);
    if ( $d-> execute != cm::Cancel) {
       my $cord = 1;
       $self-> bring( $_)-> {creationOrder} = $cord++ for @{$d-> Items-> items};
    }
    $d-> destroy;
}

package MainPanel;
use vars qw(@ISA *do_layer);
@ISA = qw(Prima::Window);

my $myText = 'Form template builder';

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      text           => $myText,
      width          => $::application-> width - 12,
      left           => 6,
      bottom         => $::application-> height - 106 -
         $::application-> get_system_value(sv::YTitleBar) - $::application-> get_system_value(sv::YMenu),
      sizeDontCare   => 0,
      originDontCare => 0,
      height         => 100,
      icon           => $VB::ico,
      onDestroy      => sub {
         $VB::main = undef;
         $::application-> close;
      },
      menuItems      => [
         ['~File' => [
            ['newitem' => '~New' => sub {$_[0]->new;}],
            ['openitem' => '~Open' => 'F3' => 'F3' => sub {$_[0]->open;}],
            ['-saveitem1' => '~Save' => 'F2' => 'F2' => sub {$_[0]->save;}],
            ['-saveitem2' =>'Save ~as...' =>           sub {$_[0]->saveas;}],
            ['closeitem' =>'~Close' =>           sub {$VB::form-> close if $VB::form}],
            [],
            ['E~xit' => 'Alt+X' => '@X' => sub{$_[0]->close;}],
         ]],
         ['edit' => '~Edit' => [
             ['~Select all' => sub { Form::fm_selectall(); }],
             ['D~uplicate'  => 'Ctrl+D' => '^D' => sub { Form::fm_duplicate(); }],
             ['~Delete' => sub { Form::fm_delete(); } ],
             [],
             ['~Align' => [
                ['~Bring to front' => sub { Form::fm_subalign(1);}],
                ['~Send to back'   => sub { Form::fm_subalign(0);}],
                ['Step ~forward'   => sub { Form::fm_stepalign(1);}],
                ['Step ~back'      => sub { Form::fm_stepalign(0);}],
             ]],
             ['~Change class...' => sub { Form::fm_reclass();}],
             ['Creation ~order' => sub { Form::fm_creationorder(); } ],
         ]],
         ['~View' => [
           ['~Object Inspector' => 'F11' => 'F11' => sub {
              if ( $VB::inspector) {
                 $VB::inspector-> restore if $VB::inspector-> windowState == ws::Minimized;
                 $VB::inspector-> bring_to_front;
                 $VB::inspector-> select;
              } else {
                 $VB::inspector = ObjectInspector-> create;
                 ObjectInspector::renew_widgets;
              }
           }],
           [],
           ['Reset ~guidelines' => sub { Form::fm_resetguidelines(); } ],
           ['*gsnap' => 'Snap to guidelines' => sub { $VB::main-> {gsnap} = $VB::main-> menu-> toggle( 'gsnap'); } ],
           [],
           ['-runitem' => '~Run' => 'Ctrl+F9' => '^F9' => \&form_run ],
           ['-breakitem' => '~Break' => \&form_cancel ],
         ]],
      ],
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);

   my %classes = eval "$config".'::classes';
   my @pages   = eval "$config".'::pages';

   $self-> set(
     sizeMin => [ 350, $self->height],
     sizeMax => [ 16384, $self->height],
   );

   $self->{newbutton} = $self-> insert( SpeedButton =>
      origin    => [ 4, $self-> height - 30],
      size      => [ 26, 26],
      hint      => 'New',
      imageFile => Prima::find_image( 'VB::new.bmp'),
      glyphs    => 2,
      onClick   => sub { $VB::main-> new; } ,
   );

   $self->{openbutton} = $self-> insert( SpeedButton =>
      origin    => [ 32, $self-> height - 30],
      size      => [ 26, 26],
      hint      => 'Open',
      imageFile => Prima::find_image( 'VB::open.bmp'),
      glyphs    => 2,
      onClick   => sub { $VB::main-> open; } ,
   );

   $self->{savebutton} = $self-> insert( SpeedButton =>
       origin    => [ 60, $self-> height - 30],
       size      => [ 26, 26],
       hint      => 'Save',
       imageFile => Prima::find_image( 'VB::save.bmp'),
       glyphs    => 2,
       onClick   => sub { $VB::main-> save; } ,
    );

   $self->{runbutton} = $self-> insert( SpeedButton =>
      origin    => [ 88, $self-> height - 30],
      size      => [ 26, 26],
      hint      => 'Run',
      imageFile => Prima::find_image( 'VB::run.bmp'),
      glyphs    => 2,
      onClick   => sub { $VB::main-> form_run} ,
   );


   $self-> {tabset} = $self-> insert( TabSet =>
      left   => 150,
      name   => 'TabSet',
      top    => $self-> height,
      width  => $self-> width - 150,
      growMode => gm::Ceiling,
      topMost  => 1,
      tabs     => [ @pages],
      buffered => 1,
   );

   $self-> {nb} = $self-> insert( Widget =>
      origin => [ 150, 0],
      size   => [$self-> width - 150, $self-> height - $self->{tabset}-> height],
      growMode => gm::Client,
      name    => 'TabbedNotebook',
      onPaint => sub {
         my ( $self, $canvas) = @_;
         my @sz = $self-> size;
         $canvas-> rect3d(0,0,$sz[0]-1,$sz[1],1,$self->light3DColor,$self->dark3DColor,$self->backColor);
      },
   );

   $self-> {nbpanel} = $self-> {nb}-> insert( Notebook =>
      origin     => [12,1],
      size       => [$self->{nb}->width-24,44],
      growMode   => gm::Floor,
      backColor  => cl::Gray,
      name       => 'NBPanel',
      delegateTo => $self,
      onPaint    => sub {
         my ( $self, $canvas) = @_;
         my @sz = $self-> size;
         $canvas-> rect3d(0,0,$sz[0]-1,$sz[1]-1,1,cl::Black,cl::Black,$self->backColor);
         my $i = 0;
         $canvas-> rectangle($i-38,2,$i,40) while (($i+=40)<($sz[0]+36));
      },
   );

   $self-> {leftScroll} = $self-> {nb}-> insert( SpeedButton =>
      origin  => [1,5],
      size    => [11,36],
      name    => 'LeftScroll',
      delegateTo => $self,
      onPaint => sub {
         $_[0]-> on_paint( $_[1]);
         $_[1]-> color( $_[0]-> enabled ? cl::Black : cl::Gray);
         $_[1]-> fillpoly([7,4,7,32,3,17]);
      },
   );

   $self-> {rightScroll} = $self-> {nb}-> insert( SpeedButton =>
      origin  => [$self-> {nb}-> width-11,5],
      size    => [11,36],
      name    => 'RightScroll',
      growMode => gm::Right,
      delegateTo => $self,
      onPaint => sub {
         $_[0]-> on_paint( $_[1]);
         $_[1]-> color( $_[0]-> enabled ? cl::Black : cl::Gray);
         $_[1]-> fillpoly([3,4,3,32,7,17]);
      },
   );

   $self->{classes} = \%classes;
   $self->{pages}   = \@pages;
   $self->{gridAlert} = 5;
   $self->{gsnap}     = 1;
   return %profile;
}

sub on_setup
{
   $_[0]-> reset_tabs;
}

sub on_close
{
   $_[0]-> clear_event if $VB::form && !$VB::form->close;
}

sub reset_tabs
{
   my $self = $_[0];
   my $nb = $self->{nbpanel};
   $nb-> lock;
   $_-> destroy for $nb-> widgets;
   $self-> {tabset}-> tabs( $self->{pages});
   $self-> {tabset}-> tabIndex(0);
   $nb-> pageCount( scalar @{$self->{pages}});
   my %offsets = ();
   my %pagofs  = ();
   my %modules = ();
   my $i=0;
   $pagofs{$_} = $i++ for @{$self->{pages}};

   $modules{$self->{classes}->{$_}->{module}}=1 for keys %{$self->{classes}};

   for ( keys %modules) {
       my $c = $_;
       eval("use $c;");
       if ( $@) {
          Prima::MsgBox::message_box( $myText, "Error loading module $_:$@", mb::Error|mb::OK);
          $modules{$_} = 0;
       }
   }

   my %iconfails = ();
   my %icongtx   = ();
   for ( keys %{$self->{classes}}) {
      my ( $class, %info) = ( $_, %{$self->{classes}->{$_}});
      $offsets{$info{page}} = 4 unless exists $offsets{$info{page}};
      next unless $modules{$info{module}};
      my $i = undef;
      if ( $info{icon}) {
         $info{icon} =~ s/\:(\d+)$//;
         my @parms = ( Prima::find_image( $info{icon}));
         push( @parms, 'index', $1) if defined $1;
         $i = Prima::Icon-> create;
         unless ( defined $parms[0] && $i-> load( @parms)) {
            $iconfails{$info{icon}} = 1;
            $i = undef;
         }
      };
      my $j = $nb-> insert_to_page( $pagofs{$info{page}}, SpeedButton =>
         hint   => $class,
         name   => 'ClassSelector',
         image  => $i,
         origin => [ $offsets{$info{page}}, 4],
         size   => [ 36, 36],
      );
      $j->{orgLeft}   = $offsets{$info{page}};
      $j->{className} = $class;
      $offsets{$info{page}} += 40;
   }
   $self->{nbIndex} = 0;
   $nb-> unlock;
   $self->{currentClass} = undef;

   if ( scalar keys %iconfails) {
      my @x = keys %iconfails;
      Prima::MsgBox::message_box( $myText, "Error loading images: @x", mb::Error|mb::OK);
   }
}

sub get_nbpanel_count
{
   return $_[0]->{nbpanel}->widgets_from_page($_[0]->{nbpanel}->pageIndex);
}

sub set_nbpanel_index
{
   my ( $self, $idx) = @_;
   return if $idx < 0;
   my $nb  = $self->{nbpanel};
   my @wp  = $nb-> widgets_from_page( $nb-> pageIndex);
   return if $idx >= scalar @wp;
   for ( @wp) {
      $_->left( $_->{orgLeft} - $idx * 40);
   }
   $self->{nbIndex} = $idx;
}

sub LeftScroll_Click
{
   $_[0]-> set_nbpanel_index( $_[0]->{nbIndex} - 1);
}

sub RightScroll_Click
{
   $_[0]-> set_nbpanel_index( $_[0]->{nbIndex} + 1);
}

sub ClassSelector_Click
{
   $_[0]->{currentClass} = $_[1]-> {className};
}


sub TabSet_Change
{
   my $self = $_[0];
   my $nb = $self->{nbpanel};
   $nb-> pageIndex( $_[1]-> tabIndex);
   $self-> set_nbpanel_index(0);
}


sub get_typerec
{
   my ( $self, $type, $valPtr) = @_;
   unless ( defined $type) {
      my $rwx = 'generic';
      if ( defined $valPtr && defined $$valPtr) {
         # checking as object
         if ( ref($$valPtr)) {
            if ( $$valPtr->isa('Prima::Icon')) {
               $rwx = 'icon';
            } elsif ( $$valPtr->isa('Prima::Image')) {
               $rwx = 'image';
            }
         }
      }
      return "Prima::VB::Types::$rwx";
   }
   $self-> {typerecs} = () unless exists $self-> {typerecs};
   my $t = $self-> {typerecs};
   return $t->{$type} if exists $t->{type};
   my $ns = \%Prima::VB::Types::;
   my $rwx = exists $ns->{$type.'::'} ? $type : 'generic';
   $rwx = 'Prima::VB::Types::'.$rwx;
   $t->{$type} = $rwx;
   return $rwx;
}

sub new
{
   my $self = $_[0];
   return if $VB::form and !$VB::form-> close;
   $VB::form = Form-> create;
   $VB::main->{fmName} = undef;
   ObjectInspector::renew_widgets;
   update_menu();
}


sub open
{
   my $self = $_[0];

   return if $VB::form and !$VB::form-> can_close;

   my $d = $self->{openFileDlg} ? $self->{openFileDlg} : Prima::OpenDialog-> create(
      icon => $VB::ico,
      filter => [['Form files' => '*.fm'], [ 'All files' => '*']],
   );
   $self->{openFileDlg} = $d;
   return unless $d-> execute;
   $VB::form-> destroy if $VB::form;
   $VB::form = undef;
   update_menu();
   $self->{fmName} = $d-> fileName;
   my $contents;

   if ( CORE::open( F, $self->{fmName})) {
      local $/;
      $contents = <F>;
      close F;
   } else {
      Prima::MsgBox::message_box( $myText, "Error loading ".$self->{fmName}, mb::OK|mb::Error);
      return;
   }

   my $pack;
   if ( $contents =~ /package\s+([^\s;].*)/m) {
      $pack = $1;
      $pack =~ s[[\s;]*$][];
   } else {
      Prima::MsgBox::message_box( $myText,  "Invalid format of ".$self->{fmName}, mb::OK|mb::Error);
      return;
   }

   eval( $contents);
   if ( $@) {
      Prima::MsgBox::message_box( $myText,  "Error loading ".$self->{fmName}.' :'.@_, mb::OK|mb::Error);
      return;
   }

   unless ( $pack->can( "AUTOFORM_DEPLOY")) {
      Prima::MsgBox::message_box( $myText,  "Error loading ".$self->{fmName}." : No sub AUTOFORM_DEPLOY", mb::OK|mb::Error);
      return;
   }

   my $i;
   my %dep;
   my @seq = $pack-> AUTOFORM_DEPLOY;
   for ( $i = 0; $i < scalar @seq; $i+= 2) {
      $dep{$seq[$i]} = $seq[$i + 1];
   }

   my $main;
   my %class = ();
   my $classes = $self->{classes};
   for ( keys %dep) {
      $main = $_, next if $dep{$_}->{parent};
      unless ( $classes->{$dep{$_}->{class}}) {
         $dep{$_}->{realClass} = $dep{$_}->{class};
         $dep{$_}->{class} = $dep{$_}->{parent} ? 'Prima::Window' : 'Prima::Widget';
      }
      $class{$dep{$_}->{class}} = 1;
   }

   my $f = Form-> create(
      realClass   => $dep{$main}->{realClass},
      class   => $dep{$main}->{class},
      module  => $dep{$main}->{module},
      creationOrder => 0,
   );
   for ( keys %{$dep{$main}->{profile}}) {
      next unless $_ eq 'size';
      my $d = $dep{$main}->{profile}->{$_};
      my @x = $f-> size;
   }
   $f-> prf_set( %{$dep{$main}->{profile}});

   my %owners  = ( $main => '');
   my %widgets = ( $main => $f);
   for ( keys %dep) {
      next if $_ eq $main;
      $owners{$_} = exists $dep{$_}->{profile}->{owner} ? $dep{$_}->{profile}->{owner} : $main;
   }
   $VB::form = $f;

   local *do_layer = sub
   {
      my $id = $_[0];
      my $i;
      for ( $i = 0; $i < scalar @seq; $i += 2) {
         $_ = $seq[$i];
         next unless $owners{$_} eq $id;
         my $c = $f-> insert(
            $classes->{$dep{$_}->{class}}->{class},
            realClass     => $dep{$_}->{realClass},
            class         => $dep{$_}->{class},
            module        => $dep{$_}->{module},
            creationOrder => $i / 2,
         );
         if ( exists $dep{$_}->{profile}->{origin}) {
            my @norg = @{$dep{$_}->{profile}->{origin}};
            unless ( exists $widgets{$owners{$_}}) {
               # validating owner entry
               $owners{$_} = $main;
               $dep{$_}->{profile}->{owner} = $main;
            }
            my @ndelta = $owners{$_} eq $main ? (0,0) : (
              $widgets{$owners{$_}}-> left,
              $widgets{$owners{$_}}-> bottom
            );
            $norg[$_] += $ndelta[$_] for 0..1;
            $dep{$_}->{profile}->{origin} = \@norg;
         }
         $c-> prf_set( %{$dep{$_}->{profile}});
         $widgets{$_} = $c;
         &do_layer( $_);
      }
   };
   &do_layer( $main, \%owners);

   ObjectInspector::renew_widgets;
   update_menu();
}


sub write_form
{
   my $self = $_[0];

   my @cmp = $VB::form-> widgets;

   my $c = <<PREHEAD;
package AUTOFORM;

sub AUTOFORM_DEPLOY
{
   return (
PREHEAD

   my $main = $VB::form-> prf( 'name');
   push( @cmp, $VB::form);
   @cmp = sort { $a->{creationOrder} <=> $b->{creationOrder}} @cmp;

   for ( @cmp) {
      my ( $class, $module) = @{$_}{'class','module'};
      $class = $_->{realClass} if defined $_->{realClass};
      my $types = $_->{types};
      my $name = $_-> prf( 'name');
      $c .= <<MEDI;
      '$name' => {
	class   => '$class',
        module  => '$module',
MEDI
      $c .= "        parent => 1,\n" if $_ == $VB::form;
      $c .= "        profile => {\n";
      my ( $x,$prf) = ($_, $_->{profile});
      my @o = $_-> get_o_delta;
      for( keys %{$prf}) {
         my $val = $prf->{$_};
         if ( $_ eq 'origin' && defined $val) {
            my @nval = (
               $$val[0] - $o[0],
               $$val[1] - $o[1],
            );
            $val = \@nval;
         }
         my $type = $self-> get_typerec( $types->{$_}, \$val);
         $val = defined($val) ? $type-> write( $_, $val) : 'undef';
         $c .= "          $_ => $val,\n";
      }
      $c .= "      }},\n";
   }

$c .= <<POSTHEAD;
   );
}
1;
POSTHEAD
   return $c;
}

sub write_PL
{
   my $self = $_[0];

   my $main = $VB::form-> prf( 'name');
   my @cmp = $VB::form-> widgets;

   my $c = <<PREPREHEAD;
use Prima;
use Prima::Classes;
PREPREHEAD

   my %modules = map { $_->{module} => 1 } @cmp;
   $c .= "use $_;\n" for sort keys %modules;
   $c .= <<PREHEAD;

package ${main}Window;
use vars qw(\@ISA);
\@ISA = qw(Prima::Window);

sub profile_default
{
   my \$def = \$_[ 0]-> SUPER::profile_default;
   my \%prf = (
PREHEAD
   my $prf   = $VB::form->{profile};
   my $types = $VB::form->{types};
   for ( keys %$prf) {
      my $val = $prf->{$_};
      my $type = $self-> get_typerec( $types->{$_}, \$val);
      $val = defined($val) ? $type-> write( $_, $val, 1) : 'undef';
      $c .= "       $_ => $val,\n";
   }
   $c .= <<HEAD2;
   );
   \@\$def{keys \%prf} = values \%prf;
   return \$def;
}

sub init
{
   my \$self = shift;
   my \%profile = \$self-> SUPER::init(\@_);
   my \%names   = ( q($main) => \$self);

HEAD2
   @cmp = sort { $a->{creationOrder} <=> $b->{creationOrder}} @cmp;
   my %names = (
      $main => 1
   );
   my @re_cmp;

AGAIN:
   for ( @cmp) {
      my $owner = $_->prf('owner');
      unless ( $names{$owner}) {
         push @re_cmp, $_;
         next;
      }
      my ( $class, $module) = @{$_}{'class','module'};
      $class = $_->{realClass} if defined $_->{realClass};
      my $types = $_->{types};
      my $name = $_-> prf( 'name');
      $names{$name} = 1;
      $c .= "   \$names{q($name)} = \$names{q($owner)}-> insert( $class => \n";
      my ( $x,$prf) = ($_, $_->{profile});
      my @o = $_-> get_o_delta;
      for ( keys %{$prf}) {
         my $val = $prf->{$_};
         if ( $_ eq 'origin' && defined $val) {
            my @nval = (
               $$val[0] - $o[0],
               $$val[1] - $o[1],
            );
            $val = \@nval;
         }
         next if $_ eq 'owner';
         my $type = $self-> get_typerec( $types->{$_}, \$val);
         $val = defined($val) ? $type-> write( $_, $val, 1) : 'undef';
         $c .= "       $_ => $val,\n";
      }
      $c .= "   );\n";
   }
   if ( scalar @re_cmp) {
      @cmp = @re_cmp;
      goto AGAIN;
   }

$c .= <<POSTHEAD;
   return \%profile;
}

sub on_destroy
{
   \$::application-> close;
}

package ${main}Auto;

use Prima::Application;
${main}Window-> create;
run Prima;

POSTHEAD
   return $c;
}


sub save
{
   my ( $self, $asPL) = @_;

   return 0 unless $VB::form;

   return $self-> saveas unless defined $self->{fmName};

   if ( CORE::open( F, ">".$self->{fmName})) {
      local $/;
      my $c = $asPL ? $self-> write_PL : $self-> write_form;
      print F $c;
   } else {
      Prima::MsgBox::message_box( $myText, "Error saving ".$self->{fmName}, mb::OK|mb::Error);
      return 0;
   }
   close F;

   $VB::form->{modified} = undef unless $asPL;

   return 1;
}

sub saveas
{
   my $self = $_[0];
   return 0 unless $VB::form;
   my $d = $self->{saveFileDlg} ? $self->{saveFileDlg} : Prima::SaveDialog-> create(
      icon => $VB::ico,
      filter => [
        ['Form files' => '*.fm'],
        ['Program scratch' => '*.pl'],
        [ 'All files' => '*']
      ],
   );
   $self->{saveFileDlg} = $d;
   return 0 unless $d-> execute;
   my $fn = $d-> fileName;
   my $asPL = ($d-> filterIndex == 1);
   my $ofn = $self->{fmName};
   $self->{fmName} = $fn;
   $self-> save( $asPL);
   $self->{fmName} = $ofn if $asPL;
   return 1;
}

sub update_menu
{
   return unless $VB::main;
   my $m = $VB::main-> menu;
   my $f = (defined $VB::form) ? 1 : 0;
   my $r = (defined $VB::main->{running}) ? 1 : 0;
   $m-> enabled( 'runitem', $f && !$r);
   $m-> enabled( 'newitem', !$r);
   $m-> enabled( 'openitem', !$r);
   $m-> enabled( 'saveitem1', $f);
   $m-> enabled( 'saveitem2', $f);
   $m-> enabled( 'closeitem', $f);
   $m-> enabled( 'breakitem', $f && $r);
   $VB::main-> {runbutton}-> enabled( $f && !$r);
   $VB::main-> {openbutton}-> enabled( !$r);
   $VB::main-> {newbutton}-> enabled( !$r);
   $VB::main-> {savebutton}-> enabled( $f);
}


sub form_cancel
{
   if ( $VB::main) {
      return unless $VB::main-> {running};
      $VB::main-> {running}-> destroy;
      $VB::main-> {running} = undef;
      update_menu();
   }
   $VB::form-> show if $VB::form;
   $VB::inspector-> show if $VB::inspector;
}


sub form_run
{
   my $self = $_[0];
   return unless $VB::form;
   if ( $VB::main->{running}) {
      $VB::main->{running}-> destroy;
      $VB::main->{running} = undef;
   }
   my $c = $self-> write_form;
   my $okCreate = 0;
   eval{
      eval("$c");
      if ( $@) {
         print "Error loading module $@";
         Prima::MsgBox::message_box( $myText, "$@", mb::OK|mb::Error);
         return;
      }
      $Prima::VB::VBLoader::builderActive = 0;
      my @d = &AUTOFORM::AUTOFORM_DEPLOY();
      $Prima::VB::VBLoader::builderActive = 1;
      my %r = Prima::VB::VBLoader::AUTOFORM_REALIZE( \@d, {});
      my $f = $r{$VB::form->prf('name')};
      $okCreate = 1;
      if ( $f) {
         $f-> set( onClose => \&form_cancel );
         $VB::main-> {running} = $f;
         update_menu();
         $f-> select;
         $VB::form-> hide;
         $VB::inspector-> hide if $VB::inspector;
      };
   };
   $Prima::VB::VBLoader::builderActive = 1;
   Prima::MsgBox::message_box( $myText, "$@", mb::OK|mb::Error) if $@;
}


package VisualBuilder;

$VB::ico = Prima::Image-> create;
$VB::ico = undef unless $VB::ico-> load( Prima::find_image( 'VB::vb1.bmp'));
$VB::main = MainPanel-> create;
$VB::inspector = ObjectInspector-> create(
   top => $VB::main-> bottom - 12 - $::application-> get_system_value(sv::YTitleBar)
);
$VB::form = Form-> create;
ObjectInspector::renew_widgets;
ObjectInspector::preload() unless $VB::fastLoad;
$VB::main-> update_menu();

run Prima;
