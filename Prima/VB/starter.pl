use Prima qw(Application VB::VBLoader);

my %ret = Prima::VB::VBLoader::AUTOFORM_CREATE( $ARGV[0],
  'Form1' => {
  }
);

run Prima;
