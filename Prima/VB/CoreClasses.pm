package Prima::VB::CoreClasses;

use Prima::StdBitmap;

sub classes
{
   return (
      'Prima::Button' => {
         icon => 'icons/button.bmp',
      },
      'Prima::Label' => {
         icon => 'icons/label.bmp',
      },
      'Prima::InputLine' => {
         icon => 'icons/input.bmp',
      },
      'Prima::Radio' => {
         icon => 'icons/radio.gif',
      },
      'Prima::CheckBox' => {
         icon => 'icons/checkbox.gif',
      },
   );
}

package Prima::VB::CommonControl;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Control);

sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      firstClick
      palette
      sizeMax
      sizeMin
      ownerColor
      ownerFont
      ownerHint
      ownerBackColor
      ownerShowHint
      ownerPalette
   );
}



package Prima::VB::Button;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool    => ['flat','vertical','default','checkable','checked'],
      uiv     => ['glyphs','borderWidth',],
      upv     => ['imageScale',],
      glyph   => ['defaultGlyph','hiliteGlyph','disabledGlyph','pressedGlyph','holdGlyph'],
      modalResult  => ['modalResult',],
      icon    => ['image',],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub caption_box
{
   my ($self,$canvas) = @_;
   my $cap = $self-> get_text;
   $cap =~ s/~//;
   return $canvas-> get_text_width( $cap), $canvas-> font-> height;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size = $canvas-> size;
   my $cl = $self-> color;
   if ( $self->prf('flat')) {
      $canvas-> rect3d( 0,0,$size[0]-1,$size[1]-1,1,cl::Gray,cl::Gray,$self-> backColor);
   } else {
      $canvas-> rect3d( 0,0,$size[0]-1,$size[1]-1,$self->prf('borderWidth'),$self->light3DColor,$self->dark3DColor,$self-> backColor);
   }
   my $i = $self->prf('image');
   my $capOk = length($self-> text) > 0;
   my ( $fw, $fh) = $capOk ? $self-> caption_box($canvas) : ( 0, 0);
   my ( $textAtX, $textAtY) = ( 2, $size[1]-3);

   if ( defined $i) {
      my $gy = $self->prf('glyphs');
      $gy = 1 unless $gy;
      my $pw = $i->width / $gy;
      my $ph = $i->height;
      my $sw = $pw * $self->prf('imageScale');
      my $sh = $ph * $self->prf('imageScale');
      my ( $imAtX, $imAtY);
      if ( $capOk) {
         if ( $self-> prf('vertical')) {
            $imAtX = ( $size[ 0] - $sw) / 2;
            $imAtY = ( $size[ 1] - $fh - $sh) / 3;
            $textAtY = $imAtY;
            $imAtY   = $size[ 1] - $imAtY - $sh;
         } else {
            $imAtX = ( $size[ 0] - $fw - $sw) / 3;
            $imAtY = ( $size[ 1] - $sh) / 2;
            $textAtX = 2 * $imAtX + $sw;
         }
      } else {
         $imAtX = ( $size[0] - $sw) / 2;
         $imAtY = ( $size[1] - $sh) / 2;
      }
      $canvas-> put_image_indirect( $i, $imAtX, $imAtY, 0, 0,$sw, $sh,$pw, $ph,rop::CopyPut);
   }

   $canvas-> color( $cl);
   if ( $capOk) {
      $canvas-> draw_text($self-> text,$textAtX,2,$size[0]-3,$textAtY,dt::DrawMnemonic|dt::Center|dt::VCenter|dt::UseClip);
   }
   $self-> common_paint($canvas);
}

sub prf_flat        { $_[0]->repaint; }
sub prf_borderWidth { $_[0]->repaint; }
sub prf_glyphs      { $_[0]->repaint; }
sub prf_vertical    { $_[0]->repaint; }
sub prf_image       { $_[0]->repaint; }
sub prf_imageScale  { $_[0]->repaint; }

