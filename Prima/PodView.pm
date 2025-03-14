package Prima::PodView;
use strict;
use warnings;

package
	Prima::PodView::Renderer;
use base qw(Prima::Drawable::Pod);

sub img_paint
{
	my ( $self, $canvas, $block, $state, $x, $y, $img) = @_;
	$self->SUPER::img_paint( $canvas, $block, $state, $x, $y, $img);
	return unless $canvas-> {selectionPaintMode};

	my ( $dx, $dy) = @{$img->{stretch}};
	my @r = $canvas->resolution;
	$dx *= $r[0] / 72;
	$dy *= $r[1] / 72;
	$canvas-> graphic_context(
		color             => $canvas->backColor,
		fillPattern       => fp::SimpleDots,
		fillPatternOffset => [$x, $y],
		sub { $canvas-> bar( $x, $y, $x + $dx - 1, $y + $dy - 1) }
	);
}

sub end_page
{
	my $self = shift;
	my $cb   = $self->{print_callback};
	return $cb ? $cb->() : 1;
}

package
	Prima::PodView;
use Config;
use Encode;
use Prima qw(Utils TextView Widget::Link Image::base64);

use vars qw(@ISA);
@ISA = qw(Prima::TextView);

# 0 and 1 are reserved in TextView for FG anf BG
use constant COLOR_INDEX_LINK_FG     => 2;
use constant COLOR_INDEX_CODE_FG     => 3;
use constant COLOR_INDEX_CODE_BG     => 4;
use constant COLOR_LINK_FOREGROUND   => COLOR_INDEX_LINK_FG | tb::COLOR_INDEX;
use constant COLOR_CODE_FOREGROUND   => COLOR_INDEX_CODE_FG | tb::COLOR_INDEX;
use constant COLOR_CODE_BACKGROUND   => COLOR_INDEX_CODE_BG | tb::COLOR_INDEX;

# formatting constants
use constant FORMAT_LINES    => 300;
use constant FORMAT_TIMEOUT  => 300;

