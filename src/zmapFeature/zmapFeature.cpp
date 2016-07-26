/*  File: zmapFeature.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description: Interface to ZMapFeature objects, includes code to 
 *              to create/modify/destroy them. Note make calls out to
 *              files for feature subtypes, e.g. alignments, transcripts.
 *
 * Exported functions: See zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>


#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtilsDebug.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapUtils.hpp>
#include <zmapFeature_P.hpp>



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
  int length;
} DataListLengthStruct, *DataListLength ;




// 
//                    Globals
//    




/*
 *                               External interface routines.
 */



/* A set of functions for allocating, populating and destroying features.
 * The feature create and add data are in two steps because currently the required
 * use is to have a struct that may need to be filled in in several steps because
 * in some data sources the data comes split up in the datastream (e.g. exons in
 * GFF). If there is a requirement for the two bundled then it should be implemented
 * via a simple new "create and add" function that merely calls both the create and
 * add functions from below. */



/* Returns a single feature correctly initialised to be a "NULL" feature. */
ZMapFeature zMapFeatureCreateEmpty(void)
{
  ZMapFeature feature ;

  feature = (ZMapFeature)zmapFeatureAnyCreateFeature(ZMAPFEATURE_STRUCT_FEATURE, NULL,
                                                     ZMAPFEATURE_NULLQUARK, ZMAPFEATURE_NULLQUARK,
                                                     NULL) ;
  feature->db_id = ZMAPFEATUREID_NULL ;
  feature->mode = ZMAPSTYLE_MODE_INVALID ;

  return feature ;
}


/* ==================================================================
 * Because the contents of this are quite a lot of work. Useful for
 * creating single features, but be warned that usually you will need
 * to keep track of uniqueness, so for large parser the GFF style of
 * doing things is better
 * ==================================================================
 */
