/*  File: zmapPeptide.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Functions to translate dna to peptide.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PEPTIDE_H
#define ZMAP_PEPTIDE_H

#include <glib.h>
#include <ZMap/zmapFeature.hpp>


/* A peptide object, contains the sequence, peptide name, length etc. */
typedef struct _ZMapPeptideStruct *ZMapPeptide ;


/* A genetic code object, contains the sequence, peptide name, length etc. */
typedef struct _ZMapGeneticCodeStruct *ZMapGeneticCode ;



char *zMapPeptideCreateRaw(char *dna, ZMapGeneticCode translation_table, gboolean include_stop) ;
char *zMapPeptideCreateRawSegment(char *dna,  int from, int length, ZMapStrand strand,
				  ZMapGeneticCode translation_table, gboolean include_stop) ;
gboolean zMapPeptideCanonical(char *peptide) ;
gboolean zMapPeptideValidate(char *peptide) ;
ZMapPeptide zMapPeptideCreate(const char *sequence_name, const char *gene_name,
			      const char *dna, ZMapGeneticCode genetic_code, gboolean include_stop) ;
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
			  char *match_template_in, ZMapStrand strand, ZMapGeneticCode translation_table,
			  char **start_out, char **end_out, char **match_str) ;
GList *zMapPeptideMatchFindAll(char *target, char *query, gboolean rev_comped,
			       ZMapStrand strand, ZMapFrame orig_frame,
			       int from, int length,
			       int max_errors, int max_Ns, gboolean return_matches) ;
void zMapPeptideDestroy(ZMapPeptide peptide) ;


#endif /* !ZMAP_PEPTIDE_H */

