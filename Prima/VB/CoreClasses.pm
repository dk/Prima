package Prima::VB::CoreClasses;
use strict;
use warnings;
use Prima qw(ComboBox Sliders);

sub classes
{
	return (
		'Prima::Button' => {
			RTModule => 'Prima::Buttons',
			class  => 'Prima::VB::Button',
			page   => 'General',
			icon   => 'VB::classes.gif:0',
		},
		'Prima::SpeedButton' => {
			RTModule => 'Prima::Buttons',
			class  => 'Prima::VB::Button',
			page   => 'Additional',
			icon   => 'VB::classes.gif:19',
		},
		'Prima::Label' => {
			RTModule => 'Prima::Label',
			class  => 'Prima::VB::Label',
			page   => 'General',
			icon   => 'VB::classes.gif:15',
		},
		'Prima::InputLine' => {
			RTModule => 'Prima::InputLine',
			class  => 'Prima::VB::InputLine',
			page   => 'General',
			icon   => 'VB::classes.gif:13',
		},
		'Prima::ListBox' => {
			RTModule => 'Prima::Lists',
			class  => 'Prima::VB::ListBox',
			page   => 'General',
			icon   => 'VB::classes.gif:16',
		},
		'Prima::DirectoryListBox' => {
			RTModule => 'Prima::Dialog::FileDialog',
			class  => 'Prima::VB::DirectoryListBox',
			page   => 'Additional',
			icon   => 'VB::classes.gif:6',
		},
		'Prima::ListViewer' => {
			RTModule => 'Prima::Lists',
			class  => 'Prima::VB::ListViewer',
			page   => 'Abstract',
			icon   => 'VB::classes.gif:16',
		},
		'Prima::CheckBox' => {
			RTModule => 'Prima::Buttons',
			class  => 'Prima::VB::CheckBox',
			page   => 'General',
			icon   => 'VB::classes.gif:2',
		},
		'Prima::Radio' => {
			RTModule => 'Prima::Buttons',
			class  => 'Prima::VB::Radio',
			page   => 'General',
			icon   => 'VB::classes.gif:18',
		},
		'Prima::GroupBox' => {
			RTModule => 'Prima::Buttons',
			class  => 'Prima::VB::GroupBox',
			page   => 'General',
			icon   => 'VB::classes.gif:10',
		},
		'Prima::ScrollBar' => {
			RTModule => 'Prima::ScrollBar',
			class  => 'Prima::VB::ScrollBar',
			page   => 'General',
			icon   => 'VB::classes.gif:20',
		},
		'Prima::ComboBox' => {
			RTModule => 'Prima::ComboBox',
			class    => 'Prima::VB::ComboBox',
			page     => 'General',
			icon     => 'VB::classes.gif:3',
		},
		'Prima::DriveComboBox' => {
			RTModule => 'Prima::Dialog::FileDialog',
			class  => 'Prima::VB::DriveComboBox',
			page   => 'Additional',
			icon   => 'VB::classes.gif:5',
		},
		'Prima::ColorComboBox' => {
			RTModule => 'Prima::Dialog::ColorDialog',
			class  => 'Prima::VB::ColorComboBox',
			page   => 'Additional',
			icon   => 'VB::classes.gif:1',
		},
		'Prima::Edit' => {
			RTModule => 'Prima::Edit',
			class  => 'Prima::VB::Edit',
			page   => 'General',
			icon   => 'VB::classes.gif:8',
		},
		'Prima::ImageViewer' => {
			RTModule => 'Prima::ImageViewer',
			class  => 'Prima::VB::ImageViewer',
			page   => 'General',
			icon   => 'VB::classes.gif:14',
		},
		'Prima::Widget::ScrollWidget' => {
			RTModule => 'Prima::Widget::ScrollWidget',
			class  => 'Prima::VB::ScrollWidget',
			page   => 'Abstract',
			icon   => 'VB::classes.gif:21',
		},
		'Prima::SpinButton' => {
			RTModule => 'Prima::Sliders',
			class  => 'Prima::VB::SpinButton',
			page   => 'Sliders',
			icon   => 'VB::classes.gif:23',
		},
		'Prima::AltSpinButton' => {
			RTModule => 'Prima::Sliders',
			class  => 'Prima::VB::AltSpinButton',
			page   => 'Sliders',
			icon   => 'VB::classes.gif:24',
		},
		'Prima::SpinEdit' => {
			RTModule => 'Prima::Sliders',
			class  => 'Prima::VB::SpinEdit',
			page   => 'Sliders',
			icon   => 'VB::classes.gif:25',
		},
		'Prima::Gauge' => {
			RTModule => 'Prima::Sliders',
			class  => 'Prima::VB::Gauge',
			page   => 'Sliders',
			icon   => 'VB::classes.gif:9',
		},
		'Prima::ProgressBar' => {
			RTModule => 'Prima::Sliders',
			class  => 'Prima::VB::ProgressBar',
			page   => 'Sliders',
			icon   => 'VB::classes.gif:34',
		},
		'Prima::Slider' => {
			RTModule => 'Prima::Sliders',
			class  => 'Prima::VB::Slider',
			page   => 'Sliders',
			icon   => 'VB::classes.gif:22',
		},
		'Prima::CircularSlider' => {
			RTModule => 'Prima::Sliders',
			class  => 'Prima::VB::CircularSlider',
			page   => 'Sliders',
			icon   => 'VB::classes.gif:4',
		},
		'Prima::StringOutline' => {
			RTModule => 'Prima::Outlines',
			class  => 'Prima::VB::StringOutline',
			page   => 'General',
			icon   => 'VB::classes.gif:17',
		},
		'Prima::OutlineViewer' => {
			RTModule => 'Prima::Outlines',
			class  => 'Prima::VB::OutlineViewer',
			page   => 'Abstract',
			icon   => 'VB::classes.gif:17',
		},
		'Prima::DirectoryOutline' => {
			RTModule => 'Prima::Outlines',
			class  => 'Prima::VB::DirectoryOutline',
			page   => 'Additional',
			icon   => 'VB::classes.gif:7',
		},
		'Prima::Notebook' => {
			RTModule => 'Prima::Notebooks',
			class  => 'Prima::VB::Notebook',
			page   => 'Abstract',
			icon   => 'VB::classes.gif:29',
		},
		'Prima::TabSet' => {
			RTModule => 'Prima::Notebooks',
			class  => 'Prima::VB::TabSet',
			page   => 'Additional',
			icon   => 'VB::classes.gif:27',
		},
		'Prima::TabbedNotebook' => {
			RTModule => 'Prima::Notebooks',
			class  => 'Prima::VB::TabbedNotebook',
			page   => 'Additional',
			icon   => 'VB::classes.gif:28',
		},
		'Prima::Widget::Header' => {
			icon     => 'VB::classes.gif:30',
			RTModule => 'Prima::Widget::Header',
			page     => 'Sliders',
			module   => 'Prima::VB::CoreClasses',
			class    => 'Prima::VB::Header',
		},
		'Prima::DetailedList' => {
			icon     => 'VB::classes.gif:31',
			RTModule => 'Prima::DetailedList',
			page     => 'General',
			module   => 'Prima::VB::CoreClasses',
			class    => 'Prima::VB::DetailedList',
		},
		'Prima::Calendar' => {
			icon     => 'VB::classes.gif:32',
			RTModule => 'Prima::Calendar',
			page     => 'Additional',
			module   => 'Prima::VB::CoreClasses',
			class    => 'Prima::VB::Calendar',
		},
		'Prima::Grid' => {
			RTModule => 'Prima::Grids',
			class  => 'Prima::VB::Grid',
			page   => 'General',
			icon   => 'VB::classes.gif:33',
		},
		'Prima::DetailedOutline' => {
			icon     => 'VB::classes.gif:35',
			page     => 'Additional',
			class    => 'Prima::VB::DetailedOutline',
			RTModule => 'Prima::DetailedOutline',
			module   => 'Prima::VB::CoreClasses',
		},
		'Prima::Widget::Date' => {
			icon     => 'VB::classes.gif:36',
			page     => 'Additional',
			class    => 'Prima::VB::Date',
			RTModule => 'Prima::Widget::Date',
			module   => 'Prima::VB::CoreClasses',
		},
		'Prima::CheckList' => {
			icon     => 'VB::classes.gif:37',
			page     => 'Additional',
			class    => 'Prima::VB::CheckList',
			RTModule => 'Prima::ExtLists',
			module   => 'Prima::VB::CoreClasses',
		},
		'Prima::KeySelector' => {
			icon     => 'VB::classes.gif:38',
			page     => 'Additional',
			class    => 'Prima::VB::KeySelector',
			RTModule => 'Prima::KeySelector',
			module   => 'Prima::VB::CoreClasses',
		},
		'Prima::FrameSet' => {
			icon     => 'VB::classes.gif:39',
			page     => 'Sliders',
			class    => 'Prima::VB::FrameSet',
			RTModule => 'Prima::FrameSet',
			module   => 'Prima::VB::CoreClasses',
		},
		'Prima::Widget::Time' => {
			icon     => 'VB::classes.gif:40',
			page     => 'Additional',
			class    => 'Prima::VB::Time',
			RTModule => 'Prima::Widget::Time',
			module   => 'Prima::VB::CoreClasses',
		},
	);
}

