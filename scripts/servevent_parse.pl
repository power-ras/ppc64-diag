#
# Copyright 2007 IBM Corporation
#
# parse_se
# Parses serviceable event data that is stored in servicelog.  The data should
# be made available on stdin.
#
# First return value:   a hash containing all the variables in the serviceable
#                       event except for FRU callouts
# Second return value:  an array containing all the FRU callouts; each
#                       element contains a space-separated callout description
sub parse_se
{
	my %se_vars = ();
	my @callouts = ();

	while (<STDIN>) {
		chomp;

		$pos = index($_, ":");
		if ($pos > 0) {
			$tag = substr($_, 0, $pos);
			$value = substr($_, $pos+2);
		}
		else {
			$tag = $_;
			$value = "";
		}

		if ($tag eq "Callout") {
			($p1, $p2, $p3, $p4, $p5, $p6, $p7, $p8) = split /\s/, $value;
			push(@callouts, "$p1 $p2 $p3 $p4 $p5 $p6 $p7 $p8");
		}
		elsif (substr($tag, 0, 8) eq "AddlWord") {
			$se_vars{$tag} = sprintf("%08X", hex(lc($value)));
		}
		else {
			$se_vars{$tag} = $value;
		}
	}
	return (\%se_vars, \@callouts);
}

1;
