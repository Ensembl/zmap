#!/usr/bin/perl -w
#
#
#  File: scripts/zmap_SO_header.pl
#  Author: Steve Miller (sm23\@sanger.ac.uk)
#  Copyright (c) 2006-2013: Genome Research Ltd.
#-------------------------------------------------------------------
# ZMap is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
#-------------------------------------------------------------------
# This file is part of the ZMap genome database package
# originated by
#     	Ed Griffiths (Sanger Institute, UK) edgrif\@sanger.ac.uk,
#        Roy Storey (Sanger Institute, UK) rds\@sanger.ac.uk,
#   Malcolm Hinsley (Sanger Institute, UK) mh17\@sanger.ac.uk
#      Steve Miller (Sanger Institute, UK) sm23\@sanger.ac.uk
#
# Description: Script to read in the SO files (OBO format) and
# automatically generate the appropriate C source header from them.
#
# Usage: zmap_SO_head.pl [-download]
#
# Where the "download" option forces the download of the OBO files from
# the appropriate URLs using wget. These addresses are specified below.
#
# The form of the arrays that are going to be written to the header
# file are as follows:
#
#    #define ZMAP_SO_DATA_TABLE01_NUM_ITEMS 2
#    static const ZMapSOIDData ZMAP_SO_DATA_TABLE01[ZMAP_SO_DATA_TABLE01_NUM_ITEMS] =
#    {
#      {1, "name01"},
#      {2, "name02"}
#    } ;
#
# Where we will have up to three of these, defined by the strings
#
# ZMAP_SO_DATA_TABLE01           SOFA
# ZMAP_SO_DATA_TABLE02           so-xp
# ZMAP_SO_DATA_TABLE03           so-xp-simple
#
# There is also a fourth dataset called "so-hack.obo" that contains a few terms 
# which were used in ZMap/otterlace GFFv2, but I am retaining for backwards 
# compatibility during the development and testing process. There is an 
# associated "so_to_mode_map_hack.txt" file that contains the ZMapStyleMode 
# for these. 
#

use Getopt::Long;
my $download;
GetOptions ("download"  => \$download);

#
# Use extra hacked backwards compatibility data. 
# 
$use_hack = 0 ; 

#
# Destination file for auto-generated header.
#
$destination_file = "zmapGFF/zmapSOData_P.h" ;

#
# Define the filenames to parse.
#
$sofa_file            = "../scripts/SOFA.obo" ;
$so_xp_file           = "../scripts/so-xp.obo";
$so_xp_simple_file    = "../scripts/so-xp-simple.obo" ;
$so_hack_file         = "../scripts/so-hack.obo" ; 

#
# Define the filenames for the SO-to-ZMapStyleMode files. 
# 
$so_to_mode_map_file           = "../scripts/so_to_mode_map.txt";
$so_to_mode_map_hack_file      = "../scripts/so_to_mode_map_hack.txt";

#
# Define URLs at which to find files.
#
$sofa_url            = 'http://sourceforge.net/p/song/svn/HEAD/tree/trunk/subsets/SOFA.obo?format=raw' ;
$so_xp_url           = 'http://sourceforge.net/p/song/svn/HEAD/tree/trunk/so-xp.obo?format=raw' ;
$so_xp_simple_url    = 'http://sourceforge.net/p/song/svn/HEAD/tree/trunk/so-xp-simple.obo?format=raw' ;

#
# Define data table names
#
$header_tablename01 = "ZMAP_SO_DATA_TABLE01" ;
$header_tablename02 = "ZMAP_SO_DATA_TABLE02" ;
$header_tablename03 = "ZMAP_SO_DATA_TABLE03" ;
$header_tablename04 = "ZMAP_SO_DATA_TABLE04_HACK" ;

