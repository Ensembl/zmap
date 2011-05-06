/*  File: zmapWindowItemText.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May  6 13:00 2011 (edgrif)
 * Created: Mon Apr  2 09:35:42 2007 (rds)
 * CVS info:   $Id: zmapWindowItemText.c,v 1.33 2011-05-06 12:02:29 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>
#include <ZMap/zmapPeptide.h>
#include <zmapWindow_P.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowItemTextFillColumn.h>

#define SHOW_TRANSLATION_COUNTER_SIZE "5"
/* wanted to use '%5d:' but wrapping is wrong. */
#define SHOW_TRANSLATION_COUNTER_FORMAT "%-" SHOW_TRANSLATION_COUNTER_SIZE "d"
#define SHOW_TRANSLATION_COUNTER_LENGTH 5


#define BUFFER_CAN_INC(BUFFER, MAX) \
((BUFFER < MAX) ? TRUE : FALSE)

/* pretty hateful stuff....sigh...the original case buffer++ to a boolean which doesn't work on
 * 64 bit machines. */
#define BUFFER_INC(BUFFER, MAX)     \
  (BUFFER_CAN_INC(BUFFER, MAX) ? (BUFFER++, TRUE) : FALSE)



typedef struct
{
  ZMapWindow      window;
  FooCanvasItem  *feature_item;
  ZMapFeature     feature;
  FooCanvasGroup *translation_column;
  ZMapFeatureTypeStyle style;
}ShowTranslationDataStruct, *ShowTranslationData;

typedef struct
{
  ZMapFeature feature;
  FooCanvasItem *item;
  int translation_start;
  int translation_end;
  char *translation;
  int cds_coord_counter;
  int phase_length;
  GList **full_exons;
  gboolean result;
} ItemShowTranslationTextDataStruct, *ItemShowTranslationTextData;


static gint canvas_allocate_show_translation_cb(FooCanvasItem   *item,
						ZMapTextDrawData draw_data,
						gint             max_width,
						gint             max_buffer_size,
						gpointer         user_data);

static gint canvas_fetch_show_transaltion_text_cb(FooCanvasItem *text_item,
						  ZMapTextDrawData draw_data,
						  char *buffer_in_out,
						  gint  buffer_size,
						  gpointer user_data);

static void show_translation_cb(ZMapWindowContainerGroup container,
				FooCanvasPoints         *this_points,
				ZMapContainerLevelType   level,
				gpointer                 user_data);

static int get_item_canvas_start(FooCanvasItem *item);





/* This function translates the bounds of the first and last cells
 * into the bounds for a polygon to highlight both cells and all those
 * in between according to logical text flow.
 *
 * e.g. a polygon like
 * 
 *       first cell
 *            |
 *           _V_________________
 * __________|AGCGGAGTGAGCTACGT|
 * |ACTACGACGGACAGCGAGCAGCGATTT|
 * |CGTTGCATTATATCCG__ACGATCG__|
 * |__CGATCGATCGTA__|
 *                 ^
 *                 |
 *             last cell
 *
 */
