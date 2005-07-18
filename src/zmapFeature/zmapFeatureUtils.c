/*  File: zmapFeature.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Manipulates features.
 *              1
 * Exported functions: See zmapFeature.h
 * HISTORY:
 * Last edited: Jul 17 12:58 2005 (edgrif)
 * Created: Tue Nov 2 2004 (rnc)
 * CVS info:   $Id: zmapFeatureUtils.c,v 1.18 2005-07-18 09:19:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>


typedef struct
{
  gboolean status ;
  GIOChannel *channel ;
} DumpFeaturesStruct, *DumpFeatures ;



static void printFeatureContext(ZMapFeatureContext feature_context, DumpFeatures feature_dump) ;
static void printFeatureAlignment(GQuark key_id, gpointer data, gpointer user_data) ;
static void printFeatureBlock(gpointer data, gpointer user_data) ;
static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void printFeature(GQuark key_id, gpointer data, gpointer user_data) ;
static gboolean printLine(GIOChannel *channel, gchar *line) ;
static gint findStyle(gconstpointer list_data, gconstpointer user_data) ;
static gint findStyleName(gconstpointer list_data, gconstpointer user_data) ;


/* This function creates a unique id for a feature. This is essential if we are to use the
 * g_datalist package to hold and reference features. Code should _ALWAYS_ use this function
 * to produce these IDs.
 * Caller must g_free() returned string when finished with. */
char *zMapFeatureCreateName(ZMapFeatureType feature_type, char *feature,
			    ZMapStrand strand, int start, int end, int query_start, int query_end)
{
  char *feature_name = NULL ;

  zMapAssert(feature_type >= 0 && feature && start >= 1 && start <= end) ;

  if (feature_type == ZMAPFEATURE_HOMOL)
    feature_name = g_strdup_printf("%s_%d.%d_%d.%d", feature,
				   start, end, query_start, query_end) ;
  else
    feature_name = g_strdup_printf("%s_%d.%d", feature, start, end) ;

  return feature_name ;
}


/* Like zMapFeatureCreateName() but returns a quark representing the feature name. */
GQuark zMapFeatureCreateID(ZMapFeatureType feature_type, char *feature,
			   ZMapStrand strand, int start, int end,
			   int query_start, int query_end)
{
  GQuark feature_id = 0 ;
  char *feature_name ;

  if ((feature_name = zMapFeatureCreateName(feature_type, feature, strand, start, end,
					    query_start, query_end)))
    {
      feature_id = g_quark_from_string(feature_name) ;
      g_free(feature_name) ;
    }

  return feature_id ;
}

GQuark zMapFeatureBlockCreateID(int ref_start, int ref_end, ZMapStrand ref_strand,
                                int non_start, int non_end, ZMapStrand non_strand)
{
  GQuark block_id = 0;
  char *id_base ;

  id_base = g_strdup_printf("%d.%d.%s_%d.%d.%s", 
			    ref_start, ref_end,
			    (ref_strand == ZMAPSTRAND_FORWARD ? "+" : "-"), 
			    non_start, non_end,
			    (non_strand == ZMAPSTRAND_FORWARD ? "+" : "-")) ;
  block_id = g_quark_from_string(id_base) ;
  g_free(id_base) ;

  return block_id;
}


/* In zmap we hold coords in the forward orientation always and get strand from the strand
 * member of the feature struct. This function looks at the supplied strand and sets the
 * coords accordingly. */
/* ACTUALLY I REALISE I'M NOT QUITE SURE WHAT I WANT THIS FUNCTION TO DO.... */
gboolean zMapFeatureSetCoords(ZMapStrand strand, int *start, int *end, int *query_start, int *query_end)
{
  gboolean result = FALSE ;

  zMapAssert(start && end && query_start && query_end) ;

  if (strand == ZMAPSTRAND_REVERSE)
    {
      if ((start && end) && start > end)
	{
	  int tmp ;

	  tmp = *start ;
	  *start = *end ;
	  *end = tmp ;

	  if (query_start && query_end)
	    {
	      tmp = *query_start ;
	      *query_start = *query_end ;
	      *query_end = tmp ;
	    }

	  result = TRUE ;
	}
    }
  else
    result = TRUE ;


  return result ;
}


