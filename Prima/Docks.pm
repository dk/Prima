package Prima::Docks;

use Prima;
use Prima::Widget::RubberBand;
use strict;
use warnings;
use Tie::RefHash;

package Prima::AbstractDocker::Interface;

sub open_session
{
	my ( $self, $profile) = @_;
	return unless $self-> check_session( $profile);
	my @mgrs = grep { $_-> isa( 'Prima::AbstractDocker::Interface') } $self-> get_components;
	if ($self-> {subdockers}) {
		@{$self-> {subdockers}} = grep { $_-> alive} @{$self-> {subdockers}};
		push( @mgrs, @{$self-> {subdockers}});
	}
	return {
		SUBMGR    => \@mgrs,
		SUBMGR_ID => -1,
	};
}

sub check_session
{
	my $p = $_[1];
	return 1 if $$p{CHECKED_OK};
	warn("No 'self' given\n"), return 0 unless $$p{self};
	for ( qw( sizes)) {
		warn("No '$_' array specified\n"), return 0
			if !defined($$p{$_});
	}
	for ( qw( sizes sizeable position sizeMin)) {
		warn("'$_' is not an array\n"), return 0
			if defined($$p{$_}) && ( ref($$p{$_}) ne 'ARRAY');
	}
	my $i = 0;
	for ( @{$$p{sizes}}) {
		warn("Size #$i is not an valid array"), return 0 if (ref($_) ne 'ARRAY') || ( @$_ != 2);
	}
	$$p{sizeable} = [0,0] unless defined $$p{sizeable};
	warn("No 'sizes' given, and not sizable"), return 0
		if (( 0 == @{$$p{sizes}}) && !$p-> {sizeable}-> [0] &&!$p-> {sizeable}-> [1]);
	$$p{sizeMin}  = [0,0] unless defined $$p{sizeMin};
	$$p{position} = [] unless defined $$p{position};
	$$p{CHECKED_OK} = 1;
	return 1;
}

sub query
{
	my ( $self, $session_id, @rect) = @_;
	return unless (ref($session_id) eq 'HASH') &&
		exists($session_id-> {SUBMGR}) && exists($session_id-> {SUBMGR_ID});
	$session_id-> {SUBMGR_ID} = 0;
	return $session_id-> {SUBMGR}-> [0];
}

sub next_docker
{
	my ( $self, $session_id, $posx, $posy) = @_;
	return unless (ref($session_id) eq 'HASH') &&
		exists($session_id-> {SUBMGR}) && exists($session_id-> {SUBMGR_ID});
	my ( $id, $array) =  ( $session_id-> {SUBMGR_ID}, $session_id-> {SUBMGR});
	while ( 1) {
		return if $id < -1 || $id >= scalar(@$array) - 1;
		$session_id-> {SUBMGR_ID}++; $id++;
		return $$array[$id] if defined( $$array[$id]) && Prima::Object::alive($$array[$id]);
	}
	undef;
}

sub close_session
{
#	my ( $self, $session_id) = @_;
	undef $_[1];
}


sub undock
{
	my ( $self, $who) = @_;
#	print $self-> name . "($self): ". $who-> name . " is undocked\n";
	return unless $self-> {docklings};
	@{$self-> {docklings}} = grep { $who != $_ } @{$self-> {docklings}};
}

sub dock
{
	my ( $self, $who) = @_;
#	print $self-> name . "($self): ". $who-> name . " is docked\n";
	$self-> {docklings} = [] unless $self-> {docklings};
	push ( @{$self-> {docklings}}, $who);
}

sub dock_bunch
{
	my $self = shift;
	push ( @{$self-> {docklings}}, @_);
	$self-> rearrange;
}

sub docklings
{
	return $_[0]-> {docklings} ? @{$_[0]-> {docklings}} : ();
}

sub replace
{
	my ( $self, $wijFrom, $wijTo) = @_;
#	print $self-> name . "($self): ". $wijFrom-> name . " is replaced by ". $wijTo-> name ."\n";
	for (@{$self-> {docklings}}) {
		next unless $_ == $wijFrom;
		$_ = $wijTo;
		$wijTo-> owner( $wijFrom-> owner) unless $wijTo-> owner == $wijFrom-> owner;
		$wijTo-> rect( $wijFrom-> rect);
		last;
	}
}


sub redock_widget
{
	my ( $self, $wij) = @_;
	if ( $wij-> can('redock')) {
		$wij-> redock;
	} else {
		my @r = $wij-> owner-> client_to_screen( $wij-> rect);
		my %prf = (
			sizes     => [[ $r[2] - $r[0], $r[3] - $r[1]]],
			sizeable  => [0,0],
			self      => $wij,
		);
		my $sid = $self-> open_session( \%prf);
		return unless defined $sid;
		my @rc = $self-> query( $sid, @r);
		$self-> close_session( $sid);
		if ( 4 == scalar @rc) {
			if (( $rc[2] - $rc[0] == $r[2] - $r[0]) && ( $rc[3] - $rc[1] == $r[3] - $r[1])) {
				my @rx = $wij-> owner-> screen_to_client( @rc[0,1]);
				$wij-> origin( $wij-> owner-> screen_to_client( @rc[0,1]))
					if $rc[0] != $r[0] || $rc[1] != $r[1];
			} else {
				$wij-> rect( $wij-> owner-> screen_to_client( @rc));
			}
			$self-> undock( $wij);
			$self-> dock( $wij);
		}
	}
}

sub rearrange
{
	my $self = $_[0];
	return unless $self-> {docklings};
	my @r = @{$self-> {docklings}};
	@{$self-> {docklings}} = ();
	$self-> redock_widget($_) for @r;
}

sub fingerprint {
	return exists($_[0]-> {fingerprint})?$_[0]-> {fingerprint}:0xFFFF unless $#_;
	$_[0]-> {fingerprint} = $_[1];
}

sub add_subdocker
{
	my ( $self, $subdocker) = @_;
	push( @{$self-> {subdockers}}, $subdocker);
}

sub remove_subdocker
{
	my ( $self, $subdocker) = @_;
	return unless $self-> {subdockers};
	@{$self-> {subdockers}} = grep { $_ != $subdocker} @{$self-> {subdockers}};
}

sub dockup
{
	return $_[0]-> {dockup} unless $#_;
	$_[0]-> {dockup}-> remove_subdocker( $_[0]) if $_[0]-> {dockup};
	$_[1]-> add_subdocker( $_[0]) if $_[1];
}

package Prima::SimpleWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::AbstractDocker::Interface);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		fingerprint => 0x0000FFFF,
		dockup      => undef,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw(fingerprint dockup));
	return %profile;
}


sub open_session
{
	my ( $self, $profile) = @_;
	return unless $self-> enabled && $self-> showing;
	return unless $self-> check_session( $profile);

	my @sz = $self-> size;
	my @asz;
	my @able  = @{$profile-> {sizeable}};
	my @minSz = @{$profile-> {sizeMin}};
	for ( @{$profile-> {sizes}}) {
		my @xsz = @$_;
		for ( 0, 1) {
			next if ( $xsz[$_] >= $sz[$_]) && !$able[$_];
			next if $sz[$_] < $minSz[$_];
			$asz[$_] = $xsz[$_];
		}
	}

	return if !defined($asz[0]) || !defined($asz[1]);

	my @offs = $self-> client_to_screen(0,0);
	return {
		minpos => \@offs,
		maxpos => [ $offs[0] + $sz[0] - $asz[0] - 0, $offs[1] + $sz[1] - $asz[1] - 0,],
		size   => \@asz,
	};
}

sub query
{
	my ( $self, $p, @rect) = @_;
	my @npx;
	my @pos = @rect[0,1];
	if ( scalar @rect) {
		@npx = @pos;
		for ( 0, 1) {
			$npx[$_] = $$p{minpos}-> [$_] if $npx[$_] <  $$p{minpos}-> [$_];
			$npx[$_] = $$p{maxpos}-> [$_] if $npx[$_] >= $$p{maxpos}-> [$_];
		}
	} else {
		@npx = @{$$p{minpos}};
	}
	return @npx[0,1], $$p{size}-> [0] + $npx[0], $$p{size}-> [1] + $npx[1];
}

package Prima::ClientWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::SimpleWidgetDocker);

sub open_session
{
	my ( $self, $profile) = @_;
	return unless $self-> enabled && $self-> showing;
	return unless $self-> check_session( $profile);

	my @sz = $self-> size;
	my @asz;
	my @able = @{$profile-> {sizeable}};
	my @minSz = @{$profile-> {sizeMin}};
	for ( @{$profile-> {sizes}}) {
		my @xsz = @$_;
		for ( 0, 1) {
			next if ( $xsz[$_] != $sz[$_]) && !$able[$_];
			next if $sz[$_] < $minSz[$_];
			$asz[$_] = $sz[$_];
		}
	}

	return if !defined($asz[0]) || !defined($asz[1]);

	my @offs = $self-> client_to_screen(0,0);
	return {
		retval => [@offs, $offs[0] + $sz[0], $offs[1] + $sz[1]],
	};
}

sub query { return @{$_[1]-> {retval}}}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $self-> size;
	$canvas-> clear( 1, 1, $sz[0]-2, $sz[1]-2);
	$canvas-> rect3d( 0, 0, $sz[0]-1, $sz[1]-1, 1, $self-> dark3DColor, $self-> light3DColor);
}

package
    grow;
# direct, ::vertical-independent
use constant ForwardLeft   => 0x01;
use constant ForwardDown   => 0x02;
use constant ForwardRight  => 0x04;
use constant ForwardUp     => 0x08;
use constant BackLeft      => 0x10;
use constant BackDown      => 0x20;
use constant BackRight     => 0x40;
use constant BackUp        => 0x80;
use constant Left          => ForwardLeft | BackLeft;
use constant Down          => ForwardDown | BackDown;
use constant Right         => ForwardRight| BackRight;
use constant Up            => ForwardUp   | BackUp;

