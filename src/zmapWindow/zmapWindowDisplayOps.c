/*  File: zmapWindowDisplayOps.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014-2015: Genome Research Ltd.
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
                                   gboolean use_mark, int mark_x, int mark_y,
                                   ZMapFeature feature,  ZMapFeatureSubPart sub_part, GArray *feature_coords) ;
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
char *zMapWindowGetSelectionText(ZMapWindow window, ZMapWindowDisplayStyle display_style)
{
  char *selection_txt = NULL ;

  /* find out if there is a feature selected or column selected and return their data. */
  if (!(selection_txt = zmapWindowMakeFeatureSelectionTextFromSelection(window, display_style))
      && !(selection_txt = zmapWindowMakeColumnSelectionText(window, 0, 0, display_style, NULL)))
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

      gff = (ZMapFeatureColumn)g_hash_table_lookup(window->context_map->columns, GUINT_TO_POINTER(column_id)) ;

      if (gff && gff->column_desc)
        selection_txt = g_strdup(gff->column_desc) ;
    }


  return selection_txt ;
}


/* Makes text for posting to clipboard from the supplied feature.
 *
 * ZMapWindowPasteFeatureType of display_style cannot be ZMAPWINDOW_PASTE_TYPE_SELECTED.
 *
 */
char *zmapWindowMakeFeatureSelectionTextFromFeature(ZMapWindow window,
                                                    ZMapWindowDisplayStyle display_style, ZMapFeature feature)
{
  char *selection = NULL ;
  GString *text ;
  gboolean free_text ;
  GArray *feature_coords ;
  gboolean revcomped ;

  zMapReturnValIfFailSafe((window && display_style->paste_feature != ZMAPWINDOW_PASTE_TYPE_SELECTED && feature),
                          NULL) ;

  text = g_string_sized_new(512) ;
  feature_coords = g_array_new(FALSE, FALSE, sizeof(FeatureCoordStruct)) ;
  revcomped = window->flags[ZMAPFLAG_REVCOMPED_FEATURES] ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      int mark_x = 0, mark_y = 0 ;
      gboolean use_mark = FALSE ;

      if (zmapWindowMarkIsSet(window->mark)
          && zmapWindowMarkGetSequenceRange(window->mark, &mark_x, &mark_y))
        {
          if (mark_x && mark_y
              && ((mark_x >= feature->x1 && mark_x <= feature->x2)
                  || (mark_y >= feature->x1 && mark_y <= feature->x2)))
          use_mark = TRUE ;
        }

      setUpFeatureTranscript(revcomped, display_style,
                             use_mark, mark_x, mark_y,
                             feature, NULL, feature_coords) ;

      makeSelectionString(window, display_style, text, feature_coords) ;
    }
  else
    {
      setUpFeatureOther(display_style, feature, feature_coords) ;

      makeSelectionString(window, display_style, text, feature_coords) ;
    }

  g_array_free(feature_coords, TRUE) ;

  if (!(text->len))
    free_text = TRUE ;
  else
    free_text = FALSE ;

  selection = g_string_free(text, free_text) ;

  return selection ;
}


/* Makes text for posting to clipboard for any selected features, returns NULL
 * if there are none.
 *
 * ZMapWindowPasteFeatureType of display_style must be ZMAPWINDOW_PASTE_TYPE_SELECTED.
 */
