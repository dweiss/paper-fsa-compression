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
# inflected_form+LKending+tags
# where '+' is a separator,
# L is the number of characters to be deleted from the begging of the word
# ("A" means none, "B" means one, "C" - 2, etc.),
# K is a character that specifies how many characters
# should be deleted from the end of the inflected form to produce the lexeme
# by concatenated the stripped string with the ending ("A" means none,
# "B' - 1, "C" - 2, and so on).
#
# So the script tries to recognize prefixes automatically.
#
# You may want to change two pseudo-constants MAX_PREFIX_LEN and MAX_INFIX_LEN.
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
	# common prefix not empty, but we may have an infix
	printf '%s%sA%c%s%s%s',			 # L=A
	       $Fld[0],				 # inflected form
	       $separator,			 # separator
	       ($l1 - $prefix + 65),		 # K (L=A in format)
	       substr($Fld[1], $prefix, 999999), # ending
	       $separator,			 # separator
	       $Fld[2];				 # tags
    }
    else {
	# we may have a prefix
	if (($prefix_found = &find_prefix($Fld[0], $Fld[1])) > 0) {
	    # a prefix
	    printf '%s%s%c%c%s%s%s',
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
	    printf '%s%sA%c%s%s%s', # L=A
	           $Fld[0],	# inflected form
	           $separator,	# separator
	           65 + $l1,	# delete the whole inflected form(L=A in format)
	           $Fld[1],	# the whole canonical form
	           $separator,	# separator
	           $Fld[2];	# tags
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
sub common_prefix {
    local($s1, $s2, $n, $i) = @_;
    for ($i = 0; $i < $n; $i++) {
	if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {	#???
	    return $i;
	}
    }
    $n;
}

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
