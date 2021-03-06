/*  File: zmapWindowUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
 * Description: Utility functions for the zMapWindow code.
 *
 * Exported functions: See ZMap/zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>
#include <string.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <ZMap/zmapUtils.hpp>
#include <zmapWindow_P.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>




/* Struct for style table callbacks. */
typedef struct
{
  ZMapWindowStyleTableCallback func ;
  gpointer data ;
} ZMapWindowStyleTableDataStruct, *ZMapWindowStyleTableData ;












/* Reworked coordinate functions
 *
 * NOTE we assume window->min_coord and window->max_coord are valid and there is only one block
 * with the implemnention as of Jan/Feb2011 this is true and
 * all features are displayed at absolute not block relative coordinates
 */


int zmapWindowCoordToDisplay(ZMapWindow window, ZMapWindowDisplayCoordinates display_mode, int coord)
{
  /* min_coord is the start of the sequence even if revcomp'ed
   * window->sequence->start is fwd strand
   * but note that max_coord has been incremented to cover beyond the last base
   */
  if (display_mode == ZMAP_WINDOW_DISPLAY_SLICE)
    coord = coord - window->min_coord + 1 ;

  if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
    {
      if (display_mode == ZMAP_WINDOW_DISPLAY_SLICE)
        {
          coord -= window->sequence->end - window->sequence->start + 2 ;
        }
      else
        {
          coord = -window->sequence->start - (window->max_coord - window->min_coord - coord) ;
        }
    }

  return coord ;
}


void zmapWindowCoordPairToDisplay(ZMapWindow window, ZMapWindowDisplayCoordinates display_mode,
                                  int start_in, int end_in,
                                  int *display_start_out, int *display_end_out)
{
  int start, end ;

  start = zmapWindowCoordToDisplay(window, display_mode, start_in) ;
  end = zmapWindowCoordToDisplay(window, display_mode, end_in) ;

  /* Note that the start/ends get swopped if we are reversed complemented and
   * display forwards coords so their order fits the "forwards" display. */
  if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) && window->display_forward_coords)
    ZMAP_SWAP_TYPE(int, start, end) ;

  *display_start_out = start ;
  *display_end_out = end ;

  return ;
}




/* start and end in current fwd strand coordinates (revcomped is -ve) */
/* function written due to scope issues */
gboolean zmapWindowGetCurrentSpan(ZMapWindow window, int *start, int *end)
{
  /* MH17 NOTE: min and max_coord have been adjusted by
     zmapWindowSeq2CanExt() see comment above that function
     i'd prefer to use sequence->start,end but on revcomp this gives us chromo coords
  */
  zmapWindowCoordPairToDisplay(window, window->display_coordinates,
                               window->min_coord,window->max_coord-1,start,end) ;

  return(TRUE);
}

int zmapWindowWorldToSequenceForward(ZMapWindow window, int coord)
{
  int new_coord = coord;
  ZMapSpan span;

  /* new coord is the 'current forward strand' sequence coordinate */

  if(zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
    {
      span = &window->feature_context->parent_span;
      //      new_coord -= span->x2 - span->x1 + 1;
      /* how did this get broken again? */
      new_coord = span->x2 - new_coord + 1;
    }

  return new_coord ;
}


int zmapWindowCoordFromDisplay(ZMapWindow window, int coord)
{
  /*! \todo #warning this needs testing/fixing: called from -> zmapWindowDNA.c/searchCB() */
  /* turns out it's the converse of zmapWindowCoordPairToDisplay()
   * coords get converted to display then converted back to sequence
   * better to fix in the caller by not calling this!
   */
  if(zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
    //      coord += window->max_coord - window->min_coord + 2;
    // max coord was seq_end+1 before revcomp
    coord += window->sequence->end - window->sequence->start + 2;
  coord = coord + window->min_coord - 1;

  return coord ;
}



ZMapStrand zmapWindowStrandToDisplay(ZMapWindow window, ZMapStrand strand_in)
{
  ZMapStrand strand = ZMAPSTRAND_NONE ;

  if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) && window->display_forward_coords)
    {
      if (strand_in == ZMAPSTRAND_FORWARD)
        strand = ZMAPSTRAND_REVERSE ;
      else if (strand_in == ZMAPSTRAND_REVERSE)
        strand = ZMAPSTRAND_FORWARD ;
    }
  else
    {
      strand = strand_in ;
    }


  return strand ;
}



