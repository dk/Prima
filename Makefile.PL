#! /usr/bin/perl -w
#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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

#
# Create Makefile for a Prima compilation
#
# This is not a ``normal'' Makefile.PL file.
# It does not use MakeMaker.
#
my $usePrigraph = defined $ENV{PRIMA_PRIGRAPH};

require 5.00502;
use strict;
use Config;
use File::Find;
use File::Basename;
use File::Path;
use File::Copy;
use Cwd;
use DynaLoader;
use Prima::Gencls;
use vars qw( %make_trans @ovvars $dir_sep $path_sep );
use vars @ovvars = qw(
    $CC $CFLAGS $CDLFLAGS $COUTOFLAG $COUTEXEFLAG $CINCPATHFLAG
    $CDEFFLAG $COMPONLYFLAG $CLIBPATHFLAG $CLINKPREFIX
    $LD $LDFLAGS $LDOUTFLAG $LDLIBPATHFLAG $LDLIBFLAG
    $OBJ_EXT $LIB_EXT $EXE_EXT $DL_EXT $SCRIPT_EXT
    @INCPATH
    @LIBPATH
    @LIBS
    $PLATFORM
    $COMPILER
    $PREFIX
    $INSTALL_BIN
    $INSTALL_LIB
    $INSTALL_DL
    $INSTALL_EXAMPLES
    $TMPDIR
    $NULLDEV
    $MAKE
    $RM
    $SHQUOTE
    $MAKETYPE
    $DEFFILE
);

use vars qw(
    $CWD
    $Win32
    $NeedX11
    $OS2
    $OS2DLLF
    %cc_specs
    %DEFINES
    %USER_VARS
    %USER_VARS_ADDONS
    %USER_DEFINES
    %USER_DEFINES_ADDONS
    %overrideable
    $ARGV_STR
    @alltml
    @alltmldeps
    @allclean
    @allojects
    %alldeps
    @allinstall
    @allbins
    @alldirs
    @headers
    @footers
    @Makefile_deps
    $PrimaTarget
    @Prima_exports
);

sub print_config
{
    foreach my $var ( sort keys %Config) {
        print "$var='", $Config{ $var} || '', "'\n";
    }
}

sub qd
{
    my ( $path_str) = @_;
    $path_str =~ s[/][$dir_sep]g;
    return $path_str;
}

sub quoted_split
{
   my @neww = ();
   my $text = shift;
   while ($text =~ m<
       ("[^\"\\]*(?:\\.[^\"\\]*)*")\s*
     | (\S+)\s*
     | \s+
   >gx) {
          push @neww, $+ if defined $+;
   }
   return @neww;
}

sub fatal
{
    open ERRLOG, ">". qd "$CWD/Makefile.err";
    print ERRLOG @_;
    close ERRLOG;
    die @_;
}

sub quotemake
{
    return join '', map { $make_trans{ $MAKETYPE}->[ ord]} split //, $_[ 0];
}

sub tempfile
{
    my $mask = shift;
    my $name;
    my $n = 0;
    do {
        $name = sprintf $mask, $n++;
    } while ( -e $name);
    return $name;
}

sub cc_command_line
{
    my ( $srcf, $objf, $exef, $compile_only, $dl) = @_;
    my @cc = ( $CC, split ' ', $CFLAGS);
    push @cc, split ' ', $CDLFLAGS if $dl;
    push @cc, $COMPONLYFLAG if $compile_only;
    push @cc, map( { "$CINCPATHFLAG$_"} @INCPATH);
    for ( keys %DEFINES) {
       next unless $DEFINES{$_};
       push @cc, "$CDEFFLAG$_=$DEFINES{$_}";
    }
    push @cc, "${CDEFFLAG}PRIGRAPH" if $usePrigraph;
    push @cc, ( $compile_only ? "$COUTOFLAG$objf" : "$COUTEXEFLAG$exef");
    push @cc, "$COUTOFLAG$objf" if $Win32 && ! $compile_only;
    push @cc, map { "$CLIBPATHFLAG$_"} @LIBPATH unless $compile_only || ( $COMPILER eq 'msvc');
    push @cc, $srcf;
    return @cc if $compile_only;
    push @cc, $CLINKPREFIX;
    push @cc, map { "$CLIBPATHFLAG$_"} @LIBPATH if $COMPILER eq 'msvc';
    push @cc, map { "$LDLIBFLAG$_"} @LIBS;
    return @cc;
}

sub ld_command_line
{
    my ( $dstf) = shift;
    my @ld = ( $LD, split( ' ', $LDFLAGS));
    if ( $COMPILER eq 'bcc32') {
        push @ld, '-L"' . join( ';', @LIBPATH) . '" c0d32.obj ';
        push @ld, @_;
        push @ld, ",", $dstf, ", ,", @LIBS, ",", "win32\\Prima.def,";
    }
    else {
        push @ld, map { "$LDLIBPATHFLAG$_"} @LIBPATH;
        push @ld, "$LDOUTFLAG$dstf", @_;
        push @ld, map { "$LDLIBFLAG$_"} @LIBS;
        push @ld, qd( "os2/Prima.def") if $COMPILER eq 'emx';
    }
    return @ld;
}

sub null_output
{
    open OLDSTDOUT, ">&STDOUT" or die "STDOUT dup failed: $!";
    open OLDSTDERR, ">&STDERR" or die "STDERR dup failed: $!";
#   $NULLDEV = ( $Win32 || $OS2) ? "CON" : "/dev/tty";
    if ( $^O !~ /linux/) {
        close STDOUT;
        close STDERR;
    }
    open STDOUT, ">$NULLDEV" or fatal "STDOUT redirect failed: $!";
    open STDERR, ">&STDOUT" or fatal "STDERR redirect failed: $!";
}

