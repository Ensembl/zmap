/*  File: zmapGFF2Dumper.c
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
 * Description: Dumps the features within a given block as GFF v2
 *
 * Exported functions: See ZMap/zmapGFF.h
 * HISTORY:
 * Last edited: Apr 22 14:46 2010 (edgrif)
 * Created: Mon Nov 14 13:21:14 2005 (edgrif)
 * CVS info:   $Id: zmapGFF2Dumper.c,v 1.19 2010-04-22 13:49:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapGFF.h>



#define GFF_SEPARATOR  "\t"
#define GFF_SEQ        "%s"
#define GFF_SOURCE     "%s"
#define GFF_FEATURE    "%s"
#define GFF_START      "%d"
#define GFF_END        "%d"
#define GFF_SCORE      "%g"
#define GFF_STRAND     "%c"
#define GFF_FRAME      "%c"
#define GFF_ATTRIBUTES "%s"
#define GFF_ATTRIBUTE_SEPARATOR ';'

#define GFF_ATTRIBUTE_TAB_BEGIN "\t"
#define GFF_ATTRIBUTE_TAB_END   ""

#define GFF_QUOTED_ATTRIBUTE "%s \"%s\""

#define GFF_SEQ_SOURCE_FEAT_START_END \
GFF_SEQ GFF_SEPARATOR     \
GFF_SOURCE GFF_SEPARATOR  \
GFF_FEATURE GFF_SEPARATOR \
GFF_START GFF_SEPARATOR   \
GFF_END
#define GFF_SEP_SCORE      GFF_SEPARATOR GFF_SCORE
#define GFF_SEP_NOSCORE    GFF_SEPARATOR "%c"
#define GFF_STRAND_FRAME   GFF_SEPARATOR GFF_STRAND GFF_SEPARATOR GFF_FRAME
#define GFF_SEP_ATTRIBUTES GFF_SEPARATOR GFF_ATTRIBUTES
#define GFF_OBLIGATORY         GFF_SEQ_SOURCE_FEAT_START_END GFF_SEP_SCORE GFF_STRAND_FRAME 
#define GFF_OBLIGATORY_NOSCORE GFF_SEQ_SOURCE_FEAT_START_END GFF_SEP_NOSCORE GFF_STRAND_FRAME 
#define GFF_FULL_LINE GFF_SEQ_SOURCE_FEAT_START_END GFF_SEP_SCORE GFF_STRAND_FRAME GFF_SEP_ATTRIBUTES

typedef struct _GFFDumpDataStruct *GFFDumpData;

/* annoyingly we need to use a gpointer for 2nd argument. */
typedef gboolean (* DumpGFFAttrFunc)(ZMapFeature feature, gpointer feature_data,
				     GString *gff_string, GError **error_out,
				     GFFDumpData gff_data);


typedef struct
{
  GString *header_string;	/* string to use as buffer. */
  const char *gff_sequence;	/* can be shared between header and dump data structs in case dumping list. */
  gboolean status, cont;	/* simple status and continuation flags */
} GFFHeaderDataStruct, *GFFHeaderData ;

/* Fields are: <sequence> <source> <feature> <start> <end> <score> <strand> <frame> <attributes> */
typedef struct _GFFDumpDataStruct
{
  GData *styles ;

  const char *gff_sequence;	/* The sequence name. e.g. 16.12345-23456 */
  char *gff_source;		/* The source e.g. augustus, TrEMBL, trf, polya_site */
  char *gff_feature;		/* The feature type e.g. Sequence, intron, exon, CDS, coding_exon, misc_feature */

  const char *feature_name;	/* The feature name e.g. AC12345.1-001 */
  char *link_term;		/* The attribute key for the parent feature name. e.g. Sequence in Sequence "AC12345.1-001" */

  /* functions for doing attributes. */
  DumpGFFAttrFunc *basic;
  DumpGFFAttrFunc *transcript;
  DumpGFFAttrFunc *homol;
  DumpGFFAttrFunc *text;
}GFFDumpDataStruct;

/* Functions to dump the header section of gff files */
static gboolean dump_full_header(ZMapFeatureAny feature_any, GIOChannel *file, GError **error_out,
				 const char **sequence_in_out) ;
static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark   key, 
							      gpointer data, 
							      gpointer user_data,
							      char   **err_out) ;

/* Functions to dump the body of the data */

