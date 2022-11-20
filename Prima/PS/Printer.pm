=pod

=head1 NAME

Prima::PS::Printer - PostScript interface to Prima::Printer

=head1 SYNOPSIS

use Prima;
use Prima::PS::Printer;

=head1 DESCRIPTION

Realizes the Prima printer interface to PostScript level 2 document language
through Prima::PS::PostScript module and PDF v1.4 through the Prima::PS::PDF
module. Allows different user profiles to be created and managed with GUI setup
dialog. The module is designed to be compliant with Prima::Printer interface.

Also contains convenience classes (File, LPR, Pipe) for non-GUI use.

=head1 SYNOPSIS

	use Prima::PS::Printer;

	my $x;
	if ( $preview) {
		$x = Prima::PS::Pipe-> new( command => 'gv $');
	} elsif ( $print_in_file) {
		$x = Prima::PS::File-> new( file => 'out.ps');
	} elsif ( $print_on_device) {
		$x = Prima::PS::LPR-> new( args => '-d colorprinter');
	} elsif ( $print_pdf_file ) {
		$x = Prima::PS::PDF::File-> new( file => 'out.pdf');
	} else {
		$x = Prima::PS::FileHandle-> new( handle => \*STDOUT );
	}
	$x-> begin_doc;
	$x-> font-> size( 300);
	$x-> text_out( "hello!", 100, 100);
	$x-> end_doc;

=cut

use strict;
use warnings;
use Prima;
use Prima::Utils;
use IO::Handle;
use Prima::PS::PostScript;
use Prima::PS::PDF;
use Prima::PS::TempFile;
use Prima::PS::Setup;
use Prima::MsgBox;

package Prima::PS::Printer::Common;
use Prima::sys::FS;

our $unix = Prima::Application-> get_system_info-> {apc} == apc::Unix;

our @spooler_types = qw(lpr file cmd fh);

our %pageSizes = ( # in points
	'A0'  => [ 2391, 3381],
	'A1'  => [ 1688, 2391],
	'A2'  => [ 1193, 1688],
	'A3'  => [ 843, 1193],
	'A4'  => [ 596, 843],
	'A5'  => [ 419, 596],
	'A6'  => [ 297, 419],
	'A7'  => [ 209, 297],
	'A8'  => [ 146, 209],
	'A9'  => [ 103, 146],
	'B0'  => [ 2929, 4141],
	'B1'  => [ 2069, 2929],
	'B10' => [ 89, 126],
	'B2'  => [ 1463, 2069],
	'B3'  => [ 1034, 1463],
	'B4'  => [ 729, 1034],
	'B5'  => [ 516, 729],
	'B6'  => [ 362, 516],
	'B7'  => [ 257, 362],
	'B8'  => [ 180, 257],
	'B9'  => [ 126, 180],
	'C5E' => [ 462, 650],
	'US Common #10 Envelope' => [ 297, 684],
	'DLE'       => [ 311, 624],
	'Executive' => [ 541, 721],
	'Folio'     => [ 596, 937],
	'Ledger'    => [ 1227, 792],
	'Legal'     => [ 613, 1011],
	'Letter'    => [ 613, 792],
	'Tabloid'   => [ 792, 1227],
);

sub data
{
	return $_[0]-> {data} unless $#_;
	my ( $self, $dd) = @_;
	my $p = $self-> {data};
	$self-> resolution( $p-> {resolution}, $p-> {resolution})
		if exists $dd-> {resolution};
	$self-> scale( $p-> {scaling}, $p-> {scaling}) if exists $dd-> {scaling};
	$self-> reversed( $p-> {portrait} ? 0 : 1) if exists $dd-> {portrait};
	$self-> pageSize(
		@{exists($pageSizes{$p-> {page}}) ?
			$pageSizes{$p-> {page}} : $pageSizes{A4}}
	) if exists $dd-> {page};
	$self-> grayscale( $p-> {color} ? 0 : 1) if exists $dd-> {color};
}

sub defaultData
{
	return $_[0]-> {defaultData} unless $#_;
	my ( $self, $dd) = @_;
	for ( keys %$dd) {
		$self-> {defaultData}-> {$_} = $dd-> {$_};
	}
}

