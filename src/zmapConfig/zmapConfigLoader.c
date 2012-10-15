/*  File: zmapConfigLoader.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Holds functions for reading config stanzas for various
 *              different parts of code....ummm, not well compartmentalised.
 *
 * Exported functions: See ZMap/zmapConfigLoader.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
//#include <unistd.h>
#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapStyle.h>
#include <zmapConfigIni_P.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapConfigStanzaStructs.h>
#include <ZMap/zmapFeature.h>


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



ZMapConfigIniContext zMapConfigIniContextProvide(char *config_file)
{
  ZMapConfigIniContext context = NULL;

  if((context = zMapConfigIniContextCreate(config_file)))
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




ZMapConfigIniContext zMapConfigIniContextProvideNamed(char *config_file, char *stanza_name_in)
{
  ZMapConfigIniContext context = NULL;

  zMapAssert(stanza_name_in && *stanza_name_in) ;

  if ((context = zMapConfigIniContextCreate(config_file)))
    {
      ZMapConfigIniContextKeyEntry stanza_group = NULL;
      char *stanza_name, *stanza_type;


      if (g_ascii_strcasecmp(stanza_name_in, ZMAPSTANZA_SOURCE_CONFIG) == 0)        // unused
	{
	  if((stanza_group = get_source_group_data(&stanza_name, &stanza_type)))
	    zMapConfigIniContextAddGroup(context, stanza_name,
					 stanza_type, stanza_group);
      }

      if (g_ascii_strcasecmp(stanza_name_in, ZMAPSTANZA_STYLE_CONFIG) == 0)
      {
        // this requires [glyphs] from main config file
        // but it's not a standard key-value format where we know the keys
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

  if (zMapConfigIniHasStanza(full_data->context->config, stanza_name,NULL) && (full_data->object_create_func))
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


GQuark zMapStyleQuark(gchar *str)
{
  GQuark q = g_quark_from_string(str);
  return( q);
}


void set_is_default(gpointer key, gpointer value, gpointer data)
{
	ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle) value;

	zMapStyleSetIsDefault(style);
}

/* get default styles w/ no reference to the styles file
 * NOTE keep this separate from zMapConfigIniGetStylesFromFile() as the config ini code
 * has a stanza priority system that prevents override, in contrast to the key code that does the opposite
 * application must merge user styles into default
 */
GHashTable * zmapConfigIniGetDefaultStyles(void)
{
  GHashTable *styles = NULL ;
  extern char * default_styles;		/* in a generated source file */

  zMapConfigIniGetStylesFromFile(NULL, NULL, NULL, &styles, default_styles);

  g_hash_table_foreach(styles, set_is_default, NULL);

  return styles;
}