void zmapWindowItemTextOverlayFromCellBounds(FooCanvasPoints *overlay_points,
					     double          *first,
					     double          *last,
					     double           minx,
					     double           maxx)
{
  zMapAssert(overlay_points->num_points >= 8);

  if(first[ITEMTEXT_CHAR_BOUND_TL_Y] > last[ITEMTEXT_CHAR_BOUND_TL_Y])
    {
      double *tmp;

      zMapLogWarning("Incorrect order of char bounds %f > %f",
                     first[ITEMTEXT_CHAR_BOUND_TL_Y],
                     last[ITEMTEXT_CHAR_BOUND_TL_Y]);
      tmp   = last;
      last  = first;
      first = tmp;
    }

  /* We use the first and last cell coords to
   * build the coords for the polygon.  We
   * also set defaults for the 4 widest x coords
   * as we can't know from the cell position how
   * wide the column is.  The exception to this
   * is dealt with below, for the case when we're
   * only overlaying across part or all of one
   * row, where we don't want to extend to the
   * edge of the column. */

  overlay_points->coords[0]    =
    overlay_points->coords[14] = first[ITEMTEXT_CHAR_BOUND_TL_X];
  overlay_points->coords[1]    =
    overlay_points->coords[3]  = first[ITEMTEXT_CHAR_BOUND_TL_Y];
  overlay_points->coords[2]    =
    overlay_points->coords[4]  = maxx;
  overlay_points->coords[5]    =
    overlay_points->coords[7]  = last[ITEMTEXT_CHAR_BOUND_TL_Y];
  overlay_points->coords[6]    =
    overlay_points->coords[8]  = last[ITEMTEXT_CHAR_BOUND_BR_X];
  overlay_points->coords[9]    =
    overlay_points->coords[11] = last[ITEMTEXT_CHAR_BOUND_BR_Y];
  overlay_points->coords[10]   =
    overlay_points->coords[12] = minx;
  overlay_points->coords[13]   =
    overlay_points->coords[15] = first[ITEMTEXT_CHAR_BOUND_BR_Y];

  /* Do some fixing of the default values if we're only on one row */
  if(first[ITEMTEXT_CHAR_BOUND_TL_Y] == last[ITEMTEXT_CHAR_BOUND_TL_Y])
    overlay_points->coords[2]   =
      overlay_points->coords[4] = last[ITEMTEXT_CHAR_BOUND_BR_X];

  if(first[ITEMTEXT_CHAR_BOUND_BR_Y] == last[ITEMTEXT_CHAR_BOUND_BR_Y])
    overlay_points->coords[10]   =
      overlay_points->coords[12] = first[ITEMTEXT_CHAR_BOUND_TL_X];

  return ;
}


/* Possibly a Container call really... */
/* function to translate coords in points from text item coordinates to
 * overlay item coordinates. (goes via world). Why? Because the text item
 * moves in y axis so it is only ever in scroll region
 * (see foozmap-canvas-floating-group.c)
 */
void zmapWindowItemTextOverlayText2Overlay(FooCanvasItem   *item,
					   FooCanvasPoints *points)
{
  ZMapWindowContainerGroup container_parent;
  ZMapWindowContainerOverlay container_overlay;
  FooCanvasItem  *overlay_item;
  int i;

  for(i = 0; i < points->num_points * 2; i++, i++)
    {
      foo_canvas_item_i2w(item, &(points->coords[i]), &(points->coords[i+1]));
    }

  container_parent  = zmapWindowContainerCanvasItemGetContainer(item);
  container_overlay = zmapWindowContainerGetOverlay(container_parent);
  overlay_item      = FOO_CANVAS_ITEM(container_overlay);

  for(i = 0; i < points->num_points * 2; i++, i++)
    {
      foo_canvas_item_w2i(overlay_item, &(points->coords[i]), &(points->coords[i+1]));
    }

  return ;
}

void debug_text(char *text, int index)
{
  char *pep = text;
  printf("Text + %d\n", index);
  pep+= index;
  for(index = 0; index < 20; index++)
    {
      printf("%c", *pep);
      pep++;
    }
  printf("\n");
  return ;
}

/* Wrapper around foo_canvas_item_text_index2item()
 * ...
 */
gboolean zmapWindowItemTextIndex2Item(FooCanvasItem *item,
				      int index,
				      double *bounds)
{
  FooCanvasGroup *item_parent;
  ZMapFeature item_feature;
  gboolean index_found = FALSE;

  if ((item_feature = zMapWindowCanvasItemGetFeature(item)))
    {
      double ix1, iy1, ix2, iy2;
      item_parent = FOO_CANVAS_GROUP(item->parent);

      /* We need to get bounds for item_parent->ypos to be correct
       * Go figure. Je ne comprend pas */
      foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);

      if (zMapFeatureSequenceIsDNA(item_feature))
	{
	  index      -= item_parent->ypos;
	  index_found = foo_canvas_item_text_index2item(item, index, bounds);
	}
      else if (zMapFeatureSequenceIsPeptide(item_feature))
	{
	  ZMapFrame frame;
	  int dindex, protein_start;
	  char *pep;

	  frame = zMapFeatureFrame(item_feature);

	  foo_canvas_item_i2w(item, &ix1, &iy1);

	  /* world to protein */
	  protein_start = (int)(index / 3);

	  pep = item_feature->description + protein_start;
#ifdef RDS_DONT_INCLUDE
	  printf("protein_start = %d\n", protein_start);
	  debug_text(item_feature->text, protein_start);
#endif

	  /* correct for where this text is drawn from. */
	  dindex         = (item_parent->ypos / 3) - 1; /* zero based */
#ifdef RDS_DONT_INCLUDE
	  debug_text(item_feature->text, dindex);
#endif
	  dindex         = (iy1 / 3) - 1; /* zero based */
#ifdef RDS_DONT_INCLUDE
	  debug_text(item_feature->text, dindex);
#endif
	  protein_start -= dindex;
#ifdef RDS_DONT_INCLUDE
	  printf("dindex = %d, protein_start = %d\n", dindex, protein_start);
#endif
	  index_found = foo_canvas_item_text_index2item(item, protein_start, bounds);
	}
    }
  return index_found;
}

