# Initializes Prima so that it skips parsing @ARGV;
use strict;
use warnings;
no warnings 'once';
push @Prima::preload, 'noargv';
1;