GQuark zMapFeatureGetStyleQuark(ZMapFeature feature)
{
  GQuark style_quark ;

  style_quark = feature->parent_set->style_id ;

  return style_quark ;
}



ZMapFeatureTypeStyle zMapFeatureGetStyle(ZMapFeature feature)
{
  ZMapFeatureTypeStyle style ;
  GList *styles = feature->parent_set->parent_block->parent_alignment->parent_context->types ;

  style = zMapFindStyle(styles, feature->parent_set->style_id) ;

  return style ;
}


ZMapFeatureTypeStyle zMapFeatureSetGetStyle(ZMapFeatureSet feature_set)
{
  ZMapFeatureTypeStyle style ;
  GList *styles = feature_set->parent_block->parent_alignment->parent_context->types ;

  style = zMapFindStyle(styles, feature_set->style_id) ;

  return style ;
}




/* Retrieve a style struct for the given style id. */
ZMapFeatureTypeStyle zMapFindStyle(GList *styles, GQuark style_id)
{
  ZMapFeatureTypeStyle style ;
  GList *list ;

  list = g_list_find_custom(styles, GUINT_TO_POINTER(style_id), findStyle) ;
  zMapAssert(list) ;

  style = list->data ;

  return style ;
}


/* Check that a style name exists in a list of styles. */
gboolean zMapStyleNameExists(GList *style_name_list, char *style_name)
{
  gboolean result = FALSE ;
  GList *list ;
  GQuark style_id ;

  style_id = zMapStyleCreateID(style_name) ;

  if ((list = g_list_find_custom(style_name_list, GUINT_TO_POINTER(style_id), findStyleName)))
    result = TRUE ;

  return result ;
}







#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* WE MAY STILL WANT A FUNCTION LIKE THIS BUT IT WILL NEED MORE ARGS, E.G. ALIGNMENT... */
ZMapFeature zMapFeatureFindFeatureInContext(ZMapFeatureContext feature_context,
					    GQuark type_id, GQuark feature_id) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


ZMapFeature zMapFeatureFindFeatureInBlock(ZMapFeatureBlock feature_block,
					  GQuark type_id, GQuark feature_id)
{
  ZMapFeature feature = NULL ;
  ZMapFeatureSet feature_set ;

  if ((feature_set = (ZMapFeatureSet)g_datalist_id_get_data(&(feature_block->feature_sets), type_id)))
    {
      feature = (ZMapFeature)g_datalist_id_get_data(&(feature_set->features), feature_id) ;
    }

  return feature ;
}



ZMapFeature zMapFeatureFindFeatureInSet(ZMapFeatureSet feature_set, GQuark feature_id)
{
  ZMapFeature feature ;

  feature = (ZMapFeature)g_datalist_id_get_data(&(feature_set->features), feature_id) ;

  return feature ;
}


GData *zMapFeatureFindSetInBlock(ZMapFeatureBlock feature_block, GQuark set_id)
{
  GData *features = NULL ;
  ZMapFeatureSet feature_set ;

  if ((feature_set = g_datalist_id_get_data(&(feature_block->feature_sets), set_id)))
    features = feature_set->features ;

  return features ;
}


/* Dump out a feature context, if file is NULL then goes to stdout. */
void zMapFeatureDump(ZMapFeatureContext feature_context, char *file)
{
  DumpFeaturesStruct dump_features ;
  GIOChannel *channel ;
  GError *channel_error = NULL ;
  char *filepath = NULL ;


  zMapAssert(feature_context) ;

  
  /* open output file */
  if (!file)
    {
      filepath = g_strdup("stdout") ;

      channel = g_io_channel_unix_new(STDOUT_FILENO) ;
    }
  else
    {
      if (g_path_is_absolute(file))
	filepath = g_strdup(file) ;
      else
	{
	  gchar *curr_dir = g_get_current_dir() ;

	  filepath = g_build_path("/", curr_dir, file, NULL) ;

	  g_free(curr_dir) ;
	}

      channel = g_io_channel_new_file(filepath, "w", &channel_error) ;
    }

  if (!channel)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zMapShowMsg(ZMAP_MSG_WARNING, "Can't open output file \"%s\": %s",
		  filepath, channel_error->message) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      g_error_free(channel_error) ;
    }
  else
    {
      dump_features.status = TRUE ;
      dump_features.channel = channel ;

      /* dump out the data */
      printFeatureContext(feature_context, &dump_features) ;

      /* close output file */
      if (g_io_channel_shutdown(channel, TRUE, &channel_error) != G_IO_STATUS_NORMAL)
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error closing output file \"%s\": %s",
		      filepath, channel_error->message) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  g_error_free(channel_error) ;
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      else 
	zMapShowMsg(ZMAP_MSG_INFORMATION, "Feature Context dumped to \"%s\"", filepath) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  g_free(filepath) ;

  return ;
}



