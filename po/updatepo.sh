#!bash

echo "Update gettext translation files."

DER=`pwd`
if [ ! -e "updatepo.sh" ]
then
    echo "Error: Run from po/, the location of hardinfo.pot and XX.po files."
    exit 1
fi

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

for f in *.po
do
    cp "$f" "$f.old"
    msgmerge -N "$f" hardinfo.pot > tmp.po && mv tmp.po "$f"
done
