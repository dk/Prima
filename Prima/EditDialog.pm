package Prima::FindDialog;
use base qw(Prima::Dialog::FindDialog);
package Prima::ReplaceDialog;
use base qw(Prima::Dialog::ReplaceDialog);
warn("Warning: Prima::EditDialog is deprecated. Use Prima::Dialog::FindDialog instead\n");
1;
