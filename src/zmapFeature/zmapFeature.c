/*  File: zmapViewFeatures.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Implements feature contexts, sets and features themselves.
 *              Includes code to create/merge/destroy contexts and sets.
 *              
 * Exported functions: See zmapView_P.h
 * HISTORY:
 * Last edited: Feb 21 16:53 2007 (rds)
 * Created: Fri Jul 16 13:05:58 2004 (edgrif)
 * CVS info:   $Id: zmapFeature.c,v 1.59 2007-02-21 17:32:44 rds Exp $
 *-------------------------------------------------------------------
 */

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
  ZMapFeatureContext current_context;
  ZMapFeatureContext servers_context;
  ZMapFeatureContext diff_context;

  ZMapFeatureAlignment current_current_align;
  ZMapFeatureBlock     current_current_block;
  ZMapFeatureSet       current_current_set;

  ZMapFeatureAlignment current_diff_align;
  ZMapFeatureBlock     current_diff_block;
  ZMapFeatureSet       current_diff_set;

  ZMapFeatureContextExecuteStatus status;
  gboolean copied_align, copied_block, copied_set;
  gboolean destroy_align, destroy_block, destroy_set, destroy_feature;

  /* sizes for the servers children */
  gint aligns, blocks, sets, features;
  GList *destroy_aligns_list, *destroy_blocks_list, 
    *destroy_sets_list, *destroy_features_list;
} MergeContextDataStruct, *MergeContextData;

typedef struct
{
  ZMapFeatureContext context;
  ZMapFeatureAlignment align;
  ZMapFeatureBlock     block;
} EmptyCopyDataStruct, *EmptyCopyData;

typedef struct
{
  int length;
}DataListLengthStruct, *DataListLength;


static void contextAddAlignment(ZMapFeatureContext   feature_context,
                                ZMapFeatureAlignment alignment, 
                                gboolean             master, 
                                GDestroyNotify       notify);
static void alignmentAddBlock(ZMapFeatureAlignment alignment, 
                              ZMapFeatureBlock     block,
                              GDestroyNotify       notify);
static void blockAddFeatureSet(ZMapFeatureBlock feature_block,
                               ZMapFeatureSet   feature_set, 
                               GDestroyNotify   notify);
static gboolean featuresetAddFeature(ZMapFeatureSet feature_set,
                                     ZMapFeature    feature,
                                     GDestroyNotify notify);

static void destroyFeatureAny(gpointer data) ;
static void destroyFeature(gpointer data) ;
static void destroyFeatureSet(gpointer data) ;
static void destroyBlock(gpointer data) ;
static void destroyAlign(gpointer data) ;
static void destroyContext(gpointer data) ;
static void withdrawFeatureAny(GQuark key_id, gpointer data, gpointer user_data) ;

/* datalist debug stuff */
static void getDataListLength(GQuark id, gpointer data, gpointer user_data);
static void printDestroyDebugInfo(ZMapFeatureAny any, GData *datalist, char *who);
static gboolean checkForPerfectAlign(GArray *gaps, unsigned int align_error) ;

static ZMapFeatureContextExecuteStatus destroyContextCB(GQuark key, 
                                                        gpointer data, 
                                                        gpointer user_data,
                                                        char **err_out);
static ZMapFeatureContextExecuteStatus mergeContextCB(GQuark key, 
                                                      gpointer data, 
                                                      gpointer user_data,
                                                      char **err_out);
static ZMapFeatureContextExecuteStatus emptyCopyCB(GQuark key, 
                                                   gpointer data, 
                                                   gpointer user_data,
                                                   char **err_out);
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
static ZMapFeatureContextExecuteStatus mergePostCB(GQuark key, 
                                                   gpointer data, 
                                                   gpointer user_data,
                                                   char **err_out);



static gboolean merge_debug_G   = TRUE;
static gboolean destroy_debug_G = TRUE;
static gboolean merge_use_safe_destroy = TRUE;

/* !
 * A set of functions for allocating, populating and destroying features.
 * The feature create and add data are in two steps because currently the required
 * use is to have a struct that may need to be filled in in several steps because
 * in some data sources the data comes split up in the datastream (e.g. exons in
 * GFF). If there is a requirement for the two bundled then it should be implemented
 * via a simple new "create and add" function that merely calls both the create and
 * add functions from below. */



/*!
 * A Blocks DNA
 */
gboolean zMapFeatureBlockDNA(ZMapFeatureBlock block,
                             char **seq_name_out, int *seq_len_out, char **sequence_out)
{
  gboolean result = FALSE;
  ZMapFeatureContext context = NULL;

  zMapAssert( block ) ;

  if(block->sequence.sequence && 
     block->sequence.type != ZMAPSEQUENCE_NONE &&
     block->sequence.type == ZMAPSEQUENCE_DNA  &&
     (context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)block,
                                                              ZMAPFEATURE_STRUCT_CONTEXT)))
    {
      if(seq_name_out)
        *seq_name_out = (char *)g_quark_to_string(context->sequence_name) ;
      if(seq_len_out)
        *seq_len_out  = block->sequence.length ;
      if(sequence_out)
        *sequence_out = block->sequence.sequence ;
      result = TRUE ;
    }

  return result;
}


/*!
 * Returns a single feature correctly intialised to be a "NULL" feature.
 * 
 * @param   void  None.
 * @return  ZMapFeature  A pointer to the new ZMapFeature.
 *  */
ZMapFeature zMapFeatureCreateEmpty(void)
{
  ZMapFeature feature ;

  feature = g_new0(ZMapFeatureStruct, 1) ;
  feature->struct_type = ZMAPFEATURE_STRUCT_FEATURE ;
  feature->unique_id = ZMAPFEATURE_NULLQUARK ;
  feature->db_id = ZMAPFEATUREID_NULL ;
  feature->type = ZMAPFEATURE_INVALID ;

  return feature ;
}


/* ==================================================================
 * Because the contents of this are quite a lot of work. Useful for
 * creating single features, but be warned that usually you will need
 * to keep track of uniqueness, so for large parser the GFF style of
 * doing things is better (assuming we get a fix for g_datalist!).
 * ==================================================================
 */
