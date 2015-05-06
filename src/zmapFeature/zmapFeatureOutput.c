/*  File: zmapFeatureOutput.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Dumping of features/feature context to various
 *              destinations.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <unistd.h>    /* STDOUT_FILENO */
#include <string.h>

#include <ZMap/zmapFeature.h>


typedef enum
  {
    DUMP_DATA_INVALID,
    DUMP_DATA_ANY,
    DUMP_DATA_RANGE
  } DumpDataType;


typedef struct
{
  DumpDataType data_type;
  gpointer     user_data;
} DumpAnyStruct, *DumpAny;

typedef struct
{
  gboolean    status ;
  GIOChannel *channel ;
  GHashTable *styles ;
  GString    *dump_string;
  GError    **dump_error ;
  DumpAny     dump_data;/* will contain the user's data in dump_data->user_data */
  GDestroyNotify dump_free;
  ZMapFeatureDumpFeatureFunc dump_func;
} DumpFeaturesToFileStruct, *DumpFeaturesToFile ;

typedef struct
{
  DumpDataType data_type;
  gpointer     user_data;
  ZMapSpan     span;
} DumpWithinRangeStruct, *DumpWithinRange;



static ZMapFeatureContextExecuteStatus dump_features_cb(GQuark key,
                                                        gpointer data, gpointer user_data,
                                                        char **err_out) ;
static void invoke_dump_features_cb(gpointer list_data, gpointer user_data) ;
static ZMapFeatureContextExecuteStatus range_invoke_dump_features_cb(GQuark   key,
                                                                     gpointer data, gpointer user_data,
                                                                     char   **err_out) ;
static gboolean simple_context_print_cb(ZMapFeatureAny feature_any, GHashTable *styles,
                                        GString *dump_string_in_out, GError **error, gpointer user_data) ;

GString *feature2Text(GString *feature_str, ZMapFeature feature) ;
static GString *basicFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature) ;
static GString *alignFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature) ;
static GString *transcriptFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature) ;
static GString *assemblyFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature) ;
static GString *sequenceFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature) ;






/*
 *                          External interface.
 */


char *zMapFeatureAsString(ZMapFeature feature)
{
  char *feature_text = NULL ;
  GString *feature_str ;

  feature_str = g_string_sized_new(4000) ;

  feature_str = feature2Text(feature_str, feature) ;

  feature_text = g_string_free(feature_str, FALSE) ;

  return feature_text ;
}




/*!
 * \brief Dump the Feature Context in a zmap speific context format.  Useful for debugging.
 */
gboolean zMapFeatureContextDump(ZMapFeatureContext feature_context, GHashTable *styles,
GIOChannel *file, GError **error_out)
{
  gboolean result = FALSE;

  result = zMapFeatureContextDumpToFile((ZMapFeatureAny)feature_context, styles,
                                        simple_context_print_cb, NULL,
                                        file, error_out);

  return result;
}

/* N.B. call only returns TRUE if the dump _and_ the io channel close succeed. */
gboolean zMapFeatureDumpStdOutFeatures(ZMapFeatureContext feature_context, GHashTable *styles, GError **error_out)
{
  gboolean result = FALSE ;
  GIOChannel *file ;

  file = g_io_channel_unix_new(STDOUT_FILENO) ;
  g_io_channel_set_close_on_unref(file, FALSE) ;

  result = zMapFeatureContextDump(feature_context, styles, file, error_out) ;

  if (g_io_channel_flush(file, error_out) != G_IO_STATUS_NORMAL)
    printf("cannot flush stdout\n") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* This seems to mess up stdout so I'm leaving the GIOChannel dangling for now,
   * not sure what to do about it...the g_io_channel_set_close_on_unref() cal
   * doesn't seem to work....sigh...l*/

  if (g_io_channel_shutdown(file, FALSE, error_out) != G_IO_STATUS_NORMAL)
    result = FALSE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return result ;
}


