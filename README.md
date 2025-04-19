Description
===========

PRIMA is a general-purpose extensible graphical user interface toolkit with a
rich set of standard widgets and an emphasis on 2D image processing tasks. A
Perl program using PRIMA looks and behaves identically on X11 and Win32.

Example
-------

![Screenshot](/pod/Prima/hello-world.gif)

	use Prima qw(Application Buttons);

	Prima::MainWindow->new(
		text     => 'Hello world!',
		size     => [ 200, 200],
	)-> insert( Button =>
		centered => 1,
		text     => 'Hello world!',
		onClick  => sub { $::application-> close },
	);

	Prima->run;

More screenshots at http://prima.eu.org/big-picture/

Installation
============

Debian/Ubuntu
-------------

  apt-get install libgtk-3-dev libgif-dev libjpeg-dev libtiff-dev libxpm-dev \
      libwebp-dev libfribidi-dev libharfbuzz-dev libthai-dev libheif-dev libfreetype-dev \
      libjxl-dev

FreeBSD
-------

  pkg install gtk3 fribidi harfbuzz libxpm libthai pkgconf tiff webp \
	giflib libheif freetype2 libheif Xrandr libXcomposite libXcursor \
	libXft fontconfig

OpenSUSE
--------

  zypper install gtk3-devel giflib-devel libjpeg-devel libtiff-devel
      libXpm-devel libXrandr-devel libXcomposite-devel libXcursor-devel
      libfribidi-devel libwebp-devel libharfbuzz-devel libthai-devel
      libheif-devel libfreetype-devel libjxl-devel


Solaris
-------

Download and install Oracle Developer Studio as the vendor-provided perl is
compiled with cc, not gcc

Cygwin
------

- install apt-cyg:

   wget https://raw.githubusercontent.com/transcode-open/apt-cyg/master/apt-cyg -O /usr/bin/apt-cyg

   chmod +x /usr/bin/apt-cyg

- install prerequisites:

   apt-cyg install libgtk3.0-devel libfribidi-devel libgif-devel libjpeg-devel libtiff-devel libXpm-devel
        libwebp-devel libharfbuzz-devel libthai-devel libheif-devel libfreetype-dev

Win32-Strawberry
----------------

v5.32 had the necessary libraries included, but v5.38 hasn't.
Install these:

   - http://prima.eu.org/download/libfribidi-1.0.10-win64.zip
   - http://prima.eu.org/download/libthai-0.1.29-win64.zip
   - http://prima.eu.org/download/libheif-1.17.6-win64.zip
   - http://prima.eu.org/download/libwebp-1.0.2-win64.zip

And optionally

   - http://prima.eu.org/download/libjxl-0.11.1-win64.zip

Win32-ActiveState
-----------------

ActiveState currently doesn't support local compilations.
The official answer is here: https://community.activestate.com/t/how-to-install-gcc-and-or-mingw/10993/2 .
Consider migrating to Strawberry Perl.

Win32-other
-----------

For the older distributions or custom Perl builds, CPAN contains binary
distributions that can be installed just for this purpose:

 - http://search.cpan.org/~karasik/Prima-codecs-win32/
 - http://search.cpan.org/~karasik/Prima-codecs-win64/

it should work for all MSVC and GCC compilers and
native, cygwin, and mingw/strawberry perl runtimes.

MacOSX
------

You'll need homebrew, XQuartz, and a set of extra libraries.

- install homebrew:

  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

- install XQuartz:

  brew install --cask xquartz

- install support libraries:

  brew install libiconv libxcomposite libxrandr libxcursor libxft fribidi fontconfig
     freetype giflib gtk+3 harfbuzz jpeg libpng libtiff webp libxpm libheif

Note: if Prima crashes in libxft, do this: remove libxft and install custom-built xorg libraries, either very minimal

  brew install dk/x11/xorg-macros dk/x11/libxft

or linux-homebrew's (not tested)

  brew tap linuxbrew/xorg

  brew install linuxbrew/xorg/libxft

- install libthai:

  brew install dk/libthai/libthai

Optional dependencies
=====================

Graphic libraries
-----------------

