/*  File: zmapViewFeatureCollapse.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *                NOTE see docs/IntWeb/Design/notes/BAM.shtml
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
#include <zmapView_P.h>



#define SQUASH_DEBUG 0
#define SQUASH_DEBUG_CONC 0



/*
 * NOTE:
 * joined reads may appear shorter due to mnmatech regions at the end of the 76 bases
 * even tho' the legnth is 76 the alignment may be much shorter
 */



/* how to do join?

  splice coordinates show where overlapping reads can be compressed into one
  but we are at the mercy of slightly flaky data.

  - overlap if no splice coord is contained
  ? overlap if splice coord less than some thereshold
  - overlap if splice overlap is the same

  we have some high volume regions with low volume noise, gaps less than a small threshold
  (eg 1 or 2 bases, sometimes 4)
  - discount splice coords then < 4 apart (8?)
  ... Adam says 38 bases is smallest physically possible taking into accoun the bending of RNA and suggests using 50


  we can get splice wobble due to random chance, so a splice coordinate can cover several bases, let-s choose +/-4 ??
  - overlap if does not cross min and max coordinate of splice coord
*/

#define MIN_INTRON_LEN 50
#define MAX_WOBBLE	4	/* unlucky mismatched bases 1 chance in 16 */




static ZMapFeatureContextExecuteStatus collapseNewFeatureset(GQuark key, gpointer data, gpointer user_data,
							     char **error_out);
static int makeConcensusSequence(ZMapFeature composite) ;
static void addCompositeFeature(GHashTable *hash, ZMapFeature composite, ZMapFeature feature,
				int y1, int y2, int len) ;
static GList *compressStrand(GList *features, GHashTable *hash, gboolean squash, gboolean collapse, int join);
static gboolean canSquash(ZMapFeature first, ZMapFeature current);
static gint featureGapCompare(gconstpointer a, gconstpointer b) ;
static int makeGaps(ZMapFeature composite, ZMapFeature feature,
		    GList **splice_list, double y1, double y2, double edge1, double edge2);
static GList *squashStrand(GList *fl, GHashTable *hash,  GList **splice_list);
static GList *collapseJoinStrand(GList *fl, GHashTable *hash, GList *splice_list, gboolean collapse, int join);
static void storeSpliceCoords(ZMapFeature feature, GList **splice_list);
static int splice_sort(gconstpointer ga,gconstpointer gb) ;
static void dumpFeaturesCB(gpointer data, gpointer user_data_unused) ;







/* collapse squash and join simple reads into composite features where these overlap meaningfully */
gboolean zMapViewCollapseFeatureSets(ZMapView view, ZMapFeatureContext diff_context)
{
  gboolean result = TRUE ;

  zMapFeatureContextExecute((ZMapFeatureAny) diff_context,
			    ZMAPFEATURE_STRUCT_FEATURESET,
			    collapseNewFeatureset,
			    NULL) ;

  return result ;
}



/* simple concensus sequence for a composite feature */
/* taking the most common base and not translating variable positions */
/* initially annotators will be warned to check dbSNP or 1000 genomes */
/* #warning interim solution for concensus sequence */

# if 0
Some data from GFF:

chr17-03	Tier1_K562_cell_longPolyA_rep2	similarity	27047872	27047911	.	+	.	Class "Sequence" ; Name "SINATRA_0006:5:34:19391:19990#0" ; Align 5 44 + ; Length 76 ; percentID 100 ; sequence CCGGCCAAAGAGCCTCGGAAGAGCGCTCCCAGGAGCAAAAAGTCTGACCACTAGGCTACCGTAACGATTCCGCTGA

So i guess this means that the match starts on the 5th base in the sequence at genomic coordinate 27047872
#endif




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

  zMapReturnValIfFail((feature_any && zMapFeatureIsValid(feature_any)), ZMAP_CONTEXT_EXEC_STATUS_ERROR) ;

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
	gboolean collapse, squash;
	int join;

	feature_set = (ZMapFeatureSet)feature_any;
	style = feature_set->style;

	if(!style || zMapStyleGetMode(style) != ZMAPSTYLE_MODE_ALIGNMENT)
	  break;

	squash = zMapStyleIsSquash(style);
	join =  zMapStyleJoinOverlap(style);
	collapse = join ? FALSE : zMapStyleIsCollapse(style);

#if SQUASH_DEBUG
	zMapLogMessage("join squash collapse: %d %d %d\n",join,squash,collapse);
#endif

	if(!collapse && !squash && !join)
	  break;


	zMapLogMessage("NEW FEATURE SET: \"%s\"", g_quark_to_string(feature_set->original_id)) ;


	zMap_g_hash_table_get_data(&features, feature_set->features) ;

	features = fl = g_list_sort(features, featureGapCompare) ;
	/* debug...check the sorting..... */
	zMapLogMessage("%s", "After sort by featureGapCompare") ;
	g_list_foreach(features, dumpFeaturesCB, NULL) ;



	/*
	 * features are sorted first by strand so we do the compositing in two stages
	 * not two passes: one scan of the data with a break at half time
	 * each part does squash first to get splice coordinates, then join and/or collapse
	 */

	/* NOTE the idea was to do a single scan of the list of features
	 * the the code might be clearer if coded explicitly as an automaton
	 * with an explict state variable
	 * oh well.... next time maybe
	 */
	fl = compressStrand(fl, feature_set->features, squash, collapse, join);
	compressStrand(fl, feature_set->features, squash, collapse, join);

	if(features)
	  g_list_free(features);

	break;
      }

    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
        zMapWarnIfReached() ;
	break;
      }
    }

  return status;
}





