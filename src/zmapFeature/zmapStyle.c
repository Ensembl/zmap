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
 * Last edited: Jun 12 21:48 2008 (rds)
 * Created: Mon Feb 26 09:12:18 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.c,v 1.14 2008-06-12 21:01:38 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapStyle_P.h>



static gboolean setColours(ZMapStyleColour colour, char *border, char *draw, char *fill) ;



/*!
 * Does a case <i>insensitive</i> comparison of the style name and
 * the supplied name, return TRUE if they are the same.
 * 
 * @param   style          The style.
 * @param   name           The name to be compared..
 * @return  gboolean       TRUE if the names are the same.
 *  */
gboolean zMapStyleNameCompare(ZMapFeatureTypeStyle style, char *name)
{
  gboolean result = FALSE ;

  zMapAssert(style && name && *name) ;

  if (g_ascii_strcasecmp(g_quark_to_string(style->original_id), name) == 0)
    result = TRUE ;

  return result ;
}





/* Checks a style to see if it relates to a "real" feature, i.e. not a Meta or
 * or glyph like feature.
 *
 * Function returns FALSE if feature is not a real feature, TRUE otherwise.
 *  */
gboolean zMapStyleIsTrueFeature(ZMapFeatureTypeStyle style)
{
  gboolean valid = FALSE ;

  zMapAssert(style) ;

  switch (style->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_TRANSCRIPT:
    case ZMAPSTYLE_MODE_ALIGNMENT:
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
      {
	valid = TRUE ;
	break ;
      }
    default:
      {
	break ;
      }
    }

  return valid ;
}




/* Checks a style to see if it contains the minimum necessary for display,
 * we need this function because we no longer allow defaults.
 * 
 * Function returns FALSE if there style is not valid and the GError says
 * what the problem was.
 *  */
