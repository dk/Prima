#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
#  $Id$
#
=pod

=head1 NAME

Prima::PS::Printer - PostScript interface to Prima::Printer

=head1 SYNOPSIS

use Prima;
use Prima::PS::Printer;

=head1 DESCRIPTION

Realizes the Prima printer interface to PostScript level 2 document language
through Prima::PS::Drawable module. Allows different user profiles to be
created and managed with GUI setup dialog. The module is designed to be 
compliant with Prima::Printer interface.

Also contains convenience classes (File, LPR, Pipe) for non-GUI use.

=head1 SYNOPSIS

   use Prima::PS::Printer;

   my $x;
   if ( $preview) {
      $x = Prima::PS::Pipe-> create( command => 'gv -');
   } elsif ( $print_in_file) {
      $x = Prima::PS::File-> create( file => 'out.ps');
   } else {
      $x = Prima::PS::LPR-> create( args => '-Pcolorprinter');
   }
   $x-> begin_doc;
   $x-> font-> size( 300);
   $x-> text_out( "hello!", 100, 100);
   $x-> end_doc;

=cut

use strict;
use Prima;
use Prima::Utils;
use IO::Handle;
use Prima::PS::Drawable;

package Prima::PS::Printer;
use vars qw(@ISA %pageSizes $unix);
@ISA = qw(Prima::PS::Drawable);

$unix = Prima::Application-> get_system_info->{apc} == apc::Unix;

use constant lpr  => 0;
use constant file => 1;
use constant exec => 2;


sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      resFile       => Prima::Utils::path . '/Printer',
      printer       => undef,
      gui           => 1,
      defaultData   => {
         color          => 1,
         resolution     => 300,
         page           => 'A4',
         copies         => 1,
         scaling        => 1,
         portrait       => 1,
         useDeviceFonts => 1,
         useDeviceFontsOnly => 0,
         spoolerType    => $unix ? lpr : file, 
         spoolerData    => '',
         devParms       => {
            MediaType                   => '', 
            MediaColor                  => '', 
            MediaWeight                 => '',       
            MediaClass                  => '', 
            InsertSheet                 => 0,  
            LeadingEdge                 => 0, 
            ManualFeed                  => 0, 
            TraySwitch                  => 0, 
            MediaPosition               => '', 
            DeferredMediaSelection      => 0, 
            MatchAll                    => 0, 
         },
      },
   );   
   @$def{keys %prf} = values %prf;
   return $def;
}
                  

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> {defaultData} = {};
   $self-> {data} = {};
   $self-> $_($profile{$_}) for qw(defaultData resFile);
   $self-> {printers} = {};
   $self-> {gui} = $profile{gui};
   
   my $pr = $profile{printer} if defined $profile{printer};

   if ( open F, $self-> {resFile}) {
      local $/;
      my $fc = '$fc = ' . <F>;
      close F;
      eval "$fc";
      $self-> {printers} = $fc if !$@ && defined($fc) && ref($fc) eq 'HASH';
   }
   
   unless ( scalar keys %{$self-> {printers}}) {
      $self-> {printers}-> {'Default printer'} = deepcopy( $self-> {defaultData});
      if ( $unix) {
         $self-> import_printers( 'printers', '/etc/printcap');
         $self-> {printers}-> {GhostView} = deepcopy( $self-> {defaultData});
         $self-> {printers}-> {GhostView}-> {spoolerType} = exec;
         $self-> {printers}-> {GhostView}-> {spoolerData} = 'gv -';
      }
      $self-> {printers}-> {File} = deepcopy( $self-> {defaultData});
      $self-> {printers}-> {File}-> {spoolerType} = file;
   }
      

   unless ( defined $pr) {
      if ( defined  $self-> {printers}-> {'Default printer'}) {
         $pr = 'Default printer';
      } else {
         my @k = keys %{$self-> {printers}};
         $pr = $k[0];
      }
   }

   $self-> printer( $pr);
   return %profile;   
}

