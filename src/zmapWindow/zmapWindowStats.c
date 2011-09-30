/*  File: zmapWindowStats.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 * Description: Functions for window statistics, e.g. number of boxes drawn.
 *              Note that individual feature stats are updated in the
 *              item factory code which "knows" how many boxes get drawn
 *              for each feature.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapIO.h>
#include <zmapWindow_P.h>


/* Stats are held in the Containers of the canvas window, this is because the stats capture
 * not only the numbers of features but also the numbers of boxes drawn etc. */


/*
 * Stats struct for each set of stats for each container.
 * (Opaque pointer declared in zmapWindow_P.h).
 */
typedef struct _ZMapWindowStatsStruct
{
  /* The basic stats... */
  int num_context_children ;				    /* e.g. how many blocks, feature sets etc. */
  int num_canvas_children ;				    /* e.g. how many canvas items to
							       represent context children. */

  GQuark feature_id ;					    /* i.e. name of block/align etc. */

  /* List of ZMapWindowStatsXXX structs, this is NULL if there are no features.
   * Note that we need a list here if we are support multiple feature types in each column. */
  GList *child_sets ;

} ZMapWindowStatsStruct ;




static void resetStats(gpointer data, gpointer user_data_unused) ;
static void printStats(gpointer data, gpointer user_data_unused) ;
static gint feature2StyleCompare(gconstpointer a, gconstpointer b) ;



/*
 *                  External functions
 */


ZMapWindowStats zmapWindowStatsCreate(ZMapFeatureAny feature_any)
{
  ZMapWindowStats stats ;
  int nbytes = sizeof(ZMapWindowStatsStruct) ;

  stats = g_slice_alloc0(nbytes) ;

  stats->feature_id = feature_any->original_id ;

  return stats ;
}


/* Note that we only record extra stats for any features added, stuff like blocks/aligns etc
 * do not get their own stats currently....... */
ZMapWindowStatsAny zmapWindowStatsAddChild(ZMapWindowStats stats, ZMapFeatureAny feature_any)
{
  ZMapWindowStatsAny stats_any = NULL ;

  /* Increment overall stats. */
  stats->num_context_children++ ;
  stats->num_canvas_children++ ;

#if MH17_THIS_CONTAINS_VERY_SLOW_LIST_APPEND_CALLS_FOR_LARGE_FEATURESETS
/* eg for 200k trembl we scna the whiole list 200k times, twice */
/* but the caller needs the return */
#endif
  /* If its a zmapfeature then some extra stats are recorded. */
  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      GList *stats_list ;
      ZMapFeature feature = (ZMapFeature)feature_any ;

      zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_FEATURE)) ;

      if ((stats_list = g_list_find_custom(stats->child_sets, (ZMapFeature)feature_any, feature2StyleCompare)))
	{
	  stats_any = (ZMapWindowStatsAny)(stats_list->data) ;
	}
      else
	{
	  int num_bytes = 0 ;

	  switch (feature->type)
	    {
	    case ZMAPSTYLE_MODE_ASSEMBLY_PATH:

	    case ZMAPSTYLE_MODE_BASIC:
	    case ZMAPSTYLE_MODE_SEQUENCE:
          case ZMAPSTYLE_MODE_GRAPH:
	    case ZMAPSTYLE_MODE_GLYPH:
	    case ZMAPSTYLE_MODE_TEXT:
	      num_bytes = sizeof(ZMapWindowStatsBasicStruct) ;
	      break ;
	    case ZMAPSTYLE_MODE_ALIGNMENT:
	      num_bytes = sizeof(ZMapWindowStatsAlignStruct) ;
	      break ;
	    case ZMAPSTYLE_MODE_TRANSCRIPT:
	      num_bytes = sizeof(ZMapWindowStatsTranscriptStruct) ;
	      break ;
	    case ZMAPSTYLE_MODE_META:
	    default:
	      break ;
	    }

	  /* THE NUM_BYTES BIT IS A HACK UNTIL WE'VE SORTED OUT STATS FOR ALL ITEM TYPES. */

	  if (num_bytes)
	    {
	      stats_any = g_slice_alloc0(num_bytes) ;
	      stats_any->feature_type = feature->type ;
	      stats_any->style_id = feature->style_id ;

	      stats->child_sets = g_list_append(stats->child_sets, stats_any) ;
	    }
	}
    }


  return stats_any ;
}


