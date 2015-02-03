/*  File: zmapGFF2Dumper.c
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Dumps the features within a given block as GFF v2
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


#define GFF_SEPARATOR  "\t"
#define GFF_SEQ        "%s"
#define GFF_SOURCE     "%s"
#define GFF_FEATURE    "%s"
#define GFF_START      "%d"
#define GFF_END        "%d"
#define GFF_SCORE      "%g"
#define GFF_STRAND     "%c"
#define GFF_PHASE      "%c"
#define GFF_ATTRIBUTES "%s"

#define GFF_ATTRIBUTE_TAB_BEGIN "\t"
#define GFF_ATTRIBUTE_TAB_END   ""

#define GFF_QUOTED_ATTRIBUTE "%s \"%s\""
#define GFF_UNQUOTED_ATTRIBUTE "%s=%s"

#define GFF_SEQ_SOURCE_FEAT_START_END \
GFF_SEQ GFF_SEPARATOR     \
GFF_SOURCE GFF_SEPARATOR  \
GFF_FEATURE GFF_SEPARATOR \
GFF_START GFF_SEPARATOR   \
GFF_END
#define GFF_SEP_NOSCORE GFF_SEPARATOR "%c"
#define GFF_SEP_STRAND GFF_SEPARATOR GFF_STRAND
#define GFF_SEP_PHASE GFF_SEPARATOR GFF_PHASE
#define GFF_OBLIGATORY_NOSCORE GFF_SEQ_SOURCE_FEAT_START_END GFF_SEP_NOSCORE GFF_SEP_STRAND GFF_SEP_PHASE

/* annoyingly we need to use a gpointer for 2nd argument. */
typedef gboolean (*zMapGFFFormatAttrFunc)(ZMapFeature feature, gpointer feature_data,
     GString *gff_string, GError **error_out,
     ZMapGFFFormatData format_data);

/* Fields are:
 * <sequence>\t<source>\t<feature>\t<start>\t<end>\t<score>\t<strand>\t<frame>\t<attributes>
 */
typedef struct _ZMapGFFFormatDataStruct
{
  GString *line ;
  const char *sequence;                 /* The sequence name. e.g. 16.12345-23456 */
  const char *source;                   /* The source e.g. augustus, TrEMBL, trf, polya_site */
  const char *so_term;                  /* The feature type e.g. Sequence, intron, exon, CDS, coding_exon, misc_feature */
  const char *feature_name;             /* The feature name e.g. AC12345.1-001 */
  int start, end ;

  struct
    {
      unsigned short attribute_name        : 1 ;
      unsigned short attribute_id          : 1 ;
      unsigned short attribute_parent      : 1 ;
      unsigned short attribute_note        : 1 ;
      unsigned short attribute_locus       : 1 ;
      unsigned short attribute_percent_id  : 1 ;
      unsigned short attribute_url         : 1 ;
      unsigned short attribute_gap         : 1 ;
      unsigned short attribute_target      : 1 ;
      unsigned short attribute_variation   : 1 ;
      unsigned short attribute_sequence    : 1 ;
      unsigned short status                : 1 ;
      unsigned short cont                  : 1 ;
    } flags ;

  /* sets of functions for doing attributes. */
  zMapGFFFormatAttrFunc *basic;
  zMapGFFFormatAttrFunc *transcript;
  zMapGFFFormatAttrFunc *homol;
  zMapGFFFormatAttrFunc *text;
} ZMapGFFFormatDataStruct ;

static gboolean formatExonGFF(GString *line,
                              const char * sequence_name, const char * source_name, const char * feature_name,
                              ZMapFeature feature,
                              int exon_start, int exon_end) ;

/* Functions to dump the header section of gff files */
static gboolean dump_full_header(ZMapFeatureAny feature_any, GIOChannel *file,
  ZMapGFFFormatData format_data, GError **error_out) ;
static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark key, gpointer data,
  gpointer user_data, char **err_out) ;

/* Functions to dump the body of the data */

/* ZMapFeatureDumpFeatureFunc to dump gff. writes lines into gstring buffer... */
static gboolean dump_gff_cb(ZMapFeatureAny feature_any, GHashTable *styles,
  GString *gff_string,  GError **error, gpointer user_data);

/*
 * dump attributes according to the format defined in the array of function pointers passed in.
 */
static gboolean dump_attributes(zMapGFFFormatAttrFunc func_array[], ZMapFeature feature,
  gpointer feature_data, GString *gff_string, GError **error, ZMapGFFFormatData format_data);

/*
 * V3 attributes
 */
static gboolean dump_text_note_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data) ;
static gboolean dump_transcript_locus_v3(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data) ;
static gboolean dump_id_as_name_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data) ;
static gboolean dump_transcript_id_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data) ;

/*
 * V3 versions of transcript output functions
 */
static gboolean dump_transcript_subpart_v3(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data) ;
static gboolean dump_transcript_foreach_subpart_v3(ZMapFeature feature, GString *buffer,
  GError **error_out, GArray *subparts, ZMapGFFFormatData format_data) ;
static gboolean dump_transcript_foreach_subpart_v3_cds(ZMapFeature feature, GString *buffer,
  GError **error_out, ZMapTranscript transcript, ZMapGFFFormatData format_data) ;
/*
 * V3 versions of alignment output functions
 */
