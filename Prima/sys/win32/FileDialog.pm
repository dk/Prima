use Prima;
use strict;
use warnings;

package Prima::sys::win32::FileDialog;
use vars qw(@ISA);
@ISA = qw(Prima::Component);
use Prima::Utils;
use Encode;

return 1 if Prima::Application-> get_system_info->{apc} != apc::Win32;

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},

		defaultExt  => '',
		fileName    => '',
		filter      => [[ 'All files' => '*.*']],
		filterIndex => 0,
		directory   => '.',

		createPrompt       => 0,
		multiSelect        => 0,
		noReadOnly         => 0,
		noTestFileCreate   => 0,
		overwritePrompt    => 1,
		pathMustExist      => 1,
		fileMustExist      => 1,
		showHelp           => 0,

		openMode           => 1,
		text               => undef,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {flags} = {
		HIDEREADONLY => 1,
		EXPLORER => 1,
	};
	for ( qw( filterIndex openMode)) { $self->{$_}=$profile{$_} }
	for ( qw( defaultExt filter directory multiSelect
		createPrompt fileMustExist noReadOnly noTestFileCreate
		overwritePrompt pathMustExist showHelp
	)) { $self->$_($profile{$_}) }
	return %profile;
}

sub quoted_split
{
	my @ret;
	$_ = $_[0];
	s/(\\[^\\\s])/\\$1/g;
	study;
	{
		/\G\s+/gc && redo;
		/\G((?:[^\\\s]|\\.)+)\s*/gc && do {
			my $z = $1;
			$z =~ s/\\(.)/$1/g;
			push(@ret, $z);
			redo;
		};
		/\G(\\)$/gc && do { push(@ret, $1); redo; };
	}
	return @ret;
}

sub filter
{
	if ( $#_) {
		my $self   = $_[0];
		my @filter = @{$_[1]};
		@filter = [[ '' => '*']] unless scalar @filter;
		my @exts;
		my @mdts;
		for ( @filter) {
			push @exts, $$_[0];
			push @mdts, $$_[1];
		}
		$self-> {filterIndex} = scalar @exts - 1
			if $self-> { filterIndex} >= scalar @exts;
		$self-> {filter} = \@filter;
	} else {
		return @{$_[0]-> {filter}};
	}
}

sub filterIndex
{
	if ( $#_) {
		$_[0]-> {filterIndex} = $_[1];
	} else {
		return $_[0]-> {filterIndex};
	}
}

sub directory
{
	return $_[0]->{directory} unless $#_;
	$_[0]->{directory} = $_[1];
}

sub createPrompt
{
	return $_[0]->{flags}->{CREATEPROMPT} unless $#_;
	$_[0]->{flags}->{CREATEPROMPT} = $_[1];
}

sub multiSelect
{
	return $_[0]->{flags}->{ALLOWMULTISELECT} unless $#_;
	$_[0]->{flags}->{ALLOWMULTISELECT} = $_[1];
}

sub noReadOnly
{
	return $_[0]->{flags}->{NOREADONLYRETURN} unless $#_;
	$_[0]->{flags}->{NOREADONLYRETURN} = $_[1];
}

sub noTestFileCreate
{
	return $_[0]->{flags}->{NOTESTFILECREATE} unless $#_;
	$_[0]->{flags}->{NOTESTFILECREATE} = $_[1];
}

sub overwritePrompt
{
	return $_[0]->{flags}->{OVERWRITEPROMPT} unless $#_;
	$_[0]->{flags}->{OVERWRITEPROMPT} = $_[1];
}

sub pathMustExist
{
	return $_[0]->{flags}->{PATHMUSTEXIST} unless $#_;
	$_[0]->{flags}->{PATHMUSTEXIST} = $_[1];
}

sub fileMustExist
{
	return $_[0]->{flags}->{FILEMUSTEXIST} unless $#_;
	$_[0]->{flags}->{FILEMUSTEXIST} = $_[1];
}