package Prima::VB::Label;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      align   => ['alignment',],
      valign  => ['valignment',],
      bool    => ['autoWidth','autoHeight','showAccelChar','showPartial','wordWrap'],
      sibling => ['focusLink',],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{focusLink};
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   my $cl = $self-> color;
   $canvas-> color( $self-> backColor);
   $canvas-> bar( 1,1,$sz[0]-2,$sz[1]-2);
   $canvas-> color( cl::Gray);
   $canvas-> rectangle( 0,0,$sz[0]-1,$sz[1]-1);
   $canvas-> color( $cl);
   my ( $a, $va, $sp, $ww, $sac) =
      $self-> prf(qw( alignment valignment showPartial wordWrap showAccelChar));
   my $flags = dt::NewLineBreak | dt::WordBreak | dt::ExpandTabs |
      (($a == ta::Left) ? dt::Left : (( $a == ta::Center) ? dt::Center : dt::Right)) |
      (ta::Top == $va ? dt::Top : (( $va == ta::Middle) ? dt::VCenter : dt::Bottom)) |
      ($sp  ? dt::DrawPartial : 0) |
      ($ww  ? 0 : dt::NoWordWrap) |
      ($sac ? 0 : dt::DrawMnemonic);
   $canvas-> draw_text( $self-> text,0,0,$sz[0]-1,$sz[1]-1,$flags);
   $self-> common_paint($canvas);
}

sub prf_alignment     { $_[0]->repaint; }
sub prf_valignment    { $_[0]->repaint; }
sub prf_showPartial   { $_[0]->repaint; }
sub prf_wordWrap      { $_[0]->repaint; }
sub prf_showAccelChar { $_[0]->repaint; }

package Prima::VB::InputLine;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);


sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      align   => ['alignment',],
      bool    => ['writeOnly','readOnly','insertMode','autoSelect','autoTab','firstChar','charOffset'],
      upoint  => ['selection',],
      uiv     => ['selStart','selEnd','maxLen','borderWidth'],
      char    => ['passwordChar',],
      string  => ['wordDelimiters',],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      selection
      selEnd
      selStart
      charOffset
      firstChar
   );
}



sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   my $cl = $self-> color;
   my ( $a, $bw, $wo, $pc) = $self-> prf(qw( alignment borderWidth writeOnly passwordChar));
   $canvas-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,$bw,$self->dark3DColor,$self->light3DColor,$self-> backColor);
   $canvas-> color( $cl);
   $a = (ta::Left == $a ? dt::Left : (ta::Center == $a ? dt::Center : dt::Right));
   my $c = $self->text;
   $c =~ s/./$pc/g if $wo;
   $canvas-> draw_text($c,2,2,$sz[0]-3,$sz[1]-3,$a|dt::VCenter|dt::UseClip|dt::ExpandTabs|dt::NoWordWrap);
   $self-> common_paint($canvas);
}

sub prf_alignment     { $_[0]->repaint; }
sub prf_writeOnly     { $_[0]->repaint; }
sub prf_borderWidth   { $_[0]->repaint; }
sub prf_passwordChar  { $_[0]->repaint; }


package Prima::VB::Cluster;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool    => ['auto','checked'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub prf_checked { $_[0]-> repaint; }

sub paint
{
   my ( $self, $canvas, $i) = @_;
   my @sz = $canvas-> size;
   $canvas-> color( $self-> backColor);
   $canvas-> bar(1,1,$sz[0]-2,$sz[1]-2);
   $canvas-> color( cl::Gray);
   $canvas-> rectangle(0,0,$sz[0]-1,$sz[1]-1);
   $canvas-> put_image( 2, ($sz[1] - $i->height)/2, $i) if $i;
   $canvas-> color( cl::Black);
   my $w = $i ? $i-> width : 0;
   $canvas-> draw_text($self-> text,2 + $w,2,$sz[0]-1,$sz[1]-1,
      dt::Center|dt::VCenter|dt::UseClip|dt::DrawMnemonic);
   $self-> common_paint($canvas);
}

package Prima::VB::CheckBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Cluster);

