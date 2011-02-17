#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# 'process any FOO=bar switches

# This script converts output of mmorph to 3 column HT-separated format.
# mmorph output has the inflected form (first column), and the base form
# (third column) in double quotes, and the second column is an equal sign.
# Spaces after the left square bracket and before the right square bracket
# are removed as well.
# The inflected form, and the base form may contain spaces!
# Author: Jan Daciuk
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

while (<>) {
    chop;	# strip record separator

    $s = "[^\"]*\"", s/$s//;
    # delete first double quote
    # Now the first double quote is the one ending the inflected form
    $s = "\"[^\"]*\"", s/$s/\t/;
    # convert the ending double quote of inflected
    # form, the equal sign, the beginning double
    # quote of the base form, and all white space
    # between them to a HT character
    $s = "\" *", s/$s/\t/;
    # convert the ending double quote of base form
    # and the following spaces to HT
    $s = "\\[ ", s/$s/[/g;
    # delete space after [
    $s = " \\]", s/$s/]/g;
    # delete space before ]
    $s = '_', s/$s/ /g;
    # mmorph uses _ to indicate spaces between 
    # words; I use it for various things, but
    # I can use spaces
    print $_;
}
