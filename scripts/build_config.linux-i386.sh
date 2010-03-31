#!/bin/echo dot script please source


# Required to get backtraces otherwise many symbols are not passed to ELF linker.
CFLAGS_args="$CFLAGS_args -rdynamic"


# I think we need /software/acedb/gtk as the default.
GTK_PREFIX=/software/acedb/gtk


# This shouldn't get overwritten.
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config

# autotools are also installed in here!
PATH=$GTK_PREFIX/bin:$PATH
