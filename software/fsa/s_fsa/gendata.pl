#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# 'process any FOO=bar switches

# Generate complete descriptions of single lexemes in one line
# Synopsis:
# gendata lexicon_file rules_file
# where
# lexicon_file contains the lexicon part of morphological descriptions
# rules_file contains rules for morphological descriptions
# both files must be in mmorph format.
use Getopt::Std;

# Customization part
$mmorph = "mmorph";		# how to invoke mmorph (path recommended)
$preamble = "preamble";		# default rule file name
$lexicon = "lexicon";		# default lexicon file name
$tmpmm = "tmpmm";		# temporary file for mmorph descriptions
$MAX_INFIX_LEN = 3;
$MAX_PREFIX_LEN = 3;
$MIN_COMMON_LEN = 2;

$infixes_used = 0;
$prefixes_used = 0;
# Process options
getopts("hipa:l:r:");
if ($opt_h) {
    &synopsis();
}
if ($opt_i) {
    $infixes_used = 1;
    if ($opt_p) {
	printf STDERR "No need to specify -p while -i is used\n";
    }
}
if ($opt_p) {
    $prefixes_used = 1;
}
if ($opt_a) {
    $archiphonemes = $opt_a;
#    printf "archiphonemes: %s\n", $archiphonemes;
}
if ($opt_h) {
    printf "Synopsis: gendata [-i|-p|-a preamble_file|-l lexicon_file|\n";
    printf "-a archiphonemes|-h\n";
    printf "where:\n";
    printf "-i\t- data contains infixes (and possibly prefixes)\n";
    printf "-p\t- data contains prefixes and no infixes\n";
    printf "-a preamble\t- specifies preamble file ";
    printf "(mmorph file up to and including \@Lexicon\n";
    printf "-l lexicon\t- everything after \@Lexicon in mmorph file\n";
    printf "-a archiphonemes\t- list of archiphonemes separated with colons\n";
    printf "-h\t- this message\n";
}
$rule_file = ($opt_r || $preamble);
$lexicon_file = ($opt_l || $lexicon);

# Process descriptions in the lexicon, transforming them in lines
# so that each one contains categories, inflected form, and lexical form
open(LEXFILE, "< $lexicon_file") or
    die "Cannot open lexicon file $lexicon_file $!\n";
while (<LEXFILE>) {
    use File::Copy;

    chop;			# strip record separator
    copy($rule_file, $tmpmm);
#    printf "Read: %s\n", $_;
    if (m/^\b*;/) {
	# this line is a comment, skip it
#	printf "Comment line\n";
    }
    elsif (m/^\b*$/) {
#	printf "Blank line\n";
    }
    elsif (!/^\b*\x22/) {
	# line with a morphological description, but no lexeme
	s/^\s+//;		# supress leading blanks
	if (m/\x5b/) {
	    # new morphological description
	    $morph_desc = $_;
	}
	else {
	    # continuation
	    $morph_desc = $morph_desc . " " . $_;
	}
    }
    elsif (m/^\b*\"/) {
	# line with a lexeme - print its full description
#	printf "Lexeme description: %s\n", $_;
	$lex_desc = $_;
	open(TMPMM, ">> $tmpmm") or die "Cannot open temp file $tmpmm $!\n";
	printf TMPMM "%s\t%s\n", $morph_desc, $_;
#	printf "Wrote: %s\t%s\n", $morph_desc, $_; # %-%
	close(TMPMM);
	# run mmorph
	# first create the database
	# We redirect input to supply EOF, and we reject output
	# Note that it will not work on systems for idiots
#	printf "Creating database...\n";
	open(OUTMM, "|$mmorph -c -m $tmpmm > /dev/null") or
	    die "Cannot fork while creating database: $!\n";
	close(OUTMM);
#	printf "Database created.\n";
	# Dump mmorph database
#	printf "Dumping database...\n";
	open(OUTMM, "$mmorph -q -m $tmpmm |") or
	    die "Cannot fork while reading mmorph: $!\n";
#	printf "Database dumped.\n";
#	printf "Reading mmorph output...\n";
	# Read mmorph output
	while (<OUTMM>) {
	    chop;		# strip record separator
#	    printf "mo: %s\n", $_;
	    # delete first double quote
	    $s = "[^\"]*\"", s/$s//;
	    # Now the first double quote is the one ending the inflected form
	    # convert the ending double quote of inflected
	    # form, the equal sign, the beginning double
	    # quote of the base form, and all white space
	    # between them to a HT character
	    $s = "\"[^\"]*\"", s/$s/\t/;
	    # convert the ending double quote of base form
	    # and the following spaces to HT
	    $s = "\" *", s/$s/\t/;
	    # delete space after [
	    $s = "\\[ ", s/$s/[/g;
	    # delete space before ]
	    $s = " \\]", s/$s/]/g;
	    # mmorph uses _ to indicate spaces between 
	    # words; I use it for various things, but
	    # I can use spaces
#	    $s = '_', s/$s/ /g;

	    # separate fields
	    ($inflected, $canonical, $categories) = split("\t", $_, 9999);
	    $s = '_', $inflected =~ s/$s/ /g;
	    $final = sprintf "%s\t%s\t%s\t%s", $morph_desc, $lex_desc,
	           $inflected, $canonical;
#	    printf "cf on: %s\n", $final;
#	    printf "where morph_desc=%s\n", $morph_desc;
#	    printf "lex_desc=%s\n", $lex_desc;
#	    printf "inflected=%s\n", $inflected;
#	    printf "canonical=%s\n", $canonical;
	    # now call code_forms to do the job
#	    printf "and _=%s\n", $_;
	    code_forms($final);
	}
	close(OUTMM);
	unlink($tmpmm);
    }
}

