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
 *                NOTE see Design_notes/notes/BAM.shtml
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



#if 0
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
#endif

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

	if(feata->strand < featb->strand)
		return(-1);
	if(feata->strand > featb->strand)
		return(1);

	/* compare gaps array (first gap only), no gaps means move to the end and compare start coord */
	/* as the align block struct holds the aligns the gap is between then :-o */
	g1 = feata->feature.homol.align;
	g2 = featb->feature.homol.align;

	if(!g2)
	{
		if(!g1)
		{
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
		return(1);
	}
	if(!g1)
		return(-1);

	a1 = &g_array_index(g1, ZMapAlignBlockStruct, 0);
	a2 = &g_array_index(g2, ZMapAlignBlockStruct, 0);

	if(a1->t2 < a2->t2)
		return(-1);
	if(a1->t2 > a2->t2)
		return(1);

	a1 = &g_array_index(g1, ZMapAlignBlockStruct, 1);
	a2 = &g_array_index(g2, ZMapAlignBlockStruct, 1);
	if(a1->t1 < a2->t1)
		return(-1);
	if(a1->t1 > a2->t1)
		return(1);

	return(0);
}


/* simple concensus sequence for a composite feature */
/* taking the most common base and not translating variable positions */
/* initially annotators will be warned to check dbSNP or 1000 genomes */
#warning interim solution for concensus sequence

# if 0
Some data from GFF:

chr17-03	Tier1_K562_cell_longPolyA_rep2	similarity	27047872	27047911	.	+	.	Class "Sequence" ; Name "SINATRA_0006:5:34:19391:19990#0" ; Align 5 44 + ; Length 76 ; percentID 100 ; sequence CCGGCCAAAGAGCCTCGGAAGAGCGCTCCCAGGAGCAAAAAGTCTGACCACTAGGCTACCGTAACGATTCCGCTGA

So i guess this means that the match starts on the 5th base in the sequence at genomic coordinate 27047872
#endif

void make_concensus_sequence(ZMapFeature composite,GList *fl, int n_feat, ZMapFeatureSet fs)
{
	int n_seq = composite->x2 - composite->x1 + 1;
#define N_ALPHABET	5
	static int *bases = NULL, *bp;
	static int n_bases = 0;
	static char index[256] = { 0 };
	ZMapFeature f;
	int i, nb;
	char *seq;
	char *base = "nacgt";


	if(composite->feature.homol.align)
	{
		/* no sequence in the gap so dionlt need as much space */
		ZMapAlignBlock ab;
		i = composite->feature.homol.align->len - 1;
		ab = & g_array_index(composite->feature.homol.align,ZMapAlignBlockStruct,i);
		n_seq = ab->q2 + 1;
	}

#if STUPID
/* will fail if not in set */
#define BASE(x) (((x | 0x20) - 0x61)>> 1) 	/* map ACGT to 0,2,6,3 then to 0,1,3,2 */
#endif
	if(!index['a'])
	{
		index['a'] = index['A'] = 1;
		index['c'] = index['C'] = 2;
		index['g'] = index['G'] = 3;
		index['t'] = index['T'] = 4;
	}

	if(n_bases < n_seq)
	{
		if(bases)
			g_free(bases);
		n_bases = n_seq + 10;	/* prevent petty re-alloc's */
		bases = g_malloc(sizeof(int) * n_bases * N_ALPHABET);
	}

	memset(bases,0,sizeof(int) * n_bases * N_ALPHABET);

	for(; n_feat--; fl = fl->prev)
	{
		f = (ZMapFeature) fl->data;
		seq = f->feature.homol.sequence + f->feature.homol.y1 - 1;	/* skip un-matching sequence */
		i = f->x1 - composite->x1;
		for(; i < n_seq && *seq; i++)
		{
			bases[i * N_ALPHABET + index[(int)*seq++]] ++;
		}
	}

	/* must not free old sequence as it's copied from a real feature */
	composite->feature.homol.sequence = seq = g_malloc(n_seq + 10);
	for(bp = bases, i = 0; i < n_seq; i++)
	{
		int base_ind, max, j;
		base_ind = max = 0;

		for(j = 0;j < N_ALPHABET;j++, bp++)
		{
			if(*bp > max)
			{
				base_ind = j;
				max = *bp;
			}
		}
		*seq++ = base[base_ind];
	}
	*seq = 0;

}


