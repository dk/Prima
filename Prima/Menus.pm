package Prima::Menus;

use strict;
use warnings;
use Prima;

package Prima::Menu::Item;
use base qw(Prima::MenuItem);

sub new
{
	my ( $class, $menu, $id, %opt ) = @_;
	my $self = $class->SUPER::new($menu, $id);
	%$self = ( %opt, %$self );
	return $self;
}

sub check_width   { 10 }
sub y_offset      { 5 }

sub selectable    { 1 }
sub vertical      { $_[0]->{vertical} }

sub new_from_itemid
{
	my ( $self, $menu, $itemid, %xopt ) = @_;

	my $class;
	unless ( defined $itemid ) {
		$class = 'Guillemots';
	} elsif ( $menu-> is_custom($itemid)) {
		$class = 'Custom';
	} elsif ( $menu-> is_separator($itemid)) {
		$class = 'Separator';
	} elsif ( $menu-> image($itemid)) {
		$class = 'Image';
	} else {
		$class = 'Text';
	}
	return "Prima::Menu::Item::$class"->new( $menu, $itemid, %xopt );
}

sub size { 0,0 }
sub draw {}
sub finalize {}

package Prima::Menu::Item::Separator;
use base qw(Prima::Menu::Item);

sub selectable { 0 }
sub size { 0, shift->y_offset * 2 }

sub draw
{
	my ( $self, $canvas, $colors, $x, $y, $w, $h, $selected ) = @_;
	$y += $h / 2 - 1;
	$canvas->color($$colors[ci::Light3DColor]);
	$canvas->line( $x - 1, $y, $x + $w - 1, $y);
	$canvas->color($$colors[ci::Dark3DColor]);
	$canvas->line( $x - 1, $y + 1, $x + $w - 1, $y + 1);
}

sub is_separator { 1 }

package Prima::Menu::Item::Simple;
use base qw(Prima::Menu::Item);

sub is_separator { 0 }

# horizontal: (CHECK=EM) EM/2 TEXT (EM/2 ACCEL)? EM/2
# vertical:    CHECK=EM  EM/2 TEXT (EM/2 ACCEL)? EM/2 DOWN=EM

sub em { $_[0]-> {em_width} }

sub finalize
{
	my ( $self, $storage ) = @_;
	$self-> {icon_width} = $storage->{icon_width};
}

sub size
{
	my ( $self, $canvas, $storage ) = @_;
	my @sz = $self->entry_size($canvas);
	my $fh = $canvas-> font-> height;
	$self->{em_width} = $canvas-> font-> width;

	my $em = $self->em;
	my $icon_width = 0;
	if ( my $icon = $self-> icon ) {
		my @asz = map { $_ / 4 } $::application-> size;
		$icon_width = $icon-> width;
		$icon_width = $asz[0] if $icon_width > $asz[0];

		my $h = $icon-> height;
		$h = $asz[1] if $h > $asz[1];
		$sz[1] = $h if $sz[1] < $h;
	} else {
		$icon_width = $em # space for *
			if $self->vertical || $self->checked || $self->autoToggle;
	}

	$storage->{icon_width} //= 0;
	$storage->{icon_width} = $icon_width if $storage->{icon_width} < $icon_width;

	$sz[0] += $icon_width;
	$sz[0] += $em; # half before, half after

	$sz[1] = $fh if $sz[1] < $fh;
	$sz[1] += $self-> y_offset * 2;

	$self->{accel_width} = length($self->accel) ? 
		$canvas->get_text_width( $self-> accel, 1 ) :
		0;
	$sz[0] += $em * 0.5 + $self->{accel_width}
		if $self->{accel_width} > 0;
	$sz[0] += $em if $self->vertical; 

	return @sz;
}