#
# Filename that contains SO term to ZMapMode mapping.
# The modes used are taken from a subset of the ones that
# make up the enum ZMapStyleMode in ZMap/zmapStyle.h,
# which are as follows:
#
# _(ZMAPSTYLE_MODE_INVALID,       , "invalid"      , "invalid mode "                                 , "") \
# _(ZMAPSTYLE_MODE_BASIC,         , "basic"        , "Basic box features "                           , "") \
# _(ZMAPSTYLE_MODE_ALIGNMENT,     , "alignment"    , "Usual homology structure "                     , "") \
# _(ZMAPSTYLE_MODE_TRANSCRIPT,    , "transcript"   , "Usual transcript like structure "              , "") \
# _(ZMAPSTYLE_MODE_SEQUENCE,      , "sequence"     , "DNA or Peptide Sequence "                      , "") \
# _(ZMAPSTYLE_MODE_ASSEMBLY_PATH, , "assembly-path", "Assembly path "                                , "") \
# _(ZMAPSTYLE_MODE_TEXT,          , "text"         , "Text only display "                            , "") \
# _(ZMAPSTYLE_MODE_GRAPH,         , "graph"        , "Graphs of various types "                      , "") \
# _(ZMAPSTYLE_MODE_GLYPH,         , "glyph"        , "Special graphics for particular feature types ", "") \
# _(ZMAPSTYLE_MODE_PLAIN,         , "plain"        , "generic non-feature graphics"                  , "") \
# _(ZMAPSTYLE_MODE_META,          , "meta"         , "Meta object controlling display of features "  , "")
#
# I use the subset:
#
# ZMAPSTYLE_MODE_BASIC
# ZMAPSTYLE_MODE_ALIGNMENT
# ZMAPSTYLE_MODE_TRANSCRIPT
# ZMAPSTYLE_MODE_SEQUENCE
# ZMAPSTYLE_MODE_ASSEMBLY_PATH
# ZMAPSTYLE_MODE_GLYPH
#
# Where an SO term is not found in the so_to_mode_map.txt file, it will be given the
# mode BASIC. The SO terms in the file are the ones taken from SOFA.obo October 2013.
# This might need to be updated at some point.
#

#
# This is the list of modes that we consider to be available for use
# in this context.
#
$available_modes{"ZMAPSTYLE_MODE_BASIC"}             = 1 ;
$available_modes{"ZMAPSTYLE_MODE_ALIGNMENT"}         = 1 ;
$available_modes{"ZMAPSTYLE_MODE_TRANSCRIPT"}        = 1 ;
$available_modes{"ZMAPSTYLE_MODE_SEQUENCE"}          = 1 ;
$available_modes{"ZMAPSTYLE_MODE_ASSEMBLY_PATH"}     = 1 ;
$available_modes{"ZMAPSTYLE_MODE_GLYPH"}             = 1 ;


#
# This is the list of available homol types; we must also associate one
# of these with each SO term that is of alignment mode.
#
# The C enum is defined as
# {ZMAPHOMOL_NONE = 0, ZMAPHOMOL_N_HOMOL, ZMAPHOMOL_X_HOMOL, ZMAPHOMOL_TX_HOMOL}
#
$available_homol{"ZMAPHOMOL_NONE"}                    = 1 ;
$available_homol{"ZMAPHOMOL_N_HOMOL"}                 = 1 ;
$available_homol{"ZMAPHOMOL_X_HOMOL"}                 = 1 ;
$available_homol{"ZMAPHOMOL_TX_HOMOL"}                = 1 ;


######################################################################################
######################################################################################
######################################################################################
#
# At some point a check must be put in such that if a mode IS an alignment then 
# the homol must NOT be ZMAPHOMOL_NONE, and if the mode IS NOT an alignment then 
# the homol MUST be ZMAPHOMOL_NONE. For the moment, however, this is ignored.
#
######################################################################################
######################################################################################
######################################################################################



