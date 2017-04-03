
/*  File: zmapMLF.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Header file for the public interface for the Multi-Line
 * Feature data structure. This is essentially a hash of hashes. The
 * top level one has the "ID" attribute (GQuark representation of this
 * string) as its key. Its value is a hash that has key-value pairs that
 * are the same pointer to feature. When the key-value pairs are the same
 * in the hash, we are effectively using a multi-valued hash map (I had
 * in mind the "multimap" of C++ in doing this).
 *
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_MLF_H
#define ZMAP_MLF_H

#include <string.h>
#include <glib.h>
#include <ZMap/zmapGFF.hpp>


/*
 * Public header for multi-line feature material. Basically this is a
 * multimap that maps ID attributes (translated into GQuark) into a
 * set of features. The top-level map has
 *               key = ID, value = hashtable
 * The second hashtable has
 *               key = feature->unique_id, value = feature->unique_id
 * Thus is effectively a set of features identified by their unique_id
 * data member, assumed to have been already constructed.
 */

/*
 * Forward declaration.
 */
typedef struct     ZMapMLFStruct_      *ZMapMLF ;

/*
 * Creation/destruction functions.
 */
ZMapMLF zMapMLFCreate() ;
gboolean zMapMLFEmpty(ZMapMLF ) ;
gboolean zMapMLFDestroy(ZMapMLF) ;

/*
 * Data manipulation and other functions.
 */
unsigned int zMapMLFNumID(ZMapMLF) ;
unsigned int zMapMLFNumFeatures(ZMapMLF, GQuark ) ;
gpointer zMapMLFIsIDPresent(ZMapMLF, GQuark ) ;
gboolean zMapMLFAddID(ZMapMLF, GQuark) ;
gboolean zMapMLFRemoveID(ZMapMLF const, GQuark ) ;
gboolean zMapMLFIsFeaturePresent(ZMapMLF, GQuark, ZMapFeature ) ;
gboolean zMapMLFAddFeatureToID(ZMapMLF, GQuark, ZMapFeature ) ;
gboolean zMapMLFRemoveFeatureFromID(ZMapMLF, GQuark , ZMapFeature ) ;

/*
 * Iteration mechanisms.
 */
gboolean zMapMLFIDIteration(ZMapMLF, gboolean (*)(GQuark, GHashTable *) ) ;


#endif
