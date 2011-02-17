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
function common_prefix(s1, s2, n,  i)
{
  for (i = 1; i <= n; i++)
    if (substr(s1, i, 1) != substr(s2, i, 1))
      return i - 1;
  return n;
}
BEGIN {separator = "+"}
{
  s = "";
  l1 = length($1);
  # invert word
  for (i = 1; i <= l1; i++) {
     s = substr($1,i,1) s;
  }
  if ((prefix = common_prefix($1, $2, l1))) {
    printf "%s_%c%c%s%c%s", s, separator, 
      (l1 - prefix + 65), substr($2, prefix + 1),
      separator, $3;
  }
  else {
    printf "%s_%c%c%s%c%s", s, separator,
      65 + l1, $2, separator, $3;
  }
# Delete the following line if your tags do not contain spaces,
# and you want to put comments at the end of lines
  for (i = 4; i <= NF; i++) printf " %s", $i;
# Do not delete this line
  printf "\n";
}
