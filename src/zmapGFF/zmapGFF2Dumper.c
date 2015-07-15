/*  File: zmapGFF2Dumper.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk),
 *          Steve Miller (sm23@sanger.ac.uk)
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Dumps the features within a given block as GFF v3
 *
 * Exported functions: See ZMap/zmapGFF.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapSO.h>
#include <ZMap/zmapGFF.h>
#include "zmapGFFAttribute.h"


/*
 * GFF version to output. Can be set to either
 * [ZMAPGFF_VERSION_2, ZMAPGFF_VERSION_3] using the
 * external interface function 'zMapGFFDumpVersion()'.
 */
static ZMapGFFVersion gff_output_version_G = ZMAPGFF_VERSION_3 ;

/*
 * This is used to pick up bits of data from different points
 * within the context/align/block/featureset/feature heirarchy.
 * It would be better if we could do without it of course. Note
 * this object does not control the lifetime of the GString *buffer.
 */
typedef struct ZMapGFFFormatDataStruct_
{
  const char *sequence;                 /* The sequence name. e.g. 16.12345-23456 */
  int start, end ;
  GString *buffer ;
  ZMapGFFAttributeFlags attribute_flags ;

  struct
    {
      unsigned int status      : 1 ;
      unsigned int cont        : 1 ;
      unsigned int over_write  : 1 ;
    } flags ;

} ZMapGFFFormatDataStruct ;
static ZMapGFFFormatData createGFFFormatData() ;
static void deleteGFFFormatData(ZMapGFFFormatData *) ;

/*
 * A hack for sources that are input as '.',
 * and also a list of characters that are to be
 * unescaped upon output (in names and ids).
 */
static const char *anon_source = "anon_source",
    *anon_source_ab = ".",
    *reserved_allowed = " ()+-:;@/" ;

/*
 * Some callbacks used for searching.
 */
static gboolean dump_full_header(ZMapFeatureAny feature_any, GIOChannel *file,
  ZMapGFFFormatData format_data, GError **error_out) ;
static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark key, gpointer data,
  gpointer user_data, char **err_out) ;
static ZMapFeatureContextExecuteStatus get_featuresets_from_ids_cb(GQuark key, gpointer data,
  gpointer user_data, char **err_out) ;
static void add_feature_to_list_cb(gpointer key, gpointer value, gpointer user_data) ;


/*
 * Data structures associated with searching; see comments with the related
 * functions.
 */
typedef struct FeaturesetSearchStruct_
  {
    GList *results ;
    GList *targets ;
  } FeaturesetSearchStruct, *FeaturesetSearch ;
static FeaturesetSearch createFeaturesetSearch() ;
static void deleteFeaturesetSearch(FeaturesetSearch *) ;

typedef struct FeatureSearchStruct_
  {
    GList *results ;
    ZMapSpan region_span ;
  } FeatureSearchStruct, *FeatureSearch ;
static FeatureSearch createFeatureSearch() ;
static void deleteFeatureSearch(FeatureSearch *) ;


/*
 * ZMapFeatureDumpFeatureFunc to dump gff. Writes lines into gstring buffer.
 *
 */
static gboolean dump_gff_cb(ZMapFeatureAny feature_any, GHashTable *styles,
  GString *gff_string,  GError **error, gpointer user_data);


/*
 * Public interface function to set the version of GFF to output.
 * Well, only v3 from now on!
 */
gboolean zMapGFFDumpVersionSet(ZMapGFFVersion gff_version )
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((gff_version == ZMAPGFF_VERSION_3), result) ;
  gff_output_version_G = gff_version ;

  return result ;
}

/*
 * Return the version of GFF that has been set for output.
 */
ZMapGFFVersion zMapGFFDumpVersionGet()
{
  return gff_output_version_G ;
}


/* NOTE: if we want this to dump a context it will have to cope with lots of different
 * sequences for the different alignments...I think the from alignment down is ok but the title
 * for the file is not if we have different alignments....and neither are the coords....?????
 *
 * I think we would need multiple Type and sequence-region comment lines for the different
 * sequences.
 *
 */
gboolean zMapGFFDump(ZMapFeatureAny dump_set, GHashTable *styles, GIOChannel *file, GError **error_out)
{
  gboolean result = TRUE;

  result = zMapGFFDumpRegion(dump_set, styles, NULL, file, error_out);

  return result;
}

gboolean zMapGFFDumpRegion(ZMapFeatureAny dump_set, GHashTable *styles,
                           ZMapSpan region_span, GIOChannel *file, GError **error_out)
{
  gboolean result = FALSE ;
  ZMapGFFFormatData format_data = NULL ;

  zMapReturnValIfFail(   file && dump_set
                      && (dump_set->struct_type != ZMAPFEATURE_STRUCT_INVALID)
                      && error_out,
                                  result) ;

  if ((format_data = createGFFFormatData()))
    result = TRUE ;

  if (result)
    {
      format_data->sequence = NULL ;
      format_data->flags.cont       = TRUE;
      format_data->flags.status     = TRUE;
      format_data->flags.over_write = TRUE ;
      result = dump_full_header(dump_set, file, format_data, error_out) ;
    }

  if (result)
    {

      /* This might get overwritten later, but as DumpToFile uses
       * Subset, there's a chance it wouldn't get set at all */
      if(region_span)
        result = zMapFeatureContextRangeDumpToFile((ZMapFeatureAny)dump_set, styles, region_span,
                   dump_gff_cb, format_data, file, error_out) ;
      else
        result = zMapFeatureContextDumpToFile((ZMapFeatureAny)dump_set, styles,
                   dump_gff_cb, format_data, file, error_out) ;
    }

  if (format_data)
    deleteGFFFormatData(&format_data) ;

  return result ;
}



/*
 * Dumps a list of featuresets in a given region.
 *
 * The featuresets to dump are passed in the GList* argument which must contain
 * values of the featureset->unique_id. If the region_span
 * is NULL then use all features available, otherwise only output ones that overlap.
 */
