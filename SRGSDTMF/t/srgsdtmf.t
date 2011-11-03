#!perl -T --       -*-CPerl-*-

#!/usr/bin/perl
# use lib "/home/soup/doc/projects/voiceglue/dist/trunk/SRGSDTMF/lib";

##  Copyright 2006,2007 Ampersand Inc., and Doug Campbell
##
##  This file is part of SRGSDTMF.
##
##  SRGSDTMF is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  SRGSDTMF is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with SRGSDTMF; if not, see <http://www.gnu.org/licenses/>.

use Test::More tests => 59;

use SRGSDTMF;

#################### Define keyset's ###################

##  Any single digit
$keyset_any1dig = {"type" => "keyset",
		   "keys" => "0123456789\\*#",
		   "fullmatch" => qr/^[0-9\*#]$/so,
		   "lentype" => "fixed",
		   "tags" => {"0" => "zero",
			      "1" => "one",
			      "\\*" => "star"},
		   "min" => 1,
		   "max" => 1,
		  };

##  Any single digit except #
$keyset_any1digbutpound = {"fullmatch" => qr/^[0-9\*]$/so,
			   "lentype" => "fixed",
			   "tags" => {"0" => "zero",
				      "1" => "one",
				      "\\*" => "star"},
			   "min" => 1,
			   "max" => 1,
			  };

##  Only #
$keyset_pound = {"fullmatch" => qr/^[#]$/so,
		 "lentype" => "fixed",
		 "tags" => {},
		 "min" => 1,
		 "max" => 1,
		};

##  Only 9
$keyset_nine = {"fullmatch" => qr/^[9]$/so,
		"lentype" => "fixed",
		"tags" => {},
		"min" => 1,
		"max" => 1,
	       };

##  2-4 numerical digits
$keyset_2to4numbers = {"fullmatch" => qr/^[0-9]{2,4}$/so,
		       "prematch" => qr/^[0-9]{0,4}$/so,
		       "lentype" => "variable",
		       "tags" => {},
		       "min" => 2,
		       "max" => 4,
		      };

##  4 numerical digits
$keyset_4numbers = {"fullmatch" => qr/^[0-9]{4}$/so,
		    "prematch" => qr/^[0-9]{0,4}$/so,
		    "lentype" => "fixed",
		    "tags" => {},
		    "min" => 4,
		    "max" => 4,
		   };

#################### Define keyseq's ###################

##  Any signle digit
$testkeyseq0 = {"min_total" => 1,
		   "max_total" => 1,
		   "has-variable" => 0,
		   "keyset" => [
				$keyset_any1dig,
			       ],
		  };

##  Sequence of any two single digits
$testkeyseq1 = {"min_total" => 2,
		   "max_total" => 2,
		   "has-variable" => 0,
		   "keyset" => [
				$keyset_any1dig,
				$keyset_any1dig,
			       ],
		  };

##  Any signle digit but pound
$testkeyseq2 = {"min_total" => 1,
		   "max_total" => 1,
		   "has-variable" => 0,
		   "keyset" => [
				$keyset_any1digbutpound,
			       ],
		  };

##  Any two signle digits but pound
$testkeyseq3 = {"min_total" => 2,
		   "max_total" => 2,
		   "has-variable" => 0,
		   "keyset" => [
				$keyset_any1digbutpound,
				$keyset_any1digbutpound,
			       ],
		  };

##  Any two signle digits but pound followed by pound
$testkeyseq4 = {"min_total" => 3,
		   "max_total" => 3,
		   "has-variable" => 0,
		   "keyset" => [
				$keyset_any1digbutpound,
				$keyset_any1digbutpound,
				$keyset_pound,
			       ],
		  };

##  2-4 numeric digits
$testkeyseq5 = {"min_total" => 2,
		   "max_total" => 4,
		   "has-variable" => 1,
		   "keyset" => [
				$keyset_2to4numbers,
			       ],
		  };

##  2-4 numeric digits followed by pound
$testkeyseq6 = {"min_total" => 3,
		   "max_total" => 5,
		   "has-variable" => 1,
		   "keyset" => [
				$keyset_2to4numbers,
				$keyset_pound,
			       ],
		  };

##  9 followed by 2-4 numeric digits followed by pound
$testkeyseq7 = {"min_total" => 4,
		   "max_total" => 6,
		   "has-variable" => 1,
		   "keyset" => [
				$keyset_nine,
				$keyset_2to4numbers,
				$keyset_pound,
			       ],
		  };

##  4 numeric digits followed by pound
$testkeyseq8 = {"min_total" => 5,
		   "max_total" => 5,
		   "has-variable" => 0,
		   "keyset" => [
				$keyset_4numbers,
				$keyset_pound,
			       ],
		  };

##  Single digit
$testgrammar_1_digit =
  '<rule id="one_digit" scope="public">
  <one-of>
    <item> 0 </item>
    <item> 1 </item>
    <item> 2 </item>
    <item> 3 </item>
    <item> 4 </item>
    <item> 5 </item>
    <item> 6 </item>
    <item> 7 </item>
    <item> 8 </item>
    <item> 9 </item>
  </one-of>
</rule>
';

##  0-4 digits
$testgrammar_0to4_digits =
  '<rule id="zero_to_four_digits" scope="public">
  <item repeat="0-4">
    <one-of>
      <item> 0 </item>
      <item> 1 </item>
      <item> 2 </item>
      <item> 3 </item>
      <item> 4 </item>
      <item> 5 </item>
      <item> 6 </item>
      <item> 7 </item>
      <item> 8 </item>
      <item> 9 </item>
    </one-of>
  </item>
</rule>
';

##  0-4 digits followed by pound
$testgrammar_0to4_digits_pound =
  '<rule id="zero_to_four_digits_pound" scope="public">
  <item repeat="0-4">
    <one-of>
      <item> 0 </item>
      <item> 1 </item>
      <item> 2 </item>
      <item> 3 </item>
      <item> 4 </item>
      <item> 5 </item>
      <item> 6 </item>
      <item> 7 </item>
      <item> 8 </item>
      <item> 9 </item>
    </one-of>
  </item>
  <item> # </item>
</rule>
';

##  star
$testgrammar_star =
  '<rule id="star" scope="public">
  <item> * </item>
</rule>
';

##  one that had trouble
$trouble_gram =
  '<?xml version=\'1.0\'?>
<grammar xmlns=\'http://www.w3.org/2001/06/grammar\' version=\'1.0\' mode=\'dtmf\' root=\'\' xml:lang=\'en\' xml:base=\'http://kvmrhel01/cgi-bin/ReturnFile\'>
  <rule id="digit" scope="private">
    <one-of><item> 0 </item><item> 1 </item><item> 2 </item><item> 3 </item><item> 4 </item><item> 5 </item><item> 6 </item><item> 7 </item><item> 8 </item><item> 9 </item>
    </one-of>
  </rule>
</grammar>
';

## 0 to 4 digits with optional pound
$testgrammar_0to4_digits_pound_optional =
  'builtin:dtmf/digits?minlength=0;maxlength=4';

##  Safe join - handles undef arguments
sub sjoin
{
    my ($joiner) = shift (@_);
    my (@list) = @_;
    foreach (@list)
    {
	if (ref ($_))
	{
	    $_ = "<OBJ>";
	}
	elsif (! defined ($_))
	{
	    $_ = "";
	};
    };
    return join ($joiner, @list);
};

##  Test any single digit
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq0, "1")),
    "2,one", "check_keyseq (\$testkeyseq0, 1)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq0, "2")),
    "2,", "check_keyseq (\$testkeyseq0, 2)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq0, "23")),
    "-1,", "check_keyseq (\$testkeyseq0, 23)");

