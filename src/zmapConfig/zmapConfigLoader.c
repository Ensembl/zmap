/*  File: zmapConfigLoader.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Holds functions for reading config stanzas for various
 *              different parts of code....ummm, not well compartmentalised.
 *
 * Exported functions: See ZMap/zmapConfigLoader.h
 *
 * HISTORY:
 * Last edited: Mar  2 14:47 2010 (edgrif)
 * Created: Thu Sep 25 14:12:05 2008 (rds)
 * CVS info:   $Id: zmapConfigLoader.c,v 1.8 2010-03-10 14:14:49 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
//#include <unistd.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapStyle.h>
#include <zmapConfigIni_P.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapConfigStanzaStructs.h>



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
static gint match_type(gconstpointer list_data, gconstpointer user_data);

typedef struct
{
  char *stanza_name;
  char *stanza_type;
  GList *keys;
}ZMapConfigIniContextStanzaEntryStruct, *ZMapConfigIniContextStanzaEntry;


typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;
  gboolean result;
} CheckRequiredKeysStruct, *CheckRequiredKeys;



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


      if (g_ascii_strcasecmp(stanza_name_in, ZMAPSTANZA_SOURCE_CONFIG) == 0)
	{
	  if((stanza_group = get_source_group_data(&stanza_name, &stanza_type)))
	    zMapConfigIniContextAddGroup(context, stanza_name,
					 stanza_type, stanza_group);
      }

      if (g_ascii_strcasecmp(stanza_name_in, ZMAPSTANZA_STYLE_CONFIG) == 0)
      {
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
						      ZMAPSTANZA_APP_CONFIG,
						      ZMAPSTANZA_APP_CONFIG,
						      "styles", "style") ;
    }

  return list ;
}

GList *zMapConfigIniContextGetStyleList(ZMapConfigIniContext context,char *styles_list)
{
  GList *list = NULL;

  if(styles_list)
      list = zMapConfigIniContextGetListedStanzas(context, create_config_style,styles_list,"style"); // get the named stanzas
  else
      list = zMapConfigIniContextGetNamedStanzas(context, create_config_style,"style") ; // get all the stanzas
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



/*
 *                    Context internals
 */


/* here we want to prefer exact matches of name and type */
/* then we'll only match types with a name == * already in the list */
static gint match_type(gconstpointer list_data, gconstpointer user_data)
{
  ZMapConfigIniContextStanzaEntry query = (ZMapConfigIniContextStanzaEntry)user_data;
  ZMapConfigIniContextStanzaEntry list_entry = (ZMapConfigIniContextStanzaEntry)list_data;
  gint result = -1;

  /* not an exact name match, do the types match? */
  if(g_ascii_strcasecmp( query->stanza_type, list_entry->stanza_type ) == 0)
    {
      /* make sure the stanza_name == * in the list the context
       * has. This is so stanzas with different names can have the
       * same basic type. e.g. the same list of keys/info/G_TYPE data
       * for keys... */
      if(g_ascii_strcasecmp(list_entry->stanza_name, "*") == 0)
      {
        query->keys = list_entry->keys;
        result = 0;           /* zero == found */
      }
      else
      result = 1;
    }
  else
    result = -1;

  return result;
}



static ZMapConfigIniContextStanzaEntry get_stanza_with_type(ZMapConfigIniContext context,
                                              char *stanza_type)
{
  ZMapConfigIniContextStanzaEntryStruct user_request = {NULL};
  ZMapConfigIniContextStanzaEntry stanza = NULL;
  GList *entry_found;

  user_request.stanza_name = "*";
  user_request.stanza_type = stanza_type;

  if((entry_found = g_list_find_custom(context->groups, &user_request, match_type)))
    {
      stanza = (ZMapConfigIniContextStanzaEntry)(entry_found->data);
    }

  return stanza;
}




/* Context Config Loading */
typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;

  char *current_stanza_name;

  ZMapConfigIniUserDataCreateFunc object_create_func;
  gpointer current_object;

  GList *object_list_out;
} FetchReferencedStanzasStruct, *FetchReferencedStanzas;

static void fill_stanza_key_value(gpointer list_data, gpointer user_data)
{
  FetchReferencedStanzas full_data = (FetchReferencedStanzas)user_data;
  ZMapConfigIniContextKeyEntry key = (ZMapConfigIniContextKeyEntry)list_data;
  GValue *value = NULL;

  if (zMapConfigIniGetValue(full_data->context->config,
                      full_data->current_stanza_name,
                      key->key,
                      &value,
                      key->type))
    {
      if (key->set_property)
      (key->set_property)(full_data->current_stanza_name, key->key, key->type, full_data->current_object, value);
      else
      zMapLogWarning("No set_property function for key '%s'", key->key);

      g_free(value);
    }

  return ;
}