gboolean zMapGFFDumpFeatureSets(ZMapFeatureAny feature_any, GHashTable *styles,
  GList* featuresets, ZMapSpan region_span, GIOChannel *file, GError **error_out)
{
  gboolean result = FALSE ;
  char *sequence = NULL ;
  GList *list_pos = NULL ;
  FeaturesetSearch fs_data = NULL ;
  FeatureSearch f_data = NULL ;
  ZMapFeatureSet featureset = NULL ;

  zMapReturnValIfFail(    feature_any
                       && (feature_any->struct_type == ZMAPFEATURE_STRUCT_CONTEXT)
                       && styles
                       && featuresets
                       && g_list_length(featuresets)
                       && file,
                          result ) ;

  /*
   * First search for the featuresets in the context that we wish to output.
   * This section will set result = TRUE if one or more featuresets were found.
   */
  fs_data = createFeaturesetSearch() ;
  if (fs_data)
    {
      fs_data->targets = featuresets ;
      zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_FEATURESET,
                                get_featuresets_from_ids_cb, fs_data) ;
      if (fs_data->results && g_list_length(fs_data->results))
        {
          result = TRUE ;
        }
    }

  if (!result)
    {
      *error_out = g_error_new(g_quark_from_string("ERROR in zMapGFFDumpFeatureSets(); "),
                                   (gint)0, " no featuresets found.") ;
    }

  /*
   * Now add all of the features from our hard-won search results to
   * the features_list GList. This section will set result = FALSE if
   * no features were found.
   */
  if (result)
    {
      f_data = createFeatureSearch() ;
      if (f_data)
        {
          f_data->region_span = region_span ;
          for (list_pos = fs_data->results; list_pos != NULL; list_pos = list_pos->next)
            {
              featureset = (ZMapFeatureSet) list_pos->data ;
              g_hash_table_foreach(featureset->features, add_feature_to_list_cb, f_data);
            }

          if (!f_data->results || !g_list_length(f_data->results))
            result = FALSE ;
        }
    }

  if (!result)
    {
      *error_out = g_error_new(g_quark_from_string("ERROR in zMapGFFDumpFeatureSets(); "),
                                   (gint)0, " no features found.") ;
    }

  /*
   * Now dump the features to file.
   */
  if (result)
    {
      result = zMapGFFDumpList(f_data->results, styles, sequence, file, NULL, error_out) ;
      if (!result)
        {
          if (*error_out)
            {
              g_prefix_error(error_out, "Error in zMapGFFDumpFeatureSets() calling zMapGFFDumpList(); ") ;
            }
        }
    }

  /*
   * Clear up on finish.
   */
  deleteFeaturesetSearch(&fs_data) ;
  deleteFeatureSearch(&f_data) ;

  return result ;
}


/*
 * Dump a list of features to file or string buffer.
 */
gboolean zMapGFFDumpList(GList *dump_list,
                         GHashTable *styles,
                         char *sequence,
                         GIOChannel *file,
                         GString *text_out,
                         GError **error_out)
{
  gboolean result = FALSE ;
  ZMapFeatureAny feature_any = NULL ;
  ZMapGFFFormatData format_data = NULL ;

  zMapReturnValIfFail(dump_list && dump_list->data && error_out, result) ;
  feature_any  = (ZMapFeatureAny)(dump_list->data);
  zMapReturnValIfFail(feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID, result ) ;

  if ((format_data = createGFFFormatData()))
    result = TRUE ;

  if (result)
    {
      format_data->sequence = sequence;
      format_data->flags.cont       = TRUE;
      format_data->flags.status     = TRUE;
      format_data->flags.over_write = FALSE ;
      if (!file && text_out)
        format_data->buffer = text_out ;
      result = dump_full_header(feature_any, file, format_data, error_out) ;
    }

  if (result)
    {
      result = zMapFeatureListDumpToFileOrBuffer(dump_list, styles, dump_gff_cb,
                                                 format_data, file, text_out, error_out) ;
    }

  if (format_data)
    deleteGFFFormatData(&format_data) ;

  return result ;
}


/* INTERNALS */

static gboolean dump_full_header(ZMapFeatureAny feature_any,
                                 GIOChannel *file,
                                 ZMapGFFFormatData format_data,
                                 GError **error_out)
{
  zMapReturnValIfFail(   feature_any
                      && (feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID)
                      && error_out, FALSE ) ;
  GString *line  = NULL ;
  gboolean write_to_gio = FALSE ;

  if (!file && format_data->buffer)
    {
      line = format_data->buffer ;
    }
  else
    {
      write_to_gio = TRUE ;
      line = g_string_new(NULL) ;
    }

  /*
   * This descends the data structures to find the ##sequence-region
   * string, start and end coordinates.
   */
  zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_BLOCK,
                            get_type_seq_header_cb, format_data) ;


  /*
   * Formats header and appends to the buffer string object.
   */
  zMapGFFFormatHeader(FALSE, line, format_data->sequence,
                      format_data->start, format_data->end ) ;

  /*
   * Now write this to the IO channel.
   */
  if(format_data->flags.status && write_to_gio)
    {
      char *error_msg = NULL ;
      format_data->flags.status = zMapGFFOutputWriteLineToGIO(file, &error_msg, line, TRUE) ;
      if (error_msg)
        {
          *error_out = g_error_new(g_quark_from_string("ERROR in dump_full_header()"),
                                   (gint)0, "message was '%s'", error_msg ) ;
          g_free(error_msg) ;
        }
    }

  if (line && write_to_gio)
    g_string_free(line, TRUE) ;

  return format_data->flags.status ;
}

/*
 * This context execute callback descends the data structures. It uses the
 * featureset ids to find pointers to the featuresets themselves. This is
 * required if you want the features themselves (e.g. to list or output).
 *
 * Data required are in the FeaturesetSearchStruct.
 *
 * FeaturesetSearchStruct.targets     GList* of featureset->unique_id values
 * FeaturesetSearchStruct.results     GList* of featureset pointer (if found)
 */
static ZMapFeatureContextExecuteStatus
  get_featuresets_from_ids_cb(GQuark key, gpointer data,
                              gpointer user_data, char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny) data ;
  FeaturesetSearch search_data = (FeaturesetSearch) user_data ;

  if (!feature_any || !search_data || !search_data->targets)
    return status ;

  switch (feature_any->struct_type)
    {
      case ZMAPFEATURE_STRUCT_CONTEXT:
      case ZMAPFEATURE_STRUCT_ALIGN:
      case ZMAPFEATURE_STRUCT_BLOCK:
        status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
        break;
      case ZMAPFEATURE_STRUCT_FEATURESET:
        {
          ZMapFeatureSet featureset = (ZMapFeatureSet) feature_any ;
          GList *l = NULL ;
          gpointer id = GINT_TO_POINTER(featureset->unique_id) ;
          for (l = search_data->targets; l != NULL; l = l->next)
            {
              if (id == l->data)
                {
                  /* const char *name = g_quark_to_string((GQuark)id) ; */
                  search_data->results =
                    g_list_prepend(search_data->results, (gpointer)featureset);
                  break ;
                }
            }
          status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
        }
        break ;
      case ZMAPFEATURE_STRUCT_FEATURE:
      default:
        status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND ;
        break;
    } ;

  return status ;
}



/*
 * This is a callback used in the call of g_hash_foreach() on a featureset->features
 * hash table. We take search data from an instance of FeatureSearchStruct; first
 * check that we have a valid feature, check that the feature overlaps the ZMapSpan
 * and then add the feature to the associated GList of results.
 *
 * FeatureSearchStruct.region_span      ZMapSpan; if valid check features against this
 * FeatureSearchStruct.results          GList* of feature pointer (if any found)
 */
