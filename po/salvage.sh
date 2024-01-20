#!bash

# To help with https://github.com/hwspeedy/hardinfo/issues/100
# Saves the old label translations.
#
# This is not an automated magical tool, it is an ugly hack of a helper
# for a tedious process. Read the instructions if you want to use it!
#
# bash updatepo.sh
# { commit here or the next commit's diff will be noisy and useless }
#
# { repeat for each language... }
#     bash salvage.sh XX.po >XX.po.salv
#     { edit XX.po.salv, remove nonsense }
#     msgcat --use-first XX.po XX.po.salv > XX.po.merged
#     diff XX.po XX.po.merged
#     { if there are any added translations, they do not actually appear
#       in hardinfo.pot, and they will end up "obsolete" and then re-processed
#       in the next salvage. So save some hassle and just remove them now. }
#     mv XX.po.merged XX.po
#
# { when all languages are done run updatepo.sh again to clean up }
# bash updatepo.sh
#

if [ -z "$@" ]; then
    echo "READ script before running!"
    echo "Error: Needs .po file to process"
    exit 1
fi

do_salvage() {
    msgid=()
    msgstr=()

    while IFS= read -r line; do
        msgid+=( "$line" )
    done <<< "$MSGEXEC_MSGID"

    while IFS= read -r line; do
        msgstr+=( "$line" )
    done <&0

    for i in ${!msgid[@]}; do
        # column titles
        msgid[$i]=`echo "${msgid[$i]}" | sed -e 's/ColumnTitle\$.*=//'`
        msgstr[$i]=`echo "${msgstr[$i]}" | sed -e 's/ColumnTitle\$.*=//'`

        # section titles
        msgid[$i]=`echo "${msgid[$i]}" | sed -e 's/\[\(.*\)\]/\1/'`
        msgstr[$i]=`echo "${msgstr[$i]}" | sed -e 's/\[\(.*\)\]/\1/'`

        # regular labels
        msgid[$i]=`echo "${msgid[$i]}" | sed -e 's/=.*$//'`
        msgstr[$i]=`echo "${msgstr[$i]}" | sed -e 's/=.*$//'`

        # junk
        msgid[$i]=`echo "${msgid[$i]}" | sed -e 's/%s//'`
        msgid[$i]=`echo "${msgid[$i]}" | sed -e 's/%s//'`

        if [[ -n "${msgid[$i]}" && -n "${msgstr[$i]}" ]]; then
            if [ "${msgid[$i]}" != "${msgstr[$i]}" ]; then
                echo "msgid \"${msgid[$i]}\""
                echo "msgstr \"${msgstr[$i]}\""
                echo ""
            fi
        fi
    done
}

cat >salvage.tmp <<'EOT'

msgid ""
msgstr ""
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

EOT

export -f do_salvage
msgattrib --only-obsolete "$@" | msgexec bash -c 'do_salvage "$0"' >>salvage.tmp
msguniq salvage.tmp
rm salvage.tmp