ZMapFeature zMapFeatureCreateFromStandardData(char *name, char *sequence, char *ontology,
                                              ZMapFeatureType feature_type, 
                                              ZMapFeatureTypeStyle style,
                                              int start, int end,
                                              gboolean has_score, double score,
                                              ZMapStrand strand, ZMapPhase phase)
{
  ZMapFeature feature = NULL;
  gboolean       good = FALSE;

  if((feature = zMapFeatureCreateEmpty()))
    {
      char *feature_name_id = NULL;
      if((feature_name_id = zMapFeatureCreateName(feature_type, name, strand,
                                                  start, end, 0, 0)) != NULL)
        {
          if((good = zMapFeatureAddStandardData(feature, feature_name_id, 
                                                name, sequence, ontology,
                                                feature_type, style,
                                                start, end, has_score, score,
                                                strand, phase)))
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
				    char *sequence, char *ontology,
				    ZMapFeatureType feature_type, ZMapFeatureTypeStyle style,
				    int start, int end,
				    gboolean has_score, double score,
				    ZMapStrand strand, ZMapPhase phase)
{
  gboolean result = FALSE ;

  /* Currently we don't overwrite features, they must be empty. */
  zMapAssert(feature) ;

  if (feature->unique_id == ZMAPFEATURE_NULLQUARK)
    {
      feature->unique_id = g_quark_from_string(feature_name_id) ;
      feature->original_id = g_quark_from_string(name) ;
      feature->type = feature_type ;
      feature->ontology = g_quark_from_string(ontology) ;
      feature->style = style ;
      feature->x1 = start ;
      feature->x2 = end ;
      feature->strand = strand ;
      feature->phase = phase ;
      if (has_score)
	{
	  feature->flags.has_score = 1 ;
	  feature->score = score ;
	}
    }

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
gboolean zMapFeatureAddTranscriptData(ZMapFeature feature,
				      gboolean cds, Coord cds_start, Coord cds_end,
				      gboolean start_not_found, ZMapPhase start_phase,
				      gboolean end_not_found,
				      GArray *exons, GArray *introns)
{
  gboolean result = FALSE ;

  zMapAssert(feature && feature->type == ZMAPFEATURE_TRANSCRIPT) ;

  if (cds)
    {
      feature->feature.transcript.flags.cds = 1 ;
      feature->feature.transcript.cds_start = cds_start ;
      feature->feature.transcript.cds_end = cds_end ;
    }

  if (start_not_found)
    {
      feature->feature.transcript.flags.start_not_found = 1 ;
      feature->feature.transcript.start_phase = start_phase ;
    }

  if (end_not_found)
    feature->feature.transcript.flags.end_not_found = 1 ;

  if (exons)
    feature->feature.transcript.exons = exons ;

  if (introns)
    feature->feature.transcript.introns = introns ;

  result = TRUE ;

  return result ;
}

/*!
 * Adds a single exon and/or intron to a feature which may be empty or may already have
 * some exons/introns.
 *  */
gboolean zMapFeatureAddTranscriptExonIntron(ZMapFeature feature,
					    ZMapSpanStruct *exon, ZMapSpanStruct *intron)
{
  gboolean result = FALSE ;

  zMapAssert(feature && feature->type == ZMAPFEATURE_TRANSCRIPT) ;

  if (exon)
    {
      if (!feature->feature.transcript.exons)
	feature->feature.transcript.exons = g_array_sized_new(FALSE, TRUE,
							      sizeof(ZMapSpanStruct), 30) ;

      g_array_append_val(feature->feature.transcript.exons, *exon) ;

      result = TRUE ;
    }
  else if (intron)
    {
      if (!feature->feature.transcript.introns)
	feature->feature.transcript.introns = g_array_sized_new(FALSE, TRUE,
								sizeof(ZMapSpanStruct), 30) ;

      g_array_append_val(feature->feature.transcript.introns, *intron) ;

      result = TRUE ;
    }

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

/*!
 * Adds homology data to a feature which may be empty or may already have partial features.
 *  */
gboolean zMapFeatureAddAlignmentData(ZMapFeature feature,
				     ZMapHomolType homol_type,
				     ZMapStrand target_strand, ZMapPhase target_phase,
				     int query_start, int query_end, int query_length,
				     GArray *gaps)
{
  gboolean result = TRUE ;				    /* Not used at the moment. */

  zMapAssert(feature && feature->type == ZMAPFEATURE_ALIGNMENT) ;

  feature->feature.homol.type = homol_type ;
  feature->feature.homol.target_strand = target_strand ;
  feature->feature.homol.target_phase = target_phase ;
  feature->feature.homol.y1 = query_start ;
  feature->feature.homol.y2 = query_end ;
  feature->feature.homol.length = query_length ;

  if (gaps)
    {
      zMapFeatureSortGaps(gaps) ;

      feature->feature.homol.align = gaps ;

      feature->feature.homol.flags.perfect = checkForPerfectAlign(feature->feature.homol.align,
								  feature->style->within_align_error) ;
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

  feature->url = g_strdup_printf(url) ;

  return result ;
}


/*!
 * Adds a Locus to the object.
 *  */
gboolean zMapFeatureAddLocus(ZMapFeature feature, GQuark locus_id)
{
  gboolean result = TRUE ;

  zMapAssert(feature && locus_id) ;

  feature->locus_id = locus_id ;

  return result ;
}


/*!
 * Returns the length of a feature. For a simple feature this is just (end - start + 1),
 * for transcripts and alignments the exons or blocks must be totalled up.
 * 
 * @param   feature      Feature for which length is required.
 * @return               nothing.
 *  */
int zMapFeatureLength(ZMapFeature feature)
{
  int length = 0 ;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

  length = (feature->x2 - feature->x1 + 1) ;

  if (feature->type == ZMAPFEATURE_TRANSCRIPT && feature->feature.transcript.exons)
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
  else if (feature->type == ZMAPFEATURE_ALIGNMENT && feature->feature.homol.align)
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
  ZMapFeatureSet feature_set;

  zMapAssert(feature) ;

  /* remove via parent! or if none and it's just standalone... just destroy it. */
  if((feature_set = (ZMapFeatureSet)(feature->parent)) &&
     zMapFeatureSetFindFeature(feature_set, feature))
    g_datalist_id_remove_data(&(feature_set->features), feature->unique_id);
  else
    destroyFeature(feature);

  return ;
}



/* Features can be NULL if there are no features yet..... */
ZMapFeatureSet zMapFeatureSetCreate(char *source, GData *features)
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

  feature_set->style = style ;

  return ;
}




/* Features can be NULL if there are no features yet.....
 * original_id  the original name of the feature set
 * unique_id    some derivation of the original name or otherwise unique id to identify this
 *              feature set. */
ZMapFeatureSet zMapFeatureSetIDCreate(GQuark original_id, GQuark unique_id,
				      ZMapFeatureTypeStyle style, GData *features)
{
  ZMapFeatureSet feature_set ;

  feature_set = g_new0(ZMapFeatureSetStruct, 1) ;
  feature_set->struct_type = ZMAPFEATURE_STRUCT_FEATURESET ;
  feature_set->unique_id = unique_id ;
  feature_set->original_id = original_id ;
  feature_set->style = style ;

  if (!features)
    g_datalist_init(&(feature_set->features)) ;
  else
    feature_set->features = features ;

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

  result = featuresetAddFeature(feature_set, feature, destroyFeatureAny);

  return result ;
}


/* Returns TRUE if the feature could be found in the feature_set, FALSE otherwise. */
gboolean zMapFeatureSetFindFeature(ZMapFeatureSet feature_set, 
                                   ZMapFeature    feature)
{
  gboolean result = FALSE ;
  gpointer stored = NULL ;
  zMapAssert(feature_set && feature) ;

  if ((stored = g_datalist_id_get_data(&(feature_set->features), feature->unique_id)) != NULL)
    result = TRUE ;

  return result ;
}

ZMapFeature zMapFeatureSetGetFeatureByID(ZMapFeatureSet feature_set, GQuark feature_id)
{
  ZMapFeature feature ;

  feature = g_datalist_id_get_data(&(feature_set->features), feature_id) ;

  return feature ;
}

/* Feature must exist in set to be removed.
 * 
 * Returns FALSE if feature is not in set.
 *  */
gboolean zMapFeatureSetRemoveFeature(ZMapFeatureSet feature_set, 
                                     ZMapFeature    feature)
{
  gboolean result = FALSE ;

  zMapAssert(feature_set && feature && feature->unique_id != ZMAPFEATURE_NULLQUARK) ;

  if (zMapFeatureSetFindFeature(feature_set, feature))
    {
      g_datalist_id_remove_no_notify(&(feature_set->features), feature->unique_id) ;
      feature->parent = NULL;
      result = TRUE ;
    }

  return result ;
}


void zMapFeatureSetDestroy(ZMapFeatureSet feature_set, gboolean free_data)
{
  ZMapFeatureBlock feature_block;

  zMapAssert(feature_set);

  if((feature_block = (ZMapFeatureBlock)(feature_set->parent)))
    {
      /* Delete the feature list, freeing the features if required. */
      if (!free_data)
        {
          /* splice out the feature set from the block */
          g_datalist_id_remove_no_notify(&(feature_block->feature_sets), feature_set->unique_id);
          
          /* Unfortunately there is no "clear but don't free the data call" so have to do it
           * the long way by removing each item but _not_ calling our destructor function. */
          g_datalist_foreach(&(feature_set->features), withdrawFeatureAny, (gpointer)feature_set) ;
          
          /* We need to clear up our own memory still... */
          destroyFeatureAny((gpointer)feature_set);
        }
      else
        {
          /* Just remove from parent datalist destroyFeatureAny [GDestroyNotify] does the rest. */
          g_datalist_id_remove_data(&(feature_block->feature_sets), feature_set->unique_id);
        }
    }
  else if(!free_data)
    {
      g_datalist_foreach(&(feature_set->features), withdrawFeatureAny, (gpointer)feature_set) ;
      destroyFeatureAny((gpointer)feature_set);
    }
  else
    destroyFeatureAny((gpointer)feature_set);

  return ;
}

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

  alignment              = g_new0(ZMapFeatureAlignmentStruct, 1) ;

  alignment->struct_type = ZMAPFEATURE_STRUCT_ALIGN ;

  alignment->original_id = g_quark_from_string(align_name) ;

  alignment->unique_id   = zMapFeatureAlignmentCreateID(align_name, master_alignment) ;

  return alignment ;
}


void zMapFeatureAlignmentAddBlock(ZMapFeatureAlignment alignment, ZMapFeatureBlock block)
{

  alignmentAddBlock(alignment, block, destroyFeatureAny);

  return ;
}

gboolean zMapFeatureAlignmentFindBlock(ZMapFeatureAlignment feature_align, 
                                       ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;
  GList *stored   = NULL;
  zMapAssert(feature_align && feature_block);

  if((stored = g_datalist_id_get_data(&(feature_align->blocks), feature_block->unique_id)))
    result = TRUE;

  return result;
}

ZMapFeatureBlock zMapFeatureAlignmentGetBlockByID(ZMapFeatureAlignment feature_align, GQuark block_id)
{
  ZMapFeatureBlock feature_block = NULL;
  
  feature_block = g_datalist_id_get_data(&(feature_align->blocks), block_id) ;

  return feature_block ;
}


gboolean zMapFeatureAlignmentRemoveBlock(ZMapFeatureAlignment feature_align,
                                         ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;

  if(zMapFeatureAlignmentFindBlock(feature_align, feature_block))
    {
      g_datalist_id_remove_no_notify(&(feature_align->blocks), 
                                     feature_block->unique_id);
      feature_block->parent = NULL;
      result = TRUE;
    }

  return result;
}

void zMapFeatureAlignmentDestroy(ZMapFeatureAlignment alignment, gboolean free_data)
{
  ZMapFeatureContext context;

  zMapAssert(alignment);

  if((context = (ZMapFeatureContext)(alignment->parent)))
    {
      if(!free_data)
        {
          g_datalist_id_remove_no_notify(&(context->alignments), alignment->unique_id);
          
          g_datalist_foreach(&(alignment->blocks), withdrawFeatureAny, (gpointer)alignment);
          
          destroyFeatureAny((gpointer)alignment);
        }
      else
        {
          g_datalist_id_remove_data(&(context->alignments), alignment->unique_id);
        }
    }
  else if(!free_data)
    {
      g_datalist_foreach(&(alignment->blocks), withdrawFeatureAny, (gpointer)alignment);

      destroyFeatureAny((gpointer)alignment);
    }
  else
    destroyFeatureAny((gpointer)alignment);

  return ;
}



ZMapFeatureBlock zMapFeatureBlockCreate(char *block_seq,
					int ref_start, int ref_end, ZMapStrand ref_strand,
					int non_start, int non_end, ZMapStrand non_strand)
{
  ZMapFeatureBlock new_block ;

  zMapAssert((ref_strand == ZMAPSTRAND_FORWARD || ref_strand == ZMAPSTRAND_REVERSE)
	     && (non_strand == ZMAPSTRAND_FORWARD || non_strand == ZMAPSTRAND_REVERSE)) ;


  new_block = g_new0(ZMapFeatureBlockStruct, 1) ;

  new_block->struct_type = ZMAPFEATURE_STRUCT_BLOCK ;

  new_block->unique_id = zMapFeatureBlockCreateID(ref_start, ref_end, ref_strand,
                                                  non_start, non_end, non_strand
                                                  );
  /* Use the sequence name for the original_id */
  new_block->original_id = g_quark_from_string(block_seq) ; 

  new_block->block_to_sequence.t1 = ref_start ;
  new_block->block_to_sequence.t2 = ref_end ;
  new_block->block_to_sequence.t_strand = ref_strand ;

  new_block->block_to_sequence.q1 = non_start ;
  new_block->block_to_sequence.q2 = non_end ;
  new_block->block_to_sequence.q_strand = non_strand ;

  g_datalist_init(&(new_block->feature_sets));

  return new_block ;
}



void zMapFeatureBlockAddFeatureSet(ZMapFeatureBlock feature_block, 
                                   ZMapFeatureSet   feature_set)
{
  blockAddFeatureSet(feature_block, feature_set, destroyFeatureAny);

  return ;
}

gboolean zMapFeatureBlockFindFeatureSet(ZMapFeatureBlock feature_block,
                                        ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE;
  gpointer stored;
  zMapAssert(feature_set && feature_block);

  if((stored = g_datalist_id_get_data(&(feature_block->feature_sets), feature_set->unique_id)))
    result = TRUE;

  return result;
}

ZMapFeatureSet zMapFeatureBlockGetSetByID(ZMapFeatureBlock feature_block, GQuark set_id)
{
  ZMapFeatureSet feature_set ;

  feature_set = g_datalist_id_get_data(&(feature_block->feature_sets), set_id) ;

  return feature_set ;
}


gboolean zMapFeatureBlockRemoveFeatureSet(ZMapFeatureBlock feature_block, 
                                          ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE;

  if(zMapFeatureBlockFindFeatureSet(feature_block, feature_set))
    {
      g_datalist_id_remove_no_notify(&(feature_block->feature_sets), 
                                     feature_set->unique_id);
      feature_set->parent = NULL;
      result = TRUE;
    }

  return result;
}

void zMapFeatureBlockDestroy(ZMapFeatureBlock block, gboolean free_data)
{
  ZMapFeatureAlignment align;

  zMapAssert(block);
  
  if((align = (ZMapFeatureAlignment)(block->parent)))
    {
      if(!free_data)
        {
          /* GList stuff ... recreate the logic of g_datalist_id_remove_no_notify */
          g_datalist_id_remove_no_notify(&(align->blocks), block->unique_id);
          //removeBlockFromAlignsList(block, align);

          /* empty the datalist, without destroying lower level objects... */
          g_datalist_foreach(&(block->feature_sets), withdrawFeatureAny, (gpointer)block);

          destroyFeatureAny((gpointer)block);
        }
      else
        {
          /* GList stuff ... recreate the logic of g_datalist_id_remove_data */
          //clearBlockFromAlignsList(block, align);
          g_datalist_id_remove_data(&(align->blocks), block->unique_id);
        }
    }
  else if(!free_data)
    {
      g_datalist_foreach(&(block->feature_sets), withdrawFeatureAny, (gpointer)block);
      destroyFeatureAny((gpointer)block);
    }
  else
    destroyFeatureAny((gpointer)block);

  return ;
}

ZMapFeatureContext zMapFeatureContextCreate(char *sequence, int start, int end,
					    GList *types, GList *set_names)
{
  ZMapFeatureContext feature_context ;

  feature_context = g_new0(ZMapFeatureContextStruct, 1) ;

  feature_context->struct_type = ZMAPFEATURE_STRUCT_CONTEXT ;

  if (sequence && *sequence)
    {
      feature_context->unique_id = feature_context->original_id
	= feature_context->sequence_name = g_quark_from_string(sequence) ;
      feature_context->sequence_to_parent.c1 = start ;
      feature_context->sequence_to_parent.c2 = end ;
    }

  feature_context->styles = types ;
  feature_context->feature_set_names = set_names ;

  g_datalist_init(&(feature_context->alignments)) ;

  return feature_context ;
}

ZMapFeatureContext zMapFeatureContextCreateEmptyCopy(ZMapFeatureContext feature_context)
{
  EmptyCopyDataStruct   empty_data = {NULL};
  ZMapFeatureContext empty_context = NULL;

  if((empty_data.context = zMapFeatureContextCreate(NULL, 0, 0, NULL, NULL)))
    {
      char *tmp;
      tmp = g_strdup_printf("HACK in %s:%d", __FILE__, __LINE__);
      empty_data.context->unique_id =
        empty_data.context->original_id = g_quark_from_string(tmp);
      g_free(tmp);

      empty_data.context->parent_span        = feature_context->parent_span; /* struct copy */
      empty_data.context->sequence_to_parent = feature_context->sequence_to_parent; /* struct copy */
      empty_data.context->length             = feature_context->length;

      zMapFeatureContextExecute((ZMapFeatureAny)feature_context,
                                ZMAPFEATURE_STRUCT_FEATURESET,
                                emptyCopyCB,
                                &empty_data);
      
      empty_context = empty_data.context;
    }

  return empty_context;
}

void zMapFeatureContextAddAlignment(ZMapFeatureContext feature_context,
				    ZMapFeatureAlignment alignment, gboolean master)
{

  contextAddAlignment(feature_context, alignment, master, destroyFeatureAny);

  return ;
}

gboolean zMapFeatureContextFindAlignment(ZMapFeatureContext   feature_context,
                                         ZMapFeatureAlignment feature_align)
{
  gboolean result = FALSE;
  gpointer stored;
  zMapAssert(feature_context && feature_align );

  if((stored = g_datalist_id_get_data(&(feature_context->alignments), 
                                      feature_align->unique_id)))
    result = TRUE;

  return result;  
}

ZMapFeatureAlignment zMapFeatureContextGetAlignmentByID(ZMapFeatureContext feature_context, 
                                                        GQuark align_id)
{
  ZMapFeatureAlignment feature_align ;

  feature_align = g_datalist_id_get_data(&(feature_context->alignments), align_id) ;

  return feature_align ;
}


gboolean zMapFeatureContextRemoveAlignment(ZMapFeatureContext feature_context,
                                           ZMapFeatureAlignment feature_alignment)
{
  gboolean result = FALSE;

  zMapAssert(feature_context && feature_alignment);

  if(zMapFeatureContextFindAlignment(feature_context, feature_alignment))
    {
      g_datalist_id_remove_no_notify(&(feature_context->alignments), 
                                     feature_alignment->unique_id);
      feature_alignment->parent = NULL;

      if(feature_context->master_align == feature_alignment)
        feature_context->master_align = NULL; 
      /* This should find the first alignment it can, rather than leave it hanging */

      result = TRUE;
    }

  return result;
}

#ifdef RDS_USE_MERGE
/* Merge two feature contexts, the contexts must be for the same sequence.
 * The merge works by:
 *
 * - Feature sets and/or features in new_context that already exist in current_context
 *   are simply removed from new_context.
 * 
 * - Feature sets and/or features in new_context that do not exist in current_context
 *   are transferred from new_context to current_context and pointers to these new
 *   features/sets are returned in diff_context.
 *
 * Hence at the start of the merge we have:
 * 
 * current_context  - contains the current feature sets and features
 *     new_context  - contains the new feature sets and features (some of which
 *                    may already be in the current_context)
 *    diff_context  - is empty
 * 
 * and at the end of the merge we will have:
 * 
 * current_context  - contains the merge of all feature sets and features
 *     new_context  - is now empty [NEEDS destroying though]
 *    diff_context  - contains just the new features sets and features that
 *                    were added to current_context by this merge.
 * 
 * We return diff_context so that the caller has the option to update its display
 * by simply adding the features in diff_context rather than completely redrawing
 * with current_context which may be very expensive.
 * 
 * If called with current_context_inout == NULL (i.e. there are no "current
 * features"), current_context_inout just gets set to new_context and
 * diff_context will be NULL as there are no differences.
 * 
 * NOTE that diff_context may also be NULL if there are no differences
 * between the current and new contexts.
 * 
 * NOTE that diff_context does _not_ have its own copies of features, it points
 * at the ones in current_context so anything you do to diff_context will be
 * reflected in current_context. In particular you should free diff_context
 * using the free context function provided in this interface.
 * 
 * 
 * Returns TRUE if the merge was successful, FALSE otherwise. */
gboolean zMapFeatureContextMergeWithDiff(ZMapFeatureContext *current_context_inout,
                                         ZMapFeatureContext new_context,
                                         ZMapFeatureContext *diff_context_out)
{
  gboolean result = FALSE ;
  ZMapFeatureContext current_context ;
  ZMapFeatureContext diff_context ;

  zMapAssert(current_context_inout && new_context && diff_context_out) ;

  current_context = *current_context_inout ;

  /* If there are no current features we just return the new ones and the diff is
   * set to NULL, otherwise we need to do a merge of the new and current feature sets. */
  if (!current_context)
    {
      if(merge_debug_G)
        zMapLogWarning("%s", "No current context, returning complete new...");
      current_context = new_context ;
      diff_context = NULL ;

      result = TRUE ;
    }
  else
    {
      /* Here we need to merge for all alignments and all blocks.... */
      MergeContextDataStruct merge_data = {NULL};
      char *diff_context_string  = NULL;

      merge_data.current_context = current_context;
      merge_data.servers_context = new_context;
      merge_data.diff_context    = diff_context = zMapFeatureContextCreate(NULL, 0, 0, NULL, NULL);
      merge_data.status          = ZMAP_CONTEXT_EXEC_STATUS_OK;
      
      diff_context->feature_set_names = new_context->feature_set_names;
      current_context->feature_set_names = g_list_concat(current_context->feature_set_names,
                                                         new_context->feature_set_names);

      /* Set the original and unique ids so that the context passes the feature validity checks */
      diff_context_string = g_strdup_printf("%s vs %s\n", 
                                            g_quark_to_string(current_context->unique_id),
                                            g_quark_to_string(new_context->unique_id));
      diff_context->original_id = 
        diff_context->unique_id = g_quark_from_string(diff_context_string);

      g_datalist_init(&(diff_context->alignments));
      g_free(diff_context_string);
      if(merge_debug_G)
        zMapLogWarning("%s", "merging ...");

      zMapFeatureContextExecuteComplete((ZMapFeatureAny)new_context, ZMAPFEATURE_STRUCT_FEATURE,
                                        mergeContextCB, destroyContextCB, &merge_data);

      if(merge_debug_G)
        zMapLogWarning("%s", "finished ...");

      /* Something's wrong if there is still a master alignment */
      if(new_context->master_align && 0)
        zMapAssertNotReached();

      if(merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
        result = TRUE;
    }

  if (result)
    {
      *current_context_inout = current_context ;
      *diff_context_out = diff_context ;
      if(diff_context)
        *diff_context_out = new_context;
    }

  return result ;
}
#endif /* RDS_USE_MERGE */

gboolean zMapFeatureContextMerge(ZMapFeatureContext *current_context_inout,
                                 ZMapFeatureContext new_context)
{
  gboolean result = FALSE ;
  ZMapFeatureContext current_context ;

  zMapAssert(current_context_inout && new_context) ;

  current_context = *current_context_inout ;

  /* If there are no current features we just return the new ones and the diff is
   * set to NULL, otherwise we need to do a merge of the new and current feature sets. */
  if (!current_context)
    {
      if(merge_debug_G)
        zMapLogWarning("%s", "No current context, returning complete new...");
      current_context = new_context ;
      result = TRUE ;
    }
  else
    {
      /* Here we need to merge for all alignments and all blocks.... */
      MergeContextDataStruct merge_data = {NULL};

      merge_data.current_context = current_context;
      merge_data.servers_context = new_context;
      merge_data.status          = ZMAP_CONTEXT_EXEC_STATUS_OK;
      
      current_context->feature_set_names = g_list_concat(current_context->feature_set_names,
                                                         new_context->feature_set_names);


      if(merge_debug_G)
        zMapLogWarning("%s", "merging ...");

      zMapFeatureContextExecuteComplete((ZMapFeatureAny)new_context, ZMAPFEATURE_STRUCT_FEATURE,
                                        mergePreCB, mergePostCB, &merge_data);

      if(merge_debug_G)
        zMapLogWarning("%s", "finished ...");

      if(merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
        result = TRUE;
    }

  if (result)
    {
      *current_context_inout = current_context ;
    }

  return result ;
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

  zMapAssert(current_context_inout && remove_context && diff_context_out) ;

  current_context = *current_context_inout ;
  
  merge_data.current_context = current_context;
  merge_data.servers_context = remove_context;
  merge_data.diff_context    = diff_context = zMapFeatureContextCreate(NULL, 0, 0, NULL, NULL);
  merge_data.status          = ZMAP_CONTEXT_EXEC_STATUS_OK;
  
  diff_context->feature_set_names = remove_context->feature_set_names;
  current_context->feature_set_names = g_list_concat(current_context->feature_set_names,
                                                     remove_context->feature_set_names);
  
  /* Set the original and unique ids so that the context passes the feature validity checks */
  diff_context_string = g_strdup_printf("%s vs %s\n", 
                                        g_quark_to_string(current_context->unique_id),
                                        g_quark_to_string(remove_context->unique_id));
  diff_context->original_id = 
    diff_context->unique_id = g_quark_from_string(diff_context_string);

  g_datalist_init(&(diff_context->alignments));
  
  g_free(diff_context_string);
  
  zMapFeatureContextExecuteComplete((ZMapFeatureAny)remove_context, ZMAPFEATURE_STRUCT_FEATURE,
                                    eraseContextCB, destroyIfEmptyContextCB, &merge_data);
  

  if(merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
    erased = TRUE;

  if (erased)
    {
      *current_context_inout = current_context ;
      *diff_context_out = diff_context ;
    }

  return erased;
}

void zMapFeatureContextDestroy(ZMapFeatureContext feature_context, gboolean free_data)
{

  if(free_data)
    {
      g_datalist_clear(&(feature_context->alignments));
    }
  feature_context->master_align = NULL;

  g_free(feature_context);

  return ;
}


/*! @} end of zmapfeatures docs. */



/* 
 *            Internal routines. 
 */

/* internal add functions... 
 * When notify == NULL child will be loosely attached to parent. 
 * i.e. child->parent will not be changed and no destroy notify is set. */
static void contextAddAlignment(ZMapFeatureContext   feature_context,
                                ZMapFeatureAlignment alignment, 
                                gboolean             master, 
                                GDestroyNotify       notify)
{
  zMapAssert(feature_context && alignment) ;

  if(!zMapFeatureContextFindAlignment(feature_context, alignment))
    {
      if(notify)
        alignment->parent = (ZMapFeatureAny)feature_context ;
      
      g_datalist_id_set_data_full(&(feature_context->alignments), 
                                  alignment->unique_id, 
                                  alignment, notify) ;

      if (master)
        feature_context->master_align = alignment ;
    }

  return ;
}

static void alignmentAddBlock(ZMapFeatureAlignment alignment, 
                              ZMapFeatureBlock     block,
                              GDestroyNotify       notify)
{
  zMapAssert(alignment && block) ;

  if(!zMapFeatureAlignmentFindBlock(alignment, block))
    {
      if(notify)
        block->parent = (ZMapFeatureAny)alignment;
      
      g_datalist_id_set_data_full(&(alignment->blocks), 
                                  block->unique_id, 
                                  block, notify) ;

    }

  return ;
}

static void blockAddFeatureSet(ZMapFeatureBlock feature_block,
                               ZMapFeatureSet   feature_set, 
                               GDestroyNotify   notify)
{
  zMapAssert(feature_block && feature_set && feature_set->unique_id) ;

  if(!zMapFeatureBlockFindFeatureSet(feature_block, feature_set))
    {
      if(notify)
        feature_set->parent = (ZMapFeatureAny)feature_block ;
      
      g_datalist_id_set_data_full(&(feature_block->feature_sets), 
                                  feature_set->unique_id, 
                                  feature_set, notify) ;
    }

  return ;
}

static gboolean featuresetAddFeature(ZMapFeatureSet feature_set,
                                     ZMapFeature    feature,
                                     GDestroyNotify notify)
{
  gboolean result = FALSE;

  zMapAssert(feature_set && feature && feature->unique_id != ZMAPFEATURE_NULLQUARK) ;

  if (!zMapFeatureSetFindFeature(feature_set, feature))
    {
      if(notify)
        feature->parent = (ZMapFeatureAny)feature_set ;

      g_datalist_id_set_data_full(&(feature_set->features), 
                                  feature->unique_id, 
                                  feature, notify);

      result = TRUE ;
    }
  return result;
}

static void swap_parents(GQuark key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data;
  ZMapFeatureAny swap = (ZMapFeatureAny)user_data;

  feature->parent = swap;

  return ;
}

static void features_swap_parents(ZMapFeatureSet from_set, ZMapFeatureSet to_set)
{
  g_datalist_clear(&(to_set->features));

  to_set->features = from_set->features;

  g_datalist_foreach(&(from_set->features), swap_parents, to_set);

  from_set->features = NULL;

  return ;
}


/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear () and also if g_datalist_id_remove_data()
 * is called. */
static void destroyFeature(gpointer data)
{
  ZMapFeature feature = (ZMapFeature)data ;

  if(destroy_debug_G)
    zMapLogWarning("%s: (%p) '%s'", __PRETTY_FUNCTION__, feature, g_quark_to_string(feature->unique_id));

  if (feature->url)
    g_free(feature->url) ;

  if(feature->text)
    g_free(feature->text) ;

  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      if (feature->feature.transcript.exons)
	g_array_free(feature->feature.transcript.exons, TRUE) ;

      if (feature->feature.transcript.introns)
	g_array_free(feature->feature.transcript.introns, TRUE) ;
    }
  else if (feature->type == ZMAPFEATURE_ALIGNMENT)
    {
      if (feature->feature.homol.align)
	g_array_free(feature->feature.homol.align, TRUE) ;
    }

  /* We could memset to zero the feature struct for safety here.... */
  g_free(feature) ;

  return ;
}

static void getDataListLength(GQuark id, gpointer data, gpointer user_data)
{
  DataListLength length = (DataListLength)user_data;

  length->length++;

  return ;
}

static void printDestroyDebugInfo(ZMapFeatureAny any, GData *datalist, char *who)
{
  DataListLengthStruct length = {0};

  g_datalist_foreach(&datalist, getDataListLength, &length);

  if(destroy_debug_G)
    zMapLogWarning("%s: (%p) '%s' datalist size %d", who, any, g_quark_to_string(any->unique_id), length.length);

  return ;
}

/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear () and also if g_datalist_id_remove_data()
 * is called. */
static void destroyFeatureSet(gpointer data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  
  if(destroy_debug_G)
    printDestroyDebugInfo((ZMapFeatureAny)feature_set, feature_set->features, __PRETTY_FUNCTION__);

  g_datalist_clear(&(feature_set->features)) ;

  g_free(feature_set) ;

  return ;
}

static void destroyBlock(gpointer block_data)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)block_data;

  if(destroy_debug_G)
    printDestroyDebugInfo((ZMapFeatureAny)block, block->feature_sets, __PRETTY_FUNCTION__);

  g_datalist_clear(&(block->feature_sets));

  g_free(block);

  return ;
}

static void destroyAlign(gpointer align_data)
{
  ZMapFeatureAlignment align = (ZMapFeatureAlignment)align_data;
  ZMapFeatureContext context;

  if((context = (ZMapFeatureContext)(align->parent)) &&
     (context->master_align == align))
    {
      context->master_align = NULL; 
    }

  if(destroy_debug_G)
    printDestroyDebugInfo((ZMapFeatureAny)align, align->blocks, __PRETTY_FUNCTION__);
  
  g_datalist_clear(&(align->blocks));

  g_free(align);

  return ;
}

static void destroyContext(gpointer context_data)
{
  ZMapFeatureContext context = (ZMapFeatureContext)context_data;

  if(destroy_debug_G)
    printDestroyDebugInfo((ZMapFeatureAny)context, context->alignments, __PRETTY_FUNCTION__);

  g_datalist_clear(&(context->alignments));

  g_free(context);

  return ;
}

static void destroyFeatureAny(gpointer any_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)any_data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      destroyContext(feature_any);
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      destroyAlign(feature_any);
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      destroyBlock(feature_any);
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      destroyFeatureSet(feature_any);
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      destroyFeature(feature_any);
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  return ;
}

/* This is GDataForeachFunc(), it is called to remove all the elements of the datalist
 * without removing the corresponding Feature data. */
static void withdrawFeatureAny(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)user_data;
  GData   *children_datalist = NULL;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
        ZMapFeatureContext context = (ZMapFeatureContext)feature_any;
        children_datalist = context->alignments;
      }
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment align = (ZMapFeatureAlignment)feature_any;
        children_datalist = align->blocks;
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock block = (ZMapFeatureBlock)feature_any;
        children_datalist = block->feature_sets;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = (ZMapFeatureSet)feature_any;
        children_datalist = feature_set->features;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      /* Nothing to do! */
      break;
    default:
      zMapAssertNotReached();
      break;
    } 

  if(children_datalist)
    g_datalist_id_remove_no_notify(&(children_datalist), key_id) ;

  return ;
}

