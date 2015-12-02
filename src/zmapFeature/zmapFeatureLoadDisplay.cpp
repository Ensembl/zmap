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


static gint colIDOrderCB(gconstpointer a, gconstpointer b,gpointer user_data) ;
static gint colOrderCB(gconstpointer a, gconstpointer b,gpointer user_data) ;


/*
 *              ZMapFeatureContextMap routines
 */


/* get the column struct for a featureset */
ZMapFeatureColumn zMapFeatureGetSetColumn(ZMapFeatureContextMap context_map,GQuark set_id)
{
  ZMapFeatureColumn column = NULL;
  ZMapFeatureSetDesc gff;

  char *name = (char *) g_quark_to_string(set_id);

  if(!context_map->featureset_2_column)
    {
      /* so that we can use autoconfigured servers */
      context_map->featureset_2_column = g_hash_table_new(NULL,NULL);
    }

  /* get the column the featureset goes in */
  gff = (ZMapFeatureSetDesc)g_hash_table_lookup(context_map->featureset_2_column,GUINT_TO_POINTER(set_id));
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
      g_hash_table_insert(context_map->featureset_2_column,GUINT_TO_POINTER(set_id),gff);
    }
  /*      else*/
  {
    map<GQuark, ZMapFeatureColumn>::iterator iter = context_map->columns->find(gff->column_id) ;

    if (iter != context_map->columns->end())
      {
        column = iter->second ;
      }
    else
      {
        ZMapFeatureSource gff_source;

        column = g_new0(ZMapFeatureColumnStruct,1);

        column->unique_id = column->column_id = set_id;

        column->order = zMapFeatureColumnOrderNext(FALSE);

        gff_source = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata,GUINT_TO_POINTER(set_id));
        column->column_desc = name;

        column->featuresets_unique_ids = g_list_append(column->featuresets_unique_ids,GUINT_TO_POINTER(set_id));
        (*context_map->columns)[set_id] = column ;
      }
  }

  return column;
}


GList *zMapFeatureGetColumnFeatureSets(ZMapFeatureContextMap context_map,GQuark column_id, gboolean unique_id)
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

  map<GQuark, ZMapFeatureColumn>::iterator col_iter = context_map->columns->find(column_id) ;

  if (col_iter != context_map->columns->end())
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
    zMap_g_hash_table_iter_init(&iter,context_map->featureset_2_column);
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
gboolean zMapFeatureIsCoverageColumn(ZMapFeatureContextMap context_map,GQuark column_id)
{
  ZMapFeatureSource src;
  GList *fsets;

  fsets = zMapFeatureGetColumnFeatureSets(context_map, column_id, TRUE);

  for (; fsets ; fsets = fsets->next)
    {
      src = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata,fsets->data);
      if(src && src->related_column)
      return TRUE;
    }

  return FALSE;
}


/* from column_id return whether it is configured from seq-data= featuresets (data side) */
gboolean zMapFeatureIsSeqColumn(ZMapFeatureContextMap context_map,GQuark column_id)
{
  ZMapFeatureSource src;
  GList *fsets;

  fsets = zMapFeatureGetColumnFeatureSets(context_map, column_id, TRUE);

  for (; fsets ; fsets = fsets->next)
    {
      src = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata,fsets->data);
      if(src && src->is_seq)
      return TRUE;
    }

  return FALSE;
}


gboolean zMapFeatureIsSeqFeatureSet(ZMapFeatureContextMap context_map,GQuark fset_id)
{
  ZMapFeatureSource src = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata,GUINT_TO_POINTER(fset_id));
  //zMapLogWarning("feature is_seq: %s -> %p", g_quark_to_string(fset_id),src);

  if(src && src->is_seq)
    return TRUE;
  return FALSE;

}


/* Get a GList of column IDs as GQuarks in the correct order according to the 'order' field in
 * each ZMapFeatureColumn struct (from the context_map.columns hash table).
 * Returns a new GList which should be free'd with g_list_free() */
GList* zMapFeatureGetOrderedColumnsListIDs(ZMapFeatureContextMap context_map)
{
  GList *columns = NULL ;
  zMapReturnValIfFail(context_map, columns) ;

  for (map<GQuark, ZMapFeatureColumn>::iterator iter = context_map->columns->begin() ; 
       iter != context_map->columns->end(); 
       ++iter)
    {
      columns = g_list_prepend(columns, GINT_TO_POINTER(iter->first));
    }

  columns = g_list_sort_with_data(columns, colIDOrderCB, context_map->columns);

  return columns ;
}


/* Get a GList of columns as ZMapFeatureColumn structs in the correct order according to 
 * the 'order' field in the struct (from the context_map.columns hash table).
 * Returns a new GList which should be free'd with g_list_free() */
GList* zMapFeatureGetOrderedColumnsList(ZMapFeatureContextMap context_map)
{
  GList *columns = NULL ;
  zMapReturnValIfFail(context_map, columns) ;

  for (map<GQuark, ZMapFeatureColumn>::iterator iter = context_map->columns->begin() ; 
       iter != context_map->columns->end(); 
       ++iter)
    {
      columns = g_list_prepend(columns, iter->second);
    }

  columns = g_list_sort_with_data(columns, colOrderCB, context_map->columns);

  return columns ;
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
void ZMapFeatureSequenceMapStructType::constructSources(const char *config_str,
                                                        char **stylesfile)
{
  // This will be the list of names from the sources stanza
  char *source_names = NULL ;

  // get any sources specified in the config file or the given config string
  GList *settings_list = zMapConfigGetSources(config_file, config_str, stylesfile) ;

  ZMapConfigIniContext context = zMapConfigIniContextProvide(config_file, ZMAPCONFIG_FILE_NONE) ;

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

          // Add it to the map (check it's not already there)
          if ((*sources)[source_name])
            zMapLogWarning("Source '%s' already exists; not adding", source_name.c_str()) ;
          else
            (*sources)[source_name] = (ZMapConfigSource)(source_item->data) ;
        }
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
