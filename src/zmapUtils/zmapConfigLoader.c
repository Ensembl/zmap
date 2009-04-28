/*  File: zmapConfigLoader.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * Description: Holds functions for reading config stanzas for various
 *              different parts of code....ummm, not well compartmentalised.
 *
 * Exported functions: See ZMap/zmapConfigLoader.h
 *              
 * HISTORY:
 * Last edited: Apr 27 11:41 2009 (edgrif)
 * Created: Thu Sep 25 14:12:05 2008 (rds)
 * CVS info:   $Id: zmapConfigLoader.c,v 1.8 2009-04-28 14:31:05 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapStyle.h>
#include <ZMap/zmapConfigLoader.h>


static ZMapConfigIniContextKeyEntry get_app_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_logging_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_debug_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_style_group_data(char **stanza_name, char **stanza_type) ;
static ZMapConfigIniContextKeyEntry get_source_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_window_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_blixem_group_data(char **stanza_name, char **stanza_type);
static gpointer create_config_source();
static void free_source_list_item(gpointer list_data, gpointer unused_data);
static void source_set_property(char *current_stanza_name, char *key, GType type,
				gpointer parent_data, GValue *property_value) ;

static gpointer create_config_style() ;
static void style_set_property(char *current_stanza_name, char *key, GType type,
			       gpointer parent_data, GValue *property_value) ;
static void free_style_list_item(gpointer list_data, gpointer unused_data)  ;





ZMapConfigIniContext zMapConfigIniContextProvide(void)
{
  ZMapConfigIniContext context = NULL;
  
  if((context = zMapConfigIniContextCreate()))
    {
      ZMapConfigIniContextKeyEntry stanza_group = NULL;
      char *stanza_name, *stanza_type;
      
      if((stanza_group = get_logging_group_data(&stanza_name, &stanza_type)))
	zMapConfigIniContextAddGroup(context, stanza_name, 
				     stanza_type, stanza_group);

      if((stanza_group = get_app_group_data(&stanza_name, &stanza_type)))
	zMapConfigIniContextAddGroup(context, stanza_name, 
				     stanza_type, stanza_group);

      if((stanza_group = get_debug_group_data(&stanza_name, &stanza_type)))
	zMapConfigIniContextAddGroup(context, stanza_name, 
				     stanza_type, stanza_group);

      if((stanza_group = get_source_group_data(&stanza_name, &stanza_type)))
	zMapConfigIniContextAddGroup(context, stanza_name, 
				     stanza_type, stanza_group);

      if((stanza_group = get_window_group_data(&stanza_name, &stanza_type)))
	zMapConfigIniContextAddGroup(context, stanza_name, 
				     stanza_type, stanza_group);

      if((stanza_group = get_blixem_group_data(&stanza_name, &stanza_type)))
	zMapConfigIniContextAddGroup(context, stanza_name, 
				     stanza_type, stanza_group);
    }
  
  return context;
}


ZMapConfigIniContext zMapConfigIniContextProvideNamed(char *stanza_name_in)
{
  ZMapConfigIniContext context = NULL;
  
  zMapAssert(stanza_name_in && *stanza_name_in) ;

  if ((context = zMapConfigIniContextCreate()))
    {
      ZMapConfigIniContextKeyEntry stanza_group = NULL;
      char *stanza_name, *stanza_type;


      if (g_ascii_strcasecmp(stanza_name_in, ZMAPSTANZA_STYLE_CONFIG) == 0)
	{
	  if((stanza_group = get_source_group_data(&stanza_name, &stanza_type)))
	    zMapConfigIniContextAddGroup(context, stanza_name, 
					 stanza_type, stanza_group);

	  if ((stanza_group = get_style_group_data(&stanza_name, &stanza_type)))
	    zMapConfigIniContextAddGroup(context, stanza_name, 
					 stanza_type, stanza_group) ;
	}

      /* OK...NOW ADD THE OTHERS... */
    }


  return context;
}



GList *zMapConfigIniContextGetSources(ZMapConfigIniContext context)
{
  GList *list = NULL;

  list = zMapConfigIniContextGetReferencedStanzas(context, create_config_source,
						  ZMAPSTANZA_APP_CONFIG, 
						  ZMAPSTANZA_APP_CONFIG, 
						  "sources", "source");
  
  return list;
}

