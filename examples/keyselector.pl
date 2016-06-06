=pod 

=item NAME

Interactive key selection and storage

=item FEATURES

Contains an example how to use standard key selection and conversion
features of Prima::KeySelector. A typical program with interactively
configurable keys reflects the changes of key assignments in menu
hot-keys and initialization file. In the example, changes are stored
in ~/.prima/keyselector INI-file.

=cut

use strict;
use Prima qw(Application KeySelector IniFile );

my $w = Prima::MainWindow-> create(
		text => 'Key selector',
		packPropagate => 0,
		menuItems => [
			[ File => [
			['FileExit' => "~Exit and save changes" => '' , '@X', => sub { $_[0]-> close } ],
			['FileQuit' => "Exit and ~discard changes", '', km::Alt|km::Shift|ord('X') => sub { 
				$_[0]-> {saveOnExit} = 0;
				$_[0]-> close;
			}],
			]],
			[ Edit => [
			[ 'EditApply'    => "~Apply changes", '', '^A' => sub {
				shift-> Editor-> apply;
			}],
			[ 'EditDefault'  => "~Restore to defaults" => sub {
				my $self = shift;
				$self-> menu-> keys_reset;
				$self-> Editor-> reset;
			}],
			]],
		],
		onClose => sub { $_[0]-> menu-> keys_save( $_[0]-> {iniFile}-> section('Options')) if $_[0]-> {saveOnExit}},
);
$w-> {saveOnExit} = 1;

$w-> {iniFile} = Prima::IniFile-> create(
	file => Prima::Utils::path('keyselector'),
);
$w-> menu-> keys_load($w-> {iniFile}-> section( 'Options'));

$w-> insert( 'Prima::KeySelector::MenuEditor' => 
	name         => 'Editor',
	pack         => { expand => 1, fill => 'both'},
	menu         => $w-> menu,
);

run Prima;