/* ZMapFeatureDumpFeatureFunc to dump gff. writes lines into gstring buffer... */
static gboolean dump_gff_cb(ZMapFeatureAny feature_any,
			    GData         *styles,
			    GString       *gff_string,
			    GError       **error,
			    gpointer       user_data);

/* dump attributes in one way... */
static gboolean dump_attributes(DumpGFFAttrFunc func_array[],
				ZMapFeature     feature,
				gpointer        feature_data,
				GString        *gff_string,
				GError        **error,
				GFFDumpData     gff_data);

static gboolean dump_text_note(ZMapFeature    feature,
			       gpointer       basic_data,
			       GString       *gff_string,
			       GError       **error,
			       GFFDumpData    gff_data);

/* transcripts */
static gboolean dump_transcript_identifier(ZMapFeature    feature,
					   gpointer       transcript_data,
					   GString       *gff_string,
					   GError       **error,
					   GFFDumpData    gff_data);

static gboolean dump_transcript_locus(ZMapFeature    feature,
				      gpointer       transcript_data,
				      GString       *gff_string,
				      GError       **error,
				      GFFDumpData    gff_data);

static gboolean dump_transcript_subpart_lines(ZMapFeature    feature,
					      gpointer       transcript_data,
					      GString       *gff_string,
					      GError       **error,
					      GFFDumpData    gff_data);
/* transcript subparts as lines... */
static gboolean dump_transcript_foreach_subpart(ZMapFeature feature,
						GString    *buffer,
						GError    **error_out,
						GArray     *subparts, 
						GFFDumpData gff_data);


/* alignments */

static gboolean dump_alignment_target(ZMapFeature feature, gpointer homol,
				      GString *gff_string, GError **error,
				      GFFDumpData gff_data);
static gboolean dump_alignment_gaps(ZMapFeature feature, gpointer homol,
				    GString *gff_string, GError **error,
				    GFFDumpData gff_data);
static gboolean dump_alignment_clone(ZMapFeature feature, gpointer homol,
				     GString *gff_string, GError **error,
				     GFFDumpData gff_data);
static gboolean dump_alignment_length(ZMapFeature feature, gpointer homol,
				      GString *gff_string, GError **error,
				      GFFDumpData gff_data);


/* utils */
static char strand2Char(ZMapStrand strand) ;
static char phase2Char(ZMapPhase phase) ;


static DumpGFFAttrFunc basic_funcs_G[] = {
  dump_text_note,		/* Note "cpg island" */
  NULL,
};
static DumpGFFAttrFunc transcript_funcs_G[] = {
  dump_transcript_identifier, /* Sequence "AC12345.1-001" */
  dump_transcript_locus,	/* RPC1-2 */
  dump_transcript_subpart_lines, /* Not really attributes like alignment subparts are. */
  NULL
};
static DumpGFFAttrFunc homol_funcs_G[] = {
  dump_alignment_target,	/* Target "Sw:Q12345.1" 101 909 */
  dump_alignment_clone,	/* Clone "AC12345.1" */
  dump_alignment_gaps,	/* Gaps "1234 1245 4567 4578, 2345 2356 5678 5689" */
  dump_alignment_length,	/* Length 789 */
  NULL
};
static DumpGFFAttrFunc text_funcs_G[] = {
  dump_text_note,
  dump_transcript_locus,
  NULL
};      


/* NOTE: if we want this to dump a context it will have to cope with lots of different
 * sequences for the different alignments...I think the from alignment down is ok but the title
 * for the file is not if we have different alignments....and neither are the coords....?????
 * 
 * I think we would need multiple Type and sequence-region comment lines for the different
 * sequences.
 * 
 *  */
gboolean zMapGFFDump(ZMapFeatureAny dump_set, GData *styles, GIOChannel *file, GError **error_out)
{
  gboolean result = TRUE;

  result = zMapGFFDumpRegion(dump_set, styles, NULL, file, error_out);

  return result;
}

