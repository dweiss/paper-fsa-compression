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
# You may want to change 3 pseudo-constants MAX_PREFIX_LEN, MAX_INFIX_LEN,
# and MIN_ILEN.
#
# Written by Jan Daciuk <jandac@pg.gda.pl>, 1999
#

# Calculate common prefix of s1 and s2, and return its length
# s1 is the inflected form
# s2 is the canonical form
# n is length of s1
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
  MAX_INFIX_LEN = 3; MAX_PREFIX_LEN = 3; MIN_ILEN;
  l1 = length($1);
  # invert word
  s = "";
  for (i = 1; i <= l1; i++) {
     s = substr($1,i,1) s;
  }
  if ((prefix = common_prefix($1, $2, l1))) {
    # common prefix not empty, but we may have an infix
    prefix_found = find_prefix($1, $2);
    prefix2 = prefix1;
    if (prefix_found >= (infix_found = find_prefix(substr($1, prefix + 1),
				   substr($2, prefix + 1)))) {
      if ((prefix_found > 0) && (prefix2 > prefix)) {
        # we have a rare case when we can have either a prefix, or an infix,
        # like in German gegurrt/gurren
	# we prefer the longer one, or if equal - the prefix
	# here the prefix is longer than the infix
	printf "%s_%c%s%cA%c%s%c%s", substr(s, 1, l1 - prefix_found),
	  separator, substr($1, 1, prefix_found), separator,
	  (l1 - prefix_found - prefix2 + 65), substr($2, prefix2 + 1),
	  separator, $3;
      }
      else {
	# both infix_found and prefix_found are 0
	# but still some characters at the beginning are the same
	printf "%s_%c%cA%c%s%c%s", s, separator,
	  separator, (l1 - prefix + 65), substr($2, prefix + 1),
	  separator, $3;
      }
    }
    else if (prefix1 > 0) {
      # we have an infix, and if there seems to be a prefix,
      # the infix is longer
      printf "%s_%c%s%c%c%c%s%c%s", substr(s, 1, l1 - prefix - infix_found),
	separator, substr($1, 1, prefix + infix_found), separator,
	infix_found + 65,
	(l1 - prefix - prefix1 - infix_found + 65),
	substr($2, prefix + prefix1 + 1), separator, $3;
    }
    else {
      # neither prefix, nor infix
      printf "%s_%c%cA%c%s%c%s", s, separator,
	separator, (l1 - prefix + 65), substr($2, prefix + 1),
	separator, $3;
    }
  }
  else {
    # we may have a prefix
    if ((prefix_found = find_prefix($1, $2)) > 0) {
      printf "%s_%c%s%cA%c%s%c%s", substr(s, 1, l1 - prefix_found),
	separator, substr($1, 1, prefix_found), separator,
	(l1 - prefix_found - prefix1 + 65), substr($2, prefix1 + 1),
	separator, $3;
    }
    else {
      printf "%s_%c%cA%c%s%c%s", s, separator, separator, length($1) + 65,
	$2, separator, $3;
    }
  }
# Delete the following (1) line if your tags do not contain spaces
# and you would like to append comments at the end of lines
  for (i = 4; i <= NF; i++) printf " %s", $i;
# Do not delete this
  printf "\n";
}
