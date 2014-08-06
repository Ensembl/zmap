/*  File: zmapWindowDisplayOps.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014: Genome Research Ltd.
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
 * Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: This file is a bit of an experiment, it's supposed to
 *              contain display related functions such as "zoom to 
 *              position from coords on clipboard" and other such things
 *              that don't sit happily in other files.
 *
 * Exported functions: See zmapWindow_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <zmapWindow_P.h>


/* Struct describing a part of a feature...are we sure this doesn't already exist
 * somewhere else....????? */
typedef struct FeatureCoordStructType
{
  char *name ;
  int start ;
  int end ;
  int length ;
} FeatureCoordStruct, *FeatureCoord ;


static void setUpFeatureTranscript(gboolean revcomped, ZMapWindowDisplayStyle display_style,
                                   ZMapFeature feature, GArray *feature_coords) ;
static void setUpFeatureOther(ZMapWindowDisplayStyle display_style, ZMapFeature feature, GArray *feature_coords) ;
static void makeSelectionString(ZMapWindow window, ZMapWindowDisplayStyle display_style,
                                GString *selection_str, GArray *feature_coords) ;
static gint sortCoordsCB(gconstpointer a, gconstpointer b) ;





/* 
 *                      External routines
 */


gboolean zMapWindowZoomFromClipboard(ZMapWindow window)
{
  gboolean result = FALSE ;


  /* Get current position we need a function to do this..... */
  /* HOW DO WE DO THIS....????? */

  result = zmapWindowZoomFromClipboard(window, 200, 200) ;


  return result ;
}


/* If there is a selected feature then returns appropriate string for clipboard for that feature,
 * if there is no feature then returns the mark coords to the clipboard, if there is no mark
 * then returns the name of any selected column, failing that returns the empty string.
 * 
 * ALL returned strings should be g_free'd when no longer wanted. */
char *zMapWindowGetSelectionText(ZMapWindow window)
{
  char *selection_txt = NULL ;
  ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_NATURAL, ZMAPWINDOW_PASTE_FORMAT_BROWSER,
                                                ZMAPWINDOW_PASTE_TYPE_EXTENT} ;

  /* find out if there is a feature selected or column selected and return their data. */
  if (!(selection_txt = zmapWindowMakeFeatureSelectionText(window, &display_style, NULL))
      && !(selection_txt = zmapWindowMakeColumnSelectionText(window, 0, 0, &display_style, NULL)))
    selection_txt = g_strdup("") ;

  return selection_txt ;
}




/* 
 *                      Package routines
 */



/* Get contents of clipboard and see if it parses as a chromosome
 * location in the form "21:36,444,999-37,666,888", if it does
 * then zoom to it.
 * 
 * To do this we need the clipboard contents but.....which one ?
 * 
 * Cntl-C seems always to go to GDK_SELECTION_CLIPBOARD
 * 
 * cursor selecting with mouse seems to go to GDK_SELECTION_PRIMARY
 * 
 * We use curr_world_x to try to stay located on the original column.
 * 
 *  */
gboolean zmapWindowZoomFromClipboard(ZMapWindow window, double curr_world_x, double curr_world_y)
{
  gboolean result = FALSE ;
  char *clipboard_primary_text = NULL ;
  char *clipboard_text = NULL ;
  char *chromosome ;
  int start, end ;

  /* Get both clipboards and check their contents, first that works is accepted. */
  zMapGUIGetClipboard(GDK_SELECTION_PRIMARY, &clipboard_primary_text) ;
  zMapGUIGetClipboard(GDK_SELECTION_CLIPBOARD, &clipboard_text) ;

  if (clipboard_primary_text || clipboard_text)
    {
      if ((clipboard_primary_text && zMapUtilsBioParseChromLoc(clipboard_primary_text, &chromosome, &start, &end))
          || (clipboard_text && zMapUtilsBioParseChromLoc(clipboard_text, &chromosome, &start, &end)))
        {
          double world_start, world_end ;

          /* convert sequence to canvas world and zoom there. */
          if (zmapWindowSeqToWorldCoords(window, start, end, &world_start, &world_end))
            {
              zmapWindowZoomToWorldPosition(window, TRUE,
                                            curr_world_x, world_start, curr_world_x, world_end) ;

              g_free(chromosome) ;

              result = TRUE ;
            }
        }
    }

  return result ;
}