/* This is the basic length calculation, obvious, but the "+ 1" is constantly overlooked. */
double zmapWindowExt(double start, double end)
{
  double extent = 0.0;

  if (start > end)
    return extent ;

  extent = end - start + 1 ;

  return extent ;
}


/* Converts a sequence extent into a canvas extent.
 *
 * Less obvious as it covers the following slightly subtle problem:
 *
 * sequence coords:           1  2  3  4  5  6  7  8
 *
 * canvas coords:            |__|__|__|__|__|__|__|__|
 *
 *                           |                       |
 *                          1.0                     9.0
 *
 * i.e. when we actually come to draw it we need to go one _past_ the sequence end
 * coord because our drawing needs to draw in the whole of the last base.
 *
 */
void zmapWindowSeq2CanExt(double *start_inout, double *end_inout)
{
  if (!start_inout || !end_inout || (*start_inout > *end_inout))
    return ;

  *end_inout += 1 ;

  return ;
}

/* Converts a start/end pair of coords into a zero based pair of coords.
 *
 * For quite a lot of the canvas group stuff we need to take two coords defining a range
 * in some kind of parent system and convert that range into the same range but starting at zero,
 * e.g.  range  3 -> 6  becomes  0 -> 3
 *
 */
void zmapWindowExt2Zero(double *start_inout, double *end_inout)
{
  if (!start_inout || !end_inout || (*start_inout > *end_inout))
    return ;

  *end_inout = *end_inout - *start_inout ;                    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  return ;
}


/* Converts a sequence extent into a zero based canvas extent.
 *
 * Combines zmapWindowSeq2CanExt() and zmapWindowExt2Zero(), used in positioning
 * groups/features a lot because in the canvas item coords are relative to their parent group
 * and hence zero-based. */
void zmapWindowSeq2CanExtZero(double *start_inout, double *end_inout)
{
  if (!start_inout || !end_inout || (*start_inout > *end_inout))
    return ;

  *end_inout = *end_inout - *start_inout ;                    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  *end_inout = *end_inout + 1 ;

  return ;
}


/* NOTE: offset may not be quite what you think, the routine recalculates */
void zmapWindowSeq2CanOffset(double *start_inout, double *end_inout, double offset)
{
  if (!start_inout || !end_inout || (*start_inout > *end_inout))
    return ;

  *start_inout -= offset ;

  *end_inout -= offset ;

  *end_inout += 1 ;

  return ;
}



ZMapGUIClampType zmapWindowClampedAtStartEnd(ZMapWindow window, double *top_inout, double *bot_inout)
{
  ZMapGUIClampType clamp_type = ZMAPGUI_CLAMP_INIT;

  clamp_type = zMapGUICoordsClampToLimits(window->min_coord, window->max_coord,
                                          top_inout, bot_inout);

  return clamp_type;                 /* ! */
}

/* Clamps the span within the length of the sequence,
 * possibly shifting the span to keep it the same size. */
ZMapGUIClampType zmapWindowClampSpan(ZMapWindow window, double *top_inout, double *bot_inout)
{
  ZMapGUIClampType clamp = ZMAPGUI_CLAMP_INIT;

  clamp = zMapGUICoordsClampSpanWithLimits(window->min_coord,
                                           window->max_coord,
                                           top_inout, bot_inout);

  return clamp;
}


/* We have some arrays of additional windows that are popped up as a result of user interaction
 * with the zmap, these need to cleaned up when the zmap goes away.
 *
 * If window_array points to NULL then function just returns. */
void zmapWindowFreeWindowArray(GPtrArray **window_array_inout, gboolean free_array)
{
  GPtrArray *window_array ;

  if (!window_array_inout)
    return ;

  if ((window_array = *window_array_inout))
    {
      /* Slightly tricky here, when the list widget gets destroyed it removes itself
       * from the array so the array shrinks by one, hence we just continue to access
       * the first element until all elements are gone. */
      while (window_array->len)
        {
          GtkWidget *widget ;

          widget = (GtkWidget *)g_ptr_array_index(window_array, 0) ;

          gtk_widget_destroy(widget) ;
        }

      if (free_array)
        {
          g_ptr_array_free(window_array, FALSE) ;
          *window_array_inout = NULL ;
        }
    }

  return ;
}




/* Get scroll region top/bottom. */
void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out)
{
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

  *top_out = scroll_y1 ;
  *bottom_out = scroll_y2 ;

  return ;
}



