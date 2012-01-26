/*  File: zmapViewFeatureCollapse.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * Description:   collapse duplicated short reads features subject to configuration
 *                NOTE see Design_notes/notes/BAN.shtml
 *                sets flags in the context per feature to say  collapsed or not
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtils.h>
//#include <ZMap/zmapGFF.h>
#include <zmapView_P.h>



#define FILE_DEBUG      0     /* can use this for performance stats, could add it to the log? */
#define PDEBUG          zMapLogWarning




typedef struct _ZMapMaskFeatureSetData
{
      ZMapFeatureBlock block; /* that contains the current featureset */
      ZMapView view;
}
ZMapMaskFeatureSetDataStruct, *ZMapMaskFeatureSetData;


static ZMapFeatureContextExecuteStatus collapseNewFeatureset(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out);


/* mask ESTs with mRNAs if configured
 * all new features are already in view->context
 * logically we can only mask features in the same block
 * NOTE even tho' masker sets are configured that does not mean we have the data yet
 *
 * returns a lists of previously displayed featureset id's that have been masked
 */
gboolean zMapViewCollapseFeatureSets(ZMapView view, ZMapFeatureContext diff_context)
{
      ZMapMaskFeatureSetDataStruct _data = { NULL };
      ZMapMaskFeatureSetData data = &_data;

      data->view = view;

      zMapFeatureContextExecute((ZMapFeatureAny) diff_context,
                                   ZMAPFEATURE_STRUCT_FEATURESET,
                                   collapseNewFeatureset,
                                   (gpointer) data);

      return(TRUE);
}