# indirect, ::vertical-dependent
use constant ForwardMajorLess => 0x0100;
use constant ForwardMajorMore => 0x0200;
use constant ForwardMinorLess => 0x0400;
use constant ForwardMinorMore => 0x0800;
use constant BackMajorLess    => 0x1000;
use constant BackMajorMore    => 0x2000;
use constant BackMinorLess    => 0x4000;
use constant BackMinorMore    => 0x8000;
use constant MajorLess        => ForwardMajorLess | BackMajorLess;
use constant MajorMore        => ForwardMajorMore | BackMajorMore;
use constant MinorLess        => ForwardMinorLess | BackMinorLess;
use constant MinorMore        => ForwardMinorMore | BackMinorMore;

# masks
use constant Forward          => 0x0F0F;
use constant Back             => 0xF0F0;

package Prima::LinearWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::AbstractDocker::Interface);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		dockup      => undef,
		vertical    => 0,
		growable    => 0, # grow::XXXX
		hasPocket   => 1,
		fingerprint => 0x0000FFFF
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	Dock      => nt::Notification,
	Undock    => nt::Notification,
	DockError => nt::Action,
);

sub notification_types { return \%RNT; }
}

sub init
{
	my $self = shift;
	$self-> {$_} = 0 for qw(growable vertical hasPocket fingerprint dockup);
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw( fingerprint growable hasPocket vertical dockup));
	return %profile;
}

sub vertical
{
	return $_[0]-> {vertical} unless $#_;
	my ( $self, $v) = @_;
	$self-> {vertical} = $v;
}

sub hasPocket
{
	return $_[0]-> {hasPocket} unless $#_;
	my ( $self, $v) = @_;
	$self-> {hasPocket} = $v;
}


sub growable
{
	return $_[0]-> {growable} unless $#_;
	my ( $self, $g) = @_;
	$self-> {growable} = $g;
}

sub __docklings
{
	my ( $self, $exclude) = @_;
	my %hmap;
	my $xid = $self-> {vertical} ? 0 : 1; # minor axis, further 'vertical'
	my $yid = $self-> {vertical} ? 1 : 0; # major axis, further 'horizontal
	my $min;
	for ( @{$self-> {docklings}}) {
		next if $_ == $exclude;  # if redocking to another position, for example
		my @rt = $_-> rect;
		$hmap{$rt[$xid]} = [0,0,0,[],[]] unless $hmap{$rt[$xid]};
		my $sz = $rt[$xid+2] - $rt[$xid];
		my $xm = $hmap{$rt[$xid]};
		$min = $rt[$xid] if !defined($min) || $min > $rt[$xid];
		$$xm[0] = $sz if $$xm[0] < $sz; # max vert extension
		$$xm[1] += $rt[$yid + 2] - $rt[$yid]; # total length occupied
		$$xm[2] = $rt[$yid+2] if $rt[$yid+2] > $$xm[2]; # farthest border
		push( @{$$xm[4]}, $_); # widget
	}

	# checking widgets
	my @ske = sort { $a <=> $b }keys %hmap;
	my @sz  = $self-> size;
	my $i;
	for ( $i = 0; $i < @ske - 1; $i++) {
		my $ext = $hmap{$ske[$i]}-> [0];
		if ( $ext + $ske[$i] < $ske[$i+1]) { # some gap here
			$hmap{$ext + $ske[$i]} = [$ske[$i+1] - $ske[$i] - $ext, 0, 0, [], []];
			@ske = sort { $a <=> $b }keys %hmap;
		}
	}
	if ( @ske) {
		my $ext = $hmap{$ske[-1]}-> [0]; # last row
		$hmap{$ext + $ske[-1]} = [$sz[$xid] - $ske[-1] - $ext, 0, 0, [], []];
	} else {
		$hmap{0} = [ $sz[$xid], 0, 0, [], []];
	}
	$hmap{0} = [ $min, 0, 0, [], []] unless $hmap{0};
# hmap structure:
# 0 - max vert extension
# 1 - total length occupied
# 2 - farther border by major axis
# 3 - array of accepted sizes
# 4 - widget list
	return \%hmap;
}

sub read_growable
{
	my ($self,$directionMask) = @_;
	my $g   = $self-> {growable} & $directionMask;
	my $xid = $self-> {vertical} ? 0 : 1;
	my $gMaxG = ( $g & grow::MajorMore) || ($g & ($xid ? grow::Right : grow::Up));
	my $gMaxL = ( $g & grow::MajorLess) || ($g & ($xid ? grow::Left  : grow::Down));
	my $gMinG = ( $g & grow::MinorMore) || ($g & ($xid ? grow::Up    : grow::Right));
	my $gMinL = ( $g & grow::MinorLess) || ($g & ($xid ? grow::Down  : grow::Left));

	return ( $gMaxG, $gMaxL, $gMinG, $gMinL);
}

sub open_session
{
	my ( $self, $profile) = @_;
	return unless $self-> enabled && $self-> visible;
	return unless $self-> check_session( $profile);

	my @sz  = $self-> size;
	my @msz = $self-> sizeMax;
	my $xid = $self-> {vertical} ? 0 : 1; # minor axis, further 'vertical'
	my $yid = $self-> {vertical} ? 1 : 0; # major axis, further 'horizontal'
	my ( $gMaxG, $gMaxL, $gMinG, $gMinL) = $self-> read_growable( grow::Forward);
	my %hmap = %{$self-> __docklings( $$profile{self})};

	# calculating row extension
	my $rows   = scalar keys %hmap;
	my $majExt = ( $gMaxG || $gMaxL) ? $msz[ $yid] : $sz[ $yid];
	my $minExt = ( $gMinG || $gMinL) ? $msz[ $xid] : $sz[ $xid];

	push( @{$$profile{sizes}}, [ @sz]) unless @{$$profile{sizes}};

	# total vertical occupied size
	my ( $gap, $vo) = (0, 0);
	for ( keys %hmap) {
		$hmap{$_}-> [1] ?
			( $vo  += $hmap{$_}-> [0]) :
			( $gap += $hmap{$_}-> [0]) ;
	}

	# put acceptable set of sizes for every line
	my @minSz = @{$$profile{sizeMin}};
	for ( keys %hmap) {
		my ( $y, $ext, $total, $border, $array) = ( $_, @{$hmap{$_}});
		for ( @{$$profile{sizes}}) {
			my @asz = @$_;
			#print "@asz:ext:$ext, minext:$minExt, vo:$vo\n";
			#print "row $y:($total $majExt)";
			#if ( $asz[$xid] > $minExt - $vo) {
			if (( $asz[$xid] > $ext) && ($asz[$xid] > $minExt - $vo)) {
				next unless $profile-> {sizeable}-> [$xid];
				my $n_ext = $minExt - $vo;
				next if $n_ext < $minSz[$xid] && $n_ext < $asz[$xid];
				$asz[$xid] = $n_ext;
			}
			#print "step1 $y :@asz|$ext $total $border = $majExt\n";
			if ($total + $asz[$yid] > $majExt) {
				if ( !$self-> {hasPocket} || ( $border >= $majExt)) {
					next unless $profile-> {sizeable}-> [$yid];
					my $nb = ( $self-> {hasPocket} ? $border : $majExt) - $total;
					#print "3: $nb $yid\n";
					next if $nb < $minSz[$yid] && $nb < $asz[$yid];
					$asz[$yid] = $nb;
				}
			}
			# print "@$_:@asz\n";
			push ( @$array, \@asz);
		}
		# print "$_(" . scalar(@{$hmap{$_}->[4]}). ')';
	}

	# add decrement line
	if ( $vo) {
		# print " and - ";
		for (@{$$profile{sizes}}) {
			my @asz = @$_;
			next if $hmap{- $asz[$xid]};
			next if $asz[$xid] > $minExt - $vo;
			$hmap{ - $asz[$xid]} = [ $asz[$xid], 0, 0, [\@asz], []];
			# print "|$asz[$xid] ";
		}
	}
	# print "\n";


	# sort out accepted sizes by 'verticalness'
	for ( keys %hmap) {
		my $s = $hmap{$_}-> [3];
		@$s = map {
			[$$_[0], $$_[1]]             # remove ratio field
		} sort {
			$$a[2] <=> $$b[2]          # sort by xid/yid ratio
		} map {
			[@$_, $$_[$xid] / ($$_[$yid]||1)] # calculate xid/yid ratio
		} @$s;
	}
	return {
		offs     => [ $self-> client_to_screen(0,0)],
		size     => \@sz,
		sizeMax  => \@msz,
		hmap     => \%hmap,
		rows     => scalar keys %hmap,
		vmap     => [ sort { $a <=> $b } keys %hmap],
		sizes    => [ sort { $$a[2] <=> $$b[2]} map { [ @$_, $$_[$yid] / ($$_[$xid]||1)]} @{$$profile{sizes}}],
		sizeable => $$profile{sizeable},
		sizeMin  => $$profile{sizeMin},
		grow     => [ $gMinG, $gMinL, $gMaxG, $gMaxL],
	};
}

