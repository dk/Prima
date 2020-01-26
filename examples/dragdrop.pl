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
		$self->backColor(cl::Hilite);
	},
	onDragEnd => sub {
		my ( $self, $clipboard, $ref) = @_;
		$self->backColor(cl::Back);
		return unless $clipboard;
		$self->text($clipboard->text) if $clipboard->format_exists('Text');
		$self->repaint;
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
		$self->backColor(cl::Hilite);
		$self->{drag} = 1;
	},
	onDragEnd => sub {
		my ( $self, $clipboard, $ref) = @_;
		$self->{drag} = 0;
		$self->backColor(cl::Back);
		return unless $clipboard;
		$self->{image} = $clipboard->image;
		$self->repaint;
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