static gboolean dump_alignment_target_v3(ZMapFeature feature, gpointer homol,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data) ;
static gboolean dump_alignment_gap_v3(ZMapFeature feature, gpointer homol_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data) ;



/*
 * These arrays of function pointers are used to obtain
 * format and output various attributes of different types
 * of features. They depend not only on the style mode of
 * the features themselves (BASIC, TRANSCRIPT, ALIGNMENT, etc)
 * but also on GFF version. There are seperate sets of
 * tables for v2 and v3.
 */

static zMapGFFFormatAttrFunc basic_funcs_G_GFF3[] = {
  dump_text_note_v3,
  dump_id_as_name_v3,
  NULL,
};
static zMapGFFFormatAttrFunc transcript_funcs_G_GFF3[] = {
  dump_transcript_id_v3,
  dump_id_as_name_v3,
  dump_transcript_subpart_v3,
  NULL
};
static zMapGFFFormatAttrFunc homol_funcs_G_GFF3[] = {
  dump_alignment_target_v3,
  dump_alignment_gap_v3,
  NULL
};
static zMapGFFFormatAttrFunc text_funcs_G_GFF3[] = {
  dump_text_note_v3,
  dump_transcript_locus_v3,
  NULL
};

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
  ZMapGFFFormatDataStruct format_data = {NULL};

  zMapReturnValIfFail(   file && dump_set
                      && (dump_set->struct_type != ZMAPFEATURE_STRUCT_INVALID)
                      && error_out,
                                  result) ;

  format_data.sequence = NULL ;
  format_data.line  = g_string_new(NULL) ;
  format_data.flags.cont   = TRUE;
  format_data.flags.status = TRUE;
  result = dump_full_header(dump_set, file, &format_data, error_out) ;

  if (result)
    {
      format_data.basic      = basic_funcs_G_GFF3;
      format_data.transcript = transcript_funcs_G_GFF3;
      format_data.homol      = homol_funcs_G_GFF3;
      format_data.text       = text_funcs_G_GFF3;

      /* This might get overwritten later, but as DumpToFile uses
       * Subset, there's a chance it wouldn't get set at all */
      if(region_span)
        result = zMapFeatureContextRangeDumpToFile((ZMapFeatureAny)dump_set, styles, region_span,
                   dump_gff_cb, &format_data, file, error_out) ;
      else
        result = zMapFeatureContextDumpToFile((ZMapFeatureAny)dump_set, styles,
                   dump_gff_cb, &format_data, file, error_out) ;
    }

  if (format_data.line)
    g_string_free(format_data.line, TRUE) ;

  return result ;
}

/*!
 * \brief Dump a list of ZMapFeatureAny to a file and/or text buffer. sequence can be NULL
 */
