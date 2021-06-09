# combo styles
package
    cs;
use constant Simple       =>  0;
use constant DropDown     =>  1;
use constant DropDownList =>  2;

use strict;
use warnings;

package Prima::ComboBox;

use vars qw(@ISA %listProps %editProps %listDynas $capture_mode);
use Prima qw( InputLine Lists Utils StdBitmap);
@ISA = qw(Prima::Widget);

use constant DefButtonX => 17;

# these are properties, methods and dynas of encapsulated widgets, edit and list

%editProps = map { $_ => 1 } qw(
	alignment      autoScroll  text         select_all
	charOffset     maxLen      insertMode   firstChar
	selection      selStart    selEnd       writeOnly
	copy           cut         delete       paste
	wordDelimiters readOnly    passwordChar focus
);

%listProps = map { $_ => 1 } qw(
	focusedItem hScroll
	integralHeight items       itemHeight
	topItem        vScroll     gridColor
	multiColumn    offset      autoHScroll
	autoVScroll
);

%listDynas = map { $_ => 1} qw(onDrawItem onSelectItem);

for ( keys %editProps) {
	eval <<GENPROC;
sub $_ { return shift-> {edit}-> $_(\@_); }
sub Prima::ComboBox::DummyEdit::$_ {}
GENPROC
}

for (keys %listProps) {
	eval <<GENPROC;
sub $_ { return shift-> {list}-> $_(\@_); }
sub Prima::ComboBox::DummyList::$_ {}
GENPROC
}

$capture_mode = (Prima::Application-> get_system_info-> {apc} == apc::Unix);