void zmapWindowItemShowTranslationRemove(ZMapWindow window, FooCanvasItem *feature_item)
{
  FooCanvasItem *translation_column = NULL;
  ZMapFeature feature;

  feature = zMapWindowCanvasItemGetFeature(feature_item);

  if(ZMAPFEATURE_IS_TRANSCRIPT(feature) && ZMAPFEATURE_FORWARD(feature))
    {
      /* get the column to draw it in, this involves possibly making it, so we can't do it in the execute call */
      if((translation_column = zmapWindowItemGetShowTranslationColumn(window, feature_item)))
	{
	  zmapWindowColumnSetState(window, FOO_CANVAS_GROUP(translation_column),
				   ZMAPSTYLE_COLDISPLAY_HIDE, TRUE);
	}
    }

  return ;
}



void zmapWindowItemShowTranslation(ZMapWindow window, FooCanvasItem *feature_to_translate)
{
  FooCanvasItem *translation_column = NULL;
  ZMapFeature feature;

  feature = zMapWindowCanvasItemGetFeature(feature_to_translate);

  if(ZMAPFEATURE_IS_TRANSCRIPT(feature) && ZMAPFEATURE_FORWARD(feature))
    {
      /* get the column to draw it in, this involves possibly making it, so we can't do it in the execute call */
      if((translation_column = zmapWindowItemGetShowTranslationColumn(window, feature_to_translate)))
	{
	  ShowTranslationDataStruct show_translation = {window,
							feature_to_translate,
							feature,
							FOO_CANVAS_GROUP(translation_column)};

	  /* This calls ContainerExecuteFull() why can't we combine them ;) */
	  zmapWindowColOrderColumns(window); /* Mainly because this one stops at STRAND level */

	  show_translation.style
            = zMapWindowContainerFeatureSetGetStyle((ZMapWindowContainerFeatureSet)(translation_column));



	  /* I'm not sure which is the best way to go here.  Do a
	   * ContainerExecuteFull() with a redraw, or do the stuff then a
	   * FullReposition() */
	  zmapWindowreDrawContainerExecute(window, show_translation_cb, &show_translation);
	}
    }

  return;
}




/* 
 *                               INTERNALS
 */



static void foo_canvas_get_scroll_region_canvas(FooCanvas *canvas,
						int *cx1, int *cy1,
						int *cx2, int *cy2)
{
  double wx1, wx2, wy1, wy2;
  int *tx1 = NULL, *tx2 = NULL, *ty1 = NULL, *ty2 = NULL;

  foo_canvas_get_scroll_region(canvas, &wx1, &wy1, &wx2, &wy2);

  if(cx1){ tx1 = cx1; }
  if(cx2){ tx2 = cx2; }
  if(cy1){ ty1 = cy1; }
  if(cy2){ ty2 = cy2; }

  foo_canvas_w2c(canvas, wx1, wy1, tx1, ty1);
  foo_canvas_w2c(canvas, wx2, wy2, tx2, ty2);

  return ;
}

