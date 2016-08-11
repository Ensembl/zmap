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
#include <sstream>

#include <zmapFeature_P.hpp>

#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapUrl.hpp>

using namespace std ;


typedef gboolean (*HashListExportValueFunc)(const char *key_str, const char *value_str) ;



/*
 *              Local function declarations
 */

// unnamed namespace
namespace
{
gint colIDOrderCB(gconstpointer a, gconstpointer b,gpointer user_data) ;
bool colOrderCB(const ZMapFeatureColumn &first, const ZMapFeatureColumn &second) ;

void updateContextHashList(ZMapConfigIniContext context,
                           ZMapConfigIniFileType file_type, 
                           const char *stanza,
                           GHashTable *ghash,
                           HashListExportValueFunc *export_func) ;

ZMapFeatureContextExecuteStatus updateContextFeatureSetStyle(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **err_out) ;

}

/**********************************************************************
 *                       ZMapFeatureContextMap
 **********************************************************************/


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

        gff_source = getSource(set_id) ;
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
  gboolean result = FALSE ;
  ZMapFeatureSource src;
  GList *fsets;

  fsets = getColumnFeatureSets(column_id, TRUE);

  for (; fsets ; fsets = fsets->next)
    {
      GQuark fset_id = (GQuark)GPOINTER_TO_INT(fsets->data) ;
      src = getSource(fset_id) ;

      if(src && src->related_column)
        {
          result = TRUE;
          break ;
        }
    }

  return result;
}


/* from column_id return whether it is configured from seq-data= featuresets (data side) */
gboolean ZMapFeatureContextMapStructType::isSeqColumn(GQuark column_id)
{
  gboolean result = FALSE ;
  ZMapFeatureSource src;
  GList *fsets;

  fsets = getColumnFeatureSets(column_id, TRUE);

  for (; fsets ; fsets = fsets->next)
    {
      GQuark fset_id = (GQuark)GPOINTER_TO_INT(fsets->data) ;
      src = getSource(fset_id) ;

      if(src && src->is_seq)
        {
          result = TRUE;
          break ;
        }
    }

  return result;
}


gboolean ZMapFeatureContextMapStructType::isSeqFeatureSet(GQuark fset_id)
{
  gboolean result = FALSE ;
  ZMapFeatureSource src = getSource(fset_id) ;

  if(src && src->is_seq)
    result = TRUE;

  return result;
}


ZMapFeatureSource ZMapFeatureContextMapStructType::getSource(GQuark fset_id)
{
  ZMapFeatureSource src = NULL ;
  zMapReturnValIfFail(fset_id, src) ;

  src = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata, GUINT_TO_POINTER(fset_id)) ;

  return src ;
}


void ZMapFeatureContextMapStructType::setSource(GQuark fset_id, ZMapFeatureSource src)
{
  zMapReturnIfFail(fset_id) ;
  
  if (getSource(fset_id))
    g_hash_table_replace(source_2_sourcedata, GUINT_TO_POINTER(fset_id), src) ;
  else
    g_hash_table_insert(source_2_sourcedata, GUINT_TO_POINTER(fset_id), src) ;
}


ZMapFeatureSource ZMapFeatureContextMapStructType::createSource(const GQuark fset_id, 
                                                                const GQuark source_id,
                                                                const GQuark source_text,
                                                                const GQuark style_id,
                                                                const GQuark related_column,
                                                                const GQuark maps_to,
                                                                const bool is_seq)
{
  ZMapFeatureSource src = NULL ;
  zMapReturnValIfFail(fset_id, src) ;
  
  src = g_new0(ZMapFeatureSourceStruct, 1) ;
  
  src->source_id = source_id ;
  src->source_text = source_text ; 
  src->style_id = style_id ;
  src->related_column = related_column ;
  src->maps_to = maps_to ;
  src->is_seq = is_seq ;

  setSource(fset_id, src) ;
  
  return src ;
}


/* Get a GList of column IDs as GQuarks in the correct order according to the 'order' field in
 * each ZMapFeatureColumn struct (from the context_map.columns hash table).
 * Returns a new GList which should be free'd with g_list_free() */