#define SQUASH_DEBUG	0
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
		gboolean collapse = FALSE, squash = FALSE;
		int join = 0;
		GArray *new_gaps = NULL;	/* to replace original in visible squashed feature */
		double y1,y2,edge1,edge2;	/* boundaries of visible squashed feature */
		GList *children;
		ZMapFeature composite = NULL;
		gboolean squash_this;		/* can only be set to TRUE for alignemnts */
		gboolean collapse_this;
		gboolean duplicate;

//		GHashTable *splice_hash;	/* accumulate all splice coordinates */
//		GList  *splice_list;		/* sorted list of splice coordinates */

		if(!style)
			break;

		if(zMapStyleGetMode(style) != ZMAPSTYLE_MODE_ALIGNMENT)
			break;

		squash   = zMapStyleIsSquash(style);
		join 	   =  zMapStyleJoinOverlap(style);
		if(!join)
			collapse = zMapStyleIsCollapse(style);	/* collaspe is like join_overlap == feature length */

		if(!collapse && !squash && !join)
			break;

		/* NOTE: rules of engagement as follows
		 *
		 * collapse combines identical features into one, they have identical start end, and gaps if present
		 * squash combines features with identical gaps (must be present) and same or varied start and end
		 * squash can only be set for alignments, collapse can be set for simple features too
		 * join will amalgamate overlapping ungapped reads between splice coordinates
		 * and assumes that these have been sorted to appear after gapped ones in the list of features.
		 *
		 * if both are selected then squash overrides collapse, but only for gapped features
		 * so we only scan the features once
		 *
		 * squashed and collapsed features are flagged as such and the visible one has a population count
		 * (the visible one is a new feature added by this code)
		 *
		 * visible squashed features have extra gaps data with contiguous regions at start and end (both optional)
		 * if we want to be flash we could do many of these to have a colour gradient according to populatiom
		 * NOTE * that would be complicated and slow *
		 *
		 * NOTE Quite a few statements rely on implied status of some flags or data.
		 * I've added some Asserts for documentation purpose *and* some helpful comments
		 */

		/* NOTE
		 * adding blocks to the gaps array turned out to be a nightmnare due to query coords being reversed sometimes
		 * 1-based coords of course make life tedious too
		 * mismatches at the start/ end of a read also cause grief
		 * I'd like to rewrite this :-(
		 * and will have to to move it to the canvas anyway.
		 *
		 * so instead of ading match blocks and adjusting coords we assume full homology and recreate the gaps array from scratch
		 * not all reads start at 1 or end at the end
		 */

		/* NOTE initially i'd adjusted one feature in situ bbut not this code added a new one for composite features */

		/* sort into start, -end coord order */
		/* sort by first gap, if no gaps sort by x1,x2: we get two groups of features */
		/* NOTE it's important that ungapped reads go to the end, to allow joining bewteen splice junctions */

		zMap_g_hash_table_get_data(&features, feature_set->features);
		features = g_list_sort(features,zMapFeatureGapCompare);



		/* flag unique features as displayed, duplicated as hidden */
		/* count up how many */
		for(fl = features; fl; fl = fl->next)
		{
			squash_this = FALSE;		/* can only be set to TRUE for alignemnts */
			collapse_this = collapse;
			/* we only squash or collapse if duplicate is TRUE, how depends on the other flags */
			duplicate = TRUE;

			f = (ZMapFeature) fl->data;
			f->population = 1;

			if(!feature)
				duplicate = FALSE;

#if SQUASH_DEBUG
{ int i;
ZMapAlignBlock ab;
GArray *f_gaps = f->feature.homol.align;

printf("feature: %s %d,%d\n",g_quark_to_string(f->original_id), f->x1,f->x2);

if (f_gaps) for(i = 0;i < f_gaps->len; i++)
{
	ab = & g_array_index(f_gaps,ZMapAlignBlockStruct,i);
	printf("feature block  %d,%d , query %d,%d\n", ab->t1,ab->t2,ab->q1,ab->q2);
}
}
#endif

			if(duplicate)
			{
				if(f->x1 != feature->x1 || f->x2 != feature->x2)
				{
					collapse_this = FALSE;	/* must be identical */
					if(!squash)
					{
//						// zMapAssert(collapse);
						duplicate = FALSE;
					}
				}
			}

//				if(f->type == ZMAPSTYLE_MODE_ALIGNMENT)
			if(duplicate)
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
						/* the AlignBlocks are the matches
							* we want to compare the gaps between
							* there is almost always exactly two matches
							* so let's just plod through it
							*/

						/* NOTE must test target not query as query will be diff */

						for(i = 0;i < g1->len; i++)
						{
							a1 = &g_array_index(g1, ZMapAlignBlockStruct, i);
							a2 = &g_array_index(g2, ZMapAlignBlockStruct, i);

							if(i)
							{
								if(a1->t1 != a2->t1)
								{
									duplicate = FALSE;
									break;
								}
							}
							if(i < g1->len -1)
							{
								if(a1->t2 != a2->t2)
								{
									duplicate = FALSE;
									break;
								}

							}
						}
					}
					if(duplicate && squash)		/* has prioriy over collapse */
					{
						squash_this = TRUE;	/* only place this gets set */
					}
				}
				else if(g1 || g2)
				{
					/* if there are no gaps then both must be ungapped for collapse */
					/* squash requires gaps */
					duplicate = FALSE;
				}

				/* collapse_this may be true here (gaps or no gaps) */
				if(!collapse_this && !squash_this)
					duplicate = FALSE;
			}

			if(duplicate)
			{
				if(!composite)
				{
//					zMapAssert(collapse_this || squash_this);

					composite = g_new0(ZMapFeatureStruct,1);
					memcpy(composite,feature,sizeof(ZMapFeatureStruct));
					if(collapse_this)
						feature->flags.collapsed = TRUE;
					else
						feature->flags.squashed = TRUE;

					composite->composite = g_list_prepend(NULL, feature);
				}
				/* save list of source features for blixem */
				composite->composite = g_list_prepend(composite->composite,f);

				if(squash_this)
				{
					/* adjust replacement gaps array data */
					/* as the gaps are identical these values are outside any gapped region */
#if SQUASH_DEBUG
printf("squash this: y, f, edge = %.1f,%.1f %d,%d %.1f,%.1f\n",y1,y2,f->x1,f->x2,edge1,edge2);
#endif
					if(f->x1 < y1)
					{
						y1 = f->x1;
						composite->flags.squashed_start = 1;
					}
					if(f->x1 > edge1)
					{
						edge1 = f->x1;
						composite->flags.squashed_start = 1;
					}
					if(f->x2 > y2)
					{
						y2 = f->x2;
						composite->flags.squashed_end = 1;
					}
					if(f->x2 < edge2)
					{
						edge2 = f->x2;
						composite->flags.squashed_end = 1;
					}

					composite->population++;
					f->flags.squashed = TRUE;
				}
				else if(collapse_this)
				{
					/* need to test collapse_this as duplicate could have been preserved to allow squash */
#if SQUASH_DEBUG
printf("collapse this\n");
#endif
					composite->population++;
					f->flags.collapsed = TRUE;
				}
			}

			if(!duplicate || !fl->next)	/* tidy up and get ready to test the next */
			{
					/* NOTE squashed is set on the features that are not displayed, nottheo one that is */
				if(composite && (composite->flags.squashed_start || composite->flags.squashed_end))
				{
					int i;
					ZMapAlignBlock first;
					ZMapAlignBlock edge = NULL;
					int diff;

					zMapAssert(squash && feature->feature.homol.align);

#if SQUASH_DEBUG
printf("\nsquashing...\n");

{ int i;
ZMapAlignBlock ab;
GArray *f_gaps = feature->feature.homol.align;

printf("feature: %s %d,%d\n",g_quark_to_string(f->original_id), feature->x1,feature->x2);

if (f_gaps) for(i = 0;i < f_gaps->len; i++)
{
	ab = & g_array_index(f_gaps,ZMapAlignBlockStruct,i);
	printf("feature block  %d,%d , query %d,%d\n", ab->t1,ab->t2,ab->q1,ab->q2);
}
}

#endif
					/* update the gaps array */

					/* NOTE we add an alternate gaps array to the feature
					 * to preserve the origonal we need to add a new composite feature
					 * so we could display the real features w/ a diff style
					 * beware interactions with canvasAlignment.c where y2 gets changed to paint homology lines
					 * but as we don't do this with style alignment-unique it should be ok.
					 */

					/* prepare a new replacement gaps array */
					/* just do the target first them make up the query
					 * as there's too many variants it's hopeless trying to adjust it OTF
					 * we force the query to start at 1
					 */

					GArray *f_gaps = feature->feature.homol.align;
					ZMapAlignBlock ab;
					int extra1, extra2;	/* added to the ends */
					int diff1, diff2;		/* featrue shrinks by this much as the edge approaches the gap */
					int q = 1;

					/* NOTE almost always there will be exactly two items but this works for all cases */

					new_gaps = g_array_sized_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct), f_gaps->len + 2);

					/* NOTE: edge 1,2 are in the common part of the block */

					extra1 = feature->x1 - y1;
					extra2 = y2 - feature->x2;
					diff1 = edge1 - feature->x1;
					diff2 = feature->x2 - edge2;