/* process features in the list till the end ot till strand changes
 *
 * squash first (features with gaps are at the front) then do join or collapse (features without gaps follow)
 * composite features are added to the hash table and have a list of child features
 * and have a concensus sequence and 1-based query coordinates.
 */


static GList *compressStrand(GList *features, GHashTable *hash, gboolean squash, gboolean collapse, int join)
{
  GList  *splice_list = NULL;	/* sorted list of splice coordinates */

  /* if we join exon reads these must be between splice coordinates,
   * but can also get noise in introns & alternate splicing
   * we sort the features so that gaps come first, that way we know where the splicing is
   * then we get the non gapped reads and join up between these if they overlap by enough
   */

  if (features && squash)
    features = squashStrand(features, hash, &splice_list) ;


  /* debug...check the sorting..... */
  zMapLogMessage("%s", "After squashStrand()") ;
  g_list_foreach(features, dumpFeaturesCB, NULL) ;


  if(features && (collapse || join))
    features = collapseJoinStrand(features, hash, splice_list, collapse, join);

  if(splice_list)
    g_list_free(splice_list);

  return features;
}



/* --------------------------------------------------------------------------------------- */
/* squash features
 *
 * this works on gapped features which appear first
 * we break if the strand changes or we get an ungapped feature or the list ends
 */
static GList *squashStrand(GList *fl, GHashTable *hash,  GList **splice_list)
{
  double y1,y2,edge1,edge2;	/* boundaries of visible squashed feature */
  ZMapFeature composite = NULL;
  ZMapFeature feature = NULL;
  ZMapFeature f;
  gboolean squash_this;
  ZMapFeature next_f;		/* required due to trying to 'nice-ify' the code */

  while(fl)
    {
      f = (ZMapFeature) fl->data;
      f->population = 1;
      squash_this = FALSE;

#if SQUASH_DEBUG
      { int i;
	ZMapAlignBlock ab;
	GArray *f_gaps = f->feature.homol.align;

	zMapLogMessage("feature: %s %d,%d\n",g_quark_to_string(f->original_id), f->x1,f->x2);

	if (f_gaps) for(i = 0;i < f_gaps->len; i++)
	  {
	    ab = & g_array_index(f_gaps,ZMapAlignBlockStruct,i);
	    zMapLogMessage("feature block  %d,%d , query %d,%d\n", ab->t1,ab->t2,ab->q1,ab->q2);
	  }
      }
#endif

      if(feature)
	squash_this = canSquash(feature,f);

      if(squash_this && !composite)
	{
	  composite = g_new0(ZMapFeatureStruct,1);
	  memcpy(composite,feature,sizeof(ZMapFeatureStruct));

	  feature->flags.squashed = TRUE;

	  composite->children = g_list_prepend(NULL, feature);
	  feature->composite = composite;
	}

      if(composite && zMapStyleJoinMax(*feature->style) && composite->population >= zMapStyleJoinMax(*feature->style))
	{
	  squash_this = FALSE;		/* leave these bumpable an give more indication of volume */
	  f->flags.squashed = FALSE;	/* make visible */
	}


      /* save list of source features for blixem */
      if(squash_this && composite)
	{
	  composite->children = g_list_prepend(composite->children,f);
	  composite->population++;
	  f->composite = composite;
	}


      if(squash_this)
	{
	  /* adjust replacement gaps array data */
	  /* as the gaps are identical these values are outside any gapped region */
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
#if SQUASH_DEBUG
	  zMapLogMessage("squash this: y, f, edge = %.1f,%.1f (%.1f) %d,%d  %.1f,%.1f\n",y1,y2, y2 - y1, f->x1,f->x2,edge1,edge2);
#endif

	  f->flags.squashed = TRUE;
	}


      /* EASY TO UNDERSTAND.....ERRRR..... */

      /* on account of a code review we decided that a function was too long
       * so i split the long function into easy to understand small ones
       * which means that i have to write extra code to cope with the re-organisation
       * NOTE this kind of edge effect is quite easy to miss and 'nice' code can be a source of bugs
       */

      /* crazy logic....are these tests mutually exclusive or what..... */

      fl = fl->next;
      next_f = fl ? (ZMapFeature) fl->data : NULL;
      if(next_f && (!next_f->feature.homol.align || next_f->feature.homol.align->len == 1))
	next_f = NULL;
      if(next_f && feature && next_f->strand != feature->strand)
	next_f = NULL;

      /* finished compressing this feature: tidy up and get ready to test the next */
      if(!squash_this || !next_f)
	{
	  if(composite)
	    {
	      int len ;

	      len = makeGaps(composite, feature, splice_list, y1, y2, edge1, edge2);


	      zMapLogMessage("Adding feature %s (%d, %d) to composite with y1 = %d, y2 = %d",
			     g_quark_to_string(feature->original_id),
			     feature->x1, feature->x2, y1, y2) ;

	      addCompositeFeature(hash, composite, feature, y1, y2, len);
	    }
	  else
	    {
	      storeSpliceCoords(f, splice_list);
	    }

	  composite = NULL;

	  /* set up for next unique feature or series of matches */
	  feature = f;

	  edge1 = y1 = feature->x1;
	  edge2 = y2 = feature->x2;
	}

      if(!next_f)
	break;
    }

  return fl;
}


