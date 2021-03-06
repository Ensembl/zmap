/*  File: zmapWindowDisplayOps.c
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
 * Description: This file is a bit of an experiment, it's supposed to
 *              contain display related functions such as "zoom to
 *              position from coords on clipboard" and other such things
 *              that don't sit happily in other files.
 *
 * Exported functions: See zmapWindow_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <zmapWindow_P.hpp>


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
char *zMapWindowGetSelectionText(ZMapWindow window, ZMapWindowDisplayStyle display_style, const gboolean expand_children)
{
  char *selection_txt = NULL ;

  /* find out if there is a feature selected or column selected and return their data. */
  if (!(selection_txt = zmapWindowMakeFeatureSelectionTextFromSelection(window, display_style, expand_children))
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
      ZMapWindowDisplayCoordinates display_coords ;

      if (display_style->coord_frame == ZMAPWINDOW_COORD_ONE_BASED)
        display_coords = ZMAP_WINDOW_DISPLAY_SLICE ;
      else
        display_coords = ZMAP_WINDOW_DISPLAY_CHROM ;


      zmapWindowMarkGetSequenceRange(window->mark, &start, &end) ;

      if (display_style->coord_frame == ZMAPWINDOW_COORD_NATURAL)
        {
          ZMapFeatureAlignment align ;

          align = window->feature_context->master_align ;
          chromosome = zMapFeatureAlignmentGetChromosome(align) ;
        }

      zmapWindowCoordPairToDisplay(window, display_coords,
                                   start, end, &start, &end) ;

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
      ZMapFeatureColumn gff = NULL ;
      GQuark column_id ;

      column_id = zmapWindowContainerFeaturesetGetColumnUniqueId(selected_column) ;

      std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = window->context_map->columns->find(column_id) ;

      if (col_iter != window->context_map->columns->end())
        gff = col_iter->second ;

      if (gff && gff->column_desc)
        selection_txt = g_strdup(gff->column_desc) ;
    }


  return selection_txt ;
}


static void doMakeFeatureSelectionTextFromFeature(ZMapWindow window,
                                                  ZMapWindowDisplayStyle display_style,
                                                  GArray *feature_coords,
                                                  ZMapFeature feature,
                                                  ZMapFeatureSubPart sub_part,
                                                  const gboolean support_mark,
                                                  const gboolean expand_children)
{
  zMapReturnIfFail(feature) ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      gboolean revcomped = zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) ;

      int mark_x = 0, mark_y = 0 ;
      gboolean use_mark = FALSE ;

      /* Not supporting mark for multiple features at the moment.... */
      if (support_mark)
        {
          /* If the mark is set then any exons not wholly inside the mark are not added
           * to the selection text. */
          if (zmapWindowMarkIsSet(window->mark)
              && zmapWindowMarkGetSequenceRange(window->mark, &mark_x, &mark_y))
            {
              if (mark_x && mark_y
                  && ((mark_x >= feature->x1 && mark_x <= feature->x2)
                      || (mark_y >= feature->x1 && mark_y <= feature->x2)))
                {
                  use_mark = TRUE ;
                }
            }
        }

      setUpFeatureTranscript(revcomped, display_style,
                             use_mark, mark_x, mark_y,
                             feature, sub_part, feature_coords) ;
    }
  else
    {
      setUpFeatureOther(display_style, feature, feature_coords) ;
    }
}


static void makeFeatureSelectionTextFromFeature(ZMapWindow window,
                                                ZMapWindowDisplayStyle display_style,
                                                GArray *feature_coords,
                                                ZMapFeature feature,
                                                ZMapFeatureSubPart sub_part,
                                                const gboolean support_mark,
                                                const gboolean expand_children)
{
  zMapReturnIfFail(feature) ;

  if (expand_children && feature->children)
    {
      for (GList *item = feature->children; item; item = item->next)
        {
          ZMapFeature child = (ZMapFeature)(item->data) ;

          doMakeFeatureSelectionTextFromFeature(window, display_style, feature_coords,
                                                child, sub_part, FALSE, FALSE) ;
        }
    }
  else
    {
      doMakeFeatureSelectionTextFromFeature(window, display_style, feature_coords,
                                            feature, sub_part, support_mark, FALSE) ;
    }
}



/* Makes text for posting to clipboard from the supplied feature.
 *
 * ZMapWindowPasteFeatureType of display_style cannot be ZMAPWINDOW_PASTE_TYPE_SELECTED.
 *
 */