static int draw_data_final_alloc(FooCanvasItem   *item,
				 ZMapTextDrawData draw_data,
				 gint             max_width,
				 gint             max_buffer_size,
				 gpointer         user_data)
{
  ZMapFeature feature = (ZMapFeature)user_data;
  int buffer_size = max_buffer_size;
  int rows = 0;
  int columns = 0;
  int table, width, height;
  int canvas_start, canvas_end, canvas_range;

  /* We need to know the extents of a character */
  width  = draw_data->table.ch_width;
  height = draw_data->table.ch_height;

  if(ZMAPFEATURE_IS_TRANSCRIPT(feature))
    {
      double wx = 0.0, wy = 0.0, raw_rows;
      int cx1, cy1, cx2, cy2, tmp;
      int start2draw, end2draw, bases2draw;

      start2draw = feature->x1;
      end2draw   = feature->x2;
      if(ZMAPFEATURE_HAS_CDS(feature))
	start2draw = feature->feature.transcript.cds_start;

      foo_canvas_item_i2w(item, &wx, &wy);
      foo_canvas_w2c(FOO_CANVAS(item->canvas),
		     0.0, wy, NULL, &canvas_start);
      tmp = start2draw - wy;
      foo_canvas_w2c(FOO_CANVAS(item->canvas),
		     0.0, end2draw - tmp,
		     NULL, &canvas_end);


      foo_canvas_get_scroll_region_canvas(item->canvas, &cx1, &cy1, &cx2, &cy2);
      if(cy1 > canvas_start)
	{
	  double cs;
	  foo_canvas_c2w(item->canvas, 0.0, cy1, NULL, &cs);
	  canvas_start = cy1;
	  start2draw   = (int)cs;
	}
      if(cy2 < canvas_end)
	{
	  double ce;
	  foo_canvas_c2w(item->canvas, 0.0, cy2, NULL, &ce);
	  canvas_end = cy2;
	  end2draw   = ce;
	}

      canvas_range = canvas_end - canvas_start + 1;
      bases2draw   = end2draw   - start2draw   + 1;

      /* All we care about to start with is the number of rows @
       * max_width columns */
      raw_rows = (double)canvas_range / (double)height;
      if((double)(rows = (int)raw_rows) < raw_rows)
	rows++;

      columns = max_width;

      /* With that many rows we get table size */
      table = rows * (columns + SHOW_TRANSLATION_COUNTER_LENGTH);

      if(table > max_buffer_size)
	{
	  double raw_chars_per_line, raw_lines;
	  int real_chars_per_line, real_lines, real_lines_space;
	  /* We must reduce! */
	  bases2draw /= 3;

	  raw_chars_per_line = (double)((double)(bases2draw * height) / (double)canvas_range);

	  /* round up raw to get real. */
	  if((double)(real_chars_per_line = (int)raw_chars_per_line) < raw_chars_per_line)
	    real_chars_per_line++;

	  /* how many lines? */
	  raw_lines = bases2draw / real_chars_per_line;

	  /* round up raw to get real */
	  if((double)(real_lines = (int)raw_lines) < raw_lines)
	    real_lines++;

	  /* How much space do real_lines takes up? */
	  real_lines_space = real_lines * height;

	  if(real_lines_space > canvas_range)
	    {
	      /* Ooops... Too much space, try one less */
	      real_lines--;
	      real_lines_space = real_lines * height;
	    }

	  /* Make sure we fill the space... */
	  if(real_lines_space < canvas_range)
	    {
	      double spacing_dbl = canvas_range;
	      double delta_factor = 0.15;
	      int spacing;
	      spacing_dbl -= (real_lines * height);
	      spacing_dbl /= real_lines;

	      /* need a fudge factor here! We want to round up if we're
	       * within delta factor of next integer */

	      if(((double)(spacing = (int)spacing_dbl) < spacing_dbl) &&
		 ((spacing_dbl + delta_factor) > (double)(spacing + 1)))
		spacing_dbl = (double)(spacing + 1);

	      draw_data->table.spacing = (int)(spacing_dbl * PANGO_SCALE);
	      draw_data->table.spacing = spacing_dbl;
	      draw_data->table.spacing = 0.0;
	    }

	  /* Now we've set the number of lines we have */
	  rows    = real_lines;
	  /* Not too wide! Default! */
	  columns = max_width;
	  /* Record this so we can do index calculations later */
	  draw_data->table.untruncated_width = real_chars_per_line;

	  if(real_chars_per_line <= columns)
	    {
	      columns = real_chars_per_line;
	      draw_data->table.truncated = FALSE;
	    }
	  else
	    draw_data->table.truncated = TRUE; /* truncated to max_width. */

	}
    }

  /* Record table dimensions */
  draw_data->table.width  = columns;
  draw_data->table.height = rows;
  table = draw_data->table.width * draw_data->table.height;

  /* We absolutely must not be bigger than buffer_size. */
  buffer_size = MIN(buffer_size, table);

  return buffer_size;
}

