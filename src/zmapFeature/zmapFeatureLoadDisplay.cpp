/*  File: zmapFeatureLoadDisplay.cpp
 *  Author: Gemma Guest (gb10@sanger.ac.uk)
 *  Copyright (c) 2015: Genome Research Ltd.
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Interface for loading, manipulating, displaying
 *              sets of features.
 *
 * Exported functions: See ZMap/zmapFeatureLoadDisplay.hpp
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <unistd.h>

#include <zmapFeature_P.hpp>

#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>

using namespace std ;


typedef gboolean (*HashListExportValueFunc)(const char *key_str, const char *value_str) ;



/*
 *              Local function declarations
 */

static gint colIDOrderCB(gconstpointer a, gconstpointer b,gpointer user_data) ;
static gint colOrderCB(gconstpointer a, gconstpointer b,gpointer user_data) ;

static void updateContextHashList(ZMapConfigIniContext context,
                                  ZMapConfigIniFileType file_type, 
                                  const char *stanza,
                                  GHashTable *ghash,
                                  HashListExportValueFunc *export_func) ;

static ZMapFeatureContextExecuteStatus updateContextFeatureSetStyle(GQuark key,
                                                                    gpointer data,
                                                                    gpointer user_data,
                                                                    char **err_out) ;


/*
 *              ZMapFeatureContextMap routines
 */


/* get the column struct for a featureset */
ZMapFeatureColumn ZMapFeatureContextMapStructType::getSetColumn(GQuark set_id)
{
  ZMapFeatureColumn column = NULL;
  ZMapFeatureSetDesc gff;

  char *name = (char *) g_quark_to_string(set_id);

  if(!featureset_2_column)
    {
      /* so that we can use autoconfigured servers */
      featureset_2_column = g_hash_table_new(NULL,NULL);
    }

  /* get the column the featureset goes in */
  gff = (ZMapFeatureSetDesc)g_hash_table_lookup(featureset_2_column,GUINT_TO_POINTER(set_id));
  if(!gff)
    {
      //            zMapLogWarning("creating featureset_2_column for %s",name);
      /* recover from un-configured error
       * NOTE this occurs for seperator features eg DNA search
       * the style is predefined but the featureset and column are created
       * blindly with no reference to config
       * NOTE ideally these should be done along with getAllPredefined() styles
       */

      /* instant fix for a bug: DNA search fails to display seperator features */
      gff = g_new0(ZMapFeatureSetDescStruct,1);
      gff->column_id =
        gff->column_ID =
        gff->feature_src_ID = set_id;
      gff->feature_set_text = name;
      g_hash_table_insert(featureset_2_column,GUINT_TO_POINTER(set_id),gff);
    }
  /*      else*/
  {
    map<GQuark, ZMapFeatureColumn>::iterator iter = columns->find(gff->column_id) ;

    if (iter != columns->end())
      {
        column = iter->second ;
      }
    else
      {
        ZMapFeatureSource gff_source;

        column = g_new0(ZMapFeatureColumnStruct,1);

        column->unique_id = column->column_id = set_id;

        column->order = zMapFeatureColumnOrderNext(FALSE);

        gff_source = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata,GUINT_TO_POINTER(set_id));
        column->column_desc = name;

        column->featuresets_unique_ids = g_list_append(column->featuresets_unique_ids,GUINT_TO_POINTER(set_id));
        (*columns)[set_id] = column ;
      }
  }

  return column;
}


