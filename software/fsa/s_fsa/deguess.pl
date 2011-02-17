#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# process any FOO=bar switches

# This script transforms the output of fsa_morph into the 3 column one.
$[ = 1;			# set array base to 1

while (<>) {
    chomp;	# strip record separator
    @Fld = split(' ', $_, 9999);

    $colon_index = index($_, ': ');
    $inverted = substr($_, 1, $colon_index - 1);
    $wl = length($Fld[1]);
    $word = '';
    for ($j = 1; $j <= $wl; $j++) {
	$word = substr($inverted, $j, 1) . $word;
    }
    $rest = substr($_, $colon_index + 2, 999999);
    $n = (@descr = split(/, /, $rest, 9999));
    for ($i = 1; $i <= $n; $i++) {
	$plus_index = index($descr[$i], '+');
	printf "%s\t%s\t%s\n", $word, substr($descr[$i], 1, $plus_index - 1), 
	substr($descr[$i], $plus_index + 1, 999999);
    }
}