// get style stanzas in styles_list of all from the file
gboolean zMapConfigIniGetStylesFromFile(char *config_file,
					char *styles_list, char *styles_file, GHashTable **styles_out, char * buffer)
{
  gboolean result = FALSE ;
  GHashTable *styles = NULL ;
  GList *settings_list = NULL, *free_this_list = NULL;
  ZMapConfigIniContext context ;
  GHashTable *shapes = NULL;
  int enum_value ;

  if ((context = zMapConfigIniContextProvideNamed(config_file, ZMAPSTANZA_STYLE_CONFIG)))
    {
	if(buffer)		/* default styles */
	{
		zMapConfigIniContextIncludeBuffer(context, buffer);
	}
	else if(styles_file)		/* separate styles file */
      {
		/* NOTE this only uses the extra_key key file */
	  zMapConfigIniContextIncludeFile(context,styles_file);
	}
      /* else styles are in main config named [style-xxx] */

	settings_list = zMapConfigIniContextGetStyleList(context,styles_list);
		/* style list is legacy and we don-t expect it to be used */
		/* this gets a list of all the stanzas in the file */

      shapes = zMapConfigIniGetGlyph(context);		/* these could be predef'd for default styles or provided/ overridden in user config */

	zMapConfigIniContextDestroy(context) ;
      context = NULL;
    }

  if (settings_list)
    {
      free_this_list = settings_list ;

      styles = g_hash_table_new(NULL,NULL);

      do
      {
        ZMapKeyValue curr_config_style ;
        ZMapFeatureTypeStyle new_style ;
        GParameter params[_STYLE_PROP_N_ITEMS + 20] ; // + 20 for good luck :-)
        guint num_params ;
        GParameter *curr_param ;
	  char *name;

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


        // if no parameters are specified the we get NULL for the name
        // quite how will take hours to unravel
        // either way the style is not usable
        if(!curr_config_style->data.str)
            continue;

  	  name = curr_config_style->data.str;
	  if(!g_ascii_strncasecmp(curr_config_style->data.str,"style-",6))
		name += 6;
	  else if(!styles_file)		/* not the styles file: must be explicitly [style-] */
		  continue;

	  g_value_set_string(&(curr_param->value), name) ;

        num_params++ ;
        curr_param++ ;
        curr_config_style++ ;

        while (curr_config_style->name)
          {
            enum_value = 0;

            if (curr_config_style->has_value)
            {
              curr_param->name = curr_config_style->name ;

              switch (curr_config_style->type)
                {
/*                case ZMAPCONF_STR_ARRAY:
                  {
                  if (curr_config_style->conv_type == ZMAPCONV_STR2COLOUR)
                    {
                      g_value_init(&(curr_param->value), G_TYPE_STRING) ;
                      g_value_set_string(&(curr_param->value), curr_config_style->data.str) ;
                    }
                  break ;
                  }
*/
                case ZMAPCONF_STR:
                  {
                  if (curr_config_style->conv_type == ZMAPCONV_STR2ENUM)
                    {
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

              // if we have a glyph shape defined by name we need to add the shape data as well
              if(enum_value)
                {
                  char *shape_param = NULL;

                  if(!strcmp(ZMAPSTYLE_PROPERTY_GLYPH_NAME,curr_config_style->name))
                    shape_param = ZMAPSTYLE_PROPERTY_GLYPH_SHAPE;

                  else if(!strcmp(ZMAPSTYLE_PROPERTY_GLYPH_NAME_5,curr_config_style->name))
                    shape_param = ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5;

                  else if(!strcmp(ZMAPSTYLE_PROPERTY_GLYPH_NAME_5_REV,curr_config_style->name))
                    shape_param = ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5_REV;

                  else if(!strcmp(ZMAPSTYLE_PROPERTY_GLYPH_NAME_3,curr_config_style->name))
                    shape_param = ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3;

                  else if(!strcmp(ZMAPSTYLE_PROPERTY_GLYPH_NAME_3_REV,curr_config_style->name))
                    shape_param = ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3_REV;

                  if(shape_param)
                    {
                      GQuark q = (GQuark) enum_value; // ie the name of the shape
                      ZMapStyleGlyphShape shape = NULL;

                      if(shapes)
                         shape = g_hash_table_lookup(shapes,GINT_TO_POINTER(q));

                      if(shape)
                        {
                          curr_param->name = shape_param;
                          g_value_init(&(curr_param->value), zMapStyleGlyphShapeGetType()) ;
                          g_value_set_boxed(&(curr_param->value),shape ) ;
                          num_params++ ;
                          curr_param++ ;
                        }
                    }
                }
            }
            curr_config_style++ ;
          }


        if ((new_style = zMapStyleCreateV(num_params, params)))
          {
            if (!zMapStyleSetAdd(styles, new_style))
            {
              /* Free style, report error and move on. */
              zMapStyleDestroy(new_style) ;

#if MH17_useless_message
// only occurs if there are no parameters specified
              zMapLogWarning("Styles file \"%s\", stanza %s could not be added.",
                         styles_file, curr_config_style->name) ;
#endif
            }
          }

      } while((settings_list = g_list_next(settings_list)));

    }

  zMapConfigStylesFreeList(free_this_list) ;
  if(shapes)
      g_hash_table_destroy(shapes);

  /* NOTE we can only inherit default styles not those from another source */
  if(!zMapStyleInheritAllStyles(styles))
	zMapLogWarning("%s", "There were errors in inheriting styles.") ;

  zMapStyleSetSubStyles( styles); /* this is not effective as a subsequent style copy will not copy this internal data */

  if(styles)
  {
      *styles_out = styles ;
      result = TRUE ;
  }

  return result ;
}



/* strip leading and trailing spaces and squash internal ones to one only */
/* operates in situ - call with g_strdup() if needed */
/* do not free the returned string */
char *zMapConfigNormaliseWhitespace(char *str,gboolean cannonical)
{
      char *p,*q;

      for(q = str;*q && *q <= ' ';q++)
            continue;

      for(p = str;*q;)
      {
            while(*q > ' ')
                  *p++ = cannonical ? g_ascii_tolower(*q++) : *q++;
            while(*q && *q <= ' ')
                  q++;
             if(*q)
                  *p++ = ' ';
      }
      *p = 0;     // there will always be room

      return(str);
}

/* Take a string containing ; separated  names (which may include whitespace)
 * (e.g.  a list of featuresets: "coding ; fgenes ; codon")
 * and convert it to a list of quarks.
 * whitespace gets normalised to one
 *
 * NOTE that this is all out of spec for glib -
 * key files are supposed to have keys containing only alphnumerics and '-'
 * but we need _ and <space> too
 * however it seems to run
 */

GList *zmapConfigString2QuarkListExtra(char *string_list, gboolean cannonical,gboolean unique_id)
{
  GList *list = NULL;
  gchar **str_array,**strv;
  char *name;
  GQuark val;

  str_array = g_strsplit(string_list,";",0);

  if (str_array)
    {
      for (strv = str_array;*strv;strv++)
	{
	  if ((name = zMapConfigNormaliseWhitespace(*strv, cannonical)) && *name)
	  {
	    if(unique_id)
            val = zMapStyleCreateID(name);
          else
            val = g_quark_from_string(name);

	    list = g_list_prepend(list,GUINT_TO_POINTER(val)) ;
	  }
	}
    }

  list = g_list_reverse(list) ;      // see glib doc for g_list_append()

  if (str_array)
    g_strfreev(str_array);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMap_g_quark_list_print(list) ;     /* debug.... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return list ;
}

GList *zMapConfigString2QuarkList(char *string_list, gboolean cannonical)
{
	return zmapConfigString2QuarkListExtra(string_list,cannonical,FALSE);
}

GList *zMapConfigString2QuarkIDList(char *string_list)
{
	return zmapConfigString2QuarkListExtra(string_list,TRUE,TRUE);
}



/*
#define ZMAPSTANZA_COLUMN_CONFIG              "columns"
#define ZMAPSTANZA_COLUMN_STYLE_CONFIG        "column-style"
#define ZMAPSTANZA_COLUMN_DESCRIPTION_CONFIG  "column-description"
#define ZMAPSTANZA_FEATURESET_STYLE_CONFIG    "featureset-style"
#define ZMAPSTANZA_GFF_SOURCE_CONFIG          "GFF-source"
#define ZMAPSTANZA_GFF_DESCRIPTION_CONFIG     "GFF-description"
*/

/*
 * read the [columns] stanza and put it in a hash table
 * NOTE: this function operates differently from normal ConfigIni in that we do not know
 *  the names of the keys in the stanza and cannot create a struct to hold these and thier values
 * So instead we have to use GLib directly.
 * the strings need to be quarked first
 */
/*
 * NOTE we set up the columns->featureset list here, which is used to order heatmap(coverage) featuresets in a column
 * as well as for requesting featuresets via columns
 * ACEDB supplies a fset to column mapping later on which does not affect pipe servers (used for coverage/BAM)
 * and if this is aditional to that already configured it's added
 */

GHashTable *zMapConfigIniGetFeatureset2Column(ZMapConfigIniContext context,GHashTable *hash, GHashTable *columns)
{
      GKeyFile *gkf;
      gchar ** keys,**freethis;
      GList *sources;
      ZMapFeatureSetDesc GFFset;

      GQuark column,column_id;
      char *names;
      gsize len;
      char *normalkey;
      ZMapFeatureColumn f_col;
      int n = g_hash_table_size(columns);

      if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_COLUMN_CONFIG,&gkf))
      {
            freethis = keys = g_key_file_get_keys(gkf,ZMAPSTANZA_COLUMN_CONFIG,&len,NULL);

            for(;len--;keys++)
            {
                  names = g_key_file_get_string(gkf,ZMAPSTANZA_COLUMN_CONFIG,*keys,NULL);

                  if(!names || !*names)
                        continue;

                  normalkey = zMapConfigNormaliseWhitespace(*keys,FALSE); // changes in situ: get names first
                  column = g_quark_from_string(normalkey);
                  column_id = zMapFeatureSetCreateID(normalkey);
                  f_col = g_hash_table_lookup(columns,GUINT_TO_POINTER(column_id));
                  if(!f_col)
                  {
                        f_col = (ZMapFeatureColumn) g_new0(ZMapFeatureColumnStruct,1);

                  	f_col->column_id = g_quark_from_string(normalkey);
                  	f_col->unique_id = column_id;
                  	f_col->column_desc = normalkey;
                  	f_col->order = ++n;

	                  g_hash_table_insert(columns,GUINT_TO_POINTER(f_col->unique_id),f_col);
                  }

#if MH17_USE_COLUMNS_HASH
      char *desc;
                        // add self ref to allow column lookup
                  GFFset = g_hash_table_lookup(hash,GUINT_TO_POINTER(column_id));
                  if(!GFFset)
                        GFFset = g_new0(ZMapFeatureSetDescStruct,1);

                  GFFset->column_id = column_id;        // lower cased name
                  GFFset->column_ID = column;           // display name
                  GFFset->feature_src_ID = column;      // display name

                  // add description if present
                  desc = normalkey;
                  GFFset->feature_set_text = g_strdup(desc);
                  g_hash_table_replace(hash,GUINT_TO_POINTER(column_id),GFFset);
#endif
                  sources = zMapConfigString2QuarkList(names,FALSE);

                  g_free(names);

                  while(sources)
                  {
                        // get featureset if present
                        GQuark key = zMapFeatureSetCreateID((char *)
                              g_quark_to_string(GPOINTER_TO_UINT(sources->data)));
                        GFFset = g_hash_table_lookup(hash,GUINT_TO_POINTER(key));

                        if(!GFFset)
                              GFFset = g_new0(ZMapFeatureSetDescStruct,1);

                        GFFset->column_id = column_id;        // lower cased name
                        GFFset->column_ID = column;           // display name
//zMapLogWarning("get f2c: set col ID %s",g_quark_to_string(GFFset->column_ID));
                        GFFset->feature_src_ID = GPOINTER_TO_UINT(sources->data);    // display name

                        g_hash_table_replace(hash,GUINT_TO_POINTER(key),GFFset);


                        /* construct reverse mapping from column to featureset */
                        if(!g_list_find(f_col->featuresets_unique_ids,GUINT_TO_POINTER(key)))
                        {
                              f_col->featuresets_unique_ids = g_list_append(f_col->featuresets_unique_ids,GUINT_TO_POINTER(key));
                        }


                        sources = g_list_delete_link(sources,sources);
                  }
            }
            if(freethis)
                  g_strfreev(freethis);
      }

      return(hash);
}


/* to handle composite featuresets we map real ones to a pretend one
 * [featuresets]
 * composite = source1 ; source2 ;  etc
 *
 * we also have to patch in the featureset to column mapping as config does not do this
 * which will make display and column style tables work
 * NOTE the composite featureset does not exist at all and is just used to name a ZMapWindowgraphDensityItem
 */
GHashTable *zMapConfigIniGetFeatureset2Featureset(ZMapConfigIniContext context,GHashTable *fset_src, GHashTable *fset2col)
{
      GKeyFile *gkf;
      gchar ** keys,**freethis;
      GList *sources;
      gsize len;

	GQuark set_id;

      char *names;

      ZMapFeatureSetDesc virtual_f2c;
      ZMapFeatureSetDesc real_f2c;


      GHashTable *virtual_featuresets = g_hash_table_new(NULL,NULL);

      if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_FEATURESETS_CONFIG,&gkf))
      {
            freethis = keys = g_key_file_get_keys(gkf,ZMAPSTANZA_FEATURESETS_CONFIG,&len,NULL);

            for(;len--;keys++)
            {
                  names = g_key_file_get_string(gkf,ZMAPSTANZA_FEATURESETS_CONFIG,*keys,NULL);

                  if(!names || !*names)
                        continue;

	                  /* this featureset will not actually exist, it's virtual */
                  set_id = zMapFeatureSetCreateID(*keys);
                  virtual_f2c = g_hash_table_lookup(fset2col,GUINT_TO_POINTER(set_id));
                  if(!virtual_f2c)
                  {
                  	/* should have been config'd to appear in a column */
                  	zMapLogWarning("cannot find virtual featureset %s",*keys);
                  	continue;
                  }

                  sources = zMapConfigString2QuarkList(names,FALSE);
                  g_free(names);

                  g_hash_table_insert(virtual_featuresets,GUINT_TO_POINTER(set_id), sources);

                  while(sources)
                  {
                  	ZMapFeatureSource f_src;
                        // get featureset if present
                        GQuark key = zMapFeatureSetCreateID((char *)
                              g_quark_to_string(GPOINTER_TO_UINT(sources->data)));
                        f_src = g_hash_table_lookup(fset_src,GUINT_TO_POINTER(key));

                        if(f_src)
                        {
					f_src->maps_to = set_id;

					/* now set up featureset to column mapping to allow the column to paint and styles to be found */
					real_f2c = g_hash_table_lookup(fset2col,GUINT_TO_POINTER(key));
					if(!real_f2c)
					{
						real_f2c = g_new0(ZMapFeatureSetDescStruct,1);
						g_hash_table_insert(fset2col,GUINT_TO_POINTER(key),real_f2c);
					}

					real_f2c->column_id = virtual_f2c->column_id;
					real_f2c->column_ID = virtual_f2c->column_ID;
	//		  		real_f2c->feature_set_text =
					real_f2c->feature_src_ID = GPOINTER_TO_UINT(sources->data);
				}
                        sources = sources->next;
                  }
            }
            if(freethis)
                  g_strfreev(freethis);
      }
      return virtual_featuresets;
}