#
# Read in the SO to mode map file.
#
$so_to_mode_map{"DUMMY_FAKE_NAME"} = "DUMMY_FAKE_MODE" ;   # to get rid of warnings  
$so_to_homol_map{"DUMMY_FAKE_NAME"} = "DUMMY_FAKE_HOMOL" ; # ditto 
open(SO_TO_MODE_MAP, "<$so_to_mode_map_file") or die "could not open file '$so_to_mode_map_file'; appears not to be present in the distribution.\n";
while (<SO_TO_MODE_MAP>)
{
  ($so_name, $zmap_mode, $zmap_homol) = (m/(\w+)\t(\w+)\t(\w+)\n/) ;
  if (defined $so_name)
    {
      if (defined $zmap_mode and defined $zmap_homol)
        {
          #
          # If we have an available mode type then
          #
          #
          if (defined $available_modes{$zmap_mode})
            {
             $so_to_mode_map{$so_name} = $zmap_mode ;
            }
          #
          # If we have an available homol type then add this 
          #
          if (defined $available_homol{$zmap_homol}) 
            {
              $so_to_homol_map{$so_name} = $zmap_homol ; 
            }
        }  
      else 
        {
          die "mode and homol must both be defined for name = $so_name\n" ; 
        }

    }
}
close (SO_TO_MODE_MAP) ;

if (defined $use_hack) 
{ 
  open(SO_TO_MODE_MAP, "<$so_to_mode_map_hack_file") or die "could not open file '$so_to_mode_map_hack_file'; appears not to be present in the distribution.\n";
  while (<SO_TO_MODE_MAP>)
  {
    ($so_name, $zmap_mode, $zmap_homol) = (m/(\w+)\t(\w+)\t(\w+)\n/) ;
    if (defined $so_name)
      {
        if (defined $zmap_mode and defined $zmap_homol)
          {
            #
            # If we have an available mode type then
            #
            #
            if (defined $available_modes{$zmap_mode})
              {
               $so_to_mode_map{$so_name} = $zmap_mode ;
              }
            #
            # If we have an available homol type then add this 
            #
            if (defined $available_homol{$zmap_homol})
              {
                $so_to_homol_map{$so_name} = $zmap_homol ;
              }
          }
        else
          {
            die "mode and homol must both be defined for name = $so_name\n" ;
          }
  
      }
  }
  close (SO_TO_MODE_MAP) ;

}



#
# Download files if command line option specifies such, or parse for
# header information.
#
@Names = () ;
@SOIDS = () ;
$header_part01 = "" ;
$header_part02 = "" ;
$header_part03 = "" ;
if (defined $use_hack)
{
  $header_part04 = ""; 
}
if ($download)
{
  system("wget $sofa_url           -O        $sofa_file");
  system("wget $so_xp_url          -O        $so_xp_file");
  system("wget $so_xp_simple_url   -O        $so_xp_simple_file");
}
else
{

  @Names = () ;
  @SOIDS = () ;
  &Read_SO_File($sofa_file) ;
  $header_part01 = &Convert_Data($header_tablename01) ;

  @Names = () ;
  @SOIDS = () ;
  $count = &Read_SO_File($so_xp_file) ;
  $header_part02 = &Convert_Data($header_tablename02) ;

  @Names = () ;
  @SOIDS = () ;
  $count = &Read_SO_File($so_xp_simple_file) ;
  $header_part03 = &Convert_Data($header_tablename03) ;

  if (defined $use_hack) 
  { 
    @Names = () ; 
    @SOIDS = () ; 
    $count = &Read_SO_File($so_hack_file) ; 
    $header_part04 = &Convert_Data($header_tablename04) ; 
  } 
}

#
# Now put the three bits together.
#
$header_data =
"/*  File: zmapSOData_P.h
 *  Author: Steve Miller (sm23\@sanger.ac.uk)
 *  Copyright (c) 2006-2013: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originated by
 *     	Ed Griffiths (Sanger Institute, UK) edgrif\@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds\@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17\@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23\@sanger.ac.uk
 *
 * Description: Private header for SO term data. This file is
 * auto-generated, DO NOT ALTER.
 *
 *-------------------------------------------------------------------
 */
" ;

$header_data .= $header_part01 ;
$header_data .= $header_part02 ;
$header_data .= $header_part03 ;
if (defined $use_hack) 
{
  $header_data .= "#define USE_SO_TERM_HACK 1\n" ; 
  $header_data .= $header_part04 ; 
}

#
# Now send to final destination file.
#
open(DESTINATION, ">$destination_file") or die "could not open file '$destination_file' for output.\n";
print DESTINATION $header_data ;
close(DESTINATION) ;






