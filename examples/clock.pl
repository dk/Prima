#!/usr/bin/env perl
use strict;
use warnings;
use Prima qw(Application Widget::DateComboBox);

my $mw = Prima::MainWindow->new( size => [200, 200]);

$mw->insert( 'Widget::DateComboBox' =>
	pack => { side => 'bottom', fill => 'x', pad => 20 },
);

$mw->insert( 'Widget' =>
	pack => { side => 'bottom', fill => 'both', pad => 20, expand => 1 },
	backColor => 0,
);

run Prima;
