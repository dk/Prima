package MakeConfig;

BEGIN
{
   unless ( grep { -f "$_/Config.pm" } @INC)
   {
      print "Trying to create a real Config!\n";
   }
}

1;
