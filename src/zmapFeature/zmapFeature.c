/*  Last edited: Jul 12 10:55 2011 (edgrif) */
/*  File: zmapFeatures.c
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements feature contexts, sets and features themselves.
 *              Includes code to create/merge/destroy contexts and sets.
 *
 * Exported functions: See zmapView_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtils.h>
#include <zmapFeature_P.h>




/*! @defgroup zmapfeatures   zMapFeatures: feature handling for ZMap
 * @{
 *
 * \brief  Feature handling for ZMap.
 *
 * zMapFeatures routines provide functions to create/modify/destroy individual
 * features, sets of features and feature contexts (contexts are sets of sets
 * of features with associated coordinate data for parent mapping etc.).
 *
 *  */


/* Could use just one struct...but one step at a time.... */


/* this may not be needed..... */
typedef struct
{
  GData **current_feature_sets ;
  GData **diff_feature_sets ;
  GData **new_feature_sets ;
} FeatureContextsStruct, *FeatureContexts ;



typedef struct
{
  GData **current_feature_sets ;
  GData **diff_feature_sets ;
  GData **new_feature_sets ;
} FeatureBlocksStruct, *FeatureBlocks ;


typedef struct
{
  GData **current_features ;
  GData **diff_features ;
  GData **new_features ;
} FeatureSetStruct, *FeatureSet ;

typedef struct
{
  ZMapFeatureContext view_context; /* The Context to compare against */
  ZMapFeatureContext iteration_context; /* The Context we're stepping through comparing to the view */
  ZMapFeatureContext diff_context; /* The result of the comparison */

  /* The current path through the view context that we're currently on */
  ZMapFeatureAny current_view_align;
  ZMapFeatureAny current_view_block;
  ZMapFeatureAny current_view_set;

  /* The path through the diff context we're currently on. */
  ZMapFeatureAny current_diff_align;
  ZMapFeatureAny current_diff_block;
  ZMapFeatureAny current_diff_set;

  /* Our status... */
  ZMapFeatureContextExecuteStatus status;

  /* the list of real featuresets (not columns) actually requested */
  GList *req_featuresets;

  gboolean new_features ;	/* this is a flag that includes featruesets and features */
  					/* don't know if it matters if we flag featuresets */
  int feature_count;		/* this is a count of the new features */

} MergeContextDataStruct, *MergeContextData;


typedef struct
{
  int length;
} DataListLengthStruct, *DataListLength ;


typedef struct _HackForForcingStyleModeStruct
{
  gboolean force;
  GHashTable *styles ;
  ZMapFeatureSet feature_set;
} HackForForcingStyleModeStruct, *HackForForcingStyleMode;


typedef struct
{
  GQuark original_id ;
  int start, end ;
  GList *feature_list ;
} FindFeaturesRangeStruct, *FindFeaturesRange ;


void print_loaded(GList *l,char *fred);

static ZMapFeatureAny featureAnyCreateFeature(ZMapFeatureStructType feature_type,
					      ZMapFeatureAny parent,
					      GQuark original_id, GQuark unique_id,
					      GHashTable *children) ;
static gboolean featureAnyAddFeature(ZMapFeatureAny feature_set, ZMapFeatureAny feature) ;
static gboolean destroyFeatureAnyWithChildren(ZMapFeatureAny feature_any, gboolean free_children) ;
static void featureAnyAddToDestroyList(ZMapFeatureContext context, ZMapFeatureAny feature_any) ;

static void destroyFeatureAny(gpointer data) ;
static void destroyFeature(ZMapFeature feature) ;
static void destroyContextSubparts(ZMapFeatureContext context) ;

static gboolean withdrawFeatureAny(gpointer key, gpointer value, gpointer user_data) ;

/* datalist debug stuff */

static void printDestroyDebugInfo(ZMapFeatureAny any, char *who) ;

static ZMapFeatureContextExecuteStatus eraseContextCB(GQuark key,
                                                      gpointer data,
                                                      gpointer user_data,
                                                      char **err_out);
static ZMapFeatureContextExecuteStatus destroyIfEmptyContextCB(GQuark key,
                                                               gpointer data,
                                                               gpointer user_data,
                                                               char **err_out);
static ZMapFeatureContextExecuteStatus mergePreCB(GQuark key,
                                                  gpointer data,
                                                  gpointer user_data,
                                                  char **err_out);

#define MH17_ADD_MODES  1
#if MH17_ADD_MODES
static ZMapFeatureContextExecuteStatus addModeCB(GQuark key_id,
						 gpointer data,
						 gpointer user_data,
						 char **error_out) ;
static void addFeatureModeCB(gpointer key, gpointer data, gpointer user_data) ;
#endif

static void logMemCalls(gboolean alloc, ZMapFeatureAny feature_any) ;

static void findFeaturesRangeCB(gpointer key, gpointer value, gpointer user_data) ;
static void findFeaturesNameCB(gpointer key, gpointer value, gpointer user_data) ;

static ZMapFeatureContextExecuteStatus addEmptySets(GQuark key,
                                                  gpointer data,
                                                  gpointer user_data,
                                                  char **err_out);

static gboolean merge_debug_G   = FALSE;
static gboolean destroy_debug_G = FALSE;


/* Currently if we use this we get seg faults so we must not be cleaning up properly somewhere... */
static gboolean USE_SLICE_ALLOC = TRUE ;
static gboolean LOG_MEM_CALLS = FALSE ;



#ifdef FEATURES_NEED_MAGIC
ZMAP_MAGIC_NEW(feature_magic_G, ZMapFeatureAnyStruct) ;
#endif /* FEATURES_NEED_MAGIC */

/* !
 * A set of functions for allocating, populating and destroying features.
 * The feature create and add data are in two steps because currently the required
 * use is to have a struct that may need to be filled in in several steps because
 * in some data sources the data comes split up in the datastream (e.g. exons in
 * GFF). If there is a requirement for the two bundled then it should be implemented
 * via a simple new "create and add" function that merely calls both the create and
 * add functions from below. */



/* Make a copy of any feature, the feature is "stand alone", i.e. it has no parent
 * and no children, these are simply not copied, or any other dynamically allocated stuff ?? */
ZMapFeatureAny zMapFeatureAnyCopy(ZMapFeatureAny orig_feature_any)
{
  ZMapFeatureAny new_feature_any  = NULL ;

  new_feature_any  = zmapFeatureAnyCopy(orig_feature_any, destroyFeatureAny) ;

  return new_feature_any ;
}


/* Returns TRUE if the feature could be found in the feature_set, FALSE otherwise. */
gboolean zMapFeatureAnyFindFeature(ZMapFeatureAny feature_set, ZMapFeatureAny feature)
{
  gboolean result = FALSE ;
  ZMapFeature hash_feature ;

  zMapAssert(feature_set && feature) ;

  if ((hash_feature = g_hash_table_lookup(feature_set->children, zmapFeature2HashKey(feature))))
    result = TRUE ;

  return result ;
}


/* Returns the feature if found, NULL otherwise. */
ZMapFeatureAny zMapFeatureAnyGetFeatureByID(ZMapFeatureAny feature_set, GQuark feature_id)
{
  ZMapFeatureAny feature ;

  feature = g_hash_table_lookup(feature_set->children, GINT_TO_POINTER(feature_id)) ;

  return feature ;
}


