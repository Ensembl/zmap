/*  File: zmapFeatureTranscript.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2011-2014: Genome Research Ltd.
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
 * Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions for processing/handling transcript features.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <zmapFeature_P.h>

#include <ZMap/zmapGLibUtils.h>


/* Custom responses for the trim-exon confirmation dialog */
enum {ZMAP_TRIM_RESPONSE_START, ZMAP_TRIM_RESPONSE_END};


typedef struct
{
  ZMapFeature feature ;

  int feature_start ;
  int feature_end ;
  int feature_coord_counter ;


  /* Keeps track of where we are in whole transcript. */
  int spliced_coord_counter ;


  /* CDS stuff, not filled in if no CDS. */
  gboolean cds ;

  /* Start/end of CDS and counter within CDS. */
  int cds_start, cds_end ;
  int cds_coord_counter ;

  /* not needed anymore ??? */
  int cds_offset ;

  /* Start/End of translation within CDS and counter for where we are in translation.
   * Translation start may be offset because beginning of CDS may not be known and a
   * start_not_found correction for translation is given. If start_not_found is FALSE
   * then translation start/end/counter are same as cds values. */
  gboolean start_not_found ;
  int start_offset ;

  int trans_start ;
  int trans_end ;
  int trans_coord_counter ;

  GList **full_exons ;

  /* optional string to hold peptide translation of this part of the CDS. */
  char *translation ;

} ItemShowTranslationTextDataStruct, *ItemShowTranslationTextData ;


/* Utility struct to store info required when constructing detailed exon data */
typedef struct _CreateExonsDataStruct
{
  ZMapSpanStruct ex_utr_5 ;
  ZMapSpanStruct ex_split_5 ;
  ZMapSpanStruct ex_start_not_found ;
  ZMapSpanStruct ex_cds ; 
  ZMapSpanStruct ex_split_3 ; 
  ZMapSpanStruct ex_utr_3 ;
  GList *full_exon_cds_list ; /* we might have multiple parts to the CDS if it is split up by
                               * variations */
  ZMapFullExon full_exon_start_not_found ;
  ZMapFullExon full_exon_utr_5 ;
  ZMapFullExon full_exon_utr_3 ;
  ZMapFullExon full_exon_split_5 ;
  ZMapFullExon full_exon_split_3 ;
} CreateExonsDataStruct, *CreateExonsData ;


static void extendTranscript(ZMapFeature transcript, ZMapSpanStruct * span) ;

static void getDetailedExon(gpointer exon_data, gpointer user_data) ;
static void calculateExonPositions(ItemShowTranslationTextData full_data, ZMapSpan exon_span, 
                                   GList *variations, CreateExonsData exons_data) ;
static void createFullExonStructs(ItemShowTranslationTextData full_data, ZMapFeature feature, 
                                  GList *variations, CreateExonsData exons_data) ;
static ZMapFullExon exonCreate(int feature_start, ExonRegionType region_type, ZMapSpan exon_span, GList *variations,
                               int *curr_feature_pos, int *curr_spliced_pos,
                               int *curr_cds_pos, int *trans_spliced_pos) ;
static GList* exonCreateCDS(const int feature_start, ZMapSpan exon_span, GList *variations,
                            int *curr_feature_pos, int *curr_spliced_pos,
                            int *curr_cds_pos, int *curr_trans_pos) ;
static ZMapFullExon exonCreateCDSSection(const int feature_start, GList *variations,
                                         const int section_start_coord, const int section_end_coord,
                                         int *curr_feature_pos, int *curr_spliced_pos,
                                         int *curr_cds_pos, int *curr_trans_pos) ;
static void createTranslationForCDS(ItemShowTranslationTextData full_data, CreateExonsData exons_data) ;
static void exonDestroy(ZMapFullExon exon) ;
static void exonListFree(gpointer data, gpointer user_data_unused) ;
static void printDetailedExons(gpointer exon_data, gpointer user_data) ;





/*
 *               External functions.
 */


/* Initialises a transcript feature.
 *
 *  */
gboolean zMapFeatureTranscriptInit(ZMapFeature feature)
{
  gboolean result = FALSE ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT
      && (!(feature->feature.transcript.exons) && !(feature->feature.transcript.introns)))
    {

      feature->feature.transcript.exons = g_array_sized_new(FALSE, TRUE,
    sizeof(ZMapSpanStruct), 30) ;

      feature->feature.transcript.introns = g_array_sized_new(FALSE, TRUE,
      sizeof(ZMapSpanStruct), 30) ;

      result = TRUE ;
    }

  return result ;
}

/*
 * Adds CDS start and end to a feature but tests values as we go along to make sure
 * that the coordinates are only changed if the range is being expanded. The phase is
 * only stored if the _start_ of the range is expanded.
 */
gboolean zMapFeatureAddTranscriptCDSDynamic(ZMapFeature feature, Coord start, Coord end, ZMapPhase phase,
                                            gboolean bStartNotFound, gboolean bEndNotFound, int iStartNotFound)
{
  gboolean result = FALSE ;

  if (!feature || (feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT))
    return result ;
  if (!start || !end)
    return result ;
  if (end < start)
    return result ;
  Coord start_s = feature->feature.transcript.cds_start,
        end_s   = feature->feature.transcript.cds_end ;

  /*
   * Set flag for this feature to be a CDS.
   */
  feature->feature.transcript.flags.cds = 1 ;

  /* first case is if the data have not yet been set, and were initialized to zero */
  if (!start_s || !end_s)
    {
      feature->feature.transcript.cds_start = start ;
      feature->feature.transcript.cds_end = end ;
      feature->feature.transcript.phase = phase ;
      result = TRUE ;
    }
  else /* we have non-zero values for cds_start and cds_end */
    {
      if (start < start_s)
        {
          feature->feature.transcript.cds_start = start ;
          feature->feature.transcript.phase = phase ;
        }
      if (end > end_s)
        {
          feature->feature.transcript.cds_end = end ;
        }
      result = TRUE ;
    }

  if (bStartNotFound)
    {
      feature->feature.transcript.flags.start_not_found = 1 ;
      feature->feature.transcript.start_not_found = iStartNotFound ;
    }
  if (bEndNotFound)
    {
      feature->feature.transcript.flags.end_not_found = 1 ;
    }

  return result ;
}


/* Adds initial data to a transcript feature, will overwrite any existing settings. */
gboolean zMapFeatureAddTranscriptCDS(ZMapFeature feature,
     gboolean cds, Coord cds_start, Coord cds_end)
{
  gboolean result = FALSE ;

  if (!feature || (feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT))
    return result ;

  /* There ought to be sanity checking of coords of cds/exons/introns here.... */
  if (cds)
    {
      feature->feature.transcript.flags.cds = 1 ;
      feature->feature.transcript.cds_start = cds_start ;
      feature->feature.transcript.cds_end = cds_end ;

      result = TRUE ;
    }

  return result ;
}