#if SQUASH_DEBUG

printf("y1,y2, egde1,2 = %.1f,%.1f %.1f,%.1f\n",y1,y2,edge1,edge2);
printf("(extra,diff) = %d,%d %d,%d\n", extra1,diff1, extra2, diff2);

#endif

					first = & g_array_index(f_gaps,ZMapAlignBlockStruct,0);

					if(extra1 || diff1)
					{
						edge = g_new0(ZMapAlignBlockStruct,1);
						diff = edge1 - y1 - 1;
						zMapAssert(diff >= 0);

						/* target blocks are always fwd strand coords */
						/* this is not immediately obvious, but Ed says it will work like this */

						edge->t1 = y1;
						edge->t2 = y1 + diff;
						edge->t_strand = first->t_strand;
						edge->q_strand = first->q_strand;

						g_array_append_val(new_gaps,*edge);
					}

					/* add first feature match block and adjust */
					g_array_append_val(new_gaps,*first);
					first = &g_array_index(new_gaps,ZMapAlignBlockStruct,1);
					first->t1 = edge1;

					/* add remaining match blocks copied from feature, normally only 1 but zebrafish has a few triple reads */
					for(i = 1;i < f_gaps->len; i++)
					{
						g_array_append_val(new_gaps, g_array_index(f_gaps,ZMapAlignBlockStruct,i));
					}
					ab = & g_array_index(new_gaps,ZMapAlignBlockStruct,new_gaps->len - 1);

					if(extra2 || diff2)
					{
						edge = g_new0(ZMapAlignBlockStruct,1);
						diff = edge2 - ab->t1 - 1;
						zMapAssert(diff >= 0);

						edge->t1 = ab->t1 + diff + 1;
						edge->t2 = y2;
						edge->t_strand = ab->t_strand;
						edge->q_strand = ab->q_strand;

						g_array_append_val(new_gaps,*edge);
						ab->t2 = ab->t1 + diff;
					}

					if(first->q_strand == first->t_strand)
					{
						for(i = 0,q = 1; i < new_gaps->len; i++)
						{
							ab = & g_array_index(new_gaps,ZMapAlignBlockStruct,i);
							ab->q1 = q;
							q += ab->t2 - ab->t1;
							ab->q2 = q++;
						}
					}
					else
					{
						for(i =  new_gaps->len,q = 1; i > 0;)
						{
							i--;
							ab = & g_array_index(new_gaps,ZMapAlignBlockStruct,i);
							ab->q1 = q;
							q += ab->t2 - ab->t1;
							ab->q2 = q++;
						}
					}

					composite->feature.homol.align = new_gaps;

					new_gaps = NULL;

