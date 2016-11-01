/*  File: zmapConfigLoader.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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

#include <ZMap/zmap.hpp>

#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapStyle.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <zmapConfigIni_P.hpp>


using namespace std ;


typedef struct
{
  const char *stanza_name;
  const char *stanza_type;
  GList *keys;
} ZMapConfigIniContextStanzaEntryStruct, *ZMapConfigIniContextStanzaEntry ;


typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;
  gboolean result;
} CheckRequiredKeysStruct, *CheckRequiredKeys ;


/* Context Config Loading */
typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;

  char *current_stanza_name;

  ZMapConfigIniUserDataCreateFunc object_create_func;
  gpointer current_object;

  GList *object_list_out;
} FetchReferencedStanzasStruct, *FetchReferencedStanzas ;




static ZMapConfigIniContextKeyEntry get_app_group_data(const char **stanza_name, const char **stanza_type);
static ZMapConfigIniContextKeyEntry get_peer_group_data(const char **stanza_name, const char **stanza_type);
static ZMapConfigIniContextKeyEntry get_logging_group_data(const char **stanza_name, const char **stanza_type);
static ZMapConfigIniContextKeyEntry get_debug_group_data(const char **stanza_name, const char **stanza_type);
static ZMapConfigIniContextKeyEntry get_style_group_data(const char **stanza_name, const char **stanza_type) ;
static ZMapConfigIniContextKeyEntry get_source_group_data(const char **stanza_name, const char **stanza_type);
static ZMapConfigIniContextKeyEntry get_window_group_data(const char **stanza_name, const char **stanza_type);
static ZMapConfigIniContextKeyEntry get_blixem_group_data(const char **stanza_name, const char **stanza_type);
static gpointer create_config_source(gpointer data);
static void free_source_list_item(gpointer list_data, gpointer unused_data);
static void source_set_property(char *current_stanza_name, const char *key, GType type,
gpointer parent_data, GValue *property_value) ;
static gpointer create_config_style(gpointer data) ;
static void style_set_property(char *current_stanza_name, const char *key, GType type,
       gpointer parent_data, GValue *property_value) ;
static void free_style_list_item(gpointer list_data, gpointer unused_data)  ;
static void free_source_names(gpointer list_data, gpointer unused_user_data) ;
static gint match_type(gconstpointer list_data, gconstpointer user_data);
static void fetch_referenced_stanzas(gpointer list_data, gpointer user_data) ;
static void fetchStanzas(FetchReferencedStanzas full_data, GKeyFile *key_file, char *stanza_name) ;
static ZMapConfigIniContextStanzaEntry get_stanza_with_type(ZMapConfigIniContext context, const char *stanza_type) ;
static GList *get_child_stanza_names_as_list(ZMapConfigIniContext context,
                                             const char *parent_name, const char *parent_key) ;
static void set_is_default(gpointer key, gpointer value, gpointer data) ;
static GList *get_names_as_list(char *styles) ;
static GList *contextGetNamedStanzas(ZMapConfigIniContext context,
     ZMapConfigIniUserDataCreateFunc object_create_func,
     const char *stanza_type, GKeyFile *extra_styles_keyfile) ;
static GList *contextGetStyleList(ZMapConfigIniContext context, char *styles_list, GKeyFile *extra_styles_keyfile) ;
static GHashTable *configIniGetGlyph(ZMapConfigIniContext context, GKeyFile *extra_styles_keyfile) ;
static void stylesFreeList(GList *config_styles_list) ;


//
//                  Globals
//

static bool debug_loading_G = false ;                       // Use to turn debugging logging/messages on/off.





/*
 *                  ZMapConfigSource class
 */

ZMapConfigSourceStruct::ZMapConfigSourceStruct()
  : name_{0},
  version{NULL},
  featuresets{NULL},
  biotypes{NULL},
  stylesfile{NULL},
  format{NULL},
  timeout{0},
  delayed{FALSE},
  provide_mapping{FALSE},
  req_styles{FALSE},
  group{0},
  recent{false},
  parent{NULL},
  children{},
  url_{NULL},
  url_obj_{NULL},
  url_parse_error_{0},
  config_file_{0},
  file_type_(),
  num_fields_{0}
{
}


ZMapConfigSourceStruct::~ZMapConfigSourceStruct()
{
  if(url_)
    g_free(url_);
  if(version)
    g_free(version);
  if(featuresets)
    g_free(featuresets);
  if(biotypes)
    g_free(biotypes);
  //  if(navigatorsets)
  //    g_free(navigatorsets);
  if(stylesfile)
    g_free(stylesfile);
  //  if(styles_list)
  //    g_free(styles_list);
  if(format)
    g_free(format);
  if (url_obj_)
    url_free(url_obj_) ;
}


void ZMapConfigSourceStruct::setUrl(const char *url)
{
  // Clear existing url, url_obj and url parser error, if set
  if (url_)
    {
      g_free(url_) ;
      url_ = NULL ;
    }

  if (url_obj_)
    {
      url_free(url_obj_) ;
      url_obj_ = NULL ;
    }

  url_parse_error_ = 0 ;

  // Set the url string
  if (url && *url)
    url_ = g_strdup(url) ;
}

void ZMapConfigSourceStruct::setConfigFile(const char *config_file)
{
  config_file_ = g_quark_from_string(config_file) ;
}

void ZMapConfigSourceStruct::setFileType(const string &file_type)
{
  file_type_ = file_type ;
}

void ZMapConfigSourceStruct::setNumFields(const int num_fields)
{
  num_fields_ = num_fields ;
}


const char* ZMapConfigSourceStruct::url() const
{
  return url_ ; 
}

const ZMapURL ZMapConfigSourceStruct::urlObj() const
{
  // If not already done, parse the string into url_obj. Set to null and set the error if this
  // fails. If the error is already set, then don't bother trying to re-parse.
  if (!url_obj_ && !url_parse_error_ && url_)
    {
      // Can return NULL and an error if it fails.
      url_obj_ = url_parse(url_, &url_parse_error_) ;
    }

  return url_obj_ ; 
}

const string ZMapConfigSourceStruct::urlError() const
{
  string result("No error") ;

  if (url_parse_error_)
    result = url_error(url_parse_error_) ;

  return result ;
}

const char* ZMapConfigSourceStruct::configFile() const
{
  return g_quark_to_string(config_file_) ; 
}

/* Return the file type. Returns an empty string if not applicable */
const string ZMapConfigSourceStruct::fileType() const
{
  return file_type_ ; 
}

/* Return the number of fileds for file sources. Returns 0 if not applicable. */
int ZMapConfigSourceStruct::numFields() const
{
  return num_fields_ ; 
}

/* Get a descriptive type for this source. This returns the type of the source, e.g. "ensembl",
 * and also the file type and number of fields, if applicable e.g. "file (bigbed 12)" */
string ZMapConfigSourceStruct::type() const
{
  string result("") ;
  
  if (urlObj())
    {
      stringstream result_ss ;
      result_ss << urlObj()->protocol ;

      if (!fileType().empty())
        {
          result_ss << " (" << fileType() ;
                
          if (numFields() > 0)
            result_ss << " " << numFields() ;

          result_ss << ")" ;
        }
      
      result = result_ss.str() ;
    }

  return result ;
}

