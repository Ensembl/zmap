/*  File: zmapViewFeatures.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Implements feature contexts, sets and features themselves.
 *              Includes code to create/merge/destroy contexts and sets.
 *              
 * Exported functions: See zmapView_P.h
 * HISTORY:
 * Last edited: Nov 22 10:12 2004 (edgrif)
 * Created: Fri Jul 16 13:05:58 2004 (edgrif)
 * CVS info:   $Id: zmapFeature.c,v 1.9 2004-11-22 17:47:09 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <strings.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapView_P.h>


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
typedef struct
{
  GData **current_feature_sets ;
  GData **diff_feature_sets ;
  GData **new_feature_sets ;
} FeatureContextsStruct, *FeatureContexts ;


typedef struct
{
  GData **current_features ;
  GData **diff_features ;
  GData **new_features ;
} FeatureSetStruct, *FeatureSet ;


/* These are GData callbacks. */
static void doNewFeatureSets(GQuark key_id, gpointer data, gpointer user_data) ;
static void doNewFeatures(GQuark key_id, gpointer data, gpointer user_data) ;

static void printFeatureContext(ZMapFeatureContext new_context) ;
static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void printFeature(GQuark key_id, gpointer data, gpointer user_data) ;


/* !
 * A set of functions for allocating, populating and destroying features.
 * The feature create and add data are in two steps because currently the required
 * use is to have a struct that may need to be filled in in several steps because
 * in some data sources the data comes split up in the datastream (e.g. exons in
 * GFF). If there is a requirement for the two bundled then it should be implemented
 * via a simple new "create and add" function that merely calls both the create and
 * add functions from below. */

/*!
 * Returns a single feature correctly intialised to be a "NULL" feature.
 * 
 * @param   void  None.
 * @return  ZMapFeature  A pointer to the new ZMapFeature.
 *  */
ZMapFeature zmapFeatureCreate(void)
{
  ZMapFeature feature ;

  feature = g_new0(ZMapFeatureStruct, 1) ;
  feature->id = ZMAPFEATUREID_NULL ;
  feature->type = ZMAPFEATURE_INVALID ;

  return feature ;
}


/*!
 * Adds data to a feature which may be "NULL" or may already have partial features,
 * e.g. transcript that does not yet have all its exons.
 * 
 * NOTE that really we need this to be a polymorphic function so that the arguments
 * are different for different features.
 *  */
gboolean zmapFeatureAugmentData(ZMapFeature feature, char *name,
				char *sequence, char *source, ZMapFeatureType feature_type,
				int start, int end, double score, ZMapStrand strand,
				ZMapPhase phase,
				ZMapHomolType homol_type, int query_start, int query_end)
{
  gboolean result = FALSE ;

  zMapAssert(feature) ;

  /* Note we have hacked this up for now...in the end we should have a unique id for each feature
   * but for now we will look at the type to determine if a feature is empty or not..... */
  /* If its an empty feature then initialise... */
  if (feature->type == ZMAPFEATURE_INVALID)
    {
      feature->name = g_strdup(name) ;
      feature->type = feature_type ;
      feature->x1 = start ;
      feature->x2 = end ;

      feature->method_name = g_strdup(source) ;

      feature->strand = strand ;
      feature->phase = phase ;
      feature->score = score ;

      result = TRUE ;
    }


  /* There is going to have to be some hacky code here to decide when something goes from being
   * a single exon to being part of a transcript..... */

  if (feature_type == ZMAPFEATURE_EXON)
    {
      ZMapSpanStruct exon ;

      exon.x1 = start ;
      exon.x2 = end ;

      /* This is still not correct I think ??? we shouldn't be using the transcript field but
       * instead the lone exon field. */

      if (!feature->feature.transcript.exons)
	feature->feature.transcript.exons = g_array_sized_new(FALSE, TRUE,
							      sizeof(ZMapSpanStruct), 30) ;

      g_array_append_val(feature->feature.transcript.exons, exon) ;

      /* If this is _not_ single exon we make it into a transcript.
       * This is a bit adhoc but so is GFF v2. */
      if (feature->type != ZMAPFEATURE_EXON
	  || feature->feature.transcript.exons->len > 1)
	feature->type = ZMAPFEATURE_TRANSCRIPT ;

      result = TRUE ;
    }
  else if (feature_type == ZMAPFEATURE_INTRON)
    {
      ZMapSpanStruct intron ;

      intron.x1 = start ;
      intron.x2 = end ;

      if (!feature->feature.transcript.introns)
	feature->feature.transcript.introns = g_array_sized_new(FALSE, TRUE,
							      sizeof(ZMapSpanStruct), 30) ;

      g_array_append_val(feature->feature.transcript.introns, intron) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I DON'T THINK WE WANT TO DO THIS BECAUSE WE MAY JUST HAVE A SET OF CONFIRMED INTRONS
       * THAT DO NOT ACTUALLY REPRESENT A TRANSCRIPT YET..... */


      /* If we have more than one intron then we are going to force the type to be
       * transcript. */
      if (feature->type != ZMAPFEATURE_TRANSCRIPT
	  && feature->feature.transcript.introns->len > 1)
	feature->type = ZMAPFEATURE_TRANSCRIPT ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      result = TRUE ;
    }
  else if (feature_type == ZMAPFEATURE_HOMOL)
    {
      /* Note that we do not put the extra "align" information for gapped alignments into the
       * GFF files from acedb so no need to worry about it just now...... */

      feature->feature.homol.type = homol_type ;
      feature->feature.homol.y1 = query_start ;
      feature->feature.homol.y2 = query_end ;
      feature->feature.homol.score = score ;
      feature->feature.homol.align = NULL ;		    /* not supported currently.... */
    }

  return result ;
}


