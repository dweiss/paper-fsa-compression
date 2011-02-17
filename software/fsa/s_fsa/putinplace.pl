#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
                        # this emulates #! processing on NIH machines.
                        # (remove #! line above if indigestible)

# This script tries to put descriptions of words into correct place
# A parameter is a name of the file where the descriptions should be stored.
# Descriptions are on the standard input.
# 
# We assume the description has two parts: features and lexemes.
# The feature part ends with a right square bracket.

sub read_desc() {
    $old_desc = "";
    while (<STDIN>) {
	chomp;
#	printf "%s\n", $_;
	$i = index($_, ']');
	$desc = substr($_, 0, $i + 1);
	$lex = substr($_, $i + 2, 9999);
#	printf "length of the list is %d\n", scalar(@lexa);
#	printf "desc is %s, lex is %s\n", $desc, $lex;
	if ($desc ne $old_desc) {
	    if ($old_desc ne "") {
		# Completed a description
#		printf "new description\n";
		$adesc{$old_desc} = [ @lexa ];
		#@lexa = []; # this shit did not work (perl as usual)
		$#lexa = -1;
	    }
	    $old_desc = $desc
	}
	else {
#	    printf "old description\n";
	}
	push @lexa, $lex;
#	printf "at end length is %d\n", scalar(@lexa);
    }
    $adesc{$desc} = [ @lexa ];
}

$fnamefull = $ARGV[0];
if ($#ARGV < 0) {
    printf "Missing file name. Exiting!\n";
    exit;
}

sub copy_and_insert() {
    $tmpdir = $ENV{TMPDIR} ? $ENV{TMPDIR} : "/tmp";
    printf "%s\n", $fname;
    ($sourcedir = $fnamefull) =~ s/[^\/]*$//;
    ($fname $fnamefull) =~ s/^.*\///;
    printf "source dir is %s, fname is %s\n", $sourcedir, $fname;
    system("cp $fnamefull $tmpdir/$fname") or die "cannot copy files\n";
    open(M1FILE, "< $tmpdir/$fname") or die "cannot open $tmpdir/$fname\n";
    open(M2FILE, "> $fnamefull") or die "cannot open $fnamefull\n";
    while (<M1FILE>) {
	$myline = $_;
	# copy line
	printf M2FILE "%s", $myline;
	foreach $d (keys %adesc) {
	    if (index($myline, $d) != -1) {
		# line matches description, insert words
		$a = $adesc{$d};
		for $f (0 .. (scalar(@$a) - 1)) {
		    printf M2FILE "%s\n", $a->[$f];
		}
		undef $adesc{$d};
		last;
	    }
	}
    }
    close(M1FILE);
    close(M2FILE);
}

sub print_rest() {
    $printed = 0;
    foreach $ (keys %adesc) {
	if (!printed) {
	    printf "The following descriptions could not be put into place:\n";
	    $printed = 1;
	}
	$a = $adesc{$d};
	for $f (0 .. (scalar(@$a) - 1)) {
	    printf "%s %s\n", $d, $a->[$f];
	}
    }
}

read_desc();
copy_and_insert();
print_rest();