##  Test sequence of any two digits
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq1, "1")),
    "0,", "check_keyseq (\$testkeyseq1, 1)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq1, "10")),
    "2,zero", "check_keyseq (\$testkeyseq1, 10)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq1, "109")),
    "-1,", "check_keyseq (\$testkeyseq1, 109)");

##  Test any single digit but pound
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq2, "1")),
    "2,one", "check_keyseq (\$testkeyseq2, 1))");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq2, "10")),
    "-1,", "check_keyseq (\$testkeyseq2, 10)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq2, "#")),
    "-1,", "check_keyseq (\$testkeyseq2, #)");

##  Test any two single digits but pound
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq3, "1")),
    "0,", "check_keyseq (\$testkeyseq3, 1)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq3, "10")),
    "2,zero", "check_keyseq (\$testkeyseq3, 10)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq3, "1#")),
    "-1,", "check_keyseq (\$testkeyseq3, 1#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq3, "#1")),
    "-1,", "check_keyseq (\$testkeyseq3, #1)");

##  Test any two single digits but pound followed by pound
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq4, "1")),
    "0,", "check_keyseq (\$testkeyseq4, 1)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq4, "10#")),
    "2,zero", "check_keyseq (\$testkeyseq4, 10#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq4, "1#0")),
    "-1,", "check_keyseq (\$testkeyseq4, 1#0)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq4, "10#1")),
    "-1,", "check_keyseq (\$testkeyseq4, 10#1)");

