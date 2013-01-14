
# set up path and library environment to allow zmap BAM scripts to work
# NOTE this uses an existing otterlace installation
# we need to set up and export a complete Perl environment w/ ZMap
# or supply scripts as a separate add-on

anasoft="/software/anacode"
OTTER_HOME=/software/anacode/otter/otter_dev

anasoft_distro="$anasoft/distro/$( $anasoft/bin/anacode_distro_code )"

otterbin="\
$OTTER_HOME/bin:\
$anasoft_distro/bin:\
$anasoft/bin:\
/software/pubseq/bin/EMBOSS-5.0.0/bin:\
/software/perl-5.12.2/bin\
"

if [ -n "$PATH" ]
then
    PATH="$otterbin:$PATH"
else
    PATH="$otterbin"
fi
export PATH

local=/nfs/users/nfs_m/mh17/Perl/lib;

PERL5LIB="\
$OTTER_HOME/PerlModules:\
$OTTER_HOME/ensembl-otter/modules:\
$OTTER_HOME/ensembl-analysis/modules:\
$OTTER_HOME/ensembl/modules:\
$OTTER_HOME/ensembl-variation/modules:\
$OTTER_HOME/lib/perl5:\
$OTTER_HOME/ensembl-otter/tk:\
$anasoft_distro/lib:\
$anasoft_distro/lib/site_perl:\
$anasoft/lib:\
$anasoft/lib/site_perl:\
$local\
"