# Print how to call the script
sub synopsis {
    printf "Synopsis\n";
    printf "gendata.pl [-i|-p] -r rule_file -l lexicon_file [-a archiphonemes\n";
    printf "Use -i if the lexicon contains infixes (and possibly prefixes).\n";
    printf "Use -p if the lexicon contains prefixes, but no infixes.\n";
    printf "Use -a if the lexicon contains archiphonemes located\n";
    printf "  towards the beginning of the lexical form. Archiphonemes\n";
    printf "  are separated with semicolons (don't use & and ; inside them)\n";
}

# The parameters are the description, the lexical description
# (consisting of the lexical form in double quotes, an equal sign,
# and the canonical form in quotes), the inflected form,
# and the canonical form, separated with HT.
#
# The output is one line, in the form:
# inverted_+pre+IK1e1+K2K3K4a1+e2+categories
# where:
# inverted	- is the inverted inflected form;
# pre           - present only if infixes_used or prefixes_used;
#		  for prefixes_used, it is a prefix;
#		  for infixes used, it is a prefix and a suffix;
# I		- says how long is an infix: 'A' means no infix, 'B' - 1,
#		  'C' - 2, and so on.
# K1 		- says how many characters to delete from the end of
# 		  the inflected form to get the canonical form;
#		  it is single letter: 'A' means none, 'B' - 1,
#		  'C' - 2, etc.
# e1		- is the ending to append after removing K1 letters to
# 		  get the canonical form;
# K2		- says how many character to delete from the end of
# 		  the inflected form to get the lexical form;
#		  it is single letter: 'A' means none, 'B' - 1,
#		  'C' - 2, etc.
# K3		- position of the arch-phoneme
#		  'A' means there are no arch-phonemes,
#		  'B' - the first character, 'C' - the second one etc.
# K4		- number of characters the phoneme replaces;
#		  'A' means 0, 'B' - 1, etc.;
#		  present only if K3 is not 'A'
# e2		- ending of lexical form
# a1		- the arch-phoneme; present only (with preceding "+")
#		  when K3 is not 'A'
#
sub code_forms {
    local ($shit) = @_;
    local ($descrs,$lex_desc,$inflected,$canonical) = split("\t", $shit, 9999);
    local ($preinfix);

#    printf "cf: the whole shit=%s\n", $shit;
#    printf "cf: descrs=%s\n", $descrs;
#    printf "cf: lex_desc=%s\n", $lex_desc;
#    printf "cf: inflected=%s\n", $inflected;
#    printf "cf: canonical=%s\n", $canonical;
    $separator = '+';
    # invert word
    $inverted = reverse($inflected);
    # see what the first part of the resulting string is
    $result = $inverted . '_' . '+';
#    printf "infixes_used=%d\n", $infixes_used;
    if ($infixes_used) {
	$result = inf_prefix($inflected, $canonical);
	($preinfix = $result) =~ s/\+.+//;
	if (length($preinfix) > 0) {
	    $result = reverse(substr($inflected, length($prefix), 9999)) .
		'_+' . $result;
	}
	else {
	    $result = reverse($inflected) . '_+' . $result;
	}
    }
    elsif ($prefixes_used) {
	$result = noinf_prefix($inflected, $canonical);
	# It is necessary to test for the first character, because
	# this shit perl includes the first plus in a line in /^[^\+]*/.
	if (substr($result, 0, 1) ne "+") {
	    #($preinfix = $result) =~ /^[^+]*/; #this shit did not work at all
	    ($preinfix = $result) =~ s/\+.+//;
	    $result = reverse(substr($inflected, length($preinfix), 9999)) .
		'_+' . $result;
	}
	else {
	    $result = reverse($inflected) . '_+' . $result;
	}
    }
    else {
	$result = $result . &common_prefix($inflected, $canonical);
    }
    # now we need to extract the lexical form, and the categories
    # from $2
    # recall that $2 has the following structure (double quotes count):
    # "lexical_form" = "canonical_form"
    # or
    # "lexical_form"
    # in the latter case, the lexical form is identical to the canonical one
    # we assume the input is correct
    $S = substr($lex_desc, index($lex_desc, "\"") + 1, 999999);
    # drop the initial double qoute
    $lexical = substr($S, 0, index($S, "\""));
#    printf "cf: inverted=%s\n", $inverted;
#    printf "cf: lexical=%s\n", $lexical;
#    printf "cf: arch_ph=%s\n", $arch_ph;
    $result = $result . '+' .
	&common_archprefix($canonical, $lexical, $arch_ph) . '+' . $descrs;
    printf "%s\n", $result;
}

