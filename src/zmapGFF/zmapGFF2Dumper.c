/*  File: zmapGFF2Dumper.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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


/*
 * GFF version to output. Can be set to either
 * [ZMAPGFF_VERSION_2, ZMAPGFF_VERSION_3] using the
 * external interface function 'zMapGFFDumpVersion()'.
 */
static ZMapGFFVersion gff_output_version = ZMAPGFF_VERSION_3 ;


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
#define GFF_ATTRIBUTE_SEPARATOR ";"

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
#define GFF_SEP_SCORE GFF_SEPARATOR GFF_SCORE
#define GFF_SEP_NOSCORE GFF_SEPARATOR "%c"
#define GFF_SEP_STRAND GFF_SEPARATOR GFF_STRAND
#define GFF_SEP_PHASE GFF_SEPARATOR GFF_PHASE
#define GFF_SEP_ATTRIBUTES GFF_SEPARATOR GFF_ATTRIBUTES
#define GFF_OBLIGATORY GFF_SEQ_SOURCE_FEAT_START_END GFF_SEP_SCORE GFF_STRAND_FRAME
#define GFF_OBLIGATORY_NOSCORE GFF_SEQ_SOURCE_FEAT_START_END GFF_SEP_NOSCORE GFF_SEP_STRAND GFF_SEP_PHASE
#define GFF_FULL_LINE GFF_SEQ_SOURCE_FEAT_START_END GFF_SEP_SCORE GFF_STRAND_FRAME GFF_SEP_ATTRIBUTES

typedef struct _GFFDumpDataStruct *GFFDumpData;

/* annoyingly we need to use a gpointer for 2nd argument. */
typedef gboolean (* DumpGFFAttrFunc)(ZMapFeature feature, gpointer feature_data,
     GString *gff_string, GError **error_out,
     GFFDumpData gff_data);


typedef struct
{
  GString *header_string;/* string to use as buffer. */
  const char *gff_sequence;/* can be shared between header and dump data structs in case dumping list. */
  gboolean status, cont;/* simple status and continuation flags */
} GFFHeaderDataStruct, *GFFHeaderData ;

/* Fields are: <sequence> <source> <feature> <start> <end> <score> <strand> <frame> <attributes> */
typedef struct _GFFDumpDataStruct
{
  GHashTable *styles ;

  const char *gff_sequence;/* The sequence name. e.g. 16.12345-23456 */
  char *gff_source;/* The source e.g. augustus, TrEMBL, trf, polya_site */
  char *gff_feature;/* The feature type e.g. Sequence, intron, exon, CDS, coding_exon, misc_feature */

  const char *feature_name;/* The feature name e.g. AC12345.1-001 */
  char *link_term;/* The attribute key for the parent feature name. e.g. Sequence in Sequence "AC12345.1-001" */

  /* functions for doing attributes. */
  DumpGFFAttrFunc *basic;
  DumpGFFAttrFunc *transcript;
  DumpGFFAttrFunc *homol;
  DumpGFFAttrFunc *text;
} GFFDumpDataStruct;

/* Functions to dump the header section of gff files */
static gboolean dump_full_header(ZMapFeatureAny feature_any, GIOChannel *file,
                                 const char **sequence_in_out, GString **header_string_out, GError **error_out) ;
static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark key, gpointer data,
  gpointer user_data, char **err_out) ;

/* Functions to dump the body of the data */

/* ZMapFeatureDumpFeatureFunc to dump gff. writes lines into gstring buffer... */
static gboolean dump_gff_cb(ZMapFeatureAny feature_any, GHashTable *styles,
  GString *gff_string, GError **error, gpointer user_data);

/*
 * dump attributes according to the format defined in the array of function pointers passed in.
 */
static gboolean dump_attributes(DumpGFFAttrFunc func_array[], ZMapFeature feature,
  gpointer feature_data, GString *gff_string, GError **error, GFFDumpData gff_data);

static gboolean dump_text_note(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, GFFDumpData gff_data);

/* transcripts */
static gboolean dump_transcript_identifier(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, GFFDumpData gff_data);

static gboolean dump_transcript_locus(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, GFFDumpData gff_data);

/*
 * V3 stuff
 */
