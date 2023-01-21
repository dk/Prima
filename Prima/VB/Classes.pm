use strict;
use warnings;
package Prima::VB::Classes;
use Prima qw(ComboBox);

sub classes
{
	return (
		'Prima::Widget' => {
			RTModule => 'Prima::Classes',
			class  => 'Prima::VB::Widget',
			page   => 'Abstract',
			icon   => 'VB::VB.gif:0',
		},
	);
}



package Prima::VB::Object;
use strict;

my %hooks = ();

sub init_profiler
{
	my ( $self, $prf) = @_;
	$self-> {class}      = $prf-> {class};
	$self-> {realClass}  = $prf-> {realClass} if defined $prf-> {realClass};
	$self-> {module}  = $prf-> {module};
	$self-> {creationOrder} = $prf-> {creationOrder};
	$self-> {creationOrder} = 0 unless defined $self-> {creationOrder};
	$self-> {profile} = {};
	$self-> {extras}  = $prf-> {extras} if $prf-> {extras};
	my %events = $self-> prf_events;
	for ( keys %{$prf-> {class}-> notification_types}) {
		$events{"on$_"} = '' unless exists $events{"on$_"};
	}
	for ( keys %events) {
		$events{$_} = 'my $self = $_[0];' unless length $events{$_};
	}
	$self-> {events}  = \%events;
	$self-> {default} = {%{$prf-> {class}-> profile_default}, %events};
	$self-> prf_adjust_default( $self-> {profile}, $self-> {default});
	$self-> prf_set( name  => $prf-> {profile}-> {name})
		if exists $prf-> {profile}-> {name};
	$self-> prf_set( owner => $prf-> {profile}-> {owner})
		if exists $prf-> {profile}-> {owner};
	delete $prf-> {profile}-> {name};
	delete $prf-> {profile}-> {owner};
	$self-> prf_set( %{$prf-> {profile}});
	my $types = $self-> prf_types;
	my %xt = ();
	for ( keys %{$types}) {
		my ( $props, $type) = ( $types-> {$_}, $_);
		$xt{$_} = $type for @$props;
	}
	$xt{$_} = 'event' for keys %events;
	$self-> {types} = \%xt;
	$self-> {prf_types} = $types;
}

sub add_hooks
{
	my ( $object, @properties) = @_;
	for ( @properties) {
		$hooks{$_} = [] unless $hooks{$_};
		push( @{$hooks{$_}}, $object);
	}
}

sub remove_hooks
{
	my ( $object, @properties) = @_;
	@properties = keys %hooks unless scalar @properties;
	for ( keys %hooks) {
		my $i = scalar @{$hooks{$_}};
		while ( $i--) {
			last if $hooks{$_}-> [$i - 1] == $object;
		}
		next if $i < 0;
		splice( @{$hooks{$_}}, $i, 1);
	}
}

sub prf_set
{
	my ( $self, %profile) = @_;
	my $name = $self-> prf('name');
	for ( keys %profile) {
		my $key = $_;
		next unless $hooks{$key};
		my $o = exists $self-> {profile}-> {$key} ?
			$self-> {profile}-> {$key} : $self-> {default}-> {$key};
		$_-> on_hook( $name, $key, $o, $profile{$key}, $self) for @{$hooks{$key}};
	}
	$self-> {profile} = {%{$self-> {profile}}, %profile};
	my $check = $VB::inspector &&
		( $VB::inspector-> {current}) &&
		( $VB::inspector-> {current} eq $self);
	for ( keys %profile) {
		my $cname = 'prf_'.$_;
		Prima::VB::ObjectInspector::widget_changed(0, $_) if $check;
		$self-> $cname( $profile{$_}) if $self-> can( $cname);
	}
}

sub prf_delete
{
	my ( $self, @dellist) = @_;
	my $df = $self-> {default};
	my $pr = $self-> {profile};
	my $check = $VB::inspector &&
		( $VB::inspector-> {opened}) &&
		( $VB::inspector-> {current} eq $self);
	for ( @dellist) {
		delete $pr-> {$_};
		my $cname = 'prf_'.$_;
		if ( $check) {
			Prima::VB::ObjectInspector::widget_changed(1, $_); # if $check eq $_;
		}
		$self-> $cname( $df-> {$_}) if $self-> can( $cname);
	}
}

sub prf
{
	my ( $self, @property) = @_;
	my @ret = ();
	for ( @property) {
		push ( @ret, exists $self-> {profile}-> {$_} ?
			$self-> {profile}-> {$_} :
			$self-> {default}-> {$_});
		warn( "$self: cannot query `$_'") unless exists $self-> {default}-> {$_};
	}
	return wantarray ? @ret : $ret[0];
}

sub prf_adjust_default
{
}

sub prf_events
{
	return ();
}

sub ext_profile
{
	return;
}

sub act_profile
{
	return;
}

package Prima::VB::Component;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::VB::Object);

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	Load => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		class      => 'Prima::Widget',
		module     => 'Prima::Classes',
		profile    => {},
		selectable => 1,
		sizeable   => 1,
		marked     => 0,
		mainEvent  => undef,
		sizeMin    => [11,11],
		selectingButtons => 0,
		accelItems => [
			['altpopup',0,0, km::Shift|km::Ctrl|kb::F9, sub{
				$_[0]-> altpopup;
				$_[0]-> clear_event;
			}],
		],
		o_delta    => [0,0],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}


sub prf_types
{
	return {
		name       => ['name'],
		Handle     => ['owner'],
		FMAction   => [qw( onBegin onFormCreate onCreate onChild onChildCreate onEnd )],
	};
}

sub prf_events
{
	return (
		onPostMessage  => 'my ( $self, $info1, $info2) = @_;',
		onChangeOwner  => 'my ( $self, $old_owner) = @_;',
		onChildEnter   => 'my ( $self, $child) = @_;',
		onChildLeave   => 'my ( $self, $child) = @_;',
	);
}


sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	$pf-> {owner} = '';
}


sub prf_types_add
{
	my ( $self, $pt, $de) = @_;
	for ( keys %{$de}) {
		# * uncomment this if you suspect property type clash *
		#
		#my $t1 = $_;
		#for ( @{$de-> {$_}}) {
		#  my $p1 = $_;
		#  for ( keys %$pt) {
		#     my $t2 = $_;
		#     for ( @{$pt-> {$_}}) {
		#        die "$self: $p1: $t2 vs $t1\n" if $p1 eq $_ && $t2 ne $t1;
		#     }
		#  }
		#}

		if ( exists $pt-> {$_}) {
			push( @{$pt-> {$_}}, @{$de-> {$_}});
		} else {
			$pt-> {$_} = [@{$de-> {$_}}];
		}
	}
}

sub prf_types_delete
{
	my ( $self, $pt) = ( shift, shift);
	for ( @_) {
		my $lookup = $_;
		for ( keys %$pt) {
			@{$pt-> {$_}} = grep { $_ ne $lookup } @{$pt-> {$_}};
		}
	}
}

sub init
{
	my $self = shift;
	for ( qw( marked sizeable)) {
		$self-> {$_}=0;
	};
	my %profile = $self-> SUPER::init(@_);
	$self->{o_delta} = $profile{o_delta};
	for ( qw( marked sizeable mainEvent)) {
		$self-> $_( $profile{$_});
	}
	my %names = map { $_-> name => 1} grep { $_ != $self } $VB::form-> widgets;
	$names{$VB::form-> name} = 1;
	my $xname = $self-> name;
	my $yname = $xname;
	my $cnt = 0;
	$yname = sprintf("%s%d", $xname, $cnt++) while exists $names{$yname};
	$profile{profile}-> {name} = $yname;
	$self-> init_profiler( \%profile);
	Prima::VB::ObjectInspector::renew_widgets();
	return %profile;
}

sub get_profile_default
{
	my $self = $_[0];
}

sub common_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	if ( $self-> {marked}) {
		$canvas-> color( cl::Black);
		$canvas-> rectangle( 1, 1, $sz[0] - 2, $sz[1] - 2);
		$canvas-> rop( rop::XorPut);
		$canvas-> color( cl::Set);
		my ( $hw, $hh) = ( int($sz[0]/2), int($sz[1]/2));
		my $mark = 2 * $::application->uiScaling;
		$mark = 2 if $mark < 2;
		my $dmark = $mark * 2;
		$canvas-> bar( 0,0,$dmark,$dmark);
		$canvas-> bar( $hw-$mark,0,$hw+$mark,$dmark);
		$canvas-> bar( $sz[0]-$dmark-1,0,$sz[0]-1,$dmark);
		$canvas-> bar( 0,$sz[1]-$dmark-1,$dmark,$sz[1]-1);
		$canvas-> bar( $hw-$mark,$sz[1]-$dmark-1,$hw+$mark,$sz[1]-1);
		$canvas-> bar( $sz[0]-$dmark-1,$sz[1]-$dmark-1,$sz[0]-1,$sz[1]-1);
		$canvas-> bar( 0,$hh-$mark,$dmark,$hh+$mark);
		$canvas-> bar( $sz[0]-$dmark-1,$hh-$mark,$sz[0]-1,$hh+$mark);
		$canvas-> rop( rop::CopyPut);
	} elsif ( $self-> {locked}) {
		my $x = $VB::form->{guidelineX} - $self-> left;
		my $y = $VB::form->{guidelineY} - $self-> bottom;
		$canvas-> rop2(rop::CopyPut);
		$canvas-> fillPattern([0,0,0,0,4,0,0,0]);
		$canvas-> backColor( cl::Clear);
		$canvas-> color( cl::Blue);
		$canvas-> rop2(rop::NoOper);
		$canvas-> bar( 0, 0, @sz);
		$canvas-> linePattern( lp::Dash);
		$canvas-> line( $x, 0, $x, $sz[1]);
		$canvas-> line( 0, $y, $sz[0], $y);
	}
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	$canvas-> color( cl::LightGray);
	$canvas-> bar( 1,1,$sz[0]-2,$sz[1]-2);
	$canvas-> color( cl::Gray);
	$canvas-> rectangle( 0,0,$sz[0]-1,$sz[1]-1);
	$self-> common_paint( $canvas);
}

sub add_o_delta
{
	my ( $self, $dx, $dy ) = @_;
	$self->{o_delta}->[0] += $dx;
	$self->{o_delta}->[1] += $dy;
}

sub get_o_delta
{
	my $w = $_[0];
	return ( 0, 0) if $w == $VB::form;

	my $ownerName = $w-> prf( 'owner');
	return ( 0, 0) if ( $ownerName eq '') || ( $ownerName eq $VB::form-> name);

	my @o = $VB::form-> bring( $ownerName)->origin;
	$o[$_] += $w->{o_delta}->[$_] for 0, 1;
	return @o;
}

sub o_delta_aperture { 0, 0 }

sub xy2part
{
	my ( $self, $x, $y) = @_;
	my @size = $self-> size;
	my $minDim = $size[0] > $size[1] ? $size[1] : $size[0];
	my $bw   = ($minDim < 12) ? (($minDim < 7)  ? 1 : 3) : 5;
	my $bwx  = ($minDim < 26) ? (($minDim < 14) ? 1 : 7) : $bw + 8;
	return q(client) unless $self-> {sizeable};
	if ( $x < $bw) {
		return q(SizeSW) if $y < $bwx;
		return q(SizeNW) if $y >= $size[1] - $bwx;
		return q(SizeW);
	} elsif ( $x >= $size[0] - $bw) {
		return q(SizeSE) if $y < $bwx;
		return q(SizeNE) if $y >= $size[1] - $bwx;
		return q(SizeE);
	} elsif (( $y < $bw) or ( $y >= $size[1] - $bw)) {
		return ( $y < $bw) ? q(SizeSW) : q(SizeNW) if $x < $bwx;
		return ( $y < $bw) ? q(SizeSE) : q(SizeNE) if $x >= $size[0] - $bwx;
		return $y < $bw ? 'SizeS' : 'SizeN';
	}
	return q(client);
}

sub xorrect
{
	my ( $self, $r0, $r1, $r2, $r3) = @_;

	return undef $self->{rubberbands} unless defined $r0;

	my @d = $self-> owner-> screen_to_client(0,0);
	my @o = $self-> get_o_delta();
	$d[$_] -= $o[$_] for 0..1;
	$VB::form-> text(
		'['.($r0+$d[0]).', '.($r1+$d[1]).'] - ['.($r2+$d[0]).', '.($r3+$d[1]).']'
	);

	my $o = $self->owner;
	$self->{rubberbands} //= [ map {
		Prima::Widget::RubberBand->new(
			breadth  => $self-> {sizeable} ? 5 : 1,
			clipRect => [$o->client_to_screen( 0, 0, $o->size )],
		)
	} 0 .. @{ $self->{extraRects} // [] } ];
	$self->{rubberbands}->[-1]->set( rect => [$o->client_to_screen($r0,$r1,$r2-1,$r3-1)] );

	if ( defined $self-> {extraRects}) {
		my ( $ax, $ay) = @{$self-> {sav}};
		my @org = ( $ax, $ay);
		$org[0] = $r0 - $org[0];
		$org[1] = $r1 - $org[1];
		my $i = 0;
		$self->{rubberbands}->[$i++]->set(
			rect    => [ $o->client_to_screen($$_[0] + $org[0], $$_[1] + $org[1], $$_[2] + $org[0], $$_[3] + $org[1]) ]
		) for @{ $self->{extraRects} };
	}
}


sub maintain_children_origin
{
	my ( $self, $oldx, $oldy) = @_;
	my ( $x, $y) = $self-> origin;
	return if $x == $oldx && $y == $oldy;
	$x -= $oldx;
	$y -= $oldy;
	my $name = $self-> name;

	for ( $VB::form-> widgets) {
		next unless $_-> prf('owner') eq $name;
		my @o = $_-> origin;
		$_-> origin( $o[0] + $x, $o[1] + $y);
		$_-> maintain_children_origin( @o);
	}
}