char *zmapWindowMakeFeatureSelectionTextFromFeature(ZMapWindow window,
                                                    ZMapWindowDisplayStyle display_style, 
                                                    ZMapFeature feature,
                                                    const gboolean expand_children)
{
  char *selection = NULL ;
  GString *text ;
  gboolean free_text ;
  GArray *feature_coords ;

  zMapReturnValIfFailSafe((window && display_style->paste_feature != ZMAPWINDOW_PASTE_TYPE_SELECTED && feature),
                          NULL) ;

  text = g_string_sized_new(512) ;
  feature_coords = g_array_new(FALSE, FALSE, sizeof(FeatureCoordStruct)) ;

  makeFeatureSelectionTextFromFeature(window, display_style, feature_coords, feature, NULL, TRUE, expand_children) ;

  makeSelectionString(window, display_style, text, feature_coords) ;

  g_array_free(feature_coords, TRUE) ;

  if (!(text->len))
    free_text = TRUE ;
  else
    free_text = FALSE ;

  selection = g_string_free(text, free_text) ;

  return selection ;
}


/* Called by zmapWindowMakeFeatureSelectionTextFromSelection for each selected feature. 
 * single_in is true if there was only 1 item in the original selection list. Note however
 * that if this is a composite feature then it will be treated like a multiple selection. */
static void makeFeatureSelectionTextFromSelectionPart(ZMapWindow window, 
                                                      GList *selected,
                                                      const gboolean single_in,
                                                      ZMapWindowDisplayStyle display_style,
                                                      GArray *feature_coords,
                                                      const gboolean expand_children)
{
  ZMapWindowFocusItem focus_item = NULL ;
  ZMapFeature feature = NULL ;
  ZMapFeatureSubPart sub_part = NULL ;
  gboolean single = single_in ;

  focus_item = (ZMapWindowFocusItem)selected->data ;
  feature = focus_item->feature ;

  if (focus_item->sub_part.subpart != ZMAPFEATURE_SUBPART_INVALID)
    sub_part = &(focus_item->sub_part) ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      if (sub_part)
        display_style->paste_feature = ZMAPWINDOW_PASTE_TYPE_SUBPART ;
      else if (single)
        display_style->paste_feature = ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS ;
      else
        display_style->paste_feature = ZMAPWINDOW_PASTE_TYPE_EXTENT ;
    }
  else
    {
      display_style->paste_feature = ZMAPWINDOW_PASTE_TYPE_EXTENT ;
    }

  makeFeatureSelectionTextFromFeature(window, display_style, feature_coords, feature, sub_part, single, expand_children) ;
}


/* Makes text for posting to clipboard for any selected features, returns NULL
 * if there are none.
 *
 * ZMapWindowPasteFeatureType of display_style must be ZMAPWINDOW_PASTE_TYPE_SELECTED.
 */
char *zmapWindowMakeFeatureSelectionTextFromSelection(ZMapWindow window,
                                                      ZMapWindowDisplayStyle display_style,
                                                      const gboolean expand_children)
{
  char *selection = NULL ;
  GList *selected ;
  GString *text ;
  gboolean free_text ;
  GArray *feature_coords ;

  text = g_string_sized_new(512) ;


  /*zMapReturnValIfFailSafe((window && display_style_in->paste_feature == ZMAPWINDOW_PASTE_TYPE_SELECTED), NULL) ;*/
  feature_coords = g_array_new(FALSE, FALSE, sizeof(FeatureCoordStruct)) ;

  /* If there are any focus items then make a selection string of their coords. */
  if ((selected = zmapWindowFocusGetFocusItems(window->focus)))
    {
      /* Construction of the string is different according whether a single selection or
       * multiple selections were made. Note that even if we only have 1 item in the list
       * here the feature may have children and this will be treated like a multiple
       * selection in the function that does the work. */
      const gboolean single = g_list_length(selected) == 1 ;

      while (selected)
        {
          makeFeatureSelectionTextFromSelectionPart(window, selected, single, display_style, feature_coords, expand_children) ;
          selected = selected->next ;
        }

      makeSelectionString(window, display_style, text, feature_coords) ;

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
  FeatureCoordStruct feature_coord ;

  /* this is not a get sub part issue, we want the whole canvas feature which is a
   * single 'exon' in a bigger alignment */
  selected_start = feature->x1 ;
  selected_end = feature->x2 ;
  selected_length = feature->x2 - feature->x1 + 1 ;

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
  ZMapWindowDisplayCoordinates display_coords ;


  if (display_style->paste_style == ZMAPWINDOW_PASTE_FORMAT_OTTERLACE)
    {
      display_coords = ZMAP_WINDOW_DISPLAY_SLICE ;
    }
  else
    {
      if (display_style->coord_frame == ZMAPWINDOW_COORD_ONE_BASED)
        display_coords = ZMAP_WINDOW_DISPLAY_SLICE ;
      else
        display_coords = ZMAP_WINDOW_DISPLAY_CHROM ;
    }


  align = window->feature_context->master_align ;
  chromosome = zMapFeatureAlignmentGetChromosome(align) ;

  for (i = 0 ; i < (int)feature_coords->len ; i++)
    {
      FeatureCoord feature_coord ;

      feature_coord = &g_array_index(feature_coords, FeatureCoordStruct, i) ;

      zmapWindowCoordPairToDisplay(window, display_coords,
                                   feature_coord->start, feature_coord->end,
                                   &feature_coord->start, &feature_coord->end) ;

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




