/*  File: zmapStyleUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Exported functions: See ZMap/zmapStyle.h
 *              
 * HISTORY:
 * Last edited: Jul 29 09:53 2009 (edgrif)
 * Created: Thu Oct 30 10:24:35 2008 (edgrif)
 * CVS info:   $Id: zmapStyleUtils.c,v 1.7 2009-07-29 12:46:12 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <unistd.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapEnum.h>
#include <ZMap/zmapConfigLoader.h>
#include <zmapStyle_P.h>




/* 
 * Set of macros to output style information.
 * 
 * NOTE, you must define TEST, STYLE_PTR and INDENT_STR before using the macros,
 * e.g.
 * 
 *  #define TEST full
 *  #define STYLE_PTR style
 *  #define INDENT_STR indent
 * 
 */

/* Printf doesn't go bools so use this. */
#define PRINTBOOL(BOOLEAN) \
  (BOOLEAN ? "TRUE" : "FALSE")

#define PRINTFIELD(DEST, FLAG, FIELD, FIELD_STR, FORMAT_CONV, FIELD_TO_STR_FUNC) \
  if (TEST || STYLE_PTR->FLAG)					\
    zMapOutWriteFormat((DEST), "%s" FIELD_STR ":\t\t%s\t\t" FORMAT_CONV "\n", \
           INDENT_STR,                                                      \
	   (STYLE_PTR->FLAG ? "<SET>" : "<UNSET>"),		\
	   FIELD_TO_STR_FUNC(STYLE_PTR->FIELD))

#define PRINTCOLOURTARGET(DEST, FIELD, FIELD_STR, TYPE, TARGET)			                    \
  if (TEST || STYLE_PTR->FIELD.TYPE.fields_set.TARGET)						    \
    zMapOutWriteFormat((DEST), "%s" FIELD_STR " " #TYPE " " #TARGET ":\t\t%s\t\tpixel\t%d\tRGB\t%d,%d,%d\n", \
           INDENT_STR,                                                                               \
	   (STYLE_PTR->FIELD.TYPE.fields_set.TARGET ? "<SET>" : "<UNSET>"),				    \
	   STYLE_PTR->FIELD.TYPE.TARGET.pixel, STYLE_PTR->FIELD.TYPE.TARGET.red, STYLE_PTR->FIELD.TYPE.TARGET.green, STYLE_PTR->FIELD.TYPE.TARGET.blue)

#define PRINTCOLOUR(DEST, FIELD, FIELD_STR, TYPE)		\
  PRINTCOLOURTARGET(DEST, FIELD, FIELD_STR, TYPE, fill) ;     \
  PRINTCOLOURTARGET(DEST, FIELD, FIELD_STR, TYPE, draw) ;     \
  PRINTCOLOURTARGET(DEST, FIELD, FIELD_STR, TYPE, border) ;

#define PRINTFULLCOLOUR(DEST, FIELD, FIELD_STR)       \
      PRINTCOLOUR(DEST, FIELD, FIELD_STR, normal) ;	\
      PRINTCOLOUR(DEST, FIELD, FIELD_STR, selected) ;


typedef struct
{
  ZMapIOOut dest ;
  gboolean full ;
} StylePrintStruct, *StylePrint ;



static void setPrintFunc(GQuark key_id, gpointer data, gpointer user_data) ;
static void listPrintFunc(gpointer data, gpointer user_data) ;






/*! @addtogroup zmapstyles
 * @{
 *  */


