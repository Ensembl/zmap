/*  File: zmapWindowItemText.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun  7 10:36 2007 (rds)
 * Created: Mon Apr  2 09:35:42 2007 (rds)
 * CVS info:   $Id: zmapWindowItemText.c,v 1.1 2007-06-07 13:19:55 rds Exp $
 *-------------------------------------------------------------------
 */


#include <math.h>
#include <zmapWindow_P.h>
#include <zmapWindowItemTextFillColumn.h>

typedef struct _ZMapWindowItemTextContextStruct
{
  PangoFontDescription *font_desc;
  int bases_per_char;
  double canvas_text_width, canvas_text_height;
  double world_text_width, world_text_height;
  double s2c_start, s2c_end, s2c_offset;
  double visible_y1, visible_y2;
  ZMapFeature feature;
  ZMapWindowItemTextIteratorStruct iterator;
  double width, height;         /* remove me later! */
  GdkColor text_colour;
}ZMapWindowItemTextContextStruct;

static gboolean text_table_get_dimensions_debug_G = TRUE;

static void text_table_get_dimensions(double region_range, double trunc_col, 
                                      double chars_per_base, int *row, int *col,
                                      double *height_inout, int *rcminusmod_out);


ZMapWindowItemTextContext zmapWindowItemTextContextCreate(GdkColor *text_colour)
{
  ZMapWindowItemTextContext context = NULL;

  if(!(context = g_new0(ZMapWindowItemTextContextStruct, 1)))
    {
      zMapAssertNotReached();
    }

  context->text_colour = *text_colour; /* struct copy */

  return context;
}

void zmapWindowItemTextContextDestroy(ZMapWindowItemTextContext context)
{
  g_free(context);
  /* what else, the font desc */
  return ;
}

void zmapWindowItemTextContextInitialise(ZMapWindowItemTextContext     context,
                                         ZMapWindowItemTextEnvironment environment)
{
  PangoFontDescription *font_desc;
  double canvas_text_width;
  double canvas_text_height;

  if(!zMapFoocanvasGetTextDimensions(environment->canvas, &font_desc, &canvas_text_width, &canvas_text_height))
    zMapLogWarning("%s", "Failed to get text dimensions");
  else
    {
      /* copy data from the environment to the context and initialise the iterator. */
      context->font_desc          = font_desc;
      context->canvas_text_width  = canvas_text_width;
      context->canvas_text_height = canvas_text_height;
      
      context->world_text_width  = canvas_text_width  / environment->canvas->pixels_per_unit_x;
      context->world_text_height = canvas_text_height / environment->canvas->pixels_per_unit_y;

      context->feature    = environment->feature;
      context->s2c_start  = context->feature->x1;
      context->s2c_end    = context->feature->x2;
      context->s2c_offset = environment->s2c_offset;

      zmapWindowSeq2CanOffset(&(context->s2c_start), 
                              &(context->s2c_end),
                              context->s2c_offset);

      context->bases_per_char = environment->bases_per_char;
      context->visible_y1 = environment->visible_y1;
      context->visible_y2 = environment->visible_y2;

      if(!zmapWindowItemTextContextGetIterator(context))
        {
          context->iterator.initialised = FALSE;
          zMapLogWarning("%s", "Failed to initialise text iterator");
        }
      else
        context->iterator.initialised = TRUE;

    }

  zMapAssert(context && context->iterator.initialised);

  return ;
}

void zmapWindowItemTextGetWidthLimitBounds(ZMapWindowItemTextContext context,
                                           double *width_min, double *width_max)
{
  ZMapWindowItemTextIterator iterator;
  double x1, x2, text_width;
  int col;

  /* Similar to the zmapWindowItemTextIndexGetBounds code, but we only
   * get the x coord of the left hand side of the first cell and the x
   * coord of the right hand side of the last cell. */

  if(context->iterator.initialised)
    {
      iterator   = &(context->iterator);
      text_width = context->world_text_width;

      if(width_min)
        {
          col = 0;                  /* the minimum col we can show */
          x1  = col * text_width;   /* obviously 0 here! */
          *width_min = x1;
        }

      if(width_max)
        {
          /* The last possible col... */
          col = (iterator->truncate_at - 1); /* 0 based indices */

          if(iterator->truncated)
            col += ELIPSIS_SIZE; /* For the elipsis */

          col++; /* because we need the right hand bounds for the cell */

          x2 = col * text_width;

          *width_max = x2;
        }
    }

  return ;
}