gboolean zMapGFFDumpList(GList *dump_list,
                         GHashTable *styles,
                         char *sequence,
                         GIOChannel *file,
                         GString **text_out,
                         GError **error_out)
{
  gboolean result = FALSE ;
  ZMapFeatureAny feature_any = NULL ;
  ZMapGFFFormatDataStruct format_data = {NULL};

  zMapReturnValIfFail(dump_list && dump_list->data && error_out, result) ;
  feature_any  = (ZMapFeatureAny)(dump_list->data);
  zMapReturnValIfFail(feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID, result ) ;

  format_data.sequence = sequence;
  format_data.line = g_string_new(NULL) ;
  format_data.flags.cont    = TRUE;
  format_data.flags.status = TRUE;
  result = dump_full_header(feature_any, file, &format_data, error_out) ;

  if (result)
    {
      format_data.basic      = basic_funcs_G_GFF3;
      format_data.transcript = transcript_funcs_G_GFF3;
      format_data.homol      = homol_funcs_G_GFF3;
      format_data.text       = text_funcs_G_GFF3;

      result = zMapFeatureListDumpToFileOrBuffer(dump_list, styles, dump_gff_cb, &format_data, file, text_out, error_out) ;

    }

  if (format_data.line)
    g_string_free(format_data.line, TRUE) ;

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
                      && file
                      && error_out, FALSE ) ;

  /*
   * This descends the data structures to find the ##sequence-region
   * string, start and end coordinates.
   */
  zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_BLOCK,
                            get_type_seq_header_cb, format_data) ;


  /*
   * Formats header and appends to the buffer string object.
   */
  zMapGFFFormatHeader(FALSE, format_data->line, format_data->sequence,
                      format_data->start, format_data->end ) ;

  /*
   * Now write this to the IO channel.
   */
  if(format_data->flags.status)
    {
      char *error_msg = NULL ;
      format_data->flags.status = zMapGFFOutputWriteLineToGIO(file, &error_msg, format_data->line, TRUE) ;
      if (error_msg)
        {
          *error_out = g_error_new(g_quark_from_string("ERROR in dump_full_header()"),
                                   (gint)0, "message was '%s'", error_msg ) ;
          g_free(error_msg) ;
        }
    }

  return format_data->flags.status ;
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
            /*
             * ZMapFeatureAlignment feature_align = (ZMapFeatureAlignment)feature_any;
             * const char *sequence;
             * format_data->status = format_data->cont = TRUE;
             * sequence = g_quark_to_string(feature_any->original_id);
             * g_string_append_printf(format_data->line, "##Type DNA %s %d %d\n",
             *   sequence, feature_align->sequence_span.x1, feature_align->sequence_span.x2);
             * if(!format_data->sequence)
             *   format_data->sequence = sequence ;
             */
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
                      && (feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID)
                      && feature_any->parent
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
           * sequence
           */
          format_data->sequence = g_quark_to_string(featureset->parent->original_id) ;

          /*
           * source
           */
          format_data->source = g_quark_to_string(gqFeatureset) ;
          if (strstr(format_data->source, "anon_source"))
              format_data->source = "." ;

          /*
           * type
           */
          format_data->so_term = g_quark_to_string(feature->SO_accession) ;

          /*
           * feature name
           */
          format_data->feature_name = g_quark_to_string(feature->original_id) ;

         /*
          * Mandatory fields
          */
         zMapGFFFormatMandatory(FALSE, line,
                                format_data->sequence,
                                format_data->source,
                                format_data->so_term,
                                feature->x1, feature->x2,
                                feature->score, feature->strand, ZMAPPHASE_NONE,
                                (gboolean)feature->flags.has_score, FALSE) ;

          /*
           * Now switch upon ZMapStyleMode and do attributes
           */
          switch(feature->mode)
            {
              case ZMAPSTYLE_MODE_BASIC:
                {
                  result = zMapGFFFormatAttributeSetBasic(format_data) ;
                  result = dump_attributes(format_data->basic,
                                           feature,
                                           &(feature->feature.basic),
                                           line,
                                           error, format_data) ;
                }
                break;
              case ZMAPSTYLE_MODE_TRANSCRIPT:
                {
                  result = zMapGFFFormatAttributeSetTranscript(format_data) ;
                  result = dump_attributes(format_data->transcript,
                                           feature,
                                           &(feature->feature.transcript),
                                           line,
                                           error, format_data) ;

                }
                break;
              case ZMAPSTYLE_MODE_ALIGNMENT:
                {
                  result = zMapGFFFormatAttributeSetAlignment(format_data) ;
                  result = dump_attributes(format_data->homol,
                                           feature,
                                           &(feature->feature.homol),
                                           line,
                                           error, format_data) ;
                }
                break;
              case ZMAPSTYLE_MODE_TEXT:
                {
                  result = zMapGFFFormatAttributeSetText(format_data) ;
                  result = dump_attributes(format_data->text,
                                           feature,
                                           NULL,
                                           line,
                                           error, format_data) ;
                }
                break;
              default:
                break;
            }

            /*
             * end of line for this feature..
             */
            g_string_append_c(line, '\n') ;
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
gboolean zMapGFFFormatAttributeUnsetAll(ZMapGFFFormatData format_data)
{
  gboolean result = FALSE ;

  if (!format_data)
    return result ;
  result = TRUE ;

  format_data->flags.attribute_name       = FALSE ;
  format_data->flags.attribute_id         = FALSE ;
  format_data->flags.attribute_parent     = FALSE ;
  format_data->flags.attribute_note       = FALSE ;
  format_data->flags.attribute_locus      = FALSE ;
  format_data->flags.attribute_percent_id = FALSE ;
  format_data->flags.attribute_url        = FALSE ;
  format_data->flags.attribute_gap        = FALSE ;
  format_data->flags.attribute_target     = FALSE ;
  format_data->flags.attribute_variation  = FALSE ;
  format_data->flags.attribute_sequence   = FALSE ;

  return result ;
}

gboolean zMapGFFFormatAttributeSetBasic(ZMapGFFFormatData format_data)
{
  gboolean result = FALSE ;

  if (!format_data)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(format_data) ;

  format_data->flags.attribute_name        = TRUE ;
  format_data->flags.attribute_url         = TRUE ;
  format_data->flags.attribute_variation   = TRUE ;
  format_data->flags.attribute_note        = TRUE ;

  return result ;
}

gboolean zMapGFFFormatAttributeSetTranscript(ZMapGFFFormatData format_data)
{
  gboolean result = FALSE ;

  if (!format_data)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(format_data) ;

  format_data->flags.attribute_name        = TRUE ;
  format_data->flags.attribute_id          = TRUE ;
  format_data->flags.attribute_note        = TRUE ;
  format_data->flags.attribute_locus       = TRUE ;
  format_data->flags.attribute_url         = TRUE ;

  return result ;
}

gboolean zMapGFFFormatAttributeSetAlignment(ZMapGFFFormatData format_data)
{
  gboolean result = FALSE ;

  if (!format_data)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(format_data) ;

  format_data->flags.attribute_name        = TRUE ;
  format_data->flags.attribute_gap         = TRUE ;
  format_data->flags.attribute_target      = TRUE ;
  format_data->flags.attribute_percent_id  = TRUE ;
  format_data->flags.attribute_sequence    = TRUE ;

  return result ;
}

gboolean zMapGFFFormatAttributeSetText(ZMapGFFFormatData format_data)
{
  gboolean result = FALSE ;

  if (!format_data)
    return result ;
  result = zMapGFFFormatAttributeUnsetAll(format_data) ;

  format_data->flags.attribute_name        = TRUE ;
  format_data->flags.attribute_note        = TRUE ;
  format_data->flags.attribute_url         = TRUE ;
  format_data->flags.attribute_locus       = TRUE ;

  return result ;
}


