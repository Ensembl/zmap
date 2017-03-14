/*  File: zmapConfigIni.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * Description: Functions for reading zmap configuration 'ini' files.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_INI_H
#define ZMAP_CONFIG_INI_H

#include <glib-object.h>
#include <map>
#include <list>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapConfigStanzaStructs.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


class ZMapStyleTree ;

struct ZMapFeatureColumnStructType ;


/* default list of in-built columns if not specified in config file */
#define ZMAP_DEFAULT_FEATURESETS "DNA ; 3 Frame ; 3 Frame Translation ; Show Translation ; Annotation "


/* This enum lists the types of config files that can be supplied. The order of this list is the
 * order of preference if the same stanza occurs in multiple different config files */
typedef enum
  {
    ZMAPCONFIG_FILE_NONE,

    ZMAPCONFIG_FILE_BUFFER, /* Config read from a text buffer rather than a file*/
    ZMAPCONFIG_FILE_STYLES, /* Styles file */
    ZMAPCONFIG_FILE_PREFS,  /* Global user preferences file */
    ZMAPCONFIG_FILE_USER,   /* User-specified config file */
    ZMAPCONFIG_FILE_ZMAP,   /* Default zmap config file */
    ZMAPCONFIG_FILE_SYS,    /* Default system config file */

    ZMAPCONFIG_FILE_NUM_TYPES /* must be last in list */
  } ZMapConfigIniFileType ;


typedef gpointer (*ZMapConfigIniUserDataCreateFunc)(gpointer data);
typedef void (*ZMapConfigIniSetPropertyFunc)(char *current_stanza_name, const char *key, GType type,
					     gpointer parent_data, GValue *property_value);

typedef struct
{
  const char *key;
  GType type;
  ZMapConfigIniSetPropertyFunc set_property;
  gboolean required ;
}ZMapConfigIniContextKeyEntryStruct, *ZMapConfigIniContextKeyEntry;

typedef struct _ZMapConfigIniContextStruct
{
  struct _ZMapConfigIniStruct *config;    //typedef'd in zmapConfigIni_P.h, b
  gboolean config_read;
  gchar *error_message;
  GList *groups;
}ZMapConfigIniContextStruct, *ZMapConfigIniContext;


typedef struct _ZMapConfigStruct *ZMapConfig ;

typedef  struct _ZMapConfigIniStruct *ZMapConfigIni;


/* This function pointer is for functions which update a context with user-specified preferences,
 * e.g. blixem prefs or control prefs */
typedef void (*ZMapConfigIniContextUpdatePrefsFunc)(ZMapConfigIniContext context, 
                                                    ZMapConfigIniFileType file_type, 
                                                    gpointer data) ;


ZMapConfigIniContext zMapConfigIniContextCreate(const char *config_file) ;
ZMapConfigIniContext zMapConfigIniContextCreateType(const char *config_file, ZMapConfigIniFileType file_type) ;
gboolean zMapConfigIniContextIncludeBuffer(ZMapConfigIniContext context, const char *buffer);
gboolean zMapConfigIniContextIncludeFile(ZMapConfigIniContext context, const char *file, ZMapConfigIniFileType file_type) ;
gchar **zMapConfigIniContextGetAllStanzaNames(ZMapConfigIniContext context);

gboolean zMapConfigIniContextAddGroup(ZMapConfigIniContext context,
				      const char *stanza_name, const char *stanza_type,
				      ZMapConfigIniContextKeyEntryStruct *keys);

gboolean zMapConfigIniContextGetValue(ZMapConfigIniContext context,
				      const char *stanza_name,
				      const char *stanza_type,
				      const char *key_name,
				      GValue **value_out);
gboolean zMapConfigIniContextGetString(ZMapConfigIniContext context,
				       const char *stanza_name,
				       const char *stanza_type,
				       const char *key_name,
				       char **value);
gboolean zMapConfigIniContextGetFilePath(ZMapConfigIniContext context,
					 const char *stanza_name,
					 const char *stanza_type,
					 const char *key_name,
					 char **value) ;
gboolean zMapConfigIniContextGetBoolean(ZMapConfigIniContext context,
					const char *stanza_name,
					const char *stanza_type,
					const char *key_name,
					gboolean  *value);
gboolean zMapConfigIniContextGetInt(ZMapConfigIniContext context,
				    const char *stanza_name,
				    const char *stanza_type,
				    const char *key_name,
				    int  *value);
gboolean zMapConfigIniContextSetValue(ZMapConfigIniContext context,
                                      ZMapConfigIniFileType file_type,
				      const char *stanza_name,
				      const char *key_name,
				      GValue *value);