sub on_paint
{
   my ( $self, $canvas) = @_;
   $self-> paint( $canvas, Prima::StdBitmap::image( $self->prf('checked') ?
      sbmp::CheckBoxChecked : sbmp::CheckBoxUnchecked));
}

package Prima::VB::Radio;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Cluster);

sub on_paint
{
   my ( $self, $canvas) = @_;
   $self-> paint( $canvas, Prima::StdBitmap::icon( $self->prf('checked') ?
      sbmp::RadioChecked : sbmp::RadioUnchecked));
}

package Prima::VB::GroupBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size   = $canvas-> size;
   my @clr    = ( $self-> color, $self-> backColor);
   $canvas-> color( $clr[1]);
   $canvas-> bar( 0, 0, @size);
   my $fh = $canvas-> font-> height;
   $canvas-> color( $self-> light3DColor);
   $canvas-> rectangle( 1, 0, $size[0] - 1, $size[1] - $fh / 2 - 2);
   $canvas-> color( $self-> dark3DColor);
   $canvas-> rectangle( 0, 1, $size[0] - 2, $size[1] - $fh / 2 - 1);
   my $c = $self-> text;
   if ( length( $c) > 0) {
      $canvas-> color( $clr[1]);
      $canvas-> bar  ( 8, $size[1] - $fh - 1, 16 + $canvas-> get_text_width( $c), $size[1] - 1);
      $canvas-> color( $clr[0]);
      $canvas-> text_out( $c, 12, $size[1] - $fh - 1);
   }
   $self-> common_paint($canvas);
}


package Prima::VB::GroupRadioBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::GroupBox);


package Prima::VB::GroupCheckBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::GroupBox);

package Prima::VB::BiScroller;

sub paint_exterior
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   my $cl = $self-> color;
   my ( $bw, $hs, $vs) = $self-> prf(qw( borderWidth hScroll vScroll));
   $canvas-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,$bw,$self->dark3DColor,$self->light3DColor,$self-> backColor);
   $canvas-> color( $cl);
   my $sw = 12;
   my $sl = 16;
   $hs = $hs ? $sw : 0;
   $vs = $vs ? $sw : 0;
   my @r = ( $bw, $bw+$hs, $sz[0]-$bw-1-$vs,$sz[1]-$bw-1);
   if ( $hs) {
      $canvas-> rectangle( $r[0], $bw, $r[2], $r[1]);
      if ($r[0]+4+$sl < $r[2]-2-$sl) {
         $canvas-> rectangle( $r[0]+2, $bw+2, $r[0]+2+$sl, $r[1]-2);
         $canvas-> rectangle( $r[2]-2-$sl, $bw+2, $r[2]-2, $r[1]-2);
      }
   }
   if ( $vs) {
      $canvas-> rectangle( $sz[0]-$bw-1-$sw,$hs+2,$sz[0]-$bw-1,$r[3]-0);
      if ( $r[3]-$sl > $hs+2+$sl) {
         $canvas-> rectangle( $sz[0]-$bw+1-$sw,$hs+4,$sz[0]-$bw-3,$hs+2+$sl);
         $canvas-> rectangle( $sz[0]-$bw+1-$sw,$r[3]-2-$sl,$sz[0]-$bw-3,$r[3]-2);
      }
   }
   return if ( $r[0] > $r[2]) || ( $r[1] >= $r[3]);
   return @r;
}

package Prima::VB::ListBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);


sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool    => ['autoWidth', 'vScroll','hScroll','multiSelect','extendedSelect',
                  'autoHeight','integralHeight','multiColumn'],
      uiv     => ['itemHeight','itemWidth','focusedItem','borderWidth','offset','topItem',],
      color   => ['gridColor',],
      items   => ['items', 'selectedItems'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (offset);
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @r = $self->paint_exterior( $canvas);
   $canvas-> draw_text( join("\n", @{$self-> prf('items')}), @r,
      dt::NoWordWrap |
      dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
      dt::UseClip
   )  if scalar @r;
   $self-> common_paint($canvas);
}