sub query
{
	my ( $self, $p, @rect) = @_;
	my $xid = $self-> {vertical} ? 0 : 1;
	my $yid = $self-> {vertical} ? 1 : 0;
	my @asz;
	my @offs = @{$p-> {offs}};
	my $hmap = $$p{hmap};
	my $vmap = $$p{vmap};
	my ( $i, $closest, $idx, $side);
	my $rows = $$p{rows};
	my $useSZ = 1;

	$useSZ = 0, @rect = ( 0, 0, 1, 1) unless scalar @rect;
	my %skip = ();
AGAIN:
	$i = 0; $idx = undef;
	for ( $i = 0; $i < $rows; $i++) {
		next if $skip{$$vmap[$i]};
		my $dist = ( $rect[ $xid] - ( $offs[$xid] + $$vmap[$i]));
		$dist *= $dist;
		$side = 0, $idx = $$vmap[$i], $closest = $dist if !defined($idx) || ( $closest > $dist);
		if ( $$vmap[$i] == 0 && !$$p{noDownSide}) {
			$dist = ( $rect[ $xid + 2] - ( $offs[$xid] + $$vmap[$i]));
			$dist *= $dist;
			$side = 1, $idx = $$vmap[$i], $closest = $dist if $closest > $dist;
		}
	}
	return unless defined $idx;
	if ( @{$hmap-> {$idx}-> [3]}) {
		@asz = @{$hmap-> {$idx}-> [3]-> [0]};
	} else {
		# print "$idx rejected\n";
		$skip{$idx} = 1;
		goto AGAIN;
	}

	@rect = ( 0, 0, @asz) unless $useSZ;
	$idx -= $rect[$xid+2] - $rect[$xid] if $side;
	if ( $rect[$yid] < $offs[$yid]) {
#     $asz[$yid] -= $offs[$yid] - $rect[$yid] if $$p{sizeable}->[$yid];
		$rect[$yid] = $offs[$yid];
	}
	my $sk = ( $p-> {sizeMin}-> [$yid] > $asz[$yid]) ? $asz[$yid] : $p-> {sizeMin}-> [$yid];
	$rect[ $yid] = $offs[$yid] + $p-> {size}-> [$yid] - $sk if
		$rect[$yid] > $offs[$yid] + $p-> {size}-> [$yid] - $sk;
#   unless ( $self-> {vertical}) {
#my @r = ( $rect[0], $idx + $offs[1], $rect[0] + $asz[0], $idx + $offs[1] + $asz[1]);
#print "q :@r\n";
#   }
	return $self-> {vertical} ?
		( $idx + $offs[0], $rect[1], $idx + $offs[0] + $asz[0], $rect[1] + $asz[1]) :
		( $rect[0], $idx + $offs[1], $rect[0] + $asz[0], $idx + $offs[1] + $asz[1]);
}

sub dock
{
	my ( $self, $who) = @_;
	$self-> SUPER::dock( $who);
	my $xid = $self-> {vertical} ? 0 : 1;
	my $yid = $self-> {vertical} ? 1 : 0;
	my @rt = $who-> rect;
	my @sz = $self-> size;
	my $hmap = $self-> __docklings( $who);
	my ( $gMaxG, $gMaxL, $gMinG, $gMinL) = $self-> read_growable( grow::Forward);

	# for ( keys %$hmap) { print "hmap:$_\n"; }

	if ( !exists $hmap-> {$rt[$xid]}) {
		if ( $rt[$xid] >= 0 || $rt[$xid+2] != 0) {
			$self-> notify(q(DockError), $who);
			return;
		}
		$hmap-> {$rt[$xid]} = [$rt[$xid+2]-$rt[$xid], 0, 0, [], [], 0];
	}

	# minor axis
	my $doMajor = $hmap-> {$rt[$xid]}-> [1];

	my $gap = 0;
	for ( keys %$hmap) {
		next if $_ < 0 || $hmap-> {$_}-> [1];
		$gap += $hmap-> {$_}-> [0];
	}

#   print "key : $rt[$xid] $rt[$xid+2]\n";
	my $maxY = $hmap-> {$rt[$xid]}-> [1] ? $hmap-> {$rt[$xid]}-> [0] : 0;
	#my $tail = $rt[$xid+2] - $rt[$xid] - $hmap->{$rt[$xid]}->[0];
	my $tail = $rt[$xid+2] - $rt[$xid] - $maxY;
	#print "$self:tail:$tail $maxY @rt\n";
	if ( $tail > 0 || $rt[$xid] < 0) {
		my @fmp  = sort { $a <=> $b} keys %$hmap;
		my $prop = $self-> {vertical} ? 'left' : 'bottom';
		my $last = 0;
		for ( @fmp) {
			my @rp = @{$hmap-> {$_}-> [4]};
			my $ht = $hmap-> {$_}-> [0];
			if ( $_ == $rt[$xid]) {
				push ( @rp, $who);
				$ht = $rt[$xid+2] - $rt[$xid] if $ht < $rt[$xid+2] - $rt[$xid];
			}
			next unless scalar @rp;
			$_-> $prop( $last) for @rp;
			$last += $ht;
		#   print "added $hmap->{$_}->[0]\n";
		}
		$tail = ($last > $sz[$xid]) ? ( $last - $sz[$xid]) : 0;
		@rt = $who-> rect;
		# print "last:$last, tail:$tail\n";
	} else {
		$tail = 0;
	}

	if ( $tail) {
		if ( $gMinG) {
			$sz[$xid] += $tail;
			$self-> size( @sz);
		} else {
			my @rect = $self-> rect;
			$rect[ $xid] -= $tail;
			$self-> rect( @rect);
		}
		@sz = $self-> size;
	}

	# major axis

	unless ( $self-> {hasPocket}) {
		my @o = @rt[0,1];
		$o[$yid] = $sz[$yid] - $rt[$yid+2] + $rt[$yid] if $rt[$yid+2] > $sz[$yid];
		$o[$yid] = 0 if $o[$yid] < 0;
	#  print "@o:@rt\n";
		$who-> origin( @o) if $o[$yid] != $rt[$yid];
		@rt[0,1] = @o;
	}

	my @fmp;
	my $edge = 0;
	for ( $who, @{$hmap-> {$rt[$xid]}-> [4]}) {
		my @rxt = $_-> rect;
		push ( @fmp, [ $_, $rxt[ $yid], $rxt[ $yid + 2] - $rxt[ $yid]]);
		$edge = $rxt[$yid+2] if $edge < $rxt[$yid+2];
	}

	if ( $doMajor) {
		@fmp = sort { $$a[1] <=> $$b[1]} @fmp;
		my $prop = $self-> {vertical} ? 'bottom' : 'left';
		my $overlap;
		my $last = 0;
		for ( @fmp) {
			$overlap = 1, last if $$_[1] < $last;
			$last = $$_[1] + $$_[2];
		}
		if ( $overlap) {
			$last = 0;
			my $i = 0;
			my @sizeMax = $self-> sizeMax;
			my @msz = ( $gMaxG || $gMaxL) ? @sizeMax : @sz;
			my $stage = 0;
			for ( @fmp) {
				$$_[1] = $last;
				$$_[0]-> $prop( $last);
				$last += $$_[2];
				$i++;
			}
			$edge = $last;
		}
	}

	if ( $edge > $sz[$yid] && ($gMaxL || $gMaxG)) {
		if ( $gMaxG) {
			$sz[$yid] = $edge;
			$self-> size( @sz);
		} else {
			my @r = $self-> rect;
			$r[$yid] -= $edge - $sz[$yid];
			$self-> rect( @r);
		}
		@sz = $self-> size;
	}

	# redocking non-fit widgets
	my $stage = 0;
	my @repush;
	for ( @fmp) {
		if ( $self-> {hasPocket}) {
			next if $$_[1] <= $sz[$yid] - 5;
			$stage = 1, next unless $stage;
		} else {
			next if $$_[1] + $$_[2] <= $sz[$yid];
		}
		push( @repush, $$_[0]);
	}
	$self-> redock_widget($_) for @repush;

	$self-> notify(q(Dock), $who);
}

sub undock
{
	my ( $self, $who) = @_;
	$self-> SUPER::undock( $who);
	my $xid = $self-> {vertical} ? 0 : 1;
	my $yid = $self-> {vertical} ? 1 : 0;
	my @rt = $who-> rect;
	my @sz = $self-> size;
	my $hmap = $self-> __docklings( $who);
	my ( $gMaxG, $gMaxL, $gMinG, $gMinL) = $self-> read_growable( grow::Back);

# collapsing minor axis
	my $xd = $rt[$xid+2] - $rt[$xid];
	if (( !$hmap-> {$rt[$xid]}-> [1] || ($hmap-> {$rt[$xid]}-> [0] < $xd)) && ( $gMinG || $gMinL)) {
		my $d = $xd - ( $hmap-> {$rt[$xid]}-> [1] ? $hmap-> {$rt[$xid]}-> [0] : 0);
		my @asz = @sz;
		$asz[$xid] -= $d;
		$self-> size( @asz);
		@sz = $self-> size;
		my $prop = $self-> {vertical} ? 'left' : 'bottom';
		for ( keys %$hmap) {
			next if $_ <= $rt[$xid];
			$_-> $prop( $_-> $prop() - $d) for @{$hmap-> {$_}-> [4]};
		}
		if ( $gMinL) {
			my @o = $self-> origin;
			$o[$xid] += $d;
			$self-> origin( @o);
		}
	}
# collapsing major axis
	my @fmp;
	my $adjacent;
	for ( @{$hmap-> {$rt[$xid]}-> [4]}) {
		my @rxt = $_-> rect;
		next if $rxt[$yid] < $rt[$yid];
		push( @fmp, $_);
		$adjacent = 1 if $rxt[$yid] == $rt[$yid + 2];
	}
	if ( $adjacent) {
		my $d = $rt[$yid+2] - $rt[$yid];
		my $prop = $self-> {vertical} ? 'bottom' : 'left';
		$_-> $prop( $_-> $prop() - $d) for @fmp;
	}

	if ( $gMaxG || $gMaxL) {
		my $edge = 0;
		for ( keys %$hmap) {
			for ( @{$hmap-> {$_}-> [4]}) {
				my @rxt = $_-> rect;
				$edge = $rxt[$yid+2] if $edge < $rxt[$yid+2];
			}
		}
		if ( $edge < $sz[$yid]) {
			if ( $gMaxG) {
				$sz[$yid] = $edge;
				$self-> size( @sz);
			} else {
				my @r = $self-> rect;
				$r[$yid] += $edge - $sz[$yid];
				$self-> rect( @r);
			}
		}
	}

	$self-> notify(q(Undock), $who);
}

sub on_dockerror
{
	my ( $self, $urchin) = @_;
	my @rt = $urchin-> rect;
	my $xid = $self-> {vertical} ? 0 : 1;
	warn "The widget $urchin didn't follow docking conventions. Info: $rt[$xid] $rt[$xid+2]\n";
	$self-> redock_widget( $urchin);
}

