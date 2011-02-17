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
# The number of occurences of various descriptions is counted.
# The file is sorted so that more frequent descriptions are close 
# to the unknown word form in individual lines.
#
# Variables you should customize:


# Count descriptions
$nr = 0;
while (<>) {
    use Text::ParseWords;
    chomp;			# strip record separator
    $filelines[$nr++] = $_;
    $colon_index = index($_, ':');
    $inf_form = substr($_, 0, $colon_index);
    @descrs = quotewords(", ", 1, substr($_, $colon_index + 2, 9999));
    if ($descrs[0] eq '*not found*') {
	//
    }
    else {
	foreach $d (@descrs) {
	    $seen{$d}++;
	}
    }
}

# Rearrange descriptions in entries
$i = 0;
while ($i < $nr) {
    use Text::ParseWords;
    $l = $filelines[$i++];
    $colon_index = index($l, ':');
    $inf_form = substr($l, 0, $colon_index);
    @descrs = quotewords(", ", 1, substr($l, $colon_index + 2, 9999));
    # Because that shit perl does not have a function that returns
    # the position of an item on a list, we have to create this hash
    $j = 0;
    @desccopy = @descrs;
    while (scalar(@desccopy)) { $itemnr{pop @desccopy} = $j++; }
    @new_descrs = sort {
	$seen{$a} == $seen{$b} ?
	    $itemnr{$a} <=> $itemnr{$b} :
		$seen{$a} <=> $seen{$b}
    } @descrs;
    undef %itemnr;
    # Print the new (ordered) description list
#    reverse(@new_descrs);
#    pop @new_descrs;		# strange pearl behaviour
    printf "%s: %s", $inf_form, pop @new_descrs;
    while (scalar(@new_descrs)) {
	 printf ", %s", pop @new_descrs;
    }
    printf "\n";
}