static void add_feature_to_list_cb(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureAny feature_any = NULL ;
  ZMapFeature feature = NULL ;
  ZMapSpan region_span = NULL ;
  FeatureSearch f_data = NULL ;
  gboolean include_feature = TRUE ;
  if (!value || !user_data)
    return ;

  feature_any = (ZMapFeatureAny) value ;
  f_data = (FeatureSearch) user_data ;
  region_span = f_data->region_span ;

  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      feature = (ZMapFeature) feature_any ;
      if (region_span && region_span->x1 && region_span->x2)
        {
          if ((feature->x2 < region_span->x1) || (feature->x1 > region_span->x2))
            include_feature = FALSE ;
        }
      if (include_feature)
        {
          f_data->results = g_list_prepend(f_data->results, (gpointer)feature_any) ;
        }
    }
}



static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark  key, gpointer data,
                                                              gpointer user_data, char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapGFFFormatData format_data  = (ZMapGFFFormatData)user_data;

  zMapReturnValIfFail(data && user_data && (feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID), status ) ;

  switch(feature_any->struct_type)
    {
      case ZMAPFEATURE_STRUCT_CONTEXT:
        break;
      case ZMAPFEATURE_STRUCT_ALIGN:
        if(format_data->flags.status && format_data->flags.cont)
          {
            status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
          }
        break;
      case ZMAPFEATURE_STRUCT_BLOCK:
        if(format_data->flags.status && format_data->flags.cont)
          {
            ZMapFeatureBlock feature_block = (ZMapFeatureBlock)feature_any;

            format_data->sequence = g_quark_to_string(feature_any->original_id) ;
            format_data->start = feature_block->block_to_sequence.block.x1 ;
            format_data->end = feature_block->block_to_sequence.block.x2 ;

            format_data->flags.status = TRUE;
            format_data->flags.cont   = FALSE;
            status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
          }
        break;
      case ZMAPFEATURE_STRUCT_FEATURESET:
      case ZMAPFEATURE_STRUCT_FEATURE:
      default:
        status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND ;
        break;
    }

  return status;
}

/*
 * This function must write results into the GString object that's
 * passed in as an argument here. The contents of that string are written
 * to the IO channel in the caller (zmapFeatureOutput::dump_features_cb(),
 * in this case).
 */
static gboolean dump_gff_cb(ZMapFeatureAny feature_any,
    GHashTable         *styles,
    GString       *line,
    GError       **error,
    gpointer       user_data)
{
  ZMapGFFFormatData format_data = (ZMapGFFFormatData)user_data;
  gboolean result = TRUE;
  ZMapFeatureSet featureset = NULL ;
  ZMapFeature feature = NULL ;

  zMapReturnValIfFail(   feature_any
                      && format_data, result ) ;

  switch(feature_any->struct_type)
    {
      case ZMAPFEATURE_STRUCT_ALIGN:
      case ZMAPFEATURE_STRUCT_CONTEXT:
      case ZMAPFEATURE_STRUCT_BLOCK:
      case ZMAPFEATURE_STRUCT_FEATURESET:
        result = TRUE;
        break;
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          /*
           * Find the featureset and feature
           */
          featureset = (ZMapFeatureSet)(feature_any->parent);
          feature = (ZMapFeature)feature_any;

          /*
           * some sources to be skipped
           */
          GQuark gqFeatureset = featureset->original_id ;
          if(   (gqFeatureset == g_quark_from_string("Locus"))
             || (gqFeatureset == g_quark_from_string("Show Translation"))
             || (gqFeatureset == g_quark_from_string("3 Frame Translation"))
             || (gqFeatureset == g_quark_from_string("Annotation"))
             || (gqFeatureset == g_quark_from_string("ORF"))
             || (gqFeatureset == g_quark_from_string("DNA"))   )
            break;

          /*
           * Now switch upon ZMapStyleMode and do attributes
           */
          switch(feature->mode)
            {
              case ZMAPSTYLE_MODE_BASIC:
                {
                  result = zMapGFFFormatAttributeSetBasic(format_data->attribute_flags)
                        && zMapGFFWriteFeatureBasic(feature, format_data->attribute_flags,
                                                    line, format_data->flags.over_write) ;
                }
                break;
              case ZMAPSTYLE_MODE_TRANSCRIPT:
                {
                  result = zMapGFFFormatAttributeSetTranscript(format_data->attribute_flags)
                        && zMapGFFWriteFeatureTranscript(feature, format_data->attribute_flags,
                                                         line, format_data->flags.over_write) ;
                }
                break;
              case ZMAPSTYLE_MODE_ALIGNMENT:
                {
                  result = zMapGFFFormatAttributeSetAlignment(format_data->attribute_flags)
                        && zMapGFFWriteFeatureAlignment(feature, format_data->attribute_flags,
                                                        line, format_data->flags.over_write, NULL) ;
                }
                break;
              case ZMAPSTYLE_MODE_TEXT:
                {
                  result = zMapGFFFormatAttributeSetText(format_data->attribute_flags)
                        && zMapGFFWriteFeatureText(feature, format_data->attribute_flags,
                                                   line, format_data->flags.over_write ) ;
                }
                break;
              case ZMAPSTYLE_MODE_GRAPH:
                {
                  result = zMapGFFFormatAttributeSetGraph(format_data->attribute_flags)
                        && zMapGFFWriteFeatureGraph(feature, format_data->attribute_flags,
                                                    line, format_data->flags.over_write ) ;
                }
                break;
              default:
                break;
            }
          }
        break;
      default:
        result = FALSE;
        break;
    }

  return result;
}

/*
 * These functions select the attributes to output for features
 * of the specified ZMapStyleMode.

 */
gboolean zMapGFFFormatAttributeUnsetAll(ZMapGFFAttributeFlags attribute_flags)
{
  gboolean result = FALSE ;

  if (!attribute_flags)
    return result ;
  result = TRUE ;

  attribute_flags->name       = FALSE ;
  attribute_flags->id         = FALSE ;
  attribute_flags->parent     = FALSE ;
  attribute_flags->note       = FALSE ;
  attribute_flags->locus      = FALSE ;
  attribute_flags->percent_id = FALSE ;
  attribute_flags->url        = FALSE ;
  attribute_flags->gap        = FALSE ;
  attribute_flags->target     = FALSE ;
  attribute_flags->variation  = FALSE ;
  attribute_flags->sequence   = FALSE ;
  attribute_flags->evidence   = FALSE ;

  return result ;
}