/* Merges CDS details from one transcript feature to another */
gboolean zMapFeatureMergeTranscriptCDS(ZMapFeature src_feature, ZMapFeature dest_feature)
{
  gboolean result = FALSE ;

  if (!src_feature || (src_feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
      || (dest_feature && (dest_feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT)))
    return result ;

  /* There ought to be sanity checking of coords of cds/exons/introns here.... */
  if (src_feature->feature.transcript.flags.cds)
    {
      dest_feature->feature.transcript.flags.cds = src_feature->feature.transcript.flags.cds ;

      /* We only use the src coords if it means the CDS region will expand. However, 0 means
       * unset so always use the src coord if the current one is unset. */
      if (dest_feature->feature.transcript.cds_start == 0 ||
          src_feature->feature.transcript.cds_start < dest_feature->feature.transcript.cds_start)
        {
          dest_feature->feature.transcript.cds_start = src_feature->feature.transcript.cds_start ;
        }

      if (dest_feature->feature.transcript.cds_end == 0 ||
          src_feature->feature.transcript.cds_end > dest_feature->feature.transcript.cds_end)
        {
          dest_feature->feature.transcript.cds_end = src_feature->feature.transcript.cds_end ;
        }

      result = TRUE ;
    }

  return result ;
}


/* Add start/end "not found" data to a transcript feature. */
gboolean zMapFeatureAddTranscriptStartEnd(ZMapFeature feature,
  gboolean start_not_found_flag, int start_not_found,
  gboolean end_not_found_flag)
{
  gboolean result = FALSE ;

  if (!(feature && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT
     && (!start_not_found_flag || (start_not_found_flag && (start_not_found >= 1 || start_not_found <= 3)))) )
    return result ;

  result = TRUE ;
  if (start_not_found_flag)
    {
      feature->feature.transcript.flags.start_not_found = TRUE ;
      feature->feature.transcript.start_not_found = start_not_found ;
    }

  if (end_not_found_flag)
    feature->feature.transcript.flags.end_not_found = 1 ;

  return result ;
}


/* Adds a single exon and/or intron to an existing transcript feature.
 *
 * NOTE: extends the transcripts start/end coords if introns and/or exons
 * exceed these coords.
 *  */
gboolean zMapFeatureAddTranscriptExonIntron(ZMapFeature feature,
    ZMapSpanStruct *exon, ZMapSpanStruct *intron)
{
  gboolean result = FALSE ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && (exon || intron))
    {
      if (exon)
        {
          g_array_append_val(feature->feature.transcript.exons, *exon) ;

          extendTranscript(feature, exon) ;

          result = TRUE ;
        }

      if (intron)
        {
          g_array_append_val(feature->feature.transcript.introns, *intron) ;

          extendTranscript(feature, intron) ;

          result = TRUE ;
        }
    }


  return result ;
}


/* Returns a variation in the list that overlaps the given variation (or null if none) */
static ZMapFeature findOverlappingVariation(ZMapFeature variation, GList *compare_list)
{
  ZMapFeature result = NULL ;

  GList *compare_item = compare_list ;

  for ( ; compare_item && !result; compare_item = compare_item->next)
    {
      ZMapFeature compare_feature = (ZMapFeature)(compare_item->data) ;
      if ((compare_feature->x1 >= variation->x1 && compare_feature->x1 <= variation->x2) ||
          (compare_feature->x2 >= variation->x1 && compare_feature->x2 <= variation->x2))
        {
          result = compare_feature ;
        }
    }
  
  return result ;
}

/* Remove all the variation metadata in a transcript. */
gboolean zMapFeatureRemoveTranscriptVariations(ZMapFeature feature, 
                                               GError **error)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(feature && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT, result) ;
  
  g_list_free(feature->feature.transcript.variations) ;
  feature->feature.transcript.variations = NULL ;

  return TRUE ;
}


/* Add a variation to a transcript feature. Stores the variation as metadata which is used
 * to modify the transcript sequence. Only applies the variation if it lies within an exon. */
gboolean zMapFeatureAddTranscriptVariation(ZMapFeature feature, 
                                           ZMapFeature variation,
                                           GError **error)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(feature && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT, result) ;
  zMapReturnValIfFail(variation && variation->mode == ZMAPSTYLE_MODE_BASIC, result) ;

  /* Find which exon the variation lies in */
  GArray *exons = feature->feature.transcript.exons;
  int i = 0 ;

  for ( ; i < exons->len; ++i)
    {
      ZMapSpan exon = &(g_array_index(exons, ZMapSpanStruct, i)) ;

      if (variation->x1 >= exon->x1 && variation->x2 <= exon->x2)
        {
          /* Ok, found the exon. Add the variation to the transcript (but
           * disallow adding of overlapping variations). */
          ZMapFeature overlapping_variation = findOverlappingVariation(variation, feature->feature.transcript.variations) ;

          if (!overlapping_variation)
            {
              feature->feature.transcript.variations = 
                g_list_insert_sorted(feature->feature.transcript.variations, 
                                     variation,
                                     zMapFeatureCmp) ;
              
              result = TRUE ;
            }
          else if (overlapping_variation->mode == ZMAPSTYLE_MODE_BASIC &&
                   overlapping_variation->feature.basic.variation_str)
            {
              g_set_error(error, g_quark_from_string("ZMap"), 99, 
                          "Cannot add variation '%s' because it overlaps existing variation '%s'\n",
                          variation->feature.basic.variation_str,
                          overlapping_variation->feature.basic.variation_str) ;
            }
          else
            {
              g_set_error(error, g_quark_from_string("ZMap"), 99, 
                          "Cannot add variation '%s' because it overlaps existing variation '<invalid type>'\n",
                          variation->feature.basic.variation_str) ;
            }

          break ;
        }
    }

  return result ;
}


/* Removes all exons */
void zMapFeatureRemoveExons(ZMapFeature feature)
{
  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.exons)
    {
      g_array_free(feature->feature.transcript.exons, TRUE);
      feature->feature.transcript.exons = g_array_sized_new(FALSE, TRUE, sizeof(ZMapSpanStruct), 30);
    }
}


/* Removes all introns */
void zMapFeatureRemoveIntrons(ZMapFeature feature)
{
  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.introns)
    {
      g_array_free(feature->feature.transcript.introns, TRUE);
      feature->feature.transcript.introns = g_array_sized_new(FALSE, TRUE, sizeof(ZMapSpanStruct), 30);
    }
}


/* Create introns based on exons */
void zMapFeatureTranscriptRecreateIntrons(ZMapFeature feature)
{
  /* First remove any existing introns */
  zMapFeatureRemoveIntrons(feature) ;

  GArray *exons;
  int multiplier = 1, start = 0, end, i;
  gboolean forward = TRUE;

  if (feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
    return ;

  exons = feature->feature.transcript.exons;

  if (exons->len > 1)
    {
      ZMapSpan first, last;
      first = &(g_array_index(exons, ZMapSpanStruct, 0));
      last  = &(g_array_index(exons, ZMapSpanStruct, exons->len - 1));

      if(first->x1 > last->x1)
        forward = FALSE;
    }

  if (forward)
    {
      end = exons->len;
    }
  else
    {
      multiplier = -1;
      start = (exons->len * multiplier) + 1;
      end   = 1;
    }

  for (i = start; i < end - 1; i++)
    {
      int index = i * multiplier ;
      ZMapSpan exon1 = &(g_array_index(exons, ZMapSpanStruct, index)) ;
      ZMapSpan exon2 = &(g_array_index(exons, ZMapSpanStruct, index + multiplier)) ;

      /* Only create an intron if exons are not butted up against one another. */
      if (exon1->x2 + 1 < exon2->x1)
        {
          ZMapSpan intron ;

          intron = g_malloc0(sizeof *intron);
          intron->x1 = exon1->x2 + 1 ;
          intron->x2 = exon2->x1 - 1 ;

          zMapFeatureAddTranscriptExonIntron(feature, NULL, intron) ;
        }
    }

  return ;
}


/* Checks that transcript has at least one exon, if not then adds an exon to
 * cover entire extent of transcript.
 *
 * If there are no introns then it adds introns derived from the existing
 * exons.
 *
 * Returns TRUE if the transcript did not need normalising or if it was
 * normalised successfully, FALSE otherwise.
 */
gboolean zMapFeatureTranscriptNormalise(ZMapFeature feature)
{
  gboolean result = TRUE ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      if (!(feature->feature.transcript.exons->len))
        {
          ZMapSpanStruct exon = {0}, *exon_ptr = NULL ;

          exon.x1 = feature->x1 ;
          exon.x2 = feature->x2 ;
          exon_ptr = &exon ;

          result = zMapFeatureAddTranscriptExonIntron(feature, exon_ptr, NULL) ;
        }
      else if (!(feature->feature.transcript.introns->len))
        {
          zMapFeatureTranscriptRecreateIntrons(feature) ;
        }
    }

  return result ;
}