sub iterate_children
{
	my ( $self, $sub, @xargs) = @_;
	my $name = $self-> name;
	for ( $VB::form-> widgets) {
		next unless $_-> prf('owner') eq $name;
		$sub-> ( $_, $self, @xargs);
		$_-> iterate_children( $sub, @xargs);
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	if ( $btn == mb::Left) {
		if ( defined $VB::main-> {currentClass}) {
			$VB::form-> insert_new_control(
				$self-> left + $x, $self-> bottom + $y, $self);
			return;
		}

		if ( $mod & km::Shift) {
			$self-> marked( $self-> marked ? 0 : 1);
			Prima::VB::ObjectInspector::update_markings();
			$self-> focus;
			return;
		}

		if ( $mod & km::Ctrl) {
			$self-> {locked} = not $self-> {locked};
			$self-> marked(0) if $self-> {locked};
			$self-> repaint;
			return;
		}

		my $part = $self-> xy2part( $x, $y);
		if ( $self-> {locked} and not $self-> marked) {
			# propagate for marquee selection
			$VB::form-> on_mousedown(
				$btn, $mod,
				$x + $self-> left,
				$y + $self-> bottom
			) if $self != $VB::form;
			return;
		}

		$self-> bring_to_front;
		$self-> focus;
		if ( $VB::inspector) {
			$VB::inspector-> {selectorChanging} = 1; # disallow auto single-select
			Prima::VB::ObjectInspector::enter_widget( $self);
			$VB::inspector-> {selectorChanging} = 0;
		}

		$self-> iterate_children( sub { $_[0]-> bring_to_front; $_[0]-> update_view; });

		my @mw;
		@mw = $VB::form-> marked_widgets if $part eq q(client) && $self-> marked;
		$self-> marked( 1, 1) unless @mw;
		$self-> clear_event;
		$self-> capture(1, $self-> owner);
		$self-> {spotX} = $x;
		$self-> {spotY} = $y;
		$VB::form-> {modified} = 1;
		if ( $part eq q(client)) {
			my @rects = ();
			for ( @mw) {
				next if $_ == $self;
				$_-> marked(1);
				push( @rects, [$_->rect]);
			}
			$self-> {sav} = [$self-> origin];
			$self-> {drag} = 1;
			$VB::form-> dm_init( $self);
			$self-> {extraWidgets} = \@mw;
			$self-> {extraRects}   = \@rects;
			$self-> {prevRect} = [$self->rect];
			$self-> update_view;
			$VB::form-> {saveHdr} = $VB::form-> text;
			$self-> xorrect( @{$self-> {prevRect}});
			return;
		}

		if ( $part =~ /^Size/) {
			$self-> {sav} = [$self-> rect];
			$part =~ s/^Size//;
			$self-> {sizeAction} = $part;
			my ( $xa, $ya) = ( 0,0);
			if    ( $part eq q(S))   { ( $xa, $ya) = ( 0,-1); }
			elsif ( $part eq q(N))   { ( $xa, $ya) = ( 0, 1); }
			elsif ( $part eq q(W))   { ( $xa, $ya) = (-1, 0); }
			elsif ( $part eq q(E))   { ( $xa, $ya) = ( 1, 0); }
			elsif ( $part eq q(SW)) { ( $xa, $ya) = (-1,-1); }
			elsif ( $part eq q(NE)) { ( $xa, $ya) = ( 1, 1); }
			elsif ( $part eq q(NW)) { ( $xa, $ya) = (-1, 1); }
			elsif ( $part eq q(SE)) { ( $xa, $ya) = ( 1,-1); }
			$self-> {dirData} = [$xa, $ya];
			$self-> {prevRect} = [$self->rect];
			$self-> update_view;
			$VB::form-> {saveHdr} = $VB::form-> text;
			$self-> xorrect( @{$self-> {prevRect}});
			return;
		}
	}

	if ( $btn == mb::Right && $mod & km::Ctrl) {
		$self-> altpopup;
		$self-> clear_event;
		return;
	}
}

sub altpopup
{
	my $self  = $_[0];
	while ( 1) {
		my $p = $self-> bring( 'AltPopup');
		if ( $p) {
			$p-> popup( $self-> pointerPos);
			last;
		}
		last if $self == $VB::form;
		my $o = $self-> prf('owner');
		$self = ( $o eq $VB::form-> name) ? $VB::form : $VB::form-> bring( $o);
		last unless $self;
	}
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;

	return unless $dbl;
	$mod &= km::Alt|km::Shift|km::Ctrl;
	if ( $mod == 0 && defined $self-> mainEvent && $VB::inspector) {
		my $a = $self-> mainEvent;
		$self-> marked(1,1);
		$VB::inspector-> set_monger_index( 1);
		my $list = $VB::inspector-> {currentList};
		my $ix = $list-> {index}-> {$a};
		if ( defined $ix) {
			$list-> focusedItem( $ix);
			$list-> notify(q(Click)) unless $list-> {check}-> [$ix];
		}
		return;
	}
	$self-> notify( q(MouseDown), $btn, $mod, $x, $y);
}


sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	if ( $self-> {drag}) {
		my $dm = $VB::form-> dragMode;
		if ( $dm != 3) {
			my @o = @{$self-> {prevRect}}[0,1];
			my $rx = $x;
			$y = $o[1] - $self->bottom + $self-> {spotY} if $dm == 1;
			$x = $o[0] - $self->left   + $self-> {spotX} if $dm == 2;
		}
		my @sz = $self-> size;
		my @og = $self-> origin;
		if ( $VB::main-> {ini}-> {SnapToGrid}) {
			return if
				!$self-> {dragImpedance} &&
				abs( $x - $self-> {spotX}) < 4 &&
				abs( $y - $self-> {spotY}) < 4
				;
			$self-> {dragImpedance} = 1;
			$x -= ( $x - $self-> {spotX} + $og[0]) % 4;
			$y -= ( $y - $self-> {spotY} + $og[1]) % 4;
		}

		if ( $VB::main-> {ini}-> {SnapToGuidelines}) {
			my $xline = $VB::form-> {guidelineX} - $og[0];
			my $yline = $VB::form-> {guidelineY} - $og[1];
			$x = $xline + $self-> {spotX}
				if abs( $xline - $x + $self-> {spotX}) < 8;
			$y = $yline + $self-> {spotY}
				if abs( $yline - $y + $self-> {spotY}) < 8;
			$x = $xline + $self-> {spotX} - $sz[0]
				if abs( $xline - $x + $self-> {spotX} - $sz[0]) < 8;
			$y = $yline + $self-> {spotY} - $sz[1]
				if abs( $yline - $y + $self-> {spotY} - $sz[1]) < 8;
		}
		my @xorg = ( $self-> left + $x - $self-> {spotX}, $self->bottom + $y - $self-> {spotY});
		$self-> {prevRect} = [ @xorg, $sz[0] + $xorg[0], $sz[1] + $xorg[1]];
		$self-> xorrect( @{$self-> {prevRect}});
	} elsif ( $self-> {sizeable}) {
		if ( $self-> {sizeAction}) {
			my @org = $_[0]-> rect;
			my @new = @org;
			my @min = $self-> sizeMin;
			my @og = $self-> origin;
			my ( $xa, $ya) = @{$self-> {dirData}};

			if ( $VB::main-> {ini}-> {SnapToGrid}) {
				return if
					!$self-> {dragImpedance} &&
					abs( $x - $self-> {spotX}) < 4 &&
					abs( $y - $self-> {spotY}) < 4
					;
				$self-> {dragImpedance} = 1;
				$x -= ( $x - $self-> {spotX} + $og[0]) % 4;
				$y -= ( $y - $self-> {spotY} + $og[1]) % 4;
			}

			if ( $VB::main-> {ini}-> {SnapToGuidelines}) {
				my @sz = $self-> size;
				my $xline = $VB::form-> {guidelineX} - $og[0];
				my $yline = $VB::form-> {guidelineY} - $og[1];
				if ( $xa != 0) {
					$x = $xline + $self-> {spotX}
						if abs( $xline - $x + $self-> {spotX}) < 8;
					$x = $xline + $self-> {spotX} - $sz[0]
						if abs( $xline - $x + $self-> {spotX} - $sz[0]) < 8;
				}
				if ( $ya != 0) {
					$y = $yline + $self-> {spotY}
						if abs( $yline - $y + $self-> {spotY}) < 8;
					$y = $yline + $self-> {spotY} - $sz[1]
						if abs( $yline - $y + $self-> {spotY} - $sz[1]) < 8;
				}
			}

			if ( $xa < 0) {
				$new[0] = $org[0] + $x - $self-> {spotX};
				$new[0] = $org[2] - $min[0] if $new[0] > $org[2] - $min[0];
			} elsif ( $xa > 0) {
				$new[2] = $org[2] + $x - $self-> {spotX};
				if ( $new[2] < $org[0] + $min[0]) {
					$new[2] = $org[0] + $min[0];
				}
			}

			if ( $ya < 0) {
				$new[1] = $org[1] + $y - $self-> {spotY};
				$new[1] = $org[3] - $min[1] if $new[1] > $org[3] - $min[1];
			} elsif ( $ya > 0) {
				$new[3] = $org[3] + $y - $self-> {spotY};
				if ( $new[3] < $org[1] + $min[1]) {
					$new[3] = $org[1] + $min[1];
				}
			}

			if (
				$org[1] != $new[1] || $org[0] != $new[0] ||
				$org[2] != $new[2] || $org[3] != $new[3]
			) {
				$self-> {prevRect} = [@new];
				$self-> xorrect( @{$self-> {prevRect}});
			}
			return;
		} else {
			return if !$self-> enabled;
			my $part = $self-> xy2part( $x, $y);
			$self-> pointer( $part =~ /^Size/ ? &{$cr::{$part}} : cr::Arrow);
		}
	}

	if ( $self-> {locked} and not $self-> marked) {
		# propagate guideline selection
		$x += $self-> left;
		$y += $self-> bottom;
		if ( abs( $VB::form-> {guidelineX} - $x) < 3) {
			$self-> pointer(( abs( $VB::form-> {guidelineY} - $y) < 3) ?
				cr::Move :
				cr::SizeWE);
		} elsif ( abs( $VB::form-> {guidelineY} - $y) < 3) {
			$self-> pointer( cr::SizeNS);
		} else {
			$self-> pointer( cr::Arrow);
		}
	}
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	if ( $btn == mb::Left)
	{
		if ( $self-> {drag}) {
			$self-> pointer( cr::Default);
			$self-> capture(0);
			$self-> {drag} = 0;
			$self-> xorrect;
			my @o = $self-> origin;
			$self-> origin( @{$self-> {prevRect}}[0,1] );
			if ( defined $self-> {extraRects}) {
				# get all children, and do _not_ move them together with us
				my @allchildren = ($self-> name);
				my %allwidgets;
				push @{$allwidgets{$_->prf('owner')}}, $_->name for $VB::form-> widgets;
				for ( my $i = 0; $i < @allchildren; $i++) {
					push @allchildren, @{$allwidgets{$allchildren[$i]}}
						if $allwidgets{$allchildren[$i]};
				}
				my %allchildren = map { $_ => 1 } @allchildren;

				my @org = @{$self-> {sav}};
				$org[0] = $self-> {prevRect}-> [0] - $org[0];
				$org[1] = $self-> {prevRect}-> [1] - $org[1];
				for my $wij ( @{$self-> {extraWidgets}}) {
					next if $allchildren{$wij-> name};
					my @o = $wij-> origin;
					$wij-> origin( $o[0] + $org[0], $o[1] + $org[1]);
				}
			}
			$VB::form-> text( $VB::form-> {saveHdr});
			$self-> {extraRects} = $self-> {extraWidgets} = undef;
		}
		if ( $self-> {sizeAction}) {
			my @r = @{$self-> {prevRect}};
			$self-> xorrect;
			my @o = $self-> origin;
			$self-> rect( @r);
			$self-> pointer( cr::Default);
			$self-> capture(0);
			$self-> {sizeAction} = 0;
			$VB::form-> text( $VB::form-> {saveHdr});
		}
	}
}

sub on_popup
{
	my $self = shift;
	my ($by_mouse, $x, $y) = @_;
	my $alt = $self-> bring('AltPopup');
	if ($alt) {
		my $aitems = $alt-> get_items('');
		my $pitems = $VB::form-> popup-> get_items('');
		my $p = Prima::Popup-> create(
				name => 'AltFormPopup',
				items => [
					@$pitems,
					[],
					[ '-' . $self-> name => '** ' . $self-> name . ' **' => qw(nope)],
					@$aitems,
				]
		);
		$p-> popup($self-> client_to_screen($x, $y));
		$self-> clear_event;
		return;
	}
}

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	if ( $key == kb::Delete) {
		$self-> clear_event;
		$_-> destroy for $VB::form-> marked_widgets;
		Prima::VB::ObjectInspector::renew_widgets();
		return;
	}
	if ( $key == kb::Esc) {
		if ( $self-> {drag} || $self-> {sizeAction}) {
			$self-> xorrect;
			$self-> {drag} = $self-> {sizeAction} = 0;
			$self-> {dirData} = $self-> {spotX} = $self-> {spotY} = undef;
			$self-> pointer( cr::Default);
			$self-> capture(0);
			$VB::form-> text( $VB::form-> {saveHdr});
			return;
		}
	}
	if ( $key == kb::Tab && $self-> {drag}) {
		$VB::form-> dm_next( $self);
		my @pp = $self->owner-> pointerPos;
		$self-> {spotX} = $pp[0] - $self-> {prevRect}-> [0];
		$self-> {spotY} = $pp[1] - $self-> {prevRect}-> [1];
		$self-> clear_event;
		return;
	}
}

sub marked
{
	if ( $#_) {
		my ( $self, $mark, $exlusive) = @_;
		$mark = $mark ? 1 : 0;
		$mark = 0 if $self == $VB::form;
		return if ( $mark == $self-> {marked}) && !$exlusive;
		if ( $exlusive) {
			$_-> marked(0) for $VB::form-> marked_widgets;
		}
		$self-> {marked} = $mark;
		$self-> repaint;
		$VB::main-> update_markings();
	} else {
		return 0 if $_[0] == $VB::form;
		return $_[0]-> {marked};
	}
}

sub sizeable
{
	return $_[0]-> {sizeable} unless $#_;
	return if $_[1] == $_[0]-> {sizeable};
	$_[0]-> {sizeable} = $_[1];
	$_[0]-> pointer( cr::Default) unless $_[1];
}

sub mainEvent
{
	return $_[0]-> {mainEvent} unless $#_;
	$_[0]-> {mainEvent} = $_[1];
}

sub prf_name
{
	my $old = $_[0]-> name;
	$_[0]-> name($_[1]);
	$_[0]-> name_changed( $old, $_[1]);
	$_[0]-> update_hint if $VB::form && $_[0] != $VB::form;

	return unless $VB::inspector;
	my $s = $VB::inspector-> Selector;
	$VB::inspector-> {selectorChanging}++;
	my @it = @{$s-> List-> items};
	my $sn = $s-> text;
	my $si = $s-> focusedItem;
	for ( @it) {
		$sn = $_ = $_[1] if $_ eq $old;
	}
	$s-> List-> items( \@it);
	$s-> text( $sn);
	$s-> focusedItem( $si);
	$VB::inspector-> {selectorChanging}--;
}

sub name_changed
{
	return unless $VB::form;
	my ( $self, $oldName, $newName) = @_;

	for ( $VB::form, $VB::form-> widgets) {
		my $pf = $_-> {prf_types};
		next unless defined $pf-> {Handle};
		my $widget = $_;
		for ( @{$pf-> {Handle}}) {
			my $val = $widget-> prf( $_);
			next unless defined $val;
			$widget-> prf_set( $_ => $newName) if $val eq $oldName;
		}
	}
}

sub owner_changed
{
	my ( $self, $from, $to) = @_;
	$self-> {lastOwner} = $to;
	return unless $VB::form;
	return if $VB::form == $self;
	if ( defined $to && defined $from) {
		return if $to eq $from;
	}
	return if !defined $to && !defined $from;
	if ( defined $from) {
		$from = $VB::form-> bring( $from);
		$from = $VB::form unless defined $from;
		$from-> prf_detach( $self);
		$self-> add_o_delta( map { -1 * $_ } $from-> o_delta_aperture);
	}
	if ( defined $to) {
		$to = $VB::form-> bring( $to);
		$to = $VB::form unless defined $to;
		$self-> add_o_delta( $to-> o_delta_aperture);
		$to-> prf_attach( $self);
	}
}

sub prf_attach {}
sub prf_detach {}

sub prf_owner
{
	my ( $self, $owner) = @_;
	$self-> owner_changed( $self-> {lastOwner}, $owner);
}

sub on_destroy
{
	my $self = $_[0];
	my $n = $self-> name;
	$self-> remove_hooks;
	$self-> name_changed( $n, $VB::form-> name) if $VB::form;
	$self-> owner_changed( $self-> prf('owner'), undef);
	if ( $hooks{DESTROY}) {
		$_-> on_hook( $self-> name, 'DESTROY', undef, undef, $self) for @{$hooks{DESTROY}};
	}
	$VB::main-> update_markings();
}

sub update_hint
{
	my $self = $_[0];
	my @d = $self-> get_o_delta();
	my @o = $self-> origin;
	$o[0] += $d[0];
	$o[1] += $d[1];
	$self-> hint(
		$self-> name . ' ['.
		join(',', @o) . '-' .
		join(',', $self-> size) .
	']');
}

package Prima::VB::Drawable;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Component);


sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		color         => ['color','backColor'],
		fillPattern   => ['fillPattern'],
		font          => ['font'],
		lineEnd       => ['lineEnd'],
		lineJoin      => ['lineJoin'],
		linePattern   => ['linePattern'],
		lineWidth     => ['lineWidth'],
		rop           => ['rop', 'rop2'],
		bool          => ['textOutBaseline', 'textOpaque', 'fillMode'],
		point         => ['translate', 'fillPatternOffset'],
		palette       => ['palette'],
		image         => ['region'],
		uiv           => ['miterLimit'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

package Prima::VB::Widget;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Drawable);

