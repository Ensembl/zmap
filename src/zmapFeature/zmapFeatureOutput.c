/*  File: zmapFeatureOutput.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Mar 29 09:16 2010 (edgrif)
 * Created: Tue Oct 28 16:20:33 2008 (rds)
 * CVS info:   $Id: zmapFeatureOutput.c,v 1.11 2010-03-29 15:32:39 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <unistd.h>		/* STDOUT_FILENO */
#include <string.h>

#include <ZMap/zmapFeature.h>

typedef enum
  {
    DUMP_DATA_INVALID,
    DUMP_DATA_ANY,
    DUMP_DATA_RANGE
  } DumpDataType;


/* Internal data structs */

typedef struct
{
  DumpDataType data_type;
  gpointer     user_data;
} DumpAnyStruct, *DumpAny;

typedef struct
{
  gboolean    status ;
  GIOChannel *channel ;
  GData      *styles ;
  GString    *dump_string;
  GError    **dump_error ;
  DumpAny     dump_data;	/* will contain the user's data in dump_data->user_data */
  GDestroyNotify dump_free;
  ZMapFeatureDumpFeatureFunc dump_func;
} DumpFeaturesToFileStruct, *DumpFeaturesToFile ;

typedef struct
{
  DumpDataType data_type;
  gpointer     user_data;
  ZMapSpan     span;
} DumpWithinRangeStruct, *DumpWithinRange;

/* Internal functions */
static ZMapFeatureContextExecuteStatus dump_features_cb(GQuark   key,
							gpointer data,
							gpointer user_data,
							char   **err_out);
static void invoke_dump_features_cb(gpointer list_data, gpointer user_data);
static gboolean simple_context_print_cb(ZMapFeatureAny feature_any,
					GData         *styles,
					GString       *dump_string_in_out,
					GError       **error,
					gpointer       user_data);
static ZMapFeatureContextExecuteStatus range_invoke_dump_features_cb(GQuark   key,
								     gpointer data,
								     gpointer user_data,
								     char   **err_out);


/* The code */

/* Public first */


/*!
 * \brief Dump the Feature Context in a zmap speific context format.  Useful for debugging.
 */
gboolean zMapFeatureContextDump(ZMapFeatureContext feature_context, GData *styles,
				GIOChannel *file, GError **error_out)
{
  gboolean result = FALSE;

  result = zMapFeatureContextDumpToFile((ZMapFeatureAny)feature_context, styles,
					simple_context_print_cb, NULL,
					file, error_out);

  return result;
}

/* N.B. call only returns TRUE if the dump _and_ the io channel close succeed. */
gboolean zMapFeatureDumpStdOutFeatures(ZMapFeatureContext feature_context, GData *styles, GError **error_out)
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


