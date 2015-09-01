/*  File: zmapConfigIni_P.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See ZMap/ConfigIni.h
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIGINI_P_H
#define ZMAP_CONFIGINI_P_H

#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapConfigKeyboard.hpp>
#include <ZMap/zmapConfigStyleDefaults.hpp>


#define FILE_COUNT 5
#define IMPORTANT_COUNT 2


typedef struct _ZMapConfigIniStruct
{
  GKeyFile *key_file[ZMAPCONFIG_FILE_NUM_TYPES] ;
  GError *key_error[ZMAPCONFIG_FILE_NUM_TYPES] ;
  GQuark key_file_name[ZMAPCONFIG_FILE_NUM_TYPES] ; 
  gboolean unsaved_changes[ZMAPCONFIG_FILE_NUM_TYPES] ;
} ZMapConfigIniStruct;





/* OH CHRIST....WHAT IS THIS....FOR GOODNESS SAKE....RUBBISH, RUBBISH, RUBBISH.... */
#ifdef MH17_IN_CONFIG_H



/* Supported data types for stanza elements. (see ZMapConfigStanzaElementStruct). */
typedef enum {ZMAPCONFIG_INVALID, ZMAPCONFIG_BOOL, ZMAPCONFIG_INT,
            ZMAPCONFIG_FLOAT, ZMAPCONFIG_STRING} ZMapConfigElementType ;


/* Defines an element of a stanza, is used both to specify a stanza spec (in which
 * case the "data" field must be used to give a default value), and also to return actual stanzas,
 * e.g. see zMapConfigGetStanzas().
 *
 * Note that if you add a new data type to the "data" union in ZMapConfigElementStruct,
 * you need to add a new enum for it to ZMapConfigElementType. */
typedef struct
{
  char *name ;                                      /*!< name of element. */
  ZMapConfigElementType type ;                            /*!< element data type. */
  union
  {
    char     *s ;                             /*!< Must be used for static initialisation. */
    gboolean b ;                              /*!< currently implemented as char 't' | 'f' */
    int      i ;
    double   f ;
  } data ;                                    /*!< element data.  */
} ZMapConfigStanzaElementStruct, *ZMapConfigStanzaElement ;



/* A set of macros for accessing the data field of a particular element of an array
 * of ZMapConfigStanzaElementStruct. Very useful for initialising the array with data values
 * in preparation for creating a stanza.
 *
 * @param ARRAY        The array of ZMapConfigStanzaElementStruct.
 * @param ELEMENT_NAME The name of the element whose data is to be accesssed.
 * @return             data according to the the macro used.
 * */
#define zMapConfigGetStructBool(ARRAY, ELEMENT_NAME)             \
(zMapConfigFindStruct((ARRAY), (ELEMENT_NAME)))->data.b
#define zMapConfigGetStructInt(ARRAY, ELEMENT_NAME)              \
(zMapConfigFindStruct((ARRAY), (ELEMENT_NAME)))->data.i
#define zMapConfigGetStructFloat(ARRAY, ELEMENT_NAME)            \
(zMapConfigFindStruct((ARRAY), (ELEMENT_NAME)))->data.f
#define zMapConfigGetStructString(ARRAY, ELEMENT_NAME)           \
(zMapConfigFindStruct((ARRAY), (ELEMENT_NAME)))->data.s




/* A set of macros for retrieving the data for a named element from the given stanza.
 *
 * @param STANZA       The stanza containing the element.
 * @param ELEMENT_NAME The name of the element containing the data.
 * @return             data according to the the macro used.
 * */
#define zMapConfigGetElementBool(STANZA, ELEMENT_NAME)             \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.b
#define zMapConfigGetElementInt(STANZA, ELEMENT_NAME)              \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.i
#define zMapConfigGetElementFloat(STANZA, ELEMENT_NAME)            \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.f
#define zMapConfigGetElementString(STANZA, ELEMENT_NAME)           \
(zMapConfigFindElement((STANZA), (ELEMENT_NAME)))->data.s




/* Config data available to application. */
typedef struct _ZMapConfigStruct *ZMapConfig ;

/* A set of stanzas, probably read from a config. */
typedef struct _ZMapConfigStanzaSetStruct *ZMapConfigStanzaSet ;

