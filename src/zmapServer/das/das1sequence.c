/*  File: das1sequence.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Sep  4 16:50 2005 (rds)
 * Created: Fri Sep  2 12:31:14 2005 (rds)
 * CVS info:   $Id: das1sequence.c,v 1.1 2005-09-05 17:27:51 rds Exp $
 *-------------------------------------------------------------------
 */

#include <das1schema.h>
#include <ZMap/zmapFeature.h>


typedef struct _dasOneSequenceStruct
{
  GQuark id;
  ZMapSequenceStruct sequence ;
  /* sequence has members 
     ZMapSequenceType type (moltype), 
     int length, 
     char *sequence */
  GQuark version;
  int start;
  int stop;
} dasOneSequenceStruct;

dasOneSequence dasOneSequence_create(char *id, int start, int stop, char *version)
{
  return dasOneSequence_create1(g_quark_from_string(id), 
                                start, stop, 
                                g_quark_from_string(version)
                                );
}
dasOneSequence dasOneSequence_create1(GQuark id, int start, int stop, GQuark version)
{
  dasOneSequence seq = NULL;
  seq = g_new0(dasOneSequenceStruct, 1);
  seq->id      = id;
  seq->version = version;
  seq->start   = start;
  seq->end     = end;

  seq->sequence         = g_new0(ZMapSequenceStruct, 1);
  seq->sequence->length = seq->end - seq->start + 1;
  seq->sequence->type   = ZMAPSEQUENCE_NONE;
  return seq;
}
void dasOneSequence_setMolType(dasOneSequence seq, char *molecule)
{
  if(strcasecmp(molecule, "dna") == 0)
    seq->sequence->type = ZMAPSEQUENCE_DNA;
  else
    seq->sequence->type = ZMAPSEQUENCE_PEPTIDE;
  return ;
}
void dasOneSequence_setSequence(dasOneSequence seq, char *sequence)
{
  /* Need a _FAST_ way to copy the string locally! 
   * And change all these sequences
   * seq->sequence->sequence = sequence;
   */
  return ;
}

