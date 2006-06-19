#!/bin/sh

base=~zmap/prefix
script=`basename $0`

OSTYPE=`uname`
if [ "$OSTYPE" = 'OSF1' ]; then
    osbin='ALPHA/bin'
    LD_LIBRARY_PATH="$base/ALPHA/lib:/usr/local/gtk2/lib"
    myBuildDir=/nfs/team71/analysis/rds/workspace/ZMap/src/build/alpha
    myAceDir=/nfs/team71/acedb/edgrif/acedb/CODE/acedb/bin.ALPHA_5
#/nfs/disk100/acedb/RELEASE.SUPPORTED/bin.ALPHA_5
    export LD_LIBRARY_PATH
    echo $LD_LIBRARY_PATH
elif [ "$OSTYPE" = 'Linux' ]; then
    osbin='LINUX/bin'
    myBuildDir=/nfs/team71/analysis/rds/workspace/ZMap/src/build/linux
    myAceDir=/nfs/disk100/acedb/RELEASE.SUPPORTED/bin.LINUX_4
else
    echo "Unknown OSTYPE '$OSTYPE'"
fi

if [ -e $myBuildDir ]; then
   PATH="$myBuildDir:$myAceDir:$PATH"
   echo "I've set path to: \n $PATH"
fi

      PERL_EXE=/usr/local/bin/perl
     PERL_ARCH=`$PERL_EXE -MConfig -e 'print $Config{archname}'`
  PERL_VERSION=`$PERL_EXE -MConfig -e 'print $Config{version}'`

OTTER_HOME=$base/otter
export OTTER_HOME

PERL5LIB=\
$OTTER_HOME/tk:\
$OTTER_HOME/PerlModules:\
$OTTER_HOME/ensembl-ace:\
$OTTER_HOME/ensembl-otter/modules:\
$OTTER_HOME/ensembl-pipeline/modules:\
$OTTER_HOME/ensembl/modules:\
$OTTER_HOME/bioperl-0.7.2:\
$OTTER_HOME/bioperl-1.2:\
$base/CPAN/lib/perl5/site_perl/$PERL_VERSION/$PERL_ARCH:\
$base/CPAN/lib/perl5/site_perl/$PERL_VERSION:\
$base/CPAN/lib/perl5/$PERL_VERSION
export PERL5LIB

export PATH

exec $OTTER_HOME/tk/$script "$@"
