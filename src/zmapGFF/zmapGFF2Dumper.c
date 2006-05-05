/*  File: zmapGFF2Dumper.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Dumps the features within a given block as GFF v2
 *
 * Exported functions: See ZMap/zmapGFF.h
 * HISTORY:
 * Last edited: May  5 11:57 2006 (rds)
 * Created: Mon Nov 14 13:21:14 2005 (edgrif)
 * CVS info:   $Id: zmapGFF2Dumper.c,v 1.3 2006-05-05 11:00:16 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>


/* My new general purpose dumper function.... */
typedef struct
{
  gboolean status ;
  GIOChannel *file ;
  GError **error_out ;
} NewDumpFeaturesStruct, *NewDumpFeatures ;


static gboolean dumpHeader(GIOChannel *file, ZMapFeatureAny any_feature, GError **error_out) ;
static gboolean dumpSeqHeaders(GIOChannel *file, ZMapFeatureAny any_feature, GError **error_out) ;
static void doAlignment(GQuark key_id, gpointer data, gpointer user_data) ;
static void doBlock(gpointer data, gpointer user_data) ;
static gboolean dumpFeature(GIOChannel *file, gpointer user_data,
			    ZMapFeatureTypeStyle style,
			    char *parent_name,
			    char *feature_name,
			    char *style_name,
			    char *ontology,
			    int x1, int x2,
			    gboolean has_score, float score,
			    ZMapStrand strand,
			    ZMapPhase phase,
			    ZMapFeatureType feature_type,
			    gpointer feature_data,
			    GError **error_out) ;
static gboolean printTranscriptSubpart(GIOChannel *file, GString *buffer,
				       GArray *subparts, gboolean coding,
				       char *parent_name, char *feature_name,
				       char *style_name, char *ontology,
				       ZMapStrand strand,
				       GError **error_out) ;
static char strand2Char(ZMapStrand strand) ;
static char phase2Char(ZMapPhase phase) ;


/* NOTE: if we want this to dump a context it will have to cope with lots of different
 * sequences for the different alignments...I think the from alignment down is ok but the title
 * for the file is not if we have different alignments....and neither are the coords....?????
 * 
 * I think we would need multiple Type and sequence-region comment lines for the different
 * sequences.
 * 
 *  */