sub _begin_doc
{
	my ( $self, $docName) = @_;
	return 0 if $self-> get_paint_state;

	$self-> {spoolingFailed} = undef;
	$self-> {spoolSTDERR} = undef;
	if ($self-> {data}-> {spoolerType} ne 'fh') {
		$self-> {spoolHandle} = undef;
	} else {
		return 0 unless $self->{spoolHandle};
	}

	my $backend = $self->{backend};

	if ( $self-> {data}-> {spoolerType} eq 'file') {
		if ( $self-> {gui}) {
			require Prima::Dialog::FileDialog;
			my $f = Prima::save_file(
				defaultExt => $self->{fileExtension},
				filter     => [
					[ $self->{formatDescription} => "*.$self->{fileExtension}"], 
					[ 'All files' => '*.*']
				],
				text       => 'Print to file',
			);
			return 0 unless defined $f;
			my $h = IO::Handle-> new;
			unless ( open $h, ">", $f) {
				undef $h;
				message("Error opening $f:$!");
				goto AGAIN;
			}
			binmode $h;
			$self-> {spoolHandle} = $h;
			$self-> {spoolName}   = $f;
		} else { # no gui
			my $h = IO::Handle-> new;
			my $f = $self-> {data}-> {spoolerData};
			unless ( open $h, ">", $f) {
				undef $h;
				return 0;
			}
			binmode $h;
			$self-> {spoolHandle} = $h;
			$self-> {spoolName}   = $f;
		}
		unless ( $backend->can('begin_doc')->( $self, $docName)) {
			unlink( $self-> {spoolName});
			close( $self-> {spoolHandle});
			return 0;
		}
		return 1;
	} elsif ( $self-> {data}-> {spoolerType} eq 'cmd' && $self->{data}->{spoolerData} =~ /\$/) {
		my $f = Prima::PS::TempFile->new(unlink => 0, warn => !$self->{gui});
		unless ( defined $f ) {
			message("Error creating temporary file: $!") if $self-> {gui};
			return 0;
		}
		$self-> {spoolTmpFile} = $f;
		$self-> {spoolHandle}  = $f->{fh};
		$self-> {spoolName}    = $f->{filename};
	}
	return $backend->can('begin_doc')->( $self, $docName);
}

my ( $sigpipe);

sub __end
{
	my ($self, $aborted) = @_;
	close( $self-> {spoolHandle}) if $self-> {spoolHandle} && $self-> {data}-> {spoolerType} ne 'fh';
	if ( $self-> {data}-> {spoolerType} ne 'file') {
		defined($sigpipe) ? $SIG{PIPE} = $sigpipe : delete($SIG{PIPE});
	}
	$self-> {spoolHandle} = undef if $self->{data}->{spoolerType} ne 'fh';
	$self-> {spoolingFailed} = undef;

	if ( $self->{data}->{spoolerType} eq 'cmd' && $self->{data}->{spoolerData} =~ /\$/) {
		unless ( $aborted ) {
			my $cmd = $self->{data}->{spoolerData};
			my $tmp = $self->{spoolName};
			$cmd =~ s/\$/$tmp/g;
			if ( $self->{gui} && $cmd !~ />2/) {
				$self->{spoolSTDERR} = Prima::PS::TempFile->new_filename;
				$cmd .= " 2>$self->{spoolSTDERR}";
			}
			$self-> show_msg("Error running '$cmd'") if system $cmd;
		}
		$self->{spoolTmpFile}->remove;
		undef $self->{spoolTmpFile};
	}

	unlink $self->{spoolSTDERR} if defined $self->{spoolSTDERR};
	$self-> {spoolSTDERR} = undef;

	$sigpipe = undef;
}

sub _end_doc
{
	my $self = $_[0];
	my $backend = $self->{backend};
	$backend->can('end_doc')->($self);
	$self-> __end(0);
}

sub _abort_doc
{
	my $self = $_[0];
	my $backend = $self->{backend};
	$backend->can('abort_doc')->($self);
	$self-> __end(1);
	unlink $self-> {spoolName} if $self-> {data}-> {spoolerType} eq 'file';
}

sub show_msg
{
	my ( $self, $msg ) = @_;
	return unless $self->{gui};

	if (
		defined($self->{spoolSTDERR}) &&
		(open my $f, "<", $self->{spoolSTDERR} )
	) {
		local $/;
		$msg .= ": " . <$f>;
		close $f;
	}

	message($msg);
}

