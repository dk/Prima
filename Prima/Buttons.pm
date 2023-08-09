# contains:
#   Button
#   CheckBox
#   Radio
#   SpeedButton
#   RadioGroup ( obsolete )
#   GroupBox
#   CheckBoxGroup ( obsolete )
#
#   AbstractButton
#   Cluster

package Prima::Buttons;
use strict;
use warnings;
use Carp;
use Prima qw(StdBitmap);

package Prima::AbstractButton;
use base qw(
	Prima::Widget
	Prima::Widget::MouseScroller
	Prima::Widget::Fader
);

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	%{Prima::Widget::Fader-> notification_types()},
	Check => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		hotKey       => undef,
		pressed      => 0,
		selectable   => 1,
		autoHeight   => 1,
		autoWidth    => 1,
		autoShaping  => 0,
		buffered     => 1,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> { autoWidth} = 0
		if exists $p-> {width} || exists $p-> {size} || exists $p-> {rect} ||
			( exists $p-> {left} && exists $p-> {right});
	$p-> {autoHeight} = 0
		if exists $p-> {height} || exists $p-> {size} || exists $p-> {rect} ||
			( exists $p-> {top} && exists $p-> {bottom});
	$self-> SUPER::profile_check_in( $p, $default);
}

sub on_translateaccel
{
	my ( $self, $code, $key, $mod) = @_;
	if (
		defined $self-> {accel} &&
		($key == kb::NoKey) &&
		lc chr $code eq $self-> { accel}
	) {
		$self-> clear_event;
		$self-> notify( 'Click');
	}
	if (
		defined $self-> {hotKey} &&
		($key == kb::NoKey) &&
		lc chr $code eq $self-> {hotKey}
	) {
		$self-> clear_event;
		$self-> notify( 'Click');
	}
	if ( $self-> { default} && $key == kb::Enter) {
		$self-> clear_event;
		$self-> notify( 'Click');
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {$_} = $profile{$_} for qw(autoWidth autoHeight hotKey pressed autoShaping);
	return %profile;
}

sub skin
{
	return $_[0]->SUPER::skin unless $#_;
	my $self = shift;
	$self->SUPER::skin($_[1]);
	if ($self->SUPER::skin eq 'flat') {
		$self->update_shape if $self->autoShaping;
	} else {
		$self->shape(undef) if $self->autoShaping;
	}
	$self->repaint;
}

sub fader_current_color
{
	my ( $self, $color) = @_;
	$color = $self->map_color($color);
	if ( $self->{hilite}) {
		return $self-> prelight_color($color);
	} elsif ( defined ( my $f = $self->fader_current_value)) {
		return cl::blend( $color, $self-> prelight_color($color), $f);
	} else {
		return $color;
	}
}

sub cancel_transaction
{
	my $self = $_[0];
	if ( $self-> {mouseTransaction} || $self-> {spaceTransaction}) {
		$self-> {spaceTransaction} = undef;
		$self-> capture(0) if $self-> {mouseTransaction};
		$self-> {mouseTransaction} = undef;
		$self-> pressed( 0);
	}
}

sub on_size { $_[0]->update_shape if $_[0]->skin eq 'flat' and $_[0]->autoShaping }

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;
	if ( $key == kb::Space) {
		$self-> clear_event;
		return if $self-> {spaceTransaction} || $self-> {mouseTransaction};
		$self-> { spaceTransaction} = 1;
		$self-> pressed( 1);
	}
	if (
		defined $self-> {accel} &&
		($key == kb::NoKey) &&
		lc chr $code eq $self-> { accel}
	) {
		$self-> clear_event;
		$self-> notify( 'Click');
	}
}

sub on_keyup
{
	my ( $self, $code, $key, $mod) = @_;

	if ( $key == kb::Space && $self-> {spaceTransaction}) {
		$self-> {spaceTransaction} = undef;
		$self-> capture(0) if $self-> {mouseTransaction};
		$self-> {mouseTransaction} = undef;
		$self-> pressed( 0);
		$self-> update_view;
		$self-> clear_event;
		$self-> notify( 'Click')
	}
}

sub on_leave
{
	my $self = $_[0];
	if ( $self-> {spaceTransaction} || $self-> {mouseTransaction}) {
		$self-> cancel_transaction;
	} else {
		$self-> repaint;
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction} || $self-> {spaceTransaction};
	return if $btn != mb::Left;
	$self-> { mouseTransaction} = 1;
	$self-> { lastMouseOver}  = 1;
	$self-> pressed( 1);
	$self-> capture(1);
	$self-> clear_event;
	$self-> scroll_timer_start if $self-> {autoRepeat};
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	return unless $dbl;
	return if $btn != mb::Left;
	return if $self-> {mouseTransaction} || $self-> {spaceTransaction};
	$self-> { mouseTransaction} = 1;
	$self-> { lastMouseOver}  = 1;
	$self-> pressed( 1);
	$self-> capture(1);
	$self-> clear_event;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	return unless $self-> {mouseTransaction};
	my @size = $self-> size;
	$self-> {mouseTransaction} = undef;
	$self-> {spaceTransaction} = undef;
	$self-> {lastMouseOver}    = undef;
	$self-> capture(0);
	$self-> pressed( 0);
	if ( $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1] ) {
		$self-> clear_event;
		$self-> update_view;
		$self-> notify( 'Click');
	}
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransaction};
	return if $self-> {autoRepeat} && !$self-> scroll_timer_semaphore;
	my @size = $self-> size;
	my $mouseOver = $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1];
	$self-> pressed( $mouseOver) if $self-> { lastMouseOver} != $mouseOver;
	$self-> { lastMouseOver} = $mouseOver;
	return unless $self-> {autoRepeat};
	$self-> scroll_timer_stop, return
		unless $mouseOver;
	$self-> scroll_timer_start, return
		unless $self-> scroll_timer_active;
	$self-> scroll_timer_semaphore(0);
	$self-> notify(q(Click));
}

sub on_fadein
{
	my ( $self, $ends_okay ) = @_;
	if ($ends_okay) {
		$self->{hilite} = 1;
		$self->repaint;
	}
}

sub on_mouseenter
{
	my $self = $_[0];
	$self-> fader_in_mouse_enter if
		!$self-> {spaceTransaction} &&
		!$self-> {mouseTransaction} &&
		$self-> enabled;
}

sub on_mouseleave
{
	my $self = shift;
	if ( $self-> enabled ) {
		$self-> fader_out_mouse_leave;
		undef $self-> {hilite};
	}
}


sub on_fontchanged
{
	$_[0]-> check_auto_size;
	$_[0]-> update_shape if $_[0]->skin eq 'flat' and $_[0]->autoShaping;
}

sub draw_veil
{
	my ($self,$canvas) = (shift, shift);
	my $back = $self-> backColor;
	$canvas-> set(
		color       => cl::Clear,
		backColor   => cl::Set,
		fillPattern => fp::SimpleDots,
		rop         => rop::AndPut
	);
	$canvas-> bar( @_);
	$canvas-> set(
		color       => $back,
		backColor   => cl::Clear,
		rop         => rop::OrPut
	);
	$canvas-> bar( @_);
	$canvas-> set(
		rop        => rop::CopyPut,
		backColor  => $back,
	);
}