#if SQUASH_DEBUG
{ int i;
ZMapAlignBlock ab;
GArray *f_gaps = feature->feature.homol.align;
int failed = 0;

printf("new feature %d,%d\n",feature->x1,feature->x2);
for(i = 0;i < f_gaps->len; i++)
{
	int q1,q2;

	ab = & g_array_index(f_gaps,ZMapAlignBlockStruct,i);
	printf("feature block  %d,%d , query %d,%d\n", ab->t1,ab->t2,ab->q1,ab->q2);

	/* now do some automated QA */
	/* test that t1 -> t2 == q1 -> q2 */
	/* test that query coords are contiguous */
	/* (this was written for a more complicated versio of the code above) */

	if(ab->q_strand == ab->t_strand)
	{
		if((ab->t2 - ab->t1) != (ab->q2 - ab->q1))
		{
			printf("failed: fwd block %d query and target are different lengths (%d %d)\n",i,ab->t2 - ab->t1, ab->q2 - ab->q1 );
			failed++;
		}

		if(i && (ab->q1 - q2) != 1)
		{
			printf("failed: fwd block %d query is not contiguous (%d,%d -> %d)\n",i, ab->q1, q2,ab->q1 - q2);
			failed++;
		}
	}
	else
	{
		if((ab->t2 - ab->t1) != (ab->q2 - ab->q1))
		{
			printf("failed: rev block %d query and target are different lengths (%d %d)\n",i,ab->t2 - ab->t1, ab->q2 - ab->q1 );
			failed++;
		}

		if(i && (q1 - ab->q2) != 1)
		{
			printf("failed: rev block %d query is not contiguous (%d,%d -> %d)\n",i, q1, ab->q2, q1 - ab->q2);
			failed++;
		}
	}
	q1 = ab->q1;
	q2 = ab->q2;
}
if(!failed)
	printf("%s tested ok\n\n",(ab->q_strand == ab->t_strand) ? "fwd" : "rev");
else
	printf("\n");
}