gint zMapFeatureCompare(gconstpointer a, gconstpointer b)
{
	ZMapFeature feata = (ZMapFeature) a;
	ZMapFeature featb = (ZMapFeature) b;

	/* we can get NULLs due to GLib being silly */
	/* this code is pedantic, but I prefer stable sorting */
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


gint zMapFeatureGapCompare(gconstpointer a, gconstpointer b)
{
	ZMapFeature feata = (ZMapFeature) a;
	ZMapFeature featb = (ZMapFeature) b;	/* these must be alignments */
	GArray *g1,*g2;
	ZMapAlignBlock a1, a2;

		/* this compare function is horrid! */
		/* good job we only do this once per featureset */

	/* we can get NULLs due to GLib being silly */
	/* this code is pedantic, but I prefer stable sorting */
	if(!featb)
	{
		if(!feata)
			return(0);
		return(1);
	}
	if(!feata)
		return(-1);

	/* compare gaps array (first gap only), no gaps or more than one means move to the end  */
	/* as the align block struct holds the aligns the gap is between then :-o */
	g1 = feata->feature.homol.align;
	g2 = featb->feature.homol.align;

	if(!g2 || g2->len != 2)
	{
		if(!g1 || g1->len != 2)
			return(0);
		return(1);
	}
	if(!g1 || g1->len != 2)
		return(-1);

	if(feata->strand < featb->strand)
		return(-1);
	if(feata->strand > featb->strand)
		return(1);

	a1 = &g_array_index(g1, ZMapAlignBlockStruct, 0);
	a2 = &g_array_index(g2, ZMapAlignBlockStruct, 0);

	if(a1->q2 < a2->q2)
		return(-1);
	if(a1->q2 > a2->q2)
		return(1);

	a1 = &g_array_index(g1, ZMapAlignBlockStruct, 1);
	a2 = &g_array_index(g2, ZMapAlignBlockStruct, 1);
	if(a1->q1 < a2->q1)
		return(-1);
	if(a1->q1 > a2->q1)
		return(1);

	return(0);
}


// mask new featureset with all (old+new) data in same block
static ZMapFeatureContextExecuteStatus collapseNewFeatureset(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  ZMapFeatureTypeStyle style;
  GList *features = NULL, *fl;
  ZMapFeature feature = NULL, f;
  gboolean duplicate;

  zMapAssert(feature_any && zMapFeatureIsValid(feature_any)) ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align = NULL;
        feature_align = (ZMapFeatureAlignment)feature_any;
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock feature_block = NULL;
        feature_block = (ZMapFeatureBlock)feature_any;
      }
      break;

    case ZMAPFEATURE_STRUCT_FEATURESET:
    {
	    ZMapFeatureSet feature_set = NULL;
          feature_set = (ZMapFeatureSet)feature_any;
	    style = feature_set->style;

	    if(!style)
		break;

	    if(zMapStyleIsCollapse(style))
	    {
		/* hide duplicated features: same start and end and same gaps array if present */

		/* sort into start, -end coord order */
		zMap_g_hash_table_get_data(&features, feature_set->features);
		features = g_list_sort(features,zMapFeatureCompare);

		/* flag unique features as displayed, duplicated as hidden */
		/* count up how many */
		for(fl = features; fl; fl = fl->next)
		{
			f = (ZMapFeature) fl->data;
			duplicate = TRUE;

//if(feature)
//	zMapLogWarning("collapse %d,%d (%d) %d,%d",feature->x1,feature->x2,feature->population,f->x1,f->x2);
			if(!feature || f->x1 != feature->x1 || f->x2 != feature->x2)
				duplicate = FALSE;

			if(feature && f->type == ZMAPSTYLE_MODE_ALIGNMENT)
			{
				/* compare gaps array */
				GArray *g1 = feature->feature.homol.align, *g2 = f->feature.homol.align;

				if(g1->len != g2->len)
				{
					duplicate = FALSE;
				}
				else
				{
					int i;
					ZMapAlignBlock a1, a2;

					for(i = 0;i < g1->len; i++)
					{
						a1 = &g_array_index(g1, ZMapAlignBlockStruct, i);
						a2 = &g_array_index(g2, ZMapAlignBlockStruct, i);
						if(memcmp(a1,a2,sizeof(ZMapAlignBlockStruct)))
						{
							duplicate = FALSE;
							break;
						}
					}
				}
			}

			if(duplicate)
			{
				feature->population++;
				f->flags.collapsed = 1;
			}
			else
			{
				feature = f;
				feature->population = 1;
			}
		}
	    }

	    if(zMapStyleIsSquash(style))	/* can only be set for alignments */
	    {
		/* hide duplicated features: same start and end and same gaps array if present */
		/* NOTE: this is quite fiddly... WTH did i dream this up???
		 * there are also some interesting interactions with make_gapped() in zmapWindowCanvasAlignment.c
		 */

		GArray *g1= NULL,*g2;
		ZMapAlignBlock ab1,ab2;
		int x1,x2;	/* current feature gap coords */
		int fx1,fx2;	/* current feature gap coords */
		int edge1,edge2;	/* define common region */


		gboolean squash = FALSE;

		/* sort into gap start, -end coord order */
		zMap_g_hash_table_get_data(&features, feature_set->features);
		features = g_list_sort(features,zMapFeatureGapCompare);		/* NOTE: slow sort */

		/* flag unique features as displayed, duplicated as hidden */
		/* count up how many */
		for(fl = features, feature = NULL; fl; fl = fl->next)
		{
			f = (ZMapFeature) fl->data;
			squash = TRUE;

			if(!feature || feature->strand != f->strand)
			{
				squash = FALSE;
			}

			if(squash)
			{
				g2 = f->feature.homol.align;
				ab1 = &g_array_index(g2, ZMapAlignBlockStruct,0);
				fx1 = ab1->q1;
				ab2 = &g_array_index(g2, ZMapAlignBlockStruct,1);
				fx2 = ab2->q2;

				/* we should test all the gaps, but that's really tedious
				* so if any reads span 3 exons we don't squash them
				*/

				if(g2->len != 2)
				{
					squash = FALSE;
				}
				else
				{
					if(x1 != ab1->q2)
					{
						squash = FALSE;
					}

					if(x2 != ab2->q1)
					{
						squash = FALSE;
					}
				}

			}

			if(squash)
			{
				feature->population++;
				f->flags.squashed = 1;

				if(edge1 < fx1)
				{
					edge1 = fx1;
					feature->flags.squashed_start = 1;
				}
				if(edge2 > fx2)
				{
					edge2 = fx2;
					feature->flags.squashed_end = 1;
				}
			}
			else
			{
				if(feature)
				{
					/* update the gaps array to include grey regions at each end */
					ZMapAlignBlock edge;

					if(feature->flags.squashed_start)		/* such attention to detail :-) */
					{
						edge = g_new0(ZMapAlignBlockStruct,1);
						memcpy(edge,ab1,sizeof (ZMapAlignBlockStruct));
						edge->t2 -= edge->q2 - edge1;
						edge->q2 = edge1;
						g_array_prepend_val(g1,edge);
					}

					if(feature->flags.squashed_end)
					{
						edge = g_new0(ZMapAlignBlockStruct,1);
						memcpy(edge,ab1,sizeof (ZMapAlignBlockStruct));
						edge->t1 += edge2 - edge->q1;
						edge->q1 = edge2;
						g_array_append_val(g1,edge);
					}
				}

				feature = f;
				feature->population = 1;
				g1 = f->feature.homol.align;
				x1 = ab1->q2;
				x2 = ab2->q1;
				edge1 = fx1;
				edge2 = fx2;
			}
		}
	    }

	    if(features)
		    g_list_free(features);
	    break;
    }

    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
      zMapAssertNotReached();
      break;
      }
    }

  return status;
}