GList *zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name)
{
  GList *list = NULL;

  zMapAssert(stanza_name && *stanza_name) ;

  if (g_ascii_strcasecmp(stanza_name, ZMAPSTANZA_STYLE_CONFIG) == 0)
    {
      list = zMapConfigIniContextGetReferencedStanzas(context, create_config_style,
						      ZMAPSTANZA_SOURCE_CONFIG, 
						      ZMAPSTANZA_SOURCE_CONFIG, 
						      "styles", "style") ;
    }
  
  return list ;
}


void zMapConfigSourcesFreeList(GList *config_sources_list)
{
  g_list_foreach(config_sources_list, free_source_list_item, NULL);
  g_list_free(config_sources_list);

  return ;
}

void zMapConfigStylesFreeList(GList *config_styles_list)
{
  g_list_foreach(config_styles_list, free_style_list_item, NULL);
  g_list_free(config_styles_list);

  return ;
}



static ZMapConfigIniContextKeyEntry get_app_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_APP_MAINWINDOW,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_SEQUENCE,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_EXIT_TIMEOUT, G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_HELP_URL,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_LOCALE,       G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_COOKIE_JAR,   G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_PFETCH_MODE,  G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_PFETCH_LOCATION, G_TYPE_STRING, NULL, FALSE },
    {NULL}
  };
  static char *name = ZMAPSTANZA_APP_CONFIG;
  static char *type = ZMAPSTANZA_APP_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}

static ZMapConfigIniContextKeyEntry get_logging_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_LOG_LOGGING,   G_TYPE_BOOLEAN, FALSE },
    { ZMAPSTANZA_LOG_FILE,      G_TYPE_BOOLEAN, FALSE },
    { ZMAPSTANZA_LOG_SHOW_CODE, G_TYPE_BOOLEAN, FALSE },
    { ZMAPSTANZA_LOG_DIRECTORY, G_TYPE_STRING,  FALSE },
    { ZMAPSTANZA_LOG_FILENAME,  G_TYPE_STRING,  FALSE },
    { ZMAPSTANZA_LOG_SHOW_CODE, G_TYPE_STRING,  FALSE },
    {NULL}
  };
  static char *name = ZMAPSTANZA_LOG_CONFIG;
  static char *type = ZMAPSTANZA_LOG_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}

static ZMapConfigIniContextKeyEntry get_debug_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_DEBUG_APP_THREADS, G_TYPE_BOOLEAN, NULL, FALSE },
    { NULL }
  };
  static char *name = ZMAPSTANZA_DEBUG_CONFIG;
  static char *type = ZMAPSTANZA_DEBUG_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}



/* 
 *                 ZMapConfigStyle
 */


static gpointer create_config_style()
{
  gpointer config_struct = NULL ;

  ZMapKeyValueStruct style_conf[] =
    {
      { ZMAPSTYLE_PROPERTY_NAME, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_DESCRIPTION, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_PARENT_STYLE, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MODE, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2Mode} },

      { ZMAPSTYLE_PROPERTY_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME0_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME1_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME2_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_REV_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

      { ZMAPSTYLE_PROPERTY_BUMP_MODE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2BumpMode} },
      { ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2BumpMode} },
      { ZMAPSTYLE_PROPERTY_BUMP_SPACING, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_MIN_MAG, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MAX_MAG, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_WIDTH, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_SCORE_MODE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2ScoreMode} },
      { ZMAPSTYLE_PROPERTY_MIN_SCORE, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MAX_SCORE, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GFF_SOURCE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GFF_FEATURE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_DISPLAYABLE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SHOW_TEXT,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_DEFERRED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_LOADED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GRAPH_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GraphMode} },
      { ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,   FALSE, ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GLYPH_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphMode} },

      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALIGN_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_WITHIN_ERROR,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_BETWEEN_ERROR,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

      { ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS,  FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

      {NULL}
    } ;


  int dummy ;

  dummy = sizeof(style_conf) ;


  /* Needs to return a block copy of this.... */
  config_struct = g_memdup(style_conf, sizeof(style_conf)) ;


  return config_struct ;
}