/* Takes a transcript feature and produces a list of "annotated" exon regions
 * which include information about 5' and 3' split codons, frame and much else,
 * see the ZMapFullExon struct.
 *
 */
gboolean zMapFeatureAnnotatedExonsCreate(ZMapFeature feature, gboolean include_protein, gboolean pad, 
                                         GList **exon_regions_list_out)
{
  gboolean result = FALSE ;
  gboolean exon_debug = FALSE ;

  if (ZMAPFEATURE_IS_TRANSCRIPT(feature))
    {
      ItemShowTranslationTextDataStruct full_data = {NULL} ;

      full_data.feature = feature ;
      full_data.feature_start = feature->x1 ;
      full_data.feature_end = feature->x2 ;
      full_data.feature_coord_counter = 0 ;

      if (ZMAPFEATURE_HAS_CDS(feature))
        {
          full_data.cds = TRUE ;

          full_data.trans_start = full_data.cds_start = feature->feature.transcript.cds_start ;
          full_data.trans_end = full_data.cds_end = feature->feature.transcript.cds_end ;

          if (feature->feature.transcript.flags.start_not_found)
            {
              full_data.start_not_found = feature->feature.transcript.flags.start_not_found ;
              full_data.start_offset = feature->feature.transcript.start_not_found ;
              full_data.trans_start = (full_data.trans_start + full_data.start_offset) - 1 ;
            }
        }


      full_data.full_exons = exon_regions_list_out ;

      /* SHOULD WE ALLOW USER TO TRANSLATE ANY TRANSCRIPT ?? PROBABLY USEFUL....
       * THINK ABOUT THIS.... */
      if (include_protein)
        {
          int real_length;

          full_data.translation = zMapFeatureTranslation(feature, &real_length, pad);
        }


      zMapFeatureTranscriptExonForeach(feature, getDetailedExon, &full_data) ;

      if (g_list_length(*exon_regions_list_out) > 0)
        result = TRUE ;

      if (exon_debug)
        g_list_foreach(*exon_regions_list_out, printDetailedExons, NULL) ;

      /* err....don't understand the code here.... */
      if (include_protein && full_data.translation)
        g_free(full_data.translation) ;
    }


  return result ;
}



void zMapFeatureAnnotatedExonsDestroy(GList *exon_list)
{
  g_list_foreach(exon_list, exonListFree, NULL) ;
  g_list_free(exon_list) ;

  return ;
}



/* ensure that exons are in fwd strand order
 * these come sorted from ACEDB and in reverse order for -ve strand from pipe servers (otterlace)
 * but GFF does not enforce this so to be safe we need to do this
 */
ZMapFeatureContextExecuteStatus zMapFeatureContextTranscriptSortExons(GQuark key,
      gpointer data,
      gpointer user_data,
      char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  ZMapFeatureTypeStyle style ;
  GList *features = NULL ;


  if (!feature_any || !zMapFeatureIsValid(feature_any))
    return status ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align = NULL;
        feature_align = (ZMapFeatureAlignment)feature_any;

        break;
      }
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock feature_block = NULL;
        feature_block = (ZMapFeatureBlock)feature_any;

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = NULL;

        feature_set = (ZMapFeatureSet)feature_any;
        style = feature_set->style;

        if(!style || zMapStyleGetMode(style) != ZMAPSTYLE_MODE_TRANSCRIPT)
          break;

        zMap_g_hash_table_get_data(&features, feature_set->features);

        for ( ; features; features = g_list_delete_link(features, features))
          {
            ZMapFeature feat = (ZMapFeature) features->data;

            zMapFeatureTranscriptSortExons(feat) ;
          }

        break;
      }

    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
        zMapWarnIfReached();
        break;
      }
    }


  return status;
}


/*!
 * \brief Get the number of exons in the given transcript.
 *
 * /return The number of exons, or -1 if the feature is not a transcript.
 */
int zMapFeatureTranscriptGetNumExons(ZMapFeature transcript)
{
  int result = -1;

  if (transcript && transcript->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      if (transcript->feature.transcript.exons)
        result = transcript->feature.transcript.exons->len;
    }

  return result;
}


/*!
 * \brief Merge a new intron into the given transcript.
 *
 * The given coords represent the start/end of the new intron to be
 * created. If these coords overlap existing intron(s) then the existing
 * introns(s) will be extended rather than a new intron being created.
 */