// Return the toplevel source name; that is, if this source is in a hierarchy return the name of
// the toplevel parent. Otherwise just return this source's name.
string ZMapConfigSourceStruct::toplevelName() const
{
  string result ;

  if (parent)
    result = parent->toplevelName() ;
  else
    result = g_quark_to_string(name_) ;

  return result ;
}


void ZMapConfigSourceStruct::countSources(uint &num_total, 
                                          uint &num_with_data, 
                                          uint &num_to_load, 
                                          const bool recent_only) const
{
  if (!recent_only || recent)
    {
      ++num_total ;

      // Check if this source has data to load. It has data if it has a valid URL object which is
      // not a trackhub url (because trackhub urls are not loaded directly - only their children
      // are loaded).
      if (urlObj() && urlObj()->scheme != SCHEME_TRACKHUB)
        {
          ++num_with_data ;

          if (!delayed)
            ++num_to_load ;
        }
    }

  // recurse
  for (ZMapConfigSource child : children)
    {
      child->countSources(num_total, num_with_data, num_to_load, recent_only) ;
    }
}



/*
 *                  External Interface routines
 */


/* Note that config_file can be null here - this returns a new context which has  
 * the config_read flag as false. file_type defaults to ZMAPCONFIG_FILE_NONE which means we will
 * read all of the config file types in priority order. Specify a specific type to read only that type. */
ZMapConfigIniContext zMapConfigIniContextProvide(const char *config_file, ZMapConfigIniFileType file_type)
{
  ZMapConfigIniContext context = NULL ;

  if (file_type == ZMAPCONFIG_FILE_NONE)
    context = zMapConfigIniContextCreate(config_file) ;
  else
    context = zMapConfigIniContextCreateType(config_file, file_type) ;

  /* Add the default groups for standard zmap config files (i.e. anything except a styles file) */
  if(context && file_type != ZMAPCONFIG_FILE_STYLES)
    {
      ZMapConfigIniContextKeyEntry stanza_group = NULL;
      const char *stanza_name, *stanza_type;

      if((stanza_group = get_logging_group_data(&stanza_name, &stanza_type)))
        zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);

      if((stanza_group = get_app_group_data(&stanza_name, &stanza_type)))
        zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);

      if((stanza_group = get_peer_group_data(&stanza_name, &stanza_type)))
        zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);

      if((stanza_group = get_debug_group_data(&stanza_name, &stanza_type)))
        zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);

      if((stanza_group = get_source_group_data(&stanza_name, &stanza_type)))
        zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);

      if((stanza_group = get_window_group_data(&stanza_name, &stanza_type)))
        zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);

      if((stanza_group = get_blixem_group_data(&stanza_name, &stanza_type)))
        zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);
    }
  else
    {
      zMapLogWarning("%s", "Error creating config file context") ;
    }

  return context;
}




ZMapConfigIniContext zMapConfigIniContextProvideNamed(const char *config_file, const char *stanza_name_in, 
                                                      ZMapConfigIniFileType file_type)
{
  ZMapConfigIniContext context = NULL;

  /* if (!stanza_name_in || !*stanza_name_in)
    return context ; */

  zMapReturnValIfFail(stanza_name_in, context) ;
  zMapReturnValIfFail(*stanza_name_in, context) ;

  if (file_type == ZMAPCONFIG_FILE_NONE)
    context = zMapConfigIniContextCreate(config_file) ;
  else
    context = zMapConfigIniContextCreateType(config_file, file_type) ;


  if (context)
    {
      ZMapConfigIniContextKeyEntry stanza_group = NULL;
      const char *stanza_name, *stanza_type;


      if (g_ascii_strcasecmp(stanza_name_in, ZMAPSTANZA_SOURCE_CONFIG) == 0)        /* unused */
        {
          if((stanza_group = get_source_group_data(&stanza_name, &stanza_type)))
            zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group);
        }

      if (g_ascii_strcasecmp(stanza_name_in, ZMAPSTANZA_STYLE_CONFIG) == 0)
        {
          /* this requires [glyphs] from main config file
             but it's not a standard key-value format where we know the keys */
          if ((stanza_group = get_style_group_data(&stanza_name, &stanza_type)))
            zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group) ;
        }

      /* OK...NOW ADD THE OTHERS... */
    }


  return context;
}



GList *zMapConfigIniContextGetSources(ZMapConfigIniContext context)
{
  GList *glist = NULL;

  glist = zMapConfigIniContextGetReferencedStanzas(context, create_config_source,
                                                   ZMAPSTANZA_APP_CONFIG,
                                                   ZMAPSTANZA_APP_CONFIG,
                                                   ZMAPSTANZA_APP_SOURCES, "source");

  return glist;
}

GList *zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name)
{
  GList *glist = NULL;

  /* if (!stanza_name || !*stanza_name)
    return glist ; */
  zMapReturnValIfFail(stanza_name, glist) ;
  zMapReturnValIfFail(*stanza_name, glist) ;

  if (g_ascii_strcasecmp(stanza_name, ZMAPSTANZA_STYLE_CONFIG) == 0)
    {
      glist = zMapConfigIniContextGetReferencedStanzas(context, create_config_style,
      ZMAPSTANZA_APP_CONFIG,
      ZMAPSTANZA_APP_CONFIG,
      "styles", "style") ;
    }

  return glist ;
}


void zMapConfigSourcesFreeList(GList *config_sources_list)
{
  zMapReturnIfFail(config_sources_list) ;

  g_list_foreach(config_sources_list, free_source_list_item, NULL);
  g_list_free(config_sources_list);

  return ;
}


GList *zMapConfigIniContextGetReferencedStanzas(ZMapConfigIniContext context,
                                                ZMapConfigIniUserDataCreateFunc object_create_func,
                                                const char *parent_name,
                                                const char *parent_type,
                                                const char *parent_key,
                                                const char *child_type)
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



/* see zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name)
 * in zmapConfigIni.c, which is a wrapper for GetReferencedStanzas() above
 * For a list of styles in a source stanza we can't get these easily as we don't know the name
 * of the source stanza, and GetNamed hard codes 'source' as the name of the parent.
 * so we find each stanza by name from the list and add to our return list.
 */

GList *zMapConfigIniContextGetListedStanzas(ZMapConfigIniContext context,
    ZMapConfigIniUserDataCreateFunc object_create_func,
    char *styles_list,const char *child_type)
{
  FetchReferencedStanzasStruct data = {NULL};
  GList *styles      = NULL;
  GList *style_names = NULL;

  data.context = context;
  data.stanza  = get_stanza_with_type(context, child_type);
  data.object_create_func = object_create_func;

  style_names = get_names_as_list(styles_list);

  if (!style_names)
    return styles ;

  g_list_foreach(style_names, fetch_referenced_stanzas, &data);

  //  g_list_foreach(style_names, free_source_names, NULL);
  g_list_free(style_names);

  styles = data.object_list_out;

  return styles;
}