sub _spool
{
	my ( $self, $data) = @_;

	my $piped = 0;

	return 0 if $self->{spoolingFailed};

	if ( $self-> {data}-> {spoolerType} ne 'file' && !$self-> {spoolHandle}) {
		my @cmds;
		if ( $self-> {data}-> {spoolerType} eq 'lpr') {
			push( @cmds, map { $_ . ' ' . $self-> {data}-> {spoolerData}} qw(
				lp lpr /bin/lp /bin/lpr /usr/bin/lp /usr/bin/lpr));
		} else {
			push( @cmds, $self-> {data}-> {spoolerData});
		}
		my $ok = 0;
		$sigpipe = $SIG{PIPE};
		$SIG{PIPE} = 'IGNORE';
		$self->{spoolSTDERR} = $self->{gui} ? Prima::PS::TempFile->new_filename : undef;
		CMDS: for ( @cmds) {
			$piped = 0;
			my $f = IO::Handle-> new;
			my $cmd = "|$_";
			$cmd .= " 2>$self->{spoolSTDERR}" if $self->{gui};
			next unless open $f, $cmd;
			binmode $f;
			$f-> autoflush(1);
			$piped = 1 unless print $f $data;
			close( $f), next if $piped;
			$ok = 1;
			$self-> {spoolHandle} = $f;
			$self-> {spoolName}   = $_;
			last;
		}
		$self-> show_msg("Error printing to '$cmds[0]'") if !$ok;
		$self-> {spoolingFailed} = 1 if !$ok;
		return $ok;
	}

	if (
		!(print {$self-> {spoolHandle}} $data) ||
		( $piped && $self-> {data}-> {spoolerType} ne 'file' )
	) {
		$self-> show_msg("Error printing to '$self->{spoolName}'");
		$self-> {spoolingFailed} = 1;
		return 0;
	}

	return 1;
}


sub install_api
{
	my $pkg = shift;
	no strict 'refs';
	no warnings 'redefine';
	for my $sym ( qw(begin_doc end_doc abort_doc spool)) {
		*{$pkg . '::' . $sym} = UNIVERSAL::can(__PACKAGE__, '_' . $sym);
	}
}

sub options
{
	my $self = shift;

	if ( 0 == @_) {
		return qw(
			Color Resolution PaperSize Scaling Orientation 
		), keys %{$self->{data}->{devParms}};
	} elsif ( 1 == @_) {
		# get value
		my $v = shift;
		my $d = $self-> {data};

		if ( $v eq 'Orientation') {
			return $d->{portrait} ? 'Portrait' : 'Landscape'
		} elsif ( $v eq 'Color') {
			return $d->{color} ? 'Color' : 'Monochrome'
		} else {
			$v = 'page' if $v eq 'PaperSize';
			$v = lcfirst $v;
			return $d-> {$v};
		}
	} else {
		my %newdata;
		my $successfully_set = 0;
		while ( @_ ) {
			# set value
			my ( $opt, $val) = ( shift, shift);
			my $d = $self-> {data};
			if ( $opt eq 'Orientation') {
				next unless $val =~ /^(?:(Landscape)|(Portrait))$/;
				$newdata{portrait} = $2 ? 1 : 0;
			} elsif ( $opt eq 'Color') {
				next unless $val =~ /^(?:(Color)|(Monochrome))$/;
				$newdata{color} = $1 ? 1 : 0;
			} else {
				$opt = lcfirst $opt;
				$opt = 'page' if $opt eq 'PaperSize';
				next unless exists $d->{$opt};
				$newdata{$opt} = $val;
			}
			$successfully_set++;
		}
		$self-> data( \%newdata);
		return $successfully_set;
	}
}

package Prima::PS::Printer;
use base qw(Prima::PS::PostScript Prima::PS::Printer::Common);