gboolean zMapGFFDump(GIOChannel *file, ZMapFeatureAny dump_set, GError **error_out)
{
  gboolean result = TRUE ;

  zMapAssert(file && dump_set && error_out) ;
  zMapAssert(dump_set->struct_type == ZMAPFEATURE_STRUCT_CONTEXT
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_ALIGN
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_BLOCK
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  result = dumpHeader(file, dump_set, error_out) ;


  if (result)
    {
      enum {GFF_BUF_SIZE = 2048} ;
      GString *buffer ;

      buffer = g_string_sized_new(GFF_BUF_SIZE) ;

      result = zMapFeatureDumpFeatures(file, dump_set, dumpFeature, (gpointer)buffer, error_out) ;

      g_string_free(buffer, TRUE) ;
    }

  return result ;
}



static gboolean dumpHeader(GIOChannel *file, ZMapFeatureAny any_feature, GError **error_out)
{
  enum {GFF_VERSION = 2} ;
  gboolean result = TRUE ;
  char *header, *time_string ;
  GIOStatus status ;
  gsize bytes_written ;

  /* Dump the standard header lines. */
  time_string = zMapGetTimeString(ZMAPTIME_YMD, NULL) ;

  header = g_strdup_printf("##gff-version %d\n"
			   "##source-version %s %s\n"
			   "##date %s\n",
			   GFF_VERSION,
			   zMapGetAppName(), zMapGetVersionString(),
			   time_string) ;

  g_free(time_string) ;


  if ((status = g_io_channel_write_chars(file, header, -1, &bytes_written, error_out))
      != G_IO_STATUS_NORMAL)
    result = FALSE ;


  /* Dump extra header lines for each alignment/block. */
  if (result)
    result = dumpSeqHeaders(file, any_feature, error_out) ;

  return result ;
}



static gboolean dumpSeqHeaders(GIOChannel *file, ZMapFeatureAny dump_set, GError **error_out)
{
  gboolean result = TRUE ;
  NewDumpFeaturesStruct dump_data ;
  GDataForeachFunc dataset_start_func = NULL ;
  GData **dataset = NULL ;
  ZMapFeatureAlignment alignment = NULL ;


  dump_data.status = TRUE ;
  dump_data.file = file ;
  dump_data.error_out = error_out ;

  /* NOTES: I cocked up in that the blocks are a GList, everything else is a GData, that needs
   * changing then GData could become part of the structAny struct to allow more generalised
   * handling.... */
  switch(dump_set->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      dataset_start_func = doAlignment ;
      dataset = &(((ZMapFeatureContext)dump_set)->alignments) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      alignment = (ZMapFeatureAlignment)dump_set ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      alignment = (ZMapFeatureAlignment)(dump_set->parent) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      alignment = (ZMapFeatureAlignment)(dump_set->parent->parent) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURE:			    /* pathological case... */
      alignment = (ZMapFeatureAlignment)(dump_set->parent->parent->parent) ;
      break ;
    case ZMAPFEATURE_STRUCT_INVALID:
      zMapAssertNotReached();
      break;
    }

  if (dataset_start_func)
    g_datalist_foreach(dataset, dataset_start_func, &dump_data) ;
  else
    doAlignment(alignment->unique_id, alignment, &dump_data) ;

  result = dump_data.status ;

  return result ;
}





static void doAlignment(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  NewDumpFeatures dump_data = (NewDumpFeatures)user_data ;
  const char *sequence_name ;
  char *header ;
  GIOStatus status ;
  gsize bytes_written ;

  sequence_name = g_quark_to_string(alignment->original_id) ;

  header = g_strdup_printf("##Type DNA %s\n", sequence_name) ;

  if ((status = g_io_channel_write_chars(dump_data->file, header, -1,
					 &bytes_written, dump_data->error_out))
      != G_IO_STATUS_NORMAL)
    {
      dump_data->status = FALSE ;
    }
  else
    {
      /* Do all the blocks within the alignment. */
      g_list_foreach(alignment->blocks, doBlock, dump_data) ;
    }

  return ;
}

static void doBlock(gpointer data, gpointer user_data)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)data ;
  NewDumpFeatures dump_data = (NewDumpFeatures)user_data ;
  const char *sequence_name ;
  int start, end ;
  gboolean reversed = FALSE ;
  char *header;
  GIOStatus status ;
  gsize bytes_written ;

  if (dump_data->status)
    {
      sequence_name = g_quark_to_string(block->parent->original_id) ;
      start = block->block_to_sequence.q1 ;
      end = block->block_to_sequence.q2 ;

      /* I'm a little unsure about the strand stuff...think more about this... */
      if (block->block_to_sequence.q_strand == ZMAPSTRAND_REVERSE)
	reversed = TRUE ;

      header = g_strdup_printf("##sequence-region %s %d %d %s\n",
			       sequence_name, start, end, (reversed ? "(reversed)" : "")) ;

      if ((status = g_io_channel_write_chars(dump_data->file, header, -1,
					     &bytes_written, dump_data->error_out))
	  != G_IO_STATUS_NORMAL)
	{
	  dump_data->status = FALSE ;
	}
    }


  return ;
}