static gboolean dump_text_note_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, GFFDumpData gff_data) ;
static gboolean dump_transcript_locus_v3(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, GFFDumpData gff_data);
static gboolean dump_id_as_name_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, GFFDumpData gff_data) ;
static gboolean dump_transcript_id_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, GFFDumpData gff_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* See Malcolms comment later.... */
static gboolean dump_known_name(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, GFFDumpData gff_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* transcript subparts as lines... */
static gboolean dump_transcript_subpart_lines(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, GFFDumpData gff_data);


static gboolean dump_transcript_foreach_subpart(ZMapFeature feature, GString *buffer,
  GError **error_out, GArray *subparts, GFFDumpData gff_data);


/*
 * V3 versions of the above.
 */
static gboolean dump_transcript_subpart_v3(ZMapFeature feature, gpointer transcript_data,
  GString *gff_string, GError **error, GFFDumpData gff_data) ;
static gboolean dump_transcript_foreach_subpart_v3(ZMapFeature feature, GString *buffer,
  GError **error_out, GArray *subparts, GFFDumpData gff_data) ;


/* alignments */

static gboolean dump_alignment_target(ZMapFeature feature, gpointer homol,
  GString *gff_string, GError **error, GFFDumpData gff_data);
static gboolean dump_alignment_gaps(ZMapFeature feature, gpointer homol,
  GString *gff_string, GError **error, GFFDumpData gff_data);
static gboolean dump_alignment_clone(ZMapFeature feature, gpointer homol,
  GString *gff_string, GError **error, GFFDumpData gff_data);
static gboolean dump_alignment_length(ZMapFeature feature, gpointer homol,
  GString *gff_string, GError **error, GFFDumpData gff_data);

/*
 * V3 versions of these
 */
static gboolean dump_alignment_target_v3(ZMapFeature feature, gpointer homol,
  GString *gff_string, GError **error, GFFDumpData gff_data) ;
static gboolean dump_alignment_gaps_v3(ZMapFeature feature, gpointer homol_data,
  GString *gff_string, GError **error, GFFDumpData gff_data) ;

/* utils */
static char strand2Char(ZMapStrand strand) ;
static char phase2Char(ZMapPhase phase) ;



/*
 * These arrays of function pointers are used to obtain
 * format and output various attributes of different types
 * of features. They depend not only on the style mode of
 * the features themselves (BASIC, TRANSCRIPT, ALIGNMENT, etc)
 * but also on GFF version. There are seperate sets of
 * tables for v2 and v3.
 */

static DumpGFFAttrFunc basic_funcs_G_GFF2[] = {
  dump_text_note,/* Note "cpg island" */
/*  dump_known_name,*/
  NULL,
};
static DumpGFFAttrFunc transcript_funcs_G_GFF2[] = {
  dump_transcript_identifier, /* Sequence "AC12345.1-001" */
  dump_transcript_locus,/* RPC1-2 */
/*  dump_known_name,*/
  dump_transcript_subpart_lines, /* Not really attributes like alignment subparts are. */
  NULL
};

static DumpGFFAttrFunc homol_funcs_G_GFF2[] = {
  dump_alignment_target,/* Target "Sw:Q12345.1" 101 909 */
  dump_alignment_clone,/* Clone "AC12345.1" */
  dump_alignment_gaps,/* Gaps "1234 1245 4567 4578, 2345 2356 5678 5689" */
  dump_alignment_length,/* Length 789 */
  NULL
};
static DumpGFFAttrFunc text_funcs_G_GFF2[] = {
  dump_text_note,
  dump_transcript_locus,
  NULL
};


static DumpGFFAttrFunc basic_funcs_G_GFF3[] = {
  dump_text_note_v3, /* Note = <unquoted_string> */
  dump_id_as_name_v3, /* Name = <unquoted_string> */
  NULL,
};
static DumpGFFAttrFunc transcript_funcs_G_GFF3[] = {
  dump_transcript_id_v3,
  dump_id_as_name_v3,
  dump_transcript_subpart_v3,
  NULL
};
static DumpGFFAttrFunc homol_funcs_G_GFF3[] = {
  dump_alignment_target_v3,/* Target=<clone_id> <start> <end> <strand> */
  dump_alignment_gaps_v3,/* Gaps=(<int><int><int><int>)+ */
  NULL
};
static DumpGFFAttrFunc text_funcs_G_GFF3[] = {
  dump_text_note_v3,
  dump_transcript_locus_v3,
  NULL
};

/*
 * Public interface function to set the version of GFF to output.
 */
gboolean zMapGFFDumpVersionSet(ZMapGFFVersion gff_version )
{
  gboolean result = FALSE ;

  zMapReturnValIfFail( (gff_version == ZMAPGFF_VERSION_2) || (gff_version == ZMAPGFF_VERSION_3), result) ;
  gff_output_version = gff_version ;

  return result ;
}

/*
 * Return the version of GFF that has been set for output.
 */
ZMapGFFVersion zMapGFFDumpVersionGet()
{
  return gff_output_version ;
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
  const char *sequence = NULL;
  gboolean result = FALSE ;

  zMapReturnValIfFail(file && dump_set && error_out, result) ;


  if ((dump_set->struct_type != ZMAPFEATURE_STRUCT_CONTEXT)
     && (dump_set->struct_type != ZMAPFEATURE_STRUCT_ALIGN)
     && (dump_set->struct_type != ZMAPFEATURE_STRUCT_BLOCK)
     && (dump_set->struct_type != ZMAPFEATURE_STRUCT_FEATURESET)
     && (dump_set->struct_type != ZMAPFEATURE_STRUCT_FEATURE) )
    return result ;

  result = dump_full_header(dump_set, file, &sequence, NULL, error_out) ;

  if (result)
    {
      GFFDumpDataStruct gff_data = {NULL};
      gff_data.basic      = NULL ;
      gff_data.transcript = NULL ;
      gff_data.homol      = NULL ;
      gff_data.text       = NULL ;
      gff_data.styles     = NULL ;

      if (gff_output_version == ZMAPGFF_VERSION_2)
        {
          gff_data.basic      = basic_funcs_G_GFF2;
          gff_data.transcript = transcript_funcs_G_GFF2;
          gff_data.homol      = homol_funcs_G_GFF2;
          gff_data.text       = text_funcs_G_GFF2;
          gff_data.styles     = styles ;
        }
      else if (gff_output_version == ZMAPGFF_VERSION_3)
        {
          gff_data.basic      = basic_funcs_G_GFF3;
          gff_data.transcript = transcript_funcs_G_GFF3;
          gff_data.homol      = homol_funcs_G_GFF3;
          gff_data.text       = text_funcs_G_GFF3;
          gff_data.styles     = styles ;
        }

      /* This might get overwritten later, but as DumpToFile uses
       * Subset, there's a chance it wouldn't get set at all */
      gff_data.gff_sequence = sequence;
      if(region_span)
        result = zMapFeatureContextRangeDumpToFile((ZMapFeatureAny)dump_set, styles, region_span,
                   dump_gff_cb, &gff_data, file, error_out) ;
      else
        result = zMapFeatureContextDumpToFile((ZMapFeatureAny)dump_set, styles, dump_gff_cb,
                   &gff_data, file, error_out) ;
    }

  return result ;
}

/*!
 * \brief Dump a list of ZMapFeatureAny to a file and/or text buffer. sequence can be NULL
 */
gboolean zMapGFFDumpList(GList *dump_list, GHashTable *styles, char *sequence, GIOChannel *file, GString **text_out, GError **error_out)
{
  const char *int_sequence = NULL;
  gboolean result = FALSE ;
  GString *header_string = NULL ;
  ZMapFeatureAny feature_any = NULL ;

  zMapReturnValIfFail(dump_list && dump_list->data && error_out, result) ;
  feature_any  = (ZMapFeatureAny)(dump_list->data);
  zMapReturnValIfFail(feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID, result ) ;
  int_sequence = sequence;
  result       = dump_full_header(feature_any, file, &int_sequence, &header_string, error_out) ;

  if (result)
    {
      GFFDumpDataStruct gff_data = {NULL};
      gff_data.basic      = NULL ;
      gff_data.transcript = NULL ;
      gff_data.homol      = NULL ;
      gff_data.text       = NULL ;
      gff_data.styles     = NULL ;

      if (gff_output_version == ZMAPGFF_VERSION_2)
        {
          gff_data.basic      = basic_funcs_G_GFF2;
          gff_data.transcript = transcript_funcs_G_GFF2;
          gff_data.homol      = homol_funcs_G_GFF2;
          gff_data.text       = text_funcs_G_GFF2;
          gff_data.styles     = styles ;
        }
      else if (gff_output_version == ZMAPGFF_VERSION_3)
        {
          gff_data.basic      = basic_funcs_G_GFF3;
          gff_data.transcript = transcript_funcs_G_GFF3;
          gff_data.homol      = homol_funcs_G_GFF3;
          gff_data.text       = text_funcs_G_GFF3;
          gff_data.styles     = styles ;
        }

      /* This might get overwritten later, but as DumpToFile uses
       * Subset, there's a chance it wouldn't get set at all */
      gff_data.gff_sequence = int_sequence;

      result = zMapFeatureListDumpToFileOrBuffer(dump_list, styles, dump_gff_cb, &gff_data, file, text_out, error_out) ;

      if (text_out && header_string && header_string->str)
        g_string_prepend(*text_out, header_string->str) ;
    }

  return result ;
}

gboolean zMapGFFDumpForeachList(ZMapFeatureAny first_feature, GHashTable *styles,
                                GIOChannel *file, GError **error_out,
                                char *sequence, GFunc *list_func_out,
                                gpointer *list_data_out)
{
  gboolean result = FALSE ;
  const char *int_sequence = NULL ;

  zMapReturnValIfFail(first_feature &&
                      (first_feature->struct_type != ZMAPFEATURE_STRUCT_INVALID) &&
                      error_out &&
                      list_func_out &&
                      list_data_out,
                                     result ) ;

  int_sequence = sequence;

  if ((result = dump_full_header(first_feature, file, &int_sequence, NULL, error_out)))
    {
      GFFDumpData gff_data ;

      gff_data = g_new0(GFFDumpDataStruct, 1);
      gff_data->basic      = NULL ;
      gff_data->transcript = NULL ;
      gff_data->homol      = NULL ;
      gff_data->text       = NULL ;
      gff_data->styles     = NULL ;

      if (gff_output_version == ZMAPGFF_VERSION_2)
        {
          gff_data->basic      = basic_funcs_G_GFF2;
          gff_data->transcript = transcript_funcs_G_GFF2;
          gff_data->homol      = homol_funcs_G_GFF2;
          gff_data->text       = text_funcs_G_GFF2;
          gff_data->styles     = styles ;
        }
      else if (gff_output_version == ZMAPGFF_VERSION_3)
        {
          gff_data->basic      = basic_funcs_G_GFF3;
          gff_data->transcript = transcript_funcs_G_GFF3;
          gff_data->homol      = homol_funcs_G_GFF3;
          gff_data->text       = text_funcs_G_GFF3;
          gff_data->styles     = styles ;
        }

      /* This might get overwritten later, but as DumpToFile uses
       * Subset, there's a chance it wouldn't get set at all */
      gff_data->gff_sequence = int_sequence;

      result = zMapFeatureListForeachDumperCreate(dump_gff_cb, styles, gff_data,
                 g_free, file, error_out, list_func_out, list_data_out) ;
    }

  return result ;
}

/* INTERNALS */

static gboolean dump_full_header(ZMapFeatureAny feature_any, GIOChannel *file,
                                 const char **sequence_in_out, GString **header_string_out,
                                 GError **error_out)
{
  GFFHeaderDataStruct header_data = {NULL};
  char *time_string ;

  zMapReturnValIfFail(feature_any && (feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID) && file && error_out, FALSE ) ;

  /* Dump the standard header lines. */
  time_string = zMapGetTimeString(ZMAPTIME_YMD, NULL) ;

  header_data.header_string = g_string_sized_new(1024); /* plenty enough */
  header_data.cont   = TRUE;
  header_data.status = TRUE;

  if(sequence_in_out && *sequence_in_out)
    header_data.gff_sequence = *sequence_in_out;

  /* gff-version, source-version, date */
  g_string_append_printf(header_data.header_string,
    "##gff-version %d\n"
    "##source-version %s %s\n"
    "##date %s\n",
    (int)gff_output_version,
    zMapGetAppName(), zMapGetAppVersionString(),
    time_string) ;

  g_free(time_string) ;

  zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_BLOCK,
                            get_type_seq_header_cb, &header_data);

  if(header_data.status)
    {
      if (file)
        {
          GIOStatus write_status ;
          gsize bytes_written ;

          write_status = g_io_channel_write_chars(file, header_data.header_string->str, header_data.header_string->len, &bytes_written, error_out) ;
          if (write_status != G_IO_STATUS_NORMAL)
            {
              header_data.status = FALSE;
            }
        }

      if(sequence_in_out)
        *sequence_in_out = header_data.gff_sequence;

      if (header_string_out)
        *header_string_out = header_data.header_string ;
      else
        g_string_free(header_data.header_string, TRUE);
    }
  else
    {
      g_string_free(header_data.header_string, TRUE);
    }

  return header_data.status;
}

