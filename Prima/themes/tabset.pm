# makes notebook tabset non-colored

Prima::Themes::register( 'Prima::themes::tabset', 'tabset-gray', [ 'Prima::TabSet' => { colored => 0}]);
Prima::Themes::register( 'Prima::themes::tabset', 'tabset-warp', [ 'Prima::TabSet' => { colored => 1}]);
