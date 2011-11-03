#!perl -T

use Test::More tests => 3;

BEGIN {
	use_ok( 'Cam::Scom' );
	use_ok( 'Cam::Scom::Event' );
	use_ok( 'Cam::Scom::Fh' );
}

diag( "Testing Cam::Scom $Cam::Scom::VERSION, Perl $], $^X" );
