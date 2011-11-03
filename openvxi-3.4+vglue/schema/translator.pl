#!perl -w

if ($#ARGV < 1) {
  print "Usage: $0 index_file output_file\n";
  print "\n";
  exit 1;
}

# Attempt to open index file.

$index_file = $ARGV[0];

if (! -e $index_file) {
  print STDERR "Error: $index_file not found.\n";
  exit 1;
}

unless (open index_file) {
    print STDERR "Can't open $index_file: $!\n";
    exit 1;
}

# Open output file

open(FINAL, ">$ARGV[1]") || die "Error: opening <$ARGV[1]> for output\n";

# Write first line
print FINAL "const VXIbyte VALIDATOR_DATA[] = {\n";

my %testSection = ();

# Concatinate all members of the index file.

LINE: while (<index_file>) {
    next LINE if /^#/; # skip comments
    next LINE if /^$/; # skip blank lines

    if (/^([^\s]+)[\s]+([^\s]+)[\s]+/) {
	$source_file = $1;

	if (! -e $source_file) {
	    print STDERR "Error: $source_file not found.\n";
	    exit 1;
	}

	unless (open source_file) {
	    print STDERR "Can't open $source_file: $!\n";
	    exit 1;
	}

        $source_file_size = 0;

INNER:  while (<source_file>) {
            my $line = $_;

	    $line =~ s/\n//g;       # added to strip line breaks
	    @chars = unpack "c*", $line;
	    push @chars, "10";
	    print FINAL join ",", @chars;
	    print FINAL ",\n";
	    $source_file_size += $#chars + 1;
	}

	print "$1\n";
	print "$2\n";

	close($source_file);

	$testSection{$source_file}[0] = $source_file_size;
    }

    print "$source_file => $source_file_size\n";
}
print FINAL "0 };\n";

close index_file;


# Stage 2:

unless (open index_file) {
    print STDERR "Can't open $index_file: $!\n";
    exit 1;
}

$offset = 0;

ENTRY: while (<index_file>) {
    next ENTRY if /^#/; # skip comments
    next ENTRY if /^$/; # skip blank lines

    if (/^([^\s]+)[\s]+([^\s]+)[\s]+/) {
	$source_file = $1;
	# $name = $2;
	$source_file_size = $testSection{$source_file}[0];

	print FINAL "const unsigned int $2 = $offset;\n";
	print FINAL "const unsigned int $2_SIZE = $source_file_size;\n";
	$offset += $source_file_size;
    }
}