use Prima::Classes;
use Prima::StdBitmap;

package Prima::VB::CommonControl;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Control);

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		firstClick
		palette
		sizeMax
		sizeMin
		ownerColor
		ownerFont
		ownerHint
		ownerBackColor
		ownerShowHint
		ownerPalette
	);
}

package Prima::VB::Button;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw(flat vertical default checkable checked autoRepeat autoHeight autoWidth)],
		uiv     => [qw(glyphs defaultGlyph hiliteGlyph disabledGlyph pressedGlyph
			holdGlyph imageScale borderWidth )],
		modalResult  => ['modalResult'],
		icon    => ['image',],
		text    => ['hotKey'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onClick',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub caption_box
{
	my ($self,$canvas) = @_;
	my $cap = $self-> text;
	$cap =~ s/~//;
	return $canvas-> get_text_width( $cap), $canvas-> font-> height;
}

sub prf_get_borderWidth { $_[0]->prf('borderWidth') // ((( $_[0]-> prf('skin') // $::application->skin // '') eq 'flat') ? 1 : 2) }

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size = $canvas-> size;
	my $cl = $self-> color;
	if ( $self-> prf('flat')) {
		$canvas-> rect_fill( 0,0,$size[0]-1,$size[1]-1,$self->prf_get_borderWidth,
			cl::Gray,$self-> backColor);
	} else {
		$canvas-> rect3d( 0,0,$size[0]-1,$size[1]-1,$self-> prf_get_borderWidth,
			$self-> light3DColor,$self-> dark3DColor,$self-> backColor);
	}
	my $i = $self-> prf('image');
	my $capOk = length($self-> text) > 0;
	my ( $fw, $fh) = $capOk ? $self-> caption_box($canvas) : ( 0, 0);
	my ( $textAtX, $textAtY) = ( 2, $size[1]-3);

	if ( defined $i) {
		my $gy = $self-> prf('glyphs');
		$gy = 1 unless $gy;
		my $pw = $i-> width / $gy;
		my $ph = $i-> height;
		my $sw = $pw * $self-> prf('imageScale');
		my $sh = $ph * $self-> prf('imageScale');
		my ( $imAtX, $imAtY);
		if ( $capOk) {
			if ( $self-> prf('vertical')) {
				$imAtX = ( $size[ 0] - $sw) / 2;
				$imAtY = ( $size[ 1] - $fh - $sh) / 3;
				$textAtY = $imAtY;
				$imAtY   = $size[ 1] - $imAtY - $sh;
			} else {
				$imAtX = ( $size[ 0] - $fw - $sw) / 3;
				$imAtY = ( $size[ 1] - $sh) / 2;
				$textAtX = 2 * $imAtX + $sw;
			}
		} else {
			$imAtX = ( $size[0] - $sw) / 2;
			$imAtY = ( $size[1] - $sh) / 2;
		}
		$canvas-> put_image_indirect( $i, $imAtX, $imAtY, 0, 0,
			$sw, $sh,$pw, $ph);
	}

	$canvas-> color( $cl);
	if ( $capOk) {
		$canvas-> draw_text($self-> text,
			$textAtX,2,$size[0]-3,$textAtY,
			dt::DrawMnemonic|dt::Center|dt::VCenter|dt::UseClip);
	}
	$self-> common_paint($canvas);
}

sub prf_flat        { $_[0]-> repaint; }
sub prf_borderWidth { $_[0]-> repaint; }
sub prf_glyphs      { $_[0]-> repaint; }
sub prf_vertical    { $_[0]-> repaint; }
sub prf_image       { $_[0]-> repaint; }
sub prf_imageScale  { $_[0]-> repaint; }


package Prima::VB::Label;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		align   => ['alignment',],
		valign  => ['valignment',],
		bool    => [qw(autoWidth autoHeight showAccelChar showPartial wordWrap)],
		Handle  => ['focusLink',],
		text    => ['hotKey'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cl = $self-> color;
	$canvas-> color( $self-> backColor);
	$canvas-> bar( 1,1,$sz[0]-2,$sz[1]-2);
	$canvas-> color( cl::Gray);
	$canvas-> rectangle( 0,0,$sz[0]-1,$sz[1]-1);
	$canvas-> color( $cl);
	my ( $a, $va, $sp, $ww, $sac) =
		$self-> prf(qw( alignment valignment showPartial wordWrap showAccelChar));
	my $flags = dt::NewLineBreak | dt::WordBreak | dt::ExpandTabs |
		(($a == ta::Left) ? dt::Left : (( $a == ta::Center) ? dt::Center : dt::Right)) |
		(ta::Top == $va ? dt::Top : (( $va == ta::Middle) ? dt::VCenter : dt::Bottom)) |
		($sp  ? dt::DrawPartial : 0) |
		($ww  ? 0 : dt::NoWordWrap) |
		($sac ? 0 : dt::DrawMnemonic);
	$canvas-> draw_text( $self-> text,0,0,$sz[0]-1,$sz[1]-1,$flags);
	$self-> common_paint($canvas);
}

sub prf_alignment     { $_[0]-> repaint; }
sub prf_valignment    { $_[0]-> repaint; }
sub prf_showPartial   { $_[0]-> repaint; }
sub prf_wordWrap      { $_[0]-> repaint; }
sub prf_showAccelChar { $_[0]-> repaint; }

package Prima::VB::InputLine;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		align   => ['alignment',],
		bool    => [qw(writeOnly readOnly insertMode autoSelect autoHeight autoTab firstChar charOffset textDirection)],
		upoint  => ['selection',],
		uiv     => ['selStart','selEnd','maxLen'],
		uiv_undef=>[ qw(borderWidth)],
		char    => ['passwordChar',],
		string  => ['wordDelimiters',],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		selection
		selEnd
		selStart
		charOffset
		firstChar
	);
}

sub prf_get_borderWidth { $_[0]->prf('borderWidth') // ((( $_[0]-> prf('skin') // $::application->skin // '') eq 'flat') ? 1 : 2) }

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cl = $self-> color;
	my ( $a, $wo, $pc) = $self-> prf(qw( alignment writeOnly passwordChar));
	my $bw = $self->prf_get_borderWidth;
	$canvas-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,$bw,$self-> dark3DColor,$self-> light3DColor,$self-> backColor);
	$canvas-> color( $cl);
	$a = (ta::Left == $a ? dt::Left : (ta::Center == $a ? dt::Center : dt::Right));
	my $c = $self-> text;
	$c =~ s/./$pc/g if $wo;
	$canvas-> draw_text($c,2,2,$sz[0]-3,$sz[1]-3,
		$a|dt::VCenter|dt::UseClip|dt::ExpandTabs|dt::NoWordWrap);
	$self-> common_paint($canvas);
}

sub prf_alignment     { $_[0]-> repaint; }
sub prf_writeOnly     { $_[0]-> repaint; }
sub prf_borderWidth   { $_[0]-> repaint; }
sub prf_passwordChar  { $_[0]-> repaint; }

package Prima::VB::Cluster;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onCheck',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw(auto checked pressed autoHeight autoWidth)],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_checked { $_[0]-> repaint; }

sub paint
{
	my ( $self, $canvas, $i) = @_;
	my @sz = $canvas-> size;
	$canvas-> color( $self-> backColor);
	$canvas-> bar(1,1,$sz[0]-2,$sz[1]-2);
	$canvas-> color( cl::Gray);
	$canvas-> rectangle(0,0,$sz[0]-1,$sz[1]-1);
	$canvas-> put_image( 2, ($sz[1] - $i-> height)/2, $i) if $i;
	$canvas-> color( cl::Black);
	my $w = $i ? $i-> width : 0;
	$canvas-> draw_text($self-> text,2 + $w,2,$sz[0]-1,$sz[1]-1,
		dt::Center|dt::VCenter|dt::UseClip|dt::DrawMnemonic);
	$self-> common_paint($canvas);
}

package Prima::VB::CheckBox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Cluster);

sub on_paint
{
	my ( $self, $canvas) = @_;
	$self-> paint( $canvas, Prima::StdBitmap::image( $self-> prf('checked') ?
		sbmp::CheckBoxChecked : sbmp::CheckBoxUnchecked));
}

package Prima::VB::Radio;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Cluster);

sub on_paint
{
	my ( $self, $canvas) = @_;
	$self-> paint( $canvas, Prima::StdBitmap::icon( $self-> prf('checked') ?
		sbmp::RadioChecked : sbmp::RadioUnchecked));
}

