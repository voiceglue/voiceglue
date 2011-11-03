#!/usr/bin/perl --
use lib "/home/soup/doc/projects/voiceglue/dist/trunk/Satc/Satc-0.12/blib";

#!perl -T --                 -*-CPerl-*-

##  Copyright (C) 2006-2011 Ampersand Inc., and Doug Campbell
##
##  This file is part of Satc.
##
##  Satc is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  Satc is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with Satc; if not, see <http://www.gnu.org/licenses/>.

use Test::More tests => 30;
use Satc;

$encoded_multi = "one tw\"o t\"wo '3 3 3 '  fou\\x09 'five\\'s' ' six\\n '";
$decoded = ["one", "two two", "3 3 3 ", "fou\t", "five's", " six\n "];
$encoded = ["\"one\"", "\"two two\"", "\"3 3 3 \"", "\"fou\\x09\"", "\"five\\'s\"", "\" six\\n \""];

##  Test decoding
($ok, $msg, $result) = Satc::_parse_SATC_fields ($encoded_multi);
is ($ok, 1, "return status from _parse_SATC_fields()");
is ($msg, undef, "error message from _parse_SATC_fields()");
is (scalar (@$result), scalar (@$decoded),
    "_parse_SATC_fields() field count");
for ($i = 0; $i < scalar (@$decoded); ++$i)
{
    is ($result->[$i], $decoded->[$i], "_parse_SATC_fields() field $i");
};

##  Test encoding
for ($i = 0; $i < scalar (@$decoded); ++$i)
{
    $result = Satc::_escape_SATC_string ($decoded->[$i]);
    is ($result, $encoded->[$i], "_escape_SATC_string() field $i");
};

##  Test multi-step encode/decode
$single_encoded = "\"one\"=1 \"two\"=\"22\" \"three\"=\"t h r e e\"";
$double_encoded = "\"\\\"one\\\"=1 \\\"two\\\"=\\\"22\\\" \\\"three\\\"=\\\"t h r e e\\\"\"";
$decoded = ["one=1", "two=22", "three=t h r e e"];
$result = Satc::_escape_SATC_string ($single_encoded);
is ($result, $double_encoded, "_escape_SATC_string(single_encoded)");
($ok, $msg, $result) = Satc::_parse_SATC_fields ($double_encoded);
is ($ok, 1, "return status from _parse_SATC_fields(double_encoded)");
is ($msg, undef, "error message from _parse_SATC_fields(double_encoded)");
is (scalar (@$result), 1, "_parse_SATC_fields (double_encoded) array size");
is ($result->[0], $single_encoded, "_parse_SATC_fields (double_encoded)");
($ok, $msg, $result) = Satc::_parse_SATC_fields ($single_encoded);
is ($ok, 1, "return status from _parse_SATC_fields(single_encoded)");
is ($msg, undef, "error message from _parse_SATC_fields(single_encoded)");
is (scalar (@$result), 3, "_parse_SATC_fields (single_encoded) array size");
is ($result->[0], $decoded->[0], "_parse_SATC_fields (single_encoded) [0]");
is ($result->[1], $decoded->[1], "_parse_SATC_fields (single_encoded) [1]");
is ($result->[2], $decoded->[2], "_parse_SATC_fields (single_encoded) [2]");

##  Test hashify
is (Satc::hashify ("one", 3, 1), "one", "hashify() with no suffix");
is (Satc::hashify ("onetwothree", 0, 1), "VDhmoRrPoV4", "hashify() with no prefix");
is (Satc::hashify ("onetwothreefour", 3, 1), "oneOPCtaXaQZJ4", "hashify() prefix and suffix");
is (Satc::hashify ("four score and seven years ago our fathers did something or other", 10, 3), "four scoremSiDB3TD8dC38RWRL6pV1C3KxnMvcpJ4C", "hashify() big");
