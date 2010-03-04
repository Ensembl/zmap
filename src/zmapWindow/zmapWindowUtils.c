/*  File: zmapWindowUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Utility functions for the zMapWindow code.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Jan 22 11:22 2010 (edgrif)
 * Created: Thu Jan 20 14:43:12 2005 (edgrif)
 * CVS info:   $Id: zmapWindowUtils.c,v 1.58 2010-03-04 15:13:28 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>

#include <zmapWindowCanvasItem.h>


/* Struct for style table callbacks. */
typedef struct
{
  ZMapWindowStyleTableCallback func ;
  gpointer data ;
} ZMapWindowStyleTableDataStruct, *ZMapWindowStyleTableData ;




static void styleDestroyCB(gpointer data) ;
static void styleTableHashCB(gpointer key, gpointer value, gpointer user_data) ;




/* Some simple coord calculation routines, if these prove too expensive they
 * can be replaced with macros. */



/* Transforming coordinates for a revcomp'd sequence:
 * 
 * Users sometimes want to see coords transformed to a "negative" version of the forward
 * coords when viewing a revcomp'd sequence, i.e. 1 -> 365700 becomes -365700 -> -1
 * 
 * These functions take care of this for zmap where we don't actually change the
 * underlying sequence coords of features to be negative but simply display them
 * as negatives to the user.
 * 
 *  */
int zmapWindowCoordToDisplay(ZMapWindow window, int coord)
{
  int new_coord ;

  new_coord = coord - (window->origin - 1) ;

  return new_coord ;
}

int zmapWindowCoordFromDisplay(ZMapWindow window, int coord)
{
  int new_coord ;
  int origin ;

  if (window->revcomped_features && window->display_forward_coords)
    origin = (window->origin - 2) * -1 ;
  else
    origin = window->origin ;

  new_coord = coord - (origin - 1) ;

  return new_coord ;
}


/* Use if you have no window.... */
int zmapWindowCoordFromOriginRaw(int origin, int coord)
{
  int new_coord ;

  new_coord = coord - (origin - 1) ;

  return new_coord ;
}


ZMapStrand zmapWindowStrandToDisplay(ZMapWindow window, ZMapStrand strand_in)
{
  ZMapStrand strand = ZMAPSTRAND_NONE ;

  if (window->revcomped_features && window->display_forward_coords)
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
  double extent ;

  zMapAssert(start <= end) ;

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
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

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
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout - *start_inout ;		    /* do this first before zeroing start ! */

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
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout - *start_inout ;		    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  *end_inout = *end_inout + 1 ;

  return ;
}


