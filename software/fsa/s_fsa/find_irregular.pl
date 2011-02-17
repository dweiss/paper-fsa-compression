#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# process any FOO=bar switches

# This script finds what it thinks are irregular words, and prints their
# lexemes. The data is in the form:
# inflected_formHTlexemeHTtags
# The decision whether the word is irregular or not is taken on the
# basis of the match between the inflected form and the corresponding
# lexeme. If we can obtain the lexeme from the inflected form by
# cutting off some (possibly none) characters from the end of the
# inflected form, and appending some (possibly none) characters at the
# end of the result of it, and we are able to do that without deleting
# the whole inflected form and appending the whole lexeme, than we
# treat the word as regular. Otherwise we treat the word as irregular.
#
# Certainly, the same lexeme may look regular and irregular with
# different inflected forms derived from it. We need only one such
# form to declare it irregular.
#
# Returns the length of the common prefix of s1 and s2

$separator = '+';

while (<>) {
    ($Fld1,$Fld2) = split('\t', $_, 9999);

    $l1 = length($Fld1);
    if (($prefix = &common_prefix($Fld1, $Fld2, $l1)) == 0) {
	printf "%s\n", $Fld2;
    }
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