list<GQuark> ZMapFeatureContextMapStructType::getOrderedColumnsListIDs(const bool unique)
{
  list<GQuark> result ;

  /* We can't easily get a sorted list of IDs from the map that are sorted by column order
   * number. This isn't likely to be performance-critical so just get the ordered list of structs
   * and loop through that to get the IDs */
  list<ZMapFeatureColumn> col_list = getOrderedColumnsList() ;
  for (auto iter = col_list.begin(); iter != col_list.end(); ++iter)
    {
      ZMapFeatureColumn col = *iter ;
      if (unique)
        result.push_back(col->unique_id) ;
      else
        result.push_back(col->column_id) ;
    }

  return result ;
}

/* Same as getOrdererdColumnsListIDs but returns a GList */
GList* ZMapFeatureContextMapStructType::getOrderedColumnsGListIDs()
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
list<ZMapFeatureColumn> ZMapFeatureContextMapStructType::getOrderedColumnsList()
{
  list<ZMapFeatureColumn> result ;

  for (map<GQuark, ZMapFeatureColumn>::iterator iter = columns->begin() ; 
       iter != columns->end(); 
       ++iter)
    {
      result.push_back(iter->second);
    }

  result.sort(colOrderCB);

  return result ;
}


/* Update the given context with the columns settings from the ContextMap */
bool ZMapFeatureContextMapStructType::updateContextColumns(_ZMapConfigIniContextStruct *context, 
                                                           ZMapConfigIniFileType file_type)
{
  zMapReturnValIfFail(context, FALSE) ;

  /* Get the new order of columns in zmap */
  list<GQuark> ordered_list = getOrderedColumnsListIDs(false) ;

  /* We need to preserve any old order in the file if it is the user prefs file. That is because
   * there may be columns they've previously saved that aren't in the current zmap and we don't
   * want to wipe that out, so merge the new order with the old list */
  if (file_type == ZMAPCONFIG_FILE_PREFS)
    {
      char *value = NULL ;
      zMapConfigIniContextGetString(context,
                                    ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                    ZMAPSTANZA_APP_COLUMNS, &value);
      if (value)
        {
          list<GQuark> old_list = zMapConfigString2QuarkList(value, FALSE) ;
          ordered_list = zMapConfigIniMergeColumnsLists(ordered_list, old_list) ;
        }
    }

  stringstream result;
  bool first = true;
  for(auto iter = ordered_list.begin(); iter != ordered_list.end(); ++iter)
    {
      /* Don't save the strand separator order - its position is always the same */
      if (*iter != g_quark_from_string(ZMAP_FIXED_STYLE_STRAND_SEPARATOR))
        {
          if(!first)
            result << ";" ;

          result << g_quark_to_string(*iter) ;
          first = false ;
        }
    }

  zMapConfigIniContextSetString(context, file_type,
                                ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                ZMAPSTANZA_APP_COLUMNS, result.str().c_str());

  return TRUE ;
}


bool ZMapFeatureContextMapStructType::updateContextColumnGroups(_ZMapConfigIniContextStruct *context, 
                                                                ZMapConfigIniFileType file_type)
{
  /* Update the column-groups stanza if column groups have changed */
  updateContextHashList(context, file_type, ZMAPSTANZA_COLUMN_GROUPS, column_groups, NULL) ;

  return TRUE ;
}



/* If this featureset has a related column, then return the ID of that column.
 * Return 0 otherwise.
 * FTM this means fset is coverage and the related is the real data (a one way relation)
 */
GQuark ZMapFeatureContextMapStructType::getRelatedColumnID(const GQuark fset_id)
{
  GQuark q = 0;
  ZMapFeatureSource src = NULL ;

  src = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata,GUINT_TO_POINTER(fset_id));
  if(src)
    q = src->related_column;
  else
    zMapLogWarning("Can't find source data for featureset '%s'.",g_quark_to_string(fset_id));

  return q;
}





/**********************************************************************
 *                      ZMapFeatureSequenceMap
 **********************************************************************/

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


/* Add a source to our list of user-created sources. Sets the error if the name already exists and
 * the source is different to the existing one. If this is a duplicate of an existing source then
 * it frees the given source and returns the original. */
