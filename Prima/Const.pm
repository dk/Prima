package Prima::Const;
use Prima '';
use Carp;

sub AUTOLOAD {
	my ($pack,$constname) = ($AUTOLOAD =~ /^(.*)::(.*)$/);
	my $val = eval "\&${pack}::constant(\$constname)";
	croak $@ if $@;
	*$AUTOLOAD = sub () { $val };
	goto &$AUTOLOAD;
}

use strict;
use warnings;

# double lines for CPAN indexer, we don't want these packages in top-level namespace
package
    nt; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# notification types
package
    kb; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# keyboard-related constants
package
    km; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# keyboard modifiers
package
    mb; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# mouse buttons & message box constants
package
    ta; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# text alignment
package
    cl; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# colors

sub from_rgb { ($_[2] & 0xff) | (($_[1] & 0xff) << 8) | (($_[0] & 0xff) << 16) }
sub to_rgb   { (( $_[0]>>16) & 0xFF, ($_[0]>>8) & 0xFF, $_[0] & 0xFF) }
sub from_bgr { ($_[0] & 0xff) | (($_[1] & 0xff) << 8) | (($_[2] & 0xff) << 16) }
sub to_bgr   { ($_[0] & 0xFF, ($_[0]>>8) & 0xFF, ( $_[0]>>16) & 0xFF) }

package
    ci; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# color indices
package
    wc; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# widget classes
package
    cm; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# commands
package
    rop; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# raster operations

sub blend($) { alpha(rop::DstAtop, (255 - $_[0]) x 2) }

sub alpha($;$$)
{
	my ($rop, $src_alpha, $dst_alpha) = @_;

	if (defined $src_alpha) {
		$src_alpha =   0 if $src_alpha < 0;
		$src_alpha = 255 if $src_alpha > 255;
		$rop |= rop::SrcAlpha | ( $src_alpha << rop::SrcAlphaShift );
	}

	if (defined $dst_alpha) {
		$dst_alpha =   0 if $dst_alpha < 0;
		$dst_alpha = 255 if $dst_alpha > 255;
		$rop |= rop::DstAlpha | ( $dst_alpha << rop::DstAlphaShift );
	}

	return $rop;
}

package
    gm; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# grow modes
package
    lp; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# line pen styles
package
    fp; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# fill styles & font pitches
package
    le; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# line ends
package
    lj; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# line joins
package
    fs; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# font styles
package
    fw; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# font weights
package
    bi; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# border icons
package
    bs; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# border styles
package
    ws; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# window states
package
    sv; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# system values
package
    im; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# image types
package
    ictp; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# Image conversion types: dithering
package
    ictd; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# Image conversion types: palette optimization
package
    ict; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# Image conversion types
package
    is; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# Image statistics types
package
    ist; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# Image scaling types
package
    am; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# Icon auto masking
package
    apc; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# OS type
package
    gui; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# GUI types
package
    dt; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# drives types & draw_text constants
package
    cr; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# pointer id's
package
    sbmp; *AUTOLOAD =\&Prima::Const::AUTOLOAD;	# system bitmaps index
package
    tw; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# text wrapping constants
package
    fds; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# find/replace dialog scope type
package
    fdo; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# find/replace dialog options
package
    fe; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# file events
package
    fr; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# fetch resource constants
package
    mt; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# modality types
package
    gt; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# geometry manager types
package
    ps; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# paint states
package
    scr; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# scroll() results
package
    dbt; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# DeviceBitmap types
package
    rgnop; *AUTOLOAD = \&Prima::Const::AUTOLOAD;# Region operations
package
    rgn; *AUTOLOAD = \&Prima::Const::AUTOLOAD;# Region.rect_inside() results
package
    fm; *AUTOLOAD = \&Prima::Const::AUTOLOAD;# fill modes
package
    ggo; *AUTOLOAD = \&Prima::Const::AUTOLOAD;# glyph outline codes
package
    fv; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# font vector constants
package
    dnd; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# drag-and-drop constants

sub is_one_action { 1 == grep { $_[0] == $_ } (dnd::Copy, dnd::Move, dnd::Link) }

