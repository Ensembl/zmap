#!/bin/bash


opsys=`uname`

case $opsys in

 Darwin )

    export AC_LOCAL_INC=" -I /opt/local/share/aclocal "

    ;;

esac


autoreconf --force --install --verbose -I m4

