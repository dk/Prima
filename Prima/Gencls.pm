#  Created by:
#     Anton Berezin  <tobez@tobez.org>
#     Dmitry Karasik <dk@plab.ku.dk>
#     Vadim Belman   <voland@plab.ku.dk>

package Prima::Gencls;
use strict;
use warnings;
use Exporter;
use vars qw( @ISA @EXPORT);
use vars qw(
	$ln
	@lineNums
	@words
	$tok
	$className
	$superClass
	$fileName
	%fileCache
	$hvUsed
	$retType
	$mtVar
	@thunks
	$arg
	$option
);

@ISA = qw( Exporter);
@EXPORT = qw( gencls);

use constant TYPES => 0;
use constant IDS   => 1;
use constant PROPS => 2;
use constant DEFS  => 3;

my ( $warnings,
	$clsMethod,
	$clsNotify,
	$clsImport,
	$clsPublic,
	$clsStatic,
	$clsWeird,
	$clsC_only,
	$clsProperty,
	$clsLocalType,
	$clsGlobalType,
	$hSelf,
	$hSuper,
	$hBase,
	$hMate,
	$hClassName,
	$hSize,
	$incSelf,
	$incCount,
	$incRes,
	$incHV,
	$incRet,
	$incClass,
	$incSV,
	$incGetMate,
	$incGetVmt,
	$incBuild,
	$incInst,
	$tmlPrefix,
	$dirPrefix,
	$dirOut,
	$suppress,
	$sayparent,
	$optimize,
	$genH,
	$genInc,
	$genTml,
	$genDyna,
	$genObject,
	$genPackage,
	@includeDirs,
	%definedClasses,
	@allVars,
	%allVars,
	@allMethods,
	@allMethodsBodies,
	@allMethodsDefaults,
	%allMethods,
	%allMethodsHosts,
	@portableMethods,
	@newMethods,
	@portableImports,
	%properties,
	%publicMethods,
	%staticMethods,
	%pipeMethods,
	$level,
	$ownClass,
	$ownOClass,
	$ownFile,
	$ownCType,
	$baseClass,
	$baseFile,
	$baseCType,
	$acc,
	$proptype,
	$propextras,
	$id,
	$piped,
	$inherit,
	@defParms,
	@context,
	%templates_xs,
	%templates_imp,
	%templates_rdf,
	%mapTypes,
	%typedefs,
	%xsConv,
	%mapPointers,
	$nextStructure,
	%structs,
	%arrays,
	%defines,
	%forbiddenParms,
	@ancestors,
	);

sub init_variables
{
	#defines
	$warnings = 1;

	#constantza
	$clsMethod  = 'method';
	$clsNotify  = 'event';
	$clsImport  = 'import';
	$clsPublic  = 'public';
	$clsStatic  = 'static';
	$clsWeird   = 'weird';
	$clsC_only  = 'c_only';
	$clsProperty = 'property';
	$clsLocalType  = 'local';
	$clsGlobalType = 'global';
	$hSelf      = "self";
	$hSuper     = "super";
	$hBase      = "base";
	$hMate      = "mate";
	$hClassName = "className";
	$hSize      = "instanceSize";
	$incSelf    = "_apt_self_";
	$incCount   = "_apt_count_";
	$incRes     = "_apt_res_";
	$incHV      = "_apt_hv_";
	$incRet     = "_apt_toReturn";
	$incClass   = "_apt_class_name_";
	$incSV      = "temporary_prf_Sv";
	$incGetMate = "gimme_the_mate";
	$incGetVmt  = "gimme_the_vmt";
	$incBuild   = "create_some_";
	$incInst    = "PAnyObject";
	$tmlPrefix  = "template_";

	#variablez
	$dirOut = "";		# output directory
	$suppress = 0;		# flag to suppress file output but produce inheritance
	$sayparent = 0;		# say only parent's name
	$optimize  = 0;		# use optimized inc-file generation model
	$genH      = 0;		# generate h-file
	$genInc    = 0;		# generate inc-file
	$genTml    = 0;		# generate tml-file
	$genDyna   = 0;		# generate exportable files
	$genObject = 0;		# generate object files
	$genPackage = 0;		# generate object or package files
	@includeDirs = qw(.);	# where to search .clses
	%definedClasses = ();	# hash for class search
	@allVars = ();		# all variables, in order of definition
	%allVars = ();		# hash for var search
	@allMethods = ();	        # methods names
	@allMethodsBodies = ();	# methods bodies, as is
	@allMethodsDefaults = ();   # methods default parameters
	%allMethods = ();	        # hash, containing indices of methods
	%allMethodsHosts = ();	# hash, bonded methods => hosts
	@portableMethods = ();	# c & perl portable methods
	@newMethods = ();        	# portable methods that declared first time
	@portableImports = ();	# perl imported calls
	%publicMethods = ();	# public methods hash
	%staticMethods = ();	# static methods hash
	%pipeMethods = ();	        # pipe methods hash
	$level = 0;	          	# file entrance level
	@context = ();		# local context for storing words etc.
	%templates_xs  = ();	# storage for computed template xs names
	%templates_imp = ();	# storage for computed template import names
	%templates_rdf = ();	# storage for computed template redefined names
	@ancestors = ();            # list of object ancestors
	%properties = ();           # hash of properties


	%mapTypes = (
		( long => 'int', U8 => 'int', char => 'int' ),
		map {( $_, $_ )} qw(int Bool Handle UV Color double SV HV),
	);

	%typedefs = ();

	%xsConv = (
		# declare   access      set          unused   unused      extra2sv POPx     newXXxx extrasv2
		# 0         1            2           3        4           5        6        7       8
		'int'     => ['int',    'SvIV',      'sv_setiv',  '',      '(IV)',     '',    'POPi',    'SViv', ''     ],
		'UV'      => ['UV',     'SvUV',      'sv_setuv',  '',      '(UV)',     '',    'POPu',    'SVuv', ''     ],
		'Color'   => ['Color',  'SvUV',      'sv_setuv',  '',      '(Color)',  '',    'POPu',    'SVuv', ''     ],
		'double'  => ['double', 'SvNV',      'sv_setnv',  '',      '(double)', '',    'POPn',    'SVnv', ''     ],
		'char*'   => ['char *', 'SvPV_nolen','sv_setpv',  '(SV*)', '',         ', 0', 'POPp',    'SVpv', '' ],
		'string'  => ['char',   'SvPV_nolen','sv_setpv',  '(SV*)', '',         ', 0', 'POPp',    'SVpv', '' ],
		'Handle'  => ['Handle', $incGetMate, '',          '',      '',         '',    '0/0',     '',     ''     ],
		'SV*'     => ['SV *',   '',          '',          '',      '',         '',    '0/0',     '',     ''     ],
		'Bool'    => ['Bool',   'SvBOOL',    'sv_setiv',  '0/0',   '0/0',      '',    'SvBOOL( POPs)', 'SViv', ''     ],
	);

	%mapPointers = ( "char" => "char*", "SV" => "SV*", "HV"=> "HV*");

	$nextStructure = 2;
	%structs = ();
	%arrays = ();
	%defines = ();
	%forbiddenParms = (
		ax=>1, sp=>1, items=>1, dSP=>1, dMARK=>1, dXSARGS=>1, I32=>1, mark=>1,
		stack_base=>1, XSRETURN=>1, croak=>1, $incSelf=>1, $incRes=>1, $incCount=>1,
		free=>1, malloc=>1, ST=>1, XSRETURN_EMPTY=>1, sv_newmortal=>1,
		sv_setiv=>1, sv_setpv=>1, sv_setnv=>1, ENTER=>1, SAVETMPS=>1, PUSHMARK=>1,
		XPUSHs=>1, sv_2mortal=>1, newSvIV=>1, newSvNV=>1, newSVPV=>1, PUTBACK=>1,
		POPn=>1, POPi=>1, POPp=>1, SPAGAIN=>1, FREETMPS=>1, LEAVE=>1, PUSH=>1, $incClass=>1,
		$incGetVmt=>1, $hBase=>1, $incRet=>1, temporary_prf_Sv=>1,
		$incHV=>1, $incSV=> 1, $clsLocalType => 1, $clsGlobalType => 1,
		string=>1, object=>1, 'package'=>1, 'use'=>1, 'define'=> 1,
	);
}

