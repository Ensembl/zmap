/*  File: zmapFeature.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Utility routines for handling features/sets/blocks etc.
 *              
 * Exported functions: See ZMap/zmapFeature.h
 * HISTORY:
 * Last edited: Jan  5 10:04 2007 (rds)
 * Created: Tue Nov 2 2004 (rnc)
 * CVS info:   $Id: zmapFeatureUtils.c,v 1.40 2007-01-05 22:25:54 rds Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapUtils.h>


typedef struct
{
  gboolean status ;
  GIOChannel *channel ;
  GError **error_out ;
} DumpFeaturesStruct, *DumpFeatures ;


/* My new general purpose dumper function.... */
typedef struct
{
  gboolean status ;
  GIOChannel *file ;
  GError **error_out ;
  ZMapFeatureDumpFeatureCallbackFunc dump_func ;
  gpointer user_data ;
} NewDumpFeaturesStruct, *NewDumpFeatures ;




static gboolean printLine(DumpFeatures dump, gchar *line) ;
static gboolean printFeatureContext(ZMapFeatureContext feature_context, DumpFeatures feature_dump) ;
static void printFeatureAlignment(GQuark key_id, gpointer data, gpointer user_data) ;
static void printFeatureBlock(gpointer data, gpointer user_data) ;
static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void printFeature(GQuark key_id, gpointer data, gpointer user_data) ;

static gint findStyle(gconstpointer list_data, gconstpointer user_data) ;
static gint findStyleName(gconstpointer list_data, gconstpointer user_data) ;
static void addTypeQuark(gpointer data, gpointer user_data) ;





static void doAlignment(GQuark key_id, gpointer data, gpointer user_data) ;
static void doBlock(gpointer data, gpointer user_data) ;
static void doFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void doFeature(GQuark key_id, gpointer data, gpointer user_data) ;

static int sortGapsByTarget(gconstpointer a, gconstpointer b) ;


/*!
 * Function to do some validity checking on a ZMapFeatureAny struct. Always more you
 * could do but this is better than nothing.
 * 
 * Returns TRUE if the struct is OK, FALSE otherwise.
 * 
 * @param   any_feature    The feature to validate.
 * @return  gboolean       TRUE if feature is valid, FALSE otherwise.
 *  */
gboolean zMapFeatureIsValid(ZMapFeatureAny any_feature)
{
  gboolean result = FALSE ;

  if (any_feature
      && zMapFeatureTypeIsValid(any_feature->struct_type)
      && any_feature->unique_id != ZMAPFEATURE_NULLQUARK
      && any_feature->original_id != ZMAPFEATURE_NULLQUARK)
    result = TRUE ;

  return result ;
}

gboolean zMapFeatureTypeIsValid(ZMapFeatureStructType group_type)
{
  gboolean result = FALSE ;

  if (group_type >= ZMAPFEATURE_STRUCT_CONTEXT
      && group_type <= ZMAPFEATURE_STRUCT_FEATURE)
    result = TRUE ;

  return result ;
}



/*!
 * Returns the original name of any feature type. The returned string belongs
 * to the feature and must _NOT_ be free'd. This function can never return
 * NULL as all features must have valid names.
 * 
 * @param   any_feature    The feature.
 * @return  char *         The name of the feature.
 *  */
char *zMapFeatureName(ZMapFeatureAny any_feature)
{
  char *feature_name = NULL ;

  zMapAssert(zMapFeatureIsValid(any_feature)) ;

  feature_name = (char *)g_quark_to_string(any_feature->original_id) ;

  return feature_name ;
}


/*!
 * Function to return the _parent_ group of group_type of the feature any_feature.
 * This is a generalised function to stop all the poking about through the context
 * hierachy that is otherwise required. Note you can only go _UP_ the tree with
 * this function because going down is a one-to-many mapping.
 * 
 * Returns the feature group or NULL if there is no parent group or there is some problem
 * with the arguments like asking for a group at or below the level of any_feature.
 * 
 * @param   any_feature    The feature for which you wish to find the parent group.
 * @param   group_type     The type/level of the parent group you want to find.
 * @return  ZMapFeatureAny The parent group or NULL.
 *  */