ZMapConfigSource ZMapFeatureSequenceMapStructType::addSource(const string &source_name, 
                                                             ZMapConfigSource source, 
                                                             GError **error)
{
  ZMapConfigSource result = source ;

  // Check if the source has already been added.
  ZMapConfigSource existing = getSource(source_name) ;

  if (existing)
    {
      if (source == existing)
        {
          // Same pointer as existing; nothing to do; already have ownership
        }
      else if (source->url == existing->url ||
               (source->url && existing->url && strcmp(source->url, existing->url) == 0))
        {
          // Different pointer but duplicate - free the given source and return the existing one.
          zMapConfigSourceDestroy(source) ;
          result = existing ;
        }
      else
        {
          // Different source with same name; error. Return the given source so the caller can
          // decide what to do with it.
          g_set_error(error, g_quark_from_string("ZMap"), 99,
                      "Source '%s' already exists", source_name.c_str()) ;
          result = source ;
        }
    }
  else
    {
      /* Doesn't exist yet. Add it and return the given source. */
      if (!sources)
        sources = new map<string, ZMapConfigSource> ;

      (*sources)[source_name] = source ;

      result = source ;
    }

  return source ;
}


/* Create a source with the given details. If allow_duplicate is true then return the existing
 * source with the same name if one exists; otherwise, give an error if the source name already exists */
ZMapConfigSource ZMapFeatureSequenceMapStructType::createSource(const char *source_name, 
                                                                const std::string &url, 
                                                                const char *featuresets,
                                                                const char *biotypes,
                                                                const bool is_child,
                                                                const bool allow_duplicate,
                                                                GError **error)
{
  return createSource(source_name, url.c_str(), featuresets, biotypes, is_child, error) ;
}


ZMapConfigSource ZMapFeatureSequenceMapStructType::createSource(const char *source_name,
                                                                const char *url,
                                                                const char *featuresets,
                                                                const char *biotypes,
                                                                const bool is_child,
                                                                const bool allow_duplicate,
                                                                GError **error)
{
  ZMapConfigSource source = NULL ;
  zMapReturnValIfFail(url, source) ;

  // Check if there's already a source with this name and return that if found
  source = getSource(source_name) ;

  if (!source)
    {
      GError *tmp_error = NULL ;
      source = new ZMapConfigSourceStruct ;

      source->name_ = g_quark_from_string(source_name) ;
      source->url = g_strdup(url) ;
      source->recent = true ;
      
      if (featuresets && *featuresets)
        source->featuresets = g_strdup(featuresets) ;

      if (biotypes && *biotypes)
        source->biotypes = g_strdup(biotypes) ;

      /* Add the new source to the view */
      std::string source_name_str(source_name) ;

      /* Add it to the sources list (unless it's a child, in which case it will be acccessed via its
       * parent hierarchy) */
      if (!is_child)
        source = addSource(source_name_str, source, &tmp_error) ;

      /* Recursively create any child sources */
      if (!tmp_error)
        createSourceChildren(source, NULL, &tmp_error) ;

      /* Indicate that there are changes that need saving */
      if (!tmp_error)
        setFlag(ZMAPFLAG_SAVE_SOURCES, TRUE) ;

      if (tmp_error)
        {
          zMapConfigSourceDestroy(source) ;
          source = NULL ;
          g_propagate_error(error, tmp_error) ;
        }
    }
  else if (!allow_duplicate)
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99,
                  "Cannot add source: source name '%s' already exists", 
                  source_name) ;
    }

  return source ;
}


/* Some sources (namely trackhub) may have child tracks. This function creates a source for each
 * child track. */
void ZMapFeatureSequenceMapStructType::createSourceChildren(ZMapConfigSource source,
                                                            GList **results_out,
                                                            GError **error)
{
  ZMapURL zmap_url = url_parse(source->url, NULL);

  if (zmap_url && zmap_url->scheme == SCHEME_TRACKHUB)
    {
      const char *trackdb_id = zmap_url->file ;

      if (trackdb_id)
        {
          string err_msg;
          gbtools::trackhub::Registry registry ;
          gbtools::trackhub::TrackDb trackdb = registry.searchTrackDb(trackdb_id, err_msg) ;

          if (err_msg.empty())
            {
              // Create a source for each individual track in this trackdb
              for (auto &track : trackdb.tracks())
                {
                  createTrackhubSourceChild(source, track, results_out) ;
                }
            }
          else
            {
              g_set_error(error, g_quark_from_string("ZMap"), 99,
                          "Error getting tracks for trackDb '%s': %s", 
                          trackdb_id, err_msg.c_str()) ;
            }
        }
      else
        {
          g_set_error(error, g_quark_from_string("ZMap"), 99,
                      "Error getting trackDb from URL: %s", source->url) ;
        }
    }
}


