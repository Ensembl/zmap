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
 * Last edited: Aug 29 13:52 2008 (edgrif)
 * Created: Mon Feb 26 09:12:18 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.c,v 1.16 2008-09-24 14:43:59 edgrif Exp $
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




/* Checks a style to see if it contains the minimum necessary to be drawn,
 * we need this function because we no longer allow defaults because we
 * we want to do inheritance.
 * 
 * Function returns FALSE if there style is not valid and the GError says
 * what the problem was.
 *  */
gboolean zMapStyleIsDrawable(ZMapFeatureTypeStyle style, GError **error)
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


/* Takes a style and defaults enough properties for the style to be used to
 * draw features (albeit boringly). */
void zMapStyleMakeDrawable(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  /* These are the absolute basic requirements without which we can't display something.... */
  if (!(style->fields_set.mode))
    {
      style->fields_set.mode = TRUE ;
      style->mode = ZMAPSTYLE_MODE_BASIC ;
    }

  if (!(style->fields_set.overlap_mode))
    {
      style->fields_set.overlap_mode = style->fields_set.overlap_default = TRUE ;
      style->curr_overlap_mode = style->default_overlap_mode = ZMAPOVERLAP_COMPLETE ;
    }

  if (!(style->fields_set.width))
     {
       style->fields_set.width = TRUE ;
       style->width = 1.0 ;
    }

  if (!(style->colours.normal.fields_set.fill) && !(style->colours.normal.fields_set.border))
    {
      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			  NULL, NULL, "black") ;
    }

  return ;
}



void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode)
{
  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(style));

  g_return_if_fail((mode >= ZMAPSTYLE_MODE_INVALID) &&
		   (mode <= ZMAPSTYLE_MODE_META));
  
  g_object_set(G_OBJECT(style),
	       "mode", mode,
	       NULL);

  return ;
}


gboolean zMapStyleHasMode(ZMapFeatureTypeStyle style)
{
  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), FALSE);

  return style->fields_set.mode ;
}


ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID;
  
  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), mode);

  mode = style->mode;

  return mode;
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

/* const char *zMapStyleMode2Str(ZMapStyleMode mode); */
ZMAP_ENUM_AS_STRING_FUNC(zMapStyleMode2Str,            ZMapStyleMode,               ZMAP_STYLE_MODE_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleMode2Str,            ZMapStyleMode,               ZMAP_STYLE_MODE_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleColDisplayState2Str, ZMapStyleColumnDisplayState, ZMAP_STYLE_COLUMN_DISPLAY_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyle3FrameMode2Str, ZMapStyle3FrameMode, ZMAP_STYLE_3_FRAME_LIST) ;
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleGraphMode2Str,       ZMapStyleGraphMode,          ZMAP_STYLE_GRAPH_MODE_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleGlyphMode2Str,       ZMapStyleGlyphMode,          ZMAP_STYLE_GLYPH_MODE_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleDrawContext2Str,     ZMapStyleDrawContext,        ZMAP_STYLE_DRAW_CONTEXT_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleColourType2Str,      ZMapStyleColourType,         ZMAP_STYLE_COLOUR_TYPE_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleColourTarget2Str,    ZMapStyleColourTarget,       ZMAP_STYLE_COLOUR_TARGET_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleScoreMode2Str,       ZMapStyleScoreMode,          ZMAP_STYLE_SCORE_MODE_LIST);
ZMAP_ENUM_AS_STRING_FUNC(zmapStyleOverlapMode2Str,     ZMapStyleOverlapMode,        ZMAP_STYLE_OVERLAP_MODE_LIST);



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
  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(style));

  style->opts.show_when_empty = show_when_empty ;

  return ;
}

gboolean zMapStyleGetShowWhenEmpty(ZMapFeatureTypeStyle style)
{
  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), FALSE);

  return style->opts.show_when_empty;
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



