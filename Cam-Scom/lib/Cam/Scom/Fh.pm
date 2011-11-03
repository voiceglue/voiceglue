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

package Cam::Scom::Fh;

use Exporter 'import';

use warnings;
use integer;
use strict;

=head1 NAME

Cam::Scom::Fh - Internal support for Cam::Scom

=head1 VERSION

Version 0.02

=cut

our $VERSION = '0.02';

=head1 SYNOPSIS

This module probably provides no value outside of support
for the Cam::Scom module, and thus should probably never
be used directly by any other module or user.

=cut

use fields
  (
   "fh",                ## name used to identify it in Cam::Scom
   "handle",            ## the perl thing that can be manipulated,
                        ##    either a string name, or the FileHandle name.
   "is_object",         ## true if "handle" is an object handle, false o.w.
   "is_fd_only",        ## true if "handle" is not real, this is only an fd
   "fd",                ## the OS fd if it is readable/writable
   "direction",         ## "<", ">", "+", or "*"
   "child_fd",          ## fd number to use in exec'd child (if defined)
   "use",               ## use flag, one of "", "?", "!"
   "term",              ## termination code it got when it stopped
   "read_open",         ## whether filehandle is readable & open for reading
   "write_open",        ## whether filehandle is writable & open for writing
   "read_buf",          ## list of unprocessed input events.
                        ##   Contents has format:
                        ##     socket server:  Cam::Scom::Event objects with
                        ##                      fields "port" (new fh), "addr"
                        ##     messaging:      Cam::Scom::Event objects with
                        ##                      fields "port", "addr", "data"
                        ##     non-messaging:  the data byte strings
   "read_buf_size",     ## total byte size of all data in read_buf
   "read_buf_limit",    ## max allowed size of read buf
   "read_buf_killer",   ## 1 ==> the filehandle will be killed if buffer fills,
                        ##    o.w. reads will be stopped in this case.
   "write_buf",         ## list of unprocessed output
   "write_buf_size",    ## total byte size of all data in write_buf
   "write_buf_limit",   ## max allowed size of write buf
   "write_buf_killer",  ## 1 ==> the filehandle will be killed if buffer fills,
                        ##    o.w. writes will be stopped in this case.
   "write_buf_killmsg", ## a mesage to send via the write handle if killed
   "read_separator",    ## input separator
   "separator_len",     ## length of input separator
   "read_server",       ## 0 ==> not socket server, o.w. next num name.
   "client_base",       ## new base name for socket server clients
   "messaging",         ## value 0 ==> stream (read/write),
                        ##       1 ==> messaging (sendto, recvfrom)
   "close_on_flush",    ## value 1 ==> close-on-flush, 0 ==> o.w.
   "close_on_unreg",    ## value 1 ==> close-on-unregister, 0 ==> o.w.
   "close_on_exec",     ## close_on_exec flag, one of "" or "."
   "read_logfile",      ## If defined, filehandle of read log file
   "write_logfile",     ## If defined, filehandle of write log file
  );

=head1 FUNCTIONS

=head2 new

Returns a new Cam::Scom::Fh object

=cut

sub new
{
    my $self = shift;
    (ref $self) || ($self = fields::new($self));
    return $self;
};

=head2 DESTROY

Destructs a Cam::Scom::Fh object

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

1; # End of Cam::Scom::Fh