static void free_style_list_item(gpointer list_data, gpointer unused_data) 
{
  ZMapKeyValue style_conf = (ZMapKeyValue)list_data ;

  while (style_conf->name != NULL)
    {
      switch (style_conf->type)
	{
	case ZMAPCONF_STR:
	  if (style_conf->has_value)
	    g_free(style_conf->data.str) ;
	  break ;
	case ZMAPCONF_STR_ARRAY:
	  if (style_conf->has_value)
	    g_strfreev(style_conf->data.str_array) ;
	  break ;
	default:
	  break ;
	}

      style_conf++ ;
    }

  g_free(list_data) ;

  return ;
}


/* This might seem a little long winded, but then so is all the gobject gubbins... */
static ZMapConfigIniContextKeyEntry get_style_group_data(char **stanza_name, char **stanza_type)
{
  static char *name = "*";
  static char *type = ZMAPSTANZA_STYLE_CONFIG;
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTYLE_PROPERTY_NAME, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_DESCRIPTION, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_PARENT_STYLE, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MODE, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_COLOURS, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FRAME0_COLOURS, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FRAME1_COLOURS, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FRAME2_COLOURS, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_REV_COLOURS, G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_BUMP_MODE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_BUMP_SPACING, G_TYPE_DOUBLE,  style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_MIN_MAG, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MAX_MAG, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_WIDTH, G_TYPE_DOUBLE,  style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_SCORE_MODE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MIN_SCORE, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MAX_SCORE, G_TYPE_DOUBLE,  style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_GFF_SOURCE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GFF_FEATURE, G_TYPE_STRING,  style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_DISPLAYABLE,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SHOW_TEXT,   G_TYPE_BOOLEAN, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC ,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FRAME_MODE,   G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,   G_TYPE_BOOLEAN, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_DEFERRED,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_LOADED,   G_TYPE_BOOLEAN, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_GRAPH_MODE,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,   G_TYPE_DOUBLE, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_GLYPH_MODE,   G_TYPE_STRING, style_set_property, FALSE },


    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALIGN_GAPS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_WITHIN_ERROR,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_BETWEEN_ERROR,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS,  G_TYPE_STRING, style_set_property, FALSE },


    {NULL}
  };


  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}


/* We can be called with key == NULL if the stanza is empty. */
static void style_set_property(char *current_stanza_name, char *key, GType type,
			       gpointer parent_data, GValue *property_value)
{
  ZMapKeyValue config_style = (ZMapKeyValue)parent_data ;

  /* Odd case, name of style is actually name of stanza so must record separately like this. */
  if (g_ascii_strcasecmp(config_style->name, ZMAPSTYLE_PROPERTY_NAME) == 0 && !(config_style->has_value))
    {
      config_style->has_value = TRUE ;
      config_style->data.str = g_strdup(current_stanza_name) ;
    }


  if (key && *key)
    {
      ZMapKeyValue curr_key = config_style ;

      while (curr_key->name)
	{
	  if (g_ascii_strcasecmp(key, curr_key->name) == 0)
	    {
	      curr_key->has_value = TRUE ;

	      if (type == G_TYPE_BOOLEAN && G_VALUE_TYPE(property_value) == type)
		curr_key->data.b = g_value_get_boolean(property_value) ;
	      else if (type == G_TYPE_INT && G_VALUE_TYPE(property_value) == type)
		curr_key->data.i = g_value_get_int(property_value);
	      else if (type == G_TYPE_DOUBLE && G_VALUE_TYPE(property_value) == type)
		curr_key->data.d = g_value_get_double(property_value);
	      else if (type == G_TYPE_STRING && G_VALUE_TYPE(property_value) == type)
		curr_key->data.str = (char *)g_value_get_string(property_value);
	      else if (type == G_TYPE_POINTER && G_VALUE_TYPE(property_value) == type)
		curr_key->data.str_array = (char **)g_value_get_pointer(property_value);

	      break ;
	    }
	  else
	    {
	      curr_key++ ;
	    }
	}
    }

  return ;
}





/* 
 * ZMapConfigSource
 */

static gpointer create_config_source()
{
  return g_new0(ZMapConfigSourceStruct, 1);
}

static void free_source_list_item(gpointer list_data, gpointer unused_data) 
{
  ZMapConfigSource source_to_free = (ZMapConfigSource)list_data;

  if(source_to_free->url)
    g_free(source_to_free->url);
  if(source_to_free->version)
    g_free(source_to_free->version);
  if(source_to_free->featuresets)
    g_free(source_to_free->featuresets);
  if(source_to_free->navigatorsets)
    g_free(source_to_free->navigatorsets);
  if(source_to_free->stylesfile)
    g_free(source_to_free->stylesfile);
  if(source_to_free->styles_list)
    g_free(source_to_free->styles_list);
  if(source_to_free->format)
    g_free(source_to_free->format);

  return ;
}