/* does the current feature have the same gaps as the previous ones? */
gboolean canSquash(ZMapFeature first, ZMapFeature current)
{
  GArray *g1 = first->feature.homol.align, *g2 = current->feature.homol.align;

  if(!first || zMapStyleGetMode(*first->style) != ZMAPSTYLE_MODE_ALIGNMENT)
    return FALSE;

  /* test gaps equal for both squash and collapse */
  if(g1 && g2)
    {
      /* if there are gaps they must be identical */
      if(g1->len != g2->len)
	return FALSE;

      /* If there are fewer than 2 alignment blocks then there is no gap!
       * (We shouldn't get this because non-gapped alignments should be
       * filtered out before we get here, but double-check because this would
       * let non-overlapping blocks slip through and cause crashing in makeGaps.) */
      zMapWarnIfFail(g1->len > 1) ;
      if(g1->len < 2)
        return FALSE;

      int i;
      ZMapAlignBlock a1, a2;
      /* the AlignBlocks are the matches
       * we want to compare the gaps between
       * there is almost always exactly two matches
       * so let's just plod through it
       */

      /* NOTE must test target not query as query may be diff */

      for(i = 0;i < g1->len; i++)
	{
	  a1 = &g_array_index(g1, ZMapAlignBlockStruct, i);
	  a2 = &g_array_index(g2, ZMapAlignBlockStruct, i);

	  if(i)
	    {
	      if(a1->t1 != a2->t1)
		return FALSE;
	    }
	  if(i < g1->len -1)
	    {
	      if(a1->t2 != a2->t2)
		return FALSE;
	    }
	}

      return TRUE;
    }

  return FALSE;
}









/* process features in the list till the end or till strand changes
 *
 * squash first (features with gaps are at the front) then do join or collapse (features without gaps follow)
 * composite features are added to the hash table and have a list of child features
 * and have a concensus sequence and 1-based query coordinates.
 */