gboolean zMapFeatureTranscriptMergeIntron(ZMapFeature transcript, Coord x1, Coord x2)
{
  gboolean result = FALSE ;
  GError *tmp_error = NULL ;

  if (!transcript || transcript->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
    return result;

  /* Disallow merging an intron that would leave us with no exon at the start/end */
  if (x1 <= transcript->x1 || x2 >= transcript->x2)
    {
      zMapWarning("%s", "Cannot merge an intron whose start/end lies outside the transcript range.") ;
      g_error_free(tmp_error) ;
    }
  else
    {
      /* Just merge each coord separately, the first as a 5' splice and the second as 3' */
      ZMapBoundaryType boundary = ZMAPBOUNDARY_5_SPLICE ;
      zMapFeatureTranscriptMergeCoord(transcript, x1, &boundary, &tmp_error) ;

      if (!tmp_error)
        {
          boundary = ZMAPBOUNDARY_3_SPLICE ;
          zMapFeatureTranscriptMergeCoord(transcript, x2, &boundary, &tmp_error) ;
        }

      result = TRUE ;
    }

  if (tmp_error)
    {
      zMapWarning("%s", tmp_error->message) ;
      g_error_free(tmp_error) ;
      result = FALSE ;
    }

  return result ;
}


/*!
 * \brief Merge a new exon into the given transcript.
 *
 * The given coords represent the start/end of the new exon to be
 * created. If these coords overlap existing exon(s) then the existing
 * exon(s) will be extended rather than a new exon being created.
 */
gboolean zMapFeatureTranscriptMergeExon(ZMapFeature transcript, Coord x1, Coord x2)
{
  gboolean result = FALSE ;

  if (!transcript || transcript->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
    return result;

  result = TRUE ;

  /* Loop through existing exons to determine how to do the merge.
   * Simple logic at the moment: if the new exon overlaps existing
   * exon(s) then replace them, otherwise insert a new exon. */
  gboolean replace = FALSE;
  gboolean overlaps = FALSE;
  GArray *array = transcript->feature.transcript.exons;
  int i = 0;
  int start = x1;
  int end = x2;

  for ( ; i < array->len; ++i)
    {
      /* Check if the new exon overlaps the existing one. Include one base
       * beyond the end of the existing exon so that abutting exons get merged */
      ZMapSpan compare_exon = &(g_array_index(array, ZMapSpanStruct, i));

      if (x2 >= compare_exon->x1 - 1 && x1 <= compare_exon->x2 + 1)
        {
          overlaps = TRUE;

          /* Check if it's different to the existing exon (otherwise we just ignore it) */
          if (x1 != compare_exon->x1 || x2 != compare_exon->x2)
            {
              replace = TRUE;

              /* Adjust new start/end to include the existing exon */
              if (compare_exon->x1 < start)
                start = compare_exon->x1;

              if (compare_exon->x2 > end)
                end = compare_exon->x2;

              /* Remove the existing exon. Note that we don't
               * increment the index because the subsequent array
               * elements are shuffled down one. */
              g_array_remove_index(array, i);
              --i;
            }
        }
    }

  /* Create the new exon. Only do this if replacing exons or
   * if we didn't overlap any exons at all (i.e. inside an intron) */
  if (replace || !overlaps)
    {
      ZMapSpan new_exon = g_malloc0(sizeof *new_exon);
      new_exon->x1 = start;
      new_exon->x2 = end;

      zMapFeatureAddTranscriptExonIntron(transcript, new_exon, NULL);
      zMapFeatureTranscriptSortExons(transcript);

      /* If this is the first/last exon set the start/end coord */
      extendTranscript(transcript, new_exon);
    }

  zMapFeatureTranscriptRecreateIntrons(transcript);

  return result ;
}


/*!
 * \brief Delete the intron/exon at the given coord
 *
 * Find the intron/exon at the given coord and delete it. If there is no intron/exon at this
 * coord, returns false and does nothing. If an intron/exon is found, the adjacent exons/introns
 * are merged.
 */
gboolean zMapFeatureTranscriptDeleteSubfeatureAtCoord(ZMapFeature transcript, Coord coord)
{
  gboolean result = FALSE ;

  if (!transcript || transcript->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
    return result;

  /* Loop through the exons looking for one containing the coord, or a gap (i.e. intron)
   * containing the coord. We sort out the exons first and then recreate the introns at the end. */
  GArray *array = transcript->feature.transcript.exons;
  ZMapSpan prev_exon = NULL ;
  int i = 0;

  for ( ; i < array->len; ++i)
    {
      ZMapSpan exon = &(g_array_index(array, ZMapSpanStruct, i));

      if (i == 0 && coord < exon->x1)
        {
          /* The coord is before the first exon - there's no feature here so nothing to delete */
          result = FALSE ;
          break ;
        }
      else if (i == array->len - 1 && coord > exon->x2)
        {
          /* The coord is after the last exon - there's no feature here so nothing to delete */
          result = FALSE ;
          break ;
        }
      else if (coord >= exon->x1 && coord <= exon->x2)
        {
          /* The coord is in this exon - just delete this exon */
          g_array_remove_index(array, i) ;
          result = TRUE ;
          break ;
        }
      else if (coord < exon->x1 && prev_exon)
        {
          /* The coord is in the gap between this and the next exon. Merge the two exons. */
          prev_exon->x2 = exon->x2 ;
          g_array_remove_index(array, i) ;

          result = TRUE ;
          break ;
        }

      prev_exon = exon ;
    }

  zMapFeatureTranscriptRecreateIntrons(transcript);

  return result ;
}





/* 
 *            Package routines.
 */

/* Do boundary_start/boundary_end match the start/end of the transcript or any of the
 * exons within the transcript. */
gboolean zmapFeatureTranscriptHasMatchingBoundary(ZMapFeature feature,
                                                  int boundary_start, int boundary_end,
                                                  int *boundary_start_out, int *boundary_end_out)
{
  gboolean result = FALSE ;
  int slop ;
  GArray *array = feature->feature.transcript.exons;
  int i ;
  int match_start, match_end ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  slop = zMapStyleSpliceHighlightTolerance(*(feature->style)) ;

  for ( i = 0 ; i < array->len; ++i)
    {
      ZMapSpan exon = &(g_array_index(array, ZMapSpanStruct, i)) ;

      if (zmapFeatureCoordsMatch(slop, boundary_start, boundary_end,
                                 exon->x1, exon->x2,
                                 &match_start, &match_end))
        {
          result = TRUE ;

          if (boundary_start_out && match_start)
            *boundary_start_out = match_start ;

          if (boundary_end_out && match_end)
            *boundary_end_out = match_end ;
        }
    }

  return result ;
}



/* 
 *           Internal routines
 */



/*!
 * \brief Ask the user whether to trim the start/end of the given exon
 * or, if in_intron is true, the previous intron.
 */
static ZMapBoundaryType showTrimStartEndConfirmation()
{
  ZMapBoundaryType result = ZMAPBOUNDARY_NONE;

  GtkWidget *dialog = gtk_dialog_new_with_buttons("Confirm edit operation",
                                                  NULL,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  "5'", ZMAP_TRIM_RESPONSE_START,
                                                  "3'", ZMAP_TRIM_RESPONSE_END,
                                                  NULL);

  char *message_text = g_strdup_printf("Is this a 5' or 3' intron boundary?");

  GtkWidget *message = gtk_label_new(message_text);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), message, FALSE, FALSE, 10);

  g_free(message_text);

  gtk_widget_show_all(dialog);

  gint response = gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);

  if (response == ZMAP_TRIM_RESPONSE_START)
    {
      result = ZMAPBOUNDARY_5_SPLICE;
    }
  else if (response == ZMAP_TRIM_RESPONSE_END)
    {
      result = ZMAPBOUNDARY_3_SPLICE;
    }

  return result;
}


/* special case: we only have one exon and x lies inside it */
static gboolean mergeCoordInsideSoleExon(ZMapFeature transcript,
                                         ZMapBoundaryType *boundary_inout,
                                         const int x,
                                         ZMapSpan exon,
                                         GError **error)
{
  gboolean merged = FALSE ;

  if (*boundary_inout == ZMAPBOUNDARY_NONE)
    *boundary_inout = showTrimStartEndConfirmation();

  if (*boundary_inout == ZMAPBOUNDARY_5_SPLICE)
    {
      /* extend the start of the next intron i.e. trim the end of this exon */
      exon->x2 = x;
      merged = TRUE;
    }
  else if (*boundary_inout == ZMAPBOUNDARY_3_SPLICE)
    {
      /* extend the end of prev intron i.e. trim the start of this exon */
      exon->x1 = x;
      merged = TRUE;
    }

  return merged ;
}


/* x lies inside or before the first exon: trim/extend its start
 * check that the boundary type matches (i.e. modify 3' end of adjacent intron) */
static gboolean mergeCoordInsideFirstExon(ZMapFeature transcript,
                                          ZMapBoundaryType *boundary_inout,
                                          const int x,
                                          ZMapSpan exon,
                                          GError **error)
{
  gboolean merged = FALSE ;

  if (*boundary_inout == ZMAPBOUNDARY_NONE || *boundary_inout == ZMAPBOUNDARY_3_SPLICE)
    {
      exon->x1 = x;
      transcript->x1 = x;
      merged = TRUE;
    }
  else
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99, "Invalid boundary type for this operation");
    }

  return merged ;
}


