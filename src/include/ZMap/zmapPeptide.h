/*  File: zmapPeptide.h
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to translate dna to peptide.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PEPTIDE_H
#define ZMAP_PEPTIDE_H

#include <glib.h>
#include <ZMap/zmapFeature.h>


/* A peptide object, contains the sequence, peptide name, length etc. */
typedef struct _ZMapPeptideStruct *ZMapPeptide ;


/* A genetic code object, contains the sequence, peptide name, length etc. */
typedef struct _ZMapGeneticCodeStruct *ZMapGeneticCode ;



char *zMapPeptideCreateRaw(char *dna, ZMapGeneticCode translation_table, gboolean include_stop) ;
char *zMapPeptideCreateRawSegment(char *dna,  int from, int length, ZMapStrand strand,
				  ZMapGeneticCode translation_table, gboolean include_stop) ;
gboolean zMapPeptideCanonical(char *peptide) ;
gboolean zMapPeptideValidate(char *peptide) ;
ZMapPeptide zMapPeptideCreate(char *sequence_name, char *gene_name,
			      char *dna, ZMapGeneticCode genetic_code, gboolean include_stop) ;
ZMapPeptide zMapPeptideCreateEmpty(char *sequence_name, char *gene_name, char *dna, gboolean include_stop) ;
ZMapPeptide zMapPeptideCreateSafely(char *sequence_name, char *gene_name,
                                    char *dna, ZMapGeneticCode genetic_code, gboolean include_stop) ;
int zMapPeptideLength(ZMapPeptide peptide) ;
int zMapPeptideFullCodonAALength(ZMapPeptide peptide);
int zMapPeptideFullSourceCodonLength(ZMapPeptide peptide);
gboolean zMapPeptideHasStopCodon(ZMapPeptide peptide) ;
char *zMapPeptideSequence(ZMapPeptide peptide) ;
char *zMapPeptideSequenceName(ZMapPeptide peptide) ;
char *zMapPeptideGeneName(ZMapPeptide peptide) ;
gboolean zMapPeptideMatch(char *cp, char *end,
			  char *template, ZMapStrand strand, ZMapGeneticCode translation_table,
			  char **start_out, char **end_out, char **match_str) ;
GList *zMapPeptideMatchFindAll(char *target, char *query, gboolean rev_comped,
			       ZMapStrand strand, ZMapFrame orig_frame,
			       int from, int length,
			       int max_errors, int max_Ns, gboolean return_matches) ;
void zMapPeptideDestroy(ZMapPeptide peptide) ;


#endif /* !ZMAP_PEPTIDE_H */