sub prf_borderWidth   { $_[0]->repaint; }
sub prf_items         { $_[0]->repaint; }
sub prf_integralHeight{ $_[0]->repaint; }
sub prf_hScroll       { $_[0]->repaint; }
sub prf_vScroll       { $_[0]->repaint; }

package Prima::VB::DirectoryListBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::ListBox);


sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      uiv     => ['indent'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (items itemWidth itemHeight);
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @r = $self->paint_exterior( $canvas);
   $canvas-> draw_text( "usr\n\tlocal\n\t\tshare\n\t\t\texamples\n\t\t\t\tetc", @r,
      dt::NoWordWrap | dt::ExpandTabs |
      dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
      dt::UseClip
   )  if scalar @r;
   $self-> common_paint($canvas);
}



package Prima::VB::ScrollBar;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool    => [qw(autoTrack vertical)],
      uiv     => [qw(minThumbSize pageStep partial step whole)],
      iv      => [qw(min max value)],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   my $cl = $self-> color;
   my ( $v) = $self-> prf(qw( vertical));
   $canvas-> color( $self-> backColor);
   $canvas-> bar(0,0,@sz);
   $canvas-> color( $cl);
   $canvas-> rectangle(0,0,$sz[0]-1,$sz[1]-1);
   my $ws = 18;
   my $ex = 2;
   if ( $sz[ $v ? 1 : 0] > $ws * 2) {
      if ( $v) {
         $canvas-> rectangle(2,2,$sz[0]-3,2+$ws);
         $canvas-> ellipse($sz[0]/2-2,$ws/2+2,$ex,$ex);
         $canvas-> rectangle(2,$sz[1]-3-$ws,$sz[0]-3,$sz[1]-3);
         $canvas-> ellipse($sz[0]/2-2,$sz[1]-$ws/2-3,$ex,$ex);
      } else {
         $canvas-> rectangle(2,2,2+$ws,$sz[1]-3);
         $canvas-> ellipse($ws/2+2,$sz[1]/2-2,$ex,$ex);
         $canvas-> rectangle($sz[0]-3-$ws,2,$sz[0]-3,$sz[1]-3);
         $canvas-> ellipse($sz[0]-$ws/2-3,$sz[1]/2-2,$ex,$ex);
      }
   }
   $self-> common_paint($canvas);
}

sub prf_vertical   { $_[0]->repaint; }


package Prima::VB::ComboBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool       => [qw(caseSensitive literal autoSelect autoHeight)],
      uiv        => [qw(entryHeight listHeight focusedItem)],
      comboStyle => ['style'],
      string     => ['text'],
      items      => ['items'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
       writeOnly        autoWidth       selectedItems
       selection        vScroll         borderWidth
       readOnly         hScroll         alignment
       insertMode       multiSelect     maxLen
       autoTab          extendedSelect  buttonClass
       selection        integralHeight  buttonProfile
       selStart         multiColumn     editClass
       selEnd           itemHeight      editProfile
       firstChar        itemWidth       listProfile
       charOffset       offset          listVisible
       passwordChar     topItem         listClass
       wordDelimiters   gridColor
   );
}



sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   my $cl = $self-> color;
   my @c3d = ( $self->dark3DColor, $self->light3DColor);
   my $s = $self->prf('style') == cs::Simple;
   my $szy = $s ? ( $sz[1]-$canvas->font->height - 5) : 0;
   $canvas-> rect3d( 0,$szy,$sz[0]-1,$sz[1]-1,1,$c3d[0],$c3d[1],$self->backColor);
   unless ( $s) {
      $canvas-> rect3d( $sz[0]-18,0,$sz[0]-1,$sz[1]-1,2,$c3d[1],$c3d[0],$self->backColor);
      $canvas-> color( cl::Black);
      $canvas-> fillpoly([
         $sz[0]-12,$sz[1] * 0.6,
         $sz[0]-8,$sz[1] * 0.6,
         $sz[0]-10,$sz[1] * 0.3,
      ]);
   }

   $canvas-> color( $cl);
   $canvas-> draw_text( $self-> text, 2, $szy + 2, $sz[0] - 3 - ( $s ? 0 : 17), $sz[1] - 3,
      dt::Left|dt::VCenter|dt::NoWordWrap|dt::UseClip|dt::ExpandTabs);
   if ( $s) {
      $canvas-> rect3d( 0,0,$sz[0]-1,$szy-1,2,$c3d[0],$c3d[1],$self->backColor);
      my $i = $self->prf('items');
      $canvas-> draw_text( join("\n", @$i),2,2,$sz[0]-3,$szy-3,
         dt::Left|dt::Top|dt::NoWordWrap|dt::UseClip|dt::NewLineBreak|dt::UseExternalLeading);
   }
   $self-> common_paint($canvas);
}

sub prf_style         { $_[0]->repaint; }
sub prf_items         { $_[0]->repaint; }
sub prf_borderWidth   { $_[0]->repaint; }

package Prima::VB::DriveComboBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::ComboBox);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      char       => ['drive'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      items
   );
}

package Prima::VB::ColorComboBox;
use vars qw(@ISA);
@ISA = qw(Prima::VB::ComboBox);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      color      => ['value'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      items
   );
}


package Prima::VB::Edit;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool    => [qw(autoIndent cursorWrap insertMode hScroll vScroll
            persistentBlock readOnly syntaxHilite wantTabs wantReturns wordWrap
         )],
      uiv     => [qw(borderWidth tabIndent)],
      editBlockType => ['blockType',],
      color   => [qw(hiliteNumbers hiliteQStrings hiliteQQStrings hiliteIDs hiliteChars hiliteREs)],
      string  => ['wordDelimiters',],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      accelItems     selection
      cursorX        markers
      cursorY        textRef
      modified       firstCol
      offset
      selection
      selEnd
      selStart
   );
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @r = $self->paint_exterior( $canvas);
   $canvas-> draw_text( $self->text, @r,
      ( $self->prf('wordWrap') ? dt::WordWrap : dt::NoWordWrap) |
      dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
      dt::UseClip | dt::ExpandTabs
   ) if scalar @r;
   $self-> common_paint($canvas);
}

sub prf_borderWidth   { $_[0]->repaint; }
sub prf_hScroll       { $_[0]->repaint; }
sub prf_vScroll       { $_[0]->repaint; }
sub prf_wordWrap      { $_[0]->repaint; }

package Prima::VB::ImageViewer;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

use Prima::ImageViewer;

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool    => [qw(hScroll vScroll quality)],
      uiv     => [qw(borderWidth zoom)],
      image   => ['image'],
      align   => ['alignment',],
      valign  => ['valignment',],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      deltaX imageFile
      deltaY
      limitX
      limitY
   );
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @r = $self->paint_exterior( $canvas);
   my $i = $self->prf('image');
   if ( $i) {
      $canvas-> clipRect( $r[0], $r[1], $r[2] - 1, $r[3] - 1);
      my ( $a, $v, $z) = $self-> prf(qw(alignment valignment zoom));
      my ( $ix, $iy) = $i-> size;
      my ( $izx, $izy) = ( $ix * $z, $iy * $z);
      my ( $ax, $ay);
      $a = ta::Left if $izx >= $r[2] - $r[0];
      $v = ta::Top  if $izy >= $r[3] - $r[1];
      if ( $a == ta::Left) {
         $ax = $r[0];
      } elsif ( $a == ta::Center) {
         $ax = $r[0] + ( $r[2] - $r[0] - $izx) / 2;
      } else {
         $ax = $r[2] - $izx;
      }
      if ( $v == ta::Bottom) {
         $ay = $r[1];
      } elsif ( $v == ta::Middle) {
         $ay = $r[1] + ( $r[3] - $r[1] - $izy) / 2;
      } else {
         $ay = $r[3] - $izy;
      }
      $canvas-> stretch_image( $ax, $ay, $izx, $izy, $i);
   }
   $self-> common_paint($canvas);
}