##  Test 2-4 numerical digits
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq5, "1")),
    "0,", "check_keyseq (\$testkeyseq5, 1)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq5, "10#")),
    "-1,", "check_keyseq (\$testkeyseq5, 10#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq5, "100")),
    "1,", "check_keyseq (\$testkeyseq5, 100)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq5, "1000")),
    "2,", "check_keyseq (\$testkeyseq5, 100)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq5, "10101")),
    "-1,", "check_keyseq (\$testkeyseq5, 10101)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq5, "*")),
    "-1,", "check_keyseq (\$testkeyseq5, *)");

##  Test 2-4 numberical digits followed by a #
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq6, "1")),
    "0,", "check_keyseq (\$testkeyseq6, 1)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq6, "10#")),
    "2,", "check_keyseq (\$testkeyseq6, 10#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq6, "100")),
    "0,", "check_keyseq (\$testkeyseq6, 100)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq6, "10101#")),
    "-1,", "check_keyseq (\$testkeyseq6, 10101#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq6, "*01#")),
    "-1,", "check_keyseq (\$testkeyseq6, *)");

##  Test 9 followed by 2-4 numberical digits followed by a #
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq7, "9")),
    "0,", "check_keyseq (\$testkeyseq7, 9)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq7, "10#")),
    "-1,", "check_keyseq (\$testkeyseq7, 10#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq7, "9100")),
    "0,", "check_keyseq (\$testkeyseq7, 9100)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq7, "91010#")),
    "2,", "check_keyseq (\$testkeyseq7, 91010#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq7, "910102#")),
    "-1,", "check_keyseq (\$testkeyseq7, 91010#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq7, "901#")),
    "2,", "check_keyseq (\$testkeyseq7, 901#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq7, "90#")),
    "-1,", "check_keyseq (\$testkeyseq7, 90#)");

##  4 numberical digits followed by a #
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq8, "9")),
    "0,", "check_keyseq (\$testkeyseq8, 9)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq8, "10#")),
    "-1,", "check_keyseq (\$testkeyseq8, 10#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq8, "9100")),
    "0,", "check_keyseq (\$testkeyseq8, 9100)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq8, "910101#")),
    "-1,", "check_keyseq (\$testkeyseq8, 910101#)");
is (sjoin (",", SRGSDTMF::check_keyseq_match ($testkeyseq8, "9011#")),
    "2,", "check_keyseq (\$testkeyseq8, 9011#)");

