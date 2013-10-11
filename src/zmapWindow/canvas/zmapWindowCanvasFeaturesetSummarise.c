/*  File: zmapWindowFeaturesetSummarise.c
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
 * Description:   avoids displaying features that cannot be seen at the current zoom level
 *                NOTE see Design_notes/notes/canvas_tweaks.html
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <glib.h>
#include <memory.h>

#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsLog.h>
#include <zmapWindowCanvasFeatureset_I.h>

/*
      initially this was an experimental module to find out the level of filtering we could do
      only activated in styles with alignment-summarise=1.0 or similar

      revisited after implementing CanvasFeatureset foo items: can finaly implement fully
      original file may be deleted.
      works from within the CanvasFeatureset, all features are there so can be displayed willy nilly
      the original code would hide features that would have been dislayed on top of others
      argualbly not a problem but we aim for the exact picture at lower zoom
      most important perfornance gains are at min zoom and this will operate fastest then
*/

/*
  - need to sort the features into y1 order (in the feature context)
      we assume this has been done and the features are fed in in order
      this is essential for efficiency

  - need a list of rectangles held in the window which holds the
    current 2D covered region, trimmed as it goes out of scope.

  - we also assume that the rectangles are left justified (as they are by the calling code)

  Results: we end up with 3k/ 28k features drawn. An alternative would be to compute the cover
  and draw that instead which would divide that by 3 or more
  .... this was without converting to canvas coords (oops!)

 */



static long n_summarise_hidden = 0;
static long n_summarise_list = 0;
static long n_summarise_max = 0;
static long n_summarise_show = 0;


#if PIX_LIST_DEBUG
static long n_block_alloc = 0;
static long n_pix_alloc = 0;
static long n_pix_free = 0;
static long n_list = 0;
static int pix_id = 0;
#endif


static PixRect pix_rect_free_G = NULL;

void pix_rect_free(PixRect pix)
{
	/* NOTE alloc list only has next pointers set */

#if PIX_LIST_DEBUG
	zMapAssert(pix && pix->alloc);
	printf("free %d: ",pix->which);

	pix->alloc = FALSE;
#endif

	pix->next = pix_rect_free_G;
	pix_rect_free_G = pix;

#if PIX_LIST_DEBUG
	n_list ++;
	n_pix_free++;

for(pix = pix_rect_free_G; pix; pix = pix->next)
	printf(" %d",pix->which);
printf("\n");
#endif
}

PixRect alloc_pix_rect(void)
{
	void *mem;
	int i;
	PixRect ret;

	if(!pix_rect_free_G)
	{
#if PIX_LIST_DEBUG
		zMapAssert(!n_list);
#endif

		mem = g_malloc(sizeof(pixRect) * N_PIXRECT_ALLOC);
		for(i = 0; i < N_PIXRECT_ALLOC; i++)
		{
			ret = (PixRect) mem;
#if PIX_LIST_DEBUG
			ret->alloc = TRUE;
			ret->which = pix_id++;
#endif
			pix_rect_free(ret);
			mem += sizeof(pixRect);
		}
#if PIX_LIST_DEBUG
		n_block_alloc++;
#endif
	}

	ret = pix_rect_free_G;
#if PIX_LIST_DEBUG
printf("alloc %d:",ret->which);

zMapAssert(!ret->alloc);
zMapAssert(ret->next || n_list == 1);
#endif

	pix_rect_free_G = pix_rect_free_G->next;

#if PIX_LIST_DEBUG
{
PixRect pix;
for(pix = pix_rect_free_G; pix; pix = pix->next)
	printf(" %d",pix->which);
printf("\n");
}
#endif

#if PIX_LIST_DEBUG
	ret->y1 = ret->y2 = ret->x2 = 0;
	ret->next = ret->prev = NULL;
	ret->feature = NULL;
	ret->alloc = TRUE;

	n_pix_alloc++;
	n_list--;
#else
	memset((void *) ret,0,sizeof(pixRect));
#endif


#if PIX_LIST_DEBUG
//	zMapAssert( !(((gboolean) pix_rect_free_G) ^ ((gboolean) n_list)));
	if(pix_rect_free_G) zMapAssert(n_list);
	if(n_list) zMapAssert(pix_rect_free_G);
	if(!pix_rect_free_G) zMapAssert(!n_list);
	if(!n_list) zMapAssert(!pix_rect_free_G);
#endif

	return ret;
}