void zmapFeatureDestroy(ZMapFeature feature)
{
  zMapAssert(feature) ;

  if (feature->name)
    g_free(feature->name) ;

  if (feature->method_name)
    g_free(feature->method_name) ;

  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      if (feature->feature.transcript.exons)
	g_array_free(feature->feature.transcript.exons, TRUE) ;

      if (feature->feature.transcript.introns)
	g_array_free(feature->feature.transcript.introns, TRUE) ;
    }

  g_free(feature) ;

  return ;
}


ZMapFeatureSet zMapFeatureSetCreate(char *source, GData *features)
{
  ZMapFeatureSet feature_set ;

  feature_set = g_new0(ZMapFeatureSetStruct, 1) ;
  feature_set->source = g_strdup(source) ;
  feature_set->features = features ;

  return feature_set ;
}

/* We fudge issue of releasing features here...we need a flag to say "free the data" or
 * not....... */
void zMapFeatureSetDestroy(ZMapFeatureSet feature_set)
{
  g_free(feature_set->source) ;

  g_free(feature_set) ;

  return ;
}


ZMapFeatureContext zMapFeatureContextCreate(void)
{
  ZMapFeatureContext feature_context ;

  feature_context = g_new0(ZMapFeatureContextStruct, 1) ;

  return feature_context ;
}