gboolean zMapGFFDumpRegion(ZMapFeatureAny dump_set, GData *styles,
			   ZMapSpan region_span, GIOChannel *file, GError **error_out)
{
  const char *sequence = NULL;
  gboolean result = TRUE ;

  zMapAssert(file && dump_set && error_out) ;
  zMapAssert(dump_set->struct_type == ZMAPFEATURE_STRUCT_CONTEXT
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_ALIGN
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_BLOCK
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  result = dump_full_header(dump_set, file, error_out, &sequence) ;

  if (result)
    {
      GFFDumpDataStruct gff_data = {NULL};

      gff_data.basic      = basic_funcs_G;
      gff_data.transcript = transcript_funcs_G;
      gff_data.homol      = homol_funcs_G;
      gff_data.text       = text_funcs_G;
      gff_data.styles     = styles ;

      /* This might get overwritten later, but as DumpToFile uses
       * Subset, there's a chance it wouldn't get set at all */
      gff_data.gff_sequence = sequence;	
      if(region_span)
	result = zMapFeatureContextRangeDumpToFile((ZMapFeatureAny)dump_set, styles, region_span, 
						   dump_gff_cb, &gff_data, file, error_out) ;
      else
	result = zMapFeatureContextDumpToFile((ZMapFeatureAny)dump_set, styles, dump_gff_cb, 
					      &gff_data, file, error_out) ;
    }

  return result ;  
}

/*!
 * \brief Dump a list of ZMapFeatureAny. sequence can be NULL
 */
gboolean zMapGFFDumpList(GList *dump_list, GData *styles, char *sequence, GIOChannel *file, GError **error_out)
{
  const char *int_sequence = NULL;
  gboolean result = FALSE ;

  if(dump_list && dump_list->data)
    {
      ZMapFeatureAny feature_any;

      feature_any  = (ZMapFeatureAny)(dump_list->data);
      int_sequence = sequence;

      result       = dump_full_header(feature_any, file, error_out, &int_sequence) ;
    }

  if (result)
    {
      GFFDumpDataStruct gff_data = {NULL};

      gff_data.basic      = basic_funcs_G;
      gff_data.transcript = transcript_funcs_G;
      gff_data.homol      = homol_funcs_G;
      gff_data.text       = text_funcs_G;

      /* This might get overwritten later, but as DumpToFile uses
       * Subset, there's a chance it wouldn't get set at all */
      gff_data.gff_sequence = int_sequence;	

      result = zMapFeatureListDumpToFile(dump_list, styles, dump_gff_cb, &gff_data, 
					 file, error_out) ;
    }

  return result ;
}

gboolean zMapGFFDumpForeachList(ZMapFeatureAny first_feature, GData *styles,
				GIOChannel *file, GError **error_out,
				char *sequence,
				GFunc *list_func_out, gpointer *list_data_out)
{
  gboolean result = FALSE ;
  const char *int_sequence = NULL ;


  if ((list_func_out && list_data_out))
    {
      int_sequence = sequence;

      if ((result = dump_full_header(first_feature, file, error_out, &int_sequence)))
	{
	  GFFDumpData gff_data = NULL;

	  gff_data             = g_new0(GFFDumpDataStruct, 1);

	  gff_data->basic      = basic_funcs_G;
	  gff_data->transcript = transcript_funcs_G;
	  gff_data->homol      = homol_funcs_G;
	  gff_data->text       = text_funcs_G;

	  /* This might get overwritten later, but as DumpToFile uses
	   * Subset, there's a chance it wouldn't get set at all */
	  gff_data->gff_sequence = int_sequence;	

	  result = zMapFeatureListForeachDumperCreate(dump_gff_cb, styles, gff_data, g_free,
						      file,          error_out, 
						      list_func_out, list_data_out) ;
	}
    }

  return result ;
}

/* INTERNALS */

static gboolean dump_full_header(ZMapFeatureAny feature_any, GIOChannel *file, GError **error_out, const char **sequence_in_out)
{
  enum {GFF_VERSION = 2} ;
  GFFHeaderDataStruct header_data = {NULL};
  char *time_string ;

  /* Dump the standard header lines. */
  time_string = zMapGetTimeString(ZMAPTIME_YMD, NULL) ;

  header_data.header_string = g_string_sized_new(1024); /* plenty enough */
  header_data.cont   = TRUE;
  header_data.status = TRUE;

  if(sequence_in_out && *sequence_in_out)
    header_data.gff_sequence = *sequence_in_out;

  /* gff-version, source-version, date */
  g_string_append_printf(header_data.header_string, 
			 "##gff-version %d\n"
			 "##source-version %s %s\n"
			 "##date %s\n",
			 GFF_VERSION,
			 zMapGetAppName(), zMapGetVersionString(),
			 time_string) ;

  g_free(time_string) ;

  zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_BLOCK,
			    get_type_seq_header_cb, &header_data);
  
  if(header_data.status)
    {
      GIOStatus write_status ;
      gsize bytes_written ;

      if ((write_status = g_io_channel_write_chars(file, 
						   header_data.header_string->str, 
						   header_data.header_string->len, 
						   &bytes_written, error_out)) != G_IO_STATUS_NORMAL)
	{
	  header_data.status = FALSE;
	}

      if(sequence_in_out)
	*sequence_in_out = header_data.gff_sequence;
    }

  g_string_free(header_data.header_string, TRUE);

  return header_data.status;
}