gboolean zMapFeatureAnyRemoveFeature(ZMapFeatureAny feature_parent, ZMapFeatureAny feature)
{
  gboolean result = FALSE;

  if (zMapFeatureAnyFindFeature(feature_parent, feature))
    {
      result = g_hash_table_steal(feature_parent->children, zmapFeature2HashKey(feature)) ;
      feature->parent = NULL;

      switch(feature->struct_type)
	{
	case ZMAPFEATURE_STRUCT_CONTEXT:
	  break ;
	case ZMAPFEATURE_STRUCT_ALIGN:
	  {
	    ZMapFeatureContext context = (ZMapFeatureContext)feature_parent ;

	    if (context->master_align == (ZMapFeatureAlignment)feature)
	      context->master_align = NULL ;

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_BLOCK:
	  break ;
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  break ;
	case ZMAPFEATURE_STRUCT_FEATURE:
	  break ;
	default:
	  zMapAssertNotReached() ;
	  break ;
	}

      result = TRUE ;
    }

  return result ;
}



#if MH17_ADD_MODES
/* legacy code that might just be sued somewhere in the world
 * should _not_ be called is we  used a styles file
 */

/* go through all the feature sets in the given AnyFeature (must be at least a feature set)
 * and set the style mode from that...a bit hacky really...think about this....
 *
 * Really this is all acedb methods which are not rich enough for what we want to set
 * in our styles...
 *
 *  */
gboolean zMapFeatureAnyAddModesToStyles(ZMapFeatureAny feature_any, GHashTable *styles)
{
  gboolean result = TRUE;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  HackForForcingStyleModeStruct hack = {FALSE} ;

  hack.force = FALSE ;
  hack.styles = styles ;

  zMapFeatureContextExecuteSubset(feature_any,
                                  ZMAPFEATURE_STRUCT_FEATURESET,
                                  addModeCB,
                                  &hack) ;

  if (status != ZMAP_CONTEXT_EXEC_STATUS_OK)
    result = FALSE ;

  return result;
}



/* This function is _only_ here for the otterlace -> zmap
 * communication processing of styles.  When using methods on the
 * acedb server, the server code makes them drawable, setting their
 * mode to basic for featureset where there are no features to get the
 * mode from... When adding new features to these once empty
 * columns, we must force the styles... */
gboolean zMapFeatureAnyForceModesToStyles(ZMapFeatureAny feature_any, GHashTable *styles)
{
  gboolean result = TRUE;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  HackForForcingStyleModeStruct hack = {FALSE} ;

  hack.force = TRUE ;
  hack.styles = styles ;

#warning This function should be removed... and zMapFeatureAnyAddModesToStyles used instead.

  zMapFeatureContextExecuteSubset(feature_any,
                                  ZMAPFEATURE_STRUCT_FEATURESET,
                                  addModeCB,
                                  &hack) ;

  if (status != ZMAP_CONTEXT_EXEC_STATUS_OK)
    result = FALSE ;

  return result;
}

#endif

ZMapFeatureAny zmapFeatureAnyCopy(ZMapFeatureAny orig_feature_any, GDestroyNotify destroy_cb)
{
  ZMapFeatureAny new_feature_any  = NULL ;
  guint bytes ;

//  zMapAssert(zMapFeatureIsValid(orig_feature_any)) ;

// mh17: trying to load features via xremote asserted if no features were already loaded
// this assert should not be here, as this module should handle incorrect input from external places
// other bits of code will assert if they need features present
// cross fingers and hope it works
// certainly should pose no problem for previous use as it didn't assert then
  if(!orig_feature_any)
    return(NULL);


  /* Copy the original struct and set common fields. */
  switch(orig_feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      bytes = sizeof(ZMapFeatureContextStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      bytes = sizeof(ZMapFeatureAlignmentStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      bytes = sizeof(ZMapFeatureBlockStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      bytes = sizeof(ZMapFeatureSetStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURE:
      bytes = sizeof(ZMapFeatureStruct) ;
      break ;
    default:
      zMapAssertNotReached();
      break;
    }


  /* What I don't like about this way of doing things is that the default behaviour
   * is to copy things that we either shouldn't be copying or should be making a
   * deep copy of.... */
  if (USE_SLICE_ALLOC)
    {
      new_feature_any = g_slice_alloc0(bytes) ;
      g_memmove(new_feature_any, orig_feature_any, bytes) ;
    }
  else
    {
      new_feature_any = g_memdup(orig_feature_any, bytes) ;
    }
  logMemCalls(TRUE, new_feature_any) ;



  /* We DO NOT copy children or parents... */
  new_feature_any->parent = NULL ;
  if (new_feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    new_feature_any->children = g_hash_table_new_full(NULL, NULL, NULL, destroy_cb) ;


  /* Fill in the fields unique to each struct type. */
  switch(new_feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
	ZMapFeatureContext new_context = (ZMapFeatureContext)new_feature_any ;

	new_context->elements_to_destroy = NULL ;

	new_context->req_feature_set_names = NULL ;

	new_context->src_feature_set_names = NULL ;

	new_context->master_align = NULL ;

	break ;
      }
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
	break;
      }
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	ZMapFeatureBlock new_block = (ZMapFeatureBlock)new_feature_any ;

	new_block->sequence.type = ZMAPSEQUENCE_NONE ;
	new_block->sequence.length = 0 ;
	new_block->sequence.sequence = NULL ;

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
	ZMapFeatureSet new_set = (ZMapFeatureSet) new_feature_any;

	/* MH17: this is for a copy of the featureset to be added to the diff context on merge
	 * we could set this to NULL to avoid a double free
	 * but there are a few calls here from WindowRemoteReceive, viewRemoteReceive, WindowDNA, WindowDraw
	 * and while it's tedious to copy the list it save a lot of starting at code
	 */

	GList *l,*copy = NULL;
	ZMapSpan span;

	for(l = new_set->loaded;l;l = l->next)
	  {
	    span = g_memdup(l->data,sizeof(ZMapSpanStruct));
	    copy = g_list_append(copy,span);    /* must preserve the order */
	  }

	new_set->loaded = copy;

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature new_feature = (ZMapFeature)new_feature_any,
	  orig_feature = (ZMapFeature)orig_feature_any ;

	new_feature->description = NULL;

	if (new_feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
	  {
	    ZMapAlignBlockStruct align;

	    if (orig_feature->feature.homol.align != NULL
		&& orig_feature->feature.homol.align->len > (guint)0)
	      {
		int i ;

		new_feature->feature.homol.align =
		  g_array_sized_new(FALSE, TRUE,
				    sizeof(ZMapAlignBlockStruct),
				    orig_feature->feature.homol.align->len);

		for (i = 0; i < orig_feature->feature.homol.align->len; i++)
		  {
		    align = g_array_index(orig_feature->feature.homol.align, ZMapAlignBlockStruct, i);
		    new_feature->feature.homol.align =
		      g_array_append_val(new_feature->feature.homol.align, align);
		  }
	      }
	  }
	else if (new_feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
	  {
	    ZMapSpanStruct span;
	    int i ;

	    if (orig_feature->feature.transcript.exons != NULL
		&& orig_feature->feature.transcript.exons->len > (guint)0)
	      {
		new_feature->feature.transcript.exons =
		  g_array_sized_new(FALSE, TRUE,
				    sizeof(ZMapSpanStruct),
				    orig_feature->feature.transcript.exons->len);

		for (i = 0; i < orig_feature->feature.transcript.exons->len; i++)
		  {
		    span = g_array_index(orig_feature->feature.transcript.exons, ZMapSpanStruct, i);
		    new_feature->feature.transcript.exons =
		      g_array_append_val(new_feature->feature.transcript.exons, span);
		  }
	      }

	    if (orig_feature->feature.transcript.introns != NULL
		&& orig_feature->feature.transcript.introns->len > (guint)0)
	      {
		new_feature->feature.transcript.introns =
		  g_array_sized_new(FALSE, TRUE,
				    sizeof(ZMapSpanStruct),
				    orig_feature->feature.transcript.introns->len);

		for (i = 0; i < orig_feature->feature.transcript.introns->len; i++)
		  {
		    span = g_array_index(orig_feature->feature.transcript.introns, ZMapSpanStruct, i);
		    new_feature->feature.transcript.introns =
		      g_array_append_val(new_feature->feature.transcript.introns, span);
		  }
	      }
	  }

	break ;
      }
    default:
      zMapAssertNotReached();
      break;
    }

  return new_feature_any ;
}


#if MH17_NOT_CALLED
void zMapFeatureAnyDestroy(ZMapFeatureAny feature_any)
{
  gboolean result ;

  zMapAssert(zMapFeatureIsValid(feature_any)) ;

  result = destroyFeatureAnyWithChildren(feature_any, TRUE) ;
  zMapAssert(result) ;

  return ;
}
#endif


/*!
 * Returns a single feature correctly intialised to be a "NULL" feature.
 *
 * @param   void  None.
 * @return  ZMapFeature  A pointer to the new ZMapFeature.
 *  */
ZMapFeature zMapFeatureCreateEmpty(void)
{
  ZMapFeature feature ;

  feature = (ZMapFeature)featureAnyCreateFeature(ZMAPFEATURE_STRUCT_FEATURE, NULL,
						 ZMAPFEATURE_NULLQUARK, ZMAPFEATURE_NULLQUARK,
						 NULL) ;
  feature->db_id = ZMAPFEATUREID_NULL ;
  feature->type = ZMAPSTYLE_MODE_INVALID ;

  return feature ;
}


/* ==================================================================
 * Because the contents of this are quite a lot of work. Useful for
 * creating single features, but be warned that usually you will need
 * to keep track of uniqueness, so for large parser the GFF style of
 * doing things is better
 * ==================================================================
 */
ZMapFeature zMapFeatureCreateFromStandardData(char *name, char *sequence, char *ontology,
					      ZMapStyleMode feature_type,
                                              ZMapFeatureTypeStyle style,
                                              int start, int end,
                                              gboolean has_score, double score,
                                              ZMapStrand strand)
{
  ZMapFeature feature = NULL;
  gboolean       good = FALSE;

  if ((feature = zMapFeatureCreateEmpty()))
    {
      char *feature_name_id = NULL;

      if ((feature_name_id = zMapFeatureCreateName(feature_type, name, strand,
						   start, end, 0, 0)) != NULL)
        {
          if ((good = zMapFeatureAddStandardData(feature, feature_name_id,
						 name, sequence, ontology,
						 feature_type, style,
						 start, end, has_score, score,
						 strand)))
            {
              /* Check I'm valid. Really worth it?? */
              if(!(good = zMapFeatureIsValid((ZMapFeatureAny)feature)))
                {
                  zMapFeatureDestroy(feature);
                  feature = NULL;
                }
            }
        }
    }

  return feature;
}

/*!
 * Adds the standard data fields to an empty feature.
 *  */
gboolean zMapFeatureAddStandardData(ZMapFeature feature, char *feature_name_id, char *name,
				    char *sequence, char *SO_accession,
				    ZMapStyleMode feature_type,
				    ZMapFeatureTypeStyle style,
				    int start, int end,
				    gboolean has_score, double score,
				    ZMapStrand strand)
{
  gboolean result = FALSE ;

  /* Currently we don't overwrite features, they must be empty. */
  zMapAssert(feature) ;

  if (feature->unique_id == ZMAPFEATURE_NULLQUARK)
    {
      feature->unique_id = g_quark_from_string(feature_name_id) ;
      feature->original_id = g_quark_from_string(name) ;
      feature->type = feature_type ;
      feature->SO_accession = g_quark_from_string(SO_accession) ;
      feature->style_id = zMapStyleGetUniqueID(style) ;
      feature->style = style;
      feature->x1 = start ;
      feature->x2 = end ;
      feature->strand = strand ;
      if (has_score)
	{
	  feature->flags.has_score = 1 ;
	  feature->score = (float) score ;
	}
    }

  if(feature->unique_id)      /* some DAS servers give use  Name "" which is an error */
    result = TRUE ;

  return result ;
}


/*!
 * Adds data to a feature which may be empty or may already have partial features,
 * e.g. transcript that does not yet have all its exons.
 *
 * NOTE that really we need this to be a polymorphic function so that the arguments
 * are different for different features.
 *  */
gboolean zMapFeatureAddKnownName(ZMapFeature feature, char *known_name)
{
  gboolean result = FALSE ;
  GQuark known_id ;

  zMapAssert(feature && (feature->type == ZMAPSTYLE_MODE_BASIC || feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)) ;

  known_id = g_quark_from_string(known_name) ;

  if (feature->type == ZMAPSTYLE_MODE_BASIC)
    feature->feature.basic.known_name = known_id ;
  else
    feature->feature.transcript.known_name = known_id ;

  result = TRUE ;

  return result ;
}





gboolean zMapFeatureAddVariationString(ZMapFeature feature, char *variation_string)
{
  gboolean result = TRUE ;

  zMapAssert(feature && (variation_string && *variation_string)) ;

  feature->feature.basic.has_attr.variation_str = TRUE ;
  feature->feature.basic.variation_str = variation_string ;

  return result ;
}


gboolean zMapFeatureAddSOaccession(ZMapFeature feature, GQuark SO_accession)
{
  gboolean result = TRUE ;

  zMapAssert(feature && SO_accession) ;

  feature->SO_accession = SO_accession ;

  return result ;
}


/*!
 * Adds homology data to a feature which may be empty or may already have partial features.
 *  */
gboolean zMapFeatureAddSplice(ZMapFeature feature, ZMapBoundaryType boundary)
{
  gboolean result = TRUE ;

  zMapAssert(feature && (boundary == ZMAPBOUNDARY_5_SPLICE || boundary == ZMAPBOUNDARY_3_SPLICE)) ;

  feature->flags.has_boundary = TRUE ;
  feature->boundary_type = boundary ;

  return result ;
}


/* Returns TRUE if feature is a sequence feature and is DNA, FALSE otherwise. */
gboolean zMapFeatureSequenceIsDNA(ZMapFeature feature)
{
  gboolean result = FALSE ;

  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny) feature, ZMAPFEATURE_STRUCT_FEATURE)) ;

  if (feature->type == ZMAPSTYLE_MODE_SEQUENCE && feature->feature.sequence.type == ZMAPSEQUENCE_DNA)
    result = TRUE ;

  return result ;
}


/* Returns TRUE if feature is a sequence feature and is Peptide, FALSE otherwise. */
gboolean zMapFeatureSequenceIsPeptide(ZMapFeature feature)
{
  gboolean result = FALSE ;

  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny) feature, ZMAPFEATURE_STRUCT_FEATURE)) ;

  if (feature->type == ZMAPSTYLE_MODE_SEQUENCE && feature->feature.sequence.type == ZMAPSEQUENCE_PEPTIDE)
    result = TRUE ;

  return result ;
}












