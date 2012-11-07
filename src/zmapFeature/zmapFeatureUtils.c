/*  File: zmapFeature.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Utility routines for handling features/sets/blocks etc.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <unistd.h>

#include <zmapFeature_P.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>


typedef struct
{
  ZMapMapBlock map;
  int limit_start;
  int limit_end;
  int counter;
} SimpleParent2ChildDataStruct, *SimpleParent2ChildData;

static ZMapFrame feature_frame(ZMapFeature feature, int start_coord);
static void get_feature_list_extent(gpointer list_data, gpointer span_data);

static gint findStyleName(gconstpointer list_data, gconstpointer user_data) ;
static void addTypeQuark(gpointer style_id, gpointer data, gpointer user_data) ;

static void map_parent2child(gpointer exon_data, gpointer user_data);
static int sortGapsByTarget(gconstpointer a, gconstpointer b) ;

static int findExon(ZMapFeature feature, int exon_start, int exon_end) ;
static gboolean calcExonPhase(ZMapFeature feature, int exon_index,
			      int *exon_cds_start, int *exon_cds_end, int *phase_out) ;

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
    {
      switch (any_feature->struct_type)
	{
	case ZMAPFEATURE_STRUCT_CONTEXT:
	case ZMAPFEATURE_STRUCT_ALIGN:
	case ZMAPFEATURE_STRUCT_BLOCK:
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  result = TRUE ;
	  break ;
	case ZMAPFEATURE_STRUCT_FEATURE:
	  if (any_feature->children == NULL)
	    result = TRUE ;
	  break ;
	default:
	  zMapAssertNotReached() ;
	}
    }

  return result ;
}
#ifdef NOT_YET
static int get_feature_allowed_types(ZMapStyleMode mode)
{
  int allowed = 0;

  switch(mode)
    {
    case ZMAPSTYLE_MODE_RAW_TEXT:
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
      allowed = (ZMAPFEATURE_TYPE_BASIC | ZMAPFEATURE_TYPE_TEXT);
      break;
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      allowed = (ZMAPFEATURE_TYPE_BASIC | ZMAPFEATURE_TYPE_TRANSCRIPT);
      break;
    case ZMAPSTYLE_MODE_ALIGNMENT:
      allowed = (ZMAPFEATURE_TYPE_BASIC | ZMAPFEATURE_TYPE_ALIGNMENT);
      break;
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_GLYPH:
      allowed = (ZMAPFEATURE_TYPE_BASIC);
      break;
    case ZMAPSTYLE_MODE_BASIC:
      allowed = (ZMAPFEATURE_TYPE_BASIC);
      break;
    case ZMAPSTYLE_MODE_META:
    case ZMAPSTYLE_MODE_INVALID:
    default:
      break;
    }

  return allowed;
}

gboolean zMapFeatureIsDrawable(ZMapFeatureAny any_feature)
{
  gboolean result = FALSE;

  switch(any_feature->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature feature;
	int allowed = ZMAPFEATURE_TYPE_INVALID;

	feature = (ZMapFeature)any_feature;

	if(*feature->style)
	  allowed = get_feature_allowed_types();

	if(allowed & feature->type)
	  result = TRUE;
      }
      break;
    default:
      break;
    }

  return result;
}
#endif /* NOT_YET */


/*!
 * Function to do some validity checking on a ZMapFeatureAny struct that in addition
 * checks to see if it is of the requested type.
 *
 * Returns TRUE if the struct is OK, FALSE otherwise.
 *
 * @param   any_feature    The feature to validate.
 * @param   type           The type that the feature must be.
 * @return  gboolean       TRUE if feature is valid, FALSE otherwise.
 *  */