ZMapFeature zMapFeatureCreateFromStandardData(const char *name, const char *sequence, const char *ontology,
                                              ZMapStyleMode feature_type,
                                              ZMapFeatureTypeStyle *style,
                                              int start, int end,
                                              gboolean has_score, double score,
                                              ZMapStrand strand)
{
  ZMapFeature feature = NULL;
  gboolean good = FALSE;

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


/* Adds the standard data fields to an empty feature. */
gboolean zMapFeatureAddStandardData(ZMapFeature feature, const char *feature_name_id, const char *name,
                                    const char *sequence, const char *SO_accession,
                                    ZMapStyleMode feature_mode,
                                    ZMapFeatureTypeStyle *style,
                                    int start, int end,
                                    gboolean has_score, double score,
                                    ZMapStrand strand)
{
  gboolean result = FALSE ;

  if (!feature)
    return result ;

  if (feature->unique_id == ZMAPFEATURE_NULLQUARK)
    {
      feature->unique_id = g_quark_from_string(feature_name_id) ;
      feature->original_id = g_quark_from_string(name) ;
      feature->mode = feature_mode ;
      feature->SO_accession = g_quark_from_string(SO_accession) ;
      feature->style = style;
      feature->x1 = start ;
      feature->x2 = end ;
      feature->strand = strand ;
      if (has_score)
        {
          feature->flags.has_score = 1 ;
          feature->score = (float) score ;
        }

      /* will need expanding.... */
      switch (feature->mode)
        {
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          {
            result = zMapFeatureTranscriptInit(feature) ;

            break ;
          }
        default:
          {
            break ;
          }
        }

      result = TRUE ;
    }

  /* UGH....WHAT IS THIS LOGIC....HORRIBLE...... */
  if(feature->unique_id)      /* some DAS servers give use  Name "" which is an error */
    result = TRUE ;


  return result ;
}


/* Adds data to a feature which may be empty or may already have partial features,
 * e.g. transcript that does not yet have all its exons.
 *
 * NOTE that really we need this to be a polymorphic function so that the arguments
 * are different for different features.
 *  */
gboolean zMapFeatureAddKnownName(ZMapFeature feature, char *known_name)
{
  gboolean result = FALSE ;
  GQuark known_id ;

  if (!(feature && (feature->mode == ZMAPSTYLE_MODE_BASIC || feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)) )
    return result ;

  known_id = g_quark_from_string(known_name) ;

  if (feature->mode == ZMAPSTYLE_MODE_BASIC)
    feature->feature.basic.known_name = known_id ;
  else
    feature->feature.transcript.known_name = known_id ;

  result = TRUE ;

  return result ;
}





gboolean zMapFeatureAddVariationString(ZMapFeature feature, char *variation_string)
{
  gboolean result = FALSE ;

  if (!(feature && (variation_string && *variation_string)) )
    return result ;

  feature->feature.basic.flags.variation_str = TRUE ;
  feature->feature.basic.variation_str = variation_string ;
  result = TRUE ;

  return result ;
}


gboolean zMapFeatureAddSOaccession(ZMapFeature feature, GQuark SO_accession)
{
  gboolean result = FALSE ;

  if (!feature || !SO_accession)
    return result ;

  feature->SO_accession = SO_accession ;
  result = TRUE ;

  return result ;
}


/* Adds splice data for splice features, usually from gene finder output. */
gboolean zMapFeatureAddSplice(ZMapFeature feature, ZMapBoundaryType boundary)
{
  gboolean result = FALSE ;

  if (!(feature && (boundary == ZMAPBOUNDARY_5_SPLICE || boundary == ZMAPBOUNDARY_3_SPLICE)) )
    return result ;

  feature->flags.has_boundary = TRUE ;
  feature->boundary_type = boundary ;
  result = TRUE ;

  return result ;
}


/* Returns TRUE if feature is a sequence feature and is DNA, FALSE otherwise. */
gboolean zMapFeatureSequenceIsDNA(ZMapFeature feature)
{
  gboolean result = FALSE ;

  if (!zMapFeatureIsValidFull((ZMapFeatureAny) feature, ZMAPFEATURE_STRUCT_FEATURE))
    return result ;

  if (feature->mode == ZMAPSTYLE_MODE_SEQUENCE && feature->feature.sequence.type == ZMAPSEQUENCE_DNA)
    result = TRUE ;

  return result ;
}


/* Returns TRUE if feature is a sequence feature and is Peptide, FALSE otherwise. */
gboolean zMapFeatureSequenceIsPeptide(ZMapFeature feature)
{
  gboolean result = FALSE ;

  if (!zMapFeatureIsValidFull((ZMapFeatureAny) feature, ZMAPFEATURE_STRUCT_FEATURE))
    return result ;

  if (feature->mode == ZMAPSTYLE_MODE_SEQUENCE && feature->feature.sequence.type == ZMAPSEQUENCE_PEPTIDE)
    result = TRUE ;

  return result ;
}

/* Adds assembly path data to a feature. */
gboolean zMapFeatureAddAssemblyPathData(ZMapFeature feature,
                                        int length, ZMapStrand strand, GArray *path)
{
  gboolean result = FALSE ;

  if (!zMapFeatureIsValid((ZMapFeatureAny)feature))
    return result ;

  if (feature->mode == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
    {
      feature->feature.assembly_path.strand = strand ;
      feature->feature.assembly_path.length = length ;
      feature->feature.assembly_path.path = path ;

      result = TRUE ;
    }

  return result ;
}

/* Adds a URL to the object, n.b. we add this as a string that must be freed, this.
 * is because I don't want the global GQuark table to be expanded by these...  */
gboolean zMapFeatureAddURL(ZMapFeature feature, char *url)
{
  gboolean result = FALSE ;                                /* We may add url checking sometime. */

  if (!feature || !url || !*url)
    return result ;

  feature->url = g_strdup(url) ;
  result = TRUE ;

  return result ;
}


/* Adds a Locus to the object. */
gboolean zMapFeatureAddLocus(ZMapFeature feature, GQuark locus_id)
{
  gboolean result = FALSE ;

  if (!feature || !locus_id)
    return result ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      feature->feature.transcript.locus_id = locus_id ;

      result = TRUE ;
    }

  return result ;
}


/* Adds descriptive text the object. */
gboolean zMapFeatureAddText(ZMapFeature feature, GQuark source_id, const char *source_text, char *feature_text)
{
  gboolean result = FALSE ;

  if (!(zMapFeatureIsValidFull((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_FEATURE)
        && (source_id || (source_text && *source_text) || (feature_text && *feature_text))) )
    return result ;

  if (source_id)
    feature->source_id = source_id ;
  if (source_text)
    feature->source_text = g_quark_from_string(source_text) ;
  if (feature_text)
    feature->description = g_strdup(feature_text) ;
  result = TRUE ;

  return result ;
}

/* Add text to the feature::description data member. If the data is set already
 * then delete it first.
 */
gboolean zMapFeatureAddDescription(ZMapFeature feature, char *data )
{
  gboolean result = FALSE ;

  // I'VE SEEN LARGE NUMBERS OF THESE ASSERTS FAILING IN THE OTTER LOGS, BUT IT'S NOT OBVIOUS TO
  //  ME FROM TRACING THE CODE HOW THIS IS HAPPENING SO I'M ADDING SOME EXTRA DEBUG INFO AND
  //  THEN WE CAN TRACE IT THROUGH....
  //  zMapReturnValIfFail(feature && data && *data, result ) ;
  zMapReturnValIfFail(feature, FALSE) ;

  if (!data || !(*data))
    {
      char *feature_name ;
      char *featureset_name ;

      feature_name = zMapFeatureName((ZMapFeatureAny)feature) ;
      featureset_name = zMapFeatureName(feature->parent) ;

      zMapLogWarning("zMapFeatureAddDescription() failed, featureset: \"%s\", feature: \"%s\" with %s string",
                     featureset_name, feature_name,
                     (!data ? "NULL description" : "empty description")) ;
      return FALSE ;
    }

  if (feature->description)
    g_free(feature->description) ;
  feature->description = g_strdup(data) ;

  return result ;
}




/* Returns the length of a feature. For a simple feature this is just (end - start + 1),
 * for transcripts and alignments the exons or blocks must be totalled up.
 *
 * @param   feature      Feature for which length is required.
 * @param   length_type  Length in target sequence coords or query sequence coords or spliced length.
 * @return               nothing.
 *  */
int zMapFeatureLength(ZMapFeature feature, ZMapFeatureLengthType length_type)
{
  int length = 0 ;

  if (!zMapFeatureIsValid((ZMapFeatureAny)feature))
    return length ;

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

        if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
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

        if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.exons)
          {
            unsigned int i ;
            ZMapSpan span ;
            GArray *exons = feature->feature.transcript.exons ;

            length = 0 ;

            for (i = 0 ; i < exons->len ; i++)
              {
                span = &g_array_index(exons, ZMapSpanStruct, i) ;

                length += (span->x2 - span->x1 + 1) ;
              }

          }
        else if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT && feature->feature.homol.align)
          {
            unsigned int i ;
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
        zMapWarnIfReached() ;

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
  if (!feature)
    return ;

  zmapDestroyFeatureAnyWithChildren((ZMapFeatureAny)feature, FALSE) ;

  return ;
}



// 
//                     Package routines
//    

void zmapFeatureBasicCopyFeature(ZMapFeature orig_feature, ZMapFeature new_feature)
{
  if (orig_feature->description)
    new_feature->description = g_strdup(orig_feature->description) ;

  if (orig_feature->url)
    new_feature->url = g_strdup(orig_feature->url) ;

  return ;
}

void zmapFeatureBasicDestroyFeature(ZMapFeature feature)
{
  if (feature->description)
    g_free(feature->description) ;

  if (feature->url)
    g_free(feature->url) ;

  return ;
}



/*
 *            Internal routines.
 */



/* A GHashTableForeachFunc() to add a mode to the styles for all features in a set, note that
 * this is not efficient as we go through all features but we would need more information
 * stored in the feature set to avoid this as there may be several different types of
 * feature stored in a feature set.
 *
 * Note that I'm setting some other style data here because we need different default bumping
 * modes etc for different feature types....
 *
 * Oct 2012: featuresets have a single style, these can be commbined into a virtual featureset
 * in a CanvasFeatureset on display so we can just process the featuresets
 */
void zMapFeatureAddStyleMode(ZMapFeatureTypeStyle style, ZMapStyleMode f_type)
{
  if (!zMapStyleHasMode(style))
    {
      ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID ;

      switch (f_type)
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
          mode = f_type;           /* grrrrr is this really correct? */
          break;
        default:
          zMapWarnIfReached() ;
          break ;
        }

      zMapStyleSetMode(style, mode) ;
    }


  return ;
}