enum
  {
    STYLE_PROP_NONE,		/* zero is invalid */

    STYLE_PROP_ORIGINAL_ID,
    STYLE_PROP_UNIQUE_ID,
    STYLE_PROP_NAME,

    STYLE_PROP_COLUMN_DISPLAY_MODE,
    STYLE_PROP_MAIN_COLOUR_NORMAL,
    STYLE_PROP_MAIN_COLOUR_SELECTED,
    STYLE_PROP_FRAME0_COLOUR_NORMAL,
    STYLE_PROP_FRAME0_COLOUR_SELECTED,
    STYLE_PROP_FRAME1_COLOUR_NORMAL,
    STYLE_PROP_FRAME1_COLOUR_SELECTED,
    STYLE_PROP_FRAME2_COLOUR_NORMAL,
    STYLE_PROP_FRAME2_COLOUR_SELECTED,
    STYLE_PROP_REV_COLOUR_NORMAL,
    STYLE_PROP_REV_COLOUR_SELECTED,

    /* properties that need fields_set setting. */
    STYLE_PROP_PARENT_STYLE,
    STYLE_PROP_DESCRIPTION,
    STYLE_PROP_MODE,
    STYLE_PROP_MIN_SCORE,
    STYLE_PROP_MAX_SCORE,
    STYLE_PROP_OVERLAP_MODE,
    STYLE_PROP_OVERLAP_DEFAULT,
    STYLE_PROP_BUMP_SPACING,
    STYLE_PROP_MIN_MAG,
    STYLE_PROP_MAX_MAG,
    STYLE_PROP_WIDTH,
    STYLE_PROP_SCORE_MODE,
    STYLE_PROP_GFF_SOURCE,
    STYLE_PROP_GFF_FEATURE,

    /* State information from the style->opts struct */
    STYLE_PROP_DISPLAYABLE,
    STYLE_PROP_SHOW_WHEN_EMPTY,
    STYLE_PROP_SHOW_TEXT,
    STYLE_PROP_PARSE_GAPS,
    STYLE_PROP_ALIGN_GAPS,
    STYLE_PROP_STRAND_SPECIFIC,
    STYLE_PROP_SHOW_REV_STRAND,
    STYLE_PROP_FRAME_MODE,
    STYLE_PROP_SHOW_ONLY_IN_SEPARATOR,
    STYLE_PROP_DIRECTIONAL_ENDS,
    STYLE_PROP_DEFERRED,
    STYLE_PROP_LOADED,
  };

#define ZMAP_TYPE_COLOUR_STRINGS (zMapStyleColourStringsGetType())

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

static ZMapBaseClass style_parent_class_G = NULL;

GType zMapFeatureTypeStyleGetType(void)
{
  static GType type = 0;
  
  if (type == 0) 
    {
      static const GTypeInfo info = 
	{
	  sizeof (zmapFeatureTypeStyleClass),
	  (GBaseInitFunc)      NULL,
	  (GBaseFinalizeFunc)  NULL,
	  (GClassInitFunc)     zmap_feature_type_style_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL /* class_data */,
	  sizeof (zmapFeatureTypeStyle),
	  0 /* n_preallocs */,
	  (GInstanceInitFunc)  zmap_feature_type_style_init,
	  NULL
	};
      
      type = g_type_register_static (ZMAP_TYPE_BASE, 
				     "ZMapFeatureTypeStyle", 
				     &info, (GTypeFlags)0);
    }
  
  return type;

}

static gpointer colour_strings_copy(gpointer in_ptr)
{
  ZMapStyleColourStrings out;
  ZMapStyleColourStrings in = (ZMapStyleColourStrings)in_ptr;

  out = g_new0(ZMapStyleColourStringsStruct, 1);

  if(in->draw)
    out->draw   = g_strdup(in->draw);
  if(in->fill)
    out->fill   = g_strdup(in->fill);
  if(in->border)
    out->border = g_strdup(in->border);

  return out;
}

static void colour_strings_free(gpointer in_ptr)
{
  ZMapStyleColourStrings in = (ZMapStyleColourStrings)in_ptr;

  if(in->draw)
    g_free(in->draw);
  if(in->fill)
    g_free(in->fill);
  if(in->border)
    g_free(in->border);

  in->fill = in->draw = in->border = NULL;
  g_free(in);

  return ;
}

GType zMapStyleColourStringsGetType(void)
{
  static GType type = 0;

  if(type == 0)
    {
      type = g_boxed_type_register_static("ZMapStyleColourStrings",
					  colour_strings_copy,
					  colour_strings_free);
    }

  return type;
}

