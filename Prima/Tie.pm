use strict;
use warnings;

package Prima::Tie::Array;
use Tie::Array;
use vars qw(@ISA);
@ISA = qw(Tie::Array);

sub TIEARRAY {
	my ($class, $widget, $property) = @_;
	bless [ $widget, $property], $class;
}

sub FETCH {
	my ($self, $idx) = @_;
	my ($widget, $property) = @$self;
	$widget-> $property()-> [$idx];
}

sub STORE {
	my ($self, $idx, $value) = @_;
	my ($widget, $property) = @$self;
	my $array = $widget-> $property();
	$array-> [$idx] = $value;
	$widget-> $property($array);
	return $value;
}

sub FETCHSIZE {
	my ($self) = @_;
	my ($widget, $property) = @$self;
	scalar @{$widget-> $property()};
}

sub STORESIZE {
	my ($self, $size) = @_;
	my ($widget, $property) = @$self;
	my $array = $widget-> $property();
	$#$array = $size - 1;
	$widget-> $property($array);
	return $size;
}


sub EXISTS
{
	my ( $self, $idx) = ( $_[0], abs($_[1]));
	return $idx >= 0 && $idx < $self-> FETCHSIZE;
}

sub DELETE
{
}


sub UNTIE {
	my ($obj,$count) = @_;
	die "untie attempted while $count inner references still exist" if $count;
}
#
# Prima::Tie::items - ties an array to a widget's items property
#

package Prima::Tie::items;
use Tie::Array;
use vars qw(@ISA);
@ISA = qw(Tie::Array);

sub TIEARRAY {
	my ($class, $widget) = @_;
	bless {
		self => $widget,
		map {
			$_  => $widget-> can($_),
		} qw(count get_items replace_items insert_items add_items delete_items)
	}, $class;
}

sub FETCH {
	my ($self, $idx) = @_;
	$self-> {self}-> items-> [$idx];
}

sub STORE {
	my ($self, $idx, $value) = @_;
	my $size = $self-> FETCHSIZE;
	if ( $idx == $size) {
		if ( $self-> {add_items}) {
			$self-> {self}-> add_items($value);
		} else {
			my $i = $self-> {self}-> items;
			push @$i, $value;
			$self-> {self}-> items($i);
		}
		return $value;
	} elsif ( $idx > $size) {
		die "Modification of non-creatable array value attempted, subscript $idx";
	} elsif ( $idx < 0) {
		my $ndx = $idx;
		$idx = $size - $idx;
		die "Modification of non-creatable array value attempted, subscript $ndx"
			if $idx < 0;
	}
	if ( $self-> {replace_items}) {
		$self-> {self}-> replace_items($idx, $value);
	} else {
		my $i = $self-> {self}-> items;
		$i-> [$idx] = $value;
		$self-> {self}-> items($i);
	}
	return $value;
}

sub FETCHSIZE {
	my ($self) = @_;
	if ( $self-> {count}) {
		return $self-> {self}-> count;
	} else {
		return scalar @{$self-> {self}-> items};
	}
}

sub STORESIZE {
	my ($self, $new_size) = @_;
	my $old_size =$self-> FETCHSIZE;
	die "Modification of non-creatable array value attempted, size $new_size"
		if $new_size < 0 || $new_size > $old_size;
	if ( $self-> {delete_items}) {
		$self-> {self}-> delete_items( $new_size .. $old_size );
	} else {
		my $i = $self-> {self}-> items;
		$#$i = $new_size - 1;
		$self-> {self}-> items($i);
	}
	return $new_size;
}

sub PUSH {
	my $self = shift;
	if ( $self-> {add_items}) {
		$self-> {self}-> add_items(@_);
	} else {
		my $i = $self-> {self}-> items;
		push @$i, @_;
		$self-> {self}-> items($i);
	}
}

sub SPLICE {
	my $self = shift;
	my $sz  = $self-> FETCHSIZE;
	my $off = (@_) ? shift : 0;
	$off += $sz if ($off < 0);
	die "Modification of non-creatable array value attempted, offset $off"
		if $off < 0;
	my $len = (@_) ? shift : $sz - $off;
	$len += $sz - $off if $len < 0;
	die "Modification of non-creatable array value attempted, length $len"
		if $len < 0;
	my @result;
	my ( $i, $iok);
	if ( $self-> {get_items}) {
		@result = $self-> {self}-> get_items( $off .. $off + $len - 1);
	} else {
		$i = $self-> {self}-> items;
		@result = @$i[ $off .. $off + $len - 1];
		$iok = 1;
	}
	$off = $sz if $off > $sz;
	$len -= $off + $len - $sz if $off + $len > $sz;
	die "Modification of non-creatable array value attempted, length $len"
		if $len < 0;

	if ( @_ == $len) {
		if ( $self-> {replace_items}) {
			$self-> {self}-> replace_items( $off, @_);
		} else {
			$i = $self-> {self}-> items, $iok = 1 unless $iok;
			splice( @$i, $off, $len, @_);
			$self-> {self}-> items( $i);
		}
	} else {
		if ( $self-> {delete_items} && $self-> {insert_items}) {
			$self-> {self}-> delete_items( $off .. $off + $len - 1);
			$self-> {self}-> insert_items( $off, @_);
		} else {
			$i = $self-> {self}-> items, $iok = 1 unless $iok;
			splice( @$i, $off, $len, @_);
			$self-> {self}-> items( $i);
		}
	}
	return wantarray ? @result : pop @result;
}