sub draw_bitmap
{
	my ( $self, $canvas, $colors, $icon, $x, $y, $w, $h) = @_;

	my $key = "$icon";

	my $cache;
	my $disabled = !$self-> enabled;
	unless ( $self->{cache}->{$key}) {
		$cache = $self->{cache}->{$key} = {};
		my $can_argb = $::application-> get_system_value( sv::LayeredWidgets );
		my $is_icon  = $icon->isa('Prima::Icon');

		my @asz = map { $_ / 4 } $::application-> size;
		my @sz  = $icon->size;
		if ( $sz[0] > $asz[0] || $sz[1] > $asz[1] ) {
			$icon = $icon->dup;
			$icon-> stretch(
				( $sz[0] > $asz[0]) ? $asz[0] : $sz[0],
				( $sz[1] > $asz[1]) ? $asz[1] : $sz[1],
			);
		}

		if ($is_icon && (( $icon->type & im::BPP ) == 1 ) && $icon-> maskType == 1 ) {
			$cache->{mono} = 1;
			my ($xor, undef) = $icon->split;
			$xor->type(im::BW);
			$cache->{bitmap} = $xor->bitmap;
		} elsif ( !$icon && !$disabled ) {
			goto FALLBACK;
		} elsif ( $can_argb && (
			$canvas->has_alpha_layer ||
			$disabled ||
			($is_icon && $icon->maskType == 8)
		)) {
			my ($xor, $and);
			if ( $is_icon ) {
				($xor, $and) = $icon->split;
				$xor->type(im::RGB);
				if ( $and->type != im::Byte ) {
					$and->data( ~$and->data);
					$and->type(im::Byte);
				}
				if ( $disabled ) {
					$and->set(
						color => 0,
						rop   => rop::alpha(rop::SrcOver, 0x80),
					);
					$and->bar(0,0,$and->size);
				}
				my $i = Prima::Icon->create_combined($xor, $and);
				$i->premultiply_alpha;
				$cache->{bitmap} = $i->bitmap;
			} else {
				$xor = $icon;
				$xor = $xor->clone(type => im::RGB) if $xor-> type != im::RGB;
				$and = Prima::Image->new(
					size  => [ $icon-> size ],
					type  => im::Byte,
					color => ($disabled ? 0x808080 : 0xffffff),
				);
				$and->bar(0,0,$and->size);
				my $i = Prima::Icon->create_combined($xor, $and);
				$cache->{bitmap} = $i->bitmap;
			}
		} elsif ( $is_icon ) {
			$cache->{bitmap} = $icon->dup;
			$cache->{stipple} = $disabled;
		} else {
			$cache->{stipple} = $disabled;
		FALLBACK:
			$cache->{bitmap} = $icon->dup;
		}
	}
	$cache = $self->{cache}->{$key};
	$icon = $cache->{bitmap};

	my @sz = $icon-> size;
	my @src = ($x + ($w - $sz[0]) / 2, $y + ($h - $sz[1]) / 2);

	if ( $cache->{mono}) {
		if ( $disabled ) {
			$canvas->set( color => cl::Light3DColor, backColor => cl::Clear);
			$canvas->put_image( $src[0] + 1, $src[1] - 1, $icon, rop::OrPut );
		}
		$canvas->set( color => cl::Clear, backColor => cl::Set);
		$canvas->put_image( @src, $icon, rop::AndPut );
		$canvas->set( color => $$colors[$disabled ? ci::DisabledText : ci::NormalText ], backColor => cl::Clear);
		$canvas->put_image( @src, $icon, rop::XorPut );
	} else {
		$canvas->put_image( @src, $icon );
	}

	if ( $cache->{stipple}) {
		$canvas->set(
			color => cl::Set,
			backColor => cl::Clear,
			fillPattern => fp::SimpleDots,
			rop         => rop::AndPut,
		);
		$canvas-> bar( @src, $src[0] + $sz[0], $src[1] + $sz[1]);
		$canvas->set(
			fillPattern => fp::Solid,
			rop         => rop::CopyPut,
		);
	}
}

sub draw
{
	my ( $self, $canvas, $colors, $x, $y, $w, $h, $selected ) = @_;
	my $enabled    = $self->enabled;
	my $vertical   = $self->vertical;
	my $checked    = $self->checked;
	my $autotoggle = $self->autoToggle;
	if ( $selected ) {
		$canvas->backColor( $$colors[$enabled ? ci::Hilite : ci::Disabled ]);
		$canvas->clear( $x, $y, $x + $w - 1, $y + $h - 1 );
	}

	my $em = $self-> em;
	my $y_offset = $self-> y_offset;

	my ($draw_check, $draw_accel, $draw_submenu);

	my $lw = 3;
	if ( my $icon = $self-> icon ) {
		$self-> draw_bitmap( $canvas, $colors, $icon, $x + 2, $y + $y_offset, $self->{icon_width}, $h - $y_offset * 2 );
	} elsif ( $checked ) {
		my $Y  = $y + $h / 2;
		my $D  = $em * 0.25;
		my $X  = $x + $lw / 2 + $D / 2 + ( $self->{icon_width} - $em) / 2;
		$draw_check = [$X, $Y, $X + $D, $Y - $D, $X + $D * 3, $Y + $D];
	} elsif ( $autotoggle ) {
		my $Y  = $y + $h / 2;
		my $D  = $em * 0.5;
		my $X  = $x + $D / 4 + ($self->{icon_width} - $em) / 2;
		my @r = ( $X, $Y - $D, $X + $D * 2, $Y + $D);
		my $c1 = $$colors[$enabled ? ci::Dark3DColor : ci::DisabledText];
		my $c2 = $enabled ? 0x404040 : $$colors[ci::Light3DColor];
		$canvas-> rect3d( @r, 1, $c1, $c2);
		$canvas-> rect3d( $r[0]+1,$r[1]+1,$r[2]-1,$r[3]-1, 1, $$colors[ci::Light3DColor], $c1);
	}

	$x += $self-> {icon_width} + $em / 2;
	$w -= $self-> {icon_width} + $em / 2;

	$y += $y_offset;
	$h -= $y_offset * 2;
	if ( $self->{accel_width} > 0 ) {
		my $X = $x + $w - $self->{accel_width} - $em * ( $vertical ? 1.5 : 0.5 );
		$draw_accel = [ $self->accel, $X, $y ];
		$w -= $self-> {accel_width} + $em * 0.5;
	}

	if ( $vertical && $self-> is_submenu ) {
		my $Y  = $y + $h / 2;
		my $D  = $em / 2;
		my $X  = $x + $w - $D * 1.85;
		$draw_submenu = [$X, $Y - $D, $X + $D, $Y, $X, $Y + $D];
	}
	$w -= $em * ($vertical ? 1.5 : 0.5 );

	if ( $draw_check || $draw_accel || $draw_submenu ) {
		$canvas-> lineEnd(le::Round) if $draw_check;
		unless ( $self-> enabled ) {
			$canvas->color( $$colors[ci::Light3DColor] );
			my @tr = $canvas->translate;
			$canvas->translate($tr[0] + 1, $tr[1] - 1);
			if ($draw_check) {
				$canvas->lineWidth($lw);
				$canvas->polyline($draw_check);
				$canvas->lineWidth(1);
			}
			$canvas->text_out( @$draw_accel ) if $draw_accel;
			$canvas->fillpoly($draw_submenu) if $draw_submenu;
			$canvas->translate(@tr);
			$canvas->color( $$colors[ci::DisabledText] );
		} else {
			$canvas->color( $$colors[$selected ? ci::HiliteText : ci::NormalText] );
		}
		if ($draw_check) {
			$canvas->lineWidth($lw);
			$canvas->polyline($draw_check);
			$canvas->lineWidth(1);
		}
		$canvas->text_out( @$draw_accel ) if $draw_accel;
		$canvas->fillpoly($draw_submenu) if $draw_submenu;
	}

	$self-> draw_entry( $canvas, $colors, $x, $y, $w, $h, $selected);
}

