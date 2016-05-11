use strict;
use warnings;
use Prima;
use Prima::Buttons;
use Prima::Label;
use Prima::Sliders;
use Prima::Edit;

package Prima::Image::jpeg;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);


sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		text   => 'JPEG filter',
		width  => 241,
		height => 192,
		designScale => [ 7, 16],
		centered => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $se = $self-> insert( qq(Prima::SpinEdit) => 
		origin => [ 5, 145],
		name => 'Quality',
		size => [ 74, 20],
		min => 1,
		max => 100,
	);
	$self-> insert( qq(Prima::Label) => 
		origin => [ 5, 169],
		size => [ 131, 20],
		text => '~Quality (1-100)',
		focusLink => $se,
	);
	$self-> insert( qq(Prima::CheckBox) => 
		origin => [ 5, 105],
		name => 'Progressive',
		size => [ 131, 36],
		text => '~Progressive',
	);
	$self-> insert( qq(Prima::Button) => 
		origin => [ 141, 150],
		name => 'OK',
		size => [ 96, 36],
		text => '~OK',
		default => 1,
		modalResult => mb::OK,
		delegations => ['Click'],
	);
	$self-> insert( qq(Prima::Button) => 
		origin => [ 141, 105],
		name => 'Cancel',
		size => [ 96, 36],
		text => 'Cancel',
		modalResult => mb::Cancel,
	);
	my $comm = $self-> insert( qq(Prima::Edit) => 
		origin => [ 5, 5],
		size   => [ 231, 75],
		name   => 'Comment',
		text   => '',
	);
	$self-> insert( qq(Prima::Label) => 
		origin => [ 5, 85],
		size => [ 131, 20],
		text => '~Comment',
		focusLink => $comm,
	);
	return %profile;
}

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	$self-> Quality-> value( exists( $image-> {extras}-> {quality}) ? 
		$image-> {extras}-> {quality} : $codec-> {saveInput}-> {quality});
	$self-> Progressive-> checked( exists( $image-> {extras}-> {progressive}) ? 
		$image-> {extras}-> {progressive} : $codec-> {saveInput}-> {progressive});
	$self-> Comment-> text( exists( $image-> {extras}-> {comment}) ?
		$image-> {extras}-> {comment} : '');
}

sub OK_Click
{
	my $self = $_[0];
	$self-> {image}-> {extras}-> {quality} = $self-> Quality-> value;
	$self-> {image}-> {extras}-> {progressive} = $self-> Progressive-> checked;
	$self-> {image}-> {extras}-> {comment} = $self-> Comment-> text;
	delete $self-> {image};
}

sub save_dialog
{
	return Prima::Image::jpeg-> create;
}