sub draw_caption
{
	my ( $self, $canvas, $x, $y) = @_;
	my ($cap, $tilde) = @{ $self-> text_wrap_shape( $self-> text, 
		undef, 
		options => tw::CalcMnemonic|tw::CollapseTilde|tw::ExpandTabs,
		tabs    => 1,
	) };
	unless ( $self->enabled) {
		my $z = $canvas-> color;
		$canvas-> color( cl::White);
		$canvas-> text_out( $cap, $x + 1, $y - 1);
		$canvas->line( 
			$x + 1 + $tilde->{tildeStart}, $y - 1, 
			$x + 1 + $tilde->{tildeEnd}, $y - 1, 
		) if defined $tilde->{tildeLine};
		$canvas-> color( $z);
	}
	$canvas-> text_out( $cap, $x + 1, $y - 1);
	$canvas->line( 
		$x + $tilde->{tildeStart}, $y, 
		$x + $tilde->{tildeEnd}, $y, 
	) if defined $tilde->{tildeLine};
	if ($self-> focused) {
		my ( $fw, $fh) = (
			$canvas-> get_text_width( $cap),
			$canvas-> font-> height,
		);
		$canvas-> rect_focus( $x - 2, $y - 2, $x + 2 + $fw, $y + 2 + $fh)
	}
}

sub caption_box
{
	my ($self,$canvas) = @_;
	my $cap = $self-> text;
	$cap =~ s/~//;
	$canvas = $self unless $canvas;
	my $end_paint_info = $canvas->get_paint_state == ps::Disabled;
	$canvas->begin_paint_info if $end_paint_info;
	$canvas-> font-> style( $canvas-> font-> style  | fs::Bold )
		if $self-> {default} && $self->skin eq 'flat';
	my ( $w, $h ) = ($canvas-> get_text_width( $cap), $canvas-> font-> height);
	$canvas->end_paint_info if $end_paint_info;
	return ($w, $h);
}

sub calc_geom_size { $_[0]-> caption_box }

sub pressed
{
	return $_[0]-> {pressed} unless $#_;
	$_[0]-> { pressed} = $_[1];
	$_[0]-> repaint;
}

sub set_text
{
	my ( $self, $caption) = @_;
	$self-> SUPER::set_text( $caption );
	$self-> {accel} = lc($1) if $caption =~ /~([a-z0-9])/i;
	$self-> check_auto_size;
	$self-> repaint;
}

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> cancel_transaction; $_[0]-> repaint; }
sub on_enter   { $_[0]-> repaint; }

sub hotKey { $#_ ? $_[0]->{hotKey} = $_[1] : $_[0]->{hotKey} }

sub autoHeight
{
	return $_[0]-> {autoHeight} unless $#_;
	my ( $self, $a) = @_;
	return if ( $self-> {autoHeight} ? 1 : 0) == ( $a ? 1 : 0);
	$self-> {autoHeight} = ( $a ? 1 : 0);
	$self-> check_auto_size if $a;
}

sub autoWidth
{
	return $_[0]-> {autoWidth} unless $#_;
	my ( $self, $a) = @_;
	return if ( $self-> {autoWidth} ? 1 : 0) == ( $a ? 1 : 0);
	$self-> {autoWidth} = ( $a ? 1 : 0);
	$self-> check_auto_size if $a;
}

sub autoShaping
{
	return $_[0]-> {autoShaping} unless $#_;
	my ( $self, $a) = @_;
	return if ( $self-> {autoShaping} ? 1 : 0) == ( $a ? 1 : 0);
	$self-> {autoShaping} = ( $a ? 1 : 0);
	if ( $a ) {
		if ( $self->skin eq 'flat') {
			$self->shape(undef);
		} else {
			$self->update_shape;
		}
	}
}

sub update_shape {}

sub check_auto_size
{
	my $self = $_[0];
	my %sets;
	if ( $self-> {autoWidth} || $self-> {autoHeight}) {
		my @geomSize = $self-> calc_geom_size;
		$sets{ geomWidth}  = $geomSize[0] if $self-> {autoWidth};
		$sets{ geomHeight} = $geomSize[1] if $self-> {autoHeight};
		$self-> set( %sets);
	}
}

package Prima::Button;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractButton);

my %standardGlyphScheme = (
		glyphs => 4,
		defaultGlyph  => 0,
		hiliteGlyph   => 0,
		disabledGlyph => 1,
		pressedGlyph  => 2,
		holdGlyph     => 3,
);

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		autoRepeat    => 0,
		borderWidth   => 2,
		checkable     => 0,
		checked       => 0,
		default       => 0,
		flat          => 0,
		glyphs        => 1,
		height        => 36,
		image         => undef,
		imageFile     => undef,
		imageScale    => 1,
		smoothScaling => 1,
		modalResult   => 0,
		vertical      => 0,
		width         => 96,
		widgetClass   => wc::Button,

		defaultGlyph  => 0,
		hiliteGlyph   => 0,
		disabledGlyph => 1,
		pressedGlyph  => 2,
		holdGlyph     => 3,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	my $checkable = exists $p-> {checkable} ? $p-> {checkable} : $default-> {checkable};
	$p-> { checked} = 0 unless $checkable;
}

sub init
{
	my $self = shift;
	$self-> {$_} = 0 for ( qw(
		borderWidth checkable checked default vertical
		defaultGlyph hiliteGlyph disabledGlyph pressedGlyph holdGlyph
		flat modalResult autoRepeat
	));
	$self-> {imageScale} = $self-> {glyphs} = 1;
	$self-> {image} = undef;
	my %profile = $self-> SUPER::init(@_);
	defined $profile{image} ?
		$self-> image( $profile{image}) :
		$self-> imageFile( $profile{imageFile});
	$self-> $_( $profile{$_}) for ( qw(
		borderWidth checkable checked default smoothScaling imageScale glyphs vertical
		defaultGlyph hiliteGlyph disabledGlyph pressedGlyph holdGlyph
		flat modalResult autoRepeat
	));
	return %profile;
}

sub paint_classic
{
	my ( $self, $canvas, $w, $h, @clr) = @_;
	my @fbar = $self-> {default} ?
		( 1, 1, $w - 2, $h - 2):
		( 0, 0, $w - 1, $h - 1);
	if ( !$self-> {flat} || $self-> {hilite}) {
		$self-> rect_bevel( $canvas, @fbar,
			fill => $self->transparent ? undef : $self-> new_gradient(
				palette  => [ $self-> dark3DColor, $clr[1], $self-> light3DColor ],
				spline   => [0,0.5,1,0.5],
				vertical => 0,
			),
			width    => $self-> {borderWidth},
			concave  => ( $_[0]-> { pressed} || $_[0]-> { checked}),
		);
	} else {
		$canvas-> color( $clr[ 1]);
		$canvas-> bar( @fbar) unless $self-> transparent;
	}
	if ( $self-> {default}) {
		$canvas-> color( cl::Black);
		$canvas-> rectangle( 0, 0, $w-1, $h-1);
	}
}

