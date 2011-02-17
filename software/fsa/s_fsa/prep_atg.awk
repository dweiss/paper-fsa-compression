# This script translates data in the form:
# inflected_formHTlexemeHTtags
# to the form:
# inverted_form+tags
# where the inverted_form is the inverted inflected_form, i.e. the last
# character of the inflected form becomes the first character of the
# inverted_form, the second - the penultimate, and so on.
# Before the inversion, a filler character ('_') is put at the front
# of the inflected form, so it becomes the last character
# of the inverted form.
# The script is intended to be used to prepare data for fsa_build -X
# to build an automaton used by fsa_guess compiled _without_ GUESS_LEXEMES.
{
  s = "";
  for (i = 1; i <= length($1); i++) {
     s=substr($1,i,1) s;
  }
  printf "%s_+%s", s, $3;
# Delete the following line if your tags do not contain spaces
# and you want to put comments at the end of lines
  for (i = 4; i <= NF; i++) printf " %s", $i;
# Do not delete this line
  printf "\n";
}
