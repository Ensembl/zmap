/*  File: zmapFASTA.c
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to handle FASTA format.
 *
 * Exported functions: See ZMap/zmapFASTA.h
 * HISTORY:
 * Last edited: Mar  5 14:33 2007 (edgrif)
 * Created: Fri Mar 17 16:24:30 2006 (edgrif)
 * CVS info:   $Id: zmapFASTA.c,v 1.6 2010-03-04 15:11:05 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFASTA.h>




/* FASTA files can lines of any length but 50 is convenient for terminal display
 * and counting... */
enum {FASTA_CHARS = 50} ;


/* NOTE that in all these calls "seq_len" is the length reported in the FASTA header, the code
 * writes out all of the supplied sequence. */


/* This code is modified from acedb code (www.acedb.org)
 * 
 * Given a sequence name and its dna sequence, will dump the data in FastA
 * format. */
gboolean zMapFASTAFile(GIOChannel *file, ZMapFASTASeqType seq_type, char *seq_name, int seq_len, char *dna,
		       char *molecule_type, char *gene_name, GError **error_out)
{
  gboolean result = TRUE ;
  GIOStatus status ;
  gsize bytes_written = 0 ;
  char *header ;
  char buffer[FASTA_CHARS + 2] ;			    /* FastA chars + \n + \0 */
  int dna_length  = 0 ;
  int lines = 0, chars_left = 0 ;
  char *cp = NULL ;
  int i ;

  zMapAssert(file && seq_name && seq_len > 0 && dna && error_out) ;

  header = zMapFASTATitle(seq_type, seq_name, molecule_type, gene_name, seq_len) ;

  if ((status = g_io_channel_write_chars(file, header, -1, &bytes_written, error_out))
      != G_IO_STATUS_NORMAL)
    {
      result = FALSE ;
    }
  else
    {
      dna_length = strlen(dna) ;
      lines = dna_length / FASTA_CHARS ;
      chars_left = dna_length % FASTA_CHARS ;
      cp = dna ;

      /* Do the full length lines.                                           */
      if (lines != 0)
	{
	  buffer[FASTA_CHARS] = '\n' ;
	  buffer[FASTA_CHARS + 1] = '\0' ; 
	  for (i = 0 ; i < lines && result ; i++)
	    {
	      memcpy(&buffer[0], cp, FASTA_CHARS) ;
	      cp += FASTA_CHARS ;

	      if ((status = g_io_channel_write_chars(file, &buffer[0], -1, &bytes_written, error_out))
		  != G_IO_STATUS_NORMAL)
		result = FALSE ;
	    }
	}

      /* Do the last line.                                                   */
      if (chars_left != 0)
	{
	  memcpy(&buffer[0], cp, chars_left) ;
	  buffer[chars_left] = '\n' ;
	  buffer[chars_left + 1] = '\0' ; 
	  if ((status = g_io_channel_write_chars(file, &buffer[0], -1, &bytes_written, error_out))
	      != G_IO_STATUS_NORMAL)
	    result = FALSE ;
	}
    }

  g_free(header) ;

  return result ;
}


/* Returns the "title" of a FASTA file, i.e. the text that forms the first line
 * of a FASTA file (note that the string _does_ end with a newline).
 * Result should be freed using g_free() by caller. */
char *zMapFASTATitle(ZMapFASTASeqType seq_type, char *seq_name, char *molecule_type, char *gene_name,
		     int sequence_length)
{
  char *title = NULL ;

  zMapAssert(seq_name && *seq_name && sequence_length > 0) ;

  /* We should add more info. really.... */
  title = g_strdup_printf(">%s %s %s %d %s\n",
			  seq_name,
			  molecule_type ? molecule_type : "",
			  gene_name ? gene_name : "",
			  sequence_length,
			  seq_type == ZMAPFASTA_SEQTYPE_DNA ? "bp" : "AA"
			  ) ;

  return title ;
}



/* Given a sequence string, returns the corresponding FASTA format string.
 * Good for displaying in windows but costly in memory as duplicates the
 * supplied sequence and adds to it....so not good for dumping a huge dna
 * sequence to file, use zMapFASTAFile() instead.
 */
char *zMapFASTAString(ZMapFASTASeqType seq_type, char *seq_name, char *molecule_type, char *gene_name,
		      int sequence_length, char *sequence)
{
  char *fasta_string = NULL ;
  char *title ;
  GString *str ;
  char buffer[FASTA_CHARS + 2] ;			    /* FastA chars + \n + \0 */
  int header_length ;
  int lines = 0, chars_left = 0 ;
  char *cp = NULL ;
  int i ;
  int true_sequence_length ;

  zMapAssert(seq_name && sequence_length > 0 && sequence) ;


  /* We should add more info. really.... */
  title = zMapFASTATitle(seq_type, seq_name, molecule_type, gene_name, sequence_length) ;

  header_length = strlen(title) + 1 ;			    /* + 1 for newline. */

  true_sequence_length = strlen(sequence) ;
  lines = true_sequence_length / FASTA_CHARS ;
  chars_left = true_sequence_length % FASTA_CHARS ;

  /* Make the string big enough to hold the whole of the FASTA formatted string, the + 10 is
   * a fudge factor to avoid reallocation. */
  str = g_string_sized_new(header_length + true_sequence_length + (lines + 1) + 10) ;


  g_string_append_printf(str, "%s", title) ;
  g_free(title) ;


  /* Do the full length lines.                                           */
  cp = sequence ;
  if (lines)
    {
      buffer[FASTA_CHARS] = '\n' ;
      buffer[FASTA_CHARS + 1] = '\0' ; 
      for (i = 0 ; i < lines ; i++)
	{
	  /* shame not to do this in one go...probably is some way using formatting.... */
	  str = g_string_append_len(str, cp, FASTA_CHARS) ;
	  str = g_string_append(str, "\n") ;

	  cp += FASTA_CHARS ;
	}
    }

  /* Do the last line.                                                   */
  if (chars_left)
    {
      /* shame not to do this in one go...probably is some way using formatting.... */
      str = g_string_append_len(str, cp, chars_left) ;
      str = g_string_append(str, "\n") ;
    }

  fasta_string = g_string_free(str, FALSE) ;

  return fasta_string ;
}