package Prima::SingleLinearWidgetDocker;
use vars qw(@ISA);
@ISA = qw(Prima::LinearWidgetDocker);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		growMode    => gm::Client,
		hasPocket   => 0,
		growable    => grow::MajorMore,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub open_session
{
	my ( $self, $profile) = @_;
	my $res = $self-> SUPER::open_session( $profile);
	return unless $res;
# keep only one row of docklings
	my %hmap = %{$res-> {hmap}};
	my @k    = keys %hmap;
	for ( @k) {
		delete $hmap{$_} if $_ != 0;
	}
	$res-> {noDownSide} = 1;
	return $res if scalar(keys %hmap) == scalar(@k);
	$res-> {hmap} = \%hmap;
	$res-> {rows} = scalar keys %hmap;
	$res-> {vmap} = [sort { $a <=> $b } keys %hmap];
	return $res;
}

package Prima::FourPartDocker;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::AbstractDocker::Interface);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		indents             => [ 0, 0, 0, 0],
		growMode            => gm::Client,
		dockup              => undef,
		fingerprint         => 0x0000FFFF,
		dockerClassLeft     => 'Prima::LinearWidgetDocker',
		dockerClassRight    => 'Prima::LinearWidgetDocker',
		dockerClassTop      => 'Prima::LinearWidgetDocker',
		dockerClassBottom   => 'Prima::LinearWidgetDocker',
		dockerClassClient   => 'Prima::ClientWidgetDocker',
		dockerProfileLeft   => {},
		dockerProfileRight  => {},
		dockerProfileTop    => {},
		dockerProfileBottom => {},
		dockerProfileClient => {},
		dockerDelegationsLeft   => [qw(Size)],
		dockerDelegationsRight  => [qw(Size)],
		dockerDelegationsTop    => [qw(Size)],
		dockerDelegationsBottom => [qw(Size)],
		dockerDelegationsClient => [],
		dockerCommonProfile     => {},
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	for ( qw( Left Right Top Bottom)) {
		my $x = "dockerDelegations$_";
		# append user-specified delegations - it may not be known beforehand
		# which delegations we are using internally
		next unless exists $p-> {$x};
		splice( @{$p-> {$x}}, scalar(@{$p-> {$x}}), 0, @{$default-> {$x}});
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw( dockup indents fingerprint));
	my @sz = $self-> size;
	my @i  = @{$self-> indents};
	$self-> insert([ $profile{dockerClassLeft} =>
		origin   => [ 0, $i[1]],
		size     => [ $i[0], $sz[1] - $i[3] - $i[1]],
		vertical => 1,
		growable => grow::Right,
		growMode => gm::GrowHiY,
		name     => 'LeftDocker',
		delegations => $profile{dockerDelegationsLeft},
		%{$profile{dockerProfileLeft}},
		%{$profile{dockerCommonProfile}},
	], [ $profile{dockerClassRight} =>
		origin   => [ $sz[0] - $i[2], $i[1]],
		size     => [ $i[2], $sz[1] - $i[3] - $i[1]],
		vertical => 1,
		growable => grow::Left,
		growMode => gm::GrowHiY|gm::GrowLoX,
		name     => 'RightDocker',
		delegations => $profile{dockerDelegationsRight},
		%{$profile{dockerProfileRight}},
		%{$profile{dockerCommonProfile}},
	], [ $profile{dockerClassTop} =>
		origin   => [ 0, $sz[1] - $i[3]],
		size     => [ $sz[0], $i[3]],
		vertical => 0,
		growable => grow::Down,
		growMode => gm::GrowLoY|gm::GrowHiX,
		name     => 'TopDocker',
		delegations => $profile{dockerDelegationsTop},
		%{$profile{dockerProfileTop}},
		%{$profile{dockerCommonProfile}},
	], [ $profile{dockerClassBottom} =>
		origin   => [ 0, 0],
		size     => [ $sz[0], $i[1]],
		vertical => 0,
		growable => grow::Up,
		growMode => gm::GrowHiX,
		name     => 'BottomDocker',
		delegations => $profile{dockerDelegationsBottom},
		%{$profile{dockerProfileBottom}},
		%{$profile{dockerCommonProfile}},
	], [ $profile{dockerClassClient} =>
		origin   => [ @i[0,1]],
		size     => [ $sz[0]-$i[2], $sz[1]-$i[3]],
		growMode => gm::Client,
		name     => 'ClientDocker',
		delegations => $profile{dockerDelegationsClient},
		%{$profile{dockerProfileClient}},
		%{$profile{dockerCommonProfile}},
	]);
	return %profile;
}

sub indents
{
	return $_[0]-> {indents} unless $#_;
	my @i = @{$_[1]};
	for ( @i) {
		$_ = 0 if $_ < 0;
	}
	return unless 4 == @i;
	$_[0]-> {indents} = \@i;
}

sub LeftDocker_Size
{
	my ( $self, $dock, $ox, $oy, $x, $y) = @_;
	return if $self-> {indents}-> [0] == $x;
	return unless $self-> can_event;
	$self-> {indents}-> [0] = $x;
	$self-> ClientDocker-> set(
		left  => $x,
		right => $self-> ClientDocker-> right,
	);
	$self-> repaint;
}

sub RightDocker_Size
{
	my ( $self, $dock, $ox, $oy, $x, $y) = @_;
	return if $self-> {indents}-> [2] == $x;
	return unless $self-> can_event;
	$self-> {indents}-> [2] = $x;
	$self-> ClientDocker-> width( $self-> width - $x - $self-> {indents}-> [0]);
	$self-> repaint;
}

sub TopDocker_Size
{
	my ( $self, $dock, $ox, $oy, $x, $y) = @_;
	return if $self-> {indents}-> [3] == $y;
	return unless $self-> can_event;
	$self-> {indents}-> [3] = $y;
	my $h = $self-> height - $y - $self-> {indents}-> [1];

	$self-> LeftDocker-> height( $h);
	$self-> RightDocker-> height( $h);
	$self-> ClientDocker-> height( $h);
	$self-> repaint;
}

sub BottomDocker_Size
{
	my ( $self, $dock, $ox, $oy, $x, $y) = @_;
	return if $self-> {indents}-> [1] == $y;
	return unless $self-> can_event;
	$self-> {indents}-> [1] = $y;
	my $h = $self-> height;
	$self-> LeftDocker-> height( $h - $y - $self-> {indents}-> [3]);
	$self-> LeftDocker-> bottom( $self-> {indents}-> [1]);
	$self-> RightDocker-> height( $h - $y - $self-> {indents}-> [3]);
	$self-> RightDocker-> bottom( $self-> {indents}-> [1]);
	$self-> ClientDocker-> set(
		bottom  => $y,
		top     => $self-> ClientDocker-> top,
	);
	$self-> repaint;
}

package Prima::InternalDockerShuttle;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	GetCaps  => nt::Command,
	Landing  => nt::Request,
	Dock     => nt::Notification,
	Undock   => nt::Notification,
	FailDock => nt::Notification,
	EDSClose => nt::Command,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		externalDockerClass     => 'Prima::ExternalDockerShuttle',
		externalDockerModule    => 'Prima::MDI',
		externalDockerProfile   => {},
		dockingRoot             => undef,
		dock                    => undef,
		snapDistance            => 10, # undef for none
		indents                 => [ 5, 5, 5, 5],
		x_sizeable              => 0,
		y_sizeable              => 0,
		fingerprint             => 0x0000FFFF,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw( indents x_sizeable y_sizeable
		externalDockerClass externalDockerModule externalDockerProfile fingerprint
		dockingRoot snapDistance));
	$self-> {__dock__} = $profile{dock};
	return %profile;
}


sub setup
{
	$_[0]-> SUPER::setup;
	$_[0]-> dock( $_[0]-> {__dock__});
	delete $_[0]-> {__dock__};
}

sub cleanup
{
	my $self = $_[0];
	$self-> SUPER::cleanup;
	$self-> {dock}-> undock( $self) if $self-> {dock};
	my $d = $self-> {externalDocker};
	$self-> {externalDocker} = $self-> {dock} = undef;
	$d-> destroy if $d;
}


sub snapDistance {
	return $_[0]-> {snapDistance} unless $#_;
	my $sd = $_[1];
	$sd = 0 if defined( $sd) && ($sd < 0);
	$_[0]-> {snapDistance} = $sd;
}

sub externalDockerClass {
	return $_[0]-> {externalDockerClass} unless $#_;
	$_[0]-> {externalDockerClass} = $_[1];
}

sub externalDockerModule {
	return $_[0]-> {externalDockerModule} unless $#_;
	$_[0]-> {externalDockerModule} = $_[1];
}

sub externalDockerProfile {
	return $_[0]-> {externalDockerProfile} unless $#_;
	$_[0]-> {externalDockerProfile} = $_[1];
}

sub dockingRoot {
	return $_[0]-> {dockingRoot} unless $#_;
	$_[0]-> {dockingRoot} = $_[1] if !defined($_[1]) || $_[1]-> isa('Prima::AbstractDocker::Interface');
}

sub x_sizeable {
	return $_[0]-> {x_sizeable} unless $#_;
	$_[0]-> {x_sizeable} = $_[1];
}

sub y_sizeable {
	return $_[0]-> {y_sizeable} unless $#_;
	$_[0]-> {y_sizeable} = $_[1];
}

sub fingerprint {
	return $_[0]-> {fingerprint} unless $#_;
	$_[0]-> {fingerprint} = $_[1];
}

sub client
{
	return $_[0]-> {client} unless $#_;
	my ( $self, $c) = @_;
	if ( !defined($c)) {
		return if !$self-> {client};
	} else {
		return if defined( $self-> {client}) && ($c == $self-> {client});
	}
	$self-> {client} = $c;
	return unless defined $c;
	return unless $self-> {externalDocker};
	my $ed = $self-> {externalDocker};
	my @cf = $ed-> client2frame( $c-> rect);
	$ed-> size( $cf[2] - $cf[0], $cf[3] - $cf[1]);
	$c-> set( map {'owner' . $_ => 0} qw( Font Hint Palette Color BackColor));
	$c-> owner( $ed-> client);
	$c-> clipOwner(1);
	$c-> origin( 0, 0);
}