gboolean zMapWindowGetVisibleSeq(ZMapWindow window, FooCanvasItem *focus, int *top_out, int *bottom_out)
{
  gboolean result = FALSE ;
  double wx1, wy1, wx2, wy2 ;
  FooCanvasGroup *block_grp ;
  int y1, y2 ;

  zmapWindowItemGetVisibleWorld(window, &wx1, &wy1, &wx2, &wy2) ;

  if (zmapWindowWorld2SeqCoords(window, focus, wx1, wy1, wx2, wy2,
                                &block_grp, &y1, &y2))
    {
      *top_out = y1 ;
      *bottom_out = y2 ;
      result = TRUE ;
    }

  return result ;
}






gboolean zMapWindowGetMaskedColour(ZMapWindow window,GdkColor **border_col, GdkColor **fill_col)
{
  *fill_col = &(window->colour_masked_feature_fill);
  *border_col = &(window->colour_masked_feature_border);
  return window->highlights_set.masked;
}


gboolean zMapWindowGetFilteredColour(ZMapWindow window, GdkColor **fill_col)
{
  if(window && window->highlights_set.filtered)
    {
      *fill_col = &(window->colour_filtered_column);
      return TRUE;
    }
  return FALSE;
}




/* create or extract a style for a column as a whole */
/* no column specicifc style has been defined */
/* style_table includes all the styles needed by the column */

ZMapFeatureTypeStyle zmapWindowColumnMergeStyle(ZMapFeatureColumn column)
{
  ZMapFeatureTypeStyle style = NULL, s;
  GList *iter;
  ZMapStyleMode mode;

  if(!column->style_table)      /* mis-configuration */
    {
      return NULL;
    }

  if(!column->style_table->next)  /* only one style: use this directly */
    {
      style = (ZMapFeatureTypeStyle) column->style_table->data;
    }
  else
    {
      /* merge styles in reverse to give priority to first in the list as defined in config
       * this models previous behaviour with a dynamic style table
       * NB: sub feature styles come after their parents, see zmapWindowFeatureSetStyle() below
       * but are likely to be a different style mode and will be ignored
       */
      s = (ZMapFeatureTypeStyle) column->style_table->data;
      mode = s->mode;

      for(iter = g_list_last(column->style_table);iter; iter = iter->prev)
        {
          s = (ZMapFeatureTypeStyle) iter->data;

          if(!s->mode || s->mode == mode)
            {
              if(!style)
                {
                  style = zMapFeatureStyleCopy(s);
                }
              else
                {
                  zMapStyleMerge(style, s);
                }
            }
        }
      /*  read-only locations, need to use GObject, use last style as that's easy, not used for lookup anyway
       *            style->unique_id = column->unique_id;
       *            style->original_id = column->column_id;
       *            can try creating empty style w/ column name and merge
       *            but it's not needed, this style will never be looked up
       */
    }
  return(style);
}


/* get the column struct for a featureset */
ZMapFeatureColumn zMapWindowGetColumn(ZMapFeatureContextMap map,GQuark col_id)
{
  ZMapFeatureColumn column = NULL;

  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = map->columns->find(col_id) ;

  if (col_iter != map->columns->end())
    column = col_iter->second ;

  if(!column)
    zMapLogWarning("no column for featureset %s",g_quark_to_string(col_id));

  return column;
}


/*
 * construct a style table for a column
 * and extract the column style or single featureset style or merge all styles into one for the column
 * overloading with the specific column style if configured
 */
void zmapWindowGenColumnStyle(ZMapWindow window,ZMapFeatureColumn column)
{
  if(!column->style)
    {
      column->style_table = zmapWindowFeatureColumnStyles(window->context_map,column->unique_id);
      column->style = zmapWindowColumnMergeStyle(column);

      if(column->style_id)
        {
          ZMapFeatureTypeStyle s;

          s = window->context_map->styles.find_style(column->style_id);
          if(column->style && s && (!s->mode || s->mode == column->style->mode))
            {
              zMapStyleMerge(column->style, s);
            }
          else
            {
              column->style = s;
            }
        }
    }
}


/*
 * construct a style table for a column
 * and extract the column style or single featureset style or merge all styles into one for the column
 */
ZMapFeatureTypeStyle zMapWindowGetSetColumnStyle(ZMapWindow window,GQuark set_id)
{
  ZMapFeatureColumn column = NULL;

  if (window && window->context_map)
    column = window->context_map->getSetColumn(set_id);

  if(!column)
    return NULL;

  zmapWindowGenColumnStyle(window,column);

  return(column->style);
}

