##                        -*-CPerl-*-
##
##  Copyright 2006,2007 Doug Campbell
##
##  This file is part of Cam-Scom.
##
##  Cam-Scom is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  Cam-Scom is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with Cam-Scom; if not, see <http://www.gnu.org/licenses/>.

package Cam::Scom::Event;

use Exporter 'import';

use warnings;
use integer;
use strict;

=head1 NAME

Cam::Scom::Event - Event object support for Cam::Scom

=head1 VERSION

Version 0.01

=cut

our $VERSION = '0.01';

=head1 SYNOPSIS

This module is used purely to define the fields of
a return event from Cam::Scom's getevents method.

=cut

use fields
  (
   "fh",                ## filehandle object being reported on
   "end",               ## true if this is a termination event
   "term",              ## termination code (if "end")
   "data",              ## the data (for non-server)           (if ! "end")
   "port",              ## sender port number (for messaging)  (if ! "end")
                        ##    new connect filehandle (for server) (if ! "end")
   "addr",              ## 4-byte sender addr (for messaging)  (if ! "end")
                        ##    4-byte connect addr (for server)    (if ! "end")
  );

=head1 FUNCTIONS

=head2 new

Creates a new Cam::Scom::Event object

=cut

sub new
{
    my $self = shift;
    (ref $self) || ($self = fields::new($self));
    return $self;
};

=head2 DESTROY

Destructs a Cam::Scom::Event object

=cut

sub DESTROY
{
};

=head1 AUTHOR

Doug Campbell, C<< <doug.campbell at pobox.com> >>

=head1 SUPPORT

You can find documentation for this module with the perldoc command.

    perldoc Cam::Scom

=head1 COPYRIGHT & LICENSE

Copyright 2006 Doug Campbell, all rights reserved.

This program is released under the following license: gpl

=cut

1; # End of Cam::Scom::Event