/* This might seem a little long winded, but then so is all the gobject gubbins... */
static ZMapConfigIniContextKeyEntry get_source_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_SOURCE_URL,           G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_TIMEOUT,       G_TYPE_INT,     source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_VERSION,       G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_FEATURESETS,   G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_STYLESFILE,    G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_STYLES,        G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_NAVIGATORSETS, G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_SEQUENCE,      G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_WRITEBACK,     G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_FORMAT,        G_TYPE_STRING,  source_set_property, FALSE },
    {NULL}
  };

  static char *name = "*";
  static char *type = ZMAPSTANZA_SOURCE_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}


static void source_set_property(char *current_stanza_name, char *key, GType type,
				gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource config_source = (ZMapConfigSource)parent_data ;
  gboolean *bool_ptr ;
  int *int_ptr ;
  double *double_ptr ;
  char **str_ptr ;

  if (key && *key)
    {
      if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_URL) == 0)
	str_ptr = &(config_source->url) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_VERSION) == 0)
	str_ptr = &(config_source->version) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_FEATURESETS) == 0)
	str_ptr = &(config_source->featuresets) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_STYLESFILE) == 0)
	str_ptr = &(config_source->stylesfile) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_STYLES) == 0)
	str_ptr = &(config_source->styles_list) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_NAVIGATORSETS) == 0)
	str_ptr = &(config_source->navigatorsets) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_TIMEOUT) == 0)
	int_ptr = &(config_source->timeout) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_SEQUENCE) == 0)
	bool_ptr = &(config_source->sequence) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_WRITEBACK) == 0)
	bool_ptr = &(config_source->writeback) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_FORMAT) == 0)
	str_ptr = &(config_source->format) ;

      if (type == G_TYPE_BOOLEAN && G_VALUE_TYPE(property_value) == type)
	*bool_ptr = g_value_get_boolean(property_value);
      else if (type == G_TYPE_INT && G_VALUE_TYPE(property_value) == type)
	*int_ptr = g_value_get_int(property_value);
      else if (type == G_TYPE_DOUBLE && G_VALUE_TYPE(property_value) == type)
	*double_ptr = g_value_get_double(property_value);
      else if (type == G_TYPE_STRING && G_VALUE_TYPE(property_value) == type)
	*str_ptr = (char *)g_value_get_string(property_value);
    }

  return ;
}



/* ZMapWindow */

static ZMapConfigIniContextKeyEntry get_window_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_WINDOW_MAXSIZE,      G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_MAXBASES,     G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_COLUMNS,      G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_WINDOW_FWD_COORDS,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_WINDOW_SHOW_3F_REV,  G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_WINDOW_A_SPACING,    G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_B_SPACING,    G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_S_SPACING,    G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_C_SPACING,    G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_F_SPACING,    G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_LINE_WIDTH,   G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_WINDOW_ROOT,         G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_ALIGNMENT,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_BLOCK,        G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_M_FORWARD,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_M_REVERSE,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_Q_FORWARD,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_Q_REVERSE,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_M_FORWARDCOL, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_M_REVERSECOL, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_Q_FORWARDCOL, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_Q_REVERSECOL, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_COL_HIGH,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_SEPARATOR,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_COL_HIGH,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_ITEM_MARK,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_ITEM_HIGH,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_FRAME_0,      G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_FRAME_1,      G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_FRAME_2,      G_TYPE_STRING,  NULL, FALSE },
    { NULL }
  };
  static char *name = ZMAPSTANZA_WINDOW_CONFIG;
  static char *type = ZMAPSTANZA_WINDOW_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}



static ZMapConfigIniContextKeyEntry get_blixem_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_BLIXEM_NETID,      G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_PORT,       G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_SCRIPT,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_CONF_FILE,  G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_SCOPE,      G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_MAX,        G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_KEEP_TEMP,  G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_KILL_EXIT,  G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_DNA_FS,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_PROT_FS,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_TRANS_FS,   G_TYPE_STRING,  NULL, FALSE },
    { NULL }
  };
  static char *name = ZMAPSTANZA_BLIXEM_CONFIG;
  static char *type = ZMAPSTANZA_BLIXEM_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}


