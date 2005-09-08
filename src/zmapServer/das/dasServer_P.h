/*  File: dasServer_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: Sep  8 16:17 2005 (rds)
 * Created: Thu Mar 18 12:02:52 2004 (edgrif)
 * CVS info:   $Id: dasServer_P.h,v 1.4 2005-09-08 17:45:36 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef DAS_SERVER_P_H
#define DAS_SERVER_P_H

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <ZMap/zmapXML.h>
#include <ZMap/zmapFeature.h>
#include <das1schema.h>

/* http://www.biodas.org/documents/spec.html */

typedef enum {
  ZMAP_DAS_UNKNOWN,             /* DOESN'T even remotely look like DAS */
  ZMAP_DASONE_UNKNOWN,          /* DOESN'T look like DAS1 */
  ZMAP_DASONE_DSN,
  ZMAP_DASONE_ENTRY_POINTS,
  ZMAP_DASONE_DNA,
  ZMAP_DASONE_SEQUENCE,
  ZMAP_DASONE_TYPES,
  ZMAP_DASONE_INTERNALFEATURES, /* Special type to get das features as */
  ZMAP_DASONE_FEATURES,         /* das not zmap like this one */
  ZMAP_DASONE_LINK,
  ZMAP_DASONE_STYLESHEET,
  ZMAP_DASTWO_UNKNOWN          /* DOESN'T look like DAS2 */
} dasDataType;

typedef enum {
  TAG_DAS_UNKNOWN,
  TAG_DASONE_UNKNOWN,
  TAG_DASONE_ERROR,
  TAG_DASONE_DSN,
  TAG_DASONE_ENTRY_POINTS,
  TAG_DASONE_DNA,
  TAG_DASONE_SEQUENCE,
  TAG_DASONE_TYPES,
  TAG_DASONE_INTERNALFEATURES,  /* THIS ISN'T NEEDED. */
  TAG_DASONE_FEATURES,
  TAG_DASONE_LINK,
  TAG_DASONE_STYLESHEET,
  TAG_DASONE_SEGMENT
} dasXMLTagType;

/* Holds all the state we need to manage the das connection. */
typedef struct _DasServerStruct
{
  GQuark protocol, host, path, user, passwd; /* url bits */
  int   port ;

  CURL *curl_handle ;

  int chunks ;						    /* for debugging at the moment... */

  zmapXMLParser parser;
  gboolean debug;

  /* error stuff... */
  CURLcode curl_error ;					    /* curl specific error stuff */
  char *curl_errmsg ;
  char *last_errmsg ;					    /* The general das msg stuf, could be
                                                               curl could be my code. */

  GList *dsn_list;
  dasOneSegment current_segment;
  GHashTable *hashtable;

  /* this will not stay like this, it should more properly be a union of types of data that might
   * be returned.... */
  dasDataType type;
  void *data ;

  ZMapFeatureContext req_context ;
  ZMapFeatureContext cur_context ;
} DasServerStruct, *DasServer ;


gboolean checkDSNExists(DasServer das, 
                        dasOneDSN *dsn);


/* Parsing handlers (see das1handlers.c) */
/* There is a pair of handlers for each DAS document type 
 * E.G one for DASDSN formatted xml (http://www.biodas.org/dtd/dasdsn.dtd)
 * userData is a ..... struct
 */
/* DSN handlers */
gboolean dsnStart(void *userData, 
                  zmapXMLElement element, 
                  zmapXMLParser parser);
gboolean dsnEnd(void *userData, 
                zmapXMLElement element, 
                zmapXMLParser parser);
/* Entry_Point handlers */
gboolean epStart(void *userData, 
                 zmapXMLElement element, 
                 zmapXMLParser parser);
gboolean epEnd(void *userData, 
               zmapXMLElement element, 
               zmapXMLParser parser);

/* Types Handlers */
gboolean typesStart(void *userData, 
                    zmapXMLElement element, 
                    zmapXMLParser parser);
gboolean typesEnd(void *userData, 
                  zmapXMLElement element, 
                  zmapXMLParser parser);

/* Internal DAS Features Handlers */
gboolean internalDasStart(void *userData, 
                          zmapXMLElement element, 
                          zmapXMLParser parser);
gboolean internalDasEnd(void *userData, 
                        zmapXMLElement element, 
                        zmapXMLParser parser);


#endif /* !DAS_SERVER_P_H */
