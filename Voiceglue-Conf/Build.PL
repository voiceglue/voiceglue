use strict;
use warnings;
use Module::Build;

my $builder = Module::Build->new(
    module_name         => 'Voiceglue::Conf',
    license             => 'gpl',
    dist_author         => 'Doug Campbell <voiceglue@voiceglue.org>',
    dist_version_from   => 'lib/Voiceglue/Conf.pm',
    build_requires => {
        'Test::More' => 0,
        'Satc' => 0,
    },
    add_to_cleanup      => [ 'Voiceglue-Conf-*' ],
    create_makefile_pl => 'traditional',
);

$builder->create_build_script();