sub entry_size { 0,0 }

package Prima::Menu::Item::Text;
use base qw(Prima::Menu::Item::Simple);

sub entry_size
{
	my ( $self, $canvas ) = @_;
	my $text = $self-> text;
	my $line = $canvas-> text_wrap( $text, -1,
		tw::ReturnFirstLineLength | tw::CalcMnemonic | tw::CollapseTilde, 1);
	return $canvas->get_text_width(substr($text, 0, $line-1), 1), $canvas->font->height;
}

sub draw_entry
{
	my ( $self, $canvas, $colors, $x, $y, $w, $h, $selected ) = @_;

	my $text = $self-> text;
	$y += ( $h - $canvas-> font-> height ) / 2;
	unless ( $self-> enabled ) {
		$canvas->color( $$colors[ci::Light3DColor] );
		$canvas->draw_text( $text,
			$x + 1, $y - 1, $x + $w + 1, $y + $h - 1,
			dt::DrawMnemonic | dt::NoWordWrap | dt::Bottom, 1
		);
		$canvas->color( $$colors[ci::DisabledText] );
	} else {
		$canvas->color( $$colors[$selected ? ci::HiliteText : ci::NormalText] );
	}

	$canvas->draw_text( $text,
		$x, $y, $x + $w, $y + $h,
		dt::DrawMnemonic | dt::NoWordWrap | dt::Bottom, 1
	);
}

package Prima::Menu::Item::Guillemots;
use base qw(Prima::Menu::Item::Text);

sub text       { ">>" }
sub enabled    { 1 }
sub checked    { 0 }
sub autoToggle { 0 }
sub accel      { "" }
sub key        { 0 }
sub data       { undef }
sub image      { undef }
sub submenu    { undef }
sub items      { undef }
sub check      { }
sub uncheck    { }
sub remove     { }
sub toggle     { }
sub enable     { }
sub disable    { }
sub id         { }
sub execute    { }
sub children   { }
sub is_separator { 0 }
sub is_submenu   { 0 }

package Prima::Menu::Item::Image;
use base qw(Prima::Menu::Item::Simple);

sub entry_size
{
	my $self = shift;
	my @sz = $self->image->size;
	my @ds = map { $_ / 4 } $::application->size;
	for ( 0, 1 ) {
		$sz[$_] = $ds[$_] if $sz[$_] > $ds[$_];
	}
	return @sz;
}

sub draw_entry
{
	my ( $self, $canvas, $colors, $x, $y, $w, $h, $selected ) = @_;
	$self-> draw_bitmap( $canvas, $colors, $self-> image, $x, $y, $w, $h );
}

package Prima::Menu::Item::Custom;
use base qw(Prima::Menu::Item);

sub size
{
	my ($self, $canvas) = @_;
	my $cb   = $self->options->{onMeasure} or return;
	my @pt = (0,0);
	$cb->( $canvas-> menu, $self, \@pt);
	return @pt;
}

sub draw
{
	my ( $self, $canvas, $colors, $x, $y, $w, $h, $selected ) = @_;
	my $cb   = $self->options->{onPaint} or return;
	$cb->( $canvas-> menu, $self, $canvas, $selected, $x, $y, $x + $w - 1, $y + $h - 1);
}

package Prima::Menu::Common;

# cached entries
use constant X             => 0;
use constant Y             => 1;
use constant WIDTH         => 2;
use constant HEIGHT        => 3;
use constant ITEMID        => 4;
use constant OBJECT        => 5;