# The following function computes the common prefix of two words
# Parameters:
# 	s1	- the first word;
# 	s2	- the second word;
# Returns:
#	K1e1, where K1 says how many letters to delete at the end of s1
#       (A - 0, B - 1, C - 2, etc.), and e1 is what should be appended
#	to the result of the deletion to obtain s2.
# Note that the comparison in done up to n characters
sub common_prefix {
    local($s1, $s2) = @_;
    local($n, $i, $k1);
    $n = length($s1);
    for ($i = 0; $i <= $n - 1; $i++) {
	if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {
	    $k1 = sprintf('%c', (65 + length($s1) - $i));
	    return $k1 . substr($s2, $i, 999999);
	}
    }
    'A' . substr($s2, $n, 999999);
}

# The following function returns the smaller of two values
sub min {
    local($i1, $i2) = @_;
    (($i1 < $i2) ? $i1 : $i2);
}

# The following function computes the common prefix of two words in
# presence of arch-phonemes; the second word is considered to be the
# lexical form, and it may contain arch-phonemes
# Parameters:
#	s1	- canonical form
#	s2	- lexical form (may contain archphonemes)
#	aph	- list of archiphonemes that need to be recognized
# Local variables:
#	i	- position of the analysed character;
#		  later also the number of characters in s1 that correspond
#		  to the archphoneme in s2;
#	c	- currently analysed letter;
#		  later also the length of the remaining part of the stem;
#	j	- index of the first letter after the archphoneme in s2.
# Returns:
#	K2K3K4a1+e2 if an archphoneme from the list is present in s2;
#	K2Ae2 if the archphoneme not present.
sub common_archprefix {
    local($s1, $s2) = @_;
    local($archiphoneme, $a, $arch_pos, $head, $tail, $lexical_wo_archiphoneme,
	  $r1, $r2, $r3, $k2, $k3, $k4, $e2, $result, $pure_a, @aph);


#    printf "common_archprefix(%s, %s, %s)\n", $s1, $s2;
    @aph = split(/;/, $archiphonemes, 9999);
    $arch_pos = -1;
    foreach $a (@aph) {
	$pure_a = $a;
	$archiphoneme = '&' . $a . ';';
	if (($arch_pos = index($s2, $archiphoneme)) != -1) {
	    last;
	}
    }
#    printf "archiphoneme=%s\n", $archiphoneme;
    if ($arch_pos != -1) {
	# As we assume that the canonical form
	# does not contain prefixes or infixes, we have 3 possibilities:
	# the archiphoneme replaces 0, 1 or 2 characters.
	# We find which case gives us the longest common prefix
	($lexical_wo_archiphoneme = $s2) =~ s/$archiphoneme//;
	$head = substr($lexical_wo_archiphoneme, 0, $arch_pos);
	$tail = substr($lexical_wo_archiphoneme,$arch_pos,99);
	$r1 = &common_prefix($s1, $lexical_wo_archiphoneme);
	$r2 = &common_prefix($s1, $head . substr($s1, $arch_pos, 1) . $tail);
	$r3 = &common_prefix($s1, $head . substr($s1, $arch_pos, 2) . $tail);
	$k3 = sprintf("%c", 66 + $arch_pos);
	if (length($r1) < length($r2)) {
	    if (length($r1) < length($r3)) {
		$k2 = substr($r1, 0, 1);
		$k4 = 'A';
		$e2 = substr($r1, 1);
	    }
	    else {
		$k2 = substr($r3, 0, 1);
		$k4 = 'C';
		$e2 = substr($r3, 1);
	    }
	} elsif (length($r2) < length($r3)) {
	    $k2 = substr($r2, 0, 1);
	    $k4 = 'B';
	    $e2 = substr($r2, 1);
	} else {
	    $k2 = substr($r3, 0, 1);
	    $k4 = 'C';
	    $e2 = substr($r3, 1);
	}
	return $k2 . $k3 . $k4 . $pure_a . '+' . $e2;
    }
    else {
	# no archiphoneme
	$result = &common_prefix($s1, $s2);
	return substr($result, 0, 1) . 'A' . substr($result, 1);
    }
}