/*!
 * Adds assembly path data to a feature.
 *  */
gboolean zMapFeatureAddAssemblyPathData(ZMapFeature feature,
					int length, ZMapStrand strand, GArray *path)
{
  gboolean result = FALSE ;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

  if (feature->type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
    {
      feature->feature.assembly_path.strand = strand ;
      feature->feature.assembly_path.length = length ;
      feature->feature.assembly_path.path = path ;

      result = TRUE ;
    }

  return result ;
}

/*!
 * Adds a URL to the object, n.b. we add this as a string that must be freed, this.
 * is because I don't want the global GQuark table to be expanded by these...
 *  */
gboolean zMapFeatureAddURL(ZMapFeature feature, char *url)
{
  gboolean result = TRUE ;				    /* We may add url checking sometime. */

  zMapAssert(feature && url && *url) ;

  feature->url = g_strdup(url) ;

  return result ;
}


/*!
 * Adds a Locus to the object.
 *  */
gboolean zMapFeatureAddLocus(ZMapFeature feature, GQuark locus_id)
{
  gboolean result = FALSE ;

  zMapAssert(feature && locus_id) ;

  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      feature->feature.transcript.locus_id = locus_id ;

      result = TRUE ;
    }

  return result ;
}


/*!
 * Adds descriptive text the object.
 *  */
gboolean zMapFeatureAddText(ZMapFeature feature, GQuark source_id, char *source_text, char *feature_text)
{
  gboolean result = TRUE ;

  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_FEATURE)
	     && (source_id || (source_text && *source_text) || (feature_text && *feature_text))) ;

  if (source_id)
    feature->source_id = source_id ;
  if (source_text)
    feature->source_text = g_quark_from_string(source_text) ;
  if (feature_text)
    feature->description = g_strdup(feature_text) ;

  return result ;
}





/*!
 * Returns the length of a feature. For a simple feature this is just (end - start + 1),
 * for transcripts and alignments the exons or blocks must be totalled up.
 *
 * @param   feature      Feature for which length is required.
 * @param   length_type  Length in target sequence coords or query sequence coords or spliced length.
 * @return               nothing.
 *  */
int zMapFeatureLength(ZMapFeature feature, ZMapFeatureLengthType length_type)
{
  int length = 0 ;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

  switch (length_type)
    {
    case ZMAPFEATURELENGTH_TARGET:
      {
	/* We just want the total length of the feature as located on the target sequence. */

	length = (feature->x2 - feature->x1 + 1) ;

	break ;
      }
    case ZMAPFEATURELENGTH_QUERY:
      {
	/* We want the length of the feature as it is in its original query sequence, this is
	 * only different from ZMAPFEATURELENGTH_TARGET for alignments. */

	if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
	  {
	    length = feature->feature.homol.y2 - feature->feature.homol.y1 + 1 ;
	  }
	else
	  {
	    length = feature->x2 - feature->x1 + 1 ;
	  }

	break ;
      }
    case ZMAPFEATURELENGTH_SPLICED:
      {
	/* We want the actual length of the feature blocks, only different for transcripts and alignments. */

	if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.exons)
	  {
	    int i ;
	    ZMapSpan span ;
	    GArray *exons = feature->feature.transcript.exons ;

	    length = 0 ;

	    for (i = 0 ; i < exons->len ; i++)
	      {
		span = &g_array_index(exons, ZMapSpanStruct, i) ;

		length += (span->x2 - span->x1 + 1) ;
	      }

	  }
	else if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT && feature->feature.homol.align)
	  {
	    int i ;
	    ZMapAlignBlock align ;
	    GArray *gaps = feature->feature.homol.align ;

	    length = 0 ;

	    for (i = 0 ; i < gaps->len ; i++)
	      {
		align = &g_array_index(gaps, ZMapAlignBlockStruct, i) ;

		length += (align->q2 - align->q1 + 1) ;
	      }
	  }
	else
	  {
	    length = (feature->x2 - feature->x1 + 1) ;
	  }

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;

	break ;
      }
    }


  return length ;
}

/*!
 * Destroys a feature, freeing up all of its resources.
 *
 * @param   feature      The feature to be destroyed.
 * @return               nothing.
 *  */
void zMapFeatureDestroy(ZMapFeature feature)
{
  gboolean result ;

  zMapAssert(feature) ;

  result = destroyFeatureAnyWithChildren((ZMapFeatureAny)feature, FALSE) ;
  zMapAssert(result) ;

  return ;
}




/*
 *                      Feature Set functions.
 */

/* Features can be NULL if there are no features yet..... */
ZMapFeatureSet zMapFeatureSetCreate(char *source, GHashTable *features)
{
  ZMapFeatureSet feature_set ;
  GQuark original_id, unique_id ;

  unique_id = zMapFeatureSetCreateID(source) ;
  original_id = g_quark_from_string(source) ;

  feature_set = zMapFeatureSetIDCreate(original_id, unique_id, NULL, features) ;

  return feature_set ;
}

void zMapFeatureSetStyle(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style)
{
  zMapAssert(feature_set && style) ;

      // MH17: was for the column (historically)
      // new put back to be used for each feature
  feature_set->style = style ;

  return ;
}


/* Features can be NULL if there are no features yet.....
 * original_id  the original name of the feature set
 * unique_id    some derivation of the original name or otherwise unique id to identify this
 *              feature set. */
ZMapFeatureSet zMapFeatureSetIDCreate(GQuark original_id, GQuark unique_id,
				      ZMapFeatureTypeStyle style, GHashTable *features)
{
  ZMapFeatureSet feature_set ;

  feature_set = (ZMapFeatureSet)featureAnyCreateFeature(ZMAPFEATURE_STRUCT_FEATURESET, NULL,
							original_id, unique_id,
							features) ;
  //feature_set->style = style ;

  return feature_set ;
}

/* Feature must not be null to be added we need at least the feature id and probably should.
 * check for more.
 *
 * Returns FALSE if feature is already in set.
 *  */
gboolean zMapFeatureSetAddFeature(ZMapFeatureSet feature_set, ZMapFeature feature)
{
  gboolean result = FALSE ;

  result = featureAnyAddFeature((ZMapFeatureAny)feature_set, (ZMapFeatureAny)feature) ;

  return result ;
}


/* Returns TRUE if the feature could be found in the feature_set, FALSE otherwise. */
gboolean zMapFeatureSetFindFeature(ZMapFeatureSet feature_set,
                                   ZMapFeature    feature)
{
  gboolean result = FALSE ;

  zMapAssert(feature_set && feature) ;

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_set, (ZMapFeatureAny)feature) ;

  return result ;
}

ZMapFeature zMapFeatureSetGetFeatureByID(ZMapFeatureSet feature_set, GQuark feature_id)
{
  ZMapFeature feature ;

  feature = (ZMapFeature)zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)feature_set, feature_id) ;

  return feature ;
}


/* Feature must exist in set to be removed.
 *
 * Returns FALSE if feature is not in set.
 *  */
gboolean zMapFeatureSetRemoveFeature(ZMapFeatureSet feature_set, ZMapFeature feature)
{
  gboolean result = FALSE ;

  zMapAssert(feature_set && feature && feature->unique_id != ZMAPFEATURE_NULLQUARK) ;

  result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_set, (ZMapFeatureAny)feature) ;

  return result ;
}



GList *zMapFeatureSetGetRangeFeatures(ZMapFeatureSet feature_set, int start, int end)
{
  GList *feature_list = NULL ;
  FindFeaturesRangeStruct find_data ;

  find_data.start = start ;
  find_data.end = end ;
  find_data.feature_list = NULL ;

  g_hash_table_foreach(feature_set->features, findFeaturesRangeCB, &find_data) ;

  feature_list = find_data.feature_list ;

  return feature_list ;
}

/* A GHFunc() to add a feature to a list if it within a given range */
static void findFeaturesRangeCB(gpointer key, gpointer value, gpointer user_data)
{
  FindFeaturesRange find_data = (FindFeaturesRange)user_data ;
  ZMapFeature feature = (ZMapFeature)value ;

  if (feature->x1 >= find_data->start && feature->x1 <= find_data->end)
    find_data->feature_list = g_list_append(find_data->feature_list, feature) ;

  return ;
}


GList *zMapFeatureSetGetNamedFeatures(ZMapFeatureSet feature_set, GQuark original_id)
{
  GList *feature_list = NULL ;
  FindFeaturesRangeStruct find_data = {0} ;

  find_data.original_id = original_id ;
  find_data.feature_list = NULL ;

  g_hash_table_foreach(feature_set->features, findFeaturesNameCB, &find_data) ;

  feature_list = find_data.feature_list ;

  return feature_list ;
}

/* A GHFunc() to add a feature to a list if it within a given range */
static void findFeaturesNameCB(gpointer key, gpointer value, gpointer user_data)
{
  FindFeaturesRange find_data = (FindFeaturesRange)user_data ;
  ZMapFeature feature = (ZMapFeature)value ;

  if (feature->original_id == find_data->original_id)
    find_data->feature_list = g_list_append(find_data->feature_list, feature) ;

  return ;
}





void zMapFeatureSetDestroy(ZMapFeatureSet feature_set, gboolean free_data)
{
  gboolean result ;

  zMapAssert(feature_set);
  result = destroyFeatureAnyWithChildren((ZMapFeatureAny)feature_set, free_data) ;
  zMapAssert(result) ;

  return ;
}


void zMapFeatureSetDestroyFeatures(ZMapFeatureSet feature_set)
{
  zMapAssert(feature_set) ;

  g_hash_table_destroy(feature_set->features) ;
  feature_set->features = NULL ;

  return ;
}




/*
 *                Alignment functions
 */

GQuark zMapFeatureAlignmentCreateID(char *align_name, gboolean master_alignment)
{
  GQuark id = 0;
  char *unique_name;

  if (master_alignment)
    unique_name = g_strdup_printf("%s_master", align_name) ;
  else
    unique_name = g_strdup(align_name) ;

  id = g_quark_from_string(unique_name) ;

  g_free(unique_name);

  return id;
}


ZMapFeatureAlignment zMapFeatureAlignmentCreate(char *align_name, gboolean master_alignment)
{
  ZMapFeatureAlignment alignment ;

  zMapAssert(align_name) ;

  alignment = (ZMapFeatureAlignment)featureAnyCreateFeature(ZMAPFEATURE_STRUCT_ALIGN,
							    NULL,
							    g_quark_from_string(align_name),
							    zMapFeatureAlignmentCreateID(align_name, master_alignment),
							    NULL) ;

  return alignment ;
}


gboolean zMapFeatureAlignmentAddBlock(ZMapFeatureAlignment alignment, ZMapFeatureBlock block)
{
  gboolean result = FALSE  ;

  zMapAssert(alignment && block) ;

  result = featureAnyAddFeature((ZMapFeatureAny)alignment, (ZMapFeatureAny)block) ;

  if(result)
  {
      /* remember where our data hails from */
      if(!alignment->sequence_span.x1 || alignment->sequence_span.x1 > block->block_to_sequence.block.x1)
            alignment->sequence_span.x1 = block->block_to_sequence.block.x1;

      if(!alignment->sequence_span.x2 || alignment->sequence_span.x2 < block->block_to_sequence.block.x2)
            alignment->sequence_span.x2 = block->block_to_sequence.block.x2;
  }

  return result ;
}