gboolean zMapFeatureDumpToFileName(ZMapFeatureContext feature_context,char *filename, char *header, GHashTable *styles, GError **error_out)
{
  gboolean result = FALSE ;
  GIOChannel *file ;
  gsize len ;

  file = g_io_channel_new_file(filename, "a", error_out) ;

  g_io_channel_write_chars(file, header, strlen(header), &len, error_out);

  result = zMapFeatureContextDump(feature_context,  styles, file, error_out) ;

  if (g_io_channel_flush(file, error_out) != G_IO_STATUS_NORMAL)
    printf("cannot flush stdout\n") ;

  if (g_io_channel_shutdown(file, TRUE, error_out) != G_IO_STATUS_NORMAL)
    result = FALSE ;

  return result ;
}




/*!
 * \brief Similar to zMapFeatureListDumpToFileOrBuffer, but expose it's internals enough that
 *        users can call g_list_foreach() on their list using the returned pointers.
 *        The GFunc expects (ZMapFeatureAny feature, gpointer dumper_data_out)
 */
/*
gboolean zMapFeatureListForeachDumperCreate(ZMapFeatureDumpFeatureFunc dump_func,
    GHashTable                *styles,
    gpointer                   dump_user_data,
    GDestroyNotify             dump_user_free,
    GIOChannel                *dump_file,
    GError                   **dump_error_out,
    GFunc                     *dumper_func_out,
    gpointer                  *dumper_data_out)
{
  gboolean result = FALSE;
  DumpFeaturesToFile dump_data = NULL;

  if (!dump_file || !dump_func || !dump_error_out || !dumper_func_out || !dumper_data_out)
    return result ;

  if((dump_data = g_new0(DumpFeaturesToFileStruct, 1)))
    {
      DumpAny dump_any = NULL;

      dump_any            = g_new0(DumpAnyStruct, 1);
      dump_any->user_data = dump_user_data;
      dump_any->data_type = DUMP_DATA_ANY;

      dump_data->status      = TRUE           ;
      dump_data->channel     = dump_file      ;
      dump_data->styles      = styles         ;
      dump_data->dump_error  = dump_error_out ;
      dump_data->dump_func   = dump_func      ;
      dump_data->dump_data   = dump_any       ;
      dump_data->dump_free   = dump_user_free ;
      dump_data->dump_string = g_string_sized_new(2000);

      *dumper_func_out = invoke_dump_features_cb;
      *dumper_data_out = dump_data;

      result = dump_data->status;
    }

  return result;
}
*/

/*!
 * \brief destroy data allocated with zMapFeatureListForeachDumperCreate()
 */
/*
gboolean zMapFeatureListForeachDumperDestroy(gpointer dumper_data)
{
  DumpFeaturesToFile dump_data = (DumpFeaturesToFile)dumper_data;
  gboolean result;
  result = dump_data->status;

  if(result)
    {
      if (g_io_channel_flush(dump_data->channel, dump_data->dump_error) != G_IO_STATUS_NORMAL)
        result = FALSE;
    }

  if(dump_data->dump_free && dump_data->dump_data->user_data)
    (dump_data->dump_free)(dump_data->dump_data->user_data);

  g_string_free(dump_data->dump_string, TRUE);

  g_free(dump_data->dump_data);

  g_free(dump_data);

  dump_data = NULL;

  return result;
}
*/

/* This dumps to the given dump_file if that is non-null, and/or returns the result
 * in text_out, if that is non-null */
gboolean zMapFeatureListDumpToFileOrBuffer(GList                     *feature_list,
                                           GHashTable                 *styles,
                                           ZMapFeatureDumpFeatureFunc dump_func,
                                           gpointer                   dump_user_data,
                                           GIOChannel                *dump_file,
                                           GString                   *buffer,
                                           GError                   **dump_error_out)
{
  gboolean result = FALSE;
  DumpFeaturesToFileStruct dump_data = {FALSE};
  DumpAnyStruct dump_any;

  if (!(dump_file || buffer) || ((!dump_file) && (!buffer))|| !dump_func || !dump_error_out)
    return result ;

  dump_any.data_type = DUMP_DATA_ANY;
  dump_any.user_data = dump_user_data;

  dump_data.status      = TRUE ;
  dump_data.channel     = dump_file ;
  dump_data.styles      = styles ;
  dump_data.dump_error  = dump_error_out ;
  dump_data.dump_func   = dump_func ;
  dump_data.dump_data   = &dump_any ;
  /*dump_data.dump_string = g_string_sized_new(2000);*/
  if (buffer)
    dump_data.dump_string = buffer ;
  else
    dump_data.dump_string = g_string_sized_new(2000) ;

  g_list_foreach(feature_list, invoke_dump_features_cb, &dump_data);

  /* If the caller has requested the text output, return the GString, otherwise free it */
  /*if (text_out)
    *text_out = dump_data.dump_string ;
  else
    g_string_free(dump_data.dump_string, TRUE);*/
  if (!buffer)
    g_string_free(dump_data.dump_string, TRUE) ;

  result = dump_data.status ;

  if(result && dump_file)
    {
      if (g_io_channel_flush(dump_file, dump_error_out) != G_IO_STATUS_NORMAL)
        result = FALSE;
    }

  return result;
}