sub parse_file
# loads & parses given file
{

local $ln;              # lines No.
local @lineNums;        # line numbers; uses only in error messages
local @words;           # file itself, broken by lexems (further lxmz)
local $tok;             # local token
local $className;       # local class name
local $superClass;      # owner name, '' if none
local $fileName;        # file name, = $className.cls

sub gettok
#return next lxm
{
	$ln = pop @lineNums;
	return pop @words;
}

sub putback
{
	push @words, $_[0];
	push @lineNums, $ln;
}

sub error
# shrieks & dies
{
	my $err = shift;
	die "$err at line $ln, $fileName\n";
}

sub expect
# expect given string
{
	my $tofind = shift;
	error "Found ``$tok'' but expecting ``$tofind''" unless ( $tok = gettok) eq $tofind;
}

sub check
# check whether $tok is equal to given parm
{
	my $tofind = shift;
	error "Found ``$tok'' but expecting ``$tofind''" unless $tok eq $tofind;
}

sub getid
# waits for indentifier
{
	$tok = gettok;
	error "Expecting identifier but found ``$tok''" unless $tok =~ /^\w+$/;
	return $tok;
}

sub getidsym
# waits for identifier on one of given symbols
{
#   my $what = shift;
	$tok = gettok;
	return $tok if index($_[0],$tok) >= 0;   # $what =~ /\Q$tok\E/;
	return $tok if $tok =~ /^\w+$/;
	error "Expecting identifier or one of ``$_[0]'' but found ``$tok''";
}

sub getObjName
{
	$tok = gettok;
	for ( split( '::', $tok))
	{
		error "Expecting object identifier but found ``$_''"
			unless $_ =~ /^\w+$/;
	}
	return $tok;
}

sub save_context
{
	push ( @context, [[@words], [@lineNums], $fileName]);
	@words = ();
	@lineNums = ();
}

sub restore_context
{
	my @c = @{pop @context};
	@words = @{$c[0]};
	@lineNums = @{$c[1]};
	$fileName = $c[2];
}

sub load_file
{
	# start of parsing
	$fileName = $_[0];
	if ( defined $fileCache{$fileName})
	{
		@words = @{$fileCache{$fileName}-> {words}};
		@lineNums = @{$fileCache{$fileName}-> {lineNums}};
		return;
	}

	open FILE, $fileName or die "Cannot open $fileName\n";

	# loading the file
	my @_words;
	my @_lineNums;
	$ln = 0;
	local $_;
	LINE: while (<FILE>)
	{
		$ln++;
		{
			if (/\G#/gc) {                                  # skip comments early
				next LINE;
			} elsif (/\G([\(\)\[\];,=>\@\%\${}\*])/gc) {    # single-char token
				push @_words, $1; push @_lineNums, $ln;
			} elsif (/\G\s+/gc) {
																			# do nothing - whitespace
			} elsif (/\G([-+\d\w.:]+)/gc) {                 # numbers and identifiers (id::id is ok)
				push @_words, $1; push @_lineNums, $ln;
			} elsif (/\G$/gc) {                             # end of line
				next LINE;
			} elsif (/\G("[^\"\\]*(?:\\.[^\"\\]*)*")/gc) {  # C style string
				push @_words, $1; push @_lineNums, $ln;
			} else {
				close FILE;
				error " Invalid character in input";
			}
			redo;
		}
	}

	close FILE;
	@words = reverse @_words;
	@lineNums = reverse @_lineNums;
	$fileCache{$fileName}-> {words} = [@words];
	$fileCache{$fileName}-> {lineNums} = [@lineNums];
}

sub skipto
{
	my $skip = shift;
	until ( undef)
	{
		$tok = gettok;
		last unless defined $tok;
		last if $tok eq $skip;
	}
	return $tok;
}

sub find_file
{
	my $module = shift;
	my @fullName = split('::', $module);
	my $fileName = $fullName[-1];
	$module =~ s/\.cls//;
	foreach (@includeDirs)
	{
		my $fit  = 0;
		my $dyna = 0;
		save_context;
		my %structSave = %structs;
		my %arraySave = %arrays;
		my %defineSave = %defines;
		my $nextStructureSave = $nextStructure;
		eval { load_file( "$_/$fileName") };
		if ( !$@ && skipto( 'object'))
		{
			$tok = gettok;
			if (( $tok eq "static") || ( $tok eq "dynamic"))
			{
				$dyna = 1 if $tok eq "dynamic";
				$tok = gettok;
			}
			$fit = $tok eq $module;
		}
		$nextStructure = $nextStructureSave;
		%structs = %structSave;
		%arrays = %arraySave;
		%defines = %defineSave;
		restore_context;
		next unless $fit;
		return "$_/$fileName", $dyna;
	}
	return undef;
}

sub checkid
{
	my $id = $_[0];
	error "Indentifier '$id' already defined or cannot be used; error"
	if $mapTypes{$id} || $structs{$id} || $arrays{$id} || $defines{$id} || $forbiddenParms{$id};
}

# structure parser
sub parse_struc
{
	my ( $strucId, $global) = @_;
	$id = getid;
	my $lineStart = $ln;
	my $incl = $fileName;
	$incl =~ s/([^.]*)\..*$/$1/;
	my $redef = exists $structs{$id} && defined ${$structs{$id}[PROPS]}{hash};
	checkid($id) unless
	exists $structs{$id} &&
		(
			(${$structs{$id}[PROPS]}{incl} eq $incl)  # already included in circular
			||
			$redef  # hash default values redefine
		);

	my @types = ();
	my @ids   = ();
	my @defs  = ();
	my %added = (
		genh  => $level    ? 0 : 1,
		incl  => $level    ? $incl : '',
		hash  => $strucId eq "%" ? 1 : undef,
		glo   => $global,
		name  => $id,
		order => $nextStructure++,
	);
	$tok = gettok;
	if ( $tok eq '=')
	{
		# determining typecasting
		my $nid = getid;
		error "Invalid casting type $id" unless ( $arrays{$nid} || $structs{$nid} || $defines{$nid});
		error "Invalid forecasting type $id" if $redef;
		expect(';');
		if ( $arrays{$nid}) {
			$arrays{ $id} = [ $arrays{$nid}[0], $arrays{$nid}[1], {%added}];
		} else {
			$structs{$id} = [[@{$structs{$nid}[0]}], [@{$structs{$nid}[1]}], {%added}, [@{$structs{$nid}[3]}]];
		}
		return;
	} else {
		if (( $strucId eq '@') && ( $mapTypes{ $tok})) {
			# ok, array found.
			my $type = $tok;
			if ( $type eq "SV") {
				expect ('*');
				$type .= '*';
			}
			expect('[');
			my $dim = gettok;
			error "Invalid array $id dimension '$dim'" if $dim <= 0;
			expect(']');
			expect(';');
			$arrays{ $id} = [ $dim, $type, {%added}];
			return;
		} else {
			check('{');
		}
	}

	# cycling entries
	until ( undef)
	{
		# field type;
		my $type = gettok;
		my $structure = $type;
		last if $type eq "}";
		if ( exists $structs{ $type}) {
			$structure = $structs{ $type};
			error "Cannot do nested arrays (yet)"
				unless $structure-> [PROPS]-> {hash};
			$type = 'SV*';
		} else {
			error "Invalid field type $type"
				if (!defined $mapTypes{$type} || $type eq 'HV') && ( $type ne 'string');
			$structure = $type = $mapTypes{$type} if $mapTypes{$type};
		}
		$tok = gettok;
		# checking pointer
		if ( $tok eq '*') {
			error "Invalid pointer to $type in structure $id"
				if ( $type ne 'char') && ( $type ne 'SV');
			$type .='*';
			$structure = $type;
			$tok = getid;
		} else {
			error "Invalid filed type $type in structure $id"
				if ( $type eq 'HV') or ( $type eq 'SV');
		}
		push ( @types, $structure);
		# field name;
		checkid( $tok);
		push ( @ids, $tok);
		if ( $strucId eq "%")
		{
			my ($def,$name);
			if
			( $type eq 'Handle') { $def = '( Handle) C_POINTER_UNDEF'; } elsif
			( $type eq 'SV*')    { $def = '( SV*) C_POINTER_UNDEF'; } elsif
			( $type eq 'int')    { $def = 'C_NUMERIC_UNDEF'; } elsif
			( $type eq 'Bool')   { $def = 'C_NUMERIC_UNDEF'; } elsif  # hack
			( $type eq 'double') { $def = 'C_NUMERIC_UNDEF'; } elsif
			( $type eq 'char*')  { $def = 'C_STRING_UNDEF'; } elsif
			( $type eq 'string') { $def = 'C_STRING_UNDEF'; };
			$tok = gettok;
			if ( $tok eq 'with' ) {
				while (1) {
					my $nt = gettok;
					if ( $nt eq 'undef') {
						$def = "undef:$def";
					} elsif ( $nt eq ';') {
						last;
					}
				}
			} elsif ( $tok eq '=') {
				$def = gettok;
				expect(';');
			} else {
				check(';');
			}
			push ( @defs, $def);
		} else {
			expect(';');
		}
	}
	error "Structure $id defined as empty" unless scalar @ids;

	if ( $redef) {
		# if it's hash default values redefine check for structure exact match
		my $saveLn = $ln;
		$ln = $lineStart;
		my @cmpType = (@{$structs{$id}[0]});
		my @cmpIds  = (@{$structs{$id}[1]});
		error "Structure $id is already defined. Error" unless @cmpType eq @types;
		for ( my $i = 0; $i < scalar @types; $i++) {
			error "Structure $id is already defined. Error"
			unless ( $cmpType[$i] eq $types[$i]) && ( $cmpIds[$i] eq $ids[$i]);
		}
		$ln = $lineStart;
		$added{genh} = ${$structs{$id}[2]}{genh};
	}
	$structs{ $id} = [[@types], [@ids], {%added}, [@defs]];
}

sub parse_typedef
{
	my ( $global) = @_;
	$id = getid;
	checkid( $id);
	expect('=');
	my $t = gettok;
	my $typecast = 0;
	my $type;
	if ( $t eq '>') {
		$typecast = 1;
		$type = getid;
	} else {
		checkid( $t);
		$type = $t;
	}
	error "Invalid casting type $type" unless ( $mapTypes{$type});
	expect(';');
	my $incl = $fileName;
	$incl =~ s/([^.]*)\..*$/$1/;
	my %added = (
		genh => $level    ? 0 : 1,
		incl => $level   ? $incl : '',
		cast => $typecast,
		glo  => $global,
	);
	$mapTypes{ $id}    = $type;
	$typedefs{ $id} = ({%added});
}

sub parse_define
{
	my ( $strucId, $global) = @_;
	$id = getid;
	checkid( $id);
	my $type = getid;
	my $incl = $fileName;
	$incl =~ s/([^.]*)\..*$/$1/;
	my %added = (
		genh =>  $level   ? 0 : 1,
		incl =>  $level   ? $incl : '',
		type =>  $type,
		glo  =>  $global,
	);
	$defines{ $id} = ({%added});
}


sub preload_method
# loads & parses common methods descriptions tables
{
	my ( $useHandle, $asProperty) = @_;
	# processing result & proc name
	$propextras = undef;
	if ( $asProperty) {
		my $lastTok = '';
		while ( 1) {
			$tok = getidsym( "*(;");
			last if $tok eq ';';
			$propextras = '', last if $tok eq '(';
			$id = $tok;
			$proptype = $lastTok;
			$lastTok .= "$tok ";
		}
		chop $proptype;
		$acc = "$proptype $id";
	} else {
		$acc = getid;
	}
	my $checkEmptyRes = '';
	unless ( $asProperty) {
		until (($tok = getidsym("*(")) eq "(")
		{
			$id = $tok;
			$acc .= " " . $tok;
			$checkEmptyRes .= $tok;
		}
		error "Function result haven't been defined" unless $checkEmptyRes;
		error "Unexpected (" unless $id;
	} else {
		error "Property cannot be 'void'" if $proptype eq 'void';
	}
	error "Method $id already defined"
		if exists($allMethodsHosts{ $id}) && $allMethodsHosts{ $id} eq $className;

	@defParms = ();

	$acc .= "( ";
	$acc .= "Handle $hSelf" if $useHandle;
	if ( $asProperty) {
		$acc .= ", " if $useHandle;
		$acc .= "Bool set";
		unless ( defined $propextras) {
			$acc .= ", $proptype value);";
			return; # simple property case
		}
	}

	#checking 'proc(void) & proc()' cases
	$tok = getidsym(")*...");
	$acc .= ', ' unless ($tok eq "void") || ($tok eq ")") || (! $useHandle);
	my $isVoid;
	if ($tok eq "void") {
		if ($words[-1] eq "*") {
			$acc .= ', ';
		} else {
			$isVoid = 1;
			expect(')');
			$tok = ')';
		}
	}

	my @types = ();

	# pre-parsing function parameters & result - laying own restrictions & other
	# , so $acc'll have all parameters divided by space
	if ( $tok ne ")")
	{
		putback ($tok);
		my $parmNo = 1;
		# cycling parameters
		my $hasEllipsis = 0;
		my $ellipsis;
		until( undef)
		{
			my @parm = ();
			$ellipsis = 0;
			until ( undef)
			{
				$tok = gettok;         # getting single parm description
				push ( @parm, $tok);
				last if ($tok eq ",") || ( $tok eq ")");
			}
			# parsing parameter description
			my @gp = @parm;

			if ( $asProperty && $parm[-1] eq ")") {
				my $l = $#parm - 1;
				$acc .= join( ' ', @parm[0..$l]) . ", $proptype value)";
			} else {
				$acc .= join(' ', @parm);
			}
			$acc =~ s/^([^=]*)(=.*)$/$1/; # removing default parms form $acc
			$acc .= $parm[-1] if $2;

			pop @gp;
			$tok = shift @gp;
			error "Invalid parameter $parmNo description" unless defined $tok;
			push ( @types, $tok); # first type description, definitive for
										# default parameter validity check
			if ( $tok eq '...') {
				$ellipsis  = 1;
				$hasEllipsis++;
				error "Property can not contain ellipsis into parameter list"
					if $asProperty;
			} else {
				error "Invalid parameter $parmNo description"
					unless defined $gp[0];
			}
			my $hasEqState = 0;
			my $defaultParm = undef;
			for ( @gp)            # cycling additive parameter description
			{
				# checking for type-name matching
				checkid($_);
				error "Indirect function and array parameters are unsupported. Error"
					if /^[\(\)\[\]\&]$/;
				error "Unexpected semicolon" if /^[\;]$/;
				error "End of default parameter description expected but found $_"
					if $hasEqState > 1;
				if ( $hasEqState == 1)
				{
					$defaultParm = $_;
					$hasEqState++;
				}
				# default parameter add
				if ( $_ eq "=") {
					error "Invalid default parameter $parmNo description"
						if $hasEqState;
					error "Property can not contain default parameters"
						if $asProperty;
					$hasEqState = 1;
				}
			}
			error "Default argument missing for parameter $parmNo"
				if !defined $defaultParm && defined $defParms[-1];
			push( @defParms, $defaultParm);
			last if ( $parm[-1] eq ")");
			$parmNo++;
		}
		error "Ellipsis (...) could be defined only in the end of parameters list"
			if $hasEllipsis > 1 || ( $hasEllipsis == 1 && $ellipsis == 0);
	} else {
		$acc .= 'void' if !$useHandle && $isVoid;
		$acc .= "$proptype value" if $asProperty;
		$acc .= $tok; # should be closing parenthesis
	};
	#  end of parameters description

	$piped = undef;
	$tok = gettok;
	if ( $tok eq "=") {
		$tok = gettok;
		error "Expected => but found $tok" unless $tok eq '>';
		$piped = getid;
		expect(';');
	} else {
		error "Expected semicolon or => but found $tok" unless $tok eq ';';
	}

	$acc .= ';';

	# checking default parameters
	for ( my $i = 0; $i <= $#types; $i++) {
		error "Parameter ${\($i+1)} of type $types[$i] cannot be used with default parameters"
		if defined $defParms[$i] && (
			$structs{$types[$i]} || $arrays{$types[$i]} ||
#        ( $types[$i] eq "Handle") || <<<< to enable being Handle default parameters
#			( $types[$i] eq "SV") ||
			( $types[$i] eq "HV")
		);
	}
}

sub cmp_func
{
	my ($xf,$xt)=@_;
	my ($fp, $tp);
	$xf =~ s/\((.*\));$//;  $fp = $1;
	$fp =~ s/\w+((?: [;)])|[;)])/$1/g;
	$fp =~ s/\s+/ /g;
	$xf .= $fp;
	$xt =~ s/\((.*\));$//;  $tp = $1;
	$tp =~ s/\w+((?: [;)])|[;)])/$1/g;
	$tp =~ s/\s+/ /g;
	$xt .= $tp;
	return $xf eq $xt;
}