static void fetch_referenced_stanzas(gpointer list_data, gpointer user_data)
{
  FetchReferencedStanzas full_data = (FetchReferencedStanzas)user_data;
  char *stanza_name = (char *)list_data;

  full_data->current_stanza_name = stanza_name;

  if (zMapConfigIniHasStanza(full_data->context->config, stanza_name) && (full_data->object_create_func))
    {
      if ((full_data->current_object = (full_data->object_create_func)()))
      {
        /* get stanza keys */
        g_list_foreach(full_data->stanza->keys, fill_stanza_key_value, user_data);

        full_data->object_list_out = g_list_append(full_data->object_list_out,
                                         full_data->current_object);
      }
      else
      zMapLogWarning("Object Create Function for stanza '%s'"
                   " failed to return anything", stanza_name);
    }

  return ;
}

GList *get_child_stanza_names_as_list(ZMapConfigIniContext context,
                              char *parent_name,
                              char *parent_key)
{
  GList *list = NULL;
  GValue *value = NULL;

  if (parent_name && parent_key &&  zMapConfigIniGetValue(context->config,
                                            parent_name, parent_key,
                                            &value, G_TYPE_POINTER))
    {
      char **strings_list = NULL;

      if ((strings_list = g_value_get_pointer(value)))
      {
        char **ptr = strings_list;

        while (ptr && *ptr)
          {
            *ptr = g_strstrip(*ptr) ;
            list = g_list_append(list, *ptr) ;
            ptr++ ;
          }

        g_free(strings_list);
      }
    }

  return list;
}

static void free_source_names(gpointer list_data, gpointer unused_user_data)
{
  g_free(list_data);
  return ;
}

#if NOT_USED
GList *zMapConfigIniContextGetStanza(ZMapConfigIniContext context,
                             ZMapConfigIniUserDataCreateFunc create,
                             char *stanza_name, char *stanza_type)
{
  GList *list = NULL;

  if(strcmp(stanza_name, stanza_type) == 0)
    list = zMapConfigIniContextGetReferencedStanzas(context, create,
                                        NULL, NULL,
                                        NULL, stanza_type);

  return list;
}

#endif

GList *zMapConfigIniContextGetReferencedStanzas(ZMapConfigIniContext context,
                                    ZMapConfigIniUserDataCreateFunc object_create_func,
                                    char *parent_name,
                                    char *parent_type,
                                    char *parent_key,
                                    char *child_type)
{
  FetchReferencedStanzasStruct data = {NULL};
  GList *sources      = NULL;
  GList *source_names = NULL;

  data.context = context;
  data.stanza  = get_stanza_with_type(context, child_type);
  data.object_create_func = object_create_func;

  source_names = get_child_stanza_names_as_list(context,
                                    parent_name,
                                    parent_key);

  if(!source_names)
    source_names = g_list_append(source_names, g_strdup(child_type));

  g_list_foreach(source_names, fetch_referenced_stanzas, &data);

  g_list_foreach(source_names, free_source_names, NULL);
  g_list_free(source_names);

  sources = data.object_list_out;

  return sources;
}




GList *get_names_as_list(char *styles)
{
  GList *list = NULL;

  char **strings_list = NULL;

  if ((strings_list = g_strsplit(styles,";",0)))
  {
    char **ptr = strings_list;

    while (ptr && *ptr)
      {
       *ptr = g_strstrip(*ptr) ;
       list = g_list_append(list, *ptr);
       ptr++ ;
      }

    g_free(strings_list);
  }
  return list;
}


/* see zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name)
 * in zmapConfigIni.c, which is a wrapper for GetReferencedStanzas() above
 * For a list of styles in a source stanza we can't get these easily as we don't know the name
 * of the source stanza, and GetNamed hard codes 'source' as the name of the parent.
 * so we find each stanza by name from the list and add to our return list.
 */