static GList *collapseJoinStrand(GList *fl, GHashTable *hash, GList *splice_list, gboolean collapse, int join)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* NOT INITIALISED AND WRONG TYPE....EXCELLENT.... */

  double y1,y2;	/* boundaries of visible feature */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  int y1, y2 ;
  ZMapStrand curr_strand ;
  ZMapFeature composite = NULL;
  ZMapFeature feature = NULL;
  gboolean collapse_this;
  gboolean join_this;
  gboolean duplicate;
  ZMapFeature f;
  ZMapFeature next_f;
  GList *l_splice = NULL;
  int prev_splice = 0;
  int next_splice = 0;
  gboolean hit_splice_fwd = FALSE;
  gboolean hit_splice_bwd = FALSE;




  /* if we join exon reads these must be between splice coordinates,
   * but can also get noise in introns & alternate splicing
   * we sort the features so that gaps come first, that way we know where the splicing is
   * then we get the non gapped reads and join up between these if they overlap by enough
   */

  if(splice_list)
    {
      l_splice = g_list_sort(splice_list,splice_sort);

#if SQUASH_DEBUG
      {
	GList *l;
	for(l = l_splice; l; l = l->next)
	  zMapLogMessage("splice at %d\n",GPOINTER_TO_UINT(l->data));
      }
#endif

      next_splice = GPOINTER_TO_UINT(l_splice->data) + MAX_WOBBLE;
      l_splice = l_splice->next;
    }

  curr_strand = ZMAPSTRAND_NONE ;
  y1 = y2 = 0 ;
  while(fl)
    {
      f = (ZMapFeature) fl->data ;
      f->population = 1 ;

      collapse_this = join_this = FALSE ;


      {
	gboolean overlap = FALSE ;

	/* probably we should set overlap true if y1/y2 are zero..... */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	if ((f->x1 >= y1 && f->x1 <= y2) || (f->x2 >= y1 && f->x2 <= y2))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  if ((y1 == 0 && y2 == 0 && curr_strand == ZMAPSTRAND_NONE)
	      || (f->strand == curr_strand && ((f->x1 >= y1 && f->x1 <= y2) || (f->x2 >= y1 && f->x2 <= y2))))
	    {
	      if (curr_strand == ZMAPSTRAND_NONE)
		curr_strand = f->strand ;

	      overlap = TRUE ;
	    }
	  else
	    {
	      overlap = FALSE ;

	    }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	zMapDebugPrintf("y1, y2 now: %d, %d\n", y1, y2) ;
	zMapDebugPrintf("Next feature (%s) has %s: %d, %d\n",
			g_quark_to_string(f->original_id),
			(overlap ? "OVERLAP" : "NO OVERLAP"),
			f->x1, f->x2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	zMapLogMessage("y1, y2 now: %d, %d", y1, y2) ;
	zMapLogMessage("Next feature (%s) has %s: %d, %d",
		       g_quark_to_string(f->original_id),
		       (overlap ? "OVERLAP" : "NO OVERLAP"),
		       f->x1, f->x2) ;
      }


      if(feature)
	{
	  if (join)
	    {

	      /* Here is where we get into trouble....sorting means this test doesn't work
	       * and actually I'm beginning to think it can't work.....the sorting needs
	       * revisiting...exactly what order do we want.....work this out a bit better. */

	      /* these are in start coord order so only 1 overlap is possible */
	      if (y2 - f->x1 >= join)
		join_this = TRUE;

	      // else we get single feature stutter: must process next splice even if we decided no join
	      //				if(join_this)
	      {
		/* only join if no overlap of a splice coord */

		if (next_splice && f->x2 > next_splice)
		  {
		    if(!hit_splice_fwd)		/* break on splice, then continue going till over it */
		      {
			join_this = FALSE;
#if SQUASH_DEBUG
			zMapLogMessage("join stopped on next splice %d\n", next_splice);
#endif
		      }

		    hit_splice_fwd = TRUE;
		  }


#if 0
		as we immediately move the next splice fwds, we will unset the flag
		else
		  {
		    if(hit_splice)
		      {

			//							join_this = FALSE;
#if SQUASH_DEBUG
			zMapLogMessage("join started after splice %d\n", next_splice);
#endif
		      }
		    hit_splice = FALSE;
		  }
#endif



		while(next_splice && f->x2 >= next_splice)
		  {
		    if(next_splice)
		      prev_splice = next_splice - MAX_WOBBLE * 2;

		    if(l_splice)
		      {
			next_splice = GPOINTER_TO_UINT(l_splice->data) + MAX_WOBBLE;
			l_splice = l_splice->next;
		      }
		    else
		      {
			next_splice = 0;
		      }

		    hit_splice_fwd = FALSE;
		    hit_splice_bwd = FALSE;

#if SQUASH_DEBUG
		    zMapLogMessage("next splice set to %d, %d\n",prev_splice, next_splice);
#endif
		  }

		if(prev_splice && f->x1 > prev_splice)
		  {
		    // trailing egde of feature has crossed the last splice coord
		    if(!hit_splice_bwd)
		      {
			join_this = FALSE;
#if SQUASH_DEBUG
			zMapLogMessage("join stopped on prev splice %d\n", prev_splice);
#endif
		      }
		    hit_splice_bwd = TRUE;
		  }


	      }

	      if(join_this)
		{
		  if(f->x2 > y2)
		    y2 = f->x2;

#if SQUASH_DEBUG
		  zMapLogMessage("join this:  %.1f %.1f\n",y1,y2);
#endif

		  f->flags.joined = TRUE;
		}
	    }
	  else if(collapse)
	    {
	      if(f->x1 == feature->x1 && f->x2 == feature->x2)
		{
		  collapse_this = TRUE;
		  f->flags.collapsed = TRUE;
		}
	    }
	}

      duplicate = join_this || collapse_this;

      if(duplicate && !composite)
	{
	  composite = g_new0(ZMapFeatureStruct,1);

	  memcpy(composite,feature,sizeof(ZMapFeatureStruct));

	  if(collapse_this)
	    feature->flags.collapsed = TRUE;
	  else if(join_this)
	    feature->flags.joined = TRUE;

	  composite->children = g_list_prepend(NULL, feature);
	  feature->composite = composite;
	}

      if(composite && zMapStyleJoinMax(*feature->style) && composite->population >= zMapStyleJoinMax(*feature->style))
	{
	  duplicate = FALSE;		/* leave these bumpable and give more indication of volume */
	  f->flags.joined = FALSE;	/* else will not be displayed */
	}

      if(duplicate && composite)
	{
	  /* save list of source features for blixem */
	  composite->children = g_list_prepend(composite->children, f) ;
	  composite->population++ ;
	  f->composite = composite ;
	}


      /* on account of a code review we decided that a function was too long
       * so i split the long function into easy to understand small ones
       * which means that i have to write extra code to cope with the re-organisation
       * NOTE this kind of edge effect is quite easy to miss and 'nice' code can be a source of
       * bugs
       * 
       * YES AND YOUR CODE IS BUGGED BECAUSE THIS IS SO LONG YOU'VE LOST TRACK OF THE EDGE
       * EFFECTS....
       * 
       */
      fl = fl->next;
      next_f = fl ? (ZMapFeature) fl->data : NULL;

      if(next_f && feature && next_f->strand != feature->strand)
	next_f = NULL ;

      /* finished compeessing this feature: tidy up and get ready to test the next */
      if(!duplicate || !next_f)
	{
	  if (composite)
	    {
	      /* debug...check the sorting..... */
	      zMapLogMessage("%s:\t%s\t%d\t%d",
			     "COMPOSITE",
			     zMapFeatureStrand2Str(composite->strand),
			     composite->x1, composite->x2) ;
	      g_list_foreach(composite->children, dumpFeaturesCB, composite) ;

	      zMapLogMessage("Adding feature %s (%d, %d) to composite with y1 = %d, y2 = %d",
			     g_quark_to_string(feature->original_id),
			     feature->x1, feature->x2, y1, y2) ;

	      addCompositeFeature(hash, composite, feature, y1, y2, y2 - y1);
	    }

	  /* HOW EXACTLY DOES THIS FIT WITH THE STATEMENT ABOVE ???? */
	  composite = NULL ;

	  /* set up for next unique feature or series of matches */
	  feature = f ;

	  y1 = feature->x1 ;
	  y2 = feature->x2 ;
	}

      if(!next_f)
	break ;
    }

  return fl;
}