/* x lies inside or after the last exon: trim/extend its end */
static gboolean mergeCoordOutsideLastExon(ZMapFeature transcript,
                                          ZMapBoundaryType *boundary_inout,
                                          const int x,
                                          ZMapSpan exon,
                                          GError **error)
{
  gboolean merged = FALSE ;

  if (*boundary_inout == ZMAPBOUNDARY_NONE || *boundary_inout == ZMAPBOUNDARY_5_SPLICE)
   {
      exon->x2 = x;
      transcript->x2 = x;
      merged = TRUE;
    }
  else
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99, "Invalid boundary type for this operation");
    }

  return merged ;
}


/* x lies in the intron before this exon: ask whether
 * to trim the start or end of the intron */
static gboolean mergeCoordInIntron(ZMapFeature transcript,
                                   ZMapBoundaryType *boundary_inout,
                                   const int x,
                                   ZMapSpan prev_exon,
                                   ZMapSpan exon,
                                   GError **error)
{
  gboolean merged = FALSE ;

  if (*boundary_inout == ZMAPBOUNDARY_NONE)
    *boundary_inout = showTrimStartEndConfirmation();

  if (*boundary_inout == ZMAPBOUNDARY_5_SPLICE)
    {
      /* to trim the intron start, we extend the prev exon end */
      prev_exon->x2 = x;
      merged = TRUE;
    }
  else if (*boundary_inout == ZMAPBOUNDARY_3_SPLICE)
    {
      /* to trim the intron end, extend the current exon start */
      exon->x1 = x;
      merged = TRUE;
    }

  return merged;
}


  /* x lies in this exon: ask whether to trim the start or end*/
static gboolean mergeCoordInExon(ZMapFeature transcript,
                                 ZMapBoundaryType *boundary_inout,
                                 const int x,
                                 ZMapSpan exon,
                                 GError **error)
{
  gboolean merged = FALSE ;

  if (*boundary_inout == ZMAPBOUNDARY_NONE)
    *boundary_inout = showTrimStartEndConfirmation();

  if (*boundary_inout == ZMAPBOUNDARY_5_SPLICE)
    {
      /* extend the start of the next intron i.e. trim the end of this exon */
      exon->x2 = x;
      merged = TRUE;
    }
  else if (*boundary_inout == ZMAPBOUNDARY_3_SPLICE)
    {
      /* extend the end of prev intron i.e. trim the start of this exon */
      exon->x1 = x;
      merged = TRUE;
    }

  return merged ;
}


/*!
 * \brief Merge a single coordinate into the given transcript.
 *
 * \return TRUE if merged, FALSE if cancelled or not possible
 */
