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
package Prima::MsgBox;

use strict;
use Prima::Classes;
use Prima::Buttons;
use Prima::StdBitmap;
use Prima::Label;
use Prima::Utils;

sub message_box
{
   my ( $title, $text, $options, $extras) = @_;
   $options = mb::Ok | mb::Error unless defined $options;
   $options |= mb::Ok unless $options & 0x0FF;
   my $buttons = $options & 0xFF;
   my $helpTopic = defined $$extras{helpTopic} ? $$extras{helpTopic} : 0;
   my $defButton = defined $$extras{defButton} ? $$extras{defButton} : 0xFF;
   $options &= 0xFF00;
   my @mbs   = qw( Error Warning Information Question);
   my $icon;
   my $nosound = $options & mb::NoSound;

   for ( @mbs)
   {
      my $const = $mb::{$_}->();
      next unless $options & $const;
      $options &= $const;
      $icon = $sbmp::{$_}->();
      last;
   }

   my $fresh;
   my $freshFirst;
   my $dlg = Prima::Dialog-> create(
       centered      => 1,
       width         => 435,
       height        => 150,
       designScale   => [7, 18],
       scaleChildren => 0,
       visible       => 0,
       text          => $title,
       font          => $::application-> get_message_font,
       onExecute     => sub {
          Prima::Utils::beep( $options) if $options && !$nosound;
       },
   );

   my ( $left, $right) = ( 10, $dlg-> width - 10);
   my $i;
   my @bConsts =  ( mb::Help, mb::Cancel, mb::Ignore, mb::Retry, mb::Abort, mb::No, mb::Yes, mb::Ok);
   my @bTexts  = qw( ~Help    ~Cancel     ~Ignore     ~Retry     ~Abort     ~No     ~Yes     ~OK);


   my $dir = Prima::Utils::get_gui;
   $dir = ( $dir == gui::Motif) ? 1 : 0;
   @bConsts = reverse @bConsts unless $dir;
   @bTexts  = reverse @bTexts  unless $dir;

   my $btns = 0;
   for ( $i = 0; $i < scalar @bConsts; $i++)
   {
      next unless $buttons & $bConsts[$i];
      my %epr;

      %epr = %{$extras->{buttons}->{$bConsts[$i]}} if $extras->{buttons} && $extras->{buttons}->{$bConsts[$i]};

      my %hpr = (
         text      => $bTexts[$i],
         ownerFont => 0,
         bottom    => 10,
         default   => ( $bConsts[$i] & $defButton) ? 1 : 0,
         current   => ( $bConsts[$i] & $defButton) ? 1 : 0,
      );
      $defButton = 0 if $bConsts[$i] & $defButton;
      $dir ? ( $hpr{right} = $right, $hpr{tabOrder} = 0) : ( $hpr{left} = $left);
      if ( $bConsts[$i] == mb::Help)
      {
         $hpr{onClick} = sub {
            message('No help available for this time');
            # $helpTopic ...
         };
         unless ( $dir)
         {
            $hpr{right} = $dlg-> width - 10;
            delete $hpr{left};
         }
      } else {
         $hpr{modalResult} = $bConsts[$i];
      }
      $fresh = $dlg-> insert( Button => %hpr, %epr);
      $freshFirst = $fresh unless $freshFirst;
      my $w = $fresh-> width + 10;
      $right -= $w;
      $left  += $w;
      last if ++$btns > 3;
   }
   $fresh = $freshFirst unless $dir;

   my $iconRight = 0;
   my $iconView;
   if ( $icon)
   {
      $icon = Prima::StdBitmap::icon( $icon);
      if ( defined $icon) {
	  $iconView = $dlg-> insert( Widget =>
				     origin         => [ 20, ($dlg-> height + $fresh-> height - $icon-> height)/2],
				     size           => [ $icon-> width, $icon-> height],
				     onPaint        => sub {
					 my $self = $_[1];
					 $self-> color( $dlg-> backColor);
					 $self-> bar( 0, 0, $self-> size);
					 $self-> put_image( 0, 0, $icon);
				     },
				   );

	  $iconRight = $iconView-> right;
      }
   }

   my $label = $dlg-> insert( Label =>
      left          => $iconRight + 15,
      right         => $dlg-> width  - 10,
      top           => $dlg-> height - 10,
      bottom        => $fresh-> height + 20,
      text          => $text,
      autoWidth     => 0,
      wordWrap      => 1,
      showAccelChar => 1,
      valignment    => ta::Middle,
      growMode      => gm::Client,
   );

   my $gl = int( $label-> height / $label-> font-> height);
   my $lc = scalar @{ $label-> text_wrap( $text, $label-> width, tw::NewLineBreak|tw::ReturnLines|tw::WordBreak|tw::ExpandTabs|tw::CalcTabs)};

   if ( $lc > $gl) {
       my $delta = ( $lc - $gl) * $label-> font-> height;
       my $dh = $dlg-> height;
       $delta = $::application-> height - $dh if $dh + $delta > $::application-> height;
       $dlg-> height( $dh + $delta);
       $dlg-> centered(1);
       $iconView-> bottom( $iconView-> bottom + $delta / 2) if $iconView;
   }

   my $ret = $dlg-> execute;
   $dlg-> destroy;
   return $ret;
}


sub message
{
   return message_box( $::application-> name, @_);
}

sub import
{
   no strict 'refs';
   my $callpkg = $Prima::__import || caller;
   for my $sym (qw(message_box message)) {
      *{"${callpkg}::$sym"} = \&{"Prima::MsgBox::$sym"}
   }
   use strict 'refs';
}

1;