sub restore_output
{
    if ( $^O !~ /linux/) {
        close STDOUT;
        close STDERR;
    }
    open STDOUT, ">&OLDSTDOUT" or fatal "STDOUT restoration failed: $!";
    open STDERR, ">&OLDSTDERR" or fatal "STDERR restoration failed: $!";
    close OLDSTDOUT;
    close OLDSTDERR;
}

sub compile
{
    my ( $text, $compile_only) = @_;
    my $tmpsrc = qd( tempfile( "$TMPDIR/pmts%04d.c"));
    my $tmpo = qd( tempfile( "$TMPDIR/pmts%04d$OBJ_EXT"));
    my $tmpexe = qd( tempfile( "$TMPDIR/pmts%04d$EXE_EXT"));
    my @tmpextras = (
       qd( tempfile( "$TMPDIR/pmts%04d.ilk")),
       qd( tempfile( "$TMPDIR/pmts%04d.pdb")),
    );

    open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed";
    print TMPSRC $text;
    close TMPSRC;

    null_output;
    my @cc = grep !/^-W/i, cc_command_line( $tmpsrc, $tmpo, $tmpexe, $compile_only || 0);
    print STDERR "@cc\n";
    my $rc = system "@cc";
    restore_output;
    unlink $tmpsrc;
    unlink $tmpo if -w $tmpo;
    unlink $tmpexe if -w $tmpexe;
    unlink $_ for @tmpextras;
    return( $rc == 0);
}

sub have_header
{
    my $header = shift;
    (my $defname = "HAVE_" . uc $header) =~ s/\W/_/g;
    return $DEFINES{$defname} if defined $DEFINES{$defname};
    my @pre_headers = map { "#include <$_>\n" } @_;
    print "Checking for presence of $header... ";
    my $present = compile( <<ENDP, 1);
@pre_headers
#include <$header>
ENDP
    $DEFINES{ $defname} = 0;
    $DEFINES{ $defname} = 1 if $present;
    print( $present ? "yes" : "no", "\n");
    return $present;
}

sub find_header
{
    my $header = shift;
    my ( $incpath, $present);
    foreach $incpath ( @_) {
        local @INCPATH = @INCPATH;
        push @INCPATH, $incpath if $incpath;
        $present = compile( <<ENDP, 1);
#include <$header>
ENDP
        return $incpath if $present;
    }
    return undef;
}

sub find_lib
{
    my $lib = shift;
    my ( $libpath, $present);
    local @LIBS = @LIBS;
    push @LIBS, $lib;
    foreach $libpath ( @_) {
        local @LIBPATH = (@LIBPATH, $libpath) if $libpath;
        $present = compile( <<ENDP);
int
main()
{
   return 0;
}
ENDP
        return $libpath if $present;
    }
    return undef;
}

sub have_func
{
    my ( $funcname, @headers) = @_;
    my $defname = "HAVE_" . uc $funcname;
    my @srchead = map { "#include <$_>\n"} @headers;
    $defname =~ s/\W/_/g;
    print "Checking for function $funcname... ";
    my $rc = compile( <<ENDP);
@srchead

int
main()
{
    void * ixi = $funcname;
    return 0;
}
ENDP
    if ( $rc) {
        $DEFINES{ $defname} = 1;
        print "yes\n";
    } else {
        $DEFINES{ $defname} = 0;
        print "no\n";
    }
    return $rc;
}

sub have_define
{
    my ( $defname) = @_;
    my $cwd = cwd;
    chdir $TMPDIR;
    my $tmpsrc = qd( tempfile( "pmts%04d.c"));
    my $tmpo = $tmpsrc;
    if ( $Win32) {
        $tmpo =~ s/\.c$/\.obj/;
    }
    else {
        $tmpo =~ s/\.c$/\.o/;
    }
    open TMPSRC, ">$tmpsrc";
    print TMPSRC <<ENDP;
int
main()
{
#if defined( $defname)
return 0;
#else
#error No
#endif
}
ENDP
    close TMPSRC;
    null_output;
    my $ccrc = system( "$CC -c $tmpsrc");
    restore_output;
    unlink $tmpsrc, $tmpo;
    chdir $cwd;
    return $ccrc == 0;
}

sub have_type
{
    my ( $type, @headers) = @_;
    (my $defname = "HAVE_" . uc $type) =~ s/\W/_/g;
    return 1 if $DEFINES{$defname};
    print "Checking for presence of type ${ type}... ";
    my @srchead = map { "#include <$_>\n"} @headers;
    my $rc = compile( <<ENDP);
@srchead

int
main()
{
    ${ type} foo;
    return 0;
}
ENDP
    if ( $rc) {
        $DEFINES{ $defname} = 1;
        print "yes\n";
    }
    else {
        print "no\n";
    }
    return $rc;
}

sub have_types_in
{
    my ($hdr, @types) = @_;
    return unless have_header( $hdr);
    for my $type (@types) {
       have_type( $type, $hdr);
    }
}

sub have_funcs_in
{
    my ($hdr, @funcs) = @_;
    return unless have_header( $hdr);
    for my $func (@funcs) {
       have_func( $func, $hdr);
    }
}

sub setvar
{
    my ( $varname) = shift;
    die "Tried to set non-overrideable variable $varname" unless $overrideable{ $varname};
    if ( $overrideable{ $varname} eq '$') {
        if ( defined $USER_VARS{ $varname}) {
            eval "\$$varname = \$USER_VARS{ \$varname}";
        }
        else {
            eval "\$$varname = join( '', \@_)";
        }
        die $@ if $@;
        if ( defined $USER_VARS_ADDONS{ $varname}) {
            eval "\$$varname .= join( '', \@{ \$USER_VARS_ADDONS{ \$varname}})";
        }
        die $@ if $@;
    }
    elsif ( $overrideable{ $varname} eq '@') {
        if ( defined $USER_VARS{ $varname}) {
            eval "\@$varname = \$USER_VARS{ \$varname}";
        }
        else {
            eval "\@$varname = \@_";
        }
        die $@ if $@;
        if ( defined $USER_VARS_ADDONS{ $varname}) {
            eval "push \@$varname, \@{ \$USER_VARS_ADDONS{ \$varname}}";
        }
        die $@ if $@;
    }
    else {
        die "Unsupported type of variable";
    }
}

