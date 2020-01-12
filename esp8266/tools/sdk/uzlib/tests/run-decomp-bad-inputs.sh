#!/bin/sh

# Make sure that globbing order below is independent on the current
# user locale
export LANG=C.UTF-8
unset LC_COLLATE

rm -f decomp-bad-inputs.log

for f in decomp-bad-inputs/*/*; do
    echo "*" $f
    ../examples/tgunzip/tgunzip "$f" /dev/null
    echo $f $? >>decomp-bad-inputs.log
done

echo

if diff -u decomp-bad-inputs.ref decomp-bad-inputs.log; then
    echo "Test passed"
else
    echo "Test FAILED"
fi