// get the complete list of columns to display, in order
// somewhere this gets mangled by strandedness
GHashTable *zMapConfigIniGetColumns(ZMapConfigIniContext context)
{
      GKeyFile *gkf;
      GList *columns = NULL,*col;
      gchar *colstr;
      ZMapFeatureColumn f_col;
      GHashTable *hash = NULL;
      GHashTable *col_desc;
      char *desc;

      hash = g_hash_table_new(NULL,NULL);

      // nb there is a small memory leak if we config description for non-existant columns
      col_desc   = zMapConfigIniGetQQHash(context,ZMAPSTANZA_COLUMN_DESCRIPTION_CONFIG,QQ_STRING);


      if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_APP_CONFIG,&gkf))
      {
            colstr = g_key_file_get_string(gkf, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_COLUMNS,NULL);

            if(colstr && *colstr)
                  columns = zMapConfigString2QuarkList(colstr,FALSE);


            for(col = columns; col;col = col->next)
            {
                  f_col = g_new0(ZMapFeatureColumnStruct,1);
                  f_col->column_id = GPOINTER_TO_UINT(col->data);

                  desc = (char *) g_quark_to_string(f_col->column_id);
                  f_col->unique_id = zMapFeatureSetCreateID(desc);

                  f_col->column_desc = g_hash_table_lookup(col_desc,
                        GUINT_TO_POINTER(f_col->column_id));
                  if(!f_col->column_desc)
                        f_col->column_desc = desc;
                  f_col->order = zMapFeatureColumnOrderNext();
                  g_hash_table_insert(hash,GUINT_TO_POINTER(f_col->unique_id),f_col);
            }
      }

      if(col_desc)
            g_hash_table_destroy(col_desc);

      return(hash);
}