ZMapFeatureAny zMapFeatureGetParentGroup(ZMapFeatureAny any_feature, ZMapFeatureStructType group_type)
{
  ZMapFeatureAny result = NULL ;

  zMapAssert(zMapFeatureIsValid(any_feature)
	     && group_type >= ZMAPFEATURE_STRUCT_CONTEXT
	     && group_type <= ZMAPFEATURE_STRUCT_FEATURE) ;

  if (any_feature->struct_type >= group_type)
    {
      ZMapFeatureAny group = any_feature ;

      while (group && group->struct_type > group_type)
	{
	  group = group->parent ;
	}

      result = group ;
    }

  return result ;
}

/* Generalised dumping function, caller supplies a callback function that does the actual
 * output and a pointer to somewhere in the feature hierachy (alignment, block, set etc)
 * and this code calls the callback to do the output of the feature in the appropriate
 * form. */
gboolean zMapFeatureDumpFeatures(GIOChannel *file, ZMapFeatureAny dump_set,
				 ZMapFeatureDumpFeatureCallbackFunc dump_func,
				 gpointer user_data,
				 GError **error_out)
{
  gboolean result = FALSE ;
  NewDumpFeaturesStruct dump_data ;
  GDataForeachFunc dataset_start_func = NULL ;
  GData **dataset = NULL ;
  GFunc list_start_func = NULL ;
  GList *list = NULL ;
  ZMapFeature feature = NULL ;


  zMapAssert(file && dump_set && dump_func && error_out) ;
  zMapAssert(dump_set->struct_type == ZMAPFEATURE_STRUCT_CONTEXT
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_ALIGN
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_BLOCK
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET
	     || dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;


  dump_data.status = TRUE ;
  dump_data.file = file ;
  dump_data.error_out = error_out ;
  dump_data.dump_func = dump_func ;
  dump_data.user_data = user_data ;

  /* NOTES: I cocked up in that the blocks are a GList, everything else is a GData, that needs
   * changing then GData could become part of the structAny struct to allow more generalised
   * handling....
   * Also, this is simple minded in that it just ends up dumping features, not any other data
   * about the context.... */
  switch(dump_set->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      dataset_start_func = doAlignment ;
      dataset = &(((ZMapFeatureContext)dump_set)->alignments) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      dataset_start_func = doFeatureSet ;
      dataset = &(((ZMapFeatureBlock)dump_set)->feature_sets) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      dataset_start_func = doFeature ;
      dataset = &(((ZMapFeatureSet)dump_set)->features) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      list_start_func = doBlock ;
      list = ((ZMapFeatureAlignment)dump_set)->blocks ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURE:			    /* pathological case... */
      feature = (ZMapFeature)dump_set ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  /* Call the appropriate function.... */
  if (dataset_start_func)
    g_datalist_foreach(dataset, dataset_start_func, &dump_data) ;
  else if (list_start_func)
    g_list_foreach(list, list_start_func, &dump_data) ;
  else
    doFeature(feature->unique_id, feature, &dump_data) ;

  result = dump_data.status ;


  return result ;
}


/* Given a feature name produce the canonicalised name as used that is used in producing
 * unique feature names.
 * 
 * NOTE that the name is canonicalised in place so caller must provide a string for this
 * to be done in.
 *  */
char *zMapFeatureCanonName(char *feature_name)
{
  char *ptr ;
  int len ;

  zMapAssert(feature_name && *feature_name) ;

  len = strlen(feature_name);

  /* lower case the feature name, only the feature part though,
   * numbers don't matter. Here we do as g_strdown does, but in place
   * rather than a g_strdup first. */
  for(ptr = feature_name; ptr <= feature_name + len; ptr++)
    {
      *ptr = g_ascii_tolower(*ptr);
    }

  return feature_name ;
}



/* This function creates a unique id for a feature. This is essential if we are to use the
 * g_datalist package to hold and reference features. Code should _ALWAYS_ use this function
 * to produce these IDs.
 * Caller must g_free() returned string when finished with. */
char *zMapFeatureCreateName(ZMapFeatureType feature_type, char *feature,
			    ZMapStrand strand, int start, int end, int query_start, int query_end)
{
  char *feature_name = NULL, *ptr ;
  int len;

  zMapAssert(feature_type && feature) ;

  /* Get the length of the feature (saving time??) for later */
  len = strlen(feature);

  if (feature_type == ZMAPFEATURE_ALIGNMENT)
    feature_name = g_strdup_printf("%s_%d.%d_%d.%d", feature,
				   start, end, query_start, query_end) ;
  else
    feature_name = g_strdup_printf("%s_%d.%d", feature, start, end) ;

  /* lower case the feature name, only the feature part though,
   * numbers don't matter. Here we do as g_strdown does, but in place
   * rather than a g_strdup first. */
  for(ptr = feature_name; ptr <= feature_name + len; ptr++)
    {
      *ptr = g_ascii_tolower(*ptr);
    }

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

GQuark zMapFeatureSetCreateID(char *set_name)
{
  return zMapStyleCreateID(set_name);
}

/* Free return when finished! */
char *zMapFeatureMakeDNAFeatureName(ZMapFeatureBlock block)
{
  char *dna_name = NULL;

  dna_name = g_strdup_printf("%s (%s)", "DNA", g_quark_to_string(block->original_id));

  return dna_name;
}


void zMapFeatureSortGaps(GArray *gaps)
{
  zMapAssert(gaps) ;

  /* Sort the array of gaps. performance hit? */
  g_array_sort(gaps, sortGapsByTarget);

  return ;
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


ZMapFeatureTypeStyle zMapFeatureGetStyle(ZMapFeature feature)
{
  ZMapFeatureTypeStyle style ;

  style = feature->style ;

  return style ;
}


char *zMapFeatureSetGetName(ZMapFeatureSet feature_set)
{
  char *set_name ;

  set_name = (char *)g_quark_to_string(feature_set->original_id) ;

  return set_name ;
}



/* Retrieve a style struct for the given style id. */
ZMapFeatureTypeStyle zMapFindStyle(GList *styles, GQuark style_id)
{
  ZMapFeatureTypeStyle style = NULL ;
  GList *list ;

  if ((list = g_list_find_custom(styles, GUINT_TO_POINTER(style_id), findStyle)))
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



/* Retrieve a Glist of the names of all the styles... */
GList *zMapStylesGetNames(GList *styles)
{
  GList *quark_list = NULL ;

  zMapAssert(styles) ;

  g_list_foreach(styles, addTypeQuark, (void *)&quark_list) ;

  return quark_list ;
}

/* GFunc() callback function, appends style names to a string, its called for lists
 * of either style name GQuarks or lists of style structs. */
static void addTypeQuark(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  GList **quarks_out = (GList **)user_data ;
  GList *quark_list = *quarks_out ;

  quark_list = g_list_append(quark_list, GUINT_TO_POINTER(style->unique_id)) ;

  *quarks_out = quark_list ;

  return ;
}

/* Dump out a feature context. */
gboolean zMapFeatureContextDump(GIOChannel *file,
				ZMapFeatureContext feature_context, GError **error_out)
{
  gboolean result = FALSE ;
  DumpFeaturesStruct dump_features ;

  zMapAssert(file && feature_context && error_out) ;
  
  dump_features.status = TRUE ;
  dump_features.channel = file ;
  dump_features.error_out = error_out ;

  result = printFeatureContext(feature_context, &dump_features) ;

  return result ;
}

/* For blocks within alignments other than the master alignment, it is not possible to simply
 * use the x1,x2 positions in the feature struct as these are the positions in the original
 * feature. We need to know the coordinates in the master alignment. */
void zMapFeature2MasterCoords(ZMapFeature feature, double *feature_x1, double *feature_x2)
{
  double master_x1 = 0.0, master_x2 = 0.0 ;
  ZMapFeatureBlock block ;
  double feature_offset = 0.0 ;

  zMapAssert(feature->parent && feature->parent->parent) ;

  block = (ZMapFeatureBlock)feature->parent->parent ;

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

  if (feature->type == ZMAPFEATURE_ALIGNMENT)
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


gboolean zMapFeatureBlockThreeFrameTranslation(ZMapFeatureBlock block, ZMapFeatureSet *set_out)
{
  ZMapFeatureSet feature_set = NULL;
  ZMapFeatureTypeStyle style = NULL;
  ZMapFeatureContext context = NULL;
  GQuark style_id = 0, feature_set_id = 0;
  gboolean still_good = FALSE, created = FALSE;

  feature_set_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_3FT_NAME);
  style_id       = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME);

  if(block->sequence.length)
    still_good = TRUE;

  if(!(context = (ZMapFeatureContext)(zMapFeatureGetParentGroup((ZMapFeatureAny)block, 
                                                                ZMAPFEATURE_STRUCT_CONTEXT))))
    still_good = FALSE;

  if((feature_set = (ZMapFeatureSet)(g_datalist_id_get_data(&(block->feature_sets), feature_set_id))))
    still_good = FALSE;         /* We've already got one */

  if(still_good &&
     (style = zMapFindStyle(context->styles, style_id)) != NULL)
    {
      feature_set = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_3FT_NAME, NULL);
      feature_set->style = style;
      created = TRUE;
    }

  if(still_good && feature_set)
    {
      int i, frame_offset;
      char *seq = NULL, *f_name = NULL, *s_name;
      ZMapFeature threeft = NULL; 
      ZMapPeptide pep = NULL;

      s_name = (char *)g_quark_to_string(block->original_id);

      seq = block->sequence.sequence ;

      frame_offset = 0 - ZMAPFRAME_0; /* set this up ! */

      for(i = ZMAPFRAME_0; seq && *seq && i <= ZMAPFRAME_2; i++, seq++)
        {
          threeft = zMapFeatureCreateEmpty();

          f_name = g_strdup_printf("%s_frame_%d",
				   g_quark_to_string(feature_set_id),
				   i);

          pep = zMapPeptideCreateSafely(NULL, NULL, seq, NULL, FALSE);
          
          threeft->text = g_strdup(zMapPeptideSequence(pep));
          
          zMapFeatureAddStandardData(threeft, f_name, f_name,
                                     s_name, "sequence",
                                     ZMAPFEATURE_PEP_SEQUENCE, style,
                                     i, zMapPeptideLength(pep) * 3 + i,
                                     FALSE, 0.0,
                                     ZMAPSTRAND_NONE, ZMAPPHASE_NONE);

          zMapFeatureSetAddFeature(feature_set, threeft);
        }
    }

  if(set_out)
    *set_out = feature_set;

  return created;
}



/* 
 *              Internal routines.
 */


/* Should have a prefix level ?? */
static gboolean printLine(DumpFeatures dump, gchar *line)
{
  gboolean status = TRUE ;
  gsize bytes_written ;

  if (g_io_channel_write_chars(dump->channel,
			       line, -1, &bytes_written, dump->error_out) != G_IO_STATUS_NORMAL)
    {
      dump->status = status = FALSE ;
    }

  return status ;
}


static gboolean printFeatureContext(ZMapFeatureContext feature_context, DumpFeatures dump)
{
  gboolean result = FALSE ;
  char *line ;

  line = g_strdup_printf("Feature Context:\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\n", 
			 g_quark_to_string(feature_context->unique_id),
			 g_quark_to_string(feature_context->original_id),
			 g_quark_to_string(feature_context->sequence_name),
			 g_quark_to_string(feature_context->parent_name),
			 feature_context->parent_span.x1,
			 feature_context->parent_span.x2,
			 feature_context->sequence_to_parent.p1,
			 feature_context->sequence_to_parent.p2,
			 feature_context->sequence_to_parent.c1,
			 feature_context->sequence_to_parent.c2) ;

  if ((result = printLine(dump, line)))
    {
      g_datalist_foreach(&(feature_context->alignments), printFeatureAlignment, dump) ;
    }

  g_free(line) ;

  return result ;
}


/* NOTE in all the below callback routines that there is no way to stop the callback being
 * called if an error has occurred so we just have to take no action. */

static void printFeatureAlignment(GQuark key_id, gpointer data, gpointer user_data)
{
  DumpFeatures dump = (DumpFeatures)user_data ;

  if (dump->status)
    {
      ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
      char *line ;

      line = g_strdup_printf("\tAlignment:\t%s\n", g_quark_to_string(alignment->unique_id)) ;

      /* Only proceed if there's no problem printing the line */
      if (printLine(dump, line))
	g_list_foreach(alignment->blocks, printFeatureBlock, (gpointer)dump) ;
      
      g_free(line) ;
    }

  return ;
}



static void printFeatureBlock(gpointer data, gpointer user_data)
{
  DumpFeatures dump = (DumpFeatures)user_data ;

  if (dump->status)
    {
      ZMapFeatureBlock block = (ZMapFeatureBlock)data ;
      char *line ;

      line = g_strdup_printf("\tBlock:\t%s\t%d\t%d\t%d\t%d\n", g_quark_to_string(block->unique_id),
			     block->block_to_sequence.t1,
			     block->block_to_sequence.t2,
			     block->block_to_sequence.q1,
			     block->block_to_sequence.q2) ;

      if (printLine(dump, line))
	g_datalist_foreach(&(block->feature_sets), printFeatureSet, dump) ;

      g_free(line) ;
    }

  return ;
}


static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  DumpFeatures dump = (DumpFeatures)user_data ;

  /* Once we have failed then we stop printing, note that there is no way to stop this routine
   * being called which is a shame.... */
  if (dump->status)
    {
      ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
      char *line ;

      line = g_strdup_printf("\tFeature Set:\t%s\t%s\n",
			     g_quark_to_string(feature_set->unique_id),
			     (char *)g_quark_to_string(feature_set->original_id)) ;

      /* Only proceed if there's no problem printing the line */
      if (printLine(dump, line))
	g_datalist_foreach(&(feature_set->features), printFeature, dump) ;
    
      g_free(line) ;
    }

  return ;
}


static void printFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  DumpFeatures dump = (DumpFeatures)user_data ;

  if (dump->status)
    {
      ZMapFeature feature = (ZMapFeature)data ;
      char *type, *strand, *phase ;
      GString *line ;

      line = g_string_sized_new(1000) ;
      type   = zMapFeatureType2Str(feature->type) ;
      strand = zMapFeatureStrand2Str(feature->strand) ;
      phase  = zMapFeaturePhase2Str(feature->phase) ;
  
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
	  g_string_append(line, feature->text) ;
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

      g_string_append_c(line, '\n') ;

      printLine(dump, line->str) ;

      g_string_free(line, TRUE) ;
    }

  return ;
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




/* 
 *             Set of callbacks for processing features in a context.
 * 
 * These functions are simply skeletons that plough down the context until they
 * reach the features level when they call the supplied callback routine to do
 * the processing.
 * 
 */

static void doAlignment(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  NewDumpFeatures dump_data = (NewDumpFeatures)user_data ;


  /* Do all the blocks within the alignment. */
  if (dump_data->status)
    g_list_foreach(alignment->blocks, doBlock, dump_data) ;


  return ;
}

static void doBlock(gpointer data, gpointer user_data)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)data ;
  NewDumpFeatures dump_data = (NewDumpFeatures)user_data ;

  if (dump_data->status)
    g_datalist_foreach(&(block->feature_sets), doFeatureSet, dump_data) ;


  return ;
}


