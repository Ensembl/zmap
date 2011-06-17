/*  File: zmapFASTA.h
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
 * Description: Interface to FASTA routines.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_DNA_H
#define ZMAP_UTILS_DNA_H

#include <glib.h>

typedef enum {ZMAPFASTA_SEQTYPE_INVALID, ZMAPFASTA_SEQTYPE_DNA, ZMAPFASTA_SEQTYPE_AA} ZMapFASTASeqType ;

gboolean zMapFASTAFile(GIOChannel *file, ZMapFASTASeqType seq_type, char *seqname, int seq_len, char *dna,
		       char *molecule_type, char *gene_name, GError **error_out) ;
char *zMapFASTAString(ZMapFASTASeqType seq_type, char *seq_name, char *molecule_type, char *gene_name,
		      int sequence_length, char *sequence) ;
char *zMapFASTATitle(ZMapFASTASeqType seq_type, char *seq_name, char *molecule_type, char *gene_name,
		     int sequence_length) ;

#endif /* ZMAP_UTILS_DNA_H */
