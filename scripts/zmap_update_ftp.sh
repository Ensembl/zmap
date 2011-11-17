#!/bin/ksh
#
# A script to load the distro tar files to the Sanger ftp site and also update the links on the zmap web page
# Derived from the Seqtools update_website.sh writtem by Gemma Barson
# recent bugs added by mh17
# I removed a few things like 'copy release notes and documentation'
# please refer to the original script for inspiration
#
# NOTE variously we use:
# home dir for build checkout and scripts and git version	home/ZMap
# svn dir for web html		   home/zmap_web
# build dir to get the tarballs  /nfs/zmap/somewhere via a symlink from zmap home
# ftp root	to upload the tarballs
#
#
# NOTE original comment from seqtools
# This script updates the external Sanger website, FTP site and intranet
# with details of the latest overnight, development or production build (as
# specified by the given type and build directory). It:
#  * copies the distribution tarball into the relevant directory on the FTP site;
#  * updates the external website with a link to the new build on the FTP site;
#  * copies the user manuals into the Docs directory on the FTP site (production build only);
#  * copies the source-code documentation to the intranet (overnight build only).
#
#                        **WARNING**
# If you alter this script you should be aware that it is called as
# a cron job on tviewsrv from the zmap userid to update the OVERNIGHT
# copy of the SeqTools install stuff on the website, FTP site and intranet.
#

home_dir=~zmap/BUILD_CHECKOUT                        # root directory for zmap checkout
scripts_dir=$home_dir/ZMap/scripts                        # scripts directory

msg_prefix="(`basename $0` on `hostname`)"

repo_name=zmap                           		     # repository name
website_svn_subdir=zmap_web                          # subdirectory of home_dir that contains the website svn repositories
# NOTE this is initialised manually via:
# cd ~zmap/BUILD_CHECKOUT
# svn co svn+ssh://web-svn.internal.sanger.ac.uk/repos/svn/www-sanger-ac-uk/trunk/htdocs/resources/software/zmap zmap_web zmap_web
# we assume they are not edited by hand and therefoere we won't get merge conflicts

#intweb_root=/nfs/WWWdev/INTWEB_docs/htdocs/Software  # root directory for intweb docs
#intweb_source_docs_dir=source_code_docs              # subdirectory on the intweb where the source docs live

########################### subroutines ###################################

# Print the supplied error message and die...
function bomb_out
{
  print
  print "$msg_prefix Failed to update website: $*"
  print
  exit 1
}


# This checks the status of the last call and if there was
# an error it bombs out (with the optional message arg)
function check_status
{
  if [ $? -ne 0 ]
  then
    bomb_out "$*"
  fi
}


# Print a warning to stderr...
function warning
{
  print -u2 "\n*** $* ***\n"
}


function print_divider
{
  print
  print '====================================================================='
  print
}


function print_with_dividers
{
  now=`date`
  print
  print "=========================================$now"
  print
  print "  $msg_prefix $*"
  print
  print '====================================================================='
  print
}



function print_usage
{
  if [[ -n $1 ]]
  then
    print
    print "Warning: $*"
  fi

  print
  print "Usage:        zmap_update_ftp.sh [-r] -t <type>]"
  print
  print "  -r  remove target directory if it exists."
  print
  print "  -t <type> OVERNIGHT | DEVELOPMENT | PRODUCTION  selects archive on ftp site (DEVELOPMENT is default),"
  print "                     archive goes to ~ftp/pub4/resources/software/zmap/"
  print

  exit 1
}




###########################    main     ###################################

# defaults
remove=""
overnight='OVERNIGHT'
development='DEVELOPMENT'
production='PRODUCTION'

ftp_root=/nfs/disk69/ftp/pub4/resources/software/zmap
build_id=$development


#
# Do the flags... (see usage string for description of switches/arguments)
#
while getopts :rt: arguments
do
  case $arguments in
    'r') remove="yes" ;;
    't') case $OPTARG in
           $overnight | $development | $production) build_id=$OPTARG ;;
           *) print_usage "-$arguments must be followed by: $overnight | $development | $production"
        esac ;;

    # garbage input
    '?') print_usage "-$OPTARG is not a valid switch" ;;
    ':') print_usage "You forgot the argument for switch -$OPTARG" ;;
  esac