GList *ZMapFeatureContextMapStructType::getColumnFeatureSets(GQuark column_id, gboolean unique_id)
{
  GList *glist = NULL;
  ZMapFeatureSetDesc fset;
  ZMapFeatureColumn column = NULL;
  gpointer key;
  GList *iter;

  /*
   * This is hopelessly inefficient if we do this for every featureset, as ext_curated has about 1000
   * so we cache the glist when we first create it.
   * can't always do it on startup as acedb provides the mapping later on

   * NOTE see zmapWindowColConfig.c/column_is_loaded_in_range() for a comment about static or dynamic lists
   * also need to scan for all calls to this func since caching the data
   */

  map<GQuark, ZMapFeatureColumn>::iterator col_iter = columns->find(column_id) ;

  if (col_iter != columns->end())
      column = col_iter->second ;

  if(!column)
    return glist;

  if(unique_id)
    {
      if(column->featuresets_unique_ids)
        glist = column->featuresets_unique_ids;
    }
  else
    {
      if(column->featuresets_names)
        glist = column->featuresets_names;
    }

  if(!glist)
  {
    zMap_g_hash_table_iter_init(&iter,featureset_2_column);
    while(zMap_g_hash_table_iter_next(&iter,&key,(gpointer*)(&fset)))
      {
        if(fset->column_id == column_id)
          glist = g_list_prepend(glist,unique_id ? key : GUINT_TO_POINTER(fset->feature_src_ID));
      }
    if(unique_id)
      column->featuresets_unique_ids = glist;
    else
      column->featuresets_names = glist;
  }
  return glist;
}


/* from column_id return whether if is configured from seq-data= featuresets (coverage side) */
gboolean ZMapFeatureContextMapStructType::isCoverageColumn(GQuark column_id)
{
  ZMapFeatureSource src;
  GList *fsets;

  fsets = getColumnFeatureSets(column_id, TRUE);

  for (; fsets ; fsets = fsets->next)
    {
      src = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata,fsets->data);
      if(src && src->related_column)
      return TRUE;
    }

  return FALSE;
}


/* from column_id return whether it is configured from seq-data= featuresets (data side) */
gboolean ZMapFeatureContextMapStructType::isSeqColumn(GQuark column_id)
{
  ZMapFeatureSource src;
  GList *fsets;

  fsets = getColumnFeatureSets(column_id, TRUE);

  for (; fsets ; fsets = fsets->next)
    {
      src = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata,fsets->data);
      if(src && src->is_seq)
      return TRUE;
    }

  return FALSE;
}


gboolean ZMapFeatureContextMapStructType::isSeqFeatureSet(GQuark fset_id)
{
  ZMapFeatureSource src = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata,GUINT_TO_POINTER(fset_id));
  //zMapLogWarning("feature is_seq: %s -> %p", g_quark_to_string(fset_id),src);

  if(src && src->is_seq)
    return TRUE;
  return FALSE;

}


/* Get a GList of column IDs as GQuarks in the correct order according to the 'order' field in
 * each ZMapFeatureColumn struct (from the context_map.columns hash table).
 * Returns a new GList which should be free'd with g_list_free() */
GList* ZMapFeatureContextMapStructType::getOrderedColumnsListIDs()
{
  GList *result = NULL ;

  for (map<GQuark, ZMapFeatureColumn>::iterator iter = columns->begin() ; 
       iter != columns->end(); 
       ++iter)
    {
      result = g_list_prepend(result, GINT_TO_POINTER(iter->first));
    }

  result = g_list_sort_with_data(result, colIDOrderCB, columns);

  return result ;
}


/* Get a GList of columns as ZMapFeatureColumn structs in the correct order according to 
 * the 'order' field in the struct (from the context_map.columns hash table).
 * Returns a new GList which should be free'd with g_list_free() */
GList* ZMapFeatureContextMapStructType::getOrderedColumnsList()
{
  GList *result = NULL ;

  for (map<GQuark, ZMapFeatureColumn>::iterator iter = columns->begin() ; 
       iter != columns->end(); 
       ++iter)
    {
      result = g_list_prepend(result, iter->second);
    }

  result = g_list_sort_with_data(result, colOrderCB, columns);

  return result ;
}


/* Update the given context with the columns settings from the ContextMap */
bool ZMapFeatureContextMapStructType::updateContextColumns(_ZMapConfigIniContextStruct *context, 
                                                           ZMapConfigIniFileType file_type)
{
  zMapReturnValIfFail(context, FALSE) ;

  /* Update the context with the columns, if they need saving */
  GList *ordered_list = getOrderedColumnsListIDs() ;
  char *result = zMap_g_list_quark_to_string(ordered_list, NULL) ;

  zMapConfigIniContextSetString(context, file_type,
                                ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                ZMAPSTANZA_APP_COLUMNS, result);

  g_free(result) ;
  
  return TRUE ;
}

