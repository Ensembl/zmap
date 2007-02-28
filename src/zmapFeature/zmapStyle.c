/*  File: zmapStyle.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Contains functions for handling styles and sets of
 *              styles.
 *
 * Exported functions: See ZMap/zmapStyle.h
 * HISTORY:
 * Last edited: Feb 26 15:49 2007 (edgrif)
 * Created: Mon Feb 26 09:12:18 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.c,v 1.1 2007-02-28 18:15:51 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapUtils.h>
#include <zmapStyle_P.h>




ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->mode ;
}



/* First attempt at using the macro trick to have enums/string equivalents defined in one place
 * and then define an enum type and strings from that. */
#define ZMAP_MODE_STR(Name) #Name,

const char *zMapStyleMode2Str(ZMapStyleMode mode)
{
  static const char *names_array[] =
    {
      ZMAP_MODE_METADEFS(ZMAP_MODE_STR)
    };
  const char *mode_str = NULL ;

  zMapAssert(mode > 0 && mode < sizeof(names_array)/sizeof(names_array[0])) ;

  mode_str = names_array[mode] ;

  return mode_str ;
}



GQuark zMapStyleGetID(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->original_id ;
}

GQuark zMapStyleGetUniqueID(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->unique_id ;
}


char *zMapStyleGetDescription(ZMapFeatureTypeStyle style)
{
  char *description = NULL ;

  zMapAssert(style) ;

  if (style->fields_set.description)
    description = style->description ;

  return description ;
}


gboolean zMapStyleIsColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context)
{
  gboolean is_colour = FALSE ;

  zMapAssert(style) ;

  switch(colour_context)
    {
    case ZMAPSTYLE_DRAW_BACKGROUND:
      is_colour = style->fields_set.background_set ;
      break ;
    case ZMAPSTYLE_DRAW_FOREGROUND:
      is_colour = style->fields_set.foreground_set ;
      break ;
    case ZMAPSTYLE_DRAW_OUTLINE:
      is_colour = style->fields_set.outline_set ;
      break ;
    default:
      zMapAssertNotReached() ;
    }

  return is_colour ;
}

GdkColor *zMapStyleGetColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context)
{
  GdkColor *colour = NULL ;

  zMapAssert(style) ;

  if (zMapStyleIsColour(style, colour_context))
    {
      switch(colour_context)
	{
	case ZMAPSTYLE_DRAW_BACKGROUND:
	  colour = &(style->normal_colours.background) ;
	  break ;
	case ZMAPSTYLE_DRAW_FOREGROUND:
	  colour = &(style->normal_colours.foreground) ;
	  break ;
	case ZMAPSTYLE_DRAW_OUTLINE:
	  colour = &(style->normal_colours.outline) ;
	  break ;
	default:
	  zMapAssertNotReached() ;
	}
    }

  return colour ;
}


char *zMapStyleGetGFFSource(ZMapFeatureTypeStyle style)
{
  char *gff_source = NULL ;

  zMapAssert(style) ;

  if (style->fields_set.gff_source)
    gff_source = (char *)g_quark_to_string(style->gff_source) ;

  return gff_source ;
}

char *zMapStyleGetGFFFeature(ZMapFeatureTypeStyle style)
{
  char *gff_feature = NULL ;

  zMapAssert(style) ;

  if (style->fields_set.gff_feature)
    gff_feature = (char *)g_quark_to_string(style->gff_feature) ;

  return gff_feature ;
}


unsigned int zmapStyleGetWithinAlignError(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->within_align_error ;
}


gboolean zMapStyleIsParseGaps(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.parse_gaps ;
}

gboolean zMapStyleIsDirectionalEnd(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.directional_end ;
}


gboolean zMapStyleIsAlignGaps(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.align_gaps ;
}


gboolean zMapStyleIsFrameSpecific(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.frame_specific ;
}

gboolean zMapStyleIsHiddenAlways(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.hidden_always ;
}

gboolean zMapStyleIsHiddenNow(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.hidden_now ;
}

gboolean zMapStyleIsStrandSpecific(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.strand_specific ;
}

gboolean zMapStyleIsShowReverseStrand(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.show_rev_strand ;
}


/* this function illustrates a problem, if we don't return the type directly it makes the
 * interface awkward for the user, and really we shouldn't as we have a "set" field to test for this field. */
double zMapStyleGetWidth(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->width ;
}

double zMapStyleGetMaxScore(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->max_score ;
}

double zMapStyleGetMinScore(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->min_score ;
}


double zMapStyleGetMinMag(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->min_mag ;
}

double zMapStyleGetMaxMag(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->max_mag ;
}



double zMapStyleBaseline(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->baseline ;
}