Prima::PS::Printer::Common::install_api(__PACKAGE__);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		resFile           => Prima::Utils::path . '/Printer',
		printer           => undef,
		gui               => 1,
		fileExtension     => 'ps',
		formatDescription => 'PostScript',
		defaultData   => {
			color          => 1,
			resolution     => 300,
			page           => 'A4',
			copies         => 1,
			isEPS          => 0,
			scaling        => 1,
			portrait       => 1,
			spoolerType    => $unix ? 'lpr' : 'file',
			spoolerData    => '',
			devParms       => {
				MediaType                   => '',
				MediaColor                  => '',
				MediaWeight                 => '',
				MediaClass                  => '',
				InsertSheet                 => 0,
				LeadingEdge                 => 0,
				ManualFeed                  => 0,
				TraySwitch                  => 0,
				MediaPosition               => '',
				DeferredMediaSelection      => 0,
				MatchAll                    => 0,
			},
		},
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p->{userData}->{$_} = $p->{$_} for qw(resolution reversed scaling);
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {backend} = 'Prima::PS::PostScript';
	$self-> {defaultData} = {};
	$self-> {data} = {};
	$self-> $_($profile{$_}) for qw(defaultData resFile);
	$self-> {printers} = {};
	$self-> {$_} = $profile{$_} for qw(gui fileExtension formatDescription );

	my $pr = $profile{printer} if defined $profile{printer};

	if ( open F, $self-> {resFile}) {
		local $/;
		my $fc = '$fc = ' . <F>;
		close F;
		eval "$fc";
		$self-> {printers} = $fc if !$@ && defined($fc) && ref($fc) eq 'HASH';
		for my $v ( values %{ $self->{printers} } ) {
			if ( $v->{spoolerType} =~ /^\d+$/ ) { # old format
				$v->{spoolerType} = $Prima::PS::Printer::Common::spooler_types[ $v->{spoolerType} ];
			}
		}
	}

	unless ( scalar keys %{$self-> {printers}}) {
		$self-> {printers}-> {'Default printer'} = deepcopy( $self-> {defaultData});
		if ( $unix) {
			$self-> import_printers( 'printers', '/etc/printcap');
			$self-> {printers}-> {GhostView} = deepcopy( $self-> {defaultData});
			$self-> {printers}-> {GhostView}-> {spoolerType} = 'cmd';
			$self-> {printers}-> {GhostView}-> {spoolerData} = 'gv $';
		}
		$self-> {printers}-> {'Save as PostScript'} = deepcopy( $self-> {defaultData});
		$self-> {printers}-> {'Save as PostScript'}-> {spoolerType} = 'file';
	}

	if ( exists $self->{printers}->{File} && ! exists $self->{printers}->{'Save as PostScript'}) {
		$self-> {printers}-> {'Save as PostScript'} = delete $self->{printers}->{File};
	}

	unless ( defined $pr) {
		if ( defined  $self-> {printers}-> {'Default printer'}) {
			$pr = 'Default printer';
		} else {
			my @k = keys %{$self-> {printers}};
			$pr = $k[0];
		}
	}

	$self-> printer( $pr);
	$self-> $_( $profile{userData}->{$_} ) for
		grep { defined $profile{userData}->{$_} }
		qw(reversed);
	$self-> resolution( @{$profile{userData}->{resolution}} )
		if defined $profile{userData}->{resolution};
	$self-> scale( ($profile{userData}->{scaling}) x 2)
		if defined $profile{userData}->{scaling};
	return %profile;
}

sub import_printers
{
	my ( $self, $slot, $file) = @_;
	return undef unless open PRINTERS, $file;
	my $np;
	my @names;
	while ( <PRINTERS>) {
		chomp;
		if ( $np) {
			$np = 0 unless /\\\\s*$/;
		} else {
			next if /^\#/ || /^\s*$/;
			push( @names, $1) if m/^([^\|\:]+)/;
			$np = 1 if /\\\s*$/;
		}
	}
	close PRINTERS;

	my @ret;
	for ( @names) {
		s/^\s*//g;
		s/\s*$//g;
		next unless length;
		my $n = "Printer '$_'";
		my $j = 0;
		while ( exists $self-> {$slot}-> {$n}) {
			$n = "Printer '$_' #$j";
			$j++;
		}
		$self-> {$slot}-> {$n} = deepcopy( $self-> {defaultData});
		$self-> {$slot}-> {$n}-> {spoolerType} = 'lpr';
		$self-> {$slot}-> {$n}-> {spoolerData} = "-d $_";
		push @ret, $n;
	}
	return @ret;
}

sub printers
{
	my $self = $_[0];

	my @res;
	for ( sort keys %{$self-> {printers}}) {
		my $d = $self-> {printers}-> {$_};
		push @res, {
			name    => $_,
			device  =>
			(( $d-> {spoolerType} eq 'lpr' ) ? "lp $d->{spoolerData}"  :
			(( $d-> {spoolerType} eq 'file') ? 'file' : $d-> {spoolerData})),
			defaultPrinter =>  ( $self-> {current} eq $_) ? 1 : 0,
		},
	}
	return \@res;
}