/* Returns TRUE if the target blocks match coords are within align_error bases of each other, if
 * there are less than two blocks then FALSE is returned.
 * 
 * Sometimes, for reasons I don't understand, its possible to have two butting matches, i.e. they
 * should really be one continuous match. It may be that this happens at a clone boundary, I don't
 * try to correct this because really its a data entry problem.
 * 
 *  */
static gboolean checkForPerfectAlign(GArray *gaps, unsigned int align_error)
{
  gboolean perfect_align = FALSE ;
  int i ;
  ZMapAlignBlock align, last_align ;

  if (gaps->len > 1)
    {
      perfect_align = TRUE ;
      last_align = &g_array_index(gaps, ZMapAlignBlockStruct, 0) ;

      for (i = 1 ; i < gaps->len ; i++)
	{
	  int prev_end, curr_start ;

	  align = &g_array_index(gaps, ZMapAlignBlockStruct, i) ;


	  /* The gaps array gets sorted by target coords, this can have the effect of reversing
	     the order of the query coords if the match is to the reverse strand of the target sequence. */
	  if (align->q2 < last_align->q1)
	    {
	      prev_end = align->q2 ;
	      curr_start = last_align->q1 ;
	    }
	  else
	    {
	      prev_end = last_align->q2 ;
	      curr_start = align->q1 ;
	    }


	  /* The "- 1" is because the default align_error is zero, i.e. zero _missing_ bases,
	     which is true when sub alignment follows on directly from the next. */
	  if ((curr_start - prev_end - 1) <= align_error)
	    {
	      last_align = align ;
	    }
	  else
	    {
	      perfect_align = FALSE ;
	      break ;
	    }
	}
    }

  return perfect_align ;
}