sub frame2client
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	my @i = @{$self-> {indents}};
	return ( $x1 + $i[0], $y1 + $i[1], $x2 - $i[2], $y2 - $i[3]);
}

sub client2frame
{
	my ( $self, $x1, $y1, $x2, $y2) = @_;
	my @i = @{$self-> {indents}};
	return ( $x1 - $i[0], $y1 - $i[1], $x2 + $i[2], $y2 + $i[3]);
}

sub xorrect
{
	my ( $self, $x1, $y1, $x2, $y2, $width) = @_;
	if ( defined $x1 ) {
		$x2--; $y2--;
		$::application-> rubberband(
			rect    => [ $x1, $y1, $x2, $y2 ],
			breadth => $width,
		);
	} else {
		$::application-> rubberband( destroy => 1 )
	}
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	$canvas-> clear( 1, 1, $sz[0]-2, $sz[1]-2);
	$canvas-> rectangle( 0, 0, $sz[0]-1, $sz[1]-1);
}

sub indents
{
	return $_[0]-> {indents} unless $#_;
	my @i = @{$_[1]};
	for ( @i) {
		$_ = 0 if $_ < 0;
	}
	return unless 4 == @i;
	$_[0]-> {indents} = \@i;
}

sub drag
{
	return $_[0]-> {drag} unless $#_;
	my ( $self, $drag, $rect, $x, $y) = @_;
	my @rrc;
	if ( $drag) {
		return if $self-> {drag};
		$self-> {orgRect} = $rect;
		$self-> {anchor}  = [$x, $y];
		$self-> {drag}  = 1;
		$self-> {pointerSave} = $self-> pointer;
		$self-> {focSave}     = $::application-> get_focused_widget;
		$self-> capture(1);
		$self-> {oldRect} = [@{$self-> {orgRect}}];
		$self-> {sessions} = {};
		tie %{$self-> {sessions}}, 'Tie::RefHash';
		@rrc = @{$self-> {oldRect}};
		$self-> pointer( cr::Move);
		$self-> xorrect( @rrc, 3);
	} else {
		return unless $self-> {drag};
		$self-> capture(0);
		@rrc = @{$self-> {oldRect}};
		$self-> pointer( $self-> {pointerSave});
		$_-> close_session( $self-> {sessions}-> {$_}) for keys %{$self-> {sessions}};
		delete $self-> {$_} for qw( anchor drag orgRect oldRect pointerSave sessions dockInfo);
		$self-> xorrect;
	}

	unless ( $drag) {
		$self-> {focSave}-> focus if
			$self-> {focSave} && ref($self-> {focSave}) && $self-> {focSave}-> alive;
		delete $self-> {focSave};
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless $btn == mb::Left;
	$self-> drag(1, [$self-> owner-> client_to_screen( $self-> rect)], $x, $y);
	$self-> clear_event;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless ($btn == mb::Left) && $self-> {drag};
	my @rc;
	$rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [0] + $x for ( 0, 2);
	$rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [1] + $y for ( 1, 3);
	my ( $dm, $rect);
	if ( $self-> {dockingRoot}) {
		( $dm, $rect) = $self-> find_docking($self-> {dockingRoot}, \@rc);
	}
	$self-> drag(0);
	if ( $self-> {dockingRoot}) {
		if ( $dm) {
			$self-> dock( $dm, @$rect); # dock or redock
		} elsif ( $self-> {externalDocker}) {
			$self-> {externalDocker}-> origin( @rc[0,1]); # just move external docker
			$self-> notify(q(FailDock), @rc[0,1]);
		} else {
			$self-> dock( undef, @rc); # convert to external state
		}
	}
	$self-> clear_event;
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return unless $self-> {drag};
	my @rc;
	my $w = 3;
	$rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [0] + $x for ( 0, 2);
	$rc[$_]  = $self-> {orgRect}-> [$_] - $self-> {anchor}-> [1] + $y for ( 1, 3);
	goto LEAVE unless $self-> {dockingRoot};
	my ( $dm, $rect) = $self-> find_docking($self-> {dockingRoot}, \@rc);
	goto LEAVE unless $dm;
	@rc = @$rect;
	$w = 1;
LEAVE:
	$self-> {oldRect} = \@rc;
	$self-> xorrect( @{$self-> {oldRect}}, $w);
	$self-> clear_event;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	if ( $self-> {drag} && $key == kb::Esc) {
		$self-> drag(0);
		$self-> clear_event;
	}
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	return unless $dbl;
	$self-> dock( undef);
}

sub on_getcaps
{
	my ( $self, $docker, $prf) = @_;
	push( @{$prf-> {sizes}}, [$self-> size]);
	$prf-> {sizeable} = [ $self-> {x_sizeable}, $self-> {y_sizeable}];
	$prf-> {sizeMin}  = [ $self-> {indents}-> [2] + $self-> {indents}-> [0], $self-> {indents}-> [3] + $self-> {indents}-> [1]];
}

sub find_docking
{
	my ( $self, $dm, $pos) = @_;
	my $sid;
	unless ( exists $self-> {sessions}-> {$dm}) {
		if ( $self-> fingerprint & $dm-> fingerprint) {
			my %caps;
			$self-> notify(q(GetCaps), $dm, \%caps);
			if ( keys %caps) { # $dm is user-approved
				$caps{position} = [ @$pos] if $pos;
				$caps{self}     = $self;
				$sid = $dm-> open_session( \%caps);
			}
		}
		$self-> {sessions}-> {$dm} = $sid;
	} else {
		$sid = $self-> {sessions}-> {$dm};
	}
	return unless $sid;
	my $relocationCount;
AGAIN:
	#print "{$dm:@$pos:";
	my @retval;
	my @rc = $dm-> query( $sid, $pos ? @$pos : ());
	#print "(@rc)\n";
	goto EXIT unless scalar @rc;
	if ( 4 == scalar @rc) { # rect returned
		my $sd = $self-> {snapDistance};
		if ( $pos && defined($sd)) {
			if ( $self-> {drag} &&
					( # have to change the shape
					(( $$pos[2] - $$pos[0]) != ( $rc[2] - $rc[0])) ||
					(( $$pos[3] - $$pos[1]) != ( $rc[3] - $rc[1])))) {
				my @pp = $::application-> pointerPos;
				my @newpos;
				#print '.';
				for ( 0, 1) {
					my ( $a, $b) = ( $_, $_ + 2);
					my $lb = (( $$pos[$a] + $$pos[$b]) / 2) > $pp[$a]; # 1 if pointer is closer to left/bottom
					my $pdist = $lb ? $pp[$a] - $$pos[$a] : $$pos[$b] - $pp[$a];
					my $sz1 = $rc[$b] - $rc[$a];
					if ( $sz1 <= $pdist * 2) {
						$newpos[$a] = $pp[$a] - int( $sz1/2);
					} else {
						$newpos[$a] = $lb ? ( $pp[$a] - $pdist) : ( $pp[$a] + $pdist - $sz1);
					}
					$newpos[$b] = $newpos[$a] + $sz1;
				}
				# asking for the new position for the shape, if $dm can accept that...
				if ( 2 >= $relocationCount++) {
				#print "case1: @newpos\n";
				$pos = \@newpos;
				goto AGAIN;
				}
			} elsif ( $self-> {drag} && ( # have to change the position
					( $$pos[0] != $rc[0]) || ( $$pos[1] != $rc[1]))) {
				my @pp = $::application-> pointerPos;
				my @newpos = @pp;
				#print ',';
				for ( 0, 1) {
					my ( $a, $b) = ( $_, $_ + 2);
					$newpos[$a] = $rc[$a] if $newpos[$a] < $rc[$a];
					$newpos[$a] = $rc[$b] if $newpos[$a] > $rc[$b];
				}
				goto EXIT  if ( $sd < abs($pp[0] - $newpos[0])) || ( $sd < abs($pp[1] - $newpos[1]));
				# asking for the new position, and maybe new shape...
				if ( 2 >= $relocationCount++) {
				#print "case2: @rc\n";
				$pos = [@rc];
				goto AGAIN;
				}
			}
			goto EXIT if ($sd < abs($rc[0] - $$pos[0])) || ($sd < abs($rc[1] - $$pos[1]));
		}
		goto EXIT unless $self-> notify(q(Landing), $dm, @rc);
		#print "@rc\n";
		@retval = ($dm, \@rc);
	} elsif ( 1 == scalar @rc) { # new docker returned
		my $next = $rc[0];
		while ( $next) {
			my ( $dm_found, $npos) = $self-> find_docking( $next, $pos);
			@retval = ($dm_found, $npos), goto EXIT if $npos;
			$next = $dm-> next_docker( $sid, $pos ? @$pos[0,1] : ());
		}
	}
EXIT:
	unless ( $self-> {drag}) {
		$dm-> close_session( $sid);
		delete $self-> {sessions};
	}
	return @retval;
}

sub dock
{
	return $_[0]-> {dock} unless $#_;
	my ( $self, $dm, @rect) = @_;
	if ( $dm) {
		my %caps;
		my $stage = 0;
		my ( $sid, @rc, @s1rc);
AGAIN:
		if ( $self-> fingerprint && $dm-> fingerprint) {
			$self-> notify(q(GetCaps), $dm, \%caps);
			if ( keys %caps) { # $dm is user-approved
				unshift(@{$caps{sizes}}, [$rect[2] - $rect[0], $rect[3] - $rect[1]]) if scalar @rect;
				$caps{position} = [ @rect[0,1]] if scalar @rect;
				$caps{self}     = $self;
				$sid = $dm-> open_session( \%caps);
			}
		}
		return 0 unless $sid;
		@rc = $dm-> query( $sid, scalar(@rect) ? @rect : ());
		@s1rc = $dm-> rect;
		$dm-> close_session( $sid);
		if ( 1 == scalar @rc) { # readdress
			my ( $dm2, $rc) = $self-> find_docking( $dm, @rect ? [@rect] : ());
			$self-> dock( $dm2, $rc ? @$rc : ());
			return;
		}
		return 0 if 4 != scalar @rc;
		return 0 unless $self-> notify(q(Landing), $dm, @rc);

		unless ( $stage) {
			$self-> {dock}-> undock( $self) if $self-> {dock};
	# during the undock $dm may change its position ( and/or size), so retrying
			my @s2rc = $dm-> rect;
			if ( grep { $s1rc[$_] != $s2rc[$_] } (0..3)) {
				$stage = 1;
				goto AGAIN;
			}
		}
		$self-> hide;
		$self-> owner( $dm);
		my @sz = $self-> size;
		$dm-> close_session( $sid);

		if ( $rc[2] - $rc[0] == $sz[0] && $rc[3] - $rc[1] == $sz[1]) {
			$self-> origin( $self-> owner-> screen_to_client( @rc[0,1]));
		} else {
			$self-> rect( $self-> owner-> screen_to_client( @rc));
		}
		unless ( $self-> {dock}) {
			my $c = $self-> client;
			if ( $c) {
				$c-> owner( $self);
				$c-> clipOwner(1);
				$c-> rect( $self-> frame2client( 0, 0, $self-> width, $self-> height));
			}
			if ($self-> {externalDocker}) {
				my $d = $self-> {externalDocker};
				delete $self-> {externalDocker};
				$d-> destroy;
			}
		}
		$self-> {dock} = $dm;
		$self-> show;
		$dm-> dock( $self);
		$self-> notify(q(Dock));
	} else {
		return if $self-> {externalDocker};
		my $c = $self-> client;
		my $s = $c || $self;
		if ( defined $self-> {externalDockerModule}) {
			eval "use $self->{externalDockerModule};";
			die $@ if $@;
		}
		my $ed = $self-> {externalDockerClass}-> create(
			%{$self-> {externalDockerProfile}},
			visible => 0,
			shuttle => $self,
			owner   => $::application,
			text    => $self-> text,
			onClose => sub { $_[0]-> clear_event unless $self-> notify(q(EDSClose))},
		);
		my @r = $s-> owner-> client_to_screen( $s-> rect);
		$ed-> rect( $ed-> client2frame( @r));
		$ed-> origin( @rect[0,1]) if @rect;
		if ( $c) {
			$c-> set( map {'owner' . $_ => 0} qw( Font Hint Palette Color BackColor));
			$c-> owner( $ed-> client);
			$c-> clipOwner(1);
			$c-> origin( 0, 0);
		}
		if ( $self-> visible) {
			$ed-> show;
			$ed-> bring_to_front;
		}
		$self-> {externalDocker} = $ed;
		if ( $self-> {dock}) {
			$self-> {lastUsedDock} = [ $self-> {dock}, [$self-> owner-> client_to_screen( $self-> rect)]];
			$self-> {dock}-> undock( $self) if $self-> {dock};
			$self-> {dock} = undef;
		}
		$self-> hide;
		$self-> owner( $::application);
		$self-> notify(q(Undock));
	}
}

sub externalDocker
{
	return $_[0]-> {externalDocker} unless $#_;
}

sub dock_back
{
	my $self = $_[0];
	return if $self-> {dock};
	my ( $dm, $rect);
	if ( $self-> {lastUsedDock}) {
		( $dm, $rect) = @{$self-> {lastUsedDock}};
		delete $self-> {lastUsedDock};
	}
	if ( !defined($dm) || !Prima::Object::alive( $dm)) {
		( $dm, $rect) = $self-> find_docking( $self-> {dockingRoot});
	}
	return unless $dm;
	$self-> dock( $dm, $rect ? @$rect : ());
}

sub redock
{
	my $self = $_[0];
	return unless $self-> {dock};
	$self-> dock( undef);
	$self-> dock_back;
}

sub set_text
{
	$_[0]-> SUPER::set_text( $_[1]);
	$_[0]-> {externalDocker}-> text($_[1]) if $_[0]-> {externalDocker};
}

package Prima::ExternalDockerShuttle;
use vars qw(@ISA);
@ISA = qw(Prima::MDI);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my $fh = int($def-> {font}-> {height} / 1.5);
	my %prf = (
		titleHeight    => $fh + 4,
		borderIcons    => bi::TitleBar | ( bi::TitleBar << 1 ),
		clipOwner      => 0,
		shuttle        => undef,
		borderStyle    => bs::Dialog,
	);
	@$def{keys %prf} = values %prf;
	$def-> {font}-> {height} = $fh;
	$def-> {font}-> {width}  = 0;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> $_($profile{$_}) for qw(shuttle);
	return %profile;
}

