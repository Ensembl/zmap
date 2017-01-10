#!/usr/bin/perl -w
#
#
#  File: zmap_DefaultStyles.pl
#  Author: Steve Miller (sm23\@sanger.ac.uk)
#  Copyright (c) 2006-2017: Genome Research Ltd.
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
#      Ed Griffiths (Sanger Institute, UK) edgrif\@sanger.ac.uk,
#        Roy Storey (Sanger Institute, UK) rds\@sanger.ac.uk,
#   Malcolm Hinsley (Sanger Institute, UK) mh17\@sanger.ac.uk
#      Steve Miller (Sanger Institute, UK) sm23\@sanger.ac.uk
#
# Description: Script to read file default_styles.txt and convert this
# to default_styles.c that can then be included in the appropriate part 
# of the config source code. It is run as part of the bootstrap process. 
#
# Usage: zmap_DefaultStyles.pl
#


#
# Input file taken from command line argument. 
#
$input_file = $ARGV[0];

$converted_data =
"/*  File: default_styles.c
 *  Author: Steve Miller (sm23\@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif\@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds\@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17\@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23\@sanger.ac.uk
 *
 * Description: File to insert default styles into zmaConfig code. 
 *              This file is auto-generated, DO NOT ALTER.
 *
 *-------------------------------------------------------------------
 */
const char *default_styles = \"\\\n" ; 

#
# open file and read into a string 
#
open(DATA, "<$input_file") or die "could not open file '$input_file'; appears not to be present.\n";
my $data ;
while (<DATA>)
{
  # remove newline from input 
  # replace with \n\ then a newline 
  chomp($_) ; 
  $data .= $_ ;
  $data .= "\\n\\\n" ; 
}
close(DATA) ; 

#
# Convert format 
#
$converted_data .= $data ; 
$converted_data .= "\";\n" ; 


#
# Now send to final destination file.
#
print $converted_data ;