gboolean zMapConfigIniContextSetString(ZMapConfigIniContext context,
                                       ZMapConfigIniFileType file_type,
				       const char *stanza_name,
				       const char *stanza_type,
				       const char *key_name,
				       const char *value_str);
gboolean zMapConfigIniContextSetInt(ZMapConfigIniContext context,
                                    ZMapConfigIniFileType file_type,
				    const char *stanza_name,
				    const char *stanza_type,
				    const char *key_name,
				    int   value_int);
gboolean zMapConfigIniContextSetBoolean(ZMapConfigIniContext context,
                                        ZMapConfigIniFileType file_type,
					const char *stanza_name,
					const char *stanza_type,
					const char *key_name,
					gboolean value_bool);

gboolean zMapConfigIniContextSave(ZMapConfigIniContext context, ZMapConfigIniFileType file_type);
void zMapConfigIniContextSetUnsavedChanges(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, const gboolean value) ;
void zMapConfigIniContextSetFile(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, const char *filename) ;
void zMapConfigIniContextCreateKeyFile(ZMapConfigIniContext context, ZMapConfigIniFileType file_type) ;
void zMapConfigIniContextSetStyles(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, gpointer data) ;

gchar *zMapConfigIniContextErrorMessage(ZMapConfigIniContext context);
gchar *zMapConfigIniContextKeyFileErrorMessage(ZMapConfigIniContext context);

ZMapConfigIniContext zMapConfigIniContextDestroy(ZMapConfigIniContext context);


GList *zMapConfigIniContextGetReferencedStanzas(ZMapConfigIniContext context,
						ZMapConfigIniUserDataCreateFunc object_create_func,
						const char *parent_name,
						const char *parent_type,
						const char *parent_key,
						const char *child_type);

GList *zMapConfigIniContextGetListedStanzas(ZMapConfigIniContext context,
                                    ZMapConfigIniUserDataCreateFunc object_create_func,
                                    char *styles_list, const char * child_type);


// zmapConfigLoader.c

ZMapConfigIniContext zMapConfigIniContextProvide(const char *config_file, ZMapConfigIniFileType file_type = ZMAPCONFIG_FILE_NONE) ;
ZMapConfigIniContext zMapConfigIniContextProvideNamed(const char *config_file, const char *stanza_name, ZMapConfigIniFileType file_type) ;

gboolean zMapConfigIniHasStanza(ZMapConfigIni config, const char *stanza_name, GKeyFile **which);
gboolean zMapConfigIniHasStanzaAll(ZMapConfigIni config, const char *stanza_name, std::list<GKeyFile*> &which);

GList *zMapConfigIniContextGetSources(ZMapConfigIniContext context) ;
GList *zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name) ;

GHashTable * zmapConfigIniGetDefaultStyles(void);
gboolean zMapConfigIniGetStylesFromFile(char *config_file,
					char *styles_list, char *styles_file, GHashTable **styles_out, const char *buffer);
GHashTable *zMapConfigIniGetFeatureset2Column(ZMapConfigIniContext context,GHashTable *ghash,
                                              std::map<GQuark, ZMapFeatureColumnStructType*> &columns);

gboolean zMapConfigLegacyStyles(char *config_file) ;

char *zMapConfigNormaliseWhitespace(char *str,gboolean cannonical);
std::list<GQuark> zMapConfigString2QuarkList(const char *string_list,gboolean cannonical);
std::list<GQuark> zMapConfigString2QuarkIDList(const char *string_list);
GList *zMapConfigString2QuarkGList(const char *string_list,gboolean cannonical);
GList *zMapConfigString2QuarkIDGList(const char *string_list);

GHashTable *zMapConfigIniGetQQHash(ZMapConfigIniContext context, const char *stanza, int how) ;
void zMapConfigIniSetQQHash(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, const char *stanza, GHashTable *ghash) ;
#define QQ_STRING 0
#define QQ_QUARK  1
#define QQ_STYLE  2

GKeyFile *zMapConfigIniGetKeyFile(ZMapConfigIniContext config, ZMapConfigIniFileType file_type) ;

std::list<GQuark> zMapConfigIniMergeColumnsLists(std::list<GQuark> &src_list, std::list<GQuark> &dest_list, const bool unique = false) ;

GHashTable *zMapConfigIniGetFeatureset2Featureset(ZMapConfigIniContext context,
						  GHashTable *fset_src, GHashTable *fset2col) ;

GHashTable *zMapConfigIniGetColumnGroups(ZMapConfigIniContext context) ;

std::map<GQuark, ZMapFeatureColumnStructType*> *zMapConfigIniGetColumns(ZMapConfigIniContext context);

GList *zMapConfigGetSources(const char *config_file, const char *config_str,char **stylesfile);

#endif /* ZMAP_CONFIG_INI_H */
