/*  File: zmapWindowCanvasFeature.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See zmapWindowCanvasFeatureset.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <zmapWindowCanvasFeatureset_I.h>




GString *zMapWindowCanvasFeature2Txt(ZMapWindowCanvasFeature canvas_feature)
{
  GString *canvas_feature_text = NULL ;
  char *indent = "" ;

  zMapReturnValIfFail(canvas_feature, NULL) ;

  canvas_feature_text = g_string_sized_new(2048) ;

  /* NEED ENUM TO STR FOR FEATURE TYPE.... */
  /* Title */
  g_string_append_printf(canvas_feature_text, "%sZMapWindowCanvasFeature %p, type = %s)\n",
                         indent, canvas_feature, zMapWindowCanvasFeatureType2ExactStr(canvas_feature->type)) ;

  indent = "  " ;

  g_string_append_printf(canvas_feature_text, "%sStart, End:  %g, %g\n",
                         indent, canvas_feature->y1, canvas_feature->y2) ;

  if (canvas_feature->feature)
    g_string_append_printf(canvas_feature_text, "%sContained ZMapFeature %p \"%s\" (unique id = \"%s\")\n",
                           indent,
                           canvas_feature->feature,
                           (char *)g_quark_to_string(canvas_feature->feature->original_id),
                           (char *)g_quark_to_string(canvas_feature->feature->unique_id)) ;

  g_string_append_printf(canvas_feature_text, "%sScore:  %g\n",
                         indent, canvas_feature->score) ;

  g_string_append_printf(canvas_feature_text, "%swidth:  %g\n",
                         indent, canvas_feature->width) ;

  g_string_append_printf(canvas_feature_text, "%sbump_offset, bump subcol:  %g, %d\n",
                         indent, canvas_feature->bump_offset, canvas_feature->bump_col) ;

  if (canvas_feature->splice_positions)
    g_string_append_printf(canvas_feature_text, "%sHas splice highlighting\n", indent) ;

  return canvas_feature_text ;
}

