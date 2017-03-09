/*  File: zmapWindowCanvas.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Functions that wrap the foocanvas calls and control
 *              the interface between our structs and the foocanvas.
 *
 * Exported functions: See zmapWindow_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <zmapWindow_P.hpp>


typedef enum {FIRST_COL, LAST_COL} ColPositionType ;

typedef struct
{
  ZMapStrand strand ;
  ColPositionType col_position ;
  GQuark column_id ;
  FooCanvasGroup *column_group ;
} StrandColStruct, *StrandCol ;


static void getFirstForwardCol(ZMapWindowContainerGroup container, FooCanvasPoints *container_points,
                               ZMapContainerLevelType container_level, gpointer func_data) ;

static void getColByID(ZMapWindowContainerGroup container, FooCanvasPoints *container_points,
                       ZMapContainerLevelType container_level, gpointer func_data) ;






/*
 *                   Package routines.
 */


/* FooCanvas inherits from the Gtk widget "layout" which has two windows: the (smaller)
 * visible one and a larger window which can be scrolled past the smaller one.
 * This function returns the sizes of these windows in pixels, the bin window is the
 * larger/background one. */
gboolean zmapWindowGetCanvasLayoutSize(FooCanvas *canvas,
                                       int *layout_win_width, int *layout_win_height,
                                       int *layout_binwin_width, int *layout_binwin_height)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(canvas &&
                      layout_win_width && layout_win_height &&
                      layout_binwin_width && layout_binwin_height, result ) ;

  if (FOO_IS_CANVAS(canvas))
    {
      GtkWidget *widget = GTK_WIDGET(canvas) ;
      GtkLayout *layout = &(canvas->layout) ;
      unsigned int get_layout_width, get_layout_height ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Check sizes for debug.... */
      int layout_alloc_width, layout_alloc_height ;
      layout_alloc_width = widget->allocation.width ;
      layout_alloc_height = widget->allocation.height ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      gtk_layout_get_size(layout, &get_layout_width, &get_layout_height) ;


      gdk_drawable_get_size(GDK_DRAWABLE(widget->window), layout_win_width, layout_win_height) ;
      gdk_drawable_get_size(GDK_DRAWABLE(layout->bin_window), layout_binwin_width, layout_binwin_height) ;

      result = TRUE ;
    }

  return result ;
}

void zmapWindowSetScrolledRegion(ZMapWindow window, double x1, double x2, double y1, double y2)
{
  FooCanvas *canvas = NULL ;

  zMapReturnIfFail(window) ;

  canvas = FOO_CANVAS(window->canvas) ;

  foo_canvas_set_scroll_region(canvas, x1, y1, x2, y2) ;

  return ;
}


/* Change the pixel/zoom for the window and update the cached sizes of the canvas/layout window. */
void zmapWindowSetPixelxy(ZMapWindow window, double pixels_per_unit_x, double pixels_per_unit_y)
{
  FooCanvas *canvas = NULL ;
  zMapReturnIfFail(window) ;
  canvas = FOO_CANVAS(window->canvas) ;

  foo_canvas_set_pixels_per_unit_xy(canvas, pixels_per_unit_x, pixels_per_unit_y) ;

  zmapWindowGetCanvasLayoutSize(canvas,
                                &(window->layout_window_width),
                                &(window->layout_window_height),
                                &(window->layout_bin_window_width),
                                &(window->layout_bin_window_height)) ;

  return ;
}


/* Returns the leftmost column if the forward strand is requested, the right most if
 * the reverse strand is requested. The column must have features and must be displayed
 * otherwise NULL is returned if no columns match this (can happen on reverse strand).
 *
 * NOTE column will be in the first block of the first align. */
FooCanvasGroup *zmapWindowGetFirstColumn(ZMapWindow window, ZMapStrand strand)
{
  StrandColStruct strand_data = {strand, FIRST_COL, 0, NULL} ;

  zMapReturnValIfFail(window, NULL) ;

  zmapWindowContainerUtilsExecute(window->feature_root_group, ZMAPCONTAINER_LEVEL_BLOCK,
                                  getFirstForwardCol, &strand_data) ;

  return strand_data.column_group ;
}


/* Returns the rightmost column if the forward strand is requested, the leftmost if
 * the reverse strand is requested. The column must have features and must be displayed
 * otherwise NULL is returned if no columns match this (can happen on reverse strand).
 *
 * NOTE column will be in the first block of the first align. */
FooCanvasGroup *zmapWindowGetLastColumn(ZMapWindow window, ZMapStrand strand)
{
  StrandColStruct strand_data = {strand, LAST_COL, 0, NULL} ;

  zMapReturnValIfFail(window, NULL) ;

  zmapWindowContainerUtilsExecute(window->feature_root_group, ZMAPCONTAINER_LEVEL_BLOCK,
                                  getFirstForwardCol, &strand_data) ;

  return strand_data.column_group ;
}


