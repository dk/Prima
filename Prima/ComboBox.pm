#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#

# combo styles
package cs;
use constant Simple       =>  0;
use constant DropDown     =>  1;
use constant DropDownList =>  2;

package Prima::ComboBox;

# contains:
#    ComboBox
#    DriveComboBox

use vars qw(@ISA %listProps %editProps %listDynas);
use Prima qw( InputLine Lists Utils StdBitmaps);
@ISA = qw(Prima::Widget);

use constant DefButtonX => 17;

# these are properties, methods and dynas of encapsulated widgets, edit and list

%editProps = (
   alignment      => 1, autoScroll  => 1, text        => 1, text        => 1,
   charOffset     => 1, maxLen      => 1, insertMode  => 1, firstChar   => 1,
   selection      => 1, selStart    => 1, selEnd      => 1, writeOnly   => 1,
   copy           => 1, cut         => 1, 'delete'    => 1, paste       => 1,
   wordDelimiters => 1, readOnly    => 1, passwordChar=> 1, focus       => 1,
   select_all     => 1,
);

%listProps = (
   autoHeight     => 1, focusedItem    => 1, hScroll        => 1,
   integralHeight => 1, items          => 1, itemHeight     => 1,
   topItem        => 1, vScroll        => 1, gridColor      => 1,
   multiColumn    => 1, offset         => 1,
);

%listDynas = ( onDrawItem => 1, onSelectItem => 1, );

for ( keys %editProps) {
   eval <<GENPROC;
   sub $_ { return shift-> {edit}-> $_(\@_); }
   sub Prima::ComboBox::DummyEdit::$_ {}
GENPROC
}

for (keys %listProps) {
   eval <<GENPROC;
   sub $_ { return shift-> {list}-> $_(\@_); }
   sub Prima::ComboBox::DummyList::$_ {}
GENPROC
}