gboolean zMapStyleDisplayValid(ZMapFeatureTypeStyle style, GError **error)
{
  gboolean valid = TRUE ;
  GQuark domain ;
  gint code = 0 ;
  char *message ;

  zMapAssert(style && error && !(*error)) ;

  domain = g_quark_from_string("ZmapStyle") ;

  /* These are the absolute basic requirements without which we can't display something.... */
  if (valid && (style->original_id == ZMAPFEATURE_NULLQUARK || style->unique_id == ZMAPFEATURE_NULLQUARK))
    {
      valid = FALSE ;
      code = 1 ;
      message = g_strdup_printf("Bad style ids, original: %d, unique: %d.", 
				style->original_id, style->unique_id) ;
    }

  if (valid && !(style->fields_set.mode))
    {
      valid = FALSE ;
      code = 2 ;
      message = g_strdup("Style mode not set.") ;
    }

  /* Check modes/sub modes. */
  if (valid)
    {
      switch (style->mode)
	{
	case ZMAPSTYLE_MODE_BASIC:
	case ZMAPSTYLE_MODE_TRANSCRIPT:
	case ZMAPSTYLE_MODE_ALIGNMENT:
	case ZMAPSTYLE_MODE_RAW_SEQUENCE:
	case ZMAPSTYLE_MODE_PEP_SEQUENCE:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_META:
	  {
	    /* We shouldn't be displaying meta styles.... */
	    valid = FALSE ;
	    code = 5 ;
	    message = g_strdup("Meta Styles cannot be displayed.") ;
	    break ;
	  }
	case ZMAPSTYLE_MODE_TEXT:
	case ZMAPSTYLE_MODE_GRAPH:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_GLYPH:
	  {
	    if (style->mode_data.glyph.mode != ZMAPSTYLE_GLYPH_SPLICE)
	      {
		valid = FALSE ;
		code = 10 ;
		message = g_strdup("Bad glyph mode.") ;
	      }
	    break ;
	  }
	default:
	  {
	    zMapAssertNotReached() ;
	  }
	}
    }

  if (valid && !(style->fields_set.overlap_mode))
    {
      valid = FALSE ;
      code = 3 ;
      message = g_strdup("Style overlap mode not set.") ;
    }

  if (valid && !(style->fields_set.width))
     {
      valid = FALSE ;
      code = 4 ;
      message = g_strdup("Style width not set.") ;
    }

  /* What colours we need depends on the mode.at least a fill colour or a border colour (e.g. for transparent objects). */
  if (valid)
    {
      switch (style->mode)
	{
	case ZMAPSTYLE_MODE_BASIC:
	case ZMAPSTYLE_MODE_TRANSCRIPT:
	case ZMAPSTYLE_MODE_ALIGNMENT:
	  {
	    if (!(style->colours.normal.fields_set.fill) && !(style->colours.normal.fields_set.border))
	      {
		valid = FALSE ;
		code = 5 ;
		message = g_strdup("Style does not have a fill or border colour.") ;
	      }
	    break ;
	  }

	case ZMAPSTYLE_MODE_RAW_SEQUENCE:
	case ZMAPSTYLE_MODE_PEP_SEQUENCE:
	case ZMAPSTYLE_MODE_TEXT:
	  {
	    if (!(style->colours.normal.fields_set.fill) || !(style->colours.normal.fields_set.draw))
	      {
		valid = FALSE ;
		code = 5 ;
		message = g_strdup("Text style requires fill and draw colours.") ;
	      }
	    break ;
	  }
	case ZMAPSTYLE_MODE_GRAPH:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_GLYPH:
	  {
	    if (style->mode_data.glyph.mode == ZMAPSTYLE_GLYPH_SPLICE
		&& (!(style->frame0_colours.normal.fields_set.fill)
		    || !(style->frame1_colours.normal.fields_set.fill)
		    || !(style->frame2_colours.normal.fields_set.fill)))
	      {
		valid = FALSE ;
		code = 11 ;
		message = g_strdup_printf("Splice mode requires all frame colours to be set unset frames are:%s%s%s",
					  (style->frame0_colours.normal.fields_set.fill ? "" : " frame0"),
					  (style->frame1_colours.normal.fields_set.fill ? "" : " frame1"),
					  (style->frame2_colours.normal.fields_set.fill ? "" : " frame2")) ;
	      }
	    break ;
	  }
	default:
	  {
	    zMapAssertNotReached() ;
	  }
	}
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Now do some mode specific stuff.... */
  if (valid)
    {
      switch (style->mode)
	{
	case ZMAPSTYLE_MODE_META:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_BASIC:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_TRANSCRIPT:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_ALIGNMENT:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_TEXT:
	  {
	    break ;
	  }
	case ZMAPSTYLE_MODE_GRAPH:
	  {
	    break ;
	  }
	default:
	  {
	    zMapAssertNotReached() ;
	  }
	}
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Construct the error if there was one. */
  if (!valid)
    *error = g_error_new_literal(domain, code, message) ;


  return valid ;
}



void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode)
{
  zMapAssert(style
	     && (mode >= ZMAPSTYLE_MODE_INVALID || mode <= ZMAPSTYLE_MODE_TEXT)) ;

#ifdef STYLES_ARE_G_OBJECTS
  g_object_set(G_OBJECT(style),
	       "mode", mode,
	       NULL);
#endif /* STYLES_ARE_G_OBJECTS */

  style->mode = mode ;
  style->fields_set.mode = TRUE ;

  return ;
}


gboolean zMapStyleHasMode(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->fields_set.mode ;
}


ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->mode ;
}

