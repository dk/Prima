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
#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk> 
#
#  $Id$

use strict;
use Prima;
use Config;
use Prima::TextView;

package Prima::PodView::Link;
use vars qw(@ISA);
@ISA = qw( Prima::TextView::EventRectangles Prima::TextView::EventContent );

sub on_mousedown
{
   my ( $self, $owner, $btn, $mod, $x, $y) = @_;
   my $r = $self-> contains( $x, $y);
   return 1 if $r < 0;
   $r = $self-> {rectangles}->[$r];
   $r = $self-> {references}->[$$r[4]];
   $owner-> link_click( $r, $btn, $mod, $x, $y);
   return 0;
}

sub on_mousemove
{
   my ( $self, $owner, $mod, $x, $y) = @_;
   my $r = ( $self-> contains( $x, $y) >= 0) ? 0 : 1;
   if ( $r != $owner-> {lastLinkPointer}) {
      $owner-> pointer( $r ? cr::Text : $Prima::PodView::handIcon);
      $owner-> {lastLinkPointer} = $r;
   }
}

package Prima::PodView;
use vars qw(@ISA %HTML_Escapes $OP_LINK $handIcon);
@ISA = qw(Prima::TextView);

use constant DEF_INDENT => 4;

use constant COLOR_LINK_FOREGROUND => 2 | tb::COLOR_INDEX;
use constant COLOR_LINK_BACKGROUND => 3 | tb::COLOR_INDEX;
use constant COLOR_CODE_FOREGROUND => 4 | tb::COLOR_INDEX;
use constant COLOR_CODE_BACKGROUND => 5 | tb::COLOR_INDEX;

use constant STYLE_CODE   => 0;
use constant STYLE_TEXT   => 1;
use constant STYLE_HEAD_1 => 2;
use constant STYLE_HEAD_2 => 3;
use constant STYLE_ITEM   => 4;
use constant STYLE_LINK   => 5;
use constant STYLE_MAX_ID => 5;

# model layout indeces
use constant M_INDENT      => 0; # pod-content driven indent
use constant M_TEXT_OFFSET => 1; # contains same info as BLK_TEXT_OFFSET
use constant M_FONT_ID     => 2; # 0 or 1 ( i.e., variable or fixed )
use constant M_START       => 3; # start of data, same purpose as BLK_START

# topic layout indeces
use constant T_MODEL_START => 0; # beginning of topic
use constant T_MODEL_END   => 1; # length of a topic
use constant T_DESCRIPTION => 2; # topic name
use constant T_STYLE       => 3; # style of STYLE_XXX
use constant T_ITEM_DEPTH  => 4; # depth of =item recursion
use constant T_LINK_OFFSET => 5; # 

# formatting constants
use constant FORMAT_LINES    => 10;
use constant FORMAT_TIMEOUT  => 300;

$OP_LINK  = tb::opcode(1);

# init a 'hand' pointer
   $handIcon = Prima::Icon->create( width=>19, height=>24, type => im::bpp4,
   hScaling => 0, vScaling => 0, conversion => ict::None,
   palette => [ 0,0,0,0,0,128,0,128,0,0,128,128,128,0,0,128,0,128,128,128,0,128,128,128,192,192,192,0,0,255,0,255,0,0,255,255,255,0,0,0,0,0,255,255,0,255,255,255],
   data => <<XDATA);
\xdd\xdd\xdd\xd0\x00\x00\x00\x00\xdd\xd0\x00\x00\xdd\xdd\xdd\xd7\xff\xff\xff\x80\xdd\xd0\x00\x00\xdd\xdd\xdd\xd7\xff\xff\xff\x80\xdd\xd0\x00\x00\xdd\xdd\xdd\x7f\xff\xff\xff\xf8\x0d\xd0\x00\x00\xdd\xdd\xd7\xff\xff\xff\xff\xf8\x0d\xd0\x00\x00\xdd\xdd\x7f\xff\xff\xff\xff\xff\x80\xd0\x00\x00\xdd\xdd\x7f\xff\xff\xff\xff\xff\x80\xd0\x00\x00\xdd\xd7\xff\xff\xff\xff\xff\xff\x80\xd0\x00\x00\xdd\xd7\xff\xff\xff\xff\xff\xff\xf8\x00\x00\x00\xdd\x7f\xff\xff\xff\xff\xff\xff\xf8\x00\x00\x00\xdd\x7f\xf8\x8f\xff\xff\xff\xff\xf8\x00\x00\x00\xd7\xff\x80\x0f\xff\xff\xff\xff\xf8\x00\x00\x00\xd7\xf8\x0d\x7f\xff\xff\xff\xf7\xf8\x00\x00\x00\x7f\x80\xdd\x7f\xf7\xff\x7f\x80\xf8\x00\x00\x00w\x0d\xdd\x7f\x80\xf8\x0f\x80\xf8\x00\x00\x00\xdd\xdd\xdd\x7f\x80\xf8\x0f\x80w\xd0\x00\x00\xdd\xdd\xdd\x7f\x80\xf8\x0f\x80\xdd\xd0\x00\x00\xdd\xdd\xdd\x7f\x80\xf8\x07}\xdd\xd0\x00\x00\xdd\xdd\xdd\x7f\x80w\xdd\xdd\xdd\xd0\x00\x00\xdd\xdd\xdd\x7f\x80\xdd\xdd\xdd\xdd\xd0\x00\x00\xdd\xdd\xdd\x7f\x80\xdd\xdd\xdd\xdd\xd0\x00\x00\xdd\xdd\xdd\x7f\x80\xdd\xdd\xdd\xdd\xd0\x00\x00\xdd\xdd\xdd\x7f\x80\xdd\xdd\xdd\xdd\xd0\x00\x00\xdd\xdd\xdd\xd7}\xdd\xdd\xdd\xdd\xd0\x00\x00
XDATA
   my ( $xor, $and) = $handIcon-> split;
   my @sz = map { Prima::Application-> get_system_value($_)} ( sv::XPointer, sv::YPointer);
   $xor-> size( @sz);
   $and-> rop( rop::Invert);
   $and-> put_image( 0, 0, $and); 
   $and-> size( @sz);
   $and-> put_image( 0, 0, $and); 
   $handIcon-> combine( $xor, $and);
   $handIcon-> {__pointerHotSpot} = [ 8, 23 ];