/*
 * read a named stanza and put it in a hash table
 * NOTE: this function operates differently from normal ConfigIni in that we do not know
 *  the names of the keys in the stanza and cannot create a struct to hold these and thier values
 * So instead we have to use GLib directly.
 * This is a simple generic table of quark->quark
 * used for [featureset_styles] [GFF_source] and [column_styles]
 */

 // how: string for throwaway values, or styleId or quark
GHashTable *zMapConfigIniGetQQHash(ZMapConfigIniContext context,char *stanza, int how)
{
      GHashTable *hash = NULL;
      GKeyFile *gkf;
      gchar ** keys = NULL;
      gsize len;
      gchar *value,*strval;

      hash = g_hash_table_new(NULL,NULL);

      if(zMapConfigIniHasStanza(context->config,stanza,&gkf))
      {

            keys = g_key_file_get_keys(gkf,stanza,&len,NULL);

            for(;len--;keys++)
            {
                  value = g_key_file_get_string(gkf,stanza,*keys,NULL);
                  if(value && *value)
                  {
                        gpointer gval;
                        gpointer kval;

                              // strips leading spaces by skipping
                        strval = zMapConfigNormaliseWhitespace(value,FALSE);

                        if(how == QQ_STRING)
                              gval = g_strdup(strval);
                        else if(how == QQ_QUARK)
                              gval =  GUINT_TO_POINTER(g_quark_from_string(strval));
                        else
                              gval = GUINT_TO_POINTER(zMapStyleCreateID(strval));

                        kval = GUINT_TO_POINTER(zMapFeatureSetCreateID(*keys));     // keys are fully normalised
                        g_hash_table_insert(hash,kval,gval);
                        g_free(value);
                  }
            }
      }

      return(hash);
}