/* NOTE: offset may not be quite what you think, the routine recalculates */
void zmapWindowSeq2CanOffset(double *start_inout, double *end_inout, double offset)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

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

  zMapAssert(window_array_inout) ;

  if ((window_array = *window_array_inout))
    {
      /* Slightly tricky here, when the list widget gets destroyed it removes itself
       * from the array so the array shrinks by one, hence we just continue to access
       * the first element until all elements are gone. */
      while (window_array->len)
	{
	  GtkWidget *widget ;

	  widget = g_ptr_array_index(window_array, 0) ;

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






void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out)
{
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

  *top_out = scroll_y1 ;
  *bottom_out = scroll_y2 ;

  return ;
}


/* 
 * Noddy functions for handling our style lists within window.
 * 
 * These might be noddy _but_ they are mucky, the logic in calling routines
 * is muddied and we need to clear this up....
 * 
 */

gboolean zmapWindowUpdateStyles(ZMapWindow window, GData **read_only_styles, GData **display_styles)
{
  gboolean result = FALSE ;

  if (*read_only_styles)
    {
      if (window->read_only_styles)
	zMapStyleDestroyStyles(&(window->read_only_styles)) ;

      result = zMapStyleCopyAllStyles(read_only_styles, &(window->read_only_styles)) ;
    }

  if (*display_styles)
    {
      if (window->display_styles)
	zMapStyleDestroyStyles(&(window->display_styles)) ;

      result = zMapStyleCopyAllStyles(display_styles, &(window->display_styles)) ;
    }

  return result ;
}


gboolean zmapWindowGetMarkedSequenceRangeFwd(ZMapWindow       window, 
					     ZMapFeatureBlock block,
					     int *start, int *end)
{
  gboolean result = FALSE ;

  result = zmapWindowMarkGetSequenceRange(window->mark, start, end);

  if(result && window->revcomped_features && start && end)
    {
      int seq_end, x, y, z;

      /* Need to reverse complement the mark here... */
      seq_end = block->block_to_sequence.q2;
      x = *start;
      y = *end;

      /* swop */
      z = x;
      x = y;
      y = z;

      /* invert */
      x = seq_end - x + 1;

      /* invert */
      y = seq_end - y + 1;

      *start = x;
      *end   = y;
    }

  return result ;
}




/* 
 *                      Style table functions
 * 
 */

/* Set of functions for managing hash tables of styles for features. Each column in zmap has
 * a hash table of styles for all the features it displays. */
GHashTable *zmapWindowStyleTableCreate(void)
{
  GHashTable *style_table = NULL ;

  style_table = g_hash_table_new_full(NULL, NULL, NULL, styleDestroyCB) ;

  return style_table ;
}


/* Adds a _copy_ of the style to the list of feature styles using the styles unique_id as the key.
 * NOTE that if the style is already there it is not added as this would overwrite
 * the existing style.
 * 
 * If the style was there or was added successfully then a pointer to the style is returned,
 * otherwise NULL is returned.
 *  */
ZMapFeatureTypeStyle zmapWindowStyleTableAddCopy(GHashTable *style_table, ZMapFeatureTypeStyle new_style)
{
  ZMapFeatureTypeStyle style = NULL ;

  zMapAssert(style_table && new_style) ;

  if (!(style = g_hash_table_lookup(style_table, GINT_TO_POINTER(zMapStyleGetUniqueID(new_style)))))
    {
      style = zMapFeatureStyleCopy(new_style) ;

      g_hash_table_insert(style_table, GINT_TO_POINTER(zMapStyleGetUniqueID(new_style)), style) ;
    }

  return style ;
}


/* Finds a feature style using the styles unique_id. */
ZMapFeatureTypeStyle zmapWindowStyleTableFind(GHashTable *style_table, GQuark style_id)
{
  ZMapFeatureTypeStyle style ;

  zMapAssert(style_table && style_id) ;

  style = g_hash_table_lookup(style_table, GINT_TO_POINTER(style_id)) ;

  return style ;
}


/* Calls the app_func for each of the styles in the table. */
void zmapWindowStyleTableForEach(GHashTable *style_table,
				 ZMapWindowStyleTableCallback app_func, gpointer app_data)
{
  ZMapWindowStyleTableDataStruct table_data ;

  zMapAssert(style_table && app_func) ;			    /* Do not assert app_data it can be zero. */

  table_data.func = app_func ;
  table_data.data = app_data ;

  /* Call the users style processing function. */
  g_hash_table_foreach(style_table, styleTableHashCB, &table_data) ;


  return ;
}

/* Destroys the style table. */
void zmapWindowStyleTableDestroy(GHashTable *style_table)
{
  zMapAssert(style_table) ;

  g_hash_table_destroy(style_table) ;

  return ;
}

/* End of Style Table functions... */


/* Free the list, but not the styles    ......um, what does this mean.....EG */
/* It means there's no copying of the style from GData[all_styles] ->
 * GList[some_styles] going on in this function. Just allocation of
 * GList items to hold onto the styles pointers. */
GList *zmapWindowFeatureSetStyles(ZMapWindow window, GData *all_styles, GQuark feature_set_id)
{
  GList *styles_list = NULL;
  GList *styles_quark_list = NULL;
  

  if ((styles_quark_list = g_hash_table_lookup(window->featureset_2_styles, 
					       GUINT_TO_POINTER(feature_set_id))))
    {
      GList *list;

      if((list = g_list_first(styles_quark_list)))
	{
	  do
	    {
	      GQuark style_id;
	      ZMapFeatureTypeStyle style;

	      style_id = GPOINTER_TO_UINT(list->data);

	      if((style = zMapFindStyle(all_styles, style_id)))
		{
		  styles_list = g_list_append(styles_list, style);
		}
	    }
	  while((list = g_list_next(list)));
	}
    }

  return styles_list;
}


char *zmapWindowGetDialogText(ZMapWindowDialogType dialog_type)
{
  char *dialog_text = NULL ;

  switch (dialog_type)
    {
    case ZMAP_DIALOG_SHOW:
      dialog_text = "Show" ;
      break ;
    case ZMAP_DIALOG_EXPORT:
      dialog_text = "Export" ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }


  return dialog_text ;
}


void zmapWindowShowStyle(ZMapFeatureTypeStyle style)
{
  ZMapIOOut dest ;
  char *string ;
  char *title ;

  title = g_strdup_printf("Style \"%s\"", zMapStyleGetName(style)) ;

  dest = zMapOutCreateStr(NULL, 0) ;

  zMapStylePrint(dest, style, NULL, TRUE) ;

  string = zMapOutGetStr(dest) ;

  zMapGUIShowText(title, string, FALSE) ;

  zMapOutDestroy(dest) ;

  return ;
}





void zMapWindowUtilsSetClipboard(ZMapWindow window, char *text)
{
  zMapGUISetClipboard(window->toplevel, text);
  return ;
}

void zmapWindowToggleMark(ZMapWindow window, guint keyval)
{
  FooCanvasItem *focus_item ;
  
  if (zmapWindowMarkIsSet(window->mark))
    {
      zMapWindowStateRecord(window);
      /* Unmark an item. */
      zmapWindowMarkReset(window->mark) ;
    }
  else
    {
      zMapWindowStateRecord(window);
      /* If there's a focus item we mark that, otherwise we check if the user set
       * a rubber band area and use that, otherwise we mark to the screen area. */
      if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
	{
	  ZMapFeature feature ;
	  
	  feature = zMapWindowCanvasItemGetFeature(focus_item);
	  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;
	  
	  /* If user presses 'M' we mark "whole feature", e.g. whole transcript, 
	   * all HSP's, otherwise we mark just the highlighted ones. */
	  if (keyval == GDK_M)
	    {
	      if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
		{
		  GList *list = NULL;
		  ZMapStrand set_strand ;
		  ZMapFrame set_frame ;
		  gboolean result ;
		  double rootx1, rooty1, rootx2, rooty2 ;
		  
		  result = zmapWindowItemGetStrandFrame(focus_item, &set_strand, &set_frame) ;
		  zMapAssert(result) ;
		  
		  list = zmapWindowFToIFindSameNameItems(window->context_to_item,
							 zMapFeatureStrand2Str(set_strand),
							 zMapFeatureFrame2Str(set_frame),
							 feature) ;
		  
		  zmapWindowGetMaxBoundsItems(window, list, &rootx1, &rooty1, &rootx2, &rooty2) ;
		  
		  zmapWindowMarkSetWorldRange(window->mark, rootx1, rooty1, rootx2, rooty2) ;
		  
		  g_list_free(list) ;
		}
	      else
		{
		  zmapWindowMarkSetItem(window->mark, focus_item) ;
		}
	    }
	  else
	    {
	      GList *focus_items ;
	      double rootx1, rooty1, rootx2, rooty2 ;
	      
	      focus_items = zmapWindowFocusGetFocusItems(window->focus) ;
	      
	      if(g_list_length(focus_items) == 1)
		{
		  zmapWindowMarkSetItem(window->mark, focus_items->data);
		}
	      else
		{
		  zmapWindowGetMaxBoundsItems(window, focus_items, &rootx1, &rooty1, &rootx2, &rooty2) ;
		  
		  zmapWindowMarkSetWorldRange(window->mark, rootx1, rooty1, rootx2, rooty2) ;
		}
	      
	      g_list_free(focus_items) ;
	    }
	}
      else if (window->rubberband)
	{
	  double rootx1, rootx2, rooty1, rooty2 ;
	  
	  my_foo_canvas_item_get_world_bounds(window->rubberband, &rootx1, &rooty1, &rootx2, &rooty2);
	  
	  zmapWindowClampedAtStartEnd(window, &rooty1, &rooty2);
	  /* We ignore any failure, perhaps we should warn the user ? If we colour
	   * the region it will be ok though.... */
	  zmapWindowMarkSetWorldRange(window->mark, rootx1, rooty1, rootx2, rooty2) ;
	}
      else
	{
	  /* If there is no feature selected and no rubberband set then mark to the
	   * visible screen. */
	  double x1, x2, y1, y2;
	  double margin ;
	  
	  zmapWindowItemGetVisibleCanvas(window, &x1, &y1, &x2, &y2) ;
	  
	  zmapWindowClampedAtStartEnd(window, &y1, &y2) ;
	  
	  /* Make the mark visible to the user by making its extent slightly smaller
	   * than the window. */
	  margin = 15.0 * (1 / window->canvas->pixels_per_unit_y) ;
	  y1 += margin ;
	  y2 -= margin ;
	  /* We only ever want the mark as wide as the scroll region */
	  zmapWindowGetScrollRegion(window, NULL, NULL, &x2, NULL);
	  
	  zmapWindowMarkSetWorldRange(window->mark, x1, y1, x2, y2) ;
	}
    }

  return ;
}

static void window_cancel_cb(ZMapGuiNotebookAny notebook_any, gpointer user_data)
{
  return;
}
#ifdef NOT_REQUIRED
static void recolour_backgrounds_cb(FooCanvasGroup *data, FooCanvasPoints *points, 
				    ZMapContainerLevelType level, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;
  FooCanvasItem *background = NULL;
  GdkColor *colour = NULL;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_BLOCK:
      background = zmapWindowContainerGetBackground(data);
      colour     = &(window->colour_separator);
      break;
    case ZMAPCONTAINER_LEVEL_STRAND:
      background = zmapWindowContainerGetBackground(data);
      if(zmapWindowContainerIsStrandSeparator(data))
	colour   = &(window->colour_separator);
      break;
    default:
      break;
    }

  if(background && colour)
    foo_canvas_item_set(background,
			"fill_color_gdk", colour,
			NULL);

  return ;
}
#endif /* NOT_REQUIRED */

