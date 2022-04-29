use strict;
use warnings;
use Prima;
use Prima::VB::VBLoader;

package Prima::Image::tiff;

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	$self-> {image}-> {extras} ||= {};
	my $e = $image-> {extras};
	for ( @{$self-> TextSelector-> items}) {
		if ( exists $e->{$_}) {
			$self-> {TextFields}->{$_} = $e->{$_};
		} else {
			$self-> {TextFields}->{$_} = $codec-> {saveInput}-> {$_};
		}
	}
	for ( qw( Compression ResolutionUnit XResolution YResolution)) {
		$self-> bring($_)-> text( exists $e-> {$_} ?  $e-> {$_} : $codec-> {saveInput}->{$_});
	}
	$self-> TextField-> text( $self-> {TextFields}-> {$self-> TextSelector-> text});
}

sub OK_Click
{
	my $self = $_[0]-> owner;
	$self-> {image}-> {extras} ||= {};
	my $e = $self-> {image}-> {extras};
	$self-> {TextFields}->{$self-> TextSelector-> text} = $self-> TextField-> text;
	for ( @{$self-> TextSelector-> items}) {
		if ( exists($self-> {TextFields}->{$_}) && length($self-> {TextFields}->{$_})) {
			$e-> {$_} = $self-> {TextFields}->{$_};
		} else {
			delete $e->{$_};
		}
	}
	$e-> {Compression} = $self-> Compression-> text;
	$e-> {ResolutionUnit} = $self-> ResolutionUnit-> text;
	if ( $e-> {ResolutionUnit} eq 'none') {
		delete $e-> {XResolution};
		delete $e-> {YResolution};
		delete $e-> {ResolutionUnit};
	} else {
		$e-> {XResolution} = $self-> XResolution-> value;
		$e-> {YResolution} = $self-> YResolution-> value;
	}
	delete $self-> {image};
}

sub Cancel_Click
{
	my $self = $_[0]-> owner;
	delete $self-> {image};
}

sub save_dialog
{
	my $codec = $_[1];
	my $dialog = Prima::VBLoad( 'Prima::Image::tiff.fm',
		Form1 => {
			visible => 0,
			onChange  => \&on_change,
		},
		Compression => { items => [ qw(NONE),
			map { m/^Compression-(.*)$/ ? $1 : () } @{$codec-> {featuresSupported}} ]},
		OK      => { onClick => \&OK_Click },
		Cancel  => { onClick => \&Cancel_Click },
	);
	Prima::message($@) unless $dialog;
	return $dialog;
}

1;
