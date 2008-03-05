#!/bin/bash

# Because for some reason autotools can't exec $() and we're lazy I've
# written a script to make the lists of scripts

SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
   BASE_DIR=$SCRIPT_DIR
fi

. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }


# ================= CONFIG =================
    SRC_DIRS="zmapConfig zmapControl zmapDAS zmapDocs zmapDraw \
zmapFeature zmapGFF zmapUtils zmapView zmapWindow"
XML_SRC_DIRS="zmapXML"
APP_SRC_DIRS="zmapApp zmapManager"
THR_SRC_DIRS="zmapServer zmapThreads"
INC_SRC_DIRS="ZMap"


# ================= MAIN PART =================

SILENT_CD=yes

zmap_cd $BASE_DIR

zmap_goto_cvs_module_root

cvs_root=$(pwd)

zmap_cd src

libZMapXML_a_SOURCES=$(find $XML_SRC_DIRS -name '*.[ch]' | grep -v CVS | \
    sed -e 's!\.c!.c \\!g; s!\.h!.h \\!g; s!^zmap!\$\(top_srcdir\)/zmap!g')

libZMapApp_a_SOURCES=$(find $APP_SRC_DIRS -name '*.[ch]' | grep -v CVS | \
    sed -e 's!\.c!.c \\!g; s!\.h!.h \\!g; s!^zmap!\$\(top_srcdir\)/zmap!g')

libZMap_a_SOURCES=$(find $SRC_DIRS -name '*.[ch]' | grep -v CVS | \
    sed -e 's!\.c!.c \\!g; s!\.h!.h \\!g; s!^zmap!\$\(top_srcdir\)/zmap!g')

libZMapThr_a_SOURCES=$(find $THR_SRC_DIRS -name '*.[ch]' | grep -v CVS | \
    sed -e 's!\.c!.c \\!g; s!\.h!.h \\!g; s!^zmap!\$\(top_srcdir\)/zmap!g')

zmap_cd include

nobase_include_HEADERS=$(find $INC_SRC_DIRS -name '*.[ch]' | grep -v CVS | \
    sed -e 's!\.c!.c \\!g; s!\.h!.h \\!g;')


# ================= OUTPUT =================

zmap_message_out "Code to include in $cvs_root/src/lib/Makefile.am"

cat <<EOF

NULL = 

libZMapXML_a_SOURCES = $libZMapXML_a_SOURCES
\$(NULL)

libZMapApp_a_SOURCES = $libZMapApp_a_SOURCES
\$(NULL)

libZMap_a_SOURCES = $libZMap_a_SOURCES
\$(NULL)

libZMapThr_a_SOURCES = $libZMapThr_a_SOURCES
\$(NULL)



EOF

zmap_message_out "Code to include in $cvs_root/src/include/Makefile.am"

cat <<EOF

NULL = 

nobase_include_HEADERS=$nobase_include_HEADERS
\$(NULL)

EOF

# ================= END OF SCRIPT =================
