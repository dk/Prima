# $ Id $
package StartupWindow;
use strict;
use vars qw(@windowStack @pointersStack);

sub import {
   shift;
   my %profileDefault = (
      size        => [$::application-> width/5,$::application-> height/6],
      centered    => 1,
      widgetClass => wc::Dialog,
      borderIcons => 0,
      borderStyle => bs::Dialog,
      color       => cl::Cyan,
      text        => 'Loading...',
      onPaint     => sub {
         my ($me,$canvas) = @_;
         my $fore = $me-> color;
         $canvas-> color( $me-> backColor);
         $canvas-> bar( 0, 0, $me-> size);
         $canvas-> color( $fore);
         $canvas-> font( name => "Tms Rmn", size => 24, style => fs::Bold);
         my $txt = $me-> text;
         $canvas-> text_out( $txt, ($me-> width - $me-> get_text_width($txt))/2, ($me-> height - $me-> font-> height)/2);
      },
   );
   my %profile = (%profileDefault,@_);
   push @pointersStack, $::application-> pointer;
   $::application-> pointer( cr::Wait);
   my $w = Window-> create(%profile);
   push @windowStack, $w;
   $::application-> yield;
   $w-> repaint;
   $w-> update_view;
}

sub unimport {
   return unless defined @windowStack;
   return if @windowStack <= 0;
   my $w = pop @windowStack;
   $w-> destroy;
   $::application-> pointer( pop @pointersStack);
}

1;
