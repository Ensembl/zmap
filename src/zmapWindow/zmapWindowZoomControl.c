/*  File: zmapWindowZoomControl.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct  7 16:38 2005 (edgrif)
 * Created: Fri Jul  8 11:37:39 2005 (rds)
 * CVS info:   $Id: zmapWindowZoomControl.c,v 1.4 2005-10-07 17:17:47 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowZoomControl_P.h>

static double getMinZoom(ZMapWindow window);
static void setZoomStatus(ZMapWindowZoomControl control);
static gboolean canZoomByFactor(ZMapWindowZoomControl control, double factor);
static ZMapWindowZoomControl controlFromWindow(ZMapWindow window);
static void printControl(ZMapWindowZoomControl control);

/* =========================================================================== */
/*                                   PUBLIC                                    */
/* =========================================================================== */
ZMapWindowZoomStatus zMapWindowGetZoomStatus(ZMapWindow window)
{
  ZMapWindowZoomStatus status   = ZMAP_ZOOM_INIT;
  ZMapWindowZoomControl control = NULL;

  control = controlFromWindow(window);

  status  = control->status;

  return status;
}


double zMapWindowGetZoomFactor(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  control = controlFromWindow(window);

  return control->zF;
}


double zMapWindowGetZoomMax(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  control = controlFromWindow(window);

  return control->maxZF ;
}


double zMapWindowGetZoomMagnification(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;
  double mag;

  control = controlFromWindow(window);
  /* Not sure what this actually needs to be calculating, 
   * but a ratio might be nice rather than just random numbers */
  mag = control->maxZF / control->zF;
  
  return mag;
}
/* =========================================================================== */
/*                                  PRIVATE                                    */
/* =========================================================================== */
/* Create the Controller... */
ZMapWindowZoomControl zmapWindowZoomControlCreate(ZMapWindow window) 
{
  ZMapWindowZoomControl num_cruncher = NULL;
  double text_height;
  int x_windows_limit = (2 >> 15) - 1;
  int user_set_limit  = (2 >> 15) - 1;  /* possibly a parameter later?!? */
  int max_window_size = 0;
  num_cruncher = g_new0(ZMapWindowZoomControlStruct, 1);

  /* Make sure this is the 1:1 text_height 14 on my machine*/
  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, 1.0);
  zMapDrawGetTextDimensions(foo_canvas_root(window->canvas), NULL, &text_height) ;

  num_cruncher->maxZF      = text_height + (double)(ZMAP_WINDOW_TEXT_BORDER * 2);
  num_cruncher->border     = text_height; /* This should _NOT_ be changed */
  num_cruncher->status     = ZMAP_ZOOM_INIT;

  //  max_window_size = (user_set_limit ? user_set_limit : x_windows_limit) 
  //    - ((num_cruncher->border * 2) * num_cruncher->maxZF);
  
  return num_cruncher;
}

/* Because we need to create and then set it up later */
void zmapWindowZoomControlInitialise(ZMapWindow window)
{
  ZMapWindowZoomControl control;

  control = controlFromWindow(window);

  control->minZF = getMinZoom(window);
  if(control->status == ZMAP_ZOOM_INIT)
    control->zF    = control->minZF;

  /* Account for SHORT sequences (shorter than canvas height) */
  setZoomStatus(control); /* This alone probably doesn't do this!! */

  return ;
}

/* Should be called when either canvas height(size-allocate) or seq
   length change. Does virtually the same as initialise... */
void zmapWindowZoomControlHandleResize(ZMapWindow window)
{
  ZMapWindowZoomControl control;
  double min;

  control = controlFromWindow(window);

  min = getMinZoom(window);

  /* Always need to reset this. */
  control->minZF = min;

  if(min > control->zF)
    zMapWindowZoom(window, min / control->zF);

  setZoomStatus(control);
 
  return ;
}

/* This just does the maths and sets zoom factor and status
 * (Might need to work with setZoomStatus??? also ZMAP_ZOOM_FIXED)
 */
gboolean zmapWindowZoomControlZoomByFactor(ZMapWindow window, double factor)
{
  ZMapWindowZoomControl control;
  gboolean did_zoom = FALSE;
  double zoom;

  control = controlFromWindow(window);

  if(canZoomByFactor(control, factor))
    {
      /* Calculate the zoom. */
      zoom = control->zF * factor ;
      if (zoom <= control->minZF)
          control->zF = control->minZF ;
      else if (zoom >= control->maxZF)
          control->zF = control->maxZF ;
      else
          control->zF = zoom ;

      setZoomStatus(control);

      did_zoom = TRUE;
    }

  return did_zoom;
}