sub profile_default
{
   my $f = $_[ 0]-> get_default_font;
   return {
      %{Prima::InputLine->profile_default},
      %{Prima::ListBox->profile_default},
      %{$_[ 0]-> SUPER::profile_default},
      style          => cs::Simple,
      listVisible    => 0,
      caseSensitive  => 0,
      entryHeight    => $f-> {height} + 2,
      listHeight     => 100,
      ownerBackColor => 1,
      selectable     => 0,
      literal        => 1,
      scaleChildren  => 0,
      editClass      => 'Prima::InputLine',
      listClass      => 'Prima::ListBox',
      buttonClass    => 'Prima::Widget',
      editProfile    => {},
      listProfile    => {},
      buttonProfile  => {},
      listDelegations   => [qw(Leave SelectItem MouseUp Click KeyDown)],
      editDelegations   => [qw(FontChanged Create Setup KeyDown KeyUp Change)],
      buttonDelegations => [qw(ColorChanged FontChanged MouseDown MouseClick MouseUp MouseMove Paint)],
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   my $style = exists $p->{style} ? $p->{style} : $default->{style};
   if ( $style != cs::Simple) {
      $p->{ entryHeight} = exists $p->{height} ? $p->{height} : $default->{height} unless exists $p->{ entryHeight};
   } else {
      my $fh = exists $p->{font}->{height} ? $p->{font}->{height} : $default->{font}->{height};
      $p->{ entryHeight} = $fh + 2 unless exists $p->{entryHeight};
   }
}

sub init
{
   my $self = shift;
   my %profile = @_;
   my $visible = $profile{visible};
   $profile{visible} = 0;
   $self->{edit} = bless [], q\Prima::ComboBox::DummyEdit\;
   $self->{list} = bless [], q\Prima::ComboBox::DummyList\;
   %profile = $self-> SUPER::init(%profile);
   my ( $w, $h) = ( $self-> size);
   $self-> {style}        = $profile{style};
   $self-> {listVisible}  = $profile{style} != cs::Simple;
   $self-> {caseSensitive}= $profile{caseSensitive};
   $self-> {literal}      = $profile{literal};
   my $eh = $self-> {entryHeight} = $profile{entryHeight};
   $self-> {listHeight}   = $profile{listHeight};

   $self-> {edit} = $self-> insert( $profile{editClass} =>
      name   => 'InputLine',
      origin => [ 0, $h - $eh],
      size   => [ $w - (( $self-> {style} == cs::Simple) ? 0 : DefButtonX), $eh],
      growMode    => gm::GrowHiX,
      selectable  => 1,
      tabStop     => 1,
      current     => 1,
      delegations => $profile{editDelegations},
      (map { $_ => $profile{$_}} keys %editProps),
      %{$profile{editProfile}},
   );

   $self-> {list} = $self-> insert( $profile{listClass} =>
      name         => 'List',
      origin       => [ 0, 0],
      width        => $w,
      height       => ( $self-> {style} == cs::Simple) ? ( $h - $eh) : $self-> {listHeight},
      growMode     => gm::Client,
      tabStop      => 0,
      multiSelect  => 0,
      clipOwner    => $self-> {style} == cs::Simple,
      visible      => $self-> {style} == cs::Simple,
      delegations  => $profile{listDelegations},
      (map { $_ => $profile{$_}} grep { exists $profile{$_} ? 1 : 0} keys %listDynas),
      (map { $_ => $profile{$_}} keys %listProps),
      %{$profile{listProfile}},
   );

   $self-> {button} = $self-> insert( $profile{buttonClass} =>
      ownerBackColor => 1,
      name           => 'Button',
      origin         => [ $w - DefButtonX, $h - $eh],
      size           => [ DefButtonX, $eh],
      visible        => $self-> {style} != cs::Simple,
      growMode       => gm::Right,
      tabStop        => 0,
      ownerFont      => 0,
      selectable     => 0,
      delegations    => $profile{buttonDelegations},
      %{$profile{buttonProfile}},
   );

   $self-> visible( $visible);
   return %profile;
}

sub on_create
{
   my $self = $_[0];
   $self-> InputLine_Change( $self->{edit}) if $self->{style} == cs::DropDownList;
}

sub on_size { $_[0]-> listVisible(0); }
sub on_move { $_[0]-> listVisible(0); }


sub List_Leave
{
   $_[0]->listVisible( 0) if $_[0]->{style} != cs::Simple;
}


sub List_SelectItem
{
   return if defined $_[1]->{interaction};
   $_[0]->{edit}->{interaction} = 1;
   $_[0]->{edit}->text($_[1]-> get_item_text( $_[1]->focusedItem));
   $_[0]->{edit}->{interaction} = undef;
   $_[0]-> notify( q(Change)) if $_[0]->{style} == cs::Simple;
}

sub List_MouseUp
{
   return unless $_[2] == mb::Left || $_[1]-> capture;
   $_[0]-> listVisible(0) if $_[0]->{style} != cs::Simple;
   $_[0]-> notify( q(Change)) if $_[0]->{style} != cs::Simple;
}

sub List_Click
{
   my $self = $_[0];
   $self-> listVisible(0);
   $self-> notify( q(Change));
}

sub List_KeyDown
{
   my ( $self, $list, $code, $key, $mod) = @_;
   if ( $key == kb::Esc)
   {
      $self-> listVisible(0);
      $list-> clear_event;
   }
   elsif ( $key == kb::Enter)
   {
      $list-> click;
      $list-> clear_event;
   }
}


sub Button_ColorChanged
{
   my $self = shift;
   if ( $self->{style} != cs::Simple)
   {
      $self->{list}->color( $self->{button}->color);
      $self->{list}->backColor( $self->{button}->backColor);
   }
}

sub Button_FontChanged
{
   my $self = shift;
   if ( $self->{style} != cs::Simple)
   {
     my $f = $self->{button}-> font;
     $self->{list}->font($f);
   }
}

sub Button_MouseDown
{
   $_[0]-> listVisible( !$_[0]-> {list}-> visible);
   $_[1]-> clear_event;
   return if !$_[0]-> {list}-> visible;
   $_[1]-> capture(1);
}

sub Button_MouseClick
{
   return unless $_[-1];
   $_[0]-> listVisible( !$_[0]-> {list}-> visible);
   $_[1]-> clear_event;
   return if !$_[0]-> {list}-> visible;
   $_[1]-> capture(1);
}


sub Button_MouseMove
{
   $_[1]-> clear_event;
   if ($_[1]-> capture)
   {
      my ($x,$y,$W,$H) = ($_[3],$_[4],$_[1]-> size);
      return unless ($x < 0) || ($y < 0) || ($x >= $W) || ($y >= $H);
      $_[1]-> capture(0);
      $_[0]-> {list}-> mouse_down( mb::Left, 0, 5, $_[0]-> {list}-> height - 5, 1)
         if ($_[0]-> {list}-> visible);
   }
}

sub Button_MouseUp { $_[1]-> capture(0); }

sub Button_Paint
{
   my ( $owner, $self, $canvas) = @_;
   my ( $w, $h)   = $canvas-> size;
   my $ena    = $self-> enabled && $owner-> enabled;
   my @clr    = $ena ?
    ( $self-> color, $self-> backColor) :
    ( $self-> disabledColor, $self-> disabledBackColor);
   my $lv = $owner-> listVisible;
   my ( $rc, $lc) = ( $self-> light3DColor, $self-> dark3DColor);
   ( $rc, $lc) = ( $lc, $rc) if $lv;
   $canvas-> rect3d( 0, 0, $w-1, $h-1, 1, $rc, $lc, $clr[1]);
   if ( $ena) {
      $canvas-> color( $rc);
      $canvas-> fillpoly([ 5, $h * 0.6 - 1, $w - 4, $h * 0.6 - 1, $w/2 + 1, $h * 0.4 - 1]);
   }
   $canvas-> color( $clr[0]);
   $canvas-> fillpoly([ 4, $h * 0.6, $w - 5, $h * 0.6, $w/2, $h * 0.4]);
}

sub InputLine_FontChanged
{
   $_[0]-> entryHeight( $_[1]-> font-> height + $_[1]-> borderWidth * 2);
}

sub InputLine_Create
{
   $_[1]->{incline} = '';
}

sub InputLine_KeyDown
{
   my ( $self, $edit, $code, $key, $mod) = @_;
   return if $mod & km::DeadKey;
   return unless $code;
   return unless $_[0]-> {literal};

   if (( $key & 0xFF00) && ( $key != kb::NoKey) && ( $key != kb::Space) && ( $key != kb::Backspace))
   {
      return if $key == kb::Tab || $key == kb::BackTab || $key == kb::NoKey;
      $edit->{incline} = '';
      $self-> listVisible(1), $edit-> clear_event
         if $key == kb::Down && $_[0]->{style} != cs::Simple;
      $_[0]-> notify( q(Change)), $edit-> clear_event
         if $key == kb::Enter && $_[0]->{style} == cs::DropDownList;
      return;
   }
   else
   {
      return if $_[0]->{style} != cs::DropDownList;
      return if $mod & ( km::Alt|km::Ctrl);
      $edit->{keyDown} = 1;
      if ( $key == kb::Backspace)
      {
        chop $edit->{incline};
      } else {
        $code = chr ( $code);
        $code = uc $code unless $self->caseSensitive;
        $edit->{incline} .= $code;
      }
      my ($ftc,$i,$txt,$t);
      $ftc = quotemeta $edit->{incline};
      for ( $i = 0; $i < $_[0]->{list}->count; $i++)
      {
         $txt = $_[0]->{list}->get_item_text($i);
         $t = $txt;
         $t = uc $t unless $self->caseSensitive;
         last if $t =~ /^$ftc/;
      }
      if ( $i < $_[0]->{list}->count)
      {
         $edit-> text( $txt);
      } else {
         chop $edit->{incline};
      }
      $edit-> selection( 0, length $edit->{incline});
   }
   $edit-> clear_event if $_[0]->{style} == cs::DropDownList;
}


sub InputLine_KeyUp
{
   $_[1]->{keyDown} = undef;
}

sub InputLine_Setup
{
   my $self = shift;
   $self-> InputLine_Change(@_) if $self->{style} == cs::DropDownList;
}

sub InputLine_Change
{
   return if defined $_[0]->{edit}->{interaction};
   return unless $_[0]-> {literal};

   $_[0]->notify(q(Change)) if $_[0]->{style} != cs::DropDownList;

   $_[0]->{list}->{interaction} = 1;
   my ( $self, $style, $list, $edit) = ($_[0], $_[0]->{style}, $_[0]->{list}, $_[1]);
   my $i;
   my $found = 0;
   my $cap = $edit->text;
   $cap = uc $cap unless $self->caseSensitive;
   my @matchArray;
   my @texts;
   my $maxMatch = 0;
   my $matchId = -1;
   # filling list
   for ( $i = 0; $i < $list-> count; $i++)
   {
      my $t = $list->get_item_text($i);
      $t = uc $t unless $self->caseSensitive;
      push ( @texts, $t);
   }
   # trying to find exact match
   for ( $i = 0; $i < scalar @texts; $i++)
   {
      if ( $texts[$i] eq $cap)
      {
         $matchId = $i;
         last;
      }
   }
   if (( $style == cs::DropDown) && ( $matchId < 0) && defined $edit->{keyDown})
   {
      my ($t,$txt);
      for ( $i = 0; $i < scalar @texts; $i++)
      {
         $txt = $t = $list->get_item_text($i);
         $t = uc $t unless $self->caseSensitive;
         last if $t =~ /^\Q$cap\E/;
      }
      # netscape 4 combo behavior
      if ( $i < scalar @texts)
      {
         $edit->{interaction} = 1;
         $edit-> text( $txt);
         $edit->{interaction} = undef;
         $t =~ /^\Q$cap\E/;
         $edit-> selection( length $cap, length $t);
         $list-> focusedItem( $i);
         return;
      }
   }
   # or unexact match
   if ( $matchId < 0)
   {
      for ( $i = 0; $i < scalar @texts; $i++)
      {
         my $l = 0;
         $l++ while $l < length($texts[$i]) && $l < length($cap)
                 && substr( $texts[$i], $l, 1) eq substr( $cap, $l, 1);
         if ( $l >= $maxMatch)
         {
            @matchArray = () if $l > $maxMatch;
            $maxMatch = $l;
            push ( @matchArray, $i);
         }
      }
      $matchId = $matchArray[0] if $matchId < 0;
   }
   $matchId = 0 unless defined $matchId;
   $list-> focusedItem( $matchId);

   if ( $style == cs::DropDownList)
   {
       $edit->{interaction} = 1;
       $edit->text( $list->get_item_text( $matchId));
       $edit->{interaction} = undef;
   }
   $list->{interaction} = undef;
}

sub set_style
{
   my ( $self, $style) = @_;
   return if $self->{style} == $style;
   my $decr = (( $self->{style} == cs::Simple) || ( $style == cs::Simple)) ? 1 : 0;
   $self-> {style} = $style;
   if ( $style == cs::Simple)
   {
      $self-> set(
         height=> $self-> height + $self-> listHeight,
         bottom=> $self-> bottom - $self-> listHeight,
      );
      $self-> {list}-> set(
        visible   => 1,
        origin    => [ 0, 0],
        width     => $self-> width,
        height    => $self-> height - $self-> entryHeight,
        clipOwner => 1,
      );
   } elsif ( $decr) {
      $self-> set(
         height   => $self-> height - $self-> listHeight,
         bottom   => $self-> bottom + $self-> listHeight,
      );
      $self-> { list}-> set(
         visible  => 0,
         height   => $self-> {listHeight},
         clipOwner=> 0,
      );
      $self-> listVisible( 0);
   }
   $self-> {edit}-> set(
      bottom => $self-> height - $self-> entryHeight,
      width  => $self-> { edit}-> width + DefButtonX * $decr *
         (( $style == cs::Simple) ? 1 : -1),
      height => $self-> entryHeight,
   );
   $self-> {button}-> set(
      bottom => $self-> height - $self-> entryHeight,
      height => $self-> entryHeight,
      visible=> $style != cs::Simple,
   );
   if ( $style == cs::DropDownList)
   {
      $self->{edit}->insertMode(1);
      $self->{edit}->text( $self->{edit}->text);
   }
}

sub set_list_visible
{
   my ( $self, $nlv) = @_;
   return if ( $self->{list}-> visible == $nlv) ||
             ( $self->{style} == cs::Simple) ||
             ( !$self-> visible);
   my ( $list, $edit) = ( $self->{list}, $self->{edit});
   if ( $nlv)
   {
      my @gp = $edit->client_to_screen( 0, -$list->height);
      $gp[1] += $edit-> height + $list-> height if $gp[1] < 0;
      $list-> origin( @gp);
   }
   $list-> bring_to_front if $nlv;
   $list-> visible( $nlv);
   $self-> {button}-> repaint;
   $nlv ? $list-> focus : $edit-> focus;
}

sub set_entry_height
{
   my ( $self, $edit, $list, $btn, $h, $new) =
      ($_[0], $_[0]->{edit}, $_[0]->{list}, $_[0]->{button}, $_[0]->height, $_[1]);
   if ( $self->style != cs::Simple)
   {
      $self-> height( $new);
      $edit-> set(
         bottom => 0,
         height => $new
      );
      $btn-> set(
         bottom => 0,
         height => $new
      );
      $list-> height( $self-> {listHeight});
   } else {
      $edit-> set(
         bottom => $h - $new,
         height => $new
      );
      $btn-> set(
         bottom => $h - $new,
         height => $new
      );
      $list-> height( $h - $new);
   }
   $self-> {entryHeight} = $new;
}

sub set_list_height
{
   my ( $self, $hei) = @_;
   if ( $self-> style == cs::Simple)
   {
      $self-> height( $self-> height + $hei - $self->{listHeight});
   } else {
      $self-> {list}-> height($hei);
   }
   $self-> {listHeight} = $hei;
}

sub get_style       { return $_[0]->{style}}
sub get_list_visible{ return $_[0]->{list} ? $_[0]->{list}-> visible : 0}
sub get_entry_height{ return $_[0]->{edit} ? $_[0]->{edit}-> height : 0}
sub get_list_height{ return  $_[0]->{list} ? $_[0]->{list}-> height : 0}

sub caseSensitive{($#_)?$_[0]->{caseSensitive}=$_[1]:return $_[0]->{caseSensitive};}
sub listVisible  {($#_)?$_[0]->set_list_visible($_[1]):return $_[0]->get_list_visible;}
sub style        {($#_)?$_[0]->set_style       ($_[1]):return $_[0]->get_style;       }
sub entryHeight  {($#_)?$_[0]->set_entry_height($_[1]):return $_[0]->get_entry_height;}
sub listHeight   {($#_)?$_[0]->set_list_height ($_[1]):return $_[0]->get_list_height;}
sub literal      {($#_)?$_[0]->{literal} =      $_[1] :return $_[0]->{literal}       }



package Prima::DriveComboBox::InputLine;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      ownerBackColor   => 1,
      selectable       => 1,
      selectingButtons => 0,
   }
}

sub text
{
   return $_[0]->SUPER::text unless $#_;
   my ($self,$cap) = @_;
   $self-> SUPER::text( $cap);
   $self-> notify(q(Change));
   $self-> repaint;
}

sub borderWidth { return 1}
sub selection   { return 0, 0; }

sub on_paint
{
   my ( $self, $canvas, $combo, $W, $H, $focused) =
      ($_[0],$_[1],$_[0]-> owner,$_[1]-> size,$_[0]->focused || $_[0]-> owner->listVisible);
   my $back = $focused ? $self-> hiliteBackColor : $self-> backColor;
   my $fore = $focused ? $self-> hiliteColor : $self-> color;
   $canvas-> rect3d( 0, 0, $W-1, $H-1, 1, $self-> dark3DColor, $self-> light3DColor, $back);
   my $icon = $combo-> {icons}[$combo-> focusedItem];
   my $adj = 3;
   if ( $icon)
   {
      my ($h, $w) = ($icon-> height, $icon-> width);
      $canvas-> put_image( 3, ($H - $h)/2, $icon);
      $adj += $w + 3;
   }
   $canvas-> color( $fore);
   $canvas-> text_out( $self-> text, $adj, ($H - $canvas-> font-> height) / 2);
}

sub on_mousedown
{
   # this code ( instead of listVisible(!listVisible)) is formed so because
   # ::InputLine is selectable, and unwilling focus() could easily hide
   # listBox automatically. Manual focus is also supported by
   # selectingButtons == 0.
   my $self = $_[0];
   my $lv = $self-> owner-> listVisible;
   $self-> owner-> listVisible(!$lv);
   $self-> focus if $lv;
   $self-> clear_event;
}

sub on_enter { $_[0]-> repaint; }
sub on_leave { $_[0]-> repaint; }

sub on_mouseclick
{
   $_[0]-> clear_event;
   return unless $_[5];
   shift->notify(q(MouseDown), @_);
}


package Prima::DriveComboBox;
use vars qw(@ISA @images);
@ISA = qw(Prima::ComboBox);

sub profile_default
{
   my %sup = %{$_[ 0]-> SUPER::profile_default};
   return {
      %sup,
      style            => cs::DropDownList,
      height           => $sup{ entryHeight},
      firstDrive       => 'A:',
      drive            => 'C:',
      editClass        => 'Prima::DriveComboBox::InputLine',
      listClass        => 'Prima::ListViewer',
      editProfile      => {},
   };
}

{
   my $i = 0;
   for ( sbmp::DriveFloppy, sbmp::DriveHDD,    sbmp::DriveNetwork,
         sbmp::DriveCDROM,  sbmp::DriveMemory, sbmp::DriveUnknown) {
      $images[ $i++] = Prima::StdBitmap::icon($_);
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $p->{ style} = cs::DropDownList;
   $self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
   my $self    = shift;
   my %profile = @_;
   $self-> {driveTransaction} = 0;
   $self-> {firstDrive} = $profile{firstDrive};
   $self-> {drives} = [split ' ', Prima::Utils::query_drives_map( $profile{firstDrive})];
   $self-> {icons} = [];
   my $maxhei = $profile{itemHeight};
   for ( @{$self-> {drives}})
   {
      my $dt = Prima::Utils::query_drive_type($_) - dt::Floppy;
      $dt = -1 if $dt < 0;
      my $ic = $images[ $dt];
      push( @{$self-> {icons}}, $ic);
      $maxhei = $ic-> height if $ic && $ic-> height > $maxhei;
   }
   $profile{text} = $profile{drive};
   $profile{items} = [@{$self-> {drives}}];
   push (@{$profile{editDelegations}}, 'KeyDown');
   push (@{$profile{listDelegations}}, qw(DrawItem FontChanged MeasureItem Stringify));
   %profile = $self-> SUPER::init(%profile);
   $self-> {drive} = $self-> text;
   $self-> {list}->itemHeight( $maxhei);
   $self-> drive( $self-> {drive});
   return %profile;
}

sub on_change
{
   my $self = shift;
   $self-> {driveTransaction} = 1;
   $self-> drive( $self-> {drives}-> [$self-> List-> focusedItem]);
   $self-> {driveTransaction} = undef;
}

sub drive
{
   return $_[0]-> {drive} unless $#_;
   return if $_[0]-> {drive} eq $_[1];
   if ( $_[0]-> {driveTransaction})
   {
      $_[0]-> {drive} = $_[1];
      return;
   }
   $_[0]-> {driveTransaction} = 1;
   $_[0]-> text( $_[1]);
   my $d = $_[0]-> {drive};
   $_[0]-> {drive} = $_[0]-> text;
   $_[0]-> notify( q(Change)) if $d ne $_[0]-> text;
   $_[0]-> {driveTransaction} = 0;
}

sub InputLine_KeyDown
{
   my ( $combo, $self, $code, $key) = @_;
   $combo-> listVisible(1), $self-> clear_event if $key == kb::Down;
   return if $key != kb::NoKey;
   $code = uc chr($code) .':';
   ($_[0]-> text( $code),
   $_[0]-> notify( q(Change)))
      if (scalar grep { $code eq $_ } @{$combo-> {drives}}) && ($code ne $_[0]-> text);
   $self-> clear_event;
}

sub List_DrawItem
{
   my ($combo, $me, $canvas, $index, $left, $bottom, $right, $top, $hilite, $focused) = @_;
   my ( $c, $bc);
   if ( $hilite) {
      $c = $me-> color;
      $bc = $me-> backColor;
      $me-> color( $me-> hiliteColor);
      $me-> backColor( $me-> hiliteBackColor);
   }
   $canvas-> clear( $left, $bottom, $right, $top);
   my $text = ${$combo-> {drives}}[$index];
   my $icon = ${$combo-> {icons}}[$index];
   my $font = $canvas-> font;
   my $x = $left + 2;
   my ($h, $w);
   if ( $icon)
   {
      ($h, $w) = ($icon-> height, $icon-> width);
      $canvas-> put_image( $x, ($top + $bottom - $h) / 2, $icon);
      $x += $w + 4;
   }
   ($h,$w) = ($font-> height, $canvas-> get_text_width( $text));
   $canvas-> text_out( $text, $x, ($top + $bottom - $h) / 2);
   if ( $hilite) {
      $canvas-> color( $c);
      $canvas-> backColor( $bc);
   }
}

sub List_FontChanged
{
   my ( $combo, $self) = @_;
   return unless $self->{autoHeight};
   my $maxHei = $self-> font-> height;
   for ( @{$combo->{icons}})
   {
      next unless defined $_;
      $maxHei = $_-> height if $maxHei < $_->height;
   }
   $self-> itemHeight( $maxHei);
   $self-> autoHeight(1);
}

sub List_MeasureItem
{
   my ( $combo, $self, $index, $sref) = @_;
   my $iw = ( $combo->{icons}->[$index] ? $combo->{icons}->[$index]-> width : 0);
   $$sref = $self->get_text_width($combo->{drives}[$index]) + $iw;
   $self-> clear_event;
}

sub List_Stringify
{
   my ( $combo, $self, $index, $sref) = @_;
   $$sref = $combo->{drives}[$index];
   $self-> clear_event;
}


sub set_style { $_[0]-> raise_ro('set_style')}



1;