/* Makes text for posting to clipboard when there is no feature selected. 
 *
 * Logic is slightly arcane in an attempt to "do the right thing":
 * 
 * If the mark is set and wy is within the top or bottom
 * shaded area of the mark or there is no selected column then the mark coords
 * are returned otherwise if there is a selected column then the column name
 * is returned otherwise NULL is returned.
 * 
 * Note: wy == 0  (no position) is assumed to mean within the mark.
 * 
 *  */
char *zmapWindowMakeColumnSelectionText(ZMapWindow window, double wx, double wy, ZMapWindowDisplayStyle display_style,
                                        ZMapWindowContainerFeatureSet selected_column)
{
  char *selection_txt = NULL ;
  gboolean mark_set = FALSE, in_mark = FALSE ;

  /* Check if there is a mark and whether wy is within the mark */
  if ((mark_set = zmapWindowMarkIsSet(window->mark)))
    {
      double world_x1, world_y1, world_x2, world_y2 ;

      zmapWindowMarkGetWorldRange(window->mark,
                                  &world_x1, &world_y1,
                                  &world_x2, &world_y2) ;

      if (wy < world_y1 || wy > world_y2)
        in_mark = TRUE ;
    }

  /* Is there a selected column ? */
  if (!selected_column)
    {
      FooCanvasGroup *focus_column ;

      if ((focus_column = zmapWindowFocusGetHotColumn(window->focus)))
        {
          selected_column = (ZMapWindowContainerFeatureSet)focus_column ;
        }
    }

  if (mark_set && (in_mark || !selected_column))
    {
      int start, end ;
      char *chromosome = NULL ;

      zmapWindowMarkGetSequenceRange(window->mark, &start, &end) ;

      if (display_style->coord_frame == ZMAPWINDOW_COORD_ONE_BASED)
        {
          zmapWindowCoordPairToDisplay(window, start, end, &start, &end) ;
        }
      else
        {
          ZMapFeatureAlignment align ;

          align = window->feature_context->master_align ;
          chromosome = zMapFeatureAlignmentGetChromosome(align) ;

          zmapWindowParentCoordPairToDisplay(window, start, end,
                                             &start, &end) ;
        }

      if (start > end)
        zMapUtilsSwop(int, start, end) ;

      /* We should have a func to make this string..... */
      selection_txt = g_strdup_printf("%s%s%d-%d",
                                      (chromosome ? chromosome : ""),
                                      (chromosome ? ":" : ""),
                                      start, end) ;
    }
  else if (selected_column)
    {
      ZMapFeatureColumn gff ;
      GQuark column_id ;

      column_id = zmapWindowContainerFeaturesetGetColumnUniqueId(selected_column) ;

      gff = g_hash_table_lookup(window->context_map->columns, GUINT_TO_POINTER(column_id)) ;

      if (gff && gff->column_desc)
        selection_txt = g_strdup(gff->column_desc) ;
    }


  return selection_txt ;
}


/* Makes text for posting to clipboard for the selected feature.
 * 
 * If no selected_feature is passed in the feature(s) come from the focus
 * list, if their are none in the focus list then NULL is returned.
 * */
