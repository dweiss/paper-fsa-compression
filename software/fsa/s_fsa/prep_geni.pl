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
# canonical_form+tags+infix+MKending
# where '+' is a separator, M is the position of infix to be inserted
# towards the beginning of the inflected form ("A" means at the beginning,
# "B" from the second character, "C" - from the third one, and so on),
# K is a character that specifies how many characters
# should be deleted from the end of the canonical form to produce the inflected
# form by concatenating the stripped string with the ending ("A" means none,
# "B' - 1, "C" - 2, and so on).
#
# So the script tries to recognize prefixes and infixes automatically.
#
# You may want to change pseudo-constants MAX_PREFIX_LEN, MAX_INFIX_LEN,
# and MIN_ILEN.
#
# Written by Jan Daciuk <jandac@pg.gda.pl>, 1997
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
    $l1 = length($Fld[0]);	# length of the inflected form
    $l2 = length($Fld[1]);	# length of the canonical form
    if (($prefix = &common_prefix($Fld[0], $Fld[1], $l1))) {
	# (now $prefix is the length of the common prefix)

	# common prefix not empty, but we may have an infix
	$prefix_found = &find_prefix($Fld[0], $Fld[1]);
	# (now prefix1 is the length of the matching part after a prefix)
	# (now prefix_found is the length of the infix)
	$prefix2 = $prefix1;
	if ($prefix_found >=
            ($infix_found = &find_prefix(substr($Fld[0], $prefix, 999999),
					 substr($Fld[1], $prefix, 999999)))) {
	    if (($prefix_found > 0) && ($prefix2 > $prefix)) {
		# We have a rare case when we can have either a prefix,
		# or an infix, like in German gegurrt/gurren,
		# i.e. either ge-gurr-t or g-eg-urr-t.
		# We prefer the longer one, or if equal - the prefix
		# here the prefix is longer than the infix, so we take it
		# Here:
		# $l1 is the length of the inflected form
		# $l2 is the length of the canonical form
		# $prefix is the common prefix, but it is ignored (assume infix)
		# $prefix_found is the length of a prefix
		# $prefix2 is the length of the common stem
		# $infix_found is ignored
		# $prefix1 is ignored
		printf "%s%s%s",
		       $Fld[1],		# canonical form
		       $separator, 	# separator
		       $Fld[2];		# tags
		# Tags may have spaces inside, so the following loop
		# appends the rest.
		# If your tags do not contain spaces, and you have comments
		# at the end of lines, delete the loop
		for ($i = 3; $i <= $#Fld; $i++) {
		    printf " %s", $Fld[$i];
		}
		printf '%s%s%sA%c%s',
		       $separator,			  # separator
		       substr($Fld[0], 0, $prefix_found), # prefix
		       $separator,			  # separator (M=A)
                       ($l2 - $prefix2 + 65),		  # K
		       substr($Fld[0], $prefix_found + $prefix2, 999999);#endng
	    }
	    else {
		# Both infix_found and prefix_found are 0
		# still some characters at the beginning are the same
		# Example: geht/gehen
		printf "%s%s%s",
		       $Fld[1],	   # canonical form
		       $separator, # separator
		       $Fld[2];	   # tags
		# Tags may have spaces inside, so the following loop
		# appends the rest.
		# If your tags do not contain spaces, and you have comments
		# at the end of lines, delete the loop
		for ($i = 3; $i <= $#Fld; $i++) {
		    printf " %s", $Fld[$i];
		}
		printf "%s%sA%c%s",
		       $separator, # separator
		       $separator, # separator (prefix/infix is null)
		       ($l2 - $prefix + 65), # K (M=A in format)
		       substr($Fld[0], $prefix, 999999); # ending
	    }
	}
	elsif ($prefix1 > 0) {
	    # we have an infix, and if there seems to be a prefix,
	    # the infix is longer
	    # $l1 is the length of the inflected form
	    # $l2 is the length of the canonical form
	    # $prefix is the length of the part of inflected f. before the infix
	    # $prefix1 is the length of the stem
	    # $prefix2 should be 0
	    # $infix_found is the length of the infix
	    printf "%s%s%s",
	           $Fld[1],	# canonical
	           $separator,	# separator
	           $Fld[2];	# tags
	    # Tags may have spaces inside, so the following loop
	    # appends the rest.
	    # If your tags do not contain spaces, and you have comments
	    # at the end of lines, delete the loop
	    for ($i = 3; $i <= $#Fld; $i++) {
		printf ' %s', $Fld[$i];
	    }
	    printf '%s%s%s%c%c%s',
	           $separator,				   # separator
	           substr($Fld[0], $prefix, $infix_found), # infix
	           $separator,				   # #separator
	           $prefix + 65,			   # M
	           ($l2 - $prefix - $prefix1 + 65),   # K
	           substr($Fld[0],
			  $prefix + $infix_found + $prefix1,
			  999999); # ending
	}
	else {
	    # The inflected form and the canonical form have some common prefix
	    #
	    # We have an infix, and if there seems to be a prefix,
	    # the infix is longer
	    # but the common prefix of two words is longer.
	    # We treat it like two words having only different endings.
	    # This does not seem to appear in practice.
	    printf '%s%s%s',
	           $Fld[1],	# canonical form
	           $separator,	# separator
	           $Fld[2];	# tags
	    # Tags may have spaces inside, so the following loop
	    # appends the rest.
	    # If your tags do not contain spaces, and you have comments
	    # at the end of lines, delete the loop
	    for ($i = 3; $i <= $#Fld; $i++) {
		printf ' %s', $Fld[$i];
	    }
	    printf "%s%sA%c%s",
	    $separator, # separator
	    $separator, # separator (prefix/infix is null)
	    ($l1 - $prefix + 65), # K (M=A in format)
	    substr($Fld[0], $prefix, 999999); # ending
	}
    }
    else {
	# We have a prefix
	if (($prefix_found = &find_prefix($Fld[0], $Fld[1])) > 0) {
	    printf '%s%s%s',
	           $Fld[1],	# canonical form
	           $separator,	# separator
	           $Fld[2];	# tags
	    # Tags may have spaces inside, so the following loop
	    # appends the rest.
	    # If your tags do not contain spaces, and you have comments
	    # at the end of lines, delete the loop
	    for ($i = 3; $i <= $#Fld; $i++) {
		printf ' %s', $Fld[$i];
	    }
	    printf '%s%s%sA%c%s',
	           $separator,			      # separator
	           substr($Fld[0], 0, $prefix_found), # prefix
	           $separator,			      # separator(M=A in format)
	           ($l1 - $prefix_found - $prefix1 + 65),
	    substr($Fld[0], $prefix_found + $prefix1, 999999); # ending
	}
	else {
	    # An irregular word
	    printf '%s%s%s',
	           $Fld[1],	# the canonical form
	           $separator,	# separator
	           $Fld[2];	# tags
	    # Tags may have spaces inside, so the following loop
	    # appends the rest.
	    # If your tags do not contain spaces, and you have comments
	    # at the end of lines, delete the loop
	    for ($i = 3; $i <= $#Fld; $i++) {
		printf ' %s', $Fld[$i];
	    }
	    printf '%s%sA%c%s',
	           $separator,	# separator
	           $separator,	# separator (empty prefix/infix)
	           65 + $l2,	# delete the whole canonical form
	           $Fld[0];	# put the whole inflected form
	}
    }
    printf "\n";		# end line
}

# Calculate common prefix of s1 and s2, n is length of s1
# Return the length of the common prefix
sub common_prefix {
    local($s1, $s2, $n, $i) = @_;
    for ($i = 0; $i < $n; $i++) {
	if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {	#???
	    return $i;
	}
    }
    $n;
}

# Find a prefix by which s1 differs from s2
# Remove characters from the beginning of s1 
# to see if then the beginnings of s1 and s2 would match
# Set prefix1 to the length of the matching parts
# Return the length of that prefix
sub find_prefix {
    local($s1, $s2, $i) = @_;
    local $l = length($s1);
    if ($l <= 1) {
	$prefix1 = "";
	return 0;
    }
    local $l_min =  $MAX_INFIX_LEN < $l ? $MAX_INFIX_LEN : $l ;
    for ($i = 1; $i <= $l_min; $i++) {
	if (($prefix1 = &common_prefix(substr($s1, $i, 999999), $s2,

	  length($s1) - $i)) > 2) {
	    return $i;
	}
    }
    0;
}