/* sort by genomic coordinate */
/* start coord then end coord reversed */
gint zMapFeatureCmp(gconstpointer a, gconstpointer b)
{
  ZMapFeature feata = (ZMapFeature) a;
  ZMapFeature featb = (ZMapFeature) b;

  if(!featb)
    {
      if(!feata)
        return(0);
      return(1);
    }
  if(!feata)
    return(-1);

  if(feata->x1 < featb->x1)
    return(-1);
  if(feata->x1 > featb->x1)
    return(1);

  if(feata->x2 > featb->x2)
    return(-1);

  if(feata->x2 < featb->x2)
    return(1);

  return(0);
}


/* Make a shallow copy of the given feature. The caller should free the returned ZMapFeature
 * struct with zMapFeatureAnyDestroyShallow */
ZMapFeature zMapFeatureShallowCopy(ZMapFeature src)
{
  ZMapFeature dest = NULL;
  zMapReturnValIfFail(src && src->mode != ZMAPSTYLE_MODE_INVALID, dest) ;

  dest = zMapFeatureCreateEmpty() ;

#ifdef FEATURES_NEED_MAGIC
  dest->magic = src->magic ;
#endif

  dest->struct_type = src->struct_type ;
  dest->parent = src->parent ;
  dest->unique_id = src->unique_id ;
  dest->original_id = src->original_id ;
  dest->no_children = src->no_children ;
  dest->flags.has_score = src->flags.has_score ;
  dest->flags.has_boundary = src->flags.has_boundary ;
  dest->flags.collapsed = src->flags.collapsed ;
  dest->flags.squashed = src->flags.squashed ;
  dest->flags.squashed_start = src->flags.squashed_start ;
  dest->flags.squashed_end = src->flags.squashed_end ;
  dest->flags.joined = src->flags.joined ;
  dest->children = src->children ;
  dest->composite = src->composite ;
  dest->db_id = src->db_id ;
  dest->mode = src->mode ;
  dest->SO_accession = src->SO_accession ;
  dest->style = src->style;
  dest->x1 = src->x1 ;
  dest->x2 = src->x2 ;
  dest->strand = src->strand ;
  dest->boundary_type = src->boundary_type ;
  dest->score = src->score ;
  dest->population = src->population;
  dest->source_id = src->source_id ; 
  dest->source_text = src->source_text ;
  dest->description = src->description ;
  dest->url = src->url ;
  dest->feature = src->feature ;

  return dest ;
}


