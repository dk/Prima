package Prima::VB::Config;

sub pages
{
   return qw(General Additional Abstract);
}

sub classes
{
   return (
   Prima::DriveComboBox => {
      icon     => 'VB::classes.gif:5',
      RTModule => 'Prima::ComboBox',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::DriveComboBox',
   },
   Prima::ScrollWidget => {
      icon     => 'VB::classes.gif:21',
      RTModule => 'Prima::ScrollWidget',
      page     => 'Abstract',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::ScrollWidget',
   },
   Prima::CheckBoxGroup => {
      icon     => 'VB::classes.gif:11',
      RTModule => 'Prima::Buttons',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::GroupCheckBox',
   },
   Prima::ImageViewer => {
      icon     => 'VB::classes.gif:14',
      RTModule => 'Prima::ImageViewer',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::ImageViewer',
   },
   Prima::SpinButton => {
      icon     => 'VB::classes.gif:23',
      RTModule => 'Prima::Sliders',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::SpinButton',
   },
   Prima::AltSpinButton => {
      icon     => 'VB::classes.gif:24',
      RTModule => 'Prima::Sliders',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::AltSpinButton',
   },
   Prima::ColorComboBox => {
      icon     => 'VB::classes.gif:1',
      RTModule => 'Prima::ColorDialog',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::ColorComboBox',
   },
   Prima::RadioGroup => {
      icon     => 'VB::classes.gif:12',
      RTModule => 'Prima::Buttons',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::GroupRadioBox',
   },
   Prima::Label => {
      icon     => 'VB::classes.gif:15',
      RTModule => 'Prima::Label',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Label',
   },
   Prima::TabbedNotebook => {
      icon     => 'VB::classes.gif:28',
      RTModule => 'Prima::Notebooks',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::TabbedNotebook',
   },
   Prima::Slider => {
      icon     => 'VB::classes.gif:22',
      RTModule => 'Prima::Sliders',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Slider',
   },
   Prima::ScrollBar => {
      icon     => 'VB::classes.gif:20',
      RTModule => 'Prima::ScrollBar',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::ScrollBar',
   },
   Prima::Edit => {
      icon     => 'VB::classes.gif:8',
      RTModule => 'Prima::Edit',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Edit',
   },
   Prima::CheckBox => {
      icon     => 'VB::classes.gif:2',
      RTModule => 'Prima::Buttons',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::CheckBox',
   },
   Prima::Gauge => {
      icon     => 'VB::classes.gif:9',
      RTModule => 'Prima::Sliders',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Gauge',
   },
   Prima::SpeedButton => {
      icon     => 'VB::classes.gif:19',
      RTModule => 'Prima::Buttons',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Button',
   },
   Prima::Radio => {
      icon     => 'VB::classes.gif:18',
      RTModule => 'Prima::Buttons',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Radio',
   },
   Prima::OutlineViewer => {
      icon     => 'VB::classes.gif:17',
      RTModule => 'Prima::Outlines',
      page     => 'Abstract',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::OutlineViewer',
   },
   Prima::DirectoryOutline => {
      icon     => 'VB::classes.gif:7',
      RTModule => 'Prima::Outlines',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::DirectoryOutline',
   },
   Prima::GroupBox => {
      icon     => 'VB::classes.gif:10',
      RTModule => 'Prima::Buttons',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::GroupBox',
   },
   Prima::StringOutline => {
      icon     => 'VB::classes.gif:17',
      RTModule => 'Prima::Outlines',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::StringOutline',
   },
   Prima::InputLine => {
      icon     => 'VB::classes.gif:13',
      RTModule => 'Prima::InputLine',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::InputLine',
   },
   Prima::CircularSlider => {
      icon     => 'VB::classes.gif:4',
      RTModule => 'Prima::Sliders',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::CircularSlider',
   },
   Prima::ComboBox => {
      icon     => 'VB::classes.gif:3',
      RTModule => 'Prima::ComboBox',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::ComboBox',
   },
   Prima::DirectoryListBox => {
      icon     => 'VB::classes.gif:6',
      RTModule => 'Prima::Lists',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::DirectoryListBox',
   },
   Prima::Button => {
      icon     => 'VB::classes.gif:0',
      RTModule => 'Prima::Buttons',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Button',
   },
   Prima::SpinEdit => {
      icon     => 'VB::classes.gif:25',
      RTModule => 'Prima::Sliders',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::SpinEdit',
   },
   Prima::Notebook => {
      icon     => 'VB::classes.gif:29',
      RTModule => 'Prima::Notebooks',
      page     => 'Abstract',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Notebook',
   },
   Prima::TabSet => {
      icon     => 'VB::classes.gif:27',
      RTModule => 'Prima::Notebooks',
      page     => 'Additional',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::TabSet',
   },
   Prima::Widget => {
      icon     => 'VB::VB.gif:0',
      RTModule => 'Prima::Classes',
      page     => 'Abstract',
      module   => 'Prima::VB::Classes',
      class    => 'Prima::VB::Widget',
   },
   Prima::ListBox => {
      icon     => 'VB::classes.gif:16',
      RTModule => 'Prima::Lists',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::ListBox',
   },
   Prima::ListViewer => {
      icon     => 'VB::classes.gif:16',
      RTModule => 'Prima::Lists',
      page     => 'Abstract',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::ListViewer',
   },
   );
}


1;
