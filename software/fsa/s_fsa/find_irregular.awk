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
function common_prefix(s1, s2, n,  i)
{
  for (i = 1; i <= n; i++)
    if (substr(s1, i, 1) != substr(s2, i, 1))
      return i - 1;
  return n;
}
BEGIN {separator = "+"}
{
  l1 = length($1);
  if ((prefix = common_prefix($1, $2, l1)) == 0) {
    printf "%s\n", $2;
  }
}