static void recolour_backgrounds(ZMapWindow window)
{
#ifdef NOT_REQUIRED
  zmapWindowContainerUtilsExecute(window->feature_root_group,
				  ZMAPCONTAINER_LEVEL_FEATURESET,
				  recolour_backgrounds_cb, window);
#endif /* NOT_REQUIRED */
  return ;
}
static void window_apply_cb(ZMapGuiNotebookAny notebook_any, gpointer user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)notebook_any;
  ZMapGuiNotebookPage page = NULL;
  ZMapWindow window = (ZMapWindow)user_data;
  gboolean update_req = FALSE;

  if((page = zMapGUINotebookFindPage(chapter, "Colours")))
    {
      char *string_value = NULL;
      if(zMapGUINotebookGetTagValue(page, "colour_separator", "string", &string_value))
	{
	  if(string_value && *string_value)
	    {
	      update_req = gdk_color_parse(string_value, &(window->colour_separator));
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

  if(update_req)
    recolour_backgrounds(window);

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
  ZMapGuiNotebookTagValue tagvalue = NULL;
  ZMapGuiNotebookChapter chapter = NULL;
  ZMapGuiNotebookPage page = NULL;
  ZMapConfigIniContext context = NULL;
  char *colour = NULL;
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

  chapter    = zMapGUINotebookCreateChapter(parent_note_book, "Window Settings", &callbacks);

  context    = zMapConfigIniContextProvide();


  /* Sizes... */
  page       = zMapGUINotebookCreatePage(chapter, "Sizes");

  subsection = zMapGUINotebookCreateSubsection(page, NULL);

  paragraph  = zMapGUINotebookCreateParagraph(subsection, NULL,
					      ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
					      NULL, NULL);

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "canvas_maxbases",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "int", window->canvas_maxwin_bases);

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "column_spacing",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "float", window->config.column_spacing);

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "strand_spacing",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "float", window->config.strand_spacing);

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "keep_empty_columns",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
					     "bool", window->keep_empty_cols);

  /* Colours... */
  page       = zMapGUINotebookCreatePage(chapter, "Colours");

  subsection = zMapGUINotebookCreateSubsection(page, NULL);


  paragraph  = zMapGUINotebookCreateParagraph(subsection, NULL,
					      ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
					      NULL, NULL);