GList *zMapConfigIniContextGetListedStanzas(ZMapConfigIniContext context,
                                    ZMapConfigIniUserDataCreateFunc object_create_func,
                                    char *styles_list,char *child_type)
{
  FetchReferencedStanzasStruct data = {NULL};
  GList *styles      = NULL;
  GList *style_names = NULL;

  data.context = context;
  data.stanza  = get_stanza_with_type(context, child_type);
  data.object_create_func = object_create_func;

  style_names = get_names_as_list(styles_list);

  zMapAssert(style_names) ;

  g_list_foreach(style_names, fetch_referenced_stanzas, &data);

//  g_list_foreach(style_names, free_source_names, NULL);
  g_list_free(style_names);

  styles = data.object_list_out;

  return styles;
}


/* return list of all stanzas in context->extra_key_file */
/* used when we don't have a styles list */
GList *zMapConfigIniContextGetNamedStanzas(ZMapConfigIniContext context,
                                    ZMapConfigIniUserDataCreateFunc object_create_func,
                                    char *stanza_type)
{
  FetchReferencedStanzasStruct data = {NULL};
  GList *styles      = NULL;
  gchar **names;
  gchar **np;
  data.context = context;
  data.stanza  = get_stanza_with_type(context, stanza_type);
  data.object_create_func = object_create_func;

  names = zMapConfigIniContextGetAllStanzaNames(context);

  for(np = names;*np;np++)
  {
      fetch_referenced_stanzas((gpointer) *np,&data);
  }

  g_strfreev(names);

  styles = data.object_list_out;

  return styles;
}



// get style stanzas in styles_list of all from the file
gboolean zMapConfigIniGetStylesFromFile(char *styles_list, char *styles_file, GData **styles_out)
{
  gboolean result = FALSE ;
  GData *styles = NULL ;
  ZMapConfigIniContext context ;
  GList *settings_list = NULL, *free_this_list = NULL;

  if ((context = zMapConfigIniContextProvideNamed(ZMAPSTANZA_STYLE_CONFIG)))
    {
      if (zMapConfigIniContextIncludeFile(context,styles_file))
      {
        settings_list = zMapConfigIniContextGetStyleList(context,styles_list);
        zMapConfigIniContextDestroy(context) ;
        context = NULL;
      }
    }

  if (settings_list)
    {
      free_this_list = settings_list ;

      g_datalist_init(&styles) ;

      do
      {
        ZMapKeyValue curr_config_style ;
        ZMapFeatureTypeStyle new_style ;
        GParameter params[100] ;                    /* TMP....make dynamic.... */
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
                         styles_file, curr_config_style->name) ;
            }
          }

      } while((settings_list = g_list_next(settings_list)));

    }

  zMapConfigStylesFreeList(free_this_list) ;

  if(styles)
  {
      *styles_out = styles ;
      result = TRUE ;
  }

  return result ;
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
    { ZMAPSTANZA_APP_SCRIPTS,      G_TYPE_STRING, NULL, FALSE },
    { ZMAPSTANZA_APP_DATA,         G_TYPE_STRING, NULL, FALSE },
    { ZMAPSTANZA_APP_STYLESFILE,   G_TYPE_STRING, NULL, FALSE },
    { ZMAPSTANZA_APP_XREMOTE_DEBUG,G_TYPE_BOOLEAN, NULL, FALSE },
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
    { ZMAPSTANZA_LOG_LOGGING,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_FILE,      G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_SHOW_CODE, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_DIRECTORY, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_LOG_FILENAME,  G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_LOG_SHOW_CODE, G_TYPE_STRING,  NULL, FALSE },
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
    { ZMAPSTANZA_DEBUG_APP_FEATURE2STYLE, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_DEBUG_APP_STYLES, G_TYPE_BOOLEAN, NULL, FALSE },
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
      { ZMAPSTYLE_PROPERTY_PARENT_STYLE, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleCreateID} },
      { ZMAPSTYLE_PROPERTY_DESCRIPTION, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
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
      { ZMAPSTYLE_PROPERTY_FRAME_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr23FrameMode} },

      { ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_DEFERRED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_LOADED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GRAPH_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GraphMode} },
      { ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,   FALSE, ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GLYPH_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphMode} },
      { ZMAPSTYLE_PROPERTY_GLYPH_TYPE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphType} },

      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphType} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

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
    { ZMAPSTYLE_PROPERTY_GLYPH_TYPE,   G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },

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
    { ZMAPSTANZA_SOURCE_STYLES,        G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_NAVIGATORSETS, G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_SEQUENCE,      G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_WRITEBACK,     G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_FORMAT,        G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_DELAYED,       G_TYPE_BOOLEAN, source_set_property, FALSE },
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
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_DELAYED) == 0)
      bool_ptr = &(config_source->delayed) ;

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