/* Make a shallow copy of the given featureset. The caller should free the returned ZMapFeatureSet
 * struct with zMapFeatureAnyDestroyShallow */
ZMapFeatureSet zMapFeatureSetShallowCopy(ZMapFeatureSet src)
{
  ZMapFeatureSet dest = NULL;
  zMapReturnValIfFail(src, dest) ;

  dest = zMapFeatureSetIDCreate(src->original_id, src->unique_id, src->style, src->features) ;

#ifdef FEATURES_NEED_MAGIC
  dest->magic = src->magic ;
#endif

  dest->parent = src->parent ;
  dest->description = src->description ;
  dest->masker_sorted_features = src->masker_sorted_features ;
  dest->loaded = src->loaded;

  return dest ;
}


/* Return the homol type for the given featureset, if it is an alignment. Returns ZMAPHOMOL_NONE
 * if not applicable or not found. */
ZMapHomolType zMapFeatureSetGetHomolType(ZMapFeatureSet feature_set)
{
  ZMapHomolType result = ZMAPHOMOL_NONE ;

  if (feature_set && 
      feature_set->features && 
      feature_set->style && 
      feature_set->style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      /* Assume that all features in the featureset are the same type so just 
       * check the first one. */
      GList *first = g_hash_table_get_values(feature_set->features) ;

      if (first)
        {
          ZMapFeature feature = (ZMapFeature)(first->data) ;

          if (feature)
            result = feature->feature.homol.type;
        }
    }

  return result ;
}