use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub _export_constants
{
	my $pkg = (caller())[0];
	no strict 'refs';
	*{ $pkg . "::$_" } = \&$_ for qw(
		X Y WIDTH HEIGHT ITEMID OBJECT
	);
}

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	Submenu      => nt::Default,
	ItemChanged  => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	return {
		%def,
		borderSize  => 2,
		parent      => undef,
		parentIndex => 0,
	}
}

sub init
{
	my $self = shift;

	$self->{selectedItem} = -1;
	for ( qw( borderSize ))
		{ $self-> {$_} = 1; }
	my %profile = $self-> SUPER::init(@_);

	$self->{lock_reset} = 1;
	for ( qw( borderSize parent parentIndex ))
		{ $self-> $_( $profile{ $_}); }
	delete $self->{lock_reset};

	return %profile;
}

sub borderSize
{
	return $_[0]->{borderSize} unless $#_;
	my ( $self, $borderSize ) = @_;
	$self->{borderSize} = $borderSize;
	$self->reset;
	$self->repaint;
}

sub parent
{
	return $_[0]->{parent} unless $#_;
	my ( $self, $parent ) = @_;
	$self->{parent} = $parent;
	$self->reset;
	$self->repaint;
}

sub parentIndex
{
	return $_[0]->{parentIndex} unless $#_;
	my ( $self, $parentIndex ) = @_;
	$self->{parentIndex} = $parentIndex;
	$self->reset;
	$self->repaint;
}

sub vertical    { $_[0]->{vertical} }

sub get_item_rect
{
	my ( $self, $i ) = @_;
	my $c = $self->{cache}->[$i];
	return @{$c}[X, Y], $$c[X] + $$c[WIDTH], $$c[Y] + $$c[HEIGHT];
}

sub navigate_selection
{
	my ( $self, $desired_item, $direction_if_not_selectable) = @_;
	my $c = $self->{cache};
	my $n = @$c - 1;
	$desired_item = $n + 1 + $desired_item if $desired_item < 0;
	$desired_item = $n if $desired_item < 0;
	$desired_item = 0 if $desired_item > $n;
	while (1) {
		my $object = $c->[$desired_item]->[OBJECT];
		last if $object && $object->selectable && $object->enabled;

		$desired_item += $direction_if_not_selectable;
		if ($desired_item < 0 || $desired_item > $n) {
			$desired_item = 0 if $desired_item > $n;
			$desired_item = $n if $desired_item < 0;
			while (1) {
				my $object = $c->[$desired_item]->[OBJECT];
				last if $object && $object->selectable && $object->enabled;

				$desired_item += $direction_if_not_selectable;
				return if $desired_item < 0 || $desired_item > $n;
			}
		}
	}

	return if $desired_item == $self->selectedItem;
	$self->selectedItem($desired_item);
	return 1;
}

sub selectedItem
{
	return $_[0]->{selectedItem} unless $#_;
	my ( $self, $i) = @_;
	my $n = @{ $self->{cache} } - 1;
	$i = -1 if $i < 0;
	$i = $n if $i > $n;

	return if $i == $self->{selectedItem};
	my $old = $self->{selectedItem};
	$self->{selectedItem} = $i;
	$self->invalidate_rect( $self->get_item_rect($old) ) if $old >= 0;
	$self->invalidate_rect( $self->get_item_rect($i) )   if $i   >= 0;
}

sub enter_submenu
{
	my $self = shift;
	my $i = $self->selectedItem;
	return if $i < 0;

	if ( $self->{submenu} ) {
		return if $i == $self->{submenu_index};
		$self->{submenu}->cancel;
		undef $self->{submenu};
		undef $self->{submenu_index};
	}

	my ($itemid, $index);
	if ( defined ( $itemid = $self->{cache}->[$i]->[ITEMID] )) {
		return unless $self-> menu-> is_submenu($itemid);
		$index = 0;
	} else {
		$itemid = $self-> parent;
		$index  = $self-> parentIndex + @{ $self->{cache} } - 1;
	}

	$self->notify(qw(Submenu), $i);

	$self->{submenu_index} = $i;

	my $submenu = $self->{submenu} = $self->insert( $self->root->popupClass,
		visible        => 0,
		root           => $self->root,
		parent         => $itemid,
		parentIndex    => $index,
		menu           => $self->menu,
		ownerColor     => 1,
		ownerBackColor => 1,
		ownerFont      => 1,
		( map { $_ , $self->$_() } 
			qw(hiliteColor hiliteBackColor disabledColor disabledBackColor)),
	);
	$submenu->origin($self->position_submenu);
	$submenu->show;
	$submenu->bring_to_front;
}

sub execute_selected
{
	my $self = shift;
	my $i = $self->selectedItem;
	return if $i < 0;

	my $cache = $self->{cache}->[$i];
	return $self-> enter_submenu if ! defined $cache->[ITEMID] or $self-> menu-> is_submenu($cache->[ITEMID]);

	$self->root->cancel;
	$cache->[OBJECT]-> execute if $cache->[OBJECT];
}

sub execute_item
{
	my ($self, $itemid) = @_;
	return unless defined $itemid;
	return $self-> enter_submenu if $self-> menu-> is_submenu($itemid);

	$self->root->cancel;
	$self->new_from_itemid( $itemid )->execute;
}