static void safe_destroy_align(gpointer quark_data, gpointer user_data)
{
  GQuark key = GPOINTER_TO_UINT(quark_data);
  ZMapFeatureContext context = (ZMapFeatureContext)user_data;

  g_datalist_id_remove_data(&(context->alignments), key);

  return ;
}

static void safe_destroy_block(gpointer quark_data, gpointer user_data)
{
  GQuark key = GPOINTER_TO_UINT(quark_data);
  ZMapFeatureAlignment align = (ZMapFeatureAlignment)user_data;

  g_datalist_id_remove_data(&(align->blocks), key);

  return ;
}

static void safe_destroy_set(gpointer quark_data, gpointer user_data)
{
  GQuark key = GPOINTER_TO_UINT(quark_data);
  ZMapFeatureBlock block = (ZMapFeatureBlock)user_data;
  
  g_datalist_id_remove_data(&(block->feature_sets), key);

  return ;
}

static void safe_destroy_feature(gpointer quark_data, gpointer user_data)
{
  GQuark key = GPOINTER_TO_UINT(quark_data);
  ZMapFeatureSet feature_set = (ZMapFeatureSet)user_data;
  
  g_datalist_id_remove_data(&(feature_set->features), key);

  return ;
}

static ZMapFeatureContextExecuteStatus destroyContextCB(GQuark key, 
                                                        gpointer data, 
                                                        gpointer user_data,
                                                        char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  MergeContextData        merge_data = (MergeContextData)user_data;
  ZMapFeatureAny         feature_any = (ZMapFeatureAny)data;
  gboolean destroy = FALSE;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      /* We don't destroy the context, so that the ContextMerge caller 
       * can do that. */
      if(merge_data->destroy_aligns_list)
        {
          g_list_foreach(merge_data->destroy_aligns_list, safe_destroy_align, feature_any);
          g_list_free(merge_data->destroy_aligns_list);
        }
      merge_data->destroy_aligns_list = NULL;
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      if(merge_data->destroy_align)
        {
          destroy = TRUE;
          merge_data->destroy_aligns_list = g_list_prepend(merge_data->destroy_aligns_list, GUINT_TO_POINTER(key));
        }
      merge_data->destroy_align  =
        merge_data->copied_align = FALSE;
      merge_data->current_current_align = 
        merge_data->current_diff_align = NULL;
      if(merge_data->destroy_blocks_list)
        {
          g_list_foreach(merge_data->destroy_blocks_list, safe_destroy_block, feature_any);
          g_list_free(merge_data->destroy_blocks_list);
        }
      merge_data->destroy_blocks_list = NULL;
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      if(merge_data->destroy_block)
        {
          destroy = TRUE;
          merge_data->destroy_blocks_list = g_list_prepend(merge_data->destroy_blocks_list, GUINT_TO_POINTER(key));
        }
      merge_data->destroy_block  = 
        merge_data->copied_block = FALSE;
      merge_data->current_current_block = 
        merge_data->current_diff_block = NULL;
      if(merge_data->destroy_sets_list)
        {
          g_list_foreach(merge_data->destroy_sets_list, safe_destroy_set, feature_any);
          g_list_free(merge_data->destroy_sets_list);
        }
      merge_data->destroy_sets_list = NULL;
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = (ZMapFeatureSet)feature_any;
        if(merge_data->destroy_set)
          {
            destroy = TRUE;
            merge_data->destroy_sets_list = g_list_prepend(merge_data->destroy_sets_list, GUINT_TO_POINTER(key));
          }
        else if(zMap_g_datalist_length(&(feature_set->features)) == 0)
          {
            merge_data->destroy_sets_list = g_list_prepend(merge_data->destroy_sets_list, GUINT_TO_POINTER(key));
          }
        merge_data->destroy_set  =
          merge_data->copied_set = FALSE;
        merge_data->current_current_set = 
          merge_data->current_diff_set = NULL;
        if(merge_data->destroy_features_list)
          {
            g_list_foreach(merge_data->destroy_features_list, safe_destroy_feature, feature_any);
            g_list_free(merge_data->destroy_features_list);
          }
        merge_data->destroy_features_list = NULL;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      if(merge_data->destroy_feature)
        {
          destroy = TRUE;
          merge_data->destroy_features_list = g_list_prepend(merge_data->destroy_features_list, GUINT_TO_POINTER(key));
        }
      merge_data->destroy_feature = FALSE;
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  if(merge_data->status == ZMAP_CONTEXT_EXEC_STATUS_OK)
    merge_data->status = status;

  return status;
}