/* attribute dumper. run through the dump functions separating the
 * attributes with GFF_ATTRIBUTE_SEPARATOR */

static gboolean dump_attributes(zMapGFFFormatAttrFunc func_array[], ZMapFeature feature,
                                gpointer feature_data, GString *gff_string,
                                GError **error, ZMapGFFFormatData format_data)
{
  zMapGFFFormatAttrFunc *funcs_ptr = NULL ;
  gboolean result = TRUE,
    need_separator = FALSE;

  funcs_ptr = func_array;

  if (gff_output_version_G == ZMAPGFF_VERSION_3)
    {
      g_string_append(gff_string, GFF_ATTRIBUTE_TAB_BEGIN ) ;
    }

  while(funcs_ptr && *funcs_ptr)
    {
      if(need_separator)
        {
          g_string_append(gff_string, ";");
          need_separator = FALSE;
        }

      if((*funcs_ptr)(feature, feature_data, gff_string, error, format_data))
        need_separator = TRUE;

      funcs_ptr++;
    }

  return result;
}

static gboolean dump_text_note_v3(ZMapFeature feature, gpointer basic_data, GString *gff_string, GError **error, ZMapGFFFormatData format_data)
{
  gboolean result = FALSE;

  if(feature->description)
  {
    g_string_append_printf(gff_string,
      GFF_UNQUOTED_ATTRIBUTE,
      "Note", feature->description);
    result = TRUE;
  }

  return result;
}

static gboolean dump_transcript_locus_v3(ZMapFeature feature, gpointer transcript_data,
                                      GString *gff_string, GError **error,
                                      ZMapGFFFormatData format_data)
{
  gboolean result = FALSE;

  if (feature->feature.transcript.locus_id)
  {
    /* attribute of the form 'locus=AC12345.1'          */
    g_string_append_printf(gff_string,
      GFF_UNQUOTED_ATTRIBUTE,
      "locus", g_quark_to_string(feature->feature.transcript.locus_id));
    result = TRUE;
  }

return result;
}


static gboolean dump_id_as_name_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data)
{
  gboolean result = FALSE ;

  if (feature->original_id)
    {
      g_string_append_printf(gff_string,
        GFF_UNQUOTED_ATTRIBUTE,
        "Name", g_quark_to_string(feature->original_id)) ;
      result = TRUE ;
    }

  return result ;
}


/*
 * Appends "ID=<id>" to the string where <id> = g_quark_to_string(feature->unique_id).
 */
static gboolean dump_transcript_id_v3(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data)
{
  gboolean result = TRUE;

  if (feature->unique_id)
    {
      g_string_append_printf(gff_string,
        GFF_UNQUOTED_ATTRIBUTE,
        "ID", g_quark_to_string(feature->unique_id));
    }

  return result;
}


/*
 * Appends "Parent=<id>" to the string where <id> = g_quark_to_string(feature->unqique_id).
 */
static gboolean dump_transcript_parent_v3(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, ZMapGFFFormatData format_data)
{
  gboolean result = TRUE;

  if (feature->unique_id)
    {
      g_string_append_printf(gff_string,
        GFF_UNQUOTED_ATTRIBUTE,
        "Parent", g_quark_to_string(feature->unique_id));
    }

  return result;
}




/* subpart full lines... */
static gboolean dump_transcript_subpart_v3(ZMapFeature feature, gpointer transcript_data,
                                              GString *gff_string, GError **error,
                                              ZMapGFFFormatData format_data)
{
  ZMapTranscript transcript = (ZMapTranscript)transcript_data;
  gboolean result = TRUE;
  if (!transcript)
    return result ;

  /*
   * Output all exons.
   */
  if(result && transcript->exons)
  {
    format_data->so_term = "exon";
    result = dump_transcript_foreach_subpart_v3(feature, gff_string, error,
                                                transcript->exons, format_data);
  }

  /*
   * Output all introns. Don't really need these, but will leave it in for
   * the time being.
   */
  if(result && transcript->introns)
  {
    format_data->so_term = "intron";
    result = dump_transcript_foreach_subpart_v3(feature, gff_string, error,
                                                transcript->introns, format_data);
  }

  /*
   * This is where the special treatment of the cds as multiple part
   * should be inserted. Probably modify the foreach function used above.
   */
  if(result && transcript->flags.cds)
  {
    format_data->so_term = "CDS";
    result = dump_transcript_foreach_subpart_v3_cds(feature, gff_string, error,
                                                    transcript, format_data) ;
  }

  return result;
}


/*
 * Dump transcript subparts.
 */
static gboolean dump_transcript_foreach_subpart_v3(ZMapFeature feature,
                                                   GString *buffer,
                                                   GError **error_out,
                                                   GArray *subparts,
                                                   ZMapGFFFormatData format_data)
{
  gboolean result = TRUE ;
  int i = 0 ;

  for (i=0; i < subparts->len; ++i)
    {
      ZMapSpanStruct span = g_array_index(subparts, ZMapSpanStruct, i) ;
      ZMapPhase phase = ZMAPPHASE_NONE ;

      g_string_append_printf(buffer, "\n" GFF_OBLIGATORY_NOSCORE,
                             format_data->sequence,
                             format_data->source,
                             format_data->so_term,
                             span.x1, span.x2,
                             '.', /* no score */
                             zMapGFFFormatStrand2Char(feature->strand),
                             zMapGFFFormatPhase2Char(phase)) ;
      g_string_append_printf(buffer, "\t") ;
      result = dump_transcript_parent_v3(feature, &(feature->feature.transcript), buffer, error_out, format_data);
    }

  return result ;
}