#
# Function to turn a pair of arrays of data into the appropriate
# static const array for a C header.
#
sub Convert_Data
{
  my $narguments = 1 ;
  my $currentFunction = "Convert_Data()" ;

  #
  # check numbers of arguments
  #
  my $n = scalar(@_);
  if ($n != $narguments)
  {
    printf("Error: $currentFunction must be called with $narguments arguments.\n");
    return 1;
  }
  my $arg = $_[0] ;

  #
  # Check sizes of arrays are the same.
  #
  my $result = "" ;
  my $sizeNames = @Names ;
  my $sizeIDS = @SOIDS ;
  if ($sizeNames != $sizeIDS )
  {
    die "Names and SOIDS arrays must have the same size. \n" ;
  }
  if ($sizeNames  == 0)
  {
    return $result ;
  }

  #
  # argument is name of array to use
  #
  $result = "#define $arg";
  $result .= "_NUM_ITEMS $sizeNames\n" ;
  $result .= "static const ZMapSOIDDataStruct $arg" ;
  $result .= "["  ;
  $result .= $arg ;
  $result .= "_NUM_ITEMS] = \n" ;
  $result .= "\{\n" ;
  my $i = 0 ;
  my $id = 0 ;
  my $nam = "" ;
  my $mode = "" ;
  my $homol = "" ; 

  $i = 0 ;
  $id = $SOIDS[$i] ;
  $nam = $Names[$i] ;
  $mode  = $so_to_mode_map{$nam} ;
  $homol = $so_to_homol_map{$nam} ; 
  if (defined $mode)
  {
    $result .= "  \{ $id, \"$nam\", $mode, $homol \}" ;
  }
  for ($i=1; $i < $sizeNames ; $i++)
  {
    $id = $SOIDS[$i] ;
    $nam = $Names[$i] ;
    $mode = $so_to_mode_map{$nam} ;
    $homol = $so_to_homol_map{$nam} ; 
    if (!defined $mode)
    {
      $mode = "ZMAPSTYLE_MODE_BASIC" ;
    }
    if (!defined $homol) 
    {
      $homol = "ZMAPHOMOL_NONE" ; 
    }
    $result .= ",\n  \{ $id, \"$nam\", $mode, $homol \}" ;

  }
  $result .= "\n\} ;\n\n" ;

  return $result ;
}



#
# Function to read OBO file format, parse out basic stuff and return
# the data found.
#
sub Read_SO_File
{
  my $count = 0 ;
  my $narguments = 1 ;
  my $currentFunction = "Read_SO_File" ;

  #
  # check numbers of arguments
  #
  my $n = scalar(@_);
  if ($n != $narguments)
  {
    printf("Error: $currentFunction must be called with $narguments arguments.\n");
    return 1;
  }

  #
  # open file
  #
  my $filename = $_[0] ;
  open(DATA, "<$filename") or die "could not open file '$filename'; appears not to be present in the distribution.\n";
  my $data ;
  while (<DATA>)
  {
    $data .= $_;
  }

  #
  # parse for internal data
  #
  while (<DATA>)
  {
    if (/^\[Term\]/ .. /^$/)
    {
      print $_ if (m/name:[\s]\w+/) ;
      print $_ if (/id:[\s]\w+/) ;
    }
  }
  close DATA;

  #
  # Split the input data based on the SO record seperator.
  #
  my @records = split(/\[Term\]/, $data) ;
  my $id = "" ;
  my $name = "" ;
  foreach $record (@records)
  {
    $id = "" ;
    $name = "" ;

    #
    # only operate if the record is non-blank
    #
    if (length($record))
    {
      #
      # extract the 'id:' and 'name:' fields.
      #
      ($id, $name) = ($record =~ m/id:[\s]*SO:[0]*(\d+)\nname:[\s]*(\S+)[\s\w]*/) ;
      if (defined $id)
      {
        if (defined $name)
        {
          if (length($name) && length($id))
          {
            #
            # now look for is_obsolete flag ...
            #
            if (!($record =~ m/is_obsolete: true/))
            {
              push(@Names, $name );
              push(@SOIDS, $id);
              $count++ ;
            }
          }
        }
      }
    }
  } # foreach $record (@records)

  return $count ;

}



exit 0 ;



