
for f in *.po
do
    cp "$f" "$f.old"
    msgmerge -N "$f" hardinfo.pot > tmp.po && mv tmp.po "$f"
done
