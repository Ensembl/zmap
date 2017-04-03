/*  File: zmapFASTA.h
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
 * Description: Interface to FASTA routines.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_DNA_H
#define ZMAP_UTILS_DNA_H

#include <glib.h>

typedef enum {ZMAPFASTA_SEQTYPE_INVALID, ZMAPFASTA_SEQTYPE_DNA, ZMAPFASTA_SEQTYPE_AA} ZMapFASTASeqType ;


gboolean zMapFASTAFile(GIOChannel *file,
                       ZMapFASTASeqType seq_type, const char *seqname, int seq_len,
                       const char *dna, const char *molecule_type, const char *gene_name,
                       GError **error_out) ;
char *zMapFASTAString(ZMapFASTASeqType seq_type,
                      const char *seq_name, const char *molecule_type, const char *gene_name,
		      int sequence_length, const char *sequence) ;
char *zMapFASTATitle(ZMapFASTASeqType seq_type,
                     const char *seq_name, const char *molecule_type, const char *gene_name,
		     int sequence_length) ;

#endif /* ZMAP_UTILS_DNA_H */