sub prf_adjust_default
{
	my ( $self, $prf, $def) = @_;
	$self-> SUPER::prf_adjust_default( $prf, $def);
	$def-> {size} = [$def-> {width}, $def-> {height}];
	$self-> size(@{$def-> {size}});

	delete $def-> {$_} for qw (
		accelTable
		clipOwner
		current
		currentWidget
		delegations
		effects
		focused
		popup
		selected
		selectedWidget
		capture
		hintVisible
		widgets
		buffered

		left
		right
		top
		bottom
		width
		height
		rect

		alpha
		antialias
		lineEnd
		lineJoin
		linePattern
		lineWidth
		fillPattern
		fillPatternOffset
		fillMode
		miterLimit
		region
		rop
		rop2
		textOpaque
		textOutBaseline
		translate
	);
	$def-> {text} = '' unless defined $def-> {text};
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		mainEvent => 'onMouseClick',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		menu          => [qw(accelTable popup)],
		menuItems     => [qw(accelItems popupItems)],
		color         => [qw(dark3DColor light3DColor disabledBackColor
			disabledColor hiliteBackColor hiliteColor popupColor
			popupBackColor popupHiliteColor popupHiliteBackColor
			popupDisabledColor popupDisabledBackColor
			popupLight3DColor popupDark3DColor
		)],
		font          => ['popupFont'],
		bool          => [qw(autoEnableChildren briefKeys buffered capture clipOwner
			centered current cursorVisible enabled firstClick focused
			hintVisible ownerColor ownerBackColor ownerFont ownerHint
			ownerShowHint ownerPalette scaleChildren
			selectable selected showHint syncPaint tabStop transparent
			visible x_centered y_centered originDontCare sizeDontCare
			packPropagate layered clipChildren
		)],
		iv            => [qw(bottom height left right top width)],
		tabOrder      => ['tabOrder'],
		rect          => ['rect'],
		point         => ['cursorPos'],
		origin        => ['origin'],
		upoint        => [qw(cursorSize designScale size sizeMin sizeMax pointerHotSpot)],
		widget        => [qw(currentWidget selectedWidget)],
		pointer       => ['pointer',],
		growMode      => ['growMode'],
		geometry      => ['geometry'],
		string        => ['helpContext', 'dndAware'],
		text          => ['text', 'hint'],
		selectingButtons=> ['selectingButtons'],
		widgetClass   => ['widgetClass'],
		image         => ['shape'],
		packInfo      => ['packInfo'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onColorChanged   => 'my ($self, $colorIndex) = @_;',
		onDragDrop       => 'my ($self, $x, $y) = @_;',
		onDragOver       => 'my ($self, $x, $y, $state) = @_;',
		onEndDrag        => 'my ($self, $x, $y) = @_;',
		onHint           => 'my ($self, $show) = @_;',
		onKeyDown        => 'my ($self, $code, $key, $mod, $repeat) = @_;',
		onKeyUp          => 'my ($self, $code, $key, $mod) = @_;',
		onMenu           => 'my ($self, $menu, $variable) = @_;',
		onMouseDown      => 'my ($self, $btn, $mod, $x, $y) = @_;',
		onMouseUp        => 'my ($self, $btn, $mod, $x, $y) = @_;',
		onMouseClick     => 'my ($self, $btn, $mod, $x, $y, $dblclk) = @_;',
		onMouseMove      => 'my ($self, $mod, $x, $y) = @_;',
		onMouseWheel     => 'my ($self, $mod, $x, $y, $z) = @_;',
		onMouseEnter     => 'my ($self, $mod, $x, $y) = @_;',
		onMove           => 'my ($self, $oldx, $oldy, $x, $y) = @_;',
		onPaint          => 'my ($self, $canvas) = @_;',
		onPopup          => 'my ($self, $mouseDriven, $x, $y) = @_;',
		onSize           => 'my ($self, $oldx, $oldy, $x, $y) = @_;',
		onTranslateAccel => 'my ($self, $code, $key, $mod) = @_;',
	);
}

sub prf_color        { $_[0]-> recolor($_[1],'color'); }
sub prf_backColor    { $_[0]-> recolor($_[1],'backColor'); }
sub prf_light3DColor { $_[0]-> recolor($_[1],'light3DColor'); }
sub prf_dark3DColor  { $_[0]-> recolor($_[1],'dark3DColor'); }
sub prf_hiliteColor       { $_[0]-> recolor($_[1],'hiliteColor'); }
sub prf_disabledColor     { $_[0]-> recolor($_[1],'disabledColor'); }
sub prf_hiliteBackColor   { $_[0]-> recolor($_[1],'hiliteBackColor'); }
sub prf_disabledBackColor { $_[0]-> recolor($_[1],'disabledBackColor'); }
sub prf_name         { $_[0]-> SUPER::prf_name($_[1]); $_[0]-> repaint;      }
sub prf_text         { $_[0]-> text($_[1]); $_[0]-> repaint; }
sub prf_font         { $_[0]-> recolor($_[1],'font'); }
sub prf_left         { $_[0]-> rerect($_[1], 'left'); }
sub prf_right        { $_[0]-> rerect($_[1], 'right'); }
sub prf_top          { $_[0]-> rerect($_[1], 'top'); }
sub prf_bottom       { $_[0]-> rerect($_[1], 'bottom'); }
sub prf_width        { $_[0]-> rerect($_[1], 'width'); }
sub prf_height       { $_[0]-> rerect($_[1], 'height'); }
sub prf_origin       { $_[0]-> rerect($_[1], 'origin'); }
sub prf_size         { $_[0]-> rerect($_[1], 'size'); }
sub prf_rect         { $_[0]-> rerect($_[1], 'rect'); }
sub prf_centered     { $_[0]-> centered(1) if $_[1]; }
sub prf_x_centered   { $_[0]-> x_centered(1) if $_[1]; }
sub prf_y_centered   { $_[0]-> y_centered(1) if $_[1]; }

sub rerect
{
	my ( $self, $data, $who) = @_;
	return if $self-> {syncRecting};
	$self-> {syncRecting} = $who;
	$self-> set(
		$who => $data,
	);
	if (( $who eq 'left') || ( $who eq 'bottom')) {
		$self-> prf_origin( [$self-> origin]);
	}
	if (( $who eq 'width') || ( $who eq 'height') || ( $who eq 'right') || ( $who eq 'top')) {
		$self-> prf_size( [ $self-> size]);
	}
	$self-> {syncRecting} = undef;
	$self-> update_hint;
}

sub recolor
{
	my ( $self, $data, $who) = @_;
	return if $self-> {syncColoring};
	$self-> {syncColoring} = $who;
	$self-> set(
		$who => $data,
	);
	$self-> {syncColoring} = undef;
}

sub on_move
{
	my ( $self, $ox, $oy, $x, $y) = @_;
	unless ( $self-> {syncRecting}) {
		$self-> {syncRecting} = $self;
		$self-> prf_set( origin => [$x, $y]);
		$self-> {syncRecting} = undef;
	}
	$self-> maintain_children_origin( $ox, $oy) if $self != $VB::form;
}

sub on_size
{
	my ( $self, $ox, $oy, $x, $y) = @_;
	return if $self-> {syncRecting};
	$self-> {syncRecting} = $self;
	$self-> prf_set( size => [$x, $y]);
	$self-> update_children_geometry( $ox, $oy, $x, $y);
	$self-> {syncRecting} = undef;
}

sub on_colorchanged
{
	my ( $self, $index) = @_;
	my @colors = qw(color backColor hiliteColor hiliteBackColor
		disabledColor disabledBackColor light3DColor dark3DColor);
	return if $self-> {syncColoring} or $index >= @colors;
	$self-> {syncColoring} = 1;
	$index = $colors[$index];
	$self-> prf_set( $index => $self-> $index());
	delete $self-> {syncColoring};
}

sub on_fontchanged
{
	my ( $self) = @_;
	return if $self-> {syncColoring};
	$self-> {syncColoring} = 1;
	$self-> prf_set( font => $self-> font);
	delete $self-> {syncColoring};
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	my $cc = $self-> color;
	$canvas-> color( $self-> backColor);
	$canvas-> bar( 1,1,$sz[0]-2,$sz[1]-2);
	$canvas-> color( $cc);
	$canvas-> rectangle( 0,0,$sz[0]-1,$sz[1]-1);
	$self-> common_paint( $canvas);
}

sub on_mouseenter
{
	my $self = shift;
	return if $self->{locked};
	local $self->{syncColoring} = 1;
	$self->{prelight} = 1;
	$self->backColor( $self->prelight_color( $self-> prf('backColor') ));
}

sub on_mouseleave
{
	my $self = shift;
	return if $self->{locked} && !$self->{prelight};
	local $self->{syncColoring} = 1;
	delete $self->{prelight};
	$self->backColor( $self-> prf('backColor'));
}

sub update_children_geometry
{
	my ($self, $ox, $oy, $x, $y) = @_;
	return unless $VB::form;
	my $name = $self-> prf('name');
	my @w    = grep { $_-> prf('owner') eq $name } $VB::form-> widgets;
	my @o    = ( $self == $VB::form) ? ( 0, 0) : $self-> origin;
	for ( @w) {
		if ( $_-> prf('geometry') == gt::GrowMode) {
			my @size  = $_-> get_virtual_size;
			my @pos   = $_-> origin;
			$pos[$_] -= $o[$_] for 0,1;
			my @osize = @size;
			my @opos  = @pos;
			my @d     = ( $x - $ox, $y - $oy);
			my $gm    = $_-> prf('growMode');
			$pos[0]  += $d[0] if $gm & gm::GrowLoX;
			$pos[1]  += $d[1] if $gm & gm::GrowLoY;
			$size[0] += $d[0] if $gm & gm::GrowHiX;
			$size[1] += $d[1] if $gm & gm::GrowHiY;
			$pos[0]   = ( $x - $size[0]) / 2 if $gm & gm::XCenter;
			$pos[1]   = ( $y - $size[1]) / 2 if $gm & gm::YCenter;
			unless ( grep { $pos[$_] != $opos[$_] and $size[$_] != $osize[$_] } 0,1) {
				$pos[$_] += $o[$_] for 0,1;
				$_-> rect( @pos, $pos[0] + $size[0], $pos[1] + $size[1]);
			}
		}
	}
}

package Prima::VB::Control;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Widget);


sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		briefKeys
		cursorPos
		cursorSize
		cursorVisible
		pointer
		pointerType
		pointerHotSpot
		pointerIcon
		scaleChildren
		selectable
		selectingButtons
		popupColor
		popupBackColor
		popupHiliteBackColor
		popupDisabledBackColor
		popupHiliteColor
		popupDisabledColor
		popupDark3DColor
		popupLight3DColor
		popupFont
		widgetClass
	);
}

package Prima::VB::Window;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Control);

sub prf_adjust_default
{
	my ( $self, $p, $pf) = @_;
	$self-> SUPER::prf_adjust_default( $p, $pf);
	delete $pf-> {$_} for qw (
		menu
		modalResult
		menuColor
		menuBackColor
		menuHiliteBackColor
		menuDisabledBackColor
		menuHiliteColor
		menuDisabledColor
		menuDark3DColor
		menuLight3DColor
		menuFont
	);
}

sub prf_events
{
	return (
		$_[0]-> SUPER::prf_events,
		onWindowState  => 'my ( $self, $windowState) = @_;',
	);
}

sub prf_types
{
	my $pt = $_[ 0]-> SUPER::prf_types;
	my %de = (
		borderIcons   => ['borderIcons'],
		borderStyle   => ['borderStyle'],
		windowState   => ['windowState'],
		icon          => ['icon'],
		menu          => ['menu'],
		menuItems     => ['menuItems'],
		color         => ['menuColor', 'menuHiliteColor','menuDisabledColor',
			'menuBackColor', 'menuHiliteBackColor','menuDisabledBackColor',
			'menuLight3DColor', 'menuDark3DColor'
		],
		font          => ['menuFont'],
		bool          => ['modalHorizon', 'taskListed', 'ownerIcon', 'onTop', 'mainWindow'],
		uiv           => ['modalResult'],
	);
	$_[0]-> prf_types_add( $pt, \%de);
	return $pt;
}


package Prima::VB::Types;

use Prima::Sliders;
use Prima::InputLine;
use Prima::Edit;
use Prima::Buttons;
use Prima::Label;
use Prima::Outlines;

package Prima::VB::Types::generic;
use strict;

sub new
{
	my $class = shift;
	my $self = {};
	bless( $self, $class);
	( $self-> {container}, $self-> {id}, $self-> {widget}) = @_;
	$self-> {changeProc} = \&Prima::VB::ObjectInspector::item_changed;
	$self-> open();
	return $self;
}

sub renew
{
	my $self = shift;
	( $self-> {id}, $self-> {widget}) = @_;
	$self-> change_id();
}

sub change_id
{
}

sub open
{
}

sub set
{
}

sub valid
{
	return 1;
}

sub change
{
	$_[0]-> {changeProc}-> ( $_[0]);
}


sub write
{
	my ( $class, $id, $data) = @_;
	return 'undef' unless defined $data;
	return "'". Prima::VB::Types::generic::quotable($data)."'" unless ref $data;
	if ( ref( $data) eq 'ARRAY') {
		my $c = '';
		for ( @$data) {
			$c .= $class-> write( $id, $_) . ', ';
		}
		return "[$c]";
	}
	if ( ref( $data) eq 'HASH') {
		my $c = '';
		for ( keys %{$data}) {
			$c .= "'". Prima::VB::Types::generic::quotable($_)."' => ".
					$class-> write(  $id, $data-> {$_}) . ', ';
		}
		return "{$c}";
	}
	return '';
}

sub quotable
{
	my $a = $_[0];
	$a =~ s/\\/\\\\/g;
	$a =~ s/\'/\\\'/g;
	return $a;
}

sub printable
{
	my $a = $_[0];
	# $a =~ s/([\0-\7])/'\\'.ord($1)/eg;
	$a =~ s/([\!\@\#\$\%\^\&\*\(\)\'\"\?\<\>\\])/'\\'.$1/eg;
	$a =~ s/([\x00-\x1f\x7f-\xff])/'\\x'.sprintf("%02x",ord($1))/eg;
	return $a;
}

sub preload_modules
{
	return ();
}

package Prima::VB::Types::textee;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub set
{
	my ( $self, $data) = @_;
	$self-> {A}-> text( $data);
}

sub get
{
	return $_[0]-> {A}-> text;
}

package Prima::VB::Types::string;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::textee);

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	$self-> {A} = $self-> {container}-> insert( InputLine =>
		origin => [ 5, $h - 36],
		width  => $self-> {container}-> width - 10,
		growMode => gm::Ceiling,
		onChange => sub {
			$self-> change;
		},
	);
}

package Prima::VB::Types::char;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::string);

sub open
{
	my $self = $_[0];
	$self-> SUPER::open( @_);
	$self-> {A}-> maxLen( 1);
}


package Prima::VB::Types::name;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::string);

sub wake { $_[0]->{A}->blink }