gboolean zMapGFFFormatAttributeSetBasic(ZMapGFFAttributeFlags attribute_flags)
{
  gboolean result = FALSE ;

  if (!attribute_flags)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(attribute_flags) ;

  if (result)
    {
      attribute_flags->name        = TRUE ;
      attribute_flags->url         = TRUE ;
      attribute_flags->variation   = TRUE ;
      attribute_flags->note        = TRUE ;
    }

  return result ;
}

gboolean zMapGFFFormatAttributeSetGraph(ZMapGFFAttributeFlags attribute_flags)
{
  gboolean result = FALSE ;

  if (!attribute_flags)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(attribute_flags) ;

  if (result)
    {
      attribute_flags->name        = TRUE ;
      attribute_flags->url         = TRUE ;
      attribute_flags->note        = TRUE ;
    }

  return result ;
}

gboolean zMapGFFFormatAttributeSetTranscript(ZMapGFFAttributeFlags attribute_flags)
{
  gboolean result = FALSE ;

  if (!attribute_flags)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(attribute_flags) ;

  if (result)
    {
      attribute_flags->name        = TRUE ;
      attribute_flags->id          = TRUE ;
      attribute_flags->note        = TRUE ;
      attribute_flags->locus       = TRUE ;
      attribute_flags->url         = TRUE ;
      attribute_flags->parent      = TRUE ;
      attribute_flags->evidence    = TRUE ;
    }

  return result ;
}

gboolean zMapGFFFormatAttributeSetAlignment(ZMapGFFAttributeFlags attribute_flags)
{
  gboolean result = FALSE ;

  if (!attribute_flags)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(attribute_flags) ;

  if (result)
    {
      attribute_flags->name        = TRUE ;
      attribute_flags->gap         = TRUE ;
      attribute_flags->target      = TRUE ;
      attribute_flags->percent_id  = TRUE ;
      attribute_flags->sequence    = TRUE ;
    }

  return result ;
}

gboolean zMapGFFFormatAttributeSetText(ZMapGFFAttributeFlags attribute_flags)
{
  gboolean result = FALSE ;

  if (!attribute_flags)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(attribute_flags) ;

  if (result)
    {
      attribute_flags->name        = TRUE ;
      attribute_flags->note        = TRUE ;
      attribute_flags->url         = TRUE ;
    }

  return result ;
}


char zMapGFFFormatStrand2Char(ZMapStrand strand)
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


char zMapGFFFormatPhase2Char(ZMapPhase phase)
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

/* Length of a block. */
static int calcBlockLength(ZMapSequenceType match_seq_type, int start, int end)
{
  int coord ;

  coord = (end - start) + 1 ;

  if (match_seq_type == ZMAPSEQUENCE_PEPTIDE)
    coord /= 3 ;

  return coord ;
}

/* Length between two blocks. */
static int calcGapLength(ZMapSequenceType match_seq_type, int start, int end)
{
  int coord ;

  coord = (end - start) - 1 ;

  if (match_seq_type == ZMAPSEQUENCE_PEPTIDE)
    coord /= 3 ;

  return coord ;
}

/*
 * Return the gaps array as a (GFFv3) Gap attribute. This is the code that
 * was originally in the file zmapViewCallBlixem. Note that there is also code
 * in zmapFeatureAlignment that puportedly does the same thing (with a greater
 * variety of formats) but I have not had time to check that they give identical
 * results.
 */
gboolean zMapGFFFormatGap2GFF(GString *attribute, GArray *gaps, ZMapStrand q_strand, ZMapSequenceType match_seq_type)
{
  gboolean result = FALSE ;
  int k=0, index=0, incr=0, coord=0,
    curr_match=0, next_match=0, curr_ref=0, next_ref=0 ;
  ZMapAlignBlock gap ;

  if (!attribute || !gaps)
    return result ;
  result = TRUE ;

  g_string_truncate(attribute, (gsize)0) ;
  g_string_printf(attribute, "Gap=") ;

      /* correct, needed ?? */
      if (q_strand == ZMAPSTRAND_REVERSE)
        {
          index = gaps->len - 1 ;
          incr = -1 ;
        }
      else
        {
          index = 0 ;
          incr = 1 ;
        }

      for (k = 0 ; k < (int)gaps->len ; k++, index += incr)
        {

          gap = &(g_array_index(gaps, ZMapAlignBlockStruct, index)) ;

          coord = calcBlockLength(match_seq_type, gap->t1, gap->t2) ;
          g_string_append_printf(attribute, "M%d ", coord) ;

          if (k < (int)gaps->len - 1)
            {
              ZMapAlignBlock next_gap ;

              next_gap = &(g_array_index(gaps, ZMapAlignBlockStruct, index + incr)) ;

              if (gap->q_strand == ZMAPSTRAND_FORWARD)
                {
                  curr_match = gap->q2 ;
                  next_match = next_gap->q1 ;
                }
              else
                {
                  /* For the match, q1 > q2 for rev strand */
                  curr_match = next_gap->q1 ;
                  next_match = gap->q2 ;
                }

              if (gap->t_strand == ZMAPSTRAND_FORWARD)
                {
                  curr_ref = gap->t2 ;
                  next_ref = next_gap->t1 ;
                }
              else
                {
                  /* For the ref, t2 > t1 for both strands */
                  curr_ref = next_gap->t2 ;
                  next_ref = gap->t1 ;
                }

              if (next_match > curr_match + 1)
                {
                  coord = calcGapLength(match_seq_type, curr_match, next_match) ;
                  g_string_append_printf(attribute, "I%d ", coord) ;
                }
              else if (next_ref > curr_ref + 1)
                {
                  coord = calcGapLength(match_seq_type, curr_ref, next_ref) ;

                  /* Check if it's an intron or deletion */
                  AlignBlockBoundaryType boundary_type = ALIGN_BLOCK_BOUNDARY_EDGE ;
                  if (gap->q_strand == gap->t_strand)
                    boundary_type = gap->end_boundary ;
                  else
                    boundary_type = gap->start_boundary ;

                  if (boundary_type == ALIGN_BLOCK_BOUNDARY_INTRON)
                    g_string_append_printf(attribute, "N%d ", coord) ;
                  else
                    g_string_append_printf(attribute, "D%d ", coord) ;
                }
              else
                zMapLogWarning("Bad coords in zMapGFFFormatGap2GFF(): gap %d,%d -> %d, %d, next_gap %d, %d -> %d, %d",
                               gap->t1, gap->t2, gap->q1, gap->q2,
                               next_gap->t1, next_gap->t2, next_gap->q1, next_gap->q2) ;
            }
        }

  return result ;
}

/*
 * This is append an attribute to a line optionally prepending the attribute with
 * a tab character (i.e. for the first attribute). Optionally adds a semi-colon at
 * the end.
 */
