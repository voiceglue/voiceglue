package Vxglue;     ## -*-CPerl-*-

use Exporter 'import';
require DynaLoader;
@ISA = qw(DynaLoader);
use Data::Dumper;

use warnings;
use strict;

=head1 NAME

Vxglue - Glue to VXML implementation

=head1 VERSION

Version 0.03

=cut

our $VERSION = '0.03';

=head1 SYNOPSIS

Vxglue provides a perl interface to the voiceglue-modified
VXML implementation in C/C++.
It is probably not useful outside of voiceglue.

    use Vxglue;


=head1 FUNCTIONS

=head1 AUTHOR

Doug Campbell, C<< <soup at ampersand.com> >>

=head1 BUGS

See the voiceglue.org web page for information on
reporting bugs.

=head1 SUPPORT

See the voiceglue.org web page for information on
support.

=head1 ACKNOWLEDGEMENTS

=head1 COPYRIGHT & LICENSE

Copyright 2006-2008 Ampersand Inc., Doug Campbell

This file is part of Vxglue.

Vxglue is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Vxglue is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vxglue; if not, see <http://www.gnu.org/licenses/>.

=cut

bootstrap Vxglue $VERSION;        ##  if using xs

1; # End of Vxglue