sub valid
{
	my $self = $_[0];
	my $tx = $self-> {A}-> text;
	$self-> wake, return 0 unless length( $tx);
	$self-> wake, return 0 if $tx =~ /[\s\\\~\!\@\#\$\%\^\&\*\(\)\-\+\=\[\]\{\}\.\,\?\;\|\`\'\"]/;
	return 1 unless $VB::inspector;
	my $l = $VB::inspector-> Selector-> List;
	my $s = $l-> items;
	my $fi= $l-> focusedItem;
	my $i = 0;
	for ( @$s) {
		next if $i++ == $fi;
		$self-> wake, return 0 if $_ eq $tx;
	}
	return 1;
}


package Prima::VB::Types::text;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::textee);
use Prima::Utils;

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	my $i = $self-> {container};
	my @sz = $i-> size;
	my $fh = $i-> font-> height;
	my $sz = 27 * $::application->uiScaling;
	$self-> {A} = $i-> insert( Edit =>
		origin   => [ 5, 5],
		size     => [ $sz[0]-10, $sz[1] - $sz - 10 ],
		growMode => gm::Client,
		onChange => sub {
			$self-> change;
		},
	);
	$i-> insert( SpeedButton =>
		origin => [5, $sz[1] - $sz - 1],
		size   => [$sz, $sz],
		hint   => 'Load',
		flat   => 1,
		growMode => gm::GrowLoY,
		image  => $VB::main-> {openbutton}-> image,
		glyphs => $VB::main-> {openbutton}-> glyphs,
		onClick => sub {
			my $d = VB::open_dialog();
			if ( $d-> execute) {
				my $f = $d-> fileName;
				if ( Prima::Utils::open( \*F, '<', $f)) {
					local $/;
					$f = <F>;
					$self-> {A}-> text( $f);
					close F;
					$self-> change;
				} else {
					Prima::MsgBox::message("Cannot load $f");
				}
			}
		},
	);
	$self-> {B} = $i-> insert( SpeedButton =>
		origin => [ 5 + 1 + $sz, $sz[1] - $sz - 1],
		size   => [$sz, $sz],
		hint   => 'Save',
		flat   => 1,
		growMode => gm::GrowLoY,
		image  => $VB::main-> {savebutton}-> image,
		glyphs => $VB::main-> {savebutton}-> glyphs,
		onClick => sub {
			my $dlg  = VB::save_dialog( filter => [
				[ 'Text files' => '*.txt'],
				[ 'All files' => '*'],
			]);
			if ( $dlg-> execute) {
				my $f = $dlg-> fileName;
				if ( Prima::Utils::open(\*F, ">", $f)) {
					local $/;
					$f = $self-> {A}-> text;
					print F $f;
					close F;
				} else {
					Prima::MsgBox::message("Cannot save $f");
				}
			}
		},
	);
	$i-> insert( SpeedButton =>
		origin => [ 5 + (1 + $sz) * 2, $sz[1] - $sz - 1],
		size   => [$sz, $sz],
		hint   => 'Clear',
		flat   => 1,
		growMode => gm::GrowLoY,
		image  => $VB::main-> {newbutton}-> image,
		glyphs => $VB::main-> {newbutton}-> glyphs,
		onClick => sub {
			$self-> set( '');
			$self-> change;
		},
	);
}

package Prima::VB::Types::fallback;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::text);

sub set
{
	my ( $self, $data) = @_;
	$self-> SUPER::set( $self-> write( $self-> {id}, $data));
}

sub get
{
	my $ret = $_[0]-> SUPER::get( @_);
	my @ev = eval $ret;
	my $ev = $#ev ? \@ev : $ev[0];
	return $ev unless $@;
	my $err = "$@";
	$err =~ s/Prima::VB::Types::fallback:*//i;
	$err =~ s/\(eval \d*\)\s*//i;
	Prima::MsgBox::message(
		$_[0]-> {widget}-> name . '::' . $_[0]-> {id} . " : $err ( $ret )");
	return '';
}

package Prima::VB::Types::iv;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::string);

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	$self-> {A} = $self-> {container}-> insert( SpinEdit =>
		origin => [ 5, $h - 36],
		min    => -16383,
		max    => 16383,
		onChange => sub {
			$self-> change;
		},
	);
}

sub write
{
	my ( $class, $id, $data) = @_;
	return $data;
}

package Prima::VB::Types::uiv;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::iv);

sub open
{
	my $self = shift;
	$self-> SUPER::open( @_);
	$self-> {A}-> min(0);
}

package Prima::VB::Types::bool;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	$self-> {A} = $self-> {container}-> insert( CheckBox =>
		origin => [ 5, $h - 36],
		text   => $self-> {id},
		onClick  => sub {
			$self-> change;
		},
	);
}

sub change_id { $_[0]-> {A}-> text( $_[0]-> {id});}
sub get       { return $_[0]-> {A}-> checked;}

sub set
{
	my ( $self, $data) = @_;
	$self-> {A}-> checked( $data);
}

sub write
{
	my ( $class, $id, $data) = @_;
	return $data ? 1 : 0;
}

package Prima::VB::Types::tabOrder;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::iv);

sub change
{
	my $self = $_[0];
	$self-> SUPER::change;
	my $val = $self-> {A}-> value;
	return if $val < 0;
	my @mine = ();
	my $owner = $self-> {widget}-> prf('owner');
	my $match = 0;
	for ( $VB::form-> widgets) {
		next unless $_-> prf('owner') eq $owner;
		next if $_ == $self-> {widget};
		push( @mine, $_);
		$match = 1 if $_-> prf('tabOrder') == $val;
	}
	return unless $match;
	for ( @mine) {
		my $to = $_-> prf('tabOrder');
		next if $to < $val;
		$_-> prf_set( tabOrder => $to + 1);
	}
}

package Prima::VB::Types::Handle;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	$self-> {A} = $self-> {container}-> insert( ComboBox =>
		origin => [ 5, $h - 36],
		width  => $self-> {container}-> width - 10,
		height => $self-> {container}-> font-> height + 4,
		style  => cs::DropDownList,
		items  => [''],
		onChange => sub {
			$self-> change;
		},
	);
}

sub set
{
	my ( $self, $data) = @_;
	if ( $self-> {widget} == $VB::form) {
		$data = '';
		$self-> {A}-> items( ['']);
	} else {
		my %items = $VB::inspector ?
			(map { $_ => 1} sort @{$VB::inspector-> Selector-> items}) :
			();
		delete $items{ $self-> {widget}-> name};
		delete @items{ map { $_-> name } $VB::form-> marked_widgets};
		$self-> {A}-> items( [ keys %items]);
		$data = $VB::form-> name unless length $data;
	}
	$self-> {A}-> text( $data);
}

sub get
{
	my $self = $_[0];
	return $self-> {A}-> text;
}

package Prima::VB::Types::color;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

my @uColors  = qw(Fore Back HiliteText Hilite DisabledText Disabled Light3DColor Dark3DColor);
my @uClasses = qw(Button CheckBox Combo Dialog Edit InputLine Label ListBox Menu
	Popup Radio ScrollBar Slider Custom Window);

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	$self-> {A} = $self-> {container}-> insert( ColorComboBox =>
		origin   => [ 85, $h - 36],
		onChange => sub {
			return if $self-> {sync};
			$self-> {sync} = 1;
			$self-> {B}-> text('undef');
			$self-> {C}-> text('undef');
			$self-> {sync} = undef;
			$self-> change;
		},
	);

	$self-> {container}-> insert( Label =>
		origin => [ 5, $h - 36],
		text      => 'RGB:',
		focusLink => $self-> {A},
	);


	$self-> {B} = $self-> {container}-> insert( ComboBox =>
		origin   => [ 5, $self-> {A}-> bottom - $self-> {container}-> font-> height - 39],
		size     => [ 136, $self-> {container}-> font-> height + 4],
		style    => cs::DropDownList,
		items    => ['undef', @uClasses],
		onChange => sub {
			return if $self-> {sync};
			$self-> change;
		},
	);

	$self-> {container}-> insert( Label =>
		origin => [ 5, $self-> {B}-> top + 5],
		text      => '~Widget class:',
		focusLink => $self-> {B},
	);


	$self-> {C} = $self-> {container}-> insert( ComboBox =>
		origin   => [ 5, $self-> {B}-> bottom - $self-> {container}-> font-> height - 39],
		size     => [ 136, $self-> {container}-> font-> height + 4],
		style    => cs::DropDownList,
		items    => ['undef', @uColors],
		onChange => sub {
			return if $self-> {sync};
			$self-> change;
		},
	);

	$self-> {container}-> insert( Label =>
		origin => [ 5, $self-> {C}-> top + 5],
		text      => '~Color index:',
		focusLink => $self-> {B},
	);
}

sub set
{
	my ( $self, $data) = @_;
	if ( $data & cl::SysFlag) {
		$self-> {A}-> value( cl::Gray);
		my ( $acl, $awc) = ( sprintf("%d",$data & ~wc::Mask), $data & wc::Mask);
		my $tx = 'undef';
		for ( @uClasses) {
			$tx = $_, last if $awc == &{$wc::{$_}}();
		}
		$self-> {B}-> text( $tx);
		$tx = 'undef';
		for ( @uColors) {
			$tx = $_, last if $acl == &{$cl::{$_}}();
		}
		$self-> {C}-> text( $tx);
	} else {
		$self-> {A}-> value( $data);
		$self-> {B}-> text( 'undef');
		$self-> {C}-> text( 'undef');
	}
}

sub get
{
	my $self = $_[0];
	my ( $a, $b, $c) = ( $self-> {A}-> value, $self-> {B}-> text, $self-> {C}-> text);
	if ( $b eq 'undef' and $c eq 'undef') {
		return $a;
	} else {
		$b = ( $b eq 'undef') ? 0 : &{$wc::{$b}}();
		$c = ( $c eq 'undef') ? 0 : &{$cl::{$c}}();
		return $b | $c;
	}
}

sub write
{
	my ( $class, $id, $data) = @_;
	my $ret = 0;
	if ( $data & cl::SysFlag) {
		my ( $acl, $awc) = ( sprintf("%d",$data & ~wc::Mask), $data & wc::Mask);
		my $tcl = '0';
		for ( @uClasses) {
			$tcl = "wc::$_", last if $awc == &{$wc::{$_}}();
		}
		my $twc = '0';
		for ( @uColors) {
			$twc = "cl::$_", last if $acl == &{$cl::{$_}}();
		}

		$ret = "$tcl | $twc";
	} else {
		$ret = '0x'.sprintf("%06x",$data);
	}
	return $ret;
}

package Prima::VB::Types::point;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	$self-> {A} = $self-> {container}-> insert( SpinEdit =>
		origin   => [ 5, $h - 72],
		min      => -16383,
		max      => 16383,
		onChange => sub {
			$self-> change;
		},
	);

	$self-> {L} = $self-> {container}-> insert( Label =>
		origin => [ 5, $self-> {A}-> top + 5],
		text      => $self-> {id}.'.x:',
		focusLink => $self-> {A},
	);


	$self-> {B} = $self-> {container}-> insert( SpinEdit =>
		origin   => [ 5, $self-> {A}-> bottom - $self-> {container}-> font-> height - 39],
		min      => -16383,
		max      => 16383,
		onChange => sub {
			$self-> change;
		},
	);

	$self-> {M} = $self-> {container}-> insert( Label =>
		origin => [ 5, $self-> {B}-> top + 5],
		text      => $self-> {id}.'.y:',
		focusLink => $self-> {B},
	);
}

sub change_id {
	$_[0]-> {L}-> text( $_[0]-> {id}.'.x:');
	$_[0]-> {M}-> text( $_[0]-> {id}.'.y:');
}

sub set
{
	my ( $self, $data) = @_;
	$data = [] unless defined $data;
	$self-> {A}-> value( defined $data-> [0] ? $data-> [0] : 0);
	$self-> {B}-> value( defined $data-> [1] ? $data-> [1] : 0);
}

sub get
{
	my $self = $_[0];
	return [$self-> {A}-> value,$self-> {B}-> value];
}

sub write
{
	my ( $class, $id, $data) = @_;
	return '[ '.$data-> [0].', '.$data-> [1].']';
}

package Prima::VB::Types::upoint;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::point);

sub open
{
	my $self = shift;
	$self-> SUPER::open( @_);
	$self-> {A}-> min(0);
	$self-> {B}-> min(0);
}

package Prima::VB::Types::origin;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::point);

sub set
{
	my ( $self, $data) = @_;
	$data = [] unless defined $data;
	my $x = defined $data-> [0] ? $data-> [0] : 0;
	my $y = defined $data-> [1] ? $data-> [1] : 0;
	my @delta = $self-> {widget}-> get_o_delta;
	$self-> {A}-> value( $x - $delta[0]);
	$self-> {B}-> value( $y - $delta[1]);
}

sub get
{
	my $self = $_[0];
	my ( $x, $y) = ($self-> {A}-> value,$self-> {B}-> value);
	my @delta = $self-> {widget}-> get_o_delta;
	return [$x + $delta[0], $y + $delta[1]];
}

package Prima::VB::Types::rect;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::point);

sub open
{
	my $self = $_[0];
	$self-> SUPER::open(@_);

	$self-> {C} = $self-> {container}-> insert( SpinEdit =>
		origin   => [ 5, $self-> {B}-> bottom - $self-> {container}-> font-> height - 39],
		min      => -16383,
		max      => 16383,
		onChange => sub {
			$self-> change;
		},
	);

	$self-> {N} = $self-> {container}-> insert( Label =>
		origin => [ 5, $self-> {C}-> top + 5],
		text      => $self-> {id}.'.x2:',
		focusLink => $self-> {C},
	);


	$self-> {D} = $self-> {container}-> insert( SpinEdit =>
		origin   => [ 5, $self-> {C}-> bottom - $self-> {container}-> font-> height - 39],
		min      => -16383,
		max      => 16383,
		onChange => sub {
			$self-> change;
		},
	);

	$self-> {O} = $self-> {container}-> insert( Label =>
		origin => [ 5, $self-> {D}-> top + 5],
		text      => $self-> {id}.'.y2:',
		focusLink => $self-> {D},
	);
}

sub change_id {
	my $self = shift;
	$self-> SUPER::change_id( @_);
	$self-> {N}-> text( $self-> {id}.'.x2:');
	$self-> {O}-> text( $self-> {id}.'.y2:');
}

sub set
{
	my ( $self, $data) = @_;
	$data = [] unless defined $data;
	$self-> SUPER::set( $data);
	$self-> {C}-> value( defined $data-> [2] ? $data-> [2] : 0);
	$self-> {D}-> value( defined $data-> [3] ? $data-> [3] : 0);
}

sub get
{
	my $self = $_[0];
	return [$self-> {A}-> value,$self-> {B}-> value,$self-> {C}-> value,$self-> {D}-> value];
}

sub write
{
	my ( $class, $id, $data) = @_;
	return '[ '.$data-> [0].', '.$data-> [1].','.$data-> [2].','.$data-> [3].']';
}

package Prima::VB::Types::urect;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::rect);

sub open
{
	my $self = shift;
	$self-> SUPER::open( @_);
	$self-> {$_}-> min(0) for qw(A B C D);
}

package Prima::VB::Types::cluster;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open_indirect
{
	my ( $self, %extraProfile) = @_;
	my $h = $self-> {container}-> height;
	$self-> {A} = $self-> {container}-> insert( ListBox =>
		left   => 5,
		top    => $h - 5,
		width  => $self-> {container}-> width - 9,
		height => $h - 10,
		growMode => gm::Client,
		onSelectItem => sub {
			$self-> change;
			$self-> on_change();
		},
		items => [$self-> IDS],
		%extraProfile,
	);
}

sub IDS    {}
sub packID {}
sub on_change {}

package Prima::VB::Types::strings;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::cluster);

sub open   { shift-> open_indirect }

sub set
{
	my ( $self, $data) = @_;
	my $i = 0;
	$self-> {A}-> focusedItem(-1), return unless defined $data;
	for ( $self-> IDS) {
		if ( $_ eq $data) {
			$self-> {A}-> focusedItem($i);
			last;
		}
		$i++;
	}
}

sub get
{
	my $self = $_[0];
	my @IDS = $self-> IDS;
	my $ix  = $self-> {A}-> focusedItem;
	return $IDS[($ix < 0) ? 0 : $ix];
}

package Prima::VB::Types::radio;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::cluster);

sub open   { shift-> open_indirect }
sub IDS    {}
sub packID {}