ZMapFeatureTypeStyle zMapStyleCreate(char *name, char *description)
{
  return zMapFeatureStyleCreate(name, description);
}

ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name, char *description)
{
  return zMapFeatureStyleCreate(name, description);
}

ZMapFeatureTypeStyle zMapFeatureStyleCreate(char *name, char *description)
{
  ZMapFeatureTypeStyle style = NULL;

  /* Why are some of these set when the paramspec mechanism should default them ?? Well
   * it seems that setting a default value has no effect in the class init.... */

  style = g_object_new(ZMAP_TYPE_FEATURE_STYLE, 
		       "name",        name,
		       "debug",       FALSE,
		       "description", description,
		       "displayable", TRUE,
		       "min-mag",     0.0,
		       "max-mag",     0.0,
		       "display",     ZMAPSTYLE_COLDISPLAY_SHOW_HIDE,
		       NULL);

  return style;
}


ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle src)
{
  ZMapFeatureTypeStyle dest = NULL ;

  zMapStyleCCopy(src, &dest) ;

#ifdef RDS
  ZMapFeatureTypeStyle dest = NULL;
  ZMapBase dest_base;

  if((dest_base = zMapBaseCopy(ZMAP_BASE(src))))
    dest = ZMAP_FEATURE_STYLE(dest_base);
#endif

  return dest ;
}


gboolean zMapStyleCCopy(ZMapFeatureTypeStyle src, ZMapFeatureTypeStyle *dest_out)
{
  gboolean copied = FALSE ;
  ZMapBase dest ;

  if(dest_out && (copied = zMapBaseCCopy(ZMAP_BASE(src), &dest)))
    *dest_out = ZMAP_FEATURE_STYLE(dest);

  (*dest_out)->mode_data = src->mode_data;

  return copied ;
}


GQuark zMapStyleGetID(ZMapFeatureTypeStyle style)
{
  GQuark original_id = 0;

  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), original_id);

  original_id = style->original_id;

  return original_id;
}

GQuark zMapStyleGetUniqueID(ZMapFeatureTypeStyle style)
{
  GQuark unique_id = 0;

  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), unique_id);

  unique_id = style->unique_id;

  return unique_id;
}

char *zMapStyleGetName(ZMapFeatureTypeStyle style)
{
  char *name = NULL;

  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), name);

  name = (char *)g_quark_to_string(style->original_id);

  return name;
}

char *zMapStyleGetDescription(ZMapFeatureTypeStyle style)
{
  char *description = NULL ;

  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), description);

  if (style->fields_set.description)
    description = style->description ;

  return description ;
}

ZMapStyleOverlapMode zMapStyleGetOverlapMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleOverlapMode mode = ZMAPOVERLAP_COMPLETE;

  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), mode);

  mode = style->curr_overlap_mode ;

  return mode;
}

ZMapStyleOverlapMode zMapStyleGetDefaultOverlapMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleOverlapMode mode = ZMAPOVERLAP_COMPLETE;

  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style), mode);

  mode = style->default_overlap_mode ;

  return mode;
}

