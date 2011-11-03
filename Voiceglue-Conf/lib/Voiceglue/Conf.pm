## -*-CPerl-*-

package Voiceglue::Conf;

use Satc;

use Exporter 'import';

use warnings;
use strict;

=head1 NAME

Voiceglue::Conf - Support for reading/writing voiceglue.conf file

=head1 VERSION

Version 0.02

=cut

our $VERSION = '0.02';

=head1 SYNOPSIS

    use Voiceglue::Conf;
    (($ok, $msg, $conf_map) =
        Voiceglue::Conf::parse_voiceglue_conf())[0] || die;
    $conf_map->{"urls"}{"techsupport"} = 100;
    (($ok, $msg) = Voiceglue::Conf::write_voiceglue_conf($conf_map))[0] || die;

=head1 EXPORT

=head1 FUNCTIONS

=head2 ($ok, $msg, $conf_map) = parse_voiceglue_conf ($conf_file_path);

Given a voiceglue.conf file path (defaults to "/etc/voiceglue.conf"),
returns a $conf_map structure that contains:
 ->{"urls"}{<exten>}		-- URL of VXML to load when <exten> is called
 ->{"params"}{<param>}[<index>]	-- values of parameter <param>
 ->{"lines"}[<line#>]		-- raw lines of text from the file
Returns $ok true on success, false on failure and sets $msg to an error msg.
Note that $conf_map may still be populated even if errors are found.

=cut

sub parse_voiceglue_conf
{
    my ($path) = shift (@_);
    my ($ok, $msg, $line, $orig_line, $name, $value, @lines, $fields);
    my ($conf_map) = {"urls" => {}, "params" => {}, "lines" => []};
    my (@delayed_errors) = ();

    defined ($path) || ($path = "/etc/voiceglue.conf");
    open (::Voiceglue::Conf::Fh, $path)
      || return (0, "Cannot open \"$path\" for reading : $!");
    @lines = <::Voiceglue::Conf::Fh>;
    close (::Voiceglue::Conf::Fh);
    $conf_map->{"lines"} = [@lines];

    while (defined ($line = shift (@lines)))
    {
	##  Strip out all sorts of bad stuff like comments & blank lines
	chomp ($line);
	$orig_line = $line;
	$line =~ s/^\s+//s;
	$line =~ s/\s+$//s;
	$line =~ s/^\#.*//s;
	if (length ($line))
	{
	    ($ok, $msg, $fields) = Satc::_parse_SATC_fields ($line);
	    if (! $ok)
	    {
		push (@delayed_errors, 
		      "Cannot parse line in \"$path\": $orig_line");
	    }
	    elsif (scalar (@$fields) == 2)
	    {
		##  A URL line
		$conf_map->{"urls"}{$fields->[0]} = $fields->[1];
	    }
	    elsif ((scalar (@$fields) > 2) && ($fields->[1] eq "="))
	    {
		##  A parameter
		$name = shift (@$fields);
		if (! defined ($conf_map->{"params"}{$name}))
		{
		    $conf_map->{"params"}{$name} = [];
		};
		shift (@$fields);
		push (@{$conf_map->{"params"}{$name}}, @$fields);
	    }
	    else
	    {
		push (@delayed_errors,
		      "Cannot parse line in \"$path\": $orig_line");
	    };
	};
    };

    return ((scalar (@delayed_errors)) ? 0 : 1,
	    join (", ", @delayed_errors),
	    $conf_map);
};

=head2 ($ok, $msg) = write_voiceglue_conf ($conf_map, $conf_file_path);

Given a voiceglue $conf_map as obtained from parse_voiceglue_conf()
and a voiceglue.conf file path (defaults to "/etc/voiceglue.conf"),
writes a new voiceglue.conf file with $conf_map's info.
Returns $ok true on success, false on failure and sets $msg to an error msg.

=cut

sub write_voiceglue_conf
{
    my ($conf_map) = shift (@_);
    my ($path) = shift (@_);
    my ($url, $param, $value);

    defined ($conf_map)
      || return (0, "usage: write_voiceglue_conf(\$conf_map)");
    defined ($path) || ($path = "/etc/voiceglue.conf");
    $conf_map->{"lines"} = [];
    foreach $url (keys (%{$conf_map->{"urls"}}))
    {
	push (@{$conf_map->{"lines"}}, $url . " " .
	      encode_field_minimally ($conf_map->{"urls"}{$url}) . "\n");
    };
    foreach $param (keys (%{$conf_map->{"params"}}))
    {
	foreach $value (@{$conf_map->{"params"}{$param}})
	{
	    push (@{$conf_map->{"lines"}}, $param . " = " .
		  encode_field_minimally ($value) . "\n");
	};
    };
    open (::FH, ">$path")
      && (print ::FH (@{$conf_map->{"lines"}}))
	&& close (::FH)
	  || return (0, "**** Failed writing $path: $!");
    return (1, "");
};

=head2 $encoded = encode_field_minimally ($string)

Returns SATC-encoded version of $string,
trying to only use encoding chars when necessary.

=cut

##  $encoded = encode_field_minimally ($string)
##    -- Returns SATC-encoded version of $string,
##       trying to only use encoding chars when necessary.
sub encode_field_minimally
{
    my ($string) = shift (@_);
    my ($encoded) = Satc::_encode_convert_escape($string);
    my ($unencoded) = substr($encoded, 1);
    $unencoded = substr ($unencoded, 0, length($unencoded)-1);
    length ($string) || return ($encoded);
    ($unencoded eq $string) && return ($string);
    return ($encoded);
}

=head1 AUTHOR

Doug Campbell, C<< <voiceglue at voiceglue.org> >>

=head1 SUPPORT

You can find documentation for this module with the perldoc command.

    perldoc Voiceglue::Conf

You can also look for information at:

=over 4

=item * VoiceGlue Home Page

L<http://voiceglue.org>

=back

=head1 COPYRIGHT & LICENSE

Copyright 2009-2010 Ampersand Inc., and Doug Campbell

This file is part of Voiceglue::Conf.

Voiceglue::Conf is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Voiceglue::Conf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Voiceglue::Conf; if not, see <http://www.gnu.org/licenses/>.

=cut

1; # End of Voiceglue::Conf
