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
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>


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
    std::map<GQuark, ZMapFeatureColumn>::iterator iter = context_map->columns->find(gff->column_id) ;

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

  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = context_map->columns->find(column_id) ;

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


/*
 *              ZMapFeatureSequenceMap routines
 */


/* Add a source to our list of user-created sources. Sets the error if the name already exists. */
void zMapFeatureSequenceMapAddSource(ZMapFeatureSequenceMap sequence_map,
                                     const std::string &source_name, 
                                     ZMapConfigSource source, 
                                     GError **error)
{
  zMapReturnIfFail(sequence_map && sequence_map->sources && source) ;

  if (sequence_map->sources && sequence_map->sources->find(source_name) != sequence_map->sources->end())
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99,
                  "Source '%s' already exists", source_name.c_str()) ;
    }
  else
    {
      if (!sequence_map->sources)
        sequence_map->sources = new std::map<std::string, ZMapConfigSource> ;

      (*sequence_map->sources)[source_name] = source ;
    }
}


void zMapFeatureSequenceMapGetCmdLineSources(ZMapFeatureSequenceMap sequence_map, GList **settings_list_inout)
{
  zMapReturnIfFailSafe(sequence_map) ;

  GSList *file_item = sequence_map->file_list ;

  for ( ; file_item; file_item = file_item->next)
    {
      char *file = (char*)(file_item->data) ;

      ZMapConfigSource src = g_new0(ZMapConfigSourceStruct, 1) ;
      src->group = SOURCE_GROUP_START ;        // default_value
      src->url = g_strdup_printf("file:///%s", file) ;
      src->featuresets = g_strdup(ZMAP_DEFAULT_FEATURESETS) ;

      *settings_list_inout = g_list_append(*settings_list_inout, (gpointer)src) ;
    }
}


/*
 *              Internal routines.
 */
