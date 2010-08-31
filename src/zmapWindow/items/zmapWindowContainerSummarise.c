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
 * CVS info:   $Id: zmapWindowContainerSummarise.c,v 1.1 2010-08-31 08:54:20 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <glib.h>

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
 */

typedef struct
{
     gint32 x1,y1,x2,y2;
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

      feature->x1 = (gint32) floor(dx1);
      start = feature->y1 = (gint32) floor(dy1);
      feature->x2 = (gint32) floor(dx2);
      end = feature->y2 = (gint32) floor(dy2);

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
                        else if(cover->y2 > feature->y2)
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

            if(start < end)
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
/* must call this, it's benign for non summarised featuresets */
void zMapWindowContainerSummariseClear(ZMapWindow window,GQuark fset)
{
      GList *l;

      /* debugging/ stats */
      if(window->n_col_cover_show)
      {
            zMapLogMessage("summarise %s: %d+%d/%d, max was %d",g_quark_to_string(fset),
                  window->n_col_cover_show,window->n_col_cover_hide,
                  window->n_col_cover_show + window->n_col_cover_hide,
                  window->n_col_cover_list);
      }
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
