#!perl -T

use Test::More tests => 1;

BEGIN {
	use_ok( 'Voiceglue::Conf' );
}

diag( "Testing Voiceglue::Conf $Voiceglue::Conf::VERSION, Perl $], $^X" );
