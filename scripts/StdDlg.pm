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
package StdDlg;

#  contains:
#      OpenDialog
#      SaveDialog
#      FindDialog
#      ReplaceDialog

use strict;
use Carp;
use Classes;
use Buttons;
use Lists;
use Label;
use InputLine;
use ComboBox;

package FileDialog;
use MsgBox;
use Cwd;
use vars qw( @ISA);
@ISA = qw( Dialog);

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
      designScale => [8, 20],

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
   $dir = Cwd::abs_path($dir);
   return "$dir/$fn";
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);

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
      width     => 200,
      text      => $profile{fileName},
   );
   $self->insert( Label=>
      origin    => [ 14 , 375],
      size      => [ 245, 25],
      focusLink => $self-> Name,
      text   => '~Filename',
   );
   $self->insert( ListBox  =>
      name        => 'Files',
      origin      => [ 14,  85 ],
      size        => [ 245, 243],
      multiSelect => $profile{ multiSelect},
   );
   $self->insert( ComboBox =>
      name    => 'Ext',
      origin  => [ 14 , 25],
      size    => [ 245, 25],
      style   => cs::DropDownList,
      items   => [ @exts],
      text => $exts[ $self->{ filterIndex}],
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
      text    => $profile{ directory},
   );
   $self->insert( DirectoryListBox =>
      name       => 'Dir',
      origin     => [ 275, 85],
      size       => [ 235, 243],
      path       => $self-> { directory},
   );
   $self->insert( DriveComboBox =>
      origin     => [ 275, 25],
      size       => [ 235, 25],
      name       => 'Drive',
      drive      => $self-> Dir-> path,
   );
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
   );
   $self->insert( Button=>
      origin  => [ 524, 350],
      size    => [ 96, 36],
      text => $self-> {openMode} ? '~Open' : '~Save',
      name    => 'Open',
      default => 1,
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
      name     => 'Help',
      text     => '~Help',
      size        => [ 96, 36],
   ) if $self->{showHelp};
   $self-> Name-> focus;
   $self-> Name-> select_all;
   $self-> {curpaths} = {};
   for ( @{$self-> Drive-> items}) { $self->{curpaths}->{lc $_} = $_}
   $self->{curpaths}->{lc $self-> Drive-> drive} = $self-> Dir-> path;
   $self->Drive-> {lastDrive} = $self->Drive-> drive;
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
   return $_[0]-> SUPER::execute != cm::Cancel ? $_[0]-> fileName : ( wantarray ? () : undef);
}


sub Files_KeyDown
{
   my ( $dlg, $self, $code, $key, $mod) = @_;
   if (( $mod & kb::Ctrl) && ( uc chr( $code & 0xFF) eq 'R'))
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
   my @a = grep { /$mask/i; } $dir-> files( '-f');
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
      #Utils::message("opapa ($drv): ".$self->{curpaths}->{$drive-> text});
      $drive->drive( $self-> Dir-> path);
   }
   $drive-> {lastDrive} = $drive-> drive;
}