static ZMapFeatureContextExecuteStatus get_type_seq_header_cb(GQuark  key, gpointer data,
                                                              gpointer user_data, char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  GFFHeaderData header_data  = (GFFHeaderData)user_data;

  zMapReturnValIfFail(data && user_data && (feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID), status ) ;

  switch(feature_any->struct_type)
    {
      case ZMAPFEATURE_STRUCT_CONTEXT:
        break;
      case ZMAPFEATURE_STRUCT_ALIGN:
        if(header_data->status && header_data->cont)
          {
            ZMapFeatureAlignment feature_align = (ZMapFeatureAlignment)feature_any;
            const char *sequence;
            header_data->status = header_data->cont = TRUE;

            sequence = g_quark_to_string(feature_any->original_id);

            g_string_append_printf(header_data->header_string, "##Type DNA %s %d %d\n",
              sequence, feature_align->sequence_span.x1, feature_align->sequence_span.x2);

            if(!header_data->gff_sequence)
              header_data->gff_sequence = sequence;
            status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
          }
        break;
      case ZMAPFEATURE_STRUCT_BLOCK:
        if(header_data->status && header_data->cont)
          {
            ZMapFeatureBlock feature_block = (ZMapFeatureBlock)feature_any;
            int start, end;
            gboolean reversed = FALSE;

            start = feature_block->block_to_sequence.block.x1;
            end   = feature_block->block_to_sequence.block.x2;

            if (feature_block->block_to_sequence.reversed)
              reversed = TRUE ;

            g_string_append_printf(header_data->header_string, "##sequence-region %s %d %d %s\n",
              g_quark_to_string(feature_any->original_id), start, end,(reversed ? "(reversed)" : "")) ;
            header_data->status = TRUE;
            header_data->cont   = FALSE;
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

static gboolean dump_gff_cb(ZMapFeatureAny feature_any,
    GHashTable         *styles,
    GString       *gff_string,
    GError       **error,
    gpointer       user_data)
{
  GFFDumpData gff_data = (GFFDumpData)user_data;
  gboolean result = TRUE;
  ZMapFeatureSet fset;

  zMapReturnValIfFail(feature_any && (feature_any->struct_type != ZMAPFEATURE_STRUCT_INVALID) && gff_data, result ) ;

  switch(feature_any->struct_type)
    {
      case ZMAPFEATURE_STRUCT_ALIGN:
        gff_data->gff_sequence = g_quark_to_string(feature_any->original_id);
      case ZMAPFEATURE_STRUCT_CONTEXT:
      case ZMAPFEATURE_STRUCT_BLOCK:
        result = TRUE;
        /* We don't dump these to gff */
        break;
      case ZMAPFEATURE_STRUCT_FEATURESET:
        /* we need to do feature filter at this level, otherwise it'll slow stuff down... */
        result = TRUE;
        break;
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          ZMapFeature feature = (ZMapFeature)feature_any;
          ZMapFeatureTypeStyle style ;

          style = *feature->style;       /*zMapFindStyle(gff_data->styles, feature->style_id) ;*/


          /* Output a GFFv2 record for the whole feature, fields are:
           *
           * <seqname> <source> <feature> <start> <end> <score> <strand> <phase> [attributes]
           *
           * i.e. all but [attributes] are mandatory.
           *
           * Firstly, the mandatory fields.
           */

#if MH17_DONT_USE_STYLE_FOR_FEATURESET_NAME
          if (!(gff_data->gff_source  = (char *)zMapStyleGetGFFSource(style)))
          gff_data->gff_source = (char *)zMapStyleGetName(style) ;
#else
          fset = (ZMapFeatureSet)(feature_any->parent);

          /* this is made up data: do not dump */
          if(fset->original_id == g_quark_from_string("Locus"))
            break;
          /* this is made up data: do not dump */
          if(fset->original_id == g_quark_from_string("Show Translation"))
            break;
          /* this is made up data: do not dump */
          if(fset->original_id == g_quark_from_string("3 Frame Translation"))
            break;
          /* this is made up data: do not dump */
          if(fset->original_id == g_quark_from_string("Annotation"))
            break;
         /* this is made up data: do not dump */
         if(fset->original_id == g_quark_from_string("ORF"))
            break;
         /* better to dump DNA to a FASTA file */
         if(fset->original_id == g_quark_from_string("DNA"))
            break;

          gff_data->gff_source = (char *) g_quark_to_string(fset->original_id);
#endif

          gff_data->gff_feature = (char *) g_quark_to_string(feature->SO_accession) ; /* zMapSOAcc2Term(feature->SO_accession) ; */
          if(!gff_data->gff_feature)
            {
              GQuark gff_ontology;

              gff_ontology = zMapStyleGetGFFFeature(style);
              gff_data->gff_feature = (char *) g_quark_to_string(gff_ontology);
            }

          g_string_append_printf(gff_string,
            GFF_SEQ_SOURCE_FEAT_START_END,
            gff_data->gff_sequence, /* saves feature->parent->parent->parent->original_id */
            gff_data->gff_source,
            gff_data->gff_feature,
            feature->x1,
            feature->x2) ;

          if (feature->flags.has_score)
            g_string_append_printf(gff_string, GFF_SEP_SCORE, feature->score) ;
          else
            g_string_append_printf(gff_string, GFF_SEP_NOSCORE, '.') ;

          g_string_append_printf(gff_string, GFF_SEP_STRAND, strand2Char(feature->strand)) ;

          g_string_append_printf(gff_string, GFF_SEP_PHASE,
              ((feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.flags.cds)
                  ? phase2Char(feature->feature.transcript.start_not_found)
                : '.')) ;

          /* Now to the attribute fields, and any subparts... */

          switch(feature->mode)
            {
              case ZMAPSTYLE_MODE_BASIC:
                {
                  result = dump_attributes(gff_data->basic, feature,
                    &(feature->feature.basic),
                    gff_string, error, gff_data);
                }
                break;
              case ZMAPSTYLE_MODE_TRANSCRIPT:
                {
                  /* Do transcript attributes, then each of the sub features need processing */
                  gff_data->feature_name = g_quark_to_string(feature->original_id);
                  gff_data->link_term    = "Sequence";
                  gff_data->link_term    = gff_data->gff_feature;

                  result = dump_attributes(gff_data->transcript, feature,
                    &(feature->feature.transcript),
                    gff_string, error, gff_data);

                }
                break;
              case ZMAPSTYLE_MODE_ALIGNMENT:
                {
                  result = dump_attributes(gff_data->homol, feature,
                    &(feature->feature.homol),
                    gff_string, error, gff_data);
                }
                break;
              case ZMAPSTYLE_MODE_TEXT:
                {
                  result = dump_attributes(gff_data->text, feature,
                    NULL, /* this needs to be &(feature->feature.text) */
                    gff_string, error, gff_data);
                }
                break;
              default:
                /* This might be a little odd... */
                break;
            }

            g_string_append_c(gff_string, '\n');
          }
        break;
      default:
        result = FALSE;
        break;
    }

  return result;
}

/* attribute dumper. run through the dump functions separating the
 * attributes with GFF_ATTRIBUTE_SEPARATOR */

static gboolean dump_attributes(DumpGFFAttrFunc func_array[], ZMapFeature feature,
                                gpointer feature_data, GString *gff_string,
                                GError **error, GFFDumpData gff_data)
{
  DumpGFFAttrFunc *funcs_ptr;
  gboolean result = TRUE, need_separator = FALSE;

  funcs_ptr = func_array;

  if (gff_output_version == ZMAPGFF_VERSION_3)
    {
      g_string_append(gff_string, GFF_ATTRIBUTE_TAB_BEGIN ) ;
    }

  while(funcs_ptr && *funcs_ptr)
    {
      if(need_separator)
        {
          g_string_append(gff_string, GFF_ATTRIBUTE_SEPARATOR);
          need_separator = FALSE;
        }

      if((*funcs_ptr)(feature, feature_data, gff_string, error, gff_data))
        need_separator = TRUE;

      funcs_ptr++;
    }

  return result;
}

static gboolean dump_text_note(ZMapFeature feature, gpointer basic_data, GString *gff_string, GError **error, GFFDumpData gff_data)
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




static gboolean dump_text_note_v3(ZMapFeature feature, gpointer basic_data, GString *gff_string, GError **error, GFFDumpData gff_data)
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
                                      GFFDumpData gff_data)
{
  gboolean result = FALSE;

  if (feature->feature.transcript.locus_id)
  {
    /* To make       Locus "AC12345.1"          */
    g_string_append_printf(gff_string,
      GFF_UNQUOTED_ATTRIBUTE,
      "locus", (char *)g_quark_to_string(feature->feature.transcript.locus_id));
    result = TRUE;
  }

return result;
}


static gboolean dump_id_as_name_v3(ZMapFeature feature, gpointer basic_data,
  GString *gff_string, GError **error, GFFDumpData gff_data)
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
  GString *gff_string, GError **error, GFFDumpData gff_data)
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
  GString *gff_string, GError **error, GFFDumpData gff_data)
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