/* get default styles w/ no reference to the styles file
 * NOTE keep this separate from zMapConfigIniGetStylesFromFile() as the config ini code
 * has a stanza priority system that prevents override, in contrast to the key code that does the opposite
 * application must merge user styles into default
 */
GHashTable * zmapConfigIniGetDefaultStyles(void)
{
  GHashTable *styles = NULL ;
  extern const char * default_styles;/* in a generated source file */

  zMapConfigIniGetStylesFromFile(NULL, NULL, NULL, &styles, default_styles) ;

  g_hash_table_foreach(styles, set_is_default, NULL);

  return styles;
}


/* get style stanzas in styles_list of all from the file */
gboolean zMapConfigIniGetStylesFromFile(char *config_file, char *styles_list, char *styles_file,
                                        GHashTable **styles_out, const char * buffer)
{
  gboolean result = FALSE ;
  GHashTable *styles = NULL ;
  GList *settings_list = NULL, *free_this_list = NULL;
  ZMapConfigIniContext context ;
  GHashTable *shapes = NULL;
  int enum_value ;


  /* ERROR HANDLING ???? */
  if ((context = zMapConfigIniContextProvideNamed(config_file, ZMAPSTANZA_STYLE_CONFIG, ZMAPCONFIG_FILE_NONE)))
    {
      GKeyFile *extra_styles_keyfile = NULL ;

      if (buffer)/* default styles */
        {
          zMapConfigIniContextIncludeBuffer(context, buffer);
        }
      else if (styles_file)/* separate styles file */
        {
          zMapConfigIniContextIncludeFile(context, styles_file, ZMAPCONFIG_FILE_STYLES) ;

          /* This is mucky....the above call puts whatever the file is into
           * config->extra_key_file and config->extra_file_name.
           * really truly not good...should all be explicit....for now we do this here so at
           * least the calls in this file are more explicit... */
          extra_styles_keyfile = context->config->key_file[ZMAPCONFIG_FILE_STYLES] ;
        }
      /* else styles are in main config named [style-xxx] */

      /* style list is legacy and we don't expect it to be used */
      /* this gets a list of all the stanzas in the file */
      settings_list = contextGetStyleList(context, styles_list, extra_styles_keyfile) ;

      shapes = configIniGetGlyph(context, extra_styles_keyfile) ; /* these could be predef'd for default
                                                                     styles or provided/ overridden in user config */

      zMapConfigIniContextDestroy(context) ;
      context = NULL ;
    }

  if (settings_list)
    {
      free_this_list = settings_list ;

      styles = g_hash_table_new(NULL,NULL);

      do
        {
          ZMapKeyValue curr_config_style ;
          ZMapFeatureTypeStyle new_style ;
          GParameter params[_STYLE_PROP_N_ITEMS + 20] ; /* + 20 for good luck :-) */
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
          g_value_init(&(curr_param->value), G_TYPE_STRING) ;

          curr_param->name = curr_config_style->name ;


          /* if no parameters are specified the we get NULL for the name
             quite how will take hours to unravel
             either way the style is not usable */
          if(!curr_config_style->data.str)
            continue;

            name = curr_config_style->data.str;

          if(!g_ascii_strncasecmp(curr_config_style->data.str,"style-",6))
            name += 6;
          else if(!styles_file)/* not the styles file: must be explicitly [style-] */
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
                          zMapWarnIfReached() ;
                          break ;
                        }
                    }
                  num_params++ ;
                  curr_param++ ;

                  /* if we have a glyph shape defined by name we need to add the shape data as well */
                  if (enum_value)
                    {
                      const char *shape_param = NULL;

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
                          GQuark q = (GQuark) enum_value; /* ie the name of the shape */
                          ZMapStyleGlyphShape shape = NULL;

                          if(shapes)
                            shape = (ZMapStyleGlyphShape)g_hash_table_lookup(shapes,GINT_TO_POINTER(q));

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


          if (!(new_style = zMapStyleCreateV(num_params, params)))
            {
              zMapLogWarning("Styles file \"%s\": could not create new style %s.",
                             styles_file, curr_config_style->name) ;
            }
          else
            {
              /* Try to add the new style, fails if a style with that name already exists, this is
                 completely to be expected so we don't usually log it. */
              if (!zMapStyleSetAdd(styles, new_style))
                {
                  /* Style already loaded so free this copy. */
                  if (debug_loading_G)
                    zMapLogWarning("Styles file \"%s\": could not add style %s as it is already in the styles set.",
                                   styles_file, zMapStyleGetName(new_style)) ;

                  zMapStyleDestroy(new_style) ;
                }
            }
        } while((settings_list = g_list_next(settings_list)));
    }

  stylesFreeList(free_this_list) ;

  if (shapes)
    g_hash_table_destroy(shapes) ;


  /* NOTE we can only inherit default styles not those from another source */
  if (!zMapStyleInheritAllStyles(styles))
    zMapLogWarning("%s", "There were errors in inheriting styles.") ;


  /* this is not effective as a subsequent style copy will not copy this internal data */
  zMapStyleSetSubStyles(styles);


  if (styles)
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
  *p = 0;     /* there will always be room */

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

list<GQuark> zmapConfigString2QuarkListExtra(const char *string_list, gboolean cannonical,gboolean unique_id)
{
  list<GQuark> result;
  gchar **str_array = NULL,**strv;
  char *name;
  GQuark val;

  if (string_list)
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

              result.push_back(val) ;
            }
        }
    }

  if (str_array)
    g_strfreev(str_array);

  return result ;
}

/* Same as zmapConfigString2QuarkListExtra but returns a GList instead of std::list. */
GList *zmapConfigString2QuarkGListExtra(const char *string_list, gboolean cannonical,gboolean unique_id)
{
  GList *result = NULL;
  gchar **str_array = NULL,**strv;
  char *name;
  GQuark val;

  if (string_list)
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

              result = g_list_prepend(result,GUINT_TO_POINTER(val)) ;
            }
        }
    }

  result = g_list_reverse(result) ;      /* see glib doc for g_list_append() */

  if (str_array)
    g_strfreev(str_array);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMap_g_quark_list_print(result) ;     /* debug.... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return result ;
}

list<GQuark> zMapConfigString2QuarkList(const char *string_list, gboolean cannonical)
{
  return zmapConfigString2QuarkListExtra(string_list,cannonical,FALSE);
}

list<GQuark> zMapConfigString2QuarkIDList(const char *string_list)
{
  return zmapConfigString2QuarkListExtra(string_list,TRUE,TRUE);
}

GList *zMapConfigString2QuarkGList(const char *string_list, gboolean cannonical)
{
  return zmapConfigString2QuarkGListExtra(string_list,cannonical,FALSE);
}

GList *zMapConfigString2QuarkIDGList(const char *string_list)
{
  return zmapConfigString2QuarkGListExtra(string_list,TRUE,TRUE);
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
 *
 * NOTE the same hashtable is returned as passed in as the "hash" parameter.
 *
 *
 */
/*
 * NOTE we set up the columns->featureset list here, which is used to order heatmap(coverage) featuresets in a column
 * as well as for requesting featuresets via columns
 * ACEDB supplies a fset to column mapping later on which does not affect pipe servers (used for coverage/BAM)
 * and if this is aditional to that already configured it's added
 */