sub pointer
{
	my $action = shift;
	if ( $action == dnd::Copy ) {
		return cr::DragCopy;
	} elsif ( $action == dnd::Move ) {
		return cr::DragMove;
	} elsif ( $action == dnd::Link ) {
		return cr::DragLink;
	} else {
		return cr::DragNone;
	}
}

sub to_one_action
{
	my $actions = shift;
	return dnd::Copy if $actions & dnd::Copy;
	return dnd::Move if $actions & dnd::Move;
	return dnd::Link if $actions & dnd::Link;
	return dnd::None;
}

sub keymod
{
	my $dnd = shift;
	return km::Ctrl  if $dnd == dnd::Move;
	return km::Shift if $dnd == dnd::Link;
	return 0;
}

package
    to; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# text-out constants

1;

=pod

=head1 NAME

Prima::Const - predefined constants

=head1 DESCRIPTION

C<Prima::Const> and L<Prima::Classes> is a minimal set of perl modules needed for
the toolkit. Since the module provides bindings for the core constants, it is required
to be included in every Prima-related module and program.

The constants are assembled under the top-level package names, with no C<Prima::>
prefix. This violates the perl guidelines about package naming, however, it was
considered way too inconvenient to prefix every constant with C<Prima::> string.

This document provides description of all core-coded constants. The constants
are also described in the articles together with the corresponding methods and
properties. For example, C<nt> constants are also described in L<Prima::Object/Flow>
article.

=head1 API

=head2 am::  - Prima::Icon auto masking

See also L<Prima::Image/autoMasking>

	am::None           - no mask update performed
	am::MaskColor      - mask update based on Prima::Icon::maskColor property
	am::MaskIndex      - mask update based on Prima::Icon::maskIndex property
	am::Auto           - mask update based on corner pixel values

=head2 apc:: - OS type

See L<Prima::Application/get_system_info>

	apc::Win32
	apc::Unix

=head2 bi::  - border icons

See L<Prima::Window/borderIcons>

	bi::SystemMenu  - system menu button and/or close button
	                  ( usually with icon ) is shown
	bi::Minimize    - minimize button
	bi::Maximize    - maximize ( and eventual restore )
	bi::TitleBar    - window title
	bi::All         - all of the above

=head2 bs::  - border styles

See L<Prima::Window/borderStyle>

	bs::None      - no border
	bs::Single    - thin border
	bs::Dialog    - thick border
	bs::Sizeable  - thick border with interactive resize capabilities

=head2 ci::  - color indices

See L<Prima::Widget/colorIndex>

	ci::NormalText or ci::Fore
	ci::Normal or ci::Back
	ci::HiliteText
	ci::Hilite
	ci::DisabledText
	ci::Disabled
	ci::Light3DColor
	ci::Dark3DColor
	ci::MaxId

=head2 cl:: - colors

See L<Prima::Widget/colorIndex>

=over

=item Direct color constants

	cl::Black
	cl::Blue
	cl::Green
	cl::Cyan
	cl::Red
	cl::Magenta
	cl::Brown
	cl::LightGray
	cl::DarkGray
	cl::LightBlue
	cl::LightGreen
	cl::LightCyan
	cl::LightRed
	cl::LightMagenta
	cl::Yellow
	cl::White
	cl::Gray

=item Indirect color constants

	cl::NormalText, cl::Fore
	cl::Normal, cl::Back
	cl::HiliteText
	cl::Hilite
	cl::DisabledText
	cl::Disabled
	cl::Light3DColor
	cl::Dark3DColor
	cl::MaxSysColor

=item Special constants

See L<Prima::gp_problems/Colors>

	cl::Set      - logical all-1 color
	cl::Clear    - logical all-0 color
	cl::Invalid  - invalid color value
	cl::SysFlag  - indirect color constant bit set
	cl::SysMask  - indirect color constant bit clear mask

=back

=head2 cm::  - commands

=over

=item Keyboard and mouse commands

See L<Prima::Widget/key_down>, L<Prima::Widget/mouse_down>

	cm::KeyDown
	cm::KeyUp
	cm::MouseDown
	cm::MouseUp
	cm::MouseClick
	cm::MouseWheel
	cm::MouseMove
	cm::MouseEnter
	cm::MouseLeave