sub shuttle
{
	return $_[0]-> {shuttle} unless $#_;
	$_[0]-> {shuttle} = $_[1];
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	if (q(caption) ne $self-> xy2part( $x, $y)) {
		$self-> SUPER::on_mousedown( $btn, $mod, $x, $y);
		return;
	}
	$self-> clear_event;
	return if $self-> {mouseTransaction};
	$self-> bring_to_front;
	$self-> select;
	return if $btn != mb::Left;
	my $s = $self-> shuttle;
	if ( $s-> client) {
		$s-> rect( $s-> client2frame( $s-> client-> rect));
	} else {
		$s-> rect( $self-> frame2client( $self-> rect));
	}
	$s-> drag( 1, [ $self-> rect], $s-> screen_to_client( $self-> client_to_screen($x, $y)));
	$self-> clear_event;
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	if (!$dbl || (q(caption) ne $self-> xy2part( $x, $y))) {
		$self-> SUPER::on_mouseclick( $btn, $mod, $x, $y, $dbl);
		return;
	}
	$self-> clear_event;
	$self-> shuttle-> dock_back;
}

sub windowState
{
	return $_[0]-> {windowState} unless $#_;
	my ( $self, $ws) = @_;
	if ( $ws == ws::Maximized) {
		$self-> shuttle-> dock_back;
	} else {
		$self-> SUPER::windowState( $ws);
	}
}

package Prima::LinearDockerShuttle;
use vars qw(@ISA);
@ISA = qw(Prima::InternalDockerShuttle);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		indent        => 2,
		headerBreadth => 8,
		vertical      => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	$self-> {$_} = 0 for ( qw(indent headerBreadth vertical));
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw(indent headerBreadth vertical));
	return %profile;
}

sub indent
{
	return $_[0]-> {indent} unless $#_;
	my ($self, $i) = @_;
	$i ||= 0;
	$i = 0 if $i < 0;
	return if $i == $self-> {indent};
	$self-> {indent} = $i;
	$self-> update_indents;
}

sub headerBreadth
{
	return $_[0]-> {headerBreadth} unless $#_;
	my ($self, $i) = @_;
	$i ||= 0;
	$i = 0 if $i < 0;
	return if $i == $self-> {headerBreadth};
	$self-> {headerBreadth} = $i;
	$self-> update_indents;
}


sub vertical
{
	return $_[0]-> {vertical} unless $#_;
	my ($self, $i) = @_;
	$i ||= 0;
	$i = 0 if $i < 0;
	return if $i == $self-> {vertical};
	$self-> {vertical} = $i;
	$self-> update_indents;
	$self-> repaint;
}

sub update_indents
{
	my $self = $_[0];
	my $vs   = $self-> { vertical};
	my $i    = $self-> {indent};
	my $hb   = $self-> {headerBreadth};
	$self-> indents([ $vs ? $i : $i + $hb, $i, $i, $vs ? $i + $hb : $i]);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my $vs = $self-> {vertical};
	my $i  = $self-> {indent};
	my $hb = $self-> {headerBreadth};
	my @sz = $self-> size;
	my @rc = ( $self-> light3DColor, $self-> dark3DColor);
	$canvas-> clear( 1, 1, $sz[0] - 2, $sz[1] - 2);
	$canvas-> color( $rc[1]);
	$canvas-> rectangle( 0, 0, $sz[0] - 1, $sz[1] - 1);
	my $j;
	for ( $j = $i; $j < $hb; $j += 4) {
		$vs ?
		$canvas-> rect3d( $i, $sz[1] - 3 - $j, $sz[0] - $i - 1, $sz[1] - 1 - $j, 1, @rc) :
		$canvas-> rect3d( $j, $i, $j+2, $sz[1] - $i - 1, 1, @rc);
	}
}

1;

=pod

=head1 NAME

Prima::Docks - dockable widgets

=head1 DESCRIPTION

The module contains a set of classes and an implementation of the dockable widgets
interface. The interface assumes two parties, the dockable widget,
and the dock widget; the generic methods for the dock widget class are contained in
the C<Prima::AbstractDocker::Interface> package.

=head1 USAGE

A dockable widget is required to take particular steps before it may land on a
dock widget. It needs to talk to the dock and find out if it is allowed to
land, or if the dock contains children dock widgets that might suit better for
the docking. If there's more than one dock widget in the program, the dockable
widget can select between the targets; this is especially actual when a
dockable widget is dragged by the mouse and the landing arbitration is based
on geometrical distance.

The interface implies that there exists at least one tree-like hierarchy of
dock widgets, linked up to a root dock widget. The hierarchy is not required to
follow parent-child relationships although this is the default behavior.  All
dockable widgets are expected to know explicitly what hierarchy tree they wish
to dock to. C<Prima::InternalDockerShuttle> introduces the C<dockingRoot> property
for this purpose.

The conversation between parties starts when a dockable widget calls the
C<open_session> method of the dock. The dockable widget passes a set of
parameters signaling if the widget is ready to change its size in case the dock
widget requires so, and how. The C<open_session> method can either refuse or accept
the widget.  In case of the positive answer from C<open_session>, the dockable
widget calls the C<query> method, which either returns a new rectangle or
another dock widget.  In the latter case, the caller can enumerate all
available dock widgets by repetitive calls to the C<next_docker> method. The
session is closed by a C<close_session> call; after that, the widget is allowed
to land by setting its C<owner> to the dock widget, the C<rect> property to the
negotiated position and size, and finally calling the C<dock> method.

The C<open_session>/C<close_session> brackets cache all necessary
calculations once, making the C<query> call as light as possible. This design allows
a dockable widget when dragged, to repeatedly ask all reachable docks in an
optimized way. The docking sessions are kept open until the drag
session is finished.

