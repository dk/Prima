package Const;

# notification types
package nt;
use constant PrivateFirst  => 0x000;
use constant CustomFirst   => 0x001;
use constant Single        => 0x000;
use constant Multiple      => 0x002;
use constant Event         => 0x004;
use constant SMASK         => Multiple | Event;

use constant Default       => PrivateFirst | Multiple;
use constant Property      => PrivateFirst | Single;
use constant Request       => PrivateFirst | Event;
use constant Notification  => CustomFirst  | Multiple;
use constant Action        => CustomFirst  | Single;
use constant Command       => CustomFirst  | Event;

# key constants
package kb;

use constant Shift     =>  0x100000  ;
use constant Ctrl      =>  0x400000  ;
use constant Alt       =>  0x800000  ;
use constant NoKey     =>  0xFF00 ;
use constant Backspace =>  0x0b00 ;
use constant Tab       =>  0x0c00 ;
use constant ShiftTab  =>  0x0d00 ;
use constant Pause     =>  0x0e00 ;
use constant Esc       =>  0x0f00 ;
use constant Space     =>  0x1000 ;
use constant PgUp      =>  0x1100 ;
use constant PgDn      =>  0x1200 ;
use constant End       =>  0x1300 ;
use constant Home      =>  0x1400 ;
use constant Left      =>  0x1500 ;
use constant Up        =>  0x1600 ;
use constant Right     =>  0x1700 ;
use constant Down      =>  0x1800 ;
use constant PrintScr  =>  0x1900 ;
use constant Insert    =>  0x1a00 ;
use constant Ins       =>  Insert ;
use constant Delete    =>  0x1b00 ;
use constant Del       =>  Delete ;
use constant Enter     =>  0x1e00 ;
use constant SysRq     =>  0x1f00 ;
use constant F1        =>  0x2000 ;
use constant F2        =>  0x2100 ;
use constant F3        =>  0x2200 ;
use constant F4        =>  0x2300 ;
use constant F5        =>  0x2400 ;
use constant F6        =>  0x2500 ;
use constant F7        =>  0x2600 ;
use constant F8        =>  0x2700 ;
use constant F9        =>  0x2800 ;
use constant F10       =>  0x2900 ;
use constant F11       =>  0x2a00 ;
use constant F12       =>  0x2b00 ;
use constant F13       =>  0x2c00 ;
use constant F14       =>  0x2d00 ;
use constant F15       =>  0x2e00 ;
use constant F16       =>  0x2f00 ;

# mouse buttons
package mb;
use constant Left   => 1;
use constant Right  => 2;
use constant Middle => 4;

package mb;
# message box
use constant OK           => 0x0001;
use constant Ok           => OK;
use constant Cancel       => 0x0004;
use constant Yes          => 0x0002;
use constant No           => 0x0008;
use constant Abort        => 0x0010;
use constant Retry        => 0x0020;
use constant Ignore       => 0x0040;
use constant Help         => 0x0080;
use constant OKCancel     => 0x0005;
use constant OkCancel     => OKCancel;
use constant YesNoCancel  => 0x000e;
use constant Error        => 0x0100;
use constant Warning      => 0x0200;
use constant Information  => 0x0400;
use constant Question     => 0x0800;
use constant NoSound      => 0x1000;

# text alignment
package ta;
use constant Left   => 1;
use constant Right  => 2;
use constant Center => 3;
use constant Top    => 4;
use constant Bottom => 8;
use constant Middle => 12;

# colors
package cl;
use constant Black => 0|(0<<8)|(0<<16);
use constant Blue => 128|(0<<8)|(0<<16);
use constant Green => 0|(128<<8)|(0<<16);
use constant Cyan => 128|(128<<8)|(0<<16);
use constant Red => 0|(0<<8)|(128<<16);
use constant Magenta => 128|(0<<8)|(128<<16);
use constant Brown => 0|(128<<8)|(128<<16);
use constant LightGray => 192|(192<<8)|(192<<16);
use constant DarkGray => 63|(63<<8)|(63<<16);
use constant LightBlue => 255|(0<<8)|(0<<16);
use constant LightGreen => 0|(255<<8)|(0<<16);
use constant LightCyan => 255|(255<<8)|(0<<16);
use constant LightRed => 0|(0<<8)|(255<<16);
use constant LightMagenta => 255|(0<<8)|(255<<16);
use constant Yellow => 0|(255<<8)|(255<<16);
use constant White => 255|(255<<8)|(255<<16);
use constant Gray => 128|(128<<8)|(128<<16);
use constant Pink => 128|(128<<8)|(255<<16);
use constant Invalid      =>   0x80000000;
use constant NormalText   =>   0x80000001;
use constant Fore         =>   0x80000001;
use constant Normal       =>   0x80000002;
use constant Back         =>   0x80000002;
use constant HiliteText   =>   0x80000003;
use constant Hilite       =>   0x80000004;
use constant DisabledText =>   0x80000005;
use constant Disabled     =>   0x80000006;
use constant Light3DColor =>   0x80000007;
use constant Dark3DColor  =>   0x80000008;