/* add any kind of compressed feature to the set's hash table */
static void addCompositeFeature(GHashTable *hash, ZMapFeature composite, ZMapFeature feature, int y1, int y2, int len)

{
  char buf[256];

  sprintf(buf,"%d_reads_%s",composite->population,g_quark_to_string(feature->unique_id));
  composite->unique_id = g_quark_from_string(buf);
  sprintf(buf,"Composite_%d_reads",composite->population);
  composite->original_id = g_quark_from_string(buf);

  g_hash_table_insert(hash, GUINT_TO_POINTER(composite->unique_id), composite);


  composite->x1 = y1;
  composite->x2 = y2;

  composite->feature.homol.y1 = 1;
  composite->feature.homol.y2 = len + 1;

  composite->url = NULL;	/* in case it gets freed */

#if SQUASH_DEBUG
  zMapLogMessage("composite: %s %d,%d (%d %d %d)\n", 
	 g_quark_to_string(composite->original_id),
	 composite->x1,composite->x2,
	 composite->flags.joined,composite->flags.squashed,composite->flags.collapsed);
#endif

  composite->feature.homol.length = makeConcensusSequence(composite);

#if SQUASH_DEBUG
  if(composite->feature.homol.length != strlen(composite->feature.homol.sequence))
    zMapLogMessage("seq lengths differ: %d, %d\n",composite->feature.homol.length,strlen(composite->feature.homol.sequence));
#endif

  if(composite->feature.homol.sequence)
    zMapWarnIfFail((composite->feature.homol.length == strlen(composite->feature.homol.sequence))) ;

  return ;
}


