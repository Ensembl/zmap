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
 * Last edited: Jan  9 09:25 2007 (rds)
 * Created: Fri Jul 16 13:05:58 2004 (edgrif)
 * CVS info:   $Id: zmapFeature.c,v 1.54 2007-01-09 09:25:36 rds Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <strings.h>
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
} MergeContextDataStruct, *MergeContextData;

typedef struct
{
  int length;
}DataListLengthStruct, *DataListLength;


static void destroyFeatureAny(gpointer data) ;
static void destroyFeature(gpointer data) ;
static void destroyFeatureSet(gpointer data) ;
static void destroyBlock(gpointer data) ;
static void destroyAlign(gpointer data) ;
static void destroyContext(gpointer data) ;
static void withdrawFeatureAny(GQuark key_id, gpointer data, gpointer user_data) ;

/* Because Blocks have to be different... some GFunc for the GList foreach calls... */
static void removeBlockFromAlignsList(gpointer block_data, gpointer user_data);
static void clearBlockFromAlignsList(gpointer block_data, gpointer user_data);
static void freeBlocks(gpointer block_data, gpointer unused);
static gint blockWithID(gconstpointer block_data, gconstpointer id_data);

/* datalist debug stuff */
static void getDataListLength(GQuark id, gpointer data, gpointer user_data);
static void printDestroyDebugInfo(GData *datalist, char *who);
static gboolean checkForPerfectAlign(GArray *gaps, unsigned int align_error) ;

static ZMapFeatureContextExecuteStatus destroyContextCB(GQuark key, 
                                                        gpointer data, 
                                                        gpointer user_data,
                                                        char **err_out);
static ZMapFeatureContextExecuteStatus mergeContextCB(GQuark key, 
                                                      gpointer data, 
                                                      gpointer user_data,
                                                      char **err_out);


static gboolean merge_debug_G   = FALSE;
static gboolean destroy_debug_G = FALSE;

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

  zMapAssert(feature_set && feature && feature->unique_id != ZMAPFEATURE_NULLQUARK) ;

  if (!zMapFeatureSetFindFeature(feature_set, feature))
    {
      feature->parent = (ZMapFeatureAny)feature_set ;

      g_datalist_id_set_data_full(&(feature_set->features), feature->unique_id, 
                                  feature, destroyFeatureAny);

      result = TRUE ;
    }

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

  if((feature_block = (ZMapFeatureBlock)(feature_block->parent)))
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


ZMapFeatureAlignment zMapFeatureAlignmentCreate(char *align_name, gboolean master_alignment)
{
  ZMapFeatureAlignment alignment ;
  char *unique_name ;

  zMapAssert(align_name) ;

  alignment = g_new0(ZMapFeatureAlignmentStruct, 1) ;

  alignment->struct_type = ZMAPFEATURE_STRUCT_ALIGN ;

  if (master_alignment)
    unique_name = g_strdup_printf("%s_master", align_name) ;
  else
    unique_name = g_strdup(align_name) ;

  alignment->original_id = g_quark_from_string(align_name) ;

  alignment->unique_id = g_quark_from_string(unique_name) ;

  g_free(unique_name) ;

  return alignment ;
}


void zMapFeatureAlignmentAddBlock(ZMapFeatureAlignment alignment, ZMapFeatureBlock block)
{
  block->parent = (ZMapFeatureAny)alignment ;

  alignment->blocks = g_list_append(alignment->blocks, block) ;

  return ;
}

gboolean zMapFeatureAlignmentFindBlock(ZMapFeatureAlignment feature_align, 
                                       ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;
  GList *stored   = NULL;
  zMapAssert(feature_align && feature_block);

  if((stored = g_list_find(feature_align->blocks, feature_block)))
    result = TRUE;

  return result;
}

ZMapFeatureBlock zMapFeatureAlignmentGetBlockByID(ZMapFeatureAlignment feature_align, GQuark block_id)
{
  ZMapFeatureBlock feature_block = NULL;
  GList *block_list;
  
  if((block_list = g_list_find_custom(feature_align->blocks, 
                                      GUINT_TO_POINTER(block_id),
                                      blockWithID)))
    {
      feature_block = (ZMapFeatureBlock)(block_list->data);
    }

  return feature_block ;
}