/* Enum -> String functions, these functions convert the enums to strings given by the XXX_LIST macros.
 * 
 * The functions all have the form
 *  const char *zMapStyleXXXXMode2Str(ZMapStyleXXXXXMode mode)
 *  {
 *    code...
 *  }
 *
 *  */
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2Mode,            ZMapStyleMode, ZMAPSTYLE_MODE_INVALID, ZMAP_STYLE_MODE_LIST, , ) ;
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColDisplayState, ZMapStyleColumnDisplayState, ZMAPSTYLE_COLDISPLAY_INVALID, ZMAP_STYLE_COLUMN_DISPLAY_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr23FrameMode,      ZMapStyle3FrameMode, ZMAPSTYLE_3_FRAME_INVALID, ZMAP_STYLE_3_FRAME_LIST, , ) ;
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2GraphMode,       ZMapStyleGraphMode,          ZMAPSTYLE_GRAPH_INVALID, ZMAP_STYLE_GRAPH_MODE_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2GlyphMode,       ZMapStyleGlyphMode,          ZMAPSTYLE_GLYPH_INVALID, ZMAP_STYLE_GLYPH_MODE_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2DrawContext,     ZMapStyleDrawContext,        ZMAPSTYLE_DRAW_INVALID, ZMAP_STYLE_DRAW_CONTEXT_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColourType,      ZMapStyleColourType,         ZMAPSTYLE_COLOURTYPE_INVALID, ZMAP_STYLE_COLOUR_TYPE_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColourTarget,    ZMapStyleColourTarget,       ZMAPSTYLE_COLOURTARGET_INVALID, ZMAP_STYLE_COLOUR_TARGET_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ScoreMode,       ZMapStyleScoreMode,          ZMAPSCORE_INVALID, ZMAP_STYLE_SCORE_MODE_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2BumpMode,     ZMapStyleBumpMode,        ZMAPBUMP_INVALID, ZMAP_STYLE_BUMP_MODE_LIST, , );




/* Enum -> String functions, these functions convert the enums _directly_ to strings.
 * 
 * The functions all have the form
 *  const char *zMapStyleXXXXMode2ExactStr(ZMapStyleXXXXXMode mode)
 *  {
 *    code...
 *  }
 *
 *  */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapStyleMode2ExactStr,            ZMapStyleMode,               ZMAP_STYLE_MODE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleColDisplayState2ExactStr, ZMapStyleColumnDisplayState, ZMAP_STYLE_COLUMN_DISPLAY_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyle3FrameMode2ExactStr, ZMapStyle3FrameMode, ZMAP_STYLE_3_FRAME_LIST) ;
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleGraphMode2ExactStr,       ZMapStyleGraphMode,          ZMAP_STYLE_GRAPH_MODE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleGlyphMode2ExactStr,       ZMapStyleGlyphMode,          ZMAP_STYLE_GLYPH_MODE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleDrawContext2ExactStr,     ZMapStyleDrawContext,        ZMAP_STYLE_DRAW_CONTEXT_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleColourType2ExactStr,      ZMapStyleColourType,         ZMAP_STYLE_COLOUR_TYPE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleColourTarget2ExactStr,    ZMapStyleColourTarget,       ZMAP_STYLE_COLOUR_TARGET_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleScoreMode2ExactStr,       ZMapStyleScoreMode,          ZMAP_STYLE_SCORE_MODE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleBumpMode2ExactStr,     ZMapStyleBumpMode,        ZMAP_STYLE_BUMP_MODE_LIST);



/* Enum -> Short Text functions, these functions convert the enums to their corresponding short
 * text description.
 * 
 * The functions all have the form
 *  const char *zMapStyleXXXXMode2ExactStr(ZMapStyleXXXXXMode mode)
 *  {
 *    code...
 *  }
 *
 *  */
ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zmapStyleBumpMode2ShortText,            ZMapStyleBumpMode,               ZMAP_STYLE_BUMP_MODE_LIST);







