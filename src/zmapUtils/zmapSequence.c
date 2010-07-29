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
 * Last edited: Jul  7 15:02 2010 (edgrif)
 * Created: Thu Sep 27 10:48:11 2007 (edgrif)
 * CVS info:   $Id: zmapSequence.c,v 1.4 2010-07-29 09:13:10 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapSequence.h>
#include <ZMap/zmapFeature.h>

/* MAYBE ALL THIS SHOULD BE IN THE DNA OR PEPTIDE FILES ???? */


ZMapFrame zMapSequenceGetFrame(int position)
{
  ZMapFrame frame = ZMAPFRAME_NONE ;
  int curr_frame ;

  curr_frame = position % 3 ;

  curr_frame = 3 - ((3 - curr_frame) % 3) ;

  frame = curr_frame ;

  return frame ;
}



/* Map peptide coords to corresponding sequence coords, no bounds checking is done,
 * it's purely a coords based thing. */
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


/* Map dna coords to corresponding peptide coords, you may need to clip the end
 * coord to lie within the peptide. */
void zMapSequenceDNA2Pep(int *start_inout, int *end_inout, ZMapFrame frame)
{
  int dna_start, dna_end, pep_start, pep_end ;
  int dna_offset ;
  int curr_frame ;

  zMapAssert(start_inout && end_inout && frame >= ZMAPFRAME_NONE && frame <= ZMAPFRAME_2) ;

  dna_start = *start_inout ;
  dna_end = *end_inout ;

  curr_frame = dna_start % 3 ;

  curr_frame = 3 - ((3 - curr_frame) % 3) ;

  printf("Start frame: %d\n", curr_frame) ;

  dna_offset = 3 - (curr_frame - frame) ;

  dna_offset = dna_offset % 3 ;

  dna_start = (dna_start + dna_offset)  ;


  curr_frame = dna_end % 3 ;

  curr_frame = 3 - ((3 - curr_frame) % 3) ;


  /* HACK UNTIL I SORT THIS OUT....HATEFUL...I'M BEING STUPID.... */
  /* This doesn't quite work for all combinations...agh....so close.... */
  if (curr_frame - frame == -2)
    dna_offset = 2 ;
  else
    dna_offset = (curr_frame - frame) + 1 ;


  dna_offset = dna_offset % 3 ;



  dna_end = (dna_end - dna_offset)  ;

  pep_start = ((dna_start - 1) + 3) / 3 ;

  pep_end = pep_start + ((dna_end - dna_start) / 3 ) ;

  *start_inout = pep_start ;
  *end_inout = pep_end ;

  return ;
}