The conversation can be schematized in the following code:

	my $dock = $self-> dockingRoot;
	my $session_id = $dock-> open_session({ self => $self });
	return unless $session_id;
	my @result = $dock-> query( $session_id, $self-> rect );
	if ( 4 == scalar @result) {       # new rectangle is returned
		if ( ..... is new rectangle acceptable ? ... ) {
			$dock-> close_session( $session_id);
			$dock-> dock( $self);
			return;
		}
	} elsif ( 1 == scalar @result) {  # another dock returned
		my $next = $result[0];
		while ( $next) {
			if ( ... is new docker acceptable? ....) {
				$dock-> close_session( $session_id);
				$next-> dock( $self);
				return;
			}
			$next = $dock-> next_docker( $session_id, $self-> origin );
		}
	}
	$dock-> close_session( $session_id);

Since even the simplified code is quite cumbersome, direct calls to
C<open_session> are rare. Instead, C<Prima::InternalDockerShuttle> implements
the C<find_docking> method which performs the arbitration automatically and
returns the appropriate dock widget.

C<Prima::InternalDockerShuttle> is the class that implements the dockable
widget functionality. It also provides a top-level window-like wrapper widget
for the dockable widget that hosts the widget automatically if it is not
docked.  By default, C<Prima::ExternalDockerShuttle> is used as the wrapper
widget class.

It is not required, however, to use either C<Prima::InternalDockerShuttle>
or C<Prima::AbstractDocker::Interface> to implement a dockable widget;
the only requirement is to respect the C<open_session>/C<close_session>
protocol.

The full hierarchy of widgets participating in the mechanism is as follows:

	Prima::AbstractDocker::Interface
		Prima::SimpleWidgetDocker
			Prima::ClientWidgetDocker
		Prima::LinearWidgetDocker
			Prima::SingleLinearWidgetDocker
		Prima::FourPartDocker

	Prima::InternalDockerShuttle
		Prima::LinearDockerShuttle

	Prima::ExternalDockerShuttle

All docker widget classes are derived from C<Prima::AbstractDocker::Interface>.
Depending on the specialization, they employ more or less sophisticated schemes
for arranging dockable widgets inside themselves. The most complicated scheme
is implemented in C<Prima::LinearWidgetDocker>; it does not allow children to
overlap, can rearrange the children, and resize itself when a widget is
docked or undocked.

The package provides only basic functionality. Module C<Prima::DockManager>
provides common dockable controls, - toolbars, panels, speed buttons, etc.
based on the C<Prima::Docks> module. See L<Prima::DockManager>.

=head1 Prima::AbstractDocker::Interface

Implements generic functionality of a docket widget. The class is
not derived from C<Prima::Widget>; is used as a secondary ascendant class
for the dock widget classes.

=head2 Properties

Since the class is not a C<Prima::Object> descendant, it provides only run-time
implementation of its properties. It is up to the descendant object whether the
properties are recognized during the creation stage or not.

=over

=item fingerprint INTEGER

A custom bit mask used by docking widgets to reject inappropriate dock widgets
at an early stage. The C<fingerprint> property is not a part of the protocol
and is not required to be present in the implementation of a dockable widget.

Default value: C<0x0000FFFF>

=item dockup DOCK_WIDGET