gboolean zMapStyleFormatMode(char *mode_str, ZMapStyleMode *mode_out)
{
  gboolean result = FALSE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapFeatureStr2EnumStruct mode_table [] = {{"INVALID", ZMAPSTYLE_MODE_INVALID},
					     {"BASIC", ZMAPSTYLE_MODE_BASIC},
					     {"TRANSCRIPT", ZMAPSTYLE_MODE_TRANSCRIPT},
					     {"ALIGNMENT", ZMAPSTYLE_MODE_ALIGNMENT},
					     {"TEXT", ZMAPSTYLE_MODE_TEXT},
					     {NULL, 0}} ;

  zMapAssert(mode_str && *mode_str && mode_out) ;

  result = zmapStr2Enum(&(mode_table[0]), mode_str, (int *)mode_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  

  return result ;
}



void zMapStyleSetGlyphMode(ZMapFeatureTypeStyle style, ZMapStyleGlyphMode glyph_mode)
{
  zMapAssert(style) ;

  switch (glyph_mode)
    {
    case ZMAPSTYLE_GLYPH_SPLICE:
      style->mode_data.glyph.mode = glyph_mode ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  return ;
}

ZMapStyleGlyphMode zMapStyleGetGlyphMode(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->mode_data.glyph.mode ;
}


ZMAP_ENUM_AS_STRING_FUNC(zMapStyleMode2Str, ZMapStyleMode, ZMAP_STYLE_MODE_LIST);

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




void zMapStyleSetPfetch(ZMapFeatureTypeStyle style, gboolean pfetchable)
{
  zMapAssert(style) ;

  style->mode_data.alignment.state.pfetchable = pfetchable ;

  return ;
}


gboolean zMapStyleGetPfetch(ZMapFeatureTypeStyle style) 
{
  zMapAssert(style) ;

  return (style->mode_data.alignment.state.pfetchable) ;
}





/* I'm going to try these more generalised functions.... */

gboolean zMapStyleSetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     char *fill, char *draw, char *border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  zMapAssert(style) ;

  switch (target)
    {
    case ZMAPSTYLE_COLOURTARGET_NORMAL:
      full_colour = &(style->colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME0:
      full_colour = &(style->frame0_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME1:
      full_colour = &(style->frame1_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME2:
      full_colour = &(style->frame2_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_CDS:
      full_colour = &(style->mode_data.transcript.CDS_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_STRAND:
      full_colour = &(style->strand_rev_colours);
      break;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  switch (type)
    {
    case ZMAPSTYLE_COLOURTYPE_NORMAL:
      colour = &(full_colour->normal) ;
      break ;
    case ZMAPSTYLE_COLOURTYPE_SELECTED:
      colour = &(full_colour->selected) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  if (!(result = setColours(colour, border, draw, fill)))
    {
      zMapLogCritical("Style \"%s\", bad colours specified:%s\"%s\"%s\"%s\"%s\"%s\"",
		      zMapStyleGetName(style),
		      border ? "  border " : "",
		      border ? border : "",
		      draw ? "  draw " : "",
		      draw ? draw : "",
		      fill ? "  fill " : "",
		      fill ? fill : "") ;
    }

  return result ;
}

gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     GdkColor **fill, GdkColor **draw, GdkColor **border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  zMapAssert(style && (fill || draw || border)) ;

  switch (target)
    {
    case ZMAPSTYLE_COLOURTARGET_NORMAL:
      full_colour = &(style->colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME0:
      full_colour = &(style->frame0_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME1:
      full_colour = &(style->frame1_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME2:
      full_colour = &(style->frame2_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_CDS:
      full_colour = &(style->mode_data.transcript.CDS_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_STRAND:
      full_colour = &(style->strand_rev_colours) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  switch (type)
    {
    case ZMAPSTYLE_COLOURTYPE_NORMAL:
      colour = &(full_colour->normal) ;
      break ;
    case ZMAPSTYLE_COLOURTYPE_SELECTED:
      colour = &(full_colour->selected) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  if (fill)
    {
      if(colour->fields_set.fill)
	{
	  *fill = &(colour->fill) ;
	  result = TRUE ;
	}
    }

  if (draw)
    {
      if(colour->fields_set.draw)
	{
	  *draw = &(colour->draw) ;
	  result = TRUE ;
	}
    }

  if (border)
    {
      if (colour->fields_set.border)
	{
	  *border = &(colour->border) ;
	  result = TRUE ;
	}
    }

  return result ;
}


gboolean zMapStyleColourByStrand(ZMapFeatureTypeStyle style)
{
  gboolean colour_by_strand = FALSE;

  if(style->strand_rev_colours.normal.fields_set.fill ||
     style->strand_rev_colours.normal.fields_set.draw ||
     style->strand_rev_colours.normal.fields_set.border)
    colour_by_strand = TRUE;

  return colour_by_strand;
}




gboolean zMapStyleIsColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context)
{
  gboolean is_colour = FALSE ;

  zMapAssert(style) ;

  switch(colour_context)
    {
    case ZMAPSTYLE_DRAW_FILL:
      is_colour = style->colours.normal.fields_set.fill ;
      break ;
    case ZMAPSTYLE_DRAW_DRAW:
      is_colour = style->colours.normal.fields_set.draw ;
      break ;
    case ZMAPSTYLE_DRAW_BORDER:
      is_colour = style->colours.normal.fields_set.border ;
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
	case ZMAPSTYLE_DRAW_FILL:
	  colour = &(style->colours.normal.fill) ;
	  break ;
	case ZMAPSTYLE_DRAW_DRAW:
	  colour = &(style->colours.normal.draw) ;
	  break ;
	case ZMAPSTYLE_DRAW_BORDER:
	  colour = &(style->colours.normal.fill) ;
	  break ;
	default:
	  zMapAssertNotReached() ;
	}
    }

  return colour ;
}




/* I NOW THINK THIS FUNCTION IS REDUNDANT.... */
/* As for zMapStyleGetColours() but defaults colours that are not set in the style according
 * to the style mode e.g. rules may be different for Transcript as opposed to Basic mode.
 * 
 *  */
gboolean zMapStyleGetColoursDefault(ZMapFeatureTypeStyle style, 
				    GdkColor **background, GdkColor **foreground, GdkColor **outline)
{
  gboolean result = FALSE ;

  zMapAssert(style) ;

  switch (style->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_TRANSCRIPT:
    case ZMAPSTYLE_MODE_ALIGNMENT:
    case ZMAPSTYLE_MODE_TEXT:
    case ZMAPSTYLE_MODE_GRAPH:
      {
	/* Our rule is that missing colours will default to the fill colour so if the fill colour
	 * is missing there is nothing we can do. */
	if (style->colours.normal.fields_set.fill)
	  {
	    result = TRUE ;				    /* We know we can default to fill
							       colour. */

	    if (background)
	      {
		*background = &(style->colours.normal.fill) ;
	      }

	    if (foreground)
	      {
		if (style->colours.normal.fields_set.draw)
		  *foreground = &(style->colours.normal.draw) ;
		else
		  *foreground = &(style->colours.normal.fill) ;
	      }

	    if (outline)
	      {
		if (style->colours.normal.fields_set.border)
		  *outline = &(style->colours.normal.border) ;
		else
		  *outline = &(style->colours.normal.fill) ;
	      }
	  }
      }
    default:
      {
	break ;
      }
    }

  return result ;
}




/* This function looks for cds colours but defaults to the styles normal colours if there are none. */
gboolean zMapStyleGetColoursCDSDefault(ZMapFeatureTypeStyle style, 
				       GdkColor **background, GdkColor **foreground, GdkColor **outline)
{
  gboolean result = FALSE ;
  ZMapStyleColour cds_colours ;

  zMapAssert(style && style->mode == ZMAPSTYLE_MODE_TRANSCRIPT) ;

  cds_colours = &(style->mode_data.transcript.CDS_colours.normal) ;

  /* Our rule is that missing CDS colours will default to the CDS fill colour, if those
   * are missing they default to the standard colours. */
  if (cds_colours->fields_set.fill || cds_colours->fields_set.border)
    {
      result = TRUE ;				    /* We know we can default to fill colour. */

      if (background)
	{
	  if (cds_colours->fields_set.fill)
	    *background = &(cds_colours->fill) ;
	}

      if (foreground)
	{
	  if (cds_colours->fields_set.draw)
	    *foreground = &(cds_colours->draw) ;
	}

      if (outline)
	{
	  if (cds_colours->fields_set.border)
	    *outline = &(cds_colours->border) ;
	}
    }
  else
    {
      GdkColor *fill, *draw, *border ;

      if (zMapStyleGetColoursDefault(style, &fill, &draw, &border))
	{
	  result = TRUE ;

	  if (background)
	    *background = fill ;

	  if (foreground)
	    *foreground = draw ;

	  if (outline)
	    *outline = border ;
	}
    }

  return result ;
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

  return style->mode_data.alignment.within_align_error ;
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

void zMapStyleSetAlignGaps(ZMapFeatureTypeStyle style, gboolean show_gaps)
{
  zMapAssert(style) ;

  style->opts.align_gaps = show_gaps ;

  return ;
}




void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean parse_gaps,
			      unsigned int within_align_error)
{
  zMapAssert(style);

  style->opts.parse_gaps = parse_gaps ;
  style->mode_data.alignment.within_align_error = within_align_error ;
  style->mode_data.alignment.fields_set.within_align_error = TRUE ;

  return ;
}


gboolean zMapStyleGetGappedAligns(ZMapFeatureTypeStyle style, unsigned int *within_align_error)
{
  gboolean result = FALSE ;

  zMapAssert(style && within_align_error) ;

  if (style->mode_data.alignment.fields_set.within_align_error && within_align_error)
    {
      *within_align_error = style->mode_data.alignment.within_align_error ;
      result = TRUE ;
    }

  return result ;
}





void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, unsigned int between_align_error)
{
  zMapAssert(style);

  style->mode_data.alignment.between_align_error = between_align_error ;
  style->mode_data.alignment.fields_set.between_align_error = TRUE ;

  return ;
}


/* Returns TRUE and returns the between_align_error if join_aligns is TRUE for the style,
 * otherwise returns FALSE. */
gboolean zMapStyleGetJoinAligns(ZMapFeatureTypeStyle style, unsigned int *between_align_error)
{
  gboolean result = FALSE ;

  zMapAssert(style);

  if (style->mode_data.alignment.fields_set.between_align_error)
    {
      *between_align_error = style->mode_data.alignment.between_align_error ;
      result = TRUE ;
    }

  return result ;
}




gboolean zMapStyleIsFrameSpecific(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.frame_specific ;
}



void zMapStyleSetDisplayable(ZMapFeatureTypeStyle style, gboolean displayable)
{
  zMapAssert(style) ;

  style->opts.displayable = displayable ;

  return ;
}

gboolean zMapStyleIsDisplayable(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.displayable ;
}


void zMapStyleSetDeferred(ZMapFeatureTypeStyle style, gboolean deferred)
{
  zMapAssert(style) ;

  style->opts.deferred = deferred ;

  return ;
}

gboolean zMapStyleIsDeferred(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.deferred ;
}

void zMapStyleSetLoaded(ZMapFeatureTypeStyle style, gboolean deferred)
{
  zMapAssert(style) ;

  style->opts.loaded = deferred ;

  return ;
}

gboolean zMapStyleIsLoaded(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.loaded ;
}


/* Controls whether the feature set is displayed. */
void zMapStyleSetDisplay(ZMapFeatureTypeStyle style, ZMapStyleColumnDisplayState col_show)
{
  zMapAssert(style
	     && col_show > ZMAPSTYLE_COLDISPLAY_INVALID && col_show <= ZMAPSTYLE_COLDISPLAY_SHOW) ;

  style->col_display_state = col_show ;

  return ;
}

ZMapStyleColumnDisplayState zMapStyleGetDisplay(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->col_display_state ;
}

gboolean zMapStyleIsHidden(ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE ;

  zMapAssert(style) ;

  if (style->col_display_state == ZMAPSTYLE_COLDISPLAY_HIDE)
    result = TRUE ;

  return result ;
}







/* Controls whether the feature set is displayed initially. */
void zMapStyleSetShowWhenEmpty(ZMapFeatureTypeStyle style, gboolean show_when_empty)
{
  zMapAssert(style) ;

  style->opts.show_when_empty = show_when_empty ;

  return ;
}

gboolean zMapStyleGetShowWhenEmpty(ZMapFeatureTypeStyle style, gboolean show_when_empty)
{
  zMapAssert(style) ;

  return style->opts.show_when_empty;
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

  return style->mode_data.graph.baseline ;
}



static gboolean setColours(ZMapStyleColour colour, char *border, char *draw, char *fill)
{
  gboolean status = TRUE ;
  ZMapStyleColourStruct tmp_colour = {{0}} ;


  zMapAssert(colour) ;

  if (status && border && *border)
    {
      if ((status = gdk_color_parse(border, &(tmp_colour.border))))
	tmp_colour.fields_set.border = TRUE ;
    }
  if (status && draw && *draw)
    {
      if ((status = gdk_color_parse(draw, &(tmp_colour.draw))))
	tmp_colour.fields_set.draw = TRUE ;
    }
  if (status && fill && *fill)
    {
      if ((status = gdk_color_parse(fill, &(tmp_colour.fill))))
	tmp_colour.fields_set.fill = TRUE ;
    }

  if (status)
    {
      *colour = tmp_colour ;
    }
  
  return status ;
}


#ifdef STYLES_ARE_G_OBJECTS

#include <zmapStyle_I.h>

enum
  {
    STYLE_PROP_NONE,		/* zero is invalid */

    STYLE_PROP_MODE,

    /* State information from the style->opts struct */
    STYLE_PROP_DISPLAYABLE,
    STYLE_PROP_SHOW_WHEN_EMPTY,
    STYLE_PROP_SHOW_TEXT,
    STYLE_PROP_PARSE_GAPS,
    STYLE_PROP_ALIGN_GAPS,
    STYLE_PROP_STRAND_SPECIFIC,
    STYLE_PROP_SHOW_REV_STRAND,
    STYLE_PROP_FRAME_SPECIFIC,
    STYLE_PROP_SHOW_ONLY_AS_3_FRAME,
    STYLE_PROP_SHOW_ONLY_IN_SEPARATOR,
    STYLE_PROP_DIRECTIONAL_ENDS,
    STYLE_PROP_DEFERRED,
    STYLE_PROP_LOADED,
  };

static void zmap_feature_type_style_class_init(ZMapFeatureTypeStyleClass style_class);
static void zmap_feature_type_style_init      (ZMapFeatureTypeStyle      style);
static void zmap_feature_type_style_set_property(GObject *gobject, 
						 guint param_id, 
						 const GValue *value, 
						 GParamSpec *pspec);
static void zmap_feature_type_style_get_property(GObject *gobject, 
						 guint param_id, 
						 GValue *value, 
						 GParamSpec *pspec);
static void zmap_feature_type_style_dispose (GObject *object);
static void zmap_feature_type_style_finalize(GObject *object);


GType zMapFeatureTypeStyleGetType(void)
{
  static GType type = 0;
  
  if (type == 0) 
    {
      static const GTypeInfo info = 
	{
	  sizeof (zmapFeatureTypeStyleClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_feature_type_style_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL /* class_data */,
	  sizeof (zmapFeatureTypeStyle),
	  0 /* n_preallocs */,
	  (GInstanceInitFunc) zmap_feature_type_style_init,
	  NULL
	};
      
      type = g_type_register_static (ZMAP_TYPE_BASE, 
				     "ZMapFeatureTypeStyle", 
				     &info, (GTypeFlags)0);
    }
  
  return type;

}

ZMapFeatureTypeStyle zMapFeatureTypeStyleCreate(void)
{
  ZMapFeatureTypeStyle style = NULL;

  style = g_object_new(ZMAP_TYPE_FEATURE_STYLE, NULL);

  return style;
}

ZMapFeatureTypeStyle zMapFeatureTypeStyleCopy(ZMapFeatureTypeStyle src)
{
  ZMapFeatureTypeStyle dest = NULL;

  return dest;
}

static void zmap_feature_type_style_class_init(ZMapFeatureTypeStyleClass style_class)
{
  GObjectClass *gobject_class;
  ZMapBaseClass zmap_class;
  
  gobject_class = (GObjectClass *)style_class;
  zmap_class    = ZMAP_BASE_CLASS(style_class);

  gobject_class->set_property = zmap_feature_type_style_set_property;
  gobject_class->get_property = zmap_feature_type_style_get_property;

  zmap_class->copy_set_property = zmap_feature_type_style_set_property;
  
  /* Mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MODE,
				  g_param_spec_uint("mode", "mode",
						    "mode", 0, 12, 0, ZMAP_PARAM_STATIC_RW));

  /* style->opts properties */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DISPLAYABLE,
				  g_param_spec_boolean("displayable", "displayable",
						       "Is the style Displayable",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_WHEN_EMPTY,
				  g_param_spec_boolean("show-when-empty", "show when empty",
						       "Does the Style get shown when empty",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_TEXT,
				  g_param_spec_boolean("show-text", "show-text",
						       "Show as Text",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_PARSE_GAPS,
				  g_param_spec_boolean("parse-gaps", "parse gaps",
						       "Parse Gaps?",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGN_GAPS,
				  g_param_spec_boolean("align-gaps", "align gaps",
						       "Align Gaps?",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_STRAND_SPECIFIC,
				  g_param_spec_boolean("strand-specific", "strand specific",
						       "Strand Specific?",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_REV_STRAND,
				  g_param_spec_boolean("show-rev-strand", "show-rev-strand",
						       "Show Rev Strand",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME_SPECIFIC,
				  g_param_spec_boolean("frame-specific", "frame specific",
						       "Frame Specific?",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_ONLY_IN_SEPARATOR,
				  g_param_spec_boolean("show-only-in-separator", "only in separator",
						       "Show Only in Separator",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DIRECTIONAL_ENDS,
				  g_param_spec_boolean("directional-ends", "directional-ends",
						       "Display pointy \"short sides\"",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DEFERRED,
				  g_param_spec_boolean("deferred", "deferred",
						       "Load only when specifically asked",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_LOADED,
				  g_param_spec_boolean("loaded", "loaded",
						       "Style Loaded from server",
						       TRUE, ZMAP_PARAM_STATIC_RW));

  style_class->boxed_type = g_boxed_type_register_static(G_OBJECT_CLASS_NAME(style_class),
							 zmap_feature_type_copy_constructor,
							 zmap_feature_type_free);


  gobject_class->dispose  = zmap_feature_type_style_dispose;
  gobject_class->finalize = zmap_feature_type_style_finalize;


  return ;
}

static void zmap_feature_type_style_init      (ZMapFeatureTypeStyle      style)
{
  return ;
}

static void zmap_feature_type_style_set_property(GObject *gobject, 
						 guint param_id, 
						 const GValue *value, 
						 GParamSpec *pspec)
{
  ZMapFeatureTypeStyle style;

  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(gobject));

  style = ZMAP_FEATURE_STYLE(gobject);

  switch(param_id)
    {
      /* style->opts stuff */
    case STYLE_PROP_DISPLAYABLE:
      style->opts.displayable = g_value_get_boolean(value);
      break;
    case STYLE_PROP_SHOW_WHEN_EMPTY:
      style->opts.show_when_empty = g_value_get_boolean(value);
      break;
    case STYLE_PROP_SHOW_TEXT:
      style->opts.showText = g_value_get_boolean(value);
      break;
    case STYLE_PROP_PARSE_GAPS:
      style->opts.parse_gaps = g_value_get_boolean(value);
      break;
    case STYLE_PROP_ALIGN_GAPS:
      style->opts.align_gaps = g_value_get_boolean(value);
      break;
    case STYLE_PROP_STRAND_SPECIFIC:
      style->opts.strand_specific = g_value_get_boolean(value);
      break;
    case STYLE_PROP_SHOW_REV_STRAND:
      style->opts.show_rev_strand = g_value_get_boolean(value);
      break;
    case STYLE_PROP_FRAME_SPECIFIC:
      style->opts.frame_specific = g_value_get_boolean(value);
      break;
    case STYLE_PROP_SHOW_ONLY_AS_3_FRAME:
      style->opts.show_only_as_3_frame = g_value_get_boolean(value);
      break;
    case STYLE_PROP_SHOW_ONLY_IN_SEPARATOR:
      style->opts.show_only_in_separator = g_value_get_boolean(value);
      break;
    case STYLE_PROP_DIRECTIONAL_ENDS:
      style->opts.directional_end = g_value_get_boolean(value);
      break;
    case STYLE_PROP_DEFERRED:
      style->opts.deferred = g_value_get_boolean(value);
      break;
    case STYLE_PROP_LOADED:
      style->opts.loaded = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }
  
  return ;
}

static void zmap_feature_type_style_get_property(GObject *gobject, 
						 guint param_id, 
						 GValue *value, 
						 GParamSpec *pspec)
{
  ZMapFeatureTypeStyle style;

  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(gobject));

  style = ZMAP_FEATURE_STYLE(gobject);

  switch(param_id)
    {
      /* style->opts stuff */
    case STYLE_PROP_DISPLAYABLE:
      g_value_set_boolean(value, style->opts.displayable);
      break;
    case STYLE_PROP_SHOW_WHEN_EMPTY:
      g_value_set_boolean(value, style->opts.show_when_empty);
      break;
    case STYLE_PROP_SHOW_TEXT:
      g_value_set_boolean(value, style->opts.showText);
      break;
    case STYLE_PROP_PARSE_GAPS:
      g_value_set_boolean(value, style->opts.parse_gaps);
      break;
    case STYLE_PROP_ALIGN_GAPS:
      g_value_set_boolean(value, style->opts.align_gaps);
      break;
    case STYLE_PROP_STRAND_SPECIFIC:
      g_value_set_boolean(value, style->opts.strand_specific);
      break;
    case STYLE_PROP_SHOW_REV_STRAND:
      g_value_set_boolean(value, style->opts.show_rev_strand);
      break;
    case STYLE_PROP_FRAME_SPECIFIC:
      g_value_set_boolean(value, style->opts.frame_specific);
      break;
    case STYLE_PROP_SHOW_ONLY_AS_3_FRAME:
      g_value_set_boolean(value, style->opts.show_only_as_3_frame);
      break;
    case STYLE_PROP_SHOW_ONLY_IN_SEPARATOR:
      g_value_set_boolean(value, style->opts.show_only_in_separator);
      break;
    case STYLE_PROP_DIRECTIONAL_ENDS:
      g_value_set_boolean(value, style->opts.directional_end);
      break;
    case STYLE_PROP_DEFERRED:
      g_value_set_boolean(value, style->opts.deferred);
      break;
    case STYLE_PROP_LOADED:
      g_value_set_boolean(value, style->opts.loaded);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }
  
  return ;
}

static void zmap_feature_type_style_dispose (GObject *object)
{
  if(style_parent_class_G->dispose)
    (*style_parent_class_G->dispose)(object);
  
  return ;
}

static void zmap_feature_type_style_finalize(GObject *object)
{
  if(style_parent_class_G->finalize)
    (*style_parent_class_G->finalize)(object);

  return ;
}


#endif /* STYLES_ARE_G_OBJECTS */