sub prf_borderWidth   { $_[0]->repaint; }
sub prf_hScroll       { $_[0]->repaint; }
sub prf_vScroll       { $_[0]->repaint; }
sub prf_image         { $_[0]->repaint; }
sub prf_alignment     { $_[0]->repaint; }
sub prf_valignment    { $_[0]->repaint; }
sub prf_zoom          { $_[0]->repaint; }


package Prima::VB::ScrollWidget;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool    => [qw(hScroll vScroll)],
      uiv     => [qw(borderWidth deltaX deltaY limitX limitY)],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   $self->paint_exterior( $canvas);
   $self-> common_paint( $canvas);
}

sub prf_borderWidth   { $_[0]->repaint; }
sub prf_hScroll       { $_[0]->repaint; }
sub prf_vScroll       { $_[0]->repaint; }

use Prima::Sliders;

package Prima::VB::SpinButton;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
   my @size = $canvas-> size;
   $canvas-> rect3d( 0, 0, $size[0] - 1, $size[1] * 0.4 - 1, 2, @c3d, $clr[1]);
   $canvas-> rect3d( 0, $size[1] * 0.4, $size[0] - 1, $size[1] * 0.6 - 1, 2, @c3d, $clr[1]);
   $canvas-> rect3d( 0, $size[1] * 0.6, $size[0] - 1, $size[1] - 1, 2, @c3d, $clr[1]);
   $canvas-> color( $clr[0]);
   $canvas-> fillpoly([
      $size[0] * 0.3, $size[1] * 0.73,
      $size[0] * 0.5, $size[1] * 0.87,
      $size[0] * 0.7, $size[1] * 0.73
   ]);
   $canvas-> fillpoly([
      $size[0] * 0.3, $size[1] * 0.27,
      $size[0] * 0.5, $size[1] * 0.13,
      $size[0] * 0.7, $size[1] * 0.27
   ]);
   $self-> common_paint( $canvas);
}

package Prima::VB::AltSpinButton;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
   my @size = $canvas-> size;
   $canvas-> color( $clr[ 1]);
   $canvas-> bar( 0, 0, $size[0]-1, $size[1]-1);

   $canvas-> color( $c3d[ 0]);
   $canvas-> line( 0, 0, 0, $size[1] - 1);
   $canvas-> line( 1, 1, 1, $size[1] - 2);
   $canvas-> line( 2, $size[1] - 2, $size[0] - 3, $size[1] - 2);
   $canvas-> line( 1, $size[1] - 1, $size[0] - 2, $size[1] - 1);

   $canvas-> color( $c3d[ 1]);
   $canvas-> line( 1, 0, $size[0] - 1, 0);
   $canvas-> line( 2, 1, $size[0] - 1, 1);
   $canvas-> line( $size[0] - 2, 1, $size[0] - 2, $size[1] - 2);
   $canvas-> line( $size[0] - 1, 1, $size[0] - 1, $size[1] - 1);

   $canvas-> color( $c3d[ 1]);
   $canvas-> line( -1, 0, $size[0] - 2, $size[1] - 1);
   $canvas-> line( 0, 0, $size[0] - 1, $size[1] - 1);
   $canvas-> color( $c3d[ 0]);
   $canvas-> line( 1, 0, $size[0], $size[1] - 1);

   $canvas-> color( $clr[0]);
   $canvas-> fillpoly([ $size[0] * 0.2, $size[1] * 0.65,
                      $size[0] * 0.3, $size[1] * 0.77,
                      $size[0] * 0.4, $size[1] * 0.65]);
   $canvas-> fillpoly([ $size[0] * 0.6, $size[1] * 0.35,
                      $size[0] * 0.7, $size[1] * 0.27,
                      $size[0] * 0.8, $size[1] * 0.35]);
   $self-> common_paint( $canvas);
}