static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark   key, 
							      gpointer data, 
							      gpointer user_data,
							      char   **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  GFFHeaderData header_data  = (GFFHeaderData)user_data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      if(header_data->status && header_data->cont)
	{
	  const char *sequence;
	  header_data->status = header_data->cont = TRUE;

	  sequence = g_quark_to_string(feature_any->original_id);

	  g_string_append_printf(header_data->header_string,
				 "##Type DNA %s\n",
				 sequence);

	  if(!header_data->gff_sequence)
	    header_data->gff_sequence = sequence;
	}
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      if(header_data->status && header_data->cont)
	{
	  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)feature_any;
	  gboolean reversed = FALSE;
	  int start, end;
	  
	  start = feature_block->block_to_sequence.q1;
	  end   = feature_block->block_to_sequence.q2;
	  
	  /* I'm a little unsure about the strand stuff...think more about this... */
	  if (feature_block->block_to_sequence.q_strand == ZMAPSTRAND_REVERSE)
	    reversed = TRUE ;
	  
	  g_string_append_printf(header_data->header_string,
				 "##sequence-region %s %d %d %s\n",
				 g_quark_to_string(feature_any->original_id), 
				 start, end, (reversed ? "(reversed)" : "")) ;
	  header_data->status = TRUE;
	  header_data->cont   = FALSE;
	}
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
    case ZMAPFEATURE_STRUCT_FEATURE:
    default:
      status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND;
      break;
    }

  return status;
}

static gboolean dump_gff_cb(ZMapFeatureAny feature_any, 
			    GData         *styles,
			    GString       *gff_string,
			    GError       **error,
			    gpointer       user_data)
{
  GFFDumpData gff_data = (GFFDumpData)user_data;
  gboolean result = TRUE;
  
  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      gff_data->gff_sequence = g_quark_to_string(feature_any->original_id);
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_BLOCK:
      result = TRUE;
      /* We don't dump these to gff */
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      /* we need to do feature filter at this level, otherwise it'll slow stuff down... */
      result = TRUE;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature feature = (ZMapFeature)feature_any;
	ZMapFeatureTypeStyle style ;

	style = zMapFindStyle(gff_data->styles, feature->style_id) ;

	/* Output a record for the whole feature, fields are:
	 *      <seqname> <source> <feature> <start> <end> <score> <strand> <frame> */
	if(!(gff_data->gff_source  = (char *)zMapStyleGetGFFSource(style)))
	  gff_data->gff_source = (char *)zMapStyleGetName(style);
	if(!(gff_data->gff_feature = (char *)zMapStyleGetGFFFeature(style)))
	  gff_data->gff_feature = (char *)g_quark_to_string(feature->ontology);
	
	/* Obligatory fields. */
	g_string_append_printf(gff_string,
			       GFF_SEQ_SOURCE_FEAT_START_END,
			       gff_data->gff_sequence, /* saves feature->parent->parent->parent->original_id */
			       gff_data->gff_source,
			       gff_data->gff_feature,
			       feature->x1,
			       feature->x2);

	if (feature->flags.has_score)
	  g_string_append_printf(gff_string, GFF_SEP_SCORE, feature->score) ;
	else
	  g_string_append_printf(gff_string, GFF_SEP_NOSCORE, '.') ;
	
	g_string_append_printf(gff_string, GFF_STRAND_FRAME,
			       strand2Char(feature->strand),
			       phase2Char(feature->phase)) ;

	/* Now to the attribute fields, and any subparts... */

	switch(feature->type)
	  {
	  case ZMAPSTYLE_MODE_BASIC:
	    {	      
	      result = dump_attributes(gff_data->basic, feature, 
				       &(feature->feature.basic),
				       gff_string, error, gff_data);
	    }
	    break;
	  case ZMAPSTYLE_MODE_TRANSCRIPT:
	    {
	      /* Do transcript attributes, then each of the sub features need processing */
	      gff_data->feature_name = g_quark_to_string(feature->original_id);
	      gff_data->link_term    = "Sequence";
	      gff_data->link_term    = gff_data->gff_feature;

	      result = dump_attributes(gff_data->transcript, feature, 
				       &(feature->feature.transcript),
				       gff_string, error, gff_data);
	      
	    }
	    break;
	  case ZMAPSTYLE_MODE_ALIGNMENT:
	    {
	      result = dump_attributes(gff_data->homol, feature, 
				       &(feature->feature.homol),
				       gff_string, error, gff_data);
     	    }
	    break;
	  case ZMAPSTYLE_MODE_TEXT:
	    {
	      result = dump_attributes(gff_data->text, feature,
				       NULL, /* this needs to be &(feature->feature.text) */
				       gff_string, error, gff_data);
	    }
	    break;
	  default:
	    /* This might be a little odd... */
	    break;
	  }

	g_string_append_c(gff_string, '\n');
      }
      break;
    default:
      result = FALSE;
      break;
    }

  return result;
}

