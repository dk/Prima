use Prima;
use Prima::ColorDialog;
use Prima::Application name => 'CV';

Prima::ColorDialog-> create(
  value => 0x3030F0,
  visible => 1,
  quality => 1,
  onDestroy => sub {$::application-> close;},
);


run Prima;