package Prima::VB::GroupBox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onRadioClick',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		uiv    => ['index'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size   = $canvas-> size;
	my @clr    = ( $self-> color, $self-> backColor);
	$canvas-> color( $clr[1]);
	$canvas-> bar( 0, 0, @size);
	my $fh = $canvas-> font-> height;
	$canvas-> color( $self-> light3DColor);
	$canvas-> rectangle( 1, 0, $size[0] - 1, $size[1] - $fh / 2 - 2);
	$canvas-> color( $self-> dark3DColor);
	$canvas-> rectangle( 0, 1, $size[0] - 2, $size[1] - $fh / 2 - 1);
	my $c = $self-> text;
	if ( length( $c) > 0) {
		$canvas-> color( $clr[1]);
		$canvas-> bar  (
			8, $size[1] - $fh - 1,
			16 + $canvas-> get_text_width( $c), $size[1] - 1
		);
		$canvas-> color( $clr[0]);
		$canvas-> text_shape_out( $c, 12, $size[1] - $fh - 1);
	}
	$self-> common_paint($canvas);
}

package Prima::VB::BiScroller;
use strict;

sub prf_get_borderWidth { $_[0]->prf('borderWidth') // ((( $_[0]-> prf('skin') // $::application->skin // '') eq 'flat') ? 1 : 2) }

sub paint_exterior
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cl = $self-> color;
	my ( $hs, $vs, $ahs, $avs) =
		$self-> prf(qw( hScroll vScroll autoHScroll autoVScroll));
	$hs ||= $ahs;
	$vs ||= $avs;
	my $bw = $self->prf_get_borderWidth;
	$canvas-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,$bw,
		$self-> dark3DColor,$self-> light3DColor,$self-> backColor);
	my $sw = 12 * $::application-> uiScaling;
	my $sl = 16 * $::application-> uiScaling;
	$hs = $hs ? $sw : 0;
	$vs = $vs ? $sw : 0;
	my @r = ( $bw, $bw+$hs, $sz[0]-$bw-1-$vs,$sz[1]-$bw-1);
	if ( $hs) {
		$self-> color( $ahs ? cl::Gray : cl::Black);
		$canvas-> rectangle( $r[0], $bw, $r[2], $r[1]);
		if ($r[0]+4+$sl < $r[2]-2-$sl) {
			$canvas-> rectangle( $r[0]+2, $bw+2, $r[0]+2+$sl, $r[1]-2);
			$canvas-> rectangle( $r[2]-2-$sl, $bw+2, $r[2]-2, $r[1]-2);
		}
	}
	if ( $vs) {
		$self-> color( $avs ? cl::Gray : cl::Black);
		$canvas-> rectangle( $sz[0]-$bw-1-$sw,$hs+2,$sz[0]-$bw-1,$r[3]-0);
		if ( $r[3]-$sl > $hs+2+$sl) {
			$canvas-> rectangle( $sz[0]-$bw+1-$sw,$hs+4,$sz[0]-$bw-3,$hs+2+$sl);
			$canvas-> rectangle( $sz[0]-$bw+1-$sw,$r[3]-2-$sl,$sz[0]-$bw-3,$r[3]-2);
		}
	}
	$canvas-> color( $cl);
	return if ( $r[0] > $r[2]) || ( $r[1] >= $r[3]);
	return @r;
}

sub prf_autoHScroll   { $_[0]-> repaint; }
sub prf_autoVScroll   { $_[0]-> repaint; }
sub prf_borderWidth   { $_[0]-> repaint; }
sub prf_hScroll       { $_[0]-> repaint; }
sub prf_vScroll       { $_[0]-> repaint; }

package Prima::VB::ListBox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onClick',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onSelectItem  => 'my ( $self, $index, $selectState) = @_;',
	);
}


sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw(autoWidth vScroll hScroll multiSelect extendedSelect
				autoHeight integralWidth integralHeight multiColumn
				autoHScroll autoVScroll drawGrid vertical)],
		uiv     => [qw(itemHeight itemWidth focusedItem offset topItem)],
		uiv_undef=>[ qw(borderWidth)],
		color   => [qw(gridColor)],
		items   => [qw(items selectedItems)],
		string  => [qw(scrollBarClass)],
		align   => [qw(align)],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (offset);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior( $canvas);
	my $align = $self->prf('align');
	if ( $align == ta::Right ) {
		$align = dt::Right;
	} elsif ( $align == ta::Center ) {
		$align = dt::Center;
	} else {
		$align = dt::Left;
	}
	$canvas-> draw_text( join("\n", @{$self-> prf('items')}), @r,
		dt::NoWordWrap |
		dt::NewLineBreak | dt::Top | dt::UseExternalLeading |
		dt::UseClip | $align
	)  if scalar @r;
	$self-> common_paint($canvas);
}

sub prf_items         { $_[0]-> repaint; }
sub prf_align         { $_[0]-> repaint; }

package Prima::VB::ListViewer;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::ListBox);

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onDrawItem    => 'my ( $self, $canvas, $itemIndex, $x, $y, $x2, $y2, $selected, $focused, $prelight, $column) = @_;',
		onStringify   => 'my ( $self, $index, $result) = @_;',
		onMeasureItem => 'my ( $self, $index, $result) = @_;',
	);
}


package Prima::VB::DirectoryListBox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::ListBox);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		uiv     => ['indent'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (items itemWidth itemHeight);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior( $canvas);
	$canvas-> draw_text( "usr\n\tlocal\n\t\tshare\n\t\t\texamples\n\t\t\t\tetc", @r,
		dt::NoWordWrap | dt::ExpandTabs |
		dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
		dt::UseClip
	)  if scalar @r;
	$self-> common_paint($canvas);
}


package Prima::VB::ScrollBar;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw(autoTrack vertical)],
		uiv     => [qw(minThumbSize pageStep partial step whole)],
		iv      => [qw(min max value)],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cl = $self-> color;
	my ( $v) = $self-> prf(qw( vertical));
	$canvas-> color( $self-> backColor);
	$canvas-> bar(0,0,@sz);
	$canvas-> color( $cl);
	$canvas-> rectangle(0,0,$sz[0]-1,$sz[1]-1);
	my $ws = 18;
	my $ex = 5;
	if ( $sz[ $v ? 1 : 0] > $ws * 2) {
		if ( $v) {
			$canvas-> rectangle(2,2,$sz[0]-3,2+$ws);
			$canvas-> ellipse($sz[0]/2-2,$ws/2+2,$ex,$ex);
			$canvas-> rectangle(2,$sz[1]-3-$ws,$sz[0]-3,$sz[1]-3);
			$canvas-> ellipse($sz[0]/2-2,$sz[1]-$ws/2-3,$ex,$ex);
		} else {
			$canvas-> rectangle(2,2,2+$ws,$sz[1]-3);
			$canvas-> ellipse($ws/2+2,$sz[1]/2-2,$ex,$ex);
			$canvas-> rectangle($sz[0]-3-$ws,2,$sz[0]-3,$sz[1]-3);
			$canvas-> ellipse($sz[0]-$ws/2-3,$sz[1]/2-2,$ex,$ex);
		}
	}
	$self-> common_paint($canvas);
}

sub prf_vertical   { $_[0]-> repaint; }

package Prima::VB::ComboBox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool       => [qw(caseSensitive literal autoSelect autoHeight)],
		uiv        => [qw(editHeight listHeight focusedItem)],
		comboStyle => ['style'],
		string     => ['text'],
		items      => ['items'],
	);
	$_[0]-> prf_types_delete( $pt, qw(text));
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		writeOnly        autoWidth       selectedItems
		selection        vScroll         borderWidth
		readOnly         hScroll         alignment
		insertMode       multiSelect     maxLen
		autoTab          extendedSelect  buttonClass
		selection        integralHeight  buttonProfile
		selStart         multiColumn     editClass
		selEnd           itemHeight      editProfile
		firstChar        itemWidth       listProfile
		charOffset       offset          listVisible
		passwordChar     topItem         listClass
		wordDelimiters   gridColor       autoHScroll
		autoVScroll      scrollBarClass
	);
}



sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cl = $self-> color;
	my @c3d = ( $self-> dark3DColor, $self-> light3DColor);
	my $s = $self-> prf('style') == cs::Simple;
	my $szy = $s ? ( $sz[1]-$canvas-> font-> height - 5) : 0;
	$canvas-> rect3d( 0,$szy,$sz[0]-1,$sz[1]-1,1,$c3d[0],$c3d[1],$self-> backColor);
	unless ( $s) {
		$canvas-> rect3d( $sz[0]-18,0,$sz[0]-1,$sz[1]-1,2,$c3d[1],$c3d[0],$self-> backColor);
		$canvas-> color( cl::Black);
		$canvas-> fillpoly([
			$sz[0]-12,$sz[1] * 0.6,
			$sz[0]-8,$sz[1] * 0.6,
			$sz[0]-10,$sz[1] * 0.3,
		]);
	}

	$canvas-> color( $cl);
	$canvas-> draw_text( $self-> text, 2, $szy + 2, $sz[0] - 3 - ( $s ? 0 : 17), $sz[1] - 3,
		dt::Left|dt::VCenter|dt::NoWordWrap|dt::UseClip|dt::ExpandTabs);
	if ( $s) {
		$canvas-> rect3d( 0,0,$sz[0]-1,$szy-1,2,$c3d[0],$c3d[1],$self-> backColor);
		my $i = $self-> prf('items');
		$canvas-> draw_text( join("\n", @$i),2,2,$sz[0]-3,$szy-3,
			dt::Left|dt::Top|dt::NoWordWrap|dt::UseClip|dt::NewLineBreak|dt::UseExternalLeading
		);
	}
	$self-> common_paint($canvas);
}