void zMapStyleDestroy(ZMapFeatureTypeStyle style)
{
  g_object_unref(G_OBJECT(style));

  return ;
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

  style_parent_class_G = g_type_class_peek_parent(style_class);

  /* original and unique ids */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ORIGINAL_ID,
				  g_param_spec_uint("original-id", "original-id",
						    "original id", 
						    0, G_MAXUINT, 0, 
						    ZMAP_PARAM_STATIC_RO));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_UNIQUE_ID,
				  g_param_spec_uint("unique-id", "unique-id",
						    "unique id", 
						    0, G_MAXUINT, 0, 
						    ZMAP_PARAM_STATIC_RO));

  /* Single method of setting original and unique ids */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_NAME,
				  g_param_spec_string("name", "name",
						      "Name", "",
						      ZMAP_PARAM_STATIC_WO));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_COLUMN_DISPLAY_MODE,
				  g_param_spec_uint("display", "display",
						    "Display mode, ", 
						    ZMAPSTYLE_COLDISPLAY_INVALID, 
						    ZMAPSTYLE_COLDISPLAY_SHOW, 
						    ZMAPSTYLE_COLDISPLAY_SHOW_HIDE, 
						    ZMAP_PARAM_STATIC_RW));

  /* Colours... */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MAIN_COLOUR_NORMAL,
				  g_param_spec_boxed("main-colour-normal", "main-colour-normal",
						     "Main normal colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MAIN_COLOUR_SELECTED,
				  g_param_spec_boxed("main-colour-selected", "main-colour-selected",
						     "Main Selected colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME0_COLOUR_NORMAL,
				  g_param_spec_boxed("frame0-colour-normal", "frame0-colour-normal",
						     "Frame0 normal colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME0_COLOUR_SELECTED,
				  g_param_spec_boxed("frame0-colour-selected", "frame0-colour-selected",
						     "Frame0 Selected colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME1_COLOUR_NORMAL,
				  g_param_spec_boxed("frame1-colour-normal", "frame1-colour-normal",
						     "Frame1 normal colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME1_COLOUR_SELECTED,
				  g_param_spec_boxed("frame1-colour-selected", "frame1-colour-selected",
						     "Frame1 Selected colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME2_COLOUR_NORMAL,
				  g_param_spec_boxed("frame2-colour-normal", "frame2-colour-normal",
						     "Frame2 normal colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME2_COLOUR_SELECTED,
				  g_param_spec_boxed("frame2-colour-selected", "frame2-colour-selected",
						     "Frame2 Selected colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_REV_COLOUR_NORMAL,
				  g_param_spec_boxed("rev-colour-normal", "main-colour-normal",
						     "Reverse Strand normal colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_REV_COLOUR_SELECTED,
				  g_param_spec_boxed("rev-colour-selected", "main-colour-selected",
						     "Reverse Strand Selected colour",
						     ZMAP_TYPE_COLOUR_STRINGS,
						     ZMAP_PARAM_STATIC_RW));




  /* Parent id */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_PARENT_STYLE,
				  g_param_spec_uint("parent-style", "parent-style",
						    "Parent style unique id", 
						    0, G_MAXUINT, 0, 
						    ZMAP_PARAM_STATIC_RW));
  /* description */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DESCRIPTION,
				  g_param_spec_string("description", "description",
						      "Description", "",
						      ZMAP_PARAM_STATIC_RW));
  /* Mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MODE,
				  g_param_spec_uint("mode", "mode", "mode", 
						    ZMAPSTYLE_MODE_INVALID, 
						    ZMAPSTYLE_MODE_META, 
						    ZMAPSTYLE_MODE_INVALID, 
						    ZMAP_PARAM_STATIC_RW));
  /* min score */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MIN_SCORE,
				  g_param_spec_double("min-score", "min-score",
						      "minimum score", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* max score */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MAX_SCORE,
				  g_param_spec_double("max-score", "max-score",
						      "maximum score", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* overlap mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_OVERLAP_MODE,
				  g_param_spec_uint("overlap-mode", "overlap-mode",
						    "The Overlap Mode", 
						    ZMAPOVERLAP_START, 
						    ZMAPOVERLAP_END, 
						    ZMAPOVERLAP_START, 
						    ZMAP_PARAM_STATIC_RW));
  /* overlap default */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_OVERLAP_DEFAULT,
				  g_param_spec_uint("default-overlap-mode", "default-overlap-mode",
						    "The Default Overlap Mode", 
						    ZMAPOVERLAP_START, 
						    ZMAPOVERLAP_END, 
						    ZMAPOVERLAP_START, 
						    ZMAP_PARAM_STATIC_RW));
  /* bump spacing */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_BUMP_SPACING,
				  g_param_spec_double("bump-spacing", "bump-spacing",
						      "space between columns in bumped columns", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* min mag */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MIN_MAG,
				  g_param_spec_double("min-mag", "min-mag",
						      "minimum magnification", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* max mag */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MAX_MAG,
				  g_param_spec_double("max-mag", "max-mag",
						      "maximum magnification", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* width */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_WIDTH,
				  g_param_spec_double("width", "width",
						      "Width", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* score mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SCORE_MODE,
				  g_param_spec_uint("score-mode", "score-mode",
						    "Score Mode",
						    ZMAPSCORE_WIDTH,
						    ZMAPSCORE_PERCENT,
						    ZMAPSCORE_WIDTH,
						    ZMAP_PARAM_STATIC_RW));
  /* gff_source */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_GFF_SOURCE,
				  g_param_spec_string("gff-source", "gff-source",
						      "GFF Source", "",
						      ZMAP_PARAM_STATIC_RW));
  /* gff_feature */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_GFF_FEATURE,
				  g_param_spec_string("gff-feature", "gff-feature",
						      "GFF Feature", "",
						      ZMAP_PARAM_STATIC_RW));

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
				  STYLE_PROP_FRAME_MODE,
				  g_param_spec_uint("frame-mode", "3 frame display mode",
						    "Defines frame sensitive display in 3 frame mode.", 
						    ZMAPSTYLE_3_FRAME_INVALID, 
						    ZMAPSTYLE_3_FRAME_ONLY_1, 
						    ZMAPSTYLE_3_FRAME_INVALID, 
						    ZMAP_PARAM_STATIC_RW));

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
    case STYLE_PROP_ORIGINAL_ID:
      {
	GQuark id;
	if((id = g_value_get_uint(value)) > 0)
	  style->original_id = id;
      }
      break;
    case STYLE_PROP_UNIQUE_ID:
      {
	GQuark id;
	if((id = g_value_get_uint(value)) > 0)
	  style->unique_id = id;
      }
      break;
    case STYLE_PROP_NAME:
      {
	const char *name;
	if((name = g_value_get_string(value)))
	  {
	    style->original_id = g_quark_from_string(name);
	    style->unique_id   = zMapStyleCreateID((char *)name);
	  }
      }
      break;

    case STYLE_PROP_COLUMN_DISPLAY_MODE:
      {
	ZMapStyleColumnDisplayState state = ZMAPSTYLE_COLDISPLAY_INVALID;
	/* Should already have been range checked by gobject code */
	if((state = g_value_get_uint(value)) >= ZMAPSTYLE_COLDISPLAY_INVALID)
	  {
	    style->col_display_state = state;
	  }
      }
      break;

    case STYLE_PROP_MAIN_COLOUR_NORMAL:
    case STYLE_PROP_MAIN_COLOUR_SELECTED:
    case STYLE_PROP_FRAME0_COLOUR_NORMAL:
    case STYLE_PROP_FRAME0_COLOUR_SELECTED:
    case STYLE_PROP_FRAME1_COLOUR_NORMAL:
    case STYLE_PROP_FRAME1_COLOUR_SELECTED:
    case STYLE_PROP_FRAME2_COLOUR_NORMAL:
    case STYLE_PROP_FRAME2_COLOUR_SELECTED:
    case STYLE_PROP_REV_COLOUR_NORMAL:
    case STYLE_PROP_REV_COLOUR_SELECTED:
      {
	ZMapStyleColourStruct colour = {{ 0 }};
	ZMapStyleColourStrings colours;
	
	if((colours = g_value_get_boxed(value)))
	  {
	    if(colours->border && *(colours->border))
	      colour.fields_set.border = gdk_color_parse(colours->border, 
							 &(colour.border));
	    if(colours->fill && *(colours->fill))
	      colour.fields_set.fill = gdk_color_parse(colours->fill, 
							 &(colour.fill));
	    if(colours->draw && *(colours->draw))
	      colour.fields_set.draw = gdk_color_parse(colours->draw, 
							 &(colour.draw));
	    /* Now copy the struct... */
	    switch(param_id)
	      {
	      case STYLE_PROP_MAIN_COLOUR_NORMAL:
		style->colours.normal = colour;
		break;
	      case STYLE_PROP_MAIN_COLOUR_SELECTED:
		style->colours.selected = colour;
		break;
	      case STYLE_PROP_FRAME0_COLOUR_NORMAL:
		style->frame0_colours.normal   = colour;
		break;
	      case STYLE_PROP_FRAME0_COLOUR_SELECTED:
		style->frame0_colours.selected = colour;
		break;
	      case STYLE_PROP_FRAME1_COLOUR_NORMAL:
		style->frame1_colours.normal   = colour;
		break;
	      case STYLE_PROP_FRAME1_COLOUR_SELECTED:
		style->frame1_colours.selected = colour;
		break;
	      case STYLE_PROP_FRAME2_COLOUR_NORMAL:
		style->frame2_colours.normal   = colour;
		break;
	      case STYLE_PROP_FRAME2_COLOUR_SELECTED:
		style->frame2_colours.selected = colour;
		break;
	      case STYLE_PROP_REV_COLOUR_NORMAL:
		style->strand_rev_colours.normal   = colour;
		break;
	      case STYLE_PROP_REV_COLOUR_SELECTED:
		style->strand_rev_colours.selected = colour;
		break;
	      default:
		break;
	      }
	  }
      }
      break;

      /* properties that need fields_set setting. */
    case STYLE_PROP_PARENT_STYLE:
      {
	GQuark parent_id;
	if((parent_id = g_value_get_uint(value)) > 0)
	  {
	    style->parent_id = parent_id;
	    style->fields_set.parent_style = 1;
	  }
	else
	  {
	    style->parent_id = 0;
	    style->fields_set.parent_style = 0;	  
	  }
      }
      break;
    case STYLE_PROP_DESCRIPTION:
      {
	const char *description;
	unsigned int set = 0;

	if((description = g_value_get_string(value)))
	  {
	    if(style->description)
	      g_free(style->description);
	    style->description = g_value_dup_string(value);
	    set = 1;
	  }
	else if(style->description)
	  {
	    /* Free if we did have a value, but now don't */
	    g_free(style->description);
	    style->description = NULL;
	  }
	/* record if we're set or not */
	style->fields_set.description = set;
      }
      break;
    case STYLE_PROP_MODE:
      {
	ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID;
	
	if((mode = g_value_get_uint(value)) > ZMAPSTYLE_MODE_INVALID)
	  {
	    style->mode            = mode;
	    style->fields_set.mode = 1;
	  }
      }
      break;
    case STYLE_PROP_MIN_SCORE:
      {
	if((style->min_score = g_value_get_double(value)) != 0.0)
	  style->fields_set.min_score = 1;
      }
      break;
    case STYLE_PROP_MAX_SCORE:
      {
	if((style->max_score = g_value_get_double(value)) != 0.0)
	  style->fields_set.max_score = 1;
      }
      break;
    case STYLE_PROP_OVERLAP_MODE:
      {
	ZMapStyleOverlapMode mode = ZMAPOVERLAP_INVALID;

	if((mode = g_value_get_uint(value)) > ZMAPOVERLAP_INVALID)
	  {
	    style->fields_set.overlap_mode = 1;
	    style->curr_overlap_mode       = mode;
	  }
      }
      break;
    case STYLE_PROP_OVERLAP_DEFAULT:
      {
	ZMapStyleOverlapMode mode = ZMAPOVERLAP_INVALID;

	if((mode = g_value_get_uint(value)) > ZMAPOVERLAP_INVALID)
	  {
	    style->fields_set.overlap_default = 1;
	    style->default_overlap_mode       = mode;
	  }
      }
      break;
    case STYLE_PROP_BUMP_SPACING:
      {
	unsigned int set = 0;
	style->fields_set.bump_spacing = set;
      }
      break; 
    case STYLE_PROP_MIN_MAG:
      {
	if((style->min_mag = g_value_get_double(value)) != 0.0)
	  style->fields_set.min_mag = 1;
      }
      break;
    case STYLE_PROP_MAX_MAG:
      {
	if((style->max_mag = g_value_get_double(value)) != 0.0)
	  style->fields_set.max_mag = 1;
      }
      break;
    case STYLE_PROP_WIDTH:
      {
	if((style->width = g_value_get_double(value)) != 0.0)
	  style->fields_set.width = 1;
      }
      break;
    case STYLE_PROP_SCORE_MODE:
      {
	unsigned int set = 0;
	style->fields_set.score_mode = set;
      }
      break; 
    case STYLE_PROP_GFF_SOURCE:
      {
	const char *gff_source_str = NULL;
	if((gff_source_str = g_value_get_string(value)) != NULL)
	  {
	    style->gff_source = g_quark_from_string(gff_source_str);
	    style->fields_set.gff_source = 1;
	  }
	else
	  {
	    style->gff_source = 0;
	    style->fields_set.gff_source = 0;	  
	  }
      }
      break;
    case STYLE_PROP_GFF_FEATURE:
      {
	const char *gff_feature_str = NULL;
	if((gff_feature_str = g_value_get_string(value)) != NULL)
	  {
	    style->gff_feature = g_quark_from_string(gff_feature_str);
	    style->fields_set.gff_feature = 1;
	  }
	else
	  {
	    style->gff_feature = 0;
	    style->fields_set.gff_feature = 0;	  
	  }
      }
      break;

    case STYLE_PROP_STRAND_SPECIFIC:
      style->opts.strand_specific = g_value_get_boolean(value);
      style->fields_set.strand_specific = TRUE ;
      break;
    case STYLE_PROP_SHOW_REV_STRAND:
      style->opts.show_rev_strand = g_value_get_boolean(value);
      style->fields_set.show_rev_strand = TRUE ;
      break;
    case STYLE_PROP_FRAME_MODE:
      {
	ZMapStyle3FrameMode frame_mode ;

	frame_mode = g_value_get_uint(value) ;

	if (frame_mode >= ZMAPSTYLE_3_FRAME_INVALID && frame_mode <= ZMAPSTYLE_3_FRAME_ONLY_1)
	  {
	    style->opts.frame_specific = style->fields_set.frame_specific = TRUE ;
	    style->frame_mode = frame_mode ;
	    style->fields_set.frame_specific = TRUE ;
	  }

	break;
      }

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
    case STYLE_PROP_ORIGINAL_ID:
      {
	if(style->original_id)
	  g_value_set_uint(value, style->original_id);
      }
      break;
    case STYLE_PROP_UNIQUE_ID:
      {
	if(style->unique_id)
	  g_value_set_uint(value, style->unique_id);
      }
      break;
    case STYLE_PROP_NAME:
      {
	if(style->original_id)
	  g_value_set_string(value, g_quark_to_string(style->original_id));
      }
      break;

    case STYLE_PROP_COLUMN_DISPLAY_MODE:
      g_value_set_uint(value, style->col_display_state);
      break;
    case STYLE_PROP_MAIN_COLOUR_NORMAL:
    case STYLE_PROP_MAIN_COLOUR_SELECTED:
    case STYLE_PROP_FRAME0_COLOUR_NORMAL:
    case STYLE_PROP_FRAME0_COLOUR_SELECTED:
    case STYLE_PROP_FRAME1_COLOUR_NORMAL:
    case STYLE_PROP_FRAME1_COLOUR_SELECTED:
    case STYLE_PROP_FRAME2_COLOUR_NORMAL:
    case STYLE_PROP_FRAME2_COLOUR_SELECTED:
    case STYLE_PROP_REV_COLOUR_NORMAL:
    case STYLE_PROP_REV_COLOUR_SELECTED:
      {
	ZMapStyleColour this_colour;
	ZMapStyleColourStrings colours = g_new0(ZMapStyleColourStringsStruct, 1);
	/* We allocate here, do a take_boxed and it's g_object_get caller's 
	 * responsibility to g_free struct & strings... */
	/* colour_strings = {"#rrrrggggbbbb", 
	 *                   "#rrrrggggbbbb", 
	 *                   "#rrrrggggbbbb"};
	 * These are just to show the style (gdk_color_to_string 2.12+ only) */

	switch(param_id)
	  {
	  case STYLE_PROP_MAIN_COLOUR_NORMAL:
	    this_colour = &(style->colours.normal);
	    break;
	  case STYLE_PROP_MAIN_COLOUR_SELECTED:
	    this_colour = &(style->colours.selected);
	    break;
	  case STYLE_PROP_FRAME0_COLOUR_NORMAL:
	    this_colour = &(style->frame0_colours.normal);
	    break;
	  case STYLE_PROP_FRAME0_COLOUR_SELECTED:
	    this_colour = &(style->frame0_colours.selected);
	    break;
	  case STYLE_PROP_FRAME1_COLOUR_NORMAL:
	    this_colour = &(style->frame1_colours.normal);
	    break;
	  case STYLE_PROP_FRAME1_COLOUR_SELECTED:
	    this_colour = &(style->frame1_colours.selected);
	    break;
	  case STYLE_PROP_FRAME2_COLOUR_NORMAL:
	    this_colour = &(style->frame2_colours.normal);
	    break;
	  case STYLE_PROP_FRAME2_COLOUR_SELECTED:
	    this_colour = &(style->frame2_colours.selected);
	    break;
	  case STYLE_PROP_REV_COLOUR_NORMAL:
	    this_colour = &(style->strand_rev_colours.normal);
	    break;
	  case STYLE_PROP_REV_COLOUR_SELECTED:
	    this_colour = &(style->strand_rev_colours.selected);
	    break;
	  default:
	    break;
	  }
	  
	if(this_colour->fields_set.draw)
	  colours->draw = g_strdup_printf("#%04X%04X%04X",
					 this_colour->draw.red,
					 this_colour->draw.green,
					 this_colour->draw.blue);
	if(this_colour->fields_set.fill)
	  colours->fill = g_strdup_printf("#%04X%04X%04X",
					 this_colour->fill.red,
					 this_colour->fill.green,
					 this_colour->fill.blue);
	if(this_colour->fields_set.border)
	  colours->border = g_strdup_printf("#%04X%04X%04X",
					   this_colour->border.red,
					   this_colour->border.green,
					   this_colour->border.blue);

	g_value_take_boxed(value, colours);

#ifdef RDS
	if(this_colour->fields_set.border && colours.border)
	  g_free(colours.border);
	if(this_colour->fields_set.fill && colours.fill)
	  g_free(colours.fill);
	if(this_colour->fields_set.draw && colours.draw)
	  g_free(colours.draw);
#endif
      }
      break;

    case STYLE_PROP_PARENT_STYLE:
      g_value_set_uint(value, style->parent_id);
      break;
    case STYLE_PROP_DESCRIPTION:
      g_value_set_string(value, style->description);
      break;
    case STYLE_PROP_MODE:
      g_value_set_uint(value, style->mode);
      break;
    case STYLE_PROP_MIN_SCORE:
      g_value_set_double(value, style->min_score);      
      break;
    case STYLE_PROP_MAX_SCORE:
      g_value_set_double(value, style->max_score);
      break;
    case STYLE_PROP_OVERLAP_MODE:
      g_value_set_uint(value, style->curr_overlap_mode);
      break;
    case STYLE_PROP_OVERLAP_DEFAULT:
      g_value_set_uint(value, style->default_overlap_mode);
      break;
    case STYLE_PROP_FRAME_MODE:
      g_value_set_uint(value, style->frame_mode) ;
      break;
    case STYLE_PROP_BUMP_SPACING:
      g_value_set_double(value, style->bump_spacing);
      break; 
    case STYLE_PROP_MIN_MAG:
      g_value_set_double(value, style->min_mag);
      break;
    case STYLE_PROP_MAX_MAG:
      g_value_set_double(value, style->max_mag);
      break;
    case STYLE_PROP_WIDTH:
      g_value_set_double(value, style->width);
      break;
    case STYLE_PROP_SCORE_MODE:
      g_value_set_uint(value, style->score_mode);
      break; 
    case STYLE_PROP_GFF_SOURCE:
      {
	unsigned int set = 0;
	
	style->fields_set.gff_source = set;
      }
      break;
    case STYLE_PROP_GFF_FEATURE:
      {
	unsigned int set = 0;
	style->fields_set.gff_feature = set;
      }
      break;

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
  GObjectClass *parent_class = G_OBJECT_CLASS(style_parent_class_G);

  if(parent_class->dispose)
    (*parent_class->dispose)(object);

  return ;
}

static void zmap_feature_type_style_finalize(GObject *object)
{
  ZMapFeatureTypeStyle style = ZMAP_FEATURE_STYLE(object);
  GObjectClass *parent_class = G_OBJECT_CLASS(style_parent_class_G);

  if(style->description)
    g_free(style->description);
  style->description = NULL;

  if(parent_class->finalize)
    (*parent_class->finalize)(object);

  return ;
}