void zmapWindowStatsReset(ZMapWindowStats stats)
{
  stats->num_context_children = 0 ;
  stats->num_canvas_children = 0 ;

  g_list_foreach(stats->child_sets, resetStats, NULL) ;

  return ;
}



void zmapWindowStatsPrint(GString *text, ZMapWindowStats stats)
{
  g_string_append_printf(text, "%s:\tContext children: %d, canvas children: %d\n",
			 g_quark_to_string(stats->feature_id),
			 stats->num_context_children, stats->num_canvas_children);
  g_list_foreach(stats->child_sets, printStats, text) ;

  return ;
}



void zmapWindowStatsDestroy(ZMapWindowStats stats)
{
  int nbytes = sizeof(ZMapWindowStatsStruct) ;

  zMapAssert(stats) ;

  g_slice_free1(nbytes, stats) ;

  return ;
}



/*
 *                            Internal functions
 */


static void resetStats(gpointer data, gpointer user_data_unused)
{
  ZMapWindowStatsAny any_stats = (ZMapWindowStatsAny)data ;
  int num_bytes = 0 ;

  switch (any_stats->feature_type)
    {
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_SEQUENCE:
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_GLYPH:
    case ZMAPSTYLE_MODE_TEXT:
      num_bytes = sizeof(ZMapWindowStatsBasicStruct) ;
      break ;
    case ZMAPSTYLE_MODE_ALIGNMENT:
      num_bytes = sizeof(ZMapWindowStatsAlignStruct) ;
      break ;
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      num_bytes = sizeof(ZMapWindowStatsTranscriptStruct) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  memset(any_stats, 0, num_bytes) ;

  return ;
}





static void printStats(gpointer data, gpointer user_data)
{
  ZMapWindowStatsAny any_stats = (ZMapWindowStatsAny)data ;
  GString *text = (GString *)user_data ;

  switch (any_stats->feature_type)
    {
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_SEQUENCE:
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_GLYPH:
    case ZMAPSTYLE_MODE_TEXT:
      {
	ZMapWindowStatsBasic basic = (ZMapWindowStatsBasic)any_stats ;

	g_string_append_printf(text, "Basic\tfeatures:%d\t boxes:%d\n", basic->features, basic->items) ;

	break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
	ZMapWindowStatsAlign align = (ZMapWindowStatsAlign)any_stats ;

	g_string_append_printf(text, "Alignment\tfeatures:%d\tgapped:%d\tnot perfect gapped:%d\tungapped:%d\t"
			   "boxes:%d\tgapped boxes:%d\tungapped boxes:%d\tgapped boxes not drawn:%d\n",
			   align->total_matches, align->gapped_matches,
			   align->not_perfect_gapped_matches, align->ungapped_matches,
			   align->total_boxes, align->gapped_boxes, align->ungapped_boxes, align->imperfect_boxes) ;
	break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
	ZMapWindowStatsTranscript transcript = (ZMapWindowStatsTranscript)any_stats ;

	g_string_append_printf(text, "Transcript\tfeatures:%d\texons:%d,\tintrons:%d,\tcds:%d\t"
			   "boxes:%d\texon_boxes:%d\tintron_boxes:%d\tcds_boxes:%d\n",
			   transcript->transcripts, transcript->exons, transcript->introns, transcript->cds,
			   transcript->items, transcript->exon_boxes, transcript->intron_boxes, transcript->cds_boxes) ;
	break ;
      }
    default:

      /* NEEDS FIXING TO DO STATS FOR OTHER STYLE MODES.... */
//     zMapLogFatalLogicErr("switch(), unknown value: %d", any_stats->feature_type) ;
      g_string_append_printf(text,"ERROR: switch(), unknown value: %d", any_stats->feature_type);

      break ;
    }

  return ;
}


static gint feature2StyleCompare(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  ZMapWindowStatsAny stats = (ZMapWindowStatsAny)a ;
  ZMapFeature feature = (ZMapFeature)b ;

  if (stats->style_id == feature->style_id)
    result = 0 ;

  return result ;
}

