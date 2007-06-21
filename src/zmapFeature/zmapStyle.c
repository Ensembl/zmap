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
 * Last edited: Jun 19 13:19 2007 (edgrif)
 * Created: Mon Feb 26 09:12:18 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.c,v 1.4 2007-06-21 12:26:17 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapUtils.h>
#include <zmapStyle_P.h>



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Hack for now...use the macro trick.... */
/* Used to construct a table of strings and corresponding enums to go between the two,
 * the table must end with an entry in which type_str is NULL. */
typedef struct
{
  char *type_str ;
  int type_enum ;
} ZMapFeatureStr2EnumStruct, *ZMapFeatureStr2Enum ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





static void setColours(ZMapStyleColour colour, char *border, char *draw, char *fill) ;



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
	case ZMAPSTYLE_MODE_META:
	  {
	    /* We shouldn't be displaying meta styles.... */
	    valid = FALSE ;
	    code = 5 ;
	    message = g_strdup("Meta Styles cannot be displayed.") ;
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

  setColours(colour, border, draw, fill) ;

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






#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapStyleSetColours(ZMapFeatureTypeStyle style, char *border, char *draw, char *fill)
{
  zMapAssert(style);

  setColours(&(style->colours.normal), border, draw, fill) ;

  return ;
}


void zMapStyleSetColoursSelected(ZMapFeatureTypeStyle style, char *border, char *draw, char *fill)
{
  zMapAssert(style);

  setColours(&(style->colours.selected), border, draw, fill) ;

  return ;
}


void zMapStyleSetColoursCDS(ZMapFeatureTypeStyle style, char *border, char *draw, char *fill)
{
  zMapAssert(style);

  setColours(&(style->mode_data.transcript.CDS_colours.normal), border, draw, fill) ;

  return ;
}

void zMapStyleSetColoursCDSSelected(ZMapFeatureTypeStyle style, char *border, char *draw, char *fill)
{
  zMapAssert(style);

  setColours(&(style->mode_data.transcript.CDS_colours.selected), border, draw, fill) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, 
			     GdkColor **background, GdkColor **foreground, GdkColor **outline)
{
  gboolean result = FALSE ;

  zMapAssert(style && (background || foreground || outline)) ;

  if (background)
    {
      if(style->colours.normal.fields_set.fill)
	{
	  *background = &(style->colours.normal.fill) ;
	  result = TRUE ;
	}
    }

  if (foreground)
    {
      if(style->colours.normal.fields_set.draw)
	{
	  *foreground = &(style->colours.normal.draw);
	  result = TRUE ;
	}
    }

  if (outline)
    {
      if (style->colours.normal.fields_set.border)
	{
	  *outline = &(style->colours.normal.border);
	  result = TRUE ;
	}
    }

  return result ;
}


gboolean zMapStyleGetColoursCDS(ZMapFeatureTypeStyle style, 
				GdkColor **background, GdkColor **foreground, GdkColor **outline)
{
  gboolean result = FALSE ;

  zMapAssert(style && (background || foreground || outline)) ;

  if (background)
    {
      if (style->mode_data.transcript.CDS_colours.normal.fields_set.fill)
	{
	  *background = &(style->mode_data.transcript.CDS_colours.normal.fill) ;
	  result = TRUE ;
	}
    }

  if (foreground)
    {
      if (style->mode_data.transcript.CDS_colours.normal.fields_set.draw)
	{
	  *foreground = &(style->mode_data.transcript.CDS_colours.normal.draw);
	  result = TRUE ;
	}
    }

  if (outline)
    {
      if (style->mode_data.transcript.CDS_colours.normal.fields_set.border)
	{
	  *outline = &(style->mode_data.transcript.CDS_colours.normal.border);
	  result = TRUE ;
	}
    }

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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


void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean show_gaps, gboolean parse_gaps,
			      unsigned int within_align_error)
{
  zMapAssert(style);

  style->opts.align_gaps = show_gaps ;
  style->opts.parse_gaps = parse_gaps ;
  style->mode_data.alignment.within_align_error = within_align_error ;
  style->mode_data.alignment.fields_set.within_align_error = TRUE ;

  return ;
}


gboolean zMapStyleGetGappedAligns(ZMapFeatureTypeStyle style, unsigned int *within_align_error)
{
  zMapAssert(style);

  if (style->opts.align_gaps && within_align_error)
    *within_align_error = style->mode_data.alignment.within_align_error ;

  return style->opts.align_gaps ;
}





void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, gboolean join_aligns, unsigned int between_align_error)
{
  zMapAssert(style);

  style->opts.join_aligns = join_aligns ;
  style->mode_data.alignment.between_align_error = between_align_error ;
  style->mode_data.alignment.fields_set.between_align_error = TRUE ;

  return ;
}


/* Returns TRUE and returns the between_align_error if join_aligns is TRUE for the style,
 * otherwise returns FALSE. */
gboolean zMapStyleGetJoinAligns(ZMapFeatureTypeStyle style, unsigned int *between_align_error)
{
  zMapAssert(style);

  if (style->opts.join_aligns)
    *between_align_error = style->mode_data.alignment.between_align_error ;

  return style->opts.join_aligns ;
}




gboolean zMapStyleIsFrameSpecific(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.frame_specific ;
}




void zMapStyleSetHideAlways(ZMapFeatureTypeStyle style, gboolean hide_always)
{
  zMapAssert(style) ;

  style->opts.hidden_always = hide_always ;


  return ;
}

gboolean zMapStyleIsHiddenAlways(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.hidden_always ;
}


/* Controls whether the feature set is displayed initially. */
void zMapStyleSetHidden(ZMapFeatureTypeStyle style, gboolean hidden)
{
  zMapAssert(style) ;

  style->opts.hidden_init = hidden ;

  return ;
}

gboolean zMapStyleIsHiddenInit(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.hidden_init ;
}


/* Controls whether the feature set is displayed initially. */
void zMapStyleSetShowWhenEmpty(ZMapFeatureTypeStyle style, gboolean show_when_empty)
{
  zMapAssert(style) ;

  style->opts.show_when_empty = show_when_empty ;

  return ;
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


static void setColours(ZMapStyleColour colour, char *border, char *draw, char *fill)
{
  zMapAssert(colour) ;

  if (border && *border)
    {
      if (gdk_color_parse(border, &(colour->border)))
	colour->fields_set.border = TRUE ;
    }
  if (draw && *draw)
    {
      if (gdk_color_parse(draw, &(colour->draw)))
	colour->fields_set.draw = TRUE ;
    }
  if (fill && *fill)
    {
      if (gdk_color_parse(fill, &(colour->fill)))
	colour->fields_set.fill = TRUE ;
    }
  
  return ;
}