# color indeces
package ci;
use constant NormalText   =>   0;
use constant Fore         =>   0;
use constant Normal       =>   1;
use constant Back         =>   1;
use constant HiliteText   =>   2;
use constant Hilite       =>   3;
use constant DisabledText =>   4;
use constant Disabled     =>   5;
use constant Light3DColor =>   6;
use constant Dark3DColor  =>   7;
use constant MaxIndex     =>   7;

# widget classes
package wc;
use constant Button    => 0x0010000;
use constant CheckBox  => 0x0020000;
use constant Combo     => 0x0030000;
use constant Dialog    => 0x0040000;
use constant Edit      => 0x0050000;
use constant InputLine => 0x0060000;
use constant Label     => 0x0070000;
use constant ListBox   => 0x0080000;
use constant Menu      => 0x0090000;
use constant Popup     => 0x00A0000;
use constant Radio     => 0x00B0000;
use constant ScrollBar => 0x00C0000;
use constant Slider    => 0x00D0000;
use constant Custom    => 0x00E0000;
use constant Window    => 0x00F0000;

# commands
package cm;
use constant Valid        =>  0x00000;
use constant Quit         =>  0x00001;
use constant Help         =>  0x00002;
use constant OK           =>  0x00003;
use constant Ok           =>  OK;
use constant Cancel       =>  0x00004;
use constant KeyDown      =>  0x00051;
use constant KeyUp        =>  0x00052;
use constant MouseDown    =>  0x00053;
use constant MouseUp      =>  0x00054;
use constant MouseMove    =>  0x00055;
use constant MouseWheel   =>  0x00056;
use constant MouseClick   =>  0x00057;
use constant User         =>  0x00100;


# raster operations
package rop;
use constant CopyPut =>        0  ;
use constant XorPut =>         1  ;
use constant AndPut =>         2  ;
use constant OrPut =>          3  ;
use constant NotPut =>         4  ;
use constant NotBlack =>       5  ;
use constant NotDestXor =>     6  ;
use constant NotDestAnd =>     7  ;
use constant NotDestOr =>      8  ;
use constant NotSrcXor =>      9  ;
use constant NotSrcAnd =>      10 ;
use constant NotSrcOr =>       11 ;
use constant NotXor =>         12 ;
use constant NotAnd =>         13 ;
use constant NotOr =>          14 ;
use constant NotBlackXor =>    15 ;
use constant NotBlackAnd =>    16 ;
use constant NotBlackOr =>     17 ;
use constant NoOper =>         18 ;
use constant Blackness =>      19 ;
use constant Whiteness =>      20 ;
use constant Erase =>          21 ;
use constant Invert =>         22 ;
use constant Pattern =>        23 ;
use constant XorPattern =>     24 ;
use constant AndPattern =>     25 ;
use constant OrPattern =>      26 ;
use constant NotSrcOrPat =>    27 ;
use constant SrcLeave =>       28 ;
use constant DestLeave =>      29 ;


# grow flags
package gm;
use constant GrowLoX  =>      0x01  ;
use constant GrowLoY  =>      0x02  ;
use constant GrowHiX  =>      0x04  ;
use constant GrowHiY  =>      0x08  ;
use constant GrowAll  =>      0x0F  ;
use constant XCenter  =>      0x10  ;
use constant YCenter  =>      0x20  ;
use constant Center   =>       XCenter + YCenter;
use constant DontCare =>      0x40  ;
# handy shortcuts
use constant Client   =>          GrowHiX + GrowHiY;
use constant Right    =>          GrowLoX + GrowHiY;
use constant Left     =>          GrowHiY;
use constant Floor    =>          GrowHiX;
use constant Ceiling  =>          GrowHiX + GrowLoY;

