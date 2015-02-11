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

/*
 * This is used to pick up bits of data from different points
 * within the context/align/block/featureset/feature heirarchy.
 * It would be better if we could do without it of course.
 */
typedef struct ZMapGFFFormatDataStruct_
{
  const char *sequence;                 /* The sequence name. e.g. 16.12345-23456 */
  int start, end ;
  ZMapGFFAttributeFlags attribute_flags ;

  struct
    {
      unsigned int status : 1 ;
      unsigned int cont   : 1 ;
    } flags ;

} ZMapGFFFormatDataStruct ;

/*
 * A hack for sources that are input as '.',
 * and also a list of characters that are to be
 * unescaped upon output (in names and ids).
 */
static const char *anon_source = "anon_source",
    *anon_source_ab = ".",
    *reserved_allowed = " ()+-:;@/" ;

/* Functions to dump the header section of gff files */
static gboolean dump_full_header(ZMapFeatureAny feature_any, GIOChannel *file,
  ZMapGFFFormatData format_data, GError **error_out) ;
static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark key, gpointer data,
  gpointer user_data, char **err_out) ;

/* ZMapFeatureDumpFeatureFunc to dump gff. writes lines into gstring buffer... */
static gboolean dump_gff_cb(ZMapFeatureAny feature_any, GHashTable *styles,
  GString *gff_string,  GError **error, gpointer user_data);

static void deleteGFFFormatData(ZMapGFFFormatData *) ;
static ZMapGFFFormatData createGFFFormatData() ;

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
      format_data->flags.cont   = TRUE;
      format_data->flags.status = TRUE;
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
  ZMapGFFFormatData format_data = NULL ;

  zMapReturnValIfFail(dump_list && dump_list->data && error_out, result) ;
  feature_any  = (ZMapFeatureAny)(dump_list->data);
  zMapReturnValIfFail(feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID, result ) ;

  if ((format_data = createGFFFormatData()))
    result = TRUE ;

  if (result)
    {
      format_data->sequence = sequence;
      format_data->flags.cont    = TRUE;
      format_data->flags.status = TRUE;
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
                      && file
                      && error_out, FALSE ) ;
  GString *line  = NULL ;
  line = g_string_new(NULL) ;

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
  if(format_data->flags.status)
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

  if (line)
    g_string_free(line, TRUE) ;

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
                        && zMapGFFWriteFeatureBasic(feature, format_data->attribute_flags, line) ;
                }
                break;
              case ZMAPSTYLE_MODE_TRANSCRIPT:
                {
                  result = zMapGFFFormatAttributeSetTranscript(format_data->attribute_flags)
                        && zMapGFFWriteFeatureTranscript(feature, format_data->attribute_flags, line) ;
                }
                break;
              case ZMAPSTYLE_MODE_ALIGNMENT:
                {
                  result = zMapGFFFormatAttributeSetAlignment(format_data->attribute_flags)
                        && zMapGFFWriteFeatureAlignment(feature, format_data->attribute_flags, line, NULL) ;
                }
                break;
              case ZMAPSTYLE_MODE_TEXT:
                {
                  result = zMapGFFFormatAttributeSetText(format_data->attribute_flags)
                        && zMapGFFWriteFeatureText(feature, format_data->attribute_flags, line ) ;
                }
                break;
              case ZMAPSTYLE_MODE_GRAPH:
                {
                  result = zMapGFFFormatAttributeSetGraph(format_data->attribute_flags)
                        && zMapGFFWriteFeatureGraph(feature, format_data->attribute_flags, line ) ;
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
      attribute_flags->variation   = TRUE ;
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
      attribute_flags->locus       = TRUE ;
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


gboolean zMapGFFWriteFeatureAlignment(ZMapFeature feature, ZMapGFFAttributeFlags flags,
                                      GString * line, const char *seq_str)
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *feature_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  char *escaped_string = NULL ;
  GString *attribute = NULL ;
  ZMapStrand strand = ZMAPSTRAND_NONE,
    s_strand = ZMAPSTRAND_NONE ;
  float percent_id = (float) 0.0 ;
  GArray *gaps = NULL ;

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
  if (flags->target && feature_name && *feature_name)
    {
      ZMapStrand s_strand = feature->feature.homol.strand ;
      int sstart = feature->feature.homol.y1,
        send = feature->feature.homol.y2 ;
      escaped_string = g_uri_escape_string(feature_name, reserved_allowed, FALSE  ) ;
      if (escaped_string)
        {
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
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_string_truncate(attribute, (gsize)0) ;
          g_free(escaped_string) ;
        }
    }

  /*
   * percentID attribute
   */
  if (flags->percent_id)
    {
      percent_id = feature->feature.homol.percent_id ;
      if (percent_id != (float)0.0)
        {
          g_string_printf(attribute, "percentID=%g", percent_id) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_string_truncate(attribute, (gsize)0) ;
        }
    }

  /*
   * Gap attribute (gffv3)
   */
  if (flags->gap)
    {
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
    }

  /*
   * Sequence attribute
   */
  if (flags->sequence && seq_str && *seq_str)
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
gboolean zMapGFFWriteFeatureTranscript(ZMapFeature feature, ZMapGFFAttributeFlags flags, GString *line)
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
  char *escaped_string = NULL ;
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
  feature_name = g_quark_to_string(feature->original_id) ;
  type = g_quark_to_string(feature->SO_accession) ;
  escaped_string = g_uri_escape_string(feature_name, reserved_allowed, FALSE ) ;

  /*
   * Mandatory fields.
   */
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, type,
                         feature->x1, feature->x2,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE ) ;

  /*
   * ID attribute
   */
  if (flags->id && escaped_string)
    {
      g_string_printf(attribute, "ID=%s", escaped_string) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * Name attribute
   */
  if (flags->name && escaped_string)
    {
      g_string_printf(attribute, "Name=%s", escaped_string) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, FALSE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * End of line for parent object.
   */
  g_string_append_c(line, '\n');

  /*
   * Write a line for each exon and each CDS if present with
   * the ID of the previous line as Parent attribute.
   */
  if (flags->parent && escaped_string)
    {
      g_string_printf(attribute, "Parent=%s", escaped_string) ;
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
                } /* if the feature has a CDS */
            } /* if the exon span is valid */
        } /* loop over exons */
    }

  if (attribute)
    g_string_free(attribute, TRUE) ;
  if (escaped_string)
    g_free(escaped_string) ;

  return status ;
}