static int makeConcensusSequence(ZMapFeature composite)
{
  int result = 0 ;

  /* composite feature must have sequence. */
  if ((composite->feature.homol.sequence))
    {
      int n_seq ;
      enum {N_ALPHABET = 5} ;
      static int *bases = NULL, *bp;
      static int n_bases = 0;
      static char index[256] = { 0 };
      ZMapFeature f;
      int i;
      char *seq;
      char *base = "nacgt";
      GList *fl;


#if SQUASH_DEBUG
  zMapLogMessage("setting seq_len to %d\n",n_seq);
#endif


      n_seq = composite->x2 - composite->x1 + 1;


      if(composite->feature.homol.align)
	{
	  /* no sequence in the gap so dionlt need as much space */
	  ZMapAlignBlock ab;

	  i = composite->feature.homol.align->len - 1;

	  ab = & g_array_index(composite->feature.homol.align,ZMapAlignBlockStruct,i);

#if SQUASH_DEBUG_CONC
	  zMapLogMessage("setting seq_len: was %d now %d, (%d)\n",n_seq,ab->q2 + 1,i);
#endif

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

      memset(bases, 0, sizeof(int) * n_bases * N_ALPHABET);

      for(fl = composite->children; fl ; fl = fl->next)
	{
	  f = (ZMapFeature) fl->data;

	  if(!f->feature.homol.sequence)	/* some features have sequence but not this ome */
	    continue;

	  seq = f->feature.homol.sequence + f->feature.homol.y1 - 1;	/* skip un-matching sequence */

	  i = f->x1 - composite->x1;


#if SQUASH_DEBUG_CONC
	  zMapLogMessage("s %*s%s ",i,"",seq);
#endif


	  for(; i < n_seq && *seq ; i++)
	    {
	      int i_val = i * N_ALPHABET ;
	      int index_val = index[(int)*seq] ;
	      int orig_calc, my_calc ;

	      my_calc = i_val + index_val ;
	      orig_calc = i * N_ALPHABET + index[(int)*seq] ;

	      if (orig_calc < 0)
		{
		  zMapLogWarning("bases not updated, bad index: %d, probably caused by bad i value %d,"
				 " (f->x1 = %d, composite->x1 = %d)",
				 orig_calc, i, f->x1, composite->x1) ;
		}
	      else
		{
		  bases[i * N_ALPHABET + index[(int)*seq++]] ++ ;
		}
	    }



#if SQUASH_DEBUG_CONC
	  zMapLogMessage("-> %d\n",i);
#endif
	}

      /* must not free old sequence as it's copied from a real feature */
      composite->feature.homol.sequence = seq = g_malloc(n_seq + 1);
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

#if SQUASH_DEBUG_CONC
      zMapLogMessage("c %s\n",composite->feature.homol.sequence);
#endif



      result = seq - composite->feature.homol.sequence ;
    }

  return result ;
}



static int makeGaps(ZMapFeature composite, ZMapFeature feature, GList **splice_list,
		    double y1, double y2, double edge1, double edge2)
{
  int q_len = 0 ;
  GArray *new_gaps ;


  zMapReturnValIfFail((feature->feature.homol.align), q_len) ;

  /* NOTE squashed is set on the features that are not displayed, nottheo one that is */
  if (composite->flags.squashed_start || composite->flags.squashed_end)
    {
      int i;
      ZMapAlignBlock first;
      ZMapAlignBlock edge = NULL;
      int diff;

#if SQUASH_DEBUG
      zMapLogMessage("%s", "\nsquashing...\n");

      { int i;
	ZMapAlignBlock ab;
	GArray *f_gaps = feature->feature.homol.align;

	zMapLogMessage("feature: %s %d,%d\n",g_quark_to_string(feature->original_id), feature->x1,feature->x2);

	if (f_gaps) for(i = 0;i < f_gaps->len; i++)
	  {
	    ab = & g_array_index(f_gaps,ZMapAlignBlockStruct,i);
	    zMapLogMessage("feature block  %d,%d , query %d,%d\n", ab->t1,ab->t2,ab->q1,ab->q2);
	  }
      }

#endif
      /* update the gaps array */

      /* NOTE we add an alternate gaps array to the feature
       * to preserve the original we need to add a new composite feature
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
      int diff1, diff2;		/* feature shrinks by this much as the edge approaches the gap */
      int q = 1;

      /* NOTE almost always there will be exactly two items but this works for all cases */

      new_gaps = g_array_sized_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct), f_gaps->len + 2);

      /* NOTE: edge 1,2 are in the common part of the block */

      extra1 = feature->x1 - y1;
      extra2 = y2 - feature->x2;
      diff1 = edge1 - feature->x1;
      diff2 = feature->x2 - edge2;
#if SQUASH_DEBUG

      zMapLogMessage("y1,y2, egde1,2 = %.1f,%.1f %.1f,%.1f\n",y1,y2,edge1,edge2);
      zMapLogMessage("(extra,diff) = %d,%d %d,%d\n", extra1,diff1, extra2, diff2);

#endif

      first = & g_array_index(f_gaps,ZMapAlignBlockStruct,0);

      if(extra1 || diff1)
	{
	  edge = g_new0(ZMapAlignBlockStruct,1);
	  diff = edge1 - y1;	// worm=bame have single base matches!, start == end


	  /* THIS FAILS WHEN YOU GET MATCHES OUTSIDE OF THE COMPOSITE.....
	   * AND WHAT'S AN ASSERT DOING HERE ANYWAY... */
	  zMapWarnIfFail(diff >= 0) ;


	  /* target blocks are always fwd strand coords */
	  /* this is not immediately obvious, but Ed says it will work like this */

	  edge->t1 = y1;
	  edge->t2 = y1 + diff;
	  edge->t_strand = first->t_strand;
	  edge->q_strand = first->q_strand;

	  /* I think this struct spans from the start to somewhere in 
	   * the first align block of the composite feature,
	   * which would make the boundaries as folows. */
	  edge->start_boundary = ALIGN_BLOCK_BOUNDARY_EDGE;
	  edge->end_boundary = ALIGN_BLOCK_BOUNDARY_MATCH;

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
	  diff = edge2 - ab->t1;


	  /* AND ANOTHER ONE.....OH DEAR..... */
	  zMapWarnIfFail(diff >= 0) ;



	  edge->t1 = ab->t1 + diff + 1;
	  edge->t2 = y2;
	  edge->t_strand = ab->t_strand;
	  edge->q_strand = ab->q_strand;

	  /* I think this struct spans from somewhere in the last
	   * align block to the end of the feature, which would
	   * make the boundaries as follows. */
	  edge->start_boundary = ALIGN_BLOCK_BOUNDARY_MATCH;
	  edge->end_boundary = ALIGN_BLOCK_BOUNDARY_EDGE;

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
      q_len = q - 1;

      composite->feature.homol.align = new_gaps;

      new_gaps = NULL;

#if SQUASH_DEBUG
      { int i;
	ZMapAlignBlock ab;
	GArray *f_gaps = feature->feature.homol.align;
	int failed = 0;

	zMapLogMessage("new feature %d,%d\n",feature->x1,feature->x2);
	for(i = 0;i < f_gaps->len; i++)
	  {
	    int q1,q2;

	    ab = & g_array_index(f_gaps,ZMapAlignBlockStruct,i);
	    zMapLogMessage("feature block  %d,%d , query %d,%d\n", ab->t1,ab->t2,ab->q1,ab->q2);

	    /* now do some automated QA */
	    /* test that t1 -> t2 == q1 -> q2 */
	    /* test that query coords are contiguous */
	    /* (this was written for a more complicated versio of the code above) */

	    if(ab->q_strand == ab->t_strand)
	      {
		if((ab->t2 - ab->t1) != (ab->q2 - ab->q1))
		  {
		    zMapLogMessage("failed: fwd block %d query and target are different lengths (%d %d)\n",i,ab->t2 - ab->t1, ab->q2 - ab->q1 );
		    failed++;
		  }

		if(i && (ab->q1 - q2) != 1)
		  {
		    zMapLogMessage("failed: fwd block %d query is not contiguous (%d,%d -> %d)\n",i, ab->q1, q2,ab->q1 - q2);
		    failed++;
		  }
	      }
	    else
	      {
		if((ab->t2 - ab->t1) != (ab->q2 - ab->q1))
		  {
		    zMapLogMessage("failed: rev block %d query and target are different lengths (%d %d)\n",i,ab->t2 - ab->t1, ab->q2 - ab->q1 );
		    failed++;
		  }

		if(i && (q1 - ab->q2) != 1)
		  {
		    zMapLogMessage("failed: rev block %d query is not contiguous (%d,%d -> %d)\n",i, q1, ab->q2, q1 - ab->q2);
		    failed++;
		  }
	      }
	    q1 = ab->q1;
	    q2 = ab->q2;
	  }
	if(!failed)
	  zMapLogMessage("%s tested ok\n\n",(ab->q_strand == ab->t_strand) ? "fwd" : "rev");
	else
	  zMapLogMessage("%s", "\n");
      }

#endif
    }
  else if(feature->feature.homol.align)
    {
      /* squashed but no variable regions */
      /* (collapsed and joined features have no gaps array) */

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
      q_len = ab->q2;



      composite->feature.homol.align = ga;
    }

  /* store the splice coordinates, using feature as we don't want to include start and end regions */
  storeSpliceCoords(feature, splice_list);

  return q_len;
}








