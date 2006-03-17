/*  File: zmapFASTA.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * Last edited: Mar 17 16:25 2006 (edgrif)
 * Created: Fri Mar 17 16:24:30 2006 (edgrif)
 * CVS info:   $Id: zmapFASTA.c,v 1.1 2006-03-17 16:25:52 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsDNA.h>


/* THIS FILE WOULD BE BETTER NAMED zmapFASTAUtils.c....perhaps I will change it... */




/* FASTA files can lines of any length but 50 is convenient for terminal display
 * and counting... */
enum {FASTA_CHARS = 50} ;


/* This code is lifted from the code that I wrote for acedb - EG
 * Given a sequence name and its dna sequence, will dump the data in FastA
 * format.
 * Routine could be extended to find the sequence length but tedious/trivial really. */
gboolean zMapFASTAFile(GIOChannel *file, char *seq_name, int seq_len, char *dna, GError **error_out)
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

  header = g_strdup_printf(">%s\n", seq_name) ;

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





/* Given a sequence string, returns the corresponding FASTA format string.
 * Good for displaying in windows.
 * Routine could be extended to find the sequence length but tedious/trivial really. */
char *zMapFASTAString(char *seq_name, char *molecule_type, char *gene_name,
		      int sequence_length, char *sequence)
{
  char *fasta_string = NULL ;
  GString *str ;
  char buffer[FASTA_CHARS + 2] ;			    /* FastA chars + \n + \0 */
  int header_length ;
  int lines = 0, chars_left = 0 ;
  char *cp = NULL ;
  int i ;

  zMapAssert(seq_name && sequence_length > 0 && sequence) ;

  header_length = 1000 ;				    /* wild guess... */
  
  lines = sequence_length / FASTA_CHARS ;
  chars_left = sequence_length % FASTA_CHARS ;

  /* Make the string big enough to hold the whole of the FASTA formatted string, the + 10 is
   * a fundge factor to avoid reallocation. */
  str = g_string_sized_new(header_length + sequence_length + (lines + 1) + 10) ;

  /* We should add more info. really.... */
  g_string_append_printf(str, ">%s %s %s %d bp\n",
			 seq_name,
			 molecule_type ? molecule_type : "",
			 gene_name ? gene_name : "",
			 sequence_length) ;

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