{
my %RNT = (
   %{Prima::TextView->notification_types()},
   Link     => nt::Default,
   Bookmark => nt::Default,
   NewPage  => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      colorMap => [ 
         $def-> {color}, 
         $def-> {backColor},
         cl::Green,              # link foreground
         $def-> {backColor},     # link background
         cl::Blue,               # code foreground
         $def-> {backColor},     # code background
      ],
      styles => [
         { fontId    => 1,                         # STYLE_CODE
           color     => COLOR_CODE_FOREGROUND }, 
         { },                                      # STYLE_TEXT
         { fontSize => 4, fontStyle => fs::Bold }, # STYLE_HEAD_1
         { fontSize => 2, fontStyle => fs::Bold }, # STYLE_HEAD_2
         { fontStyle => fs::Bold },                # STYLE_ITEM
         { color     => COLOR_LINK_FOREGROUND,     # STYLE_LINK
           fontStyle => fs::Underlined   },  
      ],
      pageName => '',
      topicView => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub init
{
   my $self = shift;
   $self-> {model} = [];
   $self-> {links} = [];
   $self-> {styles} = [];
   $self-> {pageName} = '';
   $self-> {modelRange} = [0,0,0];
   $self-> {postBlocks} = {};
   $self-> {topics}     = [];
   $self-> {hasIndex}   = 0;
   $self-> {topicView}  = 1;
   $self-> {lastLinkPointer} = -1;
   my %profile = $self-> SUPER::init(@_);

   $self-> {contents} = [ Prima::PodView::Link-> new ];

   my %font = %{$self-> fontPalette-> [0]};
   $font{pitch} = fp::Fixed; 
   $self-> {fontPalette}-> [1] = \%font;

   $self-> $_($profile{$_}) for qw( styles pageName topicView);
   
   return %profile;
}

sub on_size
{
   my ( $self, $oldx, $oldy, $x, $y) = @_;
   $self-> SUPER::on_size( $oldx, $oldy, $x, $y);
   $self-> format(1) if $oldx != $x;
}

sub on_fontchanged
{
   my $self = $_[0];
   $self-> SUPER::on_fontchanged;
   $self-> format(1);
}

# sub on_link {
#    my ( $self, $linkPointer, $mouseButtonOrKeyEventIfZero, $mod, $x, $y) = @_;
# }

# returns a storable string, that identifies position.
# can report current positions and links to the upper topic
sub make_bookmark
{
   my ( $self, $where) = @_;

   return undef unless length $self-> {pageName};
   if ( !defined $where) { # current position
      my ( $ofs, $bid) = $self-> xy2info( $self-> {offset}, $self-> {topLine});
      my $topic = $self-> {modelRange}-> [0];
      $ofs = $self-> {blocks}-> [$bid]-> [tb::BLK_TEXT_OFFSET] + $ofs;
      return "$self->{pageName}|$topic|$ofs\n";
   } elsif ( $where =~ /up|next|prev/ ) { # up
      if ( $self-> {topicView} ) {
         my $topic = $self-> {modelRange}-> [0];
         my $tid = -1;
         my $t;
         for ( @{$self->{topics}}) {
            $tid++;
            next unless $$_[T_MODEL_START] == $topic;
            $t = $_;
            last;
         }

         if ( $where =~ /next|prev/) {
            return undef unless defined $t;
            my $index = scalar @{$self->{topics}} - 1;
            $tid = -1 if $tid == $index;
            $tid += ( $where =~ /next/) ? 1 : -1;
            if ( $tid == -1) { # simulate index to be the first topic
               $t = $self-> {topics}->[-1]->[T_MODEL_START];
               return "$self->{pageName}|$t|0";
            }
            return undef if $tid < 0 || $tid >= $index;
            $t = $self-> {topics}->[$tid]->[T_MODEL_START];
            return "$self->{pageName}|$t|0";
         }
         
         return "$self->{pageName}|0|0" unless defined $t;
         return undef if $tid + 1 >= scalar @{$self->{topics}}; # already on top
         if ( $$t[ T_STYLE] == STYLE_HEAD_1 || $$t[ T_STYLE] == STYLE_HEAD_2) {
            $t = $self-> {topics}->[-1];
            return "$self->{pageName}|$$t[T_MODEL_START]|0" 
         }
         my $state = $$t[ T_STYLE] - STYLE_HEAD_1 + $$t[ T_ITEM_DEPTH];
         $state-- if $state > 0;
         while ( $tid--) {
            $t = $self-> {topics}-> [$tid];
            $t = $$t[ T_STYLE] - STYLE_HEAD_1 + $$t[ T_ITEM_DEPTH];
            $t-- if $t > 0;
            next if $t >= $state;
            $t = $self-> {topics}-> [$tid]->[T_MODEL_START];
            return "$self->{pageName}|$t|0";
         }
         # return index
         $t = $self-> {topics}->[-1]->[T_MODEL_START];
         return "$self->{pageName}|$t|0";
      } 
   } 
   return undef;
}

sub load_bookmark
{
   my ( $self, $mark) = @_;

   return 0 unless defined $mark;

   my ( $page, $topic, $ofs) = split( '\|', $mark, 3);
   return 0 unless $ofs =~ /^\d+$/ && $topic =~ /^\d+$/;


   if ( $page ne $self-> {pageName}) {
      my $ret = $self-> load_file( $page);
      return 2 if $ret != 1;
   }

   if ( $self-> {topicView}) {
      for my $k ( @{$self->{topics}}) {
         next if $$k[T_MODEL_START] != $topic;
         $self-> select_topic($k);
         last;
      }
   }
   $self-> select_text_offset( $ofs);
      
   return 1;
}

sub load_link
{
   my ( $self, $s) = @_;

   my $mark = $self-> make_bookmark;
   my $t;
   if ( $s =~ /^topic:\/\/(.*)$/) { # local topic
      $t = $1;
      return 0 unless $t =~ /^\d+$/;
      return 0 if $t < 0 || $t >= scalar @{$self-> {topics}};
   }

   my $doBookmark;

   unless ( defined $t) { # page / section / item
      my ( $page, $section, $item) = ( '', undef, 1);
      if ( $s =~ /^([^\/]*)\/(.*)$/) {
         ( $page, $section) = ( $1, $2);
      } else {
         $section = $s;
      }
      $item = 0 if $section =~ s/^\"(.*?)\"$/$1/;

      if ( !length $page) {
         my $tid = -1;
         for ( @{$self-> {topics}}) {
            $tid++;
            next unless $section eq $$_[T_DESCRIPTION];
            next if !$item && $$_[T_STYLE] == STYLE_ITEM;
            $t = $tid;
            last;
         }
         unless ( defined $t) { # no such topic, must be a page?
            $page = $section;
            $section = '';
         }
      }

      if ( length $page and $page ne $self->{pageName}) { # new page?
         if ( $self-> load_file( $page) != 1) {
            $self-> notify(q(Bookmark), $mark) if $mark; 
            return 0;
         }
         $doBookmark = 1;
      }

      if ( ! defined $t) {
         $t = -1 if length $page && $self->{topicView};
         my $tid = -1;
         for ( @{$self-> {topics}}) {
            $tid++;
            next unless $section eq $$_[T_DESCRIPTION];
            $t = $tid;
            last;
         }
      }
   }

   if ( defined $t) {
      if ( $t = $self-> {topics}-> [$t]) {
         if ( $self-> {topicView}) {
            $self-> select_topic($t);
         } else {
            $self-> select_text_offset( 
               $self-> {model}-> [ $$t[ T_MODEL_START]]-> [ M_TEXT_OFFSET]
            );
         }
         $self-> notify(q(Bookmark), $mark) if $mark; 
         return 1;
      }
   } elsif ( $doBookmark) {
      $self-> notify(q(Bookmark), $mark) if $mark; 
      return 1;
   }

   return 0;
}

sub link_click
{
   my ( $self, $s, $btn, $mod, $x, $y) = @_;

   return unless $self-> notify(q(Link), \$s, $btn, $mod, $x, $y);
   return if $btn != mb::Left;
   $self-> load_link( $s);
}

# selects a sub-page; does not check if topicView,
# so must be called with care
sub select_topic
{
   my ( $self, $t) = @_;
   my @mr1 = @{$self->{modelRange}};
   if ( defined $t) {
      $self-> {modelRange} = [
         $$t[ T_MODEL_START],
         $$t[ T_MODEL_END],
         $$t[ T_LINK_OFFSET]
      ]
   } else {
      $self-> {modelRange} = [ 0, scalar @{$self-> {model}} - 1, 0 ]
   }
   my @mr2 = @{$self->{modelRange}};
   
   if ( grep { $mr1[$_] != $mr2[$_] } 0 .. 2) {
      $self-> lock;
      $self-> topLine(0);
      $self-> offset(0);
      $self-> selection(-1,-1,-1,-1);
      $self-> format;
      $self-> unlock;
      $self-> notify(q(NewPage));
   }
}


sub topicView
{
   return $_[0]-> {topicView} unless $#_;
   my ( $self, $tv) = @_;
   $tv = ( $tv ? 1 : 0);
   return if $self-> {topicView} == $tv;
   $self-> {topicView} = $tv;
   return unless length $self-> {pageName};
   $self-> load_file( $self-> {pageName});
}


sub pageName
{
   return $_[0]-> {pageName} unless $#_;
   $_[0]-> {pageName} = $_[1];
}

sub styles
{
   return $_[0]-> {styles} unless $#_;
   my ( $self, @styles) = @_;
   @styles = @{$styles[0]} if ( scalar(@styles) == 1) && ( ref($styles[0]) eq 'ARRAY');
   if ( $#styles < STYLE_MAX_ID) {
      my @as = @{$_[0]-> {styles}};
      my @pd = @{$_[0]-> profile_default-> {styles}};
      while ( $#styles < STYLE_MAX_ID) {    
         if ( $as[ $#styles]) {
            $styles[ $#styles + 1] = $as[ $#styles + 1];
         } else {
            $styles[ $#styles + 1] = $pd[ $#styles + 1];
         }
      }
   }
   $self-> {styles} = \@styles;
   $self-> update_styles;

}

sub update_styles # used for the direct {styles} hacking
{
   my $self = $_[0];
   my @styleInfo;
   for ( @{$self-> {styles}}) {
      my $x = $_;
      my ( @forw, @rev);
      for ( qw( fontId fontSize fontStyle color backColor)) {
         next unless exists $x-> {$_};
         push @forw, $tb::{$_}->( $x-> {$_});
         push @rev,  $tb::{$_}->( 0);
      }
      push @styleInfo, \@forw, \@rev;
   }
   $self-> {styleInfo} = \@styleInfo;
}

sub message
{
   my ( $self, $message, $error) = @_;
   my $x;
   $self-> open_read;
   if ( $error) {
      $x = $self-> {styles}-> [STYLE_HEAD_1]-> {color};
      $self-> {styles}-> [STYLE_HEAD_1]-> {color} = cl::Red;
      $self-> update_styles;
   }
   $self-> read($message);
   $self-> close_read( 0);
   if ( $error) {
      my $z = $self-> {styles}-> [STYLE_HEAD_1];
      defined $x ? $z-> {color} = $x : delete $z-> {color};
      $self-> update_styles;
   }
   $self-> pageName('');
}

sub load_file 
{
   my ( $self, $manpage) = @_;
   my $pageName = $manpage;

   unless ( -f $manpage) {
      my $fn;
      my @ext =  ( '.pod', '.pm', '.pl' );
      push @ext, ( '.bat' ) if $^O =~ /win32/i;
      push @ext, ( '.bat', '.cmd' ) if $^O =~ /os2/;
      push @ext, ( '.com' ) if $^O =~ /VMS/;
      for ( map { $_, "$_/pod" } 
              grep { defined && length && -d } 
                 @INC, 
                 split( $Config::Config{path_sep}, $ENV{PATH})) {
         if ( -f "$_/$manpage") {
            $manpage = "$_/$manpage";
            last;
         }
         $fn = "$_/$manpage";
         $fn =~ s/::/\//g;
         for ( @ext ) {
            if ( -f "$fn$_") {
               $manpage = "$fn$_";
               goto FOUND;
            }
         }
      }
   }
FOUND:   

   unless ( open F, $manpage) {
      my $m = <<ERROR;
\=head1 Error

Error loading '$manpage' : $!

ERROR
      $m =~ s/^\\=/=/gm;
      $self-> message( $m, 1);
      return 0;
   }

   $self-> pointer( cr::Wait);
   
   $self-> open_read;
   $self-> read($_) while <F>;
   close F;

   $self-> pageName( $pageName);
   my $ret = $self-> close_read( $self-> {topicView});

   $self-> pointer( cr::Default);

   unless ( $ret) {
      $_ = <<ERROR;
\=head1 Warning

The file '$manpage' does not contain any POD context

ERROR
      s/^\\=/=/gm;
      $self-> message($_);
      return 2;
   }
   return 1;
}


sub open_read
{
   my $self = $_[0];
   return if $self-> {readState};
   $self-> clear_all;
   $self-> {readState} = {
      cutting      => 1,
      begun        => '',
      
      indent       => DEF_INDENT,
      indentStack  => [],

      bigofs       => 0,
      wrapstate    => '',
      wrapindent   => 0,

      topicStack   => [[-1]],
      ignoreFormat => 0,
   };
}

sub add_formatted
{
   my ( $self, $format, $text) = @_;

   return unless $self-> {readState};
   
   if ( $format eq 'text') {
      return if $self-> {readState}-> {ignoreFormat};
      $self-> add($text,STYLE_CODE,0);
   } elsif ( $format eq 'podview') {
      while ( $text =~ m/<\s*([^<>]*)\s*>/gcs) {
         my $cmd = lc $1;
         if ( $cmd eq 'nobegin') {
            $self-> {readState}-> {ignoreFormat} = 1;
         } elsif ( $cmd eq '/nobegin') {
            $self-> {readState}-> {ignoreFormat} = 0;
         } elsif ( $cmd =~ /^img\s*(.*)$/) {
            $cmd = $1;
            my ( $w, $h, $src, $nobegin);
            while ( $cmd =~ m/\s*([a-z]*)\s*\=\s*(?:(?:'([^']*)')|(?:"([^"]*)")|(\S*))\s*/igcs) {
               my ( $option, $value) = ( lc $1, defined($2)?$2:(defined $3?$3:$4));
               if ( $option eq 'width' && $value =~ /^\d+$/) { $w = $value }
               elsif ( $option eq 'height' && $value =~ /^\d+$/) { $h = $value }
               elsif ( $option eq 'src') { $src = $value }
               elsif ( $option eq 'nobegin' ) { $nobegin = $value }
            }
            if ( defined $src) {
               my $index = 0;
               if ( $src =~ /^(.*)\:(\d+)$/) {
                  ( $src, $index) = ( $1, $2);
               }
               unless ( -f $src) {
                  local @INC = 
                      map {( "$_", "$_/pod")} 
                      grep { defined && length && -d } 
                      ( @INC, split( $Config::Config{path_sep}, $ENV{PATH}));
                  $src = Prima::find_image( $src);
                  next unless $src;
               }
               $src = Prima::Image-> load( $src, index => $index);
               next unless $src;
               $w = $src-> width unless $w;
               $h = $src-> height unless $h;
               $src-> {stretch} = [$w, $h];
               $self-> {readState}-> {ignoreFormat} = $nobegin ? 1 : 0
                  if defined $nobegin;

               my @imgop = (
                         tb::wrap(0),
                         tb::code( \&_imgpaint, $src),
                         tb::moveto( $w, $h),
                         tb::wrap(1));

               if ( @{$self-> {model}}) {
                  push @{$self-> {model}->[-1]}, @imgop;
               } else {
                  push @{$self-> {model}},
                          [ $self-> {readState}-> {indent}, 
                            $self-> {readState}-> {bigofs}, 0,
                            @imgop
                          ];
               }
            }
         }
      }
   }
}

sub _imgpaint
{
   my ( $self, $canvas, $block, $state, $x, $y, $img) = @_;
   $canvas-> stretch_image( $x, $y, @{$img->{stretch}}, $img);
   if ( $self-> {selectionPaintMode}) {
      my @save = ( fillPattern => $canvas-> fillPattern, rop => $canvas-> rop);
      $canvas-> set( fillPattern => fp::Borland, rop => rop::AndPut);
      $canvas-> bar( $x, $y, $x + $img->{stretch}->[0] - 1, $y + $img->{stretch}->[1] - 1);
      $canvas-> set( @save);
   }
}

sub read
{
   my ( $self, $pod) = @_;
   my $r = $self-> {readState};
   return unless $r;

   my $odd = 0;
   for ( split ( "(\n)", $pod)) {
      next unless $odd = !$odd;
   
      $_ .= "\n";

      if ($r-> {cutting}) {
          next unless /^=/;
          $r->{cutting} = 0;
      }

      if ($r-> {begun}) {
          my $begun = $r-> {begun};
          if (/^=end\s+$begun/) {
               $r-> {begun} = '';
               $self-> add("\n",STYLE_TEXT,0); # end paragraph
          } else {
               $self-> add_formatted( $r-> {begun}, $_);
          }
          next;
      }

      1 while s{^(.*?)(\t+)(.*)$}{
          $1
          . (' ' x (length($2) * 8 - length($1) % 8))
          . $3
      }me;

      # Translate verbatim paragraph
      if (/^\s/) {
          $self-> add($_,STYLE_CODE,$r-> {indent});
          next;
      }

      if (/^=for\s+(\S+)\s*(.*)/s) {
          $self-> add_formatted( $1, $2) if defined $2;
          next;
      } elsif (/^=begin\s+(\S+)\s*(.*)/s) {
          $r-> {begun} = $1;
          $self-> add_formatted( $1, $2) if defined $2;
          next;
      }


      if (s/^=//) {
          my $Cmd;
          ($Cmd, $_) = split(' ', $_, 2);
          if ($Cmd eq 'cut') {
              $r-> {cutting} = 1;
          }
          elsif ($Cmd eq 'pod') {
              $r-> {cutting} = 0;
          }
          elsif ($Cmd eq 'head1') {
              $self-> add($_, STYLE_HEAD_1, 0);
          }
          elsif ($Cmd eq 'head2') {
              $self-> add( $_, STYLE_HEAD_2, 0);
          }
          elsif ($Cmd eq 'over') {
              push(@{$r-> {indentStack}}, $r-> {indent});
              $r-> {indent} += ( m/^(\d+)$/ ) ? $1 : DEF_INDENT;
              $self-> add("\n", STYLE_TEXT, 0);
          }
          elsif ($Cmd eq 'back') {
              $self-> _close_topic( STYLE_ITEM);
              $r-> {indent} = pop(@{$r-> {indentStack}});
              $self-> add("\n", STYLE_TEXT, 0);
          }
          elsif ($Cmd eq 'item') {
             $self-> add($_, STYLE_ITEM, $r-> {indentStack}-> [-2] || DEF_INDENT);
          }
      }
      else {
          $self-> add($_, STYLE_TEXT, $r-> {indent});
      }
   }
}

sub close_read
{
   my ( $self, $topicView) = @_;
   return unless $self-> {readState};
   $topicView = $self-> {topicView} unless defined $topicView;
   $self-> add( "\n", STYLE_TEXT, 0); # end
   $self-> {contents}-> [0]-> references( $self-> {links});
   my $topic;
   if ( $topicView) { # generate index list
      my $secid = 0;
      my $msecid = scalar(@{$self-> {topics}});
      if ( $msecid > 0) {
         my $ofs = $self-> {model}->[$self-> {topics}-> [0]-> [T_MODEL_START]]-> [M_TEXT_OFFSET];
         my $firstText = substr( ${$self-> {text}}, 0, ( $ofs > 0) ? $ofs : 0);
         if ( $firstText =~ /[^\n\s\t]/) { # the 1st lines of text are not =head 
            unshift  @{$self-> {topics}}, [ 0, $self-> {topics}-> [0]-> [T_MODEL_START] - 1,
              "Preface", STYLE_HEAD_1, 0, 0 ];
            $msecid++;
         }
         $self-> add( "Index",  STYLE_HEAD_1, 0);
         $self-> {hasIndex} = 1;
         for my $k ( @{$self-> {topics}}) {
            last if $secid == $msecid; # do not add 'Index' entry
            my ( $ofs, $end, $text, $style, $depth, $linkStart) = @$k;
            my $indent = ( $style - STYLE_HEAD_1 + $depth ) * 2;
            $self-> add("L<$text|topic://$secid>\n", STYLE_TEXT, $indent);
            $self-> add("\n", STYLE_TEXT, 0);
            $secid++;
         }
         # select index page
         $topic = $self->{topics}->[$msecid];
      } elsif ( scalar @{$self-> {model}} > 2) { # no =head's, but some info
        push @{$self->{topics}}, [ 0, scalar @{$self-> {model}} - 1,
          "Document", STYLE_HEAD_1, 0, 0 ];
      } 
   } 
   $self-> _close_topic( STYLE_HEAD_1);
   undef $self-> {readState};
   $self-> {lastLinkPointer} = -1;
   $self-> select_topic( $topic);

   $self-> notify(q(NewPage));
  
   return scalar @{$self-> {model}} > 1; # if non-empty 
}

# internal sub, called when a new topic is emerged.
# responsible to what topics can include others ( =headX to =item)
sub _close_topic
{
   my ( $self, $style, $topicToPush) = @_; 

   my $r = $self-> {readState};
   my $t = $r-> { topicStack};
   my $state = ( $style == STYLE_HEAD_1 || $style == STYLE_HEAD_2) ? 
       0 : scalar @{$r->{indentStack}};

   if ( $state <= $$t[-1]->[0]) {
      while ( scalar @$t && $state <= $$t[-1]->[0]) {
         my $nt = pop @$t;
         $nt = $$nt[1];
         $$nt[ T_MODEL_END] = scalar @{$self->{model}} - 1;
      }
      push @$t, [ $state, $topicToPush ] if $topicToPush; 
   } else {
      # assert defined $topicToPush
      push @$t, [ $state, $topicToPush ]; 
   }
}

sub noremap {
    my $a = $_[0];
    $a =~ tr/\000-\177/\200-\377/;
    return $a;
}

sub add
{
   my ( $self, $p, $style, $indent) = @_;

   my $cstyle;
   my $r = $self-> {readState};
   return unless $r;

   # line up the wrappable text
   if ( $style == STYLE_TEXT) {
      if ( $p =~ m/^\n$/ ) {
         $p = $r-> {wrapstate} . $p;
         $r-> {wrapstate} = '';
         $indent = $r-> {wrapindent};
      } else {
         chomp $p;
         $r-> {wrapindent} = $indent unless length $r->{wrapstate};
         $r-> {wrapstate} .= ' ' . $p;
         return;
      }
   } elsif ( length $r->{wrapstate}) {
      $self-> add( "\n", STYLE_TEXT, $r-> {wrapindent});
   }

   $p =~ s/\n//g; 
   my $g = [ $indent, $r-> {bigofs}, 0];
   my $styles = $self-> {styles};

   if ( $style == STYLE_CODE) { 
      $$g[ M_FONT_ID] = $styles-> [ STYLE_CODE]-> {fontId} || 1; # fixed font 
      push @$g, tb::wrap(0);
   } 

   push @$g, @{$self-> {styleInfo}-> [$style * 2]};
   $cstyle = $styles->[$style]-> {fontStyle} || 0;

   if ( $style == STYLE_CODE) { 
      push @$g, tb::text( 0, length $p),
   } else { # wrapable text
      $p =~ s/[\s\t]+/ /g;
      $p =~ s/([\200-\377])/"E<".ord($1).">"/ge;
      $p =~ s/(E<[^<>]+>)/noremap($1)/ge;
      $p =~ s/(\W)([\$\@\%\*\&])(?!Z<>)([^\s\(\)\{\}]+)/$1C<$2$3>/g; 
      $p =~ s/([:A-Za-z_][:A-Za-z_0-9]*\([^\)]*\))/C<$1>/g;
      my $maxnest = 10;
      my $linkStart = scalar @{$self->{links}};
      my $m = $p;
      my @ids = ( [-2, 'Z'], [ length($m), 'z']);
      while ( $maxnest--) {
         while ( $m =~ m/([A-Z])<([^<>]*)>/gcs) {
            push @ids, [ pos($m) - length($2) - 3, $1], [ pos($m) - 1, lc $1];
         }
         last unless $m =~ s/([A-Z])<([^<>]*)>/WW$2W/g;
      }

      my %stack = map {[]} qw( fontStyle fontId fontSize wrap color backColor);
      my %val = (
         fontStyle => $cstyle,
         fontId    => 0,
         fontSize  => 0,
         wrap      => 1,
         color     => tb::COLOR_INDEX, 
         backColor => tb::BACKCOLOR_DEFAULT,
      );
      my ( $link, $linkHREF) = ( 0, '');

      my $pofs = 0;
      $p = '';
      for ( sort { $$a[0] <=> $$b[0] } @ids) {
         my $ofs = $$_[0] + (( $$_[1] eq lc $$_[1]) ? 1 : 2);
         if ( $pofs < $$_[0]) {
            my $s = substr( $m, $pofs, $$_[0] - $pofs);
            $s =~ tr/\200-\377/\000-\177/;
            $s =~ s{
                   E<
                   (
                       ( \d+ )
                       | ( [A-Za-z]+ )
                   )
                   >	
            } {
               do {
                      defined $2
                      ? chr($2)
                      :
                   defined $HTML_Escapes{$3}
                      ? do { $HTML_Escapes{$3} }
                      : do { "E<$1>"; }
               }
            }egx;

            if ( $link) {
               my $l;
               if ( $s =~ m/^([^\|]*)\|(.*)$/) {
                  $l = $2;
                  $s = $1;
                  $linkHREF = '';
               } else {
                  $l = $s;
               }
               unless ( $linkHREF =~ /\//) {
                  my ( $page, $section) = ( '', '');
                  if ( $s =~ /^([^\/]*)\/(.*)$/) {
                     ( $page, $section) = ( $1, $2);
                  } else {
                     $section = $s;
                  }
                  $section =~ s/^\"(.*?)\"$/$1/;
                  $s = length( $page) ? "$page: $section" : $section;
               }
               $linkHREF .= $l;
            }

            push @$g, tb::text( length $p, length $s);
            $p .= $s;
         }
         $pofs = $ofs;

         if ( $$_[1] ne lc $$_[1]) { # open
            if ( $$_[1] eq 'I' || $$_[1] eq 'F') {
               push @{$stack{fontStyle}}, $val{fontStyle};
               push @$g, tb::fontStyle( $val{fontStyle} |= fs::Italic);
            } elsif ( $$_[1] eq 'C') {
               push @{$stack{wrap}}, $val{wrap};
               push @$g, tb::wrap( $val{wrap} = 0);
               my $z = $styles->[STYLE_CODE];
               for ( qw( fontId fontStyle fontSize color backColor)) {
                  next unless exists $z->{$_};
                  push @{$stack{$_}}, $val{$_};
                  push @$g, $tb::{$_}->( $val{$_} = $z->{$_});
               }
            } elsif ( $$_[1] eq 'L') {
               my $z = $styles->[STYLE_LINK];
               for ( qw( fontId fontStyle fontSize color backColor)) {
                  next unless exists $z->{$_};
                  push @{$stack{$_}}, $val{$_};
                  push @$g, $tb::{$_}->( $val{$_} = $z->{$_});
               }
               unless ($link) {
                  push @$g, $OP_LINK, $link = 1;
                  $linkHREF = '';
               }
            } elsif ( $$_[1] eq 'S') {
               push @{$stack{wrap}}, $val{wrap};
               push @$g, tb::wrap( $val{wrap} = 0);
            } elsif ( $$_[1] eq 'B') {
               push @{$stack{fontStyle}}, $val{fontStyle};
               push @$g, tb::fontStyle( $val{fontStyle} |= fs::Bold);
            }
         } else { # close
            if ( $$_[1] eq 'i' || $$_[1] eq 'f' || $$_[1] eq 'b') {
               if (( $val{fontStyle} & fs::Italic) && !( $stack{fontStyle}->[-1] & fs::Italic)) {
                  push @$g, tb::moveto( 0.5, 0, tb::X_DIMENSION_FONT_HEIGHT);
               }
               push @$g, tb::fontStyle( $val{fontStyle} = pop @{$stack{fontStyle}});
            } elsif ( $$_[1] eq 'c') {
               my $z = $styles->[STYLE_CODE];
               push @$g, tb::wrap( $val{wrap} = pop @{$stack{wrap}});
               for ( qw( fontId fontStyle fontSize color backColor)) {
                  next unless exists $z->{$_};
                  push @$g, $tb::{$_}->( $val{$_} = pop @{$stack{$_}});
               }
            } elsif ( $$_[1] eq 'l') {
               my $z = $styles->[STYLE_LINK];
               for ( qw( fontId fontStyle fontSize color backColor)) {
                  next unless exists $z->{$_};
                  push @$g, $tb::{$_}->( $val{$_} = pop @{$stack{$_}});
               }
               if ( $link) { 
                  push @$g, $OP_LINK, $link = 0;
                  push @{$self-> {links}}, $linkHREF;
                  $self-> {postBlocks}-> { scalar @{$self-> {model}}} = 1;
               }
            } elsif ( $$_[1] eq 's') {
               push @$g, tb::wrap( $val{wrap} = pop @{$stack{wrap}});
            }
         }
      }
      if ( $link) {
         push @$g, $OP_LINK, $link = 0;
         push @{$self-> {links}}, $linkHREF;
         $self-> {postBlocks}-> { scalar @{$self-> {model}}} = 1;
      }

      # add topic
      if ( $style == STYLE_HEAD_1 || $style == STYLE_HEAD_2 || 
         ( $style == STYLE_ITEM && $p !~ /^\s*[\d+\*\-]*\s*$/)) {
         my $itemDepth = ( $style == STYLE_ITEM) ?
            scalar @{$r-> {indentStack}} : 0;
         my $pp = $p;
         $pp =~ s/\|//g;
         if ( $style == STYLE_ITEM && $pp =~ /^\s*[a-z]/) {
            $pp =~ s/\s.*$//; # seems like function entry?
         }
         $pp =~ s/([<>])/'E<' . (($1 eq '<') ? 'lt' : 'gt') . '>'/ge;
         my $newTopic = [ scalar @{$self-> {model}},
            0, $pp, $style, $itemDepth, $linkStart];
         $self-> _close_topic( $style, $newTopic); 
         push @{$self-> {topics}}, $newTopic;
      }
   }


   # add text
   $p .= "\n";
   ${$self-> {text}} .= $p;

   # all-string format options - close brackets
   push @$g, @{$self-> {styleInfo}-> [$style * 2 + 1]};

   # finish block
   $r-> {bigofs} += length $p;
   push @{$self-> {model}}, $g;
}

sub stop_format
{
   my $self = $_[0];
   $self-> {formatTimer}-> destroy if $self->{formatTimer};
   undef $self-> {formatData};
   undef $self-> {formatTimer};
}

sub format
{
   my ( $self, $keepOffset) = @_;
   my ( $pw, $ph) = $self-> get_active_area(2);

   my $autoOffset;
   if ( $keepOffset) {
      if ( $self-> {formatData} && $self-> {formatData}-> {position}) {
         $autoOffset = $self-> {formatData}-> {position};
      } else {
         my ( $ofs, $bid) = $self-> xy2info( $self-> {offset}, $self-> {topLine});
         if ( $self->{blocks}->[$bid]) {
            $autoOffset = $ofs + $self->{blocks}->[$bid]->[tb::BLK_TEXT_OFFSET];
         }
      }
   }

   $self-> stop_format;

   my $paneWidth = $pw;
   my $paneHeight = 0;
   my ( $min, $max, $linkIdStart) = @{$self-> {modelRange}};
   if ( $min >= $max) {
      $self-> {blocks} = [];
      $self-> {contents}-> [0]-> rectangles([]);
      $self-> paneSize(0,0);
      return;
   }

   $self-> {blocks} = [];
   $self-> {contents}-> [0]-> rectangles( []);

   $self-> begin_paint_info;

   # cache indents
   my @indents;
   my $state = $self-> create_state;
   for ( 0 .. ( scalar @{$self-> fontPalette} - 1)) {
      $$state[ tb::BLK_FONT_ID] = $_;
      $self-> realize_state( $self, $state, tb::REALIZE_FONTS);
      $indents[$_] = $self-> font-> width;
   }
   $$state[ tb::BLK_FONT_ID] = 0;

   $self-> end_paint_info;

   $self-> {formatData} = {
      indents       => \@indents,
      state         => $state,
      orgState      => [ @$state ],
      linkId        => $linkIdStart,
      min           => $min,
      max           => $max,
      current       => $min,
      paneWidth     => $paneWidth,
      formatWidth   => $paneWidth,
      linkRects     => $self-> {contents}-> [0]-> rectangles,
      step          => FORMAT_LINES,
      position      => undef,
      positionSet   => 0,
   };

   $self-> {formatTimer} = $self-> insert( Timer =>
      name        => 'FormatTimer',
      delegations => [ 'Tick' ],
      timeout     => FORMAT_TIMEOUT,
   ) unless $self-> {formatTimer};

   $self-> paneSize(0,0);
   $self-> {formatTimer}-> start;
   $self-> select_text_offset( $autoOffset) if $autoOffset;
   $self-> format_chunks;
}

sub FormatTimer_Tick
{
  $_[0]-> format_chunks 
}

sub format_chunks
{
   my $self = $_[0];

   my $f = $self-> {formatData};

   my $time = time;
   $self-> begin_paint_info;

   my $mid = $f-> {current};
   my $postBlocks = $self->{postBlocks};
   my $max = $f-> {current} + $f-> {step};
   $max = $f-> {max} if $max > $f->{max};
   my $indents   = $f->{indents};
   my $state     = $f->{state};
   my $linkRects = $f-> {linkRects};
   my $start = scalar @{$self->{blocks}};
   my $formatWidth = $f-> {formatWidth};

   for ( ; $mid <= $max; $mid++) {
      my $g = tb::block_create();
      my $m = $self-> {model}->[$mid];
      my @blocks;
      $$g[ tb::BLK_TEXT_OFFSET] = $$m[M_TEXT_OFFSET];
      $$g[ tb::BLK_Y] = undef;
      push @$g, @$m[ M_START .. $#$m ];

      # format the paragraph
      my $indent = $$m[M_INDENT] * $$indents[ $$m[M_FONT_ID]];
     
      @blocks = $self-> block_wrap( $self, $g, $state, $formatWidth - $indent);

      # adjust size
      for ( @blocks) {
         $$_[ tb::BLK_X] += $indent;
         $f->{paneWidth} = $$_[ tb::BLK_X] + $$_[ tb::BLK_WIDTH] 
            if $$_[ tb::BLK_X] + $$_[ tb::BLK_WIDTH] > $f->{paneWidth};
      }

      # check links 
      if ( $postBlocks-> {$mid}) {
         my $linkState = 0;
         my $linkStart = 0;
         my @rect;
         for ( @blocks) {
            my ( $b, $i, $lim, $x) = ( $_, tb::BLK_START, scalar @$_, $$_[ tb::BLK_X]);

            if ( $linkState) {
               $rect[0] = $x;
               $rect[1] = $$b[ tb::BLK_Y];
            }
 
            for ( ; $i < $lim; $i += $tb::oplen[ $$b[$i] ]) {
               my $cmd = $$b[$i];
               if ( $cmd == tb::OP_TEXT) {
                  $x += $$b[ $i + 3];
               } elsif ( $cmd == tb::OP_TRANSPOSE) {
                  $x += $$b[ $i + 1] unless $$b[ $i + tb::X_FLAGS] & tb::X_EXTEND;
               } elsif ( $cmd == $OP_LINK) {
                  if ( $linkState = $$b[ $i + 1]) {
                     $rect[0] = $x;
                     $rect[1] = $$b[ tb::BLK_Y];
                  } else {
                     $rect[2] = $x;
                     $rect[3] = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
                     push @$linkRects, [ @rect, $f->{linkId} ++ ];
                  }
               }
            }

            if ( $linkState) {
               $rect[2] = $x;
               $rect[3] = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
               push @$linkRects, [ @rect, $f->{linkId}];
            }
         }
      }

      # push back
      push @{$self-> {blocks}}, @blocks;
   }

   my $paneHeight = 0;
   my @settopline;
   if ( scalar @{$self->{blocks}}) {
      my $b = $self-> {blocks}-> [-1];
      $paneHeight = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
      if ( defined $f->{position} && 
           ! $f-> {positionSet} &&
           $self->{topLine} == 0 &&
           $self->{offset} == 0 &&
           $$b[ tb::BLK_TEXT_OFFSET] >= $f-> {position}) {
         $b = $self-> text_offset2block( $f-> {position});
         $f-> {positionSet} = 1;
         if ( defined $b) {
            $b = $self->{blocks}->[$b];
            @settopline = @$b[ tb::BLK_X, tb::BLK_Y];
         }
      }
   }

   $f-> {current} = $mid;

   $self-> recalc_ymap( $start);
   $self-> end_paint_info;

   my $ps = $self-> {paneWidth};
   if ( $ps != $f-> {paneWidth}) {
      $self-> paneSize( $f->{paneWidth}, $paneHeight);
   } else {
      my $oph = $self-> {paneHeight};
      $self-> {paneHeight} = $paneHeight; # direct nasty hack
      $self-> reset_scrolls;
      $self-> repaint if $oph >= $self-> {topLine} && 
         $oph <= $self-> {topLine} + $self-> height;
   }

   if ( @settopline) {
      $self-> topLine( $settopline[1]);
      $self-> offset( $settopline[0]);
   }

   $self-> stop_format if $mid >= $f->{max};
   $f-> {step} *= 2 unless time - $time;
}

sub print
{
   my ( $self, $canvas, $callback) = @_;
   
   my ( $min, $max, $linkIdStart) = @{$self-> {modelRange}};
   return if $min >= $max;

   goto ABORT if $callback && ! $callback->();

   # cache indents
   my @indents;
   my $state = $self-> create_state;
   for ( 0 .. ( scalar @{$self-> fontPalette} - 1)) {
      $$state[ tb::BLK_FONT_ID] = $_;
      $self-> realize_state( $canvas, $state, tb::REALIZE_FONTS);
      $indents[$_] = $canvas-> font-> width;
   }
   $$state[ tb::BLK_FONT_ID] = 0;

   my ( $formatWidth, $formatHeight) = $canvas-> size;
   my $mid = $min;
   my $y = $formatHeight;

   for ( ; $mid <= $max; $mid++) {
      my $g = tb::block_create();
      my $m = $self-> {model}->[$mid];
      my @blocks;
      $$g[ tb::BLK_TEXT_OFFSET] = $$m[M_TEXT_OFFSET];
      $$g[ tb::BLK_Y] = undef;
      push @$g, @$m[ M_START .. $#$m ];

      # format the paragraph
      my $indent = $$m[M_INDENT] * $indents[ $$m[M_FONT_ID]];
      @blocks = $self-> block_wrap( $canvas, $g, $state, $formatWidth - $indent);

      # paint
      for ( @blocks) {
         my $b = $_; 
         $self-> block_draw( $canvas, $b, $indent, $y);
         while ( $y - $$b[ tb::BLK_HEIGHT] < 0) {
            goto ABORT if $callback && ! $callback->();
            $canvas-> new_page;
            $y += $formatHeight;
            $self-> block_draw( $canvas, $b, $indent, $y);
         }
         $y -= $$b[ tb::BLK_HEIGHT];
      }
   }

ABORT:
}

sub select_text_offset
{
   my ( $self, $pos) = @_;
   if ( defined $self-> {formatData}) {
      my $last = $self->{blocks}->[-1];
      $self-> {formatData}-> {position} = $pos;
      return if ! $last || $$last[tb::BLK_TEXT_OFFSET] < $pos;
   } 
   my $b = $self-> text_offset2block( $pos);
   if ( defined $b) {
      $b = $self-> {blocks}-> [$b];
      $self-> topLine( $$b[ tb::BLK_Y]);
      $self-> offset( $$b[ tb::BLK_X]);
   }
}

sub clear_all
{
   my $self = $_[0];
   $self-> SUPER::clear_all;
   $self-> {modelRange} = [0,0,0];
   $self-> {model}      = [];
   $self-> {links}      = [];
   $self-> {postBlocks} = {};
   $self-> {topics}     = [];
   $self-> {topicIndex} = {};
   $self-> {hasIndex}   = 0;
}

sub text_range
{
   my $self = $_[0];
   my @range;
   $range[0] = $self-> {model}->[ $self-> {modelRange}->[0]]-> [M_TEXT_OFFSET];
   $range[1] = ( $self-> {modelRange}->[1] + 1 >= scalar @{$self->{model}}) ?
      length ( ${$self->{text}} ) : 
      $self-> {model}->[ $self-> {modelRange}->[1] + 1]-> [M_TEXT_OFFSET];
   $range[1]-- if $range[1] > $range[0];
   return @range;
}

%HTML_Escapes = (
    'amp'	=>	'&',	#   ampersand
    'lt'	=>	'<',	#   left chevron, less-than
    'gt'	=>	'>',	#   right chevron, greater-than
    'quot'	=>	'"',	#   double quote

    "Aacute"	=>	"\xC1",	#   capital A, acute accent
    "aacute"	=>	"\xE1",	#   small a, acute accent
    "Acirc"	=>	"\xC2",	#   capital A, circumflex accent
    "acirc"	=>	"\xE2",	#   small a, circumflex accent
    "AElig"	=>	"\xC6",	#   capital AE diphthong (ligature)
    "aelig"	=>	"\xE6",	#   small ae diphthong (ligature)
    "Agrave"	=>	"\xC0",	#   capital A, grave accent
    "agrave"	=>	"\xE0",	#   small a, grave accent
    "Aring"	=>	"\xC5",	#   capital A, ring
    "aring"	=>	"\xE5",	#   small a, ring
    "Atilde"	=>	"\xC3",	#   capital A, tilde
    "atilde"	=>	"\xE3",	#   small a, tilde
    "Auml"	=>	"\xC4",	#   capital A, dieresis or umlaut mark
    "auml"	=>	"\xE4",	#   small a, dieresis or umlaut mark
    "Ccedil"	=>	"\xC7",	#   capital C, cedilla
    "ccedil"	=>	"\xE7",	#   small c, cedilla
    "Eacute"	=>	"\xC9",	#   capital E, acute accent
    "eacute"	=>	"\xE9",	#   small e, acute accent
    "Ecirc"	=>	"\xCA",	#   capital E, circumflex accent
    "ecirc"	=>	"\xEA",	#   small e, circumflex accent
    "Egrave"	=>	"\xC8",	#   capital E, grave accent
    "egrave"	=>	"\xE8",	#   small e, grave accent
    "ETH"	=>	"\xD0",	#   capital Eth, Icelandic
    "eth"	=>	"\xF0",	#   small eth, Icelandic
    "Euml"	=>	"\xCB",	#   capital E, dieresis or umlaut mark
    "euml"	=>	"\xEB",	#   small e, dieresis or umlaut mark
    "Iacute"	=>	"\xCD",	#   capital I, acute accent
    "iacute"	=>	"\xED",	#   small i, acute accent
    "Icirc"	=>	"\xCE",	#   capital I, circumflex accent
    "icirc"	=>	"\xEE",	#   small i, circumflex accent
    "Igrave"	=>	"\xCD",	#   capital I, grave accent
    "igrave"	=>	"\xED",	#   small i, grave accent
    "Iuml"	=>	"\xCF",	#   capital I, dieresis or umlaut mark
    "iuml"	=>	"\xEF",	#   small i, dieresis or umlaut mark
    "Ntilde"	=>	"\xD1",	#   capital N, tilde
    "ntilde"	=>	"\xF1",	#   small n, tilde
    "Oacute"	=>	"\xD3",	#   capital O, acute accent
    "oacute"	=>	"\xF3",	#   small o, acute accent
    "Ocirc"	=>	"\xD4",	#   capital O, circumflex accent
    "ocirc"	=>	"\xF4",	#   small o, circumflex accent
    "Ograve"	=>	"\xD2",	#   capital O, grave accent
    "ograve"	=>	"\xF2",	#   small o, grave accent
    "Oslash"	=>	"\xD8",	#   capital O, slash
    "oslash"	=>	"\xF8",	#   small o, slash
    "Otilde"	=>	"\xD5",	#   capital O, tilde
    "otilde"	=>	"\xF5",	#   small o, tilde
    "Ouml"	=>	"\xD6",	#   capital O, dieresis or umlaut mark
    "ouml"	=>	"\xF6",	#   small o, dieresis or umlaut mark
    "szlig"	=>	"\xDF",		#   small sharp s, German (sz ligature)
    "THORN"	=>	"\xDE",	#   capital THORN, Icelandic
    "thorn"	=>	"\xFE",	#   small thorn, Icelandic
    "Uacute"	=>	"\xDA",	#   capital U, acute accent
    "uacute"	=>	"\xFA",	#   small u, acute accent
    "Ucirc"	=>	"\xDB",	#   capital U, circumflex accent
    "ucirc"	=>	"\xFB",	#   small u, circumflex accent
    "Ugrave"	=>	"\xD9",	#   capital U, grave accent
    "ugrave"	=>	"\xF9",	#   small u, grave accent
    "Uuml"	=>	"\xDC",	#   capital U, dieresis or umlaut mark
    "uuml"	=>	"\xFC",	#   small u, dieresis or umlaut mark
    "Yacute"	=>	"\xDD",	#   capital Y, acute accent
    "yacute"	=>	"\xFD",	#   small y, acute accent
    "yuml"	=>	"\xFF",	#   small y, dieresis or umlaut mark

    "lchevron"	=>	"\xAB",	#   left chevron (double less than)
    "rchevron"	=>	"\xBB",	#   right chevron (double greater than)
);

1;