/* Return the column group for the column with the given ID and strand. Returns NULL if not
 * found. */
FooCanvasGroup *zmapWindowGetColumnByID(ZMapWindow window, ZMapStrand strand, GQuark column_id)
{
  zMapReturnValIfFail(window, NULL) ;

  StrandColStruct strand_data = {strand, FIRST_COL, column_id, NULL} ;

  zmapWindowContainerUtilsExecute(window->feature_root_group, ZMAPCONTAINER_LEVEL_BLOCK,
                                  getColByID, &strand_data) ;

  return strand_data.column_group ;
}


static void getColByID(ZMapWindowContainerGroup container, FooCanvasPoints *container_points,
                       ZMapContainerLevelType container_level, gpointer func_data)
{
  StrandCol strand_data = (StrandCol)func_data ;

  zMapReturnIfFail(func_data) ;

  /* Only look for a column in the requested strand. */
  if (container_level == ZMAPCONTAINER_LEVEL_BLOCK)
    {
      if (!(strand_data->column_group))
        {
          /* Haven't found it yet so keep looking */
          FooCanvasGroup *column = NULL ;
          FooCanvasGroup *strand_columns = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container) ;

          for (GList *col_ptr = strand_columns->item_list; col_ptr; col_ptr = col_ptr->next)
            {
              column = (FooCanvasGroup *)(col_ptr->data) ;

              if (ZMAP_IS_CONTAINER_FEATURESET(column))
                {
                  GQuark cur_column_id = zMapWindowContainerFeatureSetGetUniqueId(ZMAP_CONTAINER_FEATURESET(column)) ;
                  ZMapStrand cur_strand = zmapWindowContainerFeatureSetGetStrand(ZMAP_CONTAINER_FEATURESET(column)) ;

                  if (cur_column_id == strand_data->column_id && cur_strand == strand_data->strand)
                    {
                      strand_data->column_group = column ;
                      break ;
                    }
                }
            }
        }
    }
}


static void getFirstForwardCol(ZMapWindowContainerGroup container, FooCanvasPoints *container_points,
                               ZMapContainerLevelType container_level, gpointer func_data)
{
  StrandCol strand_data = (StrandCol)func_data ;

  /* Only look for a column in the requested strand. */
  if (container_level == ZMAPCONTAINER_LEVEL_BLOCK)
    {
      // Only look if we haven't yet found a column (no way to abort from search).
      if (!(strand_data->column_group))
        {
          FooCanvasGroup *strand_columns, *column = NULL ;
          GList *col_ptr ;

          /* Look for a column that's visible and has features and is not the strand separator ! */
          strand_columns = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container) ;

          if (strand_data->strand == ZMAPSTRAND_FORWARD)
            {
              if (strand_data->col_position == FIRST_COL)
                col_ptr = (strand_columns->item_list) ;
              else
                col_ptr = (strand_columns->item_list_end) ;
            }
          else
            {
              if (strand_data->col_position == FIRST_COL)
                col_ptr = (strand_columns->item_list_end) ;
              else
                col_ptr = (strand_columns->item_list) ;
            }
          
          while (col_ptr)
            {

              column = (FooCanvasGroup *)(col_ptr->data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
              ZMapWindowContainerFeatureSet container_set ;
              char *name ;


              container_set = ZMAP_CONTAINER_FEATURESET(column) ;

              name = zmapWindowContainerFeaturesetGetColumnUniqueName(container_set) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

              
              // check for column item that is visible, is not the strand separator, is on the
              // right strand and either has features or can be shown empty.
              if (zmapWindowItemIsShown(FOO_CANVAS_ITEM(column))
                  && ZMAP_IS_CONTAINER_GROUP(column)
                  && (zMapWindowContainerFeatureSetGetUniqueId(ZMAP_CONTAINER_FEATURESET(column))
                      != zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_STRAND_SEPARATOR))
                  && (zmapWindowContainerFeatureSetGetStrand(ZMAP_CONTAINER_FEATURESET(column))
                      == strand_data->strand)
                  && (zmapWindowContainerHasFeatures(ZMAP_CONTAINER_GROUP(column))
                      || zmapWindowContainerFeatureSetShowWhenEmpty(ZMAP_CONTAINER_FEATURESET(column))))
                {
                  strand_data->column_group = column ;

                  break ;
                }
              else
                {
                  if (strand_data->strand == ZMAPSTRAND_FORWARD)
                    {
                      if (strand_data->col_position == FIRST_COL)
                        col_ptr = col_ptr->next ;
                      else
                        col_ptr = col_ptr->prev ;
                    }
                  else
                    {
                      if (strand_data->col_position == FIRST_COL)
                        col_ptr = col_ptr->prev ;
                      else
                        col_ptr = col_ptr->next ;
                    }
                }
            }
        }
    }

  return ;
}
