#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# process any FOO=bar switches

# This script translates data in the form:
# inflected_formHTlexemeHTtags
# to the form:
# inverted_form+Kending+tags
# where the inverted_form is the inverted inflected_form, i.e. the last
# character of the inflected form becomes the first character of the
# inverted_form, the second - the penultimate, and so on.
# Before the inversion, a filler character ('_') is put at the front
# of the inflected form, so it becomes the last character
# of the inverted form.
# '+' is a separator, K is a character that specifies how many characters
# should be deleted from the end of the inflected form to produce the lexeme
# by concatenated the stripped string with the ending.
# The script is intended to be used to prepare data for fsa_build -X
# to build an automaton used by fsa_guess compiled _with_ GUESS_LEXEMES,
# and _without_ GUESS_PREFIX.
#

$separator = '+';

while (<>) {
    chop;	# strip record separator
    @Fld = split(' ', $_, 9999);

    $S = '';
    $l1 = length($Fld[0]);
    # invert word
    for ($i = 0; $i < $l1; $i++) {
	$S = substr($Fld[0], $i, 1) . $S;
    }
    if (($prefix = &common_prefix($Fld[0], $Fld[1], $l1))) {
	printf '%s_%s%c%s%s%s', $S, $separator, 
	($l1 - $prefix + 65), substr($Fld[1], $prefix, 999999), 
	$separator, $Fld[2];
    }
    else {
	printf '%s_%s%c%s%s%s', $S, $separator, 
	65 + $l1, $Fld[1], $separator, $Fld[2];
    }
    # Delete the following line if your tags do not contain spaces,
    # and you want to put comments at the end of lines
    for ($i = 3; $i <= $#Fld; $i++) {
	printf ' %s', $Fld[$i];
	# Do not delete this line
	;
    }
    printf "\n";
}

sub common_prefix {
    local($s1, $s2, $n, $i) = @_;
    for ($i = 0; $i < $n; $i++) {
	if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {	#???
	    return $i;
	}
    }
    $n;
}