# pen styles
package lp;
use constant Null =>        0x0000  ;   #
use constant Solid =>       0xFFFF  ;   #  ___________
use constant LongDash =>    0xFF00  ;   #  _____ _____
use constant ShortDash =>   0xCCCC  ;   #  _ _ _ _ _ _
use constant Dash =>        0xF0F0  ;   #  __ __ __ __
use constant Dot =>         0x5555  ;   #  . . . . . .
use constant DotDot =>      0x4444  ;   #  ...........
use constant DashDot =>     0xFAFA  ;   #  _._._._._._
use constant DashDotDot =>  0xEAEA  ;   #  _.._.._.._.

# fill styles
package fp;
use constant Empty       =>  0 ;
use constant Solid       =>  1 ;
use constant Line        =>  2 ;
use constant LtSlash     =>  3 ;
use constant Slash       =>  4 ;
use constant BkSlash     =>  5 ;
use constant LtBkSlash   =>  6 ;
use constant Hatch       =>  7 ;
use constant XHatch      =>  8 ;
use constant Interleave  =>  9 ;
use constant WideDot     => 10 ;
use constant CloseDot    => 11 ;
use constant SimpleDots  => 12 ;
use constant Borland     => 13 ;
use constant Parquet     => 14 ;
use constant Critters    => 15 ;

# line ends
package le;
use constant Flat =>     0  ;
use constant Square =>   1  ;
use constant Round =>    2  ;


# font subtypes
package fs;
use constant Normal     =>  0x0000  ;
use constant Bold       =>  0x0001  ;
use constant Thin       =>  0x0002  ;
use constant Italic     =>  0x0004  ;
use constant Underlined =>  0x0008  ;
use constant StruckOut  =>  0x0010  ;

# font pitches
package fp;
use constant Default       =>  0x000       ;
use constant Variable      =>  0x001       ;
use constant Fixed         =>  0x002       ;

# font types
package ft;
use constant DontCare      =>  0x000       ;
use constant Raster        =>  0x010       ;
use constant Vector        =>  0x020       ;


# font weights
package fw;
use constant UltraLight =>  1 ;
use constant ExtraLight =>  2 ;
use constant Light      =>  3 ;
use constant SemiLight  =>  4 ;
use constant Medium     =>  5 ;
use constant SemiBold   =>  6 ;
use constant Bold       =>  7 ;
use constant ExtraBold  =>  8 ;
use constant UltraBold  =>  9 ;

# border icons
package bi;
use constant SystemMenu => 1        ;
use constant Minimize   => 2        ;
use constant Maximize   => 4        ;
use constant TitleBar   => 8        ;
use constant All        => SystemMenu | Minimize | Maximize | TitleBar;

# border styles
package bs;
use constant None       => 0        ;
use constant Sizeable   => 1        ;
use constant Single     => 2        ;
use constant Dialog     => 3        ;

# window states
package ws;
use constant Normal     => 0        ;
use constant Minimized  => 1        ;
use constant Maximized  => 2        ;

# system values
package sv;
use constant YMenu         => 0     ;
use constant YTitleBar     => 1     ;
use constant MousePresent  => 2     ;
use constant MouseButtons  => 3     ;
use constant SubmenuDelay  => 4     ;
use constant FullDrag      => 5     ;

# image types
package im;
use constant None              =>  0          ;
use constant bpp1              =>  0x001      ;
use constant bpp4              =>  0x004      ;
use constant bpp8              =>  0x008      ;
use constant bpp16             =>  0x010      ;
use constant bpp24             =>  0x018      ;
use constant bpp32             =>  0x020      ;
use constant bpp64             =>  0x040      ;
use constant bpp128            =>  0x080      ;
use constant BPP               =>  0x0FF      ;
use constant GrayScale         =>  0x1000     ;
use constant RealNumber        =>  0x2000     ;
use constant ComplexNumber     =>  0x4000     ;
use constant TrigComplexNumber =>  0x8000     ;

use constant Mono         => bpp1             ;
use constant BW           => Mono  | GrayScale;
use constant Nibble       => bpp4             ;
use constant RGB          => bpp24            ;
use constant Triple       => RGB              ;
use constant Byte         => bpp8  | GrayScale;
use constant Short        => bpp16 | GrayScale;
use constant Long         => bpp32 | GrayScale;
use constant Float        => length(pack('f',0))*8    | GrayScale | RealNumber        ;
use constant Double       => length(pack('d',0))*8    | GrayScale | RealNumber        ;
use constant Complex      => length(pack('f',0))*8*2  | GrayScale | ComplexNumber     ;
use constant DComplex     => length(pack('d',0))*8*2  | GrayScale | ComplexNumber     ;
use constant TrigComplex  => length(pack('f',0))*8*2  | GrayScale | TrigComplexNumber ;
use constant TrigDComplex => length(pack('d',0))*8*2  | GrayScale | TrigComplexNumber ;