/* flag feature as viz or not, maintain PixRect list of covered region */
/* called on zoom change */
/* NOTE we could optimise this to only look at the visible scroll region - do this in the calling code */

PixRect zmapWindowCanvasFeaturesetSummarise(PixRect pix, ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature)
{
  if (zmapWindowCanvasFeatureValid(feature))
    {
      FooCanvasItem *foo = (FooCanvasItem *) &featureset->__parent__;
      PixRect this;
      PixRect blocks = NULL;
      PixRect pr,del,last = NULL;
      gboolean always_gapped = (zMapStyleGetMode(featureset->style) == ZMAPSTYLE_MODE_ALIGNMENT) &&
        zMapStyleIsAlwaysGapped(featureset->style) && feature->feature->feature.homol.align;
      
      /*
       * NOTE this was originally written for simple features, and alignments split into separate features
       * for always gapped alignments (ditags and short reads) we have to expand the match blocks and process each one
       * so we have a pix_rect for each match block and each one of these points to a feature
       * this requires us to set features visible explicilty as one feature appears in several blocks
       *
       * good news is that this process can be extended to other complex features
       * eg (vulgar alignments and transcripts) when they are implemented
       */
      
      feature->flags |= FEATURE_SUMMARISED | FEATURE_HIDDEN;
      
      /* NOTE we don't care about exact canvas coords, just the size and relative position */
      
      if(always_gapped)
	{
          /* make list of blocks */
          ZMapAlignBlock ab;
          int i;
          GArray *gaps;	/* actually blocks */
          
          /* this is less efficient as we get a long dangly list of blocks at the other side of the gap */
          /* however, due to the nature of the data we will get a lot of hits */
          /* see reults at the bottom of this file */
          /*! \todo #warning review this for efficiency when complex alignments (VULGAR) are implemented */
          for(gaps = feature->feature->feature.homol.align, i = 0; i < gaps->len; i++)
            {
              ab = & g_array_index(gaps,ZMapAlignBlockStruct, i);
              
              this = alloc_pix_rect();
              
              foo_canvas_w2c (foo->canvas, 0,ab->t1, NULL, &this->y1);
              //			this->start = this->y1;
              foo_canvas_w2c (foo->canvas, feature->width, ab->t2, &this->x2, &this->y2);
              this->feature = feature;
              
              if(last)
                last->next = this;
              /* we don't care about the prev pointer */
              
              last = this;
              if(!blocks)
                blocks = this;
            }
	}
      else
	{
          this = alloc_pix_rect();
          
          foo_canvas_w2c (foo->canvas, 0,feature->y1, NULL, &this->y1);
          //		this->start = this->y1;
          foo_canvas_w2c (foo->canvas, feature->width, feature->y2, &this->x2, &this->y2);
          this->feature = feature;
          blocks = this;
	}
      
      /* new revised algorithm !
       *
       * we emulate the existing drawing order and only hide features that are covered by ones drawn later
       * which gives us the exact same picture as we would have had, but is slower than the previous effort
       * this is still a hypothetical max of about 1000 features per column to be displayed whatever the zoom
       *
       * if the current feature start is beyond anything left in the list then that's visible
       * (features are sorted in start coord order)
       * as features get covered then thier pixbuf struct gets shrunk
       * if it shrinks to 0 then that feature is hidden.
       */
      
      last = NULL;
      
      while(blocks)
	{
          this = blocks;
          blocks = blocks->next;		/* one feature only, or a list of always gapped blocks in the feature */
          this->next = this->prev = NULL;
          last = NULL;
          
#if PIX_LIST_DEBUG
          printf("this = %d\n",this->which);
#endif
          
          /* original code now in a loop */
          
          /* trim out of scope visible rectangles off the start of the list */
          while(pix && pix->y1 < this->y1)
            {
              /* feature is visible */
              n_summarise_show++;
              n_summarise_list--;
              
              pr = pix;
              
              /* one block of the feature is visible so it all is  (if always_gapped)... flag the feature*/
              pr->feature->flags &= ~FEATURE_SUMMARISED;
              if(!(pr->feature->flags & FEATURE_HIDE_REASON))
                pr->feature->flags &= ~FEATURE_HIDDEN;
              
#if PIX_LIST_DEBUG
              printf("visible %d\n",pr->which);
#endif
              
              pix = pix->next;
              if(pix)
                pix->prev = NULL;
              pix_rect_free(pr);
            }
          
          for(pr = pix; pr; )
            {
              del = NULL;
              last = pr;
#if PIX_LIST_DEBUG
              printf("testing %d\n",pr->which);
#endif
              
              if(this->x2 >= pr->x2)		/* enough overlap to care about */
                {
                  if(pr->y1 >= this->y1)
                    pr->y1 = this->y2 + 1;	/* shrink visible region: hidden cannot be after start */
                  
                  if(pr->y1 > pr->y2)	/* feature is hidden by features on top of it */
                    {
                      del = pr;
                      pr = pr->next;
#if PIX_LIST_DEBUG
                      printf("hidden %d\n",del->which);
#endif
                      
                      n_summarise_hidden++;
                      n_summarise_list--;
                      
                      if(del->next)
                        del->next->prev = del->prev;
                      else
                        last = del->prev;
                      
                      if(del->prev)
                        del->prev->next = del->next;
                      else
                        pix = del->next;
                    }
                }
              
              if(del)
                {
                  pix_rect_free(del);
#if PIX_LIST_DEBUG
                  printf("del %d\n",del->which);
#endif
                }
              
              else
                pr = pr->next;
            }
          
          n_summarise_list++;
          if(n_summarise_list > n_summarise_max)
            n_summarise_max = n_summarise_list;
          
          if(last)
            {
              last->next = this;
              this->prev = last;
            }
          else
            {
              pix = this;
            }
	}
    }
  
  return pix;
}