=item Internal commands ( used in core only or not used at all )

	cm::Close
	cm::Create
	cm::Destroy
	cm::Hide
	cm::Show
	cm::ReceiveFocus
	cm::ReleaseFocus
	cm::Paint
	cm::Repaint
	cm::Size
	cm::Move
	cm::ColorChanged
	cm::ZOrderChanged
	cm::Enable
	cm::Disable
	cm::Activate
	cm::Deactivate
	cm::FontChanged
	cm::WindowState
	cm::Timer
	cm::Click
	cm::CalcBounds
	cm::Post
	cm::Popup
	cm::Execute
	cm::Setup
	cm::Hint
	cm::DragDrop
	cm::DragOver
	cm::EndDrag
	cm::Menu
	cm::EndModal
	cm::MenuCmd
	cm::TranslateAccel
	cm::DelegateKey

=back

=head2 cr::  - pointer cursor resources

See L<Prima::Widget/pointerType>

	cr::Default                 same pointer type as owner's
	cr::Arrow                   arrow pointer
	cr::Text                    text entry cursor-like pointer
	cr::Wait                    hourglass
	cr::Size                    general size action pointer
	cr::Move                    general move action pointer
	cr::SizeWest, cr::SizeW     right-move action pointer
	cr::SizeEast, cr::SizeE     left-move action pointer
	cr::SizeWE                  general horizontal-move action pointer
	cr::SizeNorth, cr::SizeN    up-move action pointer
	cr::SizeSouth, cr::SizeS    down-move action pointer
	cr::SizeNS                  general vertical-move action pointer
	cr::SizeNW                  up-right move action pointer
	cr::SizeSE                  down-left move action pointer
	cr::SizeNE                  up-left move action pointer
	cr::SizeSW                  down-right move action pointer
	cr::Invalid                 invalid action pointer
	cr::DragNone                pointer for an invalid dragging target
	cr::DragCopy                pointer to show that a dnd::Copy action can be accepted
	cr::DragMove                pointer to show that a dnd::Move action can be accepted
	cr::DragLink                pointer to show that a dnd::Link action can be accepted
	cr::User                    user-defined icon

=head2 dbt::  - device bitmap types

        dbt::Bitmap                 monochrome 1 bit bitmap
        dbt::Pixmap                 bitmap compatible with display format
        dbt::Layered                bitmap compatible with display format with alpha channel

=head2 dnd::  - drag and drop action constants and functions

        dnd::None                   no DND action was selected or performed
        dnd::Copy                   copy action
        dnd::Move                   move action
        dnd::Link                   link action
        dnd::Mask                   combination of all valid actions

=over

=item is_one_action ACTIONS

Returns true if C<ACTIONS> is not a combination of C<dnd::> constants.

=item pointer ACTION

Returns a C<cr::> constant corresponding to the C<ACTION>

=item to_one_action ACTIONS

Selects a best single action from combination of allowes C<ACTIONS>

=item keymod ACTION

Returns a C<km::> keyboard modifier constant the C<ACTION> will be selected,
when possible, if the user presses that modifier. Return 0 for C<dnd::Copy>
that is a standard action to be performed without any modifiers.

=back

=head2 dt::  - drive types

See L<Prima::Utils/query_drive_type>

	dt::None
	dt::Unknown
	dt::Floppy
	dt::HDD
	dt::Network
	dt::CDROM
	dt::Memory