package Prima::VB::SpinEdit;
use vars qw(@ISA);
@ISA = qw(Prima::VB::InputLine);


sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      align   => ['alignment',],
      bool    => ['writeOnly','readOnly','insertMode','autoSelect','autoTab','firstChar','charOffset'],
      upoint  => ['selection',],
      uiv     => [qw(min max step value)],
      string  => [qw(spinClass editClass)],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw (
      selection        passwordChar
      selEnd           writeOnly
      selStart
      charOffset
      firstChar
   );
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   my $cl = $self-> color;
   my ( $a, $bw) = $self-> prf(qw( alignment borderWidth));
   $canvas-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,$bw,$self->dark3DColor,$self->light3DColor,$self-> backColor);
   $canvas-> rect3d( $sz[0]-$sz[1]-$bw,$bw,$sz[0]-1,$sz[1]-1,2,$self->light3DColor,$self->dark3DColor);
   $canvas-> color( $cl);
   $a = (ta::Left == $a ? dt::Left : (ta::Center == $a ? dt::Center : dt::Right));
   my $c = $self->prf('value');
   $canvas-> draw_text($c,2,2,$sz[0]-$sz[1]-$bw*2,$sz[1]-3,$a|dt::VCenter|dt::UseClip|dt::ExpandTabs|dt::NoWordWrap);
   $self-> common_paint($canvas);
}

sub prf_alignment     { $_[0]->repaint; }
sub prf_borderWidth   { $_[0]->repaint; }

package Prima::VB::Gauge;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool         => ['vertical'],
      uiv          => [qw(min max value threshold indent)],
      gaugeRelief  => ['relief'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my ($x, $y) = $canvas-> size;
   my ($i, $relief, $v, $val, $min, $max, $l3, $d3) =
      $self-> prf(qw(indent relief vertical value min max light3DColor dark3DColor));
   my ($clComplete,$clHilite,$clBack,$clFore) = ($self-> prf('hiliteBackColor', 'hiliteColor'), $self-> backColor, $self-> color);
   my $complete = $v ? $y : $x;
   my $ediv = $max - $min;
   $ediv = 1 unless $ediv;
   $complete = int(($complete - $i*2) * $val / $ediv + 0.5);
   $canvas-> color( $clComplete);
   $canvas-> bar ( $v ? ($i, $i, $x-$i-1, $i+$complete) : ( $i, $i, $i + $complete, $y-$i-1));
   $canvas-> color( $clBack);
   $canvas-> bar ( $v ? ($i, $i+$complete+1, $x-$i-1, $y-$i-1) : ( $i+$complete+1, $i, $x-$i-1, $y-$i-1));
   # draw the border
   $canvas-> color(( $relief == gr::Sink) ? $d3 : (( $relief == gr::Border) ? cl::Black : $l3));
   for ( my $j = 0; $j < $i; $j++)
   {
      $canvas-> line( $j, $j, $j, $y - $j - 1);
      $canvas-> line( $j, $y - $j - 1, $x - $j - 1, $y - $j - 1);
   }
   $canvas-> color(( $relief == gr::Sink) ? $l3 : (( $relief == gr::Border) ? cl::Black : $d3));
   for ( my $j = 0; $j < $i; $j++)
   {
      $canvas-> line( $j + 1, $j, $x - $j - 1, $j);
      $canvas-> line( $x - $j - 1, $j, $x - $j - 1, $y - $j - 1);
   }

   # draw the text, if neccessary
   my $s = sprintf( "%2d%%", $val * 100.0 / $ediv);
   my ($fw, $fh) = ( $canvas-> get_text_width( $s), $canvas-> font-> height);
   my $xBeg = int(( $x - $fw) / 2 + 0.5);
   my $xEnd = $xBeg + $fw;
   my $yBeg = int(( $y - $fh) / 2 + 0.5);
   my $yEnd = $yBeg + $fh;
   my ( $zBeg, $zEnd) = $v ? ( $yBeg, $yEnd) : ( $xBeg, $xEnd);
   if ( $zBeg > $i + $complete)
   {
      $canvas-> color( $clFore);
      $canvas-> text_out( $s, $xBeg, $yBeg);
   }
   elsif ( $zEnd < $i + $complete + 1)
   {
      $canvas-> color( $clHilite);
      $canvas-> text_out( $s, $xBeg, $yBeg);
   }
   else
   {
      $canvas-> clipRect( $v ? ( 0, 0, $x, $i + $complete) : ( 0, 0, $i + $complete, $y));
      $canvas-> color( $clHilite);
      $canvas-> text_out( $s, $xBeg, $yBeg);
      $canvas-> clipRect( $v ? ( 0, $i + $complete + 1, $x, $y) : ( $i + $complete + 1, 0, $x, $y));
      $canvas-> color( $clFore);
      $canvas-> text_out( $s, $xBeg, $yBeg);
   }
   $self-> common_paint( $canvas);
}

