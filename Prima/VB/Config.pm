package Prima::VB::Config;

sub pages
{
   return qw( General Additional Abstract);
}

sub classes
{
   return (
      'Prima::Widget' => {
         RTModule => 'Prima::Classes',
         module => 'Prima::VB::Classes',
         class  => 'Prima::VB::Widget',
         page   => 'Abstract',
         icon   => 'VB::classes.gif:26',
      },
      'Prima::Button' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Button',
         page   => 'General',
         icon   => 'VB::classes.gif:0',
      },
      'Prima::SpeedButton' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Button',
         page   => 'Additional',
         icon   => 'VB::classes.gif:19',
      },
      'Prima::Label' => {
         RTModule => 'Prima::Label',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Label',
         page   => 'General',
         icon   => 'VB::classes.gif:15',
      },
      'Prima::InputLine' => {
         RTModule => 'Prima::InputLine',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::InputLine',
         page   => 'General',
         icon   => 'VB::classes.gif:13',
      },
      'Prima::ListBox' => {
         RTModule => 'Prima::Lists',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ListBox',
         page   => 'General',
         icon   => 'VB::classes.gif:16',
      },
      'Prima::DirectoryListBox' => {
         RTModule => 'Prima::Lists',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::DirectoryListBox',
         page   => 'Additional',
         icon   => 'VB::classes.gif:6',
      },
      'Prima::ListViewer' => {
         RTModule => 'Prima::Lists',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ListBox',
         page   => 'Abstract',
         icon   => 'VB::classes.gif:16',
      },
      'Prima::CheckBox' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::CheckBox',
         page   => 'General',
         icon   => 'VB::classes.gif:2',
      },
      'Prima::Radio' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Radio',
         page   => 'General',
         icon   => 'VB::classes.gif:18',
      },
      'Prima::GroupBox' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::GroupBox',
         page   => 'General',
         icon   => 'VB::classes.gif:10',
      },
      'Prima::CheckBoxGroup' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::GroupCheckBox',
         page   => 'Additional',
         icon   => 'VB::classes.gif:11',
      },
      'Prima::RadioGroup' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::GroupRadioBox',
         page   => 'Additional',
         icon   => 'VB::classes.gif:12',
      },
      'Prima::ScrollBar' => {
         RTModule => 'Prima::ScrollBar',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ScrollBar',
         page   => 'General',
         icon   => 'VB::classes.gif:20',
      },
      'Prima::ComboBox' => {
         RTModule => 'Prima::ComboBox',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ComboBox',
         page   => 'General',
         icon   => 'VB::classes.gif:3',
      },
      'Prima::DriveComboBox' => {
         RTModule => 'Prima::ComboBox',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::DriveComboBox',
         page   => 'Additional',
         icon   => 'VB::classes.gif:5',
      },
      'Prima::ColorComboBox' => {
         RTModule => 'Prima::ColorDialog',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ColorComboBox',
         page   => 'Additional',
         icon   => 'VB::classes.gif:1',
      },
      'Prima::Edit' => {
         RTModule => 'Prima::Edit',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Edit',
         page   => 'General',
         icon   => 'VB::classes.gif:8',
      },
      'Prima::ImageViewer' => {
         RTModule => 'Prima::ImageViewer',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ImageViewer',
         page   => 'General',
         icon   => 'VB::classes.gif:14',
      },
      'Prima::ScrollWidget' => {
         RTModule => 'Prima::ScrollWidget',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ScrollWidget',
         page   => 'Abstract',
         icon   => 'VB::classes.gif:21',
      },
      'Prima::SpinButton' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::SpinButton',
         page   => 'Additional',
         icon   => 'VB::classes.gif:23',
      },
      'Prima::AltSpinButton' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::AltSpinButton',
         page   => 'Additional',
         icon   => 'VB::classes.gif:24',
      },
      'Prima::SpinEdit' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::SpinEdit',
         page   => 'Additional',
         icon   => 'VB::classes.gif:25',
      },
      'Prima::Gauge' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Gauge',
         page   => 'Additional',
         icon   => 'VB::classes.gif:9',
      },
      'Prima::Slider' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Slider',
         page   => 'Additional',
         icon   => 'VB::classes.gif:22',
      },
      'Prima::CircularSlider' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::CircularSlider',
         page   => 'Additional',
         icon   => 'VB::classes.gif:4',
      },
      'Prima::StringOutline' => {
         RTModule => 'Prima::Outlines',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::StringOutline',
         page   => 'General',
         icon   => 'VB::classes.gif:17',
      },
      'Prima::OutlineViewer' => {
         RTModule => 'Prima::Outlines',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::OutlineViewer',
         page   => 'Abstract',
         icon   => 'VB::classes.gif:17',
      },
      'Prima::DirectoryOutline' => {
         RTModule => 'Prima::Outlines',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::DirectoryOutline',
         page   => 'Additional',
         icon   => 'VB::classes.gif:7',
      },
      'Prima::Notebook' => {
         RTModule => 'Prima::Notebooks',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Notebook',
         page   => 'Abstract',
         icon   => 'VB::classes.gif:26',
      },
      'Prima::TabSet' => {
         RTModule => 'Prima::Notebooks',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::TabSet',
         page   => 'Additional',
         icon   => 'VB::classes.gif:27',
      },
   );
}


1;
