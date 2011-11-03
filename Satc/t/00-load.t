#!perl -T

use Test::More tests => 1;

BEGIN {
	use_ok( 'Satc' );
}

diag( "Testing Satc $Satc::VERSION, Perl $], $^X" );