/* Generalised dumping function, caller supplies a callback function that does the actual
 * output and a pointer to somewhere in the feature hierachy (alignment, block, set etc)
 * and this code calls the callback to do the output of the feature in the appropriate
 * form. */
gboolean zMapFeatureContextDumpToFile(ZMapFeatureAny             dump_set,
      GHashTable                *styles,
      ZMapFeatureDumpFeatureFunc dump_func,
      gpointer                   dump_user_data,
      GIOChannel                *dump_file,
      GError                   **dump_error_out)
{
  gboolean result = FALSE ;
  DumpFeaturesToFileStruct dump_data = {FALSE};
  DumpAnyStruct dump_any;

  if (!dump_file || !dump_set || !dump_func || !dump_error_out)
    return result ;

  if ((dump_set->struct_type != ZMAPFEATURE_STRUCT_CONTEXT)    &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_ALIGN)      &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_BLOCK)      &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_FEATURESET) &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_FEATURE))
    return result ;

  dump_any.data_type = DUMP_DATA_ANY;
  dump_any.user_data = dump_user_data;

  dump_data.status      = TRUE ;
  dump_data.channel     = dump_file ;
  dump_data.styles      = styles ;
  dump_data.dump_error  = dump_error_out ;
  dump_data.dump_func   = dump_func ;
  dump_data.dump_data   = &dump_any ;
  dump_data.dump_string = g_string_sized_new(2000);

  zMapFeatureContextExecuteSubset(dump_set, ZMAPFEATURE_STRUCT_FEATURE,
  dump_features_cb, &dump_data);


  g_string_free(dump_data.dump_string, TRUE);

  result = dump_data.status ;

  if(result)
    {
      if (g_io_channel_flush(dump_file, dump_error_out) != G_IO_STATUS_NORMAL)
        result = FALSE;
    }

  return result ;
}

gboolean zMapFeatureContextRangeDumpToFile(ZMapFeatureAny             dump_set,
   GHashTable                *styles,
   ZMapSpan                   span_data,
   ZMapFeatureDumpFeatureFunc dump_func,
   gpointer                   dump_user_data,
   GIOChannel                *dump_file,
   GError                   **dump_error_out)
{
  gboolean result = FALSE ;
  DumpFeaturesToFileStruct dump_data = {FALSE};
  DumpWithinRangeStruct range_data;

  if (!dump_file || !dump_set || !dump_func || !dump_error_out)
    return result ;

  if ((dump_set->struct_type != ZMAPFEATURE_STRUCT_CONTEXT)    &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_ALIGN)      &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_BLOCK)      &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_FEATURESET) &&
      (dump_set->struct_type != ZMAPFEATURE_STRUCT_FEATURE))
    return result ;

  range_data.data_type = DUMP_DATA_RANGE;
  range_data.span      = span_data;
  range_data.user_data = dump_user_data;

  dump_data.status      = TRUE ;
  dump_data.channel     = dump_file ;
  dump_data.styles      = styles ;
  dump_data.dump_error  = dump_error_out ;
  dump_data.dump_func   = dump_func ;
  dump_data.dump_data   = (DumpAny)(&range_data) ;
  dump_data.dump_string = g_string_sized_new(2000);

  zMapFeatureContextExecuteSubset(dump_set, ZMAPFEATURE_STRUCT_FEATURE,
  range_invoke_dump_features_cb, &dump_data);

  g_string_free(dump_data.dump_string, TRUE);

  result = dump_data.status ;

  if(result)
    {
      if (g_io_channel_flush(dump_file, dump_error_out) != G_IO_STATUS_NORMAL)
        result = FALSE;
    }

  return result;
}