sub paint_flat
{
	my ( $self, $canvas, $w, $h, @clr) = @_;
	$canvas-> graphic_context( sub {
		$canvas-> color( $self-> owner-> backColor );
		$canvas-> bar( 0, 0, $w, $h);
		$canvas-> backColor( $clr[1]);
		$canvas-> color( $clr[0]);
		my $lw = 1;
		$lw++ if $self->{default};
		if ( $self->enabled ) {
			if ( $self->{hilite}) {
				$lw++;
			} elsif ( defined ( my $f = $self->fader_current_value)) {
				$lw += $f;
			}
		}
		$canvas-> antialias(1);
		my $dw;
		if ( $canvas-> antialias ) {
			$dw = $self->font->height;
		} else {
			$dw = 0;
			$lw = int($lw + .5);
		}
		$canvas-> lineWidth( $lw );
		$canvas->
			new_path->
			round_rect( 0 + $lw/2, 0 + $lw/2, $w - $lw/2 - 1, $h - $lw/2 - 1, $dw)->
			fill_stroke;
	}) unless $self-> transparent;
}

sub update_shape
{
	my $self = shift;
	my @sz = $self->size;
	my $shape = $self-> new_path-> round_rect(
		-1.5, -1.5, $sz[0]+.5, $sz[1]+.5,
		$self-> font-> height
	)-> region;
	$self->shape($shape);
}

sub on_paint
{
	my ($self,$canvas)  = @_;
	my @clr  = ( $self-> color, $self-> backColor);
	my $skin = $self->skin;

	my $boldify;
	if ($self-> { default}) {
		if ( $skin eq 'flat') {
			@clr = ( $self-> hiliteBackColor, $self-> hiliteColor);
			$boldify++;
		} else {
			@clr = ( $self-> hiliteColor, $self-> hiliteBackColor);
		}
	}
	if ( $self->enabled ) {
		$clr[1] = $self-> fader_current_color($clr[1]);
	} else {
		@clr = ( $self-> disabledColor, $self-> disabledBackColor);
	}

	my @size = $canvas-> size;

	my $shift  = $self-> {checked} ? 1 : 0;
	if ( $skin eq 'flat') {
		$self->paint_flat($canvas, @size, @clr);
	} else {
		$self->paint_classic($canvas, @size, @clr);
	}
	$shift += $self-> {pressed} ? 2 : 0;

	my $capOk = length($self-> text) > 0;
	my ( $fw, $fh) = $capOk ? $self-> caption_box($canvas) : ( 0, 0);
	my ( $textAtX, $textAtY);

	if ( defined $self-> {image}) {
		my $is = $self->{imageScale};
		my $pw = $self-> {image}-> width / $self-> { glyphs};
		my $ph = $self-> {image}-> height;
		my $sw = $pw * $is;
		my $sh = $ph * $is;
		my $imgNo = $self-> {defaultGlyph};
		my $useVeil = 0;
		my $image = $self->{image};
		if ( $self-> {hilite}) {
			if ( $self->{glyphs} > 1 ) {
				$imgNo = $self-> {hiliteGlyph}
					if $self-> {glyphs} > $self-> {hiliteGlyph} &&
						$self-> {hiliteGlyph} >= 0;
			} elsif ( ref($self-> {hiliteGlyph})) {
				$image = $self->{hiliteGlyph};
			}
		}
		if ( $self-> {checked}) {
			if ( $self->{glyphs} > 1 ) {
				$imgNo = $self-> {holdGlyph} if
					$self-> {glyphs} > $self-> {holdGlyph} &&
						$self-> {holdGlyph} >= 0;
			} elsif ( ref($self->{holdGlyph})) {
				$image = $self->{holdGlyph};
			}
		}
		if ( $self-> {pressed}) {
			if ( $self->{glyphs} > 1 ) {
				$imgNo = $self-> {pressedGlyph} if
					$self-> {glyphs} > $self-> {pressedGlyph} &&
						$self-> {pressedGlyph} >= 0;
			} elsif ( ref($self->{pressedGlyph}) ) {
				$image = $self->{pressedGlyph};
			}
		}
		if ( !$self-> enabled) {
			if ( $self->{glyphs} > 1 ) {
				( $self-> {glyphs} > $self-> {disabledGlyph} && $self-> {disabledGlyph} >= 0) ?
					$imgNo = $self-> {disabledGlyph} :
						$useVeil = 1;
			} elsif (ref($self->{disabledGlyph})) {
				$image = $self->{disabledGlyph};
			} else {
				$useVeil = 1;
			}
		}

		my ( $imAtX, $imAtY);
		if ( $capOk) {
			if ( $self-> { vertical}) {
				$imAtX = ( $size[ 0] - $sw) / 2 + $shift;
				$imAtY = ( $size[ 1] - $fh - $sh) / 3;
				$textAtX = ( $size[0] - $fw) / 2 + $shift;
				$textAtY = $size[ 1] - 2 * $imAtY - $fh - $sh - $shift;
				$imAtY   = $size[ 1] - $imAtY - $sh - $shift;
			} else {
				$imAtX = ( $size[ 0] - $fw - $sw) / 3;
				$imAtY = ( $size[ 1] - $sh) / 2 - $shift;
				$textAtX = 2 * $imAtX + $sw + $shift;
				$textAtY = ( $size[1] - $fh) / 2 - $shift;
				$imAtX += $shift;
			}
		} else {
			$imAtX = ( $size[0] - $sw) / 2 + $shift;
			$imAtY = ( $size[1] - $sh) / 2 - $shift;
		}

		if ( $image && UNIVERSAL::isa($image, 'Prima::Drawable::Metafile')) {
			if ( !$self->enabled && $useVeil && $image->type == dbt::Bitmap) {
				$canvas->color(cl::White);
				$image->execute($canvas, $imAtX+1, $imAtY-1);
				$useVeil = 0;
			}
			$canvas->color($clr[0]);
			$image->execute($canvas, $imAtX, $imAtY);
			goto CAPTION;
		}
		if ( $self-> {smoothScaling} && $is != 1.0 ) {
			my $c = $self->{smooth_cache} //= {
				zoom  => -1,
				obj   => "$image",
				cache => undef,
			};
			if ( $c->{zoom} != $is || $c->{obj} ne $image ) {
				$c->{cache} = ( $self->{glyphs} == 1 ) ?
					$image->dup : $image->extract( $imgNo * $pw, 0, $pw, $ph);
				$c->{cache}->ui_scale( zoom => $is );
				$c->{zoom} = $is;
				$c->{obj}  = "$image";
			}
			$image = $c->{cache};
			$imgNo = 0;
			($pw,$ph) = $image->size;
			($sw,$sh) = $image->size;
		}
		$canvas-> put_image_indirect(
			$image,
			$imAtX, $imAtY,
			$imgNo * $pw, 0,
			$sw, $sh,
			$pw, $ph,
			rop::CopyPut
		);
CAPTION:
		$self-> draw_veil( $canvas, $imAtX, $imAtY, $imAtX + $sw, $imAtY + $sh)
			if $useVeil;
	} else {
		$textAtX = ( $size[0] - $fw) / 2 + $shift;
		$textAtY = ( $size[1] - $fh) / 2 - $shift;
	}
	$canvas-> color( $clr[0]);
	if ($capOk) {
		$canvas-> font-> style( $canvas-> font-> style  | fs::Bold )
			if $boldify;
		$self-> draw_caption( $canvas, $textAtX, $textAtY);
	}
	$canvas-> rect_focus( 4, 4, $size[0] - 5, $size[1] - 5 ) if !$capOk && $self-> focused;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;
	if ( $key == kb::Enter) {
		$self-> clear_event;
		return $self-> notify( 'Click')
	}
	$self-> SUPER::on_keydown( $code, $key, $mod, $repeat);
}