sub EXISTS
{
	my ( $self, $idx) = ( $_[0], abs($_[1]));
	return $idx >= 0 && $idx < $self-> FETCHSIZE;
}

sub DELETE
{
}


sub UNTIE {
	my ($obj,$count) = @_;
	die "untie attempted while $count inner references still exist" if $count;
}

#
# Prima::Tie::Scalar - ties a scalar to a widget's scalar properties
#

package Prima::Tie::Scalar;

sub TIESCALAR {
	my ($class, $widget, $property) = @_;
	bless [ $widget, $property ], $class;
}

sub FETCH {
	my ($self) = @_;
	my ($widget, $property) = @$self;
	$widget-> $property();
}

sub STORE {
	my ($self, $value) = @_;
	my ( $widget, $property) = @$self;
	$widget-> $property($value);
	return $value;
}

sub UNTIE {
	my ($obj,$count) = @_;
	die "untie attempted while $count inner references still exist" if $count;
}

# some popular names

package Prima::Tie::text;
use vars qw(@ISA);
@ISA = qw(Prima::Tie::Scalar);
sub TIESCALAR { bless [ $_[1], 'text' ], $_[0] }

package Prima::Tie::value;
use vars qw(@ISA);
@ISA = qw(Prima::Tie::Scalar);
sub TIESCALAR { bless [ $_[1], 'value' ], $_[0] }


1;

=pod

=head1 NAME

Prima::Tie - tie widget properties to scalars and arrays

=head1 DESCRIPTION

Prima::Tie contains two abstract classes C<Prima::Tie::Array> and
C<Prima::Tie::Scalar> which tie an array or a scalar to a widget's arbitrary
array or scalar property.  Also, it contains classes C<Prima::Tie::items>,
C<Prima::Tie::text>, and C<Prima::Tie::value>, which tie a variable to a
widget's I<items>, I<text>, and I<value> properties respectively.

=head1 SYNOPSIS

	use Prima::Tie;

	tie @items, 'Prima::Tie::items', $widget;

	tie @some_property, 'Prima::Tie::Array', $widget, 'some_property';

	tie $text, 'Prima::Tie::text', $widget;

	tie $some_property, 'Prima::Tie::Scalar', $widget, 'some_property';

=head1 USAGE

These classes provide immediate access to a widget's array and scalar properties,
in particular to popular properties I<items> and I<text>. It is considerably simpler to say

	splice(@items,3,1,'new item');

than to say

	my @i = @{$widget->items};
	splice(@i,3,1,'new item');
	$widget->items(\@i);

That way, you can work directly with the text or items. Furthermore, if the
only reason you keep an object around after creation is to access its text or
items, you no longer need to do so:

	tie @some_array, 'Prima::Tie::items', Prima::ListBox->new(@args);

As opposed to:

	my $widget = Prima::ListBox->new(@args);
	tie @some_array, 'Prima::Tie::items', $widget;

C<Prima::Tie::items> requires the C<::items> property to be available on the widget.
Also, it takes advantage of additional C<get_items>, C<add_items>, and the like
methods if available.

=head2 Prima::Tie::items

The class is applicable to C<Prima::ListViewer>, C<Prima::ListBox>,
C<Prima::Widget::Header>, and their descendants, and in a limited fashion to
C<Prima::OutlineViewer> and its descendants C<Prima::StringOutline> and
C<Prima::Outline>.

=head2 Prima::Tie::text

The class is applicable to any widget.

=head2 Prima::Tie::value

The class is applicable to C<Prima::GroupBox>, C<Prima::Dialog::ColorDialog>,
C<Prima::SpinEdit>, C<Prima::Gauge>, C<Prima::Slider>, C<Prima::CircularSlider>,
and C<Prima::ScrollBar>.

=head1 COPYRIGHT

Copyright 2004 Teo Sankaro

This program is distributed under the BSD License.
(Although a credit would be nice.)

=head1 AUTHORS

Teo Sankaro, E<lt>teo_sankaro@hotmail.comE<gt>.
Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=cut
