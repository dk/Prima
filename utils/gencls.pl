#! /usr/bin/perl
# GENCLS
#
# APRICOT 001 PROJECT LINE
#
# .cls -> .h + .inc + .tml
# pseudo-object perl+c bondage interface parser
#
# Last modified 14-Jan-1999
#

#use strict;

# max error is APC053

# to index $struct{$type}->[]
use constant TYPES => 0;
use constant IDS   => 1;
use constant PROPS => 2;
use constant DEFS  => 3;

#defines
my $warnings = 1;

#constantza
my $clsMethod  = 'method';
my $clsNotify  = 'event';
my $clsImport  = 'import';
my $clsPublic  = 'public';
my $clsStatic  = 'static';
my $clsWeird   = 'weird';
my $clsC_only  = 'c_only';
my $clsLocalType  = 'local';
my $clsGlobalType = 'global';
my $hSelf      = "self";
my $hSuper     = "super";
my $hBase      = "base";
my $hMate      = "mate";
my $hClassName = "className";
my $hSize      = "instanceSize";
my $incSelf    = "_apt_self_";
my $incCount   = "_apt_count_";
my $incRes     = "_apt_res_";
my $incHV      = "_apt_hv_";
my $incRet     = "_apt_toReturn";
my $incClass   = "_apt_class_name_";
my $incSV      = "temporary_prf_Sv";
my $incGetMate = "gimme_the_mate";
my $incGetVmt  = "gimme_the_vmt";
my $incBuild   = "create_some_";
my $incInst    = "PAnyObject";
my $tmlPrefix  = "template_";

#variablez
my $dirPrefix;                         # startup directory
my $dirOut = "";                       # output directory
my $suppress = 0;                      # flag to suppress file output but produce inheritance
my $sayparent = 0;                     # say only parent's name
my $optimize  = 0;                     # use optimized inc-file generation model
my $genH      = 0;                     # generate h-file
my $genInc    = 0;                     # generate inc-file
my $genTml    = 0;                     # generate tml-file
my $genDyna   = 0;                     # generate exportable files
my $genObject = 0;                     # generate object files
my $genPackage = 0;                    # generate object or package files
my @includeDirs = qw(./);              # where to search .clses
my %definedClasses = ();               # hash for class search
my @allVars = ();                      # all variables, in order of definition
my %allVars = ();                      # hash for var search
my @allMethods = ();                   # methods names
my @allMethodsBodies = ();             # methods bodies, as is
my @allMethodsDefaults = ();           # methods default parameters
my %allMethods = ();                   # hash, containing indeces of methods
my %allMethodsHosts = ();              # hash, bonded methods => hosts
my @portableMethods = ();              # c & perl portable methods
my @newMethods = ();                   # portable methods that declared first time
my @portableImports = ();              # perl imported calls
my %publicMethods = ();                # public methods hash
my %staticMethods = ();                # static methods hash
my %pipeMethods = ();                  # pipe methods hash
my $level = 0;                         # file entrance level
my $ownClass;                          # own class name
my $ownFile;                           # own file name, for file.cls
my $ownCType;                          # own class c type ::-> __
my $baseClass;                         # base class name
my $baseFile ;                         # base file name width .cls
my $baseCType;                         # base class c type
my $acc;                               # local accumulator string
my $id;                                # identifier we manage with
my $piped;                             # local "pipe to" var
my $inherit;                           # local function inheritance flag
my @defParms;                          # local default values accumulator
my @context = ();                      # local context for storing words etc.
my %templates_xs  = ();                # storage for computed template xs names
my %templates_imp = ();                # storage for computed template import names
my %templates_rdf = ();                # storage for computed template redefined names


my %mapTypes = ("int" => "int", "Bool" => "Bool", "Handle" => "Handle", "long" => "int", "short" => "int",
                 "char" => "int", "U8" => "int",
                 "double" => "double", "Color" => "int", "SV"=> "SV", "HV"=> "HV");

my %typedefs = ();

my %xsConv = (# declare   access          set       unused   unused   extra2sv  POPx           newXXxx  extrasv2
              #   0         1              2          3        4         5       6                7       8
  'int'     => ['int',    'SvIV',      'sv_setiv',  '',      '(IV)',     '',    'POPi',         'SViv', ''     ],
  'double'  => ['double', 'SvNV',      'sv_setnv',  '',      '(double)', '',    'POPn',         'SVnv', ''     ],
  'char*'   => ['char *', 'SvPV',      'sv_setpv',  '(SV*)', '',         ', 0', 'POPp',         'SVpv', ', na' ],
  'string'  => ['char',   'SvPV',      'sv_setpv',  '(SV*)', '',         ', 0', 'POPp',         'SVpv', ', na' ],
  'Handle'  => ['Handle', $incGetMate, '',          '',      '',         '',    '0/0',          '',     ''     ],
  'SV*'     => ['SV *',   '',          '',          '',      '',         '',    '0/0',          '',     ''     ],
  'Bool'    => ['Bool',   'SvTRUE',    'sv_setiv',  '0/0',   '0/0',      '',    'SvTRUE(POPs)', 'SViv', ''     ],
);

my %mapPointers = ( "char" => "char*", "SV" => "SV*", "HV"=> "HV*");

my $nextStructure = 2;
my %structs = (
);
my %arrays = (
);
my %defines = (
);


my %forbiddenParms = (
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
   error "APC001: Found ``$tok'' but expecting ``$tofind''" unless ( $tok = gettok) eq $tofind;
}

sub check
# check whether $tok is equal to given parm
{
   my $tofind = shift;
   error "APC002: Found ``$tok'' but expecting ``$tofind''" unless $tok eq $tofind;
}

sub getid
# waits for indentifier
{
   $tok = gettok;
   error "APC003: Expecting identifier but found ``$tok''" unless $tok =~ /^\w+$/;
   return $tok;
}

sub getidsym
# waits for identifier on one of given symbols
{
#   my $what = shift;
   $tok = gettok;
   return $tok if index($_[0],$tok) >= 0;   # $what =~ /\Q$tok\E/;
   return $tok if $tok =~ /^\w+$/;
   error "APC004: Expecting identifier or one of ``$_[0]'' but found ``$tok''";
}

sub getObjName
{
   $tok = gettok;
   for ( split( '::', $tok))
   {
      error "APC033: Expecting object identifier but found ``$_''" unless $_ =~ /^\w+$/;
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
      @words = @{$fileCache{$fileName}->{words}};
      @lineNums = @{$fileCache{$fileName}->{lineNums}};
      return;
   }

   open FILE, $fileName or die "APC009: Cannot open $fileName\n";

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
            error "APC009a:  Invalid character in input";
         }
         redo;
      }
   }

   close FILE;
   @words = reverse @_words;
   @lineNums = reverse @_lineNums;
   $fileCache{$fileName}->{words} = [@words];
   $fileCache{$fileName}->{lineNums} = [@lineNums];
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
      eval { load_file( "$_$fileName") };
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
      return "$_$fileName", $dyna;
   }
   return undef;
}

