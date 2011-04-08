/*  File: zmapWindowContainerSummarise.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:   avoids displaying features that cannot be seen at the current zoom level
 *                NOTE see Design_notes/notes/canvas_tweaks.html
 *
 * CVS info:   $Id: zmapWindowContainerSummarise.c,v 1.4 2011-04-08 10:45:29 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <glib.h>

#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsLog.h>
#include <zmapWindow_P.h>

/*
      initially this is an experimental module to find out the level of filtering we could do
      only activated in styles with alignment-summarise=1.0 or similar
*/

/*
  - need to sort the features into y1 order (in the feature context)
      we assume this has been done and the features are fed in in order
      this is essential for efficiency

  - need a list of rectanges held in the window which holds the
    current 2D covered region, trimmed as it goes out of scope.

  - we also assume that the rectangles are left justified (as they are by the calling code)

  Results: we end up with 3k/ 28k features drawn. An alternaitve would be to compute the cover
  and draw that instead which would divide that by 3 or more
  .... this was without converting to canvas coords (oops!)

 */

typedef struct
{
     int x1,y1,x2,y2;
} pixrect, *PixRect;    /* think of a name not used elsewhere */


/* given bounds of a feature in world coords return whether or not it can be seen */
gboolean zmapWindowContainerSummariseIsItemVisible(ZMapWindow window, double dx1,double dy1,double dx2, double dy2)
{
      pixrect _feature;
      PixRect feature = &_feature;
      PixRect cover;
      GList *l,*del,*lnew;
      gint32 start,end;
      gboolean covered = FALSE;
      gint len;

      /* convert these coords to pixels at current zoom level */
      /* we don't care about the actual coordinates just the relative values */
      /* but we do care that the canvas coords are accurate in pixels */

      foo_canvas_w2c (window->canvas, dx1, dy1, &feature->x1, &feature->y1);
      foo_canvas_w2c (window->canvas, dx2, dy2, &feature->x2, &feature->y2);
      start = feature->y1;
      end = feature->y2;

      /* col_cover is in order of y1
         and consists of non overlapping rectangles in pixel coordinates
         coords are inclusive
      */

      /* trim out of scope covering rectangles off the front of the list */
      for(l = window->col_cover;l;)
      {
            cover = (PixRect) l->data;
            del = l;
            l = l->next;
            if(cover->y2 < start)
            {
                  g_free(del->data);
                  window->col_cover = g_list_delete_link(window->col_cover,del);
            }
      }

      /* test for enough width on each link and advance start if so else fail */
      for(l = window->col_cover;l;l = l->next)
      {
            cover = (PixRect) l->data;

            if(cover->y1 > start)   /* failed */
                  break;

            if(cover->x1 > feature->x1 || cover->x2 < feature->x2)      /* failed */
                  break;

            start = cover->y2 + 1;
            if(start > end)
            {
                  covered = TRUE;
                  break;
            }
      }

      if(covered)
      {
            window->n_col_cover_hide++;
      }
      else
      {
            /* this feature will be drawn so update the cover list
             * as we hope to mask lots more features than get displayed
             * we expect this will run less frequently than the above
             * so we optimise the lookup not creating the list
             */

            start = feature->y1;     /* this is the region we are trying to cover */
            end = feature->y2;

            for(l = window->col_cover;l;l = l->next)
            {
                  cover = (PixRect) l->data;

                  if(cover->x1 > feature->x1 || cover->x2 < feature->x2)  /* feature not covered by this rect */
                  {
                        if(cover->y1 < feature->y1)
                              /* there is a gap at the start: split cover into two */
                        {
                              PixRect new = g_new0(pixrect,1);

                              /* add new rect for existing cover */
                              new->y1 = cover->y1;
                              new->y2 = feature->y1 - 1;
                              new->x1 = cover->x1;
                              new->x2 = cover->x2;
                              window->col_cover = g_list_insert_before(window->col_cover,l,new);
                              //lnew = l;

                              /* adjust existing rect to match feature */
                              cover->x1 = feature->x1;
                              cover->y1 = feature->y1;
                              cover->x2 = feature->x2;

                              /* start = feature->y1;  we are already here */
                        }

                        if(cover->y2 > feature->y2)
                              /* there is a gap at the end: split cover into two */
                        {
                              PixRect new = g_new0(pixrect,1);

                              /* add new rect for feature cover */
                              new->y1 = cover->y1;
                              new->y2 = feature->y2;
                              new->x1 = feature->x1;
                              new->x2 = feature->x2;
                              window->col_cover = g_list_insert_before(window->col_cover,l,new);
                              //lnew = l->prev;
                              start = cover->y1 = feature->y2 + 1;      /* we are done */
                        }
                        else  /* perfect match: adjust cover */
                        {
                              /* adjust existing rect to match feature */
                              cover->x1 = feature->x1;
                              cover->x2 = feature->x2;
                              start = cover->y2 + 1;
                        }
                  }
                  else  /* is covered, adjust start and get next rect */
                  {
                        start = cover->y2 + 1;
                  }

                  if(start > end)
                        break;
            }

            if(start <= end)
            {
                  PixRect new = g_new0(pixrect,1);

                  /* add new rect for feature cover */
                  new->y1 = start;
                  new->y2 = end;
                  new->x1 = feature->x1;
                  new->x2 = feature->x2;
                  window->col_cover = g_list_insert_before(window->col_cover,NULL,new);
                  lnew = g_list_last(window->col_cover);
            }

            /* combine new rect if it's the same width as next or previous */
            {
            }

            window->n_col_cover_show++;
            len = g_list_length(window->col_cover);
            if(len > window->n_col_cover_list)
                  window->n_col_cover_list = len;
      }

      return(!covered);
}