=head2 dt::  - Prima::Drawable::draw_text constants

	dt::Left              - text is aligned to the left boundary
	dt::Right             - text is aligned to the right boundary
	dt::Center            - text is aligned horizontally in center
	dt::Top               - text is aligned to the upper boundary
	dt::Bottom            - text is aligned to the lower boundary
	dt::VCenter           - text is aligned vertically in center
	dt::DrawMnemonic      - tilde-escapement and underlining is used
	dt::DrawSingleChar    - sets tw::BreakSingle option to
				Prima::Drawable::text_wrap call
	dt::NewLineBreak      - sets tw::NewLineBreak option to
				Prima::Drawable::text_wrap call
	dt::SpaceBreak        - sets tw::SpaceBreak option to
			        Prima::Drawable::text_wrap call
	dt::WordBreak         - sets tw::WordBreak option to
				Prima::Drawable::text_wrap call
	dt::ExpandTabs        - performs tab character ( \t ) expansion
	dt::DrawPartial       - draws the last line, if it is visible partially
	dt::UseExternalLeading- text lines positioned vertically with respect to
				the font external leading
	dt::UseClip           - assign ::clipRect property to the boundary rectangle
	dt::QueryLinesDrawn   - calculates and returns number of lines drawn
				( contrary to dt::QueryHeight )
	dt::QueryHeight       - if set, calculates and returns vertical extension
				of the lines drawn
	dt::NoWordWrap        - performs no word wrapping by the width of the boundaries
	dt::WordWrap          - performs word wrapping by the width of the boundaries
	dt::BidiText          - use bidirectional formatting, if available
	dt::Default           - dt::NewLineBreak|dt::WordBreak|dt::ExpandTabs|
				dt::UseExternalLeading

=head2 fdo:: - find / replace dialog options

See L<Prima::EditDialog>

	fdo::MatchCase
	fdo::WordsOnly
	fdo::RegularExpression
	fdo::BackwardSearch
	fdo::ReplacePrompt

=head2 fds:: - find / replace dialog scope type

See L<Prima::EditDialog>

	fds::Cursor
	fds::Top
	fds::Bottom

=head2 fe::  - file events constants

See L<Prima::File>

	fe::Read
	fe::Write
	fe::Exception

=head2 fm::  - fill modes 

See L<Prima::Drawable/fillMode>

	fp::Alternate
	fp::Winding
	fp::Overlay

=head2 fp::  - standard fill pattern indices

See L<Prima::Drawable/fillPattern>

	fp::Empty
	fp::Solid
	fp::Line
	fp::LtSlash
	fp::Slash
	fp::BkSlash
	fp::LtBkSlash
	fp::Hatch
	fp::XHatch
	fp::Interleave
	fp::WideDot
	fp::CloseDot
	fp::SimpleDots
	fp::Borland
	fp::Parquet

=head2 fp::  - font pitches

See L<Prima::Drawable/pitch>

	fp::Default
	fp::Fixed
	fp::Variable

=head2 fr::  - fetch resource constants

See L<Prima::Widget/fetch_resource>

	fr::Color
	fr::Font
	fs::String

=head2 fs::  - font styles

See L<Prima::Drawable/style>

	fs::Normal
	fs::Bold
	fs::Thin
	fs::Italic
	fs::Underlined
	fs::StruckOut
	fs::Outline

=head2 fw::  - font weights

See L<Prima::Drawable/weight>

	fw::UltraLight
	fw::ExtraLight
	fw::Light
	fw::SemiLight
	fw::Medium
	fw::SemiBold
	fw::Bold
	fw::ExtraBold
	fw::UltraBold

=head2 ggo::  - glyph outline commands

	ggo::Move
	ggo::Line
	ggo::Conic
	ggo::Cubic

See also L<Prima::Drawable/render_glyph>

=head2 gm::  - grow modes

See L<Prima::Widget/growMode>

=over

=item Basic constants

	gm::GrowLoX	widget's left side is kept in constant
			distance from owner's right side
	gm::GrowLoY	widget's bottom side is kept in constant
			distance from owner's top side
	gm::GrowHiX	widget's right side is kept in constant
			distance from owner's right side
	gm::GrowHiY	widget's top side is kept in constant
			distance from owner's top side
	gm::XCenter	widget is kept in center on its owner's
			horizontal axis
	gm::YCenter	widget is kept in center on its owner's
			vertical axis
	gm::DontCare	widgets origin is maintained constant relative
			to the screen

=item Derived or aliased constants

	gm::GrowAll      gm::GrowLoX|gm::GrowLoY|gm::GrowHiX|gm::GrowHiY
	gm::Center       gm::XCenter|gm::YCenter
	gm::Client       gm::GrowHiX|gm::GrowHiY
	gm::Right        gm::GrowLoX|gm::GrowHiY
	gm::Left         gm::GrowHiY
	gm::Floor        gm::GrowHiX

