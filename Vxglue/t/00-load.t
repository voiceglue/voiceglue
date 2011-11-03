#!perl -T

use Test::More tests => 1;

BEGIN {
	use_ok( 'Vxglue' );
}

diag( "Testing Vxglue $Vxglue::VERSION, Perl $], $^X" );