char *zmapFeatureLookUpEnum(int id, int enumType)
{
  /* These arrays must correspond 1:1 with the enums declared in zmapFeature.h */
  static char *types[]   = {"BASIC", "HOMOL", "EXON", "INTRON", "TRANSCRIPT",
			    "VARIATION", "BOUNDARY", "SEQUENCE"} ;
  static char *strands[] = {".", "+", "-" } ;
  static char *phases[]  = {"ZMAPPHASE_NONE", "ZMAPPHASE_0", "ZMAPPHASE_1", "ZMAPPHASE_2" } ;
  static char *homolTypes[] = {"ZMAPHOMOL_N_HOMOL", "ZMAPHOMOL_X_HOMOL", "ZMAPHOMOL_TX_HOMOL"} ;
  char *enum_str = NULL ;

  zMapAssert(enumType == TYPE_ENUM || enumType == STRAND_ENUM 
	     || enumType == PHASE_ENUM || enumType == HOMOLTYPE_ENUM) ;

  switch (enumType)
    {
    case TYPE_ENUM:
      enum_str = types[id];
      break;
      
    case STRAND_ENUM:
      enum_str = strands[id];
      break;
      
    case PHASE_ENUM:
      enum_str = phases[id];
      break;

    case HOMOLTYPE_ENUM:
      enum_str = homolTypes[id];
      break;
    }
  
  return enum_str ;
}


/* For blocks within alignments other than the master alignment, it is not possible to simply
 * use the x1,x2 positions in the feature struct as these are the positions in the original
 * feature. We need to know the coordinates in the master alignment. */
void zMapFeature2MasterCoords(ZMapFeature feature, double *feature_x1, double *feature_x2)
{
  double master_x1 = 0.0, master_x2 = 0.0 ;
  ZMapFeatureBlock block ;
  double feature_offset = 0.0 ;

  zMapAssert(feature->parent_set && feature->parent_set->parent_block) ;

  block = feature->parent_set->parent_block ;

  feature_offset = block->block_to_sequence.t1 - block->block_to_sequence.q1 ;
  
  master_x1 = feature->x1 + feature_offset ;
  master_x2 = feature->x2 + feature_offset ;

  *feature_x1 = master_x1 ;
  *feature_x2 = master_x2 ;

  return ;
}



ZMapFeature zMapFeatureCopy(ZMapFeature feature)
{
  int i;
  ZMapFeature newFeature = (ZMapFeature)g_memdup(feature, sizeof(ZMapFeatureStruct));

  if (feature->type == ZMAPFEATURE_HOMOL)
    {
      ZMapAlignBlockStruct align;
      if (feature->feature.homol.align != NULL
	  && feature->feature.homol.align->len > (guint)0)
	{
	  newFeature->feature.homol.align = 
	    g_array_sized_new(FALSE, TRUE, 
			      sizeof(ZMapAlignBlockStruct),
			      feature->feature.homol.align->len);

	  for (i = 0; i < feature->feature.homol.align->len; i++)
	    {
	      align = g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i);
	      newFeature->feature.homol.align = 
		g_array_append_val(newFeature->feature.homol.align, align);
	    }
	}
    }
  else if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      ZMapSpanStruct span;
      if (feature->feature.transcript.exons != NULL
	  && feature->feature.transcript.exons->len > (guint)0)
	{
	  newFeature->feature.transcript.exons = 
	    g_array_sized_new(FALSE, TRUE, 
			      sizeof(ZMapSpanStruct),
			      feature->feature.transcript.exons->len);

	  for (i = 0; i < feature->feature.transcript.exons->len; i++)
	    {
	      span = g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
	      newFeature->feature.transcript.exons = 
		g_array_append_val(newFeature->feature.transcript.exons, span);
	    }
	}

      if (feature->feature.transcript.introns != NULL
	  && feature->feature.transcript.introns->len > (guint)0)
	{
	  newFeature->feature.transcript.introns = 
	    g_array_sized_new(FALSE, TRUE, 
			      sizeof(ZMapSpanStruct),
			      feature->feature.transcript.introns->len);

	  for (i = 0; i < feature->feature.transcript.introns->len; i++)
	    {
	      span = g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);
	      newFeature->feature.transcript.introns = 
		g_array_append_val(newFeature->feature.transcript.introns, span);
	    }
	}
    }

  return newFeature;
}