=back

=head2 gui:: - GUI types

See L<Prima::Application/get_system_info>

	gui::Default
	gui::PM
	gui::Windows
	gui::XLib
	gui::GTK

=head2 le::  - line end styles

See L<Prima::Drawable/lineEnd>

	le::Flat
	le::Square
	le::Round

=head2 lj::  - line join styles

See L<Prima::Drawable/lineJoin>

	lj::Round
	lj::Bevel
	lj::Miter

=head2 lp::  - predefined line pattern styles

See L<Prima::Drawable/linePattern>

	lp::Null           #    ""              /*              */
	lp::Solid          #    "\1"            /* ___________  */
	lp::Dash           #    "\x9\3"         /* __ __ __ __  */
	lp::LongDash       #    "\x16\6"        /* _____ _____  */
	lp::ShortDash      #    "\3\3"          /* _ _ _ _ _ _  */
	lp::Dot            #    "\1\3"          /* . . . . . .  */
	lp::DotDot         #    "\1\1"          /* ............ */
	lp::DashDot        #    "\x9\6\1\3"     /* _._._._._._  */
	lp::DashDotDot     #    "\x9\3\1\3\1\3" /* _.._.._.._.. */

=head2 im::  - image types

See L<Prima::Image/type>.

=over

=item Bit depth constants

	im::bpp1
	im::bpp4
	im::bpp8
	im::bpp16
	im::bpp24
	im::bpp32
	im::bpp64
	im::bpp128

=item Pixel format constants

	im::Color
	im::GrayScale
	im::RealNumber
	im::ComplexNumber
	im::TrigComplexNumber
	im::SignedInt


=item Mnemonic image types

	im::Mono          - im::bpp1
	im::BW            - im::bpp1 | im::GrayScale
	im::16            - im::bpp4
	im::Nibble        - im::bpp4
	im::256           - im::bpp8
	im::RGB           - im::bpp24
	im::Triple        - im::bpp24
	im::Byte          - gray 8-bit unsigned integer
	im::Short         - gray 16-bit unsigned integer
	im::Long          - gray 32-bit unsigned integer
	im::Float         - float
	im::Double        - double
	im::Complex       - dual float
	im::DComplex      - dual double
	im::TrigComplex   - dual float
	im::TrigDComplex  - dual double

=item Extra formats

	im::fmtBGR
	im::fmtRGBI
	im::fmtIRGB
	im::fmtBGRI
	im::fmtIBGR

=item Masks

	im::BPP      - bit depth constants
	im::Category - category constants
	im::FMT      - extra format constants

=back

=head2 ict:: - image conversion types

See L<Prima::Image/conversion>.

	ict::None            - no dithering, with static palette or palette optimized by source palette
	ict::Posterization   - no dithering, with optimized palette by source pixels
	ict::Ordered         - 8x8 ordered halftone dithering
	ict::ErrorDiffusion  - error diffusion dithering with static palette
	ict::Optimized       - error diffusion dithering with optimized palette

Their values are combinations of C<ictp::> and C<ictd::> constants, see below.

=head2 ictd:: - image conversion types, dithering

These constants select color correction (dithering) algorithm when downsampling
an image

	ictd::None            - no dithering, pure colors only
	ictd::Ordered         - 8x8 ordered halftone dithering (checkerboard)
	ictd::ErrorDiffusion  - error diffusion dithering (2/5 down, 2/5 right, 1/5 down/right)

=head2 ictp:: - image conversion types, palette optimization

These constants select how the target palette is made up when downsampling an image.

	ictp::Unoptimized  - use whatever color mapping method is fastest,
	                     image quality can be severely compromized
	ictp::Cubic        - use static cubic palette; a bit slower,
	                     guaranteed mediocre quality
	ictp::Optimized    - collect available colors in the image;
	                     slowest, gives best results

Not all combination of ictp and ictd constants are valid

=head2 is::  - image statistics indices

