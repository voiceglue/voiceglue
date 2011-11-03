use strict;
use warnings;
use Module::Build;

my $builder = Module::Build->new(
    module_name         => 'Satc',
    license             => 'gpl',
    dist_author         => 'Doug Campbell <soup@ampersand.com>',
    dist_version_from   => 'lib/Satc.pm',
    build_requires => {
        'Test::More' => 0,
    },
    add_to_cleanup      => [ 'Satc-*' ],
);

$builder->create_build_script();