static gint canvas_allocate_show_translation_cb(FooCanvasItem   *item,
						ZMapTextDrawData draw_data,
						gint             max_width,
						gint             max_buffer_size,
						gpointer         user_data)
{
  ZMapTextDrawDataStruct fin_table = {};
  int fin_buffer_size, inc_counter;
  int buffer_size = max_buffer_size;

  max_width -= SHOW_TRANSLATION_COUNTER_LENGTH;

  fin_table = *draw_data;
  fin_buffer_size = draw_data_final_alloc(item, &fin_table, max_width, max_buffer_size, user_data);

  *draw_data  = fin_table;

  inc_counter = (draw_data->table.width + SHOW_TRANSLATION_COUNTER_LENGTH) * draw_data->table.height;

  if(inc_counter <= max_buffer_size)
    {
      draw_data->table.width += SHOW_TRANSLATION_COUNTER_LENGTH;

      buffer_size = inc_counter;
    }
  else if(fin_buffer_size <= max_buffer_size)
    {
      draw_data->table.width = max_buffer_size / draw_data->table.height;
      buffer_size = draw_data->table.width * draw_data->table.height;
    }

  return buffer_size;
}


static int get_item_canvas_start(FooCanvasItem *item)
{
  double wx = 0.0, wy = 0.0;
  int canvas, tmp;

  foo_canvas_item_i2w(item, &wx, &wy);

  foo_canvas_w2c(FOO_CANVAS(item->canvas),
		 wx, wy, &tmp, &canvas);

  return canvas;
}

static int skip_to_exon_start(ZMapTextDrawData draw_data,
			       int min, int max,
			       char skip_char,
			       char *buffer_final,
			       char **buffer_in_out,
			       int *current_height_in_out,
			       int *itr_in_out)
{
  char *ptr;
  int i, skip = 0, width, c, height;
  int spacing, lines_skipped;
  width = draw_data->table.width - SHOW_TRANSLATION_COUNTER_LENGTH;

  spacing = PANGO_PIXELS(draw_data->table.spacing * PANGO_SCALE);

  height  = draw_data->table.ch_height + spacing;

  /* work out skip */
  if(*current_height_in_out < min)
    {
      c  = *current_height_in_out;
      /* lines first... */
      do
	{

	  c += height;
	  skip++;
	  if((min > c) && (min < (c + height)))
	    break;
	}
      while(c < min);

      /* and multiply by with to get cells */
      skip*=width;
    }

  /* get buffer */
  ptr = *buffer_in_out;
  lines_skipped = 0;

  /* and skip */
  for(i = 0; i < skip; i++)
    {
      if(i % width == 0)
	{
	  gboolean count_lines = FALSE;
	  int j;
	  (*current_height_in_out) += height;
	  lines_skipped++;
	  if(!count_lines)
	    {
	      for(j = 0; j < SHOW_TRANSLATION_COUNTER_LENGTH; j++)
		{
		  *ptr = skip_char;
		  BUFFER_INC(ptr, buffer_final);
		}
	    }
	  else			/* count lines... */
	    {
	      if (BUFFER_CAN_INC(ptr, buffer_final))
		{
		  sprintf(ptr, SHOW_TRANSLATION_COUNTER_FORMAT, lines_skipped);
		  ptr += SHOW_TRANSLATION_COUNTER_LENGTH;
		}
	    }
	}
      *ptr = skip_char;

      if (BUFFER_INC(ptr, buffer_final))
	 (*itr_in_out)++;
    }

  /* return buffer */
  *buffer_in_out = ptr;

  return lines_skipped;
}

static void skip_to_line_position(ZMapTextDrawData draw_data,
				  int skip,
				  char *max,
				  char **buffer_ptr,
				  int *itr)
{
  char *ptr;
  int i;

  /* get buffer */
  ptr = *buffer_ptr;

  /* and skip... */
  for(i = 0; i < skip; i++)
    {
      *ptr = '.';

      if (BUFFER_INC(ptr, max))
	(*itr)++;
    }

  /* return buffer */
  *buffer_ptr = ptr;

  return ;
}

