package MsgBox;

use strict;
use Classes;
use Buttons;
use StdBitmap;
use Label;
use Utils;

require Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter);
$VERSION = '1.00';
@EXPORT = qw(message_box message);
@EXPORT_OK = qw(message_box message);
%EXPORT_TAGS = ();


sub message_box
{
   my ( $title, $text, $options, $helpTopic) = @_;
   $options = mb::Ok | mb::Error unless defined $options;
   $options |= mb::Ok unless $options & 0x0FF;
   my $buttons = $options & 0xFF;
   $helpTopic ||= 0;
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
   my $dlg = Dialog-> create(
       centered      => 1,
       width         => 435,
       height        => 150,
       designScale   => [7, 18],
       scaleChildren => 0,
       visible       => 0,
       text          => $title,
       font          => $::application-> get_message_font,
       onExecute     => sub {
          $fresh-> focus;
          Utils::beep( $options) if $options && !$nosound;
       },
   );

   my ( $left, $right) = ( 10, $dlg-> width - 10);
   my $i;
   my @bConsts =  ( mb::Help, mb::Cancel, mb::Ignore, mb::Retry, mb::Abort, mb::No, mb::Yes, mb::Ok);
   my @bTexts  = qw( ~Help    ~Cancel     ~Ignore     ~Retry     ~Abort     ~No     ~Yes     ~OK);

   my $dir = Utils::get_gui;
   $dir = ( $dir == gui::Motif) ? 1 : 0;
   @bConsts = reverse @bConsts unless $dir;
   @bTexts  = reverse @bTexts  unless $dir;

   my $btns = 0;
   for ( $i = 0; $i < scalar @bConsts; $i++)
   {
      next unless $buttons & $bConsts[$i];
      my %hpr = (
         bottom    => 10,
         text      => $bTexts[$i],
         ownerFont => 0,
      );
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
      $fresh = $dlg-> insert( Button => %hpr);
      $freshFirst = $fresh unless $freshFirst;
      my $w = $fresh-> width + 10;
      $right -= $w;
      $left  += $w;
      last if ++$btns > 3;
   }
   $fresh = $freshFirst unless $dir;
   $fresh-> default(1);

   my $iconRight = 0;
   my $iconView;
   if ( $options)
   {
      $icon = StdBitmap::icon( $icon);
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

   my $label = $dlg-> insert( Label =>
      left          => $iconRight + 15,
      right         => $dlg-> width  - 10,
      top           => $dlg-> height - 10,
      bottom        => $fresh-> height + 20,
      text          => $text,
      autoWidth     => 0,
      wordWrap      => 1,
      showAccelChar => 1,
      valignment    => ta::Center,
   );

   my $lHeight = ( scalar( $label-> get_lines)-1) * $label-> font-> height;
   my $delta = $label-> height;
   while ( $lHeight > $label-> height)
   {
       $dlg-> height( $dlg-> height + $delta);
       $dlg-> centered(1);
       $iconView-> bottom( $iconView-> bottom + $delta / 2) if $iconView;
       $label-> height( $label-> height + $delta);
       $lHeight = ( scalar( $label-> get_lines)-1) * $label-> font-> height;
   }

   my $ret = $dlg-> execute;
   $dlg-> destroy;
   return $ret;
}


sub message
{
   return message_box( $::application-> name, $_[0], mb::Ok|mb::Error);
}

1;