void zmapWindowCanvasFeaturesetSummariseFree(ZMapWindowFeaturesetItem featureset, PixRect pix)
{
	PixRect pr;

	while(pix)
	{
		n_summarise_show++;
		pr = pix;

		/* the ones at the end are left over and therefore visible */
            pr->feature->flags &= ~FEATURE_SUMMARISED;
            if(!(pr->feature->flags & FEATURE_HIDE_REASON))
                  pr->feature->flags &= ~FEATURE_HIDDEN;

		pix = pix->next;
		pix_rect_free(pr);
	}

//zMapLogWarning("summarise %s (%f): %ld+%ld/%ld = %ld)\n", g_quark_to_string(featureset->id), featureset->bases_per_pixel, n_summarise_show, n_summarise_hidden, featureset->n_features, n_summarise_max);

#if PIX_LIST_DEBUG
printf        ("summarise %s (%f): %ld+%ld/%ld = %ld), alloc = %ld, %ld, %ld\n", g_quark_to_string(featureset->id), featureset->bases_per_pixel, n_summarise_show, n_summarise_hidden, featureset->n_features, n_summarise_max,
		   n_block_alloc, n_pix_alloc, n_pix_free);
#else
//printf        ("summarise %s (%f): %ld+%ld/%ld = %ld)\n", g_quark_to_string(featureset->id), featureset->bases_per_pixel, n_summarise_show, n_summarise_hidden, featureset->n_features, n_summarise_max);
#endif
	n_summarise_hidden = 0;
	n_summarise_list = 0;
	n_summarise_show = 0;
	n_summarise_max = 0;
}



/*

results:

summarise 0x82c8178_tier2_hepg2_cytosol_longpolya_rep2_-0 (129.040659): 190+1723/1832 = 20)
summarise 0x82c8178_tier2_hepg2_cytosol_longpolya_rep2_+0 (0.123354): 1218+687/1832 = 40)


NOTE these stats were due to a bug! it's really just like simple features

summarise 0x82c72a8_solexa_introns_+0 (480.702198): 1067+2372/1111 = 535)
summarise 0x82c72a8_solexa_introns_+0 (0.401186): 2393+485/1111 = 1097)


NOTE solexa/ zebrafish is a much more varied dataset than encode and includes about a dozen featuresets
The list size is a bit dissappointing: it starts to look quadratic
(but see comment about the bug a few lines above)

ENCODE includes a lot of ungapped features which helps matters somewhat
*/




