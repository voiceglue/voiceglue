## -*-CPerl-*-

package SRGSDTMF;

use XML::LibXML::Common;    ##  Gets constant definitions
use XML::LibXML;

use Exporter 'import';

use warnings;
use strict;

=head1 NAME

SRGSDTMF - An SRGS DTMF partial parser

=head1 VERSION

Version 0.01

=cut

our $VERSION = '0.01';

$SRGSDTMF::Type_to_rank = {"keyleaf" => 1,
			   "keyset" => 2,
			   "keyseq" => 3,
			   "rule" => 4};
$SRGSDTMF::Rank_to_type = [undef,
			   "keyleaf",
			   "keyset",
			   "keyseq",
			   "rule"];

$SRGSDTMF::MaxVarDigits = 256;

=head1 SYNOPSIS

    use SRGSDTMF;

=head1 EXPORT

=head1 FUNCTIONS

=head2 ($match, $interp) = check_keyseq_match ($keyseq, $input);

Given a $keyseq and keys that were $input, returns $match:
 -1 = guaranteed failure for this and all future input
  0 = partial match, could fully match with future input
  1 = full match, further input could also match
  2 = full match, no further input could match
and $interp the semantic interpretation (if any).
A keyseq is data defining a valid sequence of keys:
 ->{"type"}         -- "keyseq"
 ->{"min_total"}    -- total minimum digits
 ->{"max_total"}    -- total maximum digits
 ->{"has-variable"} -- whether has a "variable" type in "keyset"
 ->{"keyset"}[<index>]   -- sequence of keyset's, partial key sequence matches
                    {"type"}         -- "keyset"
                    {"keys"}         -- string of valid key choices (if max==1)
                    {"fullmatch"}    -- regex matching whole pattern
                    {"prematch"}     -- regex matching prefix (only for max>1)
	            {"tags"}{<key>}  -- tag for each key (if defined)
                    {"lentype"}      -- either "fixed" or "variable" length
                    {"min"}          -- minimum length
                    {"max"}          -- maximum length (same as min for fixed)

=cut

