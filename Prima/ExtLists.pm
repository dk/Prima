
# contains:
#   CheckList

use Prima::Const;
use Prima::Classes;
use Prima::Lists;
use Prima::StdBitmap;

package Prima::CheckList;
use vars qw(@ISA);
@ISA = qw(Prima::ListBox);

my @images = (
   Prima::StdBitmap::image(sbmp::CheckBoxUnchecked),
   Prima::StdBitmap::image(sbmp::CheckBoxChecked),
);

my @imgSize = (0,0);
@imgSize = $images[0]-> size;

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      vector => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub init
{
   my $self = shift;
   $self->{vector} = 0;
   my %profile = $self-> SUPER::init(@_);
   $self->{fontHeight} = $self-> font-> height;
   $self-> recalc_icons;
   $self-> vector( $profile{vector});
   return %profile;
}

sub on_measureitem
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self-> get_text_width( $self->{items}->[$index]) + $imgSize[0] + 4;
}

sub recalc_icons
{
   return unless $imgSize[0];
   my $self = $_[0];
   my $hei = $self-> font-> height + 2;
   $hei = $imgSize[1] + 2 if $hei < $imgSize[1] + 2;
   $self-> itemHeight( $hei);
}

sub on_fontchanged
{
   my $self = shift;
   $self-> recalc_icons;
   $self->{fontHeight} = $self-> font-> height;
   $self-> SUPER::on_fontchanged(@_);
}


sub draw_items
{
   my ($self,$canvas) = (shift,shift);
   my @clrs = (
      $self-> get_color,
      $self-> get_back_color,
      $self-> get_color_index( ci::HiliteText),
      $self-> get_color_index( ci::Hilite)
   );
   my @clipRect = $canvas-> get_clip_rect;
   my $i;
   my $drawVeilFoc = -1;
   my $atY    = ( $self-> {itemHeight} - $canvas-> font-> height) / 2;
   my $ih     = $self->{itemHeight};
   my $offset = $self->{offset};
   my $v      = $self->{vector};

   my @colContainer;
   for ( $i = 0; $i < $self->{columns}; $i++){ push ( @colContainer, [])};
   for ( $i = 0; $i < scalar @_; $i++) {
      push ( @{$colContainer[ $_[$i]->[7]]}, $_[$i]);
      $drawVeilFoc = $i if $_[$i]->[6];
   }
   for ( @colContainer)
   {
      my @normals;
      my @selected;
      my ( $lastNormal, $lastSelected) = (undef, undef);
      my $isSelected = 0;
      # sorting items in single column
      { $_ = [ sort { $$a[0]<=>$$b[0] } @$_]; }
      # calculating conjoint bars
      for ( $i = 0; $i < scalar @$_; $i++)
      {
         my ( $itemIndex, $x, $y, $x2, $y2, $selected, $focusedItem) = @{$$_[$i]};
         if ( $selected)
         {
            if ( defined $lastSelected && ( $y2 + 1 == $lastSelected) &&
               ( ${$selected[-1]}[3] - $lastSelected < 100))
            {
               ${$selected[-1]}[1] = $y;
               ${$selected[-1]}[5] = $$_[$i]->[0];
            } else {
               push ( @selected, [ $x, $y, $x2, $y2, $$_[$i]->[0], $$_[$i]->[0], 1]);
            }
            $lastSelected = $y;
            $isSelected = 1;
         } else {
            if ( defined $lastNormal && ( $y2 + 1 == $lastNormal) &&
               ( ${$normals[-1]}[3] - $lastNormal < 100))
            {
               ${$normals[-1]}[1] = $y;
               ${$normals[-1]}[5] = $$_[$i]->[0];
            } else {
               push ( @normals, [ $x, $y, $x2, $y2, $$_[$i]->[0], $$_[$i]->[0], 0]);
            }
            $lastNormal = $y;
         }
      }
      for ( @selected) { push ( @normals, $_); }
      # draw items
      my $yimg = int(( $ih - $imgSize[1]) / 2);
      for ( @normals)
      {
         my ( $x, $y, $x2, $y2, $first, $last, $selected) = @$_;
         $canvas-> set_color( $clrs[ $selected ? 3 : 1]);
         $canvas-> bar( $x, $y, $x2, $y2);
         $canvas-> set_color( $clrs[ $selected ? 2 : 0]);
         for ( $i = $first; $i <= $last; $i++)
         {
             next if $self-> {widths}->[$i] + $offset + $x + 1 < $clipRect[0];
             $canvas-> text_out( $self->{items}->[$i], $x + 2 + $imgSize[0],
                $y2 + $atY - ($i-$first+1) * $ih + 1);
             $canvas-> put_image( $x + 1, $y2 + $yimg - ($i-$first+1) * $ih + 1,
                $images[ vec($v, $i, 1)],
             );
         }
      }
   }
   # draw veil
   if ( $drawVeilFoc >= 0 && $self->{multiSelect})
   {
      my ( $itemIndex, $x, $y, $x2, $y2) = @{$_[$drawVeilFoc]};
      $canvas-> rect_focus( $x + $self->{offset}, $y, $x2, $y2);
   }
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   $self-> SUPER::on_mouseclick( $btn, $mod, $x, $y);
   return if $btn != mb::Left;
   my $foc = $self-> focusedItem;
   return if $foc < 0;
   my $f = vec( $self-> {vector}, $foc, 1) ? 0 : 1;
   vec( $self-> {vector}, $foc, 1) = $f;
   $self-> notify(q(Change), $index, $f);
   $self-> {singlePaint}->{$foc} = 1;
   $self-> refresh;
}

sub on_click
{
   my $self = $_[0];
   my $foc = $self-> focusedItem;
   return if $foc < 0;
   my $f = vec( $self-> {vector}, $foc, 1) ? 0 : 1;
   vec( $self-> {vector}, $foc, 1) = $f;
   $self-> notify(q(Change), $index, $f);
   $self-> {singlePaint}->{$foc} = 1;
   $self-> refresh;
}


sub vector
{
   my $self = $_[0];
   return $self->{vector} unless $#_;
   $self-> {vector} = $_[1];
   $self-> repaint;
}

sub on_change
{
#  my ( $self, $index, $state) = @_;
}

1;