sub setup_variables
{
    #   die "Oops! OS/2 isn't supported!\n" if $^O =~ /os2/i;
    my $platform;
    $NeedX11 = 0;
    if ( $Win32) {              # XXX Correct for NT.
        $platform = 'win32';
    }
    elsif ( $OS2) {
        $platform = 'os2';
    }
    else {
        $platform = 'unix';
        $NeedX11 = 1;
    }
    setvar( 'PLATFORM', $platform);

    %cc_specs = (
                 gcc => {
                         warnflags => "-Wall -Werror",
                         cflags => "",
                         coutoflag => '-o',
                         coutexeflag => '-o',
                         cdebugflags => '-g -O',
                         clinkprefix => '',
                         clibpathflag => '-L',
                         ldflags => '',
                         ldoutflag => '-o',
                         ldlibpathflag => '-L',
                         ldlibflag => '-l',
                         lddebugflags => '-g',
                         name => "GNU",
                        },
                 emx => {
                         warnflags => "-Wall -Werror",
                         cflags => "",
                         coutoflag => '-o',
                         coutexeflag => '-o',
                         cdebugflags => '-g -O',
                         clinkprefix => '',
                         clibpathflag => '-L',
                         ldflags => '',
                         ldoutflag => '-o',
                         ldlibpathflag => '-L',
                         ldlibflag => '-l',
                         lddebugflags => '-g',
                         name => "EMX",
                        },
                 msvc => {
                          warnflags => "-W3 -WX",
                          cflags => "",
                          coutoflag => '-Fo',
                          coutexeflag => '-Fe',
                          clinkprefix => '/link',
                          clibpathflag => '/LIBPATH:',
                          cdebugflags => '-Zi',
                          ldflags => '',
                          ldoutflag => '/OUT:',
                          ldlibpathflag => '/DEF:win32/Prima.def /LIBPATH:',
                          ldlibflag => '',
                          lddebugflags => '/DEBUG',
                          name => "Microsoft Visual C++",
                         },
                 bcc32 => {
                           warnflags => "-w0", # Borland is the only compiler which doesn't allow of geting rid of warnings.
                           cflags => "-tWM",
                           coutoflag => '-o',
                           coutexeflag => '-e',
                           clinkprefix => '',
                           clibpathflag => '-L',
                           cdebugflags => '-v -y',
                           ldflags => '',
                           ldoutflag => '',
                           ldlibpathflag => '-L',
                           ldlibflag => '',
                           lddebugflags => '-v',
                           name => "Borland C++",
                          },
                );

    setvar( 'TMPDIR', $ENV{ TMPDIR} || $ENV{ TEMPDIR} || ( $Win32 ? ( $ENV{ TEMP} || "$ENV{ SystemDrive}\\TEMP") : "/tmp"));
    setvar( 'NULLDEV', $Win32 ? "NUL" : "/dev/null");

    setvar( 'CC', $Config{ cc});

    print "Determining compiler type... ";
    if ( defined $USER_VARS{ COMPILER}) {
        $COMPILER = $USER_VARS{ COMPILER};
        die "Compiler type $COMPILER is unknown" unless defined $cc_specs{ $COMPILER};
    }
    elsif ( have_define( "__EMX__")) {
        $COMPILER = 'emx';
    }
    elsif ( have_define( "__GNUC__")) {
        $COMPILER = 'gcc';
    }
    elsif ( have_define( "__BORLANDC__")) {
        $COMPILER = 'bcc32';
    }
    elsif ( have_define( "_MSC_VER")) {
        $COMPILER = 'msvc';
    }
    else {
        die "Unknown or unsupported compiler\n";
    }
    print "$cc_specs{ $COMPILER}->{ name}\n";

    my $ccflags   = $Config{ ccflags};
    my $warnflags = $cc_specs{ $COMPILER}->{ warnflags};
    my $ldflags = $COMPILER eq 'emx' ? "" : "$Config{ ldflags} ";
    my $lddlflags = $Config{ lddlflags};
    if ( $COMPILER eq 'msvc' && $ccflags =~ /TP/) { # M$VC++ protection
        $warnflags =~ s/W3/W1/;
        $ldflags =~ s/(-|\/)nodefaultlib//g;
        $lddlflags =~ s/(-|\/)nodefaultlib//g;
    }
    setvar( 'CFLAGS',  "$ccflags $warnflags $cc_specs{ $COMPILER}->{ cflags}" . ( defined $ENV{ PRIMA_DEVEL} ? " $cc_specs{ $COMPILER}->{ cdebugflags}" : " " . ( $Config{ optimize} || '')) . " ");
    setvar( 'CDLFLAGS', "$Config{ cccdlflags} $Config{ ccdlflags}");
    setvar( 'COUTOFLAG', $cc_specs{ $COMPILER}->{ coutoflag});
    setvar( 'COUTEXEFLAG', $cc_specs{ $COMPILER}->{ coutexeflag});
    setvar( 'CINCPATHFLAG', '-I');
    setvar( 'CDEFFLAG', '-D');
    setvar( 'COMPONLYFLAG', '-c');
    setvar( 'CLINKPREFIX', $cc_specs{ $COMPILER}->{ clinkprefix});
    setvar( 'CLIBPATHFLAG', $cc_specs{ $COMPILER}->{ clibpathflag});
    setvar( 'LD', $Config{ ld});
    setvar( 'LDFLAGS', "$ldflags $lddlflags $cc_specs{ $COMPILER}->{ ldflags}" . ( defined $ENV{ PRIMA_DEVEL} ? " $cc_specs{ $COMPILER}->{ lddebugflags}" : "") . " ");
    setvar( 'LDOUTFLAG', $cc_specs{ $COMPILER}->{ ldoutflag});
    setvar( 'LDLIBPATHFLAG', $cc_specs{ $COMPILER}->{ ldlibpathflag});
    setvar( 'LDLIBFLAG', $cc_specs{ $COMPILER}->{ ldlibflag});
    setvar( 'OBJ_EXT', $Config{ _o});
    setvar( 'LIB_EXT', $Config{ _a});
    setvar( 'EXE_EXT', $Config{ _exe});
    setvar( 'SCRIPT_EXT',
            ( $Win32 ? '.bat' : ( $OS2 ? '.cmd' : '')));
    setvar( 'DL_EXT', "." . $Config{ dlext});
    setvar( 'INCPATH', (
                        "include",
                        qd( "include/generic"),
                        $Config{installarchlib} . qd( "/CORE"),
                       )
          );
    my @libpaths = quoted_split( $Config{ libpth});
    if ( $COMPILER eq 'msvc') {
       # explanation: link /LIBPATH:"Program Files" dirty fails, hail to M$ :E,
       # trying to link Files.obj with /LIBPATH:Program
       my %envlibs = map { lc($_) => 1} split(';', $ENV{LIB});
       @libpaths = grep {
          my $d = lc $_;
          $d =~ s/^"//;
          $d =~ s/"$//;
          exists($envlibs{$d}) ? 0 : 1
       } @libpaths;
    }
    setvar( 'LIBPATH', @libpaths);
    my @libs = map { s/^$LDLIBFLAG//; $_} quoted_split($Config{ libs});
    #    push ( @libs, 'pmprintf') if $OS2 && defined $ENV{PRIMA_PMPRINTF};
    push ( @libs, ( $Win32 || $OS2) ? 'prigraph.lib' : 'prigraph') if $usePrigraph;
    setvar( 'LIBS', ());
    my @flibs;

# here starts lot of execs...

    for my $lib (@libs) {
       if ( defined find_lib($lib,'')) {
          push @flibs, $lib;
       } else {
          print "*** Warning (probably harmless): `$lib' library not found\n";
       }
    }
    setvar( 'LIBS', @flibs);
    %DEFINES = (
                PERL_PATCHLEVEL => $Config{ patchlevel},
                PERL_SUBVERSION => $Config{ subversion},
                PRIMA_CORE => 1,
                PERL_POLLUTE => 1,
               );
    $DEFINES{ PARANOID_MALLOC} = 1
        if ( $ENV{ PRIMA_PARANOID_MALLOC}
             && $ENV{ PRIMA_PARANOID_MALLOC} =~ /^1|yes|on|true$/i);
    while ( $CFLAGS =~ s/-D(\w+)(?:=(\S+))?\s*//) {
        my ( $defname, $defvalue) = ( $1, $2 || 'yep');
        $DEFINES{ $defname} = $defvalue;
    }

    setvar( 'PREFIX', $Config{ installsitearch});
    setvar( 'INSTALL_BIN', $Config{ installbin});
    setvar( 'INSTALL_LIB', $PREFIX . qd( "/Prima"));
    setvar( 'INSTALL_DL', $PREFIX . qd( "/auto/Prima"));
    setvar( 'INSTALL_EXAMPLES', $PREFIX . qd( "/Prima/examples"));

    setvar( 'MAKETYPE', lc ( $MAKE = $Config{ make}));
    # Note that Borland's make utility also named 'make'. But its
    # usage is deprecated, thus we can ignore it.
    #
    # In Linux GNU make also has name 'make'. But Makefile been
    # generated by this script is compatible with it. If it named
    # 'gmake' then we force $MAKETYPE to contain 'make'.
    $MAKETYPE = 'make' if $MAKETYPE =~ /^gmake|pmake$/;
    die "Unknown make utility" unless $MAKETYPE =~ /^make|nmake|dmake$/;
    setvar( 'RM', $Config{ rm});
    setvar( 'SHQUOTE', $Win32 ? '"' : "'");
    $OS2DLLF = 'Prima';
    $OS2DLLF = &DynaLoader::mod2fname([$OS2DLLF]) if $OS2 && defined &DynaLoader::mod2fname;
    $PrimaTarget = qd( "auto/Prima/$OS2DLLF$DL_EXT");
    setvar( 'DEFFILE', $Win32 ? qd( "win32/Prima.def") : ( $OS2 ? qd( "os2/Prima.def") : ''));

    @Prima_exports = qw(
  boot_Prima
  Object_create
  Object_destroy
  apc_application_create
  apc_application_destroy
  apc_application_get_size
  apc_application_go
  apc_application_lock
  apc_application_unlock
  apc_component_create
  apc_component_destroy
  apc_font_pick
  apc_fonts
  apc_gp_arc
  apc_gp_bar
  apc_gp_clear
  apc_gp_done
  apc_gp_ellipse
  apc_gp_fill_ellipse
  apc_gp_flood_fill
  apc_gp_get_back_color
  apc_gp_get_color
  apc_gp_get_fill_pattern
  apc_gp_get_line_end
  apc_gp_get_line_pattern
  apc_gp_get_line_width
  apc_gp_get_pixel
  apc_gp_get_rop
  apc_gp_get_rop2
  apc_gp_get_text_width
  apc_gp_get_transform
  apc_gp_init
  apc_gp_line
  apc_gp_put_image
  apc_gp_rectangle
  apc_gp_set_back_color
  apc_gp_set_color
  apc_gp_set_fill_pattern
  apc_gp_set_font
  apc_gp_set_line_end
  apc_gp_set_line_pattern
  apc_gp_set_line_width
  apc_gp_set_pixel
  apc_gp_set_rop
  apc_gp_set_rop2
  apc_gp_set_transform
  apc_gp_stretch_image
  apc_gp_text_out
  apc_image_begin_paint
  apc_image_end_paint
  apc_menu_create
  apc_menu_default_font
  apc_menu_destroy
  apc_menu_get_color
  apc_menu_get_font
  apc_menu_item_set_accel
  apc_menu_item_set_check
  apc_menu_item_set_enabled
  apc_menu_item_set_key
  apc_menu_item_set_text
  apc_menu_set_color
  apc_menu_set_font
  apc_message
  apc_pointer_get_pos
  apc_popup
  apc_popup_create
  apc_popup_default_font
  apc_show_message
  apc_system_action
  apc_timer_create
  apc_timer_destroy
  apc_timer_get_timeout
  apc_timer_set_timeout
  apc_timer_start
  apc_timer_stop
  apc_widget_begin_paint
  apc_widget_create
  apc_widget_default_font
  apc_widget_destroy
  apc_widget_end_paint
  apc_widget_get_color
  apc_widget_get_focused
  apc_widget_get_pos
  apc_widget_get_size
  apc_widget_invalidate_rect
  apc_widget_is_captured
  apc_widget_is_enabled
  apc_widget_is_focused
  apc_widget_is_visible
  apc_widget_map_points
  apc_widget_scroll
  apc_widget_set_capture
  apc_widget_set_color
  apc_widget_set_enabled
  apc_widget_set_focused
  apc_widget_set_font
  apc_widget_set_pos
  apc_widget_set_size
  apc_widget_set_visible
  apc_widget_set_z_order
  apc_widget_update
  apc_widget_validate_rect
  apc_window_close
  apc_window_create
  apc_window_get_border_icons
  apc_window_get_border_style
  apc_window_get_client_size
  apc_window_get_window_state
  apc_window_set_client_size
  apc_window_set_menu
  apc_window_set_window_state
  build_dynamic_vmt
  build_static_vmt
  call_perl
  call_perl_indirect
  clean_perl_call_method
  clean_perl_call_pv
  create_object
  ctx_remap_def
  cv_call_perl
  debug_write
  eval
  gimme_the_mate
  gimme_the_vmt
  query_method
  kill_zombies
  kind_of
  notify_perl
  parse_hv
  pop_hv_for_REDEFINED
  push_hv
  push_hv_for_REDEFINED
  sv_call_perl
  sv_query_method
  temporary_prf_Sv
);
}

sub setup_paths
{
    if ( $NeedX11) {
        push @INCPATH, ( qd( "/usr/include"), qd( "/usr/local/include"));
        my $incpath = find_header( qd( "X11/Xlib.h"), qd( "/usr/X11R6/include"), qd( "/usr/X11/include"));
        die "Prima needs X11 headers for compilation! Perhaps, you have non-standard path to X11?\n" unless defined $incpath;
        print "X11 includes found in $incpath\n";
        push @INCPATH, $incpath;
        my $libpath = find_lib( "X11", "/usr/X11R6/lib", "/usr/X11/lib");
        die "Prima needs X11 libraries for compilation! Perhaps, you have non-standard path to X11?\n" unless defined $libpath;
        print "X11 libraries found in $libpath\n";
        push @LIBPATH, $libpath;
        push @LIBS, 'X11';
        if (defined find_lib( "Xext", '')) {
            print "Xext library found.\n";
            push @LIBS, "Xext";
        }
    }
    else {
        my $libpath = find_lib( $Config{ libperl}, "", qd( "$Config{ archlib}/CORE"));
        die "Cannot find perl library" unless defined $libpath;
        print "Perl library found in ", ( $libpath || "default LIBPATH"), "\n";
        push @LIBPATH, $libpath if $libpath;
        push @LIBS, $Config{ libperl};
    }
}

sub setup_defines
{
    have_header( "io.h");
    have_header( "unistd.h");
    have_header( "strings.h");

    have_types_in( "sys/types.h",
                   qw(int8_t int16_t int32_t));
    have_types_in( "sys/types.h",
                   qw(uint8_t uint16_t uint32_t));
    have_types_in( "sys/types.h",
                   qw(u_int8_t u_int16_t u_int32_t));

    have_types_in( "stdint.h",
                   qw(int8_t int16_t int32_t));
    have_types_in( "stdint.h",
                   qw(uint8_t uint16_t uint32_t));
    have_types_in( "stdint.h",
                   qw(u_int8_t u_int16_t u_int32_t));

    if ( $NeedX11) {
       have_header( "sys/ipc.h", "sys/types.h");
       have_header( "sys/shm.h", "sys/types.h");
       have_header( "X11/extensions/shape.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");
       have_header( "X11/extensions/XShm.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");
    }
    if ( $OS2 && defined $ENV{ PRIMA_PMPRINTF}) {
        if ( have_header( "pmprintf.h", "os2.h")) {
            my $libpath = find_lib( 'pmprintf', "");
            die "There is no library for pmprintf" unless defined $libpath;
            push @LIBS, 'pmprintf';
        }
        else {
            warn "PRIMA_PMPRINTF defined, but there is no pmprintf.h in include path";
        }
    }
    have_func( 'strcasecmp') || have_funcs_in( 'strings.h', 'stricmp');
    have_funcs_in( 'stdio.h', 'snprintf') || have_funcs_in( 'stdio.h', '_snprintf');
    have_func( 'reallocf');
    have_func( 'bzero');
    if ( $Win32) {
        have_type( "BOOLEAN", "windows.h");
    }

    foreach my $defname ( keys %USER_DEFINES) {
        if ( defined $USER_DEFINES{ $defname}) {
            $DEFINES{ $defname} = $USER_DEFINES{ $defname};
        }
        else {
            delete $DEFINES{ $defname} if exists $DEFINES{ $defname};
        }
    }
    foreach my $defname ( keys %USER_DEFINES_ADDONS) {
        $DEFINES{ $defname} = ( $DEFINES{ $defname} || '') . join( '', @{ $USER_DEFINES_ADDONS{ $defname}});
    }
}

sub generate_win32_def
{
    open PRIMADEF, ">$DEFFILE" or die "Cannot create $DEFFILE: $!";
    print PRIMADEF <<ENDP;
LIBRARY Prima
DESCRIPTION 'Prima Guts version 2.00, Copyright (C) the Protein Laboratory 1997'
CODE LOADONCALL
DATA LOADONCALL NONSHARED MULTIPLE
EXPORTS
ENDP
    if ( $COMPILER eq 'bcc32') {
        print PRIMADEF map { "\t_$_\n\t$_=_$_\n"} @Prima_exports;
    }
    else {
        print PRIMADEF map { "\t$_\n\t_$_ = $_\n"} @Prima_exports;
    }
    close PRIMADEF;
}

sub generate_os2_def
{
    open PRIMADEF, ">$DEFFILE" or die "Cannot create $DEFFILE: $!";
    print PRIMADEF <<ENDP;
LIBRARY $OS2DLLF INITINSTANCE TERMINSTANCE
DESCRIPTION 'Prima Guts version 2.00, Copyright (C) the Protein Laboratory 1997'
CODE LOADONCALL
DATA LOADONCALL NONSHARED MULTIPLE
EXPORTS
ENDP
    print PRIMADEF map { "\t$_\n"} @Prima_exports;
    close PRIMADEF;
}


sub setup_misc
{
    push @Prima_exports, 'bzero' unless $DEFINES{HAVE_BZERO};
    # Setting up translation tables for quoting string for different make utilities.
    my ( $make_tbl, $nmake_tbl, $dmake_tbl);
    $make_tbl = [ map { chr} 0..255];
    $dmake_tbl = [ map { chr} 0..255];
    $nmake_tbl = [ map { chr} 0..255];
    foreach ( ':', ';', '#', '(', ')', '^', '\\', '{', '}', '!', '@', '-') {
        $nmake_tbl->[ ord] = "^$_";
    }
    $nmake_tbl->[ ord '%'] = '%%';
    $nmake_tbl->[ ord '$'] = $make_tbl->[ ord '$'] = '$$';
    $dmake_tbl->[ ord '#'] = '\\#';
    $make_tbl->[ ord '#'] = '\\#';
    %make_trans = (
                   make => $make_tbl,
                   nmake => $nmake_tbl,
                   dmake => $dmake_tbl,
                  );
    generate_win32_def if $Win32;
    generate_os2_def if $OS2;
}

sub process_commandline
{
    %overrideable = map { /^(.)(.*)$/; $2 => $1} @ovvars; # Script variables which may be overridden.
    $ARGV_STR = join( " ", map { "\"$_\""} @ARGV);
    foreach my $arg ( @ARGV) {
        if ( $arg =~ /^\s*(\w+)\s*(\+?)\=(.*)$/) {
            my ( $varname, $setmode, $varval) = ( $1, $2 || '', $3);
            die "Unknown variable $varname" unless $overrideable{ $varname};
            if ( $overrideable{ $varname} eq '$') {
                if ( $setmode eq '+') {
                    push @{ $USER_VARS_ADDONS{ $varname}}, $varval;
                }
                else {
                    $USER_VARS{ $varname} = $varval;
                }
                die $@ if $@;
            }
            elsif ( $overrideable{ $varname} eq '@') {
                my @values = split /$path_sep/, $varval;
                if ( $setmode eq '+') {
                    push @{ $USER_VARS_ADDONS{ $varname}}, @values;
                }
                else {
                    $USER_VARS{ $varname} = @values;
                }
                die $@ if $@;
            }
        }
        elsif ( $arg =~ /^-(D|U)(\w+)(?:(\+?)=(.*))?$/) {
            my ( $defmode, $defname, $setmode, $value) = ( $1, $2, $3 || '', $4 || '');
            if ( $defmode eq 'U') {
                $USER_DEFINES{ $defname} = undef; # I.e. it will exists in the hash...
            }
            else {
                if ( $setmode eq '+') {
                    push @{ $USER_DEFINES_ADDONS{ $defname}}, $value;
                }
                else {
                    $USER_DEFINES{ $defname} = $value;
                }
            }
        }
        else {
            die "Unknown command line argument or wrong syntax: '$arg'";
        }
    }
}

sub find_file
{
    my ( $fname, $dir) = @_;
    my $pathname = qd( "$dir/$fname");
    if ( -e $pathname) {
        return ( $pathname, 1);
    }
    opendir D, $dir or die "Cannot open dir $dir: $!";
    my @entries = map { qd( "$dir/$_")} grep { /^[^.]/ && -d qd( "$dir/$_")} readdir D;
    closedir D;
    my $found;
    foreach my $entry ( @entries) {
        ( $pathname, $found) = find_file( $fname, $entry);
        return ( $pathname, 1) if $found;
    }
    return ( $fname, 0);
}

sub canon_name
{
    my ( $fname) = @_;
    my $qdirsep = quotemeta( $dir_sep);
    $fname =~ s{[^$qdirsep]+$qdirsep\.\.(?:$qdirsep|\Z)}{}
        while $fname =~ /(?:$qdirsep|\A)\.\.(?:$qdirsep|\Z)/;
    $fname =~ s{(?:(?<=$qdirsep)|(?<=\A))\.(?=$qdirsep|\Z)$qdirsep?}{}g;
    return $fname;
}

sub find_cdeps
{
    my ( $cfile, $deps, $included) = @_;

    $deps ||= {};
    $included ||= {};

    return () if exists $deps->{ $cfile};
    $deps->{ $cfile} = [];
    return @{ $alldeps{ $cfile}} if exists $alldeps{ $cfile};
    $alldeps{ $cfile} = [];
    return () unless -f $cfile;

    local *CF;
    open CF, "<$cfile" or die "Cannot open $cfile: $!";
    while ( <CF>) {
        chomp;
        next unless /^\s*\#\s*include\s+"([^\"]+)"/;
        my $incfile = $1;
        my $found = 0;
        ( $incfile, $found) = find_file( $incfile, ".");
        $incfile = qd( "include/generic/$incfile") unless $found;
        $incfile = canon_name( $incfile);
        unless ( exists $included->{ $incfile}) {
            push @{ $alldeps{ $cfile}}, $incfile;
            push @{ $deps->{ $cfile}}, $incfile;
            $included->{ $incfile} = 1;
        }
        my @subdeps = find_cdeps( $incfile, $deps, $included);
        push @{ $deps->{ $cfile}}, @subdeps;
        push @{ $alldeps{ $cfile}}, @subdeps;
    }
    close CF;
    return @{ $deps->{ $cfile}};
}

sub cmake
{
    my ( $cfile) = @_;
    print "Finding dependencies for $cfile...\n";
    my $ofile = "$1$OBJ_EXT" if $cfile =~ /^(.*)\.c$/;
    die "Internal error: illegal c file"
        unless defined $ofile;
    $cfile = qd( $cfile);
    $ofile = qd( $ofile);
    push @allclean, $ofile;
    push @allojects, $ofile;
    my @deps = find_cdeps( $cfile);
    return(
           "$ofile: Makefile $cfile @deps\n\t" .
           join( " ", cc_command_line( $cfile, $ofile, "", 1, 1)) .
           "\n\n"
          );
}

sub clsmake
{
    my ( $clsfile) = @_;
    print "Finding dependencies for $clsfile...";
    my $classname = $1 if $clsfile =~ /^(.*)\.cls$/;
    die "Internal error: illegal cls file"
        unless defined $classname;
    my $mk = qd( "include/generic/$classname.h"
               ) . ": Makefile " .
                   "$clsfile " . qd( "utils/gencls.pl ") .
		   qd( "Prima/Gencls.pm ");
    push @alltml, qd( "include/generic/$classname.tml");
    push @alltmldeps, qd( "include/generic/$classname.h");
    push @allclean, (
                     qd( "include/generic/$classname.h"),
                     qd( "include/generic/$classname.inc"),
                     qd( "include/generic/$classname.tml")
                    );
    push @allinstall, qd( "include/generic/$classname.h"), $INSTALL_LIB . qd( "/CORE/generic");
    my @ancestors = gencls( $clsfile, depend => 1);
    $mk .= qd( "include/generic/$_.h $_.cls ") foreach @ancestors;
    print "\n";
    $mk .= "\n\t$^X utils/gencls.pl " .
        "--inc --h --tml $clsfile include/generic\n\n";
}

#
# Here we go...
#

$Win32 = ( $^O =~ /mswin32/i);
$OS2 = ( $^O =~ /os2/i);
$dir_sep = (( ( $path_sep = $Config{ path_sep}) eq ':') || ( defined $Config{ emxpath})) ? '/' : '\\';

if ( $#ARGV >= 0 && $ARGV[ 0] =~ /^\-\-cp(bin)?$/) {
    my $isbin = defined $1;
    shift @ARGV;
    die qq(Even number of parameters expected) unless ( $#ARGV % 2);
    while ( scalar @ARGV) {
        my ( $src, $dst) = ( shift @ARGV, shift @ARGV);
        print qq(Installing $src -> $dst\n);
        next unless -f $src;
        if ( $isbin) {
            my $dstdir = dirname( $dst);
            mkpath $dstdir if $dstdir && ! -d $dstdir;
            open SRCPL, "<$src" or die "Cannot open $src: $!";
            open DSTPL, ">$dst" or die "Cannot create $dst: $!";
            chmod 0555, $dst unless $Win32 || $OS2;
            if ( $Win32) {
                print DSTPL <<ENDP;
\@rem = '--*-Perl-*--
\@echo off
if "%OS%" == "Windows_NT" goto WinNT
$^X -w -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
$^X -w -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\\system32\\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
\@rem ';
#!perl
ENDP
            }
            elsif ( $OS2) {
                my $perlpath = $Config{ perlpath};
                $perlpath =~ s/(perl)(\.exe)?$/$1__$2/i if $perlpath =~ /perl(\.exe)?$/i;
                print DSTPL <<ENDP;
extproc $perlpath -wS
ENDP
            }
            else {
                print DSTPL <<ENDP;
#! $Config{ perlpath} -w
ENDP
            }
            my $filestart = 1;
            while ( <SRCPL>) {
                next if $filestart && /^\#\!/;
                $filestart = 0;
                print DSTPL;
            }
            if ( $Win32) {
                print DSTPL <<ENDP;
__END__
:endofperl
ENDP
            }
            close SRCPL;
            close DSTPL;
        }
        else {
            die qq(Destination must be a directory) if -e $dst && ! -d $dst;
            mkpath $dst unless -d $dst;
            copy( $src, $dst) or die qq(Copy failed: $!);
        }
    }
}
elsif ( $#ARGV >= 0 && $ARGV[ 0] eq '--md') {
    shift @ARGV;
    mkpath \@ARGV;
}
elsif ( $#ARGV >= 0 && $ARGV[ 0] eq '--rm') {
    shift @ARGV;
    unlink @ARGV;
}
else {
    $CWD = cwd;

    die "This script should be launched from Prima root directory\n"
        unless -r qd( "include/apricot.h") && -d "Prima";

    $| = 1;

    print "Setting up working environment.\n";
    process_commandline;
    setup_variables;
    setup_paths;
    setup_defines;
    setup_misc;

    push @alldirs, qd( "include/generic"), qd( "auto/Prima");

    my $make = <<MAKE;
# Makefile for Prima project under $Config{archname}
#
# THIS IS GENERATED FILE.
#
# Do not edit -- all changes will be lost.
# Edit Makefile.PL instead.

all: prima

MAKE

    if ( $ENV{ HOME} && ( -d qd( "$ENV{ HOME}/.Makeinc"))) {
        my $hdir = "$ENV{ HOME}/.Makeinc";
        if ( -f "$hdir/header.PL") {
            push @headers, `$^X $hdir/header.PL`;
            push @Makefile_deps, "$hdir/header.PL";
        }
        if ( -f "$hdir/footer.PL") {
            push @footers, `$^X $hdir/footer.PL`;
            push @Makefile_deps, "$hdir/footer.PL";
        }
        if ( -f "$hdir/Prima.ph") {
            do "$hdir/Prima.ph";
            push @Makefile_deps, "$hdir/Prima.ph";
        }
    }

    $make .= join( "\n", @headers) . "\n";

    while ( <*.cls>) {
        $make .= clsmake( $_);
    }
    while ( <*.c>) {
        $make .= cmake( $_);
    }
    while ( <img/*.c>) {
        $make .= cmake( $_);
    }
    while ( <$PLATFORM/*.c>) {
        $make .= cmake( $_);
    }
    while ( <examples/*.pl>) {
        push @allbins, $_, $INSTALL_EXAMPLES;
    }
    while ( <utils/*.pl>) {
        push @allbins, $_, $INSTALL_BIN;
    }

    my $thunks_tinc = qd( "include/generic/thunks.tinc");
    push @allclean, $thunks_tinc;

    print "Writing Makefile...";
    $make =~ tr!/!!s;
    open MAKE, ">Makefile" or die "Creation of Makefile failed: $!";
    print MAKE $make;

# dist-related veriables
    my $os_suffix = $^O;
    $os_suffix =~ s/\s/_/g;
    my $bindist_files = 'Prima.pm auto examples Prima';
    my $srcdist_files = 'Makefile.PL *.c *.cls Prima.pm examples img include os2 prigraph Prima test unix utils win32';
    my $distname = 'Prima-1.00';

    print MAKE <<MAKE;
$thunks_tinc: Makefile @alltmldeps
\t$^X utils/tmlink.pl -Iinclude/generic -o$thunks_tinc @alltml

RM=$^X Makefile.PL --rm

clean:
\t\$(RM) @allclean $PrimaTarget $distname-$os_suffix.zip

bindist: all
\t\$(RM) $distname-$os_suffix.zip
\t\@zip -r $distname-$os_suffix.zip $bindist_files
\t\@perl -e "if(open F, q(.dist-exclude)) { my \$\$x = q(zip $distname-$os_suffix.zip -d ); while(<F>){chomp; \$\$x .= qq( \$\$_);} close F; system \$\$x; }";

zipdist:
\t\$(RM) $distname.zip
\t\@zip -r $distname.zip $srcdist_files
\t\@perl -e "if(open F, q(.dist-exclude)) { my \$\$x = q(zip $distname.zip -d ); while(<F>){chomp; \$\$x .= qq( \$\$_);} close F; system \$\$x; }";

tardist:
\t\$(RM) $distname.tar
\t\@tar -cv -X .dist-exclude -f $distname.tar $srcdist_files

dist: tardist
\t\$(RM) $distname.tar.gz
\t\@gzip -9 $distname.tar

prima: dirs $PrimaTarget

$PrimaTarget: @allojects
\t${ \eval{ return join ' ', ld_command_line( $PrimaTarget, @allojects)}}

Makefile: Makefile.PL @Makefile_deps
\t\@echo Rebuilding Makefile...
\t\@$^X Makefile.PL $ARGV_STR
\t\@$MAKE
\t\@echo You are safe to ignore the following error...
\t\@false

CP=$^X Makefile.PL --cp
CPBIN=$^X Makefile.PL --cpbin

install: all
\t\@\$(CP) \\
\t$PrimaTarget $INSTALL_DL \\
MAKE

    my ( $instcnt, $maxcnt) = ( 1, 20);
    finddepth(
              sub
              {
                  unless ( -d $_) {
                      print MAKE "\t", qd( "$File::Find::dir/$_ $PREFIX/$File::Find::dir");
                      if ( ( $instcnt % $maxcnt) == 0) {
                          print MAKE "\n\t\@\$(CP)";
                      }
                      $instcnt++;
                      print MAKE " \\\n";
                  }
              }, 'Prima');
    while ( scalar @allinstall) {
        my ( $src, $dst) = ( shift @allinstall, shift @allinstall);
        print MAKE "\t$src $dst";
        if ( ( $instcnt % $maxcnt) == 0) {
            print MAKE "\n\t\@\$(CP)";
        }
        $instcnt++;
        print MAKE " \\\n";
    }
    while ( <include/*.h>) {
        my ( $dsth) = $_;
        $dsth =~ s/^include/CORE/;
        print MAKE "\t$_ ", dirname( qd( "$INSTALL_LIB/$dsth"));
        if ( ( $instcnt % $maxcnt) == 0) {
            print MAKE "\n\t\@\$(CP)";
        }
        $instcnt++;
        print MAKE " \\\n";
    }
    while ( <include/$PLATFORM/*.h>) {
        my ( $dsth) = $_;
        $dsth =~ s/^include/CORE/;
        print MAKE "\t$_ ", dirname( qd( "$INSTALL_LIB/$dsth"));
        if ( ( $instcnt % $maxcnt) == 0) {
            print MAKE "\n\t\@\$(CP)";
        }
        $instcnt++;
        print MAKE " \\\n";
    }
    print MAKE "\tPrima.pm $PREFIX\n";

    print MAKE "\t\@\$(CPBIN)";
    $instcnt = 1;
    while ( scalar @allbins) {
        my ( $src, $dst) = ( shift @allbins, shift @allbins);
        if ( ( $instcnt % $maxcnt) == 0) {
            print MAKE "\n\t\@\$(CPBIN)";
        }
        $dst = qd( "$dst/" . basename( $src, '.pl')) . $SCRIPT_EXT;
        print MAKE " \\\n\t$src $dst"
    }
    print MAKE "\n";

    print MAKE <<MAKE;

MD=$^X Makefile.PL --md

dirs: @alldirs

@alldirs:
\t\@echo Creating directories...
\t\@\$(MD) @alldirs

MAKE

    print MAKE join( "\n", @footers), "\n";

    close MAKE;
    print "\nAll done.  Now you can run ${ MAKE}.\n";
}

exit;
