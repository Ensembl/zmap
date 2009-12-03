/*  File: zmapConfigIni.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 26 14:53 2009 (edgrif)
 * Created: Thu Sep 11 10:40:13 2008 (rds)
 * CVS info:   $Id: zmapConfigIni.h,v 1.6 2009-12-03 14:57:25 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_CONFIG_INI_H
#define ZMAP_CONFIG_INI_H

#include <glib-object.h>

typedef struct _ZMapConfigIniStruct *ZMapConfigIni ;

typedef gpointer (*ZMapConfigIniUserDataCreateFunc)(void);
typedef void (*ZMapConfigIniSetPropertyFunc)(char *current_stanza_name, char *key, GType type,
					     gpointer parent_data, GValue *property_value);

typedef struct
{
  char *key;
  GType type;
  ZMapConfigIniSetPropertyFunc set_property;
  gboolean required ;
}ZMapConfigIniContextKeyEntryStruct, *ZMapConfigIniContextKeyEntry;

typedef struct _ZMapConfigIniContextStruct *ZMapConfigIniContext;


ZMapConfigIni zMapConfigIniNew(void) ;
gboolean zMapConfigIniReadAll(ZMapConfigIni config);
gboolean zMapConfigIniReadUser(ZMapConfigIni config);
void zMapConfigIniGetStanza(ZMapConfigIni config, char *stanza_name);
void zMapConfigIniGetAllStanzas(ZMapConfigIni config);
void zMapConfigIniGetStanzaValues(ZMapConfigIni, char *stanza_name);
gboolean zMapConfigIniGetUserValue(ZMapConfigIni config,
				   char * stanza_name,
				   char * key_name,
				   GValue **value_out,
				   GType type);
gboolean zMapConfigIniGetValue(ZMapConfigIni config,
			       char * stanza_name,
			       char * key_name,
			       GValue **value_out,
			       GType type);
void zMapConfigIniSetValue(ZMapConfigIni config, 
			   char *stanza_name, 
			   char *key_name, 
			   GValue *value);
gboolean zMapConfigIniSaveUser(ZMapConfigIni config);
void zMapConfigIniDestroy(ZMapConfigIni config, gboolean save_user);


ZMapConfigIniContext zMapConfigIniContextCreate(void);
gboolean zMapConfigIniContextIncludeBuffer(ZMapConfigIniContext context, char *buffer);
gboolean zMapConfigIniContextIncludeFile(ZMapConfigIniContext context, char *file) ;
gboolean zMapConfigIniContextAddGroup(ZMapConfigIniContext context, 
				      char *stanza_name, char *stanza_type,
				      ZMapConfigIniContextKeyEntryStruct *keys);
gboolean zMapConfigIniContextGetValue(ZMapConfigIniContext context, 
				      char *stanza_name,
				      char *stanza_type,
				      char *key_name,
				      GValue **value_out);

gboolean zMapConfigIniContextGetString(ZMapConfigIniContext context,
				       char *stanza_name,
				       char *stanza_type,
				       char *key_name,
				       char **value);
gboolean zMapConfigIniContextGetBoolean(ZMapConfigIniContext context,
					char *stanza_name,
					char *stanza_type,
					char *key_name,
					gboolean  *value);
gboolean zMapConfigIniContextGetInt(ZMapConfigIniContext context,
				    char *stanza_name,
				    char *stanza_type,
				    char *key_name,
				    int  *value);
gboolean zMapConfigIniContextSetValue(ZMapConfigIniContext context,
				      char *stanza_name,
				      char *key_name,
				      GValue *value);
gboolean zMapConfigIniContextSetString(ZMapConfigIniContext context,
				       char *stanza_name,
				       char *stanza_type,
				       char *key_name,
				       char *value_str);
gboolean zMapConfigIniContextSetInt(ZMapConfigIniContext context,
				    char *stanza_name,
				    char *stanza_type,
				    char *key_name,
				    int   value_int);
gboolean zMapConfigIniContextSetBoolean(ZMapConfigIniContext context,
					char *stanza_name,
					char *stanza_type,
					char *key_name,
					gboolean value_bool);

gboolean zMapConfigIniContextSave(ZMapConfigIniContext context);

char *zMapConfigIniContextErrorMessage(ZMapConfigIniContext context);
ZMapConfigIniContext zMapConfigIniContextDestroy(ZMapConfigIniContext context);


GList *zMapConfigIniContextGetReferencedStanzas(ZMapConfigIniContext context,
						ZMapConfigIniUserDataCreateFunc object_create_func,
						char *parent_name, 
						char *parent_type, 
						char *parent_key,
						char *child_type);

GList *zMapConfigIniContextGetListedStanzas(ZMapConfigIniContext context,
                                    ZMapConfigIniUserDataCreateFunc object_create_func,
                                    char *styles_list,char * child_type);

#endif /* ZMAP_CONFIG_INI_H */
