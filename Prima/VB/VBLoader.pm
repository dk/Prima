package Prima::VB::VBLoader;
use strict;
use vars qw($builderActive);

$builderActive = 0;

sub GO_SUB
{
   return $builderActive ? $_[0] : eval "sub { $_[0] }";
}

sub AUTOFORM_REALIZE
{
   my ( $seq, $parms) = @_;
   my %ret = ();
   my %modules = ();
   my $main;
   my $i;

   my %dep;
   for ( $i = 0; $i < scalar @$seq; $i+= 2) {
      $dep{$$seq[$i]} = $$seq[$i + 1];
   }

   for ( keys %dep) {
      $modules{$dep{$_}->{module}} = 1 if $dep{$_}->{module};
      $main = $_ if $dep{$_}->{parent};
   }
   for ( keys %modules) {
      my $c = $_;
      eval("use $c;");
      die "$@" if $@;
   }

   delete $dep{$main}->{profile}->{owner};
   $ret{$main} = $dep{$main}->{class}-> create(
      %{$dep{$main}->{profile}},
      %{$parms->{$main}},
   );

   my %owners = ( $main => 0);
   for ( keys %dep) {
      next if $_ eq $main;
      $owners{$_} = exists $parms->{$_}->{owner} ? $parms->{$_}->{owner} :
         ( exists $dep{$_}->{profile}->{owner} ? $dep{$_}->{profile}->{owner} : $main);
      delete $dep{$_}->{profile}->{owner};
   }

   my $do_layer;
   $do_layer = sub
   {
      my $id = $_[0];
      my $i;
      for ( $i = 0; $i < scalar @$seq; $i += 2) {
         $_ = $$seq[$i];
         next unless $owners{$_} eq $id;
         $owners{$_} = $main unless exists $ret{$owners{$_}}; # validating owner entry
         $ret{$_} = $ret{$owners{$_}}-> insert(
            $dep{$_}->{class},
            %{$dep{$_}->{profile}},
            %{$parms->{$_}},
         );
         $do_layer->( $_);
      }
   };
   $do_layer->( $main, \%owners);

   return %ret;
}

sub AUTOFORM_CREATE
{
   my ( $filename, %parms) = @_;
   my $contents;
   {
      local $/;
      open F, $filename or die "Cannot open $filename";
      $contents = <F>;
      close F;
   }

   my $pack;
   if ( $contents =~ /package\s+([^\s;].*)/m) {
      $pack = $1;
      $pack =~ s[[\s;]*$][];
   } else {
      die "Invalid format: $filename";
   }
   eval( $contents);
   my @dep = $pack-> AUTOFORM_DEPLOY;
   return AUTOFORM_REALIZE( \@dep, \%parms);
}

1;
