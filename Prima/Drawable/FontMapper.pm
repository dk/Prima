package Prima::Drawable::FontMapper;

use strict;
use warnings;
use Prima;

sub new
{
	my ( $class, %opt ) = @_;
	my $self = bless {
		active_fonts  => [],
		passive_fonts => {},
		%opt,
		map => '',
	}, $class;
	$self-> {canvas} //= Prima::DeviceBitmap->new( size => [1,1], type => dbt::Bitmap );
	$self-> {default_font} = $self->canvas->font->name;
	$self-> enum_fonts;
	$self-> fill_bidi_marks;
	return $self;
}

use constant RANGE_VECTOR_BASE => 11; # 2048 chars or 256 bytes per vector
use constant RANGE_VECTOR_MASK => ((1 << RANGE_VECTOR_BASE) - 1);

sub enum_fonts
{
	my $self = shift;
	my $p = $self->{passive_fonts};
	for my $f ( @{ $::application-> fonts } ) {
		$p->{$f->{name}} = {};
	}
}

sub fill_bidi_marks
{
	my $self = shift;
	my $fa = $self->{bidi_marks} = [];
	for my $j (
		0x200e .. 0x200f, # dir marks
		0x202a .. 0x202e, # embedding
		0x2066 .. 0x2069, # isolates
	) {
		my $base = $j >> RANGE_VECTOR_BASE;
		$fa->[$base] //= '';
		vec($fa->[$base], $j & RANGE_VECTOR_MASK, 1) = 1;
	}
}

sub canvas { $_[0]->{canvas} }

sub update_font_ranges
{
	my ( $self, $font ) = @_;

	my $f = $self->{passive_fonts};
	return if exists $f->{$font} and exists $f->{$font}->{range};

	my $ff = $f->{$font} //= {};
	my $fa = $ff->{range} //= [];

	my $c = $self->canvas;
	$c->font->name($font);
	my $r = $c->get_font_ranges;
	my %bases;
	for ( my $i = 0; $i < @$r; $i += 2) {
		my ( $from, $to ) = @$r[$i,$i+1];
		for ( my $j = $from; $j < $to; $j++) {
			my $base = $j >> RANGE_VECTOR_BASE;
			$fa->[$base] //= '';
			$bases{$base} = 1;
			vec($fa->[$base], $j & RANGE_VECTOR_MASK, 1) = 1;
		}
	}

	my $af = $self->{active_fonts};
	for my $base ( keys %bases ) {
		push @{ $af->[$base] }, $font;
	}

	return $fa;
}

sub find_font
{
	my ( $self, $c ) = @_;
	my ($rb,$rm) = ($c >> RANGE_VECTOR_BASE, $c & RANGE_VECTOR_MASK);

	my $af = $self->{active_fonts};
	my $pf = $self->{passive_fonts};
	if ( defined $af->[$rb] ) {
		for my $font ( @{ $af->[$rb] } ) {
			my $fa = $pf->{$font}->{range};
			return $font if defined($fa->[$rb]) && vec($fa->[$rb], $rm, 1);
		}
	}

	if ( ! exists $pf->{$self->{default_font}}->{range} ) {
		my $fa = $self-> update_font_ranges($self->{default_font});
		return $self->{default_font} if defined($fa->[$rb]) && vec($fa->[$rb], $rm, 1);
	}

	reset %$pf;
	while ( my ( $k, $v) = each %$pf ) {
		next if exists $v->{range};
		my $fa = $self-> update_font_ranges($k);
		return $k if defined($fa->[$rb]) && vec($fa->[$rb], $rm, 1);
	}
}

sub text2layout
{
	my ( $self, $text, $base_font ) = @_;
	$base_font //= $self-> {default_font};
	my $fa_base = $self-> update_font_ranges($base_font);
	my $fa_bidi = $self-> {bidi_marks};
	my ($last_found_fa, $last_found_font);
	my @ret;

	for my $c ( split //, $text ) {
		my ($rb,$rm) = (ord($c) >> RANGE_VECTOR_BASE, ord($c) & RANGE_VECTOR_MASK);
		if (
			(defined( $fa_base->[$rb] ) && vec( $fa_base->[$rb], $rm, 1 )) ||
			(defined( $fa_bidi->[$rb] ) && vec( $fa_bidi->[$rb], $rm, 1 ))
		) {
			if ( @ret && $ret[-2] eq $base_font ) {
				$ret[-1] .= $c;
			} else {
				push @ret, $base_font, $c;
			}
		} elsif ( defined($last_found_fa) && defined($last_found_fa->[$rb]) && vec($last_found_fa->[$rb], $rm, 1)) {
			if ( @ret && $ret[-2] eq $last_found_font ) {
				$ret[-1] .= $c;
			} else {
				push @ret, $last_found_font, $c;
			}
		} elsif ( my $f = $self-> find_font(ord($c))) {
			$last_found_font = $f;
			$last_found_fa = $self->{passive_fonts}->{$f}->{range};
			push @ret, $f, $c;
		} else {
			push @ret, $base_font, '' unless @ret;
			$ret[-1] .= $c;
		}
	}

	return @ret;
}

1;

=pod

=encoding utf-8

=head1 NAME

Prima::Drawable::FontMapper - substitute glyphs with necessary fonts

=head1 EXAMPLES

This section is only there to test proper rendering

=over

=item Latin

B<Lorem ipsum> dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.

   Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.

=item Latin combining

D̍üi̔s͙ a̸u̵t͏eͬ ịr͡u̍r͜e̥ d͎ǒl̋o̻rͫ i̮n̓ r͐e̔p͊rͨe̾h̍e͐n̔ḋe͠r̕i̾t̅ ịn̷ vͅo̖lͦuͦpͧt̪ątͅe̪ 

   v̰e̷l̳i̯t̽ e̵s̼s̈e̮ ċi̵l͟l͙u͆m͂ d̿o̙lͭo͕r̀e̯ ḛu̅ fͩuͧg̦iͩa̓ť n̜u̼lͩl͠a̒ p̏a̽r̗i͆a͆t̳űr̀

=item Cyrillic

B<Lorem Ipsum> используют потому, что тот обеспечивает более или менее стандартное заполнение шаблона.

а также реальное распределение букв и пробелов в абзацах

=item Hebrew

זוהי עובדה מבוססת שדעתו של הקורא תהיה מוסחת על ידי טקטס קריא כאשר הוא יביט בפריסתו.

  המטרה בשימוש ב-Lorem Ipsum הוא שיש לו פחות או יותר תפוצה של אותיות, בניגוד למלל

=item Arabic

العديد من برامح النشر المكتبي وبرامح تحرير صفحات الويب تستخدم لوريم إيبسوم بشكل إفتراضي 

  كنموذج عن النص، وإذا قمت بإدخال "lorem ipsum" في أي محرك بحث ستظهر العديد من


=item Indic

Lorem Ipsum के अंश कई रूप में उपलब्ध हैं, लेकिन बहुमत को किसी अन्य रूप में परिवर्तन का सामना करना पड़ा है, हास्य डालना या क्रमरहित शब्द , 

  जो तनिक भी विश्वसनीय नहीं लग रहे हो. यदि आप Lorem Ipsum के एक अनुच्छेद का उपयोग करने जा रहे हैं, तो आप को यकीन दिला दें कि पाठ के मध्य में वहाँ कुछ भी शर्मनाक छिपा हुआ नहीं है.

=item Chinese

无可否认，当读者在浏览一个页面的排版时，难免会被可阅读的内容所分散注意力。

  Lorem Ipsum的目的就是为了保持字母多多少少标准及平

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=cut

