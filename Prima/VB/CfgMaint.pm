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
package Prima::VB::CfgMaint;
use vars qw(@pages %classes $backup $userCfg $rootCfg $systemWide);

@pages      = ();
%classes    = ();
$backup     = 1;
$systemWide = 0;
$rootCfg    = 'Prima/VB/Config.pm';
$userCfg    = '.vbconfig';

my $file;
my $pkg;


sub reset_cfg
{
   @pages   = ();
   @classes = ();
}

sub open_cfg
{
   if ( $systemWide) {
      $file = undef;
      for (@INC) {
         $file = "$_/$rootCfg", last if -f "$_/$rootCfg" && -r _;
      }
      return (0, "Cannot find $rootCfg") unless $file;
      $pkg = "Prima::VB::Config";
      $file =~ s[\\][/]g;
      eval "require \"$file\";";
   } else {
      my $home = $ENV{HOME};
      return ( 0, 'HOME environment variable not set') unless defined $home;
      $home =~ s[\\][/]g;
      $home =~ s/\\$//;
      $home =~ s/\/$//;
      $file = "$home/$userCfg";
      $pkg  = "Prima::VB::UserConfig";
      return 1 unless -f $file;
      eval "require \"$home/$userCfg\";";
   }
   return (0,  "$@") if $@;

   @pages   = eval "$pkg".'::pages';
   my @csa  = eval "$pkg".'::classes';
   return ( 0, "Invalid package \'$pkg\'") if scalar @csa % 2;
   %classes = @csa;
   return 1;
}

sub read_cfg
{
   my $sw = $systemWide;
   my ( $f, $p);
   reset_cfg;
   $systemWide = 0;
   my @r = open_cfg;
   return @r unless $r[0];
   ( $f, $p) = ( $file, $pkg) if $sw == 0;
   my %cs = %classes;
   my @pg = @pages;
   $systemWide = 1;
   @r = open_cfg;
   return @r unless $r[0];
   ( $f, $p) = ( $file, $pkg) if $sw == 1;
   my %pgref = map { $classes{$_}-> {page} => 1} keys %classes;
   %classes = ( %classes, %cs);
   for ( @pg) {
      next if $pgref{$_};
      $pgref{$_} = 1;
      push( @pages, $_);
   }
   ($systemWide, $file, $pkg) = ( $sw, $f, $p);
   return 1;
}

sub write_cfg
{
   return ( 0, "Cannot write to $file") if -f $file && ! -w _;

   if ( $backup && -f $file) {
      local $/;
      open F, "$file" or return ( 0, "Cannot read $file");
      my $f = <F>;
      close F;
      open F, ">$file.bak" or return ( 0, "Cannot write backup $file.bak");
      print F $f;
      close F;
   }

   open F, ">$file" or return ( 0, "Cannot write to $file");
   my $c = <<HEAD;
package $pkg;

sub pages
{
   return qw(@pages);
}

sub classes
{
   return (
HEAD

   for ( keys %classes) {
      my %dt = %{$classes{$_}};
      $c .= "   $_ => {\n";
      my $maxln = 0;
      for ( keys %dt) {
         $maxln = length($_) if length( $_) > $maxln;
      }
      $c .= sprintf( "      %-${maxln}s => \'$dt{$_}\',\n", $_) for keys %dt;
      $c .= "   },\n";
   }

   $c .= <<HEAD2;
   );
}


1;
HEAD2

   print F $c;
   close F;
   return 1;
}

sub add_package
{
   my $file = $_[0];
   my $pkg;

   return ( 0, "Cannot open package $file") unless open F, $file;
   while ( <F>) {
      next if /^#/;
      next unless /package\s+([^\s;].*)/m;
      $pkg = $1;
      $pkg =~ s[[\s;]*$][];
      last;
   }
   close F;
   return ( 0, "Cannot locate 'package' section in $file") unless defined $pkg;

   eval "use $pkg;";
   if ( $@) {
      my $err = "$@";
      if ( $err =~ /Can\'t locate\s([^\s]+)\sin/) {
         $err = "Corrupted package $file - internal and file names doesn't match";
      }
      return ( 0, $err);
   }
   my @clsa = eval "$pkg".'::classes';
   return ( 0, "$@") if $@;
   return ( 0, "Invalid package \'$pkg\'") if scalar @clsa % 2;
   my %cls = @clsa;
   $cls{$_}-> {module} = $pkg for keys %cls;
   my %pgref = map { $classes{$_}-> {page} => 1} keys %classes;
   %classes = ( %classes, %cls);
   for ( keys %cls) {
      my $pg = $cls{$_}-> {page};
      next if $pgref{$pg};
      $pgref{$pg} = 1;
      push @pages, $pg;
   }
   return 1;
}


1;
