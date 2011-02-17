#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# process any FOO=bar switches

# prep_gen.pl

# This script coverts data in the format:
#
# inflected_formHTlexemeHTtags
#
# (where HT is the horizontal tabulation, lexeme is the canonical form,
# and tags are annotations)
# into the form:
#
# lexeme+tags+Kending
#
# where '+' is a separator, K is a character that specifies how many characters
# should be deleted from the end of the canonical form (lexeme) to produce
# the inflected form the stripped string with the ending. K='A' means
# no deletion, 'B' - delete 1 character, 'C' - delete 2, and so on.
#
# Written by Jan Daciuk <jandac@eti.pg.gda.pl>, 2008, 2009, 2010
#

$separator = '+';

while (<>) {
    chop;	# strip record separator
    @Fld = split('\t', $_, 9999);

    $l1 = length($Fld[0]);	# length of the inflected form
    $l2 = length($Fld[1]);	# length of the canonical form
    if (($prefix = &common_prefix($Fld[0], $Fld[1], $l1))) {
	# The canonical and inflected form have a common prefix,
	# which has the length $prefix.
	# Print the canonical form, separator, tags, separator,
	# deletion code and canonical ending
	printf '%s%s%s',
	       $Fld[1],		# canonical form
	       $separator,	# separator
	       $Fld[2];		# tags
	# Delete the following loop if your tags do not contain spaces
	# and you put comments at the end of lines
	for ($i = 3; $i < $#Fld; $i++) {
	    printf ' %s', $Fld[$i];
	}
	printf '%s%c%s',
	       $separator,	# separator
	       ($l2 - $prefix + 65), # K
	       substr($Fld[0], $prefix, 999999); # ending
    }
    else {
	# The canonical form and the inflected form have no common prefix.
	# An irregular word.
	printf '%s%s%s',
	       $Fld[1],		# canonical form
	       $separator,	# separator
	       $Fld[2];		# tags
	# Delete the following loop if your tags do not contain spaces
	# and you put comments at the end of lines
	for ($i = 3; $i < $#Fld; $i++) {
	    printf ' %s', $Fld[$i];
	}
	printf '%s%c%s',
	       $separator,	# separator
	       $l2 + 65,	# delete the whole canonical form
	       $Fld[0];		# the whole inflected form
    }
    printf "\n";		# end line
}

# common_prefix finds the length of the longest common prefix
# of two strings that are its parameters.
# $1 - the first string
# $2 - the second string
# $3 - length of the first string
sub common_prefix {
    local($s1, $s2, $n, $i) = @_;
    for ($i = 0; $i < $n; $i++) {
	if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {	#???
	    return $i;
	}
    }
    $n;
}