/* Given the context and the index of the cell you want the coords for...
 * it'll return the item_world_coords_out filled in with, the coords! */

gboolean zmapWindowItemTextIndexGetBounds(ZMapWindowItemTextContext context,
                                          int index, double *item_world_coords_out)
{
  ZMapWindowItemTextIterator iterator;
  gboolean matched = FALSE;
  double x1, x2, y1, y2;
  double text_height, text_width;
  int mod, col, row, factor_idx;

  if(context->iterator.initialised)
    {
      iterator    = &(context->iterator);
      text_height = iterator->real_char_height;
      text_width  = context->world_text_width;

      matched     = TRUE;

      index      -= (iterator->index_start + 1);
      mod         = index % iterator->cols;

      factor_idx  = index - mod;

      if(mod > iterator->truncate_at)
        {
          col     = (iterator->truncate_at - 1); /* 0 based indices */
          if(iterator->truncated)
            col  += ELIPSIS_SIZE; /* For the elipsis */
          matched = FALSE;
        }
      else
        col       = mod;

      row = factor_idx / iterator->cols;

      /* Get the top left corner of the cell */
      x1 = col * text_width;
      y1 = row * text_height;
      row++;                    /* now get the bottom */
      col++;                    /* right corner of the cell */
      x2 = col * text_width;
      y2 = row * text_height;

      if(item_world_coords_out)
        {
          (item_world_coords_out[0]) = x1;
          (item_world_coords_out[1]) = y1 + iterator->offset_start;
          (item_world_coords_out[2]) = x2;
          (item_world_coords_out[3]) = y2 + iterator->offset_start;
        }
      else
        matched = FALSE;

    }

  return matched;
}

ZMapWindowItemTextIterator zmapWindowItemTextContextGetIterator(ZMapWindowItemTextContext context)
{
  ZMapWindowItemTextIterator iterator = NULL;

  /* how to memory manage this? For now its a pointer to a struct of the context. */
  iterator = &(context->iterator);

  if(!iterator->initialised)
    {
      int tmp = 0;
      double column_width = 300.0;
      double chars_per_base, feature_first_char, feature_first_base;

      column_width  /= context->bases_per_char;
      chars_per_base = 1.0 / context->bases_per_char;

      /* read the start and end from the sequence2canvas points */
      iterator->seq_start = context->s2c_start;
      iterator->seq_end   = context->s2c_end;

      /* get the offset index from what we can see and the s2c offset */
      iterator->offset_start = floor(context->visible_y1 - context->s2c_offset);

      /* not calculate the actually indices */
      feature_first_base  = floor(context->visible_y1) - context->s2c_start;
      tmp                 = (int)feature_first_base % context->bases_per_char;
      feature_first_char  = (feature_first_base - tmp) / context->bases_per_char;

      iterator->truncate_at  = floor(column_width / context->canvas_text_width);

      iterator->region_range = (ceil(context->visible_y2) - floor(context->visible_y1) + 1.0);

      iterator->real_char_height = context->world_text_height;

      text_table_get_dimensions(iterator->region_range,
                                (double)iterator->truncate_at, 
                                chars_per_base,
                                &(iterator->rows), 
                                &(iterator->cols), 
                                &(iterator->real_char_height),
                                &(iterator->last_populated_cell));

      /* Not certain this column calculation is correct (chars_per_base bit) */
      if((iterator->truncated = (gboolean)(iterator->cols > iterator->truncate_at)))
        /* Just make the cols smaller and the number of rows bigger */
        iterator->cols *= chars_per_base;
      else
        iterator->truncate_at = iterator->cols;
        
      iterator->row_text    = g_string_sized_new(iterator->truncate_at * 2);
      iterator->index_start = (int)feature_first_char - (context->bases_per_char == 1 ? 1 : 0);
      iterator->current_idx = iterator->index_start;

      for(tmp = 0; tmp < ELIPSIS_SIZE; tmp++)
        {
          iterator->elipsis[tmp] = '.';
        }

      iterator->text_colour = &(context->text_colour);
    }

  return iterator;
}