GQuark zMapFeatureContextDumpErrorDomain(void)
{
#define ZMAP_FEATURE_CONTEXT_DUMP_ERROR "context output error"
  return g_quark_from_string(ZMAP_FEATURE_CONTEXT_DUMP_ERROR);
}





/*
 *             Internal functions.
 */



static ZMapFeatureContextExecuteStatus dump_features_cb(GQuark   key,
gpointer data,
gpointer user_data,
char   **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  DumpFeaturesToFile dump_data = (DumpFeaturesToFile)user_data ;
  if (!feature_any || !dump_data)
    return status ;
  status = ZMAP_CONTEXT_EXEC_STATUS_OK ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
    case ZMAPFEATURE_STRUCT_FEATURESET:
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        if(dump_data->status)
          {
            if(dump_data->dump_func)
              dump_data->status = (dump_data->dump_func)(feature_any,
                                                         dump_data->styles,
                                                         dump_data->dump_string,
                                                         dump_data->dump_error,
                                                         dump_data->dump_data->user_data);
            else
              {
                dump_data->status = FALSE;
                *(dump_data->dump_error) = g_error_new(zMapFeatureContextDumpErrorDomain(),
                                                       ZMAPFEATURE_DUMP_NO_FUNCTION,
                                                       "%s",
                                                       "No function.");
              }
          }
      }
      break;
    default:
      /* Unknown feature type. */
      if(dump_data->status)
        {
          dump_data->status = FALSE;
          *(dump_data->dump_error) = g_error_new(zMapFeatureContextDumpErrorDomain(),
                                                 ZMAPFEATURE_DUMP_UNKNOWN_FEATURE_TYPE,
                                                 "Unknown Feature struct type '%d'.",
                                                 feature_any->struct_type);
        }
      break;
    }

  if(dump_data->status && dump_data->dump_string->len > 0)
    {
      if (dump_data->channel)
        {
          GError *io_error = NULL;
          gsize bytes_written = 0;
          gssize bytes_to_write = dump_data->dump_string->len;
          GIOStatus write_status;

          /* we can now print dump_data->dump_string to file */
          if((write_status = g_io_channel_write_chars(dump_data->channel,
                                                      dump_data->dump_string->str,
                                                      dump_data->dump_string->len,
                                                      &bytes_written,
                                                      &io_error)) == G_IO_STATUS_NORMAL)
            {
              if(bytes_written == bytes_to_write)
                dump_data->dump_string = g_string_truncate(dump_data->dump_string, 0);
              else
                dump_data->dump_string = g_string_erase(dump_data->dump_string, 0, bytes_written);
            }
          else if(write_status == G_IO_STATUS_ERROR)
            {
              /* error handling... */
              dump_data->status = FALSE;
              status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
              /* we need to pass on the error... */
            }
          else if(write_status == G_IO_STATUS_AGAIN)
            {
              /* what does this mean?  We'll probably get round to the
               * data again, do we need to g_string_erase? */

              if(bytes_written != 0)
                dump_data->dump_string = g_string_erase(dump_data->dump_string, 0, bytes_written);
            }
          else
            {
              /* EOF? we're writing aren't we! */
            }
        }
    }

  return status;
}

static void invoke_dump_features_cb(gpointer list_data, gpointer user_data)
{
  DumpFeaturesToFile dump_data = (DumpFeaturesToFile)user_data;
  ZMapFeatureAny feature_any;

  if(dump_data->status)
    {
      ZMapFeatureContextExecuteStatus fake = ZMAP_CONTEXT_EXEC_STATUS_OK;
      feature_any = (ZMapFeatureAny)list_data;
      char *dump_feature_error = NULL;

      fake = dump_features_cb(feature_any->unique_id,
      feature_any,
      dump_data,
      &dump_feature_error);

      if(fake != ZMAP_CONTEXT_EXEC_STATUS_OK)
        dump_data->status = FALSE;
    }

  return ;
}


