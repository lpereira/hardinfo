#!bash

# checks for mixed indentation and empty lines with whitespace
# run from test/

cd ..
grep -lHIrP --include=*.{h,c} -- '^((\t+ +)|( +\t+)|\s+$)' | sed 's/^/- [ ] /' > test/hardinfo-bad.txt
grep -LHIrP --include=*.{h,c} -- '^((\t+ +)|( +\t+)|\s+$)' | sed 's/^/- [x] /' > test/hardinfo-good.txt
cat test/hardinfo-bad.txt test/hardinfo-good.txt | LC_ALL=C sort -k 1.5 | grep -vP " (test|build)"
rm test/hardinfo-bad.txt test/hardinfo-good.txt
cd test