GHashTable *zMapConfigIniGetFeatureset2Column(ZMapConfigIniContext context, 
                                              GHashTable *ghash, 
                                              std::map<GQuark, ZMapFeatureColumn> &columns)
{
  GKeyFile *gkf;
  gchar ** keys,**freethis;
  GList *sources;
  ZMapFeatureSetDesc GFFset;
  GQuark column,column_id;
  char *names = NULL ;
  gsize len;
  char *normalkey;
  ZMapFeatureColumn f_col;

  zMapReturnValIfFail(context, ghash) ;

  if (zMapConfigIniHasStanza(context->config,ZMAPSTANZA_COLUMN_CONFIG,&gkf))
    {
      freethis = keys = g_key_file_get_keys(gkf,ZMAPSTANZA_COLUMN_CONFIG,&len,NULL) ;

      for( ; len-- ; keys++)
        {
          names = g_key_file_get_string(gkf,ZMAPSTANZA_COLUMN_CONFIG,*keys,NULL) ;

          if (!names || !*names)
            {
              if (!*names)    /* Can this even happen ? */
                g_free(names) ;

              continue ;
            }

          normalkey = zMapConfigNormaliseWhitespace(*keys,FALSE); /* changes in situ: get names first */
          column = g_quark_from_string(normalkey);
          column_id = zMapFeatureSetCreateID(normalkey);

          f_col = columns[column_id] ;

          if (!f_col)
            {
              f_col = (ZMapFeatureColumn) g_new0(ZMapFeatureColumnStruct,1);

              f_col->column_id = g_quark_from_string(normalkey);
              f_col->unique_id = column_id;
              f_col->column_desc = normalkey;
              f_col->order = zMapFeatureColumnOrderNext(FALSE);

              columns[column_id] = f_col ;
            }

#if MH17_USE_COLUMNS_HASH
          char *desc;
          /* add self ref to allow column lookup */
          GFFset = g_hash_table_lookup(ghash,GUINT_TO_POINTER(column_id));
          if(!GFFset)
            GFFset = g_new0(ZMapFeatureSetDescStruct,1);

          GFFset->column_id = column_id;        /* lower cased name */
          GFFset->column_ID = column;           /* display name */
          GFFset->feature_src_ID = column;      /* display name */

          /* add description if present */
          desc = normalkey;
          GFFset->feature_set_text = g_strdup(desc);
          g_hash_table_replace(ghash,GUINT_TO_POINTER(column_id),GFFset);
#endif
          sources = zMapConfigString2QuarkGList(names,FALSE);

          g_free(names);
          names = NULL ;

          while(sources)
            {
              /* get featureset if present */
              GQuark key = zMapFeatureSetCreateID((char *)
                  g_quark_to_string(GPOINTER_TO_UINT(sources->data)));
              GFFset = (ZMapFeatureSetDesc)g_hash_table_lookup(ghash,GUINT_TO_POINTER(key));

              if (!GFFset)
                GFFset = g_new0(ZMapFeatureSetDescStruct,1);

              GFFset->column_id = column_id;        /* lower cased name */
              GFFset->column_ID = column;           /* display name */
              /*zMapLogWarning("get f2c: set col ID %s",g_quark_to_string(GFFset->column_ID)); */
              GFFset->feature_src_ID = GPOINTER_TO_UINT(sources->data);    /* display name */

              g_hash_table_replace(ghash,GUINT_TO_POINTER(key),GFFset);

              /* construct reverse mapping from column to featureset */
              if (!g_list_find(f_col->featuresets_unique_ids,GUINT_TO_POINTER(key)))
                {
                  f_col->featuresets_unique_ids = g_list_append(f_col->featuresets_unique_ids,GUINT_TO_POINTER(key));
                }

              sources = g_list_delete_link(sources,sources);
            }
        }

      if (freethis)
        g_strfreev(freethis);
    }


  return ghash ;
}


/* to handle composite featuresets we map real ones to a pretend one
 * [featuresets]
 * composite = source1 ; source2 ;  etc
 *
 * we also have to patch in the featureset to column mapping as config does not do this
 * which will make display and column style tables work
 * NOTE the composite featureset does not exist at all and is just used to name a ZMapWindowgraphDensityItem
 */