See L<Prima::Image/stats>.

	is::RangeLo  - minimum pixel value
	is::RangeHi  - maximum pixel value
	is::Mean     - mean value
	is::Variance - variance
	is::StdDev   - standard deviation
	is::Sum      - sum of pixel values
	is::Sum2     - sum of squares of pixel values

=head2 kb::  - keyboard virtual codes

See also L<Prima::Widget/KeyDown>.

=over

=item Modificator keys

	kb::ShiftL   kb::ShiftR   kb::CtrlL      kb::CtrlR
	kb::AltL     kb::AltR     kb::MetaL      kb::MetaR
	kb::SuperL   kb::SuperR   kb::HyperL     kb::HyperR
	kb::CapsLock kb::NumLock  kb::ScrollLock kb::ShiftLock

=item Keys with character code defined

	kb::Backspace  kb::Tab    kb::Linefeed   kb::Enter
	kb::Return     kb::Escape kb::Esc        kb::Space

=item Function keys

	kb::F1 .. kb::F30
	kb::L1 .. kb::L10
	kb::R1 .. kb::R10

=item Other

	kb::Clear    kb::Pause   kb::SysRq  kb::SysReq
	kb::Delete   kb::Home    kb::Left   kb::Up
	kb::Right    kb::Down    kb::PgUp   kb::Prior
	kb::PageUp   kb::PgDn    kb::Next   kb::PageDown
	kb::End      kb::Begin   kb::Select kb::Print
	kb::PrintScr kb::Execute kb::Insert kb::Undo
	kb::Redo     kb::Menu    kb::Find   kb::Cancel
	kb::Help     kb::Break   kb::BackTab

=item Masking constants

	kb::CharMask - character codes
	kb::CodeMask - virtual key codes ( all other kb:: values )
	kb::ModMask  - km:: values

=back

=head2 km::  - keyboard modifiers

See also L<Prima::Widget/KeyDown>.

	km::Shift
	km::Ctrl
	km::Alt
	km::KeyPad
	km::DeadKey
	km::Unicode

=head2 mt:: - modality types

See L<Prima::Window/get_modal>, L<Prima::Window/get_modal_window>

	mt::None
	mt::Shared
	mt::Exclusive

=head2 nt::  - notification types

Used in C<Prima::Component::notification_types> to describe
event flow.

See also L<Prima::Object/Flow>.

=over

=item Starting point constants

	nt::PrivateFirst
	nt::CustomFirst

=item Direction constants

	nt::FluxReverse
	nt::FluxNormal

=item Complexity constants

	nt::Single
	nt::Multiple
	nt::Event

=item Composite constants

	nt::Default       ( PrivateFirst | Multiple | FluxReverse)
	nt::Property      ( PrivateFirst | Single   | FluxNormal )
	nt::Request       ( PrivateFirst | Event    | FluxNormal )
	nt::Notification  ( CustomFirst  | Multiple | FluxReverse )
	nt::Action        ( CustomFirst  | Single   | FluxReverse )
	nt::Command       ( CustomFirst  | Event    | FluxReverse )

=back

=head2 mb::  - mouse buttons

See also L<Prima::Widget/MouseDown>.

	mb::b1 or mb::Left
	mb::b2 or mb::Middle
	mb::b3 or mb::Right
	mb::b4
	mb::b5
	mb::b6
	mb::b7
	mb::b8

=head2 mb:: - message box constants

=over

=item Message box and modal result button commands

See also L<Prima::Window/modalResult>, L<Prima::Button/modalResult>.

	mb::OK, mb::Ok
	mb::Cancel
	mb::Yes
	mb::No
	mb::Abort
	mb::Retry
	mb::Ignore
	mb::Help

=item Message box composite ( multi-button ) constants

	mb::OKCancel, mb::OkCancel
	mb::YesNo
	mb::YesNoCancel

=item Message box icon and bell constants

	mb::Error
	mb::Warning
	mb::Information
	mb::Question

=back

=head2 ps:: - paint states

	ps::Disabled    - can neither draw, nor get/set graphical properties on an object
	ps::Enabled     - can both draw and get/set graphical properties on an object
	ps::Information - can only get/set graphical properties on an object

For brevity, ps::Disabled is equal to 0 so this allows for simple boolean testing whether one can
get/set graphical properties on an object.