/* mergeContextCB:
 * The main logic of the merge 

 * I'm not sure which is the best way to jump here.  There are other
 * ways to do this and the most promissing is probably without a diff
 * context and just removing/deleting from the new context, but for
 * now we're using the diff context... (See mergePreCB)

 * So here's what goes on...

 * Having been passed in 3 contexts (current, new and diff) we empty
 * the new context by either removing the feature (no notify) or by
 * destroying the feature depending on whether we already have a
 * matching feature.

 * CURRENT CONTEXT         NEW/SERVER CONTEXT         DIFF CONTEXT
 * -------------------+----------------------------+-----------------
 * Alignment
 *     - found here     flag for destruction 
 *                                          clone -> New Empty Align
 *
 *     - not found      remove from context, 
 *        insert here   <- alignment       copy   -> insert here 
 *                                                   [NO Destroy Handler]
 *
 * Block
 *     - found here     flag for destruction
 *                                          clone -> New Empty Block
 *
 *     - not found      remove from align, 
 *        insert here   <- block           copy   -> insert here 
 *                                                   [NO Destroy Handler]
 *
 * FeatureSet
 *     - found here     flag for destruction
 *                                          clone -> New Empty FeatureSet
 *
 *     - not found      remove from block, 
 *        insert here   <- featureset      copy   -> insert here 
 *                                                   [NO Destroy Handler]
 *
 * Feature
 *     - found here     flag for destruction       No need to clone,
 *                                                 No children 
 *
 *     - not found      remove from featureset, 
 *        insert here   <- alignment       copy   -> insert here 
 *                                                   [NO Destroy Handler]
 * -------------------------------------------------------------------

 * Stepping through the new context with a pre recurse function, we
 * check the current context for the object in the new context.  The
 * diff context only ever hold copies of the features with the
 * exception of the duplicate higher level object which may have non
 * duplicate children, which will need to be inserted into
 * something. We clone the duplicate object, creating a new "empty"
 * version which should be destroyed when the diff context is
 * destroyed.

 * [N.B.] There is a problem with blocks not having an automatic
 * GDestroyNotify
 * [N.B.] The new context ends up being empty, but still needs to be
 * destroyed
 
 * There could be some more error stuff...
 * We could memcpy the diff "empty" copies.
 */