sub on_click
{
	my $self = $_[0];
	$self-> checked( !$self-> checked)
		if $self-> { checkable};
	my $owner = $self-> owner;
	while ( $owner ) {
		if (
			$owner-> isa(q(Prima::Window)) &&
			$owner-> get_modal &&
			$self-> modalResult
		) {
			$owner-> modalResult( $self-> modalResult);
			$owner-> end_modal;
			last;
		} else {
			$owner = $owner-> owner;
		}
	}
}

sub on_check {}

sub std_calc_geom_size
{
	my $self = $_[0];
	my $capOk = length($self-> text);
	my @sz  = $capOk ? $self-> caption_box : (0,0);

	$sz[$_] += 10 for 0,1;

	if ( defined $self-> {image}) {
		my $imw = $self-> {image}-> width  / $self-> { glyphs} * $self-> {imageScale};
		my $imh = $self-> {image}-> height / $self-> { glyphs} * $self-> {imageScale};
		if ( $capOk) {
			if ( $self-> { vertical}) {
				$sz[0] = $imw if $sz[0] < $imw;
				$sz[1] += 2 + $imh;
			} else {
				$sz[0] += 2 + $imw;
				$sz[1] = $imh if $sz[1] < $imh;
			}
		} else {
			$sz[0] += $imw;
			$sz[1] += $imh;
		}
	}
	$sz[$_] += 2 for 0,1;
	$sz[$_] += $self-> {borderWidth} * 2 for 0,1;
	return @sz;
}

sub calc_geom_size
{
	my $self = shift;
	my @sz = $self-> std_calc_geom_size;
	my ($dx, $dy) = ( $self->font->width/7, $self->font->height/16);
	$sz[0] = $dx * 96 if $sz[0] < $dx * 96;
	$sz[1] = $dy * 36 if $sz[1] < $dy * 36;
	return @sz;
}

sub autoRepeat
{
	return $_[0]-> {autoRepeat} unless $#_;
	$_[0]-> {autoRepeat} = $_[1];
}

sub borderWidth
{
	return $_[0]-> {borderWidth} unless $#_;
	my ( $self, $bw) = @_;
	$bw = 0 if $bw < 0;
	$bw = int( $bw);
	return if $bw == $self-> {borderWidth};
	$self-> {borderWidth} = $bw;
	$self-> check_auto_size;
	$self-> repaint;
}

sub checkable
{
	return $_[0]-> {checkable} unless $#_;
	$_[0]-> checked( 0) unless $_[0]-> {checkable} == $_[1];
	$_[0]-> {checkable} = $_[1];
}

sub checked
{
	return $_[0]-> {checked} unless $#_;
	return unless $_[0]-> { checkable};
	return if $_[0]-> {checked}+0 == $_[1]+0;
	$_[0]-> {checked} = $_[1];
	$_[0]-> repaint;
	$_[0]-> notify( 'Check', $_[0]-> {checked});
}

sub default
{
	return $_[0]-> {default} unless $#_;
	my $self = $_[0];
	return if $self-> {default} == $_[1];
	if ( $self-> { default} = $_[1]) {
		my @widgets = $self-> owner-> widgets;
		for ( @widgets) {
			last if $_ == $self;
			$_-> default(0)
				if $_-> isa(q(Prima::Button)) && $_-> default;
		}
	}
	$self-> repaint;
}

sub hilite { $_[0]->{hilite} }

