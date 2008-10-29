/*  File: zmapConfigStanzaStructs.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct 23 16:29 2008 (edgrif)
 * Created: Tue Aug 26 12:38:28 2008 (rds)
 * CVS info:   $Id: zmapConfigStanzaStructs.h,v 1.2 2008-10-29 16:06:38 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPCONFIGSTANZASTRUCTS_H
#define ZMAPCONFIGSTANZASTRUCTS_H

typedef struct _ZMapConfigSourceStruct
{
  char *url ;
  char *version ;
  char *featuresets, *navigatorsets ;
  char *styles_list, *stylesfile ;
  char *format ;
  int timeout ;
  gboolean writeback, sequence ;
} ZMapConfigSourceStruct, *ZMapConfigSource;


typedef struct _ZMapConfigStyleStruct
{
  struct
  {
    unsigned int name : 1 ;
    unsigned int description : 1 ;

    unsigned int mode : 1 ;

    unsigned int width : 1 ;

    unsigned int overlap_mode : 1 ;

    unsigned int border : 1 ;
    unsigned int fill : 1 ;
    unsigned int draw : 1 ;

    unsigned int strand_specific : 1 ;
    unsigned int show_reverse_strand : 1 ;
    unsigned int frame_specific : 1 ;

  } fields_set ;


  char *name ;
  char *description ;
  char *mode ;
  char *border, *fill, *draw ;
  double width ;
  char *overlap_mode ;

  gboolean strand_specific, show_reverse_strand, frame_specific ;
  double min_mag, max_mag ;
  gboolean gapped_align, read_gaps ;
  gboolean init_hidden ;
} ZMapConfigStyleStruct, *ZMapConfigStyle;











#endif /* ZMAPCONFIGSTANZASTRUCTS_H */
