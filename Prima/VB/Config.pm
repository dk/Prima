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
         icon   => 'icons/widget.bmp',
      },
      'Prima::Button' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Button',
         page   => 'General',
         icon   => 'icons/button.bmp',
      },
      'Prima::SpeedButton' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Button',
         page   => 'Additional',
         icon   => 'icons/sbutton.bmp',
      },
      'Prima::Label' => {
         RTModule => 'Prima::Label',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Label',
         page   => 'General',
         icon   => 'icons/label.bmp',
      },
      'Prima::InputLine' => {
         RTModule => 'Prima::InputLine',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::InputLine',
         page   => 'General',
         icon   => 'icons/input.bmp',
      },
      'Prima::ListBox' => {
         RTModule => 'Prima::Lists',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ListBox',
         page   => 'General',
         icon   => 'icons/listbox.bmp',
      },
      'Prima::DirectoryListBox' => {
         RTModule => 'Prima::Lists',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::DirectoryListBox',
         page   => 'Additional',
         icon   => 'icons/dlistbox.bmp',
      },
      'Prima::ListViewer' => {
         RTModule => 'Prima::Lists',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ListBox',
         page   => 'Abstract',
         icon   => 'icons/listbox.bmp',
      },
      'Prima::CheckBox' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::CheckBox',
         page   => 'General',
         icon   => 'icons/checkbox.gif',
      },
      'Prima::Radio' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Radio',
         page   => 'General',
         icon   => 'icons/radio.gif',
      },
      'Prima::GroupBox' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::GroupBox',
         page   => 'General',
         icon   => 'icons/groupbox.bmp',
      },
      'Prima::CheckBoxGroup' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::GroupCheckBox',
         page   => 'Additional',
         icon   => 'icons/groupchk.bmp',
      },
      'Prima::RadioGroup' => {
         RTModule => 'Prima::Buttons',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::GroupRadioBox',
         page   => 'Additional',
         icon   => 'icons/grouprad.bmp',
      },
      'Prima::ScrollBar' => {
         RTModule => 'Prima::ScrollBar',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ScrollBar',
         page   => 'General',
         icon   => 'icons/scroll.bmp',
      },
      'Prima::ComboBox' => {
         RTModule => 'Prima::ComboBox',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ComboBox',
         page   => 'General',
         icon   => 'icons/combo.bmp',
      },
      'Prima::DriveComboBox' => {
         RTModule => 'Prima::ComboBox',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::DriveComboBox',
         page   => 'Additional',
         icon   => 'icons/dcombo.bmp',
      },
      'Prima::ColorComboBox' => {
         RTModule => 'Prima::ColorDialog',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ColorComboBox',
         page   => 'Additional',
         icon   => 'icons/ccombo.bmp',
      },
      'Prima::Edit' => {
         RTModule => 'Prima::Edit',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Edit',
         page   => 'General',
         icon   => 'icons/edit.bmp',
      },
      'Prima::ImageViewer' => {
         RTModule => 'Prima::ImageViewer',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ImageViewer',
         page   => 'General',
         icon   => 'icons/iv.bmp',
      },
      'Prima::ScrollWidget' => {
         RTModule => 'Prima::ScrollWidget',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::ScrollWidget',
         page   => 'Abstract',
         icon   => 'icons/scroller.bmp',
      },
      'Prima::SpinButton' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::SpinButton',
         page   => 'Additional',
         icon   => 'icons/spin1.bmp',
      },
      'Prima::AltSpinButton' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::AltSpinButton',
         page   => 'Additional',
         icon   => 'icons/spin2.bmp',
      },
      'Prima::SpinEdit' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::SpinEdit',
         page   => 'Additional',
         icon   => 'icons/spin3.bmp',
      },
      'Prima::Gauge' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Gauge',
         page   => 'Additional',
         icon   => 'icons/gauge.bmp',
      },
      'Prima::Slider' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::Slider',
         page   => 'Additional',
         icon   => 'icons/slider.bmp',
      },
      'Prima::CircularSlider' => {
         RTModule => 'Prima::Sliders',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::CircularSlider',
         page   => 'Additional',
         icon   => 'icons/cslider.bmp',
      },
      'Prima::StringOutline' => {
         RTModule => 'Prima::Outlines',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::StringOutline',
         page   => 'General',
         icon   => 'icons/outline.bmp',
      },
      'Prima::OutlineViewer' => {
         RTModule => 'Prima::Outlines',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::OutlineViewer',
         page   => 'Abstract',
         icon   => 'icons/outline.bmp',
      },
      'Prima::DirectoryOutline' => {
         RTModule => 'Prima::Outlines',
         module => 'Prima::VB::CoreClasses',
         class  => 'Prima::VB::DirectoryOutline',
         page   => 'Additional',
         icon   => 'icons/dlistout.bmp',
      },
   );
}


1;