/*
 * This dumps a CDS but uses a seperate line for each part of the CDS that overlaps
 * with each exon.
 */
static gboolean dump_transcript_foreach_subpart_v3_cds(ZMapFeature feature,
                                                       GString *buffer,
                                                       GError **error_out,
                                                       ZMapTranscript transcript,
                                                       ZMapGFFFormatData format_data)
{
  gboolean result = FALSE,
    start_within = FALSE,
    end_within = FALSE,
    exon_within = FALSE ;
  int i = 0,
    exon_start = 0,
    exon_end = 0,
    cds_start = 0,
    cds_end = 0,
    start = 0,
    end = 0 ;
  ZMapPhase phase = ZMAPPHASE_NONE ;

  cds_start = transcript->cds_start ;
  cds_end = transcript->cds_end ;

  phase = transcript->phase ;
  if (phase == ZMAPPHASE_NONE)
    return result ;
  result = TRUE ;

  for (i=0; i<transcript->exons->len && result; ++i)
    {
      start_within = FALSE ;
      end_within = FALSE ;
      exon_within = FALSE ;
      ZMapSpanStruct span = g_array_index(transcript->exons, ZMapSpanStruct, i) ;

      exon_start = span.x1 ;
      exon_end = span.x2 ;

      if (cds_start>=exon_start && cds_start<=exon_end)
        start_within = TRUE ;
      if (cds_end>=exon_start && cds_end<=exon_end)
        end_within = TRUE ;
      if (cds_start<exon_start && cds_end>exon_end)
        exon_within = TRUE ;

      if (start_within || end_within || exon_within)
        {
          start = cds_start > exon_start ? cds_start : exon_start ;
          end = cds_end < exon_end ? cds_end : exon_end ;

          g_string_append_printf(buffer, "\n" GFF_OBLIGATORY_NOSCORE,
                                 format_data->sequence,
                                 format_data->source,
                                 format_data->so_term,
                                 start, end, '.',
                                 zMapGFFFormatStrand2Char(feature->strand),
                                 zMapGFFFormatPhase2Char(phase) );
          g_string_append_printf(buffer, "\t") ;
          result = dump_transcript_parent_v3(feature, transcript, buffer, error_out, format_data);

        }
    }


  return result ;
}





/* alignment attributes... */

static gboolean dump_alignment_target_v3(ZMapFeature feature, gpointer homol_data,
                                      GString *gff_string, GError **error,
                                      ZMapGFFFormatData format_data)
{
  static const char * read_pair = "read_pair" ;
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean result = FALSE ;
  char *temp_string = NULL ;

  /*
   * We need different behaviour here if the
   * feature has SOtype == read_pair... Target attribute
   * is not appropriate in this case, we just want the
   * "read_pair_id" attribute.
   */

  if(homol->clone_id)
  {
    if (!strcmp(g_quark_to_string(feature->SO_accession), read_pair))
      {
        g_string_append_printf(gff_string, "read_pair_id=%s",
          g_quark_to_string(homol->clone_id));
        result = TRUE ;
      }
    else
      {
        if (zMapAttGenerateTarget(&temp_string, feature))
          {
            if (temp_string)
              {
                g_string_append_printf(gff_string, "%s", temp_string) ;
                g_free(temp_string) ;
                result = TRUE ;
              }
          }

      }
  }

  return result ;
}


/*
 * This needs to be converted to output GFF3 "Gap" attribute.
 */