sub prf_style         { $_[0]-> repaint; }
sub prf_items         { $_[0]-> repaint; }
sub prf_borderWidth   { $_[0]-> repaint; }

package Prima::VB::DriveComboBox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::ComboBox);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		char       => ['drive'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		items
	);
}

package Prima::VB::ColorComboBox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::ComboBox);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		color      => ['value'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		items
	);
}


package Prima::VB::Edit;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw(autoIndent cursorWrap insertMode hScroll vScroll
				persistentBlock readOnly syntaxHilite wantTabs wantReturns wordWrap
				autoHScroll autoVScroll
			)],
		uiv     => [qw(tabIndent undoLimit)],
		uiv_undef=>[ qw(borderWidth)],
		editBlockType => ['blockType',],
		color   => [qw(hiliteNumbers hiliteQStrings hiliteQQStrings)],
		string  => ['wordDelimiters','scrollBarClass'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}


sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		accelItems     selection
		cursorX        markers
		cursorY        textRef
		modified       topLine
		offset
		selection
		selEnd
		selStart
	);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior( $canvas);
	$canvas-> draw_text( $self-> text, @r,
		( $self-> prf('wordWrap') ? dt::WordWrap : dt::NoWordWrap) |
		dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
		dt::UseClip | dt::ExpandTabs
	) if scalar @r;
	$self-> common_paint($canvas);
}

sub prf_wordWrap      { $_[0]-> repaint; }

package Prima::VB::ImageViewer;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw(hScroll vScroll quality autoHScroll autoVScroll stretch)],
		uiv     => [qw(zoom)],
		uiv_undef=>[ qw(borderWidth)],
		image   => ['image'],
		align   => ['alignment',],
		valign  => ['valignment',],
		string  => ['scrollBarClass',],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		deltaX imageFile
		deltaY
		limitX
		limitY
	);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior( $canvas);
	my $i = $self-> prf('image');
	if ( $i) {
		$canvas-> clipRect( $r[0], $r[1], $r[2] - 1, $r[3] - 1);
		my ( $a, $v, $z) = $self-> prf(qw(alignment valignment zoom));
		my ( $ix, $iy) = $i-> size;
		my ( $izx, $izy) = ( $ix * $z, $iy * $z);
		my ( $ax, $ay);
		$a = ta::Left if $izx >= $r[2] - $r[0];
		$v = ta::Top  if $izy >= $r[3] - $r[1];
		if ( $a == ta::Left) {
			$ax = $r[0];
		} elsif ( $a == ta::Center) {
			$ax = $r[0] + ( $r[2] - $r[0] - $izx) / 2;
		} else {
			$ax = $r[2] - $izx;
		}
		if ( $v == ta::Bottom) {
			$ay = $r[1];
		} elsif ( $v == ta::Middle) {
			$ay = $r[1] + ( $r[3] - $r[1] - $izy) / 2;
		} else {
			$ay = $r[3] - $izy;
		}
		$canvas-> stretch_image( $ax, $ay, $izx, $izy, $i);
	}
	$self-> common_paint($canvas);
}

sub prf_image         { $_[0]-> repaint; }
sub prf_alignment     { $_[0]-> repaint; }
sub prf_valignment    { $_[0]-> repaint; }
sub prf_zoom          { $_[0]-> repaint; }

package Prima::VB::ScrollWidget;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw(autoHScroll autoVScroll hScroll vScroll)],
		uiv     => [qw(deltaX deltaY limitX limitY)],
		uiv_undef=>[ qw(borderWidth)],
		string  => ['scrollBarClass',],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	$self-> paint_exterior( $canvas);
	$self-> common_paint( $canvas);
}

package Prima::VB::SpinButton;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onIncrement => 'my ( $self, $increment) = @_;',
	);
}


sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @clr  = ( $self-> color, $self-> backColor);
	my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
	my @size = $canvas-> size;
	$canvas-> rect3d( 0, 0, $size[0] - 1, $size[1] * 0.4 - 1, 2, @c3d, $clr[1]);
	$canvas-> rect3d( 0, $size[1] * 0.4, $size[0] - 1, $size[1] * 0.6 - 1, 2, @c3d, $clr[1]);
	$canvas-> rect3d( 0, $size[1] * 0.6, $size[0] - 1, $size[1] - 1, 2, @c3d, $clr[1]);
	$canvas-> color( $clr[0]);
	$canvas-> fillpoly([
		$size[0] * 0.3, $size[1] * 0.73,
		$size[0] * 0.5, $size[1] * 0.87,
		$size[0] * 0.7, $size[1] * 0.73
	]);
	$canvas-> fillpoly([
		$size[0] * 0.3, $size[1] * 0.27,
		$size[0] * 0.5, $size[1] * 0.13,
		$size[0] * 0.7, $size[1] * 0.27
	]);
	$self-> common_paint( $canvas);
}

package Prima::VB::AltSpinButton;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onIncrement => 'my ( $self, $increment) = @_;',
	);
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}


sub on_paint
{
	my ( $self, $canvas) = @_;
	my @clr  = ( $self-> color, $self-> backColor);
	my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
	my @size = $canvas-> size;
	$canvas-> color( $clr[ 1]);
	$canvas-> bar( 0, 0, $size[0]-1, $size[1]-1);

	$canvas-> color( $c3d[ 0]);
	$canvas-> line( 0, 0, 0, $size[1] - 1);
	$canvas-> line( 1, 1, 1, $size[1] - 2);
	$canvas-> line( 2, $size[1] - 2, $size[0] - 3, $size[1] - 2);
	$canvas-> line( 1, $size[1] - 1, $size[0] - 2, $size[1] - 1);

	$canvas-> color( $c3d[ 1]);
	$canvas-> line( 1, 0, $size[0] - 1, 0);
	$canvas-> line( 2, 1, $size[0] - 1, 1);
	$canvas-> line( $size[0] - 2, 1, $size[0] - 2, $size[1] - 2);
	$canvas-> line( $size[0] - 1, 1, $size[0] - 1, $size[1] - 1);

	$canvas-> color( $c3d[ 1]);
	$canvas-> line( -1, 0, $size[0] - 2, $size[1] - 1);
	$canvas-> line( 0, 0, $size[0] - 1, $size[1] - 1);
	$canvas-> color( $c3d[ 0]);
	$canvas-> line( 1, 0, $size[0], $size[1] - 1);

	$canvas-> color( $clr[0]);
	$canvas-> fillpoly([
		$size[0] * 0.2, $size[1] * 0.65,
		$size[0] * 0.3, $size[1] * 0.77,
		$size[0] * 0.4, $size[1] * 0.65
	]);
	$canvas-> fillpoly([
		$size[0] * 0.6, $size[1] * 0.35,
		$size[0] * 0.7, $size[1] * 0.27,
		$size[0] * 0.8, $size[1] * 0.35
	]);
	$self-> common_paint( $canvas);
}

package Prima::VB::SpinEdit;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::InputLine);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		align   => ['alignment',],
		bool    => ['writeOnly','readOnly','insertMode','autoSelect',
			'autoTab','firstChar','charOffset'],
		upoint  => ['selection',],
		uiv     => [qw(min max step value)],
		string  => [qw(spinClass editClass)],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		selection        passwordChar
		selEnd           writeOnly
		selStart
		charOffset
		firstChar
	);
}


sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cl = $self-> color;
	my ( $a) = $self-> prf(qw( alignment ));
	my $bw = $self->prf_get_borderWidth;
	$canvas-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,$bw,
		$self-> dark3DColor,$self-> light3DColor,$self-> backColor);
	$canvas-> rect3d( $sz[0]-$sz[1]-$bw,$bw,$sz[0]-1,$sz[1]-1,2,
		$self-> light3DColor,$self-> dark3DColor);
	$canvas-> color( $cl);
	$a = (ta::Left == $a ? dt::Left : (ta::Center == $a ? dt::Center : dt::Right));
	my $c = $self-> prf('value');
	$canvas-> draw_text($c,2,2,$sz[0]-$sz[1]-$bw*2,$sz[1]-3,
		$a|dt::VCenter|dt::UseClip|dt::ExpandTabs|dt::NoWordWrap);
	$self-> common_paint($canvas);
}