See L<Drawable/get_paint_state>.

=head2 rgn:: - result of Prima::Region.rect_inside

	rgn::Inside
	rgn::Outside    - rectangle is fully outside the region
	rgn::Partially

=head2 rgnop:: - Prima::Region.combine set operations

	rgnop::Copy
	rgnop::Intersect
	rgnop::Union
	rgnop::Xor
	rgnop::Diff

=head2 rop:: - raster operation codes

See L<Prima::Drawable/Raster operations>

        rop::Blackness      #   = 0
        rop::NotOr          #   = !(src | dest)
        rop::NotSrcAnd      #  &= !src
        rop::NotPut         #   = !src
        rop::NotDestAnd     #   = !dest & src
        rop::Invert         #   = !dest
        rop::XorPut         #  ^= src
        rop::NotAnd         #   = !(src & dest)
        rop::AndPut         #  &= src
        rop::NotXor         #   = !(src ^ dest)
        rop::NotSrcXor      #     alias for rop::NotXor
        rop::NotDestXor     #     alias for rop::NotXor
        rop::NoOper         #   = dest
        rop::NotSrcOr       #  |= !src
        rop::CopyPut        #   = src
        rop::NotDestOr      #   = !dest | src
        rop::OrPut          #  |= src
        rop::Whiteness      #   = 1

12 Porter-Duff operators

        rop::Clear       # = 0
        rop::Xor         # = src ( 1 - dstA ) + dst ( 1 - srcA )
        rop::SrcOver     # = src + dst (1 - srcA) 
        rop::DstOver     # = dst + src (1 - dstA)
        rop::SrcCopy     # = src
        rop::DstCopy     # = dst
        rop::SrcIn       # = src dstA
        rop::DstIn       # = dst srcA
        rop::SrcOut      # = src ( 1 - dstA )
        rop::DstOut      # = dst ( 1 - srcA )
        rop::SrcAtop     # = src dstA + dst ( 1 - srcA )
        rop::DstAtop     # = dst srcA + src ( 1 - dstA )

        rop::PorterDuffMask - masks out all bits but the constants above

Photoshop operators

	rop::Add
	rop::Multiply
	rop::Screen
	rop::Overlay
	rop::Darken
	rop::Lighten
	rop::ColorDodge
	rop::ColorBurn
	rop::HardLight
	rop::SoftLight
	rop::Difference
	rop::Exclusion

Constant alpha flags

        rop::SrcAlpha
        rop::SrcAlphaShift
        rop::DstAlpha
        rop::DstAlphaShift
	rop::ConstantAlpha

Others

        rop::AlphaCopy

=head2 sbmp:: - system bitmaps indices

See also L<Prima::StdBitmap>.

	sbmp::Logo
	sbmp::CheckBoxChecked
	sbmp::CheckBoxCheckedPressed
	sbmp::CheckBoxUnchecked
	sbmp::CheckBoxUncheckedPressed
	sbmp::RadioChecked
	sbmp::RadioCheckedPressed
	sbmp::RadioUnchecked
	sbmp::RadioUncheckedPressed
	sbmp::Warning
	sbmp::Information
	sbmp::Question
	sbmp::OutlineCollapse
	sbmp::OutlineExpand
	sbmp::Error
	sbmp::SysMenu
	sbmp::SysMenuPressed
	sbmp::Max
	sbmp::MaxPressed
	sbmp::Min
	sbmp::MinPressed
	sbmp::Restore
	sbmp::RestorePressed
	sbmp::Close
	sbmp::ClosePressed
	sbmp::Hide
	sbmp::HidePressed
	sbmp::DriveUnknown
	sbmp::DriveFloppy
	sbmp::DriveHDD
	sbmp::DriveNetwork
	sbmp::DriveCDROM
	sbmp::DriveMemory
	sbmp::GlyphOK
	sbmp::GlyphCancel
	sbmp::SFolderOpened
	sbmp::SFolderClosed
	sbmp::Last

=head2 sv::  - system value indices

