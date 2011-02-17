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
# inverted_inflected_form+prefix+IKending+tags
# where '+' is a separator, prefix is one of two things. It is either
# a prefix of the inflected form, or it is an infix and what precedes it.
# I is a code that says how long is the prefix.
# ("A" means no infix, "B" means one, "C" - 2, etc.),
# K is a character that specifies how many characters
# should be deleted from the end of the inflected form to produce the lexeme
# by concatenated the stripped string with the ending ("A" means none,
# "B' - 1, "C" - 2, and so on).
#
# So the script tries to recognize prefixes and infixes automatically.
#
# You may want to change pseudo-constants MAX_PREFIX_LEN and MAX_INFIX_LEN.
#
# Written by Jan Daciuk <jandac@pg.gda.pl>, 1999
#

# Calculate common prefix of s1 and s2, and return its length
# s1 is the inflected form
# s2 is the canonical form
# n is length of s1
$FS = '\t';		# field separator from -F switch

$separator = '+';

while (<>) {
    chop;	# strip record separator
    @Fld = split(/[\t\n]/, $_, 9999);

    # Calculate prefix in s1, and return its length (or 0)
    # s1 is the inflected form 
    # s2 is the canonical form 

    $MAX_INFIX_LEN = 3;
    $MAX_PREFIX_LEN = 3;
    $l1 = length($Fld[0]);
    # invert word
    $S = '';
    for ($i = 0; $i < $l1; $i++) {
	$S = substr($Fld[0], $i, 1) . $S;
    }
    if (($prefix = &common_prefix($Fld[0], $Fld[1], $l1))) {
	# common prefix not empty, but we may have an infix
	$prefix_found = &find_prefix($Fld[0], $Fld[1]);
	$prefix2 = $prefix1;
	if ($prefix_found >= ($infix_found = &find_prefix(substr($Fld[0],
	  $prefix, 999999), substr($Fld[1], $prefix, 999999)))) {
	    if (($prefix_found > 0) && ($prefix_found + $prefix2 > $prefix)) {
		# we have a rare case when we can have either a prefix, or an infix,
		# like in German gegurrt/gurren
		# we prefer the longer one, or if equal - the prefix
		# here the prefix is longer than the infix
		printf '%s_%s%s%sA%c%s%s%s', substr($S,0,$l1 - $prefix_found), 
		  $separator, substr($Fld[0], 0, $prefix_found), $separator, 
		  ($l1 - $prefix_found - $prefix2 + 65),
		  substr($Fld[1], $prefix2, 999999), 
		  $separator, $Fld[2];
	    }
	    else {
		# both infix_found and prefix_found are 0
		# but still some characters at the beginning are the same
		printf '%s_%s%sA%c%s%s%s', $S, $separator, 
		  $separator, ($l1 - $prefix + 65),
		  substr($Fld[1], $prefix, 999999), 
		  $separator, $Fld[2];
	    }
	}
	elsif ($prefix1 > 0) {
	    # we have an infix, and if there seems to be a prefix,
	    # the infix is longer
	    printf '%s_%s%s%s%c%c%s%s%s',
	      substr($S, 0, $l1 - $prefix - $infix_found), 
	      $separator, substr($Fld[0], 0, $prefix + $infix_found),
	      $separator, $infix_found + 65, 
	      ($l1 - $prefix - $prefix1 - $infix_found + 65), 
	      substr($Fld[1], $prefix + $prefix1, 999999), $separator,
	      $Fld[2];
	}
	else {
	    # neither prefix, nor infix
	    printf '%s_%s%sA%c%s%s%s', $S, $separator, 
		  $separator, ($l1 - $prefix + 65),
	          substr($Fld[1], $prefix, 999999),
	          $separator, $Fld[2];
	}
    }
    else {
	# we may have a prefix
	if (($prefix_found = &find_prefix($Fld[0], $Fld[1])) > 0) {
	    printf '%s_%s%s%sA%c%s%s%s', substr($S, 0, $l1 - $prefix_found), 
	      $separator, substr($Fld[0], 0, $prefix_found), $separator, 
	      ($l1 - $prefix_found - $prefix1 + 65),
	      substr($Fld[1], $prefix1, 999999), 
	      $separator, $Fld[2];
	}
	else {
	    printf '%s_%s%sA%c%s%s%s', $S, $separator, $separator,
	      length($Fld[0]) + 65, 
	      $Fld[1], $separator, $Fld[2];
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

sub common_prefix {
    local($s1, $s2, $n, $i) = @_;
    for ($i = 0; $i < $n; $i++) {
	if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {
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
        $prefix1 = 0;
        return 0;
    }
    local $l_min =  $MAX_INFIX_LEN < $l ? $MAX_INFIX_LEN : $l ;
#    printf STDERR "l_min=%d\n", $l_min;
    $prefix1 = 0;
    for ($i = 1; $i <= $l_min; $i++) {
        if (($prefix1 = &common_prefix(substr($s1, $i, 999999), $s2,

          length($s1) - $i)) > 2) {
            return $i;
        }
    }
    0;
}
