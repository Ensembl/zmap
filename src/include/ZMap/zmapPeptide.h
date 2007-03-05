/*  File: zmapPeptide.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Description: Functions to translate dna to peptide.
 *
 * HISTORY:
 * Last edited: Mar  5 14:17 2007 (edgrif)
 * Created: Tue Mar 14 13:55:27 2006 (edgrif)
 * CVS info:   $Id: zmapPeptide.h,v 1.4 2007-03-05 14:37:35 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PEPTIDE_H
#define ZMAP_PEPTIDE_H


#include <glib.h>

typedef struct _ZMapPeptideStruct *ZMapPeptide ;



ZMapPeptide zMapPeptideCreate(char *sequence_name, char *gene_name,
			      char *dna, GArray *translation_table, gboolean include_stop) ;
ZMapPeptide zMapPeptideCreateSafely(char *sequence_name, char *gene_name,
                                    char *dna, GArray *translation_table, 
                                    gboolean include_stop) ;
int zMapPeptideLength(ZMapPeptide peptide) ;
gboolean zMapPeptideHasStopCodon(ZMapPeptide peptide) ;
char *zMapPeptideSequence(ZMapPeptide peptide) ;
char *zMapPeptideSequenceName(ZMapPeptide peptide) ;
char *zMapPeptideGeneName(ZMapPeptide peptide) ;
void zMapPeptideDestroy(ZMapPeptide peptide) ;


#endif /* !ZMAP_PEPTIDE_H */