sub xy2item
{
	my ( $self, $x, $y ) = @_;

	my $c = $self->{cache};
	for ( my $i = 0; $i < @$c; $i++) {
		my ($X1,$Y1,$X2,$Y2) = @{ $c->[$i] };
		$X2 += $X1;
		$Y2 += $Y1;
		return $i if $x >= $X1 && $y >= $Y1 && $x <= $X2 && $y < $Y2;
	}

	return -1;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;

	my $submenu = $self;
	my $level = 0;
	while ($submenu->{submenu}) {
		$submenu = $submenu->{submenu};
		$level++;
	}

	if ($key == kb::Esc) {
		$submenu->cancel;
		$self->clear_event;
		return;
	}

	my ($ok, $skip);

	if ( $level == 1 ) {
		if ( $key == kb::Left ) {
			$ok = $self-> navigate_selection( $self->selectedItem-1, -1 );
			$skip = 1;
		} elsif ( $key == kb::Right ) {
			$ok = $self-> navigate_selection( $self->selectedItem+1, 1 );
			$skip = 1;
		} elsif ( $key == kb::Home ) {
			$ok = $self-> navigate_selection( 0, 1 );
			$skip = 1;
		} elsif ( $key == kb::End ) {
			$ok = $self-> navigate_selection( -1, -1 );
			$skip = 1;
		}
		return $self-> enter_submenu if $ok;
	}

	if ( $skip ) {
		#
	} elsif ( $key == kb::Left || $key == kb::Up ) {
		$ok = $submenu-> navigate_selection( $submenu->selectedItem-1, -1 );
		$skip = 1;
	} elsif ( $key == kb::Right || $key == kb::Down ) {
		$ok = $submenu-> navigate_selection( $submenu->selectedItem+1, 1 );
		$skip = 1;
	} elsif ( $key == kb::Home || $key == kb::PgUp ) {
		$ok = $submenu-> navigate_selection( 0, 1 );
		$skip = 1;
	} elsif ( $key == kb::End || $key == kb::PgDn ) {
		$ok = $submenu-> navigate_selection( -1, -1 );
		$skip = 1;
	} elsif ( $key == kb::Enter ) {
		$submenu-> execute_selected;
		$ok = 1;
	}

	my $m = $self->menu;
	if ( defined( my $itemid = $m->find_item_by_key(
		$m->translate_key($code, $key, $mod)
	))) {
		if ( $m->enabled($itemid)) {
			$self-> execute_item($itemid);
			$ok = 1;
		}
	}

	# XXX immediate ~ hotkeys

	$ok = 1 if $skip && $level > 0;

	$self->clear_event if $ok;
}

sub on_fontchanged
{
	my $self = shift;
	$self-> reset;
}

sub on_size
{
	my $self = shift;
	$self-> reset;
}

sub on_itemchanged
{
	my $self = shift;
	$self-> reset;
}

sub on_paint
{
	my ( $self, $canvas ) = @_;
	my @c = map { $self->colorIndex($_) } ci::NormalText .. ci::Dark3DColor;
	@c[ci::NormalText, ci::Normal] = @c[ci::DisabledText, ci::Disabled] unless $self->enabled;
	my @sz = $self->size;
	my $caches = $self->{cache};
	if ( $self->{vertical} ) {
		$self->rect_bevel( $canvas, 0, 0, $sz[0]-1, $sz[1]-1,
			width => $self->{borderSize}, 
			fill => $c[ci::Normal],
		);
	} else {
		$self->clear;
	}

	for ( my $i = 0; $i < @$caches; $i++) {
		my $cache = $caches->[$i];
		next unless $cache->[OBJECT];
		my @o = @{$cache}[X, Y];
		my @s = @{$cache}[WIDTH, HEIGHT];
		$cache->[OBJECT]->draw( $canvas, \@c, @$cache[X,Y,WIDTH,HEIGHT], $i == $self->{selectedItem});
	}
}

sub new_from_itemid
{
	my ( $self, $itemid ) = @_;
	return $self->root->{itemClass}-> new_from_itemid(
		$self-> root->menu, $itemid,
		vertical => $self->vertical,
	);
}

sub new_guillemots
{
	my ( $self ) = @_;
	return $self->root->{itemClass}-> new_from_itemid(
		$self->root->menu, undef,
		vertical => $self->vertical,
	);
}

package Prima::Menu::Transient;
use vars qw(@ISA);
@ISA = qw(Prima::Menu::Common);

BEGIN { Prima::Menu::Common::_export_constants(); };

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	return {
		%def,
		font           => Prima::Widget->get_default_popup_font,
		widgetClass    => wc::Popup,
		clipOwner      => 0,
		selectable     => 0,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p->{font} //= Prima::Widget->get_default_popup_font;
	$self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
	my $self = shift;
	$self->{vertical} = 1;
	my %profile = $self-> SUPER::init(@_);
	$self->root($profile{root});
	$self->reset;
	return %profile;
}

