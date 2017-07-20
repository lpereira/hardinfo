#!bash

echo "Update gettext translation files."

DER=`pwd`
if [ ! -e "updatepo.sh" ]
then
    echo "Error: Run from po/, the location of hardinfo.pot and XX.po files."
    exit 1
fi

MSGTOTALOLD=`msgattrib --untranslated hardinfo.pot | grep -E "^msgstr \"\"" | wc -l`
echo "hardinfo.pot has $MSGTOTALOLD strings"

mv hardinfo.pot hardinfo.pot.old
echo "" > hardinfo.pot # empty existing file to join (-j) with
for d in hardinfo/ shell/ modules/;
do
    # work form hardinfo root to get reasonable file reference comments
    cd ..
    echo -n `pwd`; echo "/$d ..."
    find "$d" -type f -name "*.c" -print | sort | xargs xgettext -j -d hardinfo -o "$DER/hardinfo.pot" -k_ -kN_ -kC_:1c,2 -kNC_:1c,2 -c/ --from-code=UTF-8
    cd "$DER"
done;

MSGTOTAL=`msgattrib --untranslated hardinfo.pot | grep -E "^msgstr \"\"" | wc -l`
TDIFF=$(($MSGTOTAL - $MSGTOTALOLD))
CHANGE="$TDIFF"
if [ $TDIFF -gt 0 ]; then CHANGE="+$TDIFF"; fi
if [ $TDIFF -eq 0 ]; then CHANGE="no change"; fi
echo "hardinfo.pot now has $MSGTOTAL strings ($CHANGE)"

for f in *.po
do
    cp "$f" "$f.old"
    msgmerge -q -N "$f" hardinfo.pot > tmp.po && mv tmp.po "$f"
    UNMSG=`msgattrib --untranslated "$f" | grep -E "^msgstr \"\"" | wc -l`
    DONE=" "
    if [ $UNMSG -eq 0 ]; then
       DONE="x"
    fi
    echo "- [$DONE] $f : ($UNMSG / $MSGTOTAL remain untranslated)"
done