GData *zMapFeatureTypeGetFromFile(char *styles_list, char *styles_file_name)
{
  GData *styles = NULL ;
  ZMapConfigIniContext context ;
  GList *settings_list = NULL, *free_this_list = NULL;


  if ((context = zMapConfigIniContextProvideNamed(ZMAPSTANZA_STYLE_CONFIG)))
    {
      if (zMapConfigIniContextIncludeFile(context, styles_file_name))
	settings_list = zMapConfigIniContextGetNamed(context, ZMAPSTANZA_STYLE_CONFIG) ;
	  
      zMapConfigIniContextDestroy(context) ;
      context = NULL;
    }

  if (settings_list)
    {
      free_this_list = settings_list ;

      g_datalist_init(&styles) ;

      do
	{
	  ZMapKeyValue curr_config_style ;
	  ZMapFeatureTypeStyle new_style ;
	  GParameter params[100] ;			    /* TMP....make dynamic.... */
	  guint num_params ;
	  GParameter *curr_param ;
	  
	  /* Reset params memory.... */
	  memset(&params, 0, sizeof(params)) ;
	  curr_param = params ;
	  num_params = 0 ;
	  curr_config_style = (ZMapKeyValue)(settings_list->data) ;


	  /* The first item is special, the "name" field is the name of the style and
	   * is derived from the name of the stanza and so must be treated specially. */
	  zMapAssert(curr_config_style->name && *(curr_config_style->name)) ;
	  g_value_init(&(curr_param->value), G_TYPE_STRING) ;
	  curr_param->name = curr_config_style->name ;
	  g_value_set_string(&(curr_param->value), curr_config_style->data.str) ;
	  num_params++ ;
	  curr_param++ ;
	  curr_config_style++ ;

	  while (curr_config_style->name)
	    {
	      if (curr_config_style->has_value)
		{
		  curr_param->name = curr_config_style->name ;

		  switch (curr_config_style->type)
		    {
		    case ZMAPCONF_STR_ARRAY:
		      {
			if (curr_config_style->conv_type == ZMAPCONV_STR2COLOUR)
			  {
			    g_value_init(&(curr_param->value), G_TYPE_STRING) ;
			    g_value_set_string(&(curr_param->value), curr_config_style->data.str) ;
			  }
			break ;
		      }
		    case ZMAPCONF_STR:
		      {
			if (curr_config_style->conv_type == ZMAPCONV_STR2ENUM)
			  {
			    int enum_value ;

			    enum_value = (curr_config_style->conv_func.str2enum)(curr_config_style->data.str) ;

			    g_value_init(&(curr_param->value), G_TYPE_INT) ;
			    g_value_set_int(&(curr_param->value), enum_value) ;
			  }
			else
			  {
			    g_value_init(&(curr_param->value), G_TYPE_STRING) ;
			    g_value_set_string(&(curr_param->value), curr_config_style->data.str) ;
			  }

			break ;
		      }
		    case ZMAPCONF_DOUBLE:
		      {
			g_value_init(&(curr_param->value), G_TYPE_DOUBLE) ;
			g_value_set_double(&(curr_param->value), curr_config_style->data.d) ;
			break ;
		      }
		    case ZMAPCONF_INT:
		      {
			g_value_init(&(curr_param->value), G_TYPE_INT) ;
			g_value_set_int(&(curr_param->value), curr_config_style->data.i) ;
			break ;
		      }
		    case ZMAPCONF_BOOLEAN:
		      {
			g_value_init(&(curr_param->value), G_TYPE_BOOLEAN) ;
			g_value_set_boolean(&(curr_param->value), curr_config_style->data.b) ;
			break ;
		      }
		    default:
		      {
			zMapAssertNotReached() ;
			break ;
		      }
		    }
		  num_params++ ;
		  curr_param++ ;
		}
	      curr_config_style++ ;
	    }


	  if ((new_style = zMapStyleCreateV(num_params, params)))
	    {
	      if (!zMapStyleSetAdd(&styles, new_style))
		{
		  /* Free style, report error and move on. */
		  zMapStyleDestroy(new_style) ;

		  zMapLogWarning("Styles file \"%s\", stanza %s could not be added.",
				 styles_file_name, curr_config_style->name) ;
		}
	    }

	} while((settings_list = g_list_next(settings_list)));

    }

  zMapConfigStylesFreeList(free_this_list) ;

  return styles ;
}



void zMapStyleSetPrintAll(ZMapIOOut dest, GData *type_set, char *user_string, gboolean full)
{
  StylePrintStruct print_data ;

  print_data.dest = dest ;
  print_data.full = full ;

  zMapOutWriteFormat(dest, "\nTypes at %s\n", user_string) ;

  g_datalist_foreach(&type_set, setPrintFunc, &print_data) ;

  return ;
}

void zMapStyleSetPrintAllStdOut(GData *type_set, char *user_string, gboolean full)
{
  ZMapIOOut output ;

  output = zMapOutCreateFD(STDOUT_FILENO) ;

  zMapStyleSetPrintAll(output, type_set, user_string, full) ;

  zMapOutDestroy(output) ;

  return ;
}



