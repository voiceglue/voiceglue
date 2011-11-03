#!perl -T --                 -*-CPerl-*-

##  Copyright (C) 2006,2007 Ampersand Inc., and Doug Campbell
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

use Test::More tests => 26;
use Satc;

##  Test data
$to_encode = "\xbe" . "f\"\'\no\\o";
$result_encode = "\"\\xbef\\\"\\\'\\no\\\\o\"";
$to_decode = "one '\\'two\\' \\\"2\\\"' \x2b three";
@result_decode = ("one", "'two' \"2\"", "+", "three");
$testmsg = {
	"msgtype" => Satc::PLAY,
	Satc::CALLID => 5491,
	"files" => ["This is a really bad filename.wav",
		    " This isn't even valid\ton some systems\n "],
	"stopkeys" => "",
       };
$testmsg_description = "play callid=5491 files=(\"This is a really bad filename.wav\", \" This isn't even valid\\ton some systems\\n \") stopkeys=\"\"";
$testmsg2 = {
	"msgtype" => Satc::PLAY,
	Satc::CALLID => 5491,
	"files" => ["simple"],
	"stopkeys" => "0123456789*#",
       };
$testmsg2_description = "play callid=5491 files=(\"simple\") stopkeys=\"0123456789*#\"";
$testmsg3 = {
	     Satc::MSGTYPE => Satc::PLAYED,
	     Satc::CALLID => 5491,
	     Satc::STATUS => 0,
	     Satc::MSG => "",
	     Satc::REASON => Satc::PLAYEDFILE_REASON_DTMF_PRESSED,
       };
$testmsg3_description = "played callid=5491 status=0 msg=\"\" reason=DTMF-pressed";

##  Test internal funcs
$result = Satc::_encode_convert_escape ($to_encode);
is ($result, $result_encode, "_encode_convert_escape()");
($ok, $msg, $result) = Satc::_parse_SATC_fields ($to_decode);
is ($ok, 1, "success from _parse_SATC_fields: " . (defined ($msg) ? $msg : ""));
for ($i = 0; $i < scalar (@result_decode); ++$i)
{
    is ($result->[$i], $result_decode[$i], "_parse_SATC_fields on field $i");
};

##  Test published funcs

##  Test multi-message extraction from binary string
$obj = new Satc;
$multimsg = $to_decode . "\r\n" . $to_decode . "\n";
$extracted = $obj->extract_message (\$multimsg);
is ($extracted, $to_decode . "\r\n", "extract_message #1");
$extracted = $obj->extract_message (\$multimsg);
is ($extracted, $to_decode . "\n", "extract_message #2");

##  Test encode/decode cycle on $testmsg
$string = $obj->encode ($testmsg);
$result = $obj->decode ($string);
foreach $field (keys (%$testmsg))
{
    if (ref ($testmsg->{$field}))
    {
	is (Satc::_dump_data ($testmsg->{$field}),
	    Satc::_dump_data ($result->{$field}));
    }
    else
    {
	is ($testmsg->{$field}, $result->{$field},
	    "endoce/decode cycle on testmsg field \"$field\"");
    };
};

##  Test encode/decode cycle on $testmsg2
$string = $obj->encode ($testmsg2);
$result = $obj->decode ($string);
foreach $field (keys (%$testmsg2))
{
    if (ref ($testmsg2->{$field}))
    {
	is (Satc::_dump_data ($testmsg2->{$field}),
	    Satc::_dump_data ($result->{$field}));
    }
    else
    {
	is ($testmsg2->{$field}, $result->{$field},
	    "endoce/decode cycle on testmsg2 field \"$field\"");
    };
};

is ($obj->describe_msgtype ($result->{"msgtype"}), "play", "describe_msgtype");

##  Test encode/decode cycle on $testmsg3
$string = $obj->encode ($testmsg3);
$result = $obj->decode ($string);
foreach $field (keys (%$testmsg3))
{
    if (ref ($testmsg3->{$field}))
    {
	is (Satc::_dump_data ($testmsg3->{$field}),
	    Satc::_dump_data ($result->{$field}));
    }
    else
    {
	is ($testmsg3->{$field}, $result->{$field},
	    "endoce/decode cycle on testmsg3 field \"$field\"");
    };
};

##  Verify magic_byte
is ($obj->magic_byte(), "s", "magic_byte");

##  Test describe_msg
$descr = $obj->describe_msg ($testmsg);
is ($descr, $testmsg_description, "describe_msg()");
$descr = $obj->describe_msg ($testmsg2);
is ($descr, $testmsg2_description, "describe_msg()");
$descr = $obj->describe_msg ($testmsg3);
is ($descr, $testmsg3_description, "describe_msg()");
