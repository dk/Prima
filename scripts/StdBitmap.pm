package StdBitmap;
use strict;
use Const;
require Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter);
$VERSION = '1.00';
@EXPORT = qw();
@EXPORT_OK = qw(icon image);
%EXPORT_TAGS = ();

my %bmCache = ();
my $bmImageFile = undef;

{
   my $scriptPath = (grep { m/Script$/ } @INC)[-1];
   ( my $imagePath = $scriptPath) =~ s/Script$/Images/;
   $bmImageFile = "$imagePath/sysimage.gif";
}

sub load_std_bmp
{
   my ( $index, $asIcon, $copy) = @_;
   my $class = ( $asIcon ? q(Icon) : q(Image));
   return undef if $index < 0 || $index > sbmp::Last;
   $asIcon = ( $asIcon ? 1 : 0);
   if ( $copy)
   {
      my $i = $class-> create(name => $index);
      undef $i unless $i-> load( $bmImageFile, index => $index);
      return $i;
   }
   return $bmCache{$index}->[$asIcon] if exists $bmCache{$index} && defined $bmCache{$index}->[$asIcon];
   $bmCache{$index} = [ undef, undef] unless exists $bmCache{$index};
   my $i = $class-> create(name => $index);
   undef $i unless $i-> load( $bmImageFile, index => $index);
   $bmCache{$index}->[$asIcon] = $i;
   return $i;
}

sub icon { return load_std_bmp( $_[0], 1, 0); }
sub image{ return load_std_bmp( $_[0], 0, 0); }

1;