/* transcript calls */

/* attributes */
static gboolean dump_transcript_identifier(ZMapFeature feature, gpointer transcript_data,
                                           GString *gff_string, GError **error,
                                           GFFDumpData gff_data)
{
  gboolean result = TRUE;

  /* To make       Sequence "AC12345.1"          */
  g_string_append_printf(gff_string,
    GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
    gff_data->link_term,
    gff_data->feature_name);

  return result;
}

static gboolean dump_transcript_locus(ZMapFeature feature, gpointer transcript_data,
                                      GString *gff_string, GError **error,
                                      GFFDumpData gff_data)
{
  gboolean result = FALSE;

  if (feature->feature.transcript.locus_id)
    {
      /* To make       Locus "AC12345.1"          */
      g_string_append_printf(gff_string,
        GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
        "Locus",
        (char *)g_quark_to_string(feature->feature.transcript.locus_id));
      result = TRUE;
    }

  return result;
}

/* this generates an erro about phase...*/
#if MH17_DONT_USE
static gboolean dump_known_name(ZMapFeature feature, gpointer transcript_data, GString *gff_string, GError **error, GFFDumpData gff_data)
{
  gboolean result = FALSE;
  GQuark known_id = 0;

  if (feature->mode == ZMAPSTYLE_MODE_BASIC)
    known_id = feature->feature.basic.known_name;
  else if(feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    known_id = feature->feature.transcript.known_name;

  if(known_id)
    {
      /* To make       Locus "AC12345.1"          */
      g_string_append_printf(gff_string,
        GFF_ATTRIBUTE_TAB_BEGIN GFF_QUOTED_ATTRIBUTE GFF_ATTRIBUTE_TAB_END,
        "Known_name",
        (char *)g_quark_to_string(known_id));
      result = TRUE;
    }

  return result;
}
#endif





/* subpart full lines... */
static gboolean dump_transcript_subpart_v3(ZMapFeature feature, gpointer transcript_data,
                                              GString *gff_string, GError **error,
                                              GFFDumpData gff_data)
{
  ZMapTranscript transcript = (ZMapTranscript)transcript_data;
  gboolean result = TRUE;

  if(result && transcript->exons)
  {
    gff_data->gff_feature = "exon";
    result = dump_transcript_foreach_subpart_v3(feature, gff_string, error,
      transcript->exons, gff_data);
  }

  if(result && transcript->introns)
  {
    gff_data->gff_feature = "intron";
    result = dump_transcript_foreach_subpart_v3(feature, gff_string, error,
      transcript->introns, gff_data);
  }

  if(result && transcript->flags.cds)
  {
    gff_data->gff_feature = "CDS";
    g_string_append_printf(gff_string,
      "\n" GFF_OBLIGATORY_NOSCORE,
      gff_data->gff_sequence,
      gff_data->gff_source,
      gff_data->gff_feature,
      transcript->cds_start,
      transcript->cds_end,
      '.',
      strand2Char(feature->strand),
      phase2Char(transcript->start_not_found)) ;
    g_string_append_printf(gff_string, "\t") ;
    result = dump_transcript_parent_v3(feature, transcript,
      gff_string, error, gff_data);
  }

  return result;
}


static gboolean dump_transcript_foreach_subpart_v3(ZMapFeature feature, GString *buffer,
  GError **error_out, GArray *subparts, GFFDumpData gff_data)
{
  gboolean result = TRUE ;
  int i ;

  for (i = 0 ; i < subparts->len && result ; i++)
  {
    ZMapSpanStruct span = g_array_index(subparts, ZMapSpanStruct, i) ;
    ZMapPhase phase = ZMAPPHASE_NONE ;

    g_string_append_printf(buffer, "\n" GFF_OBLIGATORY_NOSCORE,
      gff_data->gff_sequence,
      gff_data->gff_source,
      gff_data->gff_feature,
      span.x1, span.x2,
      '.', /* no score */
      strand2Char(feature->strand),
      phase2Char(phase)) ;
    g_string_append_printf(buffer, "\t") ;

    result = dump_transcript_parent_v3(feature, &(feature->feature.transcript),
      buffer, error_out, gff_data);
  }

  return result ;
}




/* subpart full lines... */
static gboolean dump_transcript_subpart_lines(ZMapFeature feature, gpointer transcript_data,
                                              GString *gff_string, GError **error,
                                              GFFDumpData gff_data)
{
  ZMapTranscript transcript = (ZMapTranscript)transcript_data;
  gboolean result = TRUE;

  if(result && transcript->exons)
  {
    gff_data->gff_feature = "exon";
    result = dump_transcript_foreach_subpart(feature, gff_string, error,
      transcript->exons, gff_data);
  }

  if(result && transcript->introns)
  {
    gff_data->gff_feature = "intron";
    result = dump_transcript_foreach_subpart(feature, gff_string, error,
      transcript->introns, gff_data);
  }

  if(result && transcript->flags.cds)
  {
    gff_data->gff_feature = "CDS";
    g_string_append_printf(gff_string,
      "\n" GFF_OBLIGATORY_NOSCORE,
      gff_data->gff_sequence,
      gff_data->gff_source,
      gff_data->gff_feature,
      transcript->cds_start,
      transcript->cds_end,
      '.',
      strand2Char(feature->strand),
      phase2Char(transcript->start_not_found)) ;

  result = dump_transcript_identifier(feature, transcript,
    gff_string, error, gff_data);
  }

  return result;
}

static gboolean dump_transcript_foreach_subpart(ZMapFeature feature, GString *buffer,
                                                GError **error_out, GArray *subparts,
                                                GFFDumpData gff_data)
{
  gboolean result = TRUE ;
  int i ;

  for (i = 0 ; i < subparts->len && result ; i++)
    {
      ZMapSpanStruct span = g_array_index(subparts, ZMapSpanStruct, i) ;
      ZMapPhase phase = ZMAPPHASE_NONE ;

      /* 16.2810538-2863623      Transcript      intron  19397   20627   .       -       .       Sequence "AC005361.6-003" */
      /* Obligatory fields. */
      g_string_append_printf(buffer, "\n" GFF_OBLIGATORY_NOSCORE,
        gff_data->gff_sequence,
        gff_data->gff_source,
        gff_data->gff_feature,
        span.x1, span.x2,
        '.', /* no score */
        strand2Char(feature->strand),
        phase2Char(phase)) ;

      result = dump_transcript_identifier(feature, &(feature->feature.transcript),
        buffer, error_out, gff_data);
    }

  return result ;
}


/* alignment attributes... */

static gboolean dump_alignment_target(ZMapFeature feature, gpointer homol_data,
                                      GString *gff_string, GError **error,
                                      GFFDumpData gff_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean has_target = TRUE;

  gchar *homol_type = NULL;

  /* reading GFF2 we get Sequence or Motif getting set to DNA
    and we cannot translate back
    Ed says this will be resolved in GFF3
    but right now it's wrong
  */

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
}

static gboolean dump_alignment_gaps(ZMapFeature feature, gpointer homol_data,
                                    GString *gff_string, GError **error,
                                    GFFDumpData gff_data)
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

    /* remove the last comma! */
    g_string_truncate(gff_string, gff_string->len - 1);
    /* and add a quote and a tab?*/
    g_string_append_printf(gff_string, "%c" GFF_ATTRIBUTE_TAB_END, '"');
  }

  return has_gaps;
}