/* 
 *              Internal routines.
 */


static void printFeatureContext(ZMapFeatureContext feature_context, DumpFeatures dump_features)
{
  char *line ;

  line = g_strdup_printf("Feature Context:\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\n", 
			 g_quark_to_string(feature_context->sequence_name),
			 g_quark_to_string(feature_context->parent_name),
			 feature_context->parent_span.x1,
			 feature_context->parent_span.x2,
			 feature_context->sequence_to_parent.p1,
			 feature_context->sequence_to_parent.p2,
			 feature_context->sequence_to_parent.c1,
			 feature_context->sequence_to_parent.c2) ;

  /* Only proceed if there's no problem printing the line */
  if ((dump_features->status = printLine(dump_features->channel, line)))
    g_datalist_foreach(&(feature_context->alignments), printFeatureAlignment, dump_features) ;

  g_free(line) ;

  return ;
}


static void printFeatureAlignment(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  DumpFeatures dump_features = (DumpFeatures)user_data ;
  char *line ;

  /* Once we have failed then we stop printing, note that there is no way to stop this routine
   * being called which is a shame.... */
  if (!dump_features->status)
    return ;

  line = g_strdup_printf("\tAlignment:\t%s\n", g_quark_to_string(alignment->unique_id)) ;

  /* Only proceed if there's no problem printing the line */
  if ((dump_features->status = printLine(dump_features->channel, line)))
    g_list_foreach(alignment->blocks, printFeatureBlock, (gpointer)dump_features) ;

  g_free(line) ;

  return ;
}



static void printFeatureBlock(gpointer data, gpointer user_data)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)data ;
  DumpFeatures dump_features = (DumpFeatures)user_data ;
  char *line ;

  line = g_strdup_printf("\tBlock:\t%s\t%d\t%d\t%d\t%d\n", g_quark_to_string(block->unique_id),
			 block->block_to_sequence.t1,
			 block->block_to_sequence.t2,
			 block->block_to_sequence.q1,
			 block->block_to_sequence.q2) ;


  /* Only proceed if there's no problem printing the line */

  /* JUST FOR NOW.... */
  dump_features->status = printLine(dump_features->channel, line) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if ((dump_features->status = printLine(dump_features->channel, line)))
    g_datalist_foreach(&(block->feature_sets), printFeatureSet, dump_features) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  g_free(line) ;

  return ;
}


static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  DumpFeatures dump_features = (DumpFeatures)user_data ;
  char *line ;

  /* Once we have failed then we stop printing, note that there is no way to stop this routine
   * being called which is a shame.... */
  if (!dump_features->status)
    return ;

  line = g_strdup_printf("\tFeature Set:\t%s\t%s\n",
			 g_quark_to_string(feature_set->unique_id),
			 (char *)g_quark_to_string(feature_set->style_id)) ;

  /* Only proceed if there's no problem printing the line */
  if ((dump_features->status = printLine(dump_features->channel, line)))
    g_datalist_foreach(&(feature_set->features), printFeature, dump_features) ;
    
  g_free(line) ;

  return ;
}


