#!perl -T

use Test::More tests => 1;

BEGIN {
	use_ok( 'SRGSDTMF' );
}

diag( "Testing SRGSDTMF $SRGSDTMF::VERSION, Perl $], $^X" );