/* Recursively create a server for a given trackhub track and its children */
void ZMapFeatureSequenceMapStructType::createTrackhubSourceChild(ZMapConfigSource parent_source,
                                                                 const gbtools::trackhub::Track &track,
                                                                 GList **results_out)
{
  zMapReturnIfFail(parent_source) ;

  // Create the server for this track. Note that some Tracks may be parents which
  // don't have a url themselves so they won't actually load any data but are added to maintain
  // the hierarchy.
  string track_name = track.name() ;

  // If the track doesn't have a name, create one from the file name in the url
  if (track_name.empty())
    {
      ZMapURL zmap_url = url_parse(track.url().c_str(), NULL) ;

      if (zmap_url && zmap_url->file)
        track_name = string(zmap_url->file) ;
    }

  if (!track_name.empty())
    {
      GError *g_error = NULL ;

      ZMapConfigSource new_source = createSource(track.name().c_str(),
                                                 track.url().c_str(), 
                                                 parent_source->featuresets, 
                                                 parent_source->biotypes,
                                                 true, false,
                                                 &g_error) ;

      if (results_out && !track.url().empty())
        {
          *results_out = g_list_append(*results_out, new_source) ;
        }

      if (new_source && !g_error)
        {
          // Success. Set the delayed flag if the track should be hidden by default.
          // Set the parent pointer and add the child to the parent's list.
          new_source->delayed = !track.visible() ;
          new_source->parent = parent_source ;
          parent_source->children.push_back(new_source) ;

          // Recurse through any child tracks
          for (auto &child_track : track.children())
            {
              createTrackhubSourceChild(new_source, child_track, results_out) ;
            }
        }
      else if (g_error)
        {
          zMapLogWarning("Error setting up server for track %s: %s\n%s", 
                         track_name.c_str(), track.url().c_str(), g_error->message) ;
          g_error_free(g_error) ;
        }
      else
        {
          zMapLogWarning("Error setting up server for track %s: %s", track_name.c_str(), track.url().c_str()) ;
        }

    }
  else
    {
      zMapLogWarning("Error setting up server; track has no name: %s", track.url().c_str()) ;
    }
}


void ZMapFeatureSequenceMapStructType::updateSource(const char *source_name, const std::string &url, 
                                                    const char *featuresets,
                                                    const char *biotypes,
                                                    GError **error)
{
  return updateSource(source_name, url.c_str(), featuresets, biotypes, error) ;
}


/* Update the source with the given name with the given values */
void ZMapFeatureSequenceMapStructType::updateSource(const char *source_name,
                                                    const char *url,
                                                    const char *featuresets,
                                                    const char *biotypes,
                                                    GError **error)
{
  ZMapConfigSource source = NULL ;
  zMapReturnIfFail(url) ;

  source = getSource(source_name) ;

  if (source->url)
    {
      g_free(source->url) ;
      source->url = NULL ;
    }
  source->url = g_strdup(url) ;

  if (source->featuresets)
    {
      g_free(source->featuresets) ;
      source->featuresets = NULL ;
    }
  if (featuresets && *featuresets)
    source->featuresets = g_strdup(featuresets) ;

  if (source->biotypes)
    {
      g_free(source->biotypes) ;
      source->biotypes = NULL ;
    }
  if (biotypes && *biotypes)
    source->biotypes = g_strdup(biotypes) ;

  /* Indicate that there are changes that need saving */
  setFlag(ZMAPFLAG_SAVE_SOURCES, TRUE) ;
}