void zMapStyleListPrintAll(ZMapIOOut dest, GList *styles, char *user_string, gboolean full)
{
  StylePrintStruct print_data ;

  print_data.dest = dest ;
  print_data.full = full ;

  zMapOutWriteFormat(dest, "\nTypes at %s\n", user_string) ;

  g_list_foreach(styles, listPrintFunc, &print_data) ;

  return ;
}



/*!
 * Print out a style.
 *
 * @param   dest                ZMapIOOut to which the style should be printed.
 * @param   style               The style to be printed.
 * @param   prefix              Message to be output as part of the header for the style.
 * @param   full                If TRUE all possible info. printed out, FALSE only fields 
 *                              that have been set are printed.
 * @return   <nothing>
 *  */
void zMapStylePrint(ZMapIOOut dest, ZMapFeatureTypeStyle style, char *prefix, gboolean full)
{
  char *indent = "" ;

  #define TEST full
  #define INDENT_STR indent
  #define STYLE_PTR style


  zMapAssert(ZMAP_IS_FEATURE_STYLE(style)) ;

  full = TRUE ;

  zMapOutWriteFormat(dest, "%s%s Style - \"%s\" (unique id = \"%s\")\n", (prefix ? prefix : ""),
	 indent, g_quark_to_string(style->original_id), g_quark_to_string(style->unique_id)) ;

  indent = "\t" ;
  
  PRINTFIELD(dest, fields_set.parent_id, parent_id, "Parent style", "%s", g_quark_to_string) ;

  PRINTFIELD(dest, fields_set.description, description, "Description", "%s", (char *)) ;

  PRINTFIELD(dest, fields_set.mode, mode, "Feature mode", "%s", zMapStyleMode2ExactStr) ;

  zMapOutWriteFormat(dest, "%sColours -\n", indent) ;
  indent = "\t\t" ;
  PRINTFULLCOLOUR(dest, colours, "Forward Strand") ;
  PRINTFULLCOLOUR(dest, strand_rev_colours, "Reverse Strand") ;
  PRINTFULLCOLOUR(dest, frame0_colours, "Frame 0") ;
  PRINTFULLCOLOUR(dest, frame1_colours, "Frame 1") ;
  PRINTFULLCOLOUR(dest, frame2_colours, "Frame 2") ;
  indent = "\t" ;

  PRINTFIELD(dest, fields_set.frame_mode, frame_mode, "3 Frame mode", "%s", zmapStyle3FrameMode2ExactStr) ;

  PRINTFIELD(dest, fields_set.min_mag, min_mag, "Min mag", "%g", (double)) ;
  PRINTFIELD(dest, fields_set.max_mag, max_mag, "Max mag", "%g", (double)) ;

  PRINTFIELD(dest, fields_set.curr_bump_mode, curr_bump_mode, "Current Bump mode", "%s", zmapStyleBumpMode2ExactStr) ;
  PRINTFIELD(dest, fields_set.default_bump_mode, default_bump_mode, "Default Bump mode", "%s", zmapStyleBumpMode2ExactStr) ;
  PRINTFIELD(dest, fields_set.bump_fixed, opts.bump_fixed, "Bump Fixed", "%s", PRINTBOOL) ;
  PRINTFIELD(dest, fields_set.bump_spacing, bump_spacing, "Bump Spacing", "%g", (double)) ;

  PRINTFIELD(dest, fields_set.width, width, "Width", "%g", (double)) ;

  PRINTFIELD(dest, fields_set.score_mode, score_mode, "Score mode", "%s", zmapStyleScoreMode2ExactStr) ;

  PRINTFIELD(dest, fields_set.min_score, min_score, "Min score", "%g", (double)) ;
  PRINTFIELD(dest, fields_set.max_score, max_score, "Max score", "%g", (double)) ;

  PRINTFIELD(dest, fields_set.gff_source, gff_source, "GFF source", "%s", g_quark_to_string) ;
  PRINTFIELD(dest, fields_set.gff_feature, gff_feature, "GFF feature", "%s", g_quark_to_string) ;

  PRINTFIELD(dest, fields_set.col_display_state, col_display_state, "Column display state", "%s", zmapStyleColDisplayState2ExactStr) ;


  PRINTFIELD(dest, fields_set.displayable, opts.displayable, "Displayable", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, fields_set.show_when_empty, opts.show_when_empty, "Show Col When Empty", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, fields_set.showText, opts.showText, "Show Text", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, fields_set.strand_specific, opts.strand_specific, "Strand Specific", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, fields_set.show_rev_strand, opts.show_rev_strand, "Show Reverse Strand", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, fields_set.directional_end, opts.directional_end, "Directional Ends", "%s", PRINTBOOL) ;


  switch(style->mode)
    {
    case ZMAPSTYLE_MODE_INVALID:
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
    case ZMAPSTYLE_MODE_TEXT:
    case ZMAPSTYLE_MODE_META:
      {
	break ;
      }
    case ZMAPSTYLE_MODE_GRAPH:
      {
	zMapOutWriteFormat(dest, "%sGlyph Mode -\n", indent) ;

	indent = "\t\t" ;
	PRINTFIELD(dest, mode_data.graph.fields_set.mode, mode_data.graph.mode, "Mode", "%s", zmapStyleGraphMode2ExactStr) ;
	PRINTFIELD(dest, mode_data.graph.fields_set.baseline, mode_data.graph.baseline, "Graph baseline", "%g", (double)) ;

	break ;
      }
    case ZMAPSTYLE_MODE_GLYPH:
      {
	zMapOutWriteFormat(dest, "%sGlyph Mode -\n", indent) ;

	indent = "\t\t" ;
	PRINTFIELD(dest, mode_data.glyph.fields_set.mode, mode_data.glyph.mode,
		   "Mode", "%s", zmapStyleGlyphMode2ExactStr) ;

	break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
	zMapOutWriteFormat(dest, "%sAlignment Mode -\n", indent) ;

	indent = "\t\t" ;
	PRINTFIELD(dest, mode_data.alignment.fields_set.parse_gaps, mode_data.alignment.state.parse_gaps,
		   "Parse Gaps", "%s", PRINTBOOL) ;
	PRINTFIELD(dest, mode_data.alignment.fields_set.show_gaps, mode_data.alignment.state.show_gaps,
		   "Show Gaps", "%s", PRINTBOOL) ;
	PRINTFIELD(dest, mode_data.alignment.fields_set.pfetchable, mode_data.alignment.state.pfetchable,
		   "Pfetchable", "%s", PRINTBOOL) ;
	PRINTFIELD(dest, mode_data.alignment.fields_set.between_align_error, mode_data.alignment.between_align_error,
		   "Join Aligns", "%d", (unsigned int)) ;

	PRINTFULLCOLOUR(dest, mode_data.alignment.perfect, "Perfect") ;
	PRINTFULLCOLOUR(dest, mode_data.alignment.perfect, "Colinear") ;
	PRINTFULLCOLOUR(dest, mode_data.alignment.perfect, "Non-colinear") ;


	break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
	zMapOutWriteFormat(dest, "%sTranscript Mode -\n", indent) ;

	indent = "\t\t" ;
	PRINTFULLCOLOUR(dest, mode_data.transcript.CDS_colours, "CDS") ;

	break ;
      }
    case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
      {
	zMapOutWriteFormat(dest, "%sAssembly_path Mode -\n", indent) ;

	indent = "\t\t" ;
	PRINTFULLCOLOUR(dest, mode_data.assembly_path.non_path_colours, "Non-assembly") ;

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }

  return ;
}



/* Returns TRUE if bumping has been fixed to one type, FALSE otherwise. */
gboolean zmapStyleBumpIsFixed(ZMapFeatureTypeStyle style)
{
  gboolean is_fixed = FALSE ;

  if (style->fields_set.bump_fixed && style->opts.bump_fixed)
    is_fixed = TRUE ;

  return is_fixed ;
}



/*! @} end of zmapstyles docs. */





static void setPrintFunc(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  StylePrint print_data = (StylePrint)user_data ;

  zMapStylePrint(print_data->dest, style, NULL, print_data->full) ;

  return ;
}


static void listPrintFunc(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  StylePrint print_data = (StylePrint)user_data ;

  zMapStylePrint(print_data->dest, style, NULL, print_data->full) ;

  return ;
}