done


#
# get the directory where the files we want to copy live
#
# get directory where symlink points at
cd ~/BUILD.$build_id
check_status "Cannot find build directory from ~zmap/BUILD.$build_id"
dist_root=`pwd`

dist_dir=$dist_root'/Dist'
repo_dir=$dist_root'/ZMap'

echo dist root is $dist_root
echo dist dir is $dist_dir
echo repo dir is $repo_dir

# check that the source directory exists
cd $dist_root || bomb_out "can't cd to $dist_root"
if [[ ! -d $dist_dir || ! -r $dist_dir ]]
then
  bomb_out "directory $dist_dir is not accessible."
fi

# Get the lowercase file name for this build type
if [[ $build_id == $production ]]
then
  inc_file_name="production"
elif [[ $build_id == $development ]]
then
  inc_file_name="development"
elif [[ $build_id == $overnight ]]
then
  inc_file_name="overnight"
else
  bomb_out "Bad build type $build_id: expected $overnight|$development|$production"
fi

#
# Now copy the build and release notes into the relevant directory on the ftp site
#
ftp_dir=$ftp_root/$inc_file_name     # Trouble with automounting ~ftp
print_with_dividers "Transferring dist files to ftp site...$ftp_dir"

if [[ ! -z $remove ]]
then
  rm -rf $ftp_dir/* || bomb_out "Could not rm $ftp_dir."
fi

# copy the build distribution tarball
cp -p -r $dist_dir/* $ftp_dir || bomb_out "Could not copy $dist_dir to $ftp_dir."

# copy the release notes
# thse are in a sanger wiki ATM (removed)

#
# If it's the production build, copy the user manuals and README to the ftp site
#

#if [ $build_id == $production ]
#then
# (removed) the user manual is currently written by one of the Havana team
#fi


#
# Update the sanger main website page with the new build info
# The version info gets placed into a .inc file, which is
# included by the main web page
#



website_svn_dir=$home_dir/$website_svn_subdir/inc

if [ ! -d $website_svn_dir ]
then
  bomb_out "Cannot file website SVN directory: $website_svn_dir"
fi

inc_file_postfix=".inc"
inc_file=$website_svn_dir/$inc_file_name$inc_file_postfix

echo incfile is $inc_file

cd $repo_dir
check_status "Cannot find dist repository in build directory "$repo_dir

# Get the version number using git_version script.
verprog=$scripts_dir/git_version.sh

if [[ ! -f $verprog || ! -x $verprog ]]
then
  bomb_out "Can't find out ZMap build version; script $verprog does not exist or is not executable"
fi

version_string=`$verprog`
check_status "Failed to find git version"

compile_date=`date`

cd $dist_dir

echo "$msg_prefix Writing website include file: $inc_file"
# Gemma had a script called from here but we have 4 files to tag, so it's eaiser to do it inline
# $scripts_dir/zmap_update_website_version_file.sh $build_id $inc_file

ftp_link="ftp://ftp.sanger.ac.uk/pub4/resources/software/$repo_name/$inc_file_name"

echo "<p>The latest version is $version_string, created on $compile_date:" > $inc_file
for i in `ls`
do
   echo "<br /><a href=\""$ftp_link/$i"\">" $i "</a>" >> $inc_file
done
echo "</p>" >> $inc_file


check_status "Error updating $inc_file"

# Commit the changes to svn
cd $website_svn_dir
svn_status=`svn up`
echo "$msg_prefix SVN status: $svn_status"

svn_status=`svn st`

result=`svn ci -m "Updated link to $inc_file_name build." $inc_file`
check_status "Error committing changes for $inc_file; website will not be updated"

# Publish the changes
echo "$msg_prefix Pushing website changes to staging area"
result=`/software/www/bin/stage -m "Updated link to $inc_file_name build." $inc_file`
check_status "Error pushing website changes to staging area"

echo "$msg_prefix Pushing website changes to live site"
result=`/software/www/bin/publish -m "Updated link to $inc_file_name build." $inc_file`
check_status "Error pushing website changes to live website"

# (intweb documents removed)


print_with_dividers "Update of website completed successfully..."

exit 0

#########################################################