GHashTable *zMapConfigIniGetFeatureset2Featureset(ZMapConfigIniContext context,
  GHashTable *fset_src, GHashTable *fset2col)
{
  GHashTable *virtual_featuresets = NULL ;
  GKeyFile *gkf;
  gchar ** keys, **freethis ;
  GList *sources;
  gsize len;
  GQuark set_id;
  char *names;
  ZMapFeatureSetDesc virtual_f2c;
  ZMapFeatureSetDesc real_f2c;

  zMapReturnValIfFail(context, virtual_featuresets) ;

  virtual_featuresets = g_hash_table_new(NULL, NULL) ;

  if(zMapConfigIniHasStanza(context->config, ZMAPSTANZA_FEATURESETS_CONFIG, &gkf))
    {
      freethis = keys = g_key_file_get_keys(gkf,ZMAPSTANZA_FEATURESETS_CONFIG,&len,NULL);

      for(;len--;keys++)
        {
          names = g_key_file_get_string(gkf,ZMAPSTANZA_FEATURESETS_CONFIG,*keys,NULL);

          if(!names || !*names)
            {
              if (!*names)    /* Can this even happen ? */
                g_free(names) ;

              continue ;
            }

          /* this featureset will not actually exist, it's virtual */
          set_id = zMapFeatureSetCreateID(*keys);

          if (fset2col && !(virtual_f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(fset2col, GUINT_TO_POINTER(set_id))))
            {
              /* should have been config'd to appear in a column */
              zMapLogWarning("cannot find virtual featureset %s",*keys);
              continue;
            }

          sources = zMapConfigString2QuarkGList(names,FALSE);
          g_free(names);
          names = NULL ;

          g_hash_table_insert(virtual_featuresets,GUINT_TO_POINTER(set_id), sources);

          while(sources)
            {
              ZMapFeatureSource f_src;
              // get featureset if present
              GQuark key = zMapFeatureSetCreateID((char *)
                  g_quark_to_string(GPOINTER_TO_UINT(sources->data)));
              f_src = (ZMapFeatureSource)g_hash_table_lookup(fset_src,GUINT_TO_POINTER(key));

              if(f_src)
                {
                  f_src->maps_to = set_id;

                  /* now set up featureset to column mapping to allow the column to paint and styles to be found */
                  if (fset2col && !(real_f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(fset2col, GUINT_TO_POINTER(key))))
                    {
                      real_f2c = g_new0(ZMapFeatureSetDescStruct,1);
                      g_hash_table_insert(fset2col,GUINT_TO_POINTER(key),real_f2c);
                    }

                  real_f2c->column_id = virtual_f2c->column_id;
                  real_f2c->column_ID = virtual_f2c->column_ID;
                  //  real_f2c->feature_set_text =
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


/* This loads the column-groups configuration and populates a hash table of the group name
 * (GQuark) to a GList of column names (GQuark)
 * 
 * [column-groups]
 * mygroup = column1 ; column2 ;  etc
 *
 */
GHashTable *zMapConfigIniGetColumnGroups(ZMapConfigIniContext context)
{
  GHashTable *column_groups = NULL ;
  list<GKeyFile*> gkf_list ;
  gchar ** keys, **freethis ;
  GList *columns;
  gsize len = 0;
  GQuark group_id = 0;
  char *names;

  zMapReturnValIfFail(context, column_groups) ;

  column_groups = g_hash_table_new(NULL, NULL) ;

  /* Get a list of all key files that contain the columm-groups stanza. We must check all of them
   * because different key-value pairs may exist in each and we want to merge them */
  if(zMapConfigIniHasStanzaAll(context->config, ZMAPSTANZA_COLUMN_GROUPS, gkf_list))
    {
      /* Loop through each key file. */
      for (auto gkf_iter = gkf_list.begin(); gkf_iter != gkf_list.end(); ++gkf_iter)
        {
          GKeyFile *gkf = *gkf_iter;

          /* Loop through all of the keys in the stanza */
          freethis = keys = g_key_file_get_keys(gkf,ZMAPSTANZA_COLUMN_GROUPS,&len,NULL);

          for(;len--;keys++)
            {
              /* Get the value, which is a list of column names */
              names = g_key_file_get_string(gkf,ZMAPSTANZA_COLUMN_GROUPS,*keys,NULL);

              if(!names || !*names)
                {
                  if (!*names)    /* Can this even happen ? */
                    g_free(names) ;

                  continue ;
                }

              /* Create the group ID from the key. This featureset will not actually exist, it's virtual */
              group_id = zMapStyleCreateID(*keys);

              /* Convert the list of names to a GList of column name quarks */
              columns = zMapConfigString2QuarkGList(names,FALSE);
              g_free(names);
              names = NULL ;

              /* Add the group name and columns list to the hash table of column_groups, if it is
               * not already there (higher priority key files are earlier in the list so we
               * discard duplicates in later, lower priority key files). */
              if (g_hash_table_lookup(column_groups,GUINT_TO_POINTER(group_id)))
                g_list_free(columns);
              else
                g_hash_table_insert(column_groups,GUINT_TO_POINTER(group_id), columns);
            }
        }

      if(freethis)
        g_strfreev(freethis);
    }

  return column_groups;
}


// get the complete list of columns to display, in order
// somewhere this gets mangled by strandedness
std::map<GQuark, ZMapFeatureColumn> *zMapConfigIniGetColumns(ZMapConfigIniContext context)
{
  std::map<GQuark, ZMapFeatureColumn> *columns_table = NULL ;
  list<GKeyFile*> gkf_list ;
  list<GQuark> columns ;
  gchar *colstr;
  ZMapFeatureColumn f_col;
  GHashTable *col_desc;
  char *desc;

  zMapReturnValIfFail(context, columns_table) ;

  columns_table = new std::map<GQuark, ZMapFeatureColumn> ;

  // nb there is a small memory leak if we config description for non-existant columns
  col_desc   = zMapConfigIniGetQQHash(context,ZMAPSTANZA_COLUMN_DESCRIPTION_CONFIG,QQ_STRING);

  /* Add the columns from the config file(s) */
  if(zMapConfigIniHasStanzaAll(context->config,ZMAPSTANZA_APP_CONFIG,gkf_list))
    {
      for (auto gkf_iter = gkf_list.begin(); gkf_iter != gkf_list.end(); ++gkf_iter)
        {
          colstr = g_key_file_get_string(*gkf_iter, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_COLUMNS,NULL);

          if(colstr && *colstr)
            {
              list<GQuark> new_list = zMapConfigString2QuarkList(colstr,FALSE) ;
              columns = zMapConfigIniMergeColumnsLists(new_list, columns) ;
            }
        }
    }

  if (columns.size() < 1)
    {
      /* Use default list of columns */
      columns = zMapConfigString2QuarkList(ZMAP_DEFAULT_FEATURESETS, FALSE) ;
    }

  /* Add the hard-coded strand-separator column at the start of the list */
  columns.push_front(g_quark_from_string(ZMAP_FIXED_STYLE_STRAND_SEPARATOR));

  /* Loop through the list and create the column struct, and add it to the columns_table */
  for(auto col = columns.begin(); col != columns.end(); ++col)
    {
      f_col = g_new0(ZMapFeatureColumnStruct,1);
      f_col->column_id = *col;

      desc = (char *) g_quark_to_string(f_col->column_id);
      f_col->unique_id = zMapFeatureSetCreateID(desc);

      f_col->column_desc = (char*)g_hash_table_lookup(col_desc, GUINT_TO_POINTER(f_col->column_id));
      if(!f_col->column_desc)
        f_col->column_desc = desc;

      f_col->order = zMapFeatureColumnOrderNext(FALSE) ;

      (*columns_table)[f_col->unique_id] = f_col ;
    }

  if(col_desc)
    g_hash_table_destroy(col_desc);

  return columns_table ;
}


/* Get the list of sources from the given config file / config string. Note that this should not
 * be called directly - use ZMapFeatureSequenceMap::addSourcesFromConfig instead so that sources
 * get added into the sources list. */
GList *zMapConfigGetSources(const char *config_file, const char *config_str, char ** stylesfile)
{
  GList *settings_list = NULL;
  ZMapConfigIniContext context ;

  if ((context = zMapConfigIniContextProvide(config_file, ZMAPCONFIG_FILE_NONE)))
    {

      if (config_str)
        zMapConfigIniContextIncludeBuffer(context, config_str);

      settings_list = zMapConfigIniContextGetSources(context);

      // For each source, record which config file it came from
      if (config_file)
        {
          for (GList *item = settings_list; item; item = item->next)
            {
              ZMapConfigSource config_source = (ZMapConfigSource)settings_list->data ;
              config_source->setConfigFile(config_file) ;
            }
        }

      if(stylesfile)
        {
          zMapConfigIniContextGetFilePath(context,
                                          ZMAPSTANZA_APP_CONFIG,ZMAPSTANZA_APP_CONFIG,
                                          ZMAPSTANZA_APP_STYLESFILE,stylesfile);
        }
      zMapConfigIniContextDestroy(context);

    }

  return(settings_list);
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
GHashTable *zMapConfigIniGetQQHash(ZMapConfigIniContext context, const char *stanza, int how)
{
  GHashTable *ghash = NULL;
  GKeyFile *gkf;
  gchar ** keys = NULL;
  gsize len;
  gchar *value,*strval;

  zMapReturnValIfFail(context, ghash) ;

  ghash = g_hash_table_new(NULL,NULL);

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
              g_hash_table_insert(ghash,kval,gval);
              g_free(value);
            }
        }
    }

  return(ghash);
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


/* return a hash table of GArray of GdkColor indexed by the GQuark of the name */
/* (there are limits to honw many colours we can use and GArrays are tedious) */
GHashTable *zMapConfigIniGetHeatmaps(ZMapConfigIniContext context)
{
  GHashTable *ghash = NULL;
  GKeyFile *gkf;
  gchar ** keys = NULL;
  gsize len;
  GQuark name;
  char *colours, *p,*q;
  GArray *col_array;
  GdkColor col;

  zMapReturnValIfFail(context, ghash ) ;

  if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_HEATMAP_CONFIG,&gkf))
    {
      ghash = g_hash_table_new(NULL,NULL);

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

          g_hash_table_insert(ghash,GUINT_TO_POINTER(name),col_array);

          g_free(colours);
        }
    }

  return(ghash);
}


gboolean zMapConfigLegacyStyles(char *config_file)
{
  static gboolean result = FALSE;
  ZMapConfigIniContext context;
  static int got = 0;

  /* this needs to be once only as it gets called when drawing glyphs */
  if (!got)
    {
      got = TRUE ;
      if ((context = zMapConfigIniContextProvide(config_file, ZMAPCONFIG_FILE_NONE)))
        {
          zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_LEGACY_STYLES, &result);
          zMapConfigIniContextDestroy(context);
        }
    }

  return(result) ;
}