bool ZMapFeatureContextMapStructType::updateContextColumnGroups(_ZMapConfigIniContextStruct *context, 
                                                                ZMapConfigIniFileType file_type)
{
  /* Update the column-groups stanza if column groups have changed */
  updateContextHashList(context, file_type, ZMAPSTANZA_COLUMN_GROUPS, column_groups, NULL) ;

  return TRUE ;
}



/*
 *              ZMapFeatureSequenceMap routines
 */

ZMapFeatureSequenceMapStructType* ZMapFeatureSequenceMapStructType::copy()
{
  ZMapFeatureSequenceMapStructType *dest = NULL ;

  dest = g_new0(ZMapFeatureSequenceMapStruct, 1) ;

  dest->config_file = g_strdup(config_file) ;
  dest->stylesfile = g_strdup(stylesfile) ;
  dest->dataset = g_strdup(dataset) ;
  dest->sequence = g_strdup(sequence) ;
  dest->start = start ;
  dest->end = end ;
  dest->cached_parsers = cached_parsers ;
  dest->sources = sources ;

  for (int flag = 0; flag < ZMAPFLAG_NUM_FLAGS; ++flag)
    {
      dest->flags[flag] = flags[flag] ;
    }

  return dest ;
}


/* Add a source to our list of user-created sources. Sets the error if the name already exists. */
void ZMapFeatureSequenceMapStructType::addSource(const string &source_name, 
                                                 ZMapConfigSource source, 
                                                 GError **error)
{
  zMapReturnIfFail(source) ;

  if (sources && sources->find(source_name) != sources->end())
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99,
                  "Source '%s' already exists", source_name.c_str()) ;
    }
  else
    {
      if (!sources)
        sources = new map<string, ZMapConfigSource> ;

      (*sources)[source_name] = source ;
    }
}


/* Add a file source to our list of user-created sources. */
void ZMapFeatureSequenceMapStructType::addFileSource(const char *file)
{
  zMapReturnIfFail(file) ;

  /* Create the new source */
  ZMapConfigSource src = g_new0(ZMapConfigSourceStruct, 1) ;

  src->group = SOURCE_GROUP_START ;        // default_value
  src->featuresets = g_strdup(ZMAP_DEFAULT_FEATURESETS) ;

  if (strncasecmp(file, "http://", 7) != 0 && strncasecmp(file, "ftp://", 6) != 0)
    src->url = g_strdup_printf("file:///%s", file) ;

  /* Add the source to our list. Use the filename as the source name */
  string source_name(file) ;
  GError *error = NULL ;

  addSource(source_name, src, &error) ;

  if (error)
    {
      zMapLogWarning("Error creating source for file '%s': %s", file, error->message) ;
      g_error_free(error) ;
    }
}


/* Removet the source with the given name from our list. Sets the error if not found. */
void ZMapFeatureSequenceMapStructType::removeSource(const char *source_name_cstr, 
                                                    GError **error)
{
  string source_name(source_name_cstr) ;

  if (sources && sources->find(source_name) != sources->end())
    {
      sources->erase(source_name) ;
    }
  else
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99,
                  "Source '%s' does not exist", source_name_cstr) ;
    }
}


/* Get the ZMapConfigSource struct for the given source name. */
ZMapConfigSource ZMapFeatureSequenceMapStructType::getSource(string &source_name)
{
  ZMapConfigSource result = NULL ;

  if (sources)
    {
      map<string, ZMapConfigSource>::iterator iter = sources->find(source_name) ;

      if (iter != sources->end())
        result = iter->second ;
    }

  return result ;
}


/* Get the list of ZMapConfigSource structs from our sources map, returned in a glist. */
GList* ZMapFeatureSequenceMapStructType::getSources()
{
  GList *settings_list = NULL ;

  if (sources)
    {
      for (map<string, ZMapConfigSource>::iterator iter = sources->begin();
           iter != sources->end() ;
           ++iter)
        {
          settings_list = g_list_append(settings_list, iter->second) ;
        }
    }

  return settings_list ;
}


