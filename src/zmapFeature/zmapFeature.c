/*  File: zmapFeature.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
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
 * Description: Manipulates features.
 *              1
 * Exported functions: See zmapFeature.h
 * HISTORY:
<<<<<<< zmapFeature.c
 * Last edited: Nov 19 11:46 2004 (edgrif)
=======
 * Last edited: Nov 15 10:35 2004 (rnc)
>>>>>>> 1.6
 * Created: Tue Nov 2 2004 (rnc)
 * CVS info:   $Id: zmapFeature.c,v 1.7 2004-11-19 14:32:14 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>


static void contextDump_TD(ZMapFeatureContext feature_context, GIOChannel *channel);
static void setDump_TD(GQuark key_id, gpointer data, gpointer user_data);
static void featureDump_TD(GQuark key_id, gpointer data, gpointer user_data);
static gboolean printLine(GIOChannel *channel, gchar *line);



char *zmapLookUpEnum(int id, int enumType)
{
  /* These arrays must correspond 1:1 with the enums declared in zmapFeature.h */
  static char *types[8]   = {"BASIC", "HOMOL", "EXON", "INTRON", "TRANSCRIPT",
			     "VARIATION", "BOUNDARY", "SEQUENCE"} ;
  static char *strands[3] = {"ZMAPSTRAND_NONE", "ZMAPSTRAND_DOWN", "ZMAPSTRAND_UP" };
  static char *phases[4]  = {"ZMAPPHASE_NONE", "ZMAPPHASE_0", "ZMAPPHASE_1", "ZMAPPHASE_2" };

  switch (enumType)
    {
    case TYPE_ENUM:
      return types[id];
      break;

    case STRAND_ENUM:
      return strands[id];
      break;

    case PHASE_ENUM:
      return phases[id];
      break;

    default:
      break;
    }
  return NULL;
}

char *zMapFeatureCreateID(ZMapFeatureType feature_type, char *feature_name,
			  int start, int end, int query_start, int query_end)
{
  char *feature_id = NULL ;

  if (feature_type == ZMAPFEATURE_HOMOL)
    feature_id = g_strdup_printf("%s.%d-%d-%d-%d", feature_name, start, end, query_start, query_end) ;
  else
    feature_id = g_strdup_printf("%s.%d-%d", feature_name, start, end) ;

  return feature_id ;
}


/* In zmap we hold coords in the forward orientation always and get strand from the strand
 * member of the feature struct. This function looks at the supplied strand and sets the
 * coords accordingly. */
/* ACTUALLY I REALISE I'M NOT QUITE SURE WHAT I WANT THIS FUNCTION TO DO.... */
gboolean zMapFeatureSetCoords(ZMapStrand strand, int *start, int *end, int *query_start, int *query_end)
{
  gboolean result = FALSE ;

  if (strand == ZMAPSTRAND_UP)
    {
      if ((start && end) && start > end)
	{
	  int tmp ;

	  tmp = *start ;
	  *start = *end ;
	  *end = tmp ;

	  if (query_start && query_end)
	    {
	      tmp = *query_start ;
	      *query_start = *query_end ;
	      *query_end = tmp ;
	    }

	  result = TRUE ;
	}
    }
  else
    result = TRUE ;


  return result ;
}



void zmapFeatureDump(ZMapFeatureContext feature_context, char *file, int format)
{
  GIOChannel *channel;
  GError     *channel_error = NULL;
  char       *base_dir = (char *)g_get_home_dir();  /* do not free after use */
  char       *filepath = g_strdup_printf("%s/%s", base_dir, file);

  
  /* open output file */
  if (!(channel = g_io_channel_new_file(filepath, "w", &channel_error)))
    {
      zMapShowMsg(ZMAP_MSG_EXIT, "Can't open output file: %s :%s\n", filepath, channel_error->message);
      g_error_free(channel_error);
    }
  else
    {
      /* dump out the data */
      if (format == TAB_DELIMITED)
	{
	  contextDump_TD(feature_context, channel);
	}


      /* close output file */
      if (channel)
	{
	  if (!g_io_channel_shutdown(channel, TRUE, &channel_error))
	    {
	      zMapShowMsg(ZMAP_MSG_EXIT, "Error closing output file: %s :%s\n", filepath, channel_error->message);
	      g_error_free(channel_error);
	    }
	  else 
	    zMapShowMsg(ZMAP_MSG_INFORMATION, "Sequence successfully dumped to file\n");
	}
    }
  g_free(filepath);

  return;
}