#endif
				}
				else if(composite && feature->feature.homol.align)
				{
					/* squashed but no variable regions */
					/* collapsed features have no gaps array */

					/* must copy the GArray as other functions that refer to it by macro move the address
					 * if we squashed then we already replaced it
					 */
					ZMapAlignBlock ab;
					int i;
					GArray *ga,*gf;

					gf = feature->feature.homol.align;
					ga = g_array_sized_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct), gf->len);

					for(i = 0; i < gf->len; i++)
					{
						ab = & g_array_index(gf,ZMapAlignBlockStruct,i);
						g_array_append_val(ga,*ab);
					}

					composite->feature.homol.align = ga;
				}


				if(composite)
				{
					char buf[256];
					sprintf(buf,"%d_reads_%s",composite->population,g_quark_to_string(feature->unique_id));
					composite->unique_id = g_quark_from_string(buf);
					sprintf(buf,"Composite_%d_reads",composite->population);
					composite->original_id = g_quark_from_string(buf);

					g_hash_table_insert(feature_set->features, GUINT_TO_POINTER(composite->unique_id), composite);

					composite->x1 = y1;
					composite->x2 = y2;

					composite->url = NULL;	/* in case it gets freed */
#if SQUASH_DEBUG
printf("composite: %s %d,%d\n", g_quark_to_string(composite->original_id),composite->x1,composite->x2);
#endif
					make_concensus_sequence(composite,duplicate ? fl : fl->prev, composite->population, feature_set);
//if(composite->feature.homol.sequence) composite->feature.homol.sequence = g_strdup(composite->feature.homol.sequence);
				}

				composite = NULL;

				/* set up for next unique feature or series of matches */
				feature = f;

//				if(squash)	 y1,y2 needed for collapse too
				{
					edge1 = y1 = feature->x1;
					edge2 = y2 = feature->x2;
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