/*
 * read the [glyphs] stanza and put it in a hash table as text
 * NOTE: this function operates differently from normal ConfigIni in that we do not know
 *  the names of the keys in the stanza and cannot create a struct to hold these and thier values
 * So instead we have to use GLib directly.
 * the name strings need to be quarked first
 *
 * glyph shapes are simple things and not GObjects
 * they are stored in the styles objects and are accessable from there only
 * all this data gets freed after being read.
 * NOTE: when this data structre gets passed to a style then it's as a zMapStyleGlyphShapeGetType (a boxed data type)
 */


GHashTable *zMapConfigIniGetGlyph(ZMapConfigIniContext context)
{
      GHashTable *hash = NULL;
      GKeyFile *gkf;
      gchar ** keys = NULL;
      gsize len;
      char *shape;
      ZMapStyleGlyphShape glyph_shape;
      GQuark q;

      if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_GLYPH_CONFIG,&gkf))
      {
            hash = g_hash_table_new(NULL,NULL);

            keys = g_key_file_get_keys(gkf,ZMAPSTANZA_GLYPH_CONFIG,&len,NULL);

            for(;len--;keys++)
            {
                  q = g_quark_from_string(*keys);
                  shape = g_key_file_get_string(gkf,ZMAPSTANZA_GLYPH_CONFIG,*keys,NULL);

			if(!shape)
				continue;
			glyph_shape = zMapStyleGetGlyphShape(shape,q);

			if(!glyph_shape)
			{
				zMapLogWarning("Glyph shape %s: syntax error in %s",*keys,shape);
			}
			else
			{
				g_hash_table_insert(hash,GUINT_TO_POINTER(q),glyph_shape);
			}
			g_free(shape);
            }
      }

      return(hash);
}



