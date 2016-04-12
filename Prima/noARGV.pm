# Initializes Prima so that it skips parsing @ARGV;
push @Prima::preload, 'noargv';
1;