/* attribute dumper. run through the dump functions separating the
 * attributes with GFF_ATTRIBUTE_SEPARATOR */

static gboolean dump_attributes(DumpGFFAttrFunc func_array[],
				ZMapFeature     feature,
				gpointer        feature_data,
				GString        *gff_string,
				GError        **error,
				GFFDumpData     gff_data)
{
  DumpGFFAttrFunc *funcs_ptr;
  gboolean result = TRUE, need_separator = FALSE;

  funcs_ptr = func_array;

  while(funcs_ptr && *funcs_ptr)
    {
      if(need_separator)
	{
	  g_string_append_c(gff_string, GFF_ATTRIBUTE_SEPARATOR);
	  need_separator = FALSE;
	}

      if((*funcs_ptr)(feature, feature_data, gff_string, error, gff_data))
	need_separator = TRUE;
      
      funcs_ptr++;
    }
  
  return result;
}

static gboolean dump_text_note(ZMapFeature    feature,
			       gpointer       basic_data,
			       GString       *gff_string,
			       GError       **error,
			       GFFDumpData    gff_data)
{
  gboolean result = FALSE;

  if(feature->description)
    {
      g_string_append_printf(gff_string, 
			     GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
			     "Note", feature->description);
      result = TRUE;
    }

  return result;
}


/* transcript calls */

/* attributes */
static gboolean dump_transcript_identifier(ZMapFeature    feature,
					   gpointer       transcript_data,
					   GString       *gff_string,
					   GError       **error,
					   GFFDumpData    gff_data)
{
  gboolean result = TRUE;

  /* To make       Sequence "AC12345.1"          */
  g_string_append_printf(gff_string, 
			 GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
			 gff_data->link_term,
			 gff_data->feature_name);
  

  return result;
}

static gboolean dump_transcript_locus(ZMapFeature    feature,
				      gpointer       transcript_data,
				      GString       *gff_string,
				      GError       **error,
				      GFFDumpData    gff_data)
{
  gboolean result = FALSE;

  if(feature->locus_id)
    {
      /* To make       Locus "AC12345.1"          */
      g_string_append_printf(gff_string, 
			     GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
			     "Locus",
			     (char *)g_quark_to_string(feature->locus_id));
      result = TRUE;
    }
  
  return result;
}


/* subpart full lines... */
static gboolean dump_transcript_subpart_lines(ZMapFeature    feature,
					      gpointer       transcript_data,
					      GString       *gff_string,
					      GError       **error,
					      GFFDumpData    gff_data)
{
  ZMapTranscript transcript = (ZMapTranscript)transcript_data;
  gboolean result = TRUE;

  if(result && transcript->exons)
    {
      gff_data->gff_feature = "exon";
      result = dump_transcript_foreach_subpart(feature, gff_string, error, 
					       transcript->exons, gff_data);
    }

  if(result && transcript->introns)
    {
      gff_data->gff_feature = "intron";
      result = dump_transcript_foreach_subpart(feature, gff_string, error,
					       transcript->introns, gff_data);
    }

  if(result && transcript->flags.cds)
    {
      gff_data->gff_feature = "CDS";
      g_string_append_printf(gff_string, 
			     "\n" GFF_OBLIGATORY_NOSCORE,
			     gff_data->gff_sequence,
			     gff_data->gff_source,
			     gff_data->gff_feature,
			     transcript->cds_start, 
			     transcript->cds_end,
			     '.',
			     strand2Char(feature->strand),
			     phase2Char(feature->phase)) ;
      
      result = dump_transcript_identifier(feature, transcript,
					  gff_string, error, 
					  gff_data);
    }

  return result;
}

