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
# inverted_form+tags
# where the inverted_form is the inverted inflected_form, i.e. the last
# character of the inflected form becomes the first character of the
# inverted_form, the second - the penultimate, and so on.
# Before the inversion, a filler character ('_') is put at the front
# of the inflected form, so it becomes the last character
# of the inverted form.
# The script is intended to be used to prepare data for fsa_build -X
# to build an automaton used by fsa_guess compiled _without_ GUESS_LEXEMES.

while (<>) {
    chop;	# strip record separator
    @Fld = split('\t', $_, 9999);

    $S = '';
    for ($i = 0; $i < length($Fld[0]); $i++) {
	$S = substr($Fld[0], $i, 1) . $S;
    }
    printf '%s_+%s', $S, $Fld[2];
    # Delete the following line if your tags do not contain spaces
    # and you want to put comments at the end of lines
    for ($i = 3; $i < $#Fld; $i++) {
	printf ' %s', $Fld[$i];
	# Do not delete this line
	;
    }
    printf "\n";
}