sub checkid
{
   my $id = $_[0];
   error "APC017: Indentifier '$id' already defined or cannot be used; error"
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
   my $redef = exists $structs{$id} && defined ${$structs{$id}[2]}{hash};
   checkid($id) unless
     exists $structs{$id} &&
        (
           (${$structs{$id}[2]}{incl} eq $incl)  # already included in circular
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
      error "APC051: Invalid casting type $id" unless ( $arrays{$nid} || $structs{$nid} || $defines{$nid});
      error "APC052: Invalid forecasting type $id" if $redef;
      expect(';');
      if ( $arrays{$nid})
      {
         $arrays{ $id} = [ $arrays{$nid}[0], $arrays{$nid}[1], {%added}];
      } else {
         $structs{$id} = [[@{$structs{$nid}[0]}], [@{$structs{$nid}[1]}], {%added}, [@{$structs{$nid}[3]}]];
      }
      return;
   } else {
      if (( $strucId eq '@') && ( $mapTypes{ $tok}))
      {
          # ok, array found.
          my $type = $tok;
          if ( $type eq "SV")
          {
             expect ('*');
             $type .= '*';
          }
          expect('[');
          my $dim = gettok;
          error "APC050: Invalid array $id dimension '$dim'" if $dim <= 0;
          expect(']');
          expect(';');
          $arrays{ $id} = [ $dim, $type, {%added}];
          return;
      } else { check('{'); }
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
         error "APC053: Cannot do nested arrays (yet)"
            unless $structure->[PROPS]-> {hash};
         $type = 'SV*';
      } else {
         error "APC046: Invalid field type $type"
            if (!defined $mapTypes{$type} || $type eq 'HV') && ( $type ne 'string');
         $structure = $type = $mapTypes{$type} if $mapTypes{$type};
      }
      $tok = gettok;
      # checking pointer
      if ( $tok eq '*')
      {
         error "APC045: Invalid pointer to $type in structure $id"
            if ( $type ne 'char') && ( $type ne 'SV');
         $type .='*';
         $structure = $type;
         $tok = getid;
      } else {
         error "APC054: Invalid filed type $type in structure $id"
            if ( $type eq 'HV') or ( $type eq 'SV');
      }
      push ( @types, $structure);
      # field name;
      checkid( $tok);
      push ( @ids, $tok);
      if ( $strucId eq "%")
      {
         my $def;
         if ( $type eq 'Handle') { $def = '( Handle) C_POINTER_UNDEF'; } elsif
            ( $type eq 'SV*')    { $def = '( SV*) C_POINTER_UNDEF'; } elsif
            ( $type eq 'int')    { $def = 'C_NUMERIC_UNDEF'; } elsif
            ( $type eq 'Bool')   { $def = 'C_NUMERIC_UNDEF'; } elsif  # hack
            ( $type eq 'double') { $def = 'C_NUMERIC_UNDEF'; } elsif
            ( $type eq 'char*')  { $def = 'C_STRING_UNDEF'; } elsif
            ( $type eq 'string') { $def = 'C_STRING_UNDEF'; };
         $tok = gettok;
         if ( $tok eq '=')
         {
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
   error "APC047: Structure $id defined as empty" unless scalar @ids;

   if ( $redef)
   {
      # if it's hash default values redefine check for structure exact match
      my $saveLn = $ln;
      $ln = $lineStart;
      my @cmpType = (@{$structs{$id}[0]});
      my @cmpIds  = (@{$structs{$id}[1]});
      error "APC048: Structure $id is already defined. Error" unless @cmpType eq @types;
      for ( my $i = 0; $i < scalar @types; $i++)
      {
         error "APC048: Structure $id is already defined. Error"
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
   if ( $t eq '>')
   {
      $typecast = 1;
      $type = getid;
   } else {
      checkid( $t);
      $type = $t;
   }
   error "APC049: Invalid casting type $type" unless ( $mapTypes{$type});
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
   my $useHandle = pop;
   # processing result & proc name
   $acc = getid;
   my $checkEmptyRes = '';
   until (($tok = getidsym("*(")) eq "(")
   {
      $id = $tok;
      $acc .= " " . $tok;
      $checkEmptyRes .= $tok;
   }
   error "APC020: Function result haven't been defined" unless $checkEmptyRes;
   error "APC012: Unexpected (" unless $id;
   error "APC013: Method $id already defined"
      if exists($allMethodsHosts{ $id}) && $allMethodsHosts{ $id} eq $className;

   $acc .= "( ";
   $acc .= "Handle $hSelf" if $useHandle;

   #checking 'proc(void) & proc()' cases
   $tok = getidsym(")*");
   $acc .= ', ' unless ($tok eq "void") || ($tok eq ")") || (! $useHandle);
   if ($tok eq "void") {
     if ($words[-1] eq "*") {
        $acc .= ', ';
     } else {
        expect(')');
        $tok = ')';
     }
   }

   @defParms = ();
   my @types = ();

   # pre-parsing function parameters & result - laying own restrictions & other
   # , so $acc'll have all parameters divided by space
   if ( $tok ne ")")
   {
       putback ($tok);
       my $parmNo = 1;
       # cycling parameters
       until( undef)
       {
          my @parm = ();
          until ( undef)
          {
             $tok = gettok;         # getting single parm description
             push ( @parm, $tok);
             last if ($tok eq ",") || ( $tok eq ")");
          }
          # parsing parameter description
          my @gp = @parm;

          $acc .= join(' ', @parm);
          $acc =~ s/^([^=]*)(=.*)$/$1/; # removing default parms form $acc
          $acc .= $parm[-1] if $2;

          pop @gp;
          $tok = shift @gp;
          error "APC040: Invalid parameter $parmNo description" unless defined $tok;
          push ( @types, $tok); # first type description, definitive for
                                # default parameter validity check
          error "APC043: Invalid parameter $parmNo description" unless defined $gp[0];
          my $hasEqState = 0;
          my $defaultParm = undef;
          for ( @gp)            # cycling additive parameter description
          {
             # checking for type-name matching
             checkid($_);
             error "APC005: Indirect function and array parameters are unsupported. Error"
                if /^[\(\)\[\]\&]$/;
             error "APC006: Unexpected semicolon" if /^[\;]$/;
             error "APC042: End of default parameter description expected but found $_" if $hasEqState > 1;
             if ( $hasEqState == 1)
             {
                $defaultParm = $_;
                $hasEqState++;
             }
             # default parameter add
             if ( $_ eq "=")
             {
                error "APC041: Invalid default parameter $parmNo description" if $hasEqState;
                $hasEqState = 1;
             }
          }
          error "APC038: Default argument missing for parameter $parmNo"
             if !defined $defaultParm && defined $defParms[-1];
          push( @defParms, $defaultParm);
          last if ( $parm[-1] eq ")");
          $parmNo++;
       }
   } else {
      $acc .= $tok; # should be closing parenthesis
   };
   #  end of parameters description

   $piped = undef;
   $tok = gettok;
   if ( $tok eq "=")
   {
      $tok = gettok;
      error "APC044: Expected => but found $tok" unless $tok eq '>';
      $piped = getid;
      expect(';');
   } else {
      error "APC022: Expected semicolon or => but found $tok" unless $tok eq ';';
   }
   $acc .= ';';

   # checking default parameters
   for ( my $i = 0; $i <= $#types; $i++)
   {
      error "APC0039: Parameter ${\($i+1)} of type $types[$i] cannot be used with default parameters"
      if defined $defParms[$i] && (
         $structs{$types[$i]} || $arrays{$types[$i]} ||
#        ( $types[$i] eq "Handle") || <<<< to enable being Handle default parameters
         ( $types[$i] eq "SV") ||
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
   error "APC016: Header '$id' does not match previous definition"
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
   @chunks = split(",", $acc);
   $acc  = "";
   $hvUsed = 0;
   my $i = 0;
   foreach ( @chunks)
   {
      my @lxmz = split(" ", $_);
      my $valid = 0;
      foreach (@lxmz)
      {
         error "APC019: Reserved word '$_' used" if $forbiddenParms{$_};
         $hvUsed = 1 if $_ eq "HV";
      }
      for ( my $j = 0; $j <= $#lxmz; $j++)
      {
         my $keyType = $lxmz[ $j];
         my $orgType = $keyType;
         if ( exists( $structs{ $keyType}) || exists( $arrays{ $keyType}) || ( $keyType = $mapTypes{ $lxmz[ $j]}))
         {
            last if exists( $mapPointers{ $lxmz[ $j]}) && ( $lxmz[$j+1] ne "*");
            if ($lxmz[ $j+1] eq "*")
            {
               if ( exists( $mapPointers{ $lxmz[ $j]}))
               {
                  if ( $lxmz[ $j+2] eq "*") { last } else {
                     $orgType = $keyType = $mapPointers{ $lxmz[ $j]}
                  }
               } else {
                  if ( exists( $structs{ $keyType}) || exists( $arrays{ $keyType}))
                  {
                     $orgType = $keyType = "*".$lxmz[$j];
                  } else { last };
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
      error "APC021: HV* can be declared only at the end of parameters list." unless $lxmz[ 0] eq "HV";
   }
   return $acc;
}

sub load_single_part
{
   $tok = shift;
   $genObject = $tok eq "object";
   $genPackage = 1;
   $className = getObjName;

   if ( $genObject && (( $className eq "static") || ( $className eq "dynamic")))
   {
      $genDyna = ( $className eq "dynamic") unless $level;
      $className = getObjName;
   }
   unless ( $level)
   {
       $ownClass = $className;
       $ownFile  = (split( '::', $ownClass))[-1];
       $ownCType = $className;
       $ownCType =~ s/::/__/g;
   }
   error "APC010: Class $className already defined" if $definedClasses{ $className};
   $definedClasses{ $className} = 1;
   $tok = gettok;
   $superClass = '';
   if ( $genObject)
   {
      if ( $tok eq '(')
      {
         $superClass = getObjName;
         my $superFile = (split( '::', $superClass))[-1];
         unless ( $level)
         {
            $baseClass = $superClass;
            $baseFile  = $superClass;
            $baseFile  =~ s/::/\//g;
            $baseCType = $superClass;
            $baseCType =~ s/::/__/g;
         }
         expect ")";
         $tok = gettok;
         error "APC011: Cyclic inheritance detected.  Watch out ``$superClass''" if $definedClasses{ $superClass};
         do {
           $level++;
           print "ancestor: $superClass\n" if $suppress & (!$sayparent || $level==1);
           putback( $tok);
           my ( $superFile, $inhDyna) = find_file( "$superClass.cls");
           $tok = gettok;
           error "APC0035: Cannot find file corresponding to ''$superClass''" unless defined $superFile;
           error "APC0036: Static object cannot inherit dynamic module $superClass" if !$genDyna && $inhDyna;
           parse_file( $superFile) unless $sayparent;
           $level--;
         };
      } else {
         error "APC032: Cannot create dynamic object with no inheritance"
           if $genDyna && ( $level == 0);
      }
   }
   check "{";

   #parsing fields & methods
   until (( $tok = gettok) eq "}")
   {
      if ( $genObject)
      {
   # parsing object methods & vars
         if ($tok eq $clsMethod) {
           preload_method( 1);
           store_method;
           unless ($level) {
             if ( parse_parms) {
               push (@portableMethods, "${id}${acc}");
               push (@newMethods, "${id}${acc}") unless $inherit;
             }  else {
               print "Warning: method $id has not been published\n" if $warnings;
             }
           };
         } elsif ($tok eq $clsNotify) {
           preload_method( 1);
           store_method;
           unless ($level) {
             if ( parse_parms) {
               push (@newMethods, "${id}${acc}") unless $inherit;
             }  else {
               print "Warning: method $id has not been published\n" if $warnings;
             }
           };
         } elsif ($tok eq $clsImport) {
           preload_method( 1);
           store_method;
           unless ($level) {
              error "APC0018: Invalid types in import function $id declaration" unless parse_parms;
              push (@portableImports, "${id}${acc}");
              push (@newMethods, "${id}${acc}") unless $inherit;
           };
         } elsif ($tok eq $clsStatic) {
           preload_method( 0);
           $staticMethods{$id} = 1;
           store_method;
           unless ($level) {
             if ( parse_parms) {
                push (@portableMethods, "${id}${acc}");
                push (@newMethods, "${id}${acc}") unless $inherit;
             } else {
                 print "Warning: static $id has not been published\n" if $warnings;
             }
           };
         } elsif ($tok eq $clsPublic) {
           preload_method( 1);
           $publicMethods{$id} = 1;
           store_method;
           unless ($level) {
             parse_parms;
             push (@portableMethods, "${id}${acc}");
             push (@newMethods, "${id}${acc}") unless $inherit;
           };
         } elsif ($tok eq $clsWeird) {
           preload_method( 0);
           $staticMethods{$id} = 1;
           $publicMethods{$id} = 1;
           store_method;
           unless ($level) {
             parse_parms;
             push (@portableMethods, "${id}${acc}");
             push (@newMethods, "${id}${acc}") unless $inherit;
           };
         } elsif ($tok eq $clsC_only) {
            preload_method( 1);
            store_method;
         } else {
           # variable workaround
           error "APC008: Expecting identifier but found ``$tok''" unless $tok =~ /^\w+$/;
           $acc = $tok;
           # !!!!! possible bug at int [345] !!!!
           until (($tok = getidsym("*;[]()+")) eq ";")
           {
              $id = $tok;
              $acc .= " " . $tok;
           }
           $acc .= ";";
           error "APC015: Variable $id already defined" if $allVars{ $id};
           $allVars{$id} = $className;
           push(@allVars, $acc);
         }
         # end of parsing object
      } else {
         # parsing package
         putback( $tok);
         preload_method( 0);
         $staticMethods{$id} = 1;
         store_method;
         unless ($level) {
            if ( parse_parms) {
               push (@portableMethods, "${id}${acc}");
               push (@newMethods, "${id}${acc}") unless $inherit;
            } else {
               print "Warning: function $id has not been published\n" if $warnings;
            }
         };
      }
   }
} # end of load_single_part

sub load_structures
{
    until ( undef)
    {
        $tok = gettok;
        unless ( defined $tok)
        {
           putback("");
           last;
        }

        if (( $tok eq $clsLocalType) || ( $tok eq $clsGlobalType)) {
           my $global = $tok eq $clsGlobalType;
           my $tok2 = $tok;
           $tok = gettok;
           if (( $tok eq '@') || ( $tok eq '%'))
           {
              parse_struc( $tok, $global);
              next;
           } elsif ( $tok eq '$') {   # dont be stupid, cperl! '
              parse_typedef( $global);
              next;
           } elsif ( $tok eq 'define') {
              parse_define( $global);
              next;
           }
           putback( $tok2);
           putback( $tok);
        } elsif ( $tok eq 'use')
        {
           my $fid = getid;
           expect(';');
           save_context;
           load_file( "$fid.cls");
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
  if (( $tok eq "object") || ( $tok eq "package"))
  {
     load_single_part( $tok);
  } elsif ( $tok) {
     error "APC037: found $tok but expecting 'object' or 'package'";
  } else {
     unless ( $level)
     {
        #$genInc = 0;
        $ownFile = $fileName;
        $ownFile =~ s/([^.]*)\..*$/$1/;
        $ownCType = uc $ownFile;
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
      return "( $name ? (( $incInst)$name)-> $hMate : &sv_undef)";
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
   if ( $type eq 'Handle')
   {
      return "$incGetMate( $name);";
   } elsif ( $type eq 'SV*') {
      return $name;
   } else {
      return "$xsConv{$type}[1]( $name$xsConv{$type}[8])";
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
      return "strcpy( $to, $from);";
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

   for ( my $i = 0; $i < @{$methods}; $i++)
   {
      my @parms = split( " ", ${$methods}[ $i]);
      my $id = shift @parms;
      my $useHandle = !exists( $staticMethods{ $id});
      my $tok = $allMethodsBodies[$allMethods{$id}];
      my $exStr = eval( "\"$substring\"");
      $tok =~ s/$id(?=\()/$exStr/;      # incorporating class name
      $tok =~ s/;([^;]*)$/$1/;          # adding ; to the end
      print( HEADER "$tok;\n\n"), next if $publicMethods{$id} && $full;
      my $castedResult = $parms[0];
      $castedResult = exists $typedefs{$castedResult} && $typedefs{$castedResult}{cast} ? " ( $castedResult)" : '';
      my $subId = $full ? ( $imports ? "\"$ownClass\:\:$id\"" : "\"$id\"") : 'subName';

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

      if ( $useHandle)
      {
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
         my $str  = $imports ? "\"$ownClass\:\:$id\"" : "\"$id\"";
         print HEADER "   $ret$$optRef{$id}( $str, $parmz);\n}\n\n";
         next;
      }

      print HEADER "   dSP;\n";
      print HEADER "   int $incCount;\n";
      unless ( $resSub eq "void") {
        if ( $resSub eq "char*" ) {
           print HEADER "   SV *$incRes;\n\n";
        } else {
           print HEADER "   $result $eptr $incRes;\n\n";
        }
      }
      print HEADER "   ENTER;\n";
      print HEADER "   SAVETMPS;\n";
      print HEADER "   PUSHMARK( sp);\n\n";
      print HEADER "   XPUSHs((( P${ownCType})$hSelf)-> $hMate);\n" if $useHandle;

      # writing other parameters
      for ( my $j = 0; $j < scalar @parm_ids; $j++)
      {
         my $lVar = $parm_ids[ $j];
         my $type = $parms[ $j];
         my $ptr  = $type =~ /^\*/ ? "*" : "";
         $type =~ s[^\*][];
         my $hashStruc = exists($structs{ $type}) && defined ${$structs{ $type}[2]}{hash};
         if ( $hashStruc) {
         # take down the reference in this case
            $ptr = ( $ptr eq '*') ? '' : '&';
         }
         my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
         $lVar = $lp.$ptr.$lVar.$rp;

         # structs
         if ( exists $structs{ $type})
         {
            if ( $hashStruc)
            {
               print HEADER "   XPUSHs( sv_2mortal( sv_${type}2HV( $lVar)));\n";
            } else {
               for ( my $k = 0; $k < scalar @{ $structs{ $type}[0]}; $k++)
               {
                  my $lName = @{$structs{$type}[1]}[$k];
                  my $lNameLen = length $lName;
                  my $lType = @{$structs{$type}[0]}[$k];
                  my $inter = type2sv( $lType, "$lVar. $lName");
                  my $mc = mortal( $lType);
                  print HEADER "   XPUSHs( $mc( $inter));\n";
               }
            }
         } elsif ( exists $arrays{ $type})
         # arrays
         {
           my $lType  = $arrays{ $type}[1];
           my $lCount = $arrays{ $type}[0];
           my $mc = mortal( $lType);
           my $adx = ( $lCount > 1) ? "    " : "";
           print HEADER "   {\n      int $incCount;\n" if $adx;
           print HEADER "      for ( $incCount = 0; $incCount < $lCount; ${incCount}++)\n" if $adx;
           print HEADER "$adx   XPUSHs( $mc( ", type2sv( $lType, "$lVar\[$incCount\]"), "));\n";
           print HEADER "   }\n" if $adx;
         # scalars
         } else {
           if ( $type eq 'HV*')
           {
              print HEADER "   sp = push_hv_for_REDEFINED( sp, $lVar);\n";
              $hvName = $lVar;
           } elsif ( $type eq "Handle") {
              print HEADER "   XPUSHs( ", type2sv( $type, $lVar),");\n";
           } else {
              my $mc = mortal( $type);
              print HEADER "   XPUSHs( $mc( ", type2sv( $type, $lVar),"));\n";
           };
         }
      }
      print HEADER "\n   PUTBACK;\n\n";
      if ( exists( $structs{ $resSub}) || exists( $arrays{ $resSub}) || $hvName) {
        $retType = "G_ARRAY";
      } elsif ( $resSub eq "void") {
        $retType = "G_DISCARD";
      } else {
        $retType = "G_SCALAR";
      };

      if ( $full) {
         print HEADER ( $imports)
           ? "   $incCount = PERL_CALL_PV( \"$ownClass\:\:$id\", $retType);\n"
           : "   $incCount = PERL_CALL_METHOD( \"$id\", $retType);\n";
      } else {
         print HEADER ( $imports)
           ? "   $incCount = PERL_CALL_PV( subName, $retType);\n"
           : "   $incCount = PERL_CALL_METHOD( subName, $retType);\n";
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
        my $lParams = exists $structs{ $resSub} ? scalar @{ $structs{ $resSub}[0]} : $arrays{ $resSub}[0];
        my $resVar = $lpr.$eptr.$incRes.$rpr;
        print HEADER "   SPAGAIN;\n";
        print HEADER "   $incCount = pop_hv_for_REDEFINED( sp, $incCount, $hvName, $popParams);\n" if $hvName;
        print HEADER "   if ( $incCount != $lParams) croak( \"Sub result corrupted\");\n\n";
        if ( exists $structs{ $resSub})
        {
           # struct
           if ( defined ${$structs{ $resSub}[2]}{hash})
           {
              print HEADER "   $incRes = \& $castedResult ${result}_buffer;\n" if $eptr;
              my $xref = $eptr ? '' : '&';
              print HEADER "   SvHV_$resSub( POPs, $castedResult $xref$incRes, $subId);\n";
           } else {
              print HEADER "   $incRes = $castedResult ${result}_buffer;\n" if $eptr;
              for ( my $j = 0; $j < $lParams; $j++)
              {
                 my $lType = @{ $structs{ $resSub}[ 0]}[ $lParams - $j - 1];
                 my $lName = @{ $structs{ $resSub}[ 1]}[ $lParams - $j - 1];
                 my $inter = sv2type( $lType, "POPs");
                 print HEADER "   ", cwrite( $lType, $inter, "$resVar. $lName"), "\n";
              }
           }
        } else {
           # array
           print HEADER "   $incRes =$castedResult \&${result}_buffer;\n" if $eptr;
           my $lType   = $arrays{ $resSub}[1];
           my $adx = ( $lParams > 1) ? "    " : "";
           print HEADER "   {\n      int $incCount;\n" if $adx;
           print HEADER "      for ( $incCount = 0; $incCount < $lParams; ${incCount}++)\n" if $adx;
           print HEADER "$adx   ", cwrite( $lType, sv2type( $lType, "POPs"), "$resVar\[$incCount\]"), "\n";
           print HEADER "   }\n" if $adx;
        }
        print HEADER " \n";
        print HEADER "   PUTBACK;\n   FREETMPS;\n   LEAVE;\n";
        print HEADER "   return $incRes;\n";
      } elsif ( $resSub eq "void") {
        # no parms to return
        if ( $hvName)
        {
           print HEADER "\n   SPAGAIN;\n\n";
           print HEADER "   pop_hv_for_REDEFINED( sp, $incCount, $hvName, 0);\n";
           print HEADER "\n   PUTBACK;\n   FREETMPS;\n   LEAVE;\n";
        } else {
           print HEADER "\n   SPAGAIN;\n   FREETMPS;\n   LEAVE;\n";
        }
      } else {
        # 1 parms to return
        print HEADER "   SPAGAIN;\n";
        print HEADER "   $incCount = pop_hv_for_REDEFINED( sp, $incCount, $hvName, 1);\n" if $hvName;
        print HEADER "   if ( $incCount != 1) croak( \"Something really bad happened!\");\n\n";
        if ( $resSub eq 'Handle') {
           print HEADER "   $incRes =$castedResult $incGetMate( POPs);";
        } elsif ($resSub eq "SV*") {
           print HEADER "   $incRes =$castedResult SvREFCNT_inc( POPs);";
        } elsif ($resSub eq "char*") {
           print HEADER "   $incRes =$castedResult newSVsv( POPs);";
        } else {
           print HEADER "   $incRes =$castedResult ${xsConv{$resSub}[6]};";
        }
        print HEADER "\n\n";
        print HEADER "   PUTBACK;\n   FREETMPS;\n   LEAVE;\n";
        if ( $resSub eq "char*") {
print HEADER <<LABEL;
   {
      char * $incRet =$castedResult SvPV( $incRes, na);
      sv_2mortal( $incRes);
      return $incRet;
   }
LABEL
        } else {
           print HEADER "   return $incRes;\n";
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
   for ( my $i = 0; $i < scalar @{$methods}; $i++)
   {
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
            print HEADER "extern void $templates_xs{$id}( CV* cv, char* subName, void* func);\n\n";
         }
         my $func = ( exists $pipeMethods{ $id}) ? $pipeMethods{$id} : "${ownCType}_$id";
         print HEADER <<LABEL;
XS( ${ownCType}_${id}_FROMPERL) {
   $templates_xs{$id}( cv, \"${ownClass}\:\:$id\", $func);
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
      foreach (@parms)
      {                                            # adjust parms count referring
         my $lVar = $_; $lVar =~ s[^\*][];
         if ( exists( $structs{ $lVar}) && !defined( ${$structs{ $lVar}[2]}{hash}))
         {                                         # to possible structs
            $nParam += scalar @{$structs{$lVar}[0]} - 1;
         } elsif ( exists( $arrays{ $lVar})) {     # and arrays
            $nParam += $arrays{$lVar}[0] - 1;
         }
      }

      if ( $full) {
         print HEADER "XS( ${ownCType}_${id}_FROMPERL)";
      } else {
         # using pre-generated thunk name. Suggesting call is responsive to set
         # $methods to filtered array, that for sure contains that names.
         print HEADER "void $templates_xs{$id}( CV* cv, char* subName, void* func)";
      }

      print HEADER "\n{\n";
      print HEADER "   dXSARGS;\n";
      print HEADER "   Handle $incSelf;\n" if $useHandle;
      print HEADER "   (void)ax;\n" if !$useHandle && ( $nParam == 0);
      print HEADER "   if ";
      if ( $useHV)
      {
         print HEADER "(( items - $nParam + 1) % 2 != 0)";
      } else {
         if ( $lastDP)
         {
            print HEADER "(( items > $nParam) || ( items < ${\($nParam-$lastDP)}))";
         } else {
            print HEADER "( items != $nParam)";
         }
      }
      my $croakId = $full ? "${ownClass}\:\:\%s\", \"$id\"" : "\%s\", subName";
      print HEADER "\n      croak (\"Invalid usage of $croakId);\n";
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
         print HEADER "   {\n";
         print HEADER "      EXTEND( sp, $nParam - items);\n";
         for ( my $k = $firstDP; $k < scalar @defParms; $k++)
         {
            my $it=$nParam - (scalar @parms)+$k+1;
            my $dp=$defParms[$k];
            my $tp=$mapTypes{$parms[$k]}||$parms[$k];
            print HEADER "      if ( items < $it) ";
            my $mc = mortal( $tp);
            print HEADER "PUSHs( $mc( ".type2sv( $tp, $dp)."));\n";
            # print HEADER "PUSHs( sv_2mortal( new$xsConv{$tp}[7]( $dp$xsConv{$tp}[5])));\n";
            # ^^ to enable being Handle default parameters
         }
         print HEADER "   }\n";
      }
      print HEADER "   {\n      ";
      my $structCount = 0;
      my $stn = $useHandle ? 1 : 0;
      unless ($resSub eq "void") { print HEADER "$result $eptr $incRes;\n      "};
      # generating struct local vars
      foreach (@parms)
      {
         my $ptr  = $_ =~ /^\*/ ? "&" : "";
         my $lVar = $_; $lVar =~ s[^\*][];
         my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
         if ( exists $structs{$lVar} && defined ${$structs{$lVar}[2]}{hash})
         {
             # print HEADER "$lVar $incRes$structCount = SvHV_$lVar( ST( $stn), \"${ownClass}\:\:$id\");\n      ";
             print HEADER "$lVar $incRes$structCount;\n      ";
             $structCount++;
         } elsif ( exists $arrays{$lVar})
         {
             print HEADER "$lVar $incRes$structCount;\n      ";
             $structCount++;
         } elsif ( exists $structs{$lVar})
         {
             print HEADER "$lVar $incRes$structCount = {";
             my $locCount = exists $structs{$lVar} ? scalar @{$structs{$lVar}[ 0]} : $arrays{$lVar}[0];
             for ( my $j = 0; $j < $locCount; $j++)
             {
                 print HEADER "," if $j;
                 print HEADER "\n         ";
                 my $lType =  ${ $structs{ $lVar}[ 0]}[ $j];
                 my $mtType = $mapTypes{$lType}||$lType;
                 if ( $lType eq "SV*") {
                     print HEADER "ST( $stn)";
                 } elsif ( $lType eq "string") {
                     print HEADER "\"\"";
                 } else {
                     print HEADER "( $lType) $xsConv{$mtType}[1]( ST( $stn)$xsConv{$mtType}[8])";
                 }
                 $stn++;
             }
             print HEADER "\n      };\n      ";
             $structCount++;
         } else { $stn++ };
      }
      my $subId = $full ? "\"${ownClass}_$id\"" : 'subName';
      print HEADER "HV *hv = parse_hv( ax, sp, items, mark, $nParam - 1, $subId);\n      " if $useHV;
      # initializing strings and additional parameters, if any
      $stn = $useHandle ? 1 : 0;
      $structCount = 0;
      foreach (@parms)
      {
         my $ptr  = $_ =~ /^\*/ ? "&" : "";
         my $lVar = $_; $lVar =~ s[^\*][];
         my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
         # hash structure
         if ( exists $structs{$lVar} && defined ${$structs{$lVar}[2]}{hash})
         {
            print HEADER "SvHV_$lVar( ST( $stn), \&$incRes$structCount, $subId);\n      ";
            $structCount++;
            $stn++;
         } elsif ( exists $structs{$lVar})
         # struct
         {
            for ( my $j = 0; $j < scalar @{ $structs{ $lVar}[ 0]}; $j++)
            {
                if ( ${ $structs{ $lVar}[ 0]}[ $j] eq "string")
                {
                    my $lName = ${ $structs{ $lVar}[ 1]}[ $j];
                    print HEADER "strcpy( $incRes$structCount. $lName, ( char*) SvPV( ST( $stn), na));\n      ";
                }
                $stn++;
            }
            $structCount++;
         # array
         } elsif ( exists $arrays{$lVar})
         {
            my $lName = $arrays{$lVar}[1];
            my $lType = $lName;
            $lName = $mapTypes{$lName} || $lName;
            my $str;
            print HEADER "{\n         int $incCount;\n         ";
            print HEADER "for ( $incCount = 0; $incCount < $arrays{$lVar}[0]; $incCount++)\n         ";
            if ( $lName eq 'SV*') { $str = "$incRes$structCount\[$incCount\] = ST( $stn + $incCount)" }
            elsif ( $lName eq 'string')
            {
               $str = "strcpy( $incRes$structCount\[$incCount\], ( char*) SvPV( ST( $stn + $incCount), na))"
            } else {
               $str = "$incRes$structCount\[$incCount\] = ( $lType) $xsConv{$lName}[1]( ST( $stn + $incCount)$xsConv{$lName}[8])";
            }
            print HEADER "   $str;\n      }\n      ";
            $stn += $arrays{$lVar}[0];
            $structCount++;
         } else { $stn++ };
      }
      # generating call
      print HEADER "$incRes = " unless $resSub eq "void";
      if ( $full) {
         print HEADER ( exists $pipeMethods{ $id}) ? $pipeMethods{$id} : "${ownCType}_$id";
      } else {
         my @parmList = @parms;
         unshift( @parmList, 'Handle') if $useHandle;
         for ( @parmList) { s/^\*(.*)/$1\*/;}  # since "*Struc" if way to know it's user ptr, map it back
         my $parmz = join( ', ', @parmList);
         print HEADER "(( $resSub$eptr (*)( $parmz)) func)";
      }
      print HEADER "(\n";
      print HEADER "         $incSelf" if $useHandle;   # это мы закладываемся
                                                        # на Handle
      $stn = $useHandle ? 1 : 0;
      $structCount = 0;
      foreach (@parms)
      {
         my $ptr  = $_ =~ /^\*/ ? "&" : "";
         my $lVar = $_;
         $lVar =~ s[^\*][];
         $mtVar = $mapTypes{ $lVar} || $lVar;
         my ( $lp, $rp) = $ptr ? ('(',')') : ('','');
         print HEADER ",\n" if $stn > 0;
         print HEADER "         ";
         if ( exists $structs{$lVar} || exists $arrays{$lVar})
         {
            if ( exists $structs{$lVar}) {
               $stn += defined ${$structs{$lVar}[2]}{hash} ? 1 : scalar @{ $structs{ $lVar}[ 0]};
            } else {
               $stn += $arrays{$lVar}[0];
            };
            print HEADER "$ptr$incRes$structCount";
            $structCount++;
         } else {
           if ( $mtVar eq "SV*") {
              print HEADER "ST ( $stn)";
           } elsif ( $mtVar eq "HV*") {
              print HEADER "hv";
           } else {
              print HEADER "$ptr( $lVar) $xsConv{$mtVar}[1]( ST( $stn)$xsConv{$mtVar}[8])";
           }
           $stn++;
         }
      }
      print HEADER "\n";
      print HEADER "      );\n";
      print HEADER "      SPAGAIN;\n      SP -= items;\n" if (!( $resSub eq "void") || $useHV);

      my $pphv = 0;
      # result generation
      if ( exists $structs{$resSub} && defined ${$structs{$resSub}[2]}{hash})
      # hashed structure -> hash reference
      {
         my $bptr = ( $eptr eq '*') ? '' : '&';
         print HEADER "      XPUSHs( sv_2mortal( sv_${resSub}2HV( $bptr$incRes)));\n";
         $pphv = 1;
      }
      elsif ( exists( $structs{ $resSub}))
      # structure -> array
      {
        my $rParms = scalar @{ $structs{ $resSub}[ 0]};
        print HEADER "      EXTEND(sp, $rParms);\n";
        for ( my $j = 0; $j < $rParms; $j++)
        {
            my $lType = @{ $structs{ $resSub}[ 0]}[ $j];
            $lType = $mapTypes{$lType}||$lType;
            my $lName = @{ $structs{ $resSub}[ 1]}[ $j];
            my $mc = mortal( $lType);
            my $inter = type2sv( $lType, "$lpr$eptr$incRes$rpr. $lName");
            print HEADER "      PUSHs( $mc( $inter));\n";
        }
        $pphv = $rParms;
      } elsif ( exists( $arrays{ $resSub})) {
      # array-> array
        my $rParms = $arrays{$resSub}[0];
        my $lType  = $arrays{$resSub}[1];
        $lType = $mapTypes{$lType}||$lType;
        print HEADER "      EXTEND(sp, $rParms);\n";
        my $adx = ( $rParms > 1) ? "       " : "";
        print HEADER "      {\n         int $incCount;\n" if $adx;
        print HEADER "         for ( $incCount = 0; $incCount < $rParms; ${incCount}++)\n" if $adx;
        my $mc = mortal( $lType);
        my $inter = type2sv( $lType, "$lpr$eptr$incRes$rpr\[$incCount\]");
        print HEADER "$adx  PUSHs( $mc( $inter));\n";
        print HEADER "      }\n" if $adx;
        $pphv = $rParms;
      } elsif ($resSub eq "void") {
      # nothing
        $pphv = 0;
      } else {
      # scalars
        $pphv = 1;
        if ($resSub eq "Handle") {
           print HEADER "      if ( $incRes && (( $incInst) $incRes)-> $hMate && ((( $incInst) $incRes)-> $hMate != nilSV))\n";
           print HEADER "      {\n";
           print HEADER "         XPUSHs( sv_mortalcopy((( $incInst) $incRes)-> $hMate));\n";
           print HEADER "      } else XPUSHs( &sv_undef);\n";
        } elsif ($resSub eq "SV*") {
           print HEADER "      XPUSHs( sv_2mortal( $incRes));\n";
        } else {
           print HEADER   "      XPUSHs( sv_2mortal( new$xsConv{$resSub}[7]( $incRes$xsConv{$resSub}[5])));\n";
        }
      };
      print HEADER "      push_hv( ax, sp, items, mark, $pphv, hv);\n      return;\n" if $useHV;
      print HEADER "   }\n";
      print HEADER $pphv ? "   PUTBACK;\n   return;\n" : "   XSRETURN_EMPTY;\n";
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
 METHODS: for ( my $i = 0; $i < scalar @{$methods}; $i++) {
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
         my $callName = $tempPrefix . join('_', @parms);
         $$hashStorageRef{$id} = $callName;
     } # eo cycle
     } # eo sub

     fill_templates( \@portableMethods, \%templates_xs,  1, "${tmlPrefix}xs_");
     fill_templates( \@newMethods,      \%templates_rdf, 0, "${tmlPrefix}rdf_");
     fill_templates( \@portableImports, \%templates_imp, 0, "${tmlPrefix}imp_");

  }  # eo optimizing


   if ( $genH)
   {
                                                     # 1 - class.h
   open HEADER, ">$dirOut$ownFile.h" or die "APC009: Cannot open $dirOut$ownFile.h\n";
  print HEADER <<LABEL;
/* This file was automatically generated.
 * Do not edit, you\'ll loose your changes anyway.
 * file: $ownFile.h  */
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
     for ( sort { $structs{$a}->[PROPS]->{order} <=> $structs{$b}->[PROPS]->{order}} keys %structs)
     {
        my $f = ${$structs{$_}[2]}{incl};
        $toInclude{$f}=1 if $f;
     }
     for ( keys %arrays)
     {
        my $f = ${$arrays{$_}[2]}{incl};
        $toInclude{$f}=1 if $f;
     }
     for ( keys %defines)
     {
        my $f = $defines{$_}{incl};
        $toInclude{$f}=1 if $f;
     }
     for ( keys %mapTypes)
     {
        my $f = $typedefs{$_}{incl};
        $toInclude{$f}=1 if $f;
     }
     for ( keys %toInclude)
     {
        print HEADER "#include \"$_.h\"\n";
     }
  }
  print HEADER "\n";

  # generating inplaced structures
  for ( sort { $structs{$a}->[PROPS]->{order} <=> $structs{$b}->[PROPS]->{order}} keys %structs)
  {
     if ( ${$structs{$_}[PROPS]}{genh})
     {
        my @types = @{$structs{$_}->[TYPES]};
        my @ids = @{$structs{$_}->[IDS]};
        print HEADER "typedef struct _$_ {\n";
        for ( my $j = 0; $j < @types; $j++) {
	   if ( ref $types[$j]) {
	      print HEADER "   $types[$j]->[PROPS]->{name} $ids[$j];\n";
	   } elsif ( $types[$j] eq "string") {
	      print HEADER "   char $ids[$j]\[256\];\n";
	   } else {
	      print HEADER "   $types[$j] $ids[$j];\n";
	   }
        }
        print HEADER "} $_, *P$_;\n\n";
        if ( ${$structs{$_}[PROPS]}{hash})
        {
           print HEADER "extern $_ * SvHV_$_( SV * hashRef, $_ * strucRef, char * errorAt);\n";
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
// internal runtime classifiers
   char *$hClassName;
   void *$hSuper;
   void *$hBase;
   int $hSize;
   VmtPatch *patch;
   int patchLength;
   int vmtSize;
// methods definition
LABEL

      for ( my $i = 0; $i <= $#allMethods; $i++)
      {
         my $body = $allMethodsBodies[ $i];
         my $id = $allMethods[ $i];
         $body =~ s/\b$id\b/\( \*$id\)/;
         print HEADER "   $body\n";
      }
  print HEADER <<LABEL;
} ${ownCType}_vmt, *P${ownCType}_vmt;

extern P${ownCType}_vmt C${ownCType};

typedef struct _$ownCType {
// internal pointers
   P${ownCType}_vmt $hSelf;
LABEL
                                                      # vmt
if ($baseClass) { print HEADER "   P${baseCType}_vmt " } else { print HEADER "   void *"} ;
print HEADER "$hSuper;\n";
print HEADER "   SV  *$hMate;\n";
print HEADER "   struct _AnyObject *killPtr;\n";
print HEADER "// instance variables \n";


     foreach (@allVars)                                  # variables
     {
       print HEADER "   $_\n"
     }
     print HEADER "} $ownCType, *P$ownCType;\n";
     # end of object structures in .h file
  };
  print HEADER "\n";
  if ( $genPackage)
  {
     my $whatToReg = $genObject ? "Class" : "Package";
     print HEADER "extern void register_${ownCType}_${whatToReg}();\n\n";
     print HEADER "//Local methods definitions\n";
     for ( my $i = 0; $i <= $#allMethods; $i++)
     {
        my $id = $allMethods[$i];
        my $body = $allMethodsBodies[$i];
        $body =~ s{$id\(}{${ownCType}_$id\(};
        print HEADER "extern $body\n" if ( $allMethodsHosts{$id} eq $ownClass) && !exists( $pipeMethods{$id});
     }
  }

print HEADER "\n#endif\n";

close HEADER;
  } # end gen H


  if ( $genInc) {
                                                         #2 - class.inc
  open HEADER, ">$dirOut$ownFile.inc" or die "APC009: Cannot open $dirOut$ownFile.inc\n";
  print HEADER <<LABEL;
/* This file was automatically generated.
 * Do not edit, you\'ll loose your changes anyway.
 * file: $ownFile.inc */

#include "$ownFile.h"
LABEL
print HEADER "#include \"guts.h\"\n" if ( !$genDyna && $genObject);
print HEADER "\n";

   out_FROMPERL_methods( \@portableMethods, 1);                # portable methods, bodies

   my %newH = (%structs, %arrays);
   for ( keys %newH)
   {
      print HEADER "$_ ${_}_buffer;\n" if ( ${$newH{$_}[2]}{genh});
   }
   print HEADER "\n\n";

   # generating SvHV_hash & sv_hash2HV, if any
   for ( sort { $structs{$a}->[PROPS]->{order} <=> $structs{$b}->[PROPS]->{order}} keys %structs)
   {
      if ( ${$structs{$_}[2]}{genh} && ${$structs{$_}[2]}{hash})
      {
          print HEADER "$_ * SvHV_$_( SV * hashRef, $_ * strucRef, char * errorAt)\n{\n";
          print HEADER "   char * err = errorAt ? errorAt : \"$_\";\n";
          print HEADER "   HV * $incHV = ( HV*)\n   ".
          "(( SvTYPE( SvRV( hashRef)) == SVt_PVHV) ? SvRV( hashRef)\n      ".
          ": ( croak( \"Illegal hash reference passed to %s\", err), nil));\n";
          for ( my $j = 0; $j < scalar @{$structs{$_}[0]}; $j++)
          {
             my $lType = @{ $structs{$_}[ TYPES]}[$j];
             my $lName = @{ $structs{$_}[ IDS]}[$j];
             my $def   = @{ $structs{$_}[ DEFS]}[$j];
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
		$inter = "( $incSV = hv_fetch( $incHV, \"$lName\", $lNameLen, 0)) ? \n   ".
		   sv2type( $lType, "*$incSV")." : $def";
		print HEADER "   ", cwrite( $lType, $inter, "strucRef-> $lName"), "\n";
	     }
	  }
          print HEADER "   return strucRef;\n";
          print HEADER "}\n\n";

          print HEADER "SV * sv_${_}2HV( $_ * strucRef)\n{   \n";
          print HEADER "   HV * $incHV = newHV();\n";
          for ( my $k = 0; $k < @{ $structs{$_}[TYPES]}; $k++)
          {
             my $lName = @{$structs{$_}[IDS]}[$k];
             my $lNameLen = length $lName;
             my $lType = @{$structs{$_}[TYPES]}[$k];
             my $inter = type2sv( $lType, "strucRef-> $lName");
             print HEADER "   hv_store( $incHV, \"$lName\", $lNameLen, $inter, 0);\n";
          }
          print HEADER "   return newRV_noinc(( SV*) $incHV);\n";
          print HEADER "}\n\n";
      }
   }

   if ( $genObject)
   {
      out_method_profile( \@newMethods,      '${ownCType}_${id}_REDEFINED', 0, 1);
      out_method_profile( \@portableImports, '${ownCType}_${id}',           1, 1);

                                                # constructor & destructor
      print HEADER "\n";
      print HEADER "// patches\n";
      print HEADER "extern ${baseCType}_vmt ${baseCType}Vmt;\n" if ( $baseClass && !$genDyna);
      print HEADER "${ownCType}_vmt ${ownCType}Vmt;\n\n";
      print HEADER "static VmtPatch ${ownCType}VmtPatch[] =\n";    # patches
      print HEADER "{";
      for (my $j = 0; $j <= $#newMethods; $j++)
      {
         my @locp = split (" ", $newMethods[ $j]);
         my $id = $locp[0];
         if ( $j) { print HEADER ','} ;
         print HEADER "\n  { &(${ownCType}Vmt. $id), &${ownCType}_${id}_REDEFINED, \"$id\"}";
      }
      print HEADER "\n  {nil,nil,nil} // M\$C empty struct error" unless scalar @newMethods;
      print HEADER "\n};\n\n";

      my $lpCount = $#newMethods + 1;
      print HEADER <<LABEL;

//Class virtual methods table
${ownCType}_vmt ${ownCType}Vmt = {
   \"${ownClass}\",
   ${\($baseClass && !$genDyna ? "\&${baseCType}Vmt" : "nil")},
   ${\($baseClass && !$genDyna ? "\&${baseCType}Vmt" : "nil")},
   sizeof( $ownCType ),
   ${ownCType}VmtPatch,
   $lpCount,
LABEL
   print HEADER "   sizeof( ${ownCType}_vmt)";
      for ( my $i = 0; $i <= $#allMethods; $i++) {
         my $id = $allMethods[ $i];
         print HEADER ",\n   ";
         my $pt = ( exists $pipeMethods{ $id}) ? $pipeMethods{ $id} :
            "${\(do{my $c = $allMethodsHosts{$id}; $c =~ s[::][__]g; $c })}_$id";
         $pt = 'nil' if $genDyna && ( $allMethodsHosts{$id} ne $ownClass);
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
      croak ("Invalid usage of $ownClass\:\:\%s", "$incBuild$ownCType");
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
      print HEADER "void register_${ownCType}_${whatToReg}()\n";   # registration proc
      print HEADER "{";
      if ( $genObject)
      {
         print HEADER "\n   ";
         print HEADER $genDyna ? "if ( !build_dynamic_vmt( \&${ownCType}Vmt, \"$baseCType\", sizeof( ${baseCType}_vmt))) return;"
           : "build_static_vmt( \&${ownCType}Vmt);";
      }
      print HEADER "\n";
      foreach (@portableMethods)
      {
         my @parms = split(" ", $_);
         print HEADER "   newXS( \"${ownClass}::$parms[0]\", ${ownCType}_$parms[0]_FROMPERL, \"$ownClass\");\n";
      }
      print HEADER "   newXS( \"${ownClass}::create_mate\", $incBuild$ownCType, \"$ownClass\");\n"
         if $genObject && 0;
      print HEADER "}\n\n";
   }
   close HEADER;
   } # end gen Inc

  if ( $genTml) {                                        #3 - class.tml
     open HEADER, ">$dirOut$ownFile.tml" or die "APC009: Cannot open $dirOut$ownFile.tml\n";
     print HEADER <<LABEL;
// This file was automatically generated.
// Do not edit, you'll loose your changes anyway.
/*  file: $ownFile.tml   */

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

# Main
unless ( $ARGV[ 0]) {
  print <<TEXT;
Apricot project. Pseudoobject Perl+C Bondage interface parser.
format  : gencls.pl [ options] filename.cls [ out_directory]
options :
   --h          generates .h file
   --inc        generates .inc file
   --tml        generates .tml file ( and turns -O flag on)
   -O           turns optimized .inc generation on
   -Idirlist    specifies include directories list
   --depend     produces output of dependences of given object only
   --sayparent  produces parent dependency of object only
TEXT
  die "\n";
}

ARGUMENT: while( 1)
{
   $_ = $ARGV[0];
   /^--depend$/    && do { $suppress = 1; next ARGUMENT; };
   /^--sayparent$/ && do { $suppress = 1; $sayparent = 1; next ARGUMENT; };
   /^--h$/         && do { $genH = 1; next ARGUMENT; };
   /^--inc$/       && do { $genInc = 1; next ARGUMENT; };
   /^--tml$/       && do { $genTml = 1; next ARGUMENT; };
   /^-O$/          && do { $optimize = 1; next ARGUMENT; };
   /^-I(.*)$/      && do {
      my $ii = $1;
      @includeDirs = map { m{[\\/]$} ? $_ : "$_/" } (@includeDirs, split ';', $ii);
      next ARGUMENT;
   };
   last ARGUMENT;
} continue { shift @ARGV; }
die "APC000: insufficient number of parameters" unless $ARGV [0];

if (!$genH and !$genInc and !$genTml) {
   $genInc = $genH = 1;
   $genTml = $optimize;
}
$optimize = 1 if $genTml;

$ARGV[ 0] =~ m{^(.*[\\/])[^\\/]*$};
$dirPrefix = $1 || "";
$dirOut = "$ARGV[ 1]/" if $ARGV[ 1];
parse_file $ARGV[ 0];
out_struc unless $suppress;