sub resFile
{
	return $_[0]-> {resFile} unless $#_;
	my ( $self, $fn) = @_;
	if ( $fn =~ /^\s*~/) {
		my $home = defined( $ENV{HOME}) ? $ENV{HOME} : '/';
		$fn =~ s/^\s*~/$home/;
	}
	$self-> {resFile} = $fn;
}

sub resolution
{
	return $_[0]-> SUPER::resolution unless $#_;
	$_[0]-> raise_ro("resolution") if @_ != 3; # pass inherited call
	shift-> SUPER::resolution( @_);
}

sub printer
{
	return $_[0]-> {current} unless $#_;
	my ( $self, $printer) = @_;
	return undef unless exists $self-> {printers}-> {$printer};
	$self-> end_paint_info if $self-> get_paint_state == mb::Information;
	$self-> end_paint if $self-> get_paint_state;
	$self-> {current} = $printer;
	$self-> {data} = {};
	$self-> data( $self-> {defaultData});
	$self-> data( $self-> {printers}-> {$printer});
	$self-> translate( 0, 0);
	return 1;
}

sub data
{
	return $_[0]-> {data} unless $#_;
	my ( $self, $dd) = @_;
	my $dv = $dd-> {devParms};
	my $p = $self-> {data};
	for ( keys %$dd) {
		next if $_ eq 'devParms';
		$p-> {$_} = $dd-> {$_};
	}
	if ( defined $dv) {
		for ( keys %$dv) {
			$p-> {devParms}-> {$_} = $dv-> {$_};
		}
	}

	$self-> SUPER::data( $dd );
	$self-> isEPS( $p-> {isEPS}) if exists $dd-> {isEPS};

	if ( defined $dv) {
		my %dp = %{$p-> {devParms}};
		for ( keys %dp) {
			delete $dp{$_} unless length $dp{$_};
		}
		for ( qw( LeadingEdge InsertSheet ManualFeed
			DeferredMediaSelection TraySwitch MatchAll)) {
			delete $dp{$_} unless $dp{$_};
		}
		$dp{LeadingEdge}-- if exists $dp{LeadingEdge};
		for ( qw( MediaType MediaColor MediaWeight MediaClass)) {
			next unless exists $dp{$_};
			$dp{$_} =~ s/(\\|\(|\))/\\$1/g;
			$dp{$_} = '(' . $dp{$_} . ')';
		}
		for ( qw( InsertSheet ManualFeed TraySwitch
			DeferredMediaSelection MatchAll)) {
			next unless exists $dp{$_};
			$dp{$_} = $dp{$_} ? 'true' : 'false';
		}
		$self-> pageDevice( \%dp);
	}
}

sub defaultData
{
	return $_[0]-> {defaultData} unless $#_;
	my ( $self, $dd) = @_;
	my $dv = $dd-> {devParms};
	delete $dd-> {devParms};
	$self-> SUPER::defaultData($dd);
	if ( defined $dv) {
		for ( keys %$dv) {
			$self-> {defaultData}-> {devParms}-> {$_} = $dv-> {$_};
		}
	}
}

sub deepcopy
{
	my %h = %{$_[0]};
	$h{devParms} = {%{$h{devParms}}} if exists $h{devParms};
	return \%h;
}

sub setup_dialog
{
	return unless $_[0]-> {gui};
	$_[0]-> sdlg_exec;
}

sub options
{
	my $self = shift;

	if ( 0 == @_) {
		return
			$self-> SUPER::options,
			qw(EPS),
			keys %{$self->{data}->{devParms}};
	} elsif ( 1 == @_) {
		# get value
		my $v = shift;
		my $d = $self-> {data};
		return $d->{devParms}->{$v} if exists $d->{devParms}->{$v};

		if ( $v eq 'EPS') {
			return $d->{isEPS} ? '1' : '0'
		} else {
			return $self-> SUPER::options($v);
		}

	} else {
		my $successfully_set = 0;
		my (%newdata, %superdata);
		while ( @_ ) {
			# set value
			my ( $opt, $val) = ( shift, shift);
			my $d = $self-> {data};
			if ( exists $d->{devParms}->{$opt}) {
				$newdata{devParms}->{$opt} = $val;
			} else {
				$superdata{$opt} = $val;
			}
			$successfully_set++;
		}
		$self-> SUPER::data( \%superdata);
		$self-> SUPER::data( \%newdata);
		return $successfully_set;
	}
}