gboolean zMapFeatureAlignmentFindBlock(ZMapFeatureAlignment feature_align,
                                       ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;

  zMapAssert(feature_align && feature_block);

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_align, (ZMapFeatureAny)feature_block) ;

  return result;
}

ZMapFeatureBlock zMapFeatureAlignmentGetBlockByID(ZMapFeatureAlignment feature_align, GQuark block_id)
{
  ZMapFeatureBlock feature_block = NULL;

  feature_block = (ZMapFeatureBlock)zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)feature_align, block_id) ;

  return feature_block ;
}


gboolean zMapFeatureAlignmentRemoveBlock(ZMapFeatureAlignment feature_align,
                                         ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;

  result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_align, (ZMapFeatureAny)feature_block) ;

  return result;
}

void zMapFeatureAlignmentDestroy(ZMapFeatureAlignment alignment, gboolean free_data)
{
  gboolean result ;

  zMapAssert(alignment);
  result = destroyFeatureAnyWithChildren((ZMapFeatureAny)alignment, free_data) ;
  zMapAssert(result) ;

  return ;
}





/*
 *       ZMapFeatureBlock functions.
 */


ZMapFeatureBlock zMapFeatureBlockCreate(char *block_seq,
					int ref_start, int ref_end, ZMapStrand ref_strand,
					int non_start, int non_end, ZMapStrand non_strand)
{
  ZMapFeatureBlock new_block ;

  zMapAssert((ref_strand == ZMAPSTRAND_FORWARD || ref_strand == ZMAPSTRAND_REVERSE)
	     && (non_strand == ZMAPSTRAND_FORWARD || non_strand == ZMAPSTRAND_REVERSE)) ;

  new_block = (ZMapFeatureBlock)featureAnyCreateFeature(ZMAPFEATURE_STRUCT_BLOCK,
							NULL,
							g_quark_from_string(block_seq),
							zMapFeatureBlockCreateID(ref_start, ref_end, ref_strand,
										 non_start, non_end, non_strand),
							NULL) ;

  new_block->block_to_sequence.parent.x1 = ref_start ;
  new_block->block_to_sequence.parent.x2 = ref_end ;
//  new_block->block_to_sequence.t_strand = ref_strand ;

  new_block->block_to_sequence.block.x1 = non_start ;
  new_block->block_to_sequence.block.x2 = non_end ;
//  new_block->block_to_sequence.q_strand = non_strand ;

  new_block->block_to_sequence.reversed = (ref_strand != non_strand);

//  new_block->features_start = new_block->block_to_sequence.q1 ;
//  new_block->features_end = new_block->block_to_sequence.q2 ;

  return new_block ;
}



/* was used to set request coord in feature_block we prefer to set these explicitly in the req protocol
 * howeverwe still need to set thes up for sub-sequence requests to handle 'no data'
 */
gboolean zMapFeatureBlockSetFeaturesCoords(ZMapFeatureBlock feature_block,
					   int features_start, int features_end)
{
  gboolean result = FALSE  ;


  feature_block->block_to_sequence.parent.x1 = features_start ;
  feature_block->block_to_sequence.parent.x2 = features_end ;

  feature_block->block_to_sequence.block.x1 = features_start ;
  feature_block->block_to_sequence.block.x2 = features_end ;

  result = TRUE ;

  return result ;
}


gboolean zMapFeatureBlockAddFeatureSet(ZMapFeatureBlock feature_block,
				       ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE  ;

  zMapAssert(feature_block && feature_set) ;

  result = featureAnyAddFeature((ZMapFeatureAny)feature_block, (ZMapFeatureAny)feature_set) ;

  return result ;
}



gboolean zMapFeatureBlockFindFeatureSet(ZMapFeatureBlock feature_block,
                                        ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE ;

  zMapAssert(feature_set && feature_block);

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_block, (ZMapFeatureAny)feature_set) ;

  return result;
}

ZMapFeatureSet zMapFeatureBlockGetSetByID(ZMapFeatureBlock feature_block, GQuark set_id)
{
  ZMapFeatureSet feature_set ;

  feature_set = (ZMapFeatureSet)zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)feature_block, set_id) ;

  return feature_set ;
}


gboolean zMapFeatureBlockRemoveFeatureSet(ZMapFeatureBlock feature_block,
                                          ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE;

  result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_block, (ZMapFeatureAny)feature_set) ;

  return result;
}

void zMapFeatureBlockDestroy(ZMapFeatureBlock block, gboolean free_data)
{
  gboolean result ;

  zMapAssert(block);

  result = destroyFeatureAnyWithChildren((ZMapFeatureAny)block, free_data) ;
  zMapAssert(result) ;

  return ;
}

ZMapFeatureContext zMapFeatureContextCreate(char *sequence, int start, int end, GList *set_names)
{
  ZMapFeatureContext feature_context ;
  GQuark original_id = 0, unique_id = 0 ;

  if (sequence && *sequence)
    unique_id = original_id = g_quark_from_string(sequence) ;

  feature_context = (ZMapFeatureContext)featureAnyCreateFeature(ZMAPFEATURE_STRUCT_CONTEXT,
								NULL,
								original_id, unique_id,
								NULL) ;

  if (sequence && *sequence)
    {
      feature_context->sequence_name = g_quark_from_string(sequence) ;
      feature_context->parent_span.x1 = start ;
      feature_context->parent_span.x2 = end ;
    }

  feature_context->req_feature_set_names = set_names ;

  return feature_context ;
}

gboolean zMapFeatureContextAddAlignment(ZMapFeatureContext feature_context,
					ZMapFeatureAlignment alignment, gboolean master)
{
  gboolean result = FALSE  ;

  zMapAssert(feature_context && alignment) ;

  if ((result = featureAnyAddFeature((ZMapFeatureAny)feature_context, (ZMapFeatureAny)alignment)))
    {
      if (master)
	feature_context->master_align = alignment ;
    }

  return result ;
}

gboolean zMapFeatureContextFindAlignment(ZMapFeatureContext   feature_context,
                                         ZMapFeatureAlignment feature_align)
{
  gboolean result = FALSE ;

  zMapAssert(feature_context && feature_align );

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_context, (ZMapFeatureAny)feature_align) ;

  return result;
}

ZMapFeatureAlignment zMapFeatureContextGetAlignmentByID(ZMapFeatureContext feature_context,
                                                        GQuark align_id)
{
  ZMapFeatureAlignment feature_align ;

  feature_align = (ZMapFeatureAlignment)zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)feature_context, align_id) ;

  return feature_align ;
}


gboolean zMapFeatureContextRemoveAlignment(ZMapFeatureContext feature_context,
                                           ZMapFeatureAlignment feature_alignment)
{
  gboolean result = FALSE;

  zMapAssert(feature_context && feature_alignment);

  if ((result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_context, (ZMapFeatureAny)feature_alignment)))
    {
      if(feature_context->master_align == feature_alignment)
        feature_context->master_align = NULL;
    }

  return result;
}






/* Context merging is complicated when it comes to destroying the contexts
 * because the diff_context actually points to feature structs in the merged
 * context. It has to do this because we want to pass to the drawing code
 * only new features that need drawing. To do this though means that some of
 * of the "parent" parts of the context (aligns, blocks, sets) must be duplicates
 * of those in the current context. Hence we end up with a context where we
 * want to destroy some features (the duplicates) but not others (the ones that
 * are just pointers to features in the current context).
 *
 * So for the diff_context we don't set destroy functions when the context
 * is created, instead we keep a separate hash of duplicate features to be destroyed.
 *
 * If hashtables supported setting a destroy function for each element we
 * wouldn't need to do this, but they don't (unlike g_datalists, we don't
 * use those because they are too slow).
 *
 * If all ok returns ZMAPFEATURE_CONTEXT_OK, merged context in merged_context_inout
 * and diff context in diff_context_out. Otherwise returns a code to show what went
 * wrong, unaltered original context in merged_context_inout and diff_context_out is NULL,
 */

/* N.B. under new scheme, new_context_inout will be always be destroyed && NULL'd.... */

ZMapFeatureContextMergeCode zMapFeatureContextMerge(ZMapFeatureContext *merged_context_inout,
						    ZMapFeatureContext *new_context_inout,
						    ZMapFeatureContext *diff_context_out,
                                        GList *featureset_names)
{
  ZMapFeatureContextMergeCode status = ZMAPFEATURE_CONTEXT_ERROR ;
  ZMapFeatureContext current_context, new_context, diff_context = NULL ;
  MergeContextDataStruct merge_data = {NULL} ;

  zMapAssert(merged_context_inout && new_context_inout && diff_context_out) ;

  current_context = *merged_context_inout ;
  new_context = *new_context_inout ;


  /* If there are no current features we just return the new one as the merged and the diff,
   * otherwise we need to do a merge of the new and current feature sets. */
  if (!current_context)
    {
      if (merge_debug_G)
        zMapLogWarning("%s", "No current context, returning complete new...") ;

      diff_context = current_context = new_context ;
      new_context = NULL ;

      merge_data.view_context      = current_context;
      merge_data.status            = ZMAP_CONTEXT_EXEC_STATUS_OK ;
      merge_data.req_featuresets   = featureset_names;

      zMapFeatureContextExecute((ZMapFeatureAny) current_context,
				ZMAPFEATURE_STRUCT_BLOCK, addEmptySets, &merge_data);

      status = ZMAPFEATURE_CONTEXT_OK ;
    }
  else
    {
      /* Here we need to merge for all alignments and all blocks.... */

      GList *copy_features ;


      /* Note we make the diff context point at the feature list and styles of the new context,
       * I guess we could copy them but doesn't seem worth it...see code below where we NULL them
       * in new_context so they are not thrown away.... */
      diff_context = (ZMapFeatureContext)zmapFeatureAnyCopy((ZMapFeatureAny)new_context, NULL) ;
      diff_context->diff_context = TRUE ;
      diff_context->elements_to_destroy = g_hash_table_new_full(NULL, NULL, NULL, destroyFeatureAny) ;
      diff_context->req_feature_set_names = g_list_copy(new_context->req_feature_set_names) ;
      diff_context->src_feature_set_names = NULL;

      merge_data.view_context      = current_context;
      merge_data.iteration_context = new_context;
      merge_data.diff_context      = diff_context ;
      merge_data.status            = ZMAP_CONTEXT_EXEC_STATUS_OK ;
      merge_data.new_features = FALSE ;
      merge_data.feature_count = 0;
      merge_data.req_featuresets   = new_context->req_feature_set_names;


      /* THIS LOOKS SUSPECT...WHY ISN'T THE NAMES LIST COPIED FROM NEW_CONTEXT....*/
      copy_features = g_list_copy(new_context->req_feature_set_names) ;
      current_context->req_feature_set_names = g_list_concat(current_context->req_feature_set_names,
							     copy_features) ;

      if (merge_debug_G)
        zMapLogWarning("%s", "merging ...");

      /* Do the merge ! */
      zMapFeatureContextExecuteStealSafe((ZMapFeatureAny)new_context, ZMAPFEATURE_STRUCT_FEATURE,
					 mergePreCB, NULL, &merge_data) ;

      if (merge_debug_G)
        zMapLogWarning("%s", "finished ...");


      if (merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
	{
	  /* Set these to NULL as diff_context references them. */
	  new_context->req_feature_set_names = NULL ;

	  current_context = merge_data.view_context ;
	  new_context     = merge_data.iteration_context ;
	  diff_context    = merge_data.diff_context ;

	  if (merge_data.new_features)
	    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      if(merge_erase_dump_context_G)
		{
		  /* Debug stuff... */
		  GError *err = NULL ;

		  printf("(Merge) diff context:\n") ;
		  zMapFeatureDumpStdOutFeatures(diff_context, current_context->styles, &err) ;
		}

	      if(merge_erase_dump_context_G)
		{
		  /* Debug stuff... */
		  GError *err = NULL ;

		  printf("(Merge) full context:\n") ;
		  zMapFeatureDumpStdOutFeatures(current_context, current_context->styles, &err) ;
		}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#ifdef MH17_NEVER
	      // NB causes crash on 2nd load, first one is ok
	      {
		GError *err = NULL;

		zMapFeatureDumpToFileName(diff_context,"features.txt","(Merge) diff context:\n", NULL, &err) ;
		zMapFeatureDumpToFileName(current_context,"features.txt","(Merge) full context:\n", NULL, &err) ;
	      }
#endif

	      status = ZMAPFEATURE_CONTEXT_OK ;
	    }
	  else
	    {
	      if (diff_context)
		{
		  zMapFeatureContextDestroy(diff_context, TRUE);

		  diff_context = NULL ;
		}

	      status = ZMAPFEATURE_CONTEXT_NONE ;
	    }
	}
    }

  /* Clear up new_context whatever happens. If there is an error, what can anyone do with it...nothing. */
  if (new_context)
    {
      zMapFeatureContextDestroy(new_context, TRUE);
      new_context = NULL ;
    }

  if (status == ZMAPFEATURE_CONTEXT_OK || status == ZMAPFEATURE_CONTEXT_NONE)
    {
      *merged_context_inout = current_context ;
      *new_context_inout = new_context ;
      *diff_context_out = diff_context ;
    }

  return status ;
}



