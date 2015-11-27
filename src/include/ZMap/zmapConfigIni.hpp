/*  File: zmapConfigIni.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions for reading zmap configuration 'ini' files.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_INI_H
#define ZMAP_CONFIG_INI_H

#include <glib-object.h>

class ZMapStyleTree ;



/* default list of in-built columns if not specified in config file */
#define ZMAP_DEFAULT_FEATURESETS "DNA ; 3 Frame ; 3 Frame Translation ; Show Translation ; Annotation "


/* This enum lists the types of config files that can be supplied */
typedef enum
  {
    ZMAPCONFIG_FILE_NONE,

    ZMAPCONFIG_FILE_BUFFER, /* Config read from a text buffer rather than a file*/
    ZMAPCONFIG_FILE_STYLES, /* Styles file */
    ZMAPCONFIG_FILE_USER,   /* User-specified config file */
    ZMAPCONFIG_FILE_PREFS,  /* Global user preferences file */
    ZMAPCONFIG_FILE_ZMAP,   /* Default zmap config file */
    ZMAPCONFIG_FILE_SYS,    /* Default system config file */

    ZMAPCONFIG_FILE_NUM_TYPES /* must be last in list */
  } ZMapConfigIniFileType ;


typedef gpointer (*ZMapConfigIniUserDataCreateFunc)(void);
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

ZMapConfigIniContext zMapConfigIniContextProvide(const char *config_file, ZMapConfigIniFileType file_type) ;
ZMapConfigIniContext zMapConfigIniContextProvideNamed(const char *config_file, const char *stanza_name, ZMapConfigIniFileType file_type) ;

gboolean zMapConfigIniHasStanza(ZMapConfigIni config, const char *stanza_name, GKeyFile **which);

GList *zMapConfigIniContextGetSources(ZMapConfigIniContext context) ;
GList *zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name) ;

GHashTable * zmapConfigIniGetDefaultStyles(void);
gboolean zMapConfigIniGetStylesFromFile(char *config_file,
					char *styles_list, char *styles_file, GHashTable **styles_out, const char *buffer);
GHashTable *zMapConfigIniGetFeatureset2Column(ZMapConfigIniContext context,GHashTable *ghash,GHashTable *columns);

gboolean zMapConfigLegacyStyles(char *config_file) ;

char *zMapConfigNormaliseWhitespace(char *str,gboolean cannonical);
GList *zMapConfigString2QuarkList(const char *string_list,gboolean cannonical);
GList *zMapConfigString2QuarkIDList(const char *string_list);

GHashTable *zMapConfigIniGetQQHash(ZMapConfigIniContext context, const char *stanza, int how) ;
void zMapConfigIniSetQQHash(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, const char *stanza, GHashTable *ghash) ;
#define QQ_STRING 0
#define QQ_QUARK  1
#define QQ_STYLE  2

GKeyFile *zMapConfigIniGetKeyFile(ZMapConfigIniContext config, ZMapConfigIniFileType file_type) ;

GHashTable *zMapConfigIniGetFeatureset2Featureset(ZMapConfigIniContext context,
						  GHashTable *fset_src, GHashTable *fset2col) ;

GHashTable *zMapConfigIniGetColumnGroups(ZMapConfigIniContext context) ;

GHashTable *zMapConfigIniGetColumns(ZMapConfigIniContext context);

void zMapConfigSourcesFreeList(GList *config_sources_list);



#endif /* ZMAP_CONFIG_INI_H */