static gboolean skip_to_end_of_line(ZMapTextDrawData draw_data,
				    char *max,
				    char **buffer_ptr,
				    int *itr)
{
  char *ptr;
  int width;
  gboolean within_buffer = TRUE;

  width = draw_data->table.width - SHOW_TRANSLATION_COUNTER_LENGTH;

  /* get buffer */
  ptr = *buffer_ptr;
  /* and skip... */
  while(*itr % width != 0)
    {
      *ptr = '.';

      if(BUFFER_INC(ptr, max))
	(*itr)++;
    }
  /* return buffer */
  *buffer_ptr = ptr;

  return within_buffer;
}

static gint canvas_fetch_show_transaltion_text_cb(FooCanvasItem *text_item,
						  ZMapTextDrawData draw_data,
						  char *buffer_in_out,
						  gint  buffer_size,
						  gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)user_data;
  char *buffer_ptr = buffer_in_out;
  char *buffer_max = buffer_in_out + buffer_size;
  gboolean replace_spaces = FALSE, only_dashes = FALSE;
  int current_canvas_pos = 0, line_phase = 0;
  GList *exon_list = NULL, *exon_list_member;
  int spacing, itr, table_size, width, height;
  int canvas_min, canvas_max, current_row, rows;

  table_size = draw_data->table.width * draw_data->table.height;

  zMapFeatureAnnotatedExonsCreate(feature, TRUE, &exon_list) ;

  exon_list_member = g_list_first(exon_list);

  current_canvas_pos = get_item_canvas_start(text_item);

  foo_canvas_get_scroll_region_canvas(text_item->canvas, NULL, &canvas_min, NULL, &canvas_max);

  width   = draw_data->table.width - SHOW_TRANSLATION_COUNTER_LENGTH;
  spacing = PANGO_PIXELS(draw_data->table.spacing * PANGO_SCALE);
  spacing = pango_layout_get_spacing(FOO_CANVAS_TEXT(text_item)->layout);
  spacing = PANGO_PIXELS(spacing);
  height  = draw_data->table.ch_height + spacing;
  rows    = draw_data->table.height;

  /* Step through each of the exons. */
  /* We check the current position we're drawing to, skipping to the
   * correct position for the exon if required. */
  /* The peptide is then printed, complete with Amino Acid coordinates. */
  /* We only ever draw full lines, set iterator to zero. */
  itr = 0; current_row = 0;
  do
    {
      ZMapFullExon current_exon, next_exon;
      GList *next_exon_member;
      int min_coord, max_coord;

      current_exon = (ZMapFullExon)(exon_list_member->data);
      /* Get the canvas coords for the current exon. */
      foo_canvas_w2c(text_item->canvas,
		     0.0, current_exon->sequence_span.x1,
		     NULL, &min_coord);

      foo_canvas_w2c(text_item->canvas,
		     0.0, current_exon->sequence_span.x2,
		     NULL, &max_coord);

      /* have we got space before the next exon, probably, might as well use it! */
      if(((max_coord - min_coord) < draw_data->table.ch_height) &&
	 (next_exon_member = g_list_next(exon_list_member)))
	{
	  /* There's another exon in the list, update max_coord */
	  next_exon = (ZMapFullExon)(next_exon_member->data);
	  foo_canvas_w2c(text_item->canvas,
			 0.0,  next_exon->sequence_span.x1,
			 NULL, &max_coord);
	}

      /* Have we got space before the last exon??? */
      if((current_canvas_pos < min_coord) &&
	 ((max_coord - min_coord) < draw_data->table.ch_height) &&
	 (next_exon_member = g_list_last(exon_list_member)) &&
	 (next_exon_member != exon_list_member))
	{
	  /* There's another exon in the list, update max_coord */
	  next_exon = (ZMapFullExon)(next_exon_member->data);
	  foo_canvas_w2c(text_item->canvas,
			 0.0,  next_exon->sequence_span.x1,
			 NULL, &max_coord);
	}

      /* In the case of short exons it's very possible that the coord
       * check will fail so we can't assume current_exon->peptide will
       * actually still be a CDS exon. Check added! (rds) */
      if ((!((max_coord - min_coord) < draw_data->table.ch_height)) && (current_exon->peptide))
	{
	  char *seq_ptr;
	  int pep_itr;
	  /* loop through the AA of peptide. */

	  seq_ptr = current_exon->peptide;

	  /* first time through current_canvas_pos == 0 */
	  /* subsequent times it's the bottom of the line... */
	  /* We need to check if we should not be putting text
	   * into the buffer too early or late. */
	  if(current_canvas_pos > min_coord && current_canvas_pos > max_coord)
	    {
	      /* Allowing this iteration to continue would mean putting
	       * text from this exon in the middle of an intron or
	       * the next exon. */
	      if(only_dashes)
		printf("Skipping this Exon as %d > %d && %d\n", current_canvas_pos, min_coord, max_coord);

	      if(current_row < rows)
		current_row += skip_to_exon_start(draw_data, min_coord,
						  max_coord, '-', buffer_max,
						  &buffer_ptr, &current_canvas_pos,
						  &itr);
	      continue;
	    }

	  if(current_canvas_pos > canvas_max)
	    {
	      /* We've gone past the end of the canvas. No point putting
	       * any more text into the buffer... */
	      if(only_dashes)
		printf("Skipping this Exon as %d > %d\n", current_canvas_pos, canvas_max);
	      continue;
	    }

	  if(current_canvas_pos < min_coord)
	    {
	      /* We need to skip to the next exon in this bit. */
	      if(min_coord < canvas_max)
		current_row += skip_to_exon_start(draw_data, min_coord, max_coord,
						  '-', buffer_max, &buffer_ptr,
						  &current_canvas_pos, &itr);
	      else
		{
		  /* Above everything was ok, but here we're only copying
		   * the skip character as far as the end of the canvas. */
		  current_row += skip_to_exon_start(draw_data, canvas_max,
						    max_coord, '-', buffer_max,
						    &buffer_ptr, &current_canvas_pos,
						    &itr);
		  current_canvas_pos += draw_data->table.height;
		  continue;
		}
	    }


	  /* copying the text from the exon->peptide to the text buffer */
	  /* We're also formatting so that it is impossible to just memcpy. */
	  for (pep_itr = current_exon->pep_span.x1 - 1 ; pep_itr < current_exon->pep_span.x2 ; pep_itr++)
	    {
	      int not_new_row ;

	      if (!BUFFER_CAN_INC(buffer_ptr, buffer_max))
		goto save_overflow;

	      /* Because we want to finish the current row */
	      not_new_row = ((itr % width) ? 1 : 0);

	      /* I think we need a check here so we can't go past the canvas_max */
	      if ((current_canvas_pos < canvas_max) && (current_row < (rows + not_new_row)))
		{
		  if (itr % width == 0)
		    {
		      /* New line! */
		      current_row++;
		      sprintf(buffer_ptr, SHOW_TRANSLATION_COUNTER_FORMAT, pep_itr + 1);
		      buffer_ptr += SHOW_TRANSLATION_COUNTER_LENGTH;

		      current_canvas_pos += height;
		      /* first time through current_canvas_pos == ch_height */

		      if((pep_itr == current_exon->pep_span.x1 - 1))
			skip_to_line_position(draw_data,
					      (current_exon->pep_span.x1 % width) - 1, buffer_max, &buffer_ptr, &itr);

		      line_phase = 0;
		    }

		  *buffer_ptr = *seq_ptr;

		  /* step forward */
		  if(BUFFER_INC(buffer_ptr, buffer_max))
		    itr++;
		  else
		    goto save_overflow;

		  seq_ptr++;
		  line_phase++;
		}
	    }

	  skip_to_end_of_line(draw_data, buffer_max, &buffer_ptr, &itr);
	}
      else if(current_row < rows) /* last exons, too short (canvas) to get drawn  */
	{
	  current_row += skip_to_exon_start(draw_data, min_coord,
					    max_coord, '-', buffer_max,
					    &buffer_ptr, &current_canvas_pos,
					    &itr);
	}

    } while((exon_list_member = g_list_next(exon_list_member)));


  if(replace_spaces)
    {
      char *tmp_ptr;
      tmp_ptr = buffer_in_out;
      while(tmp_ptr != buffer_ptr)
	{
	  if(*tmp_ptr == ' '){ *tmp_ptr = '.'; }
	  tmp_ptr++;
	}
    }

  if(only_dashes)
    {
      buffer_ptr = buffer_in_out;
      for(itr = 0; itr < table_size; itr++)
	{
	  *buffer_ptr = '-';
	  buffer_ptr++;
	}
    }

 save_overflow:
  if (exon_list)
    {
      zMapFeatureAnnotatedExonsDestroy(exon_list) ;
    }

  return buffer_ptr - buffer_in_out;
}

