/*  File: zmapSequence.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See ZMap/zmapSequence.h
 * HISTORY:
 * Last edited: Sep 27 14:07 2007 (edgrif)
 * Created: Thu Sep 27 10:48:11 2007 (edgrif)
 * CVS info:   $Id: zmapSequence.c,v 1.3 2010-06-14 15:40:14 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapUtils.h>
#include <ZMap/zmapSequence.h>


/* Map peptide coords to corresponding sequence coords. */
void zMapSequencePep2DNA(int *start_inout, int *end_inout, ZMapFrame frame)
{
  int frame_num ;

  zMapAssert(start_inout && end_inout && frame >= ZMAPFRAME_NONE && frame <= ZMAPFRAME_2) ;

  /* Convert frame enum to base offset, ZMAPFRAME_NONE is assumed to be zero. */
  frame_num = frame ;
  if (frame != ZMAPFRAME_NONE)
    frame_num = frame - 1 ;

  *start_inout = ((*start_inout * 3) - 2) + frame_num ;
  *end_inout = (*end_inout * 3) + frame_num ;


  return ;
}