/* Create a file source and add it to our list of user-created sources. */
ZMapConfigSource ZMapFeatureSequenceMapStructType::createFileSource(const char *source_name_in, 
                                                                    const char *file)
{
  ZMapConfigSource src = NULL ;
  zMapReturnValIfFail(file, src) ;

  /* Create the new source */
  src = new ZMapConfigSourceStruct ;

  src->name_ = g_quark_from_string(source_name_in) ;
  src->group = SOURCE_GROUP_START ;        // default_value
  src->featuresets = g_strdup(ZMAP_DEFAULT_FEATURESETS) ;

  if (strncasecmp(file, "http://", 7) != 0 && 
      strncasecmp(file, "https://", 8) != 0 &&
      strncasecmp(file, "ftp://", 6) != 0)
    src->url = g_strdup_printf("file:///%s", file) ;

  /* Add the source to our list. Use the filename as the source name if none given */
  string source_name(source_name_in ? source_name_in : g_path_get_basename(file)) ;
  GError *error = NULL ;

  src = addSource(source_name, src, &error) ;

  if (error)
    {
      zMapLogWarning("Error creating source for file '%s': %s", file, error->message) ;
      g_error_free(error) ;
      zMapConfigSourceDestroy(src) ;
      src = NULL ;
    }

  return src ;
}


