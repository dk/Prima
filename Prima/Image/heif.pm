package Prima::Image::heif;
use strict;
use warnings;
use Prima;
use Prima::VB::VBLoader;
use Prima::Drawable::Markup qw(M);

use constant VISUAL_KEY   => 0;
use constant VISUAL_VALUE => 1;
use constant PARAMETER    => 2;
use constant FULLKEY      => 3;
use constant KEY          => 4;

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	my $e = $self-> {data}  = \ %{ $image->{extras} // {} };

	my $nb = $self->TabbedNotebook1->Notebook;
	if ( exists $e->{compression} ) {
		$nb->Encoder->text($e->{compression});
		$nb->Encoder->notify(q(Change));
	}
	if ( exists $e->{quality} ) {
		$nb->Quality->text($e->{compression});
		$nb->Quality->notify(q(Change));
	}
	my $tree = $self->{refs};
	my $props = $nb->Properties;
	while ( my ( $k, $v ) = each %{ $self->{data} }) {
		next unless my $node = $tree->{$k};
		$node->[VISUAL_KEY]   = M("B<$node->[KEY]>");
		$node->[VISUAL_VALUE] = M("B<$v>");
	}
	$props-> autowidths;
	$props-> repaint;
}

sub OK_Click
{
	my $self = $_[0]-> owner;
	my $e = $self-> {image}-> {extras} //= {};
	%$e = (%$e, %{$self->{data}});
}

sub Cancel_Click
{
	my $self = $_[0]-> owner;
	delete $self-> {image};
}

sub Quality_Change
{
	my ( $dialog, $value ) = @_;
	$dialog->{data}->{quality} = $value;
	while ( my ($k, $v) = each %{ $dialog->{refs} } ) {
		next unless $v->[KEY] eq 'quality';
		$v->[VISUAL_KEY]   = $v->[KEY];
		$v->[VISUAL_VALUE] = $v->[PARAMETER]->{default} // '';
		delete $dialog->{data}->{$v->[FULLKEY]};
	}
}

