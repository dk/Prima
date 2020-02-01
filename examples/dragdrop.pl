use strict;
use warnings;
use FindBin qw($Bin);
use Prima qw(Application StdBitmap Buttons);

my $w = Prima::MainWindow->new(
	size => [240, 240],
	text => 'Drag and drop',
);


sub set_dnd_mode
{
	my $self = shift;
	my $dv = 0;
	$dv |= &{$dnd::{$_}}() for grep { $self->bring($_)->checked } qw(Copy Move Link);
	if ( $dv == 0 && $self->name =~ /Drag/ ) {
		$self->Copy->checked(1);
		$dv = dnd::Copy;
	}
	$self->{dnd_mode} = $dv;
}

sub best_dnd_mode
{
	my ( $mod, $actions ) = @_;
	return $actions if grep { $_ == $actions } (dnd::Copy, dnd::Move, dnd::Link);
	my $preferred =
		($mod & km::Ctrl) ? dnd::Move :
		(($mod & km::Shift) ? dnd::Link : dnd::Copy);
	return $preferred if $preferred & $actions;
	return dnd::Copy if $actions & dnd::Copy;
	return dnd::Move if $actions & dnd::Move;
	return dnd::Link if $actions & dnd::Link;
	return dnd::None;
}

sub add_buttons
{
	my $self = shift;
	my $fh = $self->font->height;
	my $d  = $fh + 4;
	my $h = $self->height - $fh - 5 - 4;
	$self->{dnd_mode} = dnd::Copy;
	$self->insert([ SpeedButton =>
		rect => [ 5, $h, 5 + $d, $h + $d ],
		name => 'Copy',
		text => 'C',
		hint => 'Copy',
		checkable => 1,
		checked   => 1,
		onClick => sub { set_dnd_mode( $self ) },
	], [ SpeedButton => 
		rect => [ 5 + $d, $h, 5 + $d * 2, $h + $d ],
		name => 'Move',
		text => 'M',
		hint => 'Move',
		checkable => 1,
		onClick => sub { set_dnd_mode( $self ) },
	], [ SpeedButton => 
		rect => [ 5 + $d * 2, $h, 5 + $d * 3, $h + $d ],
		name => 'Link',
		text => 'L',
		hint => 'Link',
		checkable => 1,
		onClick => sub { set_dnd_mode( $self ) },
	]
	);
}

sub display_action
{
	my $action = shift;
	if ( $action < 0 ) {
		$action = 'DND failed';
	} elsif ( $action == 0 ) {
		$action = 'No action',
	} elsif ( $action == dnd::Copy ) {
		$action = 'Data copied',
	} elsif ( $action == dnd::Move ) {
		$action = 'Data moved',
	} elsif ( $action == dnd::Link ) {
		$action = 'Data linked',
	} else {
		$action = "Unknown status ($action)";
	}
	$w-> text($action);
}

$w->insert( Widget => 
	origin => [10, 10],
	size   => [100,100],
	name   => 'DropText',
	text   => "Drop\nText",
	dndAware => 1,
	onPaint => sub {
		my $self = shift;
		$self->clear;
		$self->rectangle(0,0,$self->width-1,$self->height-1);
		$self->draw_text($self->text,0,0,$self->size,dt::NewLineBreak|dt::Center|dt::VCenter);
	},
	onDragBegin => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y) = @_;
		$self->{dropable} = $clipboard->format_exists('Text') ? 1 : 0;
		$self->set(
			color     => ($self->{dropable} ? cl::HiliteText : cl::DisabledText),
			backColor => ($self->{dropable} ? cl::Hilite : cl::Disabled),
		);
		$self->{modmap} = 0;
	},
	onDragOver => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y, $ref) = @_;
		$ref->{allow} = $self->{dropable} && $self->{dnd_mode} != dnd::None;
		$self->{modmap} = $modmap;
		$ref->{action} = best_dnd_mode($modmap, $self->{dnd_mode} & $action);
		$ref->{pad} = [ 0, 0, $self-> size ]; # don't send it anymore
	},
	onDragEnd => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y, $ref) = @_;
		$self->set(
			color     => cl::Fore,
			backColor => cl::Back,
		);
		return unless $clipboard;
		$ref->{allow} = 0;
		if ($clipboard->format_exists('Text')) {
			$ref->{allow} = 1;
			$ref->{action} = best_dnd_mode($self->{modmap}, $self->{dnd_mode} & $action); 
			$self->text($clipboard->text);
			$self->repaint;
		}
	},
);