/* tidy up after drawing a featureset: clear stats and free rump of list*/

void zMapWindowContainerSummariseClear(ZMapWindow window,ZMapFeatureSet fset)
{
      GList *l;

      /* debugging/ stats */
#if !MH17_DONT_INLCUDE
      if(window->n_col_cover_show)
      {
            printf("summarise %s: %d+%d/%d, max was %d\n",g_quark_to_string(fset->unique_id),
                  window->n_col_cover_show,window->n_col_cover_hide,
                  window->n_col_cover_show + window->n_col_cover_hide,
                  window->n_col_cover_list);
      }
#endif
      window->n_col_cover_show = 0;
      window->n_col_cover_hide = 0;
      window->n_col_cover_list = 0;

      for(l = window->col_cover;l;l = l->next)
      {
            g_free(l->data);
      }
      g_list_free(window->col_cover);
      window->col_cover = NULL;
}


gboolean zMapWindowContainerSummarise(ZMapWindow window,ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE;
  double summ = 0.0;

  if(style)       /* there might be no features and therefore no style */
    {
      if((summ = zMapStyleGetSummarise(style) > zMapWindowGetZoomFactor(window) ))
      {
        result = TRUE;
      }
    }

  return result;
}




/* sort features by start coord then by size (biggest first)
 * i tried this with canvas coords but it was 30-40% slower
 */
gint startOrderCB(gconstpointer a, gconstpointer b)
{
      ZMapFeature fa = (ZMapFeature) a;
      ZMapFeature fb = (ZMapFeature) b;

      /* yes: features have x coordinates that canvas items express as y */

      if(fa->x1 == fb->x1)
            return((gint) (fb->x2 - fa->x2));
      return ((gint) (fa->x1 - fb->x1));
}


/* sort features by start coord then by size (biggest first) */

guint startOrderKey(gconstpointer thing, int digit)
{
      guint coord;
      ZMapFeature feature = (ZMapFeature) thing;

      if(digit < 2)     // we assume size is less than 64k for a single feature (alignment)
      {
            coord = G_MAXUINT - (feature->x2 - feature->x1);  /* biggest first */
      }
      else
      {
            digit -= 2;
            coord = feature->x1;         /* start coord */
      }

      while (--digit >= 0)
            coord >>= RADIX_BITS;
      coord &= 0xff;

      return coord;
}





/* extract features from the hash table into a list and sort it */
GList *zMapWindowContainerSummariseSortFeatureSet(ZMapFeatureSet fset)
{
      GList *features;

      zMap_g_hash_table_get_data(&features, fset->features);
#if !MH17_test_radix_sort
/* see zmapRadixSort.c for performance stats */
printf("%s has %d items\n",g_quark_to_string(fset->unique_id),g_list_length(features));
zMapStartTimer("normal sort","");
{ int i;


for(i = 0;i < 100;i++)
      features = g_list_sort(features,startOrderCB);
zMapStopTimer("normal sort","");

zMap_g_hash_table_get_data(&features, fset->features);      /* makes no difference */
zMapStartTimer("radix sort","");
for(i = 0;i < 100;i++)
      features = zMapRadixSort(features,startOrderKey,6); /* key of 6 bytes: 2 for length, 4 for start */
zMapStopTimer("radix sort","");

}
#else
      features = g_list_sort(features,startOrderCB);
//      features = zMapRadixSort(features,startOrderKey,6);
#endif

      return features;
}