/* Create a pipe source and add it to our list of user-created sources. */
ZMapConfigSource ZMapFeatureSequenceMapStructType::createPipeSource(const char *source_name_in, 
                                                                    const char *file,
                                                                    const char *script,
                                                                    const char *args)
{
  ZMapConfigSource src = NULL ;
  zMapReturnValIfFail(file && script, src) ;

  /* Create the new source */
  src = new ZMapConfigSourceStruct ;

  src->name_ = g_quark_from_string(source_name_in) ;
  src->group = SOURCE_GROUP_START ;        // default_value
  src->featuresets = g_strdup(ZMAP_DEFAULT_FEATURESETS) ;

  if (args)
    src->url = g_strdup_printf("pipe:///%s?%s", script, args) ;
  else
    src->url = g_strdup_printf("pipe:///%s", script) ;

  /* Add the source to our list. Use the filename as the source name if none given */
  string source_name(source_name_in ? source_name_in : g_path_get_basename(file)) ;
  GError *error = NULL ;

  src = addSource(source_name, src, &error) ;

  if (error)
    {
      zMapLogWarning("Error creating source for file '%s': %s", file, error->message) ;
      g_error_free(error) ;
      zMapConfigSourceDestroy(src) ;
      src = NULL ;
    }

  return src ;
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
ZMapConfigSource ZMapFeatureSequenceMapStructType::getSource(const string &source_name)
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


/* Get the source name of the given source struct. Returns a newly-allocated string which should
 * be free'd with g_free, or NULL if not found */
char* ZMapFeatureSequenceMapStructType::getSourceName(ZMapConfigSource source)
{
  char *result = NULL ;

  if (sources)
    {
      for (map<string, ZMapConfigSource>::iterator iter = sources->begin();
           iter != sources->end();
           ++iter)
        {
          if (iter->second == source)
            result = g_strdup(iter->first.c_str()) ;
        }
    }

  return result ;
}


/* Get the url of the given source. Checks for a cached source and if it's not cached then
 * attempts to look up the source details in the config file. The result should be free'd
 * with g_free. Returns null if not found. */
char* ZMapFeatureSequenceMapStructType::getSourceURL(const string &source_name)
{
  char *result = NULL ;

  ZMapConfigSource source = getSource(source_name) ;

  if (source && source->url)
    result = g_strdup(source->url) ;

  if (!result)
    {
      ZMapConfigIniContext context = zMapConfigIniContextProvide(config_file, ZMAPCONFIG_FILE_NONE) ;

      if (context)
        {
          zMapConfigIniContextGetString(context, source_name.c_str(), ZMAPSTANZA_SOURCE_CONFIG, 
                                        ZMAPSTANZA_SOURCE_URL, &result) ;
        }
    }

  return result ;
}


/* Get the list of ZMapConfigSource structs from our sources map, returned in a glist. Includes
 * child sources if the flag is set; otherwise, just includes toplevel sources. Note that it
 * excludes trackhub parent sources which do not have data themselves (the "trackhub://" source
 * itself and sources in the hierarchy without a data url). */
GList* ZMapFeatureSequenceMapStructType::getSources(const bool include_children)
{
  GList *result = NULL ;

  if (sources)
    {
      for (auto iter : *sources)
        {
          ZMapConfigSource source = iter.second ;

          if (source && source->url && *source->url != '\0' && strncasecmp(source->url, "trackhub://", 11) != 0)
            result = g_list_append(result, source) ;

          if (include_children)
            getSourceChildren(source, &result) ;
        }
    }

  return result ;
}


/* Recursively adds a source's children to the given list */
void ZMapFeatureSequenceMapStructType::getSourceChildren(ZMapConfigSource source,
                                                         GList **result)
{
  zMapReturnIfFail(source && result) ;
  
  for (ZMapConfigSource child : source->children)
    {
      if (source && source->url && *source->url != '\0' && !strncasecmp(source->url, "trackhub://", 11))
        *result = g_list_append(*result, child) ;

      getSourceChildren(child, result) ;
    }
}


/* This adds any sources from the given config file/string. Returns a list of the
 * newly-added sources. Note that this only returns the top level sources and not any child
 * sources (the return is only used in a twisty bit of code in zmapViewLoadFeatures which could
 * probably be improved but I'm preserving the original behaviour for now.) */
GList* ZMapFeatureSequenceMapStructType::addSourcesFromConfig(const char *filename,
                                                              const char *config_str,
                                                              char **stylesfile)
{
  GList *result = NULL ;

  // This will be the list of names from the sources stanza
  char *source_names = NULL ;

  // get any sources specified in the config file or the given config string
  GList *sources_list = zMapConfigGetSources(filename, config_str, stylesfile) ;

  ZMapConfigIniContext context = zMapConfigIniContextProvide(filename, ZMAPCONFIG_FILE_NONE) ;

  if (context && config_str)
    zMapConfigIniContextIncludeBuffer(context, config_str);

  if (context && 
      sources_list && g_list_length(sources_list) > 0 &&
      zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG, 
                                    ZMAPSTANZA_APP_SOURCES, &source_names))
    {
      GList *names_list = zMapConfigString2QuarkIDGList(source_names) ;

      // loop through all config sources
      GList *source_item = sources_list ;
      GList *name_item = names_list ;

      for ( ; source_item && name_item; source_item = g_list_next(source_item), name_item = g_list_next(name_item))
        {
          ZMapConfigSource source = (ZMapConfigSource)(source_item->data) ;
          string source_name(g_quark_to_string(GPOINTER_TO_INT(name_item->data))) ;
          GError *tmp_error = NULL ;

          // Add the source to our internal list and take ownership
          source = addSource(source_name, source, &tmp_error) ;

          if (tmp_error)
            {
              zMapLogWarning("Error creating source '%s': %s", source_name.c_str(), tmp_error->message) ;
              g_error_free(tmp_error) ;
              tmp_error = NULL ;

              zMapConfigSourceDestroy(source) ;
              source = NULL ;
              source_item->data = NULL ;
            }
          else
            {
              // Add it to the output list
              result = g_list_append(result, source) ;

              // Recursively create any child sources
              if (!tmp_error)
                createSourceChildren(source, &result, &tmp_error) ;
            }
        }

      if (names_list)
        g_list_free(names_list) ;
    }

  if (sources_list)
    g_list_free(sources_list) ;

  return result ;
}


GList* ZMapFeatureSequenceMapStructType::addSourcesFromConfig(const char *config_str,
                                                              char **stylesfile)
{
  return addSourcesFromConfig(config_file, config_str, stylesfile) ;
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

      // Loop through all top level sources (note that we generally don't want to save child
      // sources to config so they are ignored here)
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

// unnamed namespace
namespace
{

gint colIDOrderCB(gconstpointer a, gconstpointer b,gpointer user_data)
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


bool colOrderCB(const ZMapFeatureColumn &pa, const ZMapFeatureColumn &pb)
{
  bool result = true ;

  if(pa && pb)
    {
      if(pa->order < pb->order)
        result = true ;
      else
        result = false ;
    }

  return result ;
}

/* Update the given context with the given hash table of ids mapped to a glist of ids. This is
 * exported as key=value where the hash table id is the key and the value is a
 * semi-colon-separated list of the ids from the glist. export_func is an optional function that
 * takes the key and glist-value and returns true if the value should be included in the context, false if
 * not. If this function is null then all values are included.  */
void updateContextHashList(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, 
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
ZMapFeatureContextExecuteStatus updateContextFeatureSetStyle(GQuark key,
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



} // unnamed namespace