sub Properties_SelectItem
{
	my ($dialog, $self, $index) = @_;

	my %vpages = (
		bool => 0,
		int  => 1,
		str  => 2,
		none => 3,
	);

	my ($node, $lev) = $self-> get_item( $index->[0][0]);
	next unless $node and $node = $node->[0];
	my $values = $self->owner->Values;

	if ( my $param = $node->[PARAMETER] ) {
		goto NONE unless exists $vpages{$param->{type} // ''};
		my $k = $node->[FULLKEY];
		my $i = $dialog->{data};
		my $v = exists($i->{$k}) ? $i->{$k} : $param->{default};
		my $w = $values->bring("V\u$param->{type}") or goto NONE;

		local $dialog->{lock_change} = 1;
		if ( $param->{type} eq 'bool') {
			$w->checked($v);
		} elsif ( $param->{type} eq 'int') {
			$w->showHint(0);
			if ( exists $param->{min} or exists $param->{max}) {
				my $hint = '';
				if ( exists $param->{min}) {
					$w->min($param->{min});
					if ( exists $param->{max}) {
						$w->max($param->{max});
						$hint = "Value between $param->{min} and $param->{max}";
					} else {
						$hint = "Value larger than $param->{min}";
					}
				} else {
					$hint = "Value less than $param->{max}";
					$w->max($param->{max});
				}
				$w->hint($hint);
				$w->showHint(1);
			}
			$w->value($v // 0);
		} else {
			$w->items($param->{values} // []);
			$w->text($v // '');
		}
		$values->pageIndex($vpages{$param->{type}});
	} else {
	NONE:
		$values->pageIndex($vpages{none});
	}
}

sub Properties_Click
{
	my ($dialog, $self) = @_;
	my ($node, $lev) = $self-> get_item( $self-> focusedItem );
	next unless $node && $node->[0] && $node->[0]->[PARAMETER];
	local $dialog->{lock_change} = 1;
	$node = $node->[0];
	my $e = $dialog->{data};
	my $k = $node->[FULLKEY];
	if ( exists $e->{$k} || !exists $node->[PARAMETER]->{default} ) {
		delete $e->{$k};
		my $v = $e->{$k} // $node->[PARAMETER]->{default};
		$node->[VISUAL_KEY]   = $node->[KEY];
		$node->[VISUAL_VALUE] = $v;
	} else {
		$e->{$k} = $node->[PARAMETER]->{default};
		$node->[VISUAL_KEY]   = M("B<$node->[KEY]>");
		$node->[VISUAL_VALUE] = M("B<$e->{$k}>");
	}
	$self->autowidths;
	$self->repaint;
}

sub V_Change
{
	my ($dialog, $value) = @_;
	return if $dialog->{lock_change};
	my $props = $dialog->TabbedNotebook1->Notebook->Properties;
	my ($node, $lev) = $props-> get_item( $props-> focusedItem );
	next unless $node && $node->[0] && $node->[0]->[PARAMETER];
	$node = $node->[0];
	my $e = $dialog->{data};
	my $k = $node->[FULLKEY];
	$node->[VISUAL_KEY]   = M("B<$node->[KEY]>")
		unless ref $node->[VISUAL_KEY];
	$node->[VISUAL_VALUE] = M("B<$value>");
	$e->{$k} = $value;
	$props-> autowidths;
	$props-> repaint;
}

sub save_dialog
{
	my $codec = $_[1];
	my @encoders;
	my $curr_encoder = 0;
	my %keys;
	for ( @{ $codec->{featuresSupported} // [] } ) {
		next unless m[^encoder\s+(\w+)/(\w+)[^\(]*\((.*)\)$];
		push @encoders, {
			compression => $1,
			text        => "$1/$2",
			key         => $2,
			description => $3,
		};
		$curr_encoder = $#encoders if $1 eq $codec->{saveInput}->{encoder};
		$keys{$2}++;
	}
	return unless @encoders;

	my @storage;
	my %data;
	for my $key ( sort keys %keys ) {
		my @subs;
		push @storage, [[ $key, '' ], \@subs ];
		while ( my ($k, $v) = each %{ $codec->{saveInput} }) {
			next unless $k =~ m/^(\w+)\.(\w+)$/ and (ref($v) // '') eq 'HASH' and $1 eq $key;
			$data{$k} = [ $2, $v->{default} // '', $v, $k, $2 ];
			push @subs, [$data{$k}];
		}
	}

	my $dialog;
	$dialog = Prima::VBLoad( 'Prima::Image::heif.fm',
		Form1 => {
			visible => 0,
			onChange  => \&on_change,
		},
		OK      => { onClick => \&OK_Click },
		Cancel  => { onClick => \&Cancel_Click },
		Encoder => {
			items       => [ map { $_->{text} } @encoders ],
			focusedItem => $curr_encoder,
			onChange    => sub {
				my $self = shift;
				my $v =  $encoders[ $self-> focusedItem ] // {};
				$self-> owner-> Description-> text( $v->{description} // '' );
				$dialog->{data}->{compression} = $v->{compression};
			},
		},
		Properties => {
			items        => \@storage,
			onSelectItem => sub { Properties_SelectItem($dialog, @_) },
			onClick      => sub { Properties_Click($dialog, @_) },
		},
		Description => { text => $encoders[$curr_encoder]->{description} },
		Quality     => { onChange => sub { Quality_Change($dialog, shift->value) }},
		VBool       => { onCheck  => sub { V_Change( $dialog, shift->checked ) }},
		VInt        => { onChange => sub { V_Change( $dialog, shift->value   ) }},
		VStr        => { onChange => sub { V_Change( $dialog, shift->text    ) }},
	);
	$dialog->{refs} = \%data;
	$dialog->{data} = {};

	Prima::message($@) unless $dialog;
	return $dialog;
}

=pod
sub test
{
	use Data::Dumper;
	my $image = Prima::Image->new;
	$image->{extras} = {
		'x265.quality' => 30,
		compression    => 'AV1',
	};
	my ($codec) = grep { $_->{name} eq 'libheif' } @{ Prima::Image->codecs };
	die "no heif codec" unless $codec;
	my $dlg = shift->save_dialog($codec);
	$dlg->notify(q(Change), $codec, $image);
	return if $dlg->execute != mb::OK;
	print Dumper($image->{extras});
}
=cut

1;

=pod

=head1 NAME

Prima::Image::heif

=head1 DESCRIPTION

HEIF image save dialog

=head1 INSTALLATION

libheif so far could be built from the sources, and its dependencies as well,
as major distros are not yet providing its binaries. For windows you can grab
the distro from
L<http://prima.eu.org/download/libheif-1.12.0-win64.zip>.

=over

=item *

You will need gcc,nasm,cmake

=item *

Get the sources for libaom,libx265,libde265. Build and install them.

=item *

Get the sources for libheif. Build, make sure that I<configure --disable-multithreading> says this:

   configure: Multithreading: no
   configure: libaom decoder: yes
   configure: libaom encoder: yes
   configure: libde265 decoder: yes
   configure: libx265 encoder: yes

=back