/* INTERNAL */
static void text_table_get_dimensions(double region_range, double trunc_col, 
                                      double chars_per_base, int *row, int *col,
                                      double *height_inout, int *rcminusmod_out)
{
  double orig_height, final_height, fl_height, cl_height, exr_height;
  double temp_rows, temp_cols, text_height, tmp_mod = 0.0;
  gboolean row_precedence = FALSE;
  int tmp;

  if(text_table_get_dimensions_debug_G)
    printf("%s: Input, region_range = %f, max char on each line %f"
           "chars per base %f, text height %f\n", 
           __PRETTY_FUNCTION__,
           region_range, trunc_col, chars_per_base, *height_inout);

  region_range--;
  region_range *= chars_per_base;

  orig_height = text_height = *height_inout * chars_per_base;
  temp_rows   = region_range / orig_height     ;
  fl_height   = region_range / (((tmp = floor(temp_rows)) > 0) ? tmp : 1);
  cl_height   = region_range / ceil(temp_rows) ;
  exr_height  = region_range / ceil(temp_rows) + 1.0;

  if(((region_range / temp_rows) > trunc_col))
    row_precedence = TRUE;
  
  if(row_precedence)            /* dot dot dot */
    {
      if(text_table_get_dimensions_debug_G)
        printf(" * row_precdence, will be using ...\n"); 

      if(cl_height < fl_height && ((fl_height - cl_height) < (orig_height / 8)))
        {
#ifdef RUBBISH
          if(exr_height < fl_height && ((fl_height - exr_height) < (orig_height / 8)))
            final_height = exr_height;
          else
#endif
            final_height = cl_height;
        }
      else
        final_height = fl_height;

      temp_cols = floor(final_height);

      /* check here if the problem calc below will fail, then sort out
         the temp_rows to be floor if it does and modify final_height.
         I think it should == temp_cols ;(! */
      temp_rows = ceil(region_range / temp_cols);

      if(((temp_rows * temp_cols) - region_range) > (temp_cols - trunc_col))
        temp_rows  = floor(region_range /temp_cols);

      final_height = temp_cols;
    }
  else                          /* column_precedence */
    {
      double tmp_rws, tmp_len, sos;
      if(text_table_get_dimensions_debug_G)
        printf(" * col_precdence, exact positioning of text is more important\n");

      if((sos = orig_height - 0.5) <= floor(orig_height) && sos > 0.0)
        temp_cols = final_height = floor(orig_height);
      else
        temp_cols = final_height = ceil(orig_height);

      if(text_table_get_dimensions_debug_G)
        printf(" * we're going to miss %f bases at the end if using %f cols\n", 
               (tmp_mod = fmod(region_range, final_height)), final_height);

      if(final_height != 0.0 && tmp_mod != 0.0)
        {
          tmp_len = region_range + (final_height - fmod(region_range, final_height));
          tmp_rws = tmp_len / final_height;
          
          final_height = (region_range + 1) / tmp_rws;
          temp_rows    = tmp_rws;

          if(text_table_get_dimensions_debug_G)
            printf(" * so modifying... tmp_len=%f, tmp_rws=%f, final_height=%f\n", 
                   tmp_len, tmp_rws, final_height);
        }
      else if(final_height != 0.0)
        temp_rows = floor(region_range / final_height);

    }

  /* sanity */
  if(temp_rows > region_range)
    temp_rows = region_range;

  if(text_table_get_dimensions_debug_G)
    printf("%s: Output, rows=%f, height=%f, range=%f,"
           " would have been min rows=%f & height=%f,"
           " or orignal rows=%f & height=%f\n", 
           __PRETTY_FUNCTION__,
           temp_rows, final_height, region_range,
           region_range / fl_height, fl_height,
           region_range / orig_height, orig_height);

  if(height_inout)
    *height_inout = text_height = final_height / chars_per_base;
  if(row)
    *row = (int)temp_rows;
  if(col)
    *col = (int)temp_cols;
  if(rcminusmod_out)
    *rcminusmod_out = (int)(temp_rows * temp_cols - tmp_mod);

  return ;
}