/* Construct the full list of all sources. this adds any sources from the config file or 
 * the given config string to those already stored in the sources list (from the command line or
 * user-defined sources). */
void ZMapFeatureSequenceMapStructType::constructSources(const char *filename,
                                                        const char *config_str,
                                                        char **stylesfile)
{
  // This will be the list of names from the sources stanza
  char *source_names = NULL ;

  // get any sources specified in the config file or the given config string
  GList *settings_list = zMapConfigGetSources(filename, config_str, stylesfile) ;

  ZMapConfigIniContext context = zMapConfigIniContextProvide(filename, ZMAPCONFIG_FILE_NONE) ;

  if (context && config_str)
    zMapConfigIniContextIncludeBuffer(context, config_str);

  if (context && 
      settings_list && g_list_length(settings_list) > 0 &&
      zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG, 
                                    ZMAPSTANZA_APP_SOURCES, &source_names))
    {
      // Create the sources map if it doesn't exist
      if (!sources)
        sources = new map<string, ZMapConfigSource> ;

      GList *names_list = zMapConfigString2QuarkIDList(source_names) ;

      // loop through all config sources
      GList *source_item = settings_list ;
      GList *name_item = names_list ;

      for ( ; source_item && name_item; source_item = g_list_next(source_item), name_item = g_list_next(name_item))
        {
          string source_name(g_quark_to_string(GPOINTER_TO_INT(name_item->data))) ;

          (*sources)[source_name] = (ZMapConfigSource)(source_item->data) ;
        }
    }
}


void ZMapFeatureSequenceMapStructType::constructSources(const char *config_str,
                                                        char **stylesfile)
{
  constructSources(config_file, config_str, stylesfile) ;
}


/* Update the given context with any settings that need saving from the sequenceMap. Returns true
 * if the context was changed. */
bool ZMapFeatureSequenceMapStructType::updateContext(ZMapConfigIniContext context, 
                                                     ZMapConfigIniFileType file_type)
{
  bool changed = FALSE ;

  if (sources && getFlag(ZMAPFLAG_SAVE_SOURCES))
    {
      std::string sources_str ;

      for (std::map<std::string, ZMapConfigSource>::const_iterator iter = sources->begin(); iter != sources->end(); ++iter)
        {
          std::string source_name = iter->first ;
          ZMapConfigSource source = iter->second ;

          /* Append to the list of sources */
          if (sources_str.length() == 0)
            sources_str += source_name ;
          else
            sources_str += "; " + source_name ;

          /* Set the values in the source stanza */
          if (source->url)
            zMapConfigIniContextSetString(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_URL, source->url) ;

          if (source->featuresets)
            zMapConfigIniContextSetString(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_FEATURESETS, source->featuresets) ;

          if (source->biotypes)
            zMapConfigIniContextSetString(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_BIOTYPES, source->biotypes) ;

          if (source->version)
            zMapConfigIniContextSetString(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_VERSION, source->version) ;

          if (source->stylesfile)
            zMapConfigIniContextSetString(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_STYLESFILE, source->stylesfile) ;

          if (source->format)
            zMapConfigIniContextSetString(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_FORMAT, source->format) ;

          zMapConfigIniContextSetInt(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_TIMEOUT, source->timeout) ;
          zMapConfigIniContextSetInt(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_GROUP, source->group) ;

          zMapConfigIniContextSetBoolean(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_DELAYED, source->delayed) ;
          zMapConfigIniContextSetBoolean(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_MAPPING, source->provide_mapping) ;
          zMapConfigIniContextSetBoolean(context, file_type, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, ZMAPSTANZA_SOURCE_REQSTYLES, source->req_styles) ;
        }

      /* Set the list of sources for the ZMap stanza */
      zMapConfigIniContextSetString(context, file_type,
                                    ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                    ZMAPSTANZA_APP_SOURCES, sources_str.c_str()) ;

      changed = TRUE ;
    }

  return changed ;
}


