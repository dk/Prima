#
#  Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk> 
#     Anton Berezin  <tobez@plab.ku.dk>
#  Modifications by:
#     David Scott <dscott@dgt.com>

#  contains:
#      OpenDialog
#      SaveDialog
#      ChDirDialog

use strict;
use Prima::Classes;
use Prima::Buttons;
use Prima::Lists;
use Prima::Label;
use Prima::InputLine;
use Prima::ComboBox;

package Prima::DirectoryListBox;
use vars qw(@ISA @images);

@ISA = qw(Prima::ListViewer);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      path           => '.',
      openedGlyphs   => 1,
      closedGlyphs   => 1,
      openedIcon     => undef,
      closedIcon     => undef,
      indent         => 12,
      multiSelect    => 0,
   }
}

sub init
{
   unless (@images) {
      my $i = 0;
      for ( sbmp::SFolderOpened, sbmp::SFolderClosed) {
         $images[ $i++] = Prima::StdBitmap::icon($_);
      }
   }
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   for ( qw( maxWidth oneSpaceWidth))            { $self->{$_} = 0}
   for ( qw( files filesStat items))             { $self->{$_} = []; }
   for ( qw( openedIcon closedIcon openedGlyphs closedGlyphs indent))
      { $self->{$_} = $profile{$_}}
   $self-> {openedIcon} = $images[0] unless $self-> {openedIcon};
   $self-> {closedIcon} = $images[1] unless $self-> {closedIcon};
   $self->{fontHeight} = $self-> font-> height;
   $self-> recalc_icons;
   $self-> path( $profile{path});
   return %profile;
}

use constant ROOT      => 0;
use constant ROOT_ONLY => 1;
use constant LEAF      => 2;
use constant LAST_LEAF => 3;
use constant NODE      => 4;
use constant LAST_NODE => 5;

sub on_stringify
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self->{items}->[$index]->{text};
}

sub on_measureitem
{
   my ( $self, $index, $sref) = @_;
   my $item = $self->{items}->[$index];
   $$sref = $self-> get_text_width( $item-> {text}) +
           $self-> {oneSpaceWidth} +
           ( $self->{opened} ?
           ( $self->{openedIcon} ? $self->{openedIcon}->width : 0):
           ( $self->{closedIcon} ? $self->{closedIcon}->width : 0)
           ) +
           4 + $self-> {indent} * $item-> {indent};
}

sub on_fontchanged
{
   my $self = shift;
   $self-> recalc_icons;
   $self->{fontHeight} = $self-> font-> height;
   $self-> SUPER::on_fontchanged(@_);
}

sub on_click
{
   my $self = $_[0];
   my $items = $self-> {items};
   my $foc = $self-> focusedItem;
   return if $foc < 0;
   my $newP = '';
   my $ind = $items->[$foc]-> {indent};
   for (@{$items}) { $newP .= $_-> {text}."/" if $_-> {indent} < $ind; }
   $newP .= $items-> [$foc]-> {text};
   $newP .= '/' unless $newP =~ m![/\\]$!;
   $_[0]-> path( $newP);
}