/* creates a context of features of matches between the remove and
 * current, removing from the current as it goes. */
gboolean zMapFeatureContextErase(ZMapFeatureContext *current_context_inout,
				 ZMapFeatureContext remove_context,
				 ZMapFeatureContext *diff_context_out)
{
  gboolean erased = FALSE;
  /* Here we need to merge for all alignments and all blocks.... */
  MergeContextDataStruct merge_data = {NULL};
  char *diff_context_string  = NULL;
  ZMapFeatureContext current_context ;
  ZMapFeatureContext diff_context ;
  GList *copy_list ;

  zMapAssert(current_context_inout && remove_context && diff_context_out) ;

  current_context = *current_context_inout ;

  diff_context = zMapFeatureContextCreate(NULL, 0, 0, NULL);
  diff_context->diff_context        = TRUE;
  diff_context->elements_to_destroy = g_hash_table_new_full(NULL, NULL, NULL, destroyFeatureAny);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* WHY IS THIS DONE TWICE....??? */

  /* TRY COPYING THE LIST.... */
  diff_context->req_feature_set_names = g_list_copy(remove_context->req_feature_set_names) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  merge_data.view_context      = current_context;
  merge_data.iteration_context = remove_context;
  merge_data.diff_context      = diff_context;
  merge_data.status            = ZMAP_CONTEXT_EXEC_STATUS_OK;


  /* LOOKS SUSPECT...SHOULD BE COPIED....*/
  diff_context->req_feature_set_names = g_list_copy(remove_context->req_feature_set_names) ;

  copy_list = g_list_copy(remove_context->req_feature_set_names) ;
  current_context->req_feature_set_names = g_list_concat(current_context->req_feature_set_names,
                                                     copy_list) ;

  /* Set the original and unique ids so that the context passes the feature validity checks */
  diff_context_string = g_strdup_printf("%s vs %s\n",
                                        g_quark_to_string(current_context->unique_id),
                                        g_quark_to_string(remove_context->unique_id));
  diff_context->original_id =
    diff_context->unique_id = g_quark_from_string(diff_context_string);

  g_free(diff_context_string);

  zMapFeatureContextExecuteRemoveSafe((ZMapFeatureAny)remove_context, ZMAPFEATURE_STRUCT_FEATURE,
				      eraseContextCB, destroyIfEmptyContextCB, &merge_data);


  if(merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
    {
      erased = TRUE;

      *current_context_inout = current_context ;
      *diff_context_out      = diff_context ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if(merge_erase_dump_context_G)
	{
	  GError *err = NULL;

	  printf("(Erase) diff context:\n") ;
	  zMapFeatureDumpStdOutFeatures(diff_context, diff_context->styles, &err) ;

	  printf("(Erase) full context:\n") ;
	  zMapFeatureDumpStdOutFeatures(current_context, current_context->styles, &err) ;

	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return erased;
}


void zMapFeatureContextDestroy(ZMapFeatureContext feature_context, gboolean free_data)
{
  gboolean result ;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature_context)) ;

  if (feature_context->diff_context)
    {
      destroyFeatureAny(feature_context) ;
    }
  else
    {
      result = destroyFeatureAnyWithChildren((ZMapFeatureAny)feature_context, free_data) ;
      zMapAssert(result) ;
    }

  return ;
}


/*! @} end of zmapfeatures docs. */



/*
 *            Internal routines.
 */



/* Frees resources that are unique to a context, resources common to all features
 * are freed by a common routine. */
static void destroyContextSubparts(ZMapFeatureContext context)
{

  /* diff contexts are different because they are a mixture of pointers to subtrees in another
   * context, individual features in another context and copied elements in subtrees. */
  if (context->diff_context)
    {
      /* Remove the copied elements. */
      g_hash_table_destroy(context->elements_to_destroy) ;
    }


  if (context->req_feature_set_names)
    {
      g_list_free(context->req_feature_set_names) ;
      context->req_feature_set_names = NULL ;
    }
  if (context->src_feature_set_names)
    {
      g_list_free(context->src_feature_set_names) ;
      context->src_feature_set_names = NULL ;
    }
  return ;
}


static void destroyFeature(ZMapFeature feature)
{
  if (feature->url)
    g_free(feature->url) ;

  if (feature->description)
    g_free(feature->description) ;

  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      if (feature->feature.transcript.exons)
	g_array_free(feature->feature.transcript.exons, TRUE) ;

      if (feature->feature.transcript.introns)
	g_array_free(feature->feature.transcript.introns, TRUE) ;
    }
  else if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      if (feature->feature.homol.align)
	g_array_free(feature->feature.homol.align, TRUE) ;
    }

  return ;
}



/* A GDestroyNotify() function called from g_hash_table_destroy() to get rid of all children
 * in the hash. */