sub import_printers
{
   my ( $self, $slot, $file) = @_;
   return undef unless open PRINTERS, $file;
   my $np;
   my @names;
   while ( <PRINTERS>) {
      chomp;
      if ( $np) {
         $np = 0 unless /\\\\s*$/;
      } else {
         next if /^\#/ || /^\s*$/;
         push( @names, $1) if m/^([^\|\:]+)/;
         $np = 1 if /\\\s*$/;
      }
   }
   close PRINTERS;

   my @ret;
   for ( @names) {
      s/^\s*//g;
      s/\s*$//g;
      next unless length;
      my $n = "Printer '$_'";
      my $j = 0;
      while ( exists $self-> {$slot}-> {$n}) {
         $n = "Printer '$_' #$j";
         $j++;
      }
      $self-> {$slot}-> {$n} = deepcopy( $self-> {defaultData});
      $self-> {$slot}-> {$n}-> {spoolerType} = lpr;
      $self-> {$slot}-> {$n}-> {spoolerData} = "-P$_";
      push @ret, $n;
   }
   return @ret;
}

sub printers
{
   my $self = $_[0];
   my @res;
   for ( keys %{$self->{printers}}) {
      my $d = $self->{printers}-> {$_};
      push @res, {
         name    => $_,
         device  => 
           (( $d-> {spoolerType} == lpr ) ? "lp $d->{spoolerData}"  : 
           (( $d-> {spoolerType} == file) ? 'file' : $d-> {spoolerData})),
         defaultPrinter =>  ( $self-> {current} eq $_) ? 1 : 0,
      },
   }
   return \@res;
}

sub resolution
{
   return $_[0]-> SUPER::resolution unless $#_;
   $_[0]-> raise_ro("resolution") if @_ != 3; # pass inherited call
   shift-> SUPER::resolution( @_);
}

sub resFile
{
   return $_[0]-> {resFile} unless $#_;
   my ( $self, $fn) = @_;
   if ( $fn =~ /^\s*~/) {
      my $home = defined( $ENV{HOME}) ? $ENV{HOME} : '/';
      $fn =~ s/^\s*~/$home/;
   }
   $self-> {resFile} = $fn;
}

sub printer
{
   return $_[0]-> {current} unless $#_;
   my ( $self, $printer) = @_;
   return undef unless exists $self-> {printers}-> {$printer};
   $self-> end_paint_info if $self-> get_paint_state == 2;
   $self-> end_paint if $self-> get_paint_state;
   $self-> {current} = $printer;
   $self-> {data} = {};
   $self-> data( $self-> {defaultData});
   $self-> data( $self-> {printers}-> {$printer});
   $self-> translate( 0, 0);
   return 1;
}

sub data
{
   return $_[0]-> {data} unless $#_;
   my ( $self, $dd) = @_;
   my $dv = $dd-> {devParms};
   my $p = $self-> {data};
   for ( keys %$dd) {
      next if $_ eq 'devParms';
      $p->{$_} = $dd-> {$_};
   }
   if ( defined $dv) {
      for ( keys %$dv) {
         $p->{devParms}-> {$_} = $dv-> {$_};
      }
   }
   $self-> SUPER::resolution( $p-> {resolution}, $p-> {resolution}) if exists $dd-> {resolution};
   $self-> scale( $p-> {scaling}, $p-> {scaling}) if exists $dd-> {scaling};
   $self-> reversed( $p-> {portrait} ? 0 : 1) if exists $dd-> {portrait};
   $self-> pageSize( @{exists($pageSizes{$p-> {page}}) ? $pageSizes{$p-> {page}} : $pageSizes{A4}})
     if exists $dd-> {page};
   if ( exists $dd-> {page}) {
      $self-> useDeviceFonts( $p-> {useDeviceFonts});
      $self-> useDeviceFontsOnly( $p-> {useDeviceFontsOnly});
   }
   if ( defined $dv) {
      my %dp = %{$p-> {devParms}};
      for ( keys %dp) {
          delete $dp{$_} unless length $dp{$_};
      }
      for ( qw( LeadingEdge InsertSheet ManualFeed DeferredMediaSelection TraySwitch MatchAll)) {
         delete $dp{$_} unless $dp{$_};
      }
      $dp{LeadingEdge}-- if exists $dp{LeadingEdge};
      for ( qw( MediaType MediaColor MediaWeight MediaClass)) {
         next unless exists $dp{$_};
         $dp{$_} =~ s/(\\|\(|\))/\\$1/g;
         $dp{$_} = '(' . $dp{$_} . ')';
      }
      for ( qw( InsertSheet ManualFeed TraySwitch DeferredMediaSelection MatchAll)) {
         next unless exists $dp{$_};
         $dp{$_} = $dp{$_} ? 'true' : 'false';
      }
      $self-> pageDevice( \%dp);
   }
   $self-> grayscale( $p-> {color} ? 0 : 1) if exists $dd-> {color};
}

