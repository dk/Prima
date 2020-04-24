# convert .afm fonts
use strict;
use warnings;
for my $afm (<*.afm>) {
	my $pm = "1/$afm";
	$pm =~ s/\.afm$//;

	open F, "<", $afm or die $!;
	my (@c, %fm);
	while (<F>) {
		chomp;
		m/^FontName\s+(.*)$/      and $fm{name}         = $1;
		m/^FamilyName\s+(.*)$/    and $fm{family}       = $1;
		m/^IsFixedPitch\s+(.*)$/  and $fm{pitch}        = ($1 eq 'false') ? 'fp::Variable' : 'fp::Fixed';
		m/^Weight\s+(.*)$/        and $fm{weight}       = $1;
		m/^ItalicAngle\s+(.*)$/   and $fm{italic}       = ($1 ne '0');
		m/^EncodingScheme\s+(.*)$/ and $fm{encoding}    = ($1 eq 'FontSpecific') ? 'Specific' : 'Latin1';
		m/^FontBBox\s+(\-?\d+)\s+(\-?\d+)\s+(\-?\d+)\s+(\-?\d+)\s+$/ and $fm{bbox} = [$1,$2,$3,$4];
		m/C\s+(\-?\d+)\s+;\s+WX\s+(\d+)\s+;\s+N\s+(\S+)\s+;\s+B\s+(\-?\d+)\s+(\-?\d+)\s+(\-?\d+)\s+(\-?\d+)\s+;/ and push @c, [$1, $2, $3, $4, $5, $6, $7];
	}
	close F;

	$fm{weight}  = 'Medium' if $fm{weight} !~ /^(Medium|Bold|Light)$/;
	$fm{ascent}  = $fm{bbox}->[3];
	$fm{descent} = -$fm{bbox}->[1];
	$fm{height}  = $fm{ascent} + $fm{descent};
	$fm{internalLeading} = $fm{height} - 1000;
	$fm{externalLeading} = int(abs($fm{internalLeading}) / 3);
	$fm{width}   = $fm{maximalWidth} = $fm{bbox}->[3] - $fm{bbox}->[1];
	if ($fm{weight} eq 'Bold') {
		$fm{style} = $fm{italic} ? 'fs::Bold|fs::Italic' : 'fs::Bold';
	} else {
		$fm{style} = $fm{italic} ? 'fs::Italic' : 'fs::Normal';
	}
	$fm{defaultChar} = $fm{firstChar} = $fm{breakChar} = $c[0]->[0];
	for ( @c ) {
		next if $_->[0] < 0;
		$fm{lastChar} = $_->[0];
	}

	open F, ">:raw", $pm or die $!;
	print F <<HDR;
('$fm{name}' => {
  name            => '$fm{name}',
  family          => '$fm{family}',
  height          => $fm{height},
  weight          => fw::$fm{weight},
  style           => $fm{style},
  pitch           => $fm{pitch},
  vector          => 1,
  ascent          => $fm{ascent},
  descent         => $fm{descent},
  maximalWidth    => $fm{maximalWidth},
  width           => $fm{width},
  internalLeading => $fm{internalLeading},
  externalLeading => $fm{externalLeading},
  firstChar       => $fm{firstChar},
  lastChar        => $fm{lastChar},
  breakChar       => $fm{breakChar},
  defaultChar     => $fm{defaultChar},
  xDeviceRes      => 72.27,
  yDeviceRes      => 72.27,
  size            => 1000,
  encoding        => '$fm{encoding}',
  chardata        => {
HDR

	my ( $a, $b, $c, $d, $e, $f);
	my @save;
	for (@c) {
		my ($wx, $x, $y, $x2, $y2) = @$_[1,3,4,5,6];
		$a = $x;
		$d = $y + $fm{descent};
		$b = $x2 - $x;
		$e = $y2 - $y;
		$c = $wx - $a - $b;
		$f = $fm{height} - $d - $e;
		@save = ( $a,$b,$c,$d,$e,$f) unless @save;
		print F "$$_[2] => [$$_[0],$a,$b,$c,$d,$e,$f],\n";
	}
	( $a, $b, $c, $d, $e, $f) = @save;
	print F "'.notdef' => [-1,$a,$b,$c,$d,$e,$f]}});\n";
}
