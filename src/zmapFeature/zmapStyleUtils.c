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
 * Last edited: Nov 13 09:37 2008 (edgrif)
 * Created: Thu Oct 30 10:24:35 2008 (edgrif)
 * CVS info:   $Id: zmapStyleUtils.c,v 1.1 2008-11-13 10:02:52 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapEnum.h>
#include <ZMap/zmapConfigLoader.h>
#include <zmapStyle_P.h>



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
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2Mode,            ZMapStyleMode, ZMAPSTYLE_MODE_INVALID, ZMAP_STYLE_MODE_LIST) ;
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColDisplayState, ZMapStyleColumnDisplayState, ZMAPSTYLE_COLDISPLAY_INVALID, ZMAP_STYLE_COLUMN_DISPLAY_LIST);
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr23FrameMode,      ZMapStyle3FrameMode, ZMAPSTYLE_3_FRAME_INVALID, ZMAP_STYLE_3_FRAME_LIST) ;
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2GraphMode,       ZMapStyleGraphMode,          ZMAPSTYLE_GRAPH_INVALID, ZMAP_STYLE_GRAPH_MODE_LIST);
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2GlyphMode,       ZMapStyleGlyphMode,          ZMAPSTYLE_GLYPH_INVALID, ZMAP_STYLE_GLYPH_MODE_LIST);
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2DrawContext,     ZMapStyleDrawContext,        ZMAPSTYLE_DRAW_INVALID, ZMAP_STYLE_DRAW_CONTEXT_LIST);
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColourType,      ZMapStyleColourType,         ZMAPSTYLE_COLOURTYPE_INVALID, ZMAP_STYLE_COLOUR_TYPE_LIST);
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ColourTarget,    ZMapStyleColourTarget,       ZMAPSTYLE_COLOURTARGET_INVALID, ZMAP_STYLE_COLOUR_TARGET_LIST);
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2ScoreMode,       ZMapStyleScoreMode,          ZMAPSCORE_INVALID, ZMAP_STYLE_SCORE_MODE_LIST);
ZMAP_ENUM_FROM_STRING_FUNC(zMapStyleStr2OverlapMode,     ZMapStyleOverlapMode,        ZMAPOVERLAP_INVALID, ZMAP_STYLE_OVERLAP_MODE_LIST);




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
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zmapStyleOverlapMode2ExactStr,     ZMapStyleOverlapMode,        ZMAP_STYLE_OVERLAP_MODE_LIST);








GData *zMapFeatureTypeGetFromFile(char *styles_list, char *styles_file_name)
{
  GData *styles = NULL ;
  ZMapConfigIniContext context ;
  GList *settings_list = NULL, *free_this_list = NULL;


  if ((context = zMapConfigIniContextProvideNamed(ZMAPSTANZA_STYLE_CONFIG)))
    {
      zMapConfigIniContextIncludeBuffer(context, NULL) ;

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






/*! @} end of zmapstyles docs. */