sub defaultData
{
   return $_[0]-> {defaultData} unless $#_;
   my ( $self, $dd) = @_;
   my $dv = $dd-> {devParms};
   delete $dd-> {devParms};
   for ( keys %$dd) {
      $self-> {defaultData}->{$_} = $dd-> {$_};
   }
   if ( defined $dv) {
      for ( keys %$dv) {
         $self-> {defaultData}->{devParms}-> {$_} = $dv-> {$_};
      }
   }
}


%pageSizes = ( # in points
   'A0'  => [ 2391, 3381],
   'A1'  => [ 1688, 2391],
   'A2'  => [ 1193, 1688],
   'A3'  => [ 843, 1193],
   'A4'  => [ 596, 843],
   'A5'  => [ 419, 596],
   'A6'  => [ 297, 419],
   'A7'  => [ 209, 297],
   'A8'  => [ 146, 209],
   'A9'  => [ 103, 146],
   'B0'  => [ 2929, 4141],
   'B1'  => [ 2069, 2929],
   'B10' => [ 89, 126],
   'B2'  => [ 1463, 2069],
   'B3'  => [ 1034, 1463],
   'B4'  => [ 729, 1034],
   'B5'  => [ 516, 729],
   'B6'  => [ 362, 516],
   'B7'  => [ 257, 362],
   'B8'  => [ 180, 257],
   'B9'  => [ 126, 180],
   'C5E' => [ 462, 650],
   'US Common #10 Envelope' => [ 297, 684],
   'DLE'       => [ 311, 624],
   'Executive' => [ 541, 721],
   'Folio'     => [ 596, 937],
   'Ledger'    => [ 1227, 792],
   'Legal'     => [ 613, 1011],
   'Letter'    => [ 613, 792],
   'Tabloid'   => [ 792, 1227],
);

sub deepcopy
{
   my %h = %{$_[0]};
   $h{devParms} = {%{$h{devParms}}};
   return \%h;
}

sub setup_dialog
{
   return unless $_[0]->{gui};
   eval "use Prima::PS::Setup"; die "$@\n" if $@;
   $_[0]-> sdlg_exec;
}

sub begin_doc
{
   my ( $self, $docName) = @_;
   return 0 if $self-> get_paint_state;

   $self-> {spoolHandle} = undef;

   if ( $self-> {data}-> {spoolerType} == file) {
      if ( $self-> {gui}) {
         eval "use Prima::MsgBox"; die "$@\n" if $@;
         my $f = Prima::MsgBox::input_box( 'Print to file', 'Output file name:', '', mb::OKCancel, { buttons => {
            mb::OK, { 
            modalResult => undef,
            onClick => sub {
               $_[0]-> clear_event;
               my $f = $_[0]-> owner-> InputLine1-> text;
               if ( -f $f) {
                    return 0 if Prima::MsgBox::message("File $f already exists. Overwrite?",
                         mb::Warning|mb::OKCancel) != mb::OK;         
               } else {
                   unless ( open F, "> $f") {
                       Prima::MsgBox::message("Error opening $f:$!", mb::Error|mb::OK);           
                       return 0;
                   }   
                   close F;
                   unlink $f;
               } 
               $_[0]-> owner-> modalResult( mb::OK);
               $_[0]-> owner-> end_modal;
         }}}});
         return 0 unless defined $f;
         my $h = IO::Handle-> new;
         unless ( open $h, "> $f") {
            undef $h;
            Prima::message("Error opening $f:$!");
            goto AGAIN;
         }
         $self-> {spoolHandle} = $h;
         $self-> {spoolName}   = $f;
      } else { # no gui
         my $h = IO::Handle-> new;
	 my $f = $self-> {data}->{spoolerData};
         unless ( open $h, "> $f") {
            undef $h;
	    return 0;
         }
         $self-> {spoolHandle} = $h;
         $self-> {spoolName}   = $f;
      }
      unless ( $self-> SUPER::begin_doc( $docName)) {
         unlink( $self-> {spoolName});
         close( $self-> {spoolHandle});
         return 0;
      }
      return 1;
   }

   return $self-> SUPER::begin_doc( $docName);
}   

