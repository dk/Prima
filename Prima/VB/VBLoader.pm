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

   my %owners = ( $main => 0);
   for ( keys %dep) {
      next if $_ eq $main;
      $owners{$_} = exists $parms->{$_}->{owner} ? $parms->{$_}->{owner} :
         ( exists $dep{$_}->{profile}->{owner} ? $dep{$_}->{profile}->{owner} : $main);
      delete $dep{$_}->{profile}->{owner};
   }

   my @actNames  = qw( onBegin onFormCreate onCreate onChild onChildCreate onEnd);
   my %actions   = map { $_ => {}} @actNames;
   my %instances = ();
   for ( keys %dep) {
      my $key = $_;
      my $act = $dep{$_}->{actions};
      $instances{$_} = {};
      $instances{$_}-> {extras} = $dep{$_}->{extras} if $dep{$_}->{extras};

      for ( @actNames) {
         next unless exists $act->{$_};
         $actions{$_}->{$key} = $act->{$_};
      }
   }

   $actions{onBegin}->{$_}->($_, $instances{$_})
      for keys %{$actions{onBegin}};

   delete $dep{$main}->{profile}->{owner};
   $ret{$main} = $dep{$main}->{class}-> create(
      %{$dep{$main}->{profile}},
      %{$parms->{$main}},
   );
   $ret{$main}-> lock;
   $actions{onFormCreate}->{$_}->($_, $instances{$_}, $ret{$main})
      for keys %{$actions{onFormCreate}};
   $actions{onCreate}->{$main}->($main, $instances{$_}, $ret{$main})
      if $actions{onCreate}->{$main};

   my $do_layer;
   $do_layer = sub
   {
      my $id = $_[0];
      my $i;
      for ( $i = 0; $i < scalar @$seq; $i += 2) {
         $_ = $$seq[$i];
         next unless $owners{$_} eq $id;
         $owners{$_} = $main unless exists $ret{$owners{$_}}; # validating owner entry

         my $o = $owners{$_};
         $actions{onChild}->{$o}->($o, $instances{$o}, $ret{$o}, $_)
            if $actions{onChild}->{$o};

         $ret{$_} = $ret{$o}-> insert(
            $dep{$_}->{class},
            %{$dep{$_}->{profile}},
            %{$parms->{$_}},
         );

         $actions{onCreate}->{$_}->($_, $instances{$_}, $ret{$_})
            if $actions{onCreate}->{$_};
         $actions{onChildCreate}->{$o}->($o, $instances{$o}, $ret{$o}, $ret{$_})
            if $actions{onChildCreate}->{$o};

         $do_layer->( $_);
      }
   };
   $do_layer->( $main, \%owners);

   $actions{onEnd}->{$_}->($_, $instances{$_}, $ret{$_})
      for keys %{$actions{onEnd}};
   $ret{$main}-> unlock;
   return %ret;
}

sub AUTOFORM_CREATE
{
   my ( $filename, %parms) = @_;
   my $contents;
   {
      local $/;
      open F, $filename or die "Cannot open $filename\n";
      $contents = <F>;
      close F;
   }

   die "Corrupted file $filename" unless $contents =~ /^\s*sub\s*{/;
   my $sub = eval( $contents);
   die "$@" if $@;
   my @dep = $sub-> ();
   return AUTOFORM_REALIZE( \@dep, \%parms);
}

# onBegin       ( name, instance)
# onFormCreate  ( name, instance, formObject)
# onCreate      ( name, instance, object)
# onChild       ( name, instance, object, childName)
# onChildCreate ( name, instance, object, childObject)
# onEnd         ( name, instance, object)

1;
