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
# inflected_form+MLKending+tags
# where '+' is a separator, M is the position of characters to be deleted
# towards the beginning of the inflected form ("A" means from the beginning,
# "B" from the second character, "C" - from the third one, and so on),
# L is the number of characters to be deleted from the position specified by M
# ("A" means none, "B" means one, "C" - 2, etc.),
# K is a character that specifies how many characters
# should be deleted from the end of the inflected form to produce the lexeme
# by concatenating the stripped string with the ending ("A" means none,
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
    $l1 = length($Fld[0]);
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
		# we have a rare case when we can have either a prefix,
		# or an infix, like in German gegurrt/gurren
		# we prefer the longer one, or if equal - the prefix
		# here the prefix is longer than the infix, so we take it
		printf '%s%sA%c%c%s%s%s',		      # M=A
		       $Fld[0],				      # inflected form
		       $separator,			      # separator
		       $prefix_found + 65,		      # L
                       ($l1 - $prefix_found - $prefix2 + 65), # K
		       substr($Fld[1], $prefix2, 999999),     # ending
		       $separator,			      # separator
		       $Fld[2];				      # tags
	    }
	    else {
		# both infix_found and prefix_found are 0
		# still some characters at the beginning are the same
		# e.g. geht/gehen
		printf '%s%sAA%c%s%s%s',		 # M=A, L=A
		       $Fld[0],				 # inflected form
		       $separator,			 # separator
		       ($l1 - $prefix + 65),		 # K
		       substr($Fld[1], $prefix, 999999), # ending
		       $separator,			 # separator
		       $Fld[2];				 # tags
	    }
	}
	elsif ($prefix1 > 0) {
	    # we have an infix, and if there seems to be a prefix,
	    # the infix is longer
	    printf '%s%s%c%c%c%s%s%s',
	           $Fld[0],					   # inflected
	           $separator,					   # separator
	           $prefix + 65,				   # M
	           $infix_found + 65,				   # L
	           ($l1 - $prefix - $prefix1 - $infix_found + 65), # K
	           substr($Fld[1], $prefix + $prefix1, 999999),	   # ending
	           $separator,					   # separator
	           $Fld[2];					   # tags
	}
	else {
	    # we have an infix, and if there seems to be a prefix,
	    # the infix is longer
	    # but the common prefix of two words is longer
	    # we treat is as normal inflection
	    # this case does not seem to appear in practice
	    printf '%s%sAA%c%s%s%s',		     # M=A, L=A
	           $Fld[0],			     # inflected form
	           $separator,			     # separator
	           ($l1 - $prefix + 65),	     # K
	           substr($Fld[1], $prefix, 999999), # ending
	           $separator,			     # separator
	           $Fld[2];			     # tags
	}
    }
    else {
	# we may have a prefix
	if (($prefix_found = &find_prefix($Fld[0], $Fld[1])) > 0) {
	    # we have a prefix
	    printf '%s%sA%c%c%s%s%s',			  # M=A
	           $Fld[0],				  # inflected form
	           $separator,				  # separator
	           $prefix_found + 65,			  # L
	           ($l1 - $prefix_found - $prefix1 + 65), # K
	           substr($Fld[1], $prefix1, 999999),	  # ending
	           $separator,				  # separator
	           $Fld[2];				  # tags
	}
	else {
	    # irregular word
	    printf '%s%sAA%c%s%s%s', # M=A, L=A
	           $Fld[0],	     # inflected form
	           $separator,	     # separator
	           65 + $l1,	     # delete the whole inflected form
	           $Fld[1],	     # the whole canonical form
	           $separator,	     # separator
	           $Fld[2];	     # tags
	}
    }
    # Delete the following (1) line if your tags do not contain spaces
    # and you would like to append comments at the end of lines
    for ($i = 3; $i <= $#Fld; $i++) {
	printf ' %s', $Fld[$i];
	# Do not delete this
	;
    }
    printf "\n";
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