sub prf_alignment     { $_[0]-> repaint; }
sub prf_borderWidth   { $_[0]-> repaint; }

package Prima::VB::Gauge;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onStringify => 'my ( $self, $index, $result) = @_;',
	);
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool         => [qw(vertical indeterminate)],
		uiv          => [qw(min max value threshold indent)],
		gaugeRelief  => ['relief'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my ($x, $y) = $canvas-> size;
	my ($i, $relief, $v, $val, $min, $max, $l3, $d3) =
		$self-> prf(qw(indent relief vertical value min max light3DColor dark3DColor));
	my ($clComplete,$clHilite,$clBack,$clFore) =
		($self-> prf('hiliteBackColor', 'hiliteColor'), $self-> backColor, $self-> color);
	my $complete = $v ? $y : $x;
	my $ediv = $max - $min;
	$ediv = 1 unless $ediv;
	$complete = int(($complete - $i*2) * $val / $ediv + 0.5);
	$canvas-> color( $clComplete);
	$canvas-> bar ( $v ?
		($i, $i, $x-$i-1, $i+$complete) :
		( $i, $i, $i + $complete, $y-$i-1)
	);
	$canvas-> color( $clBack);
	$canvas-> bar ( $v ?
		($i, $i+$complete+1, $x-$i-1, $y-$i-1) :
		( $i+$complete+1, $i, $x-$i-1, $y-$i-1)
	);
	# draw the border
	$canvas-> color(( $relief == gr::Sink) ? $d3 : (( $relief == gr::Border) ? cl::Black : $l3));
	for ( my $j = 0; $j < $i; $j++)
	{
		$canvas-> line( $j, $j, $j, $y - $j - 1);
		$canvas-> line( $j, $y - $j - 1, $x - $j - 1, $y - $j - 1);
	}
	$canvas-> color(( $relief == gr::Sink) ? $l3 : (( $relief == gr::Border) ? cl::Black : $d3));
	for ( my $j = 0; $j < $i; $j++)
	{
		$canvas-> line( $j + 1, $j, $x - $j - 1, $j);
		$canvas-> line( $x - $j - 1, $j, $x - $j - 1, $y - $j - 1);
	}

	# draw the text, if neccessary
	my $s = sprintf( "%2d%%", $val * 100.0 / $ediv);
	my ($fw, $fh) = ( $canvas-> get_text_width( $s), $canvas-> font-> height);
	my $xBeg = int(( $x - $fw) / 2 + 0.5);
	my $xEnd = $xBeg + $fw;
	my $yBeg = int(( $y - $fh) / 2 + 0.5);
	my $yEnd = $yBeg + $fh;
	my ( $zBeg, $zEnd) = $v ? ( $yBeg, $yEnd) : ( $xBeg, $xEnd);
	if ( $zBeg > $i + $complete)
	{
		$canvas-> color( $clFore);
		$canvas-> text_shape_out( $s, $xBeg, $yBeg);
	}
	elsif ( $zEnd < $i + $complete + 1)
	{
		$canvas-> color( $clHilite);
		$canvas-> text_shape_out( $s, $xBeg, $yBeg);
	}
	else
	{
		$canvas-> clipRect( $v ?
			( 0, 0, $x, $i + $complete) :
			( 0, 0, $i + $complete, $y));
		$canvas-> color( $clHilite);
		$canvas-> text_shape_out( $s, $xBeg, $yBeg);
		$canvas-> clipRect( $v ?
			( 0, $i + $complete + 1, $x, $y) :
			( $i + $complete + 1, 0, $x, $y));
		$canvas-> color( $clFore);
		$canvas-> text_shape_out( $s, $xBeg, $yBeg);
	}
	$self-> common_paint( $canvas);
}

sub prf_min           { $_[0]-> repaint; }
sub prf_max           { $_[0]-> repaint; }
sub prf_value         { $_[0]-> repaint; }
sub prf_indent        { $_[0]-> repaint; }
sub prf_relief        { $_[0]-> repaint; }
sub prf_vertical      { $_[0]-> repaint; }

package Prima::VB::ProgressBar;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		uiv          => [qw(min max value )],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my ($x, $y) = $canvas-> size;
	my ($val, $min, $max) =
		$self-> prf(qw(value min max));
	my $complete = $x;
	my $ediv = $max - $min;
	$ediv = 1 unless $ediv;
	$complete = int($complete * $val / $ediv + 0.5);
	$canvas-> color( $self-> prf('color') );
	$canvas-> bar ( 0, 0, $complete, $y-1);
	$canvas-> color( cl::Gray );
	$canvas-> bar ( $complete+1, 0, $x-1, $y-1);
	$self-> common_paint( $canvas);
}

sub prf_min           { $_[0]-> repaint; }
sub prf_max           { $_[0]-> repaint; }
sub prf_value         { $_[0]-> repaint; }

package Prima::VB::AbstractSlider;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool         => [qw(readOnly snap autoTrack)],
		uiv          => [qw(min max value increment step)],
		sliderScheme => ['scheme'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw ( ticks);
}

package Prima::VB::Slider;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractSlider);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool         => [qw(ribbonStrip vertical)],
		uiv          => [qw(shaftBreadth)],
		tickAlign    => [qw(tickAlign)],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my ($x, $y) = $canvas-> size;
	my ( $f, $b) = ( $self-> color, $self-> backColor);
	my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
	$canvas-> color( $b);
	$canvas-> bar(0,0,$x,$y);
	$canvas-> color( $f);
	if ( $self-> prf('vertical')) {
		$canvas-> rect3d( $x * 0.4, 0, $x * 0.6, $y - 1, 1, @c3d);
		$canvas-> rect3d( $x * 0.25, 3, $x * 0.75, 20, 1, @c3d, $b);
	} else {
		$canvas-> rect3d( 0, $y * 0.4, $x - 1, $y * 0.6, 1, @c3d);
		$canvas-> rect3d( 3, $y * 0.25, 20, $y * 0.75, 1, @c3d, $b);
	}
	$self-> common_paint( $canvas);
}

sub prf_vertical  { $_[0]-> repaint; }

package Prima::VB::CircularSlider;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractSlider);

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onStringify => 'my ( $self, $index, $result) = @_;',
	);
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool         => [qw(buttons stdPointer)],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my ($x, $y) = $canvas-> size;
	my ( $f, $b) = ( $self-> color, $self-> backColor);
	my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
	$canvas-> color( $b);
	$canvas-> bar(0,0,$x,$y);
	$canvas-> color( $f);
	my ( $cx, $cy) = ( $x/2, $y/2);
	my $rad = ($x < $y) ? $cx : $cy;
	$rad -= 3;
	$canvas-> lineWidth(2);
	$canvas-> color( $c3d[0]);
	$canvas-> arc( $cx, $cy, $rad, $rad, 65, 235);
	$canvas-> color( $c3d[1]);
	$canvas-> arc( $cx, $cy, $rad, $rad, 255, 405);
	$canvas-> lineWidth(1);
	$self-> common_paint( $canvas);
}


package Prima::VB::AbstractOutline;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onClick',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onSelectItem  => 'my ( $self, $index) = @_;',
		onDrawItem    => 'my ( $self, $canvas, $node, $left, $bottom, $right, $top, $position, $selected, $focused, $prelight) = @_;',
		onMeasureItem => 'my ( $self, $node, $result) = @_;',
		onExpand      => 'my ( $self, $node, $action) = @_;',
		onDragItem    => 'my ( $self, $from, $to) = @_;',
		onStringify   => 'my ( $self, $node, $result) = @_;',
	);
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [ qw(autoHScroll autoVScroll vScroll hScroll draggable autoHeight showItemHint)],
		uiv     => [ qw(itemHeight itemWidth focusedItem offset topItem indent
				openedGlyphs closedGlyphs)],
		uiv_undef=>[ qw(borderWidth)],
		treeItems => [qw(items)],
		icon      => [qw(closedIcon openedIcon)],
		string    => ['scrollBarClass',],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}


sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (offset);
}

package Prima::VB::OutlineViewer;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractOutline);

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (items);
}


package Prima::VB::StringOutline;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractOutline);

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior( $canvas);
	my $c = '';
	my $i = $self-> prf('items');
	my $traverse;
	if ( $i && scalar @r) {
		my $fh = $canvas-> font-> height;
		my $max = int(($r[3] - $r[1]) / $fh) + 1;
		$traverse = sub{
			my ( $x, $l) = @_;
			goto ENOUGH unless $max--;
			$c .= "\t\t" x $l;
			$c .= $x-> [2] ? '[-] ' : '[+] ' if $x-> [1];
			$c .= $x-> [0] . "\n";
			$l++;
			if ( $x-> [1] && $x-> [2]) {
				$traverse-> ($_, $l) for @{$x-> [1]};
			}
		};
		$traverse-> ($_,0) for @$i;
ENOUGH:
		$canvas-> draw_text( $c, @r,
			dt::NoWordWrap | dt::ExpandTabs |
			dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
			dt::UseClip
		);
	}
	$self-> common_paint($canvas);
}

