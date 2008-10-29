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
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct 29 14:52 2008 (edgrif)
 * Created: Thu Sep 25 14:12:05 2008 (rds)
 * CVS info:   $Id: zmapConfigLoader.c,v 1.5 2008-10-29 16:08:18 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
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
 * ZMapConfigStyle
 */

static gpointer create_config_style()
{
  return g_new0(ZMapConfigStyleStruct, 1);
}

static void free_style_list_item(gpointer list_data, gpointer unused_data) 
{
  ZMapConfigStyle style_to_free = (ZMapConfigStyle)list_data;

  if(style_to_free->name)
    g_free(style_to_free->name) ;

  if(style_to_free->description)
    g_free(style_to_free->description) ;

  if(style_to_free->mode)
    g_free(style_to_free->mode) ;

  if(style_to_free->border)
    g_free(style_to_free->border) ;
  if(style_to_free->fill)
    g_free(style_to_free->fill) ;
  if(style_to_free->draw)
    g_free(style_to_free->draw) ;

  if(style_to_free->overlap_mode)
    g_free(style_to_free->overlap_mode) ;

  return ;
}


/* This might seem a little long winded, but then so is all the gobject gubbins... */
static ZMapConfigIniContextKeyEntry get_style_group_data(char **stanza_name, char **stanza_type)
{
  static char *name = "*";
  static char *type = ZMAPSTANZA_STYLE_CONFIG;
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_STYLE_DESCRIPTION, G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTANZA_STYLE_MODE, G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTANZA_STYLE_BORDER, G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTANZA_STYLE_FILL, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTANZA_STYLE_DRAW, G_TYPE_STRING,  style_set_property, FALSE },

    { ZMAPSTANZA_STYLE_WIDTH, G_TYPE_DOUBLE,  style_set_property, FALSE },

    { ZMAPSTANZA_STYLE_STRAND,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTANZA_STYLE_REVERSE,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTANZA_STYLE_FRAME,   G_TYPE_BOOLEAN, style_set_property, FALSE },

    { ZMAPSTANZA_STYLE_NODISPLAY,   G_TYPE_BOOLEAN, style_set_property, FALSE },

    { ZMAPSTANZA_STYLE_MINMAG, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTANZA_STYLE_MAXMAG, G_TYPE_DOUBLE,  style_set_property, FALSE },

    { ZMAPSTANZA_STYLE_OVERLAPMODE, G_TYPE_STRING,  style_set_property, FALSE },

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    { ZMAPSTANZA_STYLE_ALIGN,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTANZA_STYLE_READ_GAPS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    { ZMAPSTANZA_STYLE_INIT_HIDDEN,   G_TYPE_BOOLEAN, style_set_property, FALSE },

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
  ZMapConfigStyle config_style = (ZMapConfigStyle)parent_data ;
  gboolean *bool_ptr ;
  int *int_ptr ;
  double *double_ptr ;
  char **str_ptr ;

  /* Odd case, name of style is actually name of stanza so must record separately like this. */
  if (!(config_style->name))
    {
      config_style->name = g_strdup(current_stanza_name) ;
      config_style->fields_set.name = TRUE ;
    }

  if (key && *key)
    {
      if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_DESCRIPTION) == 0)
	{
	  str_ptr = &(config_style->description) ;
	  config_style->fields_set.description = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_MODE) == 0)
	{
	  str_ptr = &(config_style->mode) ;
	  config_style->fields_set.mode = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_BORDER) == 0)
	{
	  str_ptr = &(config_style->border) ;
	  config_style->fields_set.border = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_DRAW) == 0)
	{
	  str_ptr = &(config_style->draw) ;
	  config_style->fields_set.draw = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_FILL) == 0)
	{
	  str_ptr = &(config_style->fill) ;
	  config_style->fields_set.fill = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_OVERLAPMODE) == 0)
	{
	  str_ptr = &(config_style->overlap_mode) ;
	  config_style->fields_set.overlap_mode = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_WIDTH) == 0)
	{
	  double_ptr = &(config_style->width) ;
	  config_style->fields_set.width = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_STRAND) == 0)
	{
	  bool_ptr = &(config_style->strand_specific) ;
	  config_style->fields_set.strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_REVERSE) == 0)
	{
	  bool_ptr = &(config_style->show_reverse_strand) ;
	  config_style->fields_set.show_reverse_strand = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_FRAME) == 0)
	{
	  bool_ptr = &(config_style->frame_specific) ;
	  config_style->fields_set.frame_specific = TRUE ;
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_MINMAG) == 0)
	{
	  double_ptr = &(config_style->min_mag) ;
	  config_style->fields_set.min_mag = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_MAXMAG) == 0)
	{
	  double_ptr = &(config_style->max_mag) ;
	  config_style->fields_set.max_mag = TRUE ;
	}
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_STYLE_INIT_HIDDEN) == 0)
	{
	  bool_ptr = &(config_style->init_hidden) ;
	  config_style->fields_set.init_hidden = TRUE ;
	}
  
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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
    { ZMAPSTANZA_SOURCE_URL,         G_TYPE_STRING,  source_set_property,     FALSE },
    { ZMAPSTANZA_SOURCE_TIMEOUT,     G_TYPE_INT,     source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_VERSION,     G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_FEATURESETS, G_TYPE_STRING,  source_set_property,   FALSE },
    { ZMAPSTANZA_SOURCE_STYLESFILE,  G_TYPE_STRING,  source_set_property,    FALSE },
    { ZMAPSTANZA_SOURCE_STYLES,      G_TYPE_STRING,  source_set_property,    FALSE },
    { "navigator_sets",              G_TYPE_STRING,  source_set_property, FALSE },
    { "sequence",                    G_TYPE_BOOLEAN, source_set_property,      FALSE },
    { "writeback",                   G_TYPE_BOOLEAN, source_set_property,     FALSE },
    { "format",                      G_TYPE_STRING,  source_set_property,        FALSE },
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
      else if (g_ascii_strcasecmp(key, "navigator_sets") == 0)
	str_ptr = &(config_source->navigatorsets) ;
      else if (g_ascii_strcasecmp(key, "format") == 0)
	str_ptr = &(config_source->format) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_TIMEOUT) == 0)
	int_ptr = &(config_source->timeout) ;
      else if (g_ascii_strcasecmp(key, "sequence") == 0)
	bool_ptr = &(config_source->sequence) ;
      else if (g_ascii_strcasecmp(key, "writeback") == 0)
	bool_ptr = &(config_source->writeback) ;

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


