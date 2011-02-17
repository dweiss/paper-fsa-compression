# This script coverts data in the format:
# inflected_formHTlexemeHTtags
# (where HT is the horizontal tabulation)
# to the form:
# inverted_form_without_prefix+prefix+Kending+tags
# where inverted_form_without_prefix is the inflected form without a prefix
# (if the inflected form has one), and then inverted,
# '+' is an annotation separator,
# prefix is what was deleted from the beginning of the inflected form,
# K is a character that specifies how many characters
# should be deleted from the end of the inflected form to produce the lexeme
# by concatenated the stripped string with the ending ("A" means none,
# "B' - 1, "C" - 2, and so on).
#
# So the script tries to recognize prefixes automatically.
#
# The data is prepared for use with fsa_guess.
#
# You may want to change two pseudo-constants MAX_PREFIX_LEN and MAX_INFIX_LEN.
#
# Written by Jan Daciuk <jandac@pg.gda.pl>, 1999
#
# Calculate common prefix of s1 and s2, n is length of s1
function common_prefix(s1, s2, n,  i)
{
  for (i = 1; i <= n; i++)
    if (substr(s1, i, 1) != substr(s2, i, 1))
      return i - 1;
  return n;
}

# Calculate prefix in s1, and return its length (or 0)
# s1 is the inflected form 
# s2 is the canonical form 
function find_prefix(s1, s2,  i)
{
  for (i = 1; i <= MAX_INFIX_LEN; i++) {
    if ((prefix1 = common_prefix(substr(s1, i + 1), s2, length(s1) - i)) > 2)
      return i;
  }
  return 0;
}

BEGIN {separator = "+"}
{
  MAX_INFIX_LEN = 3; MAX_PREFIX_LEN = 3;
  l1 = length($1);
  # invert word
  s = "";
  for (i = 1; i <= l1; i++) {
     s = substr($1,i,1) s;
  }
  if ((prefix = common_prefix($1, $2, l1))) {
    printf "%s%c%c%c%s%c%s", s, separator, separator,
      (l1 - prefix + 65), substr($2, prefix + 1),
      separator, $3;
  }
  else {
    # we may have a prefix
    if ((prefix_found = find_prefix($1, $2)) > 0) {
      printf "%s%c%s%c%c%s%c%s", substr(s, 1, l1 - prefix_found),
	separator, substr(s, l1 - prefix_found + 1), separator
	(l1 - prefix_found - prefix1 + 65), substr($2, prefix1 + 1),
	separator, $3;
    }
    else
      printf "%s%c%c%c%s%c%s", $1, separator, separator, $2, separator, $3;
  }
# Delete the following (1) line if your tags do not contain spaces
# and you would like to append comments at the end of lines
  for (i = 4; i <= NF; i++) printf " %s", $i;
# Do not delete this
  printf "\n";
}