/*
 * construct a style table for a column
 * and extract the column style or single featureset style or merge all styles into one for the column
 */
ZMapFeatureTypeStyle zMapWindowGetColumnStyle(ZMapWindow window,GQuark col_id)
{
  ZMapFeatureColumn column = NULL;

  column = zMapWindowGetColumn(window->context_map,col_id);
  if(!column)
    return NULL;

  zmapWindowGenColumnStyle(window,column);

  return(column->style);
}


GList *zmapWindowFeatureColumnStyles(ZMapFeatureContextMap map, GQuark column_id)
{
  GList *styles_list = NULL;
  GList *styles_quark_list = NULL;
  int i;

  if ((styles_quark_list = (GList *)g_hash_table_lookup(map->column_2_styles,GUINT_TO_POINTER(column_id))))
    {
      GList *glist;

      if((glist = g_list_first(styles_quark_list)))
        {
          do
            {
              GQuark style_id;
              ZMapFeatureTypeStyle style;

              style_id = GPOINTER_TO_UINT(glist->data);

              if((style = map->styles.find_style(style_id)))    // add styles needed by featuresets
                {
                  styles_list = g_list_append(styles_list, (gpointer) style);

                  for(i = 1;i < ZMAPSTYLE_SUB_FEATURE_MAX;i++)        // add styles needed by this style
                    {
                      ZMapFeatureTypeStyle sub;

                      if((sub = style->sub_style[i]))
                        {
                          styles_list = g_list_append(styles_list, (gpointer) sub);
                        }
                    }
                }
              else
                {
                  zMapLogWarning("Could not find style %s for column \"%s\"",
                                 g_quark_to_string(style_id),g_quark_to_string(column_id));
                }
            }
          while((glist = g_list_next(glist)));
        }
    }

  return styles_list;
}



const char *zmapWindowGetDialogText(ZMapWindowDialogType dialog_type)
{
  const char *dialog_text = NULL ;

  switch (dialog_type)
    {
    case ZMAP_DIALOG_SHOW:
      dialog_text = "Show" ;
      break ;
    case ZMAP_DIALOG_EXPORT:
      dialog_text = "Export" ;
      break ;
    default:
      zMapWarnIfReached() ;
      break ;
    }


  return dialog_text ;
}


void zmapWindowShowStyle(ZMapFeatureTypeStyle style)
{
  ZMapIOOut dest ;
  char *string_local ;
  char *title ;

  title = g_strdup_printf("Style \"%s\"", zMapStyleGetName(style)) ;

  dest = zMapOutCreateStr(NULL, 0) ;

  zMapStylePrint(dest, style, NULL, TRUE) ;

  string_local = zMapOutGetStr(dest) ;

  zMapGUIShowText(title, string_local, FALSE) ;

  zMapOutDestroy(dest) ;

  return ;
}








/*
 *                   Internal routines
 */




static void window_cancel_cb(ZMapGuiNotebookAny notebook_any, gpointer user_data)
{
  return;
}


static void window_apply_cb(ZMapGuiNotebookAny notebook_any, gpointer user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)notebook_any;
  ZMapGuiNotebookPage page = NULL;
  ZMapWindow window = (ZMapWindow)user_data;

  if((page = zMapGUINotebookFindPage(chapter, "Colours")))
    {
      char *string_value = NULL;
      if(zMapGUINotebookGetTagValue(page, "colour_separator", "string", &string_value))
        {
          if(string_value && *string_value)
            {
              gdk_color_parse(string_value, &(window->colour_separator));
            }
        }

      if(zMapGUINotebookGetTagValue(page, "colour_item_highlight", "string", &string_value))
        {
          if(string_value && *string_value)
            {
              gdk_color_parse(string_value, &(window->colour_item_highlight));
              window->highlights_set.item = TRUE;
            }
        }

      if(zMapGUINotebookGetTagValue(page, "colour_column_highlight", "string", &string_value))
        {
          if(string_value && *string_value)
            {
              gdk_color_parse(string_value, &(window->colour_column_highlight));
              window->highlights_set.column = TRUE;
            }
        }

    }

  return ;
}

static void window_save_cb(ZMapGuiNotebookAny notebook_any, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;

  window_apply_cb(notebook_any, user_data);

  zMapLogWarning("window (%p) tried to write settings. Code not written.", window);

  zMapWarning("%s", "saving window settings not possible yet.");

  return ;
}