Prima can use several graphic libraries to handle image files.  Compiling Prima
with at least one library, preferably for GIF files is strongly recommended,
because internal library images are stored in GIFs. Support for the following
libraries can be compiled on all platforms:

   - libXpm
   - libpng
   - libjpeg
   - libgif
   - libtiff
   - libwebp,libwebpdemux,libwebpmux
   - libheif

(libheif is not widespread yet. See Prima/Image/heif.pm for details)

Bidirectional input and complex scripts
---------------------------------------

- To support bi-directional unicode text input and output you'll need the
fribidi library.  Additionally for unix builds you'll need the harfbuzz library
for output of complex scripts and font ligature support. Prima can compile and
work fine without these libraries, but the support of the features will be
rather primitive.

- The Thai language doesn't use spaces between the words in a sentence. To wrap
Thai texts properly Prima can be compiled with the libthai library. No special
treatment of Thai text is needed programmatically, the text wrapper does everything
under the hood.

GTK3/GTK2
---------

It is recommended to build Prima with GTK3/GTK2 on X11 installations
because in that case, Prima will use standard GTK fonts, colors, and file dialogs.
By default, Prima tries to build with it, but if you don't want it, run

    perl Makefile.PL WITH_GTK2=0 WITH_GTK3=0

Source distribution installation
================================

Create a makefile by running Makefile.PL using perl and then run make ( or
gmake, or nmake for Win32):

    perl Makefile.PL
    make
    make test
    make install

(the brave souls may run `make xtest` too)

If 'perl Makefile.PL' fails, the compilation history along with errors can be
found in makefile.log.

If make fails with message

    ** No image codecs found

that means you don't have image libraries that Prima supports in your path.
See the Installation section.

If some of the required libraries or include files can not be found,
INC=-I/some/include and LIBS=-L/some/lib semantics should be used to tell
Makefile.PL about these. Check ExtUtils::MakeMaker for more.

Binary distribution installation
================================

Available only for MSWin32. Please use the installation from the source for
the other platforms.

To install the toolkit from the binary distribution run

    perl ms_install.pl

You have to patch Prima::Config.pm manually if you need to compile
prima-dependent modules.

Usage examples
==============

Try running the toolkit examples, by default installed in
INSTALLSITEARCH/Prima/examples directory ( find it by running perl
-V:installsitearch ). All examples and programs included in the distribution
can be run either by their name or with perl as argument - for example,
..../helloworld or perl ..../helloworld .  ( perl ..../helloworld.bat for win32 )

Typical code starts with

    use Prima qw(Application);

and ends with

    Prima->run;

, the event loop. Start with the following code:

    use Prima qw(Application Buttons);

    new Prima::MainWindow(
       text     => 'Hello world!',
       size     => [ 200, 200],
    )-> insert( Button =>
       centered => 1,
       text     => 'Hello world!',
       onClick  => sub { $::application-> close },
    );
 
    Prima->run;

Alternatively, start the VB program, the toolkit visual builder.

More information
================

The toolkit contains a set of POD files describing its features and the
programming interfaces.  Run the 'podview Prima' or 'perldoc Prima' commands to
start with the main manual page.

Visit http://www.prima.eu.org/ for the recent versions of the toolkit. You can
use github.com/dk/Prima to keep in touch. See the Prima homepage for details.

Online documentation at MetaCPAN: https://metacpan.org/dist/Prima/view/Prima.pm

Printable documentation: http://prima.eu.org/download/Prima.pdf


Copyright
=========

(c) 1997-2003 The Protein Laboratory, University of Copenhagen

(c) 1997-2025 Dmitry Karasik

Author
======

 - Dmitry Karasik <dmitry@karasik.eu.org>

Credits
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
 - Timothy Witham
 - Mohammad S Anwar
 - Jean-Damien Durand
 - Zsban Ambrus
 - Max Maischein
 - Reinier Maliepaard
 - Ed J
 - Harald Joerg

Thanks to
=========

 - BSD.dk for hosting
 - SourceForge for hosting
 - Github for hosting and CI
 - Grammarly for free proofreading service
 - Perltesters for the CI