gboolean zMapFeatureAlignmentRemoveBlock(ZMapFeatureAlignment feature_align,
                                         ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;

  if(zMapFeatureAlignmentFindBlock(feature_align, feature_block))
    {
      removeBlockFromAlignsList(feature_block, feature_align);
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
          
          g_list_foreach(alignment->blocks, removeBlockFromAlignsList, (gpointer)alignment);
          
          destroyFeatureAny((gpointer)alignment);
        }
      else
        {
          g_datalist_id_remove_data(&(context->alignments), alignment->unique_id);
        }
    }
  else if(!free_data)
    {
      g_list_foreach(alignment->blocks, removeBlockFromAlignsList, (gpointer)alignment);

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
  zMapAssert(feature_block && feature_set && feature_set->unique_id) ;

  feature_set->parent = (ZMapFeatureAny)feature_block ;

  g_datalist_id_set_data_full(&(feature_block->feature_sets), 
                              feature_set->unique_id, 
                              feature_set, destroyFeatureAny) ;

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
          removeBlockFromAlignsList(block, align);

          /* empty the datalist, without destroying lower level objects... */
          g_datalist_foreach(&(block->feature_sets), withdrawFeatureAny, (gpointer)block);

          destroyFeatureAny((gpointer)block);
        }
      else
        {
          /* GList stuff ... recreate the logic of g_datalist_id_remove_data */
          clearBlockFromAlignsList(block, align);
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


void zMapFeatureContextAddAlignment(ZMapFeatureContext feature_context,
				    ZMapFeatureAlignment alignment, gboolean master)
{
  zMapAssert(feature_context && alignment) ;

  alignment->parent = (ZMapFeatureAny)feature_context ;

  g_datalist_id_set_data_full(&(feature_context->alignments), 
                              alignment->unique_id, alignment,
                              destroyFeatureAny) ;

  if (master)
    feature_context->master_align = alignment ;

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
gboolean zMapFeatureContextMerge(ZMapFeatureContext *current_context_inout,
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

      g_free(diff_context_string);
      
      zMapFeatureContextExecuteComplete((ZMapFeatureAny)new_context, ZMAPFEATURE_STRUCT_FEATURE,
                                        mergeContextCB, destroyContextCB, &merge_data);

      /* Something's wrong if there is still a master alignment */
      if(new_context->master_align)
        zMapAssertNotReached();

      if(merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
        result = TRUE;
    }

  if (result)
    {
      *current_context_inout = current_context ;
      *diff_context_out = diff_context ;
    }

  return result ;
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


/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear () and also if g_datalist_id_remove_data()
 * is called. */
static void destroyFeature(gpointer data)
{
  ZMapFeature feature = (ZMapFeature)data ;

  if (feature->url)
    g_free(feature->url) ;

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

static void printDestroyDebugInfo(GData *datalist, char *who)
{
  DataListLengthStruct length = {0};

  g_datalist_foreach(&datalist, getDataListLength, &length);

  if(destroy_debug_G)
    printf("%s: datalist size %d\n", who, length.length);

  return ;
}

/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear () and also if g_datalist_id_remove_data()
 * is called. */
static void destroyFeatureSet(gpointer data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  
  if(destroy_debug_G)
    printDestroyDebugInfo(feature_set->features, __PRETTY_FUNCTION__);

  g_datalist_clear(&(feature_set->features)) ;

  g_free(feature_set) ;

  return ;
}

static void destroyBlock(gpointer block_data)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)block_data;

  if(destroy_debug_G)
    printDestroyDebugInfo(block->feature_sets, __PRETTY_FUNCTION__);

  g_datalist_clear(&(block->feature_sets));

  g_free(block);

  return ;
}

static void destroyAlign(gpointer align_data)
{
  ZMapFeatureAlignment align = (ZMapFeatureAlignment)align_data;

  if(destroy_debug_G)
    printf("%s: glist size %d\n", __PRETTY_FUNCTION__, g_list_length(align->blocks));
  
  g_list_foreach(align->blocks, freeBlocks, NULL);

  g_free(align);

  return ;
}

static void destroyContext(gpointer context_data)
{
  ZMapFeatureContext context = (ZMapFeatureContext)context_data;

  if(destroy_debug_G)
    printDestroyDebugInfo(context->alignments, __PRETTY_FUNCTION__);

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
        /* children_datalist = align->blocks; // Blocks is a GList ! */
        removeBlockFromAlignsList(align, data); /* So this does it instead */
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


static void removeBlockFromAlignsList(gpointer block_data, gpointer user_data)
{
  ZMapFeatureAlignment feature_align = (ZMapFeatureAlignment)user_data;

  feature_align->blocks = g_list_remove(feature_align->blocks, block_data);

  return ;
}

static void clearBlockFromAlignsList(gpointer block_data, gpointer user_data)
{
  ZMapFeatureAlignment feature_align = (ZMapFeatureAlignment)user_data;

  feature_align->blocks = g_list_remove(feature_align->blocks, block_data);

  destroyFeatureAny(block_data);

  return ;
}

static void freeBlocks(gpointer block_data, gpointer unused)
{
  /* Call what should be the GDestroyNotify, but we're still using a GList :( */

  destroyFeatureAny(block_data);

  return ;
}

static gint blockWithID(gconstpointer block_data, gconstpointer id_data)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)block_data;
  GQuark  id = GPOINTER_TO_UINT(id_data);
  gint match = -1;

  if(block->unique_id == id)
    match = 0;

  return match;
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
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      if(merge_data->destroy_align)
        destroy = TRUE;
      merge_data->destroy_align = FALSE;
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      if(merge_data->destroy_block)
        destroy = TRUE;
      merge_data->destroy_block = FALSE;
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      if(merge_data->destroy_set)
        destroy = TRUE;
      merge_data->destroy_set = FALSE;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      if(merge_data->destroy_feature)
        destroy = TRUE;
      merge_data->destroy_feature = FALSE;
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  if(destroy)
    destroyFeatureAny(feature_any);

  if(merge_data->status == ZMAP_CONTEXT_EXEC_STATUS_OK)
    merge_data->status = status;

  return status;
}

/* mergeContextCB:
 * The main logic of the merge 

 * Having been passed in 3 contexts (current, new and diff) we empty
 * the server context and move the features to the correct context or
 * destroy them.  Here's a simple table

 * CURRENT CONTEXT         NEW/SERVER CONTEXT         DIFF CONTEXT
 * -------------------+----------------------------+-----------------
 * Alignment
 *     - found here     remove from context, clone -> New Empty Align
 *                      flag for destruction
 *
 *     - not found      remove from context, 
 *        insert here   <- alignment       copy   -> insert here 
 *                                                   [NO Destroy Handler]
 *
 * Block
 *     - found here     remove from align,  clone -> New Empty Block
 *                      flag for destruction
 *
 *     - not found      remove from align, 
 *        insert here   <- block           copy   -> insert here 
 *                                                   [NO Destroy Handler]
 *
 * FeatureSet
 *     - found here     remove from block,  clone -> New Empty FeatureSet
 *                      flag for destruction
 *
 *     - not found      remove from block, 
 *        insert here   <- featureset      copy   -> insert here 
 *                                                   [NO Destroy Handler]
 *
 * Feature
 *     - found here     remove from featureset,    NO need to clone,
 *                      flag for destruction       No children 
 *
 *     - not found      remove from featureset, 
 *        insert here   <- alignment       copy   -> insert here 
 *                                                   [NO Destroy Handler]
 * -------------------------------------------------------------------

 * So at each level we check the current context for the object in the 
 * new context.  The diff context only ever hold copies of the features
 * with the exception of the duplicate higher level object which may 
 * have non duplicate children, which will need to be inserted into
 * something. We clone the duplicate object, creating a new "empty"
 * version which should be destroyed when the diff context is destroyed.

 * [N.B.] The Diff context is currently not destroyed anywhere in the
 * code
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
    printf("checking %s\n", g_quark_to_string(feature_any->unique_id));

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
        remove_status = zMapFeatureContextRemoveAlignment(servers_context, 
                                                          feature_align);
        zMapAssert(remove_status);

        if(!(feature_align = zMapFeatureContextGetAlignmentByID(merge_data->current_context, 
                                                                feature_any->unique_id)))
          {
            if(merge_debug_G)
              printf("\tNot In Current - Copying\n");
            /* reset the alignment to be the one passed in (server's) */
            feature_align = (ZMapFeatureAlignment)feature_any;

            zMapFeatureContextAddAlignment(merge_data->current_context, 
                                           feature_align, FALSE);

            /* Don't set a destroy handler in the diff context! */
            g_datalist_id_set_data(&(merge_data->diff_context->alignments),
                                   feature_align->unique_id,
                                   (gpointer)feature_align);

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
              printf("\tAlready In Current - Creating In Diff\n");
            /* save the align we found in the current context */
            merge_data->current_current_align = feature_align;

            /* Need to create a copy in the diff context */
            if((diff_align = g_new0(ZMapFeatureAlignmentStruct, 1)))
              {
                diff_align->unique_id   = feature_align->unique_id;
                diff_align->original_id = feature_align->original_id;
                diff_align->struct_type = feature_align->struct_type;
                diff_align->blocks      = NULL; /* ! */
                
                zMapFeatureContextAddAlignment(merge_data->diff_context, diff_align, FALSE);
                merge_data->destroy_align = TRUE;
              }

            merge_data->current_diff_align    = diff_align;
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
          remove_status = zMapFeatureAlignmentRemoveBlock(servers_align, feature_block);
          zMapAssert(remove_status);

          zMapAssert(merge_data->current_current_align != merge_data->current_diff_align);

          if(!(feature_block = zMapFeatureAlignmentGetBlockByID(merge_data->current_current_align,
                                                                feature_any->unique_id)))
            {
              if(merge_debug_G)
                printf("\tNot In Current - Copying\n");
              /* reset the block to be the one passed in (server's) */
              feature_block = (ZMapFeatureBlock)(feature_any);

              zMapFeatureAlignmentAddBlock(merge_data->current_current_align, 
                                           feature_block);

              /* no destroy handler for the diff context!!!
               * BIG ERROR HERE! There's no automatic destroy/_no_destroy_ for this */
              merge_data->current_diff_align->blocks = 
                g_list_append(merge_data->current_diff_align->blocks, feature_block);
              
              merge_data->current_current_block = feature_block;
              merge_data->current_diff_block    = feature_block;

              merge_data->copied_block = merge_data->copied_set = TRUE;
            }
          else
            {
              ZMapFeatureBlock diff_block;

              if(merge_debug_G)
                printf("\tAlready In Current - Creating In Diff\n");
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
        printf("\tParent Copied - next\n");
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      /* Only do feature set if block wasn't copied */
      if(!merge_data->copied_block)
        {
          ZMapFeatureBlock servers_block;

          feature_set   = (ZMapFeatureSet)feature_any;
          servers_block = (ZMapFeatureBlock)feature_any->parent;
          /* Always remove the set from the new/server's block */
          remove_status = zMapFeatureBlockRemoveFeatureSet(servers_block, 
                                                           feature_set);
          zMapAssert(remove_status);

          if(!(feature_set = zMapFeatureBlockGetSetByID(merge_data->current_current_block,
                                                        feature_any->unique_id)))
            {
              if(merge_debug_G)
                printf("\tNot In Current - Copying\n");
              /* reset the set to be the one passwed in (server's) */
              feature_set = (ZMapFeatureSet)feature_any;

              zMapFeatureBlockAddFeatureSet(merge_data->current_current_block,
                                            feature_set);

              /* Don't set a destroy handler as it's only a copy... */
              g_datalist_id_set_data(&(merge_data->current_diff_block->feature_sets),
                                     feature_set->unique_id, feature_set);

              merge_data->current_current_set = feature_set;
              merge_data->current_diff_set    = feature_set;

              merge_data->copied_set = TRUE;
            }
          else
            {
              ZMapFeatureSet diff_set;

              if(merge_debug_G)
                printf("\tAlready In Current - Creating In Diff\n");

              /* Save the feature set we found in the current block */
              merge_data->current_current_set = feature_set;

              if((diff_set = g_new0(ZMapFeatureSetStruct, 1)))
                {
                  diff_set->unique_id   = feature_set->unique_id;
                  diff_set->original_id = feature_set->original_id;
                  diff_set->struct_type = feature_set->struct_type;
                  diff_set->style       = feature_set->style;

                  zMapFeatureBlockAddFeatureSet(merge_data->current_diff_block,
                                                diff_set);
                  merge_data->destroy_set = TRUE;
                }

              merge_data->current_diff_set    = diff_set;
            }
        }
      else if(merge_debug_G)
        printf("\tParent Copied - next\n");
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      /* Only do feature if feature set wasn't copied */
      if(!merge_data->copied_set)
        {
          ZMapFeatureSet servers_set;

          servers_set   = (ZMapFeatureSet)feature_any->parent;
          feature       = (ZMapFeature)feature_any;
          remove_status = zMapFeatureSetRemoveFeature(servers_set,
                                                      feature);
          zMapAssert(remove_status);


          if(!(feature = zMapFeatureSetGetFeatureByID(merge_data->current_current_set,
                                                      feature_any->unique_id)))
            {
              if(merge_debug_G)
                printf("\tNot In Current - Copying\n");
              /* reset the feature to be the one passed in (server's) */
              feature = (ZMapFeature)feature_any;

              zMapFeatureSetAddFeature(merge_data->current_current_set,
                                       feature);

              /* Don't set a destroy handler as it's only a copy... */
              g_datalist_id_set_data(&merge_data->current_diff_set->features,
                                     feature->unique_id, feature);
            }
          else
            {
              if(merge_debug_G)
                printf("\tAlready In Current - but IsFeature so no Create In Diff\n");

              merge_data->destroy_feature = TRUE;
            }
        }
      else if(merge_debug_G)
        printf("\tParent Copied - next\n");
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