static gboolean dump_alignment_gap_v3(ZMapFeature feature, gpointer homol_data,
                                       GString *gff_string, GError **error,
                                       ZMapGFFFormatData format_data)
{
  gboolean result = FALSE;
  char * temp_string = NULL ;

  if (zMapAttGenerateGap(&temp_string, feature))
    {
      if (temp_string)
        {
          g_string_append_printf(gff_string, "%s", temp_string) ;
          g_free(temp_string) ;
          result = TRUE ;
        }
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

      for (k = 0 ; k < gaps->len ; k++, index += incr)
        {

          gap = &(g_array_index(gaps, ZMapAlignBlockStruct, index)) ;

          coord = calcBlockLength(match_seq_type, gap->t1, gap->t2) ;
          g_string_append_printf(attribute, "M%d ", coord) ;

          if (k < gaps->len - 1)
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
                  g_string_append_printf(attribute, "I%d ", (next_match - curr_match) - 1) ;
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
            || !source || !*source
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


gboolean zMapGFFWriteFeatureAlignment(ZMapFeature feature, GString * line,
                               const char *seq_str)
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *feature_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  GString *attribute = NULL ;
  ZMapStrand strand = ZMAPSTRAND_NONE,
    s_strand = ZMAPSTRAND_NONE ;
  float percent_id = (float) 0.0 ;
  GArray *gaps = NULL ;

  if (!feature || !feature->parent || !feature->parent->parent || !line )
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id);
  source_name = g_quark_to_string(feature->parent->original_id) ;
  feature_name = g_quark_to_string(feature->original_id) ;
  type = g_quark_to_string(feature->SO_accession) ;
  strand = feature->strand ;

  if (!seq_str)
    {
      seq_str = feature->feature.homol.sequence ;
    }

  /*
   * Mandatory fields.
   */
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, type,
                         feature->x1, feature->x2,
                         feature->score, strand, ZMAPPHASE_NONE,
                         TRUE, TRUE ) ;

  /*
   * Target attribute; note that the strand is optional
   */
  if (feature_name && *feature_name)
    {
      ZMapStrand s_strand = feature->feature.homol.strand ;
      int sstart = feature->feature.homol.y1,
        send = feature->feature.homol.y2 ;
      if (s_strand != ZMAPSTRAND_NONE)
        {
          g_string_printf(attribute, "Target=%s %d %d %c",
                          feature_name, sstart, send,
                          zMapGFFFormatStrand2Char(s_strand)) ;
        }
      else
        {
          g_string_printf(attribute, "Target=%s %d %d",
                          feature_name, sstart, send) ;
        }
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * percentID attribute
   */
  percent_id = feature->feature.homol.percent_id ;
  if (percent_id != (float)0.0)
    {
      g_string_printf(attribute, "percentID=%g", percent_id) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * Gap attribute (gffv3)
   */
  gaps = feature->feature.homol.align ;
  if (gaps)
    {
      ZMapSequenceType match_seq_type = ZMAPSEQUENCE_NONE ;
      ZMapHomolType homol_type = feature->feature.homol.type ;
      if (homol_type == ZMAPHOMOL_N_HOMOL)
        match_seq_type = ZMAPSEQUENCE_DNA ;
      else
        match_seq_type = ZMAPSEQUENCE_PEPTIDE ;

      zMapGFFFormatGap2GFF(attribute, gaps, strand, match_seq_type) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * Sequence attribute
   */
  if (seq_str && *seq_str)
    {
      g_string_printf(attribute, "sequence=%s", seq_str) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  g_string_append_c(line, '\n') ;

  if (attribute)
    g_string_free(attribute, TRUE) ;

  return status ;
}



/*
 * Write out a transcript feature, also output the CDS if present.
 */
gboolean zMapGFFWriteFeatureTranscript(ZMapFeature feature, GString *line)
{
  static const char *SO_exon_id = "exon",
    *SO_CDS_id = "CDS" ;
  gboolean status = FALSE ;
  int i=0 ;
  ZMapSpan span = NULL ;
  const char *sequence_name = NULL,
    *feature_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  GString *attribute = NULL ;

  if (!line || !feature || !feature->parent || !feature->parent->parent)
    return status ;
  status = TRUE ;

  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id) ;
  source_name = g_quark_to_string(feature->parent->original_id) ;
  feature_name = g_quark_to_string(feature->original_id) ;
  type = g_quark_to_string(feature->SO_accession) ;

  /*
   * Mandatory fields.
   */
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, type,
                         feature->x1, feature->x2,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE ) ;

  /*
   *ID attribute
   */
  g_string_printf(attribute, "ID=%s", feature_name) ;
  zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
  g_string_truncate(attribute, (gsize)0) ;

  /*
   * Name attribute
   */
  g_string_printf(attribute, "Name=%s", feature_name) ;
  zMapGFFFormatAppendAttribute(line, attribute, FALSE, FALSE) ;
  g_string_truncate(attribute, (gsize)0) ;

  /*
   * End of line for parent object.
   */
  g_string_append_c(line, '\n');

  /*
   * Write a line for each exon and each CDS if present with
   * the ID of the previous line as Parent attribute.
   */
  g_string_printf(attribute, "Parent=%s", feature_name) ;
  for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
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
          zMapGFFFormatMandatory(FALSE, line,
                                 sequence_name, source_name, SO_exon_id,
                                 exon_start, exon_end,
                                 feature->score, feature->strand, ZMAPPHASE_NONE,
                                 (gboolean)feature->flags.has_score, TRUE ) ;

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
                      zMapGFFFormatMandatory(FALSE, line,
                                             sequence_name, source_name, SO_CDS_id,
                                             tmp_cds1, tmp_cds2,
                                             feature->score, feature->strand, phase,
                                             (gboolean)feature->flags.has_score, TRUE ) ;
                      /* parent attribute and end of line */
                      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
                      g_string_append_c(line, '\n');
                    }
                }
            } // if (is_cds)

        } // if (is_exon)

    } // loop over exons

  if (attribute)
    g_string_free(attribute, TRUE) ;

  return status ;
}

/*
 * This is for exons and CDS subfeatures only.
 */
static gboolean formatExonGFF(GString *line,
                             const char * sequence_name, const char * source_name, const char *feature_name,
                             ZMapFeature feature,
                             int exon_start, int exon_end)
{
  static const char *SO_exon_id = "exon",
    *SO_CDS_id = "CDS" ;
  gboolean status = FALSE ;
  GString *attribute = NULL ;

  if (!sequence_name || !feature)
    return status ;
  status = TRUE ;

  attribute = g_string_new(NULL) ;

  /*
   * Mandatory fields.
   */
  zMapGFFFormatMandatory(FALSE, line,
                         sequence_name, source_name, SO_exon_id,
                         exon_start, exon_end,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE ) ;

  /*
   * Parent attribute
   */
  g_string_printf(attribute, "Parent=%s", feature_name) ;
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
              zMapGFFFormatMandatory(FALSE, line,
                         sequence_name, source_name, SO_CDS_id,
                         tmp_cds1, tmp_cds2,
                         feature->score, feature->strand, phase,
                         (gboolean)feature->flags.has_score, TRUE ) ;
              /* attributes */
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
              g_string_append_c(line, '\n');
            }
        }
    }

  if (attribute)
    g_string_free(attribute, TRUE) ;

  return status ;
}

