#!perl -T --            -*-CPerl-*-
#!perl --            -*-CPerl-*-

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

use Fcntl qw(O_NDELAY F_GETFL F_SETFL);
use Test::More qw( no_plan );
use Cam::Scom;

use strict;

##  check_results ($expected_array, $computed_array)
sub check_array_results
{
  my ($computed_array) = shift (@_);
  my ($expected_array) = shift (@_);
  my ($computed_size, $expected_size, $i);

  is (ref ($computed_array), ref ($computed_array), "return type");
  $computed_size = scalar (@$computed_array);
  $expected_size = scalar (@$expected_array);
  is ($computed_size, $expected_size, "array size");
  for ($i = 0; $i < $computed_size; ++$i)
  {
      is ($computed_array->[$i], $expected_array->[$i], "array value [$i]");
  };
};

sub check_mask_to_array_in_c
{
    my ($inputs) = [
		    ["", 0],
		    ["\x00\x00", 16],
		    ["\x01\x80", 16],
		    ["\xFF", 8],
		   ];
    my ($expected) = [
		      [],
		      [],
		      [0, 15],
		      [0, 1, 2, 3, 4, 5, 6, 7],
		     ];
    my ($i, $computed);

    for ($i = 0; $i < scalar (@$inputs); ++$i)
    {
	$computed = Cam::Scom::_mask_to_array_in_c (@{$inputs->[$i]});
	check_array_results ($computed, $expected->[$i]);
    };
};

sub check_fcntl_in_c
{
    my ($fd, $flags, $r);

    open (::FH, ">/tmp/scomaccel_test.remove_me");
    $fd = fileno (::FH);
    is (defined ($fd), 1, "fileno() check");
    $flags = Cam::Scom::_fcntl_3arg_in_c ($fd, F_GETFL, 0);
    is ($flags != -1, 1, "fcntl_3arg_in_c() success from fcntl(F_GETFL)");
    is ($flags & O_NDELAY, 0, "F_GETFL starts with O_NDELAY off");
    $flags |= O_NDELAY;
    $r = Cam::Scom::_fcntl_3arg_in_c ($fd, F_SETFL, $flags);
    is ($r != -1, 1, "fcntl_3arg_in_c() success from fcntl(F_SETFL)");
    $r = Cam::Scom::_fcntl_3arg_in_c ($fd, F_GETFL, 0);
    is ($r != -1, 1, "fcntl_3arg_in_c() success from fcntl(F_GETFL try 2)");
    is ($flags eq $r, 1, "fd flags got set");
    close (::FH);
    unlink ("/tmp/scomaccel_test.remove_me");
};

sub main
{
    check_mask_to_array_in_c();
    check_fcntl_in_c();
};

main();