/* return a hash table of GArray of GdkColor indexed by the GQuark of the name */
/* (there are limits to honw many colours we can use and GArrays are tedious) */
GHashTable *zMapConfigIniGetHeatmaps(ZMapConfigIniContext context)
{
     GHashTable *hash = NULL;
      GKeyFile *gkf;
      gchar ** keys = NULL;
      gsize len;
      GQuark name;
	char *colours, *p,*q;
	GArray *col_array;
	GdkColor col;

      if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_HEATMAP_CONFIG,&gkf))
      {
            hash = g_hash_table_new(NULL,NULL);

            keys = g_key_file_get_keys(gkf,ZMAPSTANZA_HEATMAP_CONFIG,&len,NULL);

            for(;len--;keys++)
            {
                  name = g_quark_from_string(*keys);
                  colours = g_key_file_get_string(gkf,ZMAPSTANZA_HEATMAP_CONFIG,*keys,NULL);
			if(!colours)
				continue;

			col_array = g_array_sized_new(FALSE,FALSE,sizeof(GdkColor),8);

			for(p = colours; *p; p = q)
			{
				for(q = p; *q && *q != ';'; q++)
					continue;
				if(*q)
					*q++ = 0;
				else
					*q = 0;

				gdk_color_parse(p,&col);
				g_array_append_val(col_array,col);
			}

                  g_hash_table_insert(hash,GUINT_TO_POINTER(name),col_array);

			g_free(colours);
            }
      }

      return(hash);
}


gboolean zMapConfigLegacyStyles(char *config_file)
{
  static gboolean result = FALSE;
  ZMapConfigIniContext context;
  static int got = 0;

  // this needs to be once only as it gets called when drawing glyphs
  if (!got)
    {
      got = TRUE ;
      if ((context = zMapConfigIniContextProvide(NULL)))
	{
	  zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					 ZMAPSTANZA_APP_LEGACY_STYLES, &result);
	  zMapConfigIniContextDestroy(context);
	}
    }

  return(result) ;
}



static ZMapConfigIniContextKeyEntry get_app_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_APP_MAINWINDOW,   G_TYPE_BOOLEAN, NULL, FALSE },

    { ZMAPSTANZA_APP_DATASET ,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_SEQUENCE,     G_TYPE_STRING,  NULL, FALSE },
    /* csname and csver are provided by otterlace with start and end coordinates
     * but are 'always' chromosome and Otter. see zmapApp.c/getConfiguration()
     */
    { ZMAPSTANZA_APP_CSNAME,       G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_CSVER,        G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_START,        G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_END,          G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_HELP_URL,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_LOCALE,       G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_COOKIE_JAR,   G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_PFETCH_MODE,  G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_PFETCH_LOCATION, G_TYPE_STRING, NULL, FALSE },
