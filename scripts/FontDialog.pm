package FontDialog;

use strict;
use Const;
use Classes;
use Buttons;
use ComboBox;
use Label;
use MsgBox;


use vars qw( @ISA);
@ISA = qw( Dialog);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      width       => 540,
      height      => 350,
      centered    => 1,
      visible     => 0,
      designScale => [7, 16],

      showHelp    => 0,
      fixedOnly   => 0,
      logFont     => $_[ 0]-> get_default_font,
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   $p-> { logFont} = {} unless exists $p->{ logFont};
   $p-> { logFont} = Drawable-> font_match( $p->{ logFont}, $default->{ logFont}, 0);
}


sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);

   $self-> {showHelp}  = $profile{showHelp};
   $self-> {logFont}   = $profile{logFont};
   $self-> {fixedOnly} = $profile{fixedOnly};

   my $gr = $self-> insert( CheckBoxGroup =>
      origin => [ 10, 10],
      size   => [ 150, 150],
      name   => 'Style',
   );

   $gr-> insert( CheckBox =>
      origin => [ 15, 95],
      size   => [ 96, 36],
      name   => 'FontStyleButton',
      text   => '~Bold',
      delegateTo => $self,
   );

   $gr-> insert( CheckBox =>
      origin => [ 15, 65],
      size   => [ 96, 36],
      name   => 'FontStyleButton',
      text   => '~Italic',
      delegateTo => $self,
   );

   $gr-> insert( CheckBox =>
      origin => [ 15, 35],
      size   => [ 96, 36],
      name   => 'FontStyleButton',
      text   => '~Underline',
      delegateTo => $self,
   );

   $gr-> insert( CheckBox =>
      origin => [ 15, 5],
      size   => [ 96, 36],
      name   => 'FontStyleButton',
      text   => 'Strike ~out',
      delegateTo => $self,
   );

   my $name = $self-> insert( ComboBox =>
      origin => [ 10, 165],
      size   => [ 250, 150],
      name   => 'Name',
      style  => cs::Simple,
      onSelectItem => sub { $self-> Name_SelectItem( @_);},
   );

   $self-> insert( Label =>
      origin    => [ 10, 320],
      size      => [ 96, 18],
      text      => '~Font:',
      focusLink => $name,
   );

   my $size = $self-> insert( ComboBox =>
      origin => [ 275, 165],
      size   => [ 150, 150],
      name   => 'Size',
      style  => cs::Simple,
   );

   $self-> insert( Label =>
      origin    => [ 275, 320],
      size      => [ 96, 18],
      text      => '~Size:',
      focusLink => $size,
   );

   $gr = $self-> insert( GroupBox =>
      origin     => [ 175, 10],
      size       => [ 250, 150],
      name       => 'Sample',
   );

   $gr-> insert( Widget =>
      origin     => [ 5, 5],
      size       => [ 240, 120],
      name       => 'Example',
      delegateTo => $self,
   );

   $self-> insert( Button =>
      origin      => [ 435, 280],
      size        => [ 96, 36],
      text        => '~OK',
      default     => 1,
      modalResult => cm::OK,
   );

   $self-> insert( Button =>
      origin      => [ 435, 235],
      size        => [ 96, 36],
      text        => 'Cancel',
      modalResult => cm::Cancel,
   );

   $self->insert( Button =>
      origin      => [ 435, 190],
      size        => [ 96, 36],
      name        => 'Help',
      text        => '~Help',
   ) if $self->{showHelp};

   $self-> refresh_fontlist;
   $self-> logfont_to_view;
   $self-> apply( %{$self->{logFont}});

   return %profile;
}

sub refresh_fontlist
{
   my $self = $_[0];
   my %fontList  = ();
   my @fontItems = ();

   for ( sort { $a->{name} cmp $b->{name}} @{$::application-> fonts})
   {
      next;
      next if $self->{fixedOnly} and $_->{pitch} != fp::Fixed;
      $fontList{$_->{name}} = $_;
      push ( @fontItems, $_->{name});
   }

   $self->{fontList}  = \%fontList;
   $self->{fontItems} = \@fontItems;

   $self-> Name-> items( \@fontItems);
   $self-> Name-> text( $self->{logFont}->{name});
   $self-> reset_sizelist;
}

