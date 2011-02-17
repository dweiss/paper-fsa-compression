# This script transforms the output of fsa_morph into the 3 column one.
{
    colon_index = index($0, ": ");
    word = substr($0, 1, colon_index - 1);
    rest = substr($0, colon_index + 2);
    n = split(rest, descr, /, /);
    for (i = 1; i <= n; i++) {
	plus_index = index(descr[i], "+");
	printf "%s\t%s\t%s\n", word, substr(descr[i], 1, plus_index - 1),
	    substr(descr[i], plus_index + 1);
    }
}