Selects the upper link in the dock widgets hierarchy tree. The upper link is
required to be a dock widget but is not required to be a direct or an indirect
parent. In this case, however, the maintenance of the link must be implemented
separately, for example:

	$self-> dockup( $upper_dock_not_parent );

	$upper_dock_not_parent-> add_notification( 'Destroy', sub {
		return unless $_[0] == $self-> dockup;
		undef $self-> {dockup_event_id};
		$self-> dockup( undef );
	}, $self);

	$self-> {destroy_id} = $self-> add_notification( 'Destroy', sub {
		$self-> dockup( undef );
	} unless $self-> {destroy_id};

=back

=head2 Methods

=over

=item add_subdocker SUBDOCK

Appends SUBDOCK to the list of children docker widgets. The items of the list are
returned by the C<next_docker> method.

=item check_session SESSION

A debugging procedure. Checks SESSION hash, and warns if its members are
invalid or incomplete. Returns 1 if no fatal errors were encountered;
0 otherwise.

=item close_session SESSION

Closes docking SESSION and frees the associated resources.

=item dock WIDGET

Called after WIDGET successfully finished negotiations with the dock widget and
changed its C<owner> property. The method adapts the dock widget layout and
lists the WIDGET into the list of docked widgets. The method does not change
the C<owner> property of the WIDGET.

The method must not be called directly.

=item dock_bunch @WIDGETS

Effectively docks set of WIDGETS by updating internal structures
and calling C<rearrange>.

=item docklings

Returns an array of docked widgets

=item next_docker SESSION, [ X, Y ]

Enumerates children docker widgets inside the SESSION; returns
one docker widget at a time. After the last widget returns
C<undef>.

The enumeration pointer is reset by the C<query> call.

X and Y are the coordinates of the point of interest.

=item open_session PROFILE

Opens a new docking session with parameters stored in the PROFILE hash.
Returns a session ID scalar in case of success, or C<undef> otherwise.
The following keys must be set in PROFILE:

=over

=item position ARRAY

Contains two integer coordinates of the desired position of
a widget in (X,Y) format in the screen coordinate system.

=item self WIDGET

The widget that is about to dock.

=item sizeable ARRAY

Contains two boolean flags, representing if the widget can be resized
to an arbitrary size, horizontally and vertically. The arbitrary resize
option is used as a last resort if the C<sizes> key does not contain the desired
size.

=item sizeMin ARRAY

Two integers; the minimal size that the widget can accept.

=item sizes ARRAY

Contains an array of points in the (X,Y) format; each point represents an
acceptable widget size. If both of the C<sizeable> flags are set to 0
and none of the C<sizes> can be accepted by the dock widget, C<open_session>
fails.

=back

=item query SESSION [ X1, Y1, X2, Y2 ]

Checks if a dockable widget can be landed on the dock.
If it can, returns a rectangle that the widget must be set to.
If coordinates ( X1 .. Y2 ) are specified, returns the
rectangle closest to these. If C<sizes> or C<sizeable>
keys of the C<open_session> profile were set, the returned size
might be different from the current docking widget size.

Once the caller finds the result appropriate, it is allowed to reparent under
the dock; after that, it must change its origin and size correspondingly to the
result, and then call C<dock>.

If the dock cannot accept the widget but contains children dock widgets,
returns the first child widget. The caller can use subsequent calls to
C<next_docker> to enumerate all the children docks. A call to C<query> resets
the internal enumeration pointer.

If the widget may not be landed, an empty array is returned.

=item rearrange

Effectively re-docks all the docked widgets. The effect is
as same as of

	$self-> redock_widget($_) for $self-> docklings;

but usually C<rearrange> is faster.

=item redock_widget WIDGET

Effectively re-docks the docked WIDGET. If WIDGET has a C<redock>
method in its namespace, it is called instead.

=item remove_subdocker SUBDOCK

Removes SUBDOCK from the list of children docker widgets.
See also L<add_subdocker>.

=item replace FROM, TO

Assigns the widget TO the same owner and size as FROM. The FROM widget
must be a docked widget.

=item undock WIDGET

Removes WIDGET from the list of docked widgets. The layout of the dock widget
can be changed after the execution of this method. The method does not change
the C<owner> property of WIDGET.

The method must not be called directly.

=back

=head1 Prima::SimpleWidgetDocker

A simple dock widget; accepts any widget that geometrically fits into it.
Allows overlapping of the docked widgets.

=head1 Prima::ClientWidgetDocker

A simple dock widget; accepts any widget that can cover all
dock's interior.

=head1 Prima::LinearWidgetDocker

A toolbar-like docking widget class. The implementation does not allow tiling
but can reshape the dock widget and rearrange the docked widgets if necessary.

C<Prima::LinearWidgetDocker> is orientation-dependent; its main axis, managed
by the C<vertical> property, aligns the docked widgets in 'lines', which in
turn are aligned by the opposite axis ( 'major' and 'minor' terms are used in
the code for the axes ).

=head2 Properties

=over

=item growable INTEGER

A combination of the C<grow::XXX> constants that describes how
the dock widget can be resized. The constants are divided into two
sets, direct and indirect, or, C<vertical> property independent and
dependent.

The first set contains explicitly named constants:

	grow::Left       grow::ForwardLeft       grow::BackLeft
	grow::Down       grow::ForwardDown       grow::BackDown
	grow::Right      grow::ForwardRight      grow::BackRight
	grow::Up         grow::ForwardUp         grow::BackUp

that select if the widget can grow in the direction shown.
These do not change meaning when C<vertical> changes, though they do
change the dock widget behavior. The second set does not affect
dock widget behavior when C<vertical> changes, however, the names
are not that illustrative:

	grow::MajorLess  grow::ForwardMajorLess  grow::BackMajorLess
	grow::MajorMore  grow::ForwardMajorMore  grow::BackMajorMore
	grow::MinorLess  grow::ForwardMinorLess  grow::BackMinorLess
	grow::MinorMore  grow::ForwardMinorMore  grow::BackMinorMore

The C<Forward> and C<Back> prefixes select if the dock widget can be
respectively expanded or shrunk in the given direction. C<Less> and
C<More> are equivalent to C<Left> and C<Right> when C<vertical> is 0,
and to C<Up> and C<Down> otherwise.

The use of constants from the second set is preferred.

Default value: 0

=item hasPocket BOOLEAN

A boolean flag, affects the possibility of a docked widget to reside
outside the dock widget inferior. If 1, a docked widget is allowed
to stay docked ( or dock into a position ) further on the major axis
( to the right when C<vertical> is 0, up otherwise ) as if there's
a 'pocket'. If 0, a widget is neither allowed to dock outside the
inferior nor is allowed to stay docked ( and is undocked automatically )
when the dock widget shrinks so that the docked widget cannot stay in
the dock boundaries.

Default value: 1

=item vertical BOOLEAN

Selects the major axis of the dock widget. If 1, it is vertical,
horizontal otherwise.

Default value: 0

=back

=head2 Events

=over

=item Dock

Called when the C<dock> method is successfully finished.

=item DockError WIDGET

Called when the C<dock> method is unsuccessfully finished. This only happens if WIDGET
does not follow the docking protocol, and inserts itself into a non-approved
area.

=item Undock

Called when C<undock> is finished.

=back

=head1 Prima::SingleLinearWidgetDocker

Descendant of C<Prima::LinearWidgetDocker>. In addition to the constraints
introduced by the ascendant class, C<Prima::SingleLinearWidgetDocker> allows
only one row ( or column, depending on the C<vertical> property value ) of docked
widgets.

=head1 Prima::FourPartDocker

Implementation of a docking widget that hosts four children docker widgets on
its sides and one in the center.  All of the children docks can grow and shrink
automatically so that the whole setup has an effect as if the dock borders are
dynamic.

=head2 Properties

=over

=item indents ARRAY

Contains four integers specifying the breadth of offset for
each side. The first integer is the width of the left side, the second - the height
of the bottom side, the third is the width of the right side, and the fourth - height
of the top side.

=item dockerClassLeft STRING

Assigns the class of the left-side dock window.

Default value: C<Prima::LinearWidgetDocker>.
Create-only property.

=item dockerClassRight STRING

Assigns the class of the right-side dock window.

Default value: C<Prima::LinearWidgetDocker>.
Create-only property.

=item dockerClassTop STRING

Assigns the class of the top-side dock window.

Default value: C<Prima::LinearWidgetDocker>.
Create-only property.

=item dockerClassBottom STRING

Assigns the class of the bottom-side dock window.

Default value: C<Prima::LinearWidgetDocker>.
Create-only property.

=item dockerClassClient STRING

Assigns the class of the center dock window.

Default value: C<Prima::ClientWidgetDocker>.
Create-only property.

=item dockerProfileLeft HASH

Assigns a hash of properties, passed to the left-side dock widget during the creation.

Create-only property.

=item dockerProfileRight HASH

Assigns a hash of properties, passed to the right-side dock widget during the creation.

Create-only property.

=item dockerProfileTop HASH

Assigns a hash of properties, passed to the top-side dock widget during the creation.

Create-only property.

=item dockerProfileBottom HASH

Assigns a hash of properties, passed to the bottom-side dock widget during the creation.

Create-only property.

=item dockerProfileClient HASH

Assigns a hash of properties, passed to the center dock widget during the creation.

Create-only property.

=item dockerDelegationsLeft ARRAY

Assigns delegated notifications of the left-side dock.

Create-only property.

=item dockerDelegationsRight ARRAY

Assigns delegated notifications of the right-side dock.

Create-only property.

=item dockerDelegationsTop ARRAY

Assigns delegated notifications of the top-side dock.

Create-only property.

=item dockerDelegationsBottom ARRAY

Assigns delegated notifications of the bottom-side dock.

Create-only property.

=item dockerDelegationsClient ARRAY

Assigns delegated notifications of the bottom-side dock.

Create-only property.

=item dockerCommonProfile HASH

Assigns a hash of properties, passed to all the five dock widgets during the creation.

Create-only property.

=back

=head1 Prima::InternalDockerShuttle

The class provides a container, or a 'shuttle', for a client widget, while is
docked to a C<Prima::AbstractDocker::Interface> descendant instance. The
functionality includes communicating with dock widgets, the user interface for
dragging and interactive dock selection, and a client widget container for the
non-docked state. The latter is implemented by reparenting the client widget
to an external shuttle widget, selected by the C<externalDockerClass> property.
Both user interfaces for the docked and the non-docked shuttle states are
minimal.

The class implements dockable widget functionality, served by
C<Prima::AbstractDocker::Interface>, and is derived from C<Prima::Widget>.

See also: L</Prima::ExternalDockerShuttle>.

=head2 Properties

=over

=item client WIDGET

Provides access to the client widget, which always resides either in
the internal or the external shuttle. By default, there is no client,
and any widget capable of changing its parent can be set as one.
After a widget is assigned as a client, its C<owner> and C<clipOwner>
properties must not be used.

Run-time only property.

=item dock WIDGET

Selects the dock widget that the shuttle is landed on. If C<undef>,
the shuttle is in the non-docked state.

Default value: C<undef>

=item dockingRoot WIDGET

Selects the root of the dock widgets hierarchy.
If C<undef>, the shuttle can only exist in the non-docked state.

Default value: C<undef>

See L</USAGE> for reference.

=item externalDockerClass STRING

Assigns the class of the external shuttle widget.

Default value: C<Prima::ExternalDockerShuttle>

=item externalDockerModule STRING

Assigns the module that contains the external shuttle widget class.

Default value: C<Prima::MDI> ( C<Prima::ExternalDockerShuttle> is derived from C<Prima::MDI> ).

=item externalDockerProfile HASH

Assigns a hash of properties, passed to the external shuttle widget during the creation.

=item fingerprint INTEGER

A custom bit mask used to reject inappropriate dock widgets at an early stage.

Default value: C<0x0000FFFF>

=item indents ARRAY

Contains four integers, specifying the breadth of offset in pixels for each
widget side in the docked state.

Default value: C<5,5,5,5>.

=item snapDistance INTEGER

A maximum offset, in pixels, between the actual shuttle coordinates and the
coordinates proposed by the dock widget, where the shuttle is allowed to land.
In other words, it is the distance between the dock and the shuttle when the
latter 'snaps' to the dock during the dragging session.

Default value: 10

=item x_sizeable BOOLEAN

Selects whether the shuttle can change its width in case the dock widget suggests so.

Default value: 0

=item y_sizeable BOOLEAN

Selects whether the shuttle can change its height in case the dock widget suggests so.

Default value: 0

=back

=head2 Methods

=over

=item client2frame X1, Y1, X2, Y2

Returns the rectangle that the shuttle would occupy if its client rectangle is
assigned to X1, Y1, X2, Y2 .

=item dock_back

Docks to the recent dock widget, if it is still available.

=item drag STATE, RECT, ANCHOR_X, ANCHOR_Y

Initiates or aborts the dragging session, depending on the STATE boolean
flag.

If it is 1, RECT is an array with the coordinates of the shuttle rectangle
before the session has started; ANCHOR_X and ANCHOR_Y are coordinates of the
aperture point where the mouse event occurred that has initiated the session.
Depending on how the drag session ended, the shuttle can be relocated to
another dock, undocked, or left intact. Also, C<Dock>, C<Undock>, or
C<FailDock> notifications can be triggered.

If the STATE is 0, RECT, ANCHOR_X ,and ANCHOR_Y parameters are not used.

=item find_docking DOCK, [ POSITION ]

Opens a session with DOCK, unless it is already opened, and negotiates about
the possibility of landing ( at the POSITION if this parameter is
present ).

C<find_docking> caches the dock widget sessions and provides a possibility to
select different parameters passed to C<open_session> for different dock
widgets. To achieve this, the C<GetCaps> request notification is triggered,
which is expected to fill the parameters. The default action sets the C<sizeable>
option according to the C<x_sizeable> and C<y_sizeable> properties.

In case an appropriate landing area is found, the C<Landing>
notification is triggered with the proposed dock widget
and the target rectangle. The area can be rejected at this stage
if C<Landing> returns a negative answer.

On success, returns a dock widget found and the target rectangle;
the widget is not docked though. On failure returns an empty array.

This method is used by the mouse dragging routine to provide visual feedback to
the user, to indicate that a shuttle may or may not land in a particular area.

=item frame2client X1, Y1, X2, Y2

Returns the rectangle that the client would occupy if the shuttle rectangle is
assigned to X1, Y1, X2, Y2 .

=item redock

Undocks from the dock widget and immediately tries to land back.
If not docked, does not do anything.

=back

=head2 Events

=over

=item Dock

Called when the shuttle was docked.

=item EDSClose

Triggered when the user presses the close button or otherwise activates the
C<close> function of the EDS ( external docker shuttle ) pseudo-window. To
cancel the window closing C<clear_event> must be called inside the event handler.

=item FailDock X, Y

Called after the dragging session in the non-docked stage was finished
but did not result in docking. X and Y are the coordinates
of the new external shuttle position.

=item GetCaps DOCK, PROFILE

Called before the shuttle opens a docking session with the DOCK
widget. PROFILE is a hash reference, which is to be filled
inside the event handler. After that PROFILE is passed
to an C<open_session> call.

The default action sets the C<sizeable> option according to the C<x_sizeable>
and C<y_sizeable> properties.

=item Landing DOCK, X1, Y1, X2, Y2

Called inside the docking session, after an appropriate dock widget is selected
and the landing area is defined as X1, Y1, X2, Y2. To reject the landing on
either DOCK or area, C<clear_event> must be called.

=item Undock

Called when the shuttle is switched to the non-docked state.

=back

=head1 Prima::ExternalDockerShuttle

A shuttle class, hosts a client of the C<Prima::InternalDockerShuttle> widget when
it is in the non-docked state. The widget is a pseudo-window with some minimal
decorations that can be moved, resized ( this feature is not on by default
though ), and closed.

C<Prima::ExternalDockerShuttle> is inherited from the C<Prima::MDI> class, and
its window-emulating functionality is a subset of its ascendant.  See also
L<Prima::MDI>.

=head2 Properties

=over

=item shuttle WIDGET

Contains the reference to the dockable WIDGET

=back

=head1 Prima::LinearDockerShuttle

A simple descendant of C<Prima::InternalDockerShuttle>, used
for toolbars. Introduces orientation and draws a tiny header along
the minor shuttle axis. 

=head2 Properties

=over

=item headerBreadth INTEGER

The breadth of the header in pixels.

Default value: 8

=item indent INTEGER

A wrapper to the C<indents> property; besides the
space for the header, all indents are assigned to the C<indent>
property value.

=item vertical BOOLEAN

If 1, the shuttle is drawn as a vertical bar.
If 0, the shuttle is drawn as a horizontal bar.

Default value: 0

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::MDI>, L<Prima::DockManager>, F<examples/dock.pl>

=cut