static FooCanvasItem *draw_show_translation(FooCanvasGroup *container_features,
					    ZMapFeature feature,
					    double feature_offset,
					    double x1, double y1,
					    double x2, double y2,
					    ZMapFeatureTypeStyle style)
{
  FooCanvasItem *item = NULL;
  FooCanvasItem *feature_parent;
  PangoFontDescription *font_desc;
  double fstart, fend;
  GdkColor *outline = NULL,
    *background = NULL,
    *foreground = NULL;
  gboolean status;

  status = zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
			       &background, &foreground, &outline);
  zMapAssert(status);

  fstart = feature->x1;
  fend   = feature->x2;

  if(feature->feature.transcript.flags.cds)
    {
      fstart = feature->feature.transcript.cds_start;
      fend   = feature->feature.transcript.cds_end;
    }

  zmapWindowSeq2CanOffset(&fstart, &fend, feature_offset);

  feature_parent = foo_canvas_item_new(container_features,
				       foo_canvas_float_group_get_type(),
				       "x", x1,
				       "y", fstart,
				       "min-y", fstart,
				       NULL);
#ifdef RDS_DONT_INCLUDE
  g_object_set_data(G_OBJECT(feature_parent), ITEM_FEATURE_TYPE,
		    GINT_TO_POINTER(ITEM_FEATURE_PARENT));
  g_object_set_data(G_OBJECT(feature_parent), ITEM_FEATURE_DATA,
		    feature);