sub showHelp
{
	return $_[0]->{flags}->{SHOWHELP} unless $#_;
	$_[0]->{flags}->{SHOWHELP} = $_[1];
}

sub fileName
{
	unless ( $#_) {
		return $_[0]->{fileName} unless $_[0]->multiSelect;
		my @s = quoted_split( $_[0]-> {fileName});
		return $s[0] unless wantarray;
		return @s;
	}
	$_[0]->{fileName} = $_[1];
}

sub defaultExt
{
	return $_[0]->{defaultExt} unless $#_;
	$_[0]->{defaultExt} = $_[1];
}

sub openMode
{
	return $_[0]->{openMode} unless $#_;
	$_[0]->{openMode} = $_[1];
}

sub text
{
	return $_[0]->{text} unless $#_;
	$_[0]->{text} = $_[1];
}

# dummies
sub sorted { 1 }
sub showDotFiles { 1 }

# mere callbacks if someone wants these to inherit
sub ok {}
sub cancel {}

sub _set
{
	my @cmd = @_;
	for my $c ( @cmd ) {
		unless ( Encode::is_utf8($c)) {
			my $v = Prima::Utils::local2sv($c);
			$c = $v if defined $v;
		}
		$c = Encode::encode('utf-8', $c);
	}
	my $cmd = shift @cmd;
	Prima::Application-> sys_action( "win32.OpenFile.$cmd=".join('', @cmd));
}

sub _get
{
	my $cmd = shift;
	$cmd = Prima::Application-> sys_action( "win32.OpenFile.$cmd");
	return Encode::decode('utf-8', $cmd);
}

sub execute
{
	my $self = $_[0];

	_set( flags       => join(',', grep { $self->{flags}->{$_}} keys %{$self->{flags}}));
	_set( filters     => join("\0", map { "$$_[0] ($$_[1])\0$$_[1]" } @{$self->{filter}}) . "\0");
	_set( filterindex => ($self->{filterIndex}+1));
	my $dir = $self->{directory};
	$dir =~ s/\//\\/g;

	$dir = "c:/1/1/\x{76ee}/\x{5f55}";

	_set( directory   => $dir);
	_set( defext      => $self->{defaultExt});
	_set( title       => $self->{text} // 'NULL');
	my $ret = _get($self->{openMode} ? 'open' : 'save');
	if ( !defined $ret) {
		$self-> cancel;
		return wantarray ? () : undef;
	}
	$self-> {directory} = _get('directory');
	$self-> {directory} =~ s/\\/\//g;
	$self-> {directory} =~ s/\s+$//;
	$self-> {directory} .= '/' unless $self-> {directory} =~ /\/$/;
	$self-> {fileName} = $ret;
	if ( $self-> multiSelect) {
		$self-> {fileName} = join( ' ', map {
			s/\\/\//g;
			$_ = $self->{directory} . $_ unless m/^\w\:/; # win32 absolute path, if any
			s/([\\\s])/\\$1/g;
			$_;
		} quoted_split($self-> {fileName}));
	} else {
		$self-> {fileName} =~ s/\\/\//g;
	}
	$self-> {filterIndex} = _get('filterindex')-1;
	$self-> ok;
	return $self-> fileName;
}

package Prima::sys::win32::OpenDialog;
use vars qw(@ISA);
@ISA = qw(Prima::sys::win32::FileDialog);

package Prima::sys::win32::SaveDialog;
use vars qw(@ISA);
@ISA = qw(Prima::sys::win32::FileDialog);

sub profile_default
{
	return { %{$_[ 0]-> SUPER::profile_default},
		openMode        => 0,
		fileMustExist   => 0,
	}
}

1;

=head1 NAME

Prima::sys::win32::FileDialog - Windows file system dialogs.

=head1 DESCRIPTION

The module mimics Prima file dialog classes C<Prima::Dialog::OpenDialog>
and C<Prima::Dialog::SaveDialog>, defined in L<Prima::Dialog::FileDialog>. The
class names registered in the module are the same, but in C<Prima::sys::win32>
namespace.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Dialog::FileDialog>

=cut