gboolean zMapGFFFormatAppendAttribute(GString *gff_line, GString *attribute, gboolean tab_prefix, gboolean sc_terminator)
{
  static const char c_delimiter = ';',
    c_tab = '\t' ;
  gboolean result = FALSE ;

  if (!gff_line || !attribute)
    return result ;
  result = TRUE ;

  if (tab_prefix)
    {
      g_string_append_printf(gff_line, "%c", c_tab) ;
    }

  if (sc_terminator)
    g_string_append_printf(gff_line, "%s%c", attribute->str, c_delimiter) ;
  else
    g_string_append_printf(gff_line, "%s", attribute->str) ;

  return result ;
}

/*
 * Put minimal header data into the string argument. Previous contents of the string
 * are destroyed if requested.
 */
gboolean zMapGFFFormatHeader(gboolean over_write, GString *line,
                             const char *sequence_region, int start, int end)
{
  gboolean status = FALSE ;
  char *time_string = NULL ;

  if (!line)
    return status ;
  status = TRUE ;

  if (over_write)
    g_string_truncate(line, (gsize)0) ;

  g_string_append_printf(line, "##gff-version %d\n", zMapGFFDumpVersionGet() ) ;
  if (sequence_region && *sequence_region && start && end)
    {
      g_string_append_printf(line, "##sequence-region %s %d %d\n",
                             sequence_region, start, end) ;
    }
  time_string = zMapGetTimeString(ZMAPTIME_YMD, NULL) ;
  if (time_string)
    {
      g_string_append_printf(line, "##source-version %s %s\n##date %s\n",
                             zMapGetAppName(), zMapGetAppVersionString(), time_string ) ;
      g_free(time_string) ;
    }


  return status ;
}

/*
 * Put the mandatory GFF data into the string argument. Previous contents of the string
 * are destroyed if requested. Appends a tab character if necessary.
 */
gboolean zMapGFFFormatMandatory(gboolean over_write, GString *line,
                                const char *sequence, const char *source, const char *type,
                                int start, int end, float score, ZMapStrand strand, ZMapPhase phase,
                                gboolean has_score, gboolean append_tab)
{
  static const char *s_CDS = "CDS" ;
  gboolean status = FALSE ;
  char *s_score = NULL,
    c_strand = '\0',
    c_phase = '\0' ;

  if (!line || !sequence || !*sequence
       /*   || !source || !*source */
            || !type || !*type
            || (start <= 0) || (start > end))
    return status ;
  status = TRUE ;

  /*
   * Score data.
   */
  if (has_score)
    {
      s_score = g_strdup_printf("%g", score) ;
    }
  else
    {
      s_score = g_strdup_printf(".") ;
    }

  /*
   * Strand data.
   */
  c_strand = zMapGFFFormatStrand2Char(strand) ;

  /*
   * Phase data. We _must_ have a phase given for a CDS and _not_
   * specified for anything else.
   */
  if (!strcmp(type, s_CDS))
    {
      c_phase = zMapGFFFormatPhase2Char(phase) ;
      if (c_phase == '.')
        status = FALSE ;
    }
  else
    {
      c_phase = zMapGFFFormatPhase2Char(phase) ;
      if (c_phase != '.')
        status = FALSE ;
    }

  /*
   * Fill in mandatory GFF fields.
   */
  if (status)
    {
      if (over_write)
        g_string_truncate(line, (gsize)0) ;
      g_string_append_printf(line, "%s\t%s\t%s\t%i\t%i\t%s\t%c\t%c",
                             sequence, source, type, start, end, s_score, c_strand, c_phase ) ;
      if (append_tab)
        line = g_string_append_c(line, '\t') ;
    }

  if (s_score)
    g_free(s_score) ;

  return status ;
}







/*
 * Output a line to a GIOChannel, returns FALSE if write fails and sets
 * err_msg_out with the reason.
 */
