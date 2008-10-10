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
 * Last edited: Oct 10 09:34 2008 (rds)
 * Created: Thu Sep 25 14:12:05 2008 (rds)
 * CVS info:   $Id: zmapConfigLoader.c,v 1.3 2008-10-10 08:35:06 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapConfigLoader.h>


static ZMapConfigIniContextKeyEntry get_app_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_logging_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_debug_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_source_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_window_group_data(char **stanza_name, char **stanza_type);
static ZMapConfigIniContextKeyEntry get_blixem_group_data(char **stanza_name, char **stanza_type);
static gpointer create_config_source();
static void free_source_list_item(gpointer list_data, gpointer unused_data);


ZMapConfigIniContext zMapConfigIniContextProvide()
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


GList *zMapConfigIniContextGetSources(ZMapConfigIniContext context)
{
  GList *list = NULL;

  list = zMapConfigIniContextGetReferencedStanzas(context, create_config_source,
						  ZMAPSTANZA_APP_CONFIG, 
						  ZMAPSTANZA_APP_CONFIG, 
						  "sources", "source");
  
  return list;
}

void zMapConfigSourcesFreeList(GList *config_sources_list)
{
  g_list_foreach(config_sources_list, free_source_list_item, NULL);
  g_list_free(config_sources_list);

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
  if(source_to_free->format)
    g_free(source_to_free->format);

  return ;
}

static void source_set_url(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_STRING)
    source->url = g_value_get_string(property_value);

  return ;
}

static void source_set_timeout(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_INT)
    source->timeout = g_value_get_int(property_value);

  return ;
}

static void source_set_version(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_STRING)
    source->version = g_value_get_string(property_value);

  return ;
}

static void source_set_featuresets(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_STRING)
    source->featuresets = g_value_get_string(property_value);

  return ;
}
static void source_set_navigatorsets(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_STRING)
    source->navigatorsets = g_value_get_string(property_value);

  return ;
}
static void source_set_sequence(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_BOOLEAN)
    source->sequence = g_value_get_boolean(property_value);

  return ;
}

static void source_set_writeback(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_BOOLEAN)
    source->writeback = g_value_get_boolean(property_value);

  return ;
}

static void source_set_stylesfile(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_STRING)
    source->stylesfile = g_value_get_string(property_value);

  return ;
}

static void source_set_format(gpointer parent_data, GValue *property_value)
{
  ZMapConfigSource source = (ZMapConfigSource)parent_data;

  if(G_VALUE_TYPE(property_value) == G_TYPE_STRING)
    source->format = g_value_get_string(property_value);

  return ;
}

/* This might seem a little long winded, but then so is all the gobject gubbins... */
static ZMapConfigIniContextKeyEntry get_source_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_SOURCE_URL,         G_TYPE_STRING,  source_set_url,     FALSE },
    { ZMAPSTANZA_SOURCE_TIMEOUT,     G_TYPE_INT,     source_set_timeout, FALSE },
    { ZMAPSTANZA_SOURCE_VERSION,     G_TYPE_STRING,  source_set_version, FALSE },
    { ZMAPSTANZA_SOURCE_FEATURESETS, G_TYPE_STRING,  source_set_featuresets,   FALSE },
    { "navigator_sets",              G_TYPE_STRING,  source_set_navigatorsets, FALSE },
    { "sequence",                    G_TYPE_BOOLEAN, source_set_sequence,      FALSE },
    { "writeback",                   G_TYPE_BOOLEAN, source_set_writeback,     FALSE },
    { "stylesfile",                  G_TYPE_STRING,  source_set_stylesfile,    FALSE },
    { "format",                      G_TYPE_STRING,  source_set_format,        FALSE },
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