void ZMapFeatureSequenceMapStructType::setFlag(ZMapFlag flag, 
                                               const gboolean value)
{
  zMapReturnIfFail(flag >= 0 && flag < ZMAPFLAG_NUM_FLAGS) ;

  flags[flag] = value ;

}


gboolean ZMapFeatureSequenceMapStructType::getFlag(ZMapFlag flag)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(flag >= 0 && flag < ZMAPFLAG_NUM_FLAGS, result) ;

  result = flags[flag] ;

  return result ;
}


/*
 *              External routines.
 */

/* Update the given context with any configuration values that have been changed, e.g. column order */
void zMapFeatureUpdateContext(ZMapFeatureContextMap context_map,
                              ZMapFeatureSequenceMap sequence_map,
                              ZMapFeatureAny feature_any,
                              ZMapConfigIniContext context, 
                              ZMapConfigIniFileType file_type)
{
  gboolean changed = FALSE ;
  zMapReturnIfFail(sequence_map) ;

  if (context_map)
    {
      if (sequence_map->getFlag(ZMAPFLAG_SAVE_COLUMNS))
        changed |= context_map->updateContextColumns(context, file_type) ;
    
      if (sequence_map->getFlag(ZMAPFLAG_SAVE_COLUMN_GROUPS))
        changed |= context_map->updateContextColumnGroups(context, file_type) ;
    }

  if (sequence_map)
    changed |= sequence_map->updateContext(context, file_type) ;

  /* Update the featureset-style stanza if featureset->style relationshiops have changed */
  if (sequence_map->getFlag(ZMAPFLAG_SAVE_FEATURESET_STYLE))
    {
      GKeyFile *gkf = zMapConfigIniGetKeyFile(context, file_type) ;

      zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_FEATURESET, 
                                updateContextFeatureSetStyle, gkf) ;

      changed = TRUE ;
    }
  
  /* Set the unsaved flag in the context if there were any changes, and reset the individual flags */
  if (changed)
    {
      zMapConfigIniContextSetUnsavedChanges(context, file_type, TRUE) ;

      sequence_map->setFlag(ZMAPFLAG_SAVE_COLUMNS, FALSE) ;
      sequence_map->setFlag(ZMAPFLAG_SAVE_COLUMN_GROUPS, FALSE) ;
      sequence_map->setFlag(ZMAPFLAG_SAVE_SOURCES, FALSE) ;
      sequence_map->setFlag(ZMAPFLAG_SAVE_FEATURESET_STYLE, FALSE) ;
    }
}



/*
 *              Internal routines.
 */

static gint colIDOrderCB(gconstpointer a, gconstpointer b,gpointer user_data)
{
  ZMapFeatureColumn pa = NULL,pb = NULL;
  map<GQuark, ZMapFeatureColumn> *columns = (map<GQuark, ZMapFeatureColumn>*)user_data;

  if (columns)
    {
      GQuark quark1 = (GQuark)GPOINTER_TO_INT(a) ;
      GQuark quark2 = (GQuark)GPOINTER_TO_INT(b) ;
      map<GQuark, ZMapFeatureColumn>::iterator iter1 = columns->find(quark1) ;
      map<GQuark, ZMapFeatureColumn>::iterator iter2 = columns->find(quark2) ;

      if (iter1 != columns->end())
        pa = iter1->second ;

      if (iter2 != columns->end())
        pb = iter2->second ;

      if(pa && pb)
        {
          if(pa->order < pb->order)
            return(-1);
          if(pa->order > pb->order)
            return(1);
        }
    }

  return(0);
}