static void doFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  NewDumpFeatures dump_data = (NewDumpFeatures)user_data ;

  if (dump_data->status)
    g_datalist_foreach(&(feature_set->features), doFeature, dump_data) ;


  return ;
}

static void doFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ; 
  NewDumpFeatures dump_data = (NewDumpFeatures)user_data ;
  char *parent_name ;
#ifdef RDS_DONT_INCLUDE
  char *feature_name ;
  char *source_name ;
  int start, end ;
#endif
  parent_name = (char *)g_quark_to_string(feature->parent->parent->parent->original_id) ;

  if (dump_data->status)
    dump_data->status = (*(dump_data->dump_func))(dump_data->file,
						  dump_data->user_data,
						  feature->style,
						  parent_name,
						  (char *)g_quark_to_string(feature->original_id),
						  (char *)g_quark_to_string(feature->style->original_id),
						  (char *)g_quark_to_string(feature->ontology),
						  feature->x1, feature->x2,
						  feature->flags.has_score,
						  feature->score,
						  feature->strand,
						  feature->phase,
						  feature->type,
						  (gpointer)&(feature->feature),
						  dump_data->error_out) ;

  return ;
}




/* *************************************************************
 * Not entirely sure of the wisdom of this (mainly performance
 * concerns), but everywhere else we have start < end!.  previously
 * loadGaps didn't even fill in the strand or apply the start < end 
 * idiom and gaps array required a test when iterating through 
 * it (the GArray). The GArray will now be ordered as it almost
 * certainly should be to fit with the start < end idiom.  RDS 
 */
static int sortGapsByTarget(gconstpointer a, gconstpointer b)
{
  ZMapAlignBlock alignA = (ZMapAlignBlock)a, 
    alignB = (ZMapAlignBlock)b;
  return (alignA->t1 == alignB->t1 ? 0 : (alignA->t1 > alignB->t1 ? 1 : -1));
}

