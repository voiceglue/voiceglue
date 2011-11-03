#               -*-CPerl-*-
use strict;
use warnings;
use Module::Build;

my $jscflags = $::ENV{"JSCFLAGS"};
my $jslibflags = $::ENV{"JSLIBFLAGS"};
defined ($jscflags) && length ($jscflags)
  && defined ($jslibflags) && length ($jslibflags)
  || die ("Must set env vars JSCFLAGS and JSLIBFLAGS");

my $builder = Module::Build->new(
    module_name         => 'Vxglue',
    license             => 'gpl',
    dist_author         => 'Doug Campbell <soup@ampersand.com>',
    dist_version_from   => 'lib/Vxglue.pm',
    extra_compiler_flags  => $jscflags,
    extra_linker_flags  => "-lSBjsiD -lSBtrdD -lSBcharD -lSBlogD -lSBlogMapperD -lSBcacheD -lSBinetD -lVXIvalueD -lVXID -lVXItelD -lVXIclientD -lVXIpromptD -lVXIrecD -lvglue " . $jslibflags . " -lstdc++ -lpthread -ldl",
    build_requires => {
        'Test::More' => 0,
    },
    add_to_cleanup      => [ 'Vxglue-*' ],
);

$builder->create_build_script();