static ZMapFeatureContextExecuteStatus range_invoke_dump_features_cb(GQuark   key,
     gpointer data,
     gpointer user_data,
     char   **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  DumpFeaturesToFile dump_data = (DumpFeaturesToFile)user_data ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
    case ZMAPFEATURE_STRUCT_FEATURESET:
      if(dump_data->status)
        {
          status = dump_features_cb(key, data, user_data, err_out);
        }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeature feature = (ZMapFeature)feature_any;
        DumpWithinRange range_data = (DumpWithinRange)(dump_data->dump_data);

        if(dump_data->status)
          {
            /* going with ! outside only at the moment. */
            if(!(feature->x1 > range_data->span->x2 ||
                 feature->x2 < range_data->span->x1))
              {
        /* should we be clipping the features???? */
                status = dump_features_cb(key, data, user_data, err_out);
              }
          }
      }
      break;
    default:
      break;
    }

  return status;
}



static gboolean simple_context_print_cb(ZMapFeatureAny feature_any,
                                        GHashTable    *styles,
                                        GString       *dump_string_in_out,
                                        GError       **error,
                                        gpointer       user_data)
{
  gboolean result = TRUE;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
        ZMapFeatureContext feature_context;

        feature_context = (ZMapFeatureContext)feature_any;
        g_string_append_printf(dump_string_in_out,
                               "Feature Context:\t%s\t%s\t%s\t%s\t%d\t%d\n",
                               g_quark_to_string(feature_context->unique_id),
                               g_quark_to_string(feature_context->original_id),
                               g_quark_to_string(feature_context->sequence_name),
                               g_quark_to_string(feature_context->parent_name),
                               feature_context->parent_span.x1,
                               feature_context->parent_span.x2);
        break;
      }
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align;

        feature_align = (ZMapFeatureAlignment)feature_any;
        g_string_append_printf(dump_string_in_out,
                               "\tAlignment:\t%s\t%d\t%d\n",
                               g_quark_to_string(feature_align->unique_id),
                               feature_align->sequence_span.x1,
                               feature_align->sequence_span.x2);

        break;
      }
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock feature_block;
        feature_block = (ZMapFeatureBlock)feature_any;
        g_string_append_printf(dump_string_in_out,
                               "\tBlock:\t%s\t%d\t%d\n",
                               g_quark_to_string(feature_block->unique_id),
                               feature_block->block_to_sequence.parent.x1,
                               feature_block->block_to_sequence.parent.x2) ;
        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set;
        feature_set = (ZMapFeatureSet)feature_any;
        g_string_append_printf(dump_string_in_out,
                               "\tFeature Set:\t%s\t%s\n",
                               g_quark_to_string(feature_set->unique_id),
                               (char *)g_quark_to_string(feature_set->original_id)) ;
        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeature feature;
        feature = (ZMapFeature)feature_any;

        g_string_append(dump_string_in_out, "\t\t") ;
        dump_string_in_out = feature2Text(dump_string_in_out, feature) ;
        g_string_append_c(dump_string_in_out, '\n') ;

        break;
      }
    default:
      {
        result = FALSE;
        break;
      }
    }

  return result;
}