/*
 *                    Internal routines
 */



static GList *contextGetStyleList(ZMapConfigIniContext context, char *styles_list, GKeyFile *extra_styles_keyfile)
{
  GList *glist = NULL;

  if (styles_list)
    {
      // get the named stanzas
      glist = zMapConfigIniContextGetListedStanzas(context, create_config_style,styles_list,"style");
    }
  else
    {
      // get all the stanzas
      glist = contextGetNamedStanzas(context, create_config_style, "style", extra_styles_keyfile) ;
    }

  return glist ;
}


/* return list of all stanzas in context->key_file[ZMAPCONFIG_FILE_STYLES] */
/* used when we don't have a styles list */
static GList *contextGetNamedStanzas(ZMapConfigIniContext context,
     ZMapConfigIniUserDataCreateFunc object_create_func,
     const char *stanza_type, GKeyFile *extra_styles_keyfile)
{
  GList *styles = NULL ;
  FetchReferencedStanzasStruct data = {NULL} ;
  gchar **names ;
  gchar **np ;


  data.context = context ;

  data.stanza  = get_stanza_with_type(context, stanza_type) ;

  data.object_create_func = object_create_func ;

  names = zMapConfigIniContextGetAllStanzaNames(context);

  if (extra_styles_keyfile)
    {
      /* If there's a styles file then only read that. */

      for (np = names; *np; np++)
        {
          fetchStanzas(&data, extra_styles_keyfile, *np) ;
        }
    }
  else if (names)
    {
      /* Otherwise read all available config files. */

      for (np = names; *np; np++)
        {
          fetch_referenced_stanzas((gpointer) *np, &data) ;
        }
    }

  g_strfreev(names);

  styles = data.object_list_out ;

  return styles;
}



static GHashTable *configIniGetGlyph(ZMapConfigIniContext context, GKeyFile *extra_styles_keyfile)
{
  GHashTable *ghash = NULL;
  GKeyFile *gkf;
  gchar ** keys = NULL;
  gsize len;
  char *shape;
  ZMapStyleGlyphShape glyph_shape;
  GQuark q;

  if (((gkf = extra_styles_keyfile) && g_key_file_has_group(gkf, ZMAPSTANZA_GLYPH_CONFIG))
      || (zMapConfigIniHasStanza(context->config, ZMAPSTANZA_GLYPH_CONFIG, &gkf)))
    {
      ghash = g_hash_table_new(NULL,NULL);

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
              g_hash_table_insert(ghash,GUINT_TO_POINTER(q),glyph_shape);
            }
          g_free(shape);
        }
    }

  return(ghash);
}


static void stylesFreeList(GList *config_styles_list)
{
  g_list_foreach(config_styles_list, free_style_list_item, NULL);
  g_list_free(config_styles_list);

  return ;
}



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



static ZMapConfigIniContextStanzaEntry get_stanza_with_type(ZMapConfigIniContext context, const char *stanza_type)
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



/* A GFunc() called directly but also by g_list_foreach() for a list of stanza names. */
static void fetch_referenced_stanzas(gpointer list_data, gpointer user_data)
{
  FetchReferencedStanzas full_data = (FetchReferencedStanzas)user_data;
  char *stanza_name = (char *)list_data;
  gboolean found = FALSE ;

  full_data->current_stanza_name = stanza_name;

  if (full_data && 
      zMapConfigIniHasStanza(full_data->context->config, stanza_name,NULL) &&
      full_data->object_create_func &&
      full_data->stanza)
    {
      if ((full_data->current_object = (full_data->object_create_func)(stanza_name)))
        {
          found = TRUE ;

          /* get stanza keys */
          g_list_foreach(full_data->stanza->keys, fill_stanza_key_value, user_data);

          full_data->object_list_out = g_list_append(full_data->object_list_out, full_data->current_object);
        }
    }

  if (!found)
    {
      zMapLogWarning("Object Create Function for stanza '%s'"
                     " failed to return anything", stanza_name);
    }

  return ;
}



/* Duplicates above function...rationalise....!!!!
 * in this case we don't want to search in all files as zMapConfigIniHasStanza() does, but just the given one. */
static void fetchStanzas(FetchReferencedStanzas full_data, GKeyFile *key_file, char *stanza_name)
{
  full_data->current_stanza_name = stanza_name ;

  if (g_key_file_has_group(key_file, stanza_name) && (full_data->object_create_func))
    {
      if ((full_data->current_object = (full_data->object_create_func)(stanza_name)))
        {
          /* get stanza keys */
          g_list_foreach(full_data->stanza->keys, fill_stanza_key_value, full_data) ;

          full_data->object_list_out = g_list_append(full_data->object_list_out, full_data->current_object);
        }
      else
        zMapLogWarning("Object Create Function for stanza '%s'"
               " failed to return anything", stanza_name);
    }

  return ;
}





static GList *get_child_stanza_names_as_list(ZMapConfigIniContext context,
                                             const char *parent_name, const char *parent_key)
{
  GList *glist = NULL;
  GValue *value = NULL;

  if (parent_name && parent_key &&  zMapConfigIniGetValue(context->config,
  parent_name, parent_key,
  &value, G_TYPE_POINTER))
    {
      char **strings_list = NULL;

      if ((strings_list = (char**)g_value_get_pointer(value)))
        {
          char **ptr = strings_list;

          while (ptr && *ptr)
            {
              *ptr = g_strstrip(*ptr) ;
              glist = g_list_append(glist, *ptr) ;
              ptr++ ;
            }

          g_free(strings_list);
        }
    }

  return glist;
}

static void free_source_names(gpointer list_data, gpointer unused_user_data)
{
  g_free(list_data);

  return ;
}

static GList *get_names_as_list(char *styles)
{
  GList *glist = NULL;

  char **strings_list = NULL;

  if ((strings_list = g_strsplit(styles,";",0)))
    {
      char **ptr = strings_list;

      while (ptr && *ptr)
        {
          *ptr = g_strstrip(*ptr) ;
          glist = g_list_append(glist, *ptr);
          ptr++ ;
        }

      g_free(strings_list);
    }
  return glist;
}


