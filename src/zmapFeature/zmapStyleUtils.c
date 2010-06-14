/*  File: zmapStyleUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See ZMap/zmapStyle.h
 *
 * HISTORY:
 * Last edited: Jul 29 09:53 2009 (edgrif)
 * Created: Thu Oct 30 10:24:35 2008 (edgrif)
 * CVS info:   $Id: zmapStyleUtils.c,v 1.19 2010-06-14 15:40:13 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <unistd.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapEnum.h>
#include <zmapStyle_I.h>




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
  if (TEST || FLAG)					\
    zMapOutWriteFormat((DEST), "%s" FIELD_STR ":\t\t%s\t\t" FORMAT_CONV "\n", \
           INDENT_STR,                                                      \
	   (FLAG ? "<SET>" : "<UNSET>"),		\
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



static void setPrintFunc(gpointer key_id, gpointer data, gpointer user_data) ;
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
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2DrawContext,     ZMapStyleDrawContext,        ZMAPSTYLE_DRAW_INVALID, ZMAP_STYLE_DRAW_CONTEXT_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColourType,      ZMapStyleColourType,         ZMAPSTYLE_COLOURTYPE_INVALID, ZMAP_STYLE_COLOUR_TYPE_LIST, , );
//ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColourTarget,    ZMapStyleColourTarget,       ZMAPSTYLE_COLOURTARGET_INVALID, ZMAP_STYLE_COLOUR_TARGET_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ScoreMode,       ZMapStyleScoreMode,          ZMAPSCORE_INVALID, ZMAP_STYLE_SCORE_MODE_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2BumpMode,        ZMapStyleBumpMode,           ZMAPBUMP_INVALID, ZMAP_STYLE_BUMP_MODE_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2GlyphStrand,     ZMapStyleGlyphStrand,         ZMAPSTYLE_GLYPH_STRAND_INVALID, ZMAP_STYLE_GLYPH_STRAND_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2SubFeature,     ZMapStyleSubFeature,         ZMAPSTYLE_SUB_FEATURE_INVALID, ZMAP_STYLE_SUB_FEATURE_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2GlyphAlign,     ZMapStyleGlyphAlign,         ZMAPSTYLE_GLYPH_ALIGN_INVALID, ZMAP_STYLE_GLYPH_ALIGN_LIST, , );
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2BlixemType,     ZMapStyleBlixemType,         ZMAPSTYLE_BLIXEM_INVALID, ZMAP_STYLE_BLIXEM_LIST, , );



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
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleDrawContext2ExactStr,     ZMapStyleDrawContext,        ZMAP_STYLE_DRAW_CONTEXT_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleColourType2ExactStr,      ZMapStyleColourType,         ZMAP_STYLE_COLOUR_TYPE_LIST);
//ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleColourTarget2ExactStr,    ZMapStyleColourTarget,       ZMAP_STYLE_COLOUR_TARGET_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleScoreMode2ExactStr,       ZMapStyleScoreMode,          ZMAP_STYLE_SCORE_MODE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleBumpMode2ExactStr,     ZMapStyleBumpMode,        ZMAP_STYLE_BUMP_MODE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleGlyphStrand2ExactStr,     ZMapStyleGlyphStrand,        ZMAP_STYLE_GLYPH_STRAND_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleSubFeature2ExactStr,     ZMapStyleSubFeature,        ZMAP_STYLE_SUB_FEATURE_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleGlyphAlign2ExactStr,     ZMapStyleGlyphAlign,        ZMAP_STYLE_GLYPH_ALIGN_LIST);
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleBlixemType2ExactStr,     ZMapStyleBlixemType,        ZMAP_STYLE_BLIXEM_LIST);


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







void zMapStyleSetPrintAll(ZMapIOOut dest, GHashTable *type_set, char *user_string, gboolean full)
{
  StylePrintStruct print_data ;

  print_data.dest = dest ;
  print_data.full = full ;

  zMapOutWriteFormat(dest, "\nTypes at %s\n", user_string) ;

  g_hash_table_foreach(type_set, setPrintFunc, &print_data) ;

  return ;
}

void zMapStyleSetPrintAllStdOut(GHashTable *type_set, char *user_string, gboolean full)
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

  #define TEST 0
  #define INDENT_STR indent
  #define STYLE_PTR style


  zMapAssert(ZMAP_IS_FEATURE_STYLE(style)) ;

  full = TRUE ;

  zMapOutWriteFormat(dest, "%s%s Style - \"%s\" (unique id = \"%s\")\n", (prefix ? prefix : ""),
	 indent, g_quark_to_string(style->original_id), g_quark_to_string(style->unique_id)) ;

  indent = "\t" ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_PARENT_STYLE), parent_id, "Parent style", "%s", g_quark_to_string) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_DESCRIPTION), description, "Description", "%s", (char *)) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_MODE), mode, "Feature mode", "%s", zMapStyleMode2ExactStr) ;

  zMapOutWriteFormat(dest, "%sColours -\n", indent) ;
  indent = "\t\t" ;
  PRINTFULLCOLOUR(dest, colours, "Forward Strand") ;
  PRINTFULLCOLOUR(dest, strand_rev_colours, "Reverse Strand") ;
  PRINTFULLCOLOUR(dest, frame0_colours, "Frame 0") ;
  PRINTFULLCOLOUR(dest, frame1_colours, "Frame 1") ;
  PRINTFULLCOLOUR(dest, frame2_colours, "Frame 2") ;
  indent = "\t" ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_FRAME_MODE), frame_mode, "3 Frame mode", "%s", zmapStyle3FrameMode2ExactStr) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_MIN_MAG), min_mag, "Min mag", "%g", (double)) ;
  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_MAX_MAG), max_mag, "Max mag", "%g", (double)) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_BUMP_MODE), curr_bump_mode, "Current Bump mode", "%s", zmapStyleBumpMode2ExactStr) ;
  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_BUMP_DEFAULT), default_bump_mode, "Default Bump mode", "%s", zmapStyleBumpMode2ExactStr) ;
  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_BUMP_FIXED), bump_fixed, "Bump Fixed", "%s", PRINTBOOL) ;
  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_BUMP_SPACING), bump_spacing, "Bump Spacing", "%g", (double)) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_WIDTH), width, "Width", "%g", (double)) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_SCORE_MODE), score_mode, "Score mode", "%s", zmapStyleScoreMode2ExactStr) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_MIN_SCORE), min_score, "Min score", "%g", (double)) ;
  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_MAX_SCORE), max_score, "Max score", "%g", (double)) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_GFF_SOURCE), gff_source, "GFF source", "%s", g_quark_to_string) ;
  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_GFF_FEATURE), gff_feature, "GFF feature", "%s", g_quark_to_string) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_COLUMN_DISPLAY_MODE), col_display_state, "Column display state", "%s", zmapStyleColDisplayState2ExactStr) ;


  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_DISPLAYABLE), displayable, "Displayable", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_SHOW_WHEN_EMPTY), show_when_empty, "Show Col When Empty", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_SHOW_TEXT), showText, "Show Text", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_STRAND_SPECIFIC), strand_specific, "Strand Specific", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_SHOW_REV_STRAND), show_rev_strand, "Show Reverse Strand", "%s", PRINTBOOL) ;

  PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_DIRECTIONAL_ENDS), directional_end, "Directional Ends", "%s", PRINTBOOL) ;


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
	PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_GRAPH_MODE), mode_data.graph.mode, "Mode", "%s", zmapStyleGraphMode2ExactStr) ;
	PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_GRAPH_BASELINE), mode_data.graph.baseline, "Graph baseline", "%g", (double)) ;

	break ;
      }
    case ZMAPSTYLE_MODE_GLYPH:
      {

      PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_GLYPH_NAME), mode_data.glyph.glyph_name,
               "Glyph name", "%s", g_quark_to_string) ;

	break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
	zMapOutWriteFormat(dest, "%sAlignment Mode -\n", indent) ;

	indent = "\t\t" ;
	PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_ALIGNMENT_PARSE_GAPS), mode_data.alignment.parse_gaps,
		   "Parse Gaps", "%s", PRINTBOOL) ;
	PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_ALIGNMENT_SHOW_GAPS), mode_data.alignment.show_gaps,
		   "Show Gaps", "%s", PRINTBOOL) ;
	PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_ALIGNMENT_PFETCHABLE), mode_data.alignment.pfetchable,
		   "Pfetchable", "%s", PRINTBOOL) ;
	PRINTFIELD(dest, zMapStyleIsPropertySetId(style, STYLE_PROP_ALIGNMENT_BETWEEN_ERROR), mode_data.alignment.between_align_error,
		   "Join Aligns", "%d", (unsigned int)) ;

	PRINTFULLCOLOUR(dest, mode_data.alignment.perfect, "Perfect") ;
	PRINTFULLCOLOUR(dest, mode_data.alignment.colinear, "Colinear") ;
	PRINTFULLCOLOUR(dest, mode_data.alignment.noncolinear, "Non-colinear") ;

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



/*! @} end of zmapstyles docs. */





static void setPrintFunc(gpointer key_id, gpointer data, gpointer user_data)
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



