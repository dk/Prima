#
#  Created by Dmitry Karasik  <dmitry@karasik.eu.org>
#
# $Id$
#
# Initializes Prima so that it skips parsing @ARGV;

push @Prima::preload, 'noargv';
1;