sub prf_min           { $_[0]->repaint; }
sub prf_max           { $_[0]->repaint; }
sub prf_value         { $_[0]->repaint; }
sub prf_indent        { $_[0]->repaint; }
sub prf_relief        { $_[0]->repaint; }
sub prf_vertical      { $_[0]->repaint; }


package Prima::VB::AbstractSlider;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool         => [qw(readOnly snap)],
      uiv          => [qw(min max value increment step)],
      sliderScheme => ['scheme'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub prf_adjust_default
{
   my ( $self, $p, $pf) = @_;
   $self-> SUPER::prf_adjust_default( $p, $pf);
   delete $pf->{$_} for qw ( ticks);
}

package Prima::VB::Slider;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractSlider);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool         => [qw(ribbonStrip vertical)],
      uiv          => [qw(shaftBreadth)],
      align        => [qw(tickAlign)],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my ($x, $y) = $canvas-> size;
   my ( $f, $b) = ( $self-> color, $self-> backColor);
   my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
   $canvas-> color( $b);
   $canvas-> bar(0,0,$x,$y);
   $canvas-> color( $f);
   if ( $self-> prf('vertical')) {
      $canvas-> rect3d( $x * 0.4, 0, $x * 0.6, $y - 1, 1, @c3d);
      $canvas-> rect3d( $x * 0.25, 3, $x * 0.75, 20, 1, @c3d, $b);
   } else {
      $canvas-> rect3d( 0, $y * 0.4, $x - 1, $y * 0.6, 1, @c3d);
      $canvas-> rect3d( 3, $y * 0.25, 20, $y * 0.75, 1, @c3d, $b);
   }
   $self-> common_paint( $canvas);
}

sub prf_vertical  { $_[0]->repaint; }

package Prima::VB::CircularSlider;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractSlider);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      bool         => [qw(buttons stdPointer)],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my ($x, $y) = $canvas-> size;
   my ( $f, $b) = ( $self-> color, $self-> backColor);
   my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
   $canvas-> color( $b);
   $canvas-> bar(0,0,$x,$y);
   $canvas-> color( $f);
   my ( $cx, $cy) = ( $x/2, $y/2);
   my $rad = ($x < $y) ? $cx : $cy;
   $rad -= 3;
   $canvas-> lineWidth(2);
   $canvas-> color( $c3d[0]);
   $canvas-> arc( $cx, $cy, $rad, $rad, 65, 235);
   $canvas-> color( $c3d[1]);
   $canvas-> arc( $cx, $cy, $rad, $rad, 255, 405);
   $canvas-> lineWidth(1);
   $self-> common_paint( $canvas);
}


1;
