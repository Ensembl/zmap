/*  File: zmapSequence.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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


/* Map dna coords to corresponding peptide coords, the returned peptide
 * coord is for the peptide that contains the dna base, this is a one for
 * one translation of coords, bounds checking is only done for coords
 * at the start of the dna sequence.
 * 
 * Notes:
 *  - zero can be returned for dna bases 1 and 2 for frames > 1.
 *  - the end peptide coord is _not_ clipped to lie completely within
 *    the end coord of the dna.
 * 
 */
void zMapSequenceDNA2Pep(int *start_inout, int *end_inout, ZMapFrame frame)
{
  int dna_start, dna_end ;
  int start_coord, end_coord ;
  int frame_int = (int)frame ;

  zMapAssert((start_inout && end_inout && *start_inout > 0 && end_inout > 0
	      && (*start_inout <= *end_inout))
	     && frame >= ZMAPFRAME_NONE && frame <= ZMAPFRAME_2) ;

  dna_start = *start_inout ;
  dna_end = *end_inout ;

  start_coord = dna_start - frame_int ;

  if (start_coord < 0)
    {
      start_coord = 0 ;
    }
  else
    {
      start_coord = start_coord / 3 ;

      start_coord += 1 ;
    }

  end_coord = dna_end - frame_int ;

  if (end_coord < 0)
    {
      end_coord = 0 ;
    }
  else
    {
      end_coord = end_coord / 3 ;

      end_coord += 1 ;
    }

  *start_inout = start_coord ;
  *end_inout = end_coord ;

  return ;
}
