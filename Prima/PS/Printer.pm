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
#
=head1 NAME

Prima::PS::Printer - PostScript interface to Prima::Printer

=head1 SYNOPSIS

use Prima;
use Prima::PS::Printer;

=head1 DESCRIPTION

Realizes the Prima printer interface to PostScript level 2 document language
through Prima::PS::Drawable module. Allows different user profiles to be
created and mananaged with GUI setup dialog. The module is designed to be 
compliant with Prima::Printer interface.

=cut

use strict;
use Prima;
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
      resFile       => '~/.prima.printer',
      printer       => undef,
      defaultData   => {
         color          => 0,
         resolution     => 300,
         page           => 'A4',
         copies         => 1,
         scaling        => 1,
         portrait       => 1,
         useDeviceFonts => 1,
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
   
   my $pr = $profile{printer} if defined $profile{printer};

   if ( open F, $self-> {resFile}) {
      local $/;
      my $fc = '$fc = ' . <F>;
      close F;
      eval "$fc";
      $self-> {printers} = $fc if !$@ && defined($fc) && ref($fc) eq 'HASH';
   }
   
   $self-> {printers}-> {Default} = deepcopy( $self-> {defaultData})
      unless scalar keys %{$self-> {printers}};

   unless ( defined $pr) {
      if ( defined  $self-> {printers}-> {Default}) {
         $pr = 'Default';
      } else {
         my @k = keys %{$self-> {printers}};
         $pr = $k[0];
      }
   }

   $self-> printer( $pr);
   return %profile;   
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
           (( $d-> {spoolerType} == lpr ) ? 'LP'  : 
           (( $d-> {spoolerType} == file) ? 'file' : $d-> {spoolerData})),
         defaultPrinter =>  ( $self-> {current} eq $_) ? 1 : 0,
      },
   }
   return \@res;
}

sub resolution
{
   return $_[0]-> SUPER::resolution unless $#_;
   $_[0]-> raise_ro("resolution") if @_ != 3; # pass interited call
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
   $self-> transform( 0, 0);
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
   $self-> useDeviceFonts( $p-> {useDeviceFonts}) if exists $dd-> {page};
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
   require Prima::PS::Setup;
   $_[0]-> sdlg_exec;
}

sub spool
{
   my ( $self, $data) = @_;
   if ( $self-> {data}-> {spoolerType} == file) {
      require Prima::MsgBox;
      my $f = Prima::MsgBox::input_box( 'Print to file', 'Output file name:', '', mb::OKCancel, { buttons => {
         mb::OK => { 
         modalResult => undef,
         onClick => sub {
            $_[0]-> clear_event;
            my $f = $_[0]-> owner-> InputLine1-> text;
            if ( -f $f) {
                 return if Prima::MsgBox::message("File $f already exists. Overwrite?",
                      mb::Warning|mb::OKCancel) != mb::OK;         
            } else {
                unless ( open F, "> $f") {
                    Prima::MsgBox::message("Error opening $f:$!", mb::Error|mb::OK);           
                    return;
                }   
                close F;
                unlink $f;
            } 
            $_[0]-> owner-> modalResult( mb::OK);
            $_[0]-> owner-> end_modal;
      }}}});
      return unless defined $f;
      unless ( open F, "> $f") {
         Prima::message("Error opening $f:$!");
         goto AGAIN;
      }
      print F ($_, "\n") for @{$data};
      close F;
   } else {
      my @cmds;
      if ( $self-> {data}-> {spoolerType} == lpr) {
         push( @cmds, map { $_ . ' ' . $self-> {data}-> {spoolerData}} qw(
           lp lpr /bin/lp /bin/lpr /usr/bin/lp /usr/bin/lpr));
      } else {
         push( @cmds, $self-> {data}-> {spoolerData});
      }
      my $ok = 0;
      my $piped;
      my $SP = $SIG{PIPE};
      $SIG{PIPE} = sub { $piped = 1 };
      CMDS: for ( @cmds) {
         $piped = 0;
         next unless open F, "|$_";
         for ( @$data) {
            print F ( $_, "\n");
            next CMDS if $piped;
         }
         next unless close F;
         next if $piped;
         $ok = 1;
         last;
      }
      defined($SP) ? $SIG{PIPE} = $SP : delete($SIG{PIPE});
      Prima::message("Error printing to $cmds[0]") unless $ok;
   }
}

1;
