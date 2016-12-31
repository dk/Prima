# Initializes Prima so that it skips parsing @ARGV;
use strict;
use warnings;

push @Prima::preload, 'noargv';
1;
