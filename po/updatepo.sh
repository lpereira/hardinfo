#!/bin/bash

GITVER=`git describe --always --dirty`
GITHASH=`git rev-parse HEAD`

echo "Update gettext translation files."

DER=`pwd`
if [ ! -e "updatepo.sh" ]
then
    echo "Error: Run from po/, the location of hardinfo2.pot and XX.po files."
    exit 1
fi

MSGTOTALOLD=`msgattrib --untranslated hardinfo2.pot | grep -E "^msgstr \"\"" | wc -l`
echo "hardinfo2.pot has $MSGTOTALOLD strings"

echo "" > hardinfo2.pot # empty existing file to join (-j) with
for d in hardinfo2/ shell/ modules/ includes/;
do
    # work from hardinfo2 root to get reasonable file reference comments
    cd ..
    echo -n `pwd`; echo "/$d ..."
    find "$d" -type f -name "*.[hc]" -print | sort | xargs xgettext -j -d hardinfo2 -o "$DER/hardinfo2.pot" -k_ -kN_ -kC_:1c,2 -kNC_:1c,2 -c*/ --from-code=UTF-8
    cd "$DER"
done;

MSGTOTAL=`msgattrib --untranslated hardinfo2.pot | grep -E "^msgstr \"\"" | wc -l`
CMSG=`msgattrib --untranslated hardinfo2.pot | grep -E "^#,.*c-format" | wc -l`

TDIFF=$(($MSGTOTAL - $MSGTOTALOLD))
CHANGE="$TDIFF"
if [ $TDIFF -gt 0 ]; then CHANGE="+$TDIFF"; fi
if [ $TDIFF -eq 0 ]; then CHANGE="no change"; fi
echo "hardinfo2.pot now has $MSGTOTAL strings ($CHANGE), with $CMSG c-format strings"
echo "(as of $GITVER $GITHASH)"

for f in *.po
do
    
    msgmerge -q -N "$f" hardinfo2.pot > tmp.po

    # set/reset the X-Poedit-Basepath header
    grep -v '"X-Poedit-Basepath\:[^"]*"' tmp.po | sed 's:\("Language\:[^"]*"\):\1\n"X-Poedit-Basepath\: ../\\n":' >"$f"

    rm -f tmp.po

    # stats
    UNMSG=`msgattrib --untranslated "$f" | grep -E "^msgstr \"\"" | wc -l`
    FUZMSG=`msgattrib --translated "$f" | grep -E "^#,.*fuzzy" | wc -l`
    DONE=" "; if [ $UNMSG -eq 0 ]; then DONE="x"; fi
    echo "- [$DONE] $f : ($UNMSG / $MSGTOTAL remain untranslated, needs work/fuzzy: $FUZMSG)"
done