#endif


  {
    double text_width, text_height;
    zMapFoocanvasGetTextDimensions(FOO_CANVAS_ITEM(feature_parent)->canvas,
				   &font_desc,
				   &text_width,
				   &text_height);
  }

#warning should use zmapwindowcanvasitem code
  item = foo_canvas_item_new(FOO_CANVAS_GROUP(feature_parent),
			     foo_canvas_zmap_text_get_type(),
			     "x",               0.0,
			     "y",               0.0,
			     "anchor",          GTK_ANCHOR_NW,
			     "font_desc",       font_desc,
			     "full-width",      30.0 + SHOW_TRANSLATION_COUNTER_LENGTH,
			     "wrap-mode",       PANGO_WRAP_CHAR,
			     "fill_color_gdk",  foreground,
			     "allocate_func",   canvas_allocate_show_translation_cb,
			     "fetch_text_func", canvas_fetch_show_transaltion_text_cb,
			     "callback_data",   feature,
			     NULL);

  return item;
}

static void show_translation_cb(ZMapWindowContainerGroup container_group,
				FooCanvasPoints         *this_points,
				ZMapContainerLevelType   level,
				gpointer                 user_data)
{
  ShowTranslationData show_data = (ShowTranslationData)user_data;
  FooCanvasGroup *container = (FooCanvasGroup *)container_group;

  if(level == ZMAPCONTAINER_LEVEL_FEATURESET && (show_data->translation_column) == container)
    {
      ZMapFeatureSet feature_set;
      ZMapFeatureBlock feature_block;
      ZMapFeature feature;
      FooCanvasGroup *container_features = NULL;
      FooCanvasItem *item;

      /* We've found the column... */
      /* Create the features */

      feature_set   = zmapWindowItemGetFeatureSet(container);
      feature_block = (ZMapFeatureBlock)(zMapFeatureGetParentGroup((ZMapFeatureAny)feature_set,
								   ZMAPFEATURE_STRUCT_BLOCK));
      feature       = show_data->feature;

      container_features = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container_group);

      zmapWindowContainerFeatureSetRemoveAllItems((ZMapWindowContainerFeatureSet)container_group);

      item = draw_show_translation(container_features, feature,
				   feature_block->block_to_sequence.block.x1,
				   0.0,  feature->x1,
				   30.0, feature->x2,
				   show_data->style);

      /* Show the column */
      zmapWindowColumnSetState(show_data->window, FOO_CANVAS_GROUP(container),
			       ZMAPSTYLE_COLDISPLAY_SHOW, TRUE);

    }

  return ;
}
