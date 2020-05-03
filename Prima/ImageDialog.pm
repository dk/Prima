package Prima::ImageDialog;
use base qw(Prima::Dialog::ImageDialog);
package Prima::ImageOpenDialog;
use base qw(Prima::Dialog::ImageOpenDialog);
package Prima::ImageSaveDialog;
use base qw(Prima::Dialog::ImageSaveDialog);
warn("Warning: Prima::ImageDialog is deprecated. Use Prima::Dialog::ImageDialog instead\n");
1;
