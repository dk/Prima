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
#  Created by Dmitry Karasik <dk@plab.ku.dk>

use strict;
use Config;
use Prima::Config;
require 5.00502;
use File::Find;
use File::Basename;
use File::Path;
use File::Copy;
use DynaLoader;
use Cwd;
use Exporter;
use vars qw(@ISA @EXPORT);
@ISA = qw(Exporter);
@EXPORT = qw( init qd dl_name quoted_split setvar env_true addlib
          canon_name find_file find_cdeps cc_command_line ld_command_line generate_def);

use vars qw( %make_trans @ovvars $dir_sep $path_sep );

use vars @ovvars = qw(
    @INCPATH
    @LIBPATH
    @LIBS
    $PREFIX
    $INSTALL_BIN
    $INSTALL_LIB
    $INSTALL_DL
    $INSTALL_EXAMPLES
    $DEBUG
);

use vars qw(
    $Win32
    $OS2
    $Unix
    %DEFINES
    %USER_VARS
    %USER_VARS_ADDONS
    %USER_VARS_LINKS
    %USER_DEFINES
    %USER_DEFINES_ADDONS
    %overrideable
    $ARGV_STR
    @allclean
    @allrealclean
    @allobjects
    %alldeps
    %mapheader
    @allinstall
    @allbins
    @alldirs
    @allsubtargets
    @alllibtargets
    @subtargetsdirs
);


$OS2   = $Prima::Config::Config{platform} eq 'os2';
$Win32 = $Prima::Config::Config{platform} eq 'win32';
$Unix  = $Prima::Config::Config{platform} eq 'unix';

