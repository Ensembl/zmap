/*  File: zmapControlRemote_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Mar  1 11:29 2007 (rds)
 * Created: Thu Feb  1 00:29:43 2007 (rds)
 * CVS info:   $Id: zmapControlRemote_P.h,v 1.3 2007-03-02 14:29:29 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_CONTROL_REMOTE_P_H
#define ZMAP_CONTROL_REMOTE_P_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapXML.h>
#include <zmapControl_P.h>

/* For switch statements */
typedef enum {
  ZMAP_CONTROL_ACTION_ZOOM_IN = 1,
  ZMAP_CONTROL_ACTION_ZOOM_OUT,
  ZMAP_CONTROL_ACTION_ZOOM_TO,
  ZMAP_CONTROL_ACTION_CREATE_FEATURE,
  ZMAP_CONTROL_ACTION_ALTER_FEATURE,
  ZMAP_CONTROL_ACTION_DELETE_FEATURE,
  ZMAP_CONTROL_ACTION_FIND_FEATURE,  
  ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE,
  ZMAP_CONTROL_ACTION_HIGHLIGHT2_FEATURE,
  ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE,
  ZMAP_CONTROL_ACTION_REGISTER_CLIENT,

  ZMAP_CONTROL_ACTION_NEW_VIEW,
  ZMAP_CONTROL_ACTION_INSERT_VIEW_DATA,

  ZMAP_CONTROL_ACTION_ADD_DATASOURCE,

  ZMAP_CONTROL_ACTION_UNKNOWN
} ZMapControlRemoteAction;

typedef struct
{
  char *xml_action_string;
  ZMapControlRemoteAction action;
  GQuark string_as_quark;
} ActionValidatorStruct, *ActionValidator;

typedef struct {
  unsigned long xid;
  GQuark request;
  GQuark response;
} controlClientObjStruct, *controlClientObj;

typedef struct
{
  GQuark sequence;
  gint   start, end;
  char *config;
}ViewConnectDataStruct, *ViewConnectData;

typedef struct {
  ZMapControlRemoteAction action;
  ZMapWindow window;

  ZMapFeatureContext context;   /* Context to do something with. i.e. draw or delete. */
  ZMapFeatureAlignment align;
  ZMapFeatureBlock     block;
  ZMapFeatureSet feature_set;
  ZMapFeature        feature;

  GList *feature_list;

  controlClientObj  client;

  GData *styles;                /* These should be ZMapFeatureTypeStyle */
  GList *locations;             /* This is just a list of ZMapSpan Structs */

  ViewConnectDataStruct new_view;
} XMLDataStruct, *XMLData;

typedef struct
{
  ZMap zmap;
  int  code;
  gboolean handled;
  GString *messages;
} ResponseCodeZMapStruct, *ResponseCodeZMap;


ZMapXMLParser zmapControlRemoteXMLInitialise(void *data);



#endif /* ZMAP_CONTROL_REMOTE_P_H */