void zMapFeatureContextDestroy(ZMapFeatureContext feature_context)
{
  /* MOre needs adding here....we will need a "free data" flag to say if
   * data should be added. */

  g_free(feature_context) ;

  return ;
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
 * Hence at the end of the merge we will have:
 * 
 * current_context  - contains the merge of all feature sets and features
 *     new_context  - is empty
 *    diff_context  - contains all the new features sets and features that
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
  ZMapFeatureContext current_context = *current_context_inout ;
  ZMapFeatureContext diff_context ;

  zMapAssert(new_context && diff_context_out) ;

  diff_context = NULL ;					    /* default is no new features. */

  /* If there are no current features we just return the new ones and the diff is
   * set to NULL, otherwise we need to do a merge of the new and current feature sets. */
  if (!current_context)
    {
      current_context = new_context ;

      result = TRUE ;
    }
  else
    {
      /* Check that certain basic things about the sequences to be merged are true....
       * this will probably have to be revised, for now we check that the name and length of the
       * sequences are the same. */
      if (g_ascii_strcasecmp(current_context->sequence, new_context->sequence) == 0
	  && current_context->features_to_sequence.p1 == new_context->features_to_sequence.p1
	  && current_context->features_to_sequence.p2 == new_context->features_to_sequence.p2)
	{
	  FeatureContextsStruct contexts ;

	  diff_context = zMapFeatureContextCreate() ;

	  contexts.current_feature_sets = &(current_context->feature_sets) ;
	  contexts.diff_feature_sets = &(diff_context->feature_sets) ;
	  contexts.new_feature_sets = &(new_context->feature_sets) ;

	  g_datalist_foreach(&(new_context->feature_sets),
			     doNewFeatureSets, (void *)&contexts) ;

	  if (diff_context->feature_sets)
	    {
	      /* Fill in the sequence/mapping details. */
	      diff_context->sequence = g_strdup(current_context->sequence) ;
	      diff_context->parent = g_strdup(current_context->parent) ;
	      diff_context->parent_span = current_context->parent_span ; /* n.b. struct copies. */
	      diff_context->sequence_to_parent = current_context->sequence_to_parent ;
	      diff_context->features_to_sequence = current_context->features_to_sequence ;
	    }
	  else
	    {
	      zMapFeatureContextDestroy(diff_context) ;
	      diff_context = NULL ;
	    }

	  result = TRUE ;
	}
    }


  if (result)
    {
      *current_context_inout = current_context ;
      *diff_context_out = diff_context ;

      /* delete the new_context.... */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* debugging.... */

      printf("\ncurrent context\n") ;
      printFeatureContext(*current_context_inout) ;


      printf("\nnew context\n") ;
      printFeatureContext(new_context) ;

      if (*diff_context_out)
	{
	  printf("\ndiff context\n") ;
	  printFeatureContext(*diff_context_out) ;
	}

      zMapAssert(fflush(stdout) == 0) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  return result ;
}





/* 
 *            Internal routines. 
 */




/* Note that user_data is the _address_ of the pointer to a GData, we need this because
 * we need to set the GData address in the original struct to be the new GData address
 * after we have added elements to it. */
static void doNewFeatureSets(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet new_set = (ZMapFeatureSet)data ;

  FeatureContexts contexts = (FeatureContexts)user_data ;
  GData **current_result = contexts->current_feature_sets ;
  GData *current_feature_sets = *current_result ;
  GData **diff_result = contexts->diff_feature_sets ;
  GData *diff_feature_sets = *diff_result ;
  GData **new_result = contexts->new_feature_sets ;
  GData *new_feature_sets = *new_result ;

  ZMapFeatureSet current_set, diff_set ;
  char *feature_set_name = NULL ;


  feature_set_name = (char *)g_quark_to_string(key_id) ;

  zMapAssert(strcmp(feature_set_name, new_set->source) == 0) ;

  /* If the feature set is not in the current feature sets then we can simply add it to
   * them, otherwise we must do a merge of the individual features within
   * the new and current sets. */
  if (!(current_set = g_datalist_id_get_data(&(current_feature_sets), key_id)))
    {
      gpointer unused ;

      g_datalist_set_data(&(current_feature_sets), feature_set_name, new_set) ;
      g_datalist_set_data(&(diff_feature_sets), feature_set_name, new_set) ;

      /* Should remove from new set here but not sure if it will
       * work as part of a foreach loop....which is where this routine is called from... */
      unused = g_datalist_id_remove_no_notify(&(new_feature_sets), key_id) ;

      printf("stop here\n") ;
    }
  else
    {
      FeatureSetStruct features ;

      diff_set = zMapFeatureSetCreate(feature_set_name, NULL) ;

      features.current_features = &(current_set->features);
      features.diff_features = &(diff_set->features) ;
      features.new_features = &(new_set->features) ;

      g_datalist_foreach(&(new_set->features),
			 doNewFeatures, (void *)&features) ;

      /* after we have done this, if there are any features in the diff set then we must
       * add it to the diff_feature_set as above.... */
      if (diff_set->features)
	g_datalist_set_data(&(diff_feature_sets), feature_set_name, diff_set) ;
      else
	zMapFeatureSetDestroy(diff_set) ;

    }

  /* Poke the results back in to the original feature context, seems hokey but remember that
   * the address of GData will change as new elements are added so we must poke back the new
   * address. */
  *current_result = current_feature_sets ;
  *diff_result = diff_feature_sets ;
  *new_result = new_feature_sets ;

  return ;
}


static void doNewFeatures(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature new_feature = (ZMapFeature)data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GData **current_result = (GData **)user_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  FeatureSet sets = (FeatureSet)user_data ;
  GData **current_result = sets->current_features ;
  GData *current_features = *current_result ;
  GData **diff_result = sets->diff_features ;
  GData *diff_features = *diff_result ;
  GData **new_result = sets->new_features ;
  GData *new_features = *new_result ;

  ZMapFeature current_feature, diff_feature ;
  char *feature_name = NULL ;

  feature_name = (char *)g_quark_to_string(key_id) ;

  zMapAssert(strcmp(feature_name, new_feature->name) == 0) ;

  /* If the feature is not in the current feature set then we can simply add it, if its
   * already there then we don't do anything, i.e. we don't try and merge two features. */
  if (!(current_feature = g_datalist_id_get_data(&(current_features), key_id)))
    {
      gpointer unused ;

      g_datalist_set_data(&(current_features), feature_name, new_feature) ;
      g_datalist_set_data(&(diff_features), feature_name, new_feature) ;


      /* Should remove from new set here but not sure if it will
       * work as part of a foreach loop....which is where this routine is called from... */
      unused = g_datalist_id_remove_no_notify(&(new_features), key_id) ;

    }

  /* Poke the results back in to the original feature set, seems hokey but remember that
   * the address of GData will change as new elements are added so we must poke back the new
   * address. */
  *current_result = current_features ;
  *diff_result = diff_features ;
  *new_result = new_features ;

  return ;
}




/* some debugging stuff........ */
static void printFeatureContext(ZMapFeatureContext context)
{
  char *prefix = "Context" ;

  printf("%s :  %s\n", prefix, context->sequence) ;

  g_datalist_foreach(&(context->feature_sets), printFeatureSet, NULL) ;


  return ;
}

static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  char *prefix = "\t\tFeature Set" ;

  printf("%s :  %s\n", prefix, feature_set->source) ;

  g_datalist_foreach(&(feature_set->features), printFeature, NULL) ;

  return ;
}


static void printFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  char *prefix = "\t\t\t\tFeature" ;

  printf("%s :  %s\n", prefix, feature->name) ;

  return ;
}


/*! @} end of zmapfeatures docs. */