char *zmapWindowMakeFeatureSelectionText(ZMapWindow window,
                                         ZMapWindowDisplayStyle display_style, ZMapFeature selected_feature)
{
  char *selection = NULL ;
  GList *selected ;
  gint length ;
  ZMapFeature item_feature = selected_feature ;
  GString *text ;
  gboolean free_text ;
  GArray *feature_coords ;
  gboolean revcomped ;

  text = g_string_sized_new(512) ;
  feature_coords = g_array_new(FALSE, FALSE, sizeof(FeatureCoordStruct)) ;
  revcomped = window->flags[ZMAPFLAG_REVCOMPED_FEATURES] ;

  /* If we are passed a feature then just process that, otherwise try to find one in
   * the focus item list. */
  if (item_feature)
    {
      if (item_feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
        {
          setUpFeatureTranscript(revcomped, display_style, item_feature, feature_coords) ;

          makeSelectionString(window, display_style, text, feature_coords) ;
        }
      else
        {
          setUpFeatureOther(display_style, item_feature, feature_coords) ;

          makeSelectionString(window, display_style, text, feature_coords) ;
        }
    }
  else if ((selected = zmapWindowFocusGetFocusItems(window->focus)) && (length = g_list_length(selected)))
    {
      /* If there are any focus items then put their coords in the X Windows paste buffer. */
      ID2Canvas id2c;
      FooCanvasItem *item;
      ZMapWindowCanvasItem canvas_item;
      char *name ;

      id2c = (ID2Canvas) selected->data;
      item = FOO_CANVAS_ITEM(id2c->item) ;

      if (ZMAP_IS_CANVAS_ITEM(item))
        canvas_item = ZMAP_CANVAS_ITEM( item );
      else
        canvas_item = zMapWindowCanvasItemIntervalGetObject(item) ;

      item_feature = (ZMapFeature) id2c->feature_any ;

      name = (char *)g_quark_to_string(item_feature->original_id) ;

      /* Processing is different if there is only one item highlighted and it's a transcript. */
      if (ZMAP_IS_CANVAS_ITEM(item) && length == 1 && item_feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
        {
          setUpFeatureTranscript(revcomped, display_style, item_feature, feature_coords) ;

          makeSelectionString(window, display_style, text, feature_coords) ;
        }
      else
        {
          while (selected)
            {
              id2c = (ID2Canvas)(selected->data) ;
              item = FOO_CANVAS_ITEM(id2c->item) ;

              if (ZMAP_IS_CANVAS_ITEM(item))
                canvas_item = ZMAP_CANVAS_ITEM(item) ;
              else
                canvas_item = zMapWindowCanvasItemIntervalGetObject(item) ;

              item_feature = (ZMapFeature)(id2c->feature_any) ;

              setUpFeatureOther(display_style, item_feature, feature_coords) ;

              selected = selected->next ;
            }

          makeSelectionString(window, display_style, text, feature_coords) ;
        }

      selected = g_list_first(selected) ;
      g_list_free(selected) ;
    }

  g_array_free(feature_coords, TRUE) ;

  if (!(text->len))
    free_text = TRUE ;
  else
    free_text = FALSE ;
    
  selection = g_string_free(text, free_text) ;

  return selection ;
}


/* Return an array of feature coords, name, start/end, for the given transcript feature. */
static void setUpFeatureTranscript(gboolean revcomped, ZMapWindowDisplayStyle display_style,
                                   ZMapFeature feature, GArray *feature_coords)
{
  char *name ;

  name = (char *)g_quark_to_string(feature->original_id) ;


  if (display_style->paste_feature == ZMAPWINDOW_PASTE_TYPE_EXTENT)
    {
      FeatureCoordStruct feature_coord ;

      feature_coord.name = name ;
      feature_coord.start = feature->x1 ;
      feature_coord.end = feature->x2 ;
      feature_coord.length = (feature->x2 - feature->x1 + 1) ;
              
      feature_coords = g_array_append_val(feature_coords, feature_coord) ;
    }
  else
    {
      /* For a transcript feature put all the exons in the paste buffer. */
      ZMapSpan span ;
      int i ;

      for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
        {
          FeatureCoordStruct feature_coord ;
          int index ;

          if (revcomped)
            index = feature->feature.transcript.exons->len - (i + 1) ;
          else
            index = i ;

          span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, index) ;

          feature_coord.name = name ;
          feature_coord.start = span->x1 ;
          feature_coord.end = span->x2 ;
          feature_coord.length = (span->x2 - span->x1 + 1) ;

          feature_coords = g_array_append_val(feature_coords, feature_coord) ;
        }
    }

  return ;
}

/* Return an array of feature coords, name, start/end, for the given non-transcript feature. */
static void setUpFeatureOther(ZMapWindowDisplayStyle display_style, ZMapFeature feature, GArray *feature_coords)
{
  int selected_start, selected_end, selected_length ;
  ZMapFeatureSubpartType item_type_int ;
  FeatureCoordStruct feature_coord ;

  /* this is not a get sub part issue, we want the whole canvas feature which is a
   * single 'exon' in a bigger alignment */
  selected_start = feature->x1 ;
  selected_end = feature->x2 ;
  selected_length = feature->x2 - feature->x1 + 1 ;
  item_type_int = ZMAPFEATURE_SUBPART_MATCH ;

  feature_coord.name = (char *)g_quark_to_string(feature->original_id) ;
  feature_coord.start = selected_start ;
  feature_coord.end = selected_end ;
  feature_coord.length = selected_length ;
                
  feature_coords = g_array_append_val(feature_coords, feature_coord) ;

  return ;
}



/* Make the string that will be pasted using the supplied coords. */
static void makeSelectionString(ZMapWindow window, ZMapWindowDisplayStyle display_style,
                                GString *selection_str, GArray *feature_coords)
{
  int i ;
  ZMapFeatureAlignment align ;
  char *chromosome ;

  align = window->feature_context->master_align ;
  chromosome = zMapFeatureAlignmentGetChromosome(align) ;

  for (i = 0 ; i < feature_coords->len ; i++)
    {
      FeatureCoord feature_coord ;

      feature_coord = &g_array_index(feature_coords, FeatureCoordStruct, i) ;

      if (display_style->coord_frame == ZMAPWINDOW_COORD_ONE_BASED)
        {
          zmapWindowCoordPairToDisplay(window, feature_coord->start, feature_coord->end,
                                       &feature_coord->start, &feature_coord->end) ;
        }
      else
        {
          zmapWindowParentCoordPairToDisplay(window, feature_coord->start, feature_coord->end,
                                             &feature_coord->start, &feature_coord->end) ;
        }


      if (feature_coord->start > feature_coord->end)
        zMapUtilsSwop(int, feature_coord->start, feature_coord->end) ;
    }

  g_array_sort(feature_coords, sortCoordsCB) ;


  /* For "otterlace" style we display the coords + other data for each feature_coord, for
   * browser style we just give the overall extent of all the feature_coords. */
  if (display_style->paste_style == ZMAPWINDOW_PASTE_FORMAT_OTTERLACE)
    {
      for (i = 0 ; i < feature_coords->len ; i++)
        {
          FeatureCoord feature_coord ;

          feature_coord = &g_array_index(feature_coords, FeatureCoordStruct, i) ;

          g_string_append_printf(selection_str, "\"%s\"    %d %d (%d)%s",
                                 feature_coord->name,
                                 feature_coord->start, feature_coord->end, feature_coord->length,
                                 (i < feature_coords->len ? "\n" : "")) ;
        }
    }
  else
    {
      /* We can do this because the feature_coords array is sorted on position. */
      int multi_start, multi_end ;      
      FeatureCoord feature_coord ;

      feature_coord = &g_array_index(feature_coords, FeatureCoordStruct, 0) ;
      multi_start = feature_coord->start ;

      feature_coord = &g_array_index(feature_coords, FeatureCoordStruct, (feature_coords->len - 1)) ;
      multi_end = feature_coord->end ;

      g_string_append_printf(selection_str, "%s%s%d-%d",
                             (chromosome ? chromosome : ""),
                             (chromosome ? ":" : ""),
                             multi_start, multi_end) ;
    }

  return ;
}


/* Sorts lists of features names and their coords, sorts on name first then coord. */
static gint sortCoordsCB(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  FeatureCoord feature_coord1 = (FeatureCoord)a ;
  FeatureCoord feature_coord2 = (FeatureCoord)b ;

  if ((result = g_ascii_strcasecmp(feature_coord1->name, feature_coord2->name)) == 0)
    {
      if (feature_coord1->start < feature_coord2->start)
        {
          result = -1 ;
        }
      else if (feature_coord1->start == feature_coord2->start)
        {
          if (feature_coord1->end < feature_coord2->end)
            result = -1 ;
          else if (feature_coord1->end == feature_coord2->end)
            result = 0 ;
          else
            result = 1 ;
        }
      else
        {
          result = 1 ;
        }
    }

  return result ;
}