sub Ext_Change
{
   my ( $self, $ext) = @_;
   my %cont = ();
   for ( @{$self->{filter}}) { $cont{$$_[0]} = $$_[1]};
   $self-> {mask} = $cont{ $ext-> text};
   $self-> canonize_mask;
   $self-> Dir_Change( $self-> Dir);
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
   $_ = $self-> Name-> text; s/;/ /g;
   my @files = split;
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
   my %uniq = (); @files = grep { !$uniq{$_}++ } @files;

# testing for indirect directory/mask use
   if ( scalar @files == 1)
   {
      # have single directory
      if ( -d $files[ 0])
      {
         my %cont = ();
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
                 "File $_ alreay exists. Overwrite?",
                 mb::OKCancel) != mb::OK);

      }
      else
      {
         my ( $dirTo, $fileTo) = ( m{^(.*[:/\\])([^:\\/]*)$});
         if ( $self-> {openMode} && $self->{createPrompt})
         {
            return if ( message_box( $self-> text,
                "File $_ does not exists. Create?", mb::OKCancel
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

sub directory
{
   return $_[0]->Dir->path unless $#_;
   $_[0]->Dir->path($_[1]);
   $_[0]->Drive->text( $_[0]->Dir->path);
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

package OpenDialog;
use vars qw( @ISA);
@ISA = qw( FileDialog);

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

package SaveDialog;
use vars qw( @ISA);
@ISA = qw( FileDialog);

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

package FindDialog;
use vars qw(@ISA);
@ISA = qw(Dialog);

sub profile_default
{
   my %def = %{$_[ 0]-> SUPER::profile_default};
   return {
      %def,
      centered    => 1,
      designScale => [ 4, 6],
      width       => 227,
      height      => 137,
      visible     => 0,

      findText    => '',
      replaceText => '',
      scope       => fds::Cursor,
      options     => 0,
      findStyle   => 1,
   }
}


sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> insert( ComboBox =>
      name    => 'Find',
      origin  => [ 51, 120],
      size    => [ 163, 8],
      style   => cs::DropDown,
      text => $profile{findText},
   );
   $self->{findStyle} = $profile{findStyle};

   $self-> insert( Label =>
      origin    => [ 5, 120],
      size      => [ 40, 8],
      text  => '~Find:',
      focusLink => $self-> Find,
   );

   $self-> insert( ComboBox =>
      name    => 'Replace',
      origin  => [ 51, 103],
      size    => [ 163, 8],
      style   => cs::DropDown,
      enabled => !$profile{findStyle},
      text => $profile{replaceText},
   );

   $self-> insert( Label =>
      origin    => [ 5, 103],
      size      => [ 43, 8],
      text   => 'R~eplace:',
      focusLink => $self-> Replace,
      enabled   => !$profile{findStyle},
   );

   my $o = $self-> insert( CheckBoxGroup =>
      name    => 'Options',
      origin  => [ 10, 26],
      size    => [ 100, 68],
      text => 'Options',
   );

   $o-> insert( CheckBox =>
      name    => 'Case',
      origin  => [ 5, 49],
      size    => [ 90, 10],
      text => 'Match ~case',
   );

   $o-> insert( CheckBox =>
      name    => 'Words',
      origin  => [ 5, 38],
      size    => [ 90, 10],
      text => '~Whole words only',
   );

   $o-> insert( CheckBox =>
      name    => 'RE',
      origin  => [ 5, 27],
      size    => [ 90, 10],
      text => '~Regular expression',
   );

   $o-> insert( CheckBox =>
      name    => 'Back',
      origin  => [ 5,  16],
      size    => [ 90, 10],
      text => 'Search bac~kward',
   );

   $o-> insert( CheckBox =>
      name    => 'Prompt',
      origin  => [ 5,  5],
      size    => [ 90, 10],
      enabled => !$profile{findStyle},
      text => 'Replace ~prompt',
   );

   $o-> value( $profile{options});

   $o = $self-> insert( RadioGroup =>
      name    => 'Scope',
      origin  => [ 117, 48],
      size    => [ 100, 46],
      text => 'Scope from',
   );

   $o-> insert( Radio =>
      name    => 'Cursor',
      origin  => [ 5,  27],
      size    => [ 90, 10],
      text     => 'C~ursor',
   );

   $o-> insert( Radio =>
      name    => 'Top',
      origin  => [ 5,  16],
      size    => [ 90, 10],
      text => '~Top',
   );

   $o-> insert( Radio =>
      name    => 'Bottom',
      origin  => [ 5,  5],
      size    => [ 90, 10],
      text => '~Bottom',
   );
   $o-> index( $profile{scope});

   $self-> insert( Button =>
      name    => 'OK',
      origin  => [ 8,  5],
      size    => [ 40, 14],
      default => 1,
      text => '~OK',
   );

   $self-> insert( Button =>
      name    => 'ChangeAll',
      origin  => [ 56,  5],
      size    => [ 76, 14],
      text => 'Change ~all',
      enabled => !$profile{findStyle},
   );

   $self-> insert( Button =>
      origin  => [ 139, 5],
      size    => [ 40, 14],
      text => 'Cancel',
      modalResult => cm::Cancel,
   );
   $self-> Find-> focus;
   return %profile;
}

sub valid
{
   my $self = $_[0];
   my $re = $self-> Find-> text;
   return 0 if length( $re) == 0;
   if ( $self-> options & fdo::RegularExpression) {
      if ( $self-> {findStyle}) {
         eval { $_ = ''; m/$re/; };
      } else {
         my $re2 = $self-> Replace-> text;
         eval { $_ = ''; s/$re/$re2/; };
      }
      if ( $@) {
         MsgBox::message_box( $self-> text, $@, mb::OK|mb::Error);
         $self-> Find-> focus;
         return 0;
      }
   }
   my $list = $self-> Find-> List;
   $list-> insert_items( 0, $re), $self-> findText( $re)
      if $list-> count == 0 or $list-> get_items(0) ne $re;
   $list = $self-> Replace-> List;
   $re = $self-> Replace-> text;
   $list-> insert_items( 0, $re), $self-> replaceText( $re)
      if !$self->{findStyle} && ( $list-> count == 0 or $list-> get_items(0) ne $re);
   return 1;
}

sub OK_Click
{
   my $self = $_[0];
   return unless $self-> valid;
   $self-> ok;
}

sub ChangeAll_Click
{
   my $self = $_[0];
   return unless $self-> valid;
   $self-> modalResult( cm::User);
   $self-> end_modal;
}


sub scope       {($#_)?$_[0]->Scope->index($_[1])    :return $_[0]->Scope->index}
sub options     {($#_)?$_[0]->Options->value($_[1])  :return $_[0]->Options->value}
sub findText    {($#_)?$_[0]->Find->text($_[1])   :return $_[0]->Find->text}
sub replaceText {($#_)?$_[0]->Replace->text($_[1]):return $_[0]->Replace->text}

package ReplaceDialog;
use vars qw(@ISA);
@ISA = qw(FindDialog);

sub profile_default
{
   my %def = %{$_[ 0]-> SUPER::profile_default};
   return {
      %def,
      findStyle  => 0,
   }
}


1;