sub prf_items  { $_[0]-> repaint; }

package Prima::VB::DetailedOutline;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::StringOutline);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		items  => ['headers', 'widths'],
		uiv    => ['mainColumn', 'columns', 'minTabWidth', 'offset'],
		treeArrays => ['items'],
		string => ['headerClass'],
		bool   => ['clickable', 'scalable', 'multiColumn', 'autoWidth'],
	);
	$_[0]-> prf_types_delete( $pt, qw(items));
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
}

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onSort  => 'my ( $self, $column, $direction) = @_;',
	);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior( $canvas);
	my $i = $self-> prf('items');
	my @w = @{$self-> prf('widths')};
	my @h = @{$self-> prf('headers')};
	my @c = @h;
	my $cl = $self-> prf('columns');
	my $f = $canvas-> font-> height;

	my $traverse;
	return $self->common_paint($canvas) unless scalar(@r);

	$canvas-> color( cl::Gray);
	$canvas-> bar( 1, $r[3] - $f, $r[2], $r[3] - 1);
	$canvas-> color( cl::Fore);

	if ( $i && scalar @r) {
		my $max = int(($r[3] - $r[1]) / $f);
		$traverse = sub{
			my ( $x, $l) = @_;
			goto ENOUGH unless $max--;
			$c[0] .= "\n";
			$c[0] .= "\t\t" x $l;
			$c[0] .= $x-> [2] ? '[-] ' : '[+] ' if $x-> [1];
			$c[0] .= $x-> [0]->[0];
			for ( my $j = 1; $j < @{$x->[0]}; $j++) {
				$c[$j] .= "\n" . $x->[0]->[$j] // '';
			}
			$l++;
			if ( $x-> [1] && $x-> [2]) {
				$traverse-> ($_, $l) for @{$x-> [1]};
			}
		};
		$traverse-> ($_,0) for @$i;
ENOUGH:
		my $z = $r[0];
		for (my $j = 0; $j < @c; $j++) {
			my $ww = $w[$j];
			$ww = $canvas-> get_text_width( $h[$j] // ' ')
				if !(defined($ww)) || !($ww =~ m/^\s*\d+\s*$/);
			my @z = ( $z, $r[1], ( $z + $ww > $r[2]) ? $r[2] : $z + $ww, $r[3]);
			$z[2]++; $canvas-> rectangle( @z); $z[2]--;
			$z += $ww + 1;
			$canvas-> draw_text( $c[$j], @z,
				dt::NoWordWrap | dt::ExpandTabs |
				dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
				dt::UseClip
			);
		}
	}
	$self-> common_paint($canvas);
}

sub prf_items
{
	my ( $self, $data) = @_;
	my $c = $self-> prf('columns');
	my $traverse;
	$traverse = sub{
		my ($x) = @_;
		my $x0 = $x->[0];
		push @$x0, ('') x ($c - @$x0) if @$x0 < $c;
		if ( $x-> [1] && $x-> [2]) {
			$traverse->($_) for @{$x-> [1]};
		}
	};
	$traverse-> ($_) for @$data;
	$self-> repaint;
}

sub prf_columns { $_[0]-> prf_items( $_[0]-> prf('items')); }
sub prf_widths  { $_[0]-> repaint; }
sub prf_headers { $_[0]-> prf_set( columns => scalar @{$_[0]-> prf('headers')}); }

package Prima::VB::DirectoryOutline;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractOutline);

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw ( items draggable );
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior( $canvas);
	$canvas-> draw_text( "usr\n\tlocal\n\t\tshare\n\t\t\texamples\n\t\t\t\tetc", @r,
		dt::NoWordWrap | dt::ExpandTabs |
		dt::NewLineBreak | dt::Left | dt::Top | dt::UseExternalLeading |
		dt::UseClip
	)  if scalar @r;
	$self-> common_paint($canvas);
}

package Prima::VB::Notebook;
use strict;
use vars qw(@ISA);
@ISA = qw( Prima::VB::CommonControl);


sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		uiv => ['pageCount', 'pageIndex'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {defaultInsertPage};
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> {list} = {};
	$self-> {pageCount} = 0;
	$self-> {pageIndex} = 0;
	$self-> insert( Popup =>
		name => 'AltPopup',
		items => [
			['~Next page' => '+' => '+' => sub { $self-> pg_inc(1); }],
			['~Previous page' => '-' => '-' => sub { $self-> pg_inc(-1); }],
			['~Move widget to page...' => 'Ctrl+M' => '^M' => sub { $self-> widget_repage } ],
		],
	)-> selected(0);
	$self-> add_hooks(qw(name owner DESTROY));
	return %profile;
}

sub prf_pageIndex
{
	my ( $self, $pi) = @_;
	return if $pi == $self-> {pageIndex};
	my $l = $self-> {list};
	for ( $VB::form-> widgets) {
		my $n = $_-> name;
		next unless exists $l-> {$n};
		$_-> visible( $pi == $l-> {$n});
	}
	$self-> {pageIndex} = $pi;
}

sub ext_profile
{
	my $self = $_[0];
	my $l    = $self-> {list};
	return map { $_ => $l-> {$_}} keys %{$l};
}

sub act_profile
{
	my $self = $_[0];
	return (
		onChild => '$_[2]-> defaultInsertPage( $_[1]-> {extras}-> {$_[3]})',
	);
}

sub repage
{
	$_[0]-> {pageIndex} = -1; # force repage
	$_[0]-> prf_pageIndex($_[0]-> prf('pageIndex'));
}

sub on_load
{
	my $self = $_[0];
	return unless $self-> {extras};
	$self-> {list} = $self-> {extras};
	delete $self-> {extras};
	$self-> repage;
}

sub on_show
{
	my $self = $_[0];
	$self-> insert( Timer => timeout => 1, onTick => sub {
		$self-> repage;
		$_[0]-> destroy;
	})-> start;
}

sub on_hook
{
	my ( $self, $who, $prop, $old, $new, $widget) = @_;
	if ( $prop eq 'name') {
		return unless exists $self-> {list}-> {$old};
		$self-> {list}-> {$new} = $self-> {list}-> {$old};
		delete $self-> {list}-> {$old};
		return;
	}
	if ( $prop eq 'owner') {
		my $n = $self-> prf('name');
		my $l = $self-> {list};
		if (( $n eq $old) || exists $l-> {$old}) {
			return if exists $l-> {$new} || ( $n eq $new);
			delete $l-> {$who};
		} elsif (( $n eq $new) && exists $l-> {$who}) {
			return; # notebook itself was renamed
		} elsif (( $n eq $new) || exists $l-> {$new}) {
			return if exists $l-> {$old} || ( $n eq $old);
			$l-> {$who} = $self-> {pageIndex};
		}
		return;
	}
	if ( $prop eq 'DESTROY') {
		delete $self-> {list}-> {$who};
		return;
	}
}

sub widget_repage
{
	no warnings 'once';
	my $self = $_[0];
	my @mw = $VB::form-> marked_widgets;
	my $d = Prima::Dialog-> create(
		text => 'Move to page',
		size => [ 217, 63],
		centered => 1,
		icon => $VB::ico,
		visible => 0,
		designScale => [7, 16],
	);
	$d-> insert( ['Prima::SpinEdit' =>
		origin => [ 3, 8],
		name  => 'Spin',
		size  => [ 100, 20],
		value => $self-> {pageIndex},
		max   => 16383,
	], [ 'Prima::Button' =>
		origin => [ 109, 8],
		size => [ 96, 36],
		text => '~OK',
		onClick => sub { $d-> ok; },
	], [ 'Prima::Label' =>
		origin => [ 3, 36],
		size => [ 100, 20],
		text => 'Move to page',
	]);
	my $ok = $d-> execute == mb::OK;
	my $pi = $d-> Spin-> value;
	$d-> destroy;
	return unless $ok;
	return if $self-> {pageIndex} == $pi;
	my $ctrl = 0;
	for ( @mw) {
		my $name = $_-> name;
		next unless exists $self-> {list}-> {$name};
		$self-> {list}-> {$name} = $pi;
		$ctrl++;
	}
	return unless $ctrl;
	$self-> prf_set( pageIndex => $pi);
}

sub pg_inc
{
	my ( $self, $inc) = @_;
	my $np = $self-> {pageIndex} + $inc;
	return if $np < 0 || $np > 16383;
	$self-> prf_set( pageIndex => $np);
}