sub defaultGlyph {($#_)?($_[0]-> {defaultGlyph} = $_[1],$_[0]-> repaint) :return $_[0]-> {defaultGlyph}}
sub hiliteGlyph  {($#_)?($_[0]-> {hiliteGlyph}  = $_[1],$_[0]-> repaint) :return $_[0]-> {hiliteGlyph}}
sub disabledGlyph{($#_)?($_[0]-> {disabledGlyph}= $_[1],$_[0]-> repaint) :return $_[0]-> {disabledGlyph}}
sub pressedGlyph {($#_)?($_[0]-> {pressedGlyph} = $_[1],$_[0]-> repaint) :return $_[0]-> {pressedGlyph}}
sub holdGlyph    {($#_)?($_[0]-> {holdGlyph}    = $_[1],$_[0]-> repaint) :return $_[0]-> {holdGlyph}}
sub flat         {($#_)?($_[0]-> {flat}         = $_[1],$_[0]-> repaint) :return $_[0]-> {flat}}

sub image
{
	return $_[0]-> {image} unless $#_;
	my ( $self, $image) = @_;
	$self-> {image} = $image;
	delete $self-> {smooth_cache};
	$self-> check_auto_size;
	$self-> repaint;
}

sub imageFile
{
	return $_[0]-> {imageFile} unless $#_;
	my ($self,$file) = @_;
	$self-> image(undef), return unless defined $file;
	my $img = Prima::Icon-> create;
	my @fp = ($file);
	$fp[0] =~ s/\:(\d+)$//;
	push( @fp, 'index', $1) if defined $1;
	return unless $img-> load(@fp);
	$self-> {imageFile} = $file;
	$self-> image($img);
}

sub imageScale
{
	return $_[0]-> {imageScale} unless $#_;
	my ( $self, $imageScale) = @_;
	$self-> {imageScale} = $imageScale;
	delete $self-> {smooth_cache};
	if ( $self-> {image}) {
		$self-> check_auto_size;
		$self-> repaint;
	}
}

sub smoothScaling
{
	return $_[0]-> {smoothScaling} unless $#_;
	my ( $self, $smoothScaling) = @_;
	$self-> {smoothScaling} = $smoothScaling ? 1 : 0;
}

sub vertical
{
	return $_[0]-> {vertical} unless $#_;
	my ( $self, $vertical) = @_;
	$self-> {vertical} = $vertical;
	$self-> check_auto_size;
	$self-> repaint;
}

sub modalResult
{
	return $_[0]-> {modalResult} unless $#_;
	my $self = $_[0];
	$self-> { modalResult} = $_[1];
	my $owner = $self-> owner;
	while ( $owner ) {
		if (
			$owner-> isa(q(Prima::Window)) &&
			$owner-> get_modal &&
			$self-> {modalResult}
		) {
			$owner-> modalResult( $self-> { modalResult});
			$owner-> end_modal;
			last;
		} else {
			$owner = $owner-> owner;
		}
	}
}

sub glyphs
{
	return $_[0]-> {glyphs} unless $#_;
	my $maxG = defined $_[0]-> {image} ? $_[0]-> {image}-> width : 1;
	$maxG = 1 unless $maxG;
	if ( $_[1] > 0 && $_[1] <= $maxG)
	{
		$_[0]-> {glyphs} = $_[1];
		$_[0]-> repaint;
	}
}


package Prima::Cluster;
use vars qw(@ISA @images);
@ISA = qw(Prima::AbstractButton);

my @images;

Prima::Application::add_startup_notification( sub {
	my $i = 0;
	for (
		sbmp::CheckBoxUnchecked, sbmp::CheckBoxUncheckedPressed,
		sbmp::CheckBoxChecked, sbmp::CheckBoxCheckedPressed,
		sbmp::RadioUnchecked, sbmp::RadioUncheckedPressed,
		sbmp::RadioChecked, sbmp::RadioCheckedPressed
	) {
		$images[ $i] = ( $i > 3) ?
			Prima::StdBitmap::icon( $_) :
			Prima::StdBitmap::image( $_);
		$i++;
	}
});

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		auto           => 1,
		checked        => 0,
		height         => 36,
		ownerBackColor => 1,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> { auto   } = $profile{ auto   };
	$self-> { checked} = $profile{ checked};
	$self-> check_auto_size;
	return %profile;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;
	if ( $key == kb::Tab || $key == kb::BackTab) {
		my ( $next, $owner) = ( $self, $self-> owner);
		while ( $next) {
			last unless $next-> owner == $owner && $next-> isa('Prima::Cluster');
			$next = $next-> next_tab( $key == kb::Tab);
		}
		$next-> select if $next;
		$self-> clear_event;
		return;
	}
	$self-> SUPER::on_keydown( $code, $key, $mod, $repeat);
}

sub on_click
{
	my $self = $_[0];
	$self-> focus;
	$self-> checked( !$self-> checked);
}

sub on_enter
{
	my $self = $_[0];
	$self-> check if $self-> auto;
	$self-> SUPER::on_enter;
}

sub auto { ($#_) ? $_[0]-> {auto} = $_[1] : return $_[0]-> {auto}}

sub checked
{
	return $_[0]-> {checked} unless $#_;
	my $old = $_[0]-> {checked};
	my $new = $_[1] ? 1 : 0;
	if ( $old != $new) {
		$_[0]-> {checked} = $new;
		$_[0]-> repaint;
		$_[0]-> notify( 'Check', $_[0]-> {checked});
	}
}

sub toggle       { my $i = $_[0]-> checked; $_[0]-> checked( !$i); return !$i;}
sub check        { $_[0]-> checked(1)}
sub uncheck      { $_[0]-> checked(0)}

my @static_image0_size;

sub calc_geom_size
{
	my $self = $_[0];
	my @sz   = $self-> caption_box;
	$sz[$_] += 12 for 0,1;
	if ( $images[0]) {
		@static_image0_size = $images[0]-> size
			unless @static_image0_size;
		$sz[0] += $static_image0_size[0] * 1.5 + 2;
		$sz[1] = $static_image0_size[1]
			if $sz[1] < $static_image0_size[1];
	} else {
		my $s = $::application->uiScaling;
		$sz[0] += 16 * 1.5 * $s;
		$sz[1] = 16 * $s if $sz[1] < 16 * $s;
	}
	return @sz;
}

package Prima::CheckBox;
use vars qw(@ISA);
@ISA = qw(Prima::Cluster);

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		auto        => 0,
		widgetClass => wc::CheckBox,
	}
}

sub paint_classic
{
	my ( $self, $canvas, $w, $h, @clr) = @_;

	my ( $image, $imNo);
	if ( $self-> { checked}) {
		$imNo = $self-> { pressed} ? 3 : 2;
	} else {
		$imNo = $self-> { pressed} ? 1 : 0;
	};
	my $xStart;
	$image = $images[ $imNo];
	my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);

	if ( $image) {
		$canvas-> put_image( 0, ( $h - $image-> height) / 2, $image);
		$xStart = $image-> width;
	} else {
		my $s = $::application->uiScaling;
		$xStart = $s * 16;
		push ( @c3d, shift @c3d)
			if $self-> { pressed};
		$canvas-> rect3d( 1, ( $h - $s*14) / 2, $s*15, ( $h + $s*14) / 2, 1,
			@c3d, $clr[ 1]);
		if ( $self-> { checked}) {
			my $at = $self-> { pressed} ? 1 : 0;
			$canvas-> color( cl::Black);
			$canvas-> lineWidth( 2);
			my $yStart = ( $h - $s*14) / 2;
			$canvas-> line(
				$at + $s*4, $yStart - $at +  $s*8,
				$at + $s*5 , $yStart - $at + $s*3
			);
			$canvas-> line(
				$at + $s*5 , $yStart - $at + $s*3,
				$at + $s*12, $yStart - $at + $s*12
			);
			$canvas-> lineWidth( 0);
		}
	}
	return $xStart;
}

sub paint_flat_background
{
	my ( undef, $path, $height ) = @_;
	my $s = $::application->uiScaling;
	$path-> round_rect(
		1,       ( $height - $s*14) / 2,
		$s * 15, ( $height + $s*14) / 2,
		$s * 7
	);
	return $s * 15;
}

sub paint_flat_checkmark
{
	my ( undef, $path, $height ) = @_;
	my $s = $::application->uiScaling;
	my $yStart = ( $height - $s*14) / 2;
	$path-> line(
		$s*4,  $yStart + $s*8,
		$s*5 , $yStart + $s*3,
		$s*5 , $yStart + $s*3,
		$s*12, $yStart + $s*12
	);
}

sub paint_flat
{
	my ( $self, $canvas, $w, $h, @clr) = @_;

	my $s = $::application->uiScaling;
	my $xStart = $s * 16;

	$canvas-> graphic_context( sub {
		$canvas-> antialias(1);
		$canvas-> lineWidth( 1 );
		$canvas-> color($clr[0]);
		my $path1 = $canvas->new_path;
		$self-> paint_flat_background( $path1, $h);
		$path1->stroke;

		if ( $self-> { checked} || $self->{ pressed } ) {
			my $at = $self-> { pressed} ? 1 : 0;
			$canvas-> color( $clr[0]);
			$canvas-> lineWidth( 2);
			$canvas-> translate( $at, -$at);
			my $path2 = $canvas->new_path;
			$self-> paint_flat_checkmark( $path2, $h);
			$path2->stroke;
		}
	});
	return $xStart;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my @clr;
	if ( $self-> enabled) {
		if ( $self-> focused) {
			@clr = ($self-> hiliteColor, $self-> hiliteBackColor);
		} else {
			@clr = ($self-> color, $self-> backColor);
		}
		$clr[1] = $self-> fader_current_color($clr[1]);
	} else {
		@clr = ($self-> disabledColor, $self-> disabledBackColor);
	}

	my @size = $canvas-> size;
	unless ( $self-> transparent) {
		$canvas-> color( $clr[ 1]);
		$canvas-> bar( 0, 0, @size);
	}

	my $xStart;
	if ( $self->skin eq 'flat') {
		$xStart = $self->paint_flat( $canvas, @size, @clr);
	} else {
		$xStart = $self->paint_classic( $canvas, @size, @clr);
	}

	$canvas-> color( $clr[ 0]);
	my ( $fw, $fh) = $self-> caption_box( $canvas);
	$self-> draw_caption( $canvas, $xStart * 1.5, ( $size[1] - $fh) / 2 );

}

package Prima::Radio;
use vars qw(@ISA @images);
@ISA = qw(Prima::Cluster);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	@$def{qw(widgetClass)} = (wc::Radio, undef);
	return $def;
}

sub paint_classic
{
	my ( $self, $canvas, $w, $h, @clr) = @_;
	my ( $image, $imNo);
	if ( $self-> { checked}) {
		$imNo = $self-> { pressed} ? 7 : 6;
	} else {
		$imNo = $self-> { pressed} ? 5 : 4;
	};

	my $xStart;
	$image = $images[ $imNo];
	if ( $image) {
		$canvas-> put_image( 0, ( $h - $image-> height) / 2, $image);
		$xStart = $image-> width;
	} else {
		my $s = $::application->uiScaling;
		$xStart = $s * 16;
		my $y = ( $h - $s * 16) / 2;
		my @xs = map { $s * $_ } ( 0, 8, 16, 8);
		my @ys = map { $s * $_ } ( 8, 16, 8, 0);
		for ( @ys) {$_+=$y};
		my $i;
		if ( $self-> { pressed}) {
			$canvas-> color( cl::Black);
			for ( $i = -1; $i < 3; $i++) {
				$canvas-> line(
					$xs[$i], $ys[$i],
					$xs[$i + 1], $ys[$i + 1]
				)
			};
		} else {
			my @clr = $self-> {checked} ?
				( $self-> light3DColor, $self-> dark3DColor) :
				( $self-> dark3DColor, $self-> light3DColor);
			$canvas-> color( $clr[1]);
			for ( $i = -1; $i < 1; $i++) {
				$canvas-> line(
					$xs[$i], $ys[$i],
					$xs[$i + 1],$ys[$i + 1]
				)
			};
			$canvas-> color( $clr[0]);
			for ( $i = 1; $i < 3; $i++) {
				$canvas-> line(
					$xs[$i], $ys[$i],
					$xs[$i + 1],$ys[$i + 1]
				)
			};
		}
		if ( $self-> checked) {
			$canvas-> color( cl::Black);
			$canvas-> fillpoly( [ $s*6, $y+$s*8, $s*8, $y+$s*10, $s*10, $y+$s*8, $s*8, $y+$s*6]);
		}
	}
}

sub paint_flat
{
	my ( $self, $canvas, $w, $h, @clr) = @_;

	my $s = $::application->uiScaling;
	my $xStart = $s * 16;

	$canvas-> graphic_context( sub {
		$canvas-> antialias(1);
		$canvas-> lineWidth( 1 );
		$canvas-> color($clr[0]);

		my $center = $h / 2;
		my $x = 1 + $s * 7.5;
		$canvas-> ellipse( $x, $center, $s * 15, $s * 15);

		if ( $self-> { checked} || $self->{ pressed } ) {
			my $at = $self-> {pressed} ? 1 : 0;
			$canvas-> fill_ellipse( $x + $at, $center - $at, $s * 8, $s * 8);
		}
	});
	return $xStart;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my @clr;
	if ( $self-> enabled) {
		if ( $self-> focused) {
			@clr = ($self-> hiliteColor, $self-> hiliteBackColor);
		} else {
			@clr = ($self-> color, $self-> backColor);
		}
		$clr[1] = $self-> fader_current_color($clr[1]);
	} else {
		@clr = ($self-> disabledColor, $self-> disabledBackColor);
	}

	my @size = $canvas-> size;
	unless ( $self-> transparent) {
		$canvas-> color( $clr[ 1]);
		$canvas-> bar( 0, 0, @size);
	}

	my $xStart;
	if ( $self->skin eq 'flat') {
		$xStart = $self->paint_flat( $canvas, @size, @clr);
	} else {
		$xStart = $self->paint_classic( $canvas, @size, @clr);
	}

	$canvas-> color( $clr[ 0]);
	my ( $fw, $fh) = $self-> caption_box( $canvas);
	$self-> draw_caption( $canvas, $xStart * 1.5, ( $size[1] - $fh) / 2 );
}

sub on_click
{
	my $self = $_[0];
	$self-> focus;
	$self-> checked( 1) unless $self-> checked;
}

sub checked
{
	return $_[0]-> {checked} unless $#_;
	my $self = $_[0];
	my $chkOk = $self-> {checked};

	my $old = $self-> {checked} + 0;
	$self-> {checked} = $_[1] + 0;
	if ( $old != $_[1] + 0) {
		$self-> repaint;
		$chkOk = ( $self-> {checked} != $chkOk) && $self-> {checked};
		my $owner = $self-> owner;
		$owner-> notify( 'RadioClick', $self)
			if $chkOk && exists $owner-> notification_types-> {RadioClick};
		$self-> notify( 'Check', $self-> {checked});
	}
}


package Prima::SpeedButton;
use vars qw(@ISA);
@ISA = qw(Prima::Button);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my $s = $::application->uiScaling;
	@$def{qw(selectable width height text)} = (0, $s*36, $s*36, "");
	return $def;
}

sub calc_geom_size
{
	my @sz = $_[0]-> std_calc_geom_size;
	my $s = $::application->uiScaling;
	$sz[0] = $s*36 if $sz[0] < $s*36;
	$sz[1] = $s*36 if $sz[1] < $s*36;
	return @sz;
}

package Prima::GroupBox;
use vars qw(@ISA);
@ISA=qw(Prima::Widget);

{
my %RNT = (
	%{Prima::Cluster-> notification_types()},
	RadioClick => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		border             => 1,
		ownerBackColor     => 1,
		autoEnableChildren => 1,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {border}  = $profile{border};
	return %profile;
}

sub border
{
	return $_[0]->{border} unless $#_;
	$_[0]->{border} = $_[1];
	$_[0]->repaint;
}

sub on_radioclick
{
	my ($me,$rd) = @_;
	for ($me-> widgets) {
		next if "$rd" eq "$_";
		next unless $_-> isa(q(Prima::Radio));
		$_-> checked(0);
	}
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size   = $canvas-> size;
	my @clr    = $self-> enabled ?
		( $self-> color, $self-> backColor) :
		( $self-> disabledColor, $self-> disabledBackColor);
	unless ( $self-> transparent) {
		$canvas-> color( $clr[1]);
		$canvas-> bar( 0, 0, @size);
	}
	my $fh = $canvas-> font-> height;
	if ( $self->{border}) {
		if ( $self->skin eq 'flat') {
			$canvas-> color( $self-> dark3DColor);
			$canvas-> rectangle( 0, 0, $size[0] - 1, $size[1] - $fh / 2 - 1);
		} else {
			$canvas-> color( $self-> light3DColor);
			$canvas-> rectangle( 1, 0, $size[0] - 1, $size[1] - $fh / 2 - 2);
			$canvas-> color( $self-> dark3DColor);
			$canvas-> rectangle( 0, 1, $size[0] - 2, $size[1] - $fh / 2 - 1);
		}
	}
	my $c = $self->text;
	if ( length( $c) > 0) {
		$c = $self-> text_shape($c, skip_if_simple => 1) || $c;
		$canvas-> color( $clr[1]);
		$canvas-> bar  (
			8, $size[1] - $fh - 1,
			16 + $canvas-> get_text_width( $c), $size[1] - 1
		);
		$canvas-> color( $clr[0]);
		$canvas-> text_out( $c, 12, $size[1] - $fh - 1);
	}
}

sub index
{
	my $self = $_[0];
	my @c    = grep { $_-> isa(q(Prima::Radio))} $self-> widgets;
	if ( $#_) {
		my $i = $_[1];
		$i = 0 if $i < 0;
		$i = $#c if $i > $#c;
		$c[$i]-> check if $c[$i];
	} else {
		my $i;
		for ( $i = 0; $i < scalar @c; $i++) {
			return $i if $c[$i]-> checked;
		}
		return -1;
	}
}

sub text
{
	return $_[0]-> SUPER::text unless $#_;
	$_[0]-> SUPER::text($_[1]);
	$_[0]-> repaint;
}

sub value
{
	my $self = $_[0];
	my @c    = grep { $_-> isa(q(Prima::CheckBox))} $self-> widgets;
	my $i;
	if ( $#_) {
		my $value = $_[1];
		for ( $i = 0; $i < scalar @c; $i++) {
			$c[$i]-> checked( $value & ( 1 << $i));
		}
	} else {
		my $value = 0;
		for ( $i = 0; $i < scalar @c; $i++) {
			$value |= 1 << $i if $c[$i]-> checked;
		}
		return $value;
	}
}

package Prima::RadioGroup;    use vars qw(@ISA); @ISA=qw(Prima::GroupBox);
package Prima::CheckBoxGroup; use vars qw(@ISA); @ISA=qw(Prima::GroupBox);

1;

=pod

=head1 NAME

Prima::Buttons - button widgets and grouping widgets.

=head1 SYNOPSIS

	use Prima qw(Application Buttons StdBitmap);

	my $window = Prima::MainWindow-> create;
	Prima::Button-> new(
		owner => $window,
		text  => 'Simple button',
		pack  => {},
	);
	$window-> insert( 'Prima::SpeedButton' ,
		pack => {},
		image => Prima::StdBitmap::icon(0),
	);

	run Prima;

=for podview <img src="buttons.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/buttons.gif">

=head1 DESCRIPTION

Prima::Buttons provides two separate sets of classes:
the button widgets and the grouping widgets. The button widgets
include push buttons, check-boxes and radio buttons.
The grouping widgets are designed for usage as containers for the
check-boxes and radio buttons, however, any widget can be inserted
in a grouping widget.

The module provides the following classes:

	*Prima::AbstractButton ( derived from Prima::Widget and Prima::MouseScroller )
		Prima::Button
			Prima::SpeedButton
		*Prima::Cluster
			Prima::CheckBox
			Prima::Radio
	Prima::GroupBox ( derived from Prima::Widget )
		Prima::RadioGroup       ( obsolete )
		Prima::CheckBoxGroup    ( obsolete )

Note: C<*> - marked classes are abstract.

=head1 USAGE

	use Prima::Buttons;

	my $button = $widget-> insert( 'Prima::Button',
		text => 'Push button',
		onClick => sub { print "hey!\n" },
	);
	$button-> flat(1);

	my $group = $widget-> insert( 'Prima::GroupBox',
		onRadioClick => sub { print $_[1]-> text, "\n"; }
	);
	$group-> insert( 'Prima::Radio', text => 'Selection 1');
	$group-> insert( 'Prima::Radio', text => 'Selection 2', pressed => 1);
	$group-> index(0);

=head1 Prima::AbstractButton

Prima::AbstractButton realizes common functionality of buttons.
It provides reaction on mouse and keyboard events, and calls
L<Click> notification when the user activates the button. The
mouse activation is performed either by mouse double click or
successive mouse down and mouse up events within the button
boundaries. The keyboard activation is performed on the following conditions:

=over

=item *

The spacebar key is pressed

=item *

C<{default}> ( see L<default> property ) boolean variable is
set and enter key is pressed. This condition holds even if the button is out of focus.

=item *

C<{accel}> character variable is assigned and the corresponding character key
is pressed. C<{accel}> variable is extracted automatically from the text string
passed to L<text> property.
This condition holds even if the button is out of focus.

=back

=head2 Events

=over

=item Check

Abstract callback event.

=item Click

Called whenever the user presses the button.

=back

=head2 Properties

=over

=item hotKey CHAR

A key that the button will react to if pressed, even when out of focus.

=item pressed BOOLEAN

Represents the state of button widget, whether it is pressed or not.

Default value: 0

=item text STRING

The text that is drawn in the button. If STRING contains ~ ( tilde ) character,
the following character is treated as a hot key, and the character is
underlined. If the user presses the corresponding character key then
L<Click> event is called. This is true even when the button is out of focus.

=back

=head2 Methods

=over

=item draw_veil CANVAS, X1, Y1, X2, Y2

Draws a rectangular veil shape over CANVAS in given boundaries.
This is the default method of drawing the button in the disabled state.

=item draw_caption CANVAS, X, Y

Draws single line of text, stored in L<text> property on CANVAS at X, Y
coordinates. Performs underlining of eventual tilde-escaped character, and
draws the text with dimmed colors if the button is disabled. If the button
is focused, draws a dotted line around the text.

=item caption_box [ CANVAS = self ]

Calculates geometrical extensions of text string, stored in L<text> property in pixels.
Returns two integers, the width and the height of the string for the font selected on CANVAS.
If CANVAS is undefined, the widget itself is used as a graphic device.

=back

=head1 Prima::Button

A push button class, that extends Prima::AbstractButton functionality by allowing
an image to be drawn together with the text.

=head2 Properties

=over

=item autoHeight BOOLEAN

If 1, the button height is automatically changed as text extensions
change.

Default value: 1

=item autoRepeat BOOLEAN

If set, the button behaves like a keyboard button - after the first
L<Click> event, a timeout is set, after which is expired and the button
still pressed, L<Click> event is repeatedly called until the button is
released. Useful for emulating the marginal scroll-bar buttons.

Default value: 0

=item autoShaping BOOLEAN

If 1, the button C<shape> is autmatically update when the button size and/or
font are updated, if the current skin can make use of non-rectangular shapes.
Generally is unneeded unless the owner of the button has a different back color
or features some custom painting.

Default value: 0

See also: F<examples/triangle.pl>, F<examples/dragdrop.pl>

=item autoWidth BOOLEAN

If 1, the button width is automatically changed as text extensions
change.

Default value: 1

=item borderWidth INTEGER

Width of 3d-shade border around the button.

Default value: 2

=item checkable BOOLEAN

Selects if the button toggles L<checked> state when the user
presses it.

Default value: 0

=item checked BOOLEAN

Selects whether the button is checked or not. Only actual
when L<checkable> property is set. See also L<holdGlyph>.

Default value: 0

=item default BOOLEAN

Defines if the button should react when the user presses the enter button.
If set, the button is drawn with the black border, indicating that it executes
the 'default' action. Useful for OK-buttons in dialogs.

Default value: 0

=item defaultGlyph INTEGER | IMAGE | METAFILE

Selects index of the default sub-image.

Default value: 0

=item disabledGlyph INTEGER | IMAGE | METAFILE

Selects index of the sub-image for the disabled button state.
If C<image> does not contain such sub-image, the C<defaultGlyph>
sub-image is drawn, and is dimmed over with L<draw_veil> method.

Default value: 1

=item flat BOOLEAN

Selects special 'flat' mode, when a button is painted without
a border when the mouse pointer is outside the button boundaries.
This mode is useful for the toolbar buttons. See also L<hiliteGlyph>.

Default value: 0

=item glyphs INTEGER

If a button is to be drawn with the image, it can be passed in the L<image>
property. If, however, the button must be drawn with several different images,
there are no several image-holding properties. Instead, the L<image> object
can be logically split vertically into several equal sub-images. This allows
the button resource to contain all button states into one image file.
The C<glyphs> property assigns how many such sub-images the image object contains.

The sub-image indices can be assigned for rendition of the different states.
These indices are selected by the following integer properties: L<defaultGlyph>,
L<hiliteGlyph>, L<disabledGlyph>, L<pressedGlyph>, L<holdGlyph>.

Default value: 1

=item hilite

Read-only property, return 1 if the button is highlighted, 0 otherwise.

=item hiliteGlyph INTEGER | IMAGE | METAFILER

Selects index of the sub-image for the state when the mouse pointer is
over the button. This image is used only when L<flat> property is set.
If C<image> does not contain such sub-image, the C<defaultGlyph> sub-image is drawn.

Default value: 0

=item holdGlyph INTEGE | IMAGE | METAFILER

Selects index of the sub-image for the state when the button is L<checked>.
This image is used only when L<checkable> property is set.
If C<image> does not contain such sub-image, the C<defaultGlyph> sub-image is drawn.

Default value: 3

=item image OBJECT

If set, the image object is drawn next with the button text, over or left to it
( see L<vertical> property ). If OBJECT contains several sub-images, then the
corresponding sub-image is drawn for each button state. See L<glyphs> property.

Can also be a C<Prima::Drawable::Metafile> object, however, C<imageScale> factor
wouldn't work on it.

Default value: undef

=item imageFile FILENAME

Alternative to image selection by loading an image from the file.
During the creation state, if set together with L<image> property, is superseded
by the latter.

To allow easy multiframe image access, FILENAME string is checked if it contains
a number after a colon in the string end. Such, C<imageFile('image.gif:3')> call
would load the fourth frame in C<image.gif> file.

=item imageScale SCALE

Contains zoom factor for the L<image>.

Default value: 1

=item modalResult INTEGER

Contains a custom integer value, preferably one of C<mb::XXX> constants.
If a button with non-zero C<modalResult> is owned by a currently executing
modal window, and is pressed, its C<modalResult> value is copied to the C<modalResult>
property of the owner window, and the latter is closed.
This scheme is helpful for the dialog design:

	$dialog-> insert( 'Prima::Button', modalResult => mb::OK,
		text => '~Ok', default => 1);
	$dialog-> insert( 'Prima::Button', modalResult => mb::Cancel,
		text => 'Cancel);
	return if $dialog-> execute != mb::OK.

The toolkit defines the following constants for C<modalResult> use:

	mb::OK or mb::Ok
	mb::Cancel
	mb::Yes
	mb::No
	mb::Abort
	mb::Retry
	mb::Ignore
	mb::Help

However, any other integer value can be safely used.

Default value: 0

=item smoothScaling BOOL

Tries to represent the image as smooth as possible. When the system doesn't support ARGB layering,
icon objects smooth scaling will be restricted to integer-scaling only (i.e. 2x, 3x etc) because
smooth color plane will not match pixelated mask plane, and because box-scaling
with non-integer zooms looks ugly.

Default value: true

See also: L<Prima::Image/ui_scale> .

=item pressedGlyph INTEGER | IMAGE | METAFILE

Selects index of the sub-image for the pressed state of the button.
If C<image> does not contain such sub-image, the C<defaultGlyph> sub-image is drawn.

=item transparent BOOLEAN

See L<Prima::Widget/transparent>. If set, the background is not painted.

=item vertical BOOLEAN

Determines the position of image next to the text string. If 1,
the image is drawn above the text; left to the text if 0.
In a special case when L<text> is an empty string, image is centered.

=back

=head1 Prima::SpeedButton

A convenience class, same as L<Prima::Button> but with default
square shape and text property set to an empty string.

=head1 Prima::Cluster

An abstract class with common functionality of L<Prima::CheckBox> and
L<Prima::RadioButton>. Reassigns default actions on tab and back-tab keys, so
the sibling cluster widgets are not selected. Has C<ownerBackColor> property
set to 1, to prevent usage of background color from C<wc::Button> palette.

=head2 Properties

=over

=item auto BOOLEAN

If set, the button is automatically checked when the button is in focus. This
functionality allows arrow key walking by the radio buttons without pressing
spacebar key. It is also has a drawback, that if a radio button gets focused
without user intervention, or indirectly, it also gets checked, so that behavior
might cause confusion. The said can be exemplified when an unchecked radio button
in a notebook widget gets active by turning the notebook page.

Although this property is present on the L<Prima::CheckBox>, it is not used in there.

=back

=head2 Methods

=over

=item check

Alias to C<checked(1)>

=item uncheck

Alias to C<checked(0)>

=item toggle

Reverts the C<checked> state of the button and returns the new state.

=back

=head1 Prima::Radio

Represents a standard radio button, that can be either in checked, or in unchecked state.
When checked, delivers L<RadioClick> event to the owner ( if the latter provides one ).

The button uses the standard toolkit images with C<sbmp::RadioXXX> indices.
If the images can not be loaded, the button is drawn with the graphic primitives.

=head2 Events

=over

=item Check

Called when a button is checked.

=back

=head1 Prima::CheckBox

Represents a standard check box button, that can be either in checked, or in unchecked state.

The button uses the standard toolkit images with C<sbmp::CheckBoxXXX> indices.
If the images can not be loaded, the button is drawn with graphic primitives.

=head1 Prima::GroupBox

The class to be used as a container of radio and check-box buttons.
It can, however, contain any other widgets.

The widget draws a 3d-shaded box on its boundaries and a text string in its
upper left corner. Uses C<transparent> property to determine if it needs to
paint its background.

The class does not provide a method to calculate the extension of the inner rectangle.
However, it can be safely assumed that all offsets except the upper are 5 pixels.
The upper offset is dependent on a font, and constitutes the half of the font height.

=head2 Events

=over

=item RadioClick BUTTON

Called whenever one of children radio buttons is checked. BUTTON
parameter contains the newly checked button.

The default action of the class is that all checked buttons,
except BUTTON, are unchecked. Since the flow type of C<RadioClick> event
is C<nt::PrivateFirst>, C<on_radioclick> method must be directly overloaded
to disable this functionality.

=back

=head2 Properties

=over

=item border BOOLEAN

If set (default), draws a border along the widget boundaries

=item index INTEGER

Checks the child radio button with C<index>. The indexing is
based on the index in the widget list, returned by C<Prima::Widget::widgets> method.

=item value BITFIELD

BITFIELD is an unsigned integer, where each bit corresponds to the
C<checked> state of a child check-box button. The indexing is
based on the index in the widget list, returned by C<Prima::Widget::widgets> method.

=back

=head1 Prima::RadioGroup

This class is obsolete and is same as C<Prima::GroupBox>.

=head1 Prima::CheckBoxGroup

This class is obsolete and is same as C<Prima::GroupBox>.

=head1 BUGS

The push button is not capable of drawing anything other than single line of text and
single image. If an extended functionality is needed, instead of fully rewriting
the painting procedure, it might be reasonable to overload C<put_image_indirect>
method of C<Prima::Button>, and perform custom output there.

Tilde escaping in C<text> is not realized, but is planned to. There currently is no way
to avoid tilde underscoring.

Radio buttons can get unexpectedly checked when used in notebooks. See L<auto>.

C<Prima::GroupBox::value> parameter is an integer, which size is architecture-dependent.
Shift towards a vector is considered a good idea.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::Window>, 
L<Prima::Drawable::Metafile>,
L<Prima::StdBitmap>, F<examples/buttons.pl>, F<examples/buttons2.pl>.

=cut
