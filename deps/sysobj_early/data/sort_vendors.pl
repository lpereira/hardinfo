#!/usr/bin/perl

# Usage:
#   sort_vendors.pl <some_path/vendor.ids>
# Output goes to stdout.

use utf8;
use strict;
#use File::Slurp::Unicode qw(read_file);
sub read_file {
    my $file = shift;
    return do {
        local $/ = undef;
        open my $fh, "<", $file || die "Could not open $file: $!";
        <$fh>;
    };
}

my $file = shift;
die "Invalid file: $file" unless (-f $file && -r $file);
my $raw = read_file($file);
$raw =~ s/^\n*|\n*$//g;

my $c = 0;
my @coms = ();
my %vendors = ();
SEC: foreach my $s (split(/\n{2,}/, $raw)) {
    my @lines = split(/\n/, $s);
    foreach my $l (@lines) {
        if ($l =~ /^name\s+(.*)/) {
            $vendors{"$1_$c"} = $s;
            $c++;
            next SEC;
        }
    }
    push @coms, $s;
}
foreach (@coms) {
    print "$_\n\n";
}
foreach (sort { lc $a cmp lc $b } keys %vendors) {
    print "$vendors{$_}\n\n";
}