#--------------------------------------------------------------------------
# Obtain the coding for transforming the inflected form into the canonical one.
# The output has the form:
#
# prefixinfix+IKending
#
# where:
#   prefixinfix	is the prefix, or the prefix and the infix in inflected;
#               if there is no infix, this part is empty
#   I           is the infix length (A - no infix, B - 1, etc.)
#   K           is the length of the suffix in the inflected form (A - 0, etc.)
#   ending	is the ending of the canonical form.
#
# Parameters:
# inflected	- inflected form
# canonical	- canonical form
sub inf_prefix {
    local ($inflected_form, $canonical_form) = @_;
    local ($prefix, $prefix1, $prefix2, $result, $l1);

    $l1 = length($inflected_form);
    if (($prefix = &common_prefix_len($inflected_form, $canonical_form)) > 0) {
	# common prefix not empty, but we may have an infix
#	printf "prefix=%d\n", $prefix;
	($prefix_found, $prefix2) = &find_prefix($inflected_form,
						 $canonical_form);
	($infix_found,
	 $prefix1) = &find_prefix(substr($inflected_form, $prefix, 999999),
				  substr($canonical_form, $prefix, 999999));
	if ($prefix_found >= $infix_found) {
	    if (($prefix_found > 0) && ($prefix_found + $prefix2 > $prefix)) {
		# we have a rare case when we can have either a prefix,
		# or an infix,
		# like in German gegurrt/gurren
		# we prefer the longer one, or if equal - the prefix
		# here the prefix is longer than the infix
#		printf "prefix longer than infix\n";
		$result = sprintf '%s%sA%c%s',
		  substr($inflected_form, 0, $prefix_found), # prefix
		  $separator, # note that infix length is 'A'
		  ($l1 - $prefix_found - $prefix2 + 65), # suffix length
		  substr($canonical_form, $prefix2, 999999); # canonical ending
	    }
	    else {
		# both infix_found and prefix_found are 0
		# but still some characters at the beginning are the same
#		printf "normal case\n";
#		printf "l1=%d, l1-prefix=%d\n", $l1, $l1 - $prefix;
		$result = sprintf '%sA%c%s', 
		  $separator,	# note that infix length is 'A'
                  ($l1 - $prefix + 65),	# suffix length
		  substr($canonical_form, $prefix, 999999); # canonical ending
	    }
	}
	elsif ($prefix1 > 0) {
	    # we have an infix, and if there seems to be a prefix,
	    # the infix is longer
#	    printf "infix\n";
	    $result = sprintf '%s%s%c%c%s',
	      substr($inflected_form, 0, $prefix + $infix_found), # preinfix
	      $separator,
	      $infix_found + 65, # infix length
	      ($l1 - $prefix - $prefix1 - $infix_found + 65), # suffix length
	      substr($canonical_form, $prefix + $prefix1, 999999); # can ending
	}
	else {
	    # neither prefix, nor infix
#	    printf "normal case 2\n";
	    $result = sprintf '%sA%c%s%s%s',
		  $separator,	# no prefix nor infix, and infix length 'A'
                  ($l1 - $prefix + 65),	# suffix length
	          substr($canonical_form, $prefix, 999999); # canonical ending
	}
    }
    else {
#	printf "2. prefix=%d\n", $prefix;
	# we may have a prefix
	if (($prefix_found = &find_prefix($inflected_form,
					  $canonical_form)) > 0) {
#	    printf "prefix\n";
	    $result = sprintf '%s%sA%c%s',
	      substr($inflected_form, 0, $prefix_found), # prefix
	      $separator,	# note infix length 'A'
	      ($l1 - $prefix_found - $prefix1 + 65), # suffix length
	      substr($canonical_form, $prefix1, 999999); # canonical ending
	}
	else {
#	    printf "irregular\n";
	    $result = sprintf '%sA%c%s',
	      $separator,	# no prefix nor infix, infix length 'A'
	      length($inflected_form) + 65, # suffix length
	      $canonical_form;	# canonical ending
	}
    }
    return $result;
}