##  Grammar parse tests
($ok, $msg, $rule_1_digit) =
  SRGSDTMF::parse_srgsdtmf_grammar ($testgrammar_1_digit);
is ($ok, 1, "parse_srgsdtmf_grammar (\$testgrammar_1_digit) $msg");
($ok, $msg, $rule_0to4_digits) =
  SRGSDTMF::parse_srgsdtmf_grammar ($testgrammar_0to4_digits);
is ($ok, 1, "parse_srgsdtmf_grammar (\$testgrammar_0to4_digits) $msg");
($ok, $msg, $rule_0to4_digits_pound) =
  SRGSDTMF::parse_srgsdtmf_grammar ($testgrammar_0to4_digits_pound);
is ($ok, 1, "parse_srgsdtmf_grammar (\$testgrammar_0to4digits_pound) $msg");
($ok, $msg, $rule_star) =
  SRGSDTMF::parse_srgsdtmf_grammar ($testgrammar_star);
is ($ok, 1, "parse_srgsdtmf_grammar (\$testgrammar_star) $msg");
($ok, $msg, $rule_0to4_digits_pound_optional) =
  SRGSDTMF::parse_srgsdtmf_grammar ($testgrammar_0to4_digits_pound_optional);
is ($ok, 1, "parse_srgsdtmf_grammar (\$testgrammar_0to4digits_pound_optional) $msg");

##  Grammer match tests
is (sjoin (",", SRGSDTMF::check_rules_match ("1", $rule_1_digit)),
    "2,,<OBJ>", "check_rules_match (1, \$rule_1_digit)");
is (sjoin (",", SRGSDTMF::check_rules_match ("1", $rule_0to4_digits)),
    "1,,<OBJ>", "check_rules_match (1, \$rule_0to4_digits)");
is (sjoin (",", SRGSDTMF::check_rules_match ("1", $rule_0to4_digits_pound)),
    "0,,", "check_rules_match (1, \$rule_0to4_digits_pound)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("1", $rule_0to4_digits_pound_optional)),
    "1,,<OBJ>", "check_rules_match (1, \$rule_0to4_digits_optional)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("1234", $rule_0to4_digits_pound_optional)),
    "1,,<OBJ>", "check_rules_match (1, \$rule_0to4_digits_pound_optional)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("1234#", $rule_0to4_digits_pound_optional)),
    "2,,<OBJ>", "check_rules_match (1, \$rule_0to4_digits_pound_optional)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("55#", $rule_0to4_digits_pound_optional)),
    "2,,<OBJ>", "check_rules_match (1, \$rule_0to4_digits_pound_optional)");
is (sjoin (",", SRGSDTMF::check_rules_match ("1", $rule_star)),
    "-1,,", "check_rules_match (1, \$rule_star)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("1", $rule_0to4_digits_pound, $rule_star)),
    "0,,", "check_rules_match (1, \$rule_0to4_digits_pound, \$rule_star)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("12", $rule_0to4_digits_pound, $rule_star)),
    "0,,", "check_rules_match (12, \$rule_0to4_digits_pound, \$rule_star)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("#", $rule_0to4_digits_pound, $rule_star)),
    "2,,<OBJ>",
    "check_rules_match (#, \$rule_0to4_digits_pound, \$rule_star)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("*", $rule_0to4_digits_pound, $rule_star)),
    "2,,<OBJ>",
    "check_rules_match (*, \$rule_0to4_digits_pound, \$rule_star)");
is (sjoin (",", SRGSDTMF::check_rules_match
	   ("123#", $rule_0to4_digits_pound, $rule_star)),
    "2,,<OBJ>",
    "check_rules_match (123#, \$rule_0to4_digits_pound, \$rule_star)");
is (((($ok, $msg, $_) = SRGSDTMF::parse_srgsdtmf_grammar($trouble_gram)))[0],
    1,
    "check parse of once-trouble grammar");