{
my %RNT = (
	%{Prima::TextView-> notification_types()},
	%{Prima::Widget::Link-> notification_types()},
	Bookmark => nt::Default,
	NewPage  => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub default_styles
{
	return (
		{ fontId    => 1,                         # STYLE_CODE
		color     => COLOR_CODE_FOREGROUND,
		backColor => COLOR_CODE_BACKGROUND
		},
		{ },                                      # STYLE_TEXT
		{ fontSize => 4, fontStyle => fs::Bold }, # STYLE_HEAD_1
		{ fontSize => 2, fontStyle => fs::Bold }, # STYLE_HEAD_2
		{ fontSize => 1, fontStyle => fs::Bold }, # STYLE_HEAD_3
		{ fontSize => 1, fontStyle => fs::Bold }, # STYLE_HEAD_4
		{ fontSize => 0, fontStyle => fs::Bold }, # STYLE_HEAD_5
		{ fontSize => -1, fontStyle => fs::Bold },# STYLE_HEAD_6
		{ fontStyle => fs::Bold },                # STYLE_ITEM
		{ color     => COLOR_LINK_FOREGROUND},    # STYLE_LINK
		{ fontId    => 1,                         # STYLE_VERBATIM
		color     => COLOR_CODE_FOREGROUND,
		},
	);
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;

	my %prf = (
		colorMap => [
			$def-> {color},
			$def-> {backColor},
			Prima::PodView::Renderer->default_colors,
		],
		styles        => [$_[0]->default_styles],
		pageName      => '',
		topicView     => 0,
		textDirection => $::application->textDirection,
		justify       => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);

	unless ( exists $p->{colorMap} && (exists $p->{color} || exists $p->{backColor})) {
		my $cm = $default->{colorMap};
		$cm->[0] = $p->{color} if exists $p->{color};
		$cm->[1] = $cm->[3] = $p->{backColor} if exists $p->{backColor};
	}
}

sub init
{
	my $self = shift;
	$self-> {pageName} = '';
	$self-> {modelRange} = [0,0,0];
	$self-> {topicView}  = 0;
	$self-> {justify}  = 0;
	$self-> {link_handler} = Prima::Widget::Link-> new;
	$self-> {contents} = [ $self->{link_handler} ];
	my %profile = $self-> SUPER::init(@_);

	$self-> {pod_handler}  = Prima::PodView::Renderer-> new(
		styles  => $profile{styles},
	);

	my %font = %{$self-> fontPalette-> [0]};
	$font{pitch} = fp::Fixed;
	$self-> {fontPalette}-> [1] = \%font;
	$self-> {fontPaletteSize} = 2;

	$self-> $_($profile{$_}) for qw( pageName topicView justify);

	return %profile;
}

sub colorMap
{
	return $_[0]->SUPER::colorMap unless $#_;
	my ( $self, $cm) = @_;
	$self-> {link_handler}->color( $$cm[ COLOR_INDEX_LINK_FG ] );
	$self-> SUPER::colorMap($cm);
}

sub on_paint
{
	my ( $self, $canvas ) = @_;
	$self-> SUPER::on_paint($canvas);
	$self-> {link_handler}-> on_paint( $self, $canvas);
}

sub on_size
{
	my ( $self, $oldx, $oldy, $x, $y) = @_;
	$self-> SUPER::on_size( $oldx, $oldy, $x, $y);
	$self-> format(keep_offset => 1) if $oldx != $x;
}

sub on_fontchanged
{
	my $self = $_[0];
	$self-> SUPER::on_fontchanged;
	$self-> format(keep_offset => 1);
}

sub on_link
{
	my ( $self, $link_handler, $url, $btn, $mod) = @_;
	return if $btn != mb::Left;
	$self-> load_link( $url );
}

sub on_linkpreview
{
	my ( $self, $link_handler, $url) = @_;
	if ( $$url =~ /^([^|]+)$/) {
		$$url = "pod://$1";
	} else {
		$$url = '';
		$self->clear_event;
	}
}

sub on_linkadjustrect
{
	my ( $self, $link_handler, $rc) = @_;
	my ($dx, $dy) = $self->point2screen(0,0);
	$$rc[$_] = $dx + $$rc[$_] for 0,2;
	$$rc[$_] = $dy - $$rc[$_] for 1,3;
	@$rc[1,3] = @$rc[3,1];
	$self->clear_event;
}

sub link_handler { shift->{link_handler} }
sub pod_handler  { shift->{pod_handler}  }

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
			my $topics = $self->{pod_handler}->topics;
			for ( @$topics ) {
				$tid++;
				next unless $$_[pod::T_MODEL_START] == $topic;
				$t = $_;
				last;
			}

			if ( $where =~ /next|prev/) {
				return undef unless defined $t;
				my $index = @$topics - 1;
				$tid += ( $where =~ /next/) ? 1 : -1;
				return undef if $tid < 0 || $tid > $index;
				$t = $topics-> [$tid]-> [pod::T_MODEL_START];
				return "$self->{pageName}|$t|0";
			}

			return "$self->{pageName}|0|0" unless defined $t;
			if ( $$t[ pod::T_STYLE] >= pod::STYLE_HEAD_1 && $$t[ pod::T_STYLE] <= pod::STYLE_HEAD_6) {
				$t = $topics-> [0];
				return "$self->{pageName}|$$t[pod::T_MODEL_START]|0"
			}
			my $state = $$t[ pod::T_STYLE] - pod::STYLE_HEAD_1 + $$t[ pod::T_ITEM_DEPTH];
			$state-- if $state > 0;
			while ( $tid--) {
				$t = $topics-> [$tid];
				$t = $$t[ pod::T_STYLE] - pod::STYLE_HEAD_1 + $$t[ pod::T_ITEM_DEPTH];
				$t-- if $t > 0;
				next if $t >= $state;
				$t = $topics-> [$tid]-> [pod::T_MODEL_START];
				return "$self->{pageName}|$t|0";
			}
			# return index
			$t = $topics-> [-1]-> [pod::T_MODEL_START];
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
		for my $k ( @{$self-> {pod_handler}->topics}) {
			next if $$k[pod::T_MODEL_START] != $topic;
			$self-> select_topic($k);
			last;
		}
	}
	$self-> select_text_offset( $ofs);

	return 1;
}