static void contextDump_TD(ZMapFeatureContext feature_context, GIOChannel *channel)
{
  GString *line;

  if (feature_context)
    {
      line  = g_string_sized_new(150);
      g_string_printf(line, "%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", 
		      feature_context->sequence, 
		      feature_context->parent,
		      feature_context->parent_span.x1,
		      feature_context->parent_span.x2,
		      feature_context->sequence_to_parent.p1,
		      feature_context->sequence_to_parent.p2,
		      feature_context->sequence_to_parent.c1,
		      feature_context->sequence_to_parent.c2,
		      feature_context->features_to_sequence.p1,
		      feature_context->features_to_sequence.p2,
		      feature_context->features_to_sequence.c1,
		      feature_context->features_to_sequence.c2
		      );

      /* Only proceed if there's no problem printing the line */
      if (printLine(channel, line->str) == TRUE)
	g_datalist_foreach(&(feature_context->feature_sets), setDump_TD, channel);

      g_string_free(line, FALSE);
    }

  return;
}


static void setDump_TD(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data;
  GIOChannel *channel = (GIOChannel*)user_data;
  GString *line;

  if (feature_set)
    {
      line = g_string_sized_new(50);
      g_string_printf(line, "%s\n", feature_set->source);

      /* Only proceed if there's no problem printing the line */
      if (printLine(channel, line->str) == TRUE)
	g_datalist_foreach(&(feature_set->features), featureDump_TD, channel);
    
      g_string_free(line, FALSE);
    }
  return;
}


static void featureDump_TD(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data;
  GIOChannel *channel = (GIOChannel*)user_data;
  char *type, *strand, *phase;
  GString *line;
  gboolean unused;

  if (feature)
    {
      line = g_string_sized_new(150);
      type   = zmapLookUpEnum(feature->type, TYPE_ENUM);
      strand = zmapLookUpEnum(feature->strand, STRAND_ENUM);
      phase  = zmapLookUpEnum(feature->phase, PHASE_ENUM);

      g_string_printf(line, "%d\t%s\t%s\t%d\t%d\t%d\t%s\t%s\t%s\t%f", 
		      feature->id,
		      feature->name,
		      type,
		      feature->x1,
		      feature->x2,
		      feature->method,
		      feature->method_name,
		      strand,
		      phase,
		      feature->score
		      );
      if (feature->text)
	{
	  g_string_append_c(line, '\t');
	  g_string_append  (line, feature->text);
	}

      /* all these extra fields not required for now.
      *if (feature->type == ZMAPFEATURE_HOMOL)
      *{
      * line = g_strdup_printf("%s\t%d\t%d\t%d\t%d\t%d\t%f\t", line,
      *			 feature->feature.homol.type,
      *			 feature->feature.homol.y1,
      *			 feature->feature.homol.y2,
      *			 feature->feature.homol.target_strand,
      *			 feature->feature.homol.target_phase,
      *			 feature->feature.homol.score
      *			 );   
      * for (i = 0; i < feature->feature.homol.align->len; i++)
      *   {
      *     align = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i);
      *     line = g_strdup_printf("%s\t%d\t%d\t%d\t%d", line,
      *			     align->q1,
      *			     align->q2,
      *		     align->t1,
      *			     align->t2
      *			     );
      *   }
      *
      *else if (feature->type == ZMAPFEATURE_TRANSCRIPT)
      *{
      *  line = g_strdup_printf("%s\t%d\t%d\t%d\t%d\t%d", line,
      *			 feature->feature.transcript.cdsStart,
      *			 feature->feature.transcript.cdsEnd,
      *			 feature->feature.transcript.start_not_found,
      *			 feature->feature.transcript.cds_phase,
      *			 feature->feature.transcript.endNotFound
      *			 );   
      *  for (i = 0; i < feature->feature.transcript.exons->len; i++)
      *    {
      *      exon = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
      *      line = g_strdup_printf("%s\t%d\t%d", line,
      *			     exon->x1,
      *			     exon->x2
      *			     );
      *    }
      *}
      */

      g_string_append_c(line, '\n');
      unused = printLine(channel, line->str);

      g_string_free(line, FALSE);
    }


  return;
}



static gboolean printLine(GIOChannel *channel, gchar *line)
{
  gboolean status = TRUE;
  gsize bytes_written;
  GError *channel_error = NULL;


  if (channel)
    {
      if (!g_io_channel_write_chars(channel, line, -1, &bytes_written, &channel_error))
	{
	  zMapShowMsg(ZMAP_MSG_EXIT, "Error writing to output file: %30s :%s\n", line, channel_error->message);
	  g_error_free(channel_error);
	  status = FALSE;
	}
    }

  return status;
}

/****************************** end of file *******************************/



