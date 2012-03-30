/*  File: zmapUtilsXRemote.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#ifndef ZMAPUTILS_XREMOTE_H
#define ZMAPUTILS_XREMOTE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ZMap/zmapXRemote.h>
#include <ZMap/zmapXML.h>

typedef struct {
  unsigned long xid;
  GQuark request;
  GQuark response;
} ClientParametersStruct, *ClientParameters;

typedef struct
{
  int action;
  ClientParametersStruct client_params;
}CommonDataStruct;

typedef struct
{
  gpointer user_data;
  CommonDataStruct common;
}ZMapXRemoteParseCommandDataStruct, *ZMapXRemoteParseCommandData;

void zMapXRemoteInitialiseWidget(GtkWidget *widget, char *app, char *request, char *response,
                                 ZMapXRemoteCallback callback, gpointer user_data);
void zMapXRemoteInitialiseWidgetFull(GtkWidget *widget, char *app, char *request, char *response, 
				     ZMapXRemoteCallback callback, 
				     ZMapXRemoteCallback post_callback, 
				     gpointer user_data);
gboolean zMapXRemoteValidateStatusCode(ZMapXRemoteStatus *code);

unsigned long zMapXRemoteWidgetGetXID(GtkWidget *widget);
char *zMapXRemoteClientAcceptsActionsXML(unsigned long xwid, char **actions, int action_count);

/* user_data _MUST_ == ZMapXRemoteParseCommandData */
gboolean zMapXRemoteXMLGenericClientStartCB(gpointer user_data, 
                                            ZMapXMLElement client_element,
                                            ZMapXMLParser parser);


#endif /* ZMAPUTILS_XREMOTE_H */