sub on_drawitem
{
   my ($self, $canvas, $index, $left, $bottom, $right, $top, $hilite, $focusedItem) = @_;
   my $item  = $self->{items}->[$index];
   my $text  = $item->{text};
   $text =~ s[^\s*][];
   # my $clrSave = $self-> color;
   # my $backColor = $hilite ? $self-> hiliteBackColor : $self-> backColor;
   # my $color     = $hilite ? $self-> hiliteColor     : $clrSave;
   # $canvas-> color( $backColor);
   # $canvas-> bar( $left, $bottom, $right, $top);
   my ( $c, $bc);
   if ( $hilite) {
      $c = $self-> color;
      $bc = $self-> backColor;
      $self-> color($self-> hiliteColor);
      $self-> backColor($self-> hiliteBackColor);
   }
   $canvas-> clear( $left, $bottom, $right, $top);
   my $type = $item->{type};
   # $canvas-> color($color);
   my ($x, $y, $x2);
   my $indent = $self-> {indent} * $item->{indent};
   my $prevIndent = $indent - $self-> {indent};
   my ( $icon, $glyphs) = (
     $item->{opened} ? $self->{openedIcon}   : $self->{closedIcon},
     $item->{opened} ? $self->{openedGlyphs} : $self->{closedGlyphs},
   );
   my ( $iconWidth, $iconHeight) = $icon ?
      ( $icon-> width/$glyphs, $icon-> height) : ( 0, 0);
   if ( $type == ROOT || $type == NODE)
   {
      $x = $left + 2 + $indent + $iconWidth / 2;
      $x = int( $x);
      $y = ($top + $bottom) / 2;
      $canvas-> line( $x, $bottom, $x, $y);
   }
   if ( $type != ROOT && $type != ROOT_ONLY)
   {
      $x = $left + 2 + $prevIndent + $iconWidth / 2;
      $x = int( $x);
      $x2 = $left + 2 + $indent + $iconWidth / 2;
      $x2 = int( $x2);
      $y = ($top + $bottom) / 2;
      $canvas-> line( $x, $y, $x2, $y);
      $canvas-> line( $x, $y, $x, $top);
      $canvas-> line( $x, $y, $x, $bottom) if $type == LEAF;
   }
   $canvas-> put_image_indirect ( $icon,
      $left + 2 + $indent,
      int(($top + $bottom - $iconHeight) / 2+0.5),
      0, 0,
      $iconWidth, $iconHeight,
      $iconWidth, $iconHeight, rop::CopyPut);
   $canvas-> text_out( $text,
      $left + 2 + $indent + $self-> {oneSpaceWidth} + $iconWidth,
      int($top + $bottom - $self-> {fontHeight}) / 2+0.5);
   if ( $focusedItem) {
      $canvas-> rect_focus( $left + $self->{offset}, $bottom, $right, $top);
   }
   if ( $hilite) {
      $canvas-> color($c);
      $canvas-> backColor($bc);
   }
   # $canvas-> color($clrSave);
}


sub recalc_icons
{
   my $self = $_[0];
   my $hei = $self-> font-> height + 2;
   my ( $o, $c) = (
      $self->{openedIcon} ? $self->{openedIcon}-> height : 0,
      $self->{closedIcon} ? $self->{closedIcon}-> height : 0
   );
   $hei = $o if $hei < $o;
   $hei = $c if $hei < $c;
   $self-> itemHeight( $hei);
}

sub recalc_items
{
   my ($self, $items) = ($_[0], $_[0]->{items});
   $self-> begin_paint_info;
   $self-> {oneSpaceWidth} = $self-> get_text_width(' ');
   my $maxWidth = 0;
   my @widths = (
      $self->{openedIcon} ? ( $self->{openedIcon}->width / $self->{openedGlyphs}) : 0,
      $self->{closedIcon} ? ( $self->{closedIcon}->width / $self->{closedGlyphs}) : 0,
   );
   for ( @$items)
   {
      my $width = $self-> get_text_width( $_-> {text}) +
                  $self-> {oneSpaceWidth} +
                  ( $_->{opened} ? $widths[0] : $widths[1]) +
                  4 + $self-> {indent} * $_-> {indent};
      $maxWidth = $width if $maxWidth < $width;
   }
   $self-> end_paint_info;
   $self->{maxWidth} = $maxWidth;
}