sub reset_sizelist
{
   my $self = $_[0];
   my $Name = $self-> Name;
   my $fn   = $Name-> List-> get_items( $Name-> focusedItem);
   my @sizes = ();

   if ( defined $fn) {
      my @list = @{$::application-> fonts( $fn)};
      for ( @list)
      {
         if ( $_->{ vector})
         {
            @sizes = qw( 8 9 10 11 12 14 16 18 20 22 24 26 28 32 48 72);
            last;
         } else {
            push ( @sizes, $_->{size});
         }
      }
      @sizes = sort { $a <=> $b } keys %{{(map { $_ => 1 } @sizes)}};
   }
   $self-> Size-> items( \@sizes);
   $self-> Size-> focusedItem(0);
   return $fn;
}

sub logfont_to_view
{
   my $self = $_[0];
   $self-> Name-> text( $self->{logFont}-> {name});
   $self-> Size-> text( $self->{logFont}-> {size});
   my $grp = $self-> Style;
   my $style = $self->{logFont}-> {style};
   $grp-> value( 0 |
     (( $style & fs::Bold)       ? 1 : 0) |
     (( $style & fs::Italic)     ? 2 : 0) |
     (( $style & fs::Underlined) ? 4 : 0) |
     (( $style & fs::StruckOut)  ? 8 : 0)
  );
}

sub apply
{
   my ( $self, %hash) = @_;
   delete $hash{$_} for ( qw( width height direction));
   $self-> {logFont} = $self-> font_match( \%hash, $self-> {logFont}, 0);
   delete $self->{logFont}->{$_} for ( qw( width height direction));
   $self->{fixedOnly} ? $self->{logFont}->{pitch} = fp::Fixed : delete $self->{logFont}->{pitch};
   $self-> { normalFontSet} = 1;
   $self-> Sample-> Example-> font( $self-> {logFont});
   delete $self-> { normalFontSet};
}

sub Example_Paint
{
   my ( $owner, $self, $canvas) = @_;
   my @size = $canvas-> size;
   $canvas-> color( $canvas-> get_nearest_color( $self-> backColor));
   $canvas-> bar( 0, 0, @size);
   $canvas-> color( cl::Black);
   my $f = $self-> font;
   my $line = 'AaBbYyZz';
   $canvas-> text_out(
      $line,
      ( $size[0] - $canvas-> get_text_width( $line)) / 2,
      ( $size[1] - $f-> height) / 2
   );
}

sub Example_FontChanged
{
   my ( $owner, $example) = @_;
   return if $owner-> {normalFontSet};
   $owner-> logFont( $example-> font);
}

sub Name_SelectItem
{
   my ( $owner, $self, $index, $state) = @_;
   my $sz = $owner-> {logFont}->{size};
   my $fn = $owner-> reset_sizelist;
   $owner-> apply( name => $fn, size => $sz);
}

sub Size_Change
{
   my ( $owner, $self) = @_;
   my $sz = $self-> text;
   return unless $sz =~ m/^\s*([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?\s*$/;
   return if $sz < 2 or $sz > 2048;
   $owner-> apply( size => $sz);
}

sub FontStyleButton_Click
{
   my $self = $_[0];
   my $v    = $self-> Style-> value;
   $self -> apply( style => 0 |
      (( $v & 1) ? fs::Bold       : 0) |
      (( $v & 2) ? fs::Italic     : 0) |
      (( $v & 4) ? fs::Underlined : 0) |
      (( $v & 8) ? fs::StruckOut  : 0)
   );
}


sub set_fixed_only
{
   my ( $self, $fo) = @_;
   return if $fo == $self->{fixedOnly};
   $self->{fixedOnly} = $fo;
   $self-> refresh_fontlist;
}

sub logFont
{
   my $self = $_[0];
   return $self-> Sample-> Example-> get_font unless $#_;
   $self-> {logFont} = $self-> font_match( $_[1], $self->{logFont}, 1);
   $self-> logfont_to_view;
   $self-> apply( %{$self->{logFont}});
}

sub showHelp         { ($#_)? shift->raise_ro('showHelp')  : return $_[0]->{showHelp}};
sub fixedOnly        { ($#_)? shift->set_fixed_only($_[1]) : return $_[0]->{fixedOnly}};

1;