static gboolean dump_alignment_clone(ZMapFeature feature, gpointer homol_data,
                                     GString *gff_string, GError **error,
                                     GFFDumpData gff_data)
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
                                      GFFDumpData gff_data)
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









/* alignment attributes... */

static gboolean dump_alignment_target_v3(ZMapFeature feature, gpointer homol_data,
                                      GString *gff_string, GError **error,
                                      GFFDumpData gff_data)
{
  static const char * read_pair = "read_pair" ;
  ZMapHomol homol = (ZMapHomol)homol_data;
  gboolean result = FALSE ;

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
      }
    else
      {
        g_string_append_printf(gff_string, "Target=%s %d %d %s",
          g_quark_to_string(homol->clone_id),
          homol->y1, homol->y2, zMapFeatureStrand2Str(homol->strand));
      }
    result = TRUE ;
  }

  return result ;
}


static gboolean dump_alignment_gaps_v3(ZMapFeature feature, gpointer homol_data,
                                       GString *gff_string, GError **error,
                                       GFFDumpData gff_data)
{
  ZMapHomol homol = (ZMapHomol)homol_data;
  GArray *gaps_array = NULL;
  int i;
  gboolean has_gaps = FALSE;

  if((gaps_array = homol->align))
  {
    has_gaps = TRUE;

    g_string_append_printf(gff_string, "gaps=");

    for(i = 0; i < gaps_array->len; i++)
      {
        ZMapAlignBlock block = NULL;

        block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i));

        g_string_append_printf(gff_string, "%d %d %d %d,",
        block->q1, block->q2,
        block->t1, block->t2);
      }

    /* remove the last comma! */
    g_string_truncate(gff_string, gff_string->len - 1);
  }

  return has_gaps;
}







static char strand2Char(ZMapStrand strand)
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


static char phase2Char(ZMapPhase phase)
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