static gboolean dump_transcript_foreach_subpart(ZMapFeature feature,
						GString    *buffer,
						GError    **error_out,
						GArray     *subparts, 
						GFFDumpData gff_data)
{
  gboolean result = TRUE ;
  int i ;

  for (i = 0 ; i < subparts->len && result ; i++)
    {
      ZMapSpanStruct span = g_array_index(subparts, ZMapSpanStruct, i) ;
#warning Need to do something about phase here!
      ZMapPhase phase = ZMAPPHASE_NONE ;

      /* 16.2810538-2863623      Transcript      intron  19397   20627   .       -       .       Sequence "AC005361.6-003" */

      /* Obligatory fields. */
      g_string_append_printf(buffer, "\n" GFF_OBLIGATORY_NOSCORE,
			     gff_data->gff_sequence,
			     gff_data->gff_source,
			     gff_data->gff_feature,
			     span.x1, span.x2,
			     '.', /* no score */
			     strand2Char(feature->strand),
			     phase2Char(phase)) ;

      result = dump_transcript_identifier(feature, &(feature->feature.transcript),
					  buffer, error_out, gff_data);
    }
  
  return result ;
}


/* alignment attributes... */

static gboolean dump_alignment_target(ZMapFeature feature, gpointer homol_data,
				      GString *gff_string, GError **error,
				      GFFDumpData gff_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean has_target = TRUE;

  if(feature->original_id)
    {
      g_string_append_printf(gff_string, 
			     GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE " %d %d" GFF_ATTRIBUTE_TAB_END,
			     "Target", g_quark_to_string(feature->original_id),
			     homol->y1, homol->y2);
    }

  return has_target;
}

static gboolean dump_alignment_gaps(ZMapFeature feature, gpointer homol_data,
				    GString *gff_string, GError **error,
				    GFFDumpData gff_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  GArray *gaps_array = NULL;
  int i;
  gboolean has_gaps = FALSE;

  if((gaps_array = homol->align))
    {
      has_gaps = TRUE;

      g_string_append_printf(gff_string, GFF_ATTRIBUTE_TAB_BEGIN "%s ", "Gaps");
      
      g_string_append_c(gff_string, '"');

      for(i = 0; i < gaps_array->len; i++)
	{
	  ZMapAlignBlock block = NULL;
	  
	  block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i));
	  
	  g_string_append_printf(gff_string, "%d %d %d %d,", 
				 block->q1, block->q2, 
				 block->t1, block->t2);
	}
      
      /* remove the last comma! */
      g_string_truncate(gff_string, gff_string->len - 1);
      /* and add a quote and a tab?*/
      g_string_append_printf(gff_string, 
			     "%c" GFF_ATTRIBUTE_TAB_END,
			     '"');
    }
  
  return has_gaps;
}

static gboolean dump_alignment_clone(ZMapFeature feature, gpointer homol_data,
				     GString *gff_string, GError **error,
				     GFFDumpData gff_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean has_clone = FALSE;

  if(homol->flags.has_clone_id)
    {
      g_string_append_printf(gff_string, 
			     GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
			     "Clone", g_quark_to_string(homol->clone_id));
      has_clone = TRUE;
    }

  return has_clone;
}

static gboolean dump_alignment_length(ZMapFeature feature, gpointer homol_data,
				      GString *gff_string, GError **error,
				      GFFDumpData gff_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean has_length = FALSE;

  if(homol->length)
    {    
      g_string_append_printf(gff_string, 
			     GFF_ATTRIBUTE_TAB_BEGIN "Length %d" GFF_ATTRIBUTE_TAB_END,
			     homol->length);
      has_length = TRUE;
    }

  return has_length;
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
    default:
      strand_char = '.' ;
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
    default:
      phase_char = '.' ;
      break ;
    }

  return phase_char ;
}