double zmapWindowZoomControlLimitSpan(ZMapWindow window, double y1, double y2)
{
  ZMapWindowZoomControl control;
  double max_span, seq_span, new_span;
  double canv_span;

  control   = controlFromWindow(window);

  max_span  = (double)(window->canvas_maxwin_size);
  seq_span  = window->seqLength * control->zF;

  new_span  = y2 - y1 + 1 ;
  new_span *= control->zF ;

  new_span  = (new_span >= max_span ? 
               max_span : (seq_span > max_span ? 
                           max_span
                           : seq_span)) ;
  
  canv_span = new_span / control->zF;
  
  return canv_span;
}
#ifdef NOW_IN_UTILS_RDS
void zmapWindowZoomControlClampSpan(ZMapWindow window, double *top_inout, double *bot_inout)
{
  ZMapWindowZoomControl control;
  double top, bot;

  top = *top_inout;
  bot = *bot_inout;
  control = controlFromWindow(window);

  if (top < window->seq_start)
    {
      if ((bot = bot + (window->seq_start - top)) > window->seq_end)
        bot = window->seq_end ;
      top = window->seq_start ;
    }
  else if (bot > window->seq_end)
    {
      if ((top = top - (bot - window->seq_end)) < window->seq_start)
        top = window->seq_start ;
      bot = window->seq_end ;
    }

  *top_inout = top;
  *bot_inout = bot;

  return ;
}
#endif   /* NOW_IN_UTILS_RDS  */

void zmapWindowZoomControlCopyTo(ZMapWindowZoomControl orig, ZMapWindowZoomControl new)
{
  new->zF     = orig->zF;
  setZoomStatus(new);
  return ;
}

void zmapWindowGetBorderSize(ZMapWindow window, double *border)
{
  ZMapWindowZoomControl control = NULL;
  double b = 0.0;

  control = controlFromWindow(window);

  b = control->border / control->zF;  

  *border = b;

  return ;
}

#ifdef BLAH_BLAH_BLAH
void zmapWindowDebugWindowCopy(ZMapWindow window)
{
  ZMapWindowZoomControl control;
  double x1, x2, y1, y2;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(FOO_CANVAS(window->canvas))),
                             &x1, &y1,
                             &x2, &y2);
  control = controlFromWindow(window);
  printf("Window:\n"
         " min_coord %f\n"
         " max_coord %f\n"
         " seqLength %f\n"
         " x1 %f, y1 %f, x2 %f, y2 %f\n",
         window->min_coord,
         window->max_coord,
         window->seqLength,
         x1, y1, x2, y2
         );
  printControl(control);
  return ;
}
#endif /* BLAH_BLAH_BLAH */

/* =========================================================================== */
/*                                  INTERNAL                                   */
/* =========================================================================== */

/* Exactly what it says */
static double getMinZoom(ZMapWindow window)
{
  ZMapWindowZoomControl control;
  double canvas_height, border_height, zf;

  control = controlFromWindow(window);
  zMapAssert(control != NULL 
             && window->seqLength != 0 
             );

  /* These heights are in pixels */
  border_height = control->border * 2.0; 
  canvas_height = GTK_WIDGET(window->canvas)->allocation.height;

  zMapAssert(canvas_height >= border_height);
  /* rearrangement of
   *              canvas height
   * zf = ------------------------------
   *      seqLength + (text_height / zf)
   * could do with replacing allocation.height with adjuster->page_size
   */
  /*     canvas_height    two borders      length of sequence */
  zf = ( canvas_height - border_height ) / (window->seqLength + 1);

  return zf;
}


/* Private: from initialise and other public only?
 * Public: ????
 */
static void setZoomStatus(ZMapWindowZoomControl control)
{
  /* This needs to handle ZMAP_ZOOM_FIXED too!! */
  if (control->minZF >= control->maxZF)
    {
      control->status = ZMAP_ZOOM_FIXED ;  
      control->zF     = control->maxZF ;
    }
  else if (control->zF <= control->minZF)
    control->status = ZMAP_ZOOM_MIN ;
  else if (control->zF >= control->maxZF)
    control->status = ZMAP_ZOOM_MAX ;
  else
    control->status = ZMAP_ZOOM_MID ;

  return ;
}

/* No point zooming if we can't */
static gboolean canZoomByFactor(ZMapWindowZoomControl control, double factor)
{
  gboolean can_zoom = TRUE;
  if (control->status == ZMAP_ZOOM_FIXED
      || (factor > 1.0 && control->status == ZMAP_ZOOM_MAX)
      || (factor < 1.0 && control->status == ZMAP_ZOOM_MIN))
    {
      can_zoom = FALSE;
    }
  return can_zoom;
}
/* This is here incase window struct changes name of control; */
static ZMapWindowZoomControl controlFromWindow(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  zMapAssert(window->zoom);

  control = window->zoom;

  return control;
}

#ifdef BLAH_BLAH_BLAH
static void printControl(ZMapWindowZoomControl control)
{
  printf("Control:\n"
         " factor %f\n"
         " min %f\n"
         " max %f\n"
         " status %d\n",
         control->zF,
         control->minZF,
         control->maxZF,
         control->status
         );
  return ;
}
#endif /* BLAH_BLAH_BLAH */