gboolean zMapGFFOutputWriteLineToGIO(GIOChannel *gio_channel,
                                     char **err_msg_out,
                                     GString *line,
                                     gboolean truncate_after_write)
{
  gboolean status = FALSE ;
  GError *channel_error = NULL ;
  gsize bytes_written = 0 ;

  if (!line)
    return status ;
  status = TRUE ;

  if (g_io_channel_write_chars(gio_channel, line->str, -1,
                               &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
    {
      *err_msg_out = g_strdup_printf("Could not write to file, error \"%s\" for line \"%50s...\"",
                                     channel_error->message, line->str) ;
      if (channel_error)
        g_error_free(channel_error) ;
      channel_error = NULL;
      status = FALSE ;
    }
  else
    {
      if (truncate_after_write)
        g_string_truncate(line, (gsize)0) ;
    }

  return status ;
}

/*
 * Write out alignment feature and attributes.
 */
gboolean zMapGFFWriteFeatureAlignment(ZMapFeature feature, ZMapGFFAttributeFlags flags,
                                      GString * line, gboolean over_write, const char *seq_str)
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  GString *attribute = NULL ;
  ZMapStrand strand = ZMAPSTRAND_NONE ;

  if (   !feature
      || (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT)
      || !feature->parent
      || !feature->parent->parent
      || !line)
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id);
  source_name = g_quark_to_string(feature->parent->original_id) ;
  if (strstr(source_name, anon_source))
    source_name = anon_source_ab ;
  type = g_quark_to_string(feature->SO_accession) ;
  strand = feature->strand ;

  if (!seq_str)
    {
      seq_str = feature->feature.homol.sequence ;
    }

  /*
   * Mandatory fields.
   */
  status = zMapGFFFormatMandatory(over_write, line,
                                  sequence_name, source_name, type,
                                  feature->x1, feature->x2,
                                  feature->score, strand, ZMAPPHASE_NONE,
                                  TRUE, TRUE ) ;

  if (status)
    {

      /*
       * Target attribute; note that the strand is optional
       */
      if (flags->target)
        {
          if (zMapWriteAttributeTarget(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE ) ;
            }
        }

      /*
       * percentID attribute
       */
      if (flags->percent_id)
        {
          if (zMapWriteAttributePercentID(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Gap attribute (gffv3)
       */
      if (flags->gap)
        {
          if (zMapWriteAttributeGap(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Sequence attribute
       */
      if (flags->sequence)
        {
          if (zMapWriteAttributeSequence(feature, attribute, seq_str))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      g_string_append_c(line, '\n') ;

    } /* if (status) */

  /*
   * Free when finished.
   */
  if (attribute)
    g_string_free(attribute, TRUE) ;

  return status ;
}



/*
 * Write out a transcript feature, also output the CDS if present.
 */
gboolean zMapGFFWriteFeatureTranscript(ZMapFeature feature, ZMapGFFAttributeFlags flags,
                                       GString *line, gboolean over_write)
{
  static const char *SO_exon_id = "exon",
    *SO_CDS_id = "CDS" ;
  gboolean status = FALSE ;
  int i=0 ;
  ZMapSpan span = NULL ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  GString *attribute = NULL ;

  if (   !feature
      || (feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
      || !feature->parent
      || !feature->parent->parent
      || !line)
    return status ;
  status = TRUE ;

  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id) ;
  source_name = g_quark_to_string(feature->parent->original_id) ;
  if (strstr(source_name, anon_source))
    source_name = anon_source_ab ;
  type = g_quark_to_string(feature->SO_accession) ;

  /*
   * Mandatory fields.
   */
  status = zMapGFFFormatMandatory(over_write, line,
                                  sequence_name, source_name, type,
                                  feature->x1, feature->x2,
                                  feature->score, feature->strand, ZMAPPHASE_NONE,
                                  (gboolean)feature->flags.has_score, TRUE ) ;

  if (status)
    {
      /*
       * ID attribute
       */
      if (flags->id)
        {
          if (zMapWriteAttributeID(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Name attribute
       */
      if (flags->name)
        {
          if (zMapWriteAttributeName(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Locus attribute
       */
      if (flags->locus)
        {
          if (zMapWriteAttributeLocus(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * evidence attribute
       */
      if (flags->evidence)
        {
          if (zMapWriteAttributeEvidence(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * End of line for parent object.
       */
      g_string_append_c(line, '\n');

      /*
       * Write a line for each exon and each CDS if present with
       * the ID of the previous line as Parent attribute.
       */
      if (flags->parent && zMapWriteAttributeParent(feature, attribute))
        {

          for (i = 0 ; i < (int)feature->feature.transcript.exons->len ; i++)
            {
              int exon_start=0, exon_end=0 ;

              span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i) ;

              if (span)
                {
                  exon_start = span->x1 ;
                  exon_end = span->x2 ;

                  /*
                   * Mandatory fields.
                   */
                  if (zMapGFFFormatMandatory(FALSE, line,
                                         sequence_name, source_name, SO_exon_id,
                                         exon_start, exon_end,
                                         (float)0.0, feature->strand, ZMAPPHASE_NONE,
                                         FALSE, TRUE ) )
                    {

                      /*
                       * Parent attribute and end of line.
                       */
                      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE ) ;
                      g_string_append_c(line, '\n');

                      /*
                       * Now do a CDS line if the feature has one.
                       */
                      if (feature->feature.transcript.flags.cds)
                        {
                          int cds_start=0, cds_end=0 ;

                          cds_start = feature->feature.transcript.cds_start ;
                          cds_end = feature->feature.transcript.cds_end ;

                          if (exon_start > cds_end || exon_end < cds_start)
                            {
                              ;
                            }
                          else
                            {
                              int tmp_cds1=0, tmp_cds2=0;
                              ZMapPhase phase = ZMAPPHASE_NONE ;

                              if (zMapFeatureExon2CDS(feature, exon_start, exon_end,
                                                      &tmp_cds1, &tmp_cds2, &phase))
                                {
                                  /* mandatory data */
                                  if (zMapGFFFormatMandatory(FALSE, line,
                                                         sequence_name, source_name, SO_CDS_id,
                                                         tmp_cds1, tmp_cds2,
                                                         (float)0.0, feature->strand, phase,
                                                         FALSE, TRUE ))
                                    {
                                      /* parent attribute and end of line */
                                      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
                                      g_string_append_c(line, '\n');
                                    }
                                }
                            }
                        } /* if the feature has a CDS */
                    } /* if write of exon line was OK */
                } /* if the exon span is valid */
            } /* loop over exons */
        } /* if we have a parent attribute */

    } /* if (status) */

  if (attribute)
    g_string_free(attribute, TRUE) ;

  return status ;
}


gboolean zMapGFFWriteFeatureText(ZMapFeature feature, ZMapGFFAttributeFlags flags,
                                 GString* line, gboolean over_write)
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  GString * attribute = NULL ;

  if (   !feature
      || (feature->mode != ZMAPSTYLE_MODE_TEXT)
      || !feature->parent
      || !feature->parent->parent
      || !line)
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id) ;
  source_name = g_quark_to_string(feature->parent->original_id) ;
  if (strstr(source_name, anon_source))
    source_name = anon_source_ab ;
  type = g_quark_to_string(feature->SO_accession) ;

  /*
   * Mandatory fields...
   */
  status = zMapGFFFormatMandatory(over_write, line,
                                  sequence_name, source_name, type,
                                  feature->x1, feature->x2,
                                  feature->score, feature->strand, ZMAPPHASE_NONE,
                                  (gboolean)feature->flags.has_score, TRUE) ;

  if (status)
    {
      /*
       * Name attribute
       */
      if (flags->name)
        {

          if (zMapWriteAttributeName(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Note attribute
       */
      if (flags->note)
        {
          if (zMapWriteAttributeNote(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * End of this feature line.
       */
      g_string_append_c(line, '\n') ;
    }

  /*
   * Free when finished
   */
  g_string_free(attribute, TRUE) ;

  return status ;
}

/*
 * Output of a basic feature.
 */
gboolean zMapGFFWriteFeatureBasic(ZMapFeature feature, ZMapGFFAttributeFlags flags,
                                  GString* line, gboolean over_write)
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  GString *attribute = NULL ;

  if (   !feature
      || (feature->mode != ZMAPSTYLE_MODE_BASIC)
      || !feature->parent
      || !feature->parent->parent
      || !line)
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id) ;
  source_name = g_quark_to_string(feature->parent->original_id) ;
  if (strstr(source_name, anon_source))
    source_name = anon_source_ab ;
  type = g_quark_to_string(feature->SO_accession) ;

  /*
   * Mandatory fields...
   */
  status = zMapGFFFormatMandatory(over_write, line,
                                  sequence_name, source_name, type,
                                  feature->x1, feature->x2,
                                  feature->score, feature->strand, ZMAPPHASE_NONE,
                                  (gboolean)feature->flags.has_score, TRUE) ;

  if (status)
    {

      /*
       * Name attribute
       */
      if (flags->name)
        {
          if (zMapWriteAttributeName(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * URL attribute
       */
      if (flags->url)
        {
          if (zMapWriteAttributeURL(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Variation string attribute.
       */
      if (flags->variation)
        {
          if (zMapWriteAttributeVariation(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Note attribute
       */
      if (flags->note)
        {
          if (zMapWriteAttributeNote(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * End of line.
       */
      g_string_append_c(line, '\n') ;

   }

  /*
   * Free when finished.
   */
  g_string_free(attribute, TRUE) ;

  return status ;
}




/*
 * Output of a graph feature.
 */
gboolean zMapGFFWriteFeatureGraph(ZMapFeature feature, ZMapGFFAttributeFlags flags,
                                  GString* line, gboolean over_write )
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  GString *attribute = NULL ;

  if (   !feature
      || (feature->mode != ZMAPSTYLE_MODE_GRAPH)
      || !feature->parent
      || !feature->parent->parent
      || !line)
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id) ;
  source_name = g_quark_to_string(feature->parent->original_id) ;
  if (strstr(source_name, anon_source))
    source_name = anon_source_ab ;
  type = g_quark_to_string(feature->SO_accession) ;

  /*
   * Mandatory fields...
   */
  status = zMapGFFFormatMandatory(over_write, line,
                                  sequence_name, source_name, type,
                                  feature->x1, feature->x2,
                                  feature->score, feature->strand, ZMAPPHASE_NONE,
                                  (gboolean)feature->flags.has_score, TRUE) ;

  if (status)
    {

      /*
       * Name attribute
       */
      if (flags->name)
        {
          if (zMapWriteAttributeName(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * URL attribute
       */
      if (flags->url)
        {
          if (zMapWriteAttributeURL(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * Note attribute
       */
      if (flags->note)
        {
          if (zMapWriteAttributeNote(feature, attribute))
            {
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
            }
        }

      /*
       * End of line.
       */
      g_string_append_c(line, '\n') ;

    }

  /*
   * Free this when finished.
   */
  g_string_free(attribute, TRUE) ;

  return status ;
}




/*
 * Create an object of ZMapGFFFormatData type.
 */
static ZMapGFFFormatData createGFFFormatData()
{
  ZMapGFFFormatData result = NULL ;

  result = (ZMapGFFFormatData) g_new0(ZMapGFFFormatDataStruct, 1) ;
  if (result)
    {
      result->attribute_flags = (ZMapGFFAttributeFlags) g_new0(ZMapGFFAttributeFlagsStruct, 1) ;
      result->buffer = NULL ;
      result->flags.over_write = FALSE ;
      result->flags.status     = FALSE ;
      result->flags.cont       = FALSE ;
    }

  return result ;
}

/*
 * Delete an object of ZMapGFFFormatData type.
 */
static void deleteGFFFormatData(ZMapGFFFormatData *p_format_data)
{
  if (!p_format_data || !*p_format_data)
    return ;
  ZMapGFFFormatData format_data = *p_format_data ;

  if (format_data->attribute_flags)
    {
      memset(format_data->attribute_flags, 0, sizeof(ZMapGFFAttributeFlagsStruct)) ;
      g_free(format_data->attribute_flags) ;
    }

  memset(format_data, 0, sizeof(ZMapGFFFormatDataStruct)) ;
  g_free(format_data) ;
  *p_format_data = NULL ;

  return ;
}


/*
 * Function to create the FeaturesetSearch object.
 */
static FeaturesetSearch createFeaturesetSearch()
{
  FeaturesetSearch result = NULL ;
  result = (FeaturesetSearch) g_new0(FeaturesetSearchStruct, 1) ;
  if (result)
    {
      result->results = NULL ;
      result->targets = NULL ;
    }
  return result ;
}

/*
 * This object only has ownership of the results GList.
 */
static void deleteFeaturesetSearch(FeaturesetSearch *p_featureset_search)
{
  if (!p_featureset_search || !*p_featureset_search)
    return ;
  FeaturesetSearch featureset_search = *p_featureset_search ;
  if (featureset_search->results)
    {
      g_list_free(featureset_search->results) ;
    }
  memset(featureset_search, 0, sizeof(FeaturesetSearchStruct)) ;
  g_free(featureset_search) ;
  *p_featureset_search = NULL ;
}

/*
 * Function to create the FeatureSearch object.
 */
static FeatureSearch createFeatureSearch()
{
  FeatureSearch result = NULL ;
  result = (FeatureSearch) g_new0(FeatureSearchStruct, 1) ;
  if (result)
    {
      result->region_span = NULL ;
      result->results = NULL ;
    }
  return result ;
}

/*
 * Note that this object only has ownership of the results GList.
 */
static void deleteFeatureSearch(FeatureSearch *p_feature_search)
{
  if (!p_feature_search || !*p_feature_search)
    return ;
  FeatureSearch feature_search = *p_feature_search ;
  if (feature_search->results)
    {
      g_list_free(feature_search->results) ;
    }
  memset(feature_search, 0, sizeof(FeatureSearchStruct)) ;
  g_free(feature_search) ;
  *p_feature_search = NULL ;
}


/*
 * Function to return the Name attribute.
 */
gboolean zMapWriteAttributeName(ZMapFeature feature, GString *attribute)
{
  static const char *format_str = "Name=%s" ;
  gboolean result = FALSE ;
  char *string_escaped = NULL ;

  if (!feature || !attribute)
    return result ;

  string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id),
                                       reserved_allowed, FALSE) ;
  if (string_escaped)
    {
      g_string_truncate(attribute, (gsize)0) ;
      g_string_printf(attribute, format_str, string_escaped) ;
      g_free(string_escaped) ;
      result = TRUE ;
    }

  return result ;
}

/*
 * Function to return the ID attribute (same as Name at present).
 */
gboolean zMapWriteAttributeID(ZMapFeature feature, GString *attribute)
{
  static const char *format_str = "ID=%s" ;
  gboolean result = FALSE ;
  char *string_escaped = NULL ;

  if (!feature || !attribute)
    return result ;

  string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id),
                                       reserved_allowed, FALSE) ;
  if (string_escaped)
    {
      g_string_truncate(attribute, (gsize)0) ;
      g_string_printf(attribute, format_str, string_escaped) ;
      g_free(string_escaped) ;
      result = TRUE ;
    }

  return result ;
}

/*
 * Function to return the Parent attribute (same as Name and ID at present).
 */
gboolean zMapWriteAttributeParent(ZMapFeature feature, GString *attribute)
{
  static const char *format_str = "Parent=%s" ;
  gboolean result = FALSE ;
  char *string_escaped = NULL ;

  if (!feature || !attribute)
    return result ;

  string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id),
                                       reserved_allowed, FALSE) ;
  if (string_escaped)
    {
      g_string_truncate(attribute, (gsize)0) ;
      g_string_printf(attribute, format_str, string_escaped) ;
      g_free(string_escaped) ;
      result = TRUE ;
    }

  return result ;
}


/*
 * Function to return the escaped URL attribute.
 */
gboolean zMapWriteAttributeURL(ZMapFeature feature, GString *attribute)
{
  static const char *format_str = "url=%s" ;
  gboolean result = FALSE ;
  char *string_escaped = NULL ;

  if (!feature || !attribute)
    return result ;

  if (feature->url)
    {
      string_escaped = g_uri_escape_string(feature->url,
                                           NULL, FALSE) ;
      if (string_escaped)
        {
          g_string_truncate(attribute, (gsize)0) ;
          g_string_printf(attribute, format_str, string_escaped) ;
          g_free(string_escaped) ;
          result = TRUE ;
        }
    }

  return result ;
}


/*
 * Function to return the escaped Note attribute.
 */
gboolean zMapWriteAttributeNote(ZMapFeature feature, GString *attribute)
{
  static const char *format_str = "Note=%s" ;
  gboolean result = FALSE ;
  char *string_escaped = NULL ;

  if (!feature || !attribute)
    return result ;

  if (feature->description)
    {
      string_escaped = g_uri_escape_string(feature->description,
                                           reserved_allowed, FALSE) ;
      if (string_escaped)
        {
          g_string_truncate(attribute, (gsize)0) ;
          g_string_printf(attribute, format_str, string_escaped) ;
          g_free(string_escaped) ;
          result = TRUE ;
        }
    }

  return result ;
}


/*
 * Function to return the sequence attribute.
 */
gboolean zMapWriteAttributeSequence(ZMapFeature feature, GString *attribute, const char *seq_str)
{
  static const char *format_str = "sequence=%s" ;
  gboolean result = FALSE ;

  if (!feature || !attribute || (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT))
    return result ;

  if (seq_str && *seq_str)
    {
      g_string_truncate(attribute, (gsize)0) ;
      g_string_printf(attribute, format_str, seq_str) ;
      result = TRUE ;
    }

  return result ;
}


/*
 * locus attribute
 */
gboolean zMapWriteAttributeLocus(ZMapFeature feature, GString *attribute)
{
  static const char *format_str = "locus=%s" ;
  gboolean result = FALSE ;
  char *string_escaped = NULL ;
  GQuark gqID = 0 ;

  if (    !feature
       || !attribute
       || (feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT)  )
    return result ;

  gqID = feature->feature.transcript.locus_id ;
  if (gqID)
    {
      string_escaped = g_uri_escape_string(g_quark_to_string(gqID),
                                           reserved_allowed, FALSE ) ;
      if (string_escaped)
        {
          g_string_truncate(attribute, (gsize)0) ;
          g_string_printf(attribute, format_str, string_escaped) ;
          g_free(string_escaped) ;
          result = TRUE ;
        }
    }

  return result ;
}



/*
 * Variation string attribute.
 */
gboolean zMapWriteAttributeVariation(ZMapFeature feature, GString* attribute)
{
  static const char *format_str = "variant_sequence=%s" ;
  gboolean result = FALSE ;

  if (    !feature
       || !attribute
       || (feature->mode != ZMAPSTYLE_MODE_BASIC)  )
    return result ;

  if (feature->feature.basic.flags.variation_str)
    {
      g_string_truncate(attribute, (gsize)0) ;
      g_string_printf(attribute, format_str, feature->feature.basic.variation_str) ;
      result = TRUE ;
    }

  return result ;
}

/*
 * Target attribute.
 */
gboolean zMapWriteAttributeTarget(ZMapFeature feature, GString *attribute)
{
  gboolean result = FALSE ;
  char * escaped_string = NULL ;
  int sstart = 0, send = 0 ;
  ZMapStrand s_strand = ZMAPSTRAND_NONE ;

  if (!feature || !attribute || (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT) )
   return result ;

  s_strand = feature->feature.homol.strand ;
  sstart = feature->feature.homol.y1 ;
  send = feature->feature.homol.y2 ;
  escaped_string = g_uri_escape_string(g_quark_to_string(feature->original_id), reserved_allowed, FALSE) ;
  if (escaped_string)
    {
      g_string_truncate(attribute, (gsize)0) ;
      if (s_strand != ZMAPSTRAND_NONE)
        {
          g_string_printf(attribute, "Target=%s %d %d %c",
                          escaped_string, sstart, send,
                          zMapGFFFormatStrand2Char(s_strand)) ;
        }
      else
        {
          g_string_printf(attribute, "Target=%s %d %d",
                          escaped_string, sstart, send) ;
        }
      g_free(escaped_string) ;
      result = TRUE ;
    }

  return result ;
}

/*
 * percentID attribute
 */
gboolean zMapWriteAttributePercentID(ZMapFeature feature, GString *attribute)
{
  gboolean result = FALSE ;
  float percent_id = 0.0 ;

  if (!feature || !attribute || (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT) )
    return result ;

  percent_id = feature->feature.homol.percent_id ;
  if (percent_id != 0.0)
    {
      g_string_truncate(attribute, (gsize)0) ;
      g_string_printf(attribute, "percentID=%g", percent_id) ;
      result = TRUE ;
    }

  return result ;
}

/*
 * Gap attribute
 */
gboolean zMapWriteAttributeGap(ZMapFeature feature, GString *attribute)
{
  gboolean result = FALSE ;
  GArray *gaps = NULL ;
  ZMapSequenceType match_seq_type = ZMAPSEQUENCE_NONE ;
  ZMapHomolType homol_type = ZMAPHOMOL_NONE ;
  ZMapStrand strand = ZMAPSTRAND_NONE ;

  if (!feature || !attribute || (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT) )
    return result ;

  gaps = feature->feature.homol.align ;
  strand = feature->strand ;
  if (gaps)
    {
      g_string_truncate(attribute, (gsize)0) ;
      homol_type = feature->feature.homol.type ;

      if (homol_type == ZMAPHOMOL_N_HOMOL)
        match_seq_type = ZMAPSEQUENCE_DNA ;
      else
        match_seq_type = ZMAPSEQUENCE_PEPTIDE ;

      zMapGFFFormatGap2GFF(attribute, gaps, strand, match_seq_type) ;
      result = TRUE ;
    }

  return result ;
}


/*
 * evidence attribute
 */
gboolean zMapWriteAttributeEvidence(ZMapFeature feature, GString *attribute)
{
  gboolean result = FALSE ;

  GList *evidence_list = zMapFeatureTranscriptGetEvidence(feature) ;

  if (evidence_list && g_list_length(evidence_list) > 0)
    {
      g_string_truncate(attribute, (gsize)0) ;
      g_string_append(attribute, "evidence=") ;

      /* Loop through all items in the evidence list and append to the attribute as a
       * space-separated list */
      GList *item = evidence_list ;
      const char separator = ' ' ;

      for ( ; item ; item = item->next)
        {
          GQuark evidence_quark = (GQuark)GPOINTER_TO_INT(item->data) ;
          const char *evidence_name = g_quark_to_string(evidence_quark) ;

          /* If there is another item to go, also append the separator */
          if (item->next)
            g_string_append_printf(attribute, "%s%c", evidence_name, separator) ;
          else
            g_string_append(attribute, evidence_name) ;
        }
        
      result = TRUE ;
    }

  return result ;
}


