#!bash

# when you just want it all gone.
#
# it is not smart in any way, and will screw up your package manager
# if hardinfo is installed as a package.

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root."
   exit 1
fi

remove_hardinfo() {
  BASEDER="$1"
  echo "Removing hardinfo from $BASEDER ..."
  rm -rf "$BASEDER/share/hardinfo"
  rm -rf "$BASEDER/lib/hardinfo"
  rm -f "$BASEDER"/share/locale/*/LC_MESSAGES/hardinfo.mo
  rm -f "$BASEDER/bin/hardinfo"
  rm -f "$BASEDER/share/applications/hardinfo.desktop"
  rm -f "$BASEDER/share/doc/hardinfo"
  rm -f "$BASEDER/share/man/man1/hardinfo.1.gz"
  rm -f "$BASEDER/share/pixmaps/hardinfo.xpm"
  rm -f "$BASEDER/share/menu/hardinfo"
  #locate -e hardinfo | grep "$BASEDER"
}

remove_hardinfo "/usr/local"
remove_hardinfo "/usr"
# add some other base install path here