GString *feature2Text(GString *feature_str, ZMapFeature feature)
{
  GString *result = feature_str ;
  char *feature_mode, *style_mode ;
  const char *strand ;
  const char *indent = "" ;
  gsize str_len ;

  /* Title */
  g_string_append_printf(result, "%sZMapFeature %p, \"%s\" (unique id = \"%s\")\n",
                         indent,
                         feature,
                         (char *)g_quark_to_string(feature->original_id),
                         (char *)g_quark_to_string(feature->unique_id)) ;

  /* Common feature parts */
  indent = "  " ;

  feature_mode = (char *)zMapStyleMode2ExactStr(feature->mode) ;
  style_mode = (char *)zMapStyleMode2ExactStr(zMapStyleGetMode(*feature->style)) ;
  g_string_append_printf(result, "%sFeature mode: %s  Style mode: %s\n",
                         indent, feature_mode, style_mode) ;

  g_string_append_printf(result, "%sStyle: \"%s\"\n", indent, zMapStyleGetName(*feature->style)) ;

  g_string_append_printf(result, "%sSO term: \"%s\"\n",
                         indent, g_quark_to_string(feature->SO_accession)) ;

  strand = zMapFeatureStrand2Str(feature->strand) ;
  g_string_append_printf(result, "%sStrand, Start, End: %s, %d, %d\n", indent, strand, feature->x1, feature->x2) ;

  if (feature->flags.has_score)
    g_string_append_printf(result, "%sScore = %g\n", indent, feature->score) ;

  g_string_append_printf(result, "%sSource id: \"%s\"  Source text: \"%s\"\n",
                         indent,
                         g_quark_to_string(feature->source_id),
                         g_quark_to_string(feature->source_text)) ;

  if (feature->description)
    g_string_append_printf(result, "%sDescription: \"%s\"" , indent, feature->description) ;

  if (feature->url)
    g_string_append_printf(result, "%sURL: \"%s\"" , indent, feature->url) ;


  /* Now print the subpart details. */
  indent = "    " ;
  g_string_append_printf(result, "\n%s%s subparts:\n", indent, zMapStyleMode2ExactStr(feature->mode)) ;

  str_len = result->len ;                                   /* Used to detect if there any subparts. */

  indent = "      " ;
  switch (feature->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
      {
        result = basicFeature2Txt(result, indent, feature) ;

        break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
        result = alignFeature2Txt(result, indent, feature) ;

        break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
        result = transcriptFeature2Txt(result, indent, feature) ;

        break ;
      }
    case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
      {
        result = assemblyFeature2Txt(result, indent, feature) ;

        break ;
      }
    case ZMAPSTYLE_MODE_SEQUENCE:
      {
        result = sequenceFeature2Txt(result, indent, feature) ;

        break ;
      }
    default:
      {
        g_string_append_printf(result, "%s<No printable subparts>" , indent) ;

        break ;
      }
    }


  /* If there were no subparts then tell the user ! */
  if (str_len == result->len)
    g_string_append_printf(result, "%s<None>\n" , indent) ;


  return result ;
}




static GString *basicFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature)
{
  GString *result = result_in ;

  if (feature->feature.basic.known_name)
    g_string_append_printf(result, "%sKnown name: %s\n",
                           indent, g_quark_to_string(feature->feature.basic.known_name)) ;

  if (feature->feature.basic.flags.variation_str)
    g_string_append_printf(result, "%sVariation string: %s\n", indent, feature->feature.basic.variation_str) ;

  return result ;
}


static GString *alignFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature)
{
  GString *result = result_in ;
  char *homol_str ;
  const char *strand ;

  /* Need to do homol type but need to set up enum stuff..... */
  homol_str = zMapFeatureHomol2Str(feature->feature.homol.type) ;
  g_string_append_printf(result, "%sHomol type: %s\n", indent, homol_str) ;

  strand = zMapFeatureStrand2Str(feature->feature.homol.strand) ;
  g_string_append_printf(result, "%sStrand, Start, End: %s, %d, %d\n",
                         indent, strand, feature->feature.homol.y1, feature->feature.homol.y2) ;

  if (feature->feature.homol.percent_id)
    g_string_append_printf(result, "%sMatch percent id: %g\n", indent, feature->feature.homol.percent_id) ;

  if (feature->feature.homol.length)
    g_string_append_printf(result, "%sMatch sequence length: %d\n", indent, feature->feature.homol.length) ;

  if (feature->feature.homol.flags.has_clone_id)
    g_string_append_printf(result, "%sClone id for match: %s\n",
                           indent, g_quark_to_string(feature->feature.homol.clone_id)) ;

  if (feature->feature.homol.sequence)
    {
      enum {SEQ_LEN = 20} ;
      int seq_len ;

      if ((seq_len = strlen(feature->feature.homol.sequence)) > SEQ_LEN)
        seq_len = SEQ_LEN ;

      g_string_append_printf(result, "%sStart of match sequence: ", indent) ;

      result = g_string_append_len(result, feature->feature.homol.sequence, seq_len) ;

      g_string_append(result, "\n") ;
    }

  if (feature->feature.homol.align)
    {
      GArray *gaps_array = feature->feature.homol.align ;
      int i ;

      g_string_append_printf(result, "%sGapped align blocks:\n", indent) ;

      for (i = 0 ; i < gaps_array->len ; i++)
        {
          ZMapAlignBlock block = NULL ;

          block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i)) ;

          g_string_append_printf(result, "%sReference <- Match: %d, %d <- %d, %d\n",
                                 indent, block->t1, block->t2, block->q1, block->q2) ;
        }
    }

  return result ;
}