package Prima::PS::Printer::NonGUI;
use base qw(Prima::PS::Printer);

sub profile_default
{
	my $def  = $_[0]-> SUPER::profile_default;
	my %def2 = $_[0]-> profile_default2;
	delete $def2{spoolerType};
	my %prf = (
		gui  => 0,
		%def2,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my %def2    = $self-> profile_default2;
	$self-> {data}-> {spoolerType} = delete $def2{spoolerType};
	while ( my ( $k, $v ) = each %def2) {
		$self-> $k( $profile{$k} );
	}
	return %profile;
}

package Prima::PS::File::Common;

sub profile_default2 { ( file => 'out.ps', spoolerType => 'file' ) }

sub file
{
	return $_[0]-> {data}-> {spoolerData} unless $#_;
	$_[0]-> {data}-> {spoolerData} = $_[1];
}

package Prima::PS::File;
use base qw(Prima::PS::Printer::NonGUI Prima::PS::File::Common);

package Prima::PS::FileHandle::Common;

sub profile_default2 { ( handle => undef, spoolerType => 'fh' ) }

sub handle
{
	return $_[0]-> {spoolHandle} unless $#_;
	$_[0]-> {spoolHandle} = $_[1];
	binmode $_[1] if $_[1];
}

package Prima::PS::FileHandle;
use base qw(Prima::PS::Printer::NonGUI Prima::PS::FileHandle::Common);

package Prima::PS::LPR;
use base q(Prima::PS::Printer);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		gui  => 0,
		args => '',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {data}-> {spoolerType} = 'lpr';
	$self-> {data}-> {spoolerData} = $profile{args};
	return %profile;
}

sub args
{
	return $_[0]-> {data}-> {spoolerData} unless $#_;
	$_[0]-> {data}-> {spoolerData} = $_[1];
}

package Prima::PS::Pipe;
use base qw(Prima::PS::Printer);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		gui  => 0,
		command => '',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {data}-> {spoolerType} = 'cmd';
	$self-> {data}-> {spoolerData} = $profile{command};
	return %profile;
}

sub command
{
	return $_[0]-> {data}-> {spoolerData} unless $#_;
	$_[0]-> {data}-> {spoolerData} = $_[1];
}


package Prima::PS::PDF::Printer;
use base qw(Prima::PS::PDF Prima::PS::Printer::Common);

Prima::PS::Printer::Common::install_api(__PACKAGE__);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		gui               => 1,
		fileExtension     => 'pdf',
		formatDescription => 'Portable Document Format',
		defaultData       => {
			color          => 1,
			resolution     => 300,
			page           => 'A4',
			scaling        => 1,
			portrait       => 1,
			spoolerType    => 'file',
			spoolerData    => '',
		},
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {backend} = 'Prima::PS::PDF';
	$self-> {defaultData} = {};
	$self-> {data}        = {%{ $profile{defaultData}}, spoolerType => 'file'};
	$self-> {$_} = $profile{$_} for qw(gui fileExtension formatDescription);
	$self-> defaultData( $profile{defaultData} );
	$self-> data( $profile{data} );
	return %profile 
}

sub resolution
{
	return $_[0]-> SUPER::resolution unless $#_;
	$_[0]-> raise_ro("resolution") if @_ != 3; # pass inherited call
	shift-> SUPER::resolution( @_);
}