char *zmapWindowMakeFeatureSelectionTextFromSelection(ZMapWindow window, ZMapWindowDisplayStyle display_style_in)
{
  char *selection = NULL ;
  GList *selected ;
  GString *text ;
  gboolean free_text ;
  GArray *feature_coords ;
  gboolean revcomped ;
  ZMapWindowDisplayStyleStruct display_style = *(display_style_in) ;


  /*zMapReturnValIfFailSafe((window && display_style_in->paste_feature == ZMAPWINDOW_PASTE_TYPE_SELECTED), NULL) ;*/

  text = g_string_sized_new(512) ;
  feature_coords = g_array_new(FALSE, FALSE, sizeof(FeatureCoordStruct)) ;
  revcomped = window->flags[ZMAPFLAG_REVCOMPED_FEATURES] ;

  /* If there are any focus items then make a selection string of their coords. */
  if ((selected = zmapWindowFocusGetFocusItems(window->focus)))
    {
      /* Construction of the string is different according whether a single selection or
       * multiple selections were made. */

      if (g_list_length(selected) == 1)
        {
          ZMapWindowFocusItem focus_item ;
          FooCanvasItem *item ;
          ZMapFeature feature ;
          ZMapFeatureSubPart sub_part = NULL ;
          char *name ;

          focus_item = (ZMapWindowFocusItem)selected->data ;
          item = FOO_CANVAS_ITEM(focus_item->item) ;
          feature = focus_item->feature ;

          if (focus_item->sub_part.subpart != ZMAPFEATURE_SUBPART_INVALID)
            sub_part = &(focus_item->sub_part) ;

          name = (char *)g_quark_to_string(feature->original_id) ;

          if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
            {
              int mark_x = 0, mark_y = 0 ;
              gboolean use_mark = FALSE ;

              if (sub_part)
                display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_SUBPART ;
              else
                display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS ;

              /* If the mark is set then any exons not wholly inside the mark are not added
               * to the selection text. */
              if (zmapWindowMarkIsSet(window->mark)
                  && zmapWindowMarkGetSequenceRange(window->mark, &mark_x, &mark_y))
                {
                  if (mark_x && mark_y
                      && ((mark_x >= feature->x1 && mark_x <= feature->x2)
                          || (mark_y >= feature->x1 && mark_y <= feature->x2)))
                    use_mark = TRUE ;
                }

              setUpFeatureTranscript(revcomped, &display_style,
                                     use_mark, mark_x, mark_y,
                                     feature, sub_part, feature_coords) ;

              makeSelectionString(window, &display_style, text, feature_coords) ;
            }
          else
            {
              display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_EXTENT ;

              setUpFeatureOther(&display_style, feature, feature_coords) ;

              makeSelectionString(window, &display_style, text, feature_coords) ;
            }
        }
      else
        {
          while (selected)
            {
              ZMapWindowFocusItem focus_item ;
              FooCanvasItem *item ;
              ZMapFeature feature ;
              ZMapFeatureSubPart sub_part = NULL ;
              char *name ;

              /* Not supporting mark for multiple features at the moment.... */
              int mark_x = 0, mark_y = 0 ;
              gboolean use_mark = FALSE ;

              focus_item = (ZMapWindowFocusItem)selected->data ;

              item = FOO_CANVAS_ITEM(focus_item->item) ;

              feature = focus_item->feature ;

              if (focus_item->sub_part.subpart != ZMAPFEATURE_SUBPART_INVALID)
                sub_part = &(focus_item->sub_part) ;

              name = (char *)g_quark_to_string(feature->original_id) ;

              /* For transcript subparts put the subpart in the selection, not the whole transcript. */
              if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
                {
                  if (sub_part)
                    display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_SUBPART ;
                  else
                    display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_EXTENT ;

                  setUpFeatureTranscript(revcomped, &display_style,
                                         use_mark, mark_x, mark_y,
                                         feature, sub_part, feature_coords) ;
                }
              else
                {
                  display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_EXTENT ;

                  setUpFeatureOther(&display_style, feature, feature_coords) ;
                }

              selected = selected->next ;
            }

          makeSelectionString(window, &display_style, text, feature_coords) ;
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






/*
 *                         Internal routines
 */



/* Return an array of feature coords, name, start/end, for the given transcript feature. */
static void setUpFeatureTranscript(gboolean revcomped, ZMapWindowDisplayStyle display_style,
                                   gboolean use_mark, int mark_x, int mark_y,
                                   ZMapFeature feature,  ZMapFeatureSubPart sub_part,
                                   GArray *feature_coords)
{
  char *name ;

  name = (char *)g_quark_to_string(feature->original_id) ;

  switch (display_style->paste_feature)
    {
    case ZMAPWINDOW_PASTE_TYPE_EXTENT:
      {
        FeatureCoordStruct feature_coord ;

        feature_coord.name = name ;
        feature_coord.start = feature->x1 ;
        feature_coord.end = feature->x2 ;
        feature_coord.length = (feature_coord.end - feature_coord.start + 1) ;

        feature_coords = g_array_append_val(feature_coords, feature_coord) ;

        break ;
      }
    case ZMAPWINDOW_PASTE_TYPE_SUBPART:
      {
        FeatureCoordStruct feature_coord ;

        feature_coord.name = name ;
        feature_coord.start = sub_part->start ;
        feature_coord.end = sub_part->end ;
        feature_coord.length = (feature_coord.end - feature_coord.start + 1) ;

        feature_coords = g_array_append_val(feature_coords, feature_coord) ;

        break ;
      }
    default :
      {
        /* For a transcript feature put all the exons in the paste buffer. */
        ZMapSpan span ;
        int i ;

        for (i = 0 ; i < (int)feature->feature.transcript.exons->len ; i++)
          {
            FeatureCoordStruct feature_coord ;
            int index ;

            if (revcomped)
              index = feature->feature.transcript.exons->len - (i + 1) ;
            else
              index = i ;

            span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, index) ;

            if (!use_mark
                || (span->x1 > mark_x && span->x2 < mark_y))
              {
                feature_coord.name = name ;
                feature_coord.start = span->x1 ;
                feature_coord.end = span->x2 ;
                feature_coord.length = (span->x2 - span->x1 + 1) ;

                feature_coords = g_array_append_val(feature_coords, feature_coord) ;
              }
          }

        break ;
      }
    }

  return ;
}

/* Return an array of feature coords, name, start/end, for the given non-transcript feature. */
static void setUpFeatureOther(ZMapWindowDisplayStyle display_style, ZMapFeature feature, GArray *feature_coords)
{
  int selected_start, selected_end, selected_length ;
  ZMapFeatureSubPartType item_type_int ;
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

  for (i = 0 ; i < (int)feature_coords->len ; i++)
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
      for (i = 0 ; i < (int)feature_coords->len ; i++)
        {
          FeatureCoord feature_coord ;

          feature_coord = &g_array_index(feature_coords, FeatureCoordStruct, i) ;

          g_string_append_printf(selection_str, "\"%s\"    %d %d (%d)%s",
                                 feature_coord->name,
                                 feature_coord->start, feature_coord->end, feature_coord->length,
                                 (i < (int)feature_coords->len ? "\n" : "")) ;
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

      g_string_append_printf(selection_str, "%s%s%s%d-%d",
                             (chromosome && display_style->paste_style == ZMAPWINDOW_PASTE_FORMAT_BROWSER_CHR
                              ? "chr" : ""),
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




