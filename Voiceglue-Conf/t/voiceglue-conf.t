#!perl -T --       -*-CPerl-*-
#!/usr/bin/perl
#use lib "/home/soup/doc/projects/voiceglue/dist/trunk/Voiceglue-Conf/blib/lib";

##  Copyright 2009-2010 Ampersand Inc., and Doug Campbell
##
##  This file is part of Voiceglue::Conf.
##
##  Voiceglue::Conf is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  Voiceglue::Conf is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with Voiceglue::Conf; if not, see <http://www.gnu.org/licenses/>.

use Test::More tests => 10;

use Voiceglue::Conf;

$origfile = "# Test voiceglue.conf file

  #  Start of URL mappings
2 file:///var/tmp/voiceglue_0.10beta1_test/vxml/conf_file.vxml
 3 http://localhost/vg_test_www/vxml/conf_web.vxml   

# Start of params
ast_sound_dir = /usr/share/asterisk/sounds
  count = 'one 2 three'   
list = one 2 three
count = 4
";

open (::FH, ">voiceglue.conf")
  && (print ::FH ($origfile))
  && close (::FH)
  || die ("Cannot write voiceglue.conf: $!");

($ok, $msg, $conf_map) =
  Voiceglue::Conf::parse_voiceglue_conf ("voiceglue.conf");

unlink ("voiceglue.conf");

is ($ok, 1, "parse_voiceglue_conf() ok result");
is ($msg, "", "parse_voiceglue_conf() msg result");
is ($conf_map->{"urls"}{"2"},
    "file:///var/tmp/voiceglue_0.10beta1_test/vxml/conf_file.vxml",
    "url map of 2");
is ($conf_map->{"urls"}{"3"},
    "http://localhost/vg_test_www/vxml/conf_web.vxml",
    "url map of 3");
is ($conf_map->{"params"}{"ast_sound_dir"}[0],
    "/usr/share/asterisk/sounds",
    "param ast_sound_dir");
is (join (",", @{$conf_map->{"params"}{"count"}}),
    "one 2 three,4",
    "param count");
is ($conf_map->{"params"}{"list"}[0],
    "one",
    "param list[0]");
is ($conf_map->{"params"}{"list"}[1],
    "2",
    "param list[1]");
is ($conf_map->{"params"}{"list"}[2],
    "three",
    "param list[2]");
is (join ("", @{$conf_map->{"lines"}}),
    $origfile,
    "lines of file");