# Calculate the length of the prefix in s1 that should be removed to obtain s2
# Parameters:
# s1	- first word (inflected form)
# s2	- second word (canonical form)
# Result:
#	(i, prefix1),
# where:
#	i - length of the prefix
#	prefix1 - length of the common prefix of s1 and s2 after the prefix
#	          in s1 has been removed.
sub find_prefix {
    local($s1, $s2, $i) = @_;
    local ($prefix1);
#    printf STDERR "find_prefix(s1=%s, s2=%s)\n", $s1, $s2;
    local $l = length($s1);
    if ($l <= 1) {
        return (0, 0);
    }
    local $l_min =  $MAX_INFIX_LEN < $l ? $MAX_INFIX_LEN : $l ;
#    printf STDERR "l_min=%d\n", $l_min;
    $prefix1 = 0;
    for ($i = 1; $i <= $l_min; $i++) {
        if (($prefix1 = &common_prefix_len(substr($s1, $i, 999999), $s2,

          length($s1) - $i)) >= $MIN_COMMON_LEN) {
            return ($i, $prefix1);
        }
    }
    return (0, $prefix1);
}

sub common_prefix_len {
    local($s1, $s2, $n, $i) = @_;
    $n = length($s1);
    for ($i = 0; $i < $n; $i++) {
        if (substr($s1, $i, 1) ne substr($s2, $i, 1)) {
            return $i;
        }
    }
    $n;
}

#--------------------------------------------------------------------------
# Obtain the coding for transforming the inflected form into the canonical one.
# The output has the form:
#
# prefix+Kending
#
# where:
#   prefix	is the prefix, or the prefix and the infix in inflected;
#   K           is the length of the suffix in the inflected form (A - 0, etc.)
#   ending	is the ending of the canonical form.
#
# Parameters:
# inflected	- inflected form
# canonical	- canonical form
sub noinf_prefix {
    local ($inflected_form, $canonical_form) = @_;
    local ($prefix, $prefix1, $prefix2, $result, $result1, $l1);

#    printf "noinf_prefix(%s, %s)\n", $inflected_form, $canonical_form;
    $l1 = length($inflected_form);
    if (($prefix = &common_prefix_len($inflected_form, $canonical_form)) > 0) {
	$result = sprintf '%s%c%s%s%s',
		  $separator,	# no prefix
                  ($l1 - $prefix + 65),	# suffix length
	          substr($canonical_form, $prefix, 999999); # canonical ending
	# however, there might be a prefix after all
	($prefix_found, $prefix1) = &find_prefix($inflected_form,
						 $canonical_form);
	if ($prefix_found > 0) {
	    $result1 = sprintf '%s%s%c%s',
	      substr($inflected_form, 0, $prefix_found), # prefix
	      $separator,
	      ($l1 - $prefix_found - $prefix1 + 65), # suffix length
	      substr($canonical_form, $prefix1, 999999); # canonical ending
	    # if the solution with a prefix is shorter, take it!
	    if (length($result1) - $prefix_found < length($result)) {
		$result = $result1;
	    }
	}
    }
    else {
	# we may have a prefix
#	printf "we may have a prefix\n";
	($prefix_found, $prefix1) = &find_prefix($inflected_form,
						 $canonical_form);
	if ($prefix_found > 0) {
	    $result = sprintf '%s%s%c%s',
	      substr($inflected_form, 0, $prefix_found), # prefix
	      $separator,
	      ($l1 - $prefix_found - $prefix1 + 65), # suffix length
	      substr($canonical_form, $prefix1, 999999); # canonical ending
	}
	else {
	    $result = sprintf '%s%c%s',
	      $separator,	# no prefix
	      length($inflected_form) + 65, # suffix length
	      $canonical_form;	# canonical ending
	}
    }
    return $result;
}