/* A single stanza, containing elements with associated data.
 *
 * Used both to specify a stanza to retrieve from a config and to return stanzas found in the config. */
typedef struct _ZMapConfigStanzaStruct *ZMapConfigStanza ;


/* Note that currently the package does _not_ support having multiple elements within a stanza
 * with the same name.
 *  */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* I'd like to do this but it doesn't work because as far as the compiler is concerned
 * we could return any of the types so we get a type mismatch with the variable we
 * are assigning to....sigh...MUST LOOK UP UNIONS, surely there is a natty way
 * round this.... */
#define zMapConfigGetElementValue(STANZA, ELEMENT_NAME, ELEMENT_PTR)            \
((ELEMENT_PTR) = zMapConfigFindElement((STANZA), (ELEMENT_NAME)),               \
  ((ELEMENT_PTR)->type == ZMAPCONFIG_BOOL ? (gboolean)((ELEMENT_PTR)->data.b)   \
   : ((ELEMENT_PTR)->type == ZMAPCONFIG_INT ? (int)((ELEMENT_PTR)->data.i)             \
      : ((ELEMENT_PTR)->type == ZMAPCONFIG_FLOAT ? (float)((ELEMENT_PTR)->data.f)        \
       : (char *)((ELEMENT_PTR)->data.s)))))

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


ZMapConfig zMapConfigCreate(void) ;
ZMapConfig zMapConfigCreateFromFile(char *config_file_path) ;
ZMapConfig zMapConfigCreateFromBuffer(char *buffer);

ZMapConfigStanzaElement zMapConfigFindStruct(ZMapConfigStanzaElementStruct elements[],
                                   char *element_name) ;

gboolean zMapConfigFindStanzas(ZMapConfig config,
                         ZMapConfigStanza stanza, ZMapConfigStanzaSet *stanzas_out) ;

void zMapConfigDestroy(ZMapConfig config) ;

void zMapConfigDeleteStanzaSet(ZMapConfigStanzaSet stanzas) ;

ZMapConfigStanza zMapConfigMakeStanza(char *stanza_name, ZMapConfigStanzaElementStruct elements[]) ;

ZMapConfigStanza zMapConfigGetNextStanza(ZMapConfigStanzaSet stanzas, ZMapConfigStanza current_stanza) ;

ZMapConfigStanzaElement zMapConfigFindElement(ZMapConfigStanza stanza, char *element_name) ;

void zMapConfigDestroyStanza(ZMapConfigStanza stanza) ;
#endif


ZMapConfigIni zMapConfigIniNew(void) ;
gboolean zMapConfigIniReadAll(ZMapConfigIni config, const char *config_file) ;
gboolean zMapConfigIniReadUser(ZMapConfigIni config, const char *config_file);
gboolean zMapConfigIniReadSystem(ZMapConfigIni config);
gboolean zMapConfigIniReadZmap(ZMapConfigIni config);
gboolean zMapConfigIniReadBuffer(ZMapConfigIni config, const char *buffer);
gboolean zMapConfigIniReadStyles(ZMapConfigIni config, const char *file);
void zMapConfigIniGetStanza(ZMapConfigIni config, const char *stanza_name);
void zMapConfigIniGetAllStanzas(ZMapConfigIni config);
void zMapConfigIniGetStanzaValues(ZMapConfigIni, const char *stanza_name);
gboolean zMapConfigIniGetUserValue(ZMapConfigIni config,
                                   const char * stanza_name,
                                   const  char * key_name,
                                   GValue **value_out,
                                   GType type);
gboolean zMapConfigIniGetValue(ZMapConfigIni config,
                               const char * stanza_name,
                               const char * key_name,
                               GValue **value_out,
                               GType type);
void zMapConfigIniSetValue(ZMapConfigIni config,
                           ZMapConfigIniFileType file_type,
                           const char *stanza_name,
                           const char *key_name,
                           GValue *value);
gboolean zMapConfigIniSave(ZMapConfigIni config, ZMapConfigIniFileType file_type);
void zMapConfigIniDestroy(ZMapConfigIni config, gboolean save_user);




#endif /* !ZMAP_CONFIGINI_P_H */