sub set
{
	my ( $self, $data) = @_;
	my $i = 0;
	$self-> {A}-> focusedItem(-1), return unless defined $data;
	my $packID = $self-> packID;
	for ( $self-> IDS) {
		if ( $packID-> $_() == $data) {
			$self-> {A}-> focusedItem($i);
			last;
		}
		$i++;
	}
}

sub get
{
	my $self = $_[0];
	my @IDS = $self-> IDS;
	my $ix  = $self-> {A}-> focusedItem;
	return 0 if $ix < 0;
	$ix = $IDS[$ix];
	return $self-> packID-> $ix();
}

sub write
{
	my ( $class, $id, $data) = @_;
	my $packID = $class-> packID;
	for ( $class-> IDS) {
		if ( $packID-> $_() == $data) {
			return $packID.'::'.$_;
		}
	}
	return $data;
}

package Prima::VB::Types::checkbox;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::cluster);

sub open
{
	my $self = $_[0];
	$self-> open_indirect(
		multiSelect => 1,
		extendedSelect => 0,
	);
}

sub set
{
	my ( $self, $data) = @_;
	my $i = 0;
	my $packID = $self-> packID;
	my @seli   = ();
	for ( $self-> IDS) {
		push ( @seli, $i) if $data & $packID-> $_();
		$i++;
	}
	$self-> {A}-> selectedItems(\@seli);
}

sub get
{
	my $self = $_[0];
	my @IDS  = $self-> IDS;
	my $res  = 0;
	my $packID = $self-> packID;
	for ( @{$self-> {A}-> selectedItems}) {
		my $ix = $IDS[ $_];
		$res |= $packID-> $ix();
	}
	return $res;
}

sub write
{
	my ( $class, $id, $data) = @_;
	my $packID = $class-> packID;
	my $i = 0;
	my $res;
	for ( $class-> IDS) {
		if ( $data & $packID-> $_()) {
			$res = (( defined $res) ? "$res | " : '').$packID."::$_";
		}
		$i++;
	}
	return defined $res ? $res : 0;
}


package Prima::VB::Types::borderStyle;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(None Sizeable Single Dialog); }
sub packID { 'bs'; }

package Prima::VB::Types::align;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Left Center Right); }
sub packID { 'ta'; }

package Prima::VB::Types::valign;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Top Middle Bottom); }
sub packID { 'ta'; }

package Prima::VB::Types::windowState;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Normal Minimized Maximized); }
sub packID { 'ws'; }

package Prima::VB::Types::borderIcons;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::checkbox);
sub IDS    { qw(SystemMenu Minimize Maximize TitleBar); }
sub packID { 'bi'; }

package Prima::VB::Types::selectingButtons;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::checkbox);
sub IDS    { qw(Left Middle Right); }
sub packID { 'mb'; }

package Prima::VB::Types::widgetClass;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Button CheckBox Combo Dialog Edit InputLine Label ListBox Menu
	Popup Radio ScrollBar Slider Custom Window); }
sub packID { 'wc'; }

package Prima::VB::Types::rop;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(CopyPut Blackness NotOr NotSrcAnd NotPut NotDestAnd Invert
XorPut NotAnd AndPut NotXor NoOper NotSrcOr NotDestOr OrPut Whiteness NotSrcXor
NotDestXor ); }

sub packID { 'rop'; }

package Prima::VB::Types::comboStyle;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Simple DropDown DropDownList); }
sub packID { 'cs'; }
sub preload_modules { return 'Prima::ComboBox' };

package Prima::VB::Types::gaugeRelief;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Sink Border Raise); }
sub packID { 'gr'; }
sub preload_modules { return 'Prima::Sliders' };

package Prima::VB::Types::sliderScheme;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Gauge Axis Thermometer StdMinMax); }
sub packID { 'ss'; }
sub preload_modules { return 'Prima::Sliders' };

package Prima::VB::Types::tickAlign;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(Normal Alternative Dual); }
sub packID { 'tka'; }
sub preload_modules { return 'Prima::Sliders' };

package Prima::VB::Types::growMode;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::checkbox);
sub IDS    { qw( GrowLoX GrowLoY GrowHiX GrowHiY XCenter YCenter DontCare); }
sub packID { 'gm'; }

sub open
{
	my $self = shift;
	$self-> SUPER::open( @_);
	my $fh = $self-> {A}-> font-> height;
	$self-> {A}-> set(
		bottom => $self-> {A}-> bottom + $fh * 5 + 5,
		height => $self-> {A}-> height - $fh * 5 - 5,
	);
	my @ai = qw(Client Right Left Floor Ceiling);
	my $hx = $self-> {A}-> bottom;
	my $wx = $self-> {A}-> width;
	for ( @ai) {
		$hx -= $fh + 1;
		my $xgdata = $_;
		$self-> {container}-> insert( Button =>
			origin => [ 5, $hx],
			size   => [ $wx, $fh],
			text   => $xgdata,
			growMode => gm::GrowHiX,
			onClick => sub {
				my $xg = $self-> get & ~gm::GrowAll;
				$self-> set( $xg | &{$gm::{$xgdata}}());
			},
		);
	}
}

package Prima::VB::Types::geometry;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw(GrowMode Pack Place) }

sub packID { 'gt'; }

package Prima::VB::Types::font;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

my @pitches = qw(Default Fixed Variable);
my @styles  = qw(Bold Italic Underlined StruckOut);

sub refresh_fontlist
{
	my $self = $_[0];
	my %fontList  = ();
	my @fontItems = qw();

	for ( sort { $a-> {name} cmp $b-> {name}} @{$::application-> fonts})
	{
		$fontList{$_-> {name}} = $_;
		push ( @fontItems, $_-> {name});
	}

	$self-> {fontList}  = \%fontList;
	$self-> {fontItems} = \@fontItems;

	$self-> {name}-> items( \@fontItems);
	#$self-> {name}-> text( $self-> {logFont}-> {name});
	#$self-> reset_sizelist;
}


sub open
{
	my $self = $_[0];
	my $i = $self-> {container};
	my @sz = $i-> size;
	$sz[1] -= 60;
	$self-> {marker} = $i-> insert( Button =>
		origin   => [ 5, $sz[1]],
		size     => [ $sz[0]-9, 56],
		growMode => gm::Ceiling,
		text     => 'AaBbCcZz...',
		onClick  => sub {
			my %f = %{$_[0]-> font};
			delete $f{height};
			my $f = VB::font_dialog( logFont => \%f );
			$_[0]-> font( $f-> logFont) if $f-> execute == mb::OK;
		},
		onFontChanged => sub {
			return if $self-> {sync};
			$self-> {sync} = 1;
			$self-> set( $_[0]-> font);
			$self-> {sync} = 0;
		},
	);

	my $fh = $self-> {container}-> font-> height;
	$sz[1] -= $fh + 4;
	$self-> {namex} = $i-> insert( CheckBox =>
		origin  => [ 5, $sz[1]],
		size    => [ $sz[0]-9, $fh + 2],
		growMode => gm::Ceiling,
		text    => '~Name',
		onClick => sub { $self-> on_change(); },
	);

	$sz[1] -= $fh + 4;
	$self-> {name} = $i-> insert( ComboBox =>
		origin   => [ 5, $sz[1]],
		size     => [ $sz[0]-9, $fh + 2],
		growMode => gm::Ceiling,
		style    => cs::DropDown,
		text     => '',
		onChange => sub { $self-> {namex}-> check; $self-> on_change(); },
	);

	$sz[1] -= $fh + 4;
	$self-> {sizex} = $i-> insert( CheckBox =>
		origin  => [ 5, $sz[1]],
		growMode => gm::Ceiling,
		size    => [ $sz[0]-9, $fh + 2],
		text    => '~Size',
		onClick => sub { $self-> on_change(); },
	);
	$sz[1] -= $fh + 6;
	$self-> {size} = $i-> insert( SpinEdit =>
		origin   => [ 5, $sz[1]],
		size     => [ $sz[0]-9, $fh + 4],
		growMode => gm::Ceiling,
		min      => 1,
		max      => 256,
		onChange => sub { $self-> {sizex}-> check;$self-> on_change(); },
	);
	$sz[1] -= $fh + 4;
	$self-> {stylex} = $i-> insert( CheckBox =>
		origin  => [ 5, $sz[1]],
		growMode => gm::Ceiling,
		size    => [ $sz[0]-9, $fh + 2],
		text    => 'St~yle',
		onClick => sub { $self-> on_change(); },
	);
	$sz[1] -= $fh * 5 + 28;
	$self-> {style} = $i-> insert( GroupBox =>
		origin   => [ 5, $sz[1]],
		size     => [ $sz[0]-9, $fh * 5 + 28],
		growMode => gm::Ceiling,
		style    => cs::DropDown,
		text     => '',
		onChange => sub { $self-> on_change(); },
	);
	my @esz = $self-> {style}-> size;
	$esz[1] -= $fh * 2 + 4;
	$self-> {styleBold} = $self-> {style}-> insert( CheckBox =>
		origin  => [ 8, $esz[1]],
		size    => [ $esz[0] - 16, $fh + 4],
		text    => '~Bold',
		growMode => gm::GrowHiX,
		onClick => sub { $self-> {stylex}-> check;$self-> on_change(); },
	);
	$esz[1] -= $fh + 6;
	$self-> {styleItalic} = $self-> {style}-> insert( CheckBox =>
		origin  => [ 8, $esz[1]],
		size    => [ $esz[0] - 16, $fh + 4],
		text    => '~Italic',
		growMode => gm::GrowHiX,
		onClick => sub { $self-> {stylex}-> check;$self-> on_change(); },
	);
	$esz[1] -= $fh + 6;
	$self-> {styleUnderlined} = $self-> {style}-> insert( CheckBox =>
		origin  => [ 8, $esz[1]],
		size    => [ $esz[0] - 16, $fh + 4],
		text    => '~Underlined',
		growMode => gm::GrowHiX,
		onClick => sub { $self-> {stylex}-> check;$self-> on_change(); },
	);
	$esz[1] -= $fh + 6;
	$self-> {styleStruckOut} = $self-> {style}-> insert( CheckBox =>
		origin  => [ 8, $esz[1]],
		size    => [ $esz[0] - 16, $fh + 4],
		text    => '~Strikeout',
		growMode => gm::GrowHiX,
		onClick => sub { $self-> {stylex}-> check;$self-> on_change(); },
	);

	$sz[1] -= $fh + 4;
	$self-> {pitchx} = $i-> insert( CheckBox =>
		origin  => [ 5, $sz[1]],
		size    => [ $sz[0]-9, $fh + 2],
		growMode => gm::Ceiling,
		text    => '~Pitch',
		onClick => sub { $self-> on_change(); },
	);

	$sz[1] -= $fh + 4;
	$self-> {pitch} = $i-> insert( ComboBox =>
		origin   => [ 5, $sz[1]],
		size     => [ $sz[0]-9, $fh + 2],
		growMode => gm::Ceiling,
		style    => cs::DropDownList,
		items    => \@pitches,
		onChange => sub { $self-> {pitchx}-> check;$self-> on_change(); },
	);

	$self-> refresh_fontlist;
}

sub on_change
{
	my $self = $_[0];
	$self-> change;
	$self-> {sync} = 1;
	$self-> {marker}-> font( $self-> {marker}-> font_match(
		$self-> get,
		$self-> {widget}-> {default}-> {$self-> {id}},
	));
	$self-> {sync} = undef;
}

sub set
{
	my ( $self, $data) = @_;
	$self-> {sync}=1;
	my %ndata = ();
	my $def = $self-> {widget}-> {default}-> {$self-> {id}};
	for ( qw( name size style pitch)) {
		$self-> {$_.'x'}-> checked( exists $data-> {$_});
		$ndata{$_} = exists $data-> {$_}  ? $data-> {$_} : $def-> {$_};
	}
	$self-> {name}-> text( $ndata{name});
	$self-> {size}-> value( $ndata{size});
	for ( @pitches) {
		if ( &{$fp::{$_}}() == $ndata{pitch}) {
			$self-> {pitch}-> text( $_);
			last;
		}
	}
	for ( @styles) {
		$self-> {"style$_"}-> checked( &{$fs::{$_}}() & $ndata{style});
	}
	$self-> {sync}=0;
}

sub get
{
	my $self = $_[0];
	my $ret  = {};
	$ret-> {name}  = $self-> {name}-> text  if $self-> {namex}-> checked;
	$ret-> {size}  = $self-> {size}-> value if $self-> {sizex}-> checked;
	if ( $self-> {pitchx}-> checked) {
		my $i = $self-> {pitch}-> text;
		$ret-> {pitch} = &{$fp::{$i}}();
	}
	if ( $self-> {stylex}-> checked) {
		my $o = 0;
		for ( @styles) {
			$o |= &{$fs::{$_}}() if $self-> {"style$_"}-> checked;
		}
		$ret-> {style} = $o;
	}
	return $ret;
}

sub write
{
	my ( $class, $id, $data) = @_;
	my $ret = '{';
	$ret .= "name => '".Prima::VB::Types::generic::quotable($data-> {name})."', "
		if exists $data-> {name};
	$ret .= 'size => '.$data-> {size}.', ' if exists $data-> {size};
	if ( exists $data-> {style}) {
		my $s;
		for ( @styles) {
			if ( &{$fs::{$_}}() & $data-> {style}) {
				$s = ( defined $s ? ($s.'|') : '').'fs::'.$_;
			}
		}
		$s = '0' unless defined $s;
		$ret .= 'style => '.$s.', ';
	}
	if ( exists $data-> {pitch}) {
		for ( @pitches) {
			if ( &{$fp::{$_}}() == $data-> {pitch}) {
				$ret .= 'pitch => fp::'.$_;
			}
		}
	}
	return $ret.'}';
}

package Prima::VB::Types::icon;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub open
{
	my $self = $_[0];
	my $i = $self-> {container};
	my @sz = $i-> size;

	my $fh = $i-> font-> height;
	my $sz = 27 * $::application-> uiScaling;
	$self-> {A} = $i-> insert( Widget =>
		origin   => [ 5, 5],
		size     => [ $sz[0]-10, $sz[1] - 10 - $sz],
		growMode => gm::Client,
		onPaint  => sub {
			my ( $self, $canvas) = @_;
			my @s = $canvas-> size;
			$canvas-> color( $self-> backColor);
			$canvas-> bar( 0,0, @s);
			if ( $self-> {icon}) {
				my @is = $self-> {icon}-> size;
				$self-> put_image(
					($s[0] - $is[0])/2,
					($s[1] - $is[1])/2,
					$self-> {icon});
			} else {
				$canvas-> color( cl::Black);
				$canvas-> rectangle(0,0,$s[0]-1,$s[1]-1);
				$canvas-> line(0,0,$s[0]-1,$s[1]-1);
				$canvas-> line(0,$s[1]-1,$s[0]-1,0);
			}
		},
	);

	$i-> insert( SpeedButton =>
		origin => [ 5 + 0 * ( 1 + $sz), $sz[1] - $sz - 1],
		size   => [ $sz, $sz],
		hint   => 'Load',
		flat   => 1,
		image  => $VB::main-> {openbutton}-> image,
		glyphs => $VB::main-> {openbutton}-> glyphs,
		growMode => gm::GrowLoY,
		onClick => sub {
			my @r = VB::image_open_dialog-> load(
				className => $self-> imgClass,
				loadAll   => 1
			);
			return unless $r[-1];
			my $i = $r[0];
			my ( $maxH, $maxW) = (0,0);
			if ( @r > 1) {
				for ( @r) {
					my @sz = $_-> size;
					$maxH = $sz[1] if $sz[1] > $maxH;
					$maxW = $sz[0] if $sz[0] > $maxW;
				}
				$maxW += 2;
				$maxH += 2;
				my $dd = Prima::Dialog-> create(
					centered => 1,
					visible  => 0,
					borderStyle => bs::Sizeable,
					size     => [ 300, 300],
					name     => 'Select image',
				);
				my $l = $dd-> insert( ListViewer =>
					size     => [ $dd-> size],
					origin   => [ 0,0],
					itemHeight => $maxH,
					itemWidth  => $maxW,
					multiColumn => 1,
					autoWidth   => 0,
					growMode    => gm::Client,
					onDrawItem => sub {
						my (
							$self, $canvas, $index,
							$left, $bottom, $right,
							$top, $hilite, $focusedItem) = @_;
						my $bc;
						if ( $hilite) {
							$bc = $self-> backColor;
							$self-> backColor( $self-> hiliteBackColor);
						}
						$canvas-> clear( $left, $bottom, $right, $top);
						$canvas-> put_image( $left, $bottom, $r[$index]);
						$self-> backColor( $bc) if $hilite;
					},
					onClick => sub {
						my $self = $_[0];
						$i = $r[ $self-> focusedItem];
						$dd-> ok;
					},
				);
				$l-> set_count( scalar @r);
				if ( $dd-> execute == mb::OK) {
					$self-> set( $i);
					$self-> change;
				}
				$dd-> destroy;
			} else {
				$self-> set( $i);
				$self-> change;
			}
		},
	);
	$self-> {B} = $i-> insert( SpeedButton =>
		origin => [ 5 + 1 * ( 1 + $sz), $sz[1] - $sz - 1],
		size   => [ $sz, $sz],
		hint   => 'Save',
		flat   => 1,
		image  => $VB::main-> {savebutton}-> image,
		glyphs => $VB::main-> {savebutton}-> glyphs,
		growMode => gm::GrowLoY,
		onClick => sub {
			VB::image_save_dialog-> save( $self-> {A}-> {icon});
		},
	);
	$self-> {C} = $i-> insert( SpeedButton =>
		origin => [ 5 + 2 * ( 1 + $sz), $sz[1] - $sz - 1],
		size   => [ $sz, $sz],
		hint   => 'Clear',
		flat   => 1,
		glyphs => $VB::main-> {newbutton}-> glyphs,
		image  => $VB::main-> {newbutton}-> image,
		growMode => gm::GrowLoY,
		onClick => sub {
			$self-> set( undef);
			$self-> change;
		},
	);
}