//    { ZMAPSTANZA_APP_SCRIPTS,      G_TYPE_STRING, NULL, FALSE },
    { ZMAPSTANZA_APP_DATA,         G_TYPE_STRING, NULL, FALSE },
    { ZMAPSTANZA_APP_STYLESFILE,   G_TYPE_STRING, NULL, FALSE },
    { ZMAPSTANZA_APP_LEGACY_STYLES,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_STYLE_FROM_METHOD,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_XREMOTE_DEBUG,G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_REPORT_THREAD,G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_NAVIGATOR_SETS,  G_TYPE_STRING, NULL, FALSE },
    { ZMAPSTANZA_APP_SEQ_DATA,    G_TYPE_STRING, NULL, FALSE },
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
    { ZMAPSTANZA_LOG_SHOW_TIME, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_LOG_CATCH_GLIB, G_TYPE_BOOLEAN,  NULL, FALSE },
    { ZMAPSTANZA_LOG_ECHO_GLIB, G_TYPE_BOOLEAN,  NULL, FALSE },
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
    { ZMAPSTANZA_DEBUG_APP_TIMING, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_DEBUG_APP_DEVELOP, G_TYPE_BOOLEAN, NULL, FALSE },
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

      { ZMAPSTYLE_PROPERTY_DISPLAY_MODE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2ColDisplayState} },
      { ZMAPSTYLE_PROPERTY_BUMP_MODE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2BumpMode} },
      { ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2BumpMode} },
      { ZMAPSTYLE_PROPERTY_BUMP_SPACING, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_BUMP_STYLE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_MIN_MAG, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MAX_MAG, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_WIDTH, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_SCORE_MODE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2ScoreMode} },
      { ZMAPSTYLE_PROPERTY_MIN_SCORE, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MAX_SCORE, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SCORE_SCALE,  FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GraphScale} },	/* yes: GraphScale */

      { ZMAPSTYLE_PROPERTY_SUMMARISE, FALSE, ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_COLLAPSE, FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GFF_SOURCE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GFF_FEATURE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_DISPLAYABLE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SHOW_TEXT,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SUB_FEATURES, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_HIDE_FORWARD_STRAND,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr23FrameMode} },

      { ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_FOO,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_FILTER,FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },


      { ZMAPSTYLE_PROPERTY_GLYPH_NAME, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleQuark} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_5, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleQuark} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_5_REV, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleQuark} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5_REV,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_3, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleQuark} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_3_REV, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleQuark} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3_REV,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

      { ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_GLYPH_STRAND,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphStrand} },
      { ZMAPSTYLE_PROPERTY_GLYPH_ALIGN,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphAlign} },

      { ZMAPSTYLE_PROPERTY_GRAPH_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GraphMode} },
      { ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,   FALSE, ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_SCALE,  FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GraphScale} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_FIXED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_MIN_BIN,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_STAGGER,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },


      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALWAYS_GAPPED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_UNIQUE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc) zMapStyleStr2BlixemType } },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2ColDisplayState} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_GAP_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_COMMON_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_MIXED_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_MASK_SETS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_SQUASH, FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_OVERLAP, FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_THRESHOLD, FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_MAX, FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_SEQUENCE_NON_CODING_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_SEQUENCE_CODING_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_5_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_3_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_SEQUENCE_IN_FRAME_CODING_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_SEQUENCE_START_CODON_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_SEQUENCE_STOP_CODON_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

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
/*	case ZMAPCONF_STR_ARRAY:
	  if (style_conf->has_value)
	    g_strfreev(style_conf->data.str_array) ;
	  break ;
*/
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

    { ZMAPSTYLE_PROPERTY_DISPLAY_MODE, G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_BUMP_MODE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_BUMP_SPACING, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_BUMP_STYLE, G_TYPE_STRING,  style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_MIN_MAG, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MAX_MAG, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_WIDTH, G_TYPE_DOUBLE,  style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_SCORE_MODE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MIN_SCORE, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MAX_SCORE, G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SCORE_SCALE,   G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_SUMMARISE,   G_TYPE_DOUBLE, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_COLLAPSE,   G_TYPE_BOOLEAN, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_GFF_SOURCE, G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GFF_FEATURE, G_TYPE_STRING,  style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_DISPLAYABLE,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SHOW_TEXT,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SUB_FEATURES, G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC ,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_HIDE_FORWARD_STRAND,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FRAME_MODE,   G_TYPE_STRING, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FOO,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FILTER,G_TYPE_BOOLEAN, style_set_property, FALSE },


      // these three names relate to 3 more real parameters
      // the names specify a shape string to be extracted from [glyphs]
    { ZMAPSTYLE_PROPERTY_GLYPH_NAME,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_NAME_5,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_NAME_5_REV,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_NAME_3,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_NAME_3_REV,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_STRAND,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GLYPH_ALIGN,   G_TYPE_STRING, style_set_property, FALSE },


    { ZMAPSTYLE_PROPERTY_GRAPH_MODE,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,   G_TYPE_DOUBLE, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_SCALE,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_FIXED,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_MIN_BIN,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_STAGGER,   G_TYPE_INT, style_set_property, FALSE },


    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALWAYS_GAPPED,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_UNIQUE,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_GAP_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_COMMON_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_MIXED_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_MASK_SETS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_SQUASH,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_OVERLAP,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_THRESHOLD,   G_TYPE_INT, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_MAX,   G_TYPE_INT, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_SEQUENCE_NON_CODING_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SEQUENCE_CODING_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_5_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_3_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SEQUENCE_IN_FRAME_CODING_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SEQUENCE_START_CODON_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SEQUENCE_STOP_CODON_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },

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
  ZMapConfigSource src = g_new0(ZMapConfigSourceStruct, 1);

  src->group = SOURCE_GROUP_START;        // default_value
  return(src);
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
//  if(source_to_free->navigatorsets)
//    g_free(source_to_free->navigatorsets);
  if(source_to_free->stylesfile)
    g_free(source_to_free->stylesfile);