static void destroyFeatureAny(gpointer data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  gulong nbytes ;

  zMapAssert(zMapFeatureAnyHasMagic(feature_any));

  if (destroy_debug_G && feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    printDestroyDebugInfo(feature_any, "destroyFeatureAny") ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      nbytes = sizeof(ZMapFeatureContextStruct) ;
      destroyContextSubparts((ZMapFeatureContext)feature_any) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      nbytes = sizeof(ZMapFeatureAlignmentStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      nbytes = sizeof(ZMapFeatureBlockStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
	ZMapFeatureSet feature_set = (ZMapFeatureSet) feature_any;
	GList *l;

	if(feature_set->masker_sorted_features)
	  g_list_free(feature_set->masker_sorted_features);
	feature_set->masker_sorted_features = NULL;

	if(feature_set->loaded)
	  {
	    for(l = feature_set->loaded;l;l = l->next)
	      {
		g_free(l->data);
	      }
	    g_list_free(feature_set->loaded);
	  }
	feature_set->loaded = NULL;

	nbytes = sizeof(ZMapFeatureSetStruct) ;

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      nbytes = sizeof(ZMapFeatureStruct) ;
      destroyFeature((ZMapFeature)feature_any) ;
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  if (feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    {
      g_hash_table_destroy(feature_any->children) ;
    }


  logMemCalls(FALSE, feature_any) ;

  memset(feature_any, 0, nbytes) ;			    /* Make sure mem for struct is useless. */
  if (USE_SLICE_ALLOC)
    g_slice_free1(nbytes, feature_any) ;
  else
    g_free(feature_any) ;


  return ;
}







static void printDestroyDebugInfo(ZMapFeatureAny feature_any, char *who)
{
  int length = 0 ;

  if (feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    length = g_hash_table_size(feature_any->children) ;

  if(destroy_debug_G)
    zMapLogWarning("%s: (%p) '%s' datalist size %d", who, feature_any, g_quark_to_string(feature_any->unique_id), length) ;

  return ;
}





/* A GHRFunc() called by hash steal to remove items from hash _without_ calling the item destroy routine. */
static gboolean withdrawFeatureAny(gpointer key, gpointer value, gpointer user_data)
{
  gboolean remove = TRUE ;


  /* We want to remove all items so always return TRUE. */

  return remove ;
}



static ZMapFeatureContextExecuteStatus eraseContextCB(GQuark key,
                                                      gpointer data,
                                                      gpointer user_data,
                                                      char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  MergeContextData merge_data = (MergeContextData)user_data;
  ZMapFeatureAny  feature_any = (ZMapFeatureAny)data;
  ZMapFeatureAny erased_feature_any;
  ZMapFeature erased_feature;
  gboolean remove_status = FALSE;

  if(merge_debug_G)
    zMapLogWarning("checking %s", g_quark_to_string(feature_any->unique_id));

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      merge_data->current_view_align =
	(ZMapFeatureAny)(zMapFeatureContextGetAlignmentByID(merge_data->view_context,
							    key));
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      merge_data->current_view_block = zMapFeatureAnyGetFeatureByID(merge_data->current_view_align,
								    key);
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      merge_data->current_view_set   = zMapFeatureAnyGetFeatureByID(merge_data->current_view_block,
								    key);
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      /* look up in the current */

      if(merge_data->current_view_set &&
         (erased_feature_any = zMapFeatureAnyGetFeatureByID(merge_data->current_view_set, key)))
        {
	  erased_feature = (ZMapFeature)erased_feature_any;

          if(merge_debug_G)
            zMapLogWarning("%s","\tFeature in erase and current contexts...");

          /* insert into the diff context.
           * BUT, need to check if the parents exist in the diff context first.
           */
          if (!merge_data->current_diff_align)
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tno parent align... creating align in diff");

	      merge_data->current_diff_align
		= featureAnyCreateFeature(merge_data->current_view_align->struct_type,
					  NULL,
					  merge_data->current_view_align->original_id,
					  merge_data->current_view_align->unique_id,
					  NULL) ;

              zMapFeatureContextAddAlignment(merge_data->diff_context,
					     (ZMapFeatureAlignment)merge_data->current_diff_align,
					     FALSE);
            }
          if (!merge_data->current_diff_block)
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tno parent block... creating block in diff");

	      merge_data->current_diff_block
		= featureAnyCreateFeature(merge_data->current_view_block->struct_type,
					  NULL,
					  merge_data->current_view_block->original_id,
					  merge_data->current_view_block->unique_id,
					  NULL) ;

              zMapFeatureAlignmentAddBlock((ZMapFeatureAlignment)merge_data->current_diff_align,
					   (ZMapFeatureBlock)merge_data->current_diff_block);
            }
          if(!merge_data->current_diff_set)
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tno parent set... creating set in diff");

	      merge_data->current_diff_set
		= featureAnyCreateFeature(merge_data->current_view_set->struct_type,
					  NULL,
					  merge_data->current_view_set->original_id,
					  merge_data->current_view_set->unique_id,
					  NULL) ;

              zMapFeatureBlockAddFeatureSet((ZMapFeatureBlock)merge_data->current_diff_block,
					    (ZMapFeatureSet)merge_data->current_diff_set);
            }

          if(merge_debug_G)
            zMapLogWarning("%s","\tmoving feature from current to diff context ... removing ... and inserting.");

          /* remove from the current context.*/
          remove_status = zMapFeatureSetRemoveFeature((ZMapFeatureSet)merge_data->current_view_set,
						      erased_feature);
          zMapAssert(remove_status);

          zMapFeatureSetAddFeature((ZMapFeatureSet)merge_data->current_diff_set,
				   erased_feature);

#ifdef MESSES_UP_HASH
          /* destroy from the erase context.*/
          if(merge_debug_G)
            zMapLogWarning("%s","\tdestroying feature in erase context");

	  zMapFeatureDestroy(feature);
#endif /* MESSES_UP_HASH */

	  status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;
        }
      else
        {
          if(merge_debug_G)
            zMapLogWarning("%s","\tFeature absent from current context, nothing to do...");
          /* no ...
           * leave in the erase context.
           */
        }
      break;
    default:
      break;
    }

  return status ;
}

static ZMapFeatureContextExecuteStatus destroyIfEmptyContextCB(GQuark key,
                                                               gpointer data,
                                                               gpointer user_data,
                                                               char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  MergeContextData        merge_data = (MergeContextData)user_data;
  ZMapFeatureAny         feature_any = (ZMapFeatureAny)data;
  ZMapFeatureAlignment feature_align;
  ZMapFeatureBlock     feature_block;
  ZMapFeatureSet         feature_set;

  if(merge_debug_G)
    zMapLogWarning("checking %s", g_quark_to_string(feature_any->unique_id));

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      feature_align = (ZMapFeatureAlignment)feature_any;
      if (g_hash_table_size(feature_align->blocks) == 0)
        {
#ifdef MESSES_UP_HASH
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty align ... destroying");
          zMapFeatureAlignmentDestroy(feature_align, TRUE);
#endif /* MESSES_UP_HASH */
	  status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;
        }
      merge_data->current_diff_align =
        merge_data->current_view_align = NULL;
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      feature_block = (ZMapFeatureBlock)feature_any;
      if (g_hash_table_size(feature_block->feature_sets) == 0)
        {
#ifdef MESSES_UP_HASH
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty block ... destroying");
          zMapFeatureBlockDestroy(feature_block, TRUE);
#endif /* MESSES_UP_HASH */
	  status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;
        }
      merge_data->current_diff_block =
        merge_data->current_view_block = NULL;
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      feature_set = (ZMapFeatureSet)feature_any;
      if (g_hash_table_size(feature_set->features) == 0)
        {
#ifdef MESSES_UP_HASH
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty set ... destroying");
          zMapFeatureSetDestroy(feature_set, TRUE);
#endif /* MESSES_UP_HASH */
	  status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;
        }
      merge_data->current_diff_set =
        merge_data->current_view_set = NULL;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      break;
    default:
      break;
    }


  return status ;
}



/* combine loaded sequence region lists
 * almost always this is a complete load of a new featureset
 * so we just copy/move the list of one region as part of the payload of the featureset
 * but for requesting from mark we may have to munge the data
 */


void print_loaded(GList *l,char *fred)
{
      char buf[1024],*p = buf;
      ZMapSpan span;
      p += sprintf(p,"%s: ",fred);
      for(;l;l = l->next)
      {
            span = (ZMapSpan) l->data;
            p += sprintf(p," %d-%d (%p->%p)",span->x1,span->x2,l,span);
      }
      zMapLogWarning(buf,"");
}

static void mergeFeatureSetLoaded(ZMapFeatureSet view_set, ZMapFeatureSet new_set)
{
      GList *view_list,*new_list;
      ZMapSpan view_span,new_span,got_span;
      gboolean got = FALSE;

      if(view_set == new_set) /* loaded list transfered to view context, will not be freed */
            return;

if(!view_set || !new_set)
{
//      zMapLogWarning("merge: null set %p %p",view_set,new_set);
      return;
}
//zMapLogWarning("merge feature set loaded %s %s\n",g_quark_to_string(view_set->unique_id), //g_quark_to_string(new_set->unique_id));


      /* we expect to just add our seq region to the existing
       * may have to combine adjacent
       * and logically deal with overlaps, which should not happen
       *
       * new list can only have one region due to GFF constraints
       */

      view_list = view_set->loaded;
      new_list = new_set->loaded;


//      zMapAssert(view_list && new_list);  /* there must be features so there must be a region */
      if(!view_list)
      {
            view_set->loaded = new_set->loaded;
            zMapLogWarning("merge loaded %s had no loaded list", g_quark_to_string(view_set->unique_id));
      }
      if(!new_list)
      {
            zMapLogWarning("merge loaded %s had no loaded list", g_quark_to_string(new_set->unique_id));
            return;
      }

//print_loaded(view_set->loaded, "view list 1");
//print_loaded(new_set->loaded,  "new list  1");

      new_span = (ZMapSpan) new_list->data;
      if(new_list->next)
      {
            /* due to GFF format we can only have one region */
            zMapLogWarning("unexpected multiple region in new context ignored in featureset %s", g_quark_to_string(new_set->unique_id));
      }

/*
      this is disapointingly complex:

      how many cases? (13)

      | characters have no width
      x1 and x2 are inclusive

            |x1  x2|
            |vvvvvv|
       nnn  |      |
         nnn|      |
           n|nn    |
            |nnn   |
            |  nnn |
            |   nnn|
            |    nn|n
            |      |nnn
            |      |   nnn
            |nnnnnn|
         nnn|nnnnnn|
            |nnnnnn|nnn
        nnnn|nnnnnn|nnnn


      How many actions? (4)

      new    view  action

      end == x1-1  join
      end  < x1    insert before

      start <= x2+1 join

      try next, or insert after

      but we should also join up two view segemnts which implies processing twice:

        vvvv|     |vvvv
            |nnnnn|
      or:

       vvvvv|     |vvvvv|      |vvvvv
          nn|nnnnn|nnnnn|nnnnnn|nn

      so: insert or join once only, continue till end <= x2
*/

      while(view_list)
      {
            view_span = (ZMapSpan) view_list->data;

            if(got)     /* we are joining an already combined region */
            {
//printf("view,got = %d-%d, %d-%d\n",view_span->x1,view_span->x2,got_span->x1,got_span->x2);
                  if(got_span->x2 >= view_span->x1 - 1) /* join */
                  {
                        if(got_span->x2 < view_span->x2)
                              got_span->x2 = view_span->x2;
                  }

                  if(got_span->x2 >= view_span->x2)   /* remove then continue */
                  {
                        GList *del = view_list;

                        view_list = view_list->next;
                        g_free(view_span);
                        view_set->loaded = g_list_delete_link(view_set->loaded,del);
                  }
                  else /* there is a gap */
                  {
                        break;
                  }
            }
            else
            {
//zMapLogWarning("view,new = %d-%d, %d-%d",view_span->x1,view_span->x2,new_span->x1,new_span->x2);
                  if(new_span->x2 + 1 == view_span->x1)     /* join then we are finished */
                  {
                        view_span->x1 = new_span->x1;
                        got = TRUE;
                        break;
                  }
                  if(new_span->x2 < view_span->x1)          /* insert before then we are finished */
                  {
                        view_set->loaded = g_list_insert_before(view_set->loaded,view_list,
                              g_memdup(new_span,sizeof(ZMapSpanStruct)));
                        got = TRUE;
                        break;
                  }
                  if(new_span->x1 <= view_span->x2 + 1)   /* join then continue */
                  {
                        if(view_span->x1 > new_span->x1)
                              view_span->x1 = new_span->x1;
                        if(view_span->x2 < new_span->x2)
                              view_span->x2 = new_span->x2;
                        got = TRUE;
                  }
                  if(got)
                        got_span = view_span;

                  view_list = view_list->next;
            }
      }

      if(!got)
      {
            view_set->loaded = g_list_append(view_set->loaded, g_memdup(new_span,sizeof(ZMapSpanStruct)));
      }
//print_loaded(view_set->loaded, "view list 2");
//print_loaded(new_set->loaded,  "new list  2");

}


/* for most columns we just load the lot, so there will be a single range */
/* NOTE we find the featureset in the first align/block, as ATM there can only be one
 * the context is designed to have more and if so we need to supply a current block to this function
 * refer to 'Handling Multiple Blocks' in doc/Design_notes/modules/zmapWindowColConfig.shtml
 */
gboolean zMapFeatureSetIsLoadedInRange(ZMapFeatureBlock block,  GQuark unique_id,int start, int end)
{
      GList *l;
      ZMapFeatureSet fset;
      ZMapSpan span;

      fset = zMapFeatureBlockGetSetByID(block,unique_id);
      if(fset)
      {
//            if(!fset->loaded)
//            {
//                  zMapLogWarning("Featureset %s has no span",g_quark_to_string(unique_id));
//            }

            for(l = fset->loaded;l;l = l->next)
            {
                  span = l->data;
//                  zMapLogWarning("featureset span %d -> %d",span->x1,span->x2);
                  if(span->x2 < start)
                        continue;

                  if(span->x1 > start)
                        return FALSE;

                  if(!span->x2)     /* not real coordinates */
                  {
//                        zMapLogWarning("featureset %s has null coordinates", g_quark_to_string(fset->original_id));
                        return TRUE ;
                  }
                  if(span->x2 >= end)
                        return TRUE;

                  start  = span->x2 + 1;
            }
      }
      else
      {
            /* see NOTE (prefix) below */
//            printf("Featureset %s does not exist\n",g_quark_to_string(unique_id));
      }

      return FALSE;
}

#define MH17_DEBUG     0

