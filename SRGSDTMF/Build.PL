use strict;
use warnings;
use Module::Build;

my $builder = Module::Build->new(
    module_name         => 'SRGSDTMF',
    license             => 'gpl',
    dist_author         => 'Doug Campbell <d.campbell@ampersand.com>',
    dist_version_from   => 'lib/SRGSDTMF.pm',
    build_requires => {
        'Test::More' => 0,
	'XML::LibXML' => 0,
    },
    add_to_cleanup      => [ 'SRGSDTMF-*' ],
);

$builder->create_build_script();