package Prima::VB::TabSet;
use strict;
use vars qw(@ISA);
@ISA = qw( Prima::VB::CommonControl);

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onDrawTab    => 'my ( $self, $canvas, $number, $colorSet, $largePolygon, $smallPolygon) = @_;',
		onMeasureTab => 'my ( $self, $index, $result) = @_;',
	);
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		uiv   => [qw(firstTab focusedTab tabIndex)],
		bool  => [qw(colored topMost)],
		items => ['tabs'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_tabs    { $_[0]-> repaint; }
sub prf_topMost { $_[0]-> repaint; }

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $self-> size;
	my $c = $self-> color;
	$canvas-> color( $self-> backColor);
	$canvas-> bar( 0, 0, @sz);
	my $y = ( $sz[1] - $canvas-> font-> height) / 2;
	$canvas-> color( $c);
	my $x = 0;
	my @tabs = @{$self-> prf('tabs')};
	my $topMost = $self-> prf( 'topMost');
	for ( @tabs) {
		$canvas-> text_shape_out( $_, $x + 5, $y);
		my $tx = $canvas-> get_text_width( $_);
		$canvas-> polyline( $topMost ? [
			$x, 2,
			$x + 5, $sz[1] - 2,
			$x + $tx + 5, $sz[1] - 2,
			$x + $tx + 10, 2
		] : [
			$x, $sz[1] - 2,
			$x + 5, 2,
			$x + $tx + 5, 2,
			$x + $tx + 10, $sz[1] - 2
		]);
		$x += $tx + 20;
		last if $x >= $sz[0];
	}
	if ( scalar @tabs) {
		my $tx = $canvas-> get_text_width( $tabs[0]);
		$topMost ?
			$canvas-> line( $tx + 10, 2, $sz[0] - 1, 2) :
			$canvas-> line( $tx + 10, $sz[1] - 2, $sz[0] - 1, $sz[1] - 2);
	}
	$self-> common_paint($canvas);
}

package Prima::VB::TabbedNotebook;
use vars qw(@ISA);
@ISA = qw( Prima::VB::Notebook);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		items => ['tabs'],
		uiv   => ['tabIndex'],
		bool  => ['style', 'orientation'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		tabCount
		pageCount
	);
}

use constant OFFSET => 11;

sub o_delta_aperture { OFFSET, OFFSET }

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cl = $self-> color;
	my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
	$canvas-> color( $self-> backColor);
	$canvas-> bar( 0, 0, @sz);
	$canvas-> color( $cl);
	my $fh = $canvas-> font-> height;
	my $mh = $fh * 2 + 4;
	my @tabs = @{$self-> prf('tabs')};
	my $earx = 16;

	my ( $page, $last, $x, $maxx, $ix) =
		(0,'', $earx * 3, $sz[0] - $earx * 3 - 1, $self-> prf('pageIndex'));
	my $y = $self-> prf('orientation') ? 0 : $sz[1] - $mh;
	for ( @tabs) {
		next unless $page++ >= $ix;
		next if $_ eq $last;
		$last = $_;

		my $w = $canvas-> get_text_width( $last);
		$canvas-> text_shape_out( $last, $x + $earx + 2, $y + $fh/2 + 2);
		$canvas-> rectangle( $x+1, $y + $fh/2+1, $x + $earx * 2 + $w + 2, $y + $fh*3/2+2);
		$canvas-> rectangle( $x, $y + $fh/2, $x + $earx * 2 + $w + 3, $y + $fh*3/2+3)
			if $page == $ix+1;
		$x += $w + $earx * 2 + 4;
		last if $x > $maxx;
	}
	$canvas-> rect3d(
		$earx/2, $y + $fh/2,
		$earx * 2.5, $y + $fh * 3/2+4,
		2, @c3d, $canvas-> backColor);
	$canvas-> rect3d(
		$maxx + $earx/2, $y + $fh/2,
		$maxx + $earx * 2.5, $y + $fh * 3/2+4,
		2, @c3d, $canvas-> backColor);
	$canvas-> fillpoly([
		$earx, $y + $fh,
		$earx*2, $y + $fh*0.5+3,
		$earx*2, $y + $fh*1.5-1
	]);
	$canvas-> fillpoly([
		$maxx + $earx*2, $y + $fh,
		$maxx + $earx, $y + $fh*0.5+3,
		$maxx + $earx, $y + $fh*1.5-1
	]);

	my @tr = $canvas-> translate;
	$canvas-> translate( $self-> prf('orientation') ? (0, $mh) : (0,0));
	if ( $self-> prf('style')) {
		$canvas-> rect3d( 10, 10, $sz[0] - 11, $sz[1] - 10 - $mh, 1, reverse @c3d);
		$canvas-> rect3d( 2, 2, $sz[0] - 3, $sz[1] - $mh, 1, @c3d);
	}
	$canvas-> linePattern( lp::Dash);
	$canvas-> rectangle( OFFSET+1, OFFSET+1, $sz[0] - 17, $sz[1] - ($self-> prf('style') ? 48 : -4) - $mh);
	$canvas-> linePattern( lp::Solid);
	$canvas-> translate(@tr);
	$self-> common_paint( $canvas);
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	if ( $btn == mb::Left) {
		my @sz = $self-> size;
		my $fh = $self-> font-> height;
		my $ny = $self-> prf('orientation') ? 0 : $sz[1] - 2 * $fh - 4;
		my $earx = 16;
		my $maxx = $sz[0] - $earx;
		if ( $y > $ny + $fh/2 && $y < $ny + $fh*3/2+4) {
			if ( $x > $earx/2 && $x < $earx * 2.5) {
				$self-> prf_set( 'pageIndex' => $self-> prf('pageIndex') - 1);
				return;
			} elsif ( $x > $maxx - $earx*1.5 && $x < $maxx + $earx/2) {
				$self-> prf_set( 'pageIndex' => $self-> prf('pageIndex') + 1);
				return;
			}
		}
	}
	$self-> SUPER::on_mousedown( $btn, $mod, $x, $y);
}

sub prf_tabs    { $_[0]-> repaint; }
sub prf_orientation { $_[0]-> repaint }
sub prf_style { $_[0]-> repaint }
sub prf_pageIndex {
	$_[0]-> SUPER::prf_pageIndex( $_[1]);
	$_[0]-> repaint;
}


package Prima::VB::Header;
use strict;
use vars qw(@ISA);
@ISA = qw( Prima::VB::CommonControl);

# use Prima::Widget::Header;

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		items => ['items', 'widths'],
		uiv   => ['offset', 'minTabWidth'],
		iv    => ['pressed'],
		bool  => ['clickable', 'draggable', 'vertical', 'scalable'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw ( pressed);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	$canvas-> clear;
	my @w = @{$self-> prf('widths')};
	my @i = @{$self-> prf('items')};
	my $v = $self-> prf('vertical');
	my $fh = $canvas-> font-> height;
	my @c3d = ( $canvas-> light3DColor, $canvas-> dark3DColor);
	my $x;
	my $z = 0;
	for ( $x = 0; $x < @i; $x++) {
		my $ww = $w[$x];
		$ww = $canvas-> get_text_width( $i[$x]) if !(defined($ww)) || !($ww =~ m/^\s*\d+\s*$/);
		my @rc = $v ? ( 0, $z, $sz[0]-1, $z + $ww) :  ( $z, 0, $z + $ww, $sz[1]-1);
		$canvas-> rect3d( @rc, 1, @c3d);
		$canvas-> clipRect( $rc[0]+1, $rc[1]+1, $rc[2]-1, $rc[3]-1);
		$canvas-> text_shape_out( $i[$x], $rc[0] + 1, $rc[1] + ($rc[3] - $rc[1] - $fh) / 2 + 1);
		$canvas-> clipRect( 0, 0, @sz);
		$z += $ww + 2;
	}
	$self-> common_paint( $canvas);
}


sub prf_items { $_[0]-> repaint; }
sub prf_widths{ $_[0]-> repaint; }


package Prima::VB::DetailedList;
use strict;
use vars qw(@ISA);
@ISA = qw( Prima::VB::ListBox);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		items  => ['headers', 'widths'],
		multiItems  => ['items'],
		uiv    => ['mainColumn', 'columns', 'minTabWidth', 'offset'],
		string => ['headerClass'],
		bool   => ['clickable', 'draggable', 'vertical', 'scalable'],
	);
	$_[0]-> prf_types_delete( $pt, qw(items));
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw ( pressed headerProfile headerDelegations multiColumn autoWidth
	vertical offset pressed gridColor);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior($canvas);
	my @w = @{$self-> prf('widths')};
	my @h = @{$self-> prf('headers')};
	my @i = @{$self-> prf('items')};
	my $c = $self-> prf('columns');
	my $f = $canvas-> font-> height;
	if ( scalar(@r)) {
		$canvas-> color( cl::Gray);
		$canvas-> bar( 1, $r[3] - $f, $r[2], $r[3] - 1);
		$canvas-> color( cl::Fore);
		if ( $c) {
			my $z = $r[0];
			my $j;
			for ( $j = 0; $j < $c; $j++) {
				my $ww = $w[$j];
				$ww = $canvas-> get_text_width( $h[$j])
					if !(defined($ww)) || !($ww =~ m/^\s*\d+\s*$/);
				my @z = ( $z, $r[1], ( $z + $ww > $r[2]) ? $r[2] : $z + $ww, $r[3]);
				$z[2]++; $canvas-> rectangle( @z); $z[2]--;
				$canvas-> draw_text( join("\n", $h[$j], map {
					defined($i[$_]-> [$j]) ? $i[$_]-> [$j] : ''
					} 0..$#i),
					@z,
					dt::NoWordWrap | dt::NewLineBreak | dt::Left |
						dt::Top | dt::UseClip
				);
				$z += $ww + 1;
				last if $z > $r[2];
			}
		}
	}
	$self-> common_paint($canvas);
}