sub imgClass
{
	return 'Prima::Icon';
}

sub set
{
	my ( $self, $data) = @_;
	$self-> {A}-> {icon} = $data;
	$self-> {A}-> repaint;
	$self-> {B}-> enabled( $data);
	$self-> {C}-> enabled( $data);
}

sub get
{
	my ( $self) = @_;
	return $self-> {A}-> {icon};
}


sub write
{
	my ( $class, $id, $data) = @_;
	my $c = $class-> imgClass.'-> create( '.
		'width=>'.$data-> width.
		', height=>'.$data-> height;
	my $type = $data-> type;
	my $xc = '';
	for ( qw(GrayScale RealNumber ComplexNumber TrigComplexNumber SignedInt)) {
		$xc = "im::$_ | " if &{$im::{$_}}() & $type;
	}
	$xc .= 'im::bpp'.( $type & im::BPP);
	$c .= ", type => $xc";
	my $p = $data-> palette;
	$c .= ", \npalette => [ ".join(',', @$p).']' if scalar @$p;
	$p = $data-> data;
	my $i = 0;
	$xc = length( $p);
	$c .= ",\n data => \n";
	for ( $i = 0; $i < $xc; $i += 20) {
		my $a = Prima::VB::Types::generic::printable( substr( $p, $i, 20));
		$c .= "\"$a\".\n";
	}
	$c .= "'')";
	return $c;
}


package Prima::VB::Types::image;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::icon);

sub imgClass
{
	return 'Prima::Image';
}


package Prima::VB::Types::items;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::text);

sub set
{
	my ( $self, $data) = @_;
	$self-> {A}-> text( join( "\n", @$data));
}

sub get
{
	return [ split( "\n", $_[0]-> {A}-> text)];
}

sub write
{
	my ( $class, $id, $data) = @_;
	my $r = '[';
	for ( @$data) {
		$r .= "'".Prima::VB::Types::generic::quotable($_)."', ";
	}
	$r .= ']';
	return $r;
}

package Prima::VB::Types::multiItems;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::items);

sub set
{
	my ( $self, $data) = @_;
	$self-> {A}-> text( join( "\n", map { join( ' ', map {
		my $x = $_; $x =~ s/(^|[^\\])(\\|\s)/$1\\$2/g; $x; } @$_)} @$data));
}

sub get
{
	my @ret;
	for ( split( "\n", $_[0]-> {A}-> text)) {
		my @x;
		while (m<((?:[^\s\\]|(?:\\\s))+)\s*|(\S+)\s*|\s+>gx) {
			push @x, $+ if defined $+;
		}
		@x = map { s/\\(\\|\s)/$1/g; $_ } @x;
		push( @ret, \@x);
	}
	return \@ret;
}

sub write
{
	my ( $class, $id, $data) = @_;
	my $r = '[';
	for ( @$data) {
		$r .= '[';
		$r .= "'".Prima::VB::Types::generic::quotable($_)."', " for @$_;
		$r .= '],';
	}
	$r .= ']';
	return $r;
}


package Prima::VB::Types::event;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::text);

sub open
{
	my $self = shift;
	$self-> SUPER::open( @_);
	my $i = $VB::main-> {iniFile}-> section('Editor');
	$self-> {A}-> font ( map { $_,  $i-> {'Font' . ucfirst($_)}} qw(name size style encoding) );
	$self-> {A}-> syntaxHilite(1);
	push @Prima::VB::CodeEditor::editors, $self-> {A};
	$self-> {A}-> onDestroy( sub {
		my $self = $_[0];
		@Prima::VB::CodeEditor::editors = grep { $_ != $self } @Prima::VB::CodeEditor::editors;
	}) ;
}


sub write
{
	no warnings 'once';
	my ( $class, $id, $data) = @_;
	return $VB::writeMode ? "sub { $data\n}" :
		'Prima::VB::VBLoader::GO_SUB(\''.Prima::VB::Types::generic::quotable($data).
		"\n','$Prima::VB::VBLoader::eventContext[0]', '$id')";
}

package Prima::VB::Types::FMAction;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::event);


package Prima::VB::PackPropListViewer;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::PropListViewer);

sub on_click
{
	my $self = $_[0];
	my $index = $self-> focusedItem;
	my $master = $self-> {master};
	my $id = $self-> {'id'}-> [$index];
	$self-> SUPER::on_click;
	if ( $self-> {check}-> [$index]) {
		$master-> {data}-> {$id} = $master-> {prop_defaults}-> {$id};
		$self-> {master}-> item_changed;
	} else {
		$self-> {master}-> item_deleted;
	}
}

package Prima::VB::Types::pack_fill;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::strings);
sub IDS  { qw(none x y both) }

package Prima::VB::Types::pack_anchor;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::strings);
sub IDS  { qw(nw n ne w center e sw s e) }

package Prima::VB::Types::pack_side;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::strings);
sub IDS  { qw(top bottom left right) }

package Prima::VB::Types::packInfo;
use strict;
use vars qw(@ISA %packProps %packDefaults);
@ISA = qw(Prima::VB::Types::generic);

%packProps = (
	after     => 'Handle',
	before    => 'Handle',
	anchor    => 'pack_anchor',
	expand    => 'bool',
	fill      => 'pack_fill',
	padx      => 'iv',
	pady      => 'iv',
	ipadx     => 'iv',
	ipady     => 'iv',
	side      => 'pack_side',
);

%packDefaults = (
	after     => undef,
	before    => undef,
	anchor    => 'center',
	expand    => 0,
	fill      => 'none',
	padx      => 0,
	pady      => 0,
	ipadx     => 0,
	ipady     => 0,
	side      => 'top',
);

sub name { $_[0]-> {widget}-> name} # proxy call for Handle

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	my $w = $self-> {container}-> width;
	my $fh = $self-> {container}-> font-> height;

	my $divx = $h / 2;
	$self-> {A} = $self-> {container}-> insert( 'Prima::VB::PackPropListViewer' =>
		origin => [ 0, $divx + 6],
		size   => [ $w, $h - $divx - 6],
		growMode => gm::Ceiling,
		onSelectItem => sub {
			$self-> close_item;
			$self-> open_item;
		},
	);
	$self-> {A}-> {master} = $self;
	$self-> {prop_defaults} = \%packDefaults;

	$self-> {Div1} = $self-> {container}-> insert( 'Prima::VB::Divider' =>
		vertical => 0,
		origin => [ 0, $divx],
		size   => [ $w, 6],
		min    => 20,
		max    => 20,
		name   => 'Div',
		growMode => gm::Ceiling,
		onChange => sub {
			my $bottom = $_[0]-> bottom;
			$self-> {panel}-> height( $bottom);
			$self-> {A}-> set(
				top    => $self-> {container}-> height,
				bottom => $bottom + 6,
			);
		}
	);

	$self-> {panel} = $self-> {container}-> insert( Notebook =>
		origin    => [ 0, 0],
		size      => [ $w, $divx],
		growMode  => gm::Client,
		name      => 'Panel',
		pageCount => 1,
	);
	$self-> {panel}-> {pages} = {};
	$self-> {data} = {};
}

sub close_item
{
	my ( $self ) = @_;
	return unless defined $self-> {opened};
	$self-> {opened} = undef;
}

sub open_item
{
	my ( $self) = @_;
	return if defined $self-> {opened};
	my $list = $self-> {A};
	my $f = $list-> focusedItem;

	if ( $f < 0) {
		$self-> {panel}-> pageIndex(0);
		return;
	}
	my $id   = $list-> {id}-> [$f];
	my $type = $VB::main-> get_typerec( $packProps{ $id});
	my $p = $self-> {panel};
	my $pageset;
	if ( exists $p-> {pages}-> {$type}) {
		$self-> {opened} = $self-> {typeCache}-> {$type};
		$pageset = $p-> {pages}-> {$type};
		$self-> {opened}-> renew( $id, $self);
	} else {
		$p-> pageCount( $p-> pageCount + 1);
		$p-> pageIndex( $p-> pageCount - 1);
		$p-> {pages}-> {$type} = $p-> pageIndex;
		$self-> {opened} = $type-> new( $p, $id, $self);
		$self-> {opened}-> {changeProc} =
			\&Prima::VB::Types::packInfo::item_changed_from_notebook;
		$self-> {typeCache}-> {$type} = $self-> {opened};
	}
	my $data = exists $self->{data}-> {$id} ? $self->{data}-> {$id} : $packDefaults{$id};
	$self-> {sync} = 1;
	$self-> {opened}-> set( $data);
	$self-> {sync} = undef;
	$p-> pageIndex( $pageset) if defined $pageset;
}

sub item_changed_from_notebook
{
	item_changed( $_[0]-> {widget});
}

sub item_deleted
{
	my $self = $_[0];
	return unless $self;
	return unless $self-> {opened};
	return if $self-> {sync};
	$self-> {sync} = 1;
	my $id = $self-> {A}-> {id}-> [$self-> {A}-> focusedItem];
	$self-> {opened}-> set( $packDefaults{$id});
	my $list = $self-> {A};
	my $ix = $list-> {index}-> {$self-> {opened}-> {id}};
	if ( $list-> {check}-> [$ix]) {
		$list-> {check}-> [$ix] = 0;
		$list-> redraw_items( $ix);
	}
	$self-> change;
	$self-> {sync} = 0;
}

sub item_changed
{
	my $self = $_[0];
	return unless $self;
	return unless $self-> {opened};
	return if $self-> {sync};
	return unless $self-> {opened}-> valid;
	return unless $self-> {opened}-> can( 'get');

	$self-> {sync} = 1;
	my $list = $self-> {A};
	my $data = $self-> {opened}-> get;
	my @redraw;
	if ( $self-> {opened}-> {id} eq 'after') {
		delete $self-> {data}-> {before};
		push @redraw, $list-> {index}-> {before};
		$list-> {check}-> [ $redraw[-1] ] = 0;
	} elsif ( $self-> {opened}-> {id} eq 'before') {
		delete $self-> {data}-> {after};
		push @redraw, $list-> {index}-> {after};
		$list-> {check}-> [ $redraw[-1] ] = 0;
	}

	$self-> {data}-> { $self-> {opened}-> {id} } = $data;
	my $ix = $list-> {index}-> {$self-> {opened}-> {id}};
	unless ( $list-> {check}-> [$ix]) {
		$list-> {check}-> [$ix] = 1;
		push @redraw, $ix;
	}
	$list-> redraw_items( @redraw) if @redraw;
	$self-> change;
	$self-> {sync} = undef;
}

sub set
{
	my ( $self, $data) = @_;
	$self-> {sync} = 1;
	$self-> {data} = $data ? {%$data} : {};

	my $l = $self-> {A};
	my @id = sort keys %packProps;
	my @chk = ();
	my %ix  = ();
	my $num = 0;
	for ( @id) {
		push( @chk, exists $self->{data}->{$_} ? 1 : 0);
		$ix{$_} = $num++;
	}
	$l-> reset_items( \@id, \@chk, \%ix);

	if ( $self-> {opened}) {
		my $id   = $self-> {opened}-> {id};
		my $data = exists $self->{data}-> {$id} ? $self->{data}-> {$id} : $packDefaults{$id};
		$self-> {sync} = 1;
		$self-> {opened}-> set( $data);
		$self-> {sync} = undef;
	}

	$self-> {sync} = 0;
}

sub get
{
	my $self = $_[0];
	my %d = %{ $self-> {data}};
	return \%d;
}

package Prima::VB::MyOutline;
use strict;

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	$self-> SUPER::on_keydown( $code, $key, $mod);
	$mod &= (km::Ctrl|km::Shift|km::Alt);
	if ( $key == kb::Delete && $mod == 0) {
		$self-> del;
		$self-> clear_event;
		return;
	}
	if ( $key == kb::Insert && $mod == 0) {
		$self-> new;
		$self-> clear_event;
		return;
	}
}

sub new_item
{
}

sub newnode
{
	my $f = $_[0]-> focusedItem;
	my ( $x, $l) = $_[0]-> get_item( $f);
	my ( $p, $o) = $_[0]-> get_item_parent( $x);
	$o = -1 unless defined $o;
	$_[0]-> insert_items( $p, $o + 1, $_[0]-> new_item);
	$_[0]-> {master}-> change;
	( $x, $l) = $_[0]-> get_item( $f + 1);
	$_[0]-> focusedItem( $f + 1);
	$_[0]-> {master}-> enter_menuitem( $x);
}

sub makenode
{
	my $f = $_[0]-> focusedItem;
	my ( $x, $l) = $_[0]-> get_item( $f);
	return if !$x;
	if ( $x-> [1]) {
		splice( @{$x-> [1]}, 0, 0, $_[0]-> new_item);
		$_[0]-> reset_tree;
		$_[0]-> update_tree;
		$_[0]-> repaint;
		$_[0]-> reset_scrolls;
	} else {
		$x-> [1] = [$_[0]-> new_item];
		$x-> [2] = 0;
	}
	$_[0]-> adjust( $f, 1);
	$_[0]-> {master}-> change;
	( $x, $l) = $_[0]-> get_item( $f + 1);
	$_[0]-> focusedItem( $f + 1);
	$_[0]-> {master}-> enter_menuitem( $x);
}

sub del
{
	my $f = $_[0]-> focusedItem;
	my ( $x, $l) = $_[0]-> get_item( $f);
	$_[0]-> delete_item( $x);
	$_[0]-> {master}-> change;
	$f-- if $f;
	( $x, $l) = $_[0]-> get_item( $f);
	$_[0]-> focusedItem( $f);
	$_[0]-> {master}-> enter_menuitem( $x);
}


sub on_dragitem
{
	my $self = shift;
	$self-> SUPER::on_dragitem( @_);
	$self-> {master}-> change;
}

package Prima::VB::MenuOutline;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::Outline Prima::VB::MyOutline);

sub new_item
{
	return  [['New Item', { text => 'New Item',
		action => $Prima::VB::Types::menuItems::menuDefaults{action}}], undef, 0];
}