gboolean zMapFeatureIsValidFull(ZMapFeatureAny any_feature, ZMapFeatureStructType type)
{
  gboolean result = FALSE ;

  if (zMapFeatureIsValid(any_feature) && any_feature->struct_type == type)
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

gboolean zMapFeatureIsSane(ZMapFeature feature, char **insanity_explained)
{
  gboolean sane = TRUE ;
  char *insanity = NULL ;

  if (sane)
    {
      if (feature->type <= ZMAPSTYLE_MODE_INVALID ||
	  feature->type >  ZMAPSTYLE_MODE_META) /* Keep in step with zmapStyle.h */
        {
          insanity = g_strdup_printf("Feature '%s' [%s] has invalid type.", /* keep in step with zmapStyle.h */
                                     (char *)g_quark_to_string(feature->original_id),
                                     (char *)g_quark_to_string(feature->unique_id));
          sane = FALSE;
        }
    }

  if (sane)
    {
      if (feature->x1 > feature->x2)
        {
          insanity = g_strdup_printf("Feature '%s' [%s] has start > end.",
                                     (char *)g_quark_to_string(feature->original_id),
                                     (char *)g_quark_to_string(feature->unique_id));
          sane = FALSE;
        }
    }

  if(sane)
    {
      switch(feature->type)
        {
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          {
            GArray *array;
            ZMapSpan span;
            int i = 0;

            if(sane && (array = feature->feature.transcript.exons))
              {
                for(i = 0; sane && i < array->len; i++)
                  {
                    span = &(g_array_index(array, ZMapSpanStruct, i));
                    if(span->x1 > span->x2)
                      {
                        insanity = g_strdup_printf("Exon %d in feature '%s' has start > end.",
                                                   i + 1,
                                                   (char *)g_quark_to_string(feature->original_id));
                        sane = FALSE;
                      }
                  }
              }

            if(sane && (array = feature->feature.transcript.introns))
              {
                for(i = 0; sane && i < array->len; i++)
                  {
                    span = &(g_array_index(array, ZMapSpanStruct, i));
                    if(span->x1 > span->x2)
                      {
                        insanity = g_strdup_printf("Intron %d in feature '%s' has start > end.",
                                                   i + 1,
                                                   (char *)g_quark_to_string(feature->original_id));
                        sane = FALSE;
                      }
                  }
              }
            if(sane && feature->feature.transcript.flags.cds)
              {
                if(feature->feature.transcript.cds_start > feature->feature.transcript.cds_end)
                  {
                    insanity = g_strdup_printf("CDS for feature '%s' has start > end.",
                                               (char *)g_quark_to_string(feature->original_id));
                    sane = FALSE;
                  }
              }
          }
          break;

        case ZMAPSTYLE_MODE_ALIGNMENT:
        case ZMAPSTYLE_MODE_BASIC:
        case ZMAPSTYLE_MODE_SEQUENCE:
	case ZMAPSTYLE_MODE_TEXT:
	case ZMAPSTYLE_MODE_GRAPH:
	case ZMAPSTYLE_MODE_GLYPH:
          zMapLogWarning("%s", "This part of zMapFeatureIsSane() needs writing!");
          break;
	case ZMAPSTYLE_MODE_META:
	  break;
        default:
          zMapAssertNotReached();
          break;
        }
    }

  if(insanity_explained)
    *insanity_explained = insanity;

  return sane;
}


/* we might get off with insanity. */
gboolean zMapFeatureAnyIsSane(ZMapFeatureAny feature, char **insanity_explained)
{
  gboolean sane = TRUE ;
  char *insanity = NULL ;

  if (sane && !zMapFeatureIsValid(feature))
    {
      if (feature->original_id == ZMAPFEATURE_NULLQUARK)
        insanity = "Feature has bad name.";
      else if (feature->unique_id == ZMAPFEATURE_NULLQUARK)
        insanity = "Feature has bad identifier.";
      else
	insanity = g_strdup_printf("Feature '%s' [%s] has bad type.",
				   (char *)g_quark_to_string(feature->original_id),
				   (char *)g_quark_to_string(feature->unique_id));

      sane = FALSE ;
    }

  if (sane)
    {
      switch(feature->struct_type)
        {
        case ZMAPFEATURE_STRUCT_FEATURE:
          {
            ZMapFeature real_feature = (ZMapFeature)feature;

            sane = zMapFeatureIsSane(real_feature, &insanity);

	    break;
          }
        case ZMAPFEATURE_STRUCT_CONTEXT:
          zMapLogWarning("%s", "This part of zMapFeatureAnyIsSane() needs writing!");
          break;
        case ZMAPFEATURE_STRUCT_ALIGN:
        case ZMAPFEATURE_STRUCT_BLOCK:
        case ZMAPFEATURE_STRUCT_FEATURESET:
          /* Nothing to check beyond ZMapFeatureAny basics */
          break;
        default:
          zMapAssertNotReached();
          break;
        }
    }

  if (insanity)
    {
      if (insanity_explained)
	*insanity_explained = g_strdup(insanity);

      g_free(insanity);
    }

  return sane;
}




void zMapFeatureRevComp(int seq_start, int seq_end, int *coord_1, int *coord_2)
{
  zmapFeatureRevComp(Coord, seq_start, seq_end, *coord_1, *coord_2) ;

  return ;
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
 * Does a case <i>insensitive</i> comparison of the features name and
 * the supplied name, return TRUE if they are the same.
 *
 * @param   any_feature    The feature.
 * @param   name           The name to be compared..
 * @return  gboolean       TRUE if the names are the same.
 *  */
gboolean zMapFeatureNameCompare(ZMapFeatureAny any_feature, char *name)
{
  gboolean result = FALSE ;

  zMapAssert(zMapFeatureIsValid(any_feature) && name && *name) ;

  if (g_ascii_strcasecmp(g_quark_to_string(any_feature->original_id), name) == 0)
    result = TRUE ;

  return result ;
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
 * g_hash_table package to hold and reference features. Code should _ALWAYS_ use this function
 * to produce these IDs.
 * 
 * Caller must g_free() returned string when finished with.
 * 
 * OK, I THINK THERE IS A PROBLEM HERE...WE COULD HAVE FEATURES WITH SAME COORDS ON
 * OPPOSITE STRAND WHILE AN ANNOTATOR IS EDITING FEATURES....DOH.....
 * 
 */
char *zMapFeatureCreateName(ZMapStyleMode feature_type,
			    char *feature,
			    ZMapStrand strand, int start, int end, int query_start, int query_end)
{
  char *feature_unique_name = NULL ;
  char *strand_str, *ptr ;
  int len ;

  /* Turn this into a if() and return null if things not supplied.... */
  zMapAssert(feature_type && feature) ;

  strand_str = zMapFeatureStrand2Str(strand) ;

  if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    feature_unique_name = g_strdup_printf("%s_'%s'_%d.%d_%d.%d", feature,
					  strand_str, start, end, query_start, query_end) ;
  else
    feature_unique_name = g_strdup_printf("%s_'%s'_%d.%d", feature, strand_str, start, end) ;

  /* lower case the feature name, only the feature part though,
   * numbers don't matter. Here we do as g_strdown does, but in place
   * rather than a g_strdup first. */
  len = strlen(feature) ;
  for (ptr = feature_unique_name ; ptr <= feature_unique_name + len ; ptr++)
    {
      *ptr = g_ascii_tolower(*ptr) ;
    }

  return feature_unique_name ;
}


/* Like zMapFeatureCreateName() but returns a quark representing the feature name. */
GQuark zMapFeatureCreateID(ZMapStyleMode feature_type,
			   char *feature,
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

gboolean zMapFeatureBlockDecodeID(GQuark id,
                                  int *ref_start, int *ref_end, ZMapStrand *ref_strand,
                                  int *non_start, int *non_end, ZMapStrand *non_strand)
{
  gboolean valid = FALSE ;
  char *block_id ;
  char *format_str = "%d.%d.%1c_%d.%d.%1c" ;
  char ref_strand_str[2] = {'\0'}, non_strand_str[2] = {'\0'} ;
  int fields ;
  enum {EXPECTED_FIELDS = 6} ;

  block_id = (char *)g_quark_to_string(id) ;

  if ((fields = sscanf(block_id, format_str,
		       ref_start, ref_end, &ref_strand_str[0],
		       non_start, non_end, &non_strand_str[0])) != EXPECTED_FIELDS)
    {
      *ref_start = 0 ;
      *ref_end   = 0 ;
      *non_start = 0 ;
      *non_end   = 0 ;
    }
  else
    {
      zMapFeatureFormatStrand(&ref_strand_str[0], ref_strand) ;
      zMapFeatureFormatStrand(&non_strand_str[0], non_strand) ;
      valid = TRUE ;
    }

  return valid ;
}

GQuark zMapFeatureSetCreateID(char *set_name)
{
  return zMapStyleCreateID(set_name);
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


char *zMapFeatureSetGetName(ZMapFeatureSet feature_set)
{
  char *set_name ;

  set_name = (char *)g_quark_to_string(feature_set->original_id) ;

  return set_name ;
}



/* Retrieve a style struct for the given style id. */
ZMapFeatureTypeStyle zMapFindStyle(GHashTable *styles, GQuark style_id)
{
  ZMapFeatureTypeStyle style = NULL ;

  style = (ZMapFeatureTypeStyle) g_hash_table_lookup(styles, GUINT_TO_POINTER(style_id));

  return style ;
}


ZMapFeatureTypeStyle zMapFindFeatureStyle(GHashTable *styles, GQuark style_id, ZMapStyleMode feature_type)
{
  ZMapFeatureTypeStyle feature_style = NULL ;
  char *type;

  if(!(feature_style = zMapFindStyle(styles, style_id)))
  {
	/* feature_style_id is as configured or defaults to the same name as the featureset
	   * if not defined try a style with the same name as the feature type
	   */

	   type = (char *) zMapStyleMode2ExactStr(feature_type);
	   style_id = zMapStyleCreateID(type);
	   feature_style = zMapFindStyle(styles, style_id);
  }
  return feature_style;
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
GList *zMapStylesGetNames(GHashTable *styles)
{
  GList *quark_list = NULL ;

  zMapAssert(styles) ;

  g_hash_table_foreach(styles, addTypeQuark, (void *)&quark_list) ;

  return quark_list ;
}

/* GFunc() callback function, appends style names to a string, its called for lists
 * of either style name GQuarks or lists of style structs. */
static void addTypeQuark(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  GList **quarks_out = (GList **)user_data ;
  GList *quark_list = *quarks_out ;

  quark_list = g_list_append(quark_list, GUINT_TO_POINTER(zMapStyleGetUniqueID(style)));

  *quarks_out = quark_list ;

  return ;
}


/* from column_id return whether if is configured from seq-data= featuresets (coverage side) */
gboolean zMapFeatureIsCoverageColumn(ZMapFeatureContextMap map,GQuark column_id)
{
	ZMapFeatureSource src;
      GList *fsets;

      fsets = zMapFeatureGetColumnFeatureSets(map, column_id, TRUE);

	for (; fsets ; fsets = fsets->next)
	{
		src = g_hash_table_lookup(map->source_2_sourcedata,fsets->data);
		if(src && src->related_column)
			return TRUE;
	}

	return FALSE;
}

/* from column_id return whether it is configured from seq-data= featuresets (data side) */
gboolean zMapFeatureIsSeqColumn(ZMapFeatureContextMap map,GQuark column_id)
{
	ZMapFeatureSource src;
      GList *fsets;

      fsets = zMapFeatureGetColumnFeatureSets(map, column_id, TRUE);

	for (; fsets ; fsets = fsets->next)
	{
		src = g_hash_table_lookup(map->source_2_sourcedata,fsets->data);
		if(src && src->is_seq)
			return TRUE;
	}

	return FALSE;
}

gboolean zMapFeatureIsSeqFeatureSet(ZMapFeatureContextMap map,GQuark fset_id)
{
	ZMapFeatureSource src = g_hash_table_lookup(map->source_2_sourcedata,GUINT_TO_POINTER(fset_id));
//zMapLogWarning("feature is_seq: %s -> %p\n",g_quark_to_string(fset_id),src);

	if(src && src->is_seq)
		return TRUE;
	return FALSE;

}


/*
 *	for new columns that appear out of nowhere put them on the right
 *	this does not get reset on a new view, but with 100 columns it will take a very long time to wrap round
 */
int zMapFeatureColumnOrderNext(void)
{
	static int which = 0;
	return ++which;		/* 0 is invalid */
}



/* get the column struct for a featureset */
ZMapFeatureColumn zMapFeatureGetSetColumn(ZMapFeatureContextMap map,GQuark set_id)
{
      ZMapFeatureColumn column = NULL;
      ZMapFeatureSetDesc gff;

      char *name = (char *) g_quark_to_string(set_id);

	if(!map->featureset_2_column)
	{
		/* so that we can use autoconfigured servers */
		map->featureset_2_column = g_hash_table_new(NULL,NULL);
	}

      /* get the column the featureset goes in */
      gff = g_hash_table_lookup(map->featureset_2_column,GUINT_TO_POINTER(set_id));
      if(!gff)
      {
//            zMapLogWarning("creating featureset_2_column for %s",name);
            /* recover from un-configured error
             * NOTE this occurs for seperator features eg DNA search
             * the style is predefined but the featureset and column are created
             * blindly with no reference to config
             * NOTE ideally these should be done along with getAllPredefined() styles
             */

             /* instant fix for a bug: DNA search fails to display seperator features */
             gff = g_new0(ZMapFeatureSetDescStruct,1);
             gff->column_id =
             gff->column_ID =
             gff->feature_src_ID = set_id;
             gff->feature_set_text = name;
             g_hash_table_insert(map->featureset_2_column,GUINT_TO_POINTER(set_id),gff);
      }
/*      else*/
      {
            column = g_hash_table_lookup(map->columns,GUINT_TO_POINTER(gff->column_id));
            if(!column)
            {
	            ZMapFeatureSource gff_source;

//                  zMapLogWarning("creating column  %s for featureset %s (%s)", g_quark_to_string(gff->column_id), g_quark_to_string(set_id), g_quark_to_string(gff->column_ID));

                  column = g_new0(ZMapFeatureColumnStruct,1);

// don-t set this from featureset data it's column specific style from config only
//                  column->style_id =
                  column->unique_id =
                  column->column_id = set_id;

			column->order = zMapFeatureColumnOrderNext();

                  gff_source = g_hash_table_lookup(map->source_2_sourcedata,GUINT_TO_POINTER(set_id));
// don-t set this from featureset data it's column specific style from config only
//			if(gff_source)
//				column->style_id = gff_source->style_id;
                  column->column_desc = name;

                  column->featuresets_unique_ids = g_list_append(column->featuresets_unique_ids,GUINT_TO_POINTER(set_id));
//printf("window adding %s to column %s\n", g_quark_to_string(GPOINTER_TO_UINT(set_id)), g_quark_to_string(column->unique_id));

                  g_hash_table_insert(map->columns,GUINT_TO_POINTER(set_id),column);
            }
      }
      return column;
}




GList *zMapFeatureGetColumnFeatureSets(ZMapFeatureContextMap map,GQuark column_id, gboolean unique_id)
{
      GList *list = NULL;
      ZMapFeatureSetDesc fset;
      ZMapFeatureColumn column;
      gpointer key;
      GList *iter;

	/*
	This is hopelessly inefficient if we do this for every featureset, as ext_curated has about 1000
	so we cache the list when we first create it.
	can't always do it on startup as acedb provides the mapping later on

	NOTE see zmapWindowColConfig.c/column_is_loaded_in_range() for a comment about static or dynamic lists
	also need to scan for all calls to this func since caching the data
	*/

      column = g_hash_table_lookup(map->columns,GUINT_TO_POINTER(column_id));

//      zMapAssert(column);	// would crash in a mis-config
	if(!column)
		return list;

	if(unique_id)
     	{
     		if(column->featuresets_unique_ids)
           	 	list = column->featuresets_unique_ids;
      }
      else
     	{
     		if(column->featuresets_names)
           	 	list = column->featuresets_names;
      }

	if(!list)
	{
		zMap_g_hash_table_iter_init(&iter,map->featureset_2_column);
		while(zMap_g_hash_table_iter_next(&iter,&key,(gpointer) &fset))
		{
			if(fset->column_id == column_id)
				list = g_list_prepend(list,unique_id ? key : GUINT_TO_POINTER(fset->feature_src_ID));
		}
		if(unique_id)
			column->featuresets_unique_ids = list;
		else
			column->featuresets_names = list;
	}
      return list;
}




/* For blocks within alignments other than the master alignment, it is not possible to simply
 * use the x1,x2 positions in the feature struct as these are the positions in the original
 * feature. We need to know the coordinates in the master alignment. (ie as in the parent span) */
void zMapFeature2MasterCoords(ZMapFeature feature, double *feature_x1, double *feature_x2)
{
  double master_x1 = 0.0, master_x2 = 0.0 ;
  ZMapFeatureBlock block ;
  double feature_offset = 0.0 ;

  zMapAssert(feature->parent && feature->parent->parent) ;

  block = (ZMapFeatureBlock)feature->parent->parent ;

  feature_offset = block->block_to_sequence.parent.x1 - block->block_to_sequence.block.x1;

  master_x1 = feature->x1 + feature_offset ;
  master_x2 = feature->x2 + feature_offset ;

  *feature_x1 = master_x1 ;
  *feature_x2 = master_x2 ;

  return ;
}



void zMapFeature2BlockCoords(ZMapFeatureBlock block, int *x1_inout, int *x2_inout)
{
  int offset ;

  offset = block->block_to_sequence.block.x1 ;

  if (x1_inout)
    *x1_inout = (*x1_inout - offset) + 1 ;

  if (x2_inout)
    *x2_inout = (*x2_inout - offset) + 1 ;

  return ;
}



void zMapBlock2FeatureCoords(ZMapFeatureBlock block, int *x1_inout, int *x2_inout)
{
  int offset ;

  offset = block->block_to_sequence.block.x1 ;

  if (x1_inout)
    *x1_inout = (*x1_inout + offset) - 1 ;

  if (x2_inout)
    *x2_inout = (*x2_inout + offset) - 1 ;

  return ;
}





gboolean zMapFeatureGetFeatureListExtent(GList *feature_list, int *start_out, int *end_out)
{
  gboolean done = FALSE;
  ZMapSpanStruct span = {0};
  ZMapFeature feature;

  if(feature_list && (feature = (ZMapFeature)(g_list_nth_data(feature_list, 0))))
    {
      span.x1 = feature->x1;
      span.x2 = feature->x2;

      g_list_foreach(feature_list, get_feature_list_extent, &span);

      if(start_out)
        *start_out = span.x1;

      if(end_out)
        *end_out = span.x2;

      done = TRUE;
    }

  return done;
}



GArray *zMapFeatureWorld2TranscriptArray(ZMapFeature feature)
{
  GArray *t_array = NULL, *exon_array;

  if(ZMAPFEATURE_HAS_EXONS(feature))
    {
      ZMapSpanStruct span;
      int i;

      t_array    = g_array_sized_new(FALSE, FALSE, sizeof(ZMapSpanStruct), 128);
      exon_array = feature->feature.transcript.exons;

      for(i = 0; i < exon_array->len; i++)
	{
	  span = g_array_index(exon_array, ZMapSpanStruct, i);
	  zMapFeatureWorld2Transcript(feature, span.x1, span.x2, &(span.x1), &(span.x2));
	  t_array = g_array_append_val(t_array, span);
	}
    }

  return t_array;
}


gboolean zMapFeatureWorld2Transcript(ZMapFeature feature,
				     int w1, int w2,
				     int *t1, int *t2)
{
  gboolean is_transcript = FALSE;

  if(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      if(feature->x1 > w2 || feature->x2 < w1)
	is_transcript = FALSE;
      else
	{
	  SimpleParent2ChildDataStruct parent_data = { NULL };
	  ZMapMapBlockStruct map_data = { {0,0}, {0,0}, FALSE };
	  map_data.parent.x1 = w1;
	  map_data.parent.x2 = w2;

	  parent_data.map         = &map_data;
	  parent_data.limit_start = feature->x1;
	  parent_data.limit_end   = feature->x2;
	  parent_data.counter     = 0;

	  zMapFeatureTranscriptExonForeach(feature, map_parent2child,
					   &parent_data);
	  if(t1)
	    *t1 = map_data.block.x1;
	  if(t2)
	    *t2 = map_data.block.x2;

	  is_transcript = TRUE;
	}
    }
  else
    is_transcript = FALSE;

  return is_transcript;
}


void zMapFeatureTranscriptExonForeach(ZMapFeature feature, GFunc function, gpointer user_data)
{
  GArray *exons;
  unsigned index;
  int multiplier = 1, start = 0, end, i;
  gboolean forward = TRUE;

  zMapAssert(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT);

  exons = feature->feature.transcript.exons;

  if(exons->len > 1)
    {
      ZMapSpan first, last;
      first = &(g_array_index(exons, ZMapSpanStruct, 0));
      last  = &(g_array_index(exons, ZMapSpanStruct, exons->len - 1));
      zMapAssert(first && last);

      if(first->x1 > last->x1)
        forward = FALSE;
    }

  if(forward)
    end = exons->len;
  else
    {
      multiplier = -1;
      start = (exons->len * multiplier) + 1;
      end   = 1;
    }

  for(i = start; i < end; i++)
    {
      ZMapSpan exon_span;

      index = i * multiplier;

      exon_span = &(g_array_index(exons, ZMapSpanStruct, index));

      (function)(exon_span, user_data);
    }

  return ;
}




GArray *zMapFeatureWorld2CDSArray(ZMapFeature feature)
{
  GArray *cds_array = NULL ;

  if (ZMAPFEATURE_HAS_CDS(feature) && ZMAPFEATURE_HAS_EXONS(feature))
    {
      GArray *exon_array ;
      ZMapSpanStruct span;
      int i;

      cds_array  = g_array_sized_new(FALSE, FALSE, sizeof(ZMapSpanStruct), 128);
      exon_array = feature->feature.transcript.exons;

      for(i = 0; i < exon_array->len; i++)
	{
	  span = g_array_index(exon_array, ZMapSpanStruct, i);
	  zMapFeatureWorld2CDS(feature, span.x1, span.x2, &(span.x1), &(span.x2));
	  cds_array = g_array_append_val(cds_array, span);
	}
    }

  return cds_array ;
}



gboolean zMapFeatureWorld2CDS(ZMapFeature feature,
			      int exon1, int exon2, int *cds1, int *cds2)
{
  gboolean cds_exon = FALSE;

  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.flags.cds)
    {
      int cds_start, cds_end;

      cds_start = feature->feature.transcript.cds_start;
      cds_end   = feature->feature.transcript.cds_end;

      if (cds_start > exon2 || cds_end < exon1)
	{
	  cds_exon = FALSE;
	}
      else
	{
	  SimpleParent2ChildDataStruct exon_cds_data = { NULL };
	  ZMapMapBlockStruct map_data = { {0,0}, {0,0}, FALSE } ;

	  map_data.parent.x1 = exon1;
	  map_data.parent.x2 = exon2;

	  exon_cds_data.map         = &map_data;
	  exon_cds_data.limit_start = cds_start;
	  exon_cds_data.limit_end   = cds_end;
	  exon_cds_data.counter     = 0;

	  zMapFeatureTranscriptExonForeach(feature, map_parent2child,
					   &exon_cds_data);
	  if (cds1)
	    *cds1 = map_data.block.x1;
	  if (cds2)
	    *cds2 = map_data.block.x2;

	  cds_exon = TRUE;
	}
    }

  return cds_exon;
}


/* Given a transcript feature and the start/end of an exon within that transcript feature,
 * returns TRUE and gives the coords (in reference sequence space) of the CDS section
 * of the exon and also it's phase. Returns FALSE if the exon is not in the transcript
 * or if the exon has no cds section. */
gboolean zMapFeatureExon2CDS(ZMapFeature feature,
			     int exon_start, int exon_end, int *exon_cds_start, int *exon_cds_end, int *phase_out)
{
  gboolean is_cds_exon = FALSE ;
  int exon_index ;

  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.flags.cds
      && (exon_index = findExon(feature, exon_start, exon_end)) > -1)
    {
      int cds_start, cds_end ;

      cds_start = feature->feature.transcript.cds_start ;
      cds_end = feature->feature.transcript.cds_end ;

      if (!(cds_start > exon_end || cds_end < exon_start))
	{
	  /* Exon has a cds section so calculate it and find the exons phase. */
	  int start, end, phase ;

	  if (calcExonPhase(feature, exon_index, &start, &end, &phase))
	    {
	      *exon_cds_start = start ;
	      *exon_cds_end = end ;
	      *phase_out = phase ;

	      is_cds_exon = TRUE ;
	    }
	}
    }

  return is_cds_exon ;
}




ZMapFrame zMapFeatureFrame(ZMapFeature feature)
{
  ZMapFrame frame = ZMAPFRAME_NONE ;

  frame = feature_frame(feature, feature->x1);

  return frame ;
}


ZMapFrame zMapFeatureSubPartFrame(ZMapFeature feature, int coord)
{
  ZMapFrame frame = ZMAPFRAME_NONE ;

  if(coord >= feature->x1 && coord <= feature->x2)
    frame = feature_frame(feature, coord);

  return frame ;
}

/* Returns ZMAPFRAME_NONE if not a transcript */
ZMapFrame zMapFeatureTranscriptFrame(ZMapFeature feature)
{
  ZMapFrame frame = ZMAPFRAME_NONE;

  if(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      int start;
      if(feature->feature.transcript.flags.cds)
	start = feature->feature.transcript.cds_start;
      else
	start = feature->x1;

      frame = feature_frame(feature, start);
    }
  else
    {
      zMapLogWarning("Feature %s is not a Transcript.", g_quark_to_string(feature->unique_id));
    }

  return frame;
}



char *zMapFeatureTranscriptTranslation(ZMapFeature feature, int *length)
{
  char *pep_str = NULL ;
  ZMapFeatureContext context ;
  ZMapPeptide peptide ;
  char *dna_str, *name, *free_me ;

  context = (ZMapFeatureContext)(zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
							   ZMAPFEATURE_STRUCT_CONTEXT));

  if ((dna_str = zMapFeatureGetTranscriptDNA(feature, TRUE, feature->feature.transcript.flags.cds)))
    {
      free_me = dna_str;					    /* as we potentially move ptr. */
      name    = (char *)g_quark_to_string(feature->original_id);

      if (feature->feature.transcript.flags.start_not_found)
	dna_str += (feature->feature.transcript.start_not_found - 1) ;

      peptide = zMapPeptideCreate(name, NULL, dna_str, NULL, TRUE);

      if(length)
	{
	  *length = zMapPeptideLength(peptide);
	  if (zMapPeptideHasStopCodon(peptide))
	    *length = *length - 1;
	}

      pep_str = zMapPeptideSequence(peptide);
      pep_str = g_strdup(pep_str);

      zMapPeptideDestroy(peptide);
      g_free(free_me);
    }

  return pep_str ;
}

char *zMapFeatureTranslation(ZMapFeature feature, int *length)
{
  char *seq;

  if(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      seq = zMapFeatureTranscriptTranslation(feature, length);
    }
  else
    {
      GArray *rubbish;
      int i, l;
      char c = '.';

      l = feature->x2 - feature->x1 + 1;

      rubbish = g_array_sized_new(TRUE, TRUE, sizeof(char), l);

      for(i = 0; i < l; i++)
	{
	  g_array_append_val(rubbish, c);
	}

      seq = rubbish->data;

      if(length)
	*length = l;

      g_array_free(rubbish, FALSE);
    }

  return seq;
}






/*
 *              Internal routines.
 */




/* Encapulates the rules about which frame a feature is in and what enum to return.
 *
 * For ZMap this amounts to:
 *
 * ((coord mod 3) + 1) gives the enum....
 *
 * Using the offset of 1 is almost certainly wrong for the reverse strand and
 * possibly wrong for forward.  Need to think about this one ;)
 *  */

static ZMapFrame feature_frame(ZMapFeature feature, int start_coord)
{
  ZMapFrame frame;
  int offset;
  ZMapFeatureBlock block;
  int fval;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;
  zMapAssert(feature->parent && feature->parent->parent);

  block = (ZMapFeatureBlock)(feature->parent->parent);

  offset = block->block_to_sequence.block.x1;   /* start of block in sequence/parent */

  fval = ((start_coord - offset) % 3) + ZMAPFRAME_0 ;
  if(fval < ZMAPFRAME_0) 	/* eg feature starts before the block */
	  fval += 3;
  frame  = (ZMapFrame) fval;

  return frame;
}


static void get_feature_list_extent(gpointer list_data, gpointer span_data)
{
  ZMapFeature feature = (ZMapFeature)list_data;
  ZMapSpan span = (ZMapSpan)span_data;

  if(feature->x1 < span->x1)
    span->x1 = feature->x1;
  if(feature->x2 > span->x2)
    span->x2 = feature->x2;

  return ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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


static void map_parent2child(gpointer exon_data, gpointer user_data)
{
  ZMapSpan exon_span = (ZMapSpan)exon_data;
  SimpleParent2ChildData p2c_data = (SimpleParent2ChildData)user_data;

  if(!(p2c_data->limit_start > exon_span->x2 ||
       p2c_data->limit_end   < exon_span->x1))
    {
      if(exon_span->x1 <= p2c_data->map->parent.x1 &&
	 exon_span->x2 >= p2c_data->map->parent.x1)
	{
	  /* update the c1 coord*/
	  p2c_data->map->block.x1  = (p2c_data->map->parent.x1 - p2c_data->limit_start + 1);
	  p2c_data->map->block.x1 += p2c_data->counter;
	}

      if(exon_span->x1 <= p2c_data->map->parent.x2 &&
	 exon_span->x2 >= p2c_data->map->parent.x2)
	{
	  /* update the c2 coord */
	  p2c_data->map->block.x2  = (p2c_data->map->parent.x2 - p2c_data->limit_start + 1);
	  p2c_data->map->block.x2 += p2c_data->counter;
	}

      p2c_data->counter += (exon_span->x2 - exon_span->x1 + 1);
    }

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


/* Find an exon in a features list given it's start/end coords, returns -1
 * if exon not found, otherwise returns index of exon in exon array. */
static int findExon(ZMapFeature feature, int exon_start, int exon_end)
{
  int exon_index = -1 ;
  GArray *exons ;
  int i ;

  exons = feature->feature.transcript.exons ;

  for (i = 0 ; i < exons->len ; i++)
    {
      ZMapSpan next_exon ;

      next_exon = &(g_array_index(exons, ZMapSpanStruct, i)) ;

      if (next_exon->x1 == exon_start && next_exon->x2 == exon_end)
	{
	  exon_index = i ;
	  break ;
	}
    }

  return exon_index ;
}


/* Returns the coords (in reference sequence coords) of the cds section of the given exon
 * and also it's phase. */
static gboolean calcExonPhase(ZMapFeature feature, int exon_index,
			      int *exon_cds_start_out, int *exon_cds_end_out, int *phase_out)
{
  gboolean result = FALSE ;
  int cds_start, cds_end ;
  GArray *exons ;
  int i, incr, end ;
  int cds_bases ;
  gboolean first_exon ;

  cds_start = feature->feature.transcript.cds_start ;
  cds_end = feature->feature.transcript.cds_end ;
  cds_bases = 0 ;

  exons = feature->feature.transcript.exons ;
  first_exon = FALSE ;


  /* Go forwards through exons for forward strand genes and backwards for
   * reverse strand genes. */
  if (feature->strand == ZMAPSTRAND_FORWARD)
    {
      i = 0 ;
      end = feature->feature.transcript.exons->len ;
      incr = 1 ;
    }
  else
    {
      i = exons->len - 1 ;
      end = -1 ;
      incr = -1 ;
    }

  for ( ; i != end ; i += incr)
    {
      ZMapSpan next_exon ;

      next_exon = &(g_array_index(exons, ZMapSpanStruct, i)) ;

      if (cds_start > next_exon->x2 || cds_end < next_exon->x1)
	{
	  continue ;
	}
      else
	{
	  int start, end, phase ;

	  start = next_exon->x1 ;
	  end = next_exon->x2 ;

	  if (cds_start >= start && cds_start <= end)
	    {
	      start = cds_start ;

	      if (feature->strand == ZMAPSTRAND_FORWARD)
		first_exon = TRUE ;
	    }

	  if (cds_end >= start && cds_end <= end)
	    {
	      end = cds_end ;

	      if (feature->strand == ZMAPSTRAND_REVERSE)
		first_exon = TRUE ;
	    }

	  if (i == exon_index)
	    {
	      /* The first exon must have phase 0 unless it has been annotated as
	       * starting with a different phase, all others are calculated from
	       * CDS bases so far. */

	      if (first_exon)
		{
		  if (feature->feature.transcript.flags.start_not_found)
		    phase = feature->feature.transcript.start_not_found ;
		  else
		    phase = 0 ;
		}
	      else
		{
		  phase = (3 - (cds_bases % 3)) % 3 ;
		}

	      *exon_cds_start_out = start ;
	      *exon_cds_end_out = end ;
	      *phase_out = phase ;

	      result = TRUE ;

	      break ;
	    }

	  /* Keep a running count of bases so far, caculate phase of next exon from this. */
	  cds_bases += (end - start) + 1 ;

	  first_exon = FALSE ;
	}

    }

  return result ;
}