# Image conversion types
package ict;
use constant None              =>  0          ;
use constant Halftone          =>  1          ;

# Image statistics types
package is;
use constant RangeLo     => 0;
use constant RangeHi     => 1;
use constant Mean        => 2;
use constant Variance    => 3;
use constant StdDev      => 4;

# OS type
package apc;
use constant OS2               => 1;
use constant Win32             => 2;
use constant Unix              => 3;

# GUI types
package gui;
use constant Default           => 0;
use constant PM                => 1;
use constant Windows           => 2;
use constant XLib              => 3;
use constant OpenLook          => 4;
use constant Motif             => 5;

# Drives types
package dt;
use constant Unknown           => 0;
use constant None              => 1;
use constant Floppy            => 2;
use constant HDD               => 3;
use constant Network           => 4;
use constant CDROM             => 5;
use constant Memory            => 6;

# Pointer ( Cursor Resources) id's
package cr;
use constant Default           => -1;
use constant Arrow             => 0;
use constant Text              => 1;
use constant Wait              => 2;
use constant Size              => 3;
use constant Move              => 4;
use constant SizeWE            => 5;
use constant SizeNS            => 6;
use constant SizeNWSE          => 7;
use constant SizeNESW          => 8;
use constant Invalid           => 9;
use constant User              => 10;

# Sytem bitmaps index
package sbmp;
use constant CheckBoxChecked            =>  0;
use constant CheckBoxCheckedPressed     =>  1;
use constant CheckBoxUnchecked          =>  2;
use constant CheckBoxUncheckedPressed   =>  3;
use constant RadioChecked               =>  4;
use constant RadioCheckedPressed        =>  5;
use constant RadioUnchecked             =>  6;
use constant RadioUncheckedPressed      =>  7;
use constant Warning                    =>  8;
use constant Information                =>  9;
use constant Question                   => 10;
use constant OutlineCollaps             => 11;
use constant OutlineExpand              => 12;
use constant Error                      => 13;
use constant SysMenu                    => 14;
use constant SysMenuPressed             => 15;
use constant Max                        => 16;
use constant MaxPressed                 => 17;
use constant Min                        => 18;
use constant MinPressed                 => 19;
use constant Restore                    => 20;
use constant RestorePressed             => 21;
use constant Close                      => 22;
use constant ClosePressed               => 23;
use constant Hide                       => 24;
use constant HidePressed                => 25;
use constant DriveUnknown               => 26;
use constant DriveFloppy                => 27;
use constant DriveHDD                   => 28;
use constant DriveNetwork               => 29;
use constant DriveCDROM                 => 30;
use constant DriveMemory                => 31;
use constant GlyphOK                    => 32;
use constant GlyphCancel                => 33;
use constant SFolderOpened              => 34;
use constant SFolderClosed              => 35;
use constant Last                       => 35;

#clipboard predefined formats
package cf;
use constant Text                       => "Text";
use constant Image                      => "Image";

#help manager pages constants
package hmp;
use constant None                     =>  0;
use constant Owner                    => -1;
use constant Main                     => -2;
use constant Contents                 => -3;
use constant Extra                    => -4;

#text wrapping constants
package tw;
use constant CalcMnemonic             => 0x001;
use constant CalcTabs                 => 0x002;
use constant BreakSingle              => 0x004;
use constant NewLineBreak             => 0x008;
use constant SpaceBreak               => 0x010;
use constant ReturnLines              => 0x000;
use constant ReturnChunks             => 0x020;
use constant WordBreak                => 0x040;
use constant ExpandTabs               => 0x080;
use constant CollapseTilde            => 0x100;
use constant Default                  => NewLineBreak|CalcTabs|ExpandTabs|ReturnLines|WordBreak;

# find/replace dialog scope type
package fds;
use constant Cursor                   => 0;
use constant Top                      => 1;
use constant Bottom                   => 2;

# find/replace dialog options
package fdo;
use constant MatchCase                => 0x01;
use constant WordsOnly                => 0x02;
use constant RegularExpression        => 0x04;
use constant BackwardSearch           => 0x08;
use constant ReplacePrompt            => 0x10;

1;
