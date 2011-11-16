/*  File: zmapWindowFeaturesetSummarise.c
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
      the original code would hide features that wsould have been dislayed on top of others
      argualbly not a problem but we aim for the exact picture ot lower zoom
      most important perfornance gains are at min zoom and this will operate fastest then
*/

/*
  - need to sort the features into y1 order (in the feature context)
      we assume this has been done and the features are fed in in order
      this is essential for efficiency

  - need a list of rectanges held in the window which holds the
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

static PixRect pix_rect_free_G = NULL;

void pix_rect_free(PixRect pix)
{
	if(pix_rect_free_G)
		pix_rect_free_G->next = pix;
	pix_rect_free_G = pix;
}

PixRect alloc_pix_rect(void)
{
	void *mem;
	int i;
	PixRect ret;

	if(!pix_rect_free_G)
	{
		mem = g_malloc(sizeof(pixRect) * N_PIXRECT_ALLOC);
		for(i = 0; i < N_PIXRECT_ALLOC; i++)
		{
			pix_rect_free((PixRect) mem);
			mem += sizeof(pixRect);
		}
	}

	ret = pix_rect_free_G;
	memset((void *) ret,0,sizeof(pixRect));
	pix_rect_free_G = pix_rect_free_G->prev;
	return ret;
}


/* flag feature as viz or not, maintain PixRect list of covered region */
/* called on zoom change */
/* NOTE we could optimise this to only look at the visible scroll region - do this in the calling code */

PixRect zmapWindowCanvasFeaturesetSummarise(PixRect pix, ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature)
{
	FooCanvasItem *foo = &featureset->__parent__;
	PixRect this = alloc_pix_rect();
	PixRect pr,del,last = NULL;


	feature->flags &= ~FEATURE_SUMMARISED;
	if(!(feature->flags & FEATURE_HIDE_REASON))
		feature->flags &= ~FEATURE_HIDDEN;

	/* NOTE we don't care about exact canvas coords, just the size and relative position */

      foo_canvas_w2c (foo->canvas, 0,feature->y1, NULL, &this->y1);
      this->start = this->y1;
      foo_canvas_w2c (foo->canvas, feature->width, feature->y2, &this->x2, &this->y2);
      this->feature = feature;

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

      /* trim out of scope visible rectangles off the start of the list */
      while(pix && pix->y1 < this->y1)
	{
		/* feature is visible */
		n_summarise_show++;
		n_summarise_list--;

		pr = pix;
		pix = pix->next;
		if(pix)
      		pix->prev = NULL;
		pix_rect_free(pr);
	}

	for(pr = pix; pr; )
	{
		del = NULL;
		last = pr;

		if(this->x2 >= pr->x2)		/* enough overlap to care about */
		{
			pr->y1 = this->y2 + 1;	/* shrink visible region: hidden cannot be after start */

			if(pr->y1 > pr->y2)	/* feature is hidden by features on top of it */
			{
				del = pr;
				pr->feature->flags |= FEATURE_HIDDEN | FEATURE_SUMMARISED;
				pr = pr->next;

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
			pix_rect_free(del);
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

	return pix;
}




void zmapWindowCanvasFeaturesetSummariseFree(ZMapWindowFeaturesetItem featureset, PixRect pix)
{
	PixRect pr;

	while(pix)
	{
		n_summarise_show++;
		pr = pix;
		pix = pix->next;
		pix_rect_free(pr);
	}

//printf("summarise %s (%f): %ld+%ld/%ld = %ld)\n", g_quark_to_string(featureset->id), featureset->bases_per_pixel, n_summarise_show, n_summarise_hidden, featureset->n_features, n_summarise_max);

	n_summarise_hidden = 0;
	n_summarise_list = 0;
	n_summarise_show = 0;
	n_summarise_max = 0;
}