gboolean zMapFeatureDumpToFileName(ZMapFeatureContext feature_context,char *filename, char *header, GData *styles, GError **error_out)
{
  gboolean result = FALSE ;
  GIOChannel *file ;
  guint len;


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
 * \brief Similar to zMapFeatureListDumpToFile, but expose it's internals enough that
 *        users can call g_list_foreach() on their list using the returned pointers.
 *        The GFunc expects (ZMapFeatureAny feature, gpointer dumper_data_out)
 */
gboolean zMapFeatureListForeachDumperCreate(ZMapFeatureDumpFeatureFunc dump_func,
					    GData                     *styles,
					    gpointer                   dump_user_data,
					    GDestroyNotify             dump_user_free,
					    GIOChannel                *dump_file,
					    GError                   **dump_error_out,
					    GFunc                     *dumper_func_out,
					    gpointer                  *dumper_data_out)
{
  gboolean result = FALSE;
  DumpFeaturesToFile dump_data = NULL;

  zMapAssert(dump_file);
  zMapAssert(dump_func);
  zMapAssert(dump_error_out) ;

  zMapAssert(dumper_func_out);
  zMapAssert(dumper_data_out);

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

/*!
 * \brief destroy data allocated with zMapFeatureListForeachDumperCreate()
 */
gboolean zMapFeatureListForeachDumperDestroy(gpointer dumper_data)
{
  DumpFeaturesToFile dump_data = (DumpFeaturesToFile)dumper_data;
  gboolean result;

  /* I'm a little cautious about the void nature of the data here, but
   * it means the DumpFeaturesToFile struct is _really_ private!!! */

  result = dump_data->status;

  if(result)
    {
      if (g_io_channel_flush(dump_data->channel, dump_data->dump_error) != G_IO_STATUS_NORMAL)
	result = FALSE;
    }

  if(dump_data->dump_free && dump_data->dump_data->user_data)
    (dump_data->dump_free)(dump_data->dump_data->user_data);

  g_string_free(dump_data->dump_string, TRUE);

  g_free(dump_data->dump_data);	/* This is an allocated DumpAny. */

  g_free(dump_data);		/* The main struct */

  dump_data = NULL;

  return result;
}

gboolean zMapFeatureListDumpToFile(GList                     *feature_list,
				   GData                     *styles,
				   ZMapFeatureDumpFeatureFunc dump_func,
				   gpointer                   dump_user_data,
				   GIOChannel                *dump_file,
				   GError                   **dump_error_out)
{
  gboolean result = FALSE;
  DumpFeaturesToFileStruct dump_data = {FALSE};
  DumpAnyStruct dump_any;

  zMapAssert(dump_file);
  zMapAssert(dump_func);
  zMapAssert(dump_error_out) ;

  dump_any.data_type = DUMP_DATA_ANY;
  dump_any.user_data = dump_user_data;

  dump_data.status      = TRUE ;
  dump_data.channel     = dump_file ;
  dump_data.styles      = styles ;
  dump_data.dump_error  = dump_error_out ;
  dump_data.dump_func   = dump_func ;
  dump_data.dump_data   = &dump_any ;
  dump_data.dump_string = g_string_sized_new(2000);

  g_list_foreach(feature_list, invoke_dump_features_cb, &dump_data);

  g_string_free(dump_data.dump_string, TRUE);

  result = dump_data.status ;

  if(result)
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
				      GData                     *styles,
				      ZMapFeatureDumpFeatureFunc dump_func,
				      gpointer                   dump_user_data,
				      GIOChannel                *dump_file,
				      GError                   **dump_error_out)
{
  gboolean result = FALSE ;
  DumpFeaturesToFileStruct dump_data = {FALSE};
  DumpAnyStruct dump_any;

  zMapAssert(dump_file);
  zMapAssert(dump_set);
  zMapAssert(dump_func);
  zMapAssert(dump_error_out) ;

  zMapAssert(dump_set->struct_type == ZMAPFEATURE_STRUCT_CONTEXT    ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_ALIGN      ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_BLOCK      ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  dump_any.data_type = DUMP_DATA_ANY;
  dump_any.user_data = dump_user_data;

  dump_data.status      = TRUE ;
  dump_data.channel     = dump_file ;
  dump_data.styles      = styles ;
  dump_data.dump_error  = dump_error_out ;
  dump_data.dump_func   = dump_func ;
  dump_data.dump_data   = &dump_any ;
  dump_data.dump_string = g_string_sized_new(2000);

  zMapFeatureContextExecuteSubset(dump_set, ZMAPFEATURE_STRUCT_FEATURESET,
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
					   GData                     *styles,
					   ZMapSpan                   span_data,
					   ZMapFeatureDumpFeatureFunc dump_func,
					   gpointer                   dump_user_data,
					   GIOChannel                *dump_file,
					   GError                   **dump_error_out)
{
  gboolean result = FALSE ;
  DumpFeaturesToFileStruct dump_data = {FALSE};
  DumpWithinRangeStruct range_data;

  zMapAssert(dump_file);
  zMapAssert(dump_set);
  zMapAssert(dump_func);
  zMapAssert(dump_error_out) ;

  zMapAssert(dump_set->struct_type == ZMAPFEATURE_STRUCT_CONTEXT    ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_ALIGN      ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_BLOCK      ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET ||
	     dump_set->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

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
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  DumpFeaturesToFile dump_data = (DumpFeaturesToFile)user_data ;

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

static gboolean simple_context_print_cb(ZMapFeatureAny feature_any,
					GData         *styles,
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
			       "Feature Context:\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\n",
			       g_quark_to_string(feature_context->unique_id),
			       g_quark_to_string(feature_context->original_id),
			       g_quark_to_string(feature_context->sequence_name),
			       g_quark_to_string(feature_context->parent_name),
			       feature_context->parent_span.x1,
			       feature_context->parent_span.x2,
			       feature_context->sequence_to_parent.p1,
			       feature_context->sequence_to_parent.p2,
			       feature_context->sequence_to_parent.c1,
			       feature_context->sequence_to_parent.c2);
      }
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
	ZMapFeatureAlignment feature_align;

	feature_align = (ZMapFeatureAlignment)feature_any;
	g_string_append_printf(dump_string_in_out,
			       "\tAlignment:\t%s\n",
			       g_quark_to_string(feature_align->unique_id)) ;

      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	ZMapFeatureBlock feature_block;
	feature_block = (ZMapFeatureBlock)feature_any;
	g_string_append_printf(dump_string_in_out,
			       "\tBlock:\t%s\t%d\t%d\t%d\t%d\n",
			       g_quark_to_string(feature_block->unique_id),
			       feature_block->block_to_sequence.t1,
			       feature_block->block_to_sequence.t2,
			       feature_block->block_to_sequence.q1,
			       feature_block->block_to_sequence.q2) ;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
	ZMapFeatureSet feature_set;
	feature_set = (ZMapFeatureSet)feature_any;
	g_string_append_printf(dump_string_in_out,
			       "\tFeature Set:\t%s\t%s\n",
			       g_quark_to_string(feature_set->unique_id),
			       (char *)g_quark_to_string(feature_set->original_id)) ;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature feature;
	char *type = "(type)", *strand, *phase;
	ZMapFeatureTypeStyle style ;

	feature = (ZMapFeature)feature_any;

      if(styles)
      {
	      style = zMapFindStyle(styles, feature->style_id) ;
	      type   = (char *)zMapStyleMode2ExactStr(zMapStyleGetMode(style)) ;
      }
        strand = zMapFeatureStrand2Str(feature->strand) ;
        phase  = zMapFeaturePhase2Str(feature->phase) ;

	g_string_append_printf(dump_string_in_out,
			       "\t\t%s\t%d\t%s\t%s\t%d\t%d\t%s\t%s\t%f",
			       (char *)g_quark_to_string(feature->unique_id),
			       feature->db_id,
			       (char *)g_quark_to_string(feature->original_id),
			       type,
			       feature->x1,
			       feature->x2,
			       strand,
			       phase,
			       feature->score) ;

        if (feature->description)
          {
            g_string_append_c(dump_string_in_out, '\t');
            g_string_append(dump_string_in_out, feature->description) ;
          }

        g_string_append_c(dump_string_in_out, '\n') ;
      }
      break;
    default:
      result = FALSE;
      break;
    }

  return result;
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