static void set_is_default(gpointer key, gpointer value, gpointer data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle) value;

  zMapStyleSetIsDefault(style);

  return ;
}



static ZMapConfigIniContextKeyEntry get_app_group_data(const char **stanza_name, const char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_APP_MAINWINDOW,         G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_ABBREV_TITLE,       G_TYPE_BOOLEAN, NULL, FALSE },

    { ZMAPSTANZA_APP_DATASET ,           G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_SEQUENCE,           G_TYPE_STRING,  NULL, FALSE },
    /* csname and csver are provided by otterlace with start and end coordinates
     * but are 'always' chromosome and Otter. see zmapApp.c/getConfiguration()
     */
    { ZMAPSTANZA_APP_CSNAME,             G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_CSVER,              G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_CHR,                G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_START,              G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_END,                G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_SLEEP,              G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_EXIT_TIMEOUT,       G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_HELP_URL,           G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_SESSION_COLOUR,           G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_LOCALE,             G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_COOKIE_JAR,         G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_PROXY,              G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_PORT,               G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_APP_PFETCH_MODE,        G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_PFETCH_LOCATION,    G_TYPE_STRING,  NULL, FALSE },
    //    { ZMAPSTANZA_APP_SCRIPTS,      G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_DATA,               G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_STYLESFILE,         G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_LEGACY_STYLES,      G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_COLUMNS,            G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_SOURCES,            G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_STYLE_FROM_METHOD,  G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_XREMOTE_DEBUG,      G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_CURL_DEBUG,         G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_CURL_IPRESOLVE,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_CURL_CAINFO,        G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_REPORT_THREAD,      G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_NAVIGATOR_SETS,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_SEQ_DATA,           G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_APP_SHRINKABLE,         G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_HIGHLIGHT_FILTERED, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_APP_ENABLE_ANNOTATION,  G_TYPE_BOOLEAN, NULL, FALSE },
    {NULL}
  };
  static const char *name = ZMAPSTANZA_APP_CONFIG;
  static const char *type = ZMAPSTANZA_APP_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}


static ZMapConfigIniContextKeyEntry get_peer_group_data(const char **stanza_name, const char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    { ZMAPSTANZA_PEER_NAME,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_PEER_CLIPBOARD,    G_TYPE_STRING,  NULL, FALSE },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    { ZMAPSTANZA_PEER_SOCKET,    G_TYPE_STRING,  NULL, FALSE },


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    { ZMAPSTANZA_PEER_RETRIES, G_TYPE_INT,     NULL, FALSE },
    { ZMAPSTANZA_PEER_TIMEOUT, G_TYPE_INT,     NULL, FALSE },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    { ZMAPSTANZA_PEER_TIMEOUT_LIST, G_TYPE_STRING,     NULL, FALSE },


    {NULL}
  };
  static const char *name = ZMAPSTANZA_PEER_CONFIG ;
  static const char *type = ZMAPSTANZA_PEER_CONFIG ;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys ;
}

static ZMapConfigIniContextKeyEntry get_logging_group_data(const char **stanza_name, const char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_LOG_LOGGING,      G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_FILE,         G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_SHOW_PROCESS, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_SHOW_CODE,    G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_SHOW_TIME,    G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_DIRECTORY,    G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_LOG_FILENAME,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_LOG_CATCH_GLIB,   G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_LOG_ECHO_GLIB,    G_TYPE_BOOLEAN, NULL, FALSE },
    {NULL}
  };

  static const char *name = ZMAPSTANZA_LOG_CONFIG;
  static const char *type = ZMAPSTANZA_LOG_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}

static ZMapConfigIniContextKeyEntry get_debug_group_data(const char **stanza_name, const char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_DEBUG_APP_THREADS, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_DEBUG_APP_FEATURE2STYLE, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_DEBUG_APP_STYLES, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_DEBUG_APP_TIMING, G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_DEBUG_APP_DEVELOP, G_TYPE_BOOLEAN, NULL, FALSE },
    { NULL }
  };
  static const char *name = ZMAPSTANZA_DEBUG_CONFIG;
  static const char *type = ZMAPSTANZA_DEBUG_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}