#ifdef I_DONT_THINK_SO
  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_root",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "red");

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_alignment",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "orange");

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_block",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "yellow");
#endif /* I_DONT_THINK_SO */

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				    ZMAPSTANZA_WINDOW_SEPARATOR, &colour))
    colour   = ZMAP_WINDOW_STRAND_DIVIDE_COLOUR;
  
  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_separator",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", colour);

#ifdef I_DONT_THINK_SO
  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_m_forward",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "blue");

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_m_reverse",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "indigo");

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_q_forward",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "violet");

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_q_reverse",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "apple");

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_m_forwardcol",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "pear");
  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_m_reversecol",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "banana");
  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_q_forwardcol",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "cherry");
  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_q_reversecol",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", "kiwi");
#endif /* I_DONT_THINK_SO... */

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				    ZMAPSTANZA_WINDOW_ITEM_HIGH, &colour))
    colour   = ZMAP_WINDOW_COLUMN_HIGHLIGHT;

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_item_highlight",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", colour);

  if(!zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				    ZMAPSTANZA_WINDOW_COL_HIGH, &colour))
    colour   = ZMAP_WINDOW_COLUMN_HIGHLIGHT;

  tagvalue   = zMapGUINotebookCreateTagValue(paragraph, "colour_column_highlight",
					     ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					     "string", colour);

  zMapConfigIniContextDestroy(context);

  return chapter;
}


/* 
 *                  Internal routines.
 */


/* Called when a style is destroyed and the styles hash of feature_styles is destroyed. */
static void styleDestroyCB(gpointer data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  zMapFeatureTypeDestroy(style) ;

  return ;
}



/* A GHFunc() callback, used by zmapWindowStyleTableForEach() to call the applications
 * style callback. */
static void styleTableHashCB(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)value ;
  ZMapWindowStyleTableData style_data = (ZMapWindowStyleTableData)user_data ;

  /* Call the apps function passing in the current style and the apps data. */
  (style_data->func)(style, style_data->data) ;

  return ;
}
