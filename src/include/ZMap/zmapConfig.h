/*  File: zmapConfig.h
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
 * Last edited: Apr  8 17:16 2004 (edgrif)
 * Created: Fri Jan 23 13:10:06 2004 (edgrif)
 * CVS info:   $Id: zmapConfig.h,v 1.3 2004-04-08 16:21:16 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_H
#define ZMAP_CONFIG_H

#include <glib.h>

/* Opaque type, contains config info. */
typedef struct _ZMapConfigStruct *ZMapConfig ;


/* Supported data types for stanza fields. */
typedef enum {ZMAPCONFIG_INVALID, ZMAPCONFIG_BOOL, ZMAPCONFIG_INT,
	      ZMAPCONFIG_FLOAT, ZMAPCONFIG_STRING} ZMapConfigElementType ;


/* Defines an element of a stanza, is used both to give a stanza spec (in which
 * case the "data" field must be used to give a default value), and also to return actual stanzas,
 * e.g. see zMapConfigGetStanzas().
 *
 * Note that if you add a new data type to the "data" union in ZMapConfigElementStruct,
 * you need to add a new enum for it to ZMapConfigElementType. */
typedef struct
{
  char *name ;
  ZMapConfigElementType type ;
  union
  {
    gboolean b ;					    /* currently implemented as char 't' |
							       'f' */
    int      i ;
    float    f ;
    char     *s ;
  } data ;
} ZMapConfigStanzaElementStruct, *ZMapConfigStanzaElement ;


/* I'd like to do this but it doesn't work because as far as the compiler is concerned
 * we could return any of the types so we get a type mismatch with the variable we
 * are assigning to....sigh...MUST LOOK UP UNIONS, surely there is a natty way
 * round this.... */
#define zMapConfigGetElementValue(STANZA, ELEMENT_NAME, ELEMENT_PTR)            \
((ELEMENT_PTR) = zMapConfigFindElement((STANZA), (ELEMENT_NAME)),           \
  ((ELEMENT_PTR)->type == ZMAPCONFIG_BOOL ? (ELEMENT_PTR)->data.b               \
   : ((ELEMENT_PTR)->type == ZMAPCONFIG_INT ? (ELEMENT_PTR)->data.i             \
      : ((ELEMENT_PTR)->type == ZMAPCONFIG_FLOAT ? (ELEMENT_PTR)->data.f        \
	 : (ELEMENT_PTR)->data.s))))

/* For now I'll use these.... */
#define zMapConfigGetElementBool(STANZA, ELEMENT_NAME)            \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.b

#define zMapConfigGetElementInt(STANZA, ELEMENT_NAME)            \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.i

#define zMapConfigGetElementFloat(STANZA, ELEMENT_NAME)            \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.f

#define zMapConfigGetElementString(STANZA, ELEMENT_NAME)            \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.s



/* May make these types internal only..... */

/* Holds a complete stanza. */
typedef struct
{
  char *name ;
  GList *elements ;					    /* Array of stanza elements. */
} ZMapConfigStanzaStruct, *ZMapConfigStanza ;



/* Holds a list of stanzas. */
typedef struct
{
  char *name ;
  GList *stanzas ;					    /* Array of stanzas. */
} ZMapConfigStanzaSetStruct, *ZMapConfigStanzaSet ;




/* Utility to ease creating stanzas, elements must have last one with NULL name.... */
ZMapConfigStanza zMapConfigMakeStanza(char *stanza_name, ZMapConfigStanzaElement elements) ;


ZMapConfigStanza zMapConfigGetNextStanza(ZMapConfigStanzaSet stanzas, ZMapConfigStanza current_stanza) ;

ZMapConfigStanzaElement zMapConfigFindElement(ZMapConfigStanza stanza, char *element_name) ;


ZMapConfig zMapConfigCreate(char *config_file) ;

gboolean zMapConfigGetStanzas(ZMapConfig config,
			      ZMapConfigStanza stanza, ZMapConfigStanzaSet *stanzas_out) ;

void zMapConfigDestroy(ZMapConfig config) ;




/* Temp hack... */
gboolean zMapConfigGetServers(ZMapConfig config, char ***servers) ;



#endif /* !ZMAP_CONFIG_H */