gboolean zMapFeatureTranscriptMergeCoord(ZMapFeature transcript,
                                         const int x,
                                         ZMapBoundaryType *boundary_inout,
                                         GError **error)
{
  gboolean merged = FALSE;
  GError *tmp_error = NULL;

  /* Check if this is a valid transcript and has exons */
  if (!transcript)
    g_set_error(&tmp_error, g_quark_from_string("ZMap"), 99, "Transcript is null");

  if (transcript->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
    g_set_error(&tmp_error, g_quark_from_string("ZMap"), 99, "Feature is not a transcript");

  if (!transcript->feature.transcript.exons)
    g_set_error(&tmp_error, g_quark_from_string("ZMap"), 99, "Transcript has no exons");

  if (!tmp_error)
    {
      GArray *array = transcript->feature.transcript.exons;
      ZMapSpan exon = NULL;
      ZMapSpan prev_exon = NULL;
      int i = 0;

      for ( ; i < array->len; ++i, prev_exon = exon)
        {
          exon = &(g_array_index(array, ZMapSpanStruct, i));

          if (!prev_exon && i == array->len && x >= exon->x1 && x <= exon->x2)
            {
              merged = mergeCoordInsideSoleExon(transcript, boundary_inout, x, exon, &tmp_error) ;
              break;
            }
          else if (!prev_exon && x <= exon->x1)
            {
              merged = mergeCoordInsideFirstExon(transcript, boundary_inout, x, exon, &tmp_error) ;
              break;
            }
          else if (i == array->len - 1 && x >= exon->x2)
            {
              merged = mergeCoordOutsideLastExon(transcript, boundary_inout, x, exon, &tmp_error) ;
              break;
            }
          else if (x < exon->x1)
            {
              merged = mergeCoordInIntron(transcript, boundary_inout, x, prev_exon, exon, &tmp_error) ;

              if (merged)
                {
                  /* Check if we've created abutting exons and if so merge them */
                  if (prev_exon->x2 == exon->x1 - 1)
                    {
                      /* Prev exon was extended to abut to this one */
                      prev_exon->x2 = exon->x2 ;
                      g_array_remove_index(array, i) ;
                    }
                  else if (i < array->len - 1)
                    {
                      ZMapSpan next = &(g_array_index(array, ZMapSpanStruct, i+1)) ;
                      if (exon->x2 == next->x1 - 1)
                        {
                          exon->x2 = next->x2 ;
                          g_array_remove_index(array, i + 1) ;
                        }
                    }
                }

              break;
            }
          else if (x <= exon->x2)
            {
              merged = mergeCoordInExon(transcript, boundary_inout, x, exon, &tmp_error) ;
              break;
            }
        }
    }

  zMapFeatureTranscriptRecreateIntrons(transcript);

  if (tmp_error)
    g_propagate_error(error, tmp_error);

  return merged;
}


/*!
 * \brief Determine if two transcripts are equal i.e. have the same exons and cds
 *
 * \return TRUE if equal, FALSE if not
 */
gboolean zMapFeatureTranscriptsEqual(ZMapFeature feature1, ZMapFeature feature2, GError **error)
{
  gboolean result = FALSE ;
  GError *tmp_error = NULL ;

  if (!feature1 || !feature2)
    g_set_error(&tmp_error, g_quark_from_string("ZMap"), 99, "Feature is null");

  if (feature1->mode != ZMAPSTYLE_MODE_TRANSCRIPT || feature2->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
    g_set_error(&tmp_error, g_quark_from_string("ZMap"), 99, "Feature is not a transcript");

  if (!tmp_error &&
      feature1->feature.transcript.flags.cds == feature2->feature.transcript.flags.cds &&
      feature1->feature.transcript.cds_start == feature2->feature.transcript.cds_start &&
      feature1->feature.transcript.cds_end == feature2->feature.transcript.cds_end)
    {
      /* Loop through exons and check start/end */
      GArray *exons1 = feature1->feature.transcript.exons;
      GArray *exons2 = feature2->feature.transcript.exons;

      if (exons1->len == exons2->len)
        {
          /* now assume true but set result to false if we find an exon mismatch */
          result = TRUE ;
          int i = 0 ;

          for ( ; result && i < exons1->len; ++i)
            {
              ZMapSpan span1 = &(g_array_index(exons1, ZMapSpanStruct, i));
              ZMapSpan span2 = &(g_array_index(exons2, ZMapSpanStruct, i));

              if (span1->x1 != span2->x1 || span1->x2 != span2->x2)
                result = FALSE ;
            }
        }
    }

  return result ;
}



/*
 *               Internal functions.
 */


static void getDetailedExon(gpointer exon_data, gpointer user_data)
{
  ZMapSpan exon_span = (ZMapSpan)exon_data ;
  ItemShowTranslationTextData full_data = (ItemShowTranslationTextData)user_data ;
  ZMapFeature feature ;
  GList *variations = NULL ;
  CreateExonsData exons_data = g_new0(CreateExonsDataStruct,1) ;

  feature = full_data->feature ;

  if (feature && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    variations = feature->feature.transcript.variations ;

  if (!full_data->cds)
    {
      /* The pathological case is no CDS which we handle here to simplify the CDS code. */

      exons_data->ex_utr_5 = *exon_span ;    /* struct copy. */

      if (exons_data->ex_utr_5.x1)
        exons_data->full_exon_utr_5 = exonCreate(feature->x1, EXON_NON_CODING, &exons_data->ex_utr_5, variations,
                                                 &(full_data->feature_coord_counter),
                                                 &(full_data->spliced_coord_counter),
                                                 &(full_data->cds_coord_counter),
                                                 &(full_data->trans_coord_counter)) ;
    }
  else
    {
      int feature_pos, exon_start, exon_end ;

      feature_pos = exon_start = exon_span->x1 ;
      exon_end = exon_span->x2 ;

      calculateExonPositions(full_data, exon_span, variations, exons_data) ;


      /* Now we've calculated positions, create all the full exon structs. */
      createFullExonStructs(full_data, feature, variations, exons_data) ;
    }

  /* Set up the translation from CDS stuff. */
  createTranslationForCDS(full_data, exons_data) ;

  /* To help subsequent processing by the caller, exon regions are added in their
   * "logical" order i.e. in the order they occur on the sequence: if there's
   * a 5' utr put it before the full_exon, if there's a 3' utr put it after etc. */
  if (exons_data->full_exon_utr_5)
    *(full_data->full_exons) = g_list_append(*(full_data->full_exons), exons_data->full_exon_utr_5) ;

  /* These two must be mutually exclusive. */
  if (exons_data->full_exon_split_5)
    *(full_data->full_exons) = g_list_append(*(full_data->full_exons), exons_data->full_exon_split_5) ;
  else if (exons_data->full_exon_start_not_found)
    *(full_data->full_exons) = g_list_append(*(full_data->full_exons), exons_data->full_exon_start_not_found) ;

  if (exons_data->full_exon_cds_list)
    *(full_data->full_exons) = g_list_concat(*(full_data->full_exons), exons_data->full_exon_cds_list) ;

  if (exons_data->full_exon_split_3)
    *(full_data->full_exons) = g_list_append(*(full_data->full_exons), exons_data->full_exon_split_3) ;

  if (exons_data->full_exon_utr_3)
    *(full_data->full_exons) = g_list_append(*(full_data->full_exons), exons_data->full_exon_utr_3) ;

  g_free(exons_data) ;

  return ;
}


/* Calculate the positions of all the individual parts of an exon i.e. cds, utr, split codons etc. */
static void calculateExonPositions(ItemShowTranslationTextData full_data, ZMapSpan exon_span, 
                                   GList *variations, CreateExonsData exons_data)
{
  int exon_start = exon_span->x1 ;
  int exon_end = exon_span->x2 ;

  if (exon_end < full_data->cds_start)
    {
      /* 5' utr exon. */
      exons_data->ex_utr_5 = *exon_span ;    /* struct copy. */
    }
  else if (exon_start > full_data->cds_end)
    {
      /* 3' utr exon */
      exons_data->ex_utr_3 = *exon_span ;    /* struct copy. */
    }
  else if (exon_start == full_data->cds_start && exon_end < full_data->trans_start)
    {
      /* Truly pathological, first exon is less than 3 bases and although cds is
       * excluded from translation by start_not_found setting. */
      exons_data->ex_start_not_found = *exon_span ;    /* struct copy. */
      exons_data->ex_start_not_found.x2 = full_data->trans_start - 1 ;
    }
  else
    {
      /* mixed exon: may have utrs, split codons or just cds. */
      int ex_cds_start = exon_start, ex_cds_end = exon_end ;
      int exon_length = 0 ;
      int start_phase = 0, end_phase = 0 ;

      if (exon_start < full_data->cds_start)
        {
          /* 5' utr part. */
          exons_data->ex_utr_5 = *exon_span ;    /* struct copy. */
          exons_data->ex_utr_5.x2 = full_data->cds_start - 1 ;
          ex_cds_start = full_data->cds_start ;
        }

      if (exon_end > full_data->cds_end)
        {
          /* 3' utr part. */
          exons_data->ex_utr_3 = *exon_span ;    /* struct copy. */
          exons_data->ex_utr_3.x1 = full_data->cds_end + 1 ;
          ex_cds_end = full_data->cds_end ;
        }


      /* Pathological case: correction for start_not_found flag, trans_start is
       * already offset by start_not_found by caller routine.
       * We only do this if it's the first cds exon and the exon is the
       * start of the cds _and_ offset > 1 (otherwise we just treat as a normal cds exon). */
      if (exon_start == full_data->cds_start && full_data->start_offset > 1)
        {
          exons_data->ex_start_not_found = *exon_span ;    /* struct copy. */
          exons_data->ex_start_not_found.x2 = full_data->trans_start - 1 ;
          ex_cds_start = full_data->trans_start ;
        }


      /* ok, now any utr sections are removed we can work out phases of translation section. */
      exon_length = (ex_cds_end - ex_cds_start) + 1 ;
          
      /* variations may change the length of the peptide sequence in the exon */
      if (variations)
        {
          int variation_diff = zmapFeatureDNACalculateVariationDiff(ex_cds_start, ex_cds_end, variations) ;
          exon_length += variation_diff ;
        }

      if (ex_cds_start == full_data->cds_start && exon_length < 3)
        {
          /* pathological, start of whole cds is at the end of an exon and there are less than
           * 3 bases of cds. */

          exons_data->ex_split_5.x1 = ex_cds_start ;

          exons_data->ex_split_5.x2 = ex_cds_end ;
        }
      else
        {
          start_phase = (full_data->trans_coord_counter) % 3 ;
          end_phase = ((full_data->trans_coord_counter + exon_length) - 1) % 3 ;

          if (start_phase)
            {
              /* 5' split codon ends one base before full codon. */
              int correction ;

              correction = ((start_phase == 1 ? 2 : 1) - 1) ;

              exons_data->ex_split_5.x1 = ex_cds_start ;

              exons_data->ex_split_5.x2 = exons_data->ex_split_5.x1 + correction ;

              ex_cds_start = exons_data->ex_split_5.x2 + 1 ;
            }

          if (end_phase < 2)
            {
              /* 3' split codon starts one base after end of full codon. */
              int correction ;

              correction = ((end_phase + 1) % 3) - 1 ;

              exons_data->ex_split_3.x1 = ex_cds_end - correction ;

              exons_data->ex_split_3.x2 = ex_cds_end ;

              ex_cds_end = exons_data->ex_split_3.x1 - 1 ;
            }

          /* I'm not happy with this now...there are many, many combinations of pathological
           * cases to cope with and the code needs a rethink..... */

          /* cds part, "simple" now, just set to cds_start/end (only do this if there is
           * more, sometimes gene prediction programs produce very short exons so whole
           * exon is either split 5 or split 3 codon....) */
          if (ex_cds_start <= ex_cds_end
              && (ex_cds_start >= exon_span->x1 || (exons_data->ex_split_5.x2 && ex_cds_start > exons_data->ex_split_5.x2))
              && (ex_cds_end <=  exon_span->x2 || (exons_data->ex_split_3.x1 && ex_cds_end < exons_data->ex_split_3.x1)))
            {
              exons_data->ex_cds = *exon_span ;    /* struct copy. */

              if (ex_cds_start)
                exons_data->ex_cds.x1 = ex_cds_start ;

              if (ex_cds_end)
                exons_data->ex_cds.x2 = ex_cds_end ;
            }

        }
    }

}


/* Called after exon positions (cds, utr etc.) have all been calculated. This constructs the
 * structs for each individual exon section. */
static void createFullExonStructs(ItemShowTranslationTextData full_data, ZMapFeature feature, 
                                  GList *variations, CreateExonsData exons_data)
{
  if (exons_data->ex_utr_5.x1)
    exons_data->full_exon_utr_5 = exonCreate(feature->x1, EXON_NON_CODING, &exons_data->ex_utr_5, variations,
                                             &(full_data->feature_coord_counter),
                                             &(full_data->spliced_coord_counter),
                                             &(full_data->cds_coord_counter),
                                             &(full_data->trans_coord_counter)) ;

  if (exons_data->ex_split_5.x1)
    exons_data->full_exon_split_5 = exonCreate(feature->x1, EXON_SPLIT_CODON_5, &exons_data->ex_split_5, variations,
                                               &(full_data->feature_coord_counter),
                                               &(full_data->spliced_coord_counter),
                                               &(full_data->cds_coord_counter),
                                               &(full_data->trans_coord_counter)) ;
  else if (exons_data->ex_start_not_found.x1)
    exons_data->full_exon_start_not_found = exonCreate(feature->x1, EXON_START_NOT_FOUND, &exons_data->ex_start_not_found, variations,
                                                       &(full_data->feature_coord_counter),
                                                       &(full_data->spliced_coord_counter),
                                                       &(full_data->cds_coord_counter),
                                                       &(full_data->trans_coord_counter)) ;

  if (exons_data->ex_cds.x1)
    exons_data->full_exon_cds_list = exonCreateCDS(feature->x1, &exons_data->ex_cds, variations,
                                                   &(full_data->feature_coord_counter),
                                                   &(full_data->spliced_coord_counter),
                                                   &(full_data->cds_coord_counter),
                                                   &(full_data->trans_coord_counter)) ;
  if (exons_data->ex_split_3.x1)
    exons_data->full_exon_split_3 = exonCreate(feature->x1, EXON_SPLIT_CODON_3, &exons_data->ex_split_3, variations,
                                               &(full_data->feature_coord_counter),
                                               &(full_data->spliced_coord_counter),
                                               &(full_data->cds_coord_counter),
                                               &(full_data->trans_coord_counter)) ;

  if (exons_data->ex_utr_3.x1)
    exons_data->full_exon_utr_3 = exonCreate(feature->x1, EXON_NON_CODING, &exons_data->ex_utr_3, variations,
                                             &(full_data->feature_coord_counter),
                                             &(full_data->spliced_coord_counter),
                                             &(full_data->cds_coord_counter),
                                             &(full_data->trans_coord_counter)) ;
}


/* Create a exon structs for the section of the CDS split up by a variations */
static GList* exonCreateCDS(const int feature_start, ZMapSpan exon_span, GList *variations,
                            int *curr_feature_pos, int *curr_spliced_pos,
                            int *curr_cds_pos, int *curr_trans_pos)
{
  GList *result = NULL ;

  /* Make sure variations are sorted by position (they should be already but the list
   * shouldn't be long so no harm in making sure). */
  variations = g_list_sort(variations, zMapFeatureCmp) ;

  /* This keeps track of the start of the current section within the exon */
  int section_start_coord = exon_span->x1 ;

  /* Loop through each variation */
  GList *variation_item = variations ;

  for ( ; variation_item; variation_item = variation_item->next)
    {
      ZMapFeature variation = (ZMapFeature)(variation_item->data) ;

      /* If outside the max end of the exon, then none of the variations
       * can apply, so stop. */
      if (variation->x2 > exon_span->x2)
        break ;

      /* If not inside this exon then continue to the next variation */
      if (variation->x1 < exon_span->x1)
        continue ;

      /* Get the difference in length in DNA bases caused by this variation. If it's zero then
       * it doesn't affect the CDS coords so we can ignore it. */
      const int diff = zMapFeatureVariationGetSections(variation->feature.basic.variation_str,
                                                       NULL, NULL, NULL, NULL) ;

      if (diff == 0)
        continue ;

      /* Set the end of this section to be the start of the variation. */
      int section_end_coord = variation->x1 ;

      if (diff < 0)
        {
          /* For a deletion, exclude the variation x1 coord from the section, because that coord is part
           * of the deletion. We don't need to do this for an insertion because the insertion
           * point is between the x1 and x2 coord, so the coords themselves are not affected. */
          --section_end_coord ;
        }

      ZMapFullExon exon = exonCreateCDSSection(feature_start, variations, section_start_coord, section_end_coord,
                                               curr_feature_pos, curr_spliced_pos, curr_cds_pos, curr_trans_pos) ;
      
      if (exon)
        result = g_list_append(result, exon) ;

      /* Update the section start coord ready for the next section. Again, for deletions exclude
       * the variation coord but for insertions include it. */
      section_start_coord = variation->x2 ;
      
      if (diff < 0)
        ++section_start_coord ;
    }

  /* Create the final section, between the last variation and the end of the exon. (If there were
   * no variations then this will be the full span from the start to the end of the exon.) */
  ZMapFullExon exon = exonCreateCDSSection(feature_start, variations, section_start_coord, exon_span->x2,
                                           curr_feature_pos, curr_spliced_pos, curr_cds_pos, curr_trans_pos) ;

  if (exon)
    result = g_list_append(result, exon) ;

  return result ;
}


static ZMapFullExon exonCreateCDSSection(const int feature_start, GList *variations,
                                         const int section_start_coord, const int section_end_coord,
                                         int *curr_feature_pos, int *curr_spliced_pos,
                                         int *curr_cds_pos, int *curr_trans_pos)
{
  ZMapFullExon exon = NULL ;

  /* Ok, the variation affects this CDS. Create the new full exon struct for the section up to
   * the variation, and set all the positional data. */
  exon = g_new0(ZMapFullExonStruct, 1) ;
  exon->region_type = EXON_CODING ;

  /* Set the range for this section to be up to the start of this variation */
  exon->sequence_span.x1 = exon->unspliced_span.x1 = section_start_coord ;
  exon->sequence_span.x2 = exon->unspliced_span.x2 = section_end_coord ;

  /* Convert unspliced coords to local coords within the feature */
  zMapCoordsToOffset(feature_start, 1, &(exon->unspliced_span.x1), &(exon->unspliced_span.x2)) ;

  /* Offset the start coord by the total difference caused by variations before this
   * exon. Note that we do this for all local coords within the feature, but not for the
   * sequence_span since that is in reference sequence coords and they are absolute. */
  const int variation_diff = zmapFeatureDNACalculateVariationDiff(1, feature_start - 1, variations);
  exon->unspliced_span.x1 += variation_diff ;

  const int section_length = exon->sequence_span.x2 - exon->sequence_span.x1 + 1 ;

  exon->spliced_span.x1 = *curr_spliced_pos + 1 ;
  exon->spliced_span.x2 = *curr_spliced_pos + section_length ;

  exon->cds_span.x1 = *curr_cds_pos + 1 ;
  exon->cds_span.x2 = *curr_cds_pos + section_length ;

  /* start not found section is not part of translation so is not given a phase. */
  exon->start_phase = *curr_trans_pos % 3 ;
  exon->end_phase = ((*curr_trans_pos + section_length) - 1) % 3 ;

  /* Now update the position counters. */
  *curr_feature_pos += section_length ;
  *curr_spliced_pos += section_length ;
  *curr_cds_pos += section_length ;
  *curr_trans_pos += section_length ;

  return exon ;
}

/* Create a full exon with all the positional information and update the running positional
 * counters required for cds phase calcs etc. */
static ZMapFullExon exonCreate(int feature_start, ExonRegionType region_type, ZMapSpan exon_span, GList *variations,
                               int *curr_feature_pos, int *curr_spliced_pos,
                               int *curr_cds_pos, int *curr_trans_pos)
{
  ZMapFullExon exon = NULL ;
  const int exon_span_length = ZMAP_SPAN_LENGTH(exon_span) ;

  const int variation_diff1 = zmapFeatureDNACalculateVariationDiff(1, feature_start - 1, variations);
  const int variation_diff2 = zmapFeatureDNACalculateVariationDiff(feature_start, feature_start + exon_span_length, variations);
  const int exon_length = exon_span_length + variation_diff2 ;

  /* Create the new full exon struct and set all the positional data. */
  exon = g_new0(ZMapFullExonStruct, 1) ;
  exon->region_type = region_type ;
  exon->sequence_span = exon->unspliced_span = *exon_span ; /* struct copy */
  zMapCoordsToOffset(feature_start, 1, &(exon->unspliced_span.x1), &(exon->unspliced_span.x2)) ;

  exon->unspliced_span.x1 += variation_diff1 ;
  exon->unspliced_span.x2 += variation_diff2 ;

  exon->spliced_span.x1 = *curr_spliced_pos + 1 ;
  exon->spliced_span.x2 = *curr_spliced_pos + exon_length ;

  if (region_type != EXON_NON_CODING)
    {
      exon->cds_span.x1 = *curr_cds_pos + 1 ;
      exon->cds_span.x2 = *curr_cds_pos + exon_length ;

      /* start not found section is not part of translation so is not given a phase. */
      if (region_type != EXON_START_NOT_FOUND)
        {
          exon->start_phase = *curr_trans_pos % 3 ;
          exon->end_phase = ((*curr_trans_pos + exon_length) - 1) % 3 ;
        }
    }

  /* Now update the position counters. */
  *curr_feature_pos += exon_length ;
  *curr_spliced_pos += exon_length ;
  if (region_type != EXON_NON_CODING)
    {
      if (region_type != EXON_START_NOT_FOUND)
        {
        /* mh17:
         * else we get 1 out of step with peptides: CDS is used for getting AA's
         * not trans pos, which is used to set phase
         * (RT 266769)
         */

        *curr_cds_pos += exon_length ;
        }

      if (region_type != EXON_START_NOT_FOUND)
        *curr_trans_pos += exon_length ;
    }

  return exon ;
}


/* Once the sections of an exon have been constructed (utr, cds etc) this is called to set up the
 * translation (if a cds exists) */
static void createTranslationForCDS(ItemShowTranslationTextData full_data, CreateExonsData exons_data)
{
  GList *cds_item = exons_data->full_exon_cds_list ;

  for ( ; cds_item; cds_item = cds_item->next)
    {
      ZMapFullExon full_exon_cds = (ZMapFullExon)(cds_item->data) ;

      int pep_start = 0, pep_end = 0, pep_length = 0 ;
      char *peptide = NULL ;

      pep_start = (full_exon_cds->cds_span.x1 / 3) + 1 ;
      pep_end = full_exon_cds->cds_span.x2 / 3 ;

      full_exon_cds->pep_span.x1 = pep_start ;
      full_exon_cds->pep_span.x2 = pep_end ;
      pep_length = pep_end - pep_start + 1;

      if (full_data->translation)
        {
          peptide = full_data->translation + (pep_start - 1) ;
          full_exon_cds->peptide = g_strndup(peptide, pep_length) ;
        }

      if (exons_data->full_exon_split_5)
        {
          pep_start-- ;

          exons_data->full_exon_split_5->pep_span.x1 = exons_data->full_exon_split_5->pep_span.x2 = pep_start ;

          if (full_data->translation)
            {
              peptide = full_data->translation + (pep_start - 1) ;

              exons_data->full_exon_split_5->peptide = g_strndup(peptide, 1) ;
            }
        }

      if (exons_data->full_exon_split_3)
        {
          pep_end++ ;

          exons_data->full_exon_split_3->pep_span.x1 = exons_data->full_exon_split_3->pep_span.x2 = pep_end ;

          if (full_data->translation)
            {
              peptide = full_data->translation + (pep_end - 1) ;

              exons_data->full_exon_split_3->peptide = g_strndup(peptide, 1) ;
            }
        }
    }
}


static void exonDestroy(ZMapFullExon exon)
{
  if (!exon)
    return ;

  if (exon->peptide)
    g_free(exon->peptide) ;

  g_free(exon) ;

  return ;
}



/* A GFunc() callback to free each exon element of the exons list. */
static void exonListFree(gpointer data, gpointer user_data_unused)
{
  ZMapFullExon exon = (ZMapFullExon)data ;

  exonDestroy(exon) ;

  return ;
}



static void printDetailedExons(gpointer exon_data, gpointer user_data)
{
  ZMapFullExon exon = (ZMapFullExon)exon_data;

  printf("Exon: Span %d -> %d (%d)\n",
         exon->sequence_span.x1,
         exon->sequence_span.x2,
         (exon->sequence_span.x2 - exon->sequence_span.x1 + 1)) ;

  printf("Exon: Span %d -> %d (%d)\n",
         exon->unspliced_span.x1,
         exon->unspliced_span.x2,
         (exon->unspliced_span.x2 - exon->unspliced_span.x1 + 1)) ;

  printf("Exon: Span %d -> %d (%d)\n",
         exon->spliced_span.x1,
         exon->spliced_span.x2,
         (exon->spliced_span.x2 - exon->spliced_span.x1 + 1)) ;

  printf("  CDS Span %d -> %d (%d)\n",
         exon->cds_span.x1,
         exon->cds_span.x2,
         (exon->cds_span.x2 - exon->cds_span.x1 + 1)) ;

  printf("  Pep Span %d -> %d (%d)\n",
         exon->pep_span.x1,
         exon->pep_span.x2,
         (exon->pep_span.x2 - exon->pep_span.x1 + 1)) ;

  printf("  Phase = %d, End Phase = %d\n", exon->start_phase, exon->end_phase) ;

  if (exon->peptide)
    {
      char *pep_start, *pep_end;
      int print_length;
      int peptide_length;

      peptide_length = strlen(exon->peptide);

      print_length = (((peptide_length / 2) > 10) ? 10 : (peptide_length / 2));

      pep_start = g_strndup(exon->peptide, print_length);
      pep_end = g_strndup(exon->peptide + peptide_length - print_length, print_length);

      printf("  Translation (%d):\n", peptide_length);
      printf("  %s...%s\n", pep_start, pep_end);

      if(pep_start)
        g_free(pep_start) ;

      if(pep_end)
        g_free(pep_end) ;
    }


  return ;
}


/* Blindly extend transcripts start/end to encompass the given span. */
static void extendTranscript(ZMapFeature transcript, ZMapSpanStruct * span)
{
  if (transcript->x1 == 0 || span->x1 < transcript->x1)
    transcript->x1 = span->x1 ;
  if (transcript->x2 == 0 || span->x2 > transcript->x2)
    transcript->x2 = span->x2 ;

  return ;
}