/* store the splice coordinates - feature may be composite or lonely uncompressed one */
static void storeSpliceCoords(ZMapFeature feature, GList **splice_list)
{
  if(feature->feature.homol.align)
    {
      ZMapAlignBlock ab;
      int i;
      GArray *ga;
      int last = -MIN_INTRON_LEN;

      ga = feature->feature.homol.align;
      for(i = 0; i < ga->len; i++)
	{
	  ab = & g_array_index(ga,ZMapAlignBlockStruct,i);
	  if(i > 0 && (ab->t1 - last) >= MIN_INTRON_LEN)
	    {
	      *splice_list = g_list_prepend(*splice_list,GUINT_TO_POINTER(last));
	      *splice_list = g_list_prepend(*splice_list,GUINT_TO_POINTER(ab->t1));

#if SQUASH_DEBUG
	      zMapLogMessage("gap %d add splice a %d\n",i,last);
	      zMapLogMessage("gap %d add splice b %d\n",i,ab->t1);
#endif
	    }
	  last = ab->t2;
	}
      /* we prepend as that's more efficient and we have to sort anyway
       * as we could have multiple splices which could get out of order (esp zebrafish)
       * and also we can get wobble on splice junctions due to bad luck
       */
    }
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static gint featureGapCompare(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  
  ZMapFeature feata = (ZMapFeature) a;
  ZMapFeature featb = (ZMapFeature) b;	/* these must be alignments */
  GArray *g1,*g2;
  ZMapAlignBlock ab1=NULL, ab2=NULL;
  int ng1 = 0, ng2 = 0;

  /* this compare function is horrid! */

  /* good job we only do this once per featureset */


  /* THIS SURELY HAS TO SAY THAT THERE IS SOMETHING DEEPLY WRONG WITH THE LIST, NOT WITH
   * GLIB....... */

  /* we can get NULLs due to GLib being silly */
  /* this code is pedantic, but I prefer stable sorting */
  if(!featb)
    {
      if(!feata)
	result = 0 ;

      result = 1 ;
    }
  else if(!feata)
    {
      result = -1 ;
    }
  else if(feata->strand < featb->strand)
    {
      result = -1 ;
    }
  else if(feata->strand > featb->strand)
    {
      result = 1 ;
    }
  else
    {
      /* THERE'S A MASSIVE PROBLEM HERE.....THE NUMBER OF GAPS IS IRRELEVALENT IF THE ALIGNS
       * DON'T EVEN OVERLAP....IT'S HARD TO EVEN UNDERSTAND WHAT THE SORT IS TRYING TO ACHIEVE... */
      
      /* gb10: I think the aim of this routine is to place alignments with identical splice
       * sites adjacent to each other. It first sorts by the number of gaps (in reverse order),
       * so alignments with the most gaps come first and ungapped alignments come last. This
       * effectively groups alignments with the same number of gaps. Then it sorts by splice
       * site coords to group alignments with identical splice sites next to each other. */
      
      /* Note that the align block struct holds the matches, so the gap is between them */
      g1 = feata->feature.homol.align ;
      g2 = featb->feature.homol.align ;

      if(g1)
        ng1 = g1->len ;
      if(g2)
        ng2 = g2->len ;
      
      /* Sort by the number of gaps (reverse order) */
      result = ng2 - ng1 ;

      /* If identical, also sort by coords */
      if(!result)
        {
          if (ng1 > 1) /* must have at least 2 match blocks for there to be a gap between them! */
            {
              /* Gapped alignments */
              
              /* Loop through each alignment block and check the splice sites match.
               * Note that the start/end of the first/last block don't need checking because
               * they're not splice sites. Continue searching through while matches are
               * identical i.e. while result is zero. */
              int i = 0;
              for ( ; i < ng1 && !result; ++i)
                {
                  ab1 = &g_array_index(g1, ZMapAlignBlockStruct, i);
                  ab2 = &g_array_index(g2, ZMapAlignBlockStruct, i);
                  
                  /* sort on the start coord of the alignment block (unless it's the first block) */
                  if (i > 0)
                    result = ab1->t1 - ab2->t1 ;
              
                  /* If identical, also sort on the end coord of the alignment block
                   * (unless it's the last block) */
                  if (!result && i < ng1 - 1)
                    result = ab1->t2 - ab2->t2 ;
                }
            }
          else
            {
              /* Ungapped alignments - just sort by feature coords */

              /* Sort by start coord */
              result = feata->x1 - featb->x1 ;

              /* If identical, also sort by end end coord */
              if (!result)
                result = feata->x2 - featb->x2 ;
            }
        }
    }
  
  return result ;
}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* ok...let's rewrite this to do proper compares...??
 * 
 * SEE ABOVE ROUTINE FOR ORIGINAL....
 * 
 * I'M LEAVING THIS IN UNTIL I GET FEEDBACK FROM ADAM/LAURENS...
 * 
 * 
 *  */
static gint featureGapCompare(gconstpointer a, gconstpointer b)
{
  int result = 0 ;
  ZMapFeature feata = (ZMapFeature) a;
  ZMapFeature featb = (ZMapFeature) b;	/* these must be alignments */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* MALCOLM TESTED FOR NULL PTRS WHICH IS SURELY CRAZY.....
   * THIS SURELY HAS TO SAY THAT THERE IS SOMETHING DEEPLY WRONG WITH THE LIST, NOT WITH
   * GLIB.......THIS IS DEEPLY DISTURBING AS IT IMPLIES THE LISTS ARE RUBBISH....
   * 
   * HIS COMMENT WAS..... */
  /* we can get NULLs due to GLib being silly */
  /* this code is pedantic, but I prefer stable sorting */
  zMapAssert(a && b) ;

  /* I'VE REPLACED IT WITH THE NEW TEST MACRO BUT THIS REALLY NEEDS SORTING OUT.... */

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  zMapReturnValIfFail((a && b), result) ;




  /* Let's try a new sort...first by strand, then overall start/end then by gaps. */
  if (feata->strand == ZMAPSTRAND_FORWARD && featb->strand == ZMAPSTRAND_REVERSE)
    {
      result = -1 ;
    }
  else if(feata->strand == ZMAPSTRAND_REVERSE && featb->strand == ZMAPSTRAND_FORWARD)
    {
      result = 1 ;
    }
  else if (feata->x1 < featb->x1)
    {
      result = -1 ;
    }
  else if (feata->x1 > featb->x1)
    {
      result = 1 ;
    }
  else if (feata->x2 < featb->x2)
    {
      result = -1 ;
    }
  else if (feata->x2 > featb->x2)
    {
      result = 1 ;
    }
  else if (feata->feature.homol.percent_id > featb->feature.homol.percent_id)
    {
      result = -1 ;
    }
  else if (feata->feature.homol.percent_id < featb->feature.homol.percent_id)
    {
      result = 1 ;
    }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* ISN'T ALL THIS A BIT BRAIN DEAD...??? SURELY COVERAGE MATTERS MORE.... */

  else if ((g1 = feata->feature.homol.align) && (g2 = featb->feature.homol.align))
    {
      ZMapAlignBlock a1, a2 ;
      int ng1 = 0, ng2 = 0 ;
      GArray *g1, *g2 ;
  
      ng1 = g1->len ;

      ng2 = g2->len ;

      if(ng1 < ng2)	/* only compare gaps if there are the same number */
	result =  1;
      if(ng1 > ng2)
	result =  -1;

      a1 = &g_array_index(g1, ZMapAlignBlockStruct, 0);
      a2 = &g_array_index(g2, ZMapAlignBlockStruct, 0);

      if(a1->t2 < a2->t2)
	result = (1);
      if(a1->t2 > a2->t2)
	result = (-1);

      a1 = &g_array_index(g1, ZMapAlignBlockStruct, 1);
      a2 = &g_array_index(g2, ZMapAlignBlockStruct, 1);
      if(a1->t1 < a2->t1)
	result = (-1);
      if(a1->t1 > a2->t1)
	result = (1);
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* UM....ISN'T THIS THE WRONG WAY ROUND....PERHAPS IT'S MEANT TO BE THAT WAY... */
static int splice_sort(gconstpointer ga, gconstpointer gb)
{
  int result, int_a, int_b ;

  int_a = GPOINTER_TO_UINT(ga) ;
  int_b = GPOINTER_TO_UINT(gb) ;

  result = int_a - int_b ;

  return result ;
}


static void dumpFeaturesCB(gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapFeature composite = (ZMapFeature)user_data ;
  int start, end, num_gaps = 0 ;
  char *strand ;
  gboolean gaps ;
  char *gap_str ;
  char *strand_diff = "", *overlap = "" ;

  start = feature->x1 ;
  end = feature->x2 ;
  strand = zMapFeatureStrand2Str(feature->strand) ;
  if (!(feature->feature.homol.align))
    {
      gap_str = "no gaps" ;
      gaps = FALSE ;
    }
  else if (feature->feature.homol.align->len == 0)
    {
      gap_str = "GAP ARRAY BUT NO GAPS !" ;
      gaps = FALSE ;
    }
  else
    {
      gap_str = "gaps" ;
      gaps = TRUE ;
      num_gaps = feature->feature.homol.align->len ;
    }

  if (composite)
    {
      if (feature->strand != composite->strand)
	strand_diff = "OPPOSITE STRAND" ;

      if (start > composite->x2 || end < composite->x1)
	overlap = "NO OVERLAP" ;
    }

  zMapLogMessage("%s:\t%s\t%d\t%d\t%s\tnum_gaps: %d\t%s\t%s",
		 g_quark_to_string(feature->original_id),
		 strand,
		 start, end,
		 gap_str, num_gaps,
		 strand_diff, overlap) ;

  return ;
}