sub makeseparator
{
	my $f = $_[0]-> focusedItem;
	my ( $x, $l) = $_[0]-> get_item( $f);
	return if !$x;
	$x-> [0][0] = '---';
	$x-> [0][1] = {};
	$_[0]-> repaint;
	$_[0]-> {master}-> change;
	$_[0]-> {master}-> {current} = undef;
	$_[0]-> {master}-> enter_menuitem( $x);
}

package Prima::VB::MPropListViewer;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::PropListViewer);

sub on_click
{
	my $self = $_[0];
	my $index = $self-> focusedItem;
	my $current = $self-> {master}-> {current};
	return if $index < 0 or !$current;
	my $id = $self-> {'id'}-> [$index];
	$self-> SUPER::on_click;
	if ( $self-> {check}-> [$index]) {
		$current-> [0]-> [1]-> {$id} = $Prima::VB::Types::menuItems::menuDefaults{$id};
		$self-> {master}-> item_changed;
	} else {
		delete $current-> [0]-> [1]-> {$id};
		$self-> {master}-> item_deleted;
	}
}

package Prima::VB::Types::menuItems;
use strict;
use vars qw(@ISA %menuProps %menuDefaults);
@ISA = qw(Prima::VB::Types::generic);


%menuProps = (
	'key'     => 'key',
	'accel'   => 'string',
	'text'    => 'string',
	'name'    => 'menuname',
	'enabled' => 'bool',
	'checked' => 'bool',
	'image'   => 'image',
	'action'  => 'event',
);

%menuDefaults = (
	'key'     => kb::NoKey,
	'accel'   => '',
	'text'    => '',
	'name'    => 'MenuItem',
	'enabled' => 1,
	'checked' => 0,
	'image'   => undef,
	'action'  => 'my ( $self, $item) = @_;',
);


sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	my $w = $self-> {container}-> width;
	my $fh = $self-> {container}-> font-> height;

	my $divx = $h / 2;
	$self-> {A} = $self-> {container}-> insert( 'Prima::VB::MPropListViewer' =>
		origin => [ 0, 0],
		size   => [ 100, $divx],
		growMode => gm::Client,
		onSelectItem => sub {
			$self-> close_item;
			$self-> open_item;
		},
	);
	$self-> {A}-> {master} = $self;

	$self-> {Div1} = $self-> {container}-> insert( 'Prima::VB::Divider' =>
		vertical => 0,
		origin => [ 0, $divx],
		size   => [ 100, 6],
		min    => 20,
		max    => 20,
		name   => 'Div',
		growMode => gm::Ceiling,
		onChange => sub {
			my $bottom = $_[0]-> bottom;
			$self-> {A}-> height( $bottom);
			$self-> {B}-> set(
				top    => $self-> {container}-> height,
				bottom => $bottom + 6,
			);
		}
	);

	$self-> {B} = $self-> {container}-> insert( 'Prima::VB::MenuOutline' =>
		origin => [ 0, $divx + 6],
		size   => [ 100, $h - $divx - 6],
		growMode => gm::Ceiling,
		popupItems => [
			['~New' => q(newnode),],
			['~Make node' => q(makenode),],
			['Convert to ~separator' => q(makeseparator),],
			['~Delete' => q(del),],
		],
		onSelectItem => sub {
			my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
			$self-> enter_menuitem( $x);
		},
	);
	$self-> {B}-> {master} = $self;

	my $xb = $::application-> get_system_value( sv::XScrollbar);
	$self-> {B}-> insert( Button =>
		origin => [
			$self-> {B}-> indents()-> [0],
			$self-> {B}-> height - $xb - $self-> {B}-> indents()-> [3]
		],
		size   => [ ( $xb ) x 2],
		font   => { height => $xb - 4 * 0.8, style => fs::Bold },
		text   => 'X',
		growMode => gm::GrowLoY,
		onClick => sub { $self-> {B}-> popup-> popup($_[0]-> origin)},
	);

	$self-> {Div2} = $self-> {container}-> insert( 'Prima::VB::Divider' =>
		vertical => 1,
		origin => [ 100, 0],
		size   => [ 6, $h - 1],
		min    => 50,
		max    => 50,
		name   => 'Div',
		onChange => sub {
			my $right = $_[0]-> right;
			$self-> {A}-> width( $_[0]-> left);
			$self-> {Div1}-> width( $_[0]-> left);
			$self-> {B}-> width( $_[0]-> left);
			$self-> {panel}-> set(
				width => $self-> {container}-> width - $right,
				left  => $right,
			);
		}
	);

	$self-> {panel} = $self-> {container}-> insert( Notebook =>
		origin    => [ 106, 0],
		size      => [ $w - 106, $h - 1],
		growMode  => gm::Right,
		name      => 'Panel',
		pageCount => 1,
	);
	$self-> {panel}-> {pages} = {};
}

sub enter_menuitem
{
	my ( $self, $x ) = @_;
	if ( defined $x) {
		return if defined $self-> {current} and $self-> {current} == $x;
	} else {
		return unless defined $self-> {current};
	}
	$self-> {current} = $x;
	$self-> close_item;
	my $l = $self-> {A};
	if ( $self-> {current}) {
		my @id = sort keys %menuProps;
		my @chk = ();
		my %ix  = ();
		my $num = 0;
		for ( @id) {
			push( @chk, exists $x-> [0]-> [1]-> {$_} ? 1 : 0);
			$ix{$_} = $num++;
		}
		$l-> reset_items( \@id, \@chk, \%ix);
		$self-> open_item;
	} else {
		$l-> {id} = [];
		$l-> {check} = [];
		$l-> {index} = {};
		$l-> set_count( 0);
	}
}


sub close_item
{
	my ( $self ) = @_;
	return unless defined $self-> {opened};
	$self-> {opened} = undef;
}

sub open_item
{
	my ( $self) = @_;
	return if defined $self-> {opened};
	my $list = $self-> {A};
	my $f = $list-> focusedItem;

	if ( $f < 0) {
		$self-> {panel}-> pageIndex(0);
		return;
	}
	my $id   = $list-> {id}-> [$f];
	my $type = $VB::main-> get_typerec( $menuProps{ $id});
	my $p = $self-> {panel};
	my $pageset;
	if ( exists $p-> {pages}-> {$type}) {
		$self-> {opened} = $self-> {typeCache}-> {$type};
		$pageset = $p-> {pages}-> {$type};
		$self-> {opened}-> renew( $id, $self);
	} else {
		$p-> pageCount( $p-> pageCount + 1);
		$p-> pageIndex( $p-> pageCount - 1);
		$p-> {pages}-> {$type} = $p-> pageIndex;
		$self-> {opened} = $type-> new( $p, $id, $self);
		$self-> {opened}-> {changeProc} =
			\&Prima::VB::Types::menuItems::item_changed_from_notebook;
		$self-> {typeCache}-> {$type} = $self-> {opened};
	}
	my $drec = $self-> {current}-> [0]-> [1];
	my $data = exists $drec-> {$id} ? $drec-> {$id} : $menuDefaults{$id};
	$self-> {sync} = 1;
	$self-> {opened}-> set( $data);
	$self-> {sync} = undef;
	$p-> pageIndex( $pageset) if defined $pageset;
}

sub item_changed_from_notebook
{
	item_changed( $_[0]-> {widget});
}

sub item_deleted
{
	my $self = $_[0];
	return unless $self;
	return unless $self-> {opened};
	return if $self-> {sync};
	$self-> {sync} = 1;
	my $id = $self-> {A}-> {id}-> [$self-> {A}-> focusedItem];
	$self-> {opened}-> set( $menuDefaults{$id});
	$self-> change;
	my $hash = $self-> {current}-> [0]-> [1];
	if ( !exists $hash-> {name} && !exists $hash-> {text}) {
		my $newname = exists $hash-> {image} ? 'Image' : '---';
		if ( $self-> {current}-> [0]-> [0] ne $newname) {
			$self-> {current}-> [0]-> [0] = $newname;
			$self-> {B}-> reset_tree;
			$self-> {B}-> update_tree;
			$self-> {B}-> repaint;
		}
	}
	$self-> {sync} = 0;
}


sub item_changed
{
	my $self = $_[0];
	return unless $self;
	return unless $self-> {opened};
	return if $self-> {sync};
	if ( $self-> {opened}-> valid) {
		if ( $self-> {opened}-> can( 'get')) {

			my $list = $self-> {A};

			$self-> {sync} = 1;
			my $data = $self-> {opened}-> get;
			my $c = $self-> {opened}-> {widget}-> {current};
			$c-> [0]-> [1]-> {$self-> {opened}-> {id}} = $data;
			if ( $self-> {opened}-> {id} eq 'text') {
				$c-> [0]-> [0] = $data;
				$self-> {B}-> reset_tree;
				$self-> {B}-> update_tree;
				$self-> {B}-> repaint;
			} elsif (( $self-> {opened}-> {id} eq 'name') && !exists $c-> [0]-> [1]-> {text}) {
				$c-> [0]-> [0] = $data;
				$self-> {B}-> reset_tree;
				$self-> {B}-> update_tree;
				$self-> {B}-> repaint;
			} elsif  (( $self-> {opened}-> {id} eq 'key') && !exists $c-> [0]-> [1]-> {accel}) {
				$c-> [0]-> [1]-> {accel} = $menuDefaults{accel};
				$list-> {check}-> [$list-> {index}-> {accel}] = 1;
				$list-> redraw_items($list-> {index}-> {accel});
			} elsif  (( $self-> {opened}-> {id} eq 'accel') && !exists $c-> [0]-> [1]-> {key}) {
				$c-> [0]-> [1]-> {key} = $menuDefaults{key};
				$list-> {check}-> [$list-> {index}-> {key}] = 1;
				$list-> redraw_items($list-> {index}-> {key});
			} elsif  (( $self-> {opened}-> {id} eq 'image') && exists $c-> [0]-> [1]-> {text}) {
				delete $c-> [0]-> [1]-> {text};
				$list-> {check}-> [$list-> {index}-> {text}] = 0;
				$list-> redraw_items($list-> {index}-> {text});
			}

			my $ix = $list-> {index}-> {$self-> {opened}-> {id}};
			unless ( $list-> {check}-> [$ix]) {
				$list-> {check}-> [$ix] = 1;
				$list-> redraw_items( $ix);
			}
			$self-> change;
			$self-> {sync} = undef;
		}
	}

}

#use Data::Dumper;

sub set
{
	my ( $self, $data) = @_;
	$self-> {sync} = 1;
	my $setData = [];
	my $traverse;
	$traverse = sub {
		my ( $data, $set) = @_;
		my $c   = scalar @$data;
		my %items = ();
		if ( $c == 5) {
			@items{qw(name text accel key)} = @$data;
		} elsif ( $c == 4) {
			@items{qw(text accel key)} = @$data;
		} elsif ( $c == 3) {
			@items{qw(name text)} = @$data;
		} elsif ( $c == 2) {
			$items{text} = $$data[0];
		}
		if ( ref($items{text}) && $items{text}-> isa('Prima::Image')) {
			$items{image} = $items{text};
			delete $items{text};
		}
		if ( exists $items{name}) {
			$items{enabled} = 0 if $items{name} =~ s/^\-//;
			$items{checked} = 1 if $items{name} =~ s/^\*//;
			delete $items{name} unless length $items{name};
		}
		my @record = ([ defined $items{text} ? $items{text} :
		( defined $items{text} ? $items{text} :
			( defined $items{name} ? $items{name} :
				( defined $items{image} ? "Image" : "---")
		)) , \%items], undef, 0);
		if ( ref($$data[-1]) eq 'ARRAY') {
			$record[1] = [];
			$record[2] = 1;
			$traverse-> ( $_, $record[1]) for @{$$data[-1]};
		} elsif ( $c > 1) {
			$items{action} = $$data[-1];
		}
		push( @$set, \@record);
	};
	$traverse-> ( $_, $setData) for @$data;
	undef $traverse;
	$self-> {B}-> items( $setData);
	$self-> {sync} = 0;
}

sub change
{
	my $self = $_[0];
	$self-> SUPER::change;
	$VB::form-> menuItems( $self-> get) if $VB::form && $self-> {widget} == $VB::form;
}

sub get
{
	my $self = $_[0];
	my $retData = [];
	my $traverse;
	$traverse = sub {
		my ($current, $ret) = @_;
		my @in = ();
		my @cmd = ();
		my $haschild = 0;
		my $i = $current-> [0]-> [1];
		my $hastext  = exists $i-> {text};
		my $hasname  = exists $i-> {name};
		my $hasimage = exists $i-> {image};
		my $hasacc   = exists $i-> {accel};
		my $hasact   = exists $i-> {action};
		my $haskey   = exists $i-> {key};
		my $hassub   = exists $i-> {action};
		my $namepfx  = (( exists $i-> {enabled} && !$i-> {enabled}) ? '-' : '').
			(( exists $i-> {checked} &&  $i-> {checked}) ? '*' : '');

		if ( $current-> [1] && scalar @{$current-> [1]}) {
			$haschild = 1;
			$hasacc = $haskey = $hasact = 0;
			if ( $hastext || $hasimage) {
				push ( @cmd, qw(name text));
			}
		} elsif (( $hastext || $hasimage) && $hasacc && $haskey && $hasact) {
			push ( @cmd, qw( name text accel key action));
		} elsif (( $hastext || $hasimage) && $hasact) {
			push ( @cmd, qw( name text action));
		}

		for ( @cmd) {
			my $val = $i-> {$_};
			if (( $_ eq 'text') && $hasimage && !$hastext) {
				$_ = 'image';
				$val = $i-> {$_};
			} elsif ( $_ eq 'name') {
				$val = '' unless defined $val;
				$val = "$namepfx$val";
				next unless length $val;
			}
			push( @in, $val);
		}

		if ( $haschild) {
			my $cx = [];
			$traverse-> ( $_, $cx) for @{$current-> [1]};
			push @in, $cx;
		}
		push @$ret, \@in;
	};
	$traverse-> ( $_, $retData) for @{$self-> {B}-> items};
	undef $traverse;
	return $retData;
}


sub write
{
	my ( $class, $id, $data) = @_;
	return 'undef' unless defined $data;
	my $c = '';
	my $traverse;
	$traverse = sub {
		my ( $data, $level) = @_;
		my $sc    = scalar @$data;
		my @cmd   = ();
		if ( $sc == 5) {
			@cmd = qw( name text accel key);
		} elsif ( $sc == 4) {
			@cmd = qw( text accel key);
		} elsif ( $sc == 3) {
			@cmd = qw( name text);
		} elsif ( $sc == 2) {
			@cmd = qw( text);
		}

		$c .= ' ' x ( $level * 3).'[ ';
		my $i = 0;
		for ( @cmd) {
			if ( $_ eq 'text' && ref($$data[$i]) && $$data[$i]-> isa('Prima::Image')) {
				$_ = 'image';
			}
			my $type = $VB::main-> get_typerec( $menuProps{$_}, $$data[$i]);
			$c .= $type-> write( $_, $$data[$i]) . ', ';
			$i++;
		}

		if ( ref($$data[-1]) eq 'ARRAY') {
			$level++;
			$c .= "[\n";
			$traverse-> ( $_, $level) for @{$$data[-1]};
			$c .= ' ' x ( $level * 3).']';
			$level--;
		} elsif ( $sc > 1) {
			$c .= $VB::main-> get_typerec( $menuProps{'action'}, $$data[$i])->
				write( 'action', $$data[$i]);
		}
		$c .= "], \n";
	};
	$traverse-> ( $_, 0) for @$data;
	undef $traverse;
	return "\n[$c]";
}


package Prima::VB::Types::menuname;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::name);


