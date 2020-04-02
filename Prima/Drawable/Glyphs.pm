package Prima::Drawable::Glyphs;

sub new         { bless [grep { defined } @_[1..4]], $_[0] }
sub glyphs      { $_[0]->[0] }
sub clusters    { $_[0]->[1] }
sub advances    { $_[0]->[2] }
sub positions   { $_[0]->[3] }
sub text_length { $_[0]->[0]->[-1] }
sub a           { $_[0]->[2]->[-2] }
sub c           { $_[0]->[2]->[-1] }

1;