$w->insert( Widget => 
	origin => [130, 10],
	size   => [100,100],
	dndAware => 1,
	name   => 'DropImage',
	onPaint => sub {
		my $self = shift;
		$self->clear;
		$self->put_image_indirect($self->{image},
			1, 1, 0, 0,
			$self->width-2,$self->height-2,
			$self->{image}->size,
			$self->{drag} ? rop::NotPut : rop::CopyPut
		) if $self->{image};
		$self->rectangle(0,0,$self->width-1,$self->height-1);
		$self->draw_text("Drop\nImage",0,0,$self->size,dt::NewLineBreak|dt::Center|dt::VCenter);
	},
	onDragBegin => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y) = @_;
		$self->{drag} = 1;
		$self->{dropable} = $clipboard->format_exists('Image') ? 1 : 0;
		$self->set(
			color     => ($self->{dropable} ? cl::HiliteText : cl::DisabledText),
			backColor => ($self->{dropable} ? cl::Hilite : cl::Disabled),
		);
	},
	onDragOver => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y, $ref) = @_;
		$ref->{allow} = $self->{dropable} && $self->{dnd_mode} != dnd::None;
		$self->{modmap} = $modmap;
		$ref->{action} = best_dnd_mode($modmap, $self->{dnd_mode}); 
		$ref->{pad} = [ 0, 0, $self-> size ]; # don't send it anymore
	},
	onDragEnd => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y, $ref) = @_;
		$self->{drag} = 0;
		$self->set(
			color     => cl::Fore,
			backColor => cl::Back,
		);
		return unless $clipboard;
		$ref->{allow} = 0;
		if ( $self->{image} = $clipboard->image) {
			$ref->{allow} = 1;
			$ref->{action} = best_dnd_mode($self->{modmap}, $self->{dnd_mode}); 
			$self->repaint;
		}
	},
);

$w->insert( Widget => 
	origin => [10, 130],
	size   => [100,100],
	name   => 'DragText',
	onPaint => sub {
		my $self = shift;
		$self->clear;
		$self->rectangle(0,0,$self->width-1,$self->height-1);
		$self->draw_text("Drag\nText",0,0,$self->size,dt::NewLineBreak|dt::Center|dt::VCenter);
	},
	onMouseDown => sub {
		my $self = shift;
		return if $self->{grab};
		$self->{grab}++;
		my $action = $self->begin_drag(
			text    => $self->text,
			actions => $self->{dnd_mode},
		);
		display_action($action);
		$self->{grab}--;
	},
);

$w->insert( Widget => 
	origin => [130, 130],
	size   => [100,100],
	name   => 'DragImage',
	enabled => 0,
	onCreate => sub {
		my $self = shift;
		$self->{image} = Prima::Image->load("$Bin/Hand.gif") or return;
		$self->enabled(1);
	},
	onPaint => sub {
		my $self = shift;
		$self->clear;
		$self->put_image_indirect($self->{image},
			1, 1, 0, 0,
			$self->width-2,$self->height-2,
			$self->{image}->size,
			rop::CopyPut
		) if $self->{image};
		$self->rectangle(0,0,$self->width-1,$self->height-1);
		$self->draw_text("Drag\nImage",0,0,$self->size,dt::NewLineBreak|dt::Center|dt::VCenter);
	},
	onMouseDown => sub {
		my $self = shift;
		return if $self->{grab} or not $self->{image};
		$self->{grab}++;
		my $action = $self->begin_drag(
			image   => $self->{image},
			actions => $self->{dnd_mode},
		);
		display_action($action);
		$self->{grab}--;
	},
);

add_buttons($w->bring($_)) for qw(DropText DragText DropImage DragImage);

run Prima;