static ZMapFeatureContextExecuteStatus mergeContextCB(GQuark key, 
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
  ZMapFeature                feature;
  gboolean remove_status = FALSE;

  if(merge_debug_G)
    zMapLogWarning("checking %s", g_quark_to_string(feature_any->unique_id));

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      merge_data->destroy_align     =
        merge_data->destroy_block   =
        merge_data->destroy_set     =
        merge_data->destroy_feature = FALSE;
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureContext servers_context;

        merge_data->copied_align = 
          merge_data->copied_block =
          merge_data->copied_set = FALSE; /* Or true... */

        feature_align   = (ZMapFeatureAlignment)feature_any;
        servers_context = (ZMapFeatureContext)feature_any->parent;
        
        /* Always remove the align from the new/server's context */
        /* not true any more! */


        if(!(feature_align = zMapFeatureContextGetAlignmentByID(merge_data->current_context, 
                                                                feature_any->unique_id)))
          {
            if(merge_debug_G)
              zMapLogWarning("%s","\tNot In Current - Copying");
            /* reset the alignment to be the one passed in (server's) */
            feature_align = (ZMapFeatureAlignment)feature_any;

            remove_status = zMapFeatureContextRemoveAlignment(servers_context, 
                                                              feature_align);
            zMapAssert(remove_status);

            zMapFeatureContextAddAlignment(merge_data->current_context, 
                                           feature_align, FALSE);

            /* Don't set a destroy handler in the diff context! */
            contextAddAlignment(merge_data->diff_context, feature_align, FALSE, NULL);

            merge_data->current_current_align = feature_align;
            merge_data->current_diff_align    = feature_align;

            merge_data->copied_align = 
              merge_data->copied_block =
              merge_data->copied_set = TRUE;
          }
        else
          {
            ZMapFeatureAlignment diff_align;

            if(merge_debug_G)
              zMapLogWarning("%s","\tAlready In Current - Creating In Diff");
            /* save the align we found in the current context */
            merge_data->current_current_align = feature_align;

            /* Need to create a copy in the diff context */
            if((diff_align = g_new0(ZMapFeatureAlignmentStruct, 1)))
              {
                diff_align->unique_id   = feature_align->unique_id;
                diff_align->original_id = feature_align->original_id;
                diff_align->struct_type = feature_align->struct_type;
                diff_align->blocks      = NULL; /* ! GList * ! g_datalist_init(&(diff_align->blocks)); */
                
                zMapFeatureContextAddAlignment(merge_data->diff_context, diff_align, FALSE);
                merge_data->destroy_align = TRUE;
              }

            merge_data->current_diff_align = diff_align;
          }
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      /* Only do block if align wasn't copied */
      if(!merge_data->copied_align)
        {
          ZMapFeatureAlignment servers_align;
          
          servers_align = (ZMapFeatureAlignment)feature_any->parent;
          feature_block = (ZMapFeatureBlock)feature_any;
          /* Always remove the block from the new/server's align */

          zMapAssert(merge_data->current_current_align != merge_data->current_diff_align);

          if(!(feature_block = zMapFeatureAlignmentGetBlockByID(merge_data->current_current_align,
                                                                feature_any->unique_id)))
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tNot In Current - Copying");

              /* reset the block to be the one passed in (server's) */
              feature_block = (ZMapFeatureBlock)(feature_any);

              remove_status = zMapFeatureAlignmentRemoveBlock(servers_align, feature_block);
              zMapAssert(remove_status);

              zMapFeatureAlignmentAddBlock(merge_data->current_current_align, 
                                           feature_block);

              /* no destroy handler for the diff context!!! */
              alignmentAddBlock(merge_data->current_diff_align, feature_block, NULL);
              
              merge_data->current_current_block = feature_block;
              merge_data->current_diff_block    = feature_block;

              merge_data->copied_block = merge_data->copied_set = TRUE;
            }
          else
            {
              ZMapFeatureBlock diff_block;

              if(merge_debug_G)
                zMapLogWarning("%s","\tAlready In Current - Creating In Diff");

              /* save the block we found in the current align */
              merge_data->current_current_block = feature_block;

              if((diff_block = g_new0(ZMapFeatureBlockStruct, 1)))
                {
                  if(!(merge_data->current_current_block->sequence.sequence))
                    merge_data->current_current_block->sequence = 
                      ((ZMapFeatureBlock)feature_any)->sequence;

                  diff_block->unique_id   = feature_block->unique_id;
                  diff_block->original_id = feature_block->original_id;
                  diff_block->parent      = (ZMapFeatureAny)(merge_data->current_diff_align);
                  diff_block->struct_type = feature_block->struct_type;

                  g_datalist_init(&(diff_block->feature_sets));
                  
                  diff_block->block_to_sequence = 
                    feature_block->block_to_sequence; /* struct copy */

                  zMapFeatureAlignmentAddBlock(merge_data->current_diff_align,
                                               diff_block);
                  merge_data->destroy_block = TRUE;
                }

              merge_data->current_diff_block    = diff_block;
              /* Clean up the block in the new */
            }
        }
      else if(merge_debug_G)
        zMapLogWarning("%s","\tParent Copied - next");
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      /* Only do feature set if block wasn't copied */
      if(!merge_data->copied_block)
        {
          ZMapFeatureBlock servers_block;

          feature_set   = (ZMapFeatureSet)feature_any;
          servers_block = (ZMapFeatureBlock)feature_any->parent;

          zMapAssert(merge_data->current_current_block != merge_data->current_diff_block);

          if(!(feature_set = zMapFeatureBlockGetSetByID(merge_data->current_current_block,
                                                        feature_any->unique_id)))
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tNot In Current - Copying");
              /* reset the set to be the one passwed in (server's) */
              feature_set = (ZMapFeatureSet)feature_any;

              remove_status = zMapFeatureBlockRemoveFeatureSet(servers_block, 
                                                               feature_set);
              zMapAssert(remove_status);

              zMapFeatureBlockAddFeatureSet(merge_data->current_current_block,
                                            feature_set);

              /* Don't set a destroy handler as it's only a copy... */
              blockAddFeatureSet(merge_data->current_diff_block, feature_set, NULL);
              /* 
               * g_datalist_id_set_data(&(merge_data->current_diff_block->feature_sets), 
               *                        feature_set->unique_id, 
               *                        feature_set);
               */
              merge_data->current_current_set = feature_set;
              merge_data->current_diff_set    = feature_set;

              merge_data->copied_set = TRUE;
            }
          else
            {
              ZMapFeatureSet diff_set;

              /* Save the feature set we found in the current block */
              /* Also save in case the feature_set is empty... (empty columns tom foolery) */
              merge_data->current_current_set = diff_set = feature_set;

              /* deal with empty columns tom foolery... */
              if(zMap_g_datalist_length(&(feature_set->features)) == 0)
                {
                  if(merge_debug_G)
                    zMapLogWarning("%s", "\tAlready in Current, but Copying as current is empty...");
 
                  /* get the server's and remove from the server's block */
                  feature_set   = (ZMapFeatureSet)feature_any;
                  remove_status = zMapFeatureBlockRemoveFeatureSet(servers_block, 
                                                                   feature_set);
                  zMapAssert(remove_status);

                  features_swap_parents(feature_set, diff_set);

                  merge_data->destroy_set = TRUE;

                  /* Don't set a destroy handler as it's only a copy... */
                  blockAddFeatureSet(merge_data->current_diff_block, diff_set, NULL);

                  merge_data->copied_set = TRUE; /* a slight lie, but we have copied all the 
                                                  * features and current_current_set == current_diff_set */
                }
              /* otherwise just create a clone and add it to the block. */
              else if((diff_set = g_new0(ZMapFeatureSetStruct, 1)))
                {
                  if(merge_debug_G)
                    zMapLogWarning("%s","\tAlready In Current - Creating In Diff");

                  diff_set->unique_id   = feature_set->unique_id;
                  diff_set->original_id = feature_set->original_id;
                  diff_set->struct_type = feature_set->struct_type;
                  diff_set->style       = feature_set->style;

                  g_datalist_init(&(diff_set->features));

                  zMapFeatureBlockAddFeatureSet(merge_data->current_diff_block,
                                                diff_set);
                  merge_data->destroy_set = TRUE;
                }

              merge_data->current_diff_set    = diff_set;
            }
          
          if(feature_set->style == NULL)
            {
              ZMapFeatureTypeStyle style;

              zMapLogWarning("Feature set %s has no style... Looking for style in context", 
                             g_quark_to_string(feature_set->unique_id));

              if((style = zMapFindStyle(merge_data->current_context->styles, feature_set->unique_id)))
                zMapFeatureSetStyle(feature_set, style);
            }
        }
      else if(merge_debug_G)
        zMapLogWarning("%s","\tParent Copied - next");
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      /* Only do feature if feature set wasn't copied */
      if(!merge_data->copied_set)
        {
          ZMapFeatureSet servers_set;

          servers_set   = (ZMapFeatureSet)feature_any->parent;
          feature       = (ZMapFeature)feature_any;

          if(!(feature = zMapFeatureSetGetFeatureByID(merge_data->current_current_set,
                                                      feature_any->unique_id)))
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tNot In Current - Copying");
              /* reset the feature to be the one passed in (server's) */
              feature = (ZMapFeature)feature_any;

              remove_status = zMapFeatureSetRemoveFeature(servers_set,
                                                          feature);
              zMapAssert(remove_status);

              zMapFeatureSetAddFeature(merge_data->current_current_set,
                                       feature);

              /* Don't set a destroy handler as it's only a copy... */
              featuresetAddFeature(merge_data->current_diff_set, feature, NULL);
            }
          else
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tAlready In Current - but IsFeature so no Create In Diff");

              merge_data->destroy_feature = TRUE;
            }
        }
      else if(merge_debug_G)
        zMapLogWarning("%s","\tParent Copied - next");
      break;
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapAssertNotReached();
      break;
    }


  if(merge_data->status == ZMAP_CONTEXT_EXEC_STATUS_OK)
    merge_data->status = status;

  return status;
}