/* add emtpy sets to block when not present in ref, to record successful requests w/ no data */
static void zmapFeatureBlockAddEmptySets(ZMapFeatureBlock ref, ZMapFeatureBlock block, GList *feature_set_names)
{
      GList *l;

#if MH17_DEBUG
      {
            ZMapFeatureAny feature_any = (ZMapFeatureAny) block;
            zMap_g_hash_table_get_keys(&l,feature_any->children);
            for(;l;l = l->next)
            {
                  zMapLogWarning("block has featureset %s\n",g_quark_to_string(GPOINTER_TO_UINT(l->data)));
            }
      }
#endif

      for(l  = feature_set_names;l;l = l->next)
      {
            ZMapFeatureSet old_set;
            GQuark set_id = GPOINTER_TO_UINT(l->data);

            char *set_name = (char *) g_quark_to_string(set_id);
            set_id = zMapFeatureSetCreateID(set_name);

            if(!(old_set = zMapFeatureBlockGetSetByID(ref, set_id)))
            {
                  if(!g_strstr_len(set_name,-1,":"))
                  {
                  /*
                        NOTE (prefix)
                        featuresets with a 'prefix:' name are known as stubs
                        and are very numerous (eg 1000) and normally contain no data
                        they come only from ACEDB and only update via XML from otterlace
                        so if they have no data we ignore them and do not create them
                        as ACEDB takes requests as columns then a subsequent request will work

                        NOTE it would be good not to have these featuresets in the columns featureset list
                        and also in the featureset to column list
                   */

                        ZMapFeatureSet feature_set;
                        ZMapSpan span;

                        feature_set = zMapFeatureSetCreate(set_name ,NULL);
                              /* record the region we are getting */
                        span = (ZMapSpan) g_new0(ZMapSpanStruct,1);
                        span->x1 = block->block_to_sequence.block.x1;
                        span->x2 = block->block_to_sequence.block.x2;
                        feature_set->loaded = g_list_append(NULL,span);
#if  MH17_DEBUG
zMapLogWarning("adding empty featureset %s (%d -> %d) to block, could not find %s\n", g_quark_to_string(feature_set->unique_id),span->x1,span->x2, set_name);
#endif

                        zMapFeatureBlockAddFeatureSet(block, feature_set);
                  }
            }
#if MH17_DEBUG
            else
            {
zMapLogWarning("using populated featureset %s\n",g_quark_to_string(old_set->unique_id));
            }
#endif
      }
}

static ZMapFeatureContextExecuteStatus addEmptySets(GQuark key,
                                                  gpointer data,
                                                  gpointer user_data,
                                                  char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  MergeContextData merge_data = (MergeContextData)user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;


  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break ;

    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_FEATURESET:
      break;

    case ZMAPFEATURE_STRUCT_BLOCK:
#if MH17_DEBUG
zMapLogWarning("addEmptySets","");
#endif

      zmapFeatureBlockAddEmptySets((ZMapFeatureBlock)(feature_any), (ZMapFeatureBlock)(feature_any), merge_data->req_featuresets);
      break;

    default:
      {
      zMapAssertNotReached() ;
      break;
      }
    }

  return status;
}



/* It's very important to note that the diff context hash tables _do_not_ have destroy functions,
 * this is what prevents them from freeing their children. */
static ZMapFeatureContextExecuteStatus mergePreCB(GQuark key,
                                                  gpointer data,
                                                  gpointer user_data,
                                                  char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  MergeContextData merge_data = (MergeContextData)user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  gboolean new = FALSE, children = FALSE ;
  ZMapFeatureAny *view_path_parent_ptr;
  ZMapFeatureAny *view_path_ptr = NULL;
  ZMapFeatureAny *diff_path_parent_ptr;
  ZMapFeatureAny *diff_path_ptr;

  /* THE CODE BELOW ALL NEEDS FACTORISING, START WITH THE SWITCH SETTING SOME
     COMMON POINTERS FOR THE FOLLOWING CODE..... */

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_FEATURE:
      view_path_parent_ptr = &(merge_data->current_view_set);
      view_path_ptr        = NULL;
      diff_path_parent_ptr = &(merge_data->current_diff_set);
      diff_path_ptr        = NULL;
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      view_path_parent_ptr = (ZMapFeatureAny *)(&(merge_data->view_context));
      view_path_ptr        = &(merge_data->current_view_align);
      diff_path_parent_ptr = (ZMapFeatureAny *)(&(merge_data->diff_context));
      diff_path_ptr        = &(merge_data->current_diff_align);
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      view_path_parent_ptr = &(merge_data->current_view_align);
      view_path_ptr        = &(merge_data->current_view_block);
      diff_path_parent_ptr = &(merge_data->current_diff_align);
      diff_path_ptr        = &(merge_data->current_diff_block);
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      view_path_parent_ptr = &(merge_data->current_view_block);
      view_path_ptr        = &(merge_data->current_view_set);
      diff_path_parent_ptr = &(merge_data->current_diff_block);
      diff_path_ptr        = &(merge_data->current_diff_set);
      break;
    default:
      zMapAssertNotReached();
    }


  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break ;

    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
	gboolean is_master_align = FALSE;

	/* Annoyingly we have a issue with alignments */
	if (feature_any->struct_type == ZMAPFEATURE_STRUCT_ALIGN)
	  {
	    ZMapFeatureContext context = (ZMapFeatureContext)(feature_any->parent);
	    if (context->master_align == (ZMapFeatureAlignment)feature_any)
	      {
		is_master_align = TRUE;
		context->master_align = NULL;
	      }
	  }

	/* general code start */

	zMapAssert(view_path_ptr && view_path_parent_ptr &&
		   diff_path_ptr && diff_path_parent_ptr);
	zMapAssert(*view_path_parent_ptr && *diff_path_parent_ptr);

	/* Only need to do anything if this level has children. */
	if (feature_any->children)
	  {
	    ZMapFeatureAny diff_feature_any;

	    children = TRUE;	/* We have children. */

	    /* This removes the link in the "from" feature to it's parent
	     * and the status makes sure it gets removed from the parent's
	     * hash on return. */
	    feature_any->parent = NULL;
	    status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;

	    if (!(*view_path_ptr = zMapFeatureAnyGetFeatureByID(*view_path_parent_ptr,
								feature_any->unique_id)))
	      {
		/* If its new we can simply copy a pointer over to the diff context
		 * and stop recursing.... */


		merge_data->new_features = new = TRUE;	/* This is a NEW feature. */

		/* We would use featureAnyAddFeature, but it does another
		 * g_hash_table_lookup... */
		g_hash_table_insert((*view_path_parent_ptr)->children,
				    zmapFeature2HashKey(feature_any),
				    feature_any);
		feature_any->parent = *view_path_parent_ptr;
		/* end of featureAnyAddFeature bit... */

		/* update the path */
		*view_path_ptr      = feature_any;

		diff_feature_any    = feature_any;
//printf("copying pointer to diff: %p\n",diff_feature_any);

		diff_feature_any->parent = *view_path_parent_ptr;

		status |= ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND;
	      }
	    else
	      {
		new = FALSE;	/* Nothing new here */
		/* If the feature is there we need to copy it and then recurse down until
		 * we get to the individual feature level. */
		diff_feature_any = zmapFeatureAnyCopy(feature_any, NULL);
//printf("creating copy for diff: %p, found = %s %d\n", diff_feature_any,g_quark_to_string((*view_path_ptr)->unique_id),(*view_path_ptr)->struct_type);

		featureAnyAddToDestroyList(merge_data->diff_context, diff_feature_any);
	      }
            // mh17:
            // 1) featureAnyAddFeature checks to see if it's there first, which we just did :-(
            // 2) look at the comment 25 lines above about not using featureAnyAddFeature
	    featureAnyAddFeature(*diff_path_parent_ptr, diff_feature_any);

	    /* update the path */
	    *diff_path_ptr = diff_feature_any;

	    /* keep diff feature -> parent up to date with the view parent */
	    if(new)
	      (*diff_path_ptr)->parent = *view_path_parent_ptr;

#if MH17_OLD_CODE
          if (feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK &&
            (*view_path_ptr)->unique_id == feature_any->unique_id)
            {
              ((ZMapFeatureBlock)(*view_path_ptr))->features_start =
                              ((ZMapFeatureBlock)(feature_any))->features_start;
              ((ZMapFeatureBlock)(*view_path_ptr))->features_end   =
                              ((ZMapFeatureBlock)(feature_any))->features_end;
            }
#else
// debugging reveals that features start and end are 1,0 in the new data and sensible in the old
// maybe 1,0 gets reprocessed into start,end?? so let's leave it as it used to run
// BUT: this code should only run if not a new block 'cos if it's new then we've just
// assigned the pointer
// this is a block which may hold a sequence, the dna featureset is another struct
	    if (!new && feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK &&
		(*view_path_ptr)->unique_id == feature_any->unique_id)
	      {
               ZMapFeatureBlock vptr = (ZMapFeatureBlock)(*view_path_ptr);
               ZMapFeatureBlock feat = (ZMapFeatureBlock) feature_any;

#if MH17_THIS_IS_WRONG_FOR_REQ_FROM_MARK
/* sub block gets merging into a bigger one which still has the original range
 * featuresets have a span which gets added to the list in the original context featureset
 */
	  	   vptr->block_to_sequence.block.x1 = feat->block_to_sequence.block.x1;
		   vptr->block_to_sequence.block.x2 = feat->block_to_sequence.block.x2;
#endif
              // mh17: DNA did not get merged from a non first source
              // by analogy with the above we need to copy the sequence too, in fact any Block only data
              // sequence contains a pointer to a (long) string - can we just copy it?
              // seems to work without double frees...
              if(!vptr->sequence.sequence)      // let's keep the first one, should be the same
              {
//                memcpy(&vptr->block_to_sequence,&feat->block_to_sequence,sizeof(ZMapMapBlockStruct));
                memcpy(&vptr->sequence,&feat->sequence,sizeof(ZMapSequenceStruct));
	        }
            }
#endif
	  }

	/* general code stop */

     if (feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK)
       {
       /* we get only featuresest with data from the server */
       /* and record empty ones with a seq region list of requested ranges in the features context */
         zmapFeatureBlockAddEmptySets((ZMapFeatureBlock) feature_any, (ZMapFeatureBlock)(*diff_path_ptr), merge_data->req_featuresets);
         zmapFeatureBlockAddEmptySets((ZMapFeatureBlock) feature_any, (ZMapFeatureBlock)(*view_path_ptr), merge_data->req_featuresets);
       }

      if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
        {
            /* if we have a featureset there will be features (see zmapGFF2parser.c/makeNewfeature())
             * so view_path_ptr is valid here
             */

            ZMapFeatureContext context = merge_data->diff_context;
//printf("adding %s (%p) with %d features to context %p\n", g_quark_to_string(feature_any->unique_id),*diff_path_ptr,g_hash_table_size((*diff_path_ptr)->children),context);

            context->src_feature_set_names = g_list_prepend(context->src_feature_set_names, GUINT_TO_POINTER(feature_any->unique_id));

            if(!new)
            {
                  mergeFeatureSetLoaded((ZMapFeatureSet)*view_path_ptr, (ZMapFeatureSet) feature_any);
            }
        }

