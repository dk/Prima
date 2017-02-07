package Prima::Drawable::Path;

use strict;
use warnings;

sub new
{
    my ( $class, $canvas, @opt ) = @_;
    return bless {
        canvas => $canvas,
        points => [],
    }, $class;
}

sub make_global
{
    my $self = shift;
    my $p = $self->{points};
    push @$p, 0, 0 unless @$p;
    my @ret;
    for ( my $i = 0; $i < @_ ; $i += 2 ) {
        push @ret, $_[$i]   + $p->[0];
        push @ret, $_[$i+1] + $p->[1];
    }
    return @ret;
}

sub line
{
    my $self = shift;
    my $p = $#_ ? \@_ : $_[0];
    push @{ $self->{points} }, @$p;
    return $self;
}

sub line_to
{
    my $self = shift;
    my $p = $#_ ? \@_ : $_[0];
    push @{ $self->{points} }, $self-> make_global( @$p );
    return $self;
}

sub spline
{
    my $self = shift;
    my $p = $#_ ? \@_ : $_[0];
    push @{ $self->{points} }, @{ $self-> {canvas}-> render_spline( $p ) };
    return $self;
}

sub spline_to
{
    my $self = shift;
    my $p = $#_ ? \@_ : $_[0];
    $p = $self-> {canvas}-> render_spline( $p );
    push @{ $self->{points} }, $self-> make_global( @$p );
    return $self;
}

sub points { shift->{points} }
sub stroke { $_[0]->{canvas}->polyline( $_[0]->{points} ) }
sub fill   { $_[0]->{canvas}->fillpoly( $_[0]->{points} ) }

1;
