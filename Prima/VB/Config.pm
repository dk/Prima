#
#  Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
# $Id$
package Prima::VB::Config;

sub pages
{
   return qw(General Additional Sliders Abstract);
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
      page     => 'Sliders',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::SpinButton',
   },
   Prima::AltSpinButton => {
      icon     => 'VB::classes.gif:24',
      RTModule => 'Prima::Sliders',
      page     => 'Sliders',
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
      page     => 'Sliders',
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
      page     => 'Sliders',
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
      page     => 'Sliders',
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
      page     => 'Sliders',
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
   Prima::Header => {
      icon     => 'VB::classes.gif:30',
      RTModule => 'Prima::Header',
      page     => 'Sliders',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::Header',
   },
   Prima::DetailedList => {
      icon     => 'VB::classes.gif:31',
      RTModule => 'Prima::DetailedList',
      page     => 'General',
      module   => 'Prima::VB::CoreClasses',
      class    => 'Prima::VB::DetailedList',
   },
   );
}


1;