ZMapGuiNotebookChapter zMapWindowGetConfigChapter(ZMapWindow window, ZMapGuiNotebook parent_note_book)
{
  ZMapGuiNotebookSubsection subsection = NULL;
  ZMapGuiNotebookParagraph paragraph = NULL;
  ZMapGuiNotebookChapter chapter = NULL;
  ZMapGuiNotebookPage page = NULL;
  ZMapConfigIniContext context = NULL;
  char *colour = NULL ;
  ZMapGuiNotebookCBStruct callbacks = {
    /* data must be set later */
    window_cancel_cb, NULL,
    window_apply_cb,  NULL,
    NULL,           NULL,
    window_save_cb, NULL
  };

  /* callbacks data can't be set in initialisation... */
  callbacks.cancel_data = window;
  callbacks.apply_data  = window;
  callbacks.edit_data   = window;
  callbacks.save_data   = window;

  chapter = zMapGUINotebookCreateChapter(parent_note_book, "Window Settings", &callbacks);

  context = zMapConfigIniContextProvide(window->sequence->config_file, ZMAPCONFIG_FILE_NONE) ;

  /* Sizes... */
  page = zMapGUINotebookCreatePage(chapter, "Sizes");

  subsection = zMapGUINotebookCreateSubsection(page, NULL);

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                             ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
                                             NULL, NULL);

  zMapGUINotebookCreateTagValue(paragraph, "canvas_maxbases", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "int", window->canvas_maxwin_bases);

  zMapGUINotebookCreateTagValue(paragraph, "column_spacing", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "float", window->config.column_spacing);
#if USE_STRAND
  zMapGUINotebookCreateTagValue(paragraph, "strand_spacing", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "float", window->config.strand_spacing);
#endif

  zMapGUINotebookCreateTagValue(paragraph, "keep_empty_columns", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                "bool", window->keep_empty_cols);

  /* Colours... */
  page       = zMapGUINotebookCreatePage(chapter, "Colours");

  subsection = zMapGUINotebookCreateSubsection(page, NULL);


  paragraph  = zMapGUINotebookCreateParagraph(subsection, NULL,
                                              ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
                                              NULL, NULL);
#ifdef I_DONT_THINK_SO
  zMapGUINotebookCreateTagValue(paragraph, "colour_root", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "red");

  zMapGUINotebookCreateTagValue(paragraph, "colour_alignment", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "orange");

  zMapGUINotebookCreateTagValue(paragraph, "colour_block", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "yellow");
#endif /* I_DONT_THINK_SO */

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_SEPARATOR, &colour))
    colour = (char *)ZMAP_WINDOW_STRAND_DIVIDE_COLOUR;

  zMapGUINotebookCreateTagValue(paragraph, "colour_separator", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", colour);

#ifdef I_DONT_THINK_SO
  zMapGUINotebookCreateTagValue(paragraph, "colour_m_forward", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "blue");

  zMapGUINotebookCreateTagValue(paragraph, "colour_m_reverse", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "indigo");

  zMapGUINotebookCreateTagValue(paragraph, "colour_q_forward", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "violet");

  zMapGUINotebookCreateTagValue(paragraph, "colour_q_reverse", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "apple");

  zMapGUINotebookCreateTagValue(paragraph, "colour_m_forwardcol", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "pear");
  zMapGUINotebookCreateTagValue(paragraph, "colour_m_reversecol", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "banana");
  zMapGUINotebookCreateTagValue(paragraph, "colour_q_forwardcol", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "cherry");
  zMapGUINotebookCreateTagValue(paragraph, "colour_q_reversecol", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", "kiwi");
#endif /* I_DONT_THINK_SO... */

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_ITEM_HIGH, &colour))
    colour = (char *)ZMAP_WINDOW_COLUMN_HIGHLIGHT;

  zMapGUINotebookCreateTagValue(paragraph, "colour_item_highlight", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", colour);

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_COL_HIGH, &colour))
    colour = (char *)ZMAP_WINDOW_COLUMN_HIGHLIGHT;

  zMapGUINotebookCreateTagValue(paragraph, "colour_column_highlight", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", colour);

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_RUBBER_BAND, &colour))
    colour = (char *)ZMAP_WINDOW_RUBBER_BAND;

  zMapGUINotebookCreateTagValue(paragraph, "colour_rubber_band", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", colour);

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_HORIZON, &colour))
    colour = (char *)ZMAP_WINDOW_HORIZON;

  zMapGUINotebookCreateTagValue(paragraph, "colour_horizon", NULL,
                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                "string", colour);

  zMapConfigIniContextDestroy(context);

  return chapter;
}

