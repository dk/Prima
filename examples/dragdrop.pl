use strict;
use warnings;
use FindBin qw($Bin);
use Prima qw(Application StdBitmap);

my $w = Prima::MainWindow->new(
	size => [240, 240],
	text => 'Drag and drop',
);

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
		my ($self, $clipboard) = @_;
		$self->{dropable} = $clipboard->format_exists('Text') ? 1 : 0;
		$self->set(
			color     => ($self->{dropable} ? cl::HiliteText : cl::DisabledText),
			backColor => ($self->{dropable} ? cl::Hilite : cl::Disabled),
		);
	},
	onDragOver => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y, $ref) = @_;
		$ref->{allow} = $self->{dropable};
		$ref->{pad} = [ 0, 0, $self-> size ]; # don't send it anymore
	},
	onDragEnd => sub {
		my ( $self, $clipboard, $ref) = @_;
		$self->set(
			color     => cl::Fore,
			backColor => cl::Back,
		);
		return unless $clipboard;
		$ref->{allow} = 0;
		if ($clipboard->format_exists('Text')) {
			$ref->{allow} = 1;
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
			$self->{drag} ? rop::NotPut : rop::CopyPut # XXX
		) if $self->{image};
		$self->rectangle(0,0,$self->width-1,$self->height-1);
		$self->draw_text("Drop\nImage",0,0,$self->size,dt::NewLineBreak|dt::Center|dt::VCenter);
	},
	onDragBegin => sub {
		my ($self, $clipboard) = @_;
		$self->{drag} = 1;
		$self->{dropable} = $clipboard->format_exists('Image') ? 1 : 0;
		$self->set(
			color     => ($self->{dropable} ? cl::HiliteText : cl::DisabledText),
			backColor => ($self->{dropable} ? cl::Hilite : cl::Disabled),
		);
	},
	onDragOver => sub {
		my ( $self, $clipboard, $action, $modmap, $x, $y, $ref) = @_;
		$ref->{allow} = $self->{dropable};
		$ref->{pad} = [ 0, 0, $self-> size ]; # don't send it anymore
	},
	onDragEnd => sub {
		my ( $self, $clipboard, $ref) = @_;
		$self->{drag} = 0;
		$self->set(
			color     => cl::Fore,
			backColor => cl::Back,
		);
		return unless $clipboard;
		$ref->{allow} = 0;
		if ( $self->{image} = $clipboard->image) {
			$ref->{allow} = 1;
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
		my $c = $::application->get_dnd_clipboard;
		$c->text($self->text);
		$self->begin_drag;
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
		my $c = $::application->get_dnd_clipboard;
		$c->image($self->{image});
		$self->begin_drag;
		$self->{grab}--;
	},
);

run Prima;