gboolean zMapGFFWriteFeatureText(ZMapFeature feature, GString* line)
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  char *string_escaped = NULL ;
  GString * attribute = NULL ;
  GQuark gqID = 0 ;

  if (!feature || !feature->parent || !feature->parent->parent || !line)
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id) ;
  source_name = g_quark_to_string(feature->parent->original_id) ;
  type = g_quark_to_string(feature->SO_accession) ;

  /*
   * Mandatory fields...
   */
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, type,
                         feature->x1, feature->x2,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE) ;

  /*
   * Name attribute
   */
  string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id), NULL, FALSE) ;
  g_string_printf(attribute, "Name=%s", string_escaped) ;
  zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
  if (string_escaped)
    g_free(string_escaped) ;
  string_escaped = NULL ;
  g_string_truncate(attribute, (gsize)0) ;

  /*
   * Note attribute
   */
  if (feature->description)
    {
      string_escaped = g_uri_escape_string(feature->description, NULL, FALSE) ;
      if (string_escaped)
        {
          g_string_printf(attribute, "Note=%s", string_escaped) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_string_truncate(attribute, (gsize)0) ;
          g_free(string_escaped) ;
          string_escaped = NULL ;
        }
    }

  /*
   * locus_id attribute
   */
  gqID = feature->feature.transcript.locus_id ;
  if (gqID)
    {
      string_escaped = g_uri_escape_string(g_quark_to_string(gqID), NULL, FALSE ) ;
      if (string_escaped)
        {
          g_string_printf(attribute, "locus=%s", string_escaped) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_string_truncate(attribute, (gsize)0) ;
          g_free(string_escaped) ;
          string_escaped = NULL ;
        }
    }


  /*
   * End of this feature line.
   */
  g_string_append_c(line, '\n') ;
  g_string_free(attribute, TRUE) ;

  return status ;
}

/*
 *
 */
gboolean zMapGFFWriteFeatureBasic(ZMapFeature feature, GString* line )
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL ;
  char *string_escaped = NULL ;
  GString *attribute = NULL ;

  if (!feature || !feature->parent || !feature->parent->parent || !line)
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  sequence_name = g_quark_to_string(feature->parent->parent->original_id) ;
  source_name = g_quark_to_string(feature->parent->original_id) ;

  /*
   * Mandatory fields...
   */
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, g_quark_to_string(feature->SO_accession),
                         feature->x1, feature->x2,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE) ;

  /*
   * Name attribute
   */
  string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id), NULL, FALSE) ;
  g_string_printf(attribute, "Name=%s", string_escaped) ;
  zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
  if (string_escaped)
    g_free(string_escaped) ;
  string_escaped = NULL ;
  g_string_truncate(attribute, (gsize)0) ;

  /*
   * URL attribute
   */
  if (feature->url)
    {
      string_escaped = g_uri_escape_string(feature->url, NULL, FALSE) ;
      g_string_printf(attribute, "url=%s", string_escaped) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
      if (string_escaped)
        g_free(string_escaped) ;
      string_escaped = NULL ;
    }

  /*
   * Variation string attribute.
   */
  if (feature->feature.basic.flags.variation_str)
    {
      g_string_printf(attribute, "variant_sequence=%s", feature->feature.basic.variation_str) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * End of line.
   */
  g_string_append_c(line, '\n') ;
  g_string_free(attribute, TRUE) ;

  return status ;
}







/*
 *
 * What follows is the old V2 stuff.
 *
 */







/*
static gboolean dump_text_note(ZMapFeature feature, gpointer basic_data, GString *gff_string, GError **error, ZMapGFFFormatData format_data)
{
  gboolean result = FALSE;

  if(feature->description)
  {
    g_string_append_printf(gff_string,
      GFF_QUOTED_ATTRIBUTE,
      "Note", feature->description);
    result = TRUE;
  }

  return result;
}
*/





/* transcript calls */

/* attributes */
  /* To make       Sequence "AC12345.1"          */
/*
static gboolean dump_transcript_identifier(ZMapFeature feature, gpointer transcript_data,
                                           GString *gff_string, GError **error,
                                           ZMapGFFFormatData format_data)
{
  gboolean result = TRUE;

  g_string_append_printf(gff_string,
    GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
    format_data->so_term,
    format_data->feature_name);

  return result;
}*/

      /* To make       Locus "AC12345.1"          */
/*
static gboolean dump_transcript_locus(ZMapFeature feature, gpointer transcript_data,
                                      GString *gff_string, GError **error,
                                      ZMapGFFFormatData format_data)
{
  gboolean result = FALSE;

  if (feature->feature.transcript.locus_id)
    {
      g_string_append_printf(gff_string,
        GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
        "Locus",
        (char *)g_quark_to_string(feature->feature.transcript.locus_id));
      result = TRUE;
    }

  return result;
}
*/