sub prf_items
{
	my ( $self, $data) = @_;
	my $c = $self-> prf('columns');
	for ( @$data) {
		next if scalar @$_ >= $c;
		push( @$_, ('') x ( $c - scalar @$_));
	}
	$self-> repaint;
}

sub prf_columns { $_[0]-> prf_items( $_[0]-> prf('items')); }

sub prf_widths  { $_[0]-> repaint; }
sub prf_headers { $_[0]-> prf_set( columns => scalar @{$_[0]-> prf('headers')}); }


package Prima::VB::Calendar;
use strict;
use vars qw(@ISA);
@ISA = qw( Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		date   => [ 'date' ],
		bool   => ['useLocale'],
		iv     => [ 'day', 'year', 'month' ],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my ( $fh, $fw) = ( $canvas-> font-> height, $canvas-> font-> maximalWidth * 2);
	my @sz = $canvas-> size;
	$canvas-> clear;
	$canvas-> rectangle( 5, 5, $sz[0] - 6, $sz[1] - 17 - $fh * 3);
	$canvas-> rectangle( 5, $sz[1] - $fh * 2 - 10, $sz[0] - 110, $sz[1] - $fh - 6);
	$canvas-> rectangle( $sz[0] - 105, $sz[1] - $fh * 2 - 10, $sz[0] - 5, $sz[1] - $fh - 6);
	$canvas-> clipRect( 6, 6, $sz[0] - 7, $sz[1] - 18 - $fh * 3);
	my ( $xd, $yd) = ( int(( $sz[0] - 10 ) / 7), int(( $sz[1] - $fh * 4 - 22 ) / 7));
	$yd = $fh if $yd < $fh;
	$xd = $fw if $xd < $fw;
	my ( $x, $y, $i) = ( 5 + $xd/2,  $sz[1] - 17 - $fh * 4, 0);
	for ( 1 .. 31 ) {
		$canvas-> text_shape_out( $_, $x, $y);
		$x += $xd;
		next unless $i++ == 6;
		( $x, $y, $i) = ( 5 + $xd/2, $y - $yd, 0);
	}
	$canvas-> clipRect( 0, 0, @sz);
	$self-> common_paint($canvas);
}

package Prima::VB::AbstractGrid;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::CommonControl Prima::VB::BiScroller);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onClick',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onSelectCell  => 'my ( $self, $column, $row) = @_;',
		onDrawCell    => <<DRAWCELL,
my ( \$self, \$canvas,
     \$column, \$row, \$indent,
     \$sx1, \$sy1, \$sx2, \$sy2,
     \$cx1, \$cy1, \$cx2, \$cy2,
     \$selected, \$focused, \$prelight
   )  = \@_;
DRAWCELL
		onGetRange    => 'my ( $self, $axis, $index, $min, $max) = @_;',
		onMeasure     => 'my ( $self, $axis, $index, $breadth) = @_;',
		onSetExtent   => 'my ( $self, $axis, $index, $breadth) = @_;',
		onStringify   => 'my ( $self, $column, $row, $text_ref) = @_;',
	);
}


sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		bool    => [qw( allowChangeCellHeight allowChangeCellWidth autoHScroll autoVScroll
				clipCells drawHGrid drawVGrid hScroll vScroll multiSelect)],
		uiv     => [qw(columns constantCellWidth constantCellHeight gridGravity
				leftCell topCell rows)],
		uiv_undef=>[ qw(borderWidth)],
		upoint  => [qw(focusedCell)],
		color   => [qw(gridColor indentCellBackColor indentCellColor)],
		urect   => [qw(cellIndents)],
		string  => ['scrollBarClass',],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (offset);
}


package Prima::VB::Grid;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::AbstractGrid);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		multiItems  => ['cells'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @r = $self-> paint_exterior($canvas);
	my @i = @{$self-> prf('cells')};
	my $c = $self-> prf('columns');
	my $r = $self-> prf('rows');
	my $f = $canvas-> font-> height;
	if ( scalar(@r) && $c && $r) {
		my $z = $r[0];
		my $j;
		my @polyline;
		my $lowline = $r[3] - $r * $f;
		$lowline = $r[1] if $lowline < $r[1];
		for ( $j = 0; $j < $c; $j++) {
			my $ww = $canvas-> get_text_width(
				defined($i[0]-> [$j]) ? $i[0]-> [$j] : ''
			);
			my @z = ( $z, $r[1], ( $z + $ww > $r[2]) ? $r[2] : $z + $ww, $r[3]);
			push @polyline, $z[2]+1, $lowline, $z[2]+1, $r[3];
			$canvas-> draw_text( join("\n", map {
				defined($i[$_]-> [$j]) ? $i[$_]-> [$j] : ''
				} 0..($r-1)),
				@z,
				dt::NoWordWrap | dt::NewLineBreak | dt::Left | dt::Top | dt::UseClip
			);
			$z += $ww + 1;
			last if $z > $r[2];
		}
		push @polyline, $r[0], $lowline, $r[0], $r[3];
		push @polyline, map { $r[0], $r[3] - $_ * $f, $z, $r[3] - $_ * $f } 1 .. $r;
		$self-> lines(\@polyline);
	}
	$self-> common_paint($canvas);
}

sub prf_cells
{
	my ( $self, $data) = @_;
	my $c = $self-> prf('columns');
	my $r = $self-> prf('rows');
	for ( @$data) {
		next if scalar @$_ >= $c;
		push( @$_, ('') x ( $c - scalar @$_));
	}
	if ( scalar @$data < $r) {
		$r -= @$data;
		push @$data, [('') x $c] while $r--;
	}

	$self-> repaint;
}

sub prf_columns { $_[0]-> prf_cells( $_[0]-> prf('cells')); }
sub prf_rows    { $_[0]-> prf_cells( $_[0]-> prf('cells')); }

package Prima::VB::DateTime;
use base qw(Prima::VB::InputLine);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onChange',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = ( string  => ['format']);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete @{$pf}{qw(
		insertMode autoSelect autoTab firstChar charOffset textDirection selection text
		caseSensitive clipChildren focusedItem hScroll hScrollBarProfile vScroll vScrollBarProfile
		gridColor integralHeight itemWidth listVisible wordDelimiters multiSelect drawGrid
		extendedSelect autoHScroll autoVScroll integralWidth listDelegations listHeight listClass
		items maxLen multiColumn offset readOnly align buttonClass buttonDelegations editClass
		editDelegations editHeight literal scrollBarClass selectedItems textLigation topItem 
		writeOnly passwordChar
	)};
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	local $self->{default}->{writeOnly} = 0;
	local $self->{default}->{passwordChar} = '';
	$self->text($self->prf('format'));
	$self->SUPER::on_paint($canvas);
}

sub prf_format { $_[0]-> repaint }

package Prima::VB::Date;
use base qw(Prima::VB::DateTime);
package Prima::VB::Time;
use base qw(Prima::VB::DateTime);

package Prima::VB::CheckList;
use strict;
use vars qw(@ISA);
@ISA = qw( Prima::VB::ListBox);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = ( uiv  => ['vector']);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

package Prima::VB::KeySelector;
use base qw(Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = ( text => ['key'] );
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	$canvas-> clear;
	$canvas-> draw_text( 
		"NoKey\n[ ] Ctrl\n[ ] Shift\n[ ] Alt\n___________________\nPress shortcut key",
		10, 10, $self->width - 10, $self->height - 10,
		dt::NoWordWrap | dt::NewLineBreak | dt::Left | dt::Top | dt::UseClip
	);
	$self-> common_paint($canvas);
}

package Prima::VB::FrameSet;
use base qw(Prima::VB::CommonControl);

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		text => ['key'],
		uiv => [qw(frameCount sliderWidth separatorWidth)],
		bool => [qw(flexible opaqueResize vertical)],
		items => [qw(sliders frameSizes)]
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

1;

=head1 NAME

Prima::VB::CoreClasses - Visual Builder extended widgets and types

=head1 DESCRIPTION

The package provides the proxy classes that cover the majority of the
widget classes shipped with the toolkit

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<VB>, L<Prima::VB::Classes>

=cut