gboolean zMapGFFWriteFeatureText(ZMapFeature feature, ZMapGFFAttributeFlags flags, GString* line)
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  char *string_escaped = NULL ;
  GString * attribute = NULL ;
  GQuark gqID = 0 ;

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
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, type,
                         feature->x1, feature->x2,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE) ;

  /*
   * Name attribute
   */
  if (flags->name)
    {
      string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id),
                                           reserved_allowed, FALSE) ;
      if (string_escaped)
        {
          g_string_printf(attribute, "Name=%s", string_escaped) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_free(string_escaped) ;
          string_escaped = NULL ;
          g_string_truncate(attribute, (gsize)0) ;
        }
    }

  /*
   * Note attribute
   */
  if (flags->note && feature->description)
    {
      string_escaped = g_uri_escape_string(feature->description,
                                           reserved_allowed, FALSE) ;
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
  if (flags->locus && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      gqID = feature->feature.transcript.locus_id ;
      if (gqID)
        {
          string_escaped = g_uri_escape_string(g_quark_to_string(gqID),
                                                reserved_allowed, FALSE ) ;
          if (string_escaped)
            {
              g_string_printf(attribute, "locus=%s", string_escaped) ;
              zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
              g_string_truncate(attribute, (gsize)0) ;
              g_free(string_escaped) ;
              string_escaped = NULL ;
            }
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
 * Output of a basic feature.
 */
gboolean zMapGFFWriteFeatureBasic(ZMapFeature feature, ZMapGFFAttributeFlags flags, GString* line )
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  char *string_escaped = NULL ;
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
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, type,
                         feature->x1, feature->x2,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE) ;

  /*
   * Name attribute
   */
  if (flags->name)
    {
      string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id),
                                           reserved_allowed, FALSE) ;
      if (string_escaped)
        {
          g_string_printf(attribute, "Name=%s", string_escaped) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_free(string_escaped) ;
          string_escaped = NULL ;
          g_string_truncate(attribute, (gsize)0) ;
        }
    }

  /*
   * URL attribute
   */
  if (flags->url && feature->url)
    {
      string_escaped = g_uri_escape_string(feature->url,
                                           NULL, FALSE) ;
      if (string_escaped)
        {
          g_string_printf(attribute, "url=%s", string_escaped) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_string_truncate(attribute, (gsize)0) ;
          g_free(string_escaped) ;
          string_escaped = NULL ;
        }
    }

  /*
   * Variation string attribute.
   */
  if (flags->variation && feature->feature.basic.flags.variation_str)
    {
      g_string_printf(attribute, "variant_sequence=%s", feature->feature.basic.variation_str) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * Note attribute
   */
  if (flags->note && feature->description)
    {
      string_escaped = g_uri_escape_string(feature->description,
                                           reserved_allowed, FALSE) ;
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
   * End of line.
   */
  g_string_append_c(line, '\n') ;
  g_string_free(attribute, TRUE) ;

  return status ;
}




/*
 * Output of a graph feature.
 */
gboolean zMapGFFWriteFeatureGraph(ZMapFeature feature, ZMapGFFAttributeFlags flags, GString* line )
{
  gboolean status = FALSE ;
  const char *sequence_name = NULL,
    *source_name = NULL,
    *type = NULL ;
  char *string_escaped = NULL ;
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
  zMapGFFFormatMandatory(TRUE, line,
                         sequence_name, source_name, type,
                         feature->x1, feature->x2,
                         feature->score, feature->strand, ZMAPPHASE_NONE,
                         (gboolean)feature->flags.has_score, TRUE) ;

  /*
   * Name attribute
   */
  if (flags->name)
    {
      string_escaped = g_uri_escape_string(g_quark_to_string(feature->original_id),
                                           reserved_allowed, FALSE) ;
      if (string_escaped)
        {
          g_string_printf(attribute, "Name=%s", string_escaped) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_free(string_escaped) ;
          string_escaped = NULL ;
          g_string_truncate(attribute, (gsize)0) ;
        }
    }

  /*
   * URL attribute
   */
  if (flags->url && feature->url)
    {
      string_escaped = g_uri_escape_string(feature->url,
                                           NULL, FALSE) ;
      if (string_escaped)
        {
          g_string_printf(attribute, "url=%s", string_escaped) ;
          zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
          g_string_truncate(attribute, (gsize)0) ;
          g_free(string_escaped) ;
          string_escaped = NULL ;
        }
    }

  /*
   * Variation string attribute.
   */
  if (flags->variation && feature->feature.basic.flags.variation_str)
    {
      g_string_printf(attribute, "variant_sequence=%s", feature->feature.basic.variation_str) ;
      zMapGFFFormatAppendAttribute(line, attribute, FALSE, TRUE) ;
      g_string_truncate(attribute, (gsize)0) ;
    }

  /*
   * Note attribute
   */
  if (flags->note && feature->description)
    {
      string_escaped = g_uri_escape_string(feature->description,
                                           reserved_allowed, FALSE) ;
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
   * End of line.
   */
  g_string_append_c(line, '\n') ;
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
