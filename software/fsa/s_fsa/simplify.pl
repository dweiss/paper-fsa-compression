#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
                        # this emulates #! processing on NIH machines.
                        # (remove #! line above if indigestible)

while (<>) {
    chop;
    simpl($_);
}

sub simpl {
    local($l) = @_;
    local($i,$j,$first,$lastp,$fa,$la);

    if (($i = index($l, "|")) > 0) {
	$first = substr($l, 0, $i);
	$lastp = substr($l, $i+1, 9999);
	$fa = reverse($first);
	$fa =~ s{[^A-Za-z0-9_]+.*$}{};
	$first = substr($first, 0, length($first) - length($fa));
	($la = $lastp) =~ s{[^A-Za-z0-9_|]+.*$}{};
	$lastp = substr($lastp, length($la), 9999);
	simpl($first . reverse($fa) . $lastp);
	while (($j = index($la, "|")) > 0) {
	    simpl($first . substr($la, 0, $j) . $lastp);
	    $la = substr($la, $j + 1, 9999);
	}
        simpl($first . $la . $lastp);
    }
    else {
	printf "%s\n", $l;
    }
}
