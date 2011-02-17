#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
                        # this emulates #! processing on NIH machines.
                        # (remove #! line above if indigestible)

# This script takes as input the output of fsa_guess -m, which takes the form:
#
# x: a, b, c
#
# where:
# x is an unknown flectional form (and does not contain a colon)
# a, b, and c are morphological descriptions of corresponding lexemes,
# such that they do not contain commas.
#
# That data should appear on the standard input.
#
# Then it invokes mmorph with of these descriptions to check if they produce
# the unknown flectional form (x). Then the descriptions that did not produce
# that form are removed from the list. If this results in an empty list,
# it is converted to the standard fsa_guess output `*not found*'.
#
# Empty list (`*not found*') are left unchanged by the script.
#
# That data is generated on the standard output.
#
# Since it does not seem possible to make mmorph generate only forms
# described in a description stored in an additional file, 
# the generation is done in a few steps.
# 1) A preamble (everything but the lexicon) is copied to a temporary file.
# 2) The description is appended to that file.
# 3) Mmorph is made to create its database.
# 4) The database is dumped.
#
# Variables you should customize:

# Full path of mmorph
$mmorph='/usr/local/bin/mmorph';

# Full path of an mmorph description without the @Lexicon section
#$preamble="$ENV{HOME}/langacq/preamble";
$preamble="preamble";

# Full path of the file assembled from preamble and a guessed description
#$tmpmm="$ENV{HOME}/langacq/tmpmm";
$tmpmm="tmpmm";

# Full path of the file produced by mmorph
#$outmm="$ENV{HOME}/langacq/outmm";
$outmm="outmm";

while (<>) {
    use Text::ParseWords;
    chomp;			# strip record separator
    $colon_index = index($_, ':');
    $inf_form = substr($_, 0, $colon_index);
    @descrs = quotewords(", ", 1, substr($_, $colon_index + 2, 9999));
    if ($descrs[0] eq '*not found*') {
	@new_descrs = @descrs;
    }
    else {
	$#new_descrs = -1;
	foreach $d (@descrs) {
	    # Create data for mmorph
	    use File::Copy;
	    copy($preamble, $tmpmm);
	    open(TMPMM, ">>$tmpmm") or die "Cannot fork($tmpmm): $!\n";
	    printf TMPMM "%s\n", $d;
	    close(TMPMM);
#	    printf "Checking %s\n", $d;
	    # Create mmorph database
	    # We redirect input to supply EOF, and we reject output
	    # Note that it will not work on systems for idiots
	    open(TMPMM, "|$mmorph -c -m $tmpmm > /dev/null") or
		die "Cannot fork while creating database: $!\n";
	    close(TMPMM);
	    # Dump mmorph database
	    open(TMPMM, "$mmorph -q -m $tmpmm |") or
		die "Cannot fork while reading mmorph: $!\n";
	    # Read mmorph output
	    $desc_matches = 0;
	    while (<TMPMM>) {
		chomp;		# strip record separator
		s/^\"([^\"]+)\".*/$1/;
		if ($_ eq $inf_form) {
		    $desc_matches = 1;
		    last;
		}
	    }
	    if ($desc_matches) {
		push @new_descrs, $d;
	    }
	    close(TMPMM);
	}
	if ($#new_descrs == -1) {
	    push @new_descrs, '*not found*';
	}
    }
    # Print the new (filtered) description list
    reverse(@new_descrs);
    printf "%s: %s", $inf_form, pop @new_descrs;
    while (scalar(@new_descrs)) {
	printf ", %s", pop @new_descrs;
    }
    printf "\n";
}