sub init
{
   my @argv = @_;
   if ( $#argv >= 0 && $argv[ 0] =~ /^\-\-cp(bin)?$/) {
       my $isbin = defined $1;
       shift @argv;
       die qq(Even number of parameters expected) unless ( $#argv % 2);
       while ( scalar @argv) {
           my ( $src, $dst) = ( shift @argv, shift @argv);
           print qq(Installing $src -> $dst\n);
           next unless -f $src;
           if ( $isbin) {
               my $dstdir = dirname( $dst);
               mkpath $dstdir if $dstdir && ! -d $dstdir;
               open SRCPL, "<$src" or die "Cannot open $src: $!";
               open DSTPL, ">$dst" or die "Cannot create $dst: $!";
               chmod 0555, $dst unless $Win32 || $OS2;
               if ( $Win32) {
                   print DSTPL <<EOF;
\@rem = '--*-Perl-*--
\@echo off
if "%OS%" == "Windows_NT" goto WinNT
$^X -w -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
$^X -w -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\\system32\\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
\@rem ';
#!perl
EOF
               }
               elsif ( $OS2) {
                   my $perlpath = $Config{ perlpath};
                   $perlpath =~ s/(perl)(\.exe)?$/$1__$2/i if $perlpath =~ /perl(\.exe)?$/i;
                   print DSTPL <<EOF;
extproc $perlpath -wS
EOF
               }
               else {
                   print DSTPL <<EOF;
#! $Config{ perlpath} -w
EOF
               }
               my $filestart = 1;
               while ( <SRCPL>) {
                   next if $filestart && /^\#\!/;
                   $filestart = 0;
                   print DSTPL;
               }
               if ( $Win32) {
                   print DSTPL <<EOF;
__END__
:endofperl
EOF
               }
               close SRCPL;
               close DSTPL;
           }
           else {
               die qq(Destination must be a directory) if -e $dst && ! -d $dst;
               mkpath $dst unless -d $dst;
               if ( $OS2) {
                   $src =~ /(?:^|[\\\/])([^\\\/]*)$/;
                   unlink $dst.'\\'.$1 if defined $1;
               }
               copy( $src, $dst) or die qq(Copy failed: $!);
           }
       }
       return 0;
   }
   elsif ( $#argv >= 0 && $argv[ 0] eq '--md') {
       shift @argv;
       mkpath \@argv;
       return 0;
   }
   elsif ( $#argv >= 0 && $argv[ 0] eq '--rm') {
       shift @argv;
       unlink @argv;
       return 0;
   }

   %USER_VARS_LINKS = (
      INSTALL_BIN => {
          src => 'PREFIX',
          format => '%s/bin',
      },
   );

   %overrideable = map { /^(.)(.*)$/; $2 => $1} @ovvars; # Script variables which may be overridden.
   $dir_sep   = $Prima::Config::Config{ifs};
   $path_sep  = $Config{path_sep};
   %DEFINES  = map { my @z = split('=', $_, 2); scalar(@z)?(@z):($_ => 1)} @{$Prima::Config::Config{cdefs}};
   $mapheader{'apricot.h'} = undef;
   $ARGV_STR = join( " ", map { "\"$_\""} @argv);
   foreach my $arg ( @argv) {
       if ( $arg =~ /^\s*(\w+)\s*(\+?)\=(.*)$/) {
           my ( $varname, $setmode, $varval) = ( $1, $2 || '', $3);
           die "Unknown variable $varname" unless $overrideable{ $varname};
           if ( $overrideable{ $varname} eq '$') {
               if ( $setmode eq '+') {
                   push @{ $USER_VARS_ADDONS{ $varname}}, $varval;
               }
               else {
                   $USER_VARS{ $varname} = $varval;
               }
               die $@ if $@;
           }
           elsif ( $overrideable{ $varname} eq '@') {
               my @values = split /$path_sep/, $varval;
               if ( $setmode eq '+') {
                   push @{ $USER_VARS_ADDONS{ $varname}}, @values;
               }
               else {
                   $USER_VARS{ $varname} = @values;
               }
               die $@ if $@;
           }
       }
       elsif ( $arg =~ /^-(D|U)(\w+)(?:(\+?)=(.*))?$/) {
           my ( $defmode, $defname, $setmode, $value) = ( $1, $2, $3 || '', $4 || '');
           if ( $defmode eq 'U') {
               $USER_DEFINES{ $defname} = undef; # Still exists in the hash...
           }
           else {
               if ( $setmode eq '+') {
                   push @{ $USER_DEFINES_ADDONS{ $defname}}, $value;
               }
               else {
                   $USER_DEFINES{ $defname} = $value;
               }
           }
       }
       else {
           die "Unknown command line argument or wrong syntax: '$arg'";
       }
   }

   foreach my $defname ( keys %USER_DEFINES) {
      if ( defined $USER_DEFINES{ $defname}) {
          $DEFINES{ $defname} = $USER_DEFINES{ $defname};
      }
      else {
          delete $DEFINES{ $defname} if exists $DEFINES{ $defname};
       }
   }
   foreach my $defname ( keys %USER_DEFINES_ADDONS) {
       $DEFINES{ $defname} = ( $DEFINES{ $defname} || '') . join( '', @{ $USER_DEFINES_ADDONS{ $defname}});
   }
   setvar('INCPATH', @{$Prima::Config::Config{incpaths}});
   setvar('LIBPATH', @{$Prima::Config::Config{ldpaths}});
   setvar('LIBS',    @{$Prima::Config::Config{ldlibs}});
   setvar( 'DEBUG', 0);
   setvar( 'PREFIX', $Config{ installsitearch});
   setvar( 'INSTALL_BIN', $Config{ installbin});

   return 1;
}



sub qd
{
    my ( $path_str) = @_;
    $path_str =~ s[/][$dir_sep]g;
    return $path_str;
}

sub dl_name
{
   my @n = split(/\:\:/, $_[0]);
   $n[-1] = &DynaLoader::mod2fname(\@n) if $OS2 && defined &DynaLoader::mod2fname;
   return $n[-1] . $Prima::Config::Config{dlext};
}


sub quoted_split
{
   my @neww = ();
   my $text = shift;
   while ($text =~ m<
       ("[^\"\\]*(?:\\.[^\"\\]*)*")\s*
     | (\S+)\s*
     | \s+
   >gx) {
          push @neww, $+ if defined $+;
   }
   return @neww;
}

sub setvar
{
    my ( $varname) = shift;
    die "Tried to set non-overrideable variable $varname" unless $overrideable{ $varname};
    if ( $overrideable{ $varname} eq '$') {
        if ( defined $USER_VARS{ $varname}) {
            eval "\$$varname = \$USER_VARS{ \$varname}";
        }
        elsif ( defined $USER_VARS_LINKS{ $varname}
                && exists $USER_VARS{ $USER_VARS_LINKS{ $varname}->{ src}}) {
            eval "\$$varname = sprintf \"$USER_VARS_LINKS{ $varname}->{ format}\", \$$USER_VARS_LINKS{ $varname}->{ src}";
        }
        else {
            eval "\$$varname = join( '', \@_)";
        }
        die $@ if $@;
        if ( defined $USER_VARS_ADDONS{ $varname}) {
            eval "\$$varname .= join( '', \@{ \$USER_VARS_ADDONS{ \$varname}})";
        }
        die $@ if $@;
    }
    elsif ( $overrideable{ $varname} eq '@') {
        if ( defined $USER_VARS{ $varname}) {
            eval "\@$varname = \$USER_VARS{ \$varname}";
        }
        else {
            eval "\@$varname = \@_";
        }
        die $@ if $@;
        if ( defined $USER_VARS_ADDONS{ $varname}) {
            eval "push \@$varname, \@{ \$USER_VARS_ADDONS{ \$varname}}";
        }
        die $@ if $@;
    }
    else {
        die "Unsupported type of variable";
    }
}

sub env_true
{
    my ( $var) = @_;
    return( $ENV{ $var} && $ENV{ $var} =~ /^1|yes|on|true$/i);
}

sub find_file
{
    my ( $fname, $dir) = @_;
    my $pathname = qd( "$dir/$fname");
    if ( -e $pathname) {
        return ( $pathname, 1);
    }
    opendir D, $dir or die "Cannot open dir $dir: $!";
    my @entries = map { qd( "$dir/$_")} grep { /^[^.]/ && -d qd( "$dir/$_")} readdir D;
    closedir D;
    my $found;
    foreach my $entry ( @entries) {
        ( $pathname, $found) = find_file( $fname, $entry);
        return ( $pathname, 1) if $found;
    }
    return ( $fname, 0);
}

sub canon_name
{
    my ( $fname) = @_;
    my $qdirsep = quotemeta( $dir_sep);
    $fname =~ s{[^$qdirsep]+$qdirsep\.\.(?:$qdirsep|\Z)}{}
        while $fname =~ /(?:$qdirsep|\A)\.\.(?:$qdirsep|\Z)/;
    $fname =~ s{(?:(?<=$qdirsep)|(?<=\A))\.(?=$qdirsep|\Z)$qdirsep?}{}g;
    return $fname;
}

sub find_cdeps
{
    my ( $cfile, $deps, $included) = @_;

    $deps ||= {};
    $included ||= {};

    return () if exists $deps->{ $cfile};
    $deps->{ $cfile} = [];
    return @{ $alldeps{ $cfile}} if exists $alldeps{ $cfile};
    $alldeps{ $cfile} = [];
    return () unless -f $cfile;

    local *CF;
    open CF, "<$cfile" or die "Cannot open $cfile: $!";
    while ( <CF>) {
        chomp;
        next unless /^\s*\#\s*include\s+"([^\"]+)"/;
        my $incfile = $1;
        my $found = 0;
        if ( exists $mapheader{$incfile}) {
           $incfile = $mapheader{$incfile};
           next unless defined $incfile; # usually apricot.h
           $found = 1;
        } else {
           ( $incfile, $found) = find_file( $incfile, ".");
        }
        die "Can't find $incfile\n" unless $found;
        $incfile = canon_name( $incfile);
        unless ( exists $included->{ $incfile}) {
            push @{ $alldeps{ $cfile}}, $incfile;
            push @{ $deps->{ $cfile}}, $incfile;
            $included->{ $incfile} = 1;
        }
        my @subdeps = find_cdeps( $incfile, $deps, $included);
        push @{ $deps->{ $cfile}}, @subdeps;
        push @{ $alldeps{ $cfile}}, @subdeps;
    }
    close CF;
    return @{ $deps->{ $cfile}};
}

sub cc_command_line
{
    my ( $srcf, $objf) = @_;
    my $ret = $Prima::Config::Config{cc} . ' ';
    $ret .= ' ' . $Prima::Config::Config{cflags};
    $ret .= ' ' . $Prima::Config::Config{cdebugflags} if $DEBUG;
    $ret .= ' ' . join( ' ' , map { $Prima::Config::Config{cdefflag}.$_.'='.$DEFINES{$_}} keys %DEFINES);
    $ret .= ' ' . join( ' ' , map { $Prima::Config::Config{cincflag}.$_} @INCPATH);
    $ret .= ' ' . $srcf;
    $ret .= ' ' . $Prima::Config::Config{cobjflag} . $objf;
    return $ret;
}

sub ld_command_line
{
    my ( $dstf, $deffile) = (shift,shift);
    my $ret = $Prima::Config::Config{ld} . ' ';
    $ret .= ' ' . $Prima::Config::Config{ldflags};
    $ret .= ' ' . $Prima::Config::Config{lddebugflags} if $DEBUG;
    if ( $Prima::Config::Config{compiler} eq 'bcc32') {
        $ret .= '-L"' . join( ';', @LIBPATH) . '" c0d32.obj ';
        $ret .= join( ' ', @_);
    }  else {
        $ret .= ' ' . join( ' ' , map { $Prima::Config::Config{ldlibpathflag}.$_} @LIBPATH);
        $ret .= ' ' . $Prima::Config::Config{ldoutflag} . $dstf;
        $ret .= ' ' . join( ' ', @_);
        $ret .= ' ' . join( ' ' , map { $Prima::Config::Config{ldlibflag}.$_} @LIBS);
    }
    if ( $Win32) {
       if ( $Prima::Config::Config{compiler} eq 'bcc32') {
           $ret .= ',' . $dstf . ', ,' . join(' ', @LIBS) . ','.$deffile.',';
       } else {
           $ret .= ' ' . $Prima::Config::Config{lddefflag} . $deffile;
       }
    } elsif ( $OS2) {
       $ret .= ' ' . $deffile;
    }
    return $ret;
}

sub generate_def
{
   my ( $file, $libid, @exports) = @_;
   return if !$Win32 && !$OS2;
   open PRIMADEF, ">$file" or die "Cannot create $file: $!";
   if ( $Win32) {
      $libid = [split(/::/, $libid)]->[-1];
      print PRIMADEF <<EOF;
LIBRARY $libid
CODE LOADONCALL
DATA LOADONCALL NONSHARED MULTIPLE
EXPORTS
EOF
      if ( $Prima::Config::Config{compiler} eq 'bcc32') {
          print PRIMADEF map { "\t_$_\n\t$_=_$_\n"} @exports;
      }
      else {
          print PRIMADEF map { "\t$_\n\t_$_ = $_\n"} @exports;
      }
   } else {
      $libid = &DynaLoader::mod2fname([split(/::/, $libid)]) if defined &DynaLoader::mod2fname;
      print PRIMADEF <<EOF;
LIBRARY $libid INITINSTANCE TERMINSTANCE
CODE LOADONCALL
DATA LOADONCALL NONSHARED MULTIPLE
EXPORTS
EOF
       print PRIMADEF map { "\t$_\n"} @exports;
   }
   close PRIMADEF;
}

sub addlib
{
   push @LIBS, ( $Unix ? '' : 'lib') . $_[0] . $Prima::Config::Config{ldlibext} 
}


1;