static gboolean dumpFeature(GIOChannel *file, gpointer user_data,
			    ZMapFeatureTypeStyle style,
			    char *parent_name,
			    char *feature_name,
			    char *style_name,
			    char *ontology,
			    int x1, int x2,
			    gboolean has_score, float score,
			    ZMapStrand strand,
			    ZMapPhase phase,
			    ZMapFeatureType feature_type,
			    gpointer feature_data,
			    GError **error_out)
{
  gboolean result = TRUE ;
  GIOStatus status ;
  gsize bytes_written ;
  GString *buffer = (GString *)user_data ;
  const char *gff_source = NULL, *gff_feature = NULL ;


  /* Don't dump some features...this will go when dna is not longer a feature... */
  if (g_ascii_strcasecmp(feature_name, "dna") == 0)
    return TRUE ;


  /* Set up GFF output specials from style. */
  if (style->gff_source)
    gff_source = g_quark_to_string(style->gff_source) ;
  if (style->gff_feature)
    gff_feature = g_quark_to_string(style->gff_feature) ;

  /* Fields are: <seqname> <source> <feature> <start> <end> <score> <strand> <frame> */

  /* Output a record for the whole feature. */

  /* Obligatory fields. */
  g_string_append_printf(buffer, "%s\t%s\t%s\t%d\t%d",
			 parent_name,
			 (gff_source ? gff_source : style_name),
			 (gff_feature ? gff_feature : ontology),
			 x1, x2) ;

  if (has_score)
    g_string_append_printf(buffer, "\t%g", score) ;
  else
    g_string_append_printf(buffer, "\t%c", '.') ;

  g_string_append_printf(buffer, "\t%c\t%c",
			 strand2Char(strand),
			 phase2Char(phase)) ;

  /* Attribute fields. */
  g_string_append_printf(buffer, "\t\"%s\"", feature_name) ;


  if (feature_type == ZMAPFEATURE_ALIGNMENT && feature_data)
    {
      ZMapHomol homol_data = (ZMapHomol)feature_data ;

      g_string_append_printf(buffer, " %d %d", homol_data->y1, homol_data->y2) ;
    }

  g_string_append_printf(buffer, "\n") ;

  if ((status = g_io_channel_write_chars(file, buffer->str, -1, &bytes_written, error_out))
      != G_IO_STATUS_NORMAL)
    result = FALSE ;

  buffer = g_string_truncate(buffer, 0) ;		    /* Reset attribute string to empty. */




  /* If the feature has sub-parts (e.g. exons) then output them one per line. */
  if (result)
    {
      if (feature_type == ZMAPFEATURE_TRANSCRIPT && feature_data)
	{
	  ZMapTranscript transcript = (ZMapTranscript)feature_data ;

	  /* Here we need to output some SO terms for exons etc.... */
	  if (transcript->exons)
	    {
	      result = printTranscriptSubpart(file, buffer,
					      transcript->exons, FALSE,
					      parent_name, feature_name, 
					      (char *)(gff_source ? gff_source : style_name),
					      "exon",
					      strand,
					      error_out) ;
	    }

	  /* when we define CDS we should also print out "coding_exon" as for exon.... */
	  if (transcript->flags.cds)
	    {
	      result = printTranscriptSubpart(file, buffer,
					      transcript->exons, TRUE,
					      parent_name, feature_name,
					      (char *)(gff_source ? gff_source : style_name),
					      "coding_exon",
					      strand,
					      error_out) ;
	    }

	  if (result && transcript->introns)
	    {
	      result = printTranscriptSubpart(file, buffer,
					      transcript->introns, FALSE,
					      parent_name, feature_name,
					      (char *)(gff_source ? gff_source : style_name),
					      "intron",
					      strand,
					      error_out) ;
	    }


	}
      else if (feature_type == ZMAPFEATURE_ALIGNMENT)
	{
      





	}
    }


  return result ;
}

/* Dump either a set of exons or introns, ontology gives a string identifying which it is. */
static gboolean printTranscriptSubpart(GIOChannel *file, GString *buffer,
				       GArray *subparts, gboolean coding,
				       char *parent_name, char *feature_name,
				       char *style_name, char *ontology,
				       ZMapStrand strand,
				       GError **error_out)
{
  gboolean result = TRUE ;
  int i ;
  GIOStatus status ;
  gsize bytes_written ;
  ZMapPhase phase = ZMAPPHASE_NONE ;
  int cds_length = 0 ;

  for (i = 0 ; i < subparts->len && result ; i++)
    {
      ZMapSpanStruct span = g_array_index(subparts, ZMapSpanStruct, i) ;

      /* Obligatory fields. */
      g_string_append_printf(buffer, "%s\t%s\t%s\t%d\t%d\t%c",
			     parent_name,
			     style_name,
			     ontology,
			     span.x1, span.x2,
			     '.') ;

      if (coding)
	{
	  int phase_num = 0 ;

	  cds_length += span.x2 - span.x1 + 1 ;

	  phase_num = (cds_length % 3) ;

	  phase = (ZMapPhase)(phase_num + 1) ;		    /* convert to enum... */
	}

      g_string_append_printf(buffer, "\t%c\t%c",
			     strand2Char(strand),
			     phase2Char(phase)) ;


      /* Attribute fields. */
      g_string_append_printf(buffer, "\t\"%s\"\n", feature_name) ;



      if ((status = g_io_channel_write_chars(file, buffer->str, -1, &bytes_written, error_out))
	  != G_IO_STATUS_NORMAL)
	result = FALSE ;

      buffer = g_string_truncate(buffer, 0) ;	    /* Reset attribute string to empty. */
    }


  return result ;
}



static char strand2Char(ZMapStrand strand)
{
  char strand_char = '.' ;

  switch(strand)
    {
    case ZMAPSTRAND_FORWARD:
      strand_char = '+' ;
      break ;
    case ZMAPSTRAND_REVERSE:
      strand_char = '-' ;
      break ;
    }

  return strand_char ;
}


static char phase2Char(ZMapPhase phase)
{
  char phase_char = '.' ;

  switch(phase)
    {
    case ZMAPPHASE_0:
      phase_char = '0' ;
      break ;
    case ZMAPPHASE_1:
      phase_char = '1' ;
      break ;
    case ZMAPPHASE_2:
      phase_char = '2' ;
      break ;
    }

  return phase_char ;
}