static gpointer create_config_style(gpointer data)
{
  gpointer config_struct = NULL ;

  ZMapKeyValueStruct style_conf[] =
    {
      { ZMAPSTYLE_PROPERTY_NAME, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_PARENT_STYLE, FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleCreateID} },
      { ZMAPSTYLE_PROPERTY_DESCRIPTION, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MODE, FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2Mode} },

      { ZMAPSTYLE_PROPERTY_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME0_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME1_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_FRAME2_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_REV_COLOURS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

      { ZMAPSTYLE_PROPERTY_DISPLAY_MODE, FALSE,  ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2ColDisplayState} },
      { ZMAPSTYLE_PROPERTY_BUMP_MODE, FALSE,  ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2BumpMode} },
      { ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, FALSE,  ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2BumpMode} },
      { ZMAPSTYLE_PROPERTY_BUMP_SPACING, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_BUMP_STYLE, FALSE,  ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_MIN_MAG, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MAX_MAG, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_WIDTH, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_SCORE_MODE, FALSE,  ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2ScoreMode} },
      { ZMAPSTYLE_PROPERTY_MIN_SCORE, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_MAX_SCORE, FALSE,  ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_SCORE_SCALE,  FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2Scale} },

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
      { ZMAPSTYLE_PROPERTY_FRAME_MODE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2ENUM,
        {(ZMapConfStr2EnumFunc)zMapStyleStr23FrameMode} },

      { ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_COL_FILTER, FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_COL_FILTER_TOLERANCE, FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_FOO,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_FILTER,FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_OFFSET,FALSE, ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },


      { ZMAPSTYLE_PROPERTY_GLYPH_NAME, FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)g_quark_from_string} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_5, FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)g_quark_from_string} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_5_REV, FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)g_quark_from_string} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5_REV,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_3, FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)g_quark_from_string} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_NAME_3_REV, FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)g_quark_from_string} },
      { ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3_REV,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GLYPH_STRAND,   FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphStrand} },
      { ZMAPSTYLE_PROPERTY_GLYPH_ALIGN,   FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GlyphAlign} },

      { ZMAPSTYLE_PROPERTY_GRAPH_MODE,   FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2GraphMode} },
      { ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,   FALSE, ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_SCALE,  FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2Scale} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_FIXED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_MIN_BIN,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_STAGGER,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_FILL,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_GRAPH_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },

      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALWAYS_GAPPED,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_UNIQUE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM,   FALSE, ZMAPCONF_STR, {FALSE},
                ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc) zMapStyleStr2BlixemType } },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,   FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN,   FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR, FALSE,  ZMAPCONF_STR, {FALSE},
        ZMAPCONV_STR2ENUM, {(ZMapConfStr2EnumFunc)zMapStyleStr2ColDisplayState} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_GAP_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_COMMON_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_MIXED_COLOURS,   FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_STR2COLOUR, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_MASK_SETS, FALSE, ZMAPCONF_STR, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_SQUASH, FALSE, ZMAPCONF_BOOLEAN, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_OVERLAP, FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_THRESHOLD, FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },
      { ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_MAX, FALSE, ZMAPCONF_INT, {FALSE}, ZMAPCONV_NONE, {NULL} },

      { ZMAPSTYLE_PROPERTY_TRANSCRIPT_TRUNC_LEN,   FALSE, ZMAPCONF_DOUBLE, {FALSE}, ZMAPCONV_NONE, {NULL} },

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
  /*case ZMAPCONF_STR_ARRAY:
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
static ZMapConfigIniContextKeyEntry get_style_group_data(const char **stanza_name, const char **stanza_type)
{
  static const char *name = "*";
  static const char *type = ZMAPSTANZA_STYLE_CONFIG;
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

    { ZMAPSTYLE_PROPERTY_SCORE_MODE,  G_TYPE_STRING,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MIN_SCORE,   G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_MAX_SCORE,   G_TYPE_DOUBLE,  style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_SCORE_SCALE, G_TYPE_STRING,  style_set_property, FALSE },

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

    { ZMAPSTYLE_PROPERTY_COL_FILTER,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_COL_FILTER_TOLERANCE,   G_TYPE_INT, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_FOO,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_FILTER,G_TYPE_BOOLEAN, style_set_property, FALSE },

    { ZMAPSTYLE_PROPERTY_OFFSET,G_TYPE_DOUBLE, style_set_property, FALSE },


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
    { ZMAPSTYLE_PROPERTY_GRAPH_FILL,   G_TYPE_BOOLEAN, style_set_property, FALSE },
    { ZMAPSTYLE_PROPERTY_GRAPH_COLOURS,   G_TYPE_STRING, style_set_property, FALSE },

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

    { ZMAPSTYLE_PROPERTY_TRANSCRIPT_TRUNC_LEN,   G_TYPE_DOUBLE, style_set_property, FALSE },

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
static void style_set_property(char *current_stanza_name, const char *key, GType type,
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



static gpointer create_config_source(gpointer data)
{
  ZMapConfigSource src ;
  const char *source_name = (const char*)data ;

  src = new ZMapConfigSourceStruct ;

  src->name_ = g_quark_from_string(source_name) ;
  src->group = SOURCE_GROUP_START ;                         // default_value

  return src ;
}

static void free_source_list_item(gpointer list_data, gpointer unused_data)
{
  ZMapConfigSource source_to_free = (ZMapConfigSource)list_data;
  delete source_to_free ;
}


/* This might seem a little long winded, but then so is all the gobject gubbins... */
static ZMapConfigIniContextKeyEntry get_source_group_data(const char **stanza_name, const char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { ZMAPSTANZA_SOURCE_URL,           G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_TIMEOUT,       G_TYPE_INT,     source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_VERSION,       G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_FEATURESETS,   G_TYPE_STRING,  source_set_property, FALSE },
    { ZMAPSTANZA_SOURCE_BIOTYPES,      G_TYPE_STRING,  source_set_property, FALSE },
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

  static const char *name = "*";
  static const char *type = ZMAPSTANZA_SOURCE_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}


static void source_set_property(char *current_stanza_name, const char *key, GType type,
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
        str_ptr = &(config_source->url_) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_VERSION) == 0)
        str_ptr = &(config_source->version) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_FEATURESETS) == 0)
        str_ptr = &(config_source->featuresets) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_BIOTYPES) == 0)
        str_ptr = &(config_source->biotypes) ;
      //      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_STYLES) == 0)
      //str_ptr = &(config_source->styles_list) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_REQSTYLES) == 0)
        bool_ptr = &(config_source->req_styles) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_STYLESFILE) == 0)
        str_ptr = &(config_source->stylesfile) ;
      //      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_NAVIGATORSETS) == 0)
      //str_ptr = &(config_source->navigatorsets) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_TIMEOUT) == 0)
        int_ptr = &(config_source->timeout) ;
      //      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_SEQUENCE) == 0)
      //bool_ptr = &(config_source->sequence) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_FORMAT) == 0)
        str_ptr = &(config_source->format) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_DELAYED) == 0)
        bool_ptr = &(config_source->delayed) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_MAPPING) == 0)
        bool_ptr = &(config_source->provide_mapping) ;
      else if (g_ascii_strcasecmp(key, ZMAPSTANZA_SOURCE_GROUP) == 0)
        {
          const char *value = "";

          int_ptr = &(config_source->group) ;

          *int_ptr = SOURCE_GROUP_NEVER;

          if (type == G_TYPE_STRING
              && ((value = g_value_get_string(property_value)) && *value))
            {
              if (!strcmp(value,ZMAPSTANZA_SOURCE_GROUP_ALWAYS))
                *int_ptr = SOURCE_GROUP_ALWAYS;
              else if(!strcmp(value,ZMAPSTANZA_SOURCE_GROUP_START))
                *int_ptr = SOURCE_GROUP_START;
              else if(!strcmp(value,ZMAPSTANZA_SOURCE_GROUP_DELAYED))
                *int_ptr = SOURCE_GROUP_DELAYED;
              else if(strcmp(value,ZMAPSTANZA_SOURCE_GROUP_NEVER))
                zMapLogWarning("Server stanza %s group option invalid",current_stanza_name);
            }

          return ;
        }

      if (type == G_TYPE_BOOLEAN && G_VALUE_TYPE(property_value) == type)
        *bool_ptr = g_value_get_boolean(property_value);
      else if (type == G_TYPE_INT && G_VALUE_TYPE(property_value) == type)
        *int_ptr = g_value_get_int(property_value);
      // there are no doubles
      //      else if (type == G_TYPE_DOUBLE && G_VALUE_TYPE(property_value) == type)
      //*double_ptr = g_value_get_double(property_value);
      else if (type == G_TYPE_STRING && G_VALUE_TYPE(property_value) == type)
        *str_ptr = (char *)g_value_get_string(property_value);
    }

  return ;
}



/* ZMapWindow */

static ZMapConfigIniContextKeyEntry get_window_group_data(const char **stanza_name, const char **stanza_type)
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
    { ZMAPSTANZA_WINDOW_BACKGROUND_COLOUR,         G_TYPE_STRING,  NULL, FALSE },
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
    { ZMAPSTANZA_WINDOW_RUBBER_BAND,  G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_WINDOW_HORIZON,      G_TYPE_STRING,  NULL, FALSE },
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
  static const char *name = ZMAPSTANZA_WINDOW_CONFIG;
  static const char *type = ZMAPSTANZA_WINDOW_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}



static ZMapConfigIniContextKeyEntry get_blixem_group_data(const char **stanza_name, const char **stanza_type)
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
    { ZMAPSTANZA_BLIXEM_SLEEP,       G_TYPE_BOOLEAN, NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_DNA_FS,      G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_PROT_FS,     G_TYPE_STRING,  NULL, FALSE },
    { ZMAPSTANZA_BLIXEM_FS,          G_TYPE_STRING,  NULL, FALSE },
    { NULL }
  };
  static const char *name = ZMAPSTANZA_BLIXEM_CONFIG;
  static const char *type = ZMAPSTANZA_BLIXEM_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}

