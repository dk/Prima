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

# Contains stubs for load-on-demand of the following modules:
#   Prima::OpenDialog       => Prima/FileDialog.pm
#   Prima::SaveDialog       => Prima/FileDialog.pm
#   Prima::ChDirDialog      => Prima/FileDialog.pm
#   Prima::FindDialog       => Prima/SearchDialog.pm
#   Prima::ReplaceDialog    => Prima/SearchDialog.pm
#   Prima::PrintSetupDialog => Prima/PrintDialog.pm
#   Prima::ColorDialog      => Prima/ColorDialog.pm

no strict;

package Prima::ColorDialog;

sub AUTOLOAD
{
   my ($method) = $Prima::ColorDialog::AUTOLOAD =~ /::([^:]+)$/;
   delete ${Prima::ColorDialog::}{AUTOLOAD};
   require Prima::ColorDialog;
   shift->$method(@_);
}

package Prima::OpenDialog;

sub AUTOLOAD
{
   my ($method) = $Prima::OpenDialog::AUTOLOAD =~ /::([^:]+)$/;
   delete ${Prima::OpenDialog::}{AUTOLOAD};
   delete ${Prima::SaveDialog::}{AUTOLOAD};
   delete ${Prima::ChDirDialog::}{AUTOLOAD};
   require Prima::FileDialog;
   shift->$method(@_);
}


1;
