use strict;
use warnings;
use Prima;
use Config;
use Prima::Utils;
use Prima::TextView;
use Encode;

package Prima::PodView::Link;
use vars qw(@ISA);
@ISA = qw( Prima::TextView::EventRectangles Prima::TextView::EventContent );

sub on_mousedown
{
	my ( $self, $owner, $btn, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	return 1 if $r < 0;
	$r = $self-> {rectangles}-> [$r];
	$r = $self-> {references}-> [$$r[4]];
	$owner-> link_click( $r, $btn, $mod, $x, $y);
	return 0;
}

sub on_mousemove
{
	my ( $self, $owner, $mod, $x, $y) = @_;
	my $r = $self-> contains( $x, $y);
	if ( $r != $owner-> {lastLinkPointer}) {
		my $was_hand = ($owner->{lastLinkPointer} >= 0) ? 1 : 0;
		my $is_hand  = ($r >= 0) ? 1 : 0;
		if ( $is_hand != $was_hand) {
			$owner-> pointer( $is_hand ? cr::Hand : cr::Text );
		}
		my $rr = $self->rectangles;
		my ($dx, $dy) = $owner->point2screen(0,0);
		my $or = $owner->{lastLinkPointer};
		$owner-> {lastLinkPointer} = $r;
		if ( $was_hand ) {
			$or = $rr->[$or];
			$owner-> invalidate_rect($or->[0] - $dx, $dy - $or->[1], $or->[2] - $dx, $dy - $or->[3]);
		}
		if ( $is_hand ) {
			$or = $rr->[$r];
			$owner-> invalidate_rect($or->[0] - $dx, $dy - $or->[1], $or->[2] - $dx, $dy - $or->[3]);
		}
	}
}

sub on_paint
{
	my ( $self, $owner, $canvas, $ci ) = @_;
	my ($dx, $dy) = $owner->point2screen(0,0);
	my $r  = $self->rectangles->[ $owner->{lastLinkPointer} ];
	my $c  = $canvas-> color;
	$canvas-> color( $owner-> {colorMap}->[ $ci ]);
	$canvas-> line( $r->[0] - $dx, $dy - $r->[3], $r->[2] - $dx, $dy - $r->[3]);
	$canvas-> color( $c);
}

package Prima::PodView;

use vars qw(@ISA %HTML_Escapes $OP_LINK);
@ISA = qw(Prima::TextView);

use constant DEF_INDENT       => 4;
use constant DEF_FIRST_INDENT => 1;

use constant COLOR_LINK_FOREGROUND => 2 | tb::COLOR_INDEX;
use constant COLOR_LINK_BACKGROUND => 3 | tb::COLOR_INDEX;
use constant COLOR_CODE_FOREGROUND => 4 | tb::COLOR_INDEX;
use constant COLOR_CODE_BACKGROUND => 5 | tb::COLOR_INDEX;

use constant STYLE_CODE   => 0;
use constant STYLE_TEXT   => 1;
use constant STYLE_HEAD_1 => 2;
use constant STYLE_HEAD_2 => 3;
use constant STYLE_HEAD_3 => 4;
use constant STYLE_HEAD_4 => 5;
use constant STYLE_ITEM   => 6;
use constant STYLE_LINK   => 7;
use constant STYLE_VERBATIM => 8;
use constant STYLE_MAX_ID => 8;

# model layout indices
use constant M_TYPE        => 0; # T_XXXX
use constant M_INDENT      => 1; # pod-content driven indent
use constant M_TEXT_OFFSET => 2; # contains same info as BLK_TEXT_OFFSET
use constant M_FONT_ID     => 3; # 0 or 1 ( i.e., variable or fixed )
use constant M_START       => 4; # start of data, same purpose as BLK_START

# model entries
use constant T_NORMAL       => 0;
use constant T_VERBATIM_ON  => 1;
use constant T_VERBATIM_OFF => 2;

# topic layout indices
use constant T_MODEL_START => 0; # beginning of topic
use constant T_MODEL_END   => 1; # end of a topic
use constant T_DESCRIPTION => 2; # topic name
use constant T_STYLE       => 3; # style of STYLE_XXX
use constant T_ITEM_DEPTH  => 4; # depth of =item recursion
use constant T_LINK_OFFSET => 5; #

# formatting constants
use constant FORMAT_LINES    => 100;
use constant FORMAT_TIMEOUT  => 300;

$OP_LINK = tb::opcode(1, 'link');

sub model_create
{
	my %opt = @_;
	return (
		$opt{type}   // T_NORMAL,
		$opt{indent} // 0,
		$opt{offset} // 0,
		$opt{font}   // 0
	);
}

{
my %RNT = (
	%{Prima::TextView-> notification_types()},
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
			0x337ab7,               # link foreground
			$def-> {backColor},     # link background
			cl::Blue,               # code foreground
			0xf5f5f5,               # code background
		],
		images => [],
		styles => [
			{ fontId    => 1,                         # STYLE_CODE
			color     => COLOR_CODE_FOREGROUND, 
			backColor => COLOR_CODE_BACKGROUND
			},
			{ },                                      # STYLE_TEXT
			{ fontSize => 4, fontStyle => fs::Bold }, # STYLE_HEAD_1
			{ fontSize => 2, fontStyle => fs::Bold }, # STYLE_HEAD_2
			{ fontSize => 1, fontStyle => fs::Bold }, # STYLE_HEAD_3
			{ fontSize => 1, fontStyle => fs::Bold }, # STYLE_HEAD_4
			{ fontStyle => fs::Bold },                # STYLE_ITEM
			{ color     => COLOR_LINK_FOREGROUND},    # STYLE_LINK
			{ fontId    => 1,                         # STYLE_VERBATIM
			color     => COLOR_CODE_FOREGROUND,
			},
		],
		pageName      => '',
		topicView     => 0,
		textDirection  => $::application->textDirection,
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
	$self-> {manpath}  = '';
	$self-> {modelRange} = [0,0,0];
	$self-> {postBlocks} = {};
	$self-> {topics}     = [];
	$self-> {hasIndex}   = 0;
	$self-> {topicView}  = 0;
	$self-> {lastLinkPointer} = -1;
	my %profile = $self-> SUPER::init(@_);

	$self-> {contents} = [ Prima::PodView::Link-> new ];

	my %font = %{$self-> fontPalette-> [0]};
	$font{pitch} = fp::Fixed;
	$self-> {fontPalette}-> [1] = \%font;
	$self-> {fontPaletteSize} = 2;

	$self-> $_($profile{$_}) for qw( styles images pageName topicView);

	return %profile;
}

sub on_paint
{
	my ( $self, $canvas ) = @_;
	$self-> SUPER::on_paint($canvas);
	$self-> {contents}-> [0]-> on_paint( $self, $canvas, COLOR_LINK_FOREGROUND & ~tb::COLOR_INDEX )
		if $self->{lastLinkPointer} >= 0
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
# 	my ( $self, $linkPointer, $mouseButtonOrKeyEventIfZero, $mod, $x, $y) = @_;
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
			return undef if $where =~ /up|prev/ && $topic == 0; # contents
			my $tid = -1;
			my $t;
			for ( @{$self-> {topics}}) {
				$tid++;
				next unless $$_[T_MODEL_START] == $topic;
				$t = $_;
				last;
			}

			if ( $where =~ /next|prev/) {
				return undef unless defined $t;
				my $index = scalar @{$self-> {topics}} - 1;
				$tid += ( $where =~ /next/) ? 1 : -1;
				return undef if $tid < 0 || $tid > $index;
				$t = $self-> {topics}-> [$tid]-> [T_MODEL_START];
				return "$self->{pageName}|$t|0";
			}

			return "$self->{pageName}|0|0" unless defined $t;
			if ( $$t[ T_STYLE] >= STYLE_HEAD_1 && $$t[ T_STYLE] <= STYLE_HEAD_4) {
				$t = $self-> {topics}-> [0];
				return "$self->{pageName}|$$t[T_MODEL_START]|0"
			}
			my $state = $$t[ T_STYLE] - STYLE_HEAD_1 + $$t[ T_ITEM_DEPTH];
			$state-- if $state > 0;
			while ( $tid--) {
				$t = $self-> {topics}-> [$tid];
				$t = $$t[ T_STYLE] - STYLE_HEAD_1 + $$t[ T_ITEM_DEPTH];
				$t-- if $t > 0;
				next if $t >= $state;
				$t = $self-> {topics}-> [$tid]-> [T_MODEL_START];
				return "$self->{pageName}|$t|0";
			}
			# return index
			$t = $self-> {topics}-> [-1]-> [T_MODEL_START];
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
		for my $k ( @{$self-> {topics}}) {
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
		my ( $page, $section, $item, $lead_slash) = ( '', '', 1, '');
		my $default_topic = 0;

		if ( $s =~ /^file:\/\/(.*)$/) {
			$page = $1;
		} elsif ( $s =~ m{^([:\w]+)/?$} ) {
			$page = $1;
		} elsif ( $s =~ /^([^\/]*)(\/)(.*)$/) {
			( $page, $lead_slash, $section) = ( $1, $2, $3);
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
			if ( !defined $t || $t < 0) {
				$tid = -1;
				my $s = quotemeta $section;
				for ( @{$self-> {topics}}) {
					$tid++;
					next unless $$_[T_DESCRIPTION] =~ m/^$s/;
					next if !$item && $$_[T_STYLE] == STYLE_ITEM;
					$t = $tid;
					last;
				}
			}
			unless ( defined $t) { # no such topic, must be a page?
				$page = $lead_slash . $section;
				$section = '';
			}
		}
		if ( length $page and $page ne $self-> {pageName}) { # new page?
			if ( $self-> load_file( $page) != 1) {
				$self-> notify(q(Bookmark), $mark) if $mark;
				return 0;
			}
			$doBookmark = 1;
		}

		if ( ! defined $t) {
			$t = $default_topic if length $page && $self-> {topicView};
			my $tid = -1;
			for ( @{$self-> {topics}}) {
				$tid++;
				next unless $section eq $$_[T_DESCRIPTION];
				$t = $tid;
				last;
			}
			if ( length( $section) and ( !defined $t || $t < 0)) {
				$tid = -1;
				my $s = quotemeta $section;
				for ( @{$self-> {topics}}) {
					$tid++;
					next unless $$_[T_DESCRIPTION] =~ m/^$s/;
					$t = $tid;
					last;
				}
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
	my @mr1 = @{$self-> {modelRange}};
	if ( defined $t) {
		$self-> {modelRange} = [
			$$t[ T_MODEL_START],
			$$t[ T_MODEL_END],
			$$t[ T_LINK_OFFSET]
		]
	} else {
		$self-> {modelRange} = [ 0, scalar @{$self-> {model}} - 1, 0 ]
	}
	my @mr2 = @{$self-> {modelRange}};

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

sub textDirection
{
	return $_[0]-> {textDirection} unless $#_;
	my ( $self, $td ) = @_;
	$self-> {textDirection} = $td;
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

sub images
{
	return $_[0]-> {images} unless $#_;
	my ( $self, $images) = @_;
	$self-> {images} = $images;
	$self-> repaint;
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
			push @forw, $tb::{$_}-> ( $x-> {$_});
			push @rev,  $tb::{$_}-> ( 0);
		}
		push @styleInfo, \@forw, \@rev;
	}
	$self-> {styleInfo} = \@styleInfo;
}

sub message
{
	my ( $self, $message, $error) = @_;
	my $x;
	$self-> open_read( createIndex => 0 );
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
	$self-> {manpath} = '';
}

sub load_file
{
	my ( $self, $manpage) = @_;
	my $pageName = $manpage;
	my $path = '';

	unless ( -f $manpage) {
		my ( $fn, $mpath);
		my @ext =  ( '.pod', '.pm', '.pl' );
		push @ext, ( '.bat' ) if $^O =~ /win32/i;
		push @ext, ( '.com' ) if $^O =~ /VMS/;
		for ( map { $_, "$_/pod", "$_/pods" }
				grep { defined && length && -d }
					@INC,
					split( $Config::Config{path_sep}, $ENV{PATH})) {
			if ( -f "$_/$manpage") {
				$manpage = "$_/$manpage";
				$path = $_;
				last;
			}
			$fn = "$_/$manpage";
			$fn =~ s/::/\//g;
			$mpath = $fn;
			$mpath =~ s/\/[^\/]*$//;
			for ( @ext ) {
				if ( -f "$fn$_") {
					$manpage = "$fn$_";
					$path = $mpath;
					goto FOUND;
				}
			}
		}
	}
FOUND:

	unless ( open F, "< $manpage") {
		my $m = <<ERROR;
\=head1 Error

Error loading '$manpage' : $!

ERROR
		$m =~ s/^\\=/=/gm;
		undef $self-> {source_file};
		$self-> message( $m, 1);
		return 0;
	}

	$self-> pointer( cr::Wait);
	$self-> {manpath} = $path;
	$self-> {source_file} = $manpage;
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

sub load_content
{
	my ( $self, $content) = @_;
	my $path = '';
	$self-> {manpath} = '';
	undef $self-> {source_file};
	$self-> open_read;
	$self-> read($content);
	return $self-> close_read( $self-> {topicView});
}


sub open_read
{
	my ($self, @opt) = @_;
	return if $self-> {readState};
	$self-> clear_all;
	$self-> {readState} = {
		cutting       => 1,
		pod_cutting   => 1,
		begun         => '',
		bulletMode    => 0,

		indent        => DEF_INDENT,
		indentStack   => [],

		bigofs        => 0,
		wrapstate     => '',
		wrapindent    => 0,

		topicStack    => [[-1]],
		ignoreFormat  => 0,

		createIndex   => 1,
		encoding      => undef,
		bom           => undef,
		utf8          => undef,
		verbatim      => undef,

		@opt,
	};
}

sub load_image
{
	my ( $self, $src, $frame ) = @_;
	my $index = 0;
	unless ( -f $src) {
		$src =~ s!::!/!g;
		for my $path (
			map {( "$_", "$_/pod")}
			grep { defined && length && -d }
			( length($self-> {manpath}) ? $self-> {manpath} : (), @INC)
		) {
			return Prima::Icon-> load( "$path/$src", index => $frame, iconUnmask => 1)
				if -f "$path/$src" && -r _;
		}
	}
	return;
}

sub add_image
{
	my ( $self, $src, $w, $h, $cut ) = @_;

	$w = $src-> width unless $w;
	$h = $src-> height unless $h;
	my @resolution = $self-> resolution;
	$w *= 72 / $resolution[0];
	$h *= 72 / $resolution[1];
	$src-> {stretch} = [$w, $h];
	$self-> {readState}-> {pod_cutting} = $cut ? 0 : 1
		if defined $cut;

	my @imgop = (
		tb::wrap(tb::WRAP_MODE_OFF),
		tb::extend( $w, $h, tb::X_DIMENSION_POINT),
		tb::code( \&_imgpaint, $src),
		tb::moveto( $w, 0, tb::X_DIMENSION_POINT),
		tb::wrap(tb::WRAP_MODE_ON)
	);

	if ( @{$self-> {model}}) {
		push @{$self-> {model}-> [-1]}, @imgop;
	} else {
		push @{$self-> {model}}, [
			model_create(
				indent => $self-> {readState}-> {indent},
				offset => $self-> {readState}-> {bigofs}
			),
			@imgop
		];
	}
}

sub add_formatted
{
	my ( $self, $format, $text) = @_;

	return unless $self-> {readState};

	if ( $format eq 'text') {
		$self-> add($text,STYLE_CODE,0);
		$self-> add_new_line;
	} elsif ( $format eq 'podview') {
		while ( $text =~ m/<\s*([^<>]*)\s*>/gcs) {
			my $cmd = $1;
			if ( lc($cmd) eq 'cut') {
				$self-> {readState}-> {pod_cutting} = 0;
			} elsif ( lc($cmd) eq '/cut') {
				$self-> {readState}-> {pod_cutting} = 1;
			} elsif ( $cmd =~ /^img\s*(.*)$/i) {
				$cmd = $1;
				my ( $w, $h, $src, $frame, $cut);
				$frame = 0;
				while ( $cmd =~ m/\s*([a-z]*)\s*\=\s*(?:(?:'([^']*)')|(?:"([^"]*)")|(\S*))\s*/igcs) {
					my ( $option, $value) = ( lc $1, defined($2)?$2:(defined $3?$3:$4));
					if ( $option eq 'width' && $value =~ /^\d+$/) { $w = $value }
					elsif ( $option eq 'height' && $value =~ /^\d+$/) { $h = $value }
					elsif ( $option eq 'frame' && $value =~ /^\d+$/) { $frame = $value }
					elsif ( $option eq 'src') { $src = $value }
					elsif ( $option eq 'cut' ) { $cut = $value }
				}
				if ( defined $src) {
					my $img = $self->load_image($src, $frame);
					$self->add_image($img, $w, $h, $cut) if $img;
				} elsif ( defined $frame && defined $self->{images}->[$frame]) {
					$self->add_image($self->{images}->[$frame], $w, $h, $cut);
				}
			}
		}
	}
}

sub _imgpaint
{
	my ( $self, $canvas, $block, $state, $x, $y, $img) = @_;
	my ( $dx, $dy) = @{$img->{stretch}};
	my @res = $self-> resolution;
	$dx *= $res[0] / 72;
	$dy *= $res[1] / 72;
	$canvas-> stretch_image( $x, $y, $dx, $dy, $img);
	if ( $self-> {selectionPaintMode}) {
		my @save = ( fillPattern => $canvas-> fillPattern, rop => $canvas-> rop);
		$canvas-> set( fillPattern => fp::Borland, rop => rop::AndPut);
		$canvas-> bar( $x, $y, $x + $dx - 1, $y + $dy - 1);
		$canvas-> set( @save);
	}
}

sub _bulletpaint
{
	my ( $self, $canvas, $block, $state, $x, $y, $filled) = @_;
	$y -= $$block[ tb::BLK_APERTURE_Y];
	my $fh = $canvas-> font-> height * 0.3;
	$filled ?
		$canvas-> fill_ellipse( $x + $fh / 2, $y + $$block[ tb::BLK_HEIGHT] / 2, $fh, $fh) :
		$canvas-> ellipse     ( $x + $fh / 2, $y + $$block[ tb::BLK_HEIGHT] / 2, $fh, $fh);
}

sub read_paragraph
{
	my ( $self, $line ) = @_;
	my $r = $self-> {readState};

	for ( $line ) {
		if ($r-> {cutting}) {
			next unless /^=/;
			$r-> {cutting} = 0;
		}

		unless ($r-> {pod_cutting}) {
			next unless /^=/;
		}

		if ($r-> {begun}) {
			my $begun = $r-> {begun};
			if (/^=end\s+$begun/ || /^=cut/) {
				$r-> {begun} = '';
				$self-> add_new_line; # end paragraph
				$r-> {cutting} = 1 if /^=cut/;
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
			$self-> add_verbatim_mark(1) unless defined $r->{verbatim};
			$self-> add($_,STYLE_VERBATIM,$r-> {indent}) for split "\n", $_;
			$self-> add_new_line;
			next;
		}
		$self-> add_verbatim_mark(0);

		if (/^=for\s+(\S+)\s*(.*)/s) {
			$self-> add_formatted( $1, $2) if defined $2;
			next;
		} elsif (/^=begin\s+(\S+)\s*(.*)/s) {
			$r-> {begun} = $1;
			$self-> add_formatted( $1, $2) if defined $2;
			next;
		}

		if (s/^=//) {
			my ($Cmd, $args) = split(' ', $_, 2);
			$args = '' unless defined $args;
			if ($Cmd eq 'cut') {
				$r-> {cutting} = 1;
			}
			elsif ($Cmd eq 'pod') {
				$r-> {cutting} = 0;
			}
			elsif ($Cmd eq 'head1') {
				$self-> add( $args, STYLE_HEAD_1, DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'head2') {
				$self-> add( $args, STYLE_HEAD_2, DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'head3') {
				$self-> add( $args, STYLE_HEAD_3, DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'head4') {
				$self-> add( $args, STYLE_HEAD_4, DEF_FIRST_INDENT);
			}
			elsif ($Cmd eq 'over') {
				push(@{$r-> {indentStack}}, $r-> {indent});
				$r-> {indent} += ( $args =~ m/^(\d+)$/ ) ? $1 : DEF_INDENT;
			}
			elsif ($Cmd eq 'back') {
				$self-> _close_topic( STYLE_ITEM);
				$r-> {indent} = pop(@{$r-> {indentStack}}) || 0;
			}
			elsif ($Cmd eq 'item') {
				$self-> add( $args, STYLE_ITEM, $r-> {indentStack}-> [-1] || DEF_INDENT);
			}
			elsif ($Cmd eq 'encoding') {
				$r->{encoding} = Encode::find_encoding($args); # or undef
			}
		}
		else {
			s/\n/ /g;
			$self-> add($_, STYLE_TEXT, $r-> {indent});
		}

		$self-> add_new_line unless $r->{bulletMode};
	}
}

sub read
{
	my ( $self, $pod) = @_;
	my $r = $self-> {readState};
	return unless $r;

	unless ( defined $r->{bom} ) {
		if ( $pod =~ s/^(\x{ef}\x{bb}\x{bf})// ) { # don't care about other BOMs so far
			$r-> {bom} = $1;
			$r-> {encoding} = Encode::find_encoding('utf-8');
		}
	}

	my $odd = 0;
	for ( split ( "(\n)", $pod)) {
		next unless $odd = !$odd;
		$_ = $r->{encoding}->decode($_, Encode::FB_HTMLCREF) if $r->{encoding};

		if (defined $r-> {paragraph_buffer}) {
			if ( /^\s*$/) {
				my $pb = $r-> {paragraph_buffer};
				undef $r-> {paragraph_buffer};
				$self-> read_paragraph($pb);
			} else {
				$r-> {paragraph_buffer} .= "\n$_";
				next;
			}
		} elsif ( !/^$/) {
		    $r->{paragraph_buffer} = $_;
		    next;
		}
	}
}

sub close_read
{
	my ( $self, $topicView) = @_;
	return unless $self-> {readState};

	my $r = $self-> {readState};
	if ( defined $r->{paragraph_buffer}) {
		my $pb = $r-> {paragraph_buffer};
		undef $r-> {paragraph_buffer};
		$self-> read_paragraph("$pb\n");
	}

	$topicView = $self-> {topicView} unless defined $topicView;
	$self-> add_new_line; # end
	$self-> add_verbatim_mark(0);
	$self-> {contents}-> [0]-> references( $self-> {links});

	goto NO_INDEX unless $self-> {readState}-> {createIndex};

	my $secid = 0;
	my $msecid = scalar(@{$self-> {topics}});

	unless ( $msecid) {
		push @{$self-> {topics}}, [
			0, scalar @{$self-> {model}} - 1,
			"Document", STYLE_HEAD_1, 0, 0
		] if scalar @{$self-> {model}} > 2; # no =head's, but some info
		goto NO_INDEX;
	}

	## this code creates the Index section, adds it to the end of text,
	## and then uses black magic to put it in the front.

	# remember the current end state
	$self-> _close_topic( STYLE_HEAD_1);
	my @text_ends_at = (
		$r-> {bigofs},
		scalar @{$self->{model}},
		scalar @{$self->{topics}},
		scalar @{$self->{links}},
	);

	# generate index list
	my $ofs = $self-> {model}-> [$self-> {topics}-> [0]-> [T_MODEL_START]]-> [M_TEXT_OFFSET];
	my $firstText = substr( ${$self-> {text}}, 0, ( $ofs > 0) ? $ofs : 0);
	if ( $firstText =~ /[^\n\s\t]/) { # the 1st lines of text are not =head
		unshift @{$self-> {topics}}, [
			0, $self-> {topics}-> [0]-> [T_MODEL_START] - 1,
			"Preface", STYLE_HEAD_1, 0, 0
		];
		$text_ends_at[2]++;
		$msecid++;
	}
	my $start = scalar @{ $self->{model} };
	$self-> add_new_line;
	$self-> add_verbatim_mark(1);
	$self-> add( " Contents",  STYLE_HEAD_1, DEF_FIRST_INDENT);
	$self-> {hasIndex} = 1;
	$self-> {topics}->[-1]->[T_MODEL_START] = $start;
	my $last_style = STYLE_HEAD_1;
	for my $k ( @{$self-> {topics}}) {
		last if $secid == $msecid; # do not add 'Index' entry
		my ( $ofs, $end, $text, $style, $depth, $linkStart) = @$k;
		if ( $style == STYLE_ITEM ) {
			$style = $last_style;
		} else {
			$last_style = $style;
		}
		my $indent = DEF_INDENT + ( $style - STYLE_HEAD_1 + $depth ) * 2;
		$self-> add("L<$text|topic://$secid>", STYLE_TEXT, $indent);
		$secid++;
	}
	$self-> add_new_line;
	$self-> add_verbatim_mark(0);

	$self-> _close_topic( STYLE_HEAD_1);

	# remember the state after index is added
	my @index_ends_at = (
		$r-> {bigofs},
		scalar @{$self->{model}},
		scalar @{$self->{topics}},
		scalar @{$self->{links}},
	);

	# exchange places for index and body
	my @offsets = map { $index_ends_at[$_] - $text_ends_at[$_] } 0 .. 3;
	my $m = $self-> {model};
	# first shift the offsets
	$$_[M_TEXT_OFFSET] += $offsets[0]      for @$m[0..$text_ends_at[1]-1];
	$$_[M_TEXT_OFFSET] -= $text_ends_at[0] for @$m[$text_ends_at[1]..$index_ends_at[1]-1];
	# next reshuffle the model
	unshift @$m, splice( @$m, $text_ends_at[1]);
	# text
	my $t = $self-> {text};
	my $ts = substr( $$t, $text_ends_at[0]);
	substr( $$t, $text_ends_at[0]) = '';
	substr( $$t, 0, 0) = $ts;
	# topics
	$t = $self-> {topics};
	for ( @$t[0..$text_ends_at[2]-1]) {
		$$_[T_MODEL_START] += $offsets[1];
		$$_[T_MODEL_END]   += $offsets[1];
		$$_[T_LINK_OFFSET] += $offsets[3];
	}
	for ( @$t[$text_ends_at[2]..$index_ends_at[2]-1]) {
		$$_[T_MODEL_START] -= $text_ends_at[1];
		$$_[T_MODEL_END]   -= $text_ends_at[1];
		$$_[T_LINK_OFFSET] -= $text_ends_at[3];
	}
	unshift @$t, splice( @$t, $text_ends_at[2]);
	# update the map of blocks that contain OP_LINKs
	$self-> {postBlocks} = {
		map {
			( $_ >= $text_ends_at[1]) ?
			( $_ - $text_ends_at[1] ) :
			( $_ + $offsets[1] ),
			1
		} keys %{$self-> {postBlocks}}
	};
	# links
	my $l = $self-> {links};
	s/^(topic:\/\/)(\d+)$/$1 . ( $2 + $offsets[2])/e for @$l;
	unshift @{$self->{links}}, splice( @{$self->{links}}, $text_ends_at[3]);

NO_INDEX:
	# finalize
	undef $self-> {readState};
	$self-> {lastLinkPointer} = -1;

	my $topic;
	$topic = $self-> {topics}-> [$msecid] if $topicView;
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
	my $state = ( $style >= STYLE_HEAD_1 && $style <= STYLE_HEAD_4) ?
		0 : scalar @{$r-> {indentStack}};

	if ( $state <= $$t[-1]-> [0]) {
		while ( scalar @$t && $state <= $$t[-1]-> [0]) {
			my $nt = pop @$t;
			$nt = $$nt[1];
			$$nt[ T_MODEL_END] = scalar @{$self-> {model}} - 1;
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

	$p =~ s/\n//g;
	my $g = [ model_create( indent => $indent, offset => $r-> {bigofs}) ];
	my $styles = $self-> {styles};
	my $no_push_block;
	my $itemid = scalar @{$self-> {model}};

	if ( $r-> {bulletMode}) {
		if ( $style == STYLE_TEXT || $style == STYLE_CODE || $style == STYLE_VERBATIM) {
			return unless length $p;
			$g = $self-> {model}-> [-1];
			$$g[M_TEXT_OFFSET] = $r-> {bigofs};
			$no_push_block = 1;
			$itemid--;
		}
		$r-> {bulletMode} = 0;
	}

	if ( $style == STYLE_CODE || $style == STYLE_VERBATIM) {
		$$g[ M_FONT_ID] = $styles-> [$style]-> {fontId} || 1; # fixed font
		push @$g, tb::wrap(tb::WRAP_MODE_OFF);
	}

	push @$g, @{$self-> {styleInfo}-> [$style * 2]};
	$cstyle = $styles-> [$style]-> {fontStyle} || 0;

	if ( $style == STYLE_CODE || $style == STYLE_VERBATIM) {
		push @$g, tb::text( 0, length $p),
	} elsif (( $style == STYLE_ITEM) && ( $p =~ /^\*\s*$/ || $p =~ /^\d+\.?$/)) {
		push @$g,
			tb::wrap(tb::WRAP_MODE_OFF),
			tb::color(0),
			tb::code( \&_bulletpaint, ($p =~ /^\*\s*$/) ? 1 : 0),
			tb::moveto( 1, 0, tb::X_DIMENSION_FONT_HEIGHT),
			tb::wrap(tb::WRAP_MODE_ON);
		$r-> {bulletMode} = 1;
		$p = '';
	} else { # wrapable text
		$p =~ s/[\s\t]+/ /g;
		$p =~ s/([\200-\377])/"E<".ord($1).">"/ge;
		$p =~ s/(E<[^<>]+>)/noremap($1)/ge;
		$p =~ s/([:A-Za-z_][:A-Za-z_0-9]*\([^\)]*\))/C<$1>/g;
		my $maxnest = 10;
		my $linkStart = scalar @{$self-> {links}};
		my $m = $p;
		my @ids = ( [-2, 'Z', 2], [ length($m), 'z', 1]);
		while ( $maxnest--) {
			while ( $m =~ m/([A-Z])(<<+) /gcs) {
				my ( $pos, $cmd, $left, $right) = ( pos($m), $1, $2, ('>' x ( length($2))));
				if ( $m =~ m/\G.*? $right(?!>)/gcs) {
					if ( $cmd eq 'X') {
						my $d = length($cmd) + length($left) + 1;
						substr( $m, $pos - $d, pos($m) - $pos + $d, '');
					} else {
						push @ids, [
							$pos - length($left) - 2,
							$cmd,
							length($cmd)+length($left)
						], [
							pos($m) - length($right),
							lc $cmd,
							length($right)
						];
						substr $m, $ids[$_][0], $ids[$_][2], '_' x $ids[$_][2]
							for -2,-1;
					}
				}
			}
			while ( $m =~ m/([A-Z])<([^<>]*)>/gcs) {
				if ( $1 eq 'X') {
					my $d = length($2) + length($1) + 2;
					substr( $m, pos($m) - $d, $d, '');
				} else {
					push @ids,
						[ pos($m) - length($2) - 3, $1, 2],
						[ pos($m) - 1, lc $1, 1];
					substr $m, $ids[$_][0], $ids[$_][2], '_' x $ids[$_][2] for -2,-1;
				}
			}
			last unless $m =~ m/[A-Z]</;
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
			my $ofs = $$_[0] + $$_[2];
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
					unless ( $s =~ /^\w+\:\/\//) {
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
					push @$g, tb::wrap( $val{wrap} = tb::WRAP_MODE_OFF);
					my $z = $styles-> [STYLE_CODE];
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @{$stack{$_}}, $val{$_};
						push @$g, $tb::{$_}-> ( $val{$_} = $z-> {$_});
					}
				} elsif ( $$_[1] eq 'L') {
					my $z = $styles-> [STYLE_LINK];
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @{$stack{$_}}, $val{$_};
						push @$g, $tb::{$_}-> ( $val{$_} = $z-> {$_});
					}
					unless ($link) {
						push @$g, $OP_LINK, $link = 1;
						$linkHREF = '';
					}
				} elsif ( $$_[1] eq 'S') {
					push @{$stack{wrap}}, $val{wrap};
					push @$g, tb::wrap( $val{wrap} = tb::WRAP_MODE_OFF);
				} elsif ( $$_[1] eq 'B') {
					push @{$stack{fontStyle}}, $val{fontStyle};
					push @$g, tb::fontStyle( $val{fontStyle} |= fs::Bold);
				}
			} else { # close
				if ( $$_[1] eq 'i' || $$_[1] eq 'f' || $$_[1] eq 'b') {
					push @$g, tb::fontStyle( $val{fontStyle} = pop @{$stack{fontStyle}});
				} elsif ( $$_[1] eq 'c') {
					my $z = $styles-> [STYLE_CODE];
					push @$g, tb::wrap( $val{wrap} = pop @{$stack{wrap}});
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @$g, $tb::{$_}-> ( $val{$_} = pop @{$stack{$_}});
					}
				} elsif ( $$_[1] eq 'l') {
					my $z = $styles-> [STYLE_LINK];
					for ( qw( fontId fontStyle fontSize color backColor)) {
						next unless exists $z-> {$_};
						push @$g, $tb::{$_}-> ( $val{$_} = pop @{$stack{$_}});
					}
					if ( $link) {
						push @$g, $OP_LINK, $link = 0;
						push @{$self-> {links}}, $linkHREF;
						$self-> {postBlocks}-> { $itemid} = 1;
					}
				} elsif ( $$_[1] eq 's') {
					push @$g, tb::wrap( $val{wrap} = pop @{$stack{wrap}});
				}
			}
		}
		if ( $link) {
			push @$g, $OP_LINK, $link = 0;
			push @{$self-> {links}}, $linkHREF;
			$self-> {postBlocks}-> { $itemid} = 1;
		}

		# add topic
		if (
	        	( $style >= STYLE_HEAD_1 && $style <= STYLE_HEAD_4 ) ||
			(( $style == STYLE_ITEM) && $p !~ /^[0-9*]+\.?$/)
		) {
			my $itemDepth = ( $style == STYLE_ITEM) ?
				scalar @{$r-> {indentStack}} : 0;
			my $pp = $p;
			$pp =~ s/\|//g;
			$pp =~ s/([<>])/'E<' . (($1 eq '<') ? 'lt' : 'gt') . '>'/ge;
			if ( $style == STYLE_ITEM && $pp =~ /^\s*[a-z]/) {
				$pp =~ s/([\s\)\(\[\]\{\}].*)$/C<$1>/; # seems like function entry?
			}
			my $newTopic = [ $itemid, 0, $pp, $style, $itemDepth, $linkStart];
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
	push @{$self-> {model}}, $g unless $no_push_block;
}

sub add_new_line
{
	my $self = $_[0];
	my $r = $self-> {readState};
	return unless $r;
	my $p = " \n";
	${$self-> {text}} .= $p;
	push @{$self-> {model}}, [ model_create( offset => $r->{bigofs} ), tb::text(0, 1) ];
	$r-> {bigofs} += length $p;
}

sub add_verbatim_mark
{
	my ($self, $on) = @_;
	my $r = $self-> {readState};
	return unless $r;

	my $mark;
	if ( $on ) {
		return if defined $r->{verbatim};
		$mark = T_VERBATIM_ON;
		$r->{verbatim} = 1;
	} else {
		return unless defined $r->{verbatim};
		$mark = T_VERBATIM_OFF;
		undef $r->{verbatim};
	}

	push @{$self-> {model}}, [ model_create(type => $mark) ];
}

sub stop_format
{
	my $self = $_[0];
	$self-> {formatTimer}-> destroy if $self-> {formatTimer};
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
			if ( $self-> {blocks}-> [$bid]) {
				$autoOffset = $ofs + $self-> {blocks}-> [$bid]-> [tb::BLK_TEXT_OFFSET];
			}
		}
	}

	$self-> stop_format;
	$self-> selection(-1,-1,-1,-1);

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

	for my $fid ( 0 .. ( scalar @{$self-> fontPalette} - 1)) {
		$$state[ tb::BLK_FONT_ID] = $fid;
		$self-> realize_state( $self, $state, tb::REALIZE_FONTS);
		$indents[$fid] = $self-> font-> width;
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
		verbatim      => undef,
		last_ymap     => 0,
	};

	$self-> {formatTimer} = $self-> insert( Timer =>
		name        => 'FormatTimer',
		delegations => [ 'Tick' ],
		timeout     => FORMAT_TIMEOUT,
	) unless $self-> {formatTimer};

	$self-> paneSize(0,0);
	$self-> {formatTimer}-> start;
	$self-> select_text_offset( $autoOffset) if $autoOffset;

	while ( 1) {
		$self-> format_chunks;
		last unless 
			$self-> {formatData} && 
			$self-> {blocks}-> [-1] &&
			$self-> {blocks}-> [-1]-> [tb::BLK_Y] < $ph;
	}
}

sub FormatTimer_Tick
{
	$_[0]-> format_chunks
}

sub paint_code_div
{
	my ( $self, $canvas, $block, $state, $x, $y, $coord) = @_;
	my $f  = $canvas->font;
	my $w = $coord->[0];
	my $h = $coord->[1];
	my @x = ( $canvas-> backColor, $canvas-> color );
	$canvas->set(backColor => $self->{colorMap}->[5], color => 0xcccccc);
	$canvas->new_path->round_rect($x, $y, $x + $w, $y + $h, 20)->fill_stroke;
	$canvas-> set( backColor => $x[0], color => $x[1] );
}

sub add_code_div
{
	my ($self, $from, $to) = @_;

	my ($w,$y1,$y2) = (0,($self->{blocks}->[$from]->[tb::BLK_Y]) x 2);
	for my $b ( @{ $self->{blocks} } [$from .. $to] ) {
		$w = $$b[tb::BLK_X] + $$b[tb::BLK_WIDTH]  if $w < $$b[tb::BLK_X] + $$b[tb::BLK_WIDTH];
		$y1 = $$b[tb::BLK_Y] if $y1 > $$b[tb::BLK_Y];
		$y2 = $$b[tb::BLK_Y] + $$b[tb::BLK_HEIGHT] if $y2 < $$b[tb::BLK_Y] + $$b[tb::BLK_HEIGHT];
	}
	my ($fh, $fw) = ( $self->font->height, $self->font->width );
	my $h = $y2 - $y1;
	my $b = tb::block_create();
	$$b[tb::BLK_X] = $self->{blocks}->[$from]->[tb::BLK_X];
	$$b[tb::BLK_Y] = $y1 - $fh / 2;
	$w += 2 * $fw;
	$$b[tb::BLK_WIDTH]  = $w;
	$$b[tb::BLK_HEIGHT] = $h;
	$$b[tb::BLK_TEXT_OFFSET] = -1;
	push @$b,
		tb::code( \&paint_code_div, [$w, $h]),
		tb::extend($w, $h);
	return $b;
}

sub format_chunks
{
	my $self = $_[0];

	my $f = $self-> {formatData};

	my $time = time;
	$self-> begin_paint_info;

	my $mid = $f-> {current};
	my $postBlocks = $self-> {postBlocks};
	my $max = $f-> {current} + $f-> {step};
	$max = $f-> {max} if $max > $f-> {max};
	my $indents   = $f-> {indents};
	my $state     = $f-> {state};
	my $linkRects = $f-> {linkRects};
	my $formatWidth = $f-> {formatWidth};
	my $fw = $self->font->width;

	for ( ; $mid <= $max; $mid++) {
		my $g = tb::block_create();
		my $m = $self-> {model}-> [$mid];

		if ( $m->[M_TYPE] == T_VERBATIM_ON ) {
			$f->{verbatim} = scalar @{ $self->{blocks} };
			next;
		} elsif ( $m->[M_TYPE] == T_VERBATIM_OFF ) {
			splice @{ $self->{blocks} },
				$f->{verbatim}, 0,
				$self-> add_code_div( $f->{verbatim}, $#{$self->{blocks}} );
			undef $f->{verbatim};
			next;
		}

		my @blocks;
		$$g[ tb::BLK_TEXT_OFFSET] = $$m[M_TEXT_OFFSET];
		$$g[ tb::BLK_Y] = undef;
		push @$g, @$m[ M_START .. $#$m ];

		# format the paragraph

		my $next_text_offs = ( $mid == $#{$self->{model}} ) ? length( ${$self->{text}} ) : $self->{model}->[$mid + 1]->[M_TEXT_OFFSET];
		my $indent = $$m[M_INDENT] * $$indents[ $$m[M_FONT_ID]];
		@blocks = $self-> block_wrap( $self, $g, $state, $formatWidth - $indent);

		# adjust size
		for ( @blocks) {
			if ( $self->{textDirection} ) {
				$$_[ tb::BLK_X] = $f->{paneWidth} - $$_[ tb::BLK_WIDTH] - $indent;
			} else {
				$$_[ tb::BLK_X] += $indent;
			}
			$f-> {paneWidth} = $$_[ tb::BLK_X] + $$_[ tb::BLK_WIDTH]
				if $$_[ tb::BLK_X] + $$_[ tb::BLK_WIDTH] > $f-> {paneWidth};
		}

		# check links
		if ( $postBlocks-> {$mid}) {
			my $linkState = 0;
			my $linkStart = 0;
			my @rect;
			for my $b ( @blocks) {
				my @pos = ( $$b[tb::BLK_X], 0 );

				if ( $linkState) {
					$rect[0] = $$b[ tb::BLK_X];
					$rect[1] = $$b[ tb::BLK_Y];
				}

				$self-> block_walk( $b,
					position => \@pos,
					trace => tb::TRACE_POSITION,
					link  => sub {
						if ( $linkState = shift ) {
							$rect[0] = $pos[0];
							$rect[1] = $$b[ tb::BLK_Y];
						} else {
							$rect[2] = $pos[0] + $fw;
							$rect[3] = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
							push @$linkRects, [ @rect, $f-> {linkId} ++ ];
						}
					},
				);

				if ( $linkState) {
					$rect[2] = $pos[0];
					$rect[3] = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
					push @$linkRects, [ @rect, $f-> {linkId}];
				}
			}
		}

		# push back
		push @{$self-> {blocks}}, @blocks;
	}

	my $paneHeight = 0;
	my @settopline;
	if ( scalar @{$self-> {blocks}}) {
		my $b = $self-> {blocks}-> [-1];
		$paneHeight = $$b[ tb::BLK_Y] + $$b[ tb::BLK_HEIGHT];
		if ( defined $f-> {position} &&
			! $f-> {positionSet} &&
			$self-> {topLine} == 0 &&
			$self-> {offset} == 0 &&
			$$b[ tb::BLK_TEXT_OFFSET] >= $f-> {position}) {
			$b = $self-> text_offset2block( $f-> {position});
			$f-> {positionSet} = 1;
			if ( defined $b) {
				$b = $self-> {blocks}-> [$b];
				@settopline = @$b[ tb::BLK_X, tb::BLK_Y];
			}
		}
	}

	$f-> {current} = $mid;
	$self-> end_paint_info;

	if ( ! defined $f->{verbatim} ){
		$self-> recalc_ymap( $f->{last_ymap} );
		$f->{last_ymap} = scalar @{ $self->{blocks} };
		if ( $f->{suppressed_ymap} ) {
			$f->{suppressed_ymap} = 0;
			$self->repaint;
		}
	} else {
		$f->{suppressed_ymap} = 1;
	}

	my $ps = $self-> {paneWidth};
	if ( $ps != $f-> {paneWidth}) {
		$self-> paneSize( $f-> {paneWidth}, $paneHeight);
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

	$self-> stop_format if $mid >= $f-> {max};
	$f-> {step} *= 2 unless time - $time;
}

sub print
{
	my ( $self, $canvas, $callback) = @_;

	my ( $min, $max, $linkIdStart) = @{$self-> {modelRange}};
	return 1 if $min >= $max;
	my $ret = 0;

	goto ABORT if $callback && ! $callback-> ();

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
        my $hmargin = $formatWidth  / 24;
        my $vmargin = $formatHeight / 12;
        $formatWidth  -= $hmargin * 2;
        $formatHeight -= $vmargin * 2;
        $canvas->translate( $hmargin, $vmargin );

	my $mid = $min;
	my $y = $formatHeight;

	my $pageno = 1;
	my $pagenum  = sub {
		$canvas->translate( 0, 0 );
		my %save = %{$canvas->font};
		$canvas->font->set( name => $self->fontPalette->[0]->{name} || 'Default', size => 6, style => 0, pitch => fp::Default );
		$canvas->set( color => cl::Black );
		$canvas->text_out( $pageno, ( $formatWidth - $canvas->get_text_width($pageno) ) / 2, ($vmargin - $canvas->font->height ) / 2 );
		delete $save{height}; # XXX fix this
		$canvas->font(\%save);
		$pageno++;
	};
	my $new_page = sub {
		goto ABORT if $callback && ! $callback-> ();
		$pagenum->();
		goto ABORT unless $canvas-> new_page;
		$canvas->translate( $hmargin, $vmargin );
	};

	for ( ; $mid <= $max; $mid++) {
		my $g = tb::block_create();
		my $m = $self-> {model}-> [$mid];
		next if $$m[M_TYPE] != T_NORMAL; # don't print div background

		my @blocks;
		$$g[ tb::BLK_TEXT_OFFSET] = $$m[M_TEXT_OFFSET];
		$$g[ tb::BLK_Y] = undef;
		push @$g, @$m[ M_START .. $#$m ];

		# format the paragraph
		my $indent = $$m[M_INDENT] * $indents[ $$m[M_FONT_ID]];
		@blocks = $self-> block_wrap( $canvas, $g, $state, $formatWidth - $indent);

		# paint
		$self-> reset_state;
		for ( @blocks) {
			my $b = $_;
			if ( $y < $$b[ tb::BLK_HEIGHT]) {
				if ( $$b[ tb::BLK_HEIGHT] < $formatHeight) {
					$new_page->();
					$y = $formatHeight - $$b[ tb::BLK_HEIGHT];
					$self-> block_draw( $canvas, $b, $indent, $y);
				} else {
					$y -= $$b[ tb::BLK_HEIGHT];
					while ( $y < 0) {
						$new_page->();
						$self-> block_draw( $canvas, $b, $indent, $y);
						$y += $formatHeight;
					}
				}
			} else {
				$y -= $$b[ tb::BLK_HEIGHT];
				goto ABORT unless $self-> block_draw( $canvas, $b, $indent, $y);
			}
		}
	}
	$pagenum->();

	$ret = 1;
ABORT:
	return $ret;
}

sub select_text_offset
{
	my ( $self, $pos) = @_;
	if ( defined $self-> {formatData}) {
		my $last = $self-> {blocks}-> [-1];
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
	$range[0] = $self-> {model}-> [ $self-> {modelRange}-> [0]]-> [M_TEXT_OFFSET];
	$range[1] = ( $self-> {modelRange}-> [1] + 1 >= scalar @{$self-> {model}}) ?
		length ( ${$self-> {text}} ) :
		$self-> {model}-> [ $self-> {modelRange}-> [1] + 1]-> [M_TEXT_OFFSET];
	$range[1]-- if $range[1] > $range[0];
	return @range;
}

%HTML_Escapes = (
	'amp'	=>	'&',	#   ampersand
	'lt'	=>	'<',	#   left chevron, less-than
	'gt'	=>	'>',	#   right chevron, greater-than
	'quot'	=>	'"',	#   double quote

	"Aacute"=>	"\xC1",	#   capital A, acute accent
	"aacute"=>	"\xE1",	#   small a, acute accent
	"Acirc"	=>	"\xC2",	#   capital A, circumflex accent
	"acirc"	=>	"\xE2",	#   small a, circumflex accent
	"AElig"	=>	"\xC6",	#   capital AE diphthong (ligature)
	"aelig"	=>	"\xE6",	#   small ae diphthong (ligature)
	"Agrave"=>	"\xC0",	#   capital A, grave accent
	"agrave"=>	"\xE0",	#   small a, grave accent
	"Aring"	=>	"\xC5",	#   capital A, ring
	"aring"	=>	"\xE5",	#   small a, ring
	"Atilde"=>	"\xC3",	#   capital A, tilde
	"atilde"=>	"\xE3",	#   small a, tilde
	"Auml"	=>	"\xC4",	#   capital A, dieresis or umlaut mark
	"auml"	=>	"\xE4",	#   small a, dieresis or umlaut mark
	"Ccedil"=>	"\xC7",	#   capital C, cedilla
	"ccedil"=>	"\xE7",	#   small c, cedilla
	"Eacute"=>	"\xC9",	#   capital E, acute accent
	"eacute"=>	"\xE9",	#   small e, acute accent
	"Ecirc"	=>	"\xCA",	#   capital E, circumflex accent
	"ecirc"	=>	"\xEA",	#   small e, circumflex accent
	"Egrave"=>	"\xC8",	#   capital E, grave accent
	"egrave"=>	"\xE8",	#   small e, grave accent
	"ETH"	=>	"\xD0",	#   capital Eth, Icelandic
	"eth"	=>	"\xF0",	#   small eth, Icelandic
	"Euml"	=>	"\xCB",	#   capital E, dieresis or umlaut mark
	"euml"	=>	"\xEB",	#   small e, dieresis or umlaut mark
	"Iacute"=>	"\xCD",	#   capital I, acute accent
	"iacute"=>	"\xED",	#   small i, acute accent
	"Icirc"	=>	"\xCE",	#   capital I, circumflex accent
	"icirc"	=>	"\xEE",	#   small i, circumflex accent
	"Igrave"=>	"\xCD",	#   capital I, grave accent
	"igrave"=>	"\xED",	#   small i, grave accent
	"Iuml"	=>	"\xCF",	#   capital I, dieresis or umlaut mark
	"iuml"	=>	"\xEF",	#   small i, dieresis or umlaut mark
	"Ntilde"=>	"\xD1",	#   capital N, tilde
	"ntilde"=>	"\xF1",	#   small n, tilde
	"Oacute"=>	"\xD3",	#   capital O, acute accent
	"oacute"=>	"\xF3",	#   small o, acute accent
	"Ocirc"	=>	"\xD4",	#   capital O, circumflex accent
	"ocirc"	=>	"\xF4",	#   small o, circumflex accent
	"Ograve"=>	"\xD2",	#   capital O, grave accent
	"ograve"=>	"\xF2",	#   small o, grave accent
	"Oslash"=>	"\xD8",	#   capital O, slash
	"oslash"=>	"\xF8",	#   small o, slash
	"Otilde"=>	"\xD5",	#   capital O, tilde
	"otilde"=>	"\xF5",	#   small o, tilde
	"Ouml"	=>	"\xD6",	#   capital O, dieresis or umlaut mark
	"ouml"	=>	"\xF6",	#   small o, dieresis or umlaut mark
	"szlig"	=>	"\xDF",		#   small sharp s, German (sz ligature)
	"THORN"	=>	"\xDE",	#   capital THORN, Icelandic
	"thorn"	=>	"\xFE",	#   small thorn, Icelandic
	"Uacute"=>	"\xDA",	#   capital U, acute accent
	"uacute"=>	"\xFA",	#   small u, acute accent
	"Ucirc"	=>	"\xDB",	#   capital U, circumflex accent
	"ucirc"	=>	"\xFB",	#   small u, circumflex accent
	"Ugrave"=>	"\xD9",	#   capital U, grave accent
	"ugrave"=>	"\xF9",	#   small u, grave accent
	"Uuml"	=>	"\xDC",	#   capital U, dieresis or umlaut mark
	"uuml"	=>	"\xFC",	#   small u, dieresis or umlaut mark
	"Yacute"=>	"\xDD",	#   capital Y, acute accent
	"yacute"=>	"\xFD",	#   small y, acute accent
	"yuml"	=>	"\xFF",	#   small y, dieresis or umlaut mark

	"lchevron"=>	"\xAB",	#   left chevron (double less than)
	"rchevron"=>	"\xBB",	#   right chevron (double greater than)
);

1;

__END__

=pod

=head1 NAME

Prima::PodView - POD browser widget

=head1 SYNOPSIS

	use Prima qw(Application PodView);

	my $window = Prima::MainWindow-> create;
	my $podview = $window-> insert( 'Prima::PodView',
		pack => { fill => 'both', expand => 1 }
	);
	$podview-> open_read;
	$podview-> read("=head1 NAME\n\nI'm also a pod!\n\n");
	$podview-> close_read;

	run Prima;

=for podview <img src="podview.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/podview.gif">

=head1 DESCRIPTION

Prima::PodView contains a formatter ( in terms of L<perlpod> ) and viewer of
POD content. It heavily employs its ascendant class L<Prima::TextView>,
and is in turn base for the toolkit's default help viewer L<Prima::HelpViewer>.

=head1 USAGE

The package consists of the several logically separated parts. These include
file locating and loading, formatting and navigation.

=head2 Content methods

The basic access to the content is not bound to the file system. The POD
content can be supplied without any file to the viewer. Indeed, the file
loading routine C<load_file> is a mere wrapper to the content loading
functions:

=over

=item open_read %OPTIONS

Clears the current content and enters the reading mode. In this mode
the content can be appended by calling L<read> that pushes the raw POD
content to the parser.

=item read TEXT

Supplies TEXT string to the parser. Manages basic indentation,
but the main formatting is performed inside L<add> and L<add_formatted>

Must be called only within open_read/close_read brackets

=item add TEXT, STYLE, INDENT

Formats TEXT string of a given STYLE ( one of C<STYLE_XXX> constants) with
INDENT space.

Must be called only within open_read/close_read brackets.

=item add_formatted FORMAT, TEXT

Adds a pre-formatted TEXT with a given FORMAT, supplied by C<=begin> or C<=for>
POD directives. Prima::PodView understands 'text' and 'podview' FORMATs;
the latter format is for Prima::PodView itself and contains small number
of commands, aimed at inclusion of images into the document.

The 'podview' commands are:

=over

=item cut

Example:

	=for podview <cut>

	=for text just text-formatter info

		....
		text-only info
		...

	=for podview </cut>

The E<lt>cut<gt> clause skips all POD input until cancelled.
It is used in conjunction with the following command, L<img>, to allow
a POD manpage provide both graphic ('podview', 'html', etc ) and text ( 'text' )
content.

=item img [src="SRC"] [width="WIDTH"] [height="HEIGHT"] [cut="CUT"] [frame="FRAME"]

An image inclusion command, where src is a relative or an absolute path to
an image file. In case if scaling is required, C<width> and C<height> options
can be set. When the image is a multiframe image, the frame index can be
set by C<frame> option. Special C<cut> option, if set to a true value, activates the
L<cut> behavior if ( and only if ) the image load operation was unsuccessful.
This makes possible simultaneous use of 'podview' and 'text' :

	=for podview <img src="graphic.gif" cut=1 >

	=begin text

	y     .
	|  .
	|.
	+----- x

	=end text

	=for podview </cut>

In the example above 'graphic.gif' will be shown if it can be found and loaded,
otherwise the poor-man-drawings would be selected.

If "src" is omitted, image is retrieved from C<images> array, from the index C<frame>.

=back


=item close_read

Closes the reading mode and starts the text rendering by calling C<format>.
Returns C<undef> if there is no POD context, 1 otherwise.

=back

=head2 Rendering

The rendering is started by C<format> call, which returns ( almost ) immediately,
initiating the mechanism of delayed rendering, which is often time-consuming.
C<format>'s only parameter KEEP_OFFSET is a boolean flag, which, if set to 1,
remembers the current location on a page, and when the rendered text approaches
the location, scrolls the document automatically.

The rendering is based an a document model, generated by open_read/close_read session.
The model is a set of same text blocks defined by L<Prima::TextView>, except
that the header length is only three integers:

	M_INDENT       - the block X-axis indent
	M_TEXT_OFFSET  - same as BLK_TEXT_OFFSET
	M_FONT_ID      - 0 or 1, because PodView's fontPalette contains only two fonts -
	                 variable ( 0 ) and fixed ( 1 ).

The actual rendering is performed in C<format_chunks>, where model blocks are
transformed to the full text blocks, wrapped and pushed into TextView-provided
storage. In parallel, links and the corresponding event rectangles are calculated
on this stage.

=head2 Topics

Prima::PodView provides the C<::topicView> property, which governs whether
the man page is viewed by topics or as a whole. When it is viewed as topics,
C<{modelRange}> array selects the model blocks that include the topic.
Thus, having a single model loaded, text blocks change dynamically.

Topics contained in C<{topics}> array, each is an array with indices of C<T_XXX>
constants:

	T_MODEL_START - beginning of topic
	T_MODEL_END   - length of a topic
	T_DESCRIPTION - topic name
	T_STYLE       - STYLE_XXX constant
	T_ITEM_DEPTH  - depth of =item recursion
	T_LINK_OFFSET - offset in links array, started in the topic

=head2 Styles

C<::styles> property provides access to the styles, applied to different pod
text parts. These styles are:

	STYLE_CODE     - style for C<>
	STYLE_TEXT     - normal text
	STYLE_HEAD_1   - =head1
	STYLE_HEAD_2   - =head2
	STYLE_HEAD_3   - =head3
	STYLE_HEAD_4   - =head4
	STYLE_ITEM     - =item
	STYLE_LINK     - style for L<> text
	STYLE_VERBATIM - style for pre-formatted text

Each style is a hash with the following keys: C<fontId>, C<fontSize>, C<fontStyle>,
C<color>, C<backColor>, fully analogous to the tb::BLK_DATA_XXX options.
This functionality provides another layer of accessibility to the pod formatter.

In addition to styles, Prima::PodView defined C<colormap> entries for
C<STYLE_LINK> , C<STYLE_CODE>, and C<STYLE_VERBATIM>:

	COLOR_LINK_FOREGROUND
	COLOR_LINK_BACKGROUND
	COLOR_CODE_FOREGROUND
	COLOR_CODE_BACKGROUND

The default colors in the styles are mapped into these entries.

=head2 Link and navigation methods

Prima::PodView provides a hand-icon mouse pointer highlight for the link
entries and as an interactive part, the link documents or topics are loaded
when the user presses the mouse button on the link. The mechanics below that
is as follows. C<{contents}> of event rectangles, ( see L<Prima::TextView> )
is responsible for distinguishing whether a mouse is inside a link or not.
When the link is activated, C<link_click> is called, which, in turn, calls
C<load_link> method. If the page is loaded successfully, depending on C<::topicView>
property value, either C<select_topic> or C<select_text_offset> method is called.

The family of file and link access functions consists of the following methods:

=over

=item load_file MANPAGE

Loads a manpage, if it can be found in the PATH or perl installation directories.
If unsuccessful, displays an error.

=item load_link LINK

LINK is a text in format of L<perlpod> C<LE<lt>E<gt>> link: "manpage/section".
Loads the manpage, if necessary, and selects the section.

=item load_bookmark BOOKMARK

Loads a bookmark string, prepared by L<make_bookmark> function.
Used internally.

=item load_content CONTENT

Loads content into the viewer. Returns C<undef> is there is no POD
context, 1 otherwise.

=item make_bookmark [ WHERE ]

Combines the information about the currently viewing manpage, topic and text offset
into a storable string. WHERE, an optional string parameter, can be either omitted,
in such case the current settings are used, or be one of 'up', 'next' or 'prev' strings.

The 'up' string returns a bookmark to the upper level of the manpage.

The 'next' and 'prev' return a bookmark to the next or the previous topics in a manpage.

If the location cannot be stored or defined, C<undef> is returned.

=back

=head2 Events

=over

=item Bookmark BOOKMARK

When a new topic is navigated to by the user, this event is triggered with the
current topic to have it eventually stored in bookmarks or history.

=item Link LINK_REF, BUTTON, MOD, X, Y

When the user clicks on a link, this event is called with the link address,
mouse button, modificator keys, and coordinates.

=item NewPage

Called after new content is loaded

=back

=cut