sub valid
{
	my $self = $_[0];
	my $tx = $self-> {A}-> text;
	$self-> wake, return 0 unless length( $tx);
	$self-> wake, return 0
		if $tx =~ /[\s\\\~\!\@\#\$\%\^\&\*\(\)\-\+\=\[\]\{\}\.\,\?\;\|\`\'\"]/;
	my $s = $self-> {widget}-> {B};
	my $ok = 1;
	my $fi = $s-> focusedItem;
	$s-> iterate( sub {
		my ( $current, $position, $level) = @_;
		return 0 if $position == $fi;
		$ok = 0, return 1
			if defined $current-> [0]-> [1]-> {name} &&
			$current-> [0]-> [1]-> {name} eq $tx;
		return 0;
	}, 1);
	$self-> wake unless $ok;
	return $ok;
}

package Prima::VB::Types::key;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

use Prima::KeySelector;

sub open
{
	my $self = $_[0];
	my $i = $self-> {container};
	$self-> {A} = $i-> insert( KeySelector =>
		origin   => [ 5, 5],
		size     => [ $i-> width - 10, $i-> height - 10],
		growMode => gm::Ceiling,
		onChange => sub { $self-> change; },
	);
}

sub get
{
	return $_[0]-> {A}-> key;
}

sub set
{
	my ( $self, $data) = @_;
	$self-> {A}-> key( $data);
}

sub write
{
	my ( $class, $id, $data) = @_;
	return Prima::KeySelector::export( $data);
}


package Prima::VB::ItemsOutline;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::StringOutline Prima::VB::MyOutline);

sub new_item
{
	return  ['New Item', undef, 0];
}

package Prima::VB::Types::treeItems;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::generic);

sub outliner {'Prima::VB::ItemsOutline' }

sub open
{
	my $self = $_[0];
	my $h = $self-> {container}-> height;
	my $w = $self-> {container}-> width;
	my $fh = $self-> {container}-> font-> height;

	$self-> {A} = $self-> {container}-> insert( $self->outliner,
		origin => [ 0, $fh + 4],
		size   => [ $w - 1, $h - $fh - 4],
		growMode => gm::Client,
		popupItems => [
			['~New' => q(newnode),],
			['~Make node' => q(makenode),],
			['~Delete' => q(del),],
		],
		onSelectItem => sub {
			my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
			$self-> enter_menuitem( $x);
		},
	);
	$self-> {A}-> {master} = $self;
	my $xb = $self-> {A}-> {vScroll} ? $self-> {A}-> {vScrollBar}-> width : 0;
	$self-> {A}-> insert( Button =>
		origin => [
			$self-> {A}-> width - $xb - $self-> {A}-> indents()-> [2],
			$self-> {A}-> height - $xb - $self-> {A}-> indents()-> [3]
		],
		size   => [ ( $xb ) x 2],
		font   => { height => $xb - 4 * 0.8, style => fs::Bold },
		text   => 'X',
		growMode => gm::GrowLoX|gm::GrowLoY,
		onClick => sub { $self-> {A}-> popup-> popup($_[0]-> origin)},
	);
	$self-> {B} = $self-> {container}-> insert( InputLine =>
		origin => [ 0, 1],
		width  => $w,
		growMode => gm::Floor,
		text => '',
		onChange => sub {
			my ( $x, $l) = $self-> {A}-> get_item( $self-> {A}-> focusedItem);
			$self->on_b_change($x, $l, $self->{B}->text);
		},
	);
}

sub on_b_change
{
	my ( $self, $x, $l, $text) = @_;
	if ( $x) {
		$x-> [0] = $text;
		$self-> change;
		$self-> {A}-> reset_tree;
		$self-> {A}-> update_tree;
		$self-> {A}-> repaint;
	}
}

sub enter_menuitem
{
	my ( $self, $x ) = @_;
	$self-> {B}-> text( $x ? $x-> [0] : '');
}

sub get
{
	return $_[0]-> {A}-> items;
}

sub set
{
	return $_[0]-> {A}-> items( $_[1]);
}


sub write
{
	my ( $class, $id, $data) = @_;
	return '[]' unless $data;
	my $c = '';
	my $traverse;
	$traverse = sub {
		my ($x,$lev) = @_;
		$c .= ' ' x ( $lev * 3);
		$c .= "['". Prima::VB::Types::generic::quotable($x-> [0])."', ";
		if ( $x-> [1]) {
			$lev++;
			$c .= "[\n";
			$traverse-> ($_, $lev) for @{$x-> [1]};
			$lev--;
			$c .= ' ' x ( $lev * 3)."], $$x[2]";
		}
		$c .= "],\n";
	};
	$traverse-> ($_, 0) for @$data;
	undef $traverse;
	return "\n[$c]";
}

package Prima::VB::ArrayOutline;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::StringOutline Prima::VB::MyOutline);

sub new_item
{
	return  [['New', 'Item'], undef, 0];
}

sub draw_items
{
	my ($self, $canvas, $paintStruc) = @_;
	for ( @$paintStruc) {
		my ( $node, $left, $bottom, $right, $top, $position, $selected, $focused, $prelight) = @$_;
		if ( $selected || $prelight) {
			my $bc = $self->backColor;
			$self-> draw_item_background($canvas,
				$left, $bottom, $right, $top, $prelight,
				$selected ? $self-> hiliteBackColor : $self-> backColor
			);
			$self->backColor($bc);
			my $c = $canvas-> color;
			$canvas-> color( $selected ? $self-> hiliteColor : $c );
			$canvas-> text_shape_out( "@{$node->[0]}", $left, $bottom);
			$canvas-> color( $c);
		} else {
			$canvas-> text_shape_out( "@{$node->[0]}", $left, $bottom);
		}
		$canvas-> rect_focus( $left, $bottom, $right, $top) if $focused;
	}
}

sub on_measureitem
{
	my ( $self, $node, $level, $result) = @_;
	$$result = $self-> get_text_width( "@{$node->[0]}");
}

sub on_stringify
{
	my ( $self, $node, $result) = @_;
	$$result = "@{$node->[0]}";
}

package Prima::VB::Types::treeArrays;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::treeItems);

sub outliner {'Prima::VB::ArrayOutline' }

sub on_b_change
{
	my ( $self, $x, $l, $text) = @_;
	if ( $x) {
		$x-> [0] = [split ' ', $text];
		$self-> change;
		$self-> {A}-> reset_tree;
		$self-> {A}-> update_tree;
		$self-> {A}-> repaint;
	}
}

sub enter_menuitem
{
	my ( $self, $x ) = @_;
	$self-> {B}-> text( $x ? "@{$x->[0]}" : '');
}

sub write
{
	my ( $class, $id, $data) = @_;
	return '[[]]' unless $data;
	my $c = '';
	my $traverse;
	$traverse = sub {
		my ($x,$lev) = @_;
		$c .= ' ' x ( $lev * 3);
		$c .= '[[' .
			join(',',
				map {"'$_'" }
				map {Prima::VB::Types::generic::quotable($_)}
				@{$x->[0]}
			) .
			'], ';
		if ( $x-> [1]) {
			$lev++;
			$c .= "[\n";
			$traverse-> ($_, $lev) for @{$x-> [1]};
			$lev--;
			$c .= ' ' x ( $lev * 3)."], $$x[2]";
		}
		$c .= "],\n";
	};
	$traverse-> ($_, 0) for @$data;
	undef $traverse;
	return "\n[$c]";
}

1;

=pod

=head1 NAME

Prima::VB::Classes - Visual Builder widgets and types

=head1 DESCRIPTION

Visual Builder is designed without a prior knowledge of the
widget classes that would be contained in its widget palette.
Instead, it provides a registration interface for new widgets and their specific properties.

This document describes API, provided by the builder, and the widget
interface. Through the document, I<widget> or I<widget class> mean
not the original widget or class, but their representatives.

=head1 USAGE

The widget must provide specific methods to cooperate with the builder.
It is not required, however, to contain these methods in its base module
or package; it can delegate its representation to another, usually
very light class, which is used by the builder.

Such a class must be derived from C<Prima::VB::Object>, which provides
base functionality. One of basic features here is overloading of
property change method. Since the user can change any property interactively,
the class can catch the properties of interest by declaring C<prf_XXX>
method, where XXX is the property name. C<Prima::VB::Widget> declares
set of these methods, assuming that a widget would repaint when, for example,
its C<color> or C<font> properties change.

The hierarchy of VB classes mimics the one of the core toolkit classes,
but this is a mere resemblance, no other dependencies except the names
are present. The hierarchy is as follows:

	Prima::VB::Object   Prima::Widget
		Prima::VB::Component
			Prima::VB::Drawable
				Prima::VB::Widget
					Prima::VB::Control
						Prima::VB::Window

NB: C<Prima::VB::CoreClasses> extends the hierarchy to the full set of default
widget palette in the builder. This module is not provided with documentation
though since its function is obvious and its code is trivial.

Since the real widgets are used in the interaction with the builder,
their properties are not touched when changed by the object inspector
or otherwise. The widgets
keep the set of properties in a separated hash. The properties are
accessible by C<prf> and C<prf_set> methods.

A type object is a class used to represent a particular type of
property in object inspector window in the builder.
The type objects, like the widget classes, also are not hard-coded. The builder
presents a basic set of the type objects, which can be easily expanded.
The hierarchy ( incomplete ) of the type objects classes is as follows:

	Prima::VB::Types::generic
		Prima::VB::Types::bool
		Prima::VB::Types::color
		Prima::VB::Types::point
		Prima::VB::Types::icon
		Prima::VB::Types::Handle
		Prima::VB::Types::textee
			Prima::VB::Types::text
			Prima::VB::Types::string
				Prima::VB::Types::char
				Prima::VB::Types::name
				Prima::VB::Types::iv
					Prima::VB::Types::uiv

The document does not describe the types, since their function
can be observed at runtime in the object inspector.
Only C<Prima::VB::Types::generic> API is documented.

=head1 Prima::VB::Object

=head2 Properties

=over

=item class STRING

Selects the original widget class. Create-only property.

=item creationOrder INTEGER

Selects the creation order of the widget.

=item module STRING

Selects the module that contains the original widget class. Create-only property.

=item profile HASH

Selects the original widget profile. Create-only property.
Changes to profile at run-time performed by C<prf_set> method.

=back

=head2 Methods

=over

=item act_profile

Returns hash of callbacks to be stored in the form file and
executed by C<Prima::VB::VBLoader> when the form file is loaded.
The hash keys are names of VBLoader events and values - strings
with code to be eval'ed. See L<Prima::VB::VBLoader/Events>
for description and format of the callbacks.

Called when the builder writes a form file.

=item add_hooks @PROPERTIES

Registers the object as a watcher to set of PROPERTIES.
When any object changes a property listed in the hook record,
C<on_hook> callback is triggered.

Special name C<'DESTROY'> can be used to set a hook on object destruction event.


=item ext_profile

Returns a class-specific hash, written in a form file.
Its use is to serve as a set of extra parameters, passed from
the builder to C<act_profile> events.

=item prf_set %PROIFLE

A main method for setting a property of an object.
PROFILE keys are property names, and value are property values.

=item prf_adjust_default PROFILE, DEFAULT_PROFILE

DEFAULT_PROFILE is a result of C<profile_default> call
of the real object class. However, not all properties usually
are exported to the object inspector. C<prf_adjust_default>
deletes the unneeded property keys from PROFILE hash.

=item prf_delete @PROPERTIES

Removes PROPERTIES from internal properties hash.
This action results in that the PROPERTIES in the object inspector
are set back to their default values.

=item prf_events

Returns hash of a class-specific events. These appear in
the object inspector on C<Events> page. The hash keys are
event names; the hash values are default code pieces,
that describe format of the event parameters. Example:

	sub prf_events { return (
		$_[0]-> SUPER::prf_events,
		onSelectItem  => 'my ( $self, $index, $selectState) = @_;',
	)}

=item prf @PROPERTIES

Maps array of PROPERTIES names to their values. If called
in scalar context, returns the first value only; if in array
context, returns array of property values.

=item prf_types

Returns an anonymous hash, where keys are names of
the type class without C<Prima::VB::Types::> prefix,
and values are arrays of property names.

This callback returns an inverse mapping of properties
by the types.

=item prf_types_add PROFILE1, PROFILE2

Adds PROFILE2 content to PROFILE1. PROFILE1 and PROFILE2 are
hashes in format of result of C<prf_types> method.

=item prf_types_delete PROFILE, @NAMES

Removes @NAMES from PROFILE. Need to be called if property type if redefined
through the inheritance.

=item remove_hooks @PROPERTIES

Cancels watch for set of PROPERTIES.

=back

=head2 Events

=over

=item on_hook NAME, PROPERTY, OLD_VALUE, NEW_VALUE, WIDGET

Called for all objects, registered as watchers
by C<add_hooks> when PROPERTY on object NAME is changed
from OLD_VALUE to NEW_VALUE. Special PROPERTY C<'DESTROY'>
hook is called when object NAME is destroyed.

=back

=head1 Prima::VB::Component

=head2 Properties

=over

=item marked MARKED , EXCLUSIVE

Selects marked state of a widget. If MARKED flag is 1, the widget is
selected as marked. If 0, it is selected as unmarked.
If EXCLUSIVE flag is set to 1, then all marked widgets are unmarked
before the object mark flag is set.

=item sizeable BOOLEAN

If 1, the widget can be resized by the user.
If 0, in can only be moved.

=item mainEvent STRING

Selects the event name, that will be opened in the object inspector
when the user double clicks on the widget.

=back

=head2 Methods

=over

=item common_paint CANVAS

Draws selection and resize marks on the widget
if it is in the selected state. To be called from
all C<on_paint> callbacks.

=item get_o_delta

Returns offset to the owner widget. Since the builder does
not insert widgets in widgets to reflect the user-designed
object hierarchy, this method is to be used to calculate
children widgets relative positions.

=item xy2part X, Y

Maps X, Y point into part of widget. If result is not
equal to C<'client'> string, the event in X, Y point
must be ignored.

=item iterate_children SUB, @ARGS

Traverses all children widget in the hierarchy,
calling SUB routine with widget, self, and @ARGS
parameters on each.

=item altpopup

Invokes an alternative, class-specific popup menu, if present.
The popup object must be named C<'AltPopup'>.

=back

=head2 Events

=over

=item Load

Called when the widget is loaded from a file or the clipboard.

=back

=head1 Prima::VB::Types::generic

Root of all type classes.

A type class can be used with
and without object instance. The instantiated class
contains reference to ID string, which is a property
name that the object presents in the object inspector,
and WIDGET, which is the property applied to. When
the object inspector switches widgets, the type object
is commanded to update the references.

A class must also be usable without object instance,
in particular, in C<write> method. It is called to
export the property value in a storable format
as a string, better as a perl-evaluable expression.

=head2 Methods

=over


=item new CONTAINER, ID, WIDGET

Constructor method. CONTAINER is a panel widget in the object
inspector, where the type object can insert property value
selector widgets.

=item renew ID, WIDGET

Resets property name and the widget.

=item quotable STRING

Returns quotable STRING.

=item printable STRING

Returns a string that can be stored in a file.

=back

=head2 Callbacks

=over

=item change

Called when the widget property value is changed.

=item change_id

Called when the property name ( ID ) is changed.
The type object may consider update its look
or eventual internal variables on this event.

=item get

Returns property value, based on the selector widgets value.

=item open

Called when the type object is to be visualized first time.
The object must create property value selector widgets
in the C<{container}> panel widget.

=item preload_modules

Returns array of strings of modules, needed to be pre-loaded
before a form file with type class-specific information can be loaded.
Usually it is used when C<write> method
exports constant values, which are defined in another module.

=item set DATA

Called when a new value is set to the widget property by means other than the
selector widgets, so these can be updated. DATA is the property new value.

=item valid

Checks internal state of data and returns a boolean
flag, if the type object data can be exported and
set to widget profile.

=item write CLASS, ID, DATA

Called when DATA is to be written in form.
C<write> must return such a string that
can be loaded by C<Prima::VB::VBLoader> later.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<VB>, L<Prima::VB::VBLoader>, L<Prima::VB::CfgMaint>.

=cut
