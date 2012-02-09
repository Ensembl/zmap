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


// collaspe similar features into one
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
		gboolean collapse, squash;
		GArray *new_gaps = NULL;	/* to replace original in visible squashed feature */

		if(!style)
		break;

		collapse = zMapStyleIsCollapse(style);
		squash   = zMapStyleIsSquash(style);

		if(!collapse && !squash)
			break;

		/* NOTE: rules of engagement as follows
		 *
		 * collapse combines identical features into one, they have identical start end, and gaps if present
		 * squash combines features with identical gaps (must be present) and same or varied start and end
		 * squash can only be set for alignments, collapse can be set for simple features too
		 *
		 * if both are selected then squash overrides collapse, but only for gapped features
		 * so we only scan the features once
		 *
		 * squashed and collapsed features are flagged as such and the visible one has a population count
		 *
		 * visible squashed features have extra gaps data with contiguous regions at start and end (both optional)
		 * these get displayed in a diff colour
		 * but to compare gaps arrays for equeality we can only add these on afterwards :-(
		 *
		 * if we want to be flash we could do many of these to have a colour gradient according to populatiom
		 * NOTE * that would be complicated *
		 *
		 * NOTE I used some old fashioned coding methodology here,
		 * The idea is to do just what is required, taking into account squash and/or collapse,
		 * feature types and gaps array status, while doing a single _linear_ scan.
		 * There is no assumption that all features are of the same type
		 * Take great care to be aware of all cases if you modify this.
		 * Quite a few statements rely on implied status of some flags.
		 * I've added some Asserts for documentation purposes.
		 */

		/* sort into start, -end coord order */
		zMap_g_hash_table_get_data(&features, feature_set->features);
		features = g_list_sort(features,zMapFeatureCompare);


		/* flag unique features as displayed, duplicated as hidden */
		/* count up how many */
		for(fl = features; fl; fl = fl->next)
		{
			gboolean squash_this = FALSE;		/* can only be set to TRUE for alignemnts */
			gboolean collapse_this = collapse;
			gboolean duplicate = TRUE;
			/* we only squash or collapse if duplicate is TRUE, how depends on the other flags */

			f = (ZMapFeature) fl->data;
			duplicate = TRUE;
			if(!feature)
				duplicate = FALSE;

			if(duplicate)
			{
				if(f->x1 != feature->x1 || f->x2 != feature->x2)
				{
					collapse_this = FALSE;	/* must be identical */
					if(!squash)
					{
						// zMapAsset(collapse);
						duplicate = FALSE;
					}
				}

				if(f->type == ZMAPSTYLE_MODE_ALIGNMENT)
				{
					/* compare gaps array */
					/* if there are gaps the must be identical */
					/* collapse is ok without gaps */

					GArray *g1 = feature->feature.homol.align, *g2 = f->feature.homol.align;

					/* test gaps equal for both squash and collapse */
					if(g1 && g2)
					{
						/* if there are gaps they must be identical */
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
					else if(squash || g1 || g2)
					{
						/* if there are no gaps then both must be ungapped for collapse */
						/* squash requires gaps */
						duplicate = FALSE;
					}
					if(duplicate && squash)		/* has prioriy over collapse */
					{
						squash_this = TRUE;
					}
				}
			}

			if(!duplicate)	/* get ready to test the next */
			{
				if(feature && (feature->flags.squashed_start || feature->flags.squashed_end))
				{
					zMapAssert(squash && new_gaps);
					/* update the gaps array */

					/* NOTE interim implementation
					 * we alter the visible feature in situ and cannot goback to the original
					 * later we'll add a new feature to allow alternate styled displays
					 * eg of squashed and raw data
					 */

					g_array_free(feature->feature->feature.homol.align,TRUE);
					feature->feature->feature.homol.align = new_gaps;
				}
				else if(new_gaps)
				{
					g_array_free(new_gaps,TRUE);
					new_gaps = NULL;
				}

				feature = f;
				feature->population = 1;

				if(squash && f->feature->feature.homol.align)
				{
					/* prepare a new replacement gaps array */
					int i;
					GArray *f_gaps = f->feature->feature->homol.align;
//					ZMapAlignBlock ab;

					new_gaps = g_array_sized_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct), f_gaps->len + 2);
					for(i = 0;i < f_gaps->len; i++)
					{
//						ab = &g_array_index(f_gaps,ZMapAlignBlockStruct,i);
						g_array_append_val(new_gaps, g_array_index(f_gaps,ZMapAlignBlockStruct,i));
					}
					/* not very satisfactory: we will prepend the start in almost all cases */
				}

			}
			else
			{
				if(squash_this)
				{
					/* adjust replacement gaps array */

					feature->population++;
					f->flags.squashed = TRUE;
				}
				else
				{
					zMapAssert(collapse);
					feature->population++;
					f->flags.collapsed = TRUE;
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



