package Prima::VB::Config;

use strict;
use warnings;

sub pages
{
	return qw(General Additional Sliders Abstract);
}

sub classes
{
	return (
	'Prima::Gauge' => {
		icon     => 'VB::classes.gif:9',
		page     => 'Sliders',
		class    => 'Prima::VB::Gauge',
		RTModule => 'Prima::Sliders',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ProgressBar' => {
		icon     => 'VB::classes.gif:34',
		page     => 'Sliders',
		RTModule => 'Prima::Sliders',
		class    => 'Prima::VB::ProgressBar',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::StringOutline' => {
		icon     => 'VB::classes.gif:17',
		page     => 'General',
		class    => 'Prima::VB::StringOutline',
		RTModule => 'Prima::Outlines',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ImageViewer' => {
		icon     => 'VB::classes.gif:14',
		page     => 'General',
		class    => 'Prima::VB::ImageViewer',
		RTModule => 'Prima::ImageViewer',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Label' => {
		icon     => 'VB::classes.gif:15',
		page     => 'General',
		class    => 'Prima::VB::Label',
		RTModule => 'Prima::Label',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::GroupBox' => {
		icon     => 'VB::classes.gif:10',
		page     => 'General',
		class    => 'Prima::VB::GroupBox',
		RTModule => 'Prima::Buttons',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::InputLine' => {
		icon     => 'VB::classes.gif:13',
		page     => 'General',
		class    => 'Prima::VB::InputLine',
		RTModule => 'Prima::InputLine',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ScrollWidget' => {
		icon     => 'VB::classes.gif:21',
		page     => 'Abstract',
		class    => 'Prima::VB::ScrollWidget',
		RTModule => 'Prima::ScrollWidget',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Widget' => {
		icon     => 'VB::VB.gif:0',
		page     => 'Abstract',
		class    => 'Prima::VB::Widget',
		RTModule => 'Prima::Classes',
		module   => 'Prima::VB::Classes',
	},
	'Prima::OutlineViewer' => {
		icon     => 'VB::classes.gif:17',
		page     => 'Abstract',
		class    => 'Prima::VB::OutlineViewer',
		RTModule => 'Prima::Outlines',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::SpinEdit' => {
		icon     => 'VB::classes.gif:25',
		page     => 'Sliders',
		class    => 'Prima::VB::SpinEdit',
		RTModule => 'Prima::Sliders',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Calendar' => {
		icon     => 'VB::classes.gif:32',
		page     => 'Additional',
		class    => 'Prima::VB::Calendar',
		RTModule => 'Prima::Calendar',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::DirectoryListBox' => {
		icon     => 'VB::classes.gif:6',
		page     => 'Additional',
		class    => 'Prima::VB::DirectoryListBox',
		RTModule => 'Prima::Dialog::FileDialog',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ComboBox' => {
		icon     => 'VB::classes.gif:3',
		page     => 'General',
		class    => 'Prima::VB::ComboBox',
		RTModule => 'Prima::ComboBox',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Radio' => {
		icon     => 'VB::classes.gif:18',
		page     => 'General',
		class    => 'Prima::VB::Radio',
		RTModule => 'Prima::Buttons',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::SpeedButton' => {
		icon     => 'VB::classes.gif:19',
		page     => 'Additional',
		class    => 'Prima::VB::Button',
		RTModule => 'Prima::Buttons',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ListBox' => {
		icon     => 'VB::classes.gif:16',
		page     => 'General',
		class    => 'Prima::VB::ListBox',
		RTModule => 'Prima::Lists',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::CheckBox' => {
		icon     => 'VB::classes.gif:2',
		page     => 'General',
		class    => 'Prima::VB::CheckBox',
		RTModule => 'Prima::Buttons',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::CircularSlider' => {
		icon     => 'VB::classes.gif:4',
		page     => 'Sliders',
		class    => 'Prima::VB::CircularSlider',
		RTModule => 'Prima::Sliders',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::DetailedList' => {
		icon     => 'VB::classes.gif:31',
		page     => 'General',
		class    => 'Prima::VB::DetailedList',
		RTModule => 'Prima::DetailedList',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Edit' => {
		icon     => 'VB::classes.gif:8',
		page     => 'General',
		class    => 'Prima::VB::Edit',
		RTModule => 'Prima::Edit',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ColorComboBox' => {
		icon     => 'VB::classes.gif:1',
		page     => 'Additional',
		class    => 'Prima::VB::ColorComboBox',
		RTModule => 'Prima::Dialog::ColorDialog',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Slider' => {
		icon     => 'VB::classes.gif:22',
		page     => 'Sliders',
		class    => 'Prima::VB::Slider',
		RTModule => 'Prima::Sliders',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::AltSpinButton' => {
		icon     => 'VB::classes.gif:24',
		page     => 'Sliders',
		class    => 'Prima::VB::AltSpinButton',
		RTModule => 'Prima::Sliders',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Header' => {
		icon     => 'VB::classes.gif:30',
		page     => 'Sliders',
		class    => 'Prima::VB::Header',
		RTModule => 'Prima::Header',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::DirectoryOutline' => {
		icon     => 'VB::classes.gif:7',
		page     => 'Additional',
		class    => 'Prima::VB::DirectoryOutline',
		RTModule => 'Prima::Outlines',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Button' => {
		icon     => 'VB::classes.gif:0',
		page     => 'General',
		class    => 'Prima::VB::Button',
		RTModule => 'Prima::Buttons',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::TabSet' => {
		icon     => 'VB::classes.gif:27',
		page     => 'Additional',
		class    => 'Prima::VB::TabSet',
		RTModule => 'Prima::Notebooks',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ScrollBar' => {
		icon     => 'VB::classes.gif:20',
		page     => 'General',
		class    => 'Prima::VB::ScrollBar',
		RTModule => 'Prima::ScrollBar',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::ListViewer' => {
		icon     => 'VB::classes.gif:16',
		page     => 'Abstract',
		class    => 'Prima::VB::ListViewer',
		RTModule => 'Prima::Lists',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::TabbedNotebook' => {
		icon     => 'VB::classes.gif:28',
		page     => 'Additional',
		class    => 'Prima::VB::TabbedNotebook',
		RTModule => 'Prima::Notebooks',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Notebook' => {
		icon     => 'VB::classes.gif:29',
		page     => 'Abstract',
		class    => 'Prima::VB::Notebook',
		RTModule => 'Prima::Notebooks',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::DriveComboBox' => {
		icon     => 'VB::classes.gif:5',
		page     => 'Additional',
		class    => 'Prima::VB::DriveComboBox',
		RTModule => 'Prima::Dialog::FileDialog',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::SpinButton' => {
		icon     => 'VB::classes.gif:23',
		page     => 'Sliders',
		class    => 'Prima::VB::SpinButton',
		RTModule => 'Prima::Sliders',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::Grid' => {
		RTModule => 'Prima::Grids',
		class  => 'Prima::VB::Grid',
		page   => 'General',
		icon   => 'VB::classes.gif:33',
		module   => 'Prima::VB::CoreClasses',
	},
	'Prima::AbstractGrid' => {
		RTModule => 'Prima::Grids',
		class  => 'Prima::VB::AbstractGrid',
		page   => 'Abstract',
		icon   => 'VB::classes.gif:33',
		module   => 'Prima::VB::CoreClasses',
	},
	);
}


1;
