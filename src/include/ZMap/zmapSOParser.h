

/*  File: zmapSOParser.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 * Description: Public header for the SO file parser, data structure
 * and associated functions.
 *
 *-------------------------------------------------------------------
 */



#ifndef ZMAP_SOPARSER_H
#define ZMAP_SOPARSER_H

/*
 * Public header for SO parser related stuff.
 */


/*
 * Some library stuff.
 */
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ZMap/zmapStyle.h>
#include <ZMap/zmapFeature.h>


/*
 * Forward declaration of relevant data structure.
 */
typedef struct        ZMapSOIDDataStruct_       *ZMapSOIDData ;


/*
 * Enum to define which of the SO term sets is in use.
 */
typedef enum
{
  ZMAPSO_USE_SOFA,            /* Use data from SOFA.obo                            */
  ZMAPSO_USE_SOXP,            /* Use data from so-xp.obo                           */
  ZMAPSO_USE_SOXPSIMPLE,      /* Use data from so-xp-simple.obo                    */
  ZMAPSO_USE_NONE             /* Value set if nothing else found                   */
} ZMapSOSetInUse ;

/*
 * New public interface. All of the material following this section is
 * deprecated.
 */
unsigned int zMapSOSetIsNamePresent(ZMapSOSetInUse , const char * const ) ;
char * zMapSOSetIsIDPresent(ZMapSOSetInUse , unsigned int ) ;
ZMapStyleMode zMapSOSetGetStyleModeFromID(ZMapSOSetInUse, unsigned int ) ;
ZMapHomolType zMapSOSetGetHomolFromID(ZMapSOSetInUse, unsigned int ) ;
ZMapStyleMode zMapSOSetGetStyleModeFromName(ZMapSOSetInUse, const char * const ) ;
ZMapSOIDData zMapSOIDDataCreate() ;
ZMapSOIDData zMapSOIDDataCC(ZMapSOIDData) ;
ZMapSOIDData zMapSOIDDataCreateFromData(unsigned int, const char * const, ZMapStyleMode , ZMapHomolType ) ;
gboolean zMapSOIDDataDestroy(ZMapSOIDData) ;
unsigned int zMapSOIDDataGetID(ZMapSOIDData) ;
char *       zMapSOIDDataGetIDAsString(ZMapSOIDData ) ;
char *       zMapSOIDDataName2SOAcc(const char * const ) ;
char * zMapSOIDDataGetName(ZMapSOIDData ) ;
ZMapStyleMode zMapSOIDDataGetStyleMode(ZMapSOIDData ) ;
ZMapHomolType zMapSOIDDataGetHomol(ZMapSOIDData) ;












/*
 * Forward declarations of data structures. THese are the old interface (used for
 * real-time reading of files). Now deprecated.
 */
typedef struct            ZMapSOIDStruct_       *ZMapSOID ;
typedef struct          ZMapSOTermStruct_       *ZMapSOTerm ;
typedef struct    ZMapSOCollectionStruct_       *ZMapSOCollection ;
typedef struct        ZMapSOParserStruct_       *ZMapSOParser ;


/*
 * SO/Parser error states for external communication.
 */
typedef enum
{
  ZMAPSO_ERROR_PARSER_IDD,       /* Error parsing ID line             */
  ZMAPSO_ERROR_PARSER_NAM,       /* Error parsing Name line           */
  ZMAPSO_ERROR_PARSER_ISO,       /* Error parsing is_obsolete line    */
  ZMAPSO_ERROR_PARSER_OTH				    /* Error, none of the above          */
} ZMapSOError ;


/*
 * Creation/destruction/access functions for SO ID object.
 */
ZMapSOID zMapSOIDCreate() ;
gboolean zMapSOIDCopy(ZMapSOID, ZMapSOID ) ;
gboolean zMapSOIDEquals(ZMapSOID ,ZMapSOID ) ;
gboolean zMapSOIDSet(ZMapSOID, const char* const) ;
gboolean zMapSOIDInitialize(ZMapSOID) ;
gboolean zMapSOIDIsValid(ZMapSOID) ;
unsigned int zMapSOIDParseString(const char * const) ;
unsigned int zMapSOIDGetIDNumber(ZMapSOID) ;
char * zMapSOIDGetIDString(ZMapSOID) ;
gboolean zMapSOIDDestroy(ZMapSOID) ;
gboolean zMapSOIDIsAccessionNumber(const char * const ) ;


/*
 * Creation/destruction/access functions for SO Terms.
 */
ZMapSOTerm zMapSOTermCreate() ;
gboolean zMapSOTermDestroy(ZMapSOTerm) ;

gboolean zMapSOTermCopy(ZMapSOTerm , ZMapSOTerm ) ;
gboolean zMapSOTermEquals(ZMapSOTerm ,ZMapSOTerm ) ;

gboolean zMapSOTermSetName(ZMapSOTerm , const char* const ) ;
char *   zMapSOTermGetName(ZMapSOTerm ) ;
gboolean zMapSOTermSetID(ZMapSOTerm , ZMapSOID ) ;
ZMapSOID zMapSOTermGetID(ZMapSOTerm) ;

gboolean zMapSOTermIsValid(ZMapSOTerm) ;


/*
 * Access functions for SO Collection.
 */
ZMapSOCollection zMapSOCollectionCreate() ;
unsigned int zMapSOCollectionGetNumTerms(ZMapSOCollection ) ;
gboolean zMapSOCollectionAddSOTerm(ZMapSOCollection, ZMapSOTerm ) ;
gboolean zMapSOCollectionDestroy(ZMapSOCollection ) ;
gboolean zMapSOCollectionIsTermPresent(ZMapSOCollection , ZMapSOTerm ) ;
gboolean zMapSOCollectionIsNamePresent(ZMapSOCollection , const char* const) ;
ZMapSOID zMapSOCollectionFindIDFromName(ZMapSOCollection , const char* const) ;
char *   zMapSOCollectionFindNameFromID(ZMapSOCollection , ZMapSOID ) ;
ZMapSOTerm zMapSOCollectionGetTerm( ZMapSOCollection , unsigned int) ;
gboolean zMapSOCollectionCheck( ZMapSOCollection ) ;


/*
 * Parsing functions.
 */
ZMapSOParser zMapSOParserCreate() ;
gboolean zMapSOParserInitialize(ZMapSOParser , gboolean) ;
gboolean zMapSOParserInitializeForLine(ZMapSOParser) ;
gboolean zMapSOParserDestroy(ZMapSOParser ) ;
ZMapSOCollection zMapSOParserGetCollection(ZMapSOParser) ;
gboolean zMapSOParseLine(ZMapSOParser, const char * const) ;
GError *zMapSOParserGetError( ZMapSOParser) ;
unsigned int zMapSOParserGetLineCount(ZMapSOParser) ;
gboolean zMapSOParserSetStopOnError(ZMapSOParser , gboolean) ;
gboolean zMapSOParserTerminated(ZMapSOParser) ;

/*
 * File reading functions.
 */
gboolean zMapSOParseFile(ZMapSOParser, const char * const) ;
ZMapSOCollection zMapSOReadFile(const char* const) ;


/*
 * Functions for reading all of this stuff, and making it
 * publically available.
 */
gboolean zMapSOReadSOCollections() ;
gboolean zMapSODestroySOCollections() ;
ZMapSOCollection zMapSOCollectionSO() ;
ZMapSOCollection zMapSOCollectionSOFA() ;



#endif
