DESCRIPTION
===========

PRIMA is a general purpose extensible graphical user interface toolkit with a
rich set of standard widgets and an emphasis on 2D image processing tasks. A
Perl program using PRIMA looks and behaves identically on X11 and Win32.

PREREQUISITES
=============

Debian/Ubuntu
-------------

For easy setup, run this:

  apt-get install libgtk2.0-dev libgif-dev libjpeg-dev libtiff-dev libxpm-dev
      libwebp-dev libfribidi-dev libharfbuzz-dev

OpenSUSE
--------

  zypper install gtk2-devel giflib-devel libjpeg-devel libtiff-devel
      libXpm-devel libXrandr-devel libXcomposite-devel libXcursor-devel
      libfribidi-devel libwebp-devel libharfbuzz-devel

Solaris
-------

Download and install Oracle Developer Studio as perl there is compiled with cc, not gcc.

Cygwin
------

- install apt-cyg:

   wget https://raw.githubusercontent.com/transcode-open/apt-cyg/master/apt-cyg -O /usr/bin/apt-cyg

   chmod +x /usr/bin/apt-cyg

- install prerequisites:

   apt-cyg install libgtk2.0-devel libfribidi-devel libgif-devel libjpeg-devel libtiff-devel libXpm-devel
        libwebp-devel libharfbuzz-devel


Graphic libraries
-----------------

Prima can use several graphic libraries to handle image files.  Compiling Prima
with at least one library, preferably for GIF files is strongly recommended,
because internal library images are stored in GIFs. Support for the following
libraries can be compiled in on all platforms:

   - libXpm
   - libpng
   - libjpeg
   - libgif
   - libtiff
   - libwebp,libwebpdemux,libwebpmux

Win32
-----

Modern Strawberry perl come with all of these libraries already, so nothing
specific needs to be done. However for the older distributions, or ActiveState,
or custom builds, CPAN contains binary distributions that can be installed just
for this purpose:

 - http://search.cpan.org/~karasik/Prima-codecs-win32/
 - http://search.cpan.org/~karasik/Prima-codecs-win64/

it should work for all MSVC and GCC compilers and for native,
cygwin, and mingw/strawberry perl runtimes.

MacOSX
------

You'll need homebrew, XQuartz, and a set of extra libraries.

- install homebrew:

  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

- install XQuartz:

  brew install --cask xquartz

- install support libraries:

  brew install libiconv libxcomposite libxrandr libxcursor libxft fribidi fontconfig
     freetype giflib gtk+ harfbuzz jpeg libpng libtiff webp libxpm

Note: if Prima crashes in libxft, do this: remove libxft and install custom built xorg libraries, either very minimal

  brew install dk/x11/xorg-macros dk/x11/libxft

or linux-homebrew's (not tested)

  brew tap linuxbrew/xorg

  brew install linuxbrew/xorg/libxft

Bidirectional input and complex scripts
---------------------------------------

To support bi-directional unicode text input and output you'll need the fribidi
library.  Additionally for unix builds you'll need harfbuzz library for output
of complex scripts and font ligature support.

Prima can compile and work fine without these, but the support of these
features will be rather primitive.

SOURCE DISTRIBUTION INSTALLATION
================================

Create a makefile by running Makefile.PL using perl and then run make ( or
gmake, or nmake for Win32):

    perl Makefile.PL
    make
    make test
    make install

If 'perl Makefile.PL' fails, the compilation history along with errors can be
found in makefile.log.

If make fails with message

    ** No image codecs found

that means you don't have image libraries that Prima supports in your path.
See PREREQUISITES section.

If some of the required libraries or include files can not be found,
INC=-I/some/include and LIBS=-L/some/lib semantics should be used to tell
Makefile.PL about these. Check ExtUtils::MakeMaker for more.

GTK3/GTK2
---------

It is recommended to build Prima with GTK3/GTK2 on X11 installations,
because in that case Prima will use standard GTK fonts, colors, and file dialogs.
By default Prima tries to build with it, but if you don't want it, run

    perl Makefile.PL WITH_GTK2=0 WITH_GTK3=0

BINARY DISTRIBUTION INSTALLATION
================================

Available only for MSWin32. Please use installation from source for
the other platforms.

To install the toolkit from the binary distribution run

    perl ms_install.pl

You have to patch Prima::Config.pm manually if you need to compile
prima-dependent modules.

USAGE EXAMPLES
==============

Try running the toolkit examples, by default installed in
INSTALLSITEARCH/Prima/examples directory ( find it by running perl
-V:installsitearch ). All examples and programs included into the distribution
can be run either by their name or with perl as argument - for example,
..../generic or perl ..../generic .  ( perl ..../generic.bat for win32 )

Typical code starts with

    use Prima qw(Application);

and ends with

    run Prima;

, the event loop. Start from the following code:

    use Prima qw(Application Buttons);

    new Prima::MainWindow(
       text     => 'Hello world!',
       size     => [ 200, 200],
    )-> insert( Button =>
       centered => 1,
       text     => 'Hello world!',
       onClick  => sub { $::application-> close },
    );
 
    run Prima;

Or, alternatively, start the VB program, the toolkit visual builder.

MORE INFORMATION
================

The toolkit contains set of POD files describing its features, and the
programming interfaces.  Run 'podview Prima' or 'perldoc Prima' command to
start with the main manual page.

Visit http://www.prima.eu.org/ for the recent versions of the toolkit. You can
use github.com/dk/Prima to keep in touch. The mailing list on the toolkit is
available, you can ask questions there. See the Prima homepage for details.

COPYRIGHT
=========

(c) 1997-2003 The Protein Laboratory, University of Copenhagen

(c) 1997-2021 Dmitry Karasik

AUTHOR
======

 - Dmitry Karasik <dmitry@karasik.eu.org>

CREDITS
=======

 - Anton Berezin
 - Vadim Belman
 - David Scott
 - Teo Sankaro
 - Kai Fiebach
 - Johannes Blankenstein
 - Mike Castle
 - H.Merijn Brand
 - Richard Morgan
 - Kevin Ryde
 - Chris Marshall
 - Slaven Rezic
 - Waldemar Biernacki
 - Andreas Hernitscheck
 - David Mertens
 - Gabor Szabo
 - Fabio D'Alfonso
 - Rob "Sisyphus"
 - Chris Marshall
 - Reini Urban
 - Nadim Khemir
 - Vikas N Kumar
 - Upasana Shukla
 - Sergey Romanov
 - Mathieu Arnold
 - Petr Pisar
 - Judy Hawkins
 - Myra Nelson
 - Sean Healy
 - Ali Yassen
 - Maximilian Lika
 - kmx
 - Mario Roy