sub check_keyseq_match
{
    my ($keyseq) = shift (@_);
    my ($input) = shift (@_);
    my ($num_components, $components, $keyset, $interp, $orig_end_input);
    my ($front_index, $back_index, $len, $regex, $match, $key, $vardigmax);
    my ($digits_len);
    my ($failed_end_match) = 0;
    my ($last_chance) = 0;
    my ($is_suffix) = 0;

    ##  Check for simple case of oversize input first
    $len = $keyseq->{"max_total"};
    (length ($input) > $len) && return (-1, undef);
    $last_chance = (length ($input) == $len);

    $components = $keyseq->{"keyset"};
    $num_components = scalar (@$components);

    ##  Match against the fixed-length beginning of the sequence
    $front_index = 0;
    while ($num_components &&
	   ($components->[$front_index]{"lentype"} eq "fixed") &&
	   length ($input))
    {
	$keyset = $components->[$front_index];
	$len = $keyset->{"min"};
	if ($len > length ($input))
	{
	    ##  Look for prefix match if any defined
	    if (defined ($regex = $keyset->{"prematch"}) &&
		($input !~ $regex))
	    {
		return (-1, undef);
	    };
	    return (0, undef);
	};
	##  Look for exact match
	$regex = $keyset->{"fullmatch"};
	$key = substr ($input, 0, $len);
	if ($key !~ $regex)
	{
	    return (-1, undef);
	};
	if (defined ($keyset->{"tags"}{$key}))
	{
	    $interp = $keyset->{"tags"}{$key};
	};
	substr ($input, 0, $len, "");
	--$num_components;
	++$front_index;
    };
    if (! $num_components)
    {
	return (2, $interp);
    };
    if (! length ($input))
    {
	return (0, undef);
    };

    ##  There has to be a variable length portion at this point,
    ##  indexed by $front_index

    ##  Match against any fixed end of input
    $orig_end_input = $input;
    $back_index = scalar (@$components) - 1;
    while (($back_index > $front_index) &&
	   length ($input) &&
	   (! $failed_end_match))
    {
	##  Check for match at end
	$is_suffix = 1;
	$keyset = $components->[$back_index];
	$len = $keyset->{"min"};
	if ($len > length ($input))
	{
	    ##  Not enough chars yet
	    $failed_end_match = 1;
	    $input = $orig_end_input;
	    next;
	};
	$key = substr ($input, -$len, $len);
	$regex = $keyset->{"fullmatch"};
	if ($key !~ $regex)
	{
	    $last_chance && return (-1, undef);
	    $failed_end_match = 1;
	    $input = $orig_end_input;
	    next;
	};
	if (defined ($keyset->{"tags"}{$key}))
	{
	    $interp = $keyset->{"tags"}{$key};
	};
	substr ($input, -$len, $len, "");
	--$num_components;
	--$back_index;
    };

    ##  Finally, match against the variable portion
    $keyset = $components->[$front_index];
    $len = $keyset->{"min"};
    if (length ($input) < $len)
    {
	$is_suffix && (! $failed_end_match) && return (-1, undef);
	##  Look for prefix match if defined
	if (defined ($regex = $keyset->{"prematch"}) &&
	    ($input !~ $regex))
	{
	    return (-1, undef);
	};
	return (0, undef);
    };
    ##  Look for full match
    $regex = $keyset->{"fullmatch"};
    if ($input =~ $regex)
    {
	##  Got a full match
	defined ($keyset->{"tags"}{$input}) &&
	  ($interp = $keyset->{"tags"}{$input});
	if (! $failed_end_match)
	{
	    ##  Special case: check for vardigmax case of variable digits
	    if (defined ($vardigmax = $keyset->{"vardigmax"}))
	    {
		$digits_len = length ($input);
		if ($input =~ /#$/)
		{
		    ##  "#" entered
		    --$digits_len;
		    if ($digits_len > $vardigmax)
		    {
			##  Failure
			return (-1, $interp);
		    };
		    ##  Final Success
		    return (2, $interp);
		}
		else
		{
		    ##  No "#" entered
		    if ($digits_len > $vardigmax)
		    {
			##  Eventual failure, but allow further input
			return (0, $interp);
		    };
		    ##  Current success
		    return (1, $interp);
		};
	    };

	    ##  Normal case
	    if ($is_suffix || $last_chance ||
		(defined ($regex = $keyset->{"maxmatch"}) &&
		 ($input =~ $regex)))
	    {
		return (2, $interp);
	    }
	    else
	    {
		return (1, $interp);
	    };
	};
    };
    $last_chance && return (-1, undef);
    $is_suffix || return (-1, undef);
    $failed_end_match || return (-1, undef);
    return (0, undef);
};

##  ($ok, $msg, $keyset) = _apply_repeatitem_to_keyset ($keyset, $element)
##    -- Adds repeat {$min,$max} to $keyset
sub _apply_repeatitem_to_keyset
{
    my ($keyset) = shift (@_);
    my ($element) = shift (@_);
    my ($key, $repeat_spec, $min, $max);

    if (! $element->hasAttribute ("repeat"))
    {
	return (0, "Cannot have nested <item>s without repeat attr");
    };

    ##  Repeats can only refer to single fixed keyset's
    ($keyset->{"lentype"} eq "fixed")
      || return (0, "Cannot apply repeats recursively");
    defined ($key = $keyset->{"keys"})
      || return (0, "Cannot apply repeats to multi-key sequences");

    $repeat_spec = $element->getAttribute ("repeat");
    if ($repeat_spec =~ /^(\d*)-(\d*)$/)
    {
	$min = $1;
	$max = $2;
    }
    else
    {
	return (0, "Invalid \"repeat\" attr \"" . $repeat_spec . "\"");
    };
    (defined ($min) && length ($min)) || ($min = 0);
    $min = $min + 0;
    (defined ($max) && length ($max))
      || return (0, "Invalid \"repeat\" attr, must specify max: \"" .
		 $repeat_spec . "\"");
    $max = $max + 0;
    $keyset->{"fullmatch"} = qr/^[$key]{$min,$max}$/s;
    $keyset->{"prematch"} = qr/^[$key]{0,$max}$/s;
    $keyset->{"lentype"} = "variable";
    $keyset->{"min"} = $min;
    $keyset->{"max"} = $max;
    return (1, "", $keyset);
};

##  ($ok, $msg, $item) = _parse_leaf ($element)
##    -- Returns result from parsing of leaf $element.
##       $item could be either a keyleaf or a keyset.
sub _parse_leaf
{
    my ($element) = shift (@_);
    my ($tag_name, $key, $tag, $len, $child, @children);

    ## I am a leaf, so I must be an <item>
    $tag_name = $element->tagName();
    if ($tag_name ne "item")
    {
	return (0, "Leaf elements must be items, not " .
		$tag_name);
    };
    ##  ZZZZZZ item repeats???
    ##  Look for all text and <tag> children to define the key
    @children = $element->childNodes();
    $key = "";
    $tag = "";
    foreach $child (@children)
    {
	if ($child->nodeType() == TEXT_NODE)
	{
	    $key .= $child->data();
	}
	elsif (($child->nodeType() == ELEMENT_NODE) &&
	       ($child->tagName() eq "tag"))
	{
	    $tag = $child->textContent();
	};
    };
    $key =~ s/\s+//gs;
    $tag =~ s/\s+//gs;

    if (length ($key) == 0)
    {
	##  Empty item
	return (1, "", {"type" => "keyleaf",
			"key" => $key,
			"tag" => $tag});
    };

    if (length ($key) == 1)
    {
	##  Standard single-key item
	$key =~ s/\*/\\\*/g;
	return (1, "", {"type" => "keyleaf",
			"key" => $key,
			"tag" => $tag});
    };

    ##  Multi-key sequence must be handled as a keyset
    $len = length ($key);
    $key =~ s/\*/\\\*/g;
    return (1, "", {"type" => "keyset",
		    "fullmatch" => qr/^$key$/s,
		    "tags" => (length ($tag) ?
			       {$key => $tag} : {}),
		    "lentype" => "fixed",
		    "min" => $len,
		    "max" => $len,
		   });
};

##  ($ok, $msg, $keyset) = _keyleafs_to_keyset (@keyleafs)
##    -- Returns the $keyset from combining @keyleafs
sub _keyleafs_to_keyset
{
    my (@keyleafs) = @_;
    my ($key, $tag_map);

    ##  Merge up the keyleafs into a keyset
    $key = join ("", map ($_->{"key"}, @keyleafs));
    $tag_map = {};
    grep ($tag_map->{$_->{"key"}} = $_->{"tag"}, @keyleafs);
    return (1, "", {"type" => "keyset",
		    "keys" => $key,
		    "fullmatch" => qr/^[$key]$/s,
		    "tags" => $tag_map,
		    "lentype" => "fixed",
		    "min" => 1,
		    "max" => 1,
		   });
};

##  ($ok, $msg, $keyseq) = _keyset_to_keyseq ($keyset);
##    -- Returns $keyseq that is a sequence of the single $keyset
sub _keyset_to_keyseq
{
    my ($keyset) = shift (@_);
    my ($result);

    $result = {"type" => "keyseq",
	       "min_total" => $keyset->{"min"},
	       "max_total" => $keyset->{"max"},
	       "has-variable" => ($keyset->{"lentype"} eq "variable"),
	       "keyset" => [ $keyset ],
	      };
    return (1, "", $result);
};

##  ($ok, $msg, $keyseq) = _merge_keyseqs (@keyseqs);
##    -- Returns $keyseq that is the sequence concatenation of the @keyseqs
sub _merge_keyseqs
{
    my (@keyseqs) = @_;
    my ($keyseq, $min, $max, $has_variable, $keysets);

    $has_variable = 0;
    $min = 0;
    $max = 0;
    $keysets = [];
    foreach $keyseq (@keyseqs)
    {
	$min += $keyseq->{"min_total"};
	$max += $keyseq->{"max_total"};
	if ($keyseq->{"has-variable"})
	{
	    if ($has_variable)
	    {
		return (0, "Cannot have more than 1 " .
			"variable length (repeat) keyset in a keyseq");
	    };
	    $has_variable = 1;
	};
	push (@$keysets, @{$keyseq->{"keyset"}});
    };

    return (1, "", {"type" => "keyseq",
		    "min_total" => $min,
		    "max_total" => $max,
		    "has-variable" => $has_variable,
		    "keyset" => $keysets});
};

##  ($ok, $msg, $keyseq) = _keyseqs_to_rule (@keyseqs);
##    -- Returns $keyseq that is the sequence of the @keyseqs
sub _keyseqs_to_rule
{
    my (@keyseqs) = @_;

    return (1, "", {"type" => "rule",
		    "keyseq" => [ @keyseqs ]});
};

##  ($ok, $msg, $rule) = _merge_rules (@rules);
##    -- Returns $rule that is the disjunction of the @rules
sub _merge_rules
{
    my (@rules) = @_;
    my ($rule, $keyseqlist);

    $keyseqlist = [];
    foreach $rule (@rules)
    {
	push (@$keyseqlist, @{$rule->{"keyseq"}});
    };
    return (1, "", {"type" => "rule",
		    "keyseq" => $keyseqlist});
};

##  ($ok, $msg, $result) = _promote ($from_item, $to_type)
##    -- Given $from_item that is a keyleaf, keyset, keyseq, or rule,
##       promotes it to a $to_type.
sub _promote
{
    my ($item) = shift (@_);
    my ($to_type) = shift (@_);
    my ($ok, $msg, $from_type, $from_rank, $to_rank, $result);

    $from_rank = $SRGSDTMF::Type_to_rank->{$item->{"type"}};
    $to_rank = $SRGSDTMF::Type_to_rank->{$to_type};
    ($from_rank == $to_rank) && return (1, "", $item);
    ($from_rank > $to_rank) &&
      return (0, "Cannot promote from \"" . $item->{"type"} . "\" to \""
	      . $to_type . ": reverse order");

    ##  Recurse on lower promotions
    if ($from_rank + 1 < $to_rank)
    {
	(($ok, $msg, $result) =
	 _promote ($item, $SRGSDTMF::Rank_to_type->[$to_rank - 1]))[0]
	   || return (0, $msg);
	$item = $result;
	$from_rank = $to_rank - 1;
    };

    ##  Handle single-level promotion
    $from_type = $item->{"type"};
    if ($from_type eq "keyleaf")
    {
	##  Promote from keyleaf to keyset
	return _keyleafs_to_keyset ($item);
    }
    elsif ($from_type eq "keyset")
    {
	##  Promote from keyset to keyseq
	return _keyset_to_keyseq ($item);
    }
    elsif ($from_type eq "keyseq")
    {
	##  Promote from keyseq to rule
	return _keyseqs_to_rule ($item);
    }
    else
    {
	return (0, "Cannot promote from type \"" . $from_type .
		"\" : Unrecognized type");
    };
};

##  ($ok, $msg) = _promote_all ($type, @$items)
##    -- Promotes all in @$items to $type
sub _promote_all
{
    my ($type) = shift (@_);
    my ($items) = shift (@_);
    my ($index, $len, $ok, $msg, $result);

    $len = scalar (@$items);
    for ($index = 0; $index < $len; ++$index)
    {
	(($ok, $msg, $result) = _promote ($items->[$index], $type))[0]
	  || return (0, $msg);
	$items->[$index] = $result;
    };

    return (1, "");
};

##  ($ok, $msg, $result) = _rec_dec_parse ($element);
##    -- Given XML $element, returns one of the following $result:
##          - {"type" => "keyleaf"      -- one key from an <item>
##             "key" =>   -- the key (one of "0" - "9", "\*", "#")
##             "tag" =>   -- the tag if any
##            }
##          - keyset     (defined above)
##          - keyseq  (defined above)
##          - {"type" => "rule"         -- a full DTMF-recognizing rule
##             "keyseq" => [<index>] -- list of or-ed keyseq rules
##            }
sub _rec_dec_parse
{
    my ($element) = shift (@_);
    my ($tag_name, @children, @child_elements, $child_results);
    my ($key, $tag, $len, $child, $result, $result_type);
    my ($keyset, $keysetleaf, @results);
    my ($ok, $msg, $max_rank, $rank, $func);

    ##  Find child elements of this element
    @children = $element->childNodes();
    @child_elements = grep ((($_->nodeType == ELEMENT_NODE) &&
			     ($_->tagName() ne "tag")), @children);

    ##  Handle leaf case
    if (! scalar (@child_elements))
    {
	return _parse_leaf ($element);
    };

    ##  Get results from children
    $child_results = [];
    foreach $child (@child_elements)
    {
	($ok, $msg, $result) = _rec_dec_parse ($child);
	$ok || return ($ok, $msg);
	push (@$child_results, $result);
    };

    ##  Make sure child result types match
    ##  by promoting each to highest level using ranking:
    ##    keyleaf -> keyset -> keyseq -> rule
    ##  First, find the highest rank
    $max_rank = 0;
    foreach $result (@$child_results)
    {
	$result_type = $result->{"type"};
	($SRGSDTMF::Type_to_rank->{$result_type} > $max_rank)
	  && ($max_rank = $SRGSDTMF::Type_to_rank->{$result_type})
    };
    ##  Now promote all children to the highest rank
    $result_type = $SRGSDTMF::Rank_to_type->[$max_rank];
    (($ok, $msg) = _promote_all ($result_type, $child_results))[0]
      || return (0, $msg);

    ##  Evaluate this element
    $tag_name = $element->tagName();
    if ($tag_name eq "one-of")
    {
	##  There are two different behaviors of one-of:
	##  1.  Children are keyleafs, merge into a keyset
	if ($result_type eq "keyleaf")
	{
	    return _keyleafs_to_keyset (@$child_results);
	};
	##  2.  Children are keysets or higher, merge into a rule
	if ($result_type ne "rule")
	{
	    (($ok, $msg) = _promote_all ("rule", $child_results))[0]
	      || return (0, $msg);
	    $result_type = "rule";
	};
	return _merge_rules (@$child_results);
    }
    elsif (($tag_name eq "item") && ($element->hasAttribute ("repeat")))
    {
	##  A repeat specifier is only valid on a single keyset
	if ($result_type eq "keyleaf")
	{
	    (($ok, $msg) = _promote_all ("keyset", $child_results))[0]
	      || return (0, $msg);
	    $result_type = "keyset";
	};
	($result_type eq "keyset")
	  || return (0, "Cannot apply repeats to sequences " .
		     "or non-leaf disjunctions");
	(scalar (@$child_results) == 1)
	  || return (0, "Cannot apply repeats to multiple child elements");
	return _apply_repeatitem_to_keyset ($child_results->[0], $element);
    }
    elsif (($tag_name eq "item") || ($tag_name eq "rule"))
    {
	##  Sequence of children
	if ($result_type eq "rule")
	{
	    if (scalar (@$child_results) > 1)
	    {
		return (0, "Cannot use <item> or <rule> " .
			"to sequence disjunctions");
	    }
	    else
	    {
		return (1, "", $child_results->[0]);
	    };
	};
	if ($result_type ne "keyseq")
	{
	    (($ok, $msg) = _promote_all ("keyseq", $child_results))[0]
	      || return (0, $msg);
	    $result_type = "keyseq";
	};
	return _merge_keyseqs (@$child_results);
    }
    else
    {
	return (0, "Unsupported element \"" . $tag_name . "\"");
    };
};

=head2 ($ok, $msg, $rule) = parse_srgsdtmf_grammar ($grammar)

Parses $grammar, which is an SRGS DTMF grammar.
On success returns $rule, defined above in _rec_dec_parse().
Returns (0, msg, undef) on error.

=cut

sub parse_srgsdtmf_grammar
{
    my ($grammar) = shift (@_);
    my ($builtin_type, $builtin_args, $builtin_arg_map, @builtin_arglist);
    my ($ok, $msg, $keyseq, $min, $max, $keyset, $keysetlist, $xml_parser);
    my ($xml_doc, $root, $type, $element);
    my ($result) = {};

    ##  Handle all builtin: cases
    if ($grammar =~ /^builtin:[^\/]*\/(boolean|digits)\??(.*)/)
    {
	##  It's a builtin
	$builtin_type = $1;
	$builtin_args = $2;
	$builtin_arg_map = {};
	if (defined ($builtin_args))
	{
	    @builtin_arglist = split (/;/, $builtin_args);
	    foreach $builtin_args (@builtin_arglist)
	    {
		if ($builtin_args =~ /^([^=]+)=(.*)$/)
		{
		    $builtin_arg_map->{$1} = $2;
		}
		else
		{
		    return (0, "Cannot parse builtin arg spec \"" .
			    $builtin_args . "\"");
		};
	    };
	};
	##  Generate the result
	if ($builtin_type eq "boolean")
	{
	    $keyseq = {"min_total" => 1,
		       "max_total" => 1,
		       "has-variable" => 0,
		       "keyset" => ["fullmatch" => qr/^[0-9\*#]$/s,
				    {"tags" => {"0" => "false",
						"1" => "true",
						"2" => "false",
						"3" => "false",
						"4" => "false",
						"5" => "false",
						"6" => "false",
						"7" => "false",
						"8" => "false",
						"9" => "false",
						"\\*" => "false",
						"#" => "false"},
				     "lentype" => "fixed",
				     "min" => 1,
				     "max" => 1,
				    }],
		      };
	    if (defined ($builtin_arg_map->{"y"}) &&
		($builtin_arg_map->{"y"} =~ /^[0-9\*#]$/so))
	    {
		$keyseq->{"keyset"}[0]{"tags"}{$builtin_arg_map->{"y"}}
		  = "true";
	    };
	    if (defined ($builtin_arg_map->{"n"}) &&
		($builtin_arg_map->{"n"} =~ /^[0-9\*#]$/so))
	    {
		$keyseq->{"keyset"}[0]{"tags"}{$builtin_args->{"n"}}
		  = "false";
	    };
	    return (1, "", {"type" => "rule",
			    "keyseq" => [$keyseq]});
	}
	else  ##  builtin type is digits
	{
	    $min = 1;
	    $max = $SRGSDTMF::MaxVarDigits;
	    if ((defined ($builtin_arg_map->{"length"})) &&
		($builtin_arg_map->{"length"} =~ /^\d+$/so))
	    {
		$min = $builtin_arg_map->{"length"};
		$max = $builtin_arg_map->{"length"};
	    };
	    if ((defined ($builtin_arg_map->{"minlength"})) &&
		($builtin_arg_map->{"minlength"} =~ /^\d+$/so))
	    {
		$min = $builtin_arg_map->{"minlength"};
	    };
	    if ((defined ($builtin_arg_map->{"maxlength"})) &&
		($builtin_arg_map->{"maxlength"} =~ /^\d+$/so))
	    {
		$max = $builtin_arg_map->{"maxlength"};
	    };
	    if ($max == 0)
	    {
		return (0, "Invalid digits maxlength of 0");
	    };
	    if ($max < $min)
	    {
		return (0, "Invalid digits minlength > maxlength");
	    };
	    if ($min == $max)
	    {
		##  Fixed-length non-zero digit sequence
		$keyseq = {"min_total" => $min,
			   "max_total" => $max,
			   "has-variable" => 0,
			   "keyset" => [
					{
					 "fullmatch" =>
					 qr/^[0-9]{$min,$max}$/s,
					 "prematch" =>
					 qr/^[0-9]{0,$max}$/s,
					 "tags" => {},
					 "lentype" => "fixed",
					 "min" => $min,
					 "max" => $max,
					},
				       ],
			  };
		return (1, "", {"type" => "rule",
				"keyseq" =>  [$keyseq]});
	    }
	    else
	    {
		##  Variable-length digit sequence
		$keyseq = {"min_total" => $min,
			   "max_total" => $SRGSDTMF::MaxVarDigits,
			   "has-variable" => 1,
			   "keyset" =>
			   [
			    {
			     "fullmatch" =>
			     qr/^[0-9]{$min,$SRGSDTMF::MaxVarDigits}#?$/s,
			     "maxmatch" =>
			     qr/^[0-9]{$min,$SRGSDTMF::MaxVarDigits}#$/s,
			     "prematch" =>
			     qr/^[0-9]{0,$SRGSDTMF::MaxVarDigits}$/s,
			     "tags" => {},
			     "lentype" => "variable",
			     "min" => $min,
			     "max" => $SRGSDTMF::MaxVarDigits,
			     "vardigmax" => $max
			    },
			   ],
			  };
		return (1, "", {"type" => "rule",
				"keyseq" =>  [$keyseq]});
	    };
	};
    }
    elsif ($grammar =~ /^builtin:/)
    {
	##  Not a recognized builtin
	return (0, "Unsupported builtin grammar");
    };

    ##  Only other option besides builtin is SRGS XML-form

    ##  Perform DOM porse
    $xml_parser = XML::LibXML->new();
    eval
    {
	$xml_doc = $xml_parser->parse_string ($grammar);
    };
    (length ("$@"))
      && (return (0, "Cannot parse XML grammar: $@"));

    ##  Validate root element
    $root = $xml_doc->documentElement();
    if ($root->nodeType() != ELEMENT_NODE)
    {
	return (0, "Unrecognized type of root element: " .
		$root->nodeType());
    };
    if ($root->tagName() eq "grammar")
    {
	my (@rules) = $root->getChildrenByTagName("rule");
	(scalar (@rules) == 0) &&
	  return (0, "No rule tags in grammar");
	(scalar (@rules) > 1) &&
	  return (0, "Multiple rules in grammar not yet supported");
	$root = $rules[0];
    };
    if ($root->tagName() ne "rule")
    {
	return (0, "Unrecognized root element tag \"" .
		$root->tagName() . "\"");
    };

    ##  Simple recursive-descent parse
    (($ok, $msg, $result) = _rec_dec_parse ($root))[0]
      || return (0, $msg);

    ##  Promote it to a rule
    (($ok, $msg, $result) = _promote ($result, "rule"))[0]
      || return (0, $msg);

    return (1, "", $result);
};

=head2 ($match, $interp, $rule) = check_rules_match ($input, @rules);

Given @rules generated from parse_srgsdtmf_grammar()
and keys that were $input, returns $match:
 -1 = guaranteed failure for this and all future input
  0 = partial match, could fully match with future input
  1 = full match, further input could also match
  2 = full match, no further input could match
and $interp the semantic interpretation (if any),
and $rule that particular rule in @rules that matched (if any).

=cut

sub check_rules_match
{
    my ($input) = shift (@_);
    my (@rules) = @_;
    my ($rule, $match, $interp, $keyseq);
    my ($need_more_input) = 0;

    scalar (@rules) || return (-1, undef, undef);
    while (defined ($rule = shift (@rules)))
    {
	foreach $keyseq (@{$rule->{"keyseq"}})
	{
	    ($match, $interp) = check_keyseq_match ($keyseq, $input);
	    if ($match > 0)
	    {
		##  Got a match
		return ($match, $interp, $rule);
	    };
	    if ($match == 0)
	    {
		$need_more_input = 1;
	    };
	};
    };

    return ($need_more_input ? 0 : -1, undef, undef);
};

=head1 AUTHOR

Doug Campbell, C<< <d.campbell at ampersand.com> >>

=head1 SUPPORT

You can find documentation for this module with the perldoc command.

    perldoc SRGSDTMF

You can also look for information at:

=over 4

=item * VoiceGlue Home Page

L<http://voiceglue.org>

=back

=head1 COPYRIGHT & LICENSE

Copyright 2006,2007 Ampersand Inc., and Doug Campbell

This file is part of SRGSDTMF.

SRGSDTMF is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

SRGSDTMF is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SRGSDTMF; if not, see <http://www.gnu.org/licenses/>.

=cut

1; # End of SRGSDTMF