See also L<Prima::Application/get_system_value>

	sv::YMenu            - height of menu bar in top-level windows
	sv::YTitleBar        - height of title bar in top-level windows
	sv::XIcon            - width and height of main icon dimensions,
	sv::YIcon              acceptable by the system
	sv::XSmallIcon       - width and height of alternate icon dimensions,
	sv::YSmallIcon         acceptable by the system
	sv::XPointer         - width and height of mouse pointer icon
	sv::YPointer           acceptable by the system
	sv::XScrollbar       - width of the default vertical scrollbar
	sv::YScrollbar       - height of the default horizontal scrollbar
	sv::XCursor          - width of the system cursor
	sv::AutoScrollFirst  - the initial and the repetitive
	sv::AutoScrollNext     scroll timeouts
	sv::InsertMode       - the system insert mode
	sv::XbsNone          - widths and heights of the top-level window
	sv::YbsNone            decorations, correspondingly, with borderStyle
	sv::XbsSizeable        bs::None, bs::Sizeable, bs::Single, and
	sv::YbsSizeable        bs::Dialog.
	sv::XbsSingle
	sv::YbsSingle
	sv::XbsDialog
	sv::YbsDialog
	sv::MousePresent     - 1 if the mouse is present, 0 otherwise
	sv::MouseButtons     - number of the mouse buttons
	sv::WheelPresent     - 1 if the mouse wheel is present, 0 otherwise
	sv::SubmenuDelay     - timeout ( in ms ) before a sub-menu shows on
				an implicit selection
	sv::FullDrag         - 1 if the top-level windows are dragged dynamically,
	                       0 - with marquee mode
	sv::DblClickDelay    - mouse double-click timeout in milliseconds
	sv::ShapeExtension   - 1 if Prima::Widget::shape functionality is supported,
	                       0 otherwise
	sv::ColorPointer     - 1 if system accepts color pointer icons.
	sv::CanUTF8_Input    - 1 if system can generate key codes in unicode
	sv::CanUTF8_Output   - 1 if system can output utf8 text
	sv::CompositeDisplay - 1 if system uses double-buffering and alpha composition for the desktop,
	                       0 if it doesn't, -1 if unknown
	sv::LayeredWidgets   - 1 if system supports layering
	sv::DWM              - 1 if system supports DWM API
	sv::FixedPointerSize - 0 if system doesn't support arbitrary sized pointers and will resize custom icons to the system size

=head2 ta::  - alignment constants

Used in: L<Prima::InputLine>, L<Prima::ImageViewer>, L<Prima::Label>.

	ta::Left
	ta::Right
	ta::Center

	ta::Top
	ta::Bottom
	ta::Middle

=head2 tw::  - text wrapping constants

See L<Prima::Drawable/text_wrap>

	tw::CalcMnemonic          - calculates tilde underline position
	tw::CollapseTilde         - removes escaping tilde from text
	tw::CalcTabs              - wraps text with respect to tab expansion
	tw::ExpandTabs            - expands tab characters
	tw::BreakSingle           - determines if text is broken to single
	                            characters when text cannot be fit
	tw::NewLineBreak          - breaks line on newline characters
	tw::SpaceBreak            - breaks line on space or tab characters
	tw::ReturnChunks          - returns wrapped text chunks
	tw::ReturnLines           - returns positions and lengths of wrapped
	                            text chunks
	tw::WordBreak             - defines if text break by width goes by the
	                            characters or by the words
	tw::ReturnFirstLineLength - returns length of the first wrapped line
	tw::Default               - tw::NewLineBreak | tw::CalcTabs | tw::ExpandTabs |
	                            tw::ReturnLines | tw::WordBreak

=head2 wc::  - widget classes

See L<Prima::Widget/widgetClass>

	wc::Undef
	wc::Button
	wc::CheckBox
	wc::Combo
	wc::Dialog
	wc::Edit
	wc::InputLine
	wc::Label
	wc::ListBox
	wc::Menu
	wc::Popup
	wc::Radio
	wc::ScrollBar
	wc::Slider
	wc::Widget, wc::Custom
	wc::Window
	wc::Application

=head2 ws::  - window states

See L<Prima::Window/windowState>

	ws::Normal
	ws::Minimized
	ws::Maximized

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Classes>

=cut