//  if(source_to_free->styles_list)
//    g_free(source_to_free->styles_list);
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
//    { ZMAPSTANZA_SOURCE_STYLES,        G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_REQSTYLES,     G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_STYLESFILE,    G_TYPE_STRING,  source_set_property, FALSE },
//    { ZMAPSTANZA_SOURCE_NAVIGATORSETS, G_TYPE_STRING,  source_set_property, FALSE },
//    { ZMAPSTANZA_SOURCE_SEQUENCE,      G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_FORMAT,        G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_DELAYED,       G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_MAPPING,       G_TYPE_BOOLEAN, source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_GROUP,           G_TYPE_STRING,  source_set_property, FALSE },
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
//  double *double_ptr ;
  char **str_ptr ;

  if (key && *key)
    {
      if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_URL) == 0)
	str_ptr = &(config_source->url) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_VERSION) == 0)
	str_ptr = &(config_source->version) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_FEATURESETS) == 0)
	str_ptr = &(config_source->featuresets) ;
//      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_STYLES) == 0)
//	str_ptr = &(config_source->styles_list) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_REQSTYLES) == 0)
	bool_ptr = &(config_source->req_styles) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_STYLESFILE) == 0)
      str_ptr = &(config_source->stylesfile) ;
//      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_NAVIGATORSETS) == 0)
//	str_ptr = &(config_source->navigatorsets) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_TIMEOUT) == 0)
	int_ptr = &(config_source->timeout) ;
//      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_SEQUENCE) == 0)
//	bool_ptr = &(config_source->sequence) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_FORMAT) == 0)
	str_ptr = &(config_source->format) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_DELAYED) == 0)
      bool_ptr = &(config_source->delayed) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_MAPPING) == 0)
      bool_ptr = &(config_source->provide_mapping) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_GROUP) == 0)
      {
        int_ptr = &(config_source->group) ;

        // painful bit of code but there you go
        *int_ptr = SOURCE_GROUP_NEVER;
        char *value = "";
        if(type == G_TYPE_STRING)
            value = (char *)g_value_get_string(property_value);

        if     (!strcmp(value,ZMAPSTANZA_SOURCE_GROUP_ALWAYS))
          *int_ptr = SOURCE_GROUP_ALWAYS;
        else if(!strcmp(value,ZMAPSTANZA_SOURCE_GROUP_START))
          *int_ptr = SOURCE_GROUP_START;
        else if(!strcmp(value,ZMAPSTANZA_SOURCE_GROUP_DELAYED))
          *int_ptr = SOURCE_GROUP_DELAYED;
        else if(strcmp(value,ZMAPSTANZA_SOURCE_GROUP_NEVER))
          zMapLogWarning("Server stanza %s group option invalid",current_stanza_name);

        return;
      }

      if (type == G_TYPE_BOOLEAN && G_VALUE_TYPE(property_value) == type)
	*bool_ptr = g_value_get_boolean(property_value);
      else if (type == G_TYPE_INT && G_VALUE_TYPE(property_value) == type)
	*int_ptr = g_value_get_int(property_value);
// there are no doubles
//      else if (type == G_TYPE_DOUBLE && G_VALUE_TYPE(property_value) == type)
//	*double_ptr = g_value_get_double(property_value);
      else if (type == G_TYPE_STRING && G_VALUE_TYPE(property_value) == type)
	*str_ptr = (char *)g_value_get_string(property_value);
    }

  return ;
}



/* ZMapWindow */

static ZMapConfigIniContextKeyEntry get_window_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_WINDOW_CURSOR,       G_TYPE_STRING,  NULL, FALSE },
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
    { ZMAPSTANZA_WINDOW_ITEM_EVIDENCE_BORDER, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_ITEM_EVIDENCE_FILL,   G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_MASKED_FEATURE_FILL,   G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_MASKED_FEATURE_BORDER,   G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_FILTERED_COLUMN,   G_TYPE_STRING,  NULL, FALSE },
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
    { ZMAPSTANZA_BLIXEM_NETID,       G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_PORT,        G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_SCRIPT,      G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_CONF_FILE,   G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_SCOPE,       G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_SCOPE_MARK,  G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_FEATURES_MARK,  G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_FILE_FORMAT, G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_MAX,         G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_KEEP_TEMP,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_KILL_EXIT,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_DNA_FS,      G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_PROT_FS,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_FS,          G_TYPE_STRING,  NULL, FALSE },
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


