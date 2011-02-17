#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# 'process any FOO=bar switches
# This script first looks into a preamble of a morphological
# dictionary. It attempts to find there declarations of types
# (i.e. mostly parts of speech) and the attributes (features)
# associated with them. Then it builds lists used for comparison.
# The comparison is then used in sorting descriptions of words.
# The data for sorting is taken from the standard input, and the
# results are written on the standard output.

# Variables subject to customization
# the preamble file
$preamble="preamble";

open(PREAMBLE, "< $preamble") or die "Cannot open file $preamble : $!\n";
$current_state = 0;		# outside relevant sections
$current_class = "";		# current attribute or type name
$ident_visible = 0;		# no identifiers matter outside sections
while (<PREAMBLE>) {
    chop;			# strip record separator

    s/;.*//g;			# strip comments

    while (/[\x30-\xff]/) {
#	printf "while: %s\n", $_;
	if (/^\s*@\s*Attributes/i) {
	    # beginning of the attribute section
	    $current_state = 1;
	    s/^\s*@\s*Attributes//i; # remove from the current line
	}
	elsif (/^\s*@\s*Types/i) {
	    # beginning of the types section
	    $current_state = 2;
	    s/^\s*@\s*Types//i;	# remove from the current line
	}
	elsif (/^\s*@\s*[A-Za-z]+/) {
	    # beginninge of a different, irrelevant section
	    $current_state = 0;
	    s/^\s*@\s*[A-Za-z]+//; # remove from the current line
	}
	elsif ($current_state == 0) {
	    # irrelevant section
	    # delete everything except the beginning of a new section
	    s/^[^@]*//;
	}
	elsif (/^b*\|/) {
	    $ident_visible = 0;
	    s/^b*\|//;		# remove from the current line
	}
	elsif (/^\s*[0-9a-z_]+\s*:/i) {
	    # class (attribute, type) identifier
	    $ident_visible = 1;
	    ($ident = $_) =~ s/^\s*([0-9a-z_]+)\s*:.*/$1/i;
	    s/^\s*[0-9a-z_]+\s*://i; # remove from the current line
	    $current_class = $ident;
	}
	elsif (/^\s*[0-9a-z_]+/i) {
	    # class (attribute, type) value
	    ($ident = $_) =~ s/^\s*([0-9a-z_]+).*/$1/i;
	    s/^\s*[0-9a-z_]+//i; # remove from the current line
	    if ($current_state == 1) {
		if (exists($attrs{$current_class})) {
		    $attrs{$current_class} = $attrs{$current_class} .
			',' . $ident;
		}
		else {
		    $attrs{$current_class} = $ident;
		}
	    }
	    else {
		if (exists($types{$current_class})) {
		    $types{$current_class} = $types{$current_class} .
			',' . $ident;
		}
		else {
		    $types{$current_class} = $ident;
		}
	    }
	}
	else {
	    # delete everything except the new section
	    s/^[^@]*//;
	}
    }
}
close(PREAMBLE);
# \x3c\x3e
@lines = sort { compare($a, $b) } <>;
foreach $l (@lines) { printf "%s", $l; }

sub compare {
    local ($a, $b) = @_;
    local ($leftbr, $c, $d, $cv, $ca, $cb, $av);

#    printf "comparing %s and %s", $a, $b;
    foreach $c (keys %types) {
	$leftbr = '\[';
	if (($d = $a) =~ /$c$leftbr/) {
	    if (($d = $b) =~ /$c$leftbr/) {
		# both categories present, compare attributes
		foreach $cv (split(/,/, $types{$c})) {
#		    printf "checking %s\n", $cv;
		    if (($d = $a) =~ /$cv=/) {
			# attribute present in $a, find its values
			($ca = $a) =~ s/.*\s$cv\s*=\s*([a-z0-9_|]+)\s.*/$1/i;
#			printf "a: found %s=%s\n", $cv, $ca;
			if (($d =$b) =~ /$cv=/) {
			    # attribute present in $b, find its values
			    ($cb = $b) =~
				s/.*\b$cv\s*=\s*([a-z0-9_|]+)\b.*/$1/i;
#			    printf "b: found %s=%s\n", $cv, $cb;
#			    printf "cat(%s)=%s\n", $cv, $attrs{$cv};
			    foreach $av (split(/,/,$attrs{$cv})) {
#				printf "checking: %s\n", $av;
				if (($d = $ca) =~ /\b$av\b/) {
#				    printf "a:%s found in %s\n", $av,$ca;
				    # attribute value present in $a
				    if (($d = $cb) =~ /\b$av\b/) {
#					printf "b:%s found in %s\n", $av,$cb;
					# attribute value present in $b
					# compare other values
					next;
				    }
				    else {
#					printf "b:%s not found in %s\n", $av,$cb;
					# attribute value in $a, but not in $b
					# $a is smaller
					return -1;
				    }
				}
				elsif (($d = $cb) =~ /\b$av\b/) {
#				    printf "a:%s not found in %s\n", $av,$ca;
#				    printf "b: found %s=%s\n", $cv, $cb;
				    # attribute value in $b, but not in $a
				    # $b is smaller
				    return 1;
				}
				# not present in both, pass
#				printf "a:%s not found in %s\n", $av,$ca;
#				printf "b:%s not found in %s\n", $av,$cb;
			    }
			}
			else {
			    # attribute present present in $a, but not in $b
			    # $b is smaller
			    return -1;
			}
		    }
		    elsif (($d = $b) =~ /$cv=/) {
			# attribute present in $b, but not in $a
			# $a is smaller
			return 1;
		    }
		    # else both attributes not present, pass
		}
	    }
	    else {
		# category present in $a, but not in $b
		# $b is smaller
		return -1;
	    }
	}
	elsif (($d = $b) =~ /$c$leftbr/) {
	    # category present in $b, but not in $a
	    # $a is smaller
	    return 1;
	}
	# else both categories not present, pass
    }
    # if we got here, both entries have identical descriptions
    return 0;
}