/* subpart full lines... */
/*
static gboolean dump_transcript_subpart_lines(ZMapFeature feature, gpointer transcript_data,
                                              GString *gff_string, GError **error,
                                              ZMapGFFFormatData format_data)
{
  ZMapTranscript transcript = (ZMapTranscript)transcript_data;
  gboolean result = TRUE;

  if(result && transcript->exons)
  {
    format_data->so_term = "exon";
    result = dump_transcript_foreach_subpart(feature, gff_string, error,
      transcript->exons, format_data);
  }

  if(result && transcript->introns)
  {
    format_data->so_term = "intron";
    result = dump_transcript_foreach_subpart(feature, gff_string, error,
      transcript->introns, format_data);
  }

  if(result && transcript->flags.cds)
  {
    format_data->so_term = "CDS";
    g_string_append_printf(gff_string,
      "\n" GFF_OBLIGATORY_NOSCORE,
      format_data->sequence,
      format_data->source,
      format_data->so_term,
      transcript->cds_start,
      transcript->cds_end,
      '.',
      zMapGFFFormatStrand2Char(feature->strand),
      zMapGFFFormatPhase2Char(transcript->start_not_found)) ;

  result = dump_transcript_identifier(feature, transcript,
    gff_string, error, format_data);
  }

  return result;
}
*/
      /* 16.2810538-2863623      Transcript      intron  19397   20627   .       -       .       Sequence "AC005361.6-003" */
      /* Obligatory fields. */
/*
static gboolean dump_transcript_foreach_subpart(ZMapFeature feature, GString *buffer,
                                                GError **error_out, GArray *subparts,
                                                ZMapGFFFormatData format_data)
{
  gboolean result = TRUE ;
  int i ;

  for (i = 0 ; i < subparts->len && result ; i++)
    {
      ZMapSpanStruct span = g_array_index(subparts, ZMapSpanStruct, i) ;
      ZMapPhase phase = ZMAPPHASE_NONE ;


      g_string_append_printf(buffer, "\n" GFF_OBLIGATORY_NOSCORE,
        format_data->sequence,
        format_data->source,
        format_data->so_term,
        span.x1, span.x2, '.',
        zMapGFFFormatStrand2Char(feature->strand),
        zMapGFFFormatPhase2Char(phase)) ;

      result = dump_transcript_identifier(feature, &(feature->feature.transcript),
        buffer, error_out, format_data);
    }

  return result ;
}
*/


/* alignment attributes... */
  /* reading GFF2 we get Sequence or Motif getting set to DNA
    and we cannot translate back
    Ed says this will be resolved in GFF3
    but right now it's wrong
  */
/*
static gboolean dump_alignment_target(ZMapFeature feature, gpointer homol_data,
                                      GString *gff_string, GError **error,
                                      ZMapGFFFormatData format_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean has_target = TRUE;

  gchar *homol_type = NULL;


  if(feature->feature.homol.type == ZMAPHOMOL_N_HOMOL)
    homol_type = "Sequence";
  else if(feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
    homol_type = "Protein";

  if(feature->original_id && homol_type)
  {
    g_string_append_printf(gff_string,
      GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_SEPARATOR
      GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_SEPARATOR "Align %d %d %s"  GFF_ATTRIBUTE_TAB_END,
      "Class", homol_type,
      "Name", g_quark_to_string(feature->original_id),
      homol->y1, homol->y2, zMapFeatureStrand2Str(homol->strand));
  }

  return has_target;
}*/

/*
static gboolean dump_alignment_gaps(ZMapFeature feature, gpointer homol_data,
                                    GString *gff_string, GError **error,
                                    ZMapGFFFormatData format_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  GArray *gaps_array = NULL;
  int i;
  gboolean has_gaps = FALSE;

  if((gaps_array = homol->align))
  {
    has_gaps = TRUE;

    g_string_append_printf(gff_string, GFF_ATTRIBUTE_TAB_BEGIN "%s ", "Gaps");

    g_string_append_c(gff_string, '"');

    for(i = 0; i < gaps_array->len; i++)
      {
        ZMapAlignBlock block = NULL;

        block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i));

        g_string_append_printf(gff_string, "%d %d %d %d,",
        block->q1, block->q2,
        block->t1, block->t2);
      }


    g_string_truncate(gff_string, gff_string->len - 1);
    g_string_append_printf(gff_string, "%c" GFF_ATTRIBUTE_TAB_END, '"');
  }

  return has_gaps;
}
*/

/*
static gboolean dump_alignment_clone(ZMapFeature feature, gpointer homol_data,
                                     GString *gff_string, GError **error,
                                     ZMapGFFFormatData format_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean has_clone = FALSE;

  if(homol->flags.has_clone_id)
  {
    g_string_append_printf(gff_string,
      GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
      "Clone", g_quark_to_string(homol->clone_id));
    has_clone = TRUE;
  }

  return has_clone;
}

static gboolean dump_alignment_length(ZMapFeature feature, gpointer homol_data,
                                      GString *gff_string, GError **error,
                                      ZMapGFFFormatData format_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean has_length = FALSE;

  if(homol->length)
    {
      g_string_append_printf(gff_string,
        GFF_ATTRIBUTE_TAB_BEGIN "Length %d" GFF_ATTRIBUTE_TAB_END,
        homol->length);
      has_length = TRUE;
    }

  return has_length;
}
*/


