use strict;
use warnings;
use Module::Build;

my $builder = Module::Build->new(
    module_name         => 'Cam::Scom',
    license             => 'gpl',
    dist_author         => 'Doug Campbell <doug.campbell@pobox.com>',
    dist_version_from   => 'lib/Cam/Scom.pm',
    build_requires => {
        'Test::More' => 0,
	'BSD::Resource' => 0,
    },
    add_to_cleanup      => [ 'Cam-Scom-*' ],
);

$builder->create_build_script();