sub printer { $#_ ? 0 : __PACKAGE__ }

sub setup_dialog
{
	return unless $_[0]-> {gui};
	$_[0]-> sdlg_exec(1);
}

package Prima::PS::PDF::NonGUI;
use base qw(Prima::PS::PDF::Printer);

sub profile_default
{
	my $def  = $_[0]-> SUPER::profile_default;
	my %def2 = $_[0]-> profile_default2;
	delete $def2{spoolerType};
	my %prf = (
		gui  => 0,
		%def2,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my %def2    = $self-> profile_default2;
	$self-> {data}-> {spoolerType} = delete $def2{spoolerType};
	while ( my ( $k, $v ) = each %def2) {
		$self-> $k( $profile{$k} );
	}

	return %profile;
}

package Prima::PS::PDF::File;
use base qw(Prima::PS::PDF::NonGUI Prima::PS::File::Common);

sub profile_default2 { ( file => 'out.pdf', spoolerType => 'file' ) }

package Prima::PS::PDF::FileHandle;
use base qw(Prima::PS::PDF::NonGUI Prima::PS::FileHandle::Common);

1;

=pod

=head1 Printer options

Below is the list of options supported by C<options> method:

=over

=item Color STRING

One of : C<Color, Monochrome>

=item Resolution INTEGER

Dots per inch.

=item PageSize STRING

One of: C<AI<integer>, BI<integer>, Executive, Folio, Ledger, Legal, Letter, Tabloid,
US Common #10 Envelope>.

=item Copies INTEGER

(not applicable to PDF)

=item Scaling FLOAT

1 is 100%, 1.5 is 150%, etc.

=item Orientation

One of : C<Portrait>, C<Landscape>.

=item MediaType STRING

An arbitrary string representing special attributes of the medium other
than its size, color, and weight. This parameter can be used to identify special
media such as envelopes, letterheads, or preprinted forms.

(not applicable to PDF)

=item MediaColor STRING

A string identifying the color of the medium.

(not applicable to PDF)

=item MediaWeight FLOAT

The weight of the medium in grams per square meter. "Basis weight" or
or null "ream weight" in pounds can be converted to grams per square meter by
multiplying by 3.76; for example, 10-pound paper is approximately 37.6
grams per square meter.

(not applicable to PDF)

=item MediaClass STRING

(Level 3) An arbitrary string representing attributes of the medium
that may require special action by the output device, such as the selection
of a color rendering dictionary. Devices should use the value of this
parameter to trigger such media-related actions, reserving the MediaType
parameter (above) for generic attributes requiring no device-specific action.
The MediaClass entry in the output device dictionary defines the allowable
values for this parameter on a given device; attempting to set it to an unsupported
value will cause a configuration error.

(not applicable to PDF)

=item InsertSheet BOOLEAN

(Level 3) A flag specifying whether to insert a sheet of some special
medium directly into the output document. Media coming from a source
for which this attribute is Yes are sent directly to the output bin without
passing through the device's usual imaging mechanism (such as the fuser
assembly on a laser printer). Consequently, nothing painted on the current
page is actually imaged on the inserted medium.

(not applicable to PDF)

=item LeadingEdge BOOLEAN

(Level 3) A value specifying the edge of the input medium that will
enter the printing engine or imager first and across which data will be imaged.
Values reflect positions relative to a canonical page in portrait orientation
(width smaller than height). When duplex printing is enabled, the canonical
page orientation refers only to the front (recto) side of the medium.

(not applicable to PDF)

=item ManualFeed BOOLEAN

Flag indicating whether the input medium is to be fed manually by a human
operator (Yes) or automatically (No). A Yes value asserts that the
human operator will manually feed media conforming to the specified attributes
( MediaColor, MediaWeight, MediaType, MediaClass, and InsertSheet). Thus, those
attributes are not used to select from available media sources in the normal way,
although their values may be presented to the human operator as an aid in selecting
the correct medium. On devices that offer more than one manual feeding mechanism,
the attributes may select among them.

(not applicable to PDF)

=item TraySwitch BOOLEAN

(Level 3)  A flag specifying whether the output device supports
automatic switching of media sources. When the originally selected source
runs out of medium, some devices with multiple media sources can switch
automatically, without human intervention, to an alternate source with the
same attributes (such as PageSize and MediaColor) as the original.

(not applicable to PDF)

=item MediaPosition STRING

(Level 3) The position number of the media source to be used.
This parameter does not override the normal media selection process
described in the text, but if specified it will be honored - provided it can
satisfy the input media request in a manner consistent with normal media
selection - even if the media source it specifies is not the best available
match for the requested attributes.

(not applicable to PDF)

=item DeferredMediaSelection BOOLEAN

(Level 3) A flag determining when to perform media selection.
If Yes, media will be selected by an independent printing subsystem associated
with the output device itself.

(not applicable to PDF)

=item MatchAll BOOLEAN

A flag specifying whether input media request should match to all
non-null values - MediaColor, MediaWeight etc.

(not applicable to PDF)

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Printer>, L<Prima::Drawable>, L<Prima::PS::PDF>

