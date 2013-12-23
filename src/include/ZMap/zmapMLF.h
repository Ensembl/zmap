
/*  File: zmapMLF.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2013: Genome Research Ltd.
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
 *     	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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
#include <ZMap/zmapGFF.h>


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
gboolean zMapMLFEmpty(ZMapMLF const ) ;
gboolean zMapMLFDestroy(ZMapMLF) ;

/*
 * Data manipulation and other functions.
 */
unsigned int zMapMLFNumID(const ZMapMLF const ) ;
unsigned int zMapMLFNumFeatures(const ZMapMLF const, GQuark ) ;
gpointer zMapMLFIsIDPresent(const ZMapMLF const, GQuark ) ;
gboolean zMapMLFAddID(const ZMapMLF const, GQuark) ;
gboolean zMapMLFRemoveID(ZMapMLF const, GQuark ) ;
gboolean zMapMLFIsFeaturePresent(const ZMapMLF const, GQuark, const ZMapFeature const ) ;
gboolean zMapMLFAddFeatureToID(const ZMapMLF const , GQuark, const ZMapFeature const ) ;
gboolean zMapMLFRemoveFeatureFromID(const ZMapMLF const, GQuark , const ZMapFeature const ) ;

/*
 * Iteration mechanisms.
 */
gboolean zMapMLFIDIteration(const ZMapMLF const, gboolean (*)(GQuark, GHashTable *) ) ;


#endif