sub new_directory
{
   my $self = shift;
   my $p = $self-> path;
   my @fs = Prima::Utils::getdir( $p);
   unless ( scalar @fs) {
      $self-> path('.'), return unless $p =~ tr{/\\}{} > 1;
      $self-> {path} =~ s{[/\\][^/\\]+[/\\]?$}{/};
      $self-> path('.'), return if $p eq $self-> {path};
      $self-> path($self-> {path});
      return;
   }

   my $oldPointer = $::application-> pointer;
   $::application-> pointer( cr::Wait);
   my $i;
   my @fs1;
   my @fs2;
   for ( $i = 0; $i < scalar @fs; $i += 2) {
      push( @fs1, $fs[ $i]);
      push( @fs2, $fs[ $i + 1]);
   }

   $self-> {files}     = \@fs1;
   $self-> {filesStat} = \@fs2;
   my @d   = sort grep { $_ ne '.' && $_ ne '..' } $self-> files( 'dir');
   my $ind = 0;
   my @ups = split /[\/\\]/, $p;
   my @lb;
   my $wasRoot = 0;
   for ( @ups)
   {
      push @lb, {
         text          => $_,
         opened        => 1,
         type          => $wasRoot ? NODE : ROOT,
         indent        => $ind++,
      };
      $wasRoot = 1;
   }
   $lb[-1]-> {type} = LAST_LEAF unless scalar @d;
   $lb[-1]-> {type} = ROOT_ONLY if $#ups == 0 && scalar @d == 0;
   my $foc = $#ups;
   for (@d)
   {
      push @lb, {
         text   => $_,
         opened => 0,
         type   => LEAF,
         indent => $ind,
      };
   }
   $lb[-1]-> {type} = LAST_LEAF if scalar @d;
   $self-> items([@lb]);
   $self-> focusedItem( $foc);
   $self-> notify( q(Change));
   $::application-> pointer( $oldPointer);
}


sub path
{
   return $_[0]-> {path} unless $#_;
   my $p = $_[1];
   $p =~ s{^([^\\\/]*[\\\/][^\\\/]*)[\\\/]$}{$1};
   $p = eval { Cwd::abs_path($p) };
   $p = "." if $@ || !defined $p;
   $p = "" unless -d $p;
   $p .= '/' unless $p =~ m![/\\]$!;
   $_[0]-> {path} = $p;
   return if defined $_[0]-> {recursivePathCall} && $_[0]-> {recursivePathCall} > 2;
   $_[0]-> {recursivePathCall}++;
   $_[0]-> new_directory;
   $_[0]-> {recursivePathCall}--;
}