#ifdef NO_IDEA_WHAT_SHOULD_HAPPEN_HERE
	/* possibly nothing... unsure where the master alignment status [will] comes from. */
	/* and finish the alignment */
	if(is_master_align && feature_any->struct_type == ZMAPFEATURE_STRUCT_ALIGN)
	  {
	    merge_data->diff_context->master_align = (ZMapFeatureAlignment)diff_feature_any;
	  }
#endif /* NO_IDEA_WHAT_SHOULD_HAPPEN_HERE */
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	zMapAssert(view_path_parent_ptr && *view_path_parent_ptr &&
		   diff_path_parent_ptr && *diff_path_parent_ptr);

	feature_any->parent = NULL;
	status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;

	if (!(zMapFeatureAnyGetFeatureByID(*view_path_parent_ptr, feature_any->unique_id)))
	  {
	    merge_data->new_features = new = TRUE ;
	    merge_data->feature_count++ ;

	    featureAnyAddFeature(*diff_path_parent_ptr, feature_any) ;

	    featureAnyAddFeature(*view_path_parent_ptr, feature_any) ;


	    if (merge_debug_G)
	      zMapLogWarning("feature(%p)->parent = %p. current_view_set = %p",
			     feature_any, feature_any->parent, *view_path_parent_ptr) ;
	  }
	else
	  {
	    new = FALSE ;
	  }
      }
      break;


    default:
      {
      zMapAssertNotReached() ;
      break;
      }
    }

  if (merge_debug_G)
    zMapLogWarning("%s (%p) '%s' is %s and has %s",
               zMapFeatureStructType2Str(feature_any->struct_type),
               feature_any,
               g_quark_to_string(feature_any->unique_id),
               (new == TRUE ? "new" : "old"),
               (children ? "children and was added" : "no children and was not added"));
#if 0
    printf("%s (%p) '%s' is %s and has %s\n",
               zMapFeatureStructType2Str(feature_any->struct_type),
               feature_any,
               g_quark_to_string(feature_any->unique_id),
               (new == TRUE ? "new" : "old"),
               (children ? "children and was added" : "no children and was not added"));
#endif


  return status;
}



/*
 *              Following functions all operate on any feature type,
 *              they were written to reduce duplication of code.
 */


/* Allocate a feature structure of the requested type, filling in the feature any fields. */
static ZMapFeatureAny featureAnyCreateFeature(ZMapFeatureStructType struct_type,
					      ZMapFeatureAny parent,
					      GQuark original_id, GQuark unique_id,
					      GHashTable *children)
{
  ZMapFeatureAny feature_any = NULL ;
  gulong nbytes ;

  switch(struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      nbytes = sizeof(ZMapFeatureContextStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      nbytes = sizeof(ZMapFeatureAlignmentStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      nbytes = sizeof(ZMapFeatureBlockStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      nbytes = sizeof(ZMapFeatureSetStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURE:
      nbytes = sizeof(ZMapFeatureStruct) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  if (USE_SLICE_ALLOC)
    feature_any = (ZMapFeatureAny)g_slice_alloc0(nbytes) ;
  else
    feature_any = (ZMapFeatureAny)g_malloc0(nbytes) ;

#ifdef FEATURES_NEED_MAGIC
  feature_any->magic = feature_magic_G;
#endif

  feature_any->struct_type = struct_type ;

  logMemCalls(TRUE, feature_any) ;

  if (struct_type != ZMAPFEATURE_STRUCT_CONTEXT)
    feature_any->parent = parent ;

  feature_any->original_id = original_id ;
  feature_any->unique_id = unique_id ;

  if (struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    {
      if (children)
	feature_any->children = children ;
      else
	feature_any->children = g_hash_table_new_full(NULL, NULL, NULL, destroyFeatureAny) ;
    }

  return feature_any ;
}



/* Hooks up the feature into the feature_sets children and makes feature_set the parent
 * of feature _if_ the feature is not already in feature_set. */
static gboolean featureAnyAddFeature(ZMapFeatureAny feature_any, ZMapFeatureAny feature)
{
  gboolean result = FALSE ;

  if (!zMapFeatureAnyFindFeature(feature_any, feature))
    {
      g_hash_table_insert(feature_any->children, zmapFeature2HashKey(feature), feature) ;

      feature->parent = feature_any ;

      result = TRUE ;
    }

  return result ;
}


static gboolean destroyFeatureAnyWithChildren(ZMapFeatureAny feature_any, gboolean free_children)
{
  gboolean result = TRUE ;

  zMapAssert(feature_any);

  /* I think this is equivalent to the below code.... */

  /* If we have a parent then remove ourselves from the parent. */
  if (result && feature_any->parent)
    {
      /* splice out the feature_any from parent */
      result = g_hash_table_steal(feature_any->parent->children, zmapFeature2HashKey(feature_any)) ;
    }

  /* If we have children but they should not be freed, then remove them before destroying the
   * feature, otherwise the children will be destroyed. */
  if (result && (feature_any->children && !free_children))
    {
      if ((g_hash_table_foreach_steal(feature_any->children,
				      withdrawFeatureAny, (gpointer)feature_any)))
	{
	  zMapAssert(g_hash_table_size(feature_any->children) == 0) ;
	  result = TRUE ;
	}
    }

  /* Now destroy the feature. */
  if (result)
    {
      destroyFeatureAny((gpointer)feature_any) ;
      result = TRUE ;
    }

  return result ;
}



static void featureAnyAddToDestroyList(ZMapFeatureContext context, ZMapFeatureAny feature_any)
{
  zMapAssert(context && feature_any) ;
//printf("add to destroy list %s\n",g_quark_to_string(feature_any->unique_id));

  g_hash_table_insert(context->elements_to_destroy, zmapFeature2HashKey(feature_any), feature_any) ;

  return ;
}


#if MH17_ADD_MODES
static ZMapFeatureContextExecuteStatus addModeCB(GQuark key_id,
						 gpointer data,
						 gpointer user_data,
						 char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  HackForForcingStyleMode hack = (HackForForcingStyleMode)user_data ;
  ZMapFeatureStructType feature_type ;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  feature_type = feature_any->struct_type ;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	break ;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        hack->feature_set = (ZMapFeatureSet)feature_any ;

	g_hash_table_foreach(hack->feature_set->features, addFeatureModeCB, hack) ;

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
	status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
	zMapAssertNotReached();
	break;
      }
    }

  return status ;
}



/* A GHashTableForeachFunc() to add a mode to the styles for all features in a set, note that
 * this is not efficient as we go through all features but we would need more information
 * stored in the feature set to avoid this as there may be several different types of
 * feature stored in a feature set.
 *
 * Note that I'm setting some other style data here because we need different default bumping
 * modes etc for different feature types....
 *
 *  */
static void addFeatureModeCB(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  HackForForcingStyleMode hack = (HackForForcingStyleMode)user_data;
  ZMapFeatureSet feature_set = NULL;
  ZMapFeatureTypeStyle style ;
  gboolean force = FALSE;


  style = zMapFindStyle(hack->styles, feature->style_id) ;
  zMapAssert(style) ;

  feature_set = hack->feature_set;
  force       = hack->force;

  if(force)
    {
      if(zMapStyleHasMode(style))
	g_warning("Force=TRUE and style '%s' has mode (Couldn't have used zMapFeatureAnyAddModesToStyles)", g_quark_to_string(feature->style_id));
      else
	g_warning("Force=TRUE and style '%s' has no mode (Could have used zMapFeatureAnyAddModesToStyles)", g_quark_to_string(feature->style_id));
    }

  if (force || !zMapStyleHasMode(style))
    {
      ZMapStyleMode mode ;

      switch (feature->type)
	{
	case ZMAPSTYLE_MODE_BASIC:
	  {
	    mode = ZMAPSTYLE_MODE_BASIC ;

	    if (g_ascii_strcasecmp(g_quark_to_string(zMapStyleGetID(style)), "GF_splice") == 0)
	      {
		  mode = ZMAPSTYLE_MODE_GLYPH ;

              // would like to patch legacy 3frame glyph here, but this function
              // does not get called from the ACE interafce
              // see zMapFeatureAnyAddModesToStyles() above: 'it's an ACEDB issue'
              // perhaps it's an _old_ ACEDB issue?
              // refer also to zMapStyleMakeDrawable()
	      }
	    break ;
	  }
	case ZMAPSTYLE_MODE_ALIGNMENT:
	  {
	    mode = ZMAPSTYLE_MODE_ALIGNMENT ;

	    /* Initially alignments should not be bumped. */
	    zMapStyleInitBumpMode(style, ZMAPBUMP_NAME_COLINEAR, ZMAPBUMP_UNBUMP) ;

	    break ;
	  }
	case ZMAPSTYLE_MODE_TRANSCRIPT:
	  {
	    mode = ZMAPSTYLE_MODE_TRANSCRIPT ;

	    /* We simply never want transcripts to bump. */
	    zMapStyleInitBumpMode(style, ZMAPBUMP_NAME_INTERLEAVE, ZMAPBUMP_NAME_INTERLEAVE) ;

	    /* We also never need them to be hidden when they don't bump the marked region. */
	    zMapStyleSetDisplay(style, ZMAPSTYLE_COLDISPLAY_SHOW) ;

	    break ;
	  }
	case ZMAPSTYLE_MODE_SEQUENCE:
	  mode = ZMAPSTYLE_MODE_SEQUENCE ;
	  break ;
	case ZMAPSTYLE_MODE_ASSEMBLY_PATH: // mh17: hoping this works...
	case ZMAPSTYLE_MODE_TEXT:
	case ZMAPSTYLE_MODE_GLYPH:
	case ZMAPSTYLE_MODE_GRAPH:
	  mode = feature->type;           /* grrrrr is this really correct? */
	  break;
	default:
	  zMapAssertNotReached() ;
	  break ;
	}

      zMapStyleSetMode(style, mode) ;
    }


  return ;
}
#endif

gboolean zMapFeatureAnyHasMagic(ZMapFeatureAny feature_any)
{
  gboolean has_magic = TRUE;

#ifdef FEATURES_NEED_MAGIC
  has_magic = ZMAP_MAGIC_IS_VALID(feature_magic_G, feature_any->magic);
#endif

  return has_magic;
}



static void logMemCalls(gboolean alloc, ZMapFeatureAny feature_any)
{
  char *func ;

  if (LOG_MEM_CALLS)
    {
      if (USE_SLICE_ALLOC)
	{
	  if (alloc)
	    func = "g_slice_alloc0" ;
	  else
	    func = "g_slice_free1" ;
	}
      else
	{
	  if (alloc)
	    func = "g_malloc" ;
	  else
	    func = "g_free" ;
	}

      zMapLogWarning("%s: %s at %p\n", func, zMapFeatureStructType2Str(feature_any->struct_type), feature_any) ;
    }

  return ;
}