static gint colOrderCB(gconstpointer a, gconstpointer b,gpointer user_data)
{
  ZMapFeatureColumn pa = NULL,pb = NULL;
  map<GQuark, ZMapFeatureColumn> *columns = (map<GQuark, ZMapFeatureColumn>*) user_data;

  if (columns)
    {
      GQuark quark1 = (GQuark)GPOINTER_TO_INT(a) ;
      GQuark quark2 = (GQuark)GPOINTER_TO_INT(b) ;
      map<GQuark, ZMapFeatureColumn>::iterator iter1 = columns->find(quark1) ;
      map<GQuark, ZMapFeatureColumn>::iterator iter2 = columns->find(quark2) ;

      if (iter1 != columns->end())
        pa = iter1->second ;

      if (iter2 != columns->end())
        pb = iter2->second ;

      if(pa && pb)
        {
          if(pa->order < pb->order)
            return(-1);
          if(pa->order > pb->order)
            return(1);
        }
    }

  return(0);
}

/* Update the given context with the given hash table of ids mapped to a glist of ids. This is
 * exported as key=value where the hash table id is the key and the value is a
 * semi-colon-separated list of the ids from the glist. export_func is an optional function that
 * takes the key and glist-value and returns true if the value should be included in the context, false if
 * not. If this function is null then all values are included.  */
static void updateContextHashList(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, 
                                  const char *stanza, GHashTable *ghash, HashListExportValueFunc *export_func)
{
  zMapReturnIfFail(context && context->config) ;

  GKeyFile *gkf = zMapConfigIniGetKeyFile(context, file_type) ;

  if (gkf)
    {
      /* Loop through all entries in the hash table */
      GList *iter = NULL ;
      gpointer key = NULL,value = NULL;

      zMap_g_hash_table_iter_init(&iter, ghash) ;

      while(zMap_g_hash_table_iter_next(&iter, &key, &value))
        {
          const char *key_str = g_quark_to_string(GPOINTER_TO_INT(key)) ;

          if (key_str)
            {
              GString *values_str = NULL ;

              for (GList *item = (GList*)value ; item ; item = item->next) 
                {
                  const char *value_str = g_quark_to_string(GPOINTER_TO_INT(item->data)) ;

                  if (!export_func || (*export_func)(key_str, value_str))
                    {
                      if (!values_str)
                        values_str = g_string_new(value_str) ;
                      else
                        g_string_append_printf(values_str, " ; %s", value_str) ;
                    }
                }

              if (values_str)
                {
                  g_key_file_set_string(gkf, stanza, key_str, values_str->str) ;
                  g_string_free(values_str, TRUE) ;
                }
            }
        }
    }
}

/* Callback called for all featuresets to set the key-value pair for the featureset-style stanza
 * in the given key file. Note that featuresets that have their default are not included. */
static ZMapFeatureContextExecuteStatus updateContextFeatureSetStyle(GQuark key,
                                                                    gpointer data,
                                                                    gpointer user_data,
                                                                    char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  GKeyFile *gkf = (GKeyFile *)user_data ;

  if (gkf && 
      feature_any && 
      feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET &&
      ((ZMapFeatureSet)feature_any)->style)
    {
      ZMapFeatureSet featureset = (ZMapFeatureSet)feature_any ;
      const char *key_str = g_quark_to_string(featureset->unique_id) ;
      const char *value_str = g_quark_to_string(featureset->style->unique_id) ;
          
      /* Only export if value is different to key (otherwise the style name is the
       * same as the column name, and this stanza doesn't add anything). Also don't
       * export if it's a default style. */
      if (featureset->style && featureset->unique_id != featureset->style->unique_id &&
          strcmp(value_str, "invalid") &&
          strcmp(value_str, "basic") &&
          strcmp(value_str, "alignment") &&
          strcmp(value_str, "transcript") &&
          strcmp(value_str, "sequence") &&
          strcmp(value_str, "assembly-path") &&
          strcmp(value_str, "text") &&
          strcmp(value_str, "graph") &&
          strcmp(value_str, "glyph") &&
          strcmp(value_str, "plain") &&
          strcmp(value_str, "meta") &&
          strcmp(value_str, "search hit marker"))
        {
          key_str = g_quark_to_string(featureset->original_id) ;
          value_str = g_quark_to_string(featureset->style->original_id) ;

          g_key_file_set_string(gkf, ZMAPSTANZA_FEATURESET_STYLE_CONFIG, key_str, value_str) ;
        }
    }

  return status ;
}