sub root        { $#_ ? $_[0]->{root}        = $_[1] : $_[0]->{root} }
sub menu        { shift->root->menu }

sub cancel
{
	my $self = shift;
	my $o = $self->owner;
	if ( $o && $o->isa('Prima::Menu::Common')) {
		undef $o->{submenu};
		$self->destroy;
	}
}

sub reset
{
	my $self = shift;
	return if $self->{lock_reset};

	my @cache;
	my @size = $self-> size;
	$self-> begin_paint_info;
	my $space_width = $self->get_text_width(' ');
	my $menu = $self-> menu or return;

	my $items = $menu-> get_children( $self-> parent );
	splice( @$items, 0, $self->parentIndex ) if $self->parentIndex > 0;
	my $borderSize = $self->{borderSize};
	my ($x, $y) = (0,0);
	my @ds = $::application->size;
	my $r = $ds[0] - $borderSize * 2;
	my $h = $ds[1] - $borderSize * 2;
	my @max = (0,0);
	my %storage;
	for my $itemid ( @$items ) {
		my $object = $self->new_from_itemid( $itemid );
		my ($W, $H) = $object-> size($self, \%storage);
		if ( $y + $H > $h ) {
			$object = $self->new_guillemots;
			($W, $H) = $object-> size($self);
			$max[0] = $W if $max[0] < $W;
			if ( @cache ) {
				@{$cache[-1]}[WIDTH,HEIGHT,ITEMID,OBJECT] = ($W, $H, undef, $object);
			} else {
				push @cache, [ $x, $y, $W, $H, undef, $object ];
			};
			last;
		}
		push @cache, [ $x, $y, $W, $H, $itemid, $object ];
		$y += $H;
		$max[0] = $W if $max[0] < $W;
	}
	$self-> end_paint_info;

	$self->{cache} = \@cache;
	if ( @cache ) {
		$max[1] = $cache[-1]->[Y] + $cache[-1]->[HEIGHT];
		$max[0] = $r if $max[0] > $r;

		for (@cache) {
			$_->[X] += $borderSize;
			$_->[WIDTH] = $max[0];
			$_->[Y] = $max[1] - $_->[Y] - $_->[HEIGHT] + $borderSize;
			$_->[OBJECT]-> finalize(\%storage);
		}
		$max[$_] += $borderSize * 2 for 0,1;
		if ( $size[0] != $max[0] || $size[1] != $max[1] ) {
			local $self->{lock_reset} = 1;
			$self->size(@max);
		}
	}
}

sub position_submenu
{
	my $self = shift;
	my $sel = $self->selectedItem;
	my @r = $self-> client_to_screen($self->get_item_rect( $sel ));
	my @sz = $self->{submenu}->size;
	my @dp = $::application->size;
	my @pos;

	if ( $r[2] + $sz[0] < $dp[0]) {
		$pos[0] = $r[2];
	} elsif ( $r[0] > $sz[0] ) {
		$pos[0] = $r[0] - $sz[0];
	} else {
		$pos[0] = 0;
	}

	if ( $r[3] > $sz[1] ) {
		$pos[1] = $r[3] - $sz[1];
	} elsif ( $r[1] + $sz[1] < $dp[1] ) {
		$pos[1] = $r[1];
	} else {
		$pos[1] = 0;
	}

	return @pos;
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;

	my $i = $self-> xy2item($x,$y);

	return if $i < 0;
	my $o = $self->{cache}->[$i]->[OBJECT];
	return unless $o && $o->selectable && $o->enabled;

	$self->selectedItem($i);
	$self->execute_selected;
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;

	my $i = $self-> xy2item($x,$y);
	return if $i < 0;
	my $o = $self->{cache}->[$i]->[OBJECT];
	return unless $o && $o->selectable && $o->enabled;

	if ( $i != $self->selectedItem ) {
		$self->selectedItem($i);
		$self->enter_submenu;
	}
}

sub on_mouseleave
{
	my ( $self ) = @_;
	$self->selectedItem(-1);
}

package Prima::Menu::Root;

use Prima::Utils qw(alarm);

sub on_menuchange
{
	my ( $self, $menu, $what, @params) = @_;
	$self->reset;
}

sub set_menu
{
	my ( $self, $menu ) = @_;
	if ($self->{menu}) {
		if ($self->{menu_hook}) {
			$self-> {menu}-> remove_notification( $self-> {menu_hook} );
			undef $self->{menu_hook};
		}
		$self->cancel;
	}
	$self->{menu} = $menu;
	$self->{menu_hook} = $menu-> add_notification( Change => \&on_menuchange, $self )
		if $menu;
	$self->reset;
	$self->repaint;
}

sub on_menuenter
{
	my $self = shift;
	my $owner = $self->owner;

	$self-> {hooks} = [];
	my $cancel = sub { shift-> cancel };
	while ( $owner && $owner != $::application) {
		push @{$self->{hooks}}, $owner, $owner->add_notification( Move => $cancel, $self);
		$owner = $owner->owner;
	}
}

sub on_menuleave
{
	my $self = shift;
	my $hooks = $self->{hooks} // [];
	for ( my $i = 0; $i < @$hooks; $i += 2 ) {
		my ( $owner, $id ) = @{$hooks}[$i,$i+1];
		$owner->remove_notification($id);
	}
	$self->{hooks} = undef;
}

sub on_leave
{
	my $self = shift;

	alarm( 0.2, sub {
		my $f = $::application->get_focused_widget;
		$self->cancel unless $f && $f->isa('Prima::Menu::Common') && $f->root == $self->root;
	});
}

sub on_destroy { undef $_[0]->{hooks} }

sub itemClass   { $#_ ? $_[0]->{itemClass}   = $_[1] : $_[0]->{itemClass}  }
sub popupClass  { $#_ ? $_[0]->{popupClass}  = $_[1] : $_[0]->{popupClass}  }

package Prima::Menu::Popup;
use vars qw(@ISA);
@ISA = qw(Prima::Menu::Transient Prima::Menu::Root);

{
my %RNT = (
	%{Prima::Menu::Common-> notification_types()},
	MenuEnter  => nt::Default,
	MenuLeave  => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	return {
		%def,
		visible        => 0,
		itemClass      => 'Prima::Menu::Item',
		popupClass     => 'Prima::Menu::Transient',
		parent         => "",
		parentIndex    => 0,
		selectable     => 1,
	}
}

sub init
{
	my $self = shift;

	my %profile = $self-> SUPER::init(@_);
	$self->$_($profile{$_}) for qw(itemClass popupClass menu);
	return %profile;
}

sub root { $_[0] }
sub menu { $#_ ? $_[0]->set_menu($_[1]) : $_[0]->{menu} }

sub cancel
{
	my $self = shift;
	my $s = $self->{submenu};
	while ( $s ) {
		my $ss = $s->{submenu};
		$s->destroy;
		$s = $ss;
	}
	$self->{submenu} = undef;
	$self->{selectedItem} = -1;
	$self-> notify(q(MenuLeave));
	$self-> hide;
}

sub popup
{
	my ( $self, $x, $y ) = @_;
	unless ( defined($x) && defined($y)) {
		my @p = $self->owner->pointerPos;
		$x //= $p[0];
		$y //= $p[1];
	}
	my @sz = $self->size;
	my @dp = $::application->size;

	if ( $x + $sz[0] > $dp[0]) {
		if ( $x > $sz[0] ) {
			$x -= $sz[0];
		} else {
			$x = 0;
		}
	}

	if ( $y > $sz[1] ) {
		$y -= $sz[1];
	} elsif ( $y + $sz[1] > $dp[1] ) {
		$y = 0;
	}

	$self-> notify(q(MenuEnter));
	$self-> origin($x, $y);
	$self-> show;
	$self-> bring_to_front;
	$self-> focus;
}

package Prima::Menu::Bar;
use vars qw(@ISA);
@ISA = qw(Prima::Menu::Common Prima::Menu::Root);

BEGIN { Prima::Menu::Common::_export_constants(); };

{
my %RNT = (
	%{Prima::Menu::Common-> notification_types()},
	MenuEnter  => nt::Default,
	MenuLeave  => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	return {
		%def,
		font           => Prima::Window->get_default_menu_font,
		autoHeight     => 1,
		itemClass      => 'Prima::Menu::Item',
		popupClass     => 'Prima::Menu::Transient',
		parent         => "", # root menu item name
		selectable     => 1,
		widgetClass    => wc::Menu,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> {autoHeight} = 0 if !exists $p->{autoHeight} && (
		exists $p-> {height} || exists $p-> {size} || exists $p-> {rect} || ( exists $p-> {top} && exists $p-> {bottom})
	);
	$p->{font} //= Prima::Window->get_default_menu_font;
	$self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
	my $self = shift;

	$self->{autoHeight} = 1;
	$self->{vertical} = 0;
	my %profile = $self-> SUPER::init(@_);
	$self-> {lock_reset} = 1;
	$self->$_($profile{$_}) for qw(autoHeight menu itemClass popupClass);
	delete $self-> {lock_reset};
	$self->reset;

	return %profile;
}

sub root { $_[0] }
sub menu { $#_ ? $_[0]->set_menu($_[1]) : $_[0]->{menu} }

sub autoHeight
{
	return $_[0]->{autoHeight} unless $#_;
	my ( $self, $autoHeight ) = @_;
	$self->{autoHeight} = $autoHeight;
	$self-> geomHeight( $self-> font-> height + 10 + $self-> {borderSize} * 2 )
		if $autoHeight;
}

sub cancel
{
	my $self = shift;
	my $s = $self->{submenu};
	while ( $s ) {
		my $ss = $s->{submenu};
		$s->destroy;
		$s = $ss;
	}
	$self->{submenu} = undef;
	$self->{selectedItem} = -1;
	$self->focused(0);
	$self->repaint;
	$self-> notify(q(MenuLeave));
}


sub reset
{
	my $self = shift;
	return if $self->{lock_reset};

	my @cache;
	my @size = $self-> size;
	$self-> begin_paint_info;
	my $space_width = $self->get_text_width(' ');
	my $menu = $self->menu or return;

	my $items = $menu-> get_children( $self-> parent );
	splice( @$items, 0, $self->parentIndex ) if $self->parentIndex > 0;
	my $right_align;
	my ($x, $y) = ($self->{borderSize}) x 2;
	my $h = $size[1] - $y * 2;
	my $r = $size[0] - $x * 2;
	my %storage;
	for my $itemid ( @$items ) {
		if ( $self-> menu-> is_separator($itemid)) {
			push @cache, $right_align //= [0,0,0,0,undef,undef];
			next;
		}

		my $object = $self->new_from_itemid($itemid);
		my ($W, $H) = $object-> size($self, \%storage);
		if ( $x + $W > $r ) {
			$object = $self->new_guillemots;
			($W, $H) = $object-> size($self);
			undef $right_align;
			if ( @cache && $x + $W > $r) {
				pop @cache while @cache && !defined $cache[-1]->[OBJECT];
			}
			if ( @cache && $x + $W > $r) {
				@{$cache[-1]}[WIDTH,HEIGHT,ITEMID,OBJECT] = ($W, $H, undef, $object);
			} else {
				push @cache, [ $x, $y, $W, $H, undef, $object ];
			}
			last;
		}
		push @cache, [ $x, $y, $W, $H, $itemid, $object ];
		$x += $W;
	}

	if ( $right_align) {
		my $dx = $r - $cache[-1]->[X] - $cache[-1]->[WIDTH];
		my $got_align;
		for my $cache ( @cache ) {
			$got_align = 1 if $cache == $right_align;
			next unless $got_align;
			$cache->[X] += $dx;
		}
	}
	$_->[OBJECT]-> finalize(\%storage) for grep { defined $_->[OBJECT] } @cache;

	$self-> end_paint_info;
	$self->{cache} = \@cache;
}

sub position_submenu
{
	my $self = shift;
	my $sel = $self->selectedItem;
	my @r = $self-> client_to_screen($self->get_item_rect( $sel ));
	my @sz = $self->{submenu}->size;
	my @dp = $::application->size;
	my @pos;

	my $right_aligned;
	for ( my $i = 0; $i < $sel; $i++) {
		next if $self->{cache}->[$i]->[OBJECT];
		$right_aligned = 1;
		last;
	}

	$pos[0] = 0;
	if ( $right_aligned ) {
		if ( $r[2] > $sz[0] ) {
			$pos[0] = $r[2] - $sz[0];
		} elsif ( $r[0] + $sz[0] < $dp[0]) {
			$pos[0] = $r[0];
		}
	} else {
		if ( $r[0] + $sz[0] < $dp[0]) {
			$pos[0] = $r[0];
		} elsif ( $r[2] > $sz[0] ) {
			$pos[0] = $r[2] - $sz[0];
		}
	}

	if ( $r[1] > $sz[1] ) {
		$pos[1] = $r[1] - $sz[1];
	} elsif ( $r[3] + $sz[1] < $dp[1] ) {
		$pos[1] = $r[3];
	} else {
		$pos[1] = 0;
	}

	return @pos;
}

sub selectedItem
{
	return shift->SUPER::selectedItem unless $#_;
	my ( $self, $i ) = @_;

	my $old = $self->{selectedItem};
	$self->SUPER::selectedItem($i);
	if ( $old < 0 && $self->{selectedItem} >= 0 ) {
		$self-> notify(q(MenuEnter));
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;

	my $i = $self-> xy2item($x,$y);

	return if $i < 0;
	my $o = $self->{cache}->[$i]->[OBJECT];
	return unless $o && $o->selectable && $o->enabled;

	$self->selectedItem($i);
	$self->execute_selected;
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;

	return if $self->selectedItem < 0;

	my $i = $self-> xy2item($x,$y);
	return if $i < 0;
	my $o = $self->{cache}->[$i]->[OBJECT];
	return unless $o && $o->selectable && $o->enabled;

	if ( $i != $self->selectedItem ) {
		$self->selectedItem($i);
		$self->enter_submenu;
	}
}

sub on_enter
{
	my $self = shift;
	$self->selectedItem(0) if
		$self->selectedItem < 0 && $self->enabled && @{ $self->{cache} // [] };
	$self->repaint;
}

sub on_disable
{
	my $self = shift;
	$self->cancel;
	$self->repaint;
}

sub on_enable
{
	my $self = shift;
	$self->repaint;
}

sub on_size
{
	my $self = $_[0];
	$self->cancel;
	$self->reset;
}

1;

=pod

=head1 NAME

Prima::Menus - menu widgets

=head1 DESCRIPTION

This module contains classes that can create menu widgets used as
normal widget, without special consideration about system-depended
menus.

=head1 SYNOPSIS

	use Prima qw(Application Menus);
	my $w = Prima::MainWindow->new(
		accelItems => [['~File' => [
			['Exit' => sub { exit } ],
		]]],
		onMouseDown => sub {
			Prima::Menu::Popup->new(menu => $_[0]-> accelTable)->popup;
		},
		height => 100,
	);
	$w->insert( 'Prima::Menu::Bar',
		pack  => { fill => 'x', expand => 1},
		menu  => $w-> accelTable,
	);
	run Prima;

=for podview <img src="Prima/menu.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/menu.gif">

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Menu>, F<examples/menu.pl>

=cut