static GString *transcriptFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature)
{
  GString *result = result_in ;

  if (feature->feature.transcript.known_name)
    g_string_append_printf(result, "%sKnown name: %s\n",
                           indent, g_quark_to_string(feature->feature.transcript.known_name)) ;

  if (feature->feature.transcript.locus_id)
    g_string_append_printf(result, "%sKnown name: %s\n",
                           indent, g_quark_to_string(feature->feature.transcript.locus_id)) ;

  if (feature->feature.transcript.flags.cds)
    g_string_append_printf(result, "%sCDS start = %d  end = %d\n",
                           indent, feature->feature.transcript.cds_start, feature->feature.transcript.cds_end) ;

  if (feature->feature.transcript.flags.start_not_found || feature->feature.transcript.flags.end_not_found)
    {
      g_string_append_printf(result, "%s", indent) ;

      if (feature->feature.transcript.flags.start_not_found)
        g_string_append_printf(result, "Start_not_found = %d  ", feature->feature.transcript.start_not_found) ;

      if (feature->feature.transcript.flags.end_not_found)
        g_string_append(result, "End_not_found = TRUE") ;
    }

  if (feature->feature.transcript.exons)
    {
      GArray *span_array = feature->feature.transcript.exons ;
      int i ;

      g_string_append_printf(result, "%sExons:\n", indent) ;

      for (i = 0 ; i < span_array->len ; i++)
        {
          ZMapSpan span = NULL ;

          span = &(g_array_index(span_array, ZMapSpanStruct, i)) ;

          g_string_append_printf(result, "%s  %d: %d, %d\n",
                                 indent, i, span->x1, span->x2) ;
        }
    }



  return result ;
}


static GString *assemblyFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature)
{
  GString *result = result_in ;
  const char *strand ;

  strand = zMapFeatureStrand2Str(feature->feature.assembly_path.strand) ;
  g_string_append_printf(result, "%sStrand = %s\n", indent, strand) ;

  if (feature->feature.assembly_path.length)
    g_string_append_printf(result, "%sAssembly sequence length: %d\n", indent, feature->feature.assembly_path.length) ;

  /* Print out path ?? */
  if (feature->feature.assembly_path.path)
    {
      GArray *span_array = feature->feature.assembly_path.path ;
      int i ;

      g_string_append_printf(result, "%sSpans:\n", indent) ;

      for (i = 0 ; i < span_array->len ; i++)
        {
          ZMapSpan span = NULL ;

          span = &(g_array_index(span_array, ZMapSpanStruct, i)) ;

          g_string_append_printf(result, "%sSpan: %d, %d\n",
                                 indent, span->x1, span->x2) ;
        }
    }


  return result ;
}


static GString *sequenceFeature2Txt(GString *result_in, const char *indent, ZMapFeature feature)
{
  GString *result = result_in ;
  const char *sequence_type ;

  if (feature->feature.sequence.name)
    g_string_append_printf(result, "%sSequence name: %s\n",
                           indent, g_quark_to_string(feature->feature.sequence.name)) ;

  /* Hacked the sequence type...needs proper enuming... */
  if (feature->feature.sequence.type == ZMAPSEQUENCE_DNA)
    sequence_type = "DNA" ;
  else
    sequence_type = "Peptide" ;
  g_string_append_printf(result, "%sSequence type: %s\n", indent, sequence_type) ;

  if (feature->feature.sequence.length)
    g_string_append_printf(result, "%sAssembly sequence length: %d\n", indent, feature->feature.sequence.length) ;

  if (feature->feature.sequence.sequence)
    {
      enum {SEQ_LEN = 20} ;
      int seq_len ;

      if ((seq_len = strlen(feature->feature.sequence.sequence)) > SEQ_LEN)
        seq_len = SEQ_LEN ;

      g_string_append_printf(result, "%sStart of match sequence: ", indent) ;

      result = g_string_append_len(result, feature->feature.sequence.sequence, seq_len) ;

      g_string_append(result, "\n") ;
    }


  return result ;
}