sub load_link
{
	my ( $self, $s, %opt) = @_;

	my $mark = $self-> make_bookmark;
	my %link = $self->{pod_handler}->parse_link($s);
	my $doBookmark;

	if ( defined $link{file} and $link{file} ne $self-> {pageName}) { # new page?
		if ( $self-> load_file( $link{file}, %opt) != 1) {
			$self-> notify(q(Bookmark), $mark) if $mark;
			return 0;
		}
		%link = $self->{pod_handler}->parse_link($s);
		$doBookmark = 1;
	}

	if ( defined ( my $t = $link{topic})) {
		if ( $self-> {topicView}) {
			$self-> select_topic($t, %opt);
		} else {
			$self-> select_text_offset(
				$self-> {pod_handler}->model-> [$$t[ pod::T_MODEL_START]]-> [pod::M_TEXT_OFFSET]
			);
		}
		$self-> notify(q(Bookmark), $mark) if $mark;
		return 1;
	} elsif ( $doBookmark) {
		$self-> notify(q(Bookmark), $mark) if $mark;
		return 1;
	}

	return 0;
}

# selects a sub-page; does not check if topicView,
# so must be called with care
sub select_topic
{
	my ( $self, $t, %opt) = @_;
	my @mr1 = @{$self-> {modelRange}};
	if ( defined $t) {
		$self-> {modelRange} = [
			$$t[ pod::T_MODEL_START],
			$$t[ pod::T_MODEL_END],
			$$t[ pod::T_LINK_OFFSET]
		];
	} else {
		$self-> {modelRange} = [ 0, $self-> {pod_handler}->model_length - 1, 0 ]
	}

	return unless $opt{format} // 1;

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

sub justify
{
	return $_[0]-> {justify} unless $#_;
	my ( $self, $justify) = @_;
	$justify = ( $justify ? 1 : 0);
	return if $self-> {justify} == $justify;
	$self-> {justify} = $justify;
	$self-> format(keep_offset => 1);
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

sub images
{
	return $_[0]-> {pod_handler}->images unless $#_;
	my ( $self, $images) = @_;
	$self-> {pod_handler}->images($images);
	$self-> repaint;
}

sub message
{
	my ( $self, $message, $error) = @_;
	my $x;
	$self-> open_read( create_index => 0 );
	if ( $error) {
		$x = $self-> {pod_handler}->styles-> [pod::STYLE_HEAD_1]-> {color};
		$self-> {pod_handler}->styles-> [pod::STYLE_HEAD_1]-> {color} = cl::Red;
		$self-> {pod_handler}->update_styles;
	}
	$self-> read($message);
	$self-> close_read( 0);
	if ( $error) {
		my $z = $self-> {pod_handler}->styles-> [pod::STYLE_HEAD_1];
		defined $x ? $z-> {color} = $x : delete $z-> {color};
		$self-> {pod_handler}->update_styles;
	}
	$self-> pageName('');
	$self-> {pod_handler}->manpath('');
}

sub load_file
{
	my ( $self, $manpage, %opt) = @_;
	my $pageName = $manpage;
	my $path = '';
	my $ok = 1;

	unless (-f $manpage) {
		my ($new_manpage, $new_path) = $self->{pod_handler}->podpath2file($manpage);
		if ( defined $new_manpage ) {
			($manpage, $path) = ($new_manpage, $new_path);
		} else {
			$ok = 0;
		}
	} elsif ( $manpage =~ m/^(.*)[\\\/]([^\\\/]+)/ ) {
		$path = $1;
	}


	my $f;
	unless ( $ok && open $f, "<", $manpage) {
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
	$self-> {pod_handler}->manpath($path);
	$self-> {source_file} = $manpage;
	$self-> open_read(%opt);
	$self-> read($_) while <$f>;
	close $f;

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
	$self-> {pod_handler}->manpath('');
	undef $self-> {source_file};
	$self-> open_read;
	$self-> read($content);
	return $self-> close_read( $self-> {topicView});
}

sub open_read
{
	my ($self, @opt) = @_;
	return if $self-> {pod_handler}-> is_reading;
	$self-> clear_all;
	$self-> {pod_handler}-> open_read(@opt);
	$self-> {text} = $self->{pod_handler}-> text_ref;
}

sub read { $_[0]->{pod_handler}->read($_[1]) }

sub close_read
{
	my ( $self, $topicView) = @_;
	return unless $self-> {pod_handler}->is_reading;

	my $r = $self-> {pod_handler}->close_read // {};

	$self-> {contents}-> [0]-> references( $self-> {pod_handler}-> links);

	my $topic;
	$topicView //= $self-> {topicView};
	$topic = $self-> {pod_handler}->topics-> [$r->{topic_id}]
		if $topicView and defined $r->{topic_id};
	$self-> select_topic( $topic, format => 1 );
	$self-> notify(q(NewPage));

	return $r->{success};
}

sub stop_format
{
	my $self = $_[0];
	$self-> {pod_handler}->end_format;
	$self-> {formatTimer}-> destroy if $self-> {formatTimer};
	undef $self-> {formatData};
	undef $self-> {formatTimer};
}

sub format
{
	my ( $self, %opt) = @_;
	my ( $pw, $ph) = $self-> get_active_area(2);

	my $autoOffset;
	if ( $opt{keep_offset}) {
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

	my $paneHeight = 0;
	my ( $min, $max, $linkIdStart) = @{$self-> {modelRange}};
	if ( $min >= $max) {
		$self-> {blocks} = [];
		$self-> link_handler->clear_positions;
		$self-> paneSize(0,0);
		return;
	}

	$self-> {blocks} = [];
	$self-> link_handler->clear_positions;

	$self-> begin_paint_info;
	$self->{pod_handler}->begin_format(
		canvas            => $self,
		width             => $pw,
		fontmap           => $self->fontPalette,
		justify           => $self->{justify},
		resolution        => [$self->resolution],
		text_direction    => $self->textDirection,
		exportable        => $opt{exportable},
		default_font_size => $self->{defaultFontSize},
	);
	$self-> end_paint_info;

	$self-> {formatData} = {
		linkId            => $linkIdStart,
		min               => $min,
		max               => $max,
		current           => $min,
		step              => FORMAT_LINES,
		position          => undef,
		positionSet       => 0,
		verbatim          => undef,
		last_ymap         => 0,
		exportable        => $opt{exportable},
	};

	if ( !$opt{sync} ) {
		$self-> {formatTimer} = $self-> insert( Timer =>
			name        => 'FormatTimer',
			delegations => [ 'Tick' ],
			timeout     => FORMAT_TIMEOUT,
		) unless $self-> {formatTimer};
		$self-> {formatTimer}-> start;
	}

	$self-> paneSize(0,0);
	$self-> select_text_offset( $autoOffset) if $autoOffset;

	while ( 1) {
		$self-> format_chunks;
		last unless 
			$self-> {formatData} && 
			$self-> {blocks}-> [-1] &&
			$self-> {blocks}-> [-1]-> [tb::BLK_Y] < $ph;
	}

	do {} while $opt{sync} && $self-> format_chunks;
}

sub FormatTimer_Tick
{
	$_[0]-> format_chunks
}

sub paint_code_div
{
	my ( $self, $canvas, $block, $state, $x, $y, $coord) = @_;
	my $f  = $canvas->font;
	my ($style, $w, $h, $corner) = @$coord;
	my %save = map { $_ => $canvas-> $_() } qw(color);
	my $path = $canvas->new_path->round_rect(
		$x, $y,
		$x + $w, $y + $h,
		$corner
	);
	if ( $style == pod::TDIVSTYLE_SOLID ) {
		$save{backColor} = $canvas->backColor;
		$canvas->set(
			color     => 0xcccccc,
			backColor => $self->{colorMap}->[ COLOR_INDEX_CODE_BG ],
		);
		$path->fill_stroke;
	} else {
		$canvas->set(color => 0x808080);
		$path->stroke;
	}
	$canvas-> set(%save);
}

sub add_code_div
{
	my ($self, $style, $from, $to) = @_;

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
	$$b[tb::BLK_Y] = int( $y1 - $fh / 2 + .5);
	$w += 2 * $fw;
	$$b[tb::BLK_WIDTH]  = $w;
	$$b[tb::BLK_HEIGHT] = $h;
	$$b[tb::BLK_TEXT_OFFSET] = -1;

	push @$b,
		tb::code( \&paint_code_div, [$style, $w, $h, $self->{defaultFontSize} * 2 * $self->{resolution}->[0] / 96.0]),
		tb::extend($w, $h);
	return $b;
}

sub format_chunks
{
	my $self = $_[0];

	my $f = $self-> {formatData} or return 0;

	my $time = time;
	$self-> begin_paint_info;

	my $mid = $f-> {current};
	my $link_map = $self-> {pod_handler}->link_map;
	my $max = $f-> {current} + $f-> {step};
	$max = $f-> {max} if $max > $f-> {max};
	my $model = $self->{pod_handler}->model;

	for ( ; $mid <= $max; $mid++) {

		my $m = $model-> [$mid];
		if (( $m->[pod::M_TYPE] & pod::T_TYPE_MASK) == pod::T_DIV ) {
			next if $f->{exportable};
			if ( $m->[pod::MDIV_TAG] == pod::TDIVTAG_OPEN) {
				$f->{verbatim} = scalar @{ $self->{blocks} };
			} else {
				splice @{ $self->{blocks} },
					$f->{verbatim}, 0,
					$self-> add_code_div( $m->[pod::MDIV_STYLE], $f->{verbatim}, $#{$self->{blocks}} );
				undef $f->{verbatim};
			}
			next;
		}

		my @blocks = $self->{pod_handler}->format_model($m);

		# check links
		if ( $link_map-> {$mid}) {
			$f->{linkId} = $self-> {link_handler}-> add_positions_from_blocks($f->{linkId}, \@blocks);
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
	my $ww = $self-> {pod_handler}->accumulated_width_overrun;
	if ( $ps != $ww ) {
		$self-> paneSize( $ww, $paneHeight);
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

	$f-> {step} *= 2 unless time - $time;

	my $ret = 1;
	$self-> stop_format, $ret = 0 if $mid >= $f-> {max};
	return $ret;
}

sub print
{
	my ( $self, $canvas, $_callback) = @_;
	return unless $_callback-> ();

	my ($min, $max) = @{$self-> {modelRange}};
	local $self->{print_callback} = $_callback;
	return $self->{pod_handler}->print(
		from           => $min,
		to             => $max,
		canvas         => $canvas,
		fontmap        => $self->fontPalette,
		justify        => $self->{justify},
		text_direction => $self->textDirection,
	);
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
	$self-> {pod_handler}->reset;
	$self-> {modelRange} = [0,0,0];
}

sub text_range
{
	my $self = $_[0];
	my @range;
	my $pod = $self->{pod_handler};
	$range[0] = $pod-> model-> [ $self-> {modelRange}-> [0]]-> [pod::M_TEXT_OFFSET];
	$range[1] = ( $self-> {modelRange}-> [1] + 1 >= $self->{pod_handler}->model_length ) ?
		$pod->text_length :
		$pod->model->[$self-> {modelRange}-> [1] + 1]-> [pod::M_TEXT_OFFSET];
	$range[1]-- if $range[1] > $range[0];
	return @range;
}

1;

__END__

=pod

=head1 NAME

Prima::PodView - POD browser widget

=head1 SYNOPSIS

	use Prima qw(Application PodView);

	my $window = Prima::MainWindow-> new;
	my $podview = $window-> insert( 'Prima::PodView',
		pack => { fill => 'both', expand => 1 }
	);
	$podview-> open_read;
	$podview-> read("=head1 NAME\n\nI'm also a pod!\n\n");
	$podview-> close_read;

	Prima->run;

=for podview <img src="podview.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/podview.gif">

=head1 DESCRIPTION

Prima::PodView contains a formatter ( in terms of L<perlpod> ) and a viewer of
the POD content. It heavily employs its ascendant class L<Prima::TextView>, and
is in turn the base class for the toolkit's default help viewer L<Prima::HelpViewer>.

=head1 USAGE

The package consists of several logically separated parts. These include
file locating and loading, formatting, and navigation.

=head2 Content methods

The basic access to the content is not bound to the file system. The POD
content can be supplied without any file to the viewer. Indeed, the file
loading routine C<load_file> is a mere wrapper to the following content-loading
functions:

=over

=item open_read %OPTIONS

Clears the current content and enters the reading mode. In this mode, the
content can be appended by repeatedly calling the C<read> method that pushes
the raw POD content to the parser.

=item read TEXT

Supplies the TEXT string to the parser. Parses basic indentation,
but the main formatting is performed inside L<add> and L<add_formatted>.

Must be called only within the open_read/close_read brackets

=item close_read

Closes the reading mode and starts the text rendering by calling C<format>.
Returns C<undef> if there is no POD context, 1 otherwise.

=back

=head2 Rendering

The rendering is started by the C<format> call which returns almost immediately,
initiating the mechanism of delayed rendering, which is often time-consuming.
C<format>'s only parameter KEEP_OFFSET is a boolean flag, which, if set to 1,
remembers the current location on a page, and when the rendered text approaches
the location, scrolls the document automatically.

The rendering is based on a document model, generated by the open_read/close_read session.
The model is a set of the same text blocks defined by L<Prima::TextView>, except
that the header length is only three integers:

	pod::M_INDENT       - the block X-axis indent
	pod::M_TEXT_OFFSET  - same as BLK_TEXT_OFFSET
	pod::M_FONT_ID      - 0 or 1, because PodView's fontPalette contains only two fonts -
	                 variable ( 0 ) and fixed ( 1 ).

The actual rendering is performed in C<format_chunks>, where model blocks are
transformed into text blocks, wrapped, and pushed into the TextView-provided
storage. In parallel, links and the corresponding event rectangles are calculated
at this stage.

=head2 Topics

Prima::PodView provides the C<::topicView> property, which manages whether
the man page is viewed by topics or as a whole. When a page is in the single topic mode,
the C<{modelRange}> array selects the model blocks that include the topic to be displayed.
That way the model stays the same while text blocks inside the widget can be changed.

=head2 Styles

In addition to styles provided by L<Prima::Drawable::Pod>, Prima::PodView defines C<colorMap> entries for
C<pod::STYLE_LINK> , C<pod::STYLE_CODE>, and C<pod::STYLE_VERBATIM>:

	COLOR_LINK_FOREGROUND
	COLOR_CODE_FOREGROUND
	COLOR_CODE_BACKGROUND

The default colors in the styles are mapped into these entries.

=head2 Link and navigation methods

Prima::PodView provides the hand-icon mouse pointer that highlights links.
Also, the link documents or topics are loaded in the widget when the user
presses the mouse button on the link. L<Prima::Widget::Link> is used for the
implementation of the link mechanics.

If the page is loaded successfully, depending on the C<::topicView> property value,
either the C<select_topic> or C<select_text_offset> method is called.

The family of file and link access functions consists of the following methods:

=over

=item load_file MANPAGE

Loads the manpage if it can be found in the PATH or perl installation directories.
If unsuccessful, displays an error.

=item load_link LINK

LINK is a text in the format of L<perlpod> C<LE<lt>E<gt>> link: "manpage/section".
Loads the manpage, if necessary, and selects the section.

=item load_bookmark BOOKMARK

Loads the bookmark string prepared by the L<make_bookmark> function.
Used internally.

=item load_content CONTENT

Loads content into the viewer. Returns C<undef> if there is no POD
context, 1 otherwise.

=item make_bookmark [ WHERE ]

Combines the information about the currently viewing page source, topic, and
text offset, into a storable string. WHERE, an optional string parameter, can
be either omitted, in such case the current settings are used, or be one of the
'up', 'next', or 'prev' strings.

The 'up' string returns a bookmark to the upper level of the manpage.

The 'next' and 'prev' return a bookmark to the next or the previous topics in the
manpage.

If the location cannot be stored or defined, C<undef> is returned.

=back

=head2 Events

=over

=item Bookmark BOOKMARK

When a new topic is navigated by the user, this event is triggered with the
current topic to have it eventually stored in the bookmarks or user history.

=item Link LINK_REF, BUTTON, MOD, X, Y

When the user clicks on a link, this event is called with the link address,
mouse button, modification keys, and coordinates.

=item NewPage

Called after new content is loaded

=back

=head1 SEE ALSO

L<Prima::Drawable::Pod>, L<Prima::TextView>

=cut
