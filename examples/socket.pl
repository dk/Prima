#
#  Copyright (c) 1997-2001 The Protein Laboratory, University of Copenhagen
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
#  $Id$
#

use strict;
use Prima 'InputLine', 'Edit', Application => { name => 'WebSource' };
use Socket;


my $w = Prima::Window-> create(
   size => [ $::application-> width * 0.8, $::application-> height * 0.8],
   text => 'Socket demo',
);

my $e = $w-> insert( Edit =>
   origin => [ 0, 30],
   size => [ $w-> width, $w-> height - 30],
   text => '',
   hScroll => 1,
   vScroll => 1,
   growMode => gm::Client,
   wordWrap => 1,
);

my %amps = (
   amp => '&',
   oslash => 'o',
   lt => '<',
   gt => '>',
   nbsp => '',
   raquo => '>>',
);


sub parse
{
   my $t = $_[0];
   $$t =~ s/(\r|\n)//g;
   $$t =~ s/<br>/\n/gi;
   $$t =~ s/<p>/\n\n/gi;
   $$t =~ s/<[^<]*>//g;
   $$t =~ s/&([^&;]*);/exists($amps{lc($1)})?$amps{lc($1)}:'?'/ge;
}

my $data;
my $watcher = Prima::File->create(
    onRead => sub {
       my ($me,$fh) = @_;
       my ($d,$l);
       $l = sysread $fh, $d, 10240;
       if ($l) {
          $data .= $d;
          $e-> text( ( length $data) . " bytes loaded");
       } else {
          parse( \$data);
          $e-> textRef( \$data);
          close $fh;
          $me-> file( undef);
       }
    },
    onWrite => sub {
       my ($me,$fh) = @_;
       my $r = "GET $me->{remote_file}\r\n";
       syswrite $fh, $r, length($r);
       $me-> mask( fe::Read); # want no write notifications
    },
);    

my $il = $w-> insert( InputLine =>
   origin => [ 5, 3],
   size => [ $w-> width - 10, 20],
   text => '',
   current => 1,
   onKeyDown => sub {
     my ( $me, $code, $key, $mod) = @_;
     return unless $key == kb::Enter;
     $me-> clear_event;
     my $t = $me-> text;
     
     unless ($t =~ m/^(?:http:\/\/)?([^\/]*)((?:\/.*$)|$)/) {
        $e-> text( "Invalid URL");
        return;
     }

     my $remote = $1;
     my $remote_file = $2;
     $remote_file = '/' unless length $remote_file;
     $watcher-> {remote_file} = $remote_file;
     
     my $port = 80;
     my $proto = getprotobyname('tcp');
     my $iaddr;
     my $tx = $w-> text;
     $w-> text( "Resolving $remote...");
     unless ( $iaddr = inet_aton($remote)) {
        $w-> text( $tx);
        $e->text( "Cannot resolve $remote");
        return;
     }
     my $paddr = sockaddr_in($port, $iaddr);
     unless (socket(SOCK, PF_INET, SOCK_STREAM, $proto)) {
        $w-> text( $tx);
        $e-> text( "Error creating socket: $!");
        return;
     }
     $w-> text( "Connecting to $remote...");
     unless( connect(SOCK, $paddr)) {
        $w->text( $tx);
        $e->text( "Error connecting to $remote: $!");
        return;
     }
     $w-> text( $tx);
     $e-> text( $data = '');
     $watcher-> file( *SOCK);
     $watcher-> mask( fe::Read | fe::Write);
  }
);

$il-> text("Enter URL:");
$il-> select_all;

run Prima;