my ( $sigpipe);

sub __end
{
   my $self = $_[0];
   close( $self-> {spoolHandle}) if $self-> {spoolHandle};
   defined($sigpipe) ? $SIG{PIPE} = $sigpipe : delete($SIG{PIPE});
   $self-> {spoolHandle} = undef;
   $sigpipe = undef;
}

sub end_doc
{
   my $self = $_[0];
   $self-> SUPER::end_doc;
   $self-> __end;
}

sub abort_doc
{
   my $self = $_[0];
   $self-> SUPER::abort_doc;
   $self-> __end;
   unlink $self-> {spoolName} if $self-> {data}-> {spoolerType} == file;
}

sub spool
{
   my ( $self, $data) = @_;

   my $piped = 0;
   if ( $self-> {data}-> {spoolerType} != file && !$self-> {spoolHandle}) {
      my @cmds;
      if ( $self-> {data}-> {spoolerType} == lpr) {
         push( @cmds, map { $_ . ' ' . $self-> {data}-> {spoolerData}} qw(
            lp lpr /bin/lp /bin/lpr /usr/bin/lp /usr/bin/lpr));
      } else {
         push( @cmds, $self-> {data}-> {spoolerData});
      } 
      my $ok = 0;
      $sigpipe = $SIG{PIPE};
      $SIG{PIPE} = 'IGNORE';
      CMDS: for ( @cmds) {
         $piped = 0;
	 my $f = IO::Handle-> new;
         next unless open $f, "|$_";
	 $f-> autoflush(1);
         $piped = 1 unless print $f $data;
         close( $f), next if $piped;
         $ok = 1;
         $self-> {spoolHandle} = $f;
         $self-> {spoolName}   = $_;
         last;
      }
      Prima::message("Error printing to '$cmds[0]'") if !$ok && $self->{gui};
      return $ok;
   }

   if ( !(print {$self->{spoolHandle}} $data) || 
         ( $piped && $self-> {data}-> {spoolerType} != file )
      ) {
      Prima::message( "Error printing to '$self->{spoolName}'") if $self->{gui};
      return 0;
   }

   return 1;
}

package Prima::PS::File;
use vars qw(@ISA);
@ISA=q(Prima::PS::Printer);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      file => 'out.ps',
      gui  => 0,
   );   
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> {data}->{spoolerType} = Prima::PS::Printer::file;
   $self-> {data}->{spoolerData} = $profile{file};
}

sub file
{
   return $_[0]->{data}->{spoolerData} unless $#_;
   $_[0]->{data}->{spoolerData} = $_[1];
}

package Prima::PS::LPR;
use vars qw(@ISA);
@ISA=q(Prima::PS::Printer);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      gui  => 0,
      args => '',
   );   
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> {data}->{spoolerType} = Prima::PS::Printer::lpr;
   $self-> {data}->{spoolerData} = $profile{args};
}

sub args
{
   return $_[0]->{data}->{spoolerData} unless $#_;
   $_[0]->{data}->{spoolerData} = $_[1];
}

package Prima::PS::Pipe;
use vars qw(@ISA);
@ISA=q(Prima::PS::Printer);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      gui  => 0,
      command => '',
   );   
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> {data}->{spoolerType} = Prima::PS::Printer::exec;
   $self-> {data}->{spoolerData} = $profile{command};
}

sub command
{
   return $_[0]->{data}->{spoolerData} unless $#_;
   $_[0]->{data}->{spoolerData} = $_[1];
}

1;