static void printFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  DumpFeatures dump_features = (DumpFeatures)user_data ;
  char *type, *strand, *phase ;
  GString *line ;
  gboolean unused ;


  /* Once we have failed then we stop printing, note that there is no way to stop this routine
   * being called which is a shame.... */
  if (!dump_features->status)
    return ;


  line = g_string_sized_new(150);
  type   = zmapFeatureLookUpEnum(feature->type, TYPE_ENUM);
  strand = zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM);
  phase  = zmapFeatureLookUpEnum(feature->phase, PHASE_ENUM);
  
  g_string_printf(line, "\t\t%s\t%d\t%s\t%s\t%d\t%d\t%s\t%s\t%f", 
		  (char *)g_quark_to_string(key_id),
		  feature->db_id,
		  (char *)g_quark_to_string(feature->original_id),
		  type,
		  feature->x1,
		  feature->x2,
		  strand,
		  phase,
		  feature->score) ;
  if (feature->text)
    {
      g_string_append_c(line, '\t');
      g_string_append(line, feature->text);
    }

  /* all these extra fields not required for now.
   *if (feature->type == ZMAPFEATURE_HOMOL)
   *{
   * line = g_strdup_printf("%s\t%d\t%d\t%d\t%d\t%d\t%f\t", line,
   *			 feature->feature.homol.type,
   *			 feature->feature.homol.y1,
   *			 feature->feature.homol.y2,
   *			 feature->feature.homol.target_strand,
   *			 feature->feature.homol.target_phase,
   *			 feature->feature.homol.score
   *			 );   
   * for (i = 0; i < feature->feature.homol.align->len; i++)
   *   {
   *     align = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i);
   *     line = g_strdup_printf("%s\t%d\t%d\t%d\t%d", line,
   *			     align->q1,
   *			     align->q2,
   *		     align->t1,
   *			     align->t2
   *			     );
   *   }
   *
   *else if (feature->type == ZMAPFEATURE_TRANSCRIPT)
   *{
   *  line = g_strdup_printf("%s\t%d\t%d\t%d\t%d\t%d", line,
   *			 feature->feature.transcript.cdsStart,
   *			 feature->feature.transcript.cdsEnd,
   *			 feature->feature.transcript.start_not_found,
   *			 feature->feature.transcript.cds_phase,
   *			 feature->feature.transcript.endNotFound
   *			 );   
   *  for (i = 0; i < feature->feature.transcript.exons->len; i++)
   *    {
   *      exon = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
   *      line = g_strdup_printf("%s\t%d\t%d", line,
   *			     exon->x1,
   *			     exon->x2
   *			     );
   *    }
   *}
   */

  g_string_append_c(line, '\n');

  dump_features->status = printLine(dump_features->channel, line->str) ;

  g_string_free(line, TRUE) ;

  return ;
}


/* Should have a prefix level ?? */
static gboolean printLine(GIOChannel *channel, gchar *line)
{
  gboolean status = TRUE ;
  gsize bytes_written ;
  GError *channel_error = NULL ;

  if (g_io_channel_write_chars(channel, line, -1, &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zMapShowMsg(ZMAP_MSG_WARNING, "Error writing to output file: %50s... : %s",
		  line, channel_error->message) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      g_error_free(channel_error) ;
      status = FALSE ;
    }

  return status ;
}


/* GCompareFunc function, called for each member of a list of styles to see if the supplied 
 * style id matches the that in the style. */
static gint findStyle(gconstpointer list_data, gconstpointer user_data)
{
  gint result = -1 ;
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)list_data ;
  GQuark style_quark =  GPOINTER_TO_UINT(user_data) ;

  if (style_quark == style->unique_id)
    result = 0 ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("Looking for: %s   Found: %s\n",
	 g_quark_to_string(style_quark), g_quark_to_string(style->unique_id)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return result ;
}


/* GCompareFunc function, called for each member of a list of styles ids to see if the supplied 
 * style id matches one in the style list. */
static gint findStyleName(gconstpointer list_data, gconstpointer user_data)
{
  gint result = -1 ;
  GQuark style_list_id = GPOINTER_TO_UINT(list_data) ;
  GQuark style_quark =  GPOINTER_TO_UINT(user_data) ;

  if (style_quark == style_list_id)
    result = 0 ;

  return result ;
}