sub files {
   my ( $fn, $fs) = ( $_[0]->{files}, $_[0]-> {filesStat});
   return wantarray ? @$fn : $fn unless ($#_);
   my @f;
   for ( my $i = 0; $i < scalar @$fn; $i++)
   {
      push ( @f, $$fn[$i]) if $$fs[$i] eq $_[1];
   }
   return wantarray ? @f : \@f;
}

sub openedIcon
{
   return $_[0]->{openedIcon} unless $#_;
   $_[0]-> {openedIcon} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

sub closedIcon
{
   return $_[0]->{closedIcon} unless $#_;
   $_[0]->{closedIcon} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

sub openedGlyphs
{
   return $_[0]->{openedGlyphs} unless $#_;
   $_[1] = 1 if $_[1] < 1;
   $_[0]->{openedGlyphs} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

sub closedGlyphs
{
   return $_[0]->{closedGlyphs} unless $#_;
   $_[1] = 1 if $_[1] < 1;
   $_[0]->{closedGlyphs} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

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


package Prima::FileDialog;
use Prima::MsgBox;
use Cwd;
use vars qw( @ISA);
@ISA = qw( Prima::Dialog);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      width       => 635,
      height      => 410,
      centered    => 1,
      visible     => 0,

      defaultExt  => '',
      fileName    => '',
      filter      => [[ 'All files' => '*']],
      filterIndex => 0,
      directory   => '',
      designScale => [ 8, 20],

      createPrompt       => 0,
      multiSelect        => 0,
      noReadOnly         => 0,
      noTestFileCreate   => 0,
      overwritePrompt    => 1,
      pathMustExist      => 1,
      fileMustExist      => 1,
      showHelp           => 0,
      sorted             => 1,

      openMode           => 1,
   }
}

sub canonize_mask
{
   my $self = shift;
   my @ary = split ';', $self-> { mask};
   for (@ary)
   {
      s{^.*[:/\\]([^:\\/]*)$}{$1};
      s{([^\w*?])}{\\$1}g;
      s{\*}{.*}g;
      s{\?}{.?}g;
   }
   $self-> { mask} = "^(${\(join( '|', @ary))})\$";
}

sub canon_path
{
   my $p = shift;
   return Cwd::abs_path($p) if -d $p;
   my $dir = $p;
   my $fn;
   if ($dir =~ s{[/\\]([^\\/]+)$}{}) {
      $fn = $1;
   } else {
      $fn = $p;
      $dir = '.';
   }
   $dir = eval { Cwd::abs_path($dir) };
   $dir = "." if $@;
   $dir = "" unless -d $dir;
   $dir =~ s/(\\|\/)$//;
   return "$dir/$fn";
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);

   my $drives = length( Prima::Utils::query_drives_map);
   $self-> {hasDrives} = $drives;

   for ( qw( defaultExt filter directory filterIndex
             createPrompt fileMustExist noReadOnly noTestFileCreate
             overwritePrompt pathMustExist showHelp openMode sorted
   )) { $self->{$_} = $profile{$_} }
   @{$self-> {filter}}  = [[ '' => '*']] unless scalar @{$self->{filter}};
   my @exts;
   my @mdts;
   for ( @{$self->{filter}})
   {
      push @exts, $$_[0];
      push @mdts, $$_[1];
   }
   $self-> { filterIndex} = scalar @exts - 1 if $self-> { filterIndex} >= scalar @exts;
   $self-> { mask} = $mdts[ $self-> { filterIndex}];
   $self-> { mask} = $profile{fileName} if $profile{fileName} =~ /[*?]/;
   $self-> canonize_mask;
   $self->insert( InputLine =>
      name      => 'Name',
      origin    => [ 14, 343],
      size      => [ 245, 25],
      text      => $profile{fileName},
      maxLen    => 32768,
   );
   $self->insert( Label=>
      origin    => [ 14 , 375],
      size      => [ 245, 25],
      focusLink => $self-> Name,
      text   => '~Filename',
   );

   $self-> insert( ListBox  =>
      name        => 'Files',
      origin      => [ 14,  85 ],
      size        => [ 245, 243],
      multiSelect => $profile{ multiSelect},
      delegations => [qw(KeyDown SelectItem Click)],
   );

   $self->insert( ComboBox =>
      name    => 'Ext',
      origin  => [ 14 , 25],
      size    => [ 245, 25],
      style   => cs::DropDownList,
      items   => [ @exts],
      text    => $exts[ $self->{ filterIndex}],
      delegations => [qw(Change)],
   );

   $self->insert( Label=>
      origin    => [ 14, 55],
      size      => [ 245, 25],
      focusLink => $self-> Ext,
      text   => '~Extensions',
   );

   $self->insert( Label =>
      name      => 'Directory',
      origin    => [ 275, 343],
      size      => [ 235, 25],
      autoWidth => 0,
      text      => $profile{ directory},
      delegations => [qw(FontChanged)],
   );

   $self->insert( DirectoryListBox =>
      name       => 'Dir',
      origin     => [ 275, $drives ? 85 : 25],
      size       => [ 235, $drives ? 243 : 303],
      path       => $self-> { directory},
      delegations=> [qw(Change)],
   );

   $self->insert( DriveComboBox =>
      origin     => [ 275, 25],
      size       => [ 235, 25],
      name       => 'Drive',
      drive      => $self-> Dir-> path,
      delegations=> [qw(Change)],
   ) if $drives;

   $self->insert( Label=>
      origin    => [ 275, 375],
      size      => [ 235, 25],
      text   => 'Di~rectory',
      focusLink => $self-> Dir,
   );

   $self->insert( Label =>
      origin    => [ 275, 55],
      size      => [ 235, 25],
      text   => '~Drives',
      focusLink => $self-> Drive,
   ) if $drives;

   $self->insert( Button=>
      origin  => [ 524, 350],
      size    => [ 96, 36],
      text => $self-> {openMode} ? '~Open' : '~Save',
      name    => 'Open',
      default => 1,
      delegations => [qw(Click)],
   );

   $self->insert( Button=>
      origin      => [ 524, 294],
      name    => 'Cancel',
      text    => 'Cancel',
      size        => [ 96, 36],
      modalResult => cm::Cancel,
   );
   $self->insert( Button=>
      origin      => [ 524, 224],
      name        => 'Help',
      text        => '~Help',
      size        => [ 96, 36],
   ) if $self->{showHelp};
   $self-> Name-> current(1);
   $self-> Name-> select_all;
   $self-> {curpaths} = {};
   if ( $drives) {
      for ( @{$self-> Drive-> items}) { $self->{curpaths}->{lc $_} = $_}
      $self->{curpaths}->{lc $self-> Drive-> drive} = $self-> Dir-> path;
      $self->Drive-> {lastDrive} = $self->Drive-> drive;
   }
   return %profile;
}

sub on_create
{
   my $self = $_[0];
   $self-> Dir_Change( $self-> Dir);
}


sub on_show
{
   my $self = $_[0];
   $self-> Dir_Change( $self-> Dir);
}

sub execute
{
   return ($_[0]-> SUPER::execute != cm::Cancel) ? $_[0]-> fileName : ( wantarray ? () : undef);
}


sub Files_KeyDown
{
   my ( $dlg, $self, $code, $key, $mod) = @_;
   if (( $mod & km::Ctrl) && ( uc chr( $code & 0xFF) eq 'R'))
   {
      $dlg-> Dir-> path( $dlg-> Dir-> path);
      $self-> clear_event;
   }
}

sub Directory_FontChanged
{
   my ( $self, $dc) = @_;
   my ( $w, $path) = ( $dc-> width, $self-> Dir-> path);
   if ( $w < $dc-> get_text_width( $path))
   {
      $path =~ s{(./)}{$1...};
      while ( $w < $dc-> get_text_width( $path)) { $path =~ s{(./\.\.\.).}{$1}};
   }
   $dc-> text( $path);
}

sub Dir_Change
{
   my ( $self, $dir) = @_;
   my $mask = $self-> {mask};
   my @a = grep { /$mask/i; } $dir-> files( 'reg');
   @a = sort {uc($a) cmp uc($b)} @a if $self->{sorted};
   $self-> Files-> items([@a]);
   $self-> Directory_FontChanged( $self-> Directory);
}

sub Drive_Change
{
   my ( $self, $drive) = @_;
   my $newDisk = $drive-> text . "/";
   until (-d $newDisk) {
      last if message_box( $self-> text, "Drive $newDisk is not ready!",
                   mb::Cancel | mb::Retry | mb::Warning) != mb::Retry;
   }
   unless (-d $newDisk) {
      $drive-> drive($drive-> {lastDrive});
      $drive-> clear_event;
      return;
   }
   my $path = $self-> Dir-> path;
   my $drv  = lc substr($path,0,2);
   $self->{curpaths}->{$drv} = $path;
   $self-> Dir-> path( $self->{curpaths}->{lc $drive-> text});
   if ( lc $drive-> text ne lc substr($self-> Dir-> path,0,2))
   {
      $drive->drive( $self-> Dir-> path);
   }
   $drive-> {lastDrive} = $drive-> drive;
}

sub Ext_Change
{
   my ( $self, $ext) = @_;
   my %cont;
   for ( @{$self->{filter}}) { $cont{$$_[0]} = $$_[1]; };
   $self-> {mask} = $cont{ $ext-> text};
   $self-> canonize_mask;
   $self-> Dir_Change( $self-> Dir);
   $self-> {filterIndex} = $ext-> List-> focusedItem;
}

sub Files_SelectItem
{
   my ( $self, $lst) = @_;
   my @items = $lst-> get_items($lst-> selectedItems);
   $self-> Name-> text( scalar @items ? ( join ' ', @items) : '');
}

sub Files_Click
{
   my $self = shift;
   $self-> Files_SelectItem( @_);
   $self-> Open_Click( $self-> Open);
}

sub Open_Click
{
   my $self = shift;
   $_ = $self-> Name-> text;
   my @files = split /[; ]/; # XXX
   s/^\s+// for @files;
   s/\s+$// for @files;
   return unless scalar @files;
   @files = ($files[ 0]) if ( !$self->multiSelect and scalar @files > 1);
   my $compoundMask = join( ';', grep {/[*?]/} map {m!([^/\\]*)$! ? $1 : $_} grep {/[*?]/} @files);
   (@files = grep {/[*?]/} @files), @files = ($files[ 0]) if /[*?]/;
# validating names
   for ( @files)
   {
      s{\\}{/}g;
      $_ = $self-> directory . $_ unless m{^/|:};
      my $pwd = cwd; chdir $self-> directory;
      $_ = canon_path($_);
      chdir $pwd;
   }
   my %uniq; @files = grep { !$uniq{$_}++ } @files;

# testing for indirect directory/mask use
   if ( scalar @files == 1)
   {
      # have single directory
      if ( -d $files[ 0])
      {
         my %cont;
         for ( @{$self->{filter}}) { $cont{$$_[0]} = $$_[1]};
         $self-> directory( $files[ 0]);
         $self-> Name-> text( $cont{ $self-> Ext-> text});
         $self-> Name->focus;
         return;
      }
      my ( $dirTo, $fileTo) = ( $files[ 0] =~ m{^(.*[:/\\])([^:\\/]*)$});
      $dirTo  or $dirTo = '';
      $fileTo = $files[ 0] unless $fileTo || $dirTo;
      $fileTo =~ s/^\s*(.*)\s*$/$1/;
      $dirTo =~ s/^\s*(.*)\s*$/$1/;
      # have directory with mask
      if ( $fileTo =~ /[*?]/)
      {
         $self-> Name-> text( $compoundMask);
         $self-> {mask} = $compoundMask;
         $self-> canonize_mask;
         $self-> directory( $dirTo);
         $self-> Name->focus;
         return;
      }
      if ( $dirTo =~ /[*?]/)
      {
         message_box( $self-> text, "Invalid path name " . $self-> Name-> text, mb::OK | mb::Error);
         $self->Name->select_all;
         $self->Name->focus;
         return;
      }
   }
# possible commands recognized, treating names as files
   for ( @files)
   {
      $_ .= $self-> {defaultExt} if $self-> {openMode} && !m{\.[^/]*$};
      if ( -f $_)
      {
         if ( $self-> {noReadOnly} && !(-w $_))
         {
            message_box( $self-> text, "File $_ is read only", mb::OK | mb::Error);
            $self->Name->select_all;
            $self->Name->focus;
            return;
         }
         return if !$self-> {openMode} && $self->{overwritePrompt} && (
                 message_box( $self-> text,
                 "File $_ already exists. Overwrite?",
                 mb::OKCancel|mb::Warning) != mb::OK);

      }
      else
      {
         my ( $dirTo, $fileTo) = ( m{^(.*[:/\\])([^:\\/]*)$});
         if ( $self-> {openMode} && $self->{createPrompt})
         {
            return if ( message_box( $self-> text,
                "File $_ does not exists. Create?", mb::OKCancel|mb::Information
             ) != mb::OK);
            if ( open FILE, ">$_") { close FILE; } else {
               message_box( $self-> text, "Cannot create file $_: $!", mb::OK | mb::Error);
               $self->Name->select_all;
               $self->Name->focus;
               return;
            }
         }
         if ( $self-> {pathMustExist} and !( -d $dirTo))
         {
            message_box( $self-> text, "Directory $dirTo does not exist", mb::OK | mb::Error);
            $self->Name->select_all;
            $self->Name->focus;
            return;
         }
         if ( $self-> {fileMustExist} and !( -f $_))
         {
            message_box( $self-> text, "File $_ does not exist", mb::OK | mb::Error);
            $self->Name->select_all;
            $self->Name->focus;
            return;
         }
      }
      if ( !$self->{openMode} && !$self->{noTestFileCreate})
      {
         if ( open FILE, ">>$_") { close FILE; } else {
            message_box( $self-> text, "Cannot create file $_: $!", mb::OK | mb::Error);
            $self->Name->select_all;
            $self->Name->focus;
            return;
         }
      }
   };
# flags & files processed, ending process
   $self-> Name-> text( join ( ' ', @files));
   $self-> ok;
}

sub filter
{
   if ( $#_) {
      my $self   = $_[0];
      my @filter = @{$_[1]};
      @filter = [[ '' => '*']] unless scalar @filter;
      my @exts;
      my @mdts;
      for ( @filter) {
         push @exts, $$_[0];
         push @mdts, $$_[1];
      }
      $self-> { filterIndex} = scalar @exts - 1 if $self-> { filterIndex} >= scalar @exts;
      $self-> { mask} = $mdts[ $self-> { filterIndex}];
      $self-> { mask} = '*' unless defined $self-> { mask};
      $self-> canonize_mask;
      $self-> {filter} = \@filter;
      $self-> Ext-> items( \@exts);
      $self-> Ext-> text( $exts[$self-> { filterIndex}]);
   } else {
      return @{$_[0]-> {filter}};
   }
}

sub filterIndex
{
   if ( $#_) {
      return if $_[1] == $_[0]-> Ext-> focusedItem;
      $_[0]-> Ext-> focusedItem( $_[1]);
      $_[0]-> Ext-> notify(q(Change));
   } else {
      return $_[0]-> {filterIndex};
   }
}

sub directory
{
   return $_[0]->Dir->path unless $#_;
   $_[0]->Dir->path($_[1]);
   $_[0]->Drive->text( $_[0]->Dir->path) if $_[0]-> {hasDrives};
}

sub fileName
{
   $_[0]->Name->text($_[1]), return if ($#_);
   return $_[0]->Name->text unless wantarray;
   $_ = $_[0]->Name->text; s/;/ /g;
   return split;
}

sub sorted
{
   return $_[0]-> {sorted} unless $#_;
   return if $_[0]-> {sorted} == $_[1];
   $_[0]-> {sorted} = $_[1];
   $_[0]-> Dir_Change( $_[0]-> Dir);
}

sub reread
{
   $_[0]-> Dir_Change( $_[0]-> Dir);
}

sub multiSelect      { ($#_)? $_[0]->Files->multiSelect($_[1]) : return $_[0]->Files->multiSelect };
sub createPrompt     { ($#_)? $_[0]->{createPrompt} = ($_[1])  : return $_[0]->{createPrompt} };
sub noReadOnly       { ($#_)? $_[0]->{noReadOnly}   = ($_[1])  : return $_[0]->{noReadOnly} };
sub noTestFileCreate { ($#_)? $_[0]->{noTestFileCreate}   = ($_[1])  : return $_[0]->{noTestFileCreate} };
sub overwritePrompt  { ($#_)? $_[0]->{overwritePrompt}   = ($_[1])  : return $_[0]->{overwritePrompt} };
sub pathMustExist    { ($#_)? $_[0]->{pathMustExist}     = ($_[1])  : return $_[0]->{pathMustExist} };
sub fileMustExist    { ($#_)? $_[0]->{fileMustExist}   = ($_[1])  : return $_[0]->{fileMustExist} };
sub showHelp         { ($#_)? shift->raise_ro('showHelp')  : return $_[0]->{showHelp} };

package Prima::OpenDialog;
use vars qw( @ISA);
@ISA = qw( Prima::FileDialog);

sub profile_default {
   return { %{$_[ 0]-> SUPER::profile_default},
            text  => 'Open file',
            openMode => 1,
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $p->{ openMode} = 1;
   $self-> SUPER::profile_check_in( $p, $default);
}

package Prima::SaveDialog;
use vars qw( @ISA);
@ISA = qw( Prima::FileDialog);

sub profile_default  {
   return { %{$_[ 0]-> SUPER::profile_default},
            text         => 'Save file',
            openMode        => 0,
            fileMustExist   => 0,
   }
}
sub profile_check_in
{
    my ( $self, $p, $default) = @_;
    $p->{ openMode} = 0;
    $self-> SUPER::profile_check_in( $p, $default);
}

package Prima::ChDirDialog;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      width       => 500,
      height      => 236,
      centered    => 1,
      visible     => 0,
      text        => 'Change directory',

      directory   => '',
      designScale => [7, 16],
      showHelp    => 0,

      borderStyle => bs::Sizeable,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   my $j;
   my $drives = length( Prima::Utils::query_drives_map);
   $self-> {hasDrives} = $drives;

   for ( qw( showHelp directory
   )) { $self->{$_} = $profile{$_} }

   $self-> insert( DirectoryListBox =>
      origin   => [ 10, $drives ? 40 : 10],
      width    => 480,
      growMode => gm::Client,
      height   => $drives ? 160 : 190,
      name     => 'Dir',
      current  => 1,
      path     => $self-> { directory},
      delegations => [qw(KeyDown)],
   );

   $self->insert( Label =>
      name      => 'Directory',
      origin    => [ 10, 202],
      growMode => gm::Ceiling,
      autoWidth => 1,
      autoHeight => 1,
      text      => '~Directory',
      focusLink => $self-> Dir,
   );

   $self-> insert( DriveComboBox =>
      origin => [ 10, 10],
      width  => 150,
      name   => 'Drive',
      delegations => [qw(Change)],
   ) if $drives;

   $self-> insert( Button =>
      origin  => [ 200, 3],
      size        => [ 80, 30],
      text    => '~OK',
      name    => 'OK',
      default => 1,
      delegations => [qw(Click)],
   );

   $self->insert( Button=>
      origin      => [ 300, 3],
      name        => 'Cancel',
      text        => 'Cancel',
      size        => [ 80, 30],
      modalResult => cm::Cancel,
   );

   $self->insert( Button=>
      origin      => [ 400, 3],
      name        => 'Help',
      text        => '~Help',
      size        => [ 80, 30],
   ) if $self->{showHelp};

   $self-> {curpaths} = {};
   if ( $drives) {
      for ( @{$self-> Drive-> items}) { $self->{curpaths}->{lc $_} = $_}
      $self->{curpaths}->{lc $self-> Drive-> drive} = $self-> Dir-> path;
      $self->Drive-> {lastDrive} = $self->Drive-> drive;
   }

   return %profile;
}



sub Dir_KeyDown
{
   my ( $dlg, $self, $code, $key, $mod) = @_;
   if (( $mod & km::Ctrl) && ( uc chr( $code & 0xFF) eq 'R'))
   {
      $dlg-> Dir-> path( $dlg-> Dir-> path);
      $self-> clear_event;
   }
}

sub Drive_Change
{
   my ( $self, $drive) = @_;
   my $newDisk = $drive-> text . "/";
   until (-d $newDisk) {
      last if message_box( $self-> text, "Drive $newDisk is not ready!",
                   mb::Cancel | mb::Retry | mb::Warning) != mb::Retry;
   }
   unless (-d $newDisk) {
      $drive-> drive($drive-> {lastDrive});
      $drive-> clear_event;
      return;
   }
   my $path = $self-> Dir-> path;
   my $drv  = lc substr($path,0,2);
   $self->{curpaths}->{$drv} = $path;
   $self-> Dir-> path( $self->{curpaths}->{lc $drive-> text});
   if ( lc $drive-> text ne lc substr($self-> Dir-> path,0,2))
   {
      $drive->drive( $self-> Dir-> path);
   }
   $drive-> {lastDrive} = $drive-> drive;
}

sub OK_Click
{
   my $self = $_[0];
   $self-> ok;
}

sub directory
{
   return $_[0]->Dir->path unless $#_;
   $_[0]-> Dir->path($_[1]);
   $_[0]-> Drive->text( $_[0]->Dir->path) if $_[0]-> {hasDrives};
}

sub showHelp         { ($#_)? shift->raise_ro('showHelp')  : return $_[0]->{showHelp} };

1;