static ZMapFeatureContextExecuteStatus emptyCopyCB(GQuark key, 
                                                   gpointer data, 
                                                   gpointer user_data,
                                                   char **err_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  EmptyCopyData    copy_data = (EmptyCopyData)user_data;
  ZMapFeatureAlignment align, copy_align;
  ZMapFeatureBlock     block, copy_block;
  ZMapFeatureSet feature_set, copy_feature_set;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      copy_data->context = zMapFeatureContextCreate(NULL, 0, 0, NULL, NULL);
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        char *align_name;
        gboolean is_master = FALSE;
        
        align = (ZMapFeatureAlignment)feature_any;

        if(((ZMapFeatureContext)(feature_any->parent))->master_align == align)
          is_master = TRUE;

        align_name = (char *)g_quark_to_string(align->original_id);
        copy_align = zMapFeatureAlignmentCreate(align_name, is_master);
        zMapFeatureContextAddAlignment(copy_data->context, copy_align, is_master);
        copy_data->align = copy_align;
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        char *block_seq;
        block = (ZMapFeatureBlock)feature_any;
        block_seq  = (char *)g_quark_to_string(block->original_id);
        copy_block = zMapFeatureBlockCreate(block_seq, 
                                            block->block_to_sequence.t1, 
                                            block->block_to_sequence.t2,
                                            block->block_to_sequence.t_strand,
                                            block->block_to_sequence.q1,
                                            block->block_to_sequence.q2,
                                            block->block_to_sequence.q_strand);
        copy_block->unique_id = block->unique_id;
        zMapFeatureAlignmentAddBlock(copy_data->align, copy_block);
        copy_data->block = copy_block;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        char *set_name;
        feature_set = (ZMapFeatureSet)feature_any;
        set_name    = (char *)g_quark_to_string(feature_set->original_id);

        copy_feature_set = zMapFeatureSetCreate(set_name, NULL);
        zMapFeatureBlockAddFeatureSet(copy_data->block, copy_feature_set);
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      /* We don't copy features, hence the name emptyCopyCB!! */
    default:
      zMapAssertNotReached();
      break;
    }

  return status;
}

static ZMapFeatureContextExecuteStatus eraseContextCB(GQuark key, 
                                                      gpointer data, 
                                                      gpointer user_data,
                                                      char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  MergeContextData merge_data = (MergeContextData)user_data;
  ZMapFeatureAny  feature_any = (ZMapFeatureAny)data;
  ZMapFeature feature, erased_feature;
  gboolean remove_status = FALSE;

  if(merge_debug_G)
    zMapLogWarning("checking %s", g_quark_to_string(feature_any->unique_id));

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      merge_data->current_current_align = zMapFeatureContextGetAlignmentByID(merge_data->current_context, 
                                                                             key);
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      merge_data->current_current_block = zMapFeatureAlignmentGetBlockByID(merge_data->current_current_align, 
                                                                           key);
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      merge_data->current_current_set   = zMapFeatureBlockGetSetByID(merge_data->current_current_block, 
                                                                     key);
      break;      
    case ZMAPFEATURE_STRUCT_FEATURE:
      /* look up in the current */
      feature = (ZMapFeature)feature_any;

      if(merge_data->current_current_set &&
         (erased_feature = zMapFeatureSetGetFeatureByID(merge_data->current_current_set, key)))
        {
          if(merge_debug_G)
            zMapLogWarning("%s","\tFeature in erase and current contexts...");

          /* insert into the diff context. 
           * BUT, need to check if the parents exist in the diff context first.
           */
          if(!merge_data->current_diff_align && 
             (merge_data->current_diff_align = g_new0(ZMapFeatureAlignmentStruct, 1)))
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tno parent align... creating align in diff");

              merge_data->current_diff_align->unique_id   = merge_data->current_current_align->unique_id;
              merge_data->current_diff_align->original_id = merge_data->current_current_align->original_id;
              merge_data->current_diff_align->struct_type = merge_data->current_current_align->struct_type;
              merge_data->current_diff_align->parent      = NULL;

              zMapFeatureContextAddAlignment(merge_data->diff_context, merge_data->current_diff_align, FALSE);
            }
          if(!merge_data->current_diff_block &&
             (merge_data->current_diff_block = g_new0(ZMapFeatureBlockStruct, 1)))
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tno parent block... creating block in diff");

              merge_data->current_diff_block->unique_id   = merge_data->current_current_block->unique_id;
              merge_data->current_diff_block->original_id = merge_data->current_current_block->original_id;
              merge_data->current_diff_block->struct_type = merge_data->current_current_block->struct_type;
              merge_data->current_diff_block->parent      = NULL;

              zMapFeatureAlignmentAddBlock(merge_data->current_diff_align, merge_data->current_diff_block);
            }
          if(!merge_data->current_diff_set &&
             (merge_data->current_diff_set = g_new0(ZMapFeatureSetStruct, 1)))
            {
              if(merge_debug_G)
                zMapLogWarning("%s","\tno parent set... creating set in diff");

              merge_data->current_diff_set->unique_id   = merge_data->current_current_set->unique_id;
              merge_data->current_diff_set->original_id = merge_data->current_current_set->original_id;
              merge_data->current_diff_set->struct_type = merge_data->current_current_set->struct_type;
              merge_data->current_diff_set->parent      = NULL;

              zMapFeatureBlockAddFeatureSet(merge_data->current_diff_block, merge_data->current_diff_set); 
            }

          if(merge_debug_G)
            zMapLogWarning("%s","\tmoving feature from current to diff context ... removing ... and inserting.");

          /* remove from the current context.*/
          remove_status = zMapFeatureSetRemoveFeature(merge_data->current_current_set, erased_feature);
          zMapAssert(remove_status);

          zMapFeatureSetAddFeature(merge_data->current_diff_set, erased_feature);

          /* destroy from the erase context.*/
          if(merge_debug_G)
            zMapLogWarning("%s","\tdestroying feature in erase context");
          zMapFeatureDestroy(feature);
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
      if(zMap_g_datalist_length(&(feature_align->blocks)) == 0)
        {
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty align ... destroying");
          zMapFeatureAlignmentDestroy(feature_align, TRUE);
        }
      merge_data->current_diff_align = 
        merge_data->current_current_align = NULL;
      merge_data->copied_align = FALSE;
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      feature_block = (ZMapFeatureBlock)feature_any;
      if(zMap_g_datalist_length(&(feature_block->feature_sets)) == 0)
        {
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty block ... destroying");
          zMapFeatureBlockDestroy(feature_block, TRUE);
        }
      merge_data->current_diff_block =
        merge_data->current_current_block = NULL;
      merge_data->copied_block = FALSE;
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      feature_set = (ZMapFeatureSet)feature_any;
      if(zMap_g_datalist_length(&(feature_set->features)) == 0)
        {
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty set ... destroying");
          zMapFeatureSetDestroy(feature_set, TRUE);
        }
      merge_data->current_diff_set =
        merge_data->current_current_set = NULL;
      merge_data->copied_set = FALSE;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      break;
    default:
      break;
    }


  return status ;
}


