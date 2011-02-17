#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# process any FOO=bar switches

# This script coverts data in the format:
# inflected_formHTlexemeHTtags
# (where HT is the horizontal tabulation)
# to the form:
# canonical_form+tags+prefix+Kending
# where '+' is a separator,
# prefix is the prefix that should be prepended to the canonical form,
# K is a character that specifies how many characters
# should be deleted from the end of the canonical form to produce
# the inflected form by concatenating the stripped string
# with the ending ("A" means none, "B' - 1, "C" - 2, and so on).
#
# So the script tries to recognize prefixes automatically.
#
# You may want to change two pseudo-constants MAX_PREFIX_LEN and MAX_INFIX_LEN.
#
# Written by Jan Daciuk <jandac@eti.pg.gda.pl>, 2009
#


$separator = '+';

while (<>) {
    chop;	# strip record separator
    @Fld = split(' ', $_, 9999);

    # Calculate prefix in s1, and return its length (or 0)
    # s1 is the inflected form 
    # s2 is the canonical form 

    $MAX_INFIX_LEN = 3;
    $MAX_PREFIX_LEN = 3;
    $S = '';
    $l1 = length($Fld[0]);	# length of the inflected form
    $l2 = length($Fld[1]);	# length of the canonical form
    if (($prefix = &common_prefix($Fld[0], $Fld[1], $l1))) {
	# We have a common prefix of the inflected form and the canonical form,
	# so the inflected form does not have a prefix
	printf '%s%s%s',
	       $Fld[1],		# canonical form
	       $separator,	# separator
	       $Fld[2];			 # tags
	# There may be more tags at the end of the line, now stored in Fld[3..]
	for ($i = 3; $i < $#Fld; $i++) {
	    printf ' %s', $Fld[$i];
	}
	printf '%s%s%c%s',
	       $separator, $separator,   # two separators
	       ($l2 - $prefix + 65),     # K
	       substr($Fld[0], $prefix, 999999); # inflected form ending
    }
    else {
	# The inflected form and the canonical form do not have a common start
	# The inflected form may have a prefix
	if (($prefix_found = &find_prefix($Fld[0], $Fld[1])) > 0) {
	    # There is a prefix of length prefix_found
	    # $prefix1 is the length of the common part
	    # $prefix1 = &common_prefix($Fld[0] + prefix_found, $Fld[1], 99999);
	    printf '%s%s%s',
	           $Fld[1],				   # canonical form
	           $separator,				   # separator
	           $Fld[2];				   # tags
	    # There may be more tags at the end of the line,
	    # now stored in Fld[3..]. Delete the loop if your tags
	    # do not contain spaces and you put comments at ends of lines
	    for ($i = 3; $i < $#Fld; $i++) {
		printf ' %s', $Fld[$i];
	    }
	    printf '%s%s%s%c%s',
	           $separator,				   # separator
	           substr($Fld[0], 0, $prefix_found),	   # prefix
	           $separator,				   # separator
	           ($l2 - $prefix1 + 65),                  # K
	           substr($Fld[0], $prefix_found + $prefix1, 999999); # ending
	}
	else {
	    # This looks like an irregular word
	    printf '%s%s%s',
	           $Fld[1],	# canonical form
	           $separator,	# separator
	           $Fld[2];	# tags
	    # There may be more tags at the end of the line,
	    # now stored in Fld[3..]. Delete the loop if your tags
	    # do not contain spaces and you put comments at ends of lines
	    for ($i = 3; $i < $#Fld; $i++) {
		printf ' %s', $Fld[$i];
	    }
	    printf '%s%s%c%s',
	           $separator,	# separator
	           $separator,	# separator (no prefix)
	           65 + $l2,	# K: delete the whole canonical form
	           $Fld[0];	# inflected form in full
	}
    }
    printf "\n";		# end line
}

# Calculate length of the common prefix of s1 and s2, n is length of s1
sub common_prefix {
    local($s1, $s2, $n, $i) = @_;
    for ($i = 0; $i < $n; $i++) {
	if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {	#???
	    return $i;
	}
    }
    $n;
}

# Calculate length of a prefix of s1 that precedes the common part of s1 and s2
sub find_prefix {
    local($s1, $s2, $i) = @_;
#    printf STDERR "find_prefix(s1=%s, s2=%s)\n", $s1, $s2;
    local $l = length($s1);
    if ($l <= 1) {
	$prefix1 = "";
	return 0;
    }
    local $l_min =  $MAX_INFIX_LEN < $l ? $MAX_INFIX_LEN : $l ;
#    printf STDERR "l_min=%d\n", $l_min;
    for ($i = 1; $i <= $l_min; $i++) {
	if (($prefix1 = &common_prefix(substr($s1, $i, 999999), $s2,

	  length($s1) - $i)) > 2) {
	    return $i;
	}
    }
    0;
}