sub profile_default
{
	my $f = $_[ 0]-> get_default_font;
	return {
		%{Prima::InputLine-> profile_default},
		%{Prima::ListBox-> profile_default},
		%{$_[ 0]-> SUPER::profile_default},
		style          => cs::Simple,
		caseSensitive  => 0,
		autoHeight     => 1,

		autoHScroll    => 0,
		autoVScroll    => 1,
		listVisible    => 0,
		editHeight     => $f-> {height} + 2,
		listHeight     => $::application-> uiScaling * 100,
		ownerBackColor => 1,
		selectable     => 0,
		literal        => 1,
		scaleChildren  => 0,
		autoEnableChildren => 1,

		editClass      => 'Prima::InputLine',
		listClass      => 'Prima::ListBox',
		buttonClass    => 'Prima::Widget',
		scrollBarClass => 'Prima::ScrollBar',
		editProfile    => {},
		listProfile    => {},
		buttonProfile  => {},
		hScrollBarProfile => {},
		vScrollBarProfile => {},
		listDelegations   => [qw(Leave SelectItem MouseUp Click KeyDown)],
		editDelegations   => [qw(FontChanged Create Setup KeyDown KeyUp Change Leave MouseWheel)],
		buttonDelegations => [qw(ColorChanged FontChanged MouseDown MouseClick
			MouseUp MouseMove MouseEnter MouseLeave Paint Enable Disable)],
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	my $style = exists $p-> {style} ? $p-> {style} : $default-> {style};
	$p-> {autoHeight} = 0
		if exists $p-> {height} || exists $p-> {size} || exists $p-> {rect} || ( exists $p-> {top} && exists $p-> {bottom});
	if ( $style != cs::Simple) {
		$p-> { editHeight } = exists $p-> {height} ? $p-> {height} : $default-> {height}
			unless exists $p-> { editHeight };
	} else {
		my $fh = exists $p-> {font}-> {height} ?
			$p-> {font}-> {height} :
			$default-> {font}-> {height};
		$p-> { editHeight } = $fh + 2
			unless exists $p-> {editHeight };
	}
	$p-> {autoHScroll} = 0 if exists $p-> {hScroll};
	$p-> {autoVScroll} = 0 if exists $p-> {vScroll};
}

sub init
{
	my $self = shift;
	my %profile = @_;
	my $visible = $profile{visible};
	$profile{visible} = 0;
	$self-> {edit} = bless [], q\Prima::ComboBox::DummyEdit\;
	$self-> {list} = bless [], q\Prima::ComboBox::DummyList\;
	%profile = $self-> SUPER::init(%profile);
	my ( $w, $h) = ( $self-> size);
	$self-> {style}        = $profile{style};
	$self-> {listVisible}  = $profile{style} != cs::Simple;
	$self-> {caseSensitive}= $profile{caseSensitive};
	$self-> {literal}      = $profile{literal};
	my $eh = $self-> {editHeight } = $profile{editHeight };
	$self-> {listHeight}   = $profile{listHeight};
	$self-> {autoHeight}   = $profile{autoHeight};

	$self-> {edit} = $self-> insert( $profile{editClass} =>
		name        => 'InputLine',
		origin      => [ 0, $h - $eh],
		size        => [ $w - (( $self-> {style} == cs::Simple) ? 0 : ( $::application-> uiScaling * DefButtonX)), $eh],
		growMode    => gm::GrowHiX | gm::GrowLoY,
		selectable  => 1,
		tabStop     => 1,
		current     => 1,
		dndAware    => (( $self->{style} == cs::DropDown) ? 'Text' : 0 ),
		delegations => $profile{editDelegations},
		(map { $_   => $profile{$_}} keys %editProps),
		%{$profile{editProfile}},
	);

	if ( $self->{autoHeight} && $self->{style} != cs::Simple) {
		$self->check_auto_size;
		( $w, $h) = ( $self-> size);
		$eh = $self-> geomHeight;
		$self->{edit}->set( height => $eh, bottom => $h - $eh );
	}

	my %lp = (%listProps, scrollBarClass => 1, hScrollBarProfile => 1, vScrollBarProfile => 1);
	delete $lp{hScroll} if $profile{autoHScroll};
	delete $lp{vScroll} if $profile{autoVScroll};
	$self-> {list} = $self-> insert( $profile{listClass} =>
		name         => 'List',
		origin       => [ 0, 0],
		selectable   => $capture_mode ? ($self->{style} == cs::Simple) : 1,
		width        => $w,
		height       => ( $self-> {style} == cs::Simple) ? ( $h - $eh) : $self-> {listHeight},
		growMode     => gm::Client,
		tabStop      => 0,
		multiSelect  => 0,
		clipOwner    => $self-> {style} == cs::Simple,
		visible      => $self-> {style} == cs::Simple,
		delegations  => $profile{listDelegations},
		(map { $_    => $profile{$_}} grep { exists $profile{$_} ? 1 : 0} keys %listDynas),
		(map { $_    => $profile{$_}} keys %lp),
		%{$profile{listProfile}},
	);

	$self-> {button} = $self-> insert( $profile{buttonClass} =>
		ownerBackColor => 1,
		name           => 'Button',
		origin         => [ $w - $::application-> uiScaling * DefButtonX, $h - $eh],
		size           => [ $::application-> uiScaling * DefButtonX, $eh],
		visible        => $self-> {style} != cs::Simple,
		growMode       => gm::GrowLoX | gm::GrowLoY,
		tabStop        => 0,
		ownerFont      => 0,
		selectable     => 0,
		delegations    => $profile{buttonDelegations},
		%{$profile{buttonProfile}},
	);

	$self-> visible( $visible);
	return %profile;
}

sub check_auto_size
{
	my $self = $_[0];
	$self-> geomHeight( $self-> {edit}-> default_geom_height )
		if $self-> {autoHeight} && $self->{style} != cs::Simple && $self->{edit}->can('default_geom_height');
}

sub on_create
{
	my $self = $_[0];
	$self-> InputLine_Change( $self-> {edit})
		if $self-> {style} == cs::DropDownList;
}

sub on_size { $_[0]-> listVisible(0); }
sub on_move { $_[0]-> listVisible(0); }
sub on_hide { $_[0]-> listVisible(0); }

sub List_Leave
{
	$_[0]-> listVisible( 0) if $_[0]-> {style} != cs::Simple;
}

sub List_SelectItem
{
	return if defined $_[1]-> {interaction};
	$_[0]-> {edit}-> {interaction} = 1;
	$_[0]-> {edit}-> text($_[1]-> get_item_text( $_[1]-> focusedItem));
	$_[0]-> {edit}-> {interaction} = undef;
	$_[0]-> notify( q(Change)) if $_[0]-> {style} == cs::Simple;
}

sub List_MouseUp
{
	return unless $_[2] == mb::Left || $_[1]-> capture;
	$_[0]-> listVisible(0) if $_[0]-> {style} != cs::Simple;
	$_[0]-> notify( q(Change)) if $_[0]-> {style} != cs::Simple;
}

sub List_Click
{
	my ( $self, $list) = @_;
	$self-> {edit}-> {interaction} = 1;
	$self-> {edit}-> text( $list-> get_item_text( $list-> focusedItem));
	$self-> {edit}-> {interaction} = undef;
	$self-> listVisible(0);
	$self-> notify( q(Change));
}

sub List_KeyDown
{
	my ( $self, $list, $code, $key, $mod) = @_;
	if ( $key == kb::Esc) {
		$self-> listVisible(0);
		$list-> clear_event;
	} elsif ( $key == kb::Enter) {
		$list-> notify(q(Click));
		$list-> clear_event;
	}
}

sub Button_ColorChanged
{
	my $self = shift;
	if ( $self-> {style} != cs::Simple) {
		$self-> {list}-> color( $self-> {button}-> color);
		$self-> {list}-> backColor( $self-> {button}-> backColor);
	}
}

sub Button_FontChanged
{
	my $self = shift;
	if ( $self-> {style} != cs::Simple) {
		my $f = $self-> {button}-> font;
		$self-> {list}-> font($f);
	}
}

sub Button_MouseDown
{
	$_[0]-> listVisible( !$_[0]-> {list}-> visible);
	$_[1]-> clear_event;
	return if !$_[0]-> {list}-> visible;
	$_[1]-> capture(1);
}

sub Button_MouseClick
{
	return unless $_[-1];
	$_[0]-> listVisible( !$_[0]-> {list}-> visible);
	$_[1]-> clear_event;
	return if !$_[0]-> {list}-> visible;
	$_[1]-> capture(1);
}


sub Button_MouseMove
{
	$_[1]-> clear_event;
	if ($_[1]-> capture) {
		my ($x,$y,$W,$H) = ($_[3],$_[4],$_[1]-> size);
		return unless ($x < 0) || ($y < 0) || ($x >= $W) || ($y >= $H);
		$_[1]-> capture(0);
		$_[0]-> {list}-> mouse_down( mb::Left, 0, 5, $_[0]-> {list}-> height - 5, 1)
			if ($_[0]-> {list}-> visible);
	}
}

sub Button_MouseEnter
{
	my ( $self, $button ) = @_;
	if ( !$button->capture && $self->enabled) {
		$button->{prelight} = 1;
		$button->repaint;
	}
}

sub Button_MouseLeave
{
	my ( $self, $button ) = @_;
	if ( !$button->capture && $button->{prelight}) {
		delete $button->{prelight};
		$button->repaint;
	}

}

sub Button_MouseUp { $_[1]-> capture(0); }

sub fix_triangle
{
	my @spot = map { int($_ + .5) } @_;
	my $dx = $spot[4] - $spot[0];
	my $dy = $spot[1] - $spot[3];
	if ($dx % 2) {
		$spot[2] = $spot[0] + ($dx - 1) / 2;
		$spot[4]--;
		$dx--;
	}
	if ( $dx == 2 ) {
		$spot[4]++;
		$spot[0]--;
		$dx += 2;
	}
	$spot[3]-- if $dy < $dx / 2;
	return @spot;
}

sub Button_Paint
{
	my ( $owner, $self, $canvas) = @_;
	my ( $w, $h)   = $canvas-> size;
	my $ena    = $self-> enabled;
	my @clr    = $ena ?
		( $self-> color, $self-> backColor) :
		( $self-> disabledColor, $self-> disabledBackColor);
	$clr[1] = $self->prelight_color($clr[1]) if $self->{prelight};
	my $lv = $owner-> listVisible;
	my ( $rc, $lc) = ( $self-> light3DColor, $self-> dark3DColor);
	( $rc, $lc) = ( $lc, $rc) if $lv;
	$self-> rect_bevel(
		$canvas, 0, 0, $w-1, $h-1,
		fill => $self-> new_gradient(
			palette  => [ $self-> dark3DColor, $clr[1], $self-> light3DColor ],
			spline   => [0,0.5,1,0.5],
			vertical => 0,
		),
		width => 2
	);
	my @triangle = fix_triangle( 4, $h * 0.6, $w/2, $h * 0.4, $w - 4, $h * 0.6 );
	if ( $ena) {
		$canvas-> color( $rc);
		$canvas-> translate(1,-1);
		$canvas-> fillpoly(\@triangle);
		$canvas-> translate(0,0);
	}
	$canvas-> color( $clr[0]);
	$canvas-> fillpoly(\@triangle);
}

sub Button_Enable  { $_[1]-> repaint }
sub Button_Disable { $_[1]-> repaint }

sub InputLine_Leave
{
	$_[0]-> listVisible( 0) if $capture_mode and $_[0]-> {style} != cs::Simple;
}

sub InputLine_FontChanged
{
	$_[0]-> editHeight ( $_[1]-> default_geom_height );
	$_[0]-> check_auto_size;
}

sub InputLine_Create
{
	$_[1]-> {incline} = '';
}

sub InputLine_KeyDown
{
	my ( $self, $edit, $code, $key, $mod, $repeat) = @_;

	if ( $self-> listVisible) {
		$self-> {list}-> notify(q(KeyDown), $code, $key, $mod, $repeat);
		$edit-> clear_event;
		return;
	}

	return if $mod & km::DeadKey;
	if (
		( $key & 0xFF00) &&
		( $key != kb::NoKey) &&
		( $key != kb::Space) &&
		( $key != kb::Backspace)
	) {
		return if $key == kb::Tab || $key == kb::BackTab || $key == kb::NoKey;
		$edit-> {incline} = '';
		$self-> listVisible(1), $edit-> clear_event
			if $key == kb::Down && $_[0]-> {style} != cs::Simple;
		$_[0]-> notify( q(Change)), $edit-> clear_event
			if $key == kb::Enter && $_[0]-> {style} == cs::DropDownList;
		return;
	} else {
		return unless $code;
		return unless $_[0]-> {literal};
		return if $_[0]-> {style} != cs::DropDownList;
		return if $mod & ( km::Alt|km::Ctrl);
		$edit-> {keyDown} = 1;
		if ( $key == kb::Backspace) {
			chop $edit-> {incline};
		} else {
			$code = chr ( $code);
			$code = uc $code unless $self-> caseSensitive;
			$edit-> {incline} .= $code;
		}
		my ($ftc,$i,$txt,$t);
		$ftc = quotemeta $edit-> {incline};
		for ( $i = 0; $i < $_[0]-> {list}-> count; $i++) {
			$txt = $_[0]-> {list}-> get_item_text($i);
			$t = $txt;
			$t = uc $t unless $self-> caseSensitive;
			last if $t =~ /^$ftc/;
		}
		if ( $i < $_[0]-> {list}-> count) {
			$edit-> text( $txt);
		} else {
			chop $edit-> {incline};
		}
		$edit-> selection( 0, length $edit-> {incline});
	}
	$edit-> clear_event if $_[0]-> {style} == cs::DropDownList;
}


sub InputLine_KeyUp
{
	$_[1]-> {keyDown} = undef;
}

sub InputLine_Setup
{
	my $self = shift;
	$self-> InputLine_Change(@_) if $self-> {style} == cs::DropDownList;
}

sub InputLine_Change
{
	return if defined $_[0]-> {edit}-> {interaction};
	return unless $_[0]-> {literal};

	$_[0]-> notify(q(Change)) if $_[0]-> {style} != cs::DropDownList;

	$_[0]-> {list}-> {interaction} = 1;
	my ( $self, $style, $list, $edit) = ($_[0], $_[0]-> {style}, $_[0]-> {list}, $_[1]);
	my $i;
	my $found = 0;
	my $cap = $edit-> text;
	$cap = uc $cap unless $self-> caseSensitive;
	my @matchArray;
	my @texts;
	my $maxMatch = 0;
	my $matchId = -1;
	# filling list
	for ( $i = 0; $i < $list-> count; $i++) {
		my $t = $list-> get_item_text($i);
		$t = uc $t unless $self-> caseSensitive;
		push ( @texts, $t);
	}
	# trying to find exact match
	for ( $i = 0; $i < scalar @texts; $i++) {
		if ( $texts[$i] eq $cap) {
			$matchId = $i;
			last;
		}
	}
	if (
		( $style == cs::DropDown) &&
		( $matchId < 0) &&
		defined $edit-> {keyDown}
	) {
		my ($t,$txt);
		for ( $i = 0; $i < scalar @texts; $i++) {
			$txt = $t = $list-> get_item_text($i);
			$t = uc $t unless $self-> caseSensitive;
			last if $t =~ /^\Q$cap\E/;
		}
		# netscape 4 combo behavior
		if ( $i < scalar @texts) {
			$edit-> {interaction} = 1;
			$edit-> text( $txt);
			$edit-> {interaction} = undef;
			$t =~ /^\Q$cap\E/;
			$edit-> selection( length $cap, length $t);
			$list-> focusedItem( $i);
			return;
		}
	}
	# or unexact match
	if ( $matchId < 0) {
		for ( $i = 0; $i < scalar @texts; $i++) {
			my $l = 0;
			$l++ while
				$l < length($texts[$i]) && $l < length($cap)
				&& substr( $texts[$i], $l, 1) eq substr( $cap, $l, 1);
			if ( $l >= $maxMatch) {
				@matchArray = () if $l > $maxMatch;
				$maxMatch = $l;
				push @matchArray, $i;
			}
		}
		$matchId = $matchArray[0] if $matchId < 0;
	}
	$matchId = 0 unless defined $matchId;
	$list-> focusedItem( $matchId);

	if ( $style == cs::DropDownList) {
		$edit-> {interaction} = 1;
		$edit-> text( $list-> get_item_text( $matchId) // '');
		$edit-> {interaction} = undef;
	}
	$list-> {interaction} = undef;
}

sub InputLine_MouseWheel
{
	my ( $self, $edit, $mod, $x, $y, $z) = @_;

	my $f = $self-> {list}-> focusedItem;
	$f += (($z > 0) ? -1 : 1);
	return if $f < 0;
	$self-> {list}-> focusedItem($f);
	$self-> notify(q(Change));
	$edit-> clear_event;
}

sub set_style
{
	my ( $self, $style) = @_;
	return if $self-> {style} == $style;
	my $decr = (( $self-> {style} == cs::Simple) || ( $style == cs::Simple)) ? 1 : 0;
	$self-> {style} = $style;
	if ( $style == cs::Simple) {
		$self-> set(
			height=> $self-> height + $self-> listHeight,
			bottom=> $self-> bottom - $self-> listHeight,
		);
		$self-> {list}-> set(
			visible    => 1,
			origin     => [ 0, 0],
			width      => $self-> width,
			height     => $self-> height - $self-> editHeight ,
			clipOwner  => 1,
			selectable => 1,
		);
	} elsif ( $decr) {
		$self-> set(
			height   => $self-> height - $self-> listHeight,
			bottom   => $self-> bottom + $self-> listHeight,
		);
		$self-> { list}-> set(
			visible    => 0,
			height     => $self-> {listHeight},
			clipOwner  => 0,
			selectable   => $capture_mode ? 0 : 1,
		);
		$self-> listVisible( 0);
	}
	$self-> {edit}-> set(
		bottom => $self-> height - $self-> editHeight ,
		width  => $self-> { edit}-> width + $::application-> uiScaling * DefButtonX * $decr *
			(( $style == cs::Simple) ? 1 : -1),
		height => $self-> editHeight ,
		dndAware => (( $style == cs::DropDown) ? 'Text' : 0 ),
	);
	$self-> {button}-> set(
		bottom => $self-> height - $self-> editHeight ,
		height => $self-> editHeight ,
		visible=> $style != cs::Simple,
	);
	if ( $style == cs::DropDownList) {
		$self-> {edit}-> insertMode(1);
		$self-> {edit}-> text( $self-> {edit}-> text);
	}
}

sub set_list_visible
{
	my ( $self, $nlv) = @_;
	return if ( $self-> {list}-> visible == $nlv) ||
				( $self-> {style} == cs::Simple) ||
				( !$self-> visible && $nlv);
	my ( $list, $edit) = ( $self-> {list}, $self-> {edit});
	if ( $nlv) {
		my @gp = $edit-> client_to_screen( 0, -$list-> height);
		$gp[1] += $edit-> height + $list-> height if $gp[1] < 0;
		$gp[0] = $::application->width - $list->width if $gp[0] + $list->width > $::application->width;
		$list-> origin( @gp);
	}
	$list-> bring_to_front if $nlv;
	$list-> visible( $nlv);
	$self-> {button}-> repaint;
	if ( $capture_mode) {
		$list-> capture( $nlv ? 1 : 0);
		$edit-> focus;
	} else {
		$nlv ? $list-> focus : $edit-> focus;
	}
}

sub set_edit_height
{
	my ( $self, $edit, $list, $btn, $h, $new) =
		($_[0], $_[0]-> {edit}, $_[0]-> {list}, $_[0]-> {button}, $_[0]-> height, $_[1]);
	if ( $self-> style != cs::Simple) {
		$self-> height( $new);
		$edit-> set(
			bottom => 0,
			height => $new
		);
		$btn-> set(
			bottom => 0,
			height => $new
		);
		$list-> height( $self-> {listHeight});
	} else {
		$edit-> set(
			bottom => $h - $new,
			height => $new
		);
		$btn-> set(
			bottom => $h - $new,
			height => $new
		);
		$list-> height( $h - $new);
	}
	$self-> {editHeight} = $new;
}

sub set_list_height
{
	my ( $self, $hei) = @_;
	if ( $self-> style == cs::Simple) {
		$self-> height( $self-> height + $hei - $self-> {listHeight});
	} else {
		$self-> {list}-> height($hei);
	}
	$self-> {listHeight} = $hei;
}

sub get_style       { return $_[0]-> {style}}
sub get_list_visible{ return $_[0]-> {list} ? $_[0]-> {list}-> visible : 0}
sub get_edit_height { return $_[0]-> {edit} ? $_[0]-> {edit}-> height : 0}
sub get_list_height { return  $_[0]-> {list} ? $_[0]-> {list}-> height : 0}

sub caseSensitive{($#_)?$_[0]-> {caseSensitive}=$_[1]:return $_[0]-> {caseSensitive};}
sub listVisible  {($#_)?$_[0]-> set_list_visible($_[1]):return $_[0]-> get_list_visible;}
sub style        {($#_)?$_[0]-> set_style       ($_[1]):return $_[0]-> get_style;       }
sub editHeight   {($#_)?$_[0]-> set_edit_height($_[1]):return $_[0]-> get_edit_height;}
sub listHeight   {($#_)?$_[0]-> set_list_height ($_[1]):return $_[0]-> get_list_height;}
sub literal      {($#_)?$_[0]-> {literal} =      $_[1] :return $_[0]-> {literal}       }

1;

=pod

=head1 NAME

Prima::ComboBox - standard combo box widget

=head1 SYNOPSIS

	use Prima qw(Application ComboBox);

	my $combo = Prima::ComboBox-> new( style => cs::DropDown, items => [ 1 .. 10 ]);
	$combo-> style( cs::DropDownList );
	print $combo-> text;

	run Prima;

=for podview <img src="combo.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/combo.gif">

=head1 DESCRIPTION

Provides a combo box widget which consists of an input line, list box of possible
selections and eventual drop-down button. The combo box can be either in form
with a drop-down selection list, that is shown by the command of the user,
or in form when the selection list is always visible.

The combo box is a grouping widget, and contains neither painting nor user-input
code. All such functionality is delegated into the children widgets: input line, list
box and button. C<Prima::ComboBox> exports a fixed list of methods and properties from
namespaces of L<Prima::InputLine> and L<Prima::ListBox>. Since, however, it is
possible to tweak the C<Prima::ComboBox> ( using its L<editClass> and L<listClass>
create-only properties ) so the input line and list box would be other classes,
it is not necessarily that all default functionality would work.
The list of exported names is stored in package variables %listProps, %editProps
and %listDynas. These also described in L<Exported names> section.

The module defines C<cs::> package for the constants used by L<style> property.

=head1 API

=head2 Properties

=over

=item autoHeight BOOLEAN

If 1, adjusts the height of the widget automatically when its font changes.
Only when style is not C<cs::Simple>.

Default value: 1

=item buttonClass STRING

Assigns a drop-down button class.

Create-only property.

Default value: C<Prima::Widget>

=item buttonDelegations ARRAY

Assigns a drop-down button list of delegated notifications.

Create-only property.

=item buttonProfile HASH

Assigns hash of properties, passed to the drop-down button during the creation.

Create-only property.

=item caseSensitive BOOLEAN

Selects whether the user input is case-sensitive or not, when a value
is picked from the selection list.

Default value: 0

=item editClass STRING

Assigns an input line class.

Create-only property.

Default value: C<Prima::InputLine>

=item editProfile HASH

Assigns hash of properties, passed to the input line during the creation.

Create-only property.

=item editDelegations ARRAY

Assigns an input line list of delegated notifications.

Create-only property.

=item editHeight INTEGER

Selects height of an input line.

=item items ARRAY

Mapped onto the list widget's C<items> property. See L<Prima::Lists> for details.

=item listClass STRING

Assigns a listbox class.

Create-only property.

Default value: C<Prima::ListBox>

=item listHeight INTEGER

Selects height of the listbox widget.

Default value: 100

=item listVisible BOOLEAN

Sets whether the listbox is visible or not. Not writable
when L<style> is C<cs::Simple>.

=item listProfile HASH

Assigns hash of properties, passed to the listbox during the creation.

Create-only property.

=item listDelegations ARRAY

Assigns a selection listbox list of delegated notifications.

Create-only property.

=item literal BOOLEAN

Selects whether the combo box user input routine assume that
the listbox contains literal strings, that can be fetched via
C<get_item_text> ( see L<Prima::Lists> ). As an example when
this property is set to 0 is C<Prima::ColorComboBox> from L<Prima::ComboBox> package.

Default value: 1

=item style INTEGER

Selected one of three styles:

=over

=item cs::Simple

The listbox is always visible, and the drop-down button is not.

=item cs::DropDown

The listbox is not visible, but the drop-down button is. When the
use presses the drop-down button, the listbox is shown; when the list-box
is defocused, it gets hidden.

=item cs::DropDownList

Same as C<cs::DropDown>, but the user is restricted in the selection:
the input line can only accept user input that is contained in listbox.
If L<literal> set to 1, the auto completion feature is provided.

=back

=item text STRING

Mapped onto the edit widget's C<text> property.

=back

=head2 Events

=over

=item Change

Triggered with ComboBox value is changed.

=item List events

ComboBox forwards C<SelectItem> and C<DrawItem> events from the list box, and
these are executed in the List's context (therefore $self there is not
ComboBox, but the ComboBox->List). If you use C<SelectItem> you probably need
C<Change> instead.

See more in L<Prima::Lists>.

=back

=head2 Exported names

=over

=item %editProps

	alignment      autoScroll  text         text
	charOffset     maxLen      insertMode   firstChar
	selection      selStart    selEnd       writeOnly
	copy           cut         delete       paste
	wordDelimiters readOnly    passwordChar focus
	select_all

=item %listProps

		       focusedItem    hScroll
	integralHeight items          itemHeight
	topItem        vScroll        gridColor
	multiColumn    offset

=item %listDynas

	onDrawItem
	onSelectItem

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::InputLine>, L<Prima::Lists>, L<Prima::Dialog::ColorDialog>, L<Prima::Dialog::FileDialog>,
F<examples/listbox.pl>.

=cut