static ZMapFeatureContextExecuteStatus mergePreCB(GQuark key, 
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
  ZMapFeature                feature;
  gboolean new = TRUE;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      merge_data->aligns++;
      if(!(merge_data->current_current_align = zMapFeatureContextGetAlignmentByID(merge_data->current_context, 
                                                                                  feature_any->unique_id)))
        {
          ZMapFeatureContext server_context = (ZMapFeatureContext)(feature_any->parent);
          feature_align = (ZMapFeatureAlignment)(feature_any);

          merge_data->copied_align = TRUE;
          /* remove from the server context */
          zMapFeatureContextRemoveAlignment(server_context, feature_align);
          /* add to the full context */
          zMapFeatureContextAddAlignment(merge_data->current_context, feature_align, FALSE);
          /* re-add loosely to the server context */
          contextAddAlignment(server_context, feature_align, FALSE, NULL);

          merge_data->current_current_align = feature_align;
        }
      else if(merge_debug_G)
        {
          /* ... align already exists, but new blocks might be below ... */
          /* children need reparenting if they don't exist. */
          /* add to list for destruction on the way back up. */
          new = FALSE;
        }

      if(merge_debug_G)
        zMapLogWarning("Align (%p) '%s' is %s", feature_any, g_quark_to_string(feature_any->unique_id), (new == TRUE ? "new" : "old"));
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      merge_data->blocks++;
      if(!(merge_data->current_current_block = zMapFeatureAlignmentGetBlockByID(merge_data->current_current_align,
                                                                                feature_any->unique_id)) &&
         !(merge_data->copied_align))
        {
          ZMapFeatureAlignment server_align = (ZMapFeatureAlignment)(feature_any->parent);
          feature_block = (ZMapFeatureBlock)(feature_any);
          merge_data->copied_block = TRUE;
          /* remove from server align */
          zMapFeatureAlignmentRemoveBlock(server_align, feature_block);
          /* add to the full context align */
          zMapFeatureAlignmentAddBlock(merge_data->current_current_align, feature_block);
          /* re-add loosely to the server align */
          alignmentAddBlock(server_align, feature_block, NULL);

          merge_data->current_current_block = feature_block;
        }
      else if(merge_data->copied_align)
        {
          merge_data->copied_block = TRUE;
        }
      else if(merge_debug_G)
        {
          new = FALSE;
          /* block already exists, but  */
          /* add to list for destruction on the way back up. */
        }

      if(merge_debug_G)
        zMapLogWarning("Block (%p) '%s' is %s", feature_any, g_quark_to_string(feature_any->unique_id), (new == TRUE ? "new" : "old"));
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      merge_data->sets++;
      if(!(merge_data->current_current_set = zMapFeatureBlockGetSetByID(merge_data->current_current_block,
                                                                        feature_any->unique_id)) &&
         !(merge_data->copied_block))
        {
          ZMapFeatureBlock server_block = (ZMapFeatureBlock)(feature_any->parent);
          feature_set = (ZMapFeatureSet)(feature_any);

          merge_data->copied_set = TRUE;
          /* remove from the server block */
          zMapFeatureBlockRemoveFeatureSet(server_block, feature_set);
          /* add to the full context block */
          zMapFeatureBlockAddFeatureSet(merge_data->current_current_block, feature_set);
          /* re-add loosely to the server block */
          blockAddFeatureSet(server_block, feature_set, NULL);

          merge_data->current_current_set = feature_set;
        }
      else if(merge_data->copied_block)
        {
          merge_data->copied_set = TRUE;
        }
      else if(merge_debug_G)
        {
          /* set already exists, but some of children might be new... */
          /* add to list for destruction on the way back up. */
          new = FALSE;
        }

      if(merge_debug_G)
        zMapLogWarning("Set (%p) '%s' is %s", feature_any, g_quark_to_string(feature_any->unique_id), (new == TRUE ? "new" : "old"));
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      merge_data->features++;
      if(!(zMapFeatureSetGetFeatureByID(merge_data->current_current_set,
                                        feature_any->unique_id)) &&
         !(merge_data->copied_set))
        {
          ZMapFeatureSet server_set = (ZMapFeatureSet)(feature_any->parent);
          feature = (ZMapFeature)(feature_any);
          
          /* remove from the server set */
          zMapFeatureSetRemoveFeature(server_set, feature);
          /* add to the full context set */
          zMapFeatureSetAddFeature(merge_data->current_current_set, feature);
          /* re-add loosely to the server set */
          featuresetAddFeature(server_set, feature, NULL);
          zMapLogWarning("feature(%p)->parent = %p. current_current_set = %p", feature, feature->parent, merge_data->current_current_set);
        }
      else if(!merge_data->copied_set)
        {
          if(merge_use_safe_destroy)
            merge_data->destroy_features_list = g_list_prepend(merge_data->destroy_features_list, GUINT_TO_POINTER(key));
          else
            zMapFeatureDestroy((ZMapFeature)feature_any);
          merge_data->features--;
          new = FALSE;
        }
        
      if(merge_debug_G)
        zMapLogWarning("Feature (%p) '%s' is %s", feature_any, g_quark_to_string(feature_any->unique_id), (new == TRUE ? "new" : "old"));
      break;
    default:
      break;
    }

  return status;
}

static ZMapFeatureContextExecuteStatus mergePostCB(GQuark key, 
                                                   gpointer data, 
                                                   gpointer user_data,
                                                   char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  MergeContextData        merge_data = (MergeContextData)user_data;
  ZMapFeatureAny         feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContext feature_context;
  ZMapFeatureAlignment feature_align;
  ZMapFeatureBlock     feature_block;
  ZMapFeatureSet         feature_set;
  int datalist_length = 0;

  if(merge_debug_G)
    zMapLogWarning("checking %s", g_quark_to_string(feature_any->unique_id));

  
  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      feature_context = (ZMapFeatureContext)feature_any;

      if(merge_debug_G)
        {
          datalist_length = zMap_g_datalist_length(&(feature_context->alignments));
          zMapLogWarning("Context (%p) '%s' ... (cache=%d, length=%d)... Destroying", 
                         feature_any, g_quark_to_string(feature_any->unique_id),
                         merge_data->aligns, datalist_length);
        }

      if(merge_use_safe_destroy && merge_data->destroy_aligns_list)
        {
          g_list_foreach(merge_data->destroy_aligns_list, safe_destroy_align, feature_any);
          g_list_free(merge_data->destroy_aligns_list);
          merge_data->destroy_aligns_list = NULL;
        }

      merge_data->aligns = 0;
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      feature_align = (ZMapFeatureAlignment)feature_any;
      if(merge_data->blocks == 0)
        {
          if(merge_debug_G)
            {
              datalist_length = zMap_g_datalist_length(&(feature_align->blocks));
              zMapLogWarning("Align (%p) '%s' is empty (cache=%d, length=%d)... Destroying", 
                             feature_any, g_quark_to_string(feature_any->unique_id),
                             merge_data->blocks, datalist_length);
            }
          if(merge_use_safe_destroy)
            merge_data->destroy_aligns_list = g_list_prepend(merge_data->destroy_aligns_list, GUINT_TO_POINTER(key));
          else
            zMapFeatureAlignmentDestroy(feature_align, TRUE);
          merge_data->aligns--;
        }

      if(merge_use_safe_destroy && merge_data->destroy_blocks_list)
        {
          g_list_foreach(merge_data->destroy_blocks_list, safe_destroy_block, feature_any);
          g_list_free(merge_data->destroy_blocks_list);
          merge_data->destroy_blocks_list = NULL;
        }

      merge_data->current_diff_align = merge_data->current_current_align = NULL;
      merge_data->copied_align = FALSE;
      merge_data->blocks = 0;
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      feature_block = (ZMapFeatureBlock)feature_any;
      if(merge_data->sets == 0)
        {
          if(merge_debug_G)
            {
              datalist_length = zMap_g_datalist_length(&(feature_block->feature_sets));
              zMapLogWarning("Block (%p) '%s' is empty (cache=%d, length=%d)... Destroying", 
                             feature_any, g_quark_to_string(feature_any->unique_id),
                             merge_data->sets, datalist_length);
            }
          if(merge_use_safe_destroy)
            merge_data->destroy_blocks_list = g_list_prepend(merge_data->destroy_blocks_list, GUINT_TO_POINTER(key));
          else
            zMapFeatureBlockDestroy(feature_block, TRUE);
          merge_data->blocks--;
        }

      if(merge_use_safe_destroy && merge_data->destroy_sets_list)
        {
          g_list_foreach(merge_data->destroy_sets_list, safe_destroy_set, feature_any);
          g_list_free(merge_data->destroy_sets_list);
          merge_data->destroy_sets_list = NULL;
        }

      merge_data->current_diff_block = merge_data->current_current_block = NULL;
      merge_data->copied_block = FALSE;
      merge_data->sets = 0;
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      feature_set = (ZMapFeatureSet)feature_any;

      if(merge_data->features == 0)
        {
          if(merge_debug_G)
            {
              datalist_length = zMap_g_datalist_length(&(feature_set->features));
              zMapLogWarning("FeatureSet (%p) '%s' is empty (cache=%d, length=%d)... Destroying", 
                             feature_any, g_quark_to_string(feature_any->unique_id),
                             merge_data->features, datalist_length);
            }
          if(merge_use_safe_destroy)
            merge_data->destroy_sets_list = g_list_prepend(merge_data->destroy_sets_list, GUINT_TO_POINTER(key));
          else
            zMapFeatureSetDestroy(feature_set, TRUE);
          merge_data->sets--;
        }

      if(merge_use_safe_destroy && merge_data->destroy_features_list)
        {
          g_list_foreach(merge_data->destroy_features_list, safe_destroy_feature, feature_any);
          g_list_free(merge_data->destroy_features_list);
          merge_data->destroy_features_list = NULL;
        }

      merge_data->current_diff_set = merge_data->current_current_set = NULL;
      merge_data->copied_set = FALSE;
      merge_data->features = 0;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
    default:
      zMapAssertNotReached();
      break;
    }

  return status;
}