sub store_method
{
	# comparing virtual overloaded methods bodies - die if unequal
	error "Header '$id' does not match previous definition"
		if !exists($staticMethods{ $id}) &&
			exists($allMethodsHosts{ $id}) && !($allMethodsHosts{ $id} eq $className) &&
			($allMethodsHosts{ $id}) && !( cmp_func($allMethodsBodies[$allMethods{$id}], $acc));

	# storing method, checking whether it was overloaded
	unless ( $allMethodsHosts{$id})
	{
		# first entrance
		push (@allMethodsBodies, $acc);
		push (@allMethodsDefaults, [@defParms]);
		push (@allMethods, $id);
		$allMethods{$id} = $#allMethodsBodies;
	} else {
		my $idx = $allMethods{$id};
		$allMethodsBodies[ $idx]   = $acc;
		$allMethodsDefaults[ $idx] = [@defParms];
		delete $pipeMethods{$id} unless $piped;
	}

	# binding with host
	$inherit = exists $allMethodsHosts{$id};
	$allMethodsHosts{$id} = $className;
	# setting "pipe to"
	$pipeMethods{$id} = $piped if $piped;
}

sub parse_parms
# parsin' method result type & parameters - returns if they
# could be passed to perl.
{
	my $db = $acc =~ /notify/;
	my @chunks;                    # parm or result
	$acc =~ s/\(/,/;               # separating ( to ,
	$acc =~ s/\)\;$//;             # eliminating ending );
	$acc =~ s/\,\s*$//;            # eliminating tailing commas, if any
	@chunks = grep { !/^\s*void$/ } split(",", $acc);
	$acc  = "";
	$hvUsed = 0;
	my $i = 0;
	foreach ( @chunks)
	{
		my @lxmz = split(" ", $_);
		my $valid = 0;
		foreach (@lxmz)
		{
			error "Reserved word '$_' used" if $forbiddenParms{$_};
			$hvUsed = 1 if $_ eq "HV";
		}
		for ( my $j = 0; $j <= $#lxmz; $j++)
		{
			my $keyType = $lxmz[ $j];
			my $orgType = $keyType;
			if (
				exists( $structs{ $keyType}) ||
				exists( $arrays{ $keyType}) ||
				( $keyType = $mapTypes{ $lxmz[ $j]})
			) {
				last if exists( $mapPointers{ $lxmz[ $j]}) && ( $lxmz[$j+1] ne "*");
				if ($lxmz[ $j+1] eq "*") {
					if ( exists( $mapPointers{ $lxmz[ $j]})) {
						if ( $lxmz[ $j+2] eq "*") {
							last
						} else {
							$orgType = $keyType = $mapPointers{ $lxmz[ $j]}
						}
					} else {
						if (
							exists( $structs{ $keyType}) ||
							exists( $arrays{ $keyType})
						) {
							$orgType = $keyType = "*".$lxmz[$j];
						} else {
							last
						};
					}
				}
				$acc .= " $orgType";
				$valid = 1;
				last;
			} elsif (( $lxmz[ $j] eq "void") && !$i) {
				# when return is void
				$acc .= " void";
				$valid = 1;
				last;
			}
		}
		$i++;
		unless ( $valid)
		{
			# if not valid in some chunk, exit immediately
			$acc = "";
			last;
		}
	}

	if ( $hvUsed) {
		my @lxmz = split(" ", $chunks[ $#chunks]);
		error "HV* can be declared only at the end of parameters list." unless $lxmz[ 0] eq "HV";
	}
	return $acc;
}

sub load_single_part
{
	$tok = shift;
	$genObject = $tok eq "object";
	$genPackage = 1;
	$className = getObjName;

	if ( $genObject && (( $className eq "static") || ( $className eq "dynamic"))) {
		$genDyna = ( $className eq "dynamic") unless $level;
		$className = getObjName;
	}

	unless ( $level) {
		$ownOClass = $ownClass = $className;
		$ownClass =~ s/^Prima:://;
		$ownFile  = (split( '::', $ownClass))[-1];
		$ownCType = $className;
		$ownCType =~ s/^Prima:://;
		$ownCType =~ s/::/__/g;
	}
	error "Class $className already defined" if $definedClasses{ $className};
	$definedClasses{ $className} = 1;
	$tok = gettok;
	$superClass = '';
	if ( $genObject) {
		if ( $tok eq '(') {
			$superClass = getObjName;
			my $superFile = (split( '::', $superClass))[-1];
			unless ( $level) {
				$baseClass = $superClass;
				$baseFile  = $superClass;
				$baseFile  =~ s/^Prima:://g;
				$baseFile  =~ s/::/\//g;
				$baseCType = $superClass;
				$baseCType  =~ s/^Prima:://g;
				$baseCType =~ s/::/__/g;
			}
			expect ")";
			$tok = gettok;
			error "Cyclic inheritance detected.  Watch out ``$superClass''"
				if $definedClasses{ $superClass};

			do {
				$level++;
				if ( $suppress & (!$sayparent || $level==1)) {
					my $superXClass = $superClass;
					$superXClass =~ s/^Prima:://;
					push @ancestors, $superXClass;
				}
				putback( $tok);
				my ( $superFile, $inhDyna) = find_file( "$superClass.cls");
				$tok = gettok;
				error "Cannot find file corresponding to ''$superClass''"
					unless defined $superFile;
				error "Static object cannot inherit dynamic module $superClass"
					if !$genDyna && $inhDyna;
				parse_file( $superFile) unless $sayparent;
				$level--;
			};
		} else {
			error "Cannot create dynamic object with no inheritance"
			if $genDyna && ( $level == 0);
		}
	}
	check "{";

	#parsing fields & methods
	until (( $tok = gettok) eq "}") {
		if ( $genObject) {
	# parsing object methods & vars
			if ($tok eq $clsMethod) {
				preload_method( 1, 0);
				store_method;
				unless ($level) {
					if ( parse_parms) {
						push (@portableMethods, "${id}${acc}");
						push (@newMethods, "${id}${acc}")
							unless $inherit;
					}  else {
						print "Warning: method $id has not been published\n"
							if $warnings;
					}
				};
			} elsif ($tok eq $clsNotify) {
				preload_method( 1, 0);
				store_method;
				unless ($level) {
					if ( parse_parms) {
						push (@newMethods, "${id}${acc}") unless $inherit;
					}  else {
						print "Warning: method $id has not been published\n"
							if $warnings;
					}
				};
			} elsif ($tok eq $clsImport) {
				preload_method( 1, 0);
				store_method;
				unless ($level) {
					error "Invalid types in import function $id declaration"
						unless parse_parms;
					push (@portableImports, "${id}${acc}");
					push (@newMethods, "${id}${acc}") unless $inherit;
				};
			} elsif ($tok eq $clsStatic) {
				preload_method( 0, 0);
				$staticMethods{$id} = 1;
				store_method;
				unless ($level) {
					if ( parse_parms) {
						push (@portableMethods, "${id}${acc}");
						push (@newMethods, "${id}${acc}") unless $inherit;
					} else {
						print "Warning: static $id has not been published\n"
							if $warnings;
					}
				};
			} elsif ($tok eq $clsPublic) {
				preload_method( 1, 0);
				$publicMethods{$id} = 1;
				store_method;
				unless ($level) {
					parse_parms;
					push (@portableMethods, "${id}${acc}");
					push (@newMethods, "${id}${acc}") unless $inherit;
				};
			} elsif ($tok eq $clsWeird) {
				preload_method( 0, 0);
				$staticMethods{$id} = 1;
				$publicMethods{$id} = 1;
				store_method;
				unless ($level) {
					parse_parms;
					push (@portableMethods, "${id}${acc}");
					push (@newMethods, "${id}${acc}") unless $inherit;
				};
			} elsif ($tok eq $clsC_only) {
				preload_method( 1, 0);
				store_method;
			} elsif ( $tok eq $clsProperty) {
				preload_method( 1, 1);
				store_method;
				unless ( $level) {
					if ( parse_parms) {
						push (@portableMethods, "$id$acc");
						push (@newMethods, "$id$acc") unless $inherit;
					} else {
						print "Warning: property $id has not been published\n"
							if $warnings;
					}
				}
				$properties{$id} = $proptype;
			} else {
			# variable workaround
				error "Expecting identifier but found ``$tok''"
					unless $tok =~ /^\w+$/;
				$acc = $tok;
				until (($tok = getidsym("*;[]()+")) eq ";")
				{
					$id = $tok;
					$acc .= " " . $tok;
				}
				$acc .= ";";
				error "Variable $id already defined" if $allVars{ $id};
				$allVars{$id} = $className;
				push(@allVars, $acc);
			}
			# end of parsing object
		} else {
			# parsing package
			putback( $tok);
			preload_method( 0, 0);
			$staticMethods{$id} = 1;
			store_method;
			unless ($level) {
				if ( parse_parms) {
					push (@portableMethods, "${id}${acc}");
					push (@newMethods, "${id}${acc}") unless $inherit;
				} else {
					print "Warning: function $id has not been published\n"
						if $warnings;
				}
			};
		}
	}
} # end of load_single_part

sub load_structures
{
	until ( undef) {
		$tok = gettok;
		unless ( defined $tok) {
			putback("");
			last;
		}

		if (( $tok eq $clsLocalType) || ( $tok eq $clsGlobalType)) {
			my $global = $tok eq $clsGlobalType;
			my $tok2 = $tok;
			$tok = gettok;
			if (( $tok eq '@') || ( $tok eq '%')) {
				parse_struc( $tok, $global);
				next;
			} elsif ( $tok eq '$') {
				parse_typedef( $global);
				next;
			} elsif ( $tok eq 'define') {
				parse_define( $global);
				next;
			}
			putback( $tok2);
			putback( $tok);
		} elsif ( $tok eq 'use') {
			my $fid = getid;
			expect(';');
			save_context;
			my $loaded;
			foreach (@includeDirs) {
				my $f = "$_/$fid.cls";
				next unless -f $f;
				$loaded = $f;
			}
			die "$fid.cls not found\n" unless defined $loaded;
			load_file( $loaded);
			$level++;
			&load_structures;
			$level--;
			restore_context;
		} else {
			putback ( $tok);
			last;
		}
	}
}

#start of parse_file
load_file( shift);
load_structures;
$tok = gettok;
if (( $tok eq "object") || ( $tok eq "package")) {
	load_single_part( $tok);
} elsif ( $tok) {
	error "found $tok but expecting 'object' or 'package'";
} else {
	unless ( $level) {
		#$genInc = 0;
		$ownFile = $fileName;
		$ownFile =~ s/([^.]*)\..*$/$1/;
		my $out = $ownFile;
		$out =~ s[.*\/][];
		$ownCType = uc $out;
	}
}
} # end of parse_file

sub type2sv
{
	my ( $type, $name) = @_;
	$type = $mapTypes{ $type} || $type;
	if ( ref $type) {
		return "sv_$type->[PROPS]->{name}2HV(&($name))";
	} elsif ( $type eq 'Handle') {
		return "( $name ? (( $incInst)$name)-> $hMate : &PL_sv_undef)";
	} elsif ( $type eq 'string') {
		my $fname = $name;
		$fname =~ s/(.*)\b(\w+)$/${1}is_utf8.$2/;
		return "prima_svpv_utf8($name, $fname)";
	} elsif ( $type eq 'SV*') {
		return $name;
	} else {
		return "new${xsConv{$type}[7]}( $name$xsConv{$type}[5])";
	}
}

sub sv2type
{
	my ( $type, $name) = @_;
	$type = $mapTypes{ $type} || $type;
	if ( $type eq 'Handle') {
		return "$incGetMate( $name);";
	} elsif ( $type eq 'SV*') {
		return $name;
	} else {
		return "$xsConv{$type}[1]( $name$xsConv{$type}[8])";
	}
}

sub sv2type_pop
{
	my ( $type) = @_;
	$type = $mapTypes{ $type} || $type;
	if ( $type eq 'Handle') {
		return "$incGetMate( POPs);";
	} elsif ( $type eq 'SV*') {
		return 'POPs';
	} else {
		return "( $xsConv{$type}[0]) $xsConv{$type}[6]";
	}
}


sub mortal
{
	my $type = $mapTypes{ $_[0]} || $_[0];
	if ( $type eq 'SV*') {
		return '';
	} elsif ( $type eq 'Handle') {
		return 'sv_mortalcopy';
	} else {
		return 'sv_2mortal';
	}
}

sub cwrite
{
	my ( $type, $from, $to) = @_;
	$type = $mapTypes{ $type} || $type;
	if ( $type eq 'string') {
		return "strncpy( $to, $from, 255); $to\[255\]=0;";
	} else {
		return "$to = $from;";
	}
}

sub out_method_profile
# common method print sub
{
	my ( $methods, $substring, $imports, $full) = @_;
	my $optRef = $imports ? \%templates_imp : \%templates_rdf;
	my %thunks = ();

	for ( my $i = 0; $i < @{$methods}; $i++) {
		my @parms = split( " ", ${$methods}[ $i]);
		my $id = shift @parms;
		my $useHandle = !exists( $staticMethods{ $id});
		my $tok = $allMethodsBodies[$allMethods{$id}];
		my $exStr = eval( "\"$substring\"");

		$tok =~ s/$id(?=\()/$exStr/;      # incorporating class name
		$tok =~ s/;([^;]*)$/$1/;          # adding ; to the end

		print( HEADER "$tok;\n\n"), next if $publicMethods{$id} && $full;
		my $castedResult = $parms[0];
		$castedResult = exists $typedefs{$castedResult} && $typedefs{$castedResult}{cast}
			? " ( $castedResult)" :
			'';
		my $subId = $full ? ( $imports ? "\"$ownOClass\:\:$id\"" : "\"$id\"") : 'subName';
		my $property  = defined $properties{ $id};

		my $result = shift @parms;
		my $eptr  = $result =~ /^\*/ ? "*" : "";
		$result =~ s[^\*][];
		my ( $lpr, $rpr) = $eptr ? ('(',')') : ('','');
		my $resSub = $mapTypes { $result} || $result;
		my $nParam = scalar @parms;
		my $hvName = $nParam ? $parms[ $#parms] eq "HV*" : 0;
		$tok =~ /\((.*)\)/;
		my @parm_ids  = $nParam ? split( ',', $1) : ();
		my $thunkParms;

		if ( !$full || $optimize) {
			my $j;
			$thunkParms = 'char* subName';
			for ( $j = 0; $j < scalar @parms; $j++) {
				$thunkParms .= ",$parm_ids[$j]";
			}
		}

		if ( $useHandle) {
			shift @parms;      # self-aware
			shift @parm_ids;   # construction
		}
		for ( @parm_ids) {
			/(\w+)\W*$/;
			$_ = $1;
		}

		if ( $full) {
			if ( exists $$optRef{$id} && !exists( $thunks{ $$optRef{$id}})) {
				$thunks{ $$optRef{$id}} = 1;
				print HEADER "extern $resSub$eptr $$optRef{$id}( $thunkParms);\n\n";
			}
			print HEADER "$tok\n";
		} else {
			# using pre-generated thunk name. Suggesting call is responsive to set
			# $methods to filtered array, that for sure contains that names.
			print HEADER "$resSub$eptr $$optRef{$id}( $thunkParms)\n";
		}
		print HEADER "{\n";

		if ( $full & $optimize && $$optRef{$id}) {
			my $ret   = ( $resSub eq 'void') ? '' : 'return ';
			unshift( @parm_ids, 'self') if $useHandle;
			my $parmz = join( ', ', @parm_ids);
			my $str  = $imports ? "\"$ownOClass\:\:$id\"" : "\"$id\"";
			print HEADER "\t$ret$$optRef{$id}( $str, $parmz);\n}\n\n";
			next;
		}

		print HEADER "\tdSP;\n";
		print HEADER "\tint $incCount;\n";
		unless ( $resSub eq "void") {
		if ( $resSub eq "char*") {
			print HEADER "\tSV *$incRes;\n\n";
		} else {
			print HEADER "\t$result $eptr $incRes;\n\n";
		}
		}
		print HEADER "\t(void)$incCount;\n";
		print HEADER "\tENTER;\n";
		print HEADER "\tSAVETMPS;\n";
		print HEADER "\tPUSHMARK( sp);\n\n";
		print HEADER "\tXPUSHs((( P${ownCType})$hSelf)-> $hMate);\n" if $useHandle;

		# writing other parameters
		for ( my $j = 0; $j < scalar @parm_ids; $j++) {
			$j = 1 if $j == 0 && $property;
			print HEADER " \n\tif ( !set) goto CALL_POINT;\n"
				if $property && $j == $#parm_ids;

			my $lVar = $parm_ids[ $j];
			my $type = $parms[ $j];
			my $ptr  = $type =~ /^\*/ ? "*" : "";
			$type =~ s[^\*][];

			my $hashStruc =
				exists($structs{ $type}) &&
				defined ${$structs{ $type}[2]}{hash};
			if ( $hashStruc) {
			# take down the reference in this case
				$ptr = ( $ptr eq '*') ? '' : '&';
			}
			my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
			$lVar = $lp.$ptr.$lVar.$rp;

			# structs
			if ( exists $structs{ $type}) {
				if ( $hashStruc) {
					print HEADER "\tXPUSHs( sv_2mortal( sv_${type}2HV( $lVar)));\n";
				} else {
					for ( my $k = 0; $k < scalar @{ $structs{ $type}[0]}; $k++) {
						my $lName = @{$structs{$type}[1]}[$k];
						my $lNameLen = length $lName;
						my $lType = @{$structs{$type}[0]}[$k];
						my $inter = type2sv( $lType, "$lVar. $lName");
						my $mc = mortal( $lType);
						print HEADER "\tXPUSHs( $mc( $inter));\n";
					}
				}
			} elsif ( exists $arrays{ $type}) {
			# arrays
				my $lType  = $arrays{ $type}[1];
				my $lCount = $arrays{ $type}[0];
				my $mc = mortal( $lType);
				my $adx = ( $lCount > 1) ? "\t" : "";
				print HEADER "\t{\n\t\tint $incCount;\n"
					if $adx;
				print HEADER "\t\tfor ( $incCount = 0; $incCount < $lCount; ${incCount}++)\n"
					if $adx;
				print HEADER "$adx\tXPUSHs( $mc( ", type2sv( $lType, "$lVar\[$incCount\]"), "));\n";
				print HEADER "\t}\n"
					if $adx;
				# scalars
			} else {
				if ( $type eq 'HV*') {
					print HEADER "\tsp = push_hv_for_REDEFINED( sp, $lVar);\n";
					$hvName = $lVar;
				} elsif ( $type eq "Handle") {
					print HEADER "\tXPUSHs( ", type2sv( $type, $lVar),");\n";
				} else {
					my $mc = mortal( $type);
					print HEADER "\tXPUSHs( $mc( ", type2sv( $type, $lVar),"));\n";
				};
			}
		}
		print HEADER "CALL_POINT:\n" if $property;
		print HEADER "\n\tPUTBACK;\n\n";
		if ( exists( $structs{ $resSub}) && ${$structs{ $resSub}[2]}{hash}) {
			$retType = "G_SCALAR";
		} elsif ( exists( $structs{ $resSub}) || exists( $arrays{ $resSub}) || $hvName) {
			$retType = "G_ARRAY";
		} elsif ( $resSub eq "void") {
			$retType = "G_DISCARD";
		} else {
			$retType = "G_SCALAR";
		};

		$retType = "set ? G_DISCARD : $retType" if $property;


		if ( $full) {
			print HEADER ( $imports)
			? "\t$incCount = PERL_CALL_PV( \"$ownOClass\:\:$id\", $retType);\n"
			: "\t$incCount = PERL_CALL_METHOD( \"$id\", $retType);\n";
		} else {
			print HEADER ( $imports)
			? "\t$incCount = PERL_CALL_PV( subName, $retType);\n"
			: "\t$incCount = PERL_CALL_METHOD( subName, $retType);\n";
		}
		if ( exists( $structs{ $resSub}) || exists( $arrays{ $resSub}))
		{
			# struct to return
			my $popParams;
			if ( exists $structs{ $resSub})
			{
				$popParams = defined ${$structs{ $resSub}[2]}{hash} ? 1 :
					scalar @{ $structs{ $resSub}[0]};
			} else {
				$popParams = $arrays{ $resSub}[0];
			}
			my $lParams = exists $structs{ $resSub} ? (
				${$structs{ $resSub}[2]}{hash} ? 1 : scalar @{ $structs{ $resSub}[0]}
			) : $arrays{ $resSub}[0];
			my $resVar = $lpr.$eptr.$incRes.$rpr;
			print HEADER "\tSPAGAIN;\n";
			print HEADER "\t$incCount = pop_hv_for_REDEFINED( sp, $incCount, $hvName, $popParams);\n"
				if $hvName;
			print HEADER "\tif ( set) {\n\t\tFREETMPS;\n\t\tLEAVE;\n\t\tmemset(&$incRes,0,sizeof($incRes));\n\t\treturn $incRes;\n\t}\n\n"
				if $property;
			print HEADER "\tif ( $incCount != $lParams) croak( \"Sub result corrupted\");\n\n";
			if ( exists $structs{ $resSub}) {
				# struct
				if ( defined ${$structs{ $resSub}[2]}{hash}) {
					print HEADER "\t$incRes = \& $castedResult ${result}_buffer;\n"
						if $eptr;
					my $xref = $eptr ? '' : '&';
					print HEADER "\tSvHV_$resSub( POPs, $castedResult $xref$incRes, $subId);\n";
				} else {
					print HEADER "\t$incRes = $castedResult ${result}_buffer;\n"
						if $eptr;
					for ( my $j = 0; $j < $lParams; $j++) {
						my $lType = @{ $structs{ $resSub}[ 0]}[ $lParams - $j - 1];
						my $lName = @{ $structs{ $resSub}[ 1]}[ $lParams - $j - 1];
						my $inter = sv2type_pop( $lType);
						print HEADER "\t", cwrite( $lType, $inter, "$resVar. $lName"), "\n";
					}
				}
			} else {
				# array
				print HEADER "\t$incRes =$castedResult \&${result}_buffer;\n" if $eptr;
				my $lType   = $arrays{ $resSub}[1];
				my $adx = ( $lParams > 1) ? "\t" : "";
				print HEADER "\t{\n\tint $incCount;\n"
					if $adx;
				print HEADER "\tfor ( $incCount = 0; $incCount < $lParams; ${incCount}++)\n"
					if $adx;
				print HEADER "$adx\t",
					cwrite( $lType, sv2type_pop( $lType),
					"$resVar\[$incCount\]"),
					"\n";
				print HEADER "\t}\n" if $adx;
			}
			print HEADER "\n";
			print HEADER "\tPUTBACK;\n\tFREETMPS;\n\tLEAVE;\n";
			print HEADER "\treturn $incRes;\n";
			# end struct to return
		} elsif ( $resSub eq "void") {
			# no parms to return
			if ( $hvName) {
				print HEADER "\n\tSPAGAIN;\n\n";
				print HEADER "\tpop_hv_for_REDEFINED( sp, $incCount, $hvName, 0);\n";
				print HEADER "\n\tPUTBACK;\n\tFREETMPS;\n\tLEAVE;\n";
			} else {
				print HEADER "\n\tSPAGAIN;\n\tFREETMPS;\n\tLEAVE;\n";
			}
		} else {
		# 1 param to return
			print HEADER "\tSPAGAIN;\n";
			print HEADER "\t$incCount = pop_hv_for_REDEFINED( sp, $incCount, $hvName, 1);\n"
				if $hvName;
			print HEADER "\tif ( set) {\n\t\tFREETMPS;\n\t\tLEAVE;\n\t\treturn ( $resSub$eptr) 0;\n\t}\n\n"
				if $property;
			print HEADER "\tif ( $incCount != 1) croak( \"Something really bad happened!\");\n\n";
			if ( $resSub eq 'Handle') {
				print HEADER "\t$incRes =$castedResult $incGetMate( POPs);";
			} elsif ($resSub eq "SV*") {
				print HEADER "\t$incRes =$castedResult SvREFCNT_inc( POPs);";
			} elsif ($resSub eq "char*") {
				print HEADER "\t$incRes =$castedResult newSVsv( POPs);";
			} else {
				print HEADER "\t$incRes =$castedResult ${xsConv{$resSub}[6]};";
			}
			print HEADER "\n\n";
			print HEADER "\tPUTBACK;\n\tFREETMPS;\n\tLEAVE;\n";
			if ( $resSub eq "char*") {
print HEADER <<LABEL;
	{
		char * $incRet =$castedResult SvPV_nolen( $incRes);
		sv_2mortal( $incRes);
		return $incRet;
	}
LABEL
			} else {
				print HEADER "\treturn $incRes;\n";
			}
		}
		print HEADER "}\n\n";
	};
}

sub out_FROMPERL_methods
{
	my ( $methods, $full) = @_;
	my %thunks = ();
																	# portable methods, bodies
	for ( my $i = 0; $i < scalar @{$methods}; $i++) {
		my @parms = split( " ", $$methods[ $i]);
		my $id = shift @parms;

		# forward declarations
		if ( $full && $publicMethods{$id})
		{
			print HEADER "XS( ${ownCType}_${id}_FROMPERL);\n\n";
			next;
		}

		if ( $full && $optimize && $templates_xs{$id}) {
			unless ( exists $thunks{$templates_xs{$id}}) {
				$thunks{$templates_xs{$id}} = 1;
				print HEADER "extern void $templates_xs{$id}( XS_STARTPARAMS, char* subName, void* func);\n\n";
			}
			my $func = ( exists $pipeMethods{ $id}) ?
				$pipeMethods{$id} :
				"${ownCType}_$id";
			print HEADER <<LABEL;
XS( ${ownCType}_${id}_FROMPERL) {
	$templates_xs{$id}( XS_CALLPARAMS, \"${ownOClass}\:\:$id\", (void*)$func);
}

LABEL
			next;
		}

		my $result    = shift @parms;                      # result type
		my $eptr  = $result =~ /^\*/ ? "*" : "";           # whether result is a pointer
		my ( $lpr, $rpr) = $eptr ? ('(',')') : ('','');
		$result =~ s[^\*][];                               # strip * signs
		my $resSub    = $mapTypes { $result} || $result;   # basic result type ( typecast skip)
		my $nParam    = scalar @parms;
		my $useHandle = !exists( $staticMethods{ $id});
		@defParms     = @{$allMethodsDefaults[$allMethods{$id}]};
		my $firstDP   = undef;
		my $lastDP    = 0;
		my $property  = defined $properties{$id};
		my $ifpropset;
		my $propparms = 1;
		my $propextras;
		$propextras = 1 if $property && $nParam > ( $useHandle ? 3 : 2);

		for ( my $k = 0; $k < scalar @defParms; $k++)
		{
			if ( defined $defParms[$k])
			{
				$firstDP = $k unless defined $firstDP;
				$lastDP++;
			}
		}
		my $useHV     = $nParam ? $parms[ $#parms] eq 'HV*' : 0;
		shift @parms if $useHandle;
		shift @parms if $property;
		my $delta = 0;
		foreach (@parms)
		{                                            # adjust parms count referring
			my $lVar = $_; $lVar =~ s[^\*][];
			if ( exists( $structs{ $lVar}) && !defined( ${$structs{ $lVar}[2]}{hash}))
			{                                         # to possible structs
				$delta = scalar @{$structs{$lVar}[0]} - 1;
			} elsif ( exists( $arrays{ $lVar})) {     # and arrays
				$delta = $arrays{$lVar}[0] - 1;
			} else {
				$delta = 0;
			}
			$nParam += $delta;
		}
		$propparms += $delta if $property;

		if ( $full) {
			print HEADER "XS( ${ownCType}_${id}_FROMPERL)";
		} else {
			# using pre-generated thunk name. Suggesting call is responsive to set
			# $methods to filtered array, that for sure contains that names.
			print HEADER "void $templates_xs{$id}( XS_STARTPARAMS, char* subName, void* func)";
		}

		print HEADER "\n{\n";
		print HEADER "	dXSARGS;\n";
		print HEADER "	Handle $incSelf;\n" if $useHandle;
		print HEADER "	(void)ax;\n" if !$useHandle && ( $nParam == 0);
		print HEADER "	if ";
		if ( $useHV)
		{
			print HEADER "(( items - $nParam + 1) % 2 != 0)";
		} else {
			if ( $property) {
				my $min = $nParam - $propparms - ( $useHandle ? 1 : 2);
				my $max = $nParam - 1;
				$ifpropset = "items > $min";
				print HEADER "(( items != $min) && ( items != $max))";
			} elsif ( $lastDP) {
				print HEADER "(( items > $nParam) || ( items < ${\($nParam-$lastDP)}))";
			} else {
				print HEADER "( items != $nParam)";
			}
		}
		my $croakId = $full ? "${ownOClass}\:\:\%s\", \"$id\"" : "\%s\", subName";
		print HEADER "\n\t\tcroak (\"Invalid usage of $croakId);\n";
		if ( $useHandle)
		{
print HEADER <<LABEL;
	$incSelf = $incGetMate( ST( 0));
	if ( $incSelf == nilHandle)
		croak( "Illegal object reference passed to $croakId);
LABEL

		}
		if ( defined $firstDP)
		{
			print HEADER "\t{\n";
			print HEADER "\t\tEXTEND( sp, $nParam - items);\n";
			for ( my $k = $firstDP; $k < scalar @defParms; $k++)
			{
				my $it=$nParam - (scalar @parms)+$k+1;
				my $dp=$defParms[$k];
				my $tp=$mapTypes{$parms[$k]}||$parms[$k];
				print HEADER "\tif ( items < $it) ";
				my $mc = mortal( $tp);
				print HEADER "PUSHs( $mc( ".type2sv( $tp, $dp)."));\n";
				# print HEADER "PUSHs( sv_2mortal( new$xsConv{$tp}[7]( $dp$xsConv{$tp}[5])));\n";
				# ^^ to enable being Handle default parameters
			}
			print HEADER "\t}\n";
		}
		print HEADER "\t{\n\t\t";
		my $structCount = 0;
		my $stn = $useHandle ? 1 : 0;
		unless ($resSub eq "void") {
			print HEADER "$result $eptr $incRes;\n\t\t"
		};
		# generating struct local vars
		my $reuseStructVar;
		my $idparm = 0;
		foreach (@parms)
		{
			my $ptr  = $_ =~ /^\*/ ? "&" : "";
			my $lVar = $_; $lVar =~ s[^\*][];
			my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
			if ( exists $structs{$lVar} && defined ${$structs{$lVar}[2]}{hash}) {
				$reuseStructVar = 1, last
					if $property && $idparm == $#parms && ( $ptr eq "");
				print HEADER "$lVar $incRes$structCount;\n\t\t";
				$structCount++;
			} elsif ( exists $arrays{$lVar} || exists $structs{$lVar}) {
				$reuseStructVar = 1, last
					if $property && $idparm == $#parms && ( $ptr eq "");
				print HEADER "$lVar $incRes$structCount;\n\t\t";
				$structCount++;
			} else {
				$stn++
			};
			$idparm++;
		}
		my $subId = $full ? "\"${ownClass}_$id\"" : 'subName';
		print HEADER "HV *hv = parse_hv( ax, sp, items, mark, $nParam - 1, $subId);\n\t\t"
			if $useHV;
		# initializing strings and additional parameters, if any
		$stn = $useHandle ? 1 : 0;
		$structCount = 0;
		my $paramAuxSet = '';
		$idparm = 0;
		foreach (@parms) {
			my $ptr  = $_ =~ /^\*/ ? "&" : "";
			my $lVar = $_; $lVar =~ s[^\*][];
			my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
			# hash structure
			if ( $property && $idparm == $#parms) {
				$structCount = '' if $reuseStructVar;
				$paramAuxSet .= '****'; # label point
			}
			if ( exists $structs{$lVar} && defined ${$structs{$lVar}[2]}{hash})
			{
				$paramAuxSet .= "SvHV_$lVar( ST( $stn), \&$incRes$structCount, $subId);\n\t\t";
				$structCount++;
				$stn++;
			} elsif ( exists $structs{$lVar}) # struct
			{
				for ( my $j = 0; $j < scalar @{ $structs{ $lVar}[ 0]}; $j++) {
					my $lType  = ${ $structs{ $lVar}[ 0]}[ $j];
					my $lName  = ${ $structs{ $lVar}[ 1]}[ $j];
					if ( $lType eq "string") {
						$paramAuxSet .= "strncpy( $incRes$structCount. $lName, ( char*) SvPV_nolen( ST( $stn)), 255); $incRes$structCount. $lName\[255\]=0;\n\t\t";
					} else {
						$paramAuxSet .= "$incRes$structCount. $lName = ";
						if ( $lType eq "SV*") {
							$paramAuxSet .= "ST( $stn);\n\t\t";
						} else {
							my $mtType = $mapTypes{$lType} || $lType;
							$paramAuxSet .= "( $lType) $xsConv{$mtType}[1]( ST( $stn)$xsConv{$mtType}[8]);\n\t\t";
						}
					}
					$stn++;
				}
				$structCount++;
			# array
			} elsif ( exists $arrays{$lVar}) {
				my $lName = $arrays{$lVar}[1];
				my $lType = $lName;
				$lName = $mapTypes{$lName} || $lName;
				my $str;
				$paramAuxSet .= "{\n\t\t\tint $incCount;\n\t\t\t";
				$paramAuxSet .= "for ( $incCount = 0; $incCount < $arrays{$lVar}[0]; $incCount++)\n\t\t\t";
				if ( $lName eq 'SV*') {
					$str = "$incRes$structCount\[$incCount\] = ST( $stn + $incCount)"
				} elsif ( $lName eq 'string') {
					$str = "strncpy( $incRes$structCount\[$incCount\], ( char*) SvPV_nolen( ST( $stn + $incCount)), 255);$incRes$structCount\[$incCount\]\[255\]=0"
				} else {
					$str = "$incRes$structCount\[$incCount\] = ( $lType) $xsConv{$lName}[1]( ST( $stn + $incCount)$xsConv{$lName}[8])";
				}
				$paramAuxSet .= "\t$str;\n\t\t}\n\t\t";
				$stn += $arrays{$lVar}[0];
				$structCount++;
			} else {
				$stn++
			};
			$idparm++;
		}
		# generating call
		my $lpaus = ( length( $paramAuxSet) > 4) || ( $paramAuxSet eq '****');
		if ( $lpaus || $property) {
			if ( $property && $lpaus) {
				my $label =  "if ( !( $ifpropset)) {\n";
				$label .= "\t\t\tmemset(&$incRes,0,sizeof($incRes));\n" if $reuseStructVar;
				$label .= "\t\t\tgoto CALL_POINT;\n\t\t}\n\t\t";
				$paramAuxSet =~ s/\*\*\*\*/$label/;
			}
			print HEADER $paramAuxSet;
			print HEADER "CALL_POINT : " if $property && $lpaus;
		}
		print HEADER "$incRes = " unless $resSub eq "void";
		if ( $full) {
			print HEADER ( exists $pipeMethods{ $id}) ?
				$pipeMethods{$id} :
				"${ownCType}_$id";
		} else {
			my @parmList = @parms;
			unshift( @parmList, 'Bool') if $property;
			unshift( @parmList, 'Handle') if $useHandle;
			for ( @parmList) {
				s/^\*(.*)/$1\*/;
			}  # since "*Struc" if way to know it's user ptr, map it back
			my $parmz = join( ', ', @parmList);
			print HEADER "(( $resSub$eptr (*)( $parmz)) func)";
		}
		print HEADER "(\n";
		print HEADER "\t\t\t$incSelf" if $useHandle;

		$stn = 0;
		$stn++ if $useHandle;

		if ( $property) {
			print HEADER ",\n" if $stn > 0;
			print HEADER "\t\t\t$ifpropset";
		}

		$structCount = 0;
		$idparm = 0;
		foreach (@parms) {
			my $ptr  = $_ =~ /^\*/ ? "&" : "";
			my $lVar = $_;
			$lVar =~ s[^\*][];
			$mtVar = $mapTypes{ $lVar} || $lVar;
			my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
			$structCount = '' if $property && $reuseStructVar && $idparm == $#parms;
			print HEADER ",\n" if $stn > 0;
			print HEADER "\t\t\t";
			print HEADER "( $ifpropset) ? ( " if
				$property && !$reuseStructVar && $idparm == $#parms;
			my $defPropParm;
			if ( exists $structs{$lVar} || exists $arrays{$lVar})
			{
				if ( exists $structs{$lVar}) {
					$stn += defined ${$structs{$lVar}[2]}{hash} ?
						1 :
						scalar @{ $structs{ $lVar}[ 0]};
				} else {
					$stn += $arrays{$lVar}[0];
				};
				print HEADER "$ptr$incRes$structCount";
				$defPropParm = "${lVar}_buffer";
				$structCount++;
			} else {
				if ( $mtVar eq "SV*") {
					print HEADER "ST ( $stn)";
				} elsif ( $mtVar eq "HV*") {
					print HEADER "hv";
				} else {
					print HEADER "$ptr( $lVar) $xsConv{$mtVar}[1]( ST( $stn)$xsConv{$mtVar}[8])";
				}
				$defPropParm = "($lVar)0";
				$stn++;
			}
			print HEADER ") : $defPropParm" if
				$property && !$reuseStructVar && $idparm == $#parms;
			$idparm++;
		}
		print HEADER "\n";
		print HEADER "\t\t);\n";
		print HEADER "\t\tSPAGAIN;\n\t\tSP -= items;\n"
			if (!( $resSub eq "void") || $useHV);
		print HEADER "\t\tif ( $ifpropset) {\n\t\t\tXSRETURN_EMPTY;\n\t\t\treturn;\n\t\t}\n"
			if $property;

		my $pphv = 0;
		# result generation
		if ( exists $structs{$resSub} && defined ${$structs{$resSub}[2]}{hash}) {
		# hashed structure -> hash reference
			my $bptr = ( $eptr eq '*') ? '' : '&';
			print HEADER "\t\tXPUSHs( sv_2mortal( sv_${resSub}2HV( $bptr$incRes)));\n";
			$pphv = 1;
		} elsif ( exists( $structs{ $resSub})) {
		# structure -> array
			my $rParms = scalar @{ $structs{ $resSub}[ 0]};
			print HEADER "\t\tEXTEND(sp, $rParms);\n";
			for ( my $j = 0; $j < $rParms; $j++) {
				my $lType = @{ $structs{ $resSub}[ 0]}[ $j];
				$lType = $mapTypes{$lType}||$lType;
				my $lName = @{ $structs{ $resSub}[ 1]}[ $j];
				my $mc = mortal( $lType);
				my $inter = type2sv( $lType, "$lpr$eptr$incRes$rpr. $lName");
				print HEADER "\t\tPUSHs( $mc( $inter));\n";
			}
			$pphv = $rParms;
		} elsif ( exists( $arrays{ $resSub})) {
		# array-> array
			my $rParms = $arrays{$resSub}[0];
			my $lType  = $arrays{$resSub}[1];
			$lType = $mapTypes{$lType}||$lType;
			print HEADER "\t\tEXTEND(sp, $rParms);\n";
			my $adx = ( $rParms > 1) ? "\t\t" : "";
			print HEADER "\t\t{\n\t\t\tint $incCount;\n"
				if $adx;
			print HEADER "\t\t\tfor ( $incCount = 0; $incCount < $rParms; ${incCount}++)\n"
				if $adx;
			my $mc = mortal( $lType);
			my $inter = type2sv( $lType, "$lpr$eptr$incRes$rpr\[$incCount\]");
			print HEADER "${adx}\tPUSHs( $mc( $inter));\n";
			print HEADER "\t\t}\n" if $adx;
			$pphv = $rParms;
		} elsif ($resSub eq "void") {
		# nothing
			$pphv = 0;
		} else {
		# scalars
			$pphv = 1;
			if ($resSub eq "Handle") {
				print HEADER "\t\tif ( $incRes && (( $incInst) $incRes)-> $hMate && ((( $incInst) $incRes)-> $hMate != nilSV))\n";
				print HEADER "\t\t{\n";
				print HEADER "\t\t\tXPUSHs( sv_mortalcopy((( $incInst) $incRes)-> $hMate));\n";
				print HEADER "\t\t} else XPUSHs( &PL_sv_undef);\n";
			} elsif ($resSub eq "SV*") {
				print HEADER "\t\tXPUSHs( sv_2mortal( $incRes));\n";
			} else {
				print HEADER "\t\tXPUSHs( sv_2mortal( new$xsConv{$resSub}[7]( $incRes$xsConv{$resSub}[5])));\n";
			}
		};
		print HEADER "\t\tpush_hv( ax, sp, items, mark, $pphv, hv);\n\t\treturn;\n"
			if $useHV;
		print HEADER "\t}\n";
		print HEADER $pphv ? "\tPUTBACK;\n\treturn;\n" : "\tXSRETURN_EMPTY;\n";
		print HEADER "}\n\n";
	}
}

sub out_struc
# printing out profiles
{
# calculating template names
if ( $optimize) {
	sub fill_templates
	{
		my ( $methods, $hashStorageRef, $checkDefParms, $tempPrefix) = @_;
METHODS: 	for ( my $i = 0; $i < scalar @{$methods}; $i++) {
			my @parms = split( " ", $$methods[ $i]);
			my $id = shift @parms;
			next if $publicMethods{$id};
			if ( $checkDefParms) {
				my $defParms = $allMethodsDefaults[$allMethods{$id}];
				next if @$defParms && defined $$defParms[-1];
			}
			my @ptrAddMap = ();
			# checking whether optimiztion could be applied - that's possible if all types are global here
			for ( @parms) {
				push( @ptrAddMap, ( s/\*//g) ? 1 : 0);
				next METHODS if (
					( exists( $structs {$_}) && !(${$structs{$_}[2]}{glo})) ||
					( exists( $arrays  {$_}) && !(${$arrays {$_}[2]}{glo}))
				);
			}
			# ok, do the optimizing code
			my $j;
			for ( $j = 0; $j < scalar @parms; $j++) {
				# cast types
				$_ = $parms[ $j];
				$_ = $mapTypes{$_}      if $mapTypes{$_};
				$_ = $defines{$_}{type} if $defines{$_}{type};
				$parms[ $j] = $_.($ptrAddMap[$j] ? 'Ptr' : '');
			}

			my $callName = $tempPrefix;
			$callName .= 'p_' if defined $properties{$id};
			$callName .= 's_' if defined $staticMethods{$id};
			$$hashStorageRef{$id} = $callName . join('_', @parms);;
	} # eo cycle
	} # eo sub

	fill_templates( \@portableMethods, \%templates_xs,  1, "${tmlPrefix}xs_");
	fill_templates( \@newMethods,      \%templates_rdf, 0, "${tmlPrefix}rdf_");
	fill_templates( \@portableImports, \%templates_imp, 0, "${tmlPrefix}imp_");

}  # eo optimizing


	if ( $genH) {

	my $out = $ownFile;
	$out =~ s[.*\/][];
	open HEADER, ">$dirOut$out.h" or die "Cannot open $dirOut$out.h\n";
print HEADER <<LABEL;
/* This file was automatically generated.
 * Do not edit, you\'ll lose your changes anyway.
 * file: $out.h  */
#ifndef ${ownCType}_H_
#define ${ownCType}_H_
#ifndef _APRICOT_H_
#include \"apricot.h\"
#endif
LABEL
																		# struct
print HEADER "#include \"$baseFile.h\"\n" if $baseClass;
{
	my %toInclude = ();
	for ( sort {
		$structs{$a}-> [PROPS]-> {order} <=> $structs{$b}-> [PROPS]-> {order}
	} keys %structs) {
		my $f = ${$structs{$_}[PROPS]}{incl};
		$toInclude{$f}=1 if $f;
	}

	for ( keys %arrays) {
		my $f = ${$arrays{$_}[2]}{incl};
		$toInclude{$f}=1 if $f;
	}

	for ( keys %defines) {
		my $f = $defines{$_}{incl};
		$toInclude{$f}=1 if $f;
	}

	for ( keys %mapTypes) {
		my $f = $typedefs{$_}{incl};
		$toInclude{$f}=1 if $f;
	}

	for ( keys %toInclude) {
		s[.*\/][];
		print HEADER "#include \"$_.h\"\n";
	}
}
print HEADER "\n";
print HEADER <<SD;
#ifdef __cplusplus
extern "C" {
#endif
SD

# generating inplaced structures
for ( sort { $structs{$a}-> [PROPS]-> {order} <=> $structs{$b}-> [PROPS]-> {order}} keys %structs)
{
	my $s = $structs{$_};
	if ( $$s[PROPS]{genh})
	{
		my @types = @{$$s[TYPES]};
		my @ids   = @{$$s[IDS]};
		my @def   = @{$$s[DEFS]};
		print HEADER "typedef struct _$_ {\n";
		my ($maxw_undefs, @undefs) = (0);
		my ($maxw_utfs, @utfs) = (0);
		for ( my $j = 0; $j < @types; $j++) {
			if ( ref $types[$j]) {
				print HEADER "\t$types[$j]->[PROPS]->{name} $ids[$j];\n";
			} elsif ( $types[$j] eq "string") {
				print HEADER "\tchar $ids[$j]\[256\];\n";
				push @utfs, $ids[$j];
				$maxw_utfs = length $ids[$j] if length($ids[$j]) > $maxw_utfs;
			} else {
				print HEADER "\t$types[$j] $ids[$j];\n";
			}

			if (($def[$j] // '') =~ /^undef:/) {
				push @undefs, $ids[$j];
				$maxw_undefs = length $ids[$j] if length($ids[$j]) > $maxw_undefs;
			}
		}
		my $wtab = $maxw_undefs // 0;
		$wtab = $maxw_utfs if ($maxw_utfs // 0) > $wtab;
		if ( @undefs ) {
			print HEADER "\tstruct {\n";
			printf HEADER "\t\tunsigned %\-${wtab}s : 1;\n", $_ for @undefs;
			print HEADER "\t} undef;\n";
		}
		if ( @utfs ) {
			print HEADER "\tstruct {\n";
			printf HEADER "\t\tunsigned %\-${wtab}s : 1;\n", $_ for @utfs;
			print HEADER "\t} is_utf8;\n";
		}
		print HEADER "} $_, *P$_;\n\n";
		if ( $$s[PROPS]{hash})
		{
			print HEADER "extern $_ * SvHV_$_( SV * hashRef, $_ * strucRef, const char * errorAt);\n";
			print HEADER "extern SV * sv_${_}2HV( $_ * strucRef);\n";
		}
		print HEADER "extern $_ ${_}_buffer;\n\n";
	}
}
# generating inplaced arrays
for ( keys %arrays)
{
	if ( ${$arrays{$_}[2]}{genh})
	{
		print HEADER "typedef $arrays{$_}[1] $_\[ $arrays{$_}[0]\];\n";
		print HEADER "extern $_ ${_}_buffer;\n\n";
	}
}
# and typedefs
for ( keys %mapTypes)
{
	if ( $typedefs{$_}{genh})
	{
		print HEADER "typedef $mapTypes{$_} $_;\n" unless $typedefs{$_}{cast};
	}
}
# defines
for ( keys %defines)
{
	if ( $defines{$_}{genh})
	{
		print HEADER "#define $_ $defines{$_}{type}\n";
	}
}

if ( $genObject)
{
	print HEADER <<LABEL;

typedef struct _${ownCType}_vmt {
/* internal runtime classifiers */
	char *$hClassName;
	void *$hSuper;
	void *$hBase;
	int $hSize;
	VmtPatch *patch;
	int patchLength;
	int vmtSize;
/* methods definition */
LABEL

		for ( my $i = 0; $i <= $#allMethods; $i++)
		{
			my $body = $allMethodsBodies[ $i];
			my $id = $allMethods[ $i];
			$body =~ s/\b$id\b/\( \*$id\)/;
			print HEADER "\t$body\n";
		}
print HEADER <<LABEL;
} ${ownCType}_vmt, *P${ownCType}_vmt;

extern P${ownCType}_vmt C${ownCType};

typedef struct _$ownCType {
/* internal pointers */
	P${ownCType}_vmt $hSelf;
LABEL
																		# vmt
if ($baseClass) { print HEADER "\tP${baseCType}_vmt " } else { print HEADER "\tvoid *"} ;
print HEADER "$hSuper;\n";
print HEADER "\tSV  *$hMate;\n";
print HEADER "\tstruct _AnyObject *killPtr;\n";
print HEADER "/* instance variables */\n";


	foreach (@allVars)                                  # variables
	{
		print HEADER "\t$_\n"
	}
	print HEADER "} $ownCType, *P$ownCType;\n";
	# end of object structures in .h file
};
print HEADER "\n";
if ( $genPackage)
{
	my $whatToReg = $genObject ? "Class" : "Package";
	print HEADER "extern void register_${ownCType}_${whatToReg}( void);\n\n";
	print HEADER "/* Local methods definitions */\n";
	for ( my $i = 0; $i <= $#allMethods; $i++)
	{
		my $id = $allMethods[$i];
		my $body = $allMethodsBodies[$i];
		$body =~ s{$id\(}{${ownCType}_$id\(};
		print HEADER "extern $body\n"
			if ( $allMethodsHosts{$id} eq $ownOClass) && !exists( $pipeMethods{$id});
	}
}

# property set/get defines
print HEADER "\n";

my %propBody = map {
	my @x = split( ' ', $_);
	( scalar(@x) > 5 && $properties{$x[0]}) ? ( $x[0] => $#x - 4) : ();
} @newMethods;

for ( keys %properties) {
	my $type = $properties{$_};
	my $def  = ( exists $structs{ $type} || exists( $arrays{ $type})) ?
		"${type}_buffer" :
		"($type)0";
	my @lval;
	my @rval;
	if ( defined $propBody{$_}) {
		@lval = ',' . join( ',', map { "__var$_"} ( 1..$propBody{$_}));
		@rval = ',' . join( ',', map { "(__var$_)"} ( 1..$propBody{$_}));
	}
	print HEADER <<SD;
#undef  get_$_
#undef  set_$_
#define get_$_(__hs@lval)         $_((__hs),0@rval,$def)
#define set_$_(__hs@lval,__val)   $_((__hs),1@rval,(__val))
SD
}


print HEADER <<SD;

#ifdef __cplusplus
}
#endif
#endif
SD

close HEADER;
} # end gen H


if ( $genInc) {
																			#2 - class.inc
my $out = $ownFile;
$out =~ s[.*\/][];
open HEADER, ">$dirOut$out.inc" or die "Cannot open $dirOut$out.inc\n";
print HEADER <<LABEL;
/* This file was automatically generated.
 * Do not edit, you\'ll lose your changes anyway.
 * file: $out.inc */

#include "$out.h"
LABEL
print HEADER "#include \"guts.h\"\n" if ( !$genDyna && $genObject);
print HEADER <<SD;
#ifdef __cplusplus
extern "C" {
#endif

SD

	out_FROMPERL_methods( \@portableMethods, 1);                # portable methods, bodies

	my %newH = (%structs, %arrays);
	for ( keys %newH)
	{
		print HEADER "$_ ${_}_buffer;\n" if ( ${$newH{$_}[2]}{genh});
	}
	print HEADER "\n\n";

	# generating SvHV_hash & sv_hash2HV, if any
	for ( sort { $structs{$a}-> [PROPS]-> {order} <=> $structs{$b}-> [PROPS]-> {order}} keys %structs)
	{
		my $S = $_;
		my $s = $structs{$S};
		if ( $s->[PROPS]->{genh} && $s->[PROPS]->{hash})
		{
			print HEADER "$S * SvHV_$S( SV * hashRef, $S * strucRef, const char * errorAt)\n{\n";
			print HEADER "\tconst char * err = errorAt ? errorAt : \"$S\";\n";
			print HEADER "\tHV * $incHV = ( HV*)\n\t".
				"(( SvROK( hashRef) && ( SvTYPE( SvRV( hashRef)) == SVt_PVHV)) ? SvRV( hashRef)\n\t\t".
				": ( croak( \"Illegal hash reference passed to %s\", err), nil));\n";
			print HEADER "\tSV ** $incSV;\n\n\t(void)$incSV;\n\n";
			for ( my $j = 0; $j < scalar @{$s->[TYPES]}; $j++)
			{
				my $lType = @{ $s->[TYPES]}[$j];
				my $lName = @{ $s->[IDS]}[$j];
				my $def   = @{ $s->[DEFS]}[$j];
				my $inter;
				my $lNameLen = length $lName;

				if ( ref $lType) {
					print HEADER <<CONTAINED_STRUCTURE;
	{
		SV *sv = nil;
		SV **svp = hv_fetch( $incHV, "$lName", $lNameLen, 0);
		if ( !svp) {
			sv = newRV_noinc(( SV*) newHV());
			svp = &sv;
		}
		SvHV_$lType->[PROPS]->{name}( *svp, &(strucRef-> $lName), errorAt);
		if ( sv)
			sv_free( sv);
	}
CONTAINED_STRUCTURE
				} else {
					print HEADER "\t$incSV = hv_fetch( $incHV, \"$lName\", $lNameLen, 0);\n";
					if ($def =~ /^undef:(.*)$/) {
						print HEADER "\tstrucRef-> undef.$lName = ($incSV == NULL);\n";
						$def = $1;
					}
					if ( $lType eq 'string') {
						print HEADER "\tstrucRef->is_utf8.$lName = ($incSV && prima_is_utf8_sv(*$incSV)) ? 1 : 0;\n";
					}
					$inter = "$incSV ? " . sv2type( $lType, "*$incSV") . " : $def";
					print HEADER "\t", cwrite( $lType, $inter, "strucRef-> $lName"), "\n\n";
				}
			}
			print HEADER "\treturn strucRef;\n";
			print HEADER "}\n\n";

			print HEADER "SV * sv_${_}2HV( $_ * strucRef)\n{\t\n";
			print HEADER "\tHV * $incHV = newHV();\n";
			for ( my $k = 0; $k < @{ $s->[TYPES]}; $k++)
			{
				my $lName = @{$s->[IDS]}[$k];
				my $lNameLen = length $lName;
				my $lType = @{$s->[TYPES]}[$k];
				my $inter = type2sv( $lType, "strucRef->$lName");
				my $prefix =
					($s->[DEFS]->[$k] =~ /^undef:/) ?
						"if (!strucRef->undef.$lName)" :
						"";
				print HEADER "\t$prefix (void) hv_store( $incHV, \"$lName\", $lNameLen, $inter, 0);\n";
			}
			print HEADER "\treturn newRV_noinc(( SV*) $incHV);\n";
			print HEADER "}\n\n";
		}
	}

	if ( $genObject)
	{
		out_method_profile( \@newMethods,      '${ownCType}_${id}_REDEFINED', 0, 1);
		out_method_profile( \@portableImports, '${ownCType}_${id}',           1, 1);

																# constructor & destructor
		print HEADER "\n";
		print HEADER "/* patches */\n";
		print HEADER "extern ${baseCType}_vmt ${baseCType}Vmt;\n" if ( $baseClass && !$genDyna);
		print HEADER "extern ${ownCType}_vmt ${ownCType}Vmt;\n\n";
		print HEADER "static VmtPatch ${ownCType}VmtPatch[] =\n";    # patches
		print HEADER "{";
		for (my $j = 0; $j <= $#newMethods; $j++)
		{
			my @locp = split (" ", $newMethods[ $j]);
			my $id = $locp[0];
			if ( $j) { print HEADER ','} ;
			print HEADER "\n\t{ &(${ownCType}Vmt. $id), (void*)${ownCType}_${id}_REDEFINED, \"$id\"}";
		}
		print HEADER "\n\t{nil,nil,nil} /* M\$C empty struct error */" unless scalar @newMethods;
		print HEADER "\n};\n\n";

		my $lpCount = $#newMethods + 1;
		print HEADER <<LABEL;

/* Class virtual methods table */
${ownCType}_vmt ${ownCType}Vmt = {
	\"${ownOClass}\",
	${\($baseClass && !$genDyna ? "\&${baseCType}Vmt" : "nil")},
	${\($baseClass && !$genDyna ? "\&${baseCType}Vmt" : "nil")},
	sizeof( $ownCType ),
	${ownCType}VmtPatch,
	$lpCount,
LABEL
	print HEADER "\tsizeof( ${ownCType}_vmt)";
		for ( my $i = 0; $i <= $#allMethods; $i++) {
			my $id = $allMethods[ $i];
			print HEADER ",\n\t";
			my $pt = ( exists $pipeMethods{ $id}) ? $pipeMethods{ $id} :
				"${\(do{my $c = $allMethodsHosts{$id}; $c =~ s[^Prima::][]; $c =~ s[::][__]g;  $c })}_$id";
			$pt = 'nil' if $genDyna && ( $allMethodsHosts{$id} ne $ownOClass);
			print HEADER $pt;
		}
		print HEADER "\n};\n\n";
		print HEADER "P${ownCType}_vmt C${ownCType} = \&${ownCType}Vmt;\n\n";

		print HEADER <<LABEL if 0;
XS( $incBuild$ownCType)
{
	dXSARGS;
	$ownCType *$incRes;
	char *$incClass;

	if ( items != 1)
		croak ("Invalid usage of $ownOClass\:\:\%s", "$incBuild$ownCType");
	{
		$incRes = malloc( sizeof( $ownCType));
		memset( $incRes, 0, sizeof( $ownCType));
		$incClass = HvNAME( SvSTASH( SvRV( ST(0))));
		$incRes-> self = ( P${ownCType}_vmt) $incGetVmt( $incClass, ( PVMT)C${ownCType}, sizeof( ${ownCType}Vmt));
		$incRes-> $hSuper = $incRes-> self-> super;

		ST( 0) = sv_newmortal();
		sv_setiv( ST(0), (IV)( Handle)$incRes);
	}
	XSRETURN(1);
}
LABEL
	}  # end of object vmt part
	print HEADER "\n";
	if ( $genPackage)
	{
		my $whatToReg = $genObject ? "Class" : "Package";
		print HEADER "void register_${ownCType}_${whatToReg}( void)\n";   # registration proc
		print HEADER "{";
		if ( $genObject)
		{
			print HEADER "\n\t";
			print HEADER $genDyna ? "if ( !build_dynamic_vmt( \&${ownCType}Vmt, \"$baseCType\", sizeof( ${baseCType}_vmt))) return;"
			: "build_static_vmt( \&${ownCType}Vmt);";
		}
		print HEADER "\n";
		foreach (@portableMethods)
		{
			my @parms = split(" ", $_);
			print HEADER "\tnewXS( \"${ownOClass}::$parms[0]\", ${ownCType}_$parms[0]_FROMPERL, \"$ownOClass\");\n";
		}
		print HEADER "\tnewXS( \"${ownOClass}::create_mate\", $incBuild$ownCType, \"$ownOClass\");\n"
			if $genObject && 0;
		print HEADER "}\n\n";
	}

print HEADER <<SD;

#ifdef __cplusplus
}
#endif
SD

	close HEADER;
	} # end gen Inc

if ( $genTml) {                                        #3 - class.tml
	my $out = $ownFile;
	$out =~ s[.*\/][];
	open HEADER, ">$dirOut$out.tml" or die "Cannot open $dirOut$out.tml\n";
	print HEADER <<LABEL;
/* This file was automatically generated.
 Do not edit, you'll lose your changes anyway.
 file: $out.tml   */

LABEL

	local @thunks;
	sub del_dup {
		my ( $hashRef, $methods) = @_;
		my %names = (reverse (%{$hashRef}));
		%names = (reverse (%names));
		@thunks = ();
		for ( @{$methods}) {
				m/^([^\s]*)/;
				push( @thunks, $_) if exists $names{$1};
		}
	}

	del_dup( \%templates_xs, \@portableMethods);
	out_FROMPERL_methods( \@thunks, 0);

	del_dup( \%templates_rdf, \@newMethods);
	out_method_profile( \@thunks, '${ownCType}_${id}_REDEFINED', 0, 0);

	del_dup( \%templates_imp, \@portableImports);
	out_method_profile( \@thunks, '${ownCType}_${id}',           1, 0);

	# my ( $reduced, $whole)  = ( scalar keys %names, scalar( keys %templates_xs) + scalar( keys %templates_imp) + scalar( keys %templates_rdf));
	# print "whole: $whole, reduced: $reduced\n";
	close HEADER;

} # end gen Tml
}

sub bool($)
{
	return $_[ 0] ? 1 : 0;
}

sub gencls
{
	my ( $clsfile) = shift;
	die "CLS file is not specified" unless defined $clsfile;
	init_variables;
	my $args;
	if ( ref( $_[ 0]) eq 'HASH') {
		$args = $_[ 0];
	}
	else {
		$args = { @_};
	}
	OPTION:
	foreach ( keys %$args) {
		/^depend$/    && do { $suppress = bool $args-> { depend}; next OPTION; };
		/^sayparent$/ && do { $suppress = bool $args-> { sayparent};
										$sayparent = bool $args-> { sayparent};
										next OPTION; };
		/^genH$/      && do { $genH = bool $args-> { genH}; next OPTION; };
		/^genInc$/    && do { $genInc = bool $args-> { genInc}; next OPTION; };
		/^genTml$/    && do { $genTml = bool $args-> { genTml}; next OPTION; };
		/^optimize$/  && do { $optimize = bool $args-> { optimize}; next OPTION; };
		/^dirPrefix$/ && do { $dirPrefix = $args-> { dirPrefix};
										$dirPrefix .= '/' unless $dirPrefix =~ /\/\\$/;
										next OPTION; };
		/^dirOut$/    && do { $dirOut = $args-> { dirOut};
										$dirOut .= '/' unless $dirOut =~ /\/\\$/;
										next OPTION; };
		/^incpath$/   && do { @includeDirs = @{ $args-> { incpath}}; next OPTION; };
		die "Unknown profile option $_";
	}

	if (!$genH and !$genInc and !$genTml) {
		$genInc = $genH = 1;
		$genTml = $optimize;
	}
	$optimize = 1 if $genTml;

	parse_file $clsfile;
	out_struc unless $suppress;
	return @ancestors ? @ancestors : ();
}

1;
