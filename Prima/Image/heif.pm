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

sub BOLD($) { M("B<< $_[0] >>") }

sub set_node_value
{
	my ( $node, $value ) = @_;
	if ( defined $value ) {
		$node->[VISUAL_KEY]   = BOLD $node->[KEY] unless ref $node->[VISUAL_KEY];
		$node->[VISUAL_VALUE] = BOLD $value;
	} else {
		$node->[VISUAL_KEY]   = $node->[KEY];
		$node->[VISUAL_VALUE] = $node->[PARAMETER]->{default} // '';
	}
}

sub on_change
{
	my ( $self, $codec, $image) = @_;

	$self-> {image} = $image;
	my $e = $self-> {data}  = \ %{ $image->{extras} // {} };

	if ( exists $e->{encoder} ) {
		$self->Encoder->text($e->{encoder});
		$self->Encoder->notify(q(Change));
	}
	if ( exists $e->{quality} ) {
		if ( $e->{quality} eq 'lossless') {
			$self->Lossless->checked;
			$self->Lossless->notify(q(Change));
		} else {
			$self->Quality->value($e->{quality});
			$self->Quality->enabled(1);
			$self->Quality->notify(q(Change))
		}
	}
	$e->{quality} //= 50;
	my $tree = $self->{refs};
	my $props = $self->Properties;

	my %e = %$e;
	while ( my ( $k, $v ) = each %e) {
		next unless my $node = $tree->{$k};
		set_node_value($node, $v);
	}
	$props-> autowidths;
	$props-> repaint;
}

sub OK_Click
{
	my $self = $_[0]-> owner;
	my $e = $self-> {image}-> {extras} //= {};
	%$e = (%$e, %{ $self->{data} });
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
		set_node_value($v);
		delete $dialog->{data}->{$v->[FULLKEY]};
	}
}

sub Lossless_Check
{
	my ( $dialog, $value ) = @_;
	if ( $value ) {
		$dialog->{data}->{quality} = 'lossless';
	} else {
		$dialog->{data}->{quality} = $dialog->Quality->value;
	}
	while ( my ($k, $v) = each %{ $dialog->{refs} } ) {
		next unless $v->[KEY] eq 'lossless' or $v->[KEY] eq 'quality';
		set_node_value($v);
		delete $dialog->{data}->{$v->[FULLKEY]};
	}
	$dialog->Quality->enabled(!$value)
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
		set_node_value($node);
	} else {
		set_node_value($node, $e->{$k} = $node->[PARAMETER]->{default});
	}
	$self->autowidths;
	$self->repaint;
}

sub V_Change
{
	my ($dialog, $value) = @_;
	return if $dialog->{lock_change};
	my $props = $dialog->Properties;
	my ($node, $lev) = $props-> get_item( $props-> focusedItem );
	next unless $node && $node->[0] && $node->[0]->[PARAMETER];
	$node = $node->[0];
	set_node_value($node, $value);
	$dialog->{data}->{$node->[FULLKEY]} = $value;
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
		next unless m[^encoder\s+(\w+)\s\w+[^\(]*\((.*)\)$];
		push @encoders, {
			key         => $1,
			description => $2,
		};
		$curr_encoder = $#encoders if $1 eq $codec->{saveInput}->{encoder};
		$keys{$1}++;
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
			items       => [ map { $_->{key} } @encoders ],
			text        => $encoders[$curr_encoder]->{key},
			onChange    => sub {
				my $self = shift;
				my $v =  $encoders[ $self-> focusedItem ] // {};
				$self-> owner-> Description-> text( $v->{description} // '' );
				$dialog->{data}->{encoder} = $v->{key};
			},
		},
		Properties => {
			items        => \@storage,
			onSelectItem => sub { Properties_SelectItem($dialog, @_) },
			onClick      => sub { Properties_Click($dialog, @_) },
		},
		Description => { text => $encoders[$curr_encoder]->{description} },
		Quality     => { onChange => sub { Quality_Change($dialog, shift->value) }},
		Lossless    => { onCheck  => sub { Lossless_Check($dialog, shift->checked) }},
		VBool       => { onCheck  => sub { V_Change( $dialog, shift->checked ) }},
		VInt        => { onChange => sub { V_Change( $dialog, shift->value   ) }},
		VStr        => { onChange => sub { V_Change( $dialog, shift->text    ) }},
	);
	unless($dialog) {
		Prima::message($@);
		return;
	}
	$dialog->deepChildLookup(1);
	$dialog->Properties->expand_all;
	$dialog->{refs} = \%data;
	$dialog->{data} = {};

	return $dialog;
}

# sub test
# {
# 	use Data::Dumper;
#       use Prima::Application;
# 	my $image = Prima::Image->new;
# 	$image->{extras} = {
# 		'x265.quality' => 30,
# 		encoder    => 'x265',
# 	};
# 	my ($codec) = grep { $_->{name} eq 'libheif' } @{ Prima::Image->codecs };
# 	die "no heif codec" unless $codec;
# 	my $dlg = shift->save_dialog($codec) or die $@;
# 	$dlg->notify(q(Change), $codec, $image);
# 	return if $dlg->execute != mb::OK;
# 	print Dumper($image->{extras});
# }

1;

=pod

=head1 NAME

Prima::Image::heif - standard HEIF image save dialog

=head1 DESCRIPTION

Provides the standard dialog with save options for HEIF images

=head1 INSTALLATION

libheif so far could be built from the sources, and its dependencies as well,
as major distros do not yet provide the binaries. For Windows, you can grab
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
